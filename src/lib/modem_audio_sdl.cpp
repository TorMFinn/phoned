#include "phoned/modem_audio.hpp"
#define BOOST_ASIO_DISABLE_CONCEPTS
#include <boost/asio/io_context.hpp>
#include <boost/asio/serial_port.hpp>

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

    // SDL audio

    // We need to read all the time, else
    // We will buffer up a lot of data.
    std::thread pa_record_thd;
    bool should_read = true;
    uint8_t pa_record_buf[255];

    Data()
        : serial_port(io_ctx) {
        io_thd = std::thread([&](){
            // Work guard, so that run() does not quit before doing first task
            boost::asio::io_context::work work(io_ctx);
            io_ctx.run();
        });

        init_pulse_audio();
    }

    ~Data() {
        if (io_thd.joinable()) {
            io_ctx.stop();
            io_thd.join();
        }

        should_read = false;
        pa_record_thd.join();

        pa_simple_free(playback);
        pa_simple_free(record);
    }

    void init_pulse_audio() {
        // Create playback and record stream
        pa_sample_spec ss;
        ss.format = PA_SAMPLE_S16LE;
        ss.channels = 1;
        ss.rate = 8000;

        playback = pa_simple_new(nullptr,
                                 "phoned-voice-rcv",
                                 PA_STREAM_PLAYBACK,
                                 nullptr,
                                 "voice rx",
                                 &ss,
                                 nullptr,
                                 nullptr,
                                 nullptr);
        if (!playback) {
            throw std::runtime_error("Failed to create playback device");
        }

        record = pa_simple_new(nullptr,
                               "phoned-voice-tx",
                               PA_STREAM_RECORD,
                               nullptr,
                               "voice tx",
                               &ss,
                               nullptr,
                               nullptr,
                               nullptr);

        if (!record) {
            throw std::runtime_error("Failed to create record device");
        }

        pa_record_thd = std::thread([&]() {
            int error;
            while(should_read) {
                int bytes_read = pa_simple_read(record, pa_record_buf, sizeof(pa_record_buf), &error);
                if (bytes_read < 0) {
                    std::cerr << "failed to read data from record stream: "
                              <<  pa_strerror(error) << std::endl;
                    continue;
                } else if (bytes_read > 0) {
                    std::cout << "read " << bytes_read << " from record stream" << std::endl;
                    if (serial_port.is_open()) {
                        serial_port.write_some(boost::asio::buffer(pa_record_buf, bytes_read));
                    }
                }
            }
        });
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
            int error;
            int amount_written = pa_simple_write(playback, recv_buf.data(), bytes_received, &error);
            if (amount_written < 0) {
                std::cerr << "failed to read write data to port: " << pa_strerror(error) << std::endl;
            }
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
