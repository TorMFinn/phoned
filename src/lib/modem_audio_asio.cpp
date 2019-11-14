#include "phoned/modem_audio.hpp"
#include <boost/asio.hpp>
#include <SDL2/SDL.h>
#include <thread>
#include <iostream>

using namespace phoned;

struct modem_audio::Data {
    Data(const std::string &device, int baud) 
    : serial_port(io_ctx) {
        serial_device = device;
        baudrate = baud;

        init_sdl();

        io_thd = std::thread([this]() {
            boost::asio::executor_work_guard<boost::asio::io_context::executor_type> guard = boost::asio::make_work_guard(io_ctx);
            io_ctx.run();
        });
    }

    ~Data() {
        io_ctx.stop();
        if(io_thd.joinable()) {
            io_thd.join();
        }
    }

    void close_serial() {
        serial_port.close();
    }

    void open_serial() {
        try {
            serial_port.open(serial_device);
            serial_port.set_option(boost::asio::serial_port_base::baud_rate(baudrate));
        } catch (const boost::system::system_error &err) {
            std::cerr << "failed to open serial port: " << err.what() << std::endl;
        }

        start_receive();
    }
    
    void init_sdl() {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            throw std::runtime_error(std::string("failed to initialize sdl audio subsystem: ") + SDL_GetError());
        }
        SDL_AudioSpec playback_want, playback_have;
        SDL_AudioSpec record_want, record_have;
        SDL_memset(&playback_want, 0, sizeof(playback_want));
        SDL_memset(&record_want, 0, sizeof(record_want));
        playback_want.freq = 8000;
        playback_want.format = AUDIO_S16LSB;
        playback_want.channels = 1;
        playback_want.samples = 128;
        playback_want.callback = nullptr;

        record_want.freq = 8000;
        record_want.format = AUDIO_S16LSB;
        record_want.channels = 1;
        record_want.samples = 128;
        record_want.callback = sdl_audio_handler;
        record_want.userdata = this;

        playback_dev = SDL_OpenAudioDevice(nullptr, 0, &playback_want, &playback_have, 0);
        if (playback_dev == 0) {
            throw std::runtime_error(std::string("failed to create playback device: ") + SDL_GetError());
        }
        std::cout << "opened playback device " << std::endl;

        // Verify format maybe

        record_dev = SDL_OpenAudioDevice(nullptr, 1, &record_want, &record_have, 0);
        if (record_dev == 0) {
            throw std::runtime_error(std::string("failed to create recording device: ") + SDL_GetError());
        }

        std::cout << "opened record device " << std::endl;
    }

    static void sdl_audio_handler(void *userdata, Uint8* stream, int len) {
        std::cout << "got recorded audio " << std::endl;
        modem_audio::Data *data = reinterpret_cast<modem_audio::Data*>(userdata);
        if (data->serial_port.is_open()) {
            boost::asio::async_write(data->serial_port, boost::asio::buffer(stream, len), [](const boost::system::error_code &error, std::size_t bytes_written){
                if (error) {
                    std::cerr << "failed to write bytes: " << error.message() << std::endl;
                }
                std::cout << "bytes written " << std::endl;
            });
        }
    }

    void start_receive() {
        boost::asio::async_read(serial_port, boost::asio::buffer(recv_buf, 128), boost::asio::transfer_exactly(128), [this](const boost::system::error_code &error, std::size_t bytes_read) {
            // Write to SDL audio port
            std::cout << "read from device: " << bytes_read << std::endl;
            if (!error) {
                SDL_QueueAudio(playback_dev, recv_buf, bytes_read);
                start_receive();
            }
        });
    }

    uint8_t recv_buf[128];
    boost::asio::io_context io_ctx;
    boost::asio::serial_port serial_port;
    std::thread io_thd;

    std::string serial_device;
    int baudrate;

    SDL_AudioSpec want, have;
    SDL_AudioDeviceID playback_dev;
    SDL_AudioDeviceID record_dev;
};

modem_audio::modem_audio(const std::string &serial_device, int baudrate) 
: m_data(new Data(serial_device, baudrate)) {
}

modem_audio::~modem_audio() {
}

void modem_audio::start_transfer() {
    m_data->open_serial();
    SDL_PauseAudioDevice(m_data->playback_dev, 0);
    SDL_PauseAudioDevice(m_data->record_dev, 0);
}

void modem_audio::stop_transfer() {
    m_data->close_serial();
    SDL_PauseAudioDevice(m_data->playback_dev, 1);
    SDL_PauseAudioDevice(m_data->record_dev, 1);
}