#include "phoned/modem_audio.hpp"
#define BOOST_ASIO_DISABLE_CONCEPTS
#include <boost/asio/io_context.hpp>
#include <boost/asio/serial_port.hpp>

#include <pulse/pulseaudio.h>

#include <thread>
#include <iostream>
#include <syslog.h>

using namespace phoned;

struct modem_audio::Data {
    // Boost Asio stash
    boost::asio::io_context io_ctx;
    boost::asio::serial_port serial_port;
    std::thread io_thd;
    std::array<uint8_t, 255> recv_buf;

    std::string serial_device;
    int baudrate;

    // PulseAudio/Lennart stash
    pa_threaded_mainloop *mainloop;
    pa_mainloop_api *mainloop_api;
    pa_context *context;
    pa_stream *playback_stream;
    pa_stream *record_stream;
    bool pa_quit = false;
    bool pa_ready = false;
    std::thread pa_thread;

    Data()
        : serial_port(io_ctx) {
        io_thd = std::thread([&](){
            // Work guard, so that run() does not quit before doing first task
            boost::asio::io_context::work work(io_ctx);
            io_ctx.run();
        });

        init_pulse_audio();

        /*
        pa_thread = std::thread([&]() {
            int err;
            while(!pa_quit) {
                if ((err = pa_mainloop_iterate(mainloop, 1, nullptr)) < 0) {
                    break;
                }
            }
        });
        */
    }

    ~Data() {
        if (io_thd.joinable()) {
            io_ctx.stop();
            io_thd.join();
        }

        /*
        if (pa_thread.joinable()) {
            pa_quit = true;
            pa_thread.join();
        }
        */
        pa_stream_disconnect(playback_stream);
        pa_stream_disconnect(record_stream);

        pa_stream_unref(playback_stream);
        pa_stream_unref(record_stream);
    }

    void init_pulse_audio() {
        mainloop = pa_threaded_mainloop_new();
        if (!mainloop) {
            throw std::runtime_error("Failed to create threaded pulseaudio mainloop");
        }
        mainloop_api = pa_threaded_mainloop_get_api(mainloop);
        context = pa_context_new(mainloop_api, "pcm-playback");
        if (!context) {
            throw std::runtime_error("Failed to create pulse audio context");
        }
        if (int r; (r = pa_context_connect(context, nullptr, {}, nullptr) < 0)) {
            throw std::runtime_error("Failed to connect pulseaudio context");
        }
        pa_context_set_state_callback(context, pa_state_cb, &pa_ready);
        pa_threaded_mainloop_start(mainloop);

        while(!pa_ready) {
            std::cout << "waiting for pa ready" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        std::cout << "pa ready" << std::endl;

        // Create playback and record stream
        pa_sample_spec ss;
        ss.format = PA_SAMPLE_S16LE;
        ss.channels = 1;
        ss.rate = 8000;

        playback_stream = pa_stream_new(context, "Playback", &ss, nullptr);
        if (!playback_stream) {
            throw std::runtime_error("Failed to create playback stream");
        }
        record_stream = pa_stream_new(context, "Record", &ss, nullptr);
        if (!record_stream) {
            throw std::runtime_error("Failed to create record stream");
        }

        pa_buffer_attr bufattr;
        bufattr.fragsize = pa_usec_to_bytes(20000, &ss);
        bufattr.maxlength = (uint32_t) - 1;
        bufattr.minreq = (uint32_t) - 1;
        bufattr.prebuf = (uint32_t) - 1;
        bufattr.tlength = pa_usec_to_bytes(20000, &ss);

        int r;
        r = pa_stream_connect_playback(playback_stream, nullptr, &bufattr,
                                       static_cast<pa_stream_flags_t>(PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_ADJUST_LATENCY | PA_STREAM_AUTO_TIMING_UPDATE | PA_STREAM_START_CORKED), nullptr, nullptr);
        if (r < 0) {
            throw std::runtime_error("failed to connect playback stream");
        }
        r = pa_stream_connect_record(record_stream, nullptr, &bufattr,
                                     static_cast<pa_stream_flags_t>(PA_STREAM_ADJUST_LATENCY | PA_STREAM_START_CORKED));

    }

    static void pa_state_cb(pa_context *c, void *userdata) {
        pa_context_state_t state = pa_context_get_state(c);
        bool *ready = static_cast<bool*>(userdata);
        switch (state) {
        case PA_CONTEXT_READY:
            *ready = true;
            break;
        default:
            *ready = false;
            break;
        }
    }

    static void playback_want_data(pa_stream *stream, size_t length, void *userdata) {
        Data* data = reinterpret_cast<Data*>(userdata);
    }

    static void record_has_data(pa_stream *stream, size_t length, void *userdata) {
        Data* data = reinterpret_cast<Data*>(userdata);
    }

    bool open_serial_port(){
        try {
            std::cout << "Opening serial port" << std::endl;
            serial_port.open(serial_device);
            serial_port.set_option(boost::asio::serial_port_base::baud_rate(baudrate));
            start_receive();
            std::cout << "Port opened" << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "Failed to open serial port: " << e.what() << std::endl;
            return false;
        }
        return true;
    }

    void disconnect() {
        serial_port.close();
    }

    void start_receive() {
        serial_port.async_read_some(boost::asio::buffer(recv_buf), [&](const boost::system::error_code &err, std::size_t bytes_received) {
            if (err) {
                syslog(LOG_ERR, "ModemAudio: failed to read from port: %s", err.message().c_str());
                std::cerr << "failed to read from port: " << err.message() << std::endl;
                // Disconnect port
                return;
            }

            std::cout << "received: " << bytes_received << std::endl;

            // Write data to audio device
            start_receive();
        });
    }

    ssize_t write_data(const std::vector<uint8_t> &data) {
        return serial_port.write_some(boost::asio::buffer(data));
    }
};


modem_audio::modem_audio(const std::string &serial_device, int baudrate)
    : m_data(new Data()) {
    m_data->serial_device = serial_device;
    m_data->baudrate = baudrate;
}

modem_audio::~modem_audio() = default;

void modem_audio::start_voice_transfer() {
    if (m_data->serial_port.is_open() == false) {
        m_data->open_serial_port();
    }
}

void modem_audio::stop_voice_transfer() {
    m_data->disconnect();
}

void modem_audio::start_transfer() {
    start_voice_transfer();
}

void modem_audio::stop_transfer() {
    stop_voice_transfer();
}
