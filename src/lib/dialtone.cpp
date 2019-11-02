
#include "phoned/dialtone.hpp"
#include <SDL2/SDL.h>
#include <cmath>
#include <iostream>

using namespace phoned;

const float m_2pi = 6.28318530;

struct dialtone::Data {
    SDL_AudioDeviceID audio_dev = 0;
    SDL_AudioSpec want, have;
    float time = 0;

    Data() {
    }

    ~Data() {
        if(audio_dev != 0) {
            SDL_CloseAudioDevice(audio_dev);
        }
        SDL_Quit();
    }

    static void audio_callback(void *userdata, Uint8 *stream, int len) {
        dialtone::Data *data = static_cast<dialtone::Data*>(userdata);
        int buflen = len/2; // 16 bit data
        uint16_t *buf = reinterpret_cast<uint16_t*>(stream);
        for (int i = 0; i < buflen; i++) {
            buf[i] = 32000 * sin(data->time);
            data->time += (m_2pi * 425.0) / 44100.0;
            if (data->time > m_2pi) {
                data->time = -m_2pi;
            }
        }
    }

    std::tuple<bool, std::string> init_audio() {
        if(SDL_Init(SDL_INIT_AUDIO) < 0) {
            return { false, "failed to init sdl audio" };
        }
 
        memset(&want, 0, sizeof(want));
        want.channels = 1;
        want.freq  = 44100;
        want.format = AUDIO_S16;
        want.samples = 4096;
        want.userdata = this;
        want.callback = audio_callback;

        audio_dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
        if (audio_dev == 0) {
            return { false, "failed to open audio device" };
        }

        /*
        if (want.format != have.format) {
            return { false, "failed to get requested audio format" };
        }
        */

        return {true, ""};
    }

};

dialtone::dialtone()
    : m_data(new Data()) {
    auto [ok, msg] = m_data->init_audio();
    if (!ok) {
        throw std::runtime_error(msg);
    }
}

dialtone::~dialtone() {
}

void dialtone::start() {
  if (m_data->audio_dev != 0) {
      SDL_PauseAudioDevice(m_data->audio_dev, 0);
  }
}

void dialtone::stop() {
  if (m_data->audio_dev != 0) {
      SDL_PauseAudioDevice(m_data->audio_dev, 1);
  }
}
