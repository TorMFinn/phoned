#include "phoned/audio.hpp"
#include <pulse/simple.h>
#include <pulse/error.h>
#include <thread>
#include <iostream>

using namespace phoned;

struct audio::Data {
    Data() {
        init_pulse_audio();
    }

    ~Data() {
        stop_recording();
    }

    audio::audio_sample_callback sample_callback;

    // Pulseaudio/Lennart stash
    pa_simple *playback;
    pa_simple *record;
    pa_sample_spec ss;
    pa_buffer_attr bufattr;
    std::thread record_thd;

    bool quit;

    void init_pulse_audio() {
        // Create playback and record stream
        ss.format = PA_SAMPLE_S16LE;
        ss.channels = 1;
        ss.rate = 8000;

        bufattr.fragsize = pa_usec_to_bytes(20000, &ss);
        bufattr.maxlength = (uint32_t) - 1;
        bufattr.minreq = (uint32_t) - 1;
        bufattr.prebuf = (uint32_t) - 1;
        bufattr.tlength = pa_usec_to_bytes(20000, &ss);

        playback = pa_simple_new(nullptr,
                                 "Phoned",
                                 PA_STREAM_PLAYBACK,
                                 nullptr,
                                 "Playback",
                                 &ss,
                                 nullptr,
                                 nullptr,
                                 nullptr);
        if (!playback) {
            throw std::runtime_error("Failed to create playback stream");
        }

        record = pa_simple_new(nullptr,
                               "Phoned",
                               PA_STREAM_RECORD,
                               nullptr,
                               "Playback",
                               &ss,
                               nullptr,
                               &bufattr,
                               nullptr);

        if (!record) {
            throw std::runtime_error("Failed to create record stream");
        }

    }

    void start_recording() {
        if (!record_thd.joinable()) {
            // Means we are not running yet.
            record_thd = std::thread([&]() {
                int error;
                while(!quit) {
                    uint8_t buf[bufattr.fragsize];
                    if (pa_simple_read(record, buf, sizeof(buf), &error) < 0) {
                        throw std::runtime_error("failed to read audio data WTF: " + std::string(pa_strerror(error)));
                    } else {
                        try {
                            sample_callback(std::vector<uint8_t>(buf, buf + bufattr.fragsize));
                        } catch(...) {}
                    }
                }
            });
        }
    }

    void stop_recording() {
        quit = true;
        if (record_thd.joinable()) {
            record_thd.join();
        }
    }
};

audio::audio()
    : m_data (new Data()) {
}

audio::~audio() = default;

void audio::start() {
    m_data->start_recording();
}

void audio::stop() {
    m_data->stop_recording();
}

void audio::set_audio_sample_callback(audio_sample_callback callback) {
    m_data->sample_callback = callback;
}

int audio::write_sample(const std::vector<uint8_t>& sample) {
    int error;
    std::cout << "writing data to playback size: " << sample.size() << std::endl;
    if (pa_simple_write(m_data->playback, sample.data(), sample.size(), &error) < 0) {
        std::cerr << "failed to write data to pulse audio: " << pa_strerror(error) << std::endl;
        return error;
    }
    return sample.size();
}

int audio::get_sample_buffer_size() {
    return m_data->bufattr.tlength;
}
