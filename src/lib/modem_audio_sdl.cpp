#include "phoned/modem_audio.hpp"
#include <SDL2/SDL.h>
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

struct modem_audio::Data {
    Data(const std::string &device, int baud) {
        serial_device = device;
        baudrate = baud;
        init_sdl_audio();
    }

    ~Data() {
        close_serial();
    }

    void init_sdl_audio() {
        if(SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            std::cerr << "failed to initialize sdl audio subsystem" << std::endl;
        }
        SDL_AudioSpec want_playback, have_playback;
        SDL_AudioSpec want_record, have_record;
        want_playback.callback = nullptr;
        want_playback.channels = 1;
        want_playback.format = AUDIO_S16LSB;
        want_playback.freq = 8000;
        want_playback.samples = 512;
        want_playback.userdata = nullptr;

        want_record.callback = audio_recording_handler;
        want_record.userdata = this;
        want_record.channels = 1;
        want_record.format = AUDIO_S16LSB;
        want_record.freq = 8000;
        want_record.samples = 128;

        playback_device = SDL_OpenAudioDevice(nullptr, 0, &want_playback, &have_playback, 0);
        if (playback_device == 0) {
            std::runtime_error(std::string("Failed to create SDL audio device: ") + SDL_GetError());
        }

        record_device = SDL_OpenAudioDevice(nullptr, 1, &want_record, &have_record, 0);
        if (record_device == 0) {
            std::runtime_error(std::string("Failed to create SDL record device: ") + SDL_GetError());
        }
        
        SDL_PauseAudioDevice(record_device, 0);
        SDL_PauseAudioDevice(playback_device, 0);
    }

    static void audio_recording_handler(void *userdata, uint8_t *buffer, int len) {
        modem_audio::Data *data = reinterpret_cast<modem_audio::Data*>(userdata);
        if (data) {
            if (data->output_possible) {
                ssize_t bytes_written = write(data->serial_fd, buffer, len);
                std::cout << "bytes written: " << bytes_written << std::endl;
                data->output_possible = false;
            }
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
                        uint8_t buf[1024];
                        ssize_t bytes_read;
                        bytes_read = read(serial_fd, buf, 1024);
                        if (bytes_read == 0) {
                            // We are done with this transfer
                            /*
                            std::cout << "No more data to read" << std::endl;
                            break;
                            */
                           continue;
                        }
                        if (bytes_read > 0) {
                            data_was_received = true;
                            SDL_QueueAudio(playback_device, buf, bytes_read);
                        }
                        std::cout << "read: " << bytes_read << std::endl;
                    } 
                    if (fds[0].revents & POLLOUT) {
                        if (data_was_received && start_voice_transfer) {
                            // Do not send before we have actually received something from the modem
                            // This signals that it is infact ready
                            output_possible = true;
                        }
                    }
                } else if (r < 0) {
                    std::cerr << "read error: " << std::strerror(errno) << std::endl;
                }
            }
            output_possible = false;
            data_was_received = false;
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
    bool data_was_received = false;
    bool transfer_started = false;
    bool quit;
    std::atomic_bool output_possible = false;
    int serial_fd;
    int baudrate;
    std::string serial_device;
    std::thread read_thd;
    SDL_AudioDeviceID playback_device;
    SDL_AudioDeviceID record_device;
};

modem_audio::modem_audio(const std::string &device, int baudrate) 
: m_data(new Data(device, baudrate)) {

}

modem_audio::~modem_audio() = default;

void modem_audio::start_voice_transfer() {
    m_data->start_voice_transfer = true;
}

void modem_audio::stop_voice_transfer() {
    m_data->start_voice_transfer = false;
}

void modem_audio::start_transfer() {
    m_data->start_serial();
    std::cout << "Starting transfer " << std::endl;
}

void modem_audio::stop_transfer() {
    m_data->close_serial();
    std::cout << "Stopping transfer " << std::endl;
}