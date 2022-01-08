#include "alsa/asoundlib.h"
#include "phoned/dialtone.hpp"
#include <iostream>

namespace phoned
{
struct Dialtone::Data
{
    bool InitSound() {
        int err;
        err = snd_pcm_open(&sound_device_, "plughw:0,0", SND_PCM_STREAM_PLAYBACK, 0);
        if (err < 0) {
            std::cerr << "failed to open sound device"
                      << " " << snd_strerror(err) << std::endl;
            return false;
        }

        snd_pcm_hw_params_alloca(&hw_params_);
        snd_pcm_hw_params_any(sound_device_, hw_params_);

        // Set formats
        err = snd_pcm_hw_params_set_access(sound_device_, hw_params_, SND_PCM_ACCESS_RW_INTERLEAVED);
        if (err < 0) {
            std::cerr << "cant set interleaved mode. " << snd_strerror(err) << std::endl;
            return false;
        }

        err = snd_pcm_hw_params_set_format(sound_device_, hw_params_, SND_PCM_FORMAT_S16_LE);
        if (err < 0) {
            std::cerr << "failed to set format: " << snd_strerror(err) << std::endl;
            return false;
        }

        err = snd_pcm_hw_params_set_channels(sound_device_, hw_params_, 1);
        if (err < 0) {
            std::cerr << "failed to set channels: " << snd_strerror(err) << std::endl;
            return false;
        }

	int rate = 8000;
	err = snd_pcm_hw_params_set_rate_near(sound_device_, hw_params_, &rate, nullptr);

        return true;
    }

    snd_pcm_t *sound_device_;
    snd_pcm_hw_params_t *hw_params_;
};

Dialtone::Dialtone() : m_data(new Data()) {
}

Dialtone::~Dialtone() {
}

// Start the dialtone signal
void Dialtone::Start() {
}

// Stop the dialtone signal
void Dialtone::Stop() {
}

} // namespace phoned