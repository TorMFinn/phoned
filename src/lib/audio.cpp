#include "phoned/audio.hpp"
#include <cstring>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <poll.h>
#include <thread>

#include <SDL2/SDL.h>

using namespace phoned;

const int buf_size_read = 312;

struct audio::Data {
    // Thread control variables
    bool quit = false;
    std::thread read_thd;

    // Serial variables
    char input_buffer[buf_size_read];
    int serial_fd = -1;

    // SDL Audio variables
    SDL_AudioDeviceID playback;

    void init_sdl_audio() {
        if (SDL_Init(SDL_INIT_AUDIO) < 0 ) {
            throw std::runtime_error("failed to initialize sdl audio");
        }

        SDL_AudioSpec want, have;
        SDL_memset(&want, 0, sizeof(want));
        want.freq = 8000;
        want.channels = 1;
        want.format = AUDIO_S16LSB;
        want.samples = 512;
        want.callback = nullptr;

        playback = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    }
    

    bool open_serial_port(const std::string &device, int baudrate) {
        serial_fd = open(device.c_str(), O_RDWR);
        if (serial_fd < 0) {
            std::cerr << "failed to open serial device: " << std::strerror(errno)
                      << std::endl;
            return false;
        }

        // Set Raw terminal options
        struct termios serial_settings = {};
        cfmakeraw(&serial_settings);
        cfsetspeed(&serial_settings, baudrate);

        if (tcsetattr(serial_fd, TCSANOW, &serial_settings) < 0) {
            std::cerr << "failed to set serial port settings: "
                      << std::strerror(errno) << std::endl;
            return false;
        }

        return true;
    }

    void close_port() {
        close(serial_fd);
    }

    void start_read() {
        init_sdl_audio();
        struct pollfd pfd;
        pfd.fd = serial_fd;
        pfd.events = POLLIN | POLLOUT;

        quit = false;
        while (!quit) {
            int p = poll(&pfd, 1, 100);
            if (p > 0 && pfd.revents & POLLIN) {
                // We have data to read
                int bytes_read = read(serial_fd, input_buffer, buf_size_read);
                if (bytes_read > 0) {
                    std::cout << "received : " << bytes_read << " data" << std::endl;
                    if(SDL_QueueAudio(playback, input_buffer, buf_size_read) == 0) {
                        std::cout << "Queued audio" << std::endl;
                    }
                }
            }
            else if (p > 0 && pfd.revents & POLLOUT) {
                // Modem is ready to receive data
                //std::cout << "ready to receive data" << std::endl;
            }
        }
    }

    void stop_read() {
        if (read_thd.joinable()) {
            quit = true;
            read_thd.join();
        }
    }
};

audio::audio()
    : m_data(new Data()) {}

audio::~audio() = default;

bool audio::start_transfer(const std::string device_name, int baudrate) {
    if(m_data->open_serial_port(device_name, baudrate)) {
        m_data->start_read();
        return true;
    }
    return false;
}

void audio::stop_transfer() {
    m_data->close_port();
    m_data->stop_read();
}

