#include "phoned/dialtone.hpp"
#include <SDL2/SDL.h>
#include <cmath>
#include <iostream>

using namespace phoned;

const int sample_rate = 8000;

struct Dialtone::Data {
    ~Data() {
        SDL_CloseAudioDevice(dev);
    }

    bool InitAudio() {
        SDL_InitSubSystem(SDL_INIT_AUDIO);
        want = SDL_AudioSpec{};
        want.freq = sample_rate;
        want.format = AUDIO_S16LSB;
        want.channels = 1;
        want.samples = 4096;
        want.callback = audio_callback;
        want.userdata = this;

        dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
        if (dev == 0) {
            std::cerr << "failed to open audio device: " << SDL_GetError() << std::endl;
            return false;
        }

        return true;
    }

    static void audio_callback(void *userdata, Uint8 *stream, int len) {
        Data *data = reinterpret_cast<Data *>(userdata);
        Uint16 *stream_data = reinterpret_cast<Uint16 *>(stream);
        for (int i = 0; i < len / 2; i++) {
            stream_data[i] = 15000 * sin(data->t * data->frequency);
            data->t += data->step;
            if (data->t >= 2 * M_PI) {
                data->t -= 2 * M_PI;
            }
        }
    }

    SDL_AudioDeviceID dev;
    SDL_AudioSpec want, have;

  private:
    float step = (M_PI * 2) / sample_rate;
    float t = 0.0;
    int frequency = 420; // This is the dialtone in Europe/Norway.
};

Dialtone::Dialtone() : m_data(new Data()) {
    if (!m_data->InitAudio()) {
        throw std::runtime_error("failed to init dialtone device");
    }
}

Dialtone::~Dialtone() = default;

void Dialtone::Start() {
    // Start playback by unpausing.
    SDL_PauseAudioDevice(m_data->dev, 0);
}

void Dialtone::Stop() {
    // Stop playback by pausing.
    SDL_PauseAudioDevice(m_data->dev, 1);
}
