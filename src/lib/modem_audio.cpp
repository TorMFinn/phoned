#include "phoned/modem_audio.hpp"
#include <pulse/simple.h>
#include <pulse/error.h>
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

using namespace phoned;

const int audio_bufsize = 64;

struct modem_audio::Data {
    Data(const std::string &device, int baud) {
        serial_device = device;
        baudrate = baud;
        init_pulse_audio();
    }

    ~Data() {
        close_serial();
    }

    void init_pulse_audio() {
        sample_spec.channels = 1;
        sample_spec.format = PA_SAMPLE_S16LE;
        sample_spec.rate = 8000;

        pulse_play = pa_simple_new(nullptr,
                                   "phoned_audiod",
                                   PA_STREAM_PLAYBACK,
                                   nullptr,
                                   "modem_playback",
                                   &sample_spec,
                                   nullptr,
                                   nullptr,
                                   nullptr);

        pulse_rec = pa_simple_new(nullptr,
                                  "phoned_audiod",
                                  PA_STREAM_RECORD,
                                  nullptr,
                                  "modem_record",
                                  &sample_spec,
                                  nullptr,
                                  nullptr,
                                  nullptr);

        if (!pulse_rec && !pulse_play) {
            throw std::runtime_error("failed to create pulse audio devices");
        }
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
    }

    void start_serial() {
        if (transfer_started) {
            // Serial is allready started
            return;
        }
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
                        // We have data
                        uint8_t audio_recv_buf[audio_bufsize];
                        ssize_t bytes_read;

                        bytes_read = read(serial_fd, audio_recv_buf, audio_bufsize);
                        if (bytes_read == 0) {
                            // We are done with this transfer
                            /*
                            std::cout << "No more data to read" << std::endl;
                            break;
                            */
                           continue;
                        }
                        if (bytes_read > 0) {
                            pa_simple_write(pulse_play, audio_recv_buf, bytes_read, nullptr);
                        }
                        std::cout << "read: " << bytes_read << std::endl;
                    } 
                    if (fds[0].revents & POLLOUT) {
                        if (start_voice_transfer) {
                            uint8_t buf[1024];
                            int error;
                            if (pa_simple_read(pulse_rec, buf, 1024, &error) < 0) {
                                std::cerr << "failed to record" << std::endl;
                            } else {
                                int bytes_written = write(serial_fd, buf, 1024);
                                std::cout << "wrote: " << bytes_written << std::endl;
                            }
                        }
                    }
                } else if (r < 0) {
                    std::cerr << "read error: " << std::strerror(errno) << std::endl;
                }
            }
            transfer_started = false;
        });
    }

    void open_serial() {
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
    }

    bool start_voice_transfer = false;
    bool transfer_started = false;
    bool quit;
    int serial_fd;
    int baudrate;
    std::string serial_device;
    std::thread read_thd;

    pa_simple *pulse_play;
    pa_simple *pulse_rec;
    pa_sample_spec sample_spec;
};

modem_audio::modem_audio(const std::string &device, int baudrate) 
: m_data(new Data(device, baudrate)) {
    m_data->start_serial();
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
    //m_data->start_serial();
    std::cout << "Starting transfer " << std::endl;
}

void modem_audio::stop_transfer() {
    //m_data->close_serial();
    std::cout << "Stopping transfer " << std::endl;
}
