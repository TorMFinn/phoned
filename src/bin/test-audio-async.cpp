#include <pulse/pulseaudio.h>
#include <pulse/error.h>
#include <stdexcept>
#include <thread>

const int latency = 50000; //50ms delay

class PulseAudio {
public:
    PulseAudio() {
        
    }
    ~PulseAudio() {
    }

private:
    void init_pulseaudio() {
        mainloop = pa_threaded_mainloop_new();
        if (!mainloop) {
            throw std::runtime_error("Failed to create mainloop");
        }
        mainloop_api = pa_threaded_mainloop_get_api(mainloop);
        if (!mainloop_api) {
            throw std::runtime_error("Failed to get mainloop api");
        }
        context = pa_context_new(mainloop_api, "phoned_audio");
        if (!context) {
            throw std::runtime_error("failed to create pulse context");
        }
        pa_context_connect(context, nullptr, static_cast<pa_context_flags>(0), nullptr);
        pa_context_set_state_callback(context, pa_state_cb, &pa_ready);
        pa_threaded_mainloop_start(mainloop);

        while(pa_ready == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        if (pa_ready == 2) {
            throw std::runtime_error("failed to initialize context");
        }

        pa_sample_spec ss;
        ss.channels = 1;
        ss.format = PA_SAMPLE_S16LE;
        ss.rate = 8000;

        playback = pa_stream_new(context, "phoned-playback", &ss, nullptr);
        if (!playback) {
            throw std::runtime_error("Failed to create playback stream");
        }
        record = pa_stream_new(context, "phoned-read", &ss, nullptr);
    }

    // This callback gets called when our context changes state.  We really only
    static void pa_state_cb(pa_context *c, void *userdata) {
        pa_context_state_t state;
        int *pa_ready_ptr = static_cast<int*>(userdata);
        state = pa_context_get_state(c);
        switch  (state) {
            // These are just here for reference
        case PA_CONTEXT_UNCONNECTED:
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
        default:
            break;
        case PA_CONTEXT_FAILED:
        case PA_CONTEXT_TERMINATED:
            *pa_ready = 2;
            break;
        case PA_CONTEXT_READY:
            *pa_ready = 1;
            break;
        }
    }

    short sampledata[256];
    pa_threaded_mainloop *mainloop;
    pa_mainloop_api *mainloop_api;
    pa_context *context;
    pa_stream *record;
    pa_stream *playback;
    int pa_ready = 0;
};

int main(int argc, char**argv) {
}
