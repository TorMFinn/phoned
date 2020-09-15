#include "phoned/modem_audio.hpp"
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <thread>
#include <mutex>
#include <cstring>
#include <iostream>
#include <atomic>
#include <queue>

#include <pulse/pulseaudio.h>

using namespace phoned;

const int audio_bufsize = 64;
const int latency = 20000; // 20ms latency

struct modem_audio::Data {
    Data(const std::string &device, int baud) {
        serial_device = device;
        baudrate = baud;
    }

    ~Data() {
        close_serial();
    }

    void close_serial() {
        // Wo don't say quit, because we need read to return 0 first
        std::cout << "Closing serial port" << std::endl;
        quit=true;
        if (read_thd.joinable()) {
            read_thd.join();
        }

        int r = close(serial_fd);
        if (r < 0) {
            std::cerr << "failed to close serial fd: " << std::strerror(errno) << std::endl;
        }
        std::cout << "closed port" << std::endl;
        serial_fd = -1;
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

    static void stream_play_cb(pa_stream* s, size_t length, void *userdata) {
        Data* data = reinterpret_cast<Data*>(userdata);
        std::cout << "play callback" << std::endl;
        if (data->data_read_available) {
            uint8_t buf[data->bufattr.tlength];
            std::size_t bytes_read = read(data->serial_fd, buf, data->bufattr.tlength);
            if (bytes_read > 0) {
                std::cout << "bytes read: " << bytes_read << std::endl;
                pa_stream_write(s, buf, bytes_read, nullptr, 0, PA_SEEK_RELATIVE);
            }
            data->data_read_available = false;
        }
    }

    static void stream_record_cb(pa_stream* s, size_t length, void *userdata) {
        Data* data = reinterpret_cast<Data*>(userdata);
        std::size_t nbytes;
        const void *audio_data;
        if(pa_stream_peek(s, &audio_data, &nbytes) < 0) {
            std::cerr << "failed to record data" << std::endl;
        }
        if(audio_data && nbytes > 0) {
            if (data->data_write_available) {
                int written = write(data->serial_fd, audio_data, nbytes);
                std::cout << "wrote: " << written << std::strerror(errno) << std::endl;
                data->data_write_available = false;
            }
            pa_stream_drop(s);
        }
    }

    static void write_audio_cb(pa_mainloop_api* api, pa_io_event *io_event, int fd, pa_io_event_flags events, void *userdata) {
        Data* data = reinterpret_cast<Data*>(userdata);
        if (!data->pa_record) {
            return;
        }
        if(pa_stream_readable_size(data->pa_record) >= data->bufattr.fragsize) {
            std::size_t nbytes;
            const void *audio_data;
            if(pa_stream_peek(data->pa_record, &audio_data, &nbytes) < 0) {
                std::cerr << "failed to record data" << std::endl;
                return;
            } else if(audio_data && nbytes > 0) {
                int written = write(fd, audio_data, nbytes);
                std::cout << "wrote: " << written << std::strerror(errno) << std::endl;
                pa_stream_drop(data->pa_record);
            }
        }
    }

    static void read_audio_cb(pa_mainloop_api* api, pa_io_event *io_event, int fd, pa_io_event_flags events, void *userdata) {
        Data* data = reinterpret_cast<Data*>(userdata);
        if (!data->pa_playback) {
            return;
        }
        uint8_t buf[data->bufattr.tlength];
        int bytes_read;
        do {
            bytes_read = read(fd, buf, data->bufattr.tlength);
            if (bytes_read > 0) {
                std::cout << "bytes read: " << bytes_read << std::endl;
                pa_stream_write(data->pa_playback, buf, bytes_read, nullptr, 0, PA_SEEK_RELATIVE);
            }
        } while (bytes_read > 0);
    }

    void init_pulseaudio() {
        pa_ml = pa_threaded_mainloop_new();
        if (!pa_ml) {
            std::cerr << "failed to create threaded mainloop for pulseaudio" << std::endl;
        }
        pa_mlapi = pa_threaded_mainloop_get_api(pa_ml);
        if (!pa_mlapi) {
            std::cerr << "failed to get mainloop api" << std::endl;
        }
        pa_ctx = pa_context_new(pa_mlapi, "modem_audio");
        if (!pa_ctx) {
            std::cerr << "failed to create pulseaudio context" << std::endl;
        }

        bool pa_ready = false;
        if(int r; (r = pa_context_connect(pa_ctx, nullptr, {}, nullptr) < 0)) {
            std::cerr << "failed to create pa context" << std::endl;
        }
        pa_context_set_state_callback(pa_ctx, pa_state_cb, &pa_ready);

        std::cout << "creating io events" << std::endl;
        pa_io_event *read_event = pa_mlapi->io_new(pa_mlapi, serial_fd, pa_io_event_flags_t::PA_IO_EVENT_INPUT, read_audio_cb, this);
        pa_io_event *write_event = pa_mlapi->io_new(pa_mlapi, serial_fd, pa_io_event_flags_t::PA_IO_EVENT_OUTPUT, write_audio_cb, this);

        // Start the mainloop
        pa_threaded_mainloop_start(pa_ml);

        while(!pa_ready) {
            std::cout << "waiting for pa ready" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        std::cout << "pa ready" << std::endl;

        ss.channels = 1;
        ss.format = PA_SAMPLE_S16LE;
        ss.rate = 8000;
        pa_playback = pa_stream_new(pa_ctx, "modem_audio_playback", &ss, nullptr);
        if (!pa_playback) {
            std::cerr << "failed to create playback stream" << std::endl;
        }
        pa_record = pa_stream_new(pa_ctx, "modem_audio_record", &ss, nullptr);
        if (!pa_record) {
            std::cerr << "failed to create record stream" << std::endl;
        }

        bufattr.fragsize = pa_usec_to_bytes(latency, &ss);
        bufattr.maxlength = (uint32_t) - 1;
        bufattr.minreq = (uint32_t) - 1;
        bufattr.prebuf = (uint32_t) - 1;
        bufattr.tlength = pa_usec_to_bytes(latency, &ss);

        int r;
        r = pa_stream_connect_playback(pa_playback, nullptr, &bufattr,
                                       static_cast<pa_stream_flags_t>(PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_ADJUST_LATENCY | PA_STREAM_AUTO_TIMING_UPDATE | PA_STREAM_START_CORKED),
                                       nullptr, nullptr);
        if (r < 0) {
            std::cerr << "failed to connect playback stream" << std::endl;
        }
        r = pa_stream_connect_record(pa_record, nullptr, &bufattr,
                                     static_cast<pa_stream_flags_t>(PA_STREAM_ADJUST_LATENCY | PA_STREAM_START_CORKED));
        if (r < 0) {
            std::cerr << "failed to connect record stream" << std::endl;
        }
    }

    void start_serial() {
        open_serial();
        read_thd = std::thread([this]() {
            transfer_started = true;
            struct pollfd fds[1];
            fds[0].fd = serial_fd;
            fds[0].events = POLLIN | POLLOUT;

            quit = false;
            while(!quit) {
                int r = poll(fds, 1, 500);
                if (r > 0) {
                    if (fds[0].revents & POLLIN) {
                        uint8_t buf[bufattr.tlength];
                        int bytes_read;
                        do {
                            bytes_read = read(serial_fd, buf, bufattr.tlength);
                            if (bytes_read > 0) {
                                std::cout << "bytes read: " << bytes_read << std::endl;
                                pa_stream_write(pa_playback, buf, bytes_read, nullptr, 0, PA_SEEK_RELATIVE);
                            }
                        } while (bytes_read > 0);
                        /*
                        std::scoped_lock lock(read_mutex);
                        data_read_available = true;
                        */
                    } 
                    if (fds[0].revents & POLLOUT) {
                        if(pa_stream_readable_size(pa_record) >= bufattr.fragsize) {
                            std::size_t nbytes;
                            const void *audio_data;
                            if(pa_stream_peek(pa_record, &audio_data, &nbytes) < 0) {
                                std::cerr << "failed to record data" << std::endl;
                                continue;
                            } else if(audio_data && nbytes > 0) {
                                int written = write(serial_fd, audio_data, nbytes);
                                std::cout << "wrote: " << written << std::strerror(errno) << std::endl;
                                pa_stream_drop(pa_record);
                            }
                        }
                    }
                } else if (r < 0) {
                    std::cerr << "poll error: " << std::strerror(errno) << std::endl;
                }
            }
        });
    }

    void open_serial() {
        if (serial_fd != -1) {
            // allready open
            return;
        }
        int ret;
        serial_fd = open(serial_device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (serial_fd == -1) {
            throw std::runtime_error(std::string("failed to open serial device: ") + std::strerror(errno));
        }

        struct termios config;
        cfmakeraw(&config);
        ret = cfsetspeed(&config, baudrate);
        if (ret < 0) {
            throw std::runtime_error(std::string("failed to set baudrate: ") + std::strerror(errno));
        }

        ret = tcsetattr(serial_fd, TCSANOW, &config);
        if (ret < 0) {
            throw std::runtime_error(std::string("failed to set termios config: ") + std::strerror(errno));
        }

        ret = tcflush(serial_fd, TCIOFLUSH);
        if (ret < 0) {
            std::cerr << "failed to flush" << std::endl;
        }
    }

    bool data_read_available = false;
    bool data_write_available = false;

    bool start_voice_transfer = false;
    bool transfer_started = false;
    bool quit;
    int serial_fd = -1;
    int baudrate;
    std::string serial_device;
    std::thread read_thd;

    std::mutex read_mutex;
    std::mutex write_mutex;

    pa_context *pa_ctx = nullptr;
    pa_stream* pa_playback = nullptr;
    pa_stream* pa_record = nullptr;

    pa_sample_spec ss;
    pa_buffer_attr bufattr;

    pa_threaded_mainloop* pa_ml;
    pa_mainloop_api* pa_mlapi;
};

modem_audio::modem_audio(const std::string &device, int baudrate) 
: m_data(new Data(device, baudrate)) {
    //m_data->start_serial();
    m_data->open_serial();
    m_data->init_pulseaudio();
}

modem_audio::~modem_audio() {
    m_data->close_serial();
}

void modem_audio::start_voice_transfer() {
    m_data->start_voice_transfer = true;
}

void modem_audio::stop_voice_transfer() {
    m_data->start_voice_transfer = false;
}

void modem_audio::start_transfer() {
    m_data->open_serial();
    int ret = tcflush(m_data->serial_fd, TCIOFLUSH);
    if (ret < 0) {
        std::cerr << "failed to flush" << std::endl;
    }

    if (pa_stream_is_corked(m_data->pa_playback) == 1) {
        pa_stream_cork(m_data->pa_playback, 0, nullptr, nullptr);
    }
    if (pa_stream_is_corked(m_data->pa_record) == 1) {
        pa_stream_cork(m_data->pa_record, 0, nullptr, nullptr);
    }
    std::cout << "Starting transfer " << std::endl;
}

void modem_audio::stop_transfer() {
    int ret = tcflush(m_data->serial_fd, TCIOFLUSH);
    if (ret < 0) {
        std::cerr << "failed to flush" << std::endl;
    }
    if (pa_stream_is_corked(m_data->pa_playback) == 0) {
        //pa_stream_drain(m_data->pa_playback, nullptr, nullptr);
        pa_stream_cork(m_data->pa_playback, 1, nullptr, nullptr);
    }
    if (pa_stream_is_corked(m_data->pa_record) == 0) {
        pa_stream_cork(m_data->pa_record, 1, nullptr, nullptr);
    }
    m_data->close_serial();
    std::cout << "Stopping transfer " << std::endl;
}
