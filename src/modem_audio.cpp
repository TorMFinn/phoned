#include "modem_audio.hpp"
#include <boost/asio.hpp>
#include <thread>
#include <iostream>
#include <SDL2/SDL.h>

using namespace phoned;

struct modem_audio::Data {
    boost::asio::io_context io;
    boost::asio::serial_port port;
    std::thread io_thd;
    uint8_t recv_buf[4096];
    bool writing_audio = false;
    std::size_t play_bufsize;
    std::size_t rec_bufsize;
    SDL_AudioDeviceID play_device, record_device;

    Data() : io(), port(io) {}

    ~Data() {
        if (not io.stopped()) {
            io.stop();
        }
    }

    void handle_received_data(const boost::system::error_code &error, std::size_t bytes_received) {
        //std::cout << "received modem data" << bytes_received << std::endl;
        SDL_QueueAudio(play_device, recv_buf, bytes_received);

        /*
        uint8_t buf[512];
        int bytes_dequed = SDL_DequeueAudio(record_device, buf, 512);
        port.async_write_some(boost::asio::buffer(buf, bytes_dequed),
        std::bind(&Data::write_handler, this, std::placeholders::_1, std::placeholders::_2));
        */
        start_receive();
    }

    void write_handler(const boost::system::error_code, std::size_t bytes_transferred) {
        std::cout << "Audio written" << std::endl;
        writing_audio = false;
    }

    void start_receive() {
        boost::asio::async_read(
            port,
            boost::asio::buffer(recv_buf, play_bufsize),
            std::bind(&Data::handle_received_data, this, std::placeholders::_1, std::placeholders::_2));
    }

    static void audio_record_callback(void *userdata, uint8_t *stream, int len) {
        modem_audio::Data *data = static_cast<modem_audio::Data*>(userdata);
        //std::cout << "audio data received" << std::endl;
        if (not data->writing_audio) {
            data->writing_audio = true;
            data->port.async_write_some(
                boost::asio::buffer(stream, len), 
                std::bind(&Data::write_handler, data, 
                std::placeholders::_1, std::placeholders::_2));
        }
    }

    bool init_sdl_audio() {
        if(SDL_Init(SDL_INIT_AUDIO) < 0) {
            std::cerr << "failed to init sdl audio: " << SDL_GetError() << std::endl;
            return false;
        }
        SDL_AudioSpec play_want, play_have;
        SDL_AudioSpec rec_want, rec_have;

        SDL_memset(&play_want, 0, sizeof(play_want)); /* or SDL_zero(want) */
        play_want.freq = 8000; // Sample rate
        play_want.format = AUDIO_S16LSB;
        play_want.channels = 1;
        play_want.samples = 128;
        play_want.callback = nullptr;  // you wrote this function elsewhere.
        play_device = SDL_OpenAudioDevice(NULL, 0, &play_want, &play_have, 0);
        play_bufsize = play_have.size;

        if (play_device < 0) {
            std::cerr << "failed to open play audio device: " << SDL_GetError() << std::endl;
            return false;
        }

        SDL_memset(&rec_want, 0, sizeof(rec_want)); /* or SDL_zero(want) */
        rec_want.freq = 8000; // Sample rate
        rec_want.format = AUDIO_S16LSB;
        rec_want.channels = 1;
        rec_want.samples = 128;
        rec_want.userdata = this;
        rec_want.callback = audio_record_callback;
        record_device = SDL_OpenAudioDevice(NULL, 1, &rec_want, &rec_have, 0);
        rec_bufsize = rec_have.size;

        if (record_device < 0) {
            std::cerr << "failed to open record audio device: " << SDL_GetError() << std::endl;
            return false;
        }

        return true;
    }
};

modem_audio::modem_audio() 
: m_data(new Data()) {

}

modem_audio::~modem_audio() {
    stop_audio_transfer();
}

bool modem_audio::init_audio(const std::string &port, int baudrate) {
    try {
        m_data->port.open(port);
        m_data->port.set_option(boost::asio::serial_port_base::baud_rate(baudrate));
        if(m_data->init_sdl_audio()) {
            m_data->start_receive();
            m_data->io_thd = std::thread([&](){
                m_data->io.run();
            });
            return true;
        }
    } catch (const boost::system::error_code &err) {
        return false;
    }

    return false;
}

void modem_audio::start_audio_transfer() {
    SDL_PauseAudioDevice(m_data->record_device, 0);
    SDL_PauseAudioDevice(m_data->play_device, 0);
}

void modem_audio::stop_audio_transfer() {
    SDL_PauseAudioDevice(m_data->record_device, 1);
    SDL_PauseAudioDevice(m_data->play_device, 1);
}