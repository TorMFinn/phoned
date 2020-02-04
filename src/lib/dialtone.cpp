#include "phoned/dialtone.hpp"
#include <cmath>
#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <pulse/simple.h>

using namespace phoned;

const float m_2pi = 2 * M_PI;
const int audio_bufsize = 256;

struct dialtone::Data {
    Data() {
        spec.channels = 1;
        spec.format = PA_SAMPLE_S16NE;
        spec.rate = 8000;
        int error;

        pa = pa_simple_new(nullptr,
                           "phoned_audiod",
                           PA_STREAM_PLAYBACK,
                           nullptr,
                           "dialtone",
                           &spec,
                           nullptr,
                           nullptr,
                           &error);
        if (!pa) {
            throw std::runtime_error("pulseaudio error: " + std::to_string(error));
        }

        tone_thd = std::thread([&]() {
            quit = false;
            int16_t audio_buf[audio_bufsize];
            float step = (m_2pi) / spec.rate;
            float t = 0;
            while (not quit) {
                if (!enable_tone) {
                    std::unique_lock lock(cv_mutex);
                    tone_cv.wait(lock);
                }
                if (enable_tone) {
                    for (int i = 0; i < audio_bufsize; i++) {
                        audio_buf[i] = 32335 * sin(t * 420);
                        t += step;
                        if (t >= m_2pi) {
                            t -= m_2pi;
                        }
                    }
                    pa_simple_write(pa, audio_buf, audio_bufsize * 2, nullptr);
                } else {
                    pa_simple_flush(pa, nullptr);
                }
                if (quit) {
                    break;
                }
            }
        });
    }

    ~Data() {
        if (tone_thd.joinable()) {
            tone_cv.notify_all();
            tone_thd.join();
        }
    }

    pa_simple *pa;
    pa_sample_spec spec;

    std::thread tone_thd;
    bool quit;
    bool enable_tone;
    std::condition_variable tone_cv;
    std::mutex cv_mutex;
};

dialtone::dialtone()
    : m_data(new Data()) {
}

dialtone::~dialtone() {
}

void dialtone::start() {
    m_data->enable_tone = true;
    m_data->tone_cv.notify_all();
}

void dialtone::stop() {
    m_data->enable_tone = false;
}
