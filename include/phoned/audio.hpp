#pragma once
#include <vector>
#include <memory>
#include <functional>
#include <cstdint>

namespace phoned {
    class audio {
    public:
        using audio_sample_callback = std::function<void (const std::vector<uint8_t>&)>;

        audio();
        ~audio();

        void start();
        void stop();

        int get_sample_buffer_size();

        void set_audio_sample_callback(audio_sample_callback callback);
        int write_sample(const std::vector<uint8_t>& sample);

    private:
        struct Data;
        std::unique_ptr<Data> m_data;
    };
}
