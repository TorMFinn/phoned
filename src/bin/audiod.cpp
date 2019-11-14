#include "phoned/dialtone.hpp"
#include "phoned/modem_audio.hpp"
#include <systemd/sd-bus-vtable.h>
#include <systemd/sd-bus.h>
#include <csignal>
#include <atomic>
#include <systemd/sd-event.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <thread>
#include <iostream>
#include <cstring>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

sd_event *event;

struct program_state {
    phoned::modem_audio modem_audio;
    phoned::dialtone dialtone;

    program_state()
    : modem_audio("/dev/ttyUSB4", 921600) {}
};

void sighandler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        sd_event_exit(event, 0);
    }
}

static int handle_start_dialtone(sd_bus_message* m, void *userdata, sd_bus_error *ret_error) {
    program_state* state = reinterpret_cast<program_state*>(userdata);
    state->dialtone.start();
    return sd_bus_reply_method_return(m, "", nullptr);
}

static int handle_stop_dialtone(sd_bus_message* m, void *userdata, sd_bus_error *ret_error) {
    program_state* state = reinterpret_cast<program_state*>(userdata);
    state->dialtone.stop();
    return sd_bus_reply_method_return(m, "", nullptr);
}

static int handle_call_started(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    program_state *state = reinterpret_cast<program_state*>(userdata);
    state->modem_audio.start_transfer();
    return sd_bus_reply_method_return(m, "", nullptr);
}

static int handle_call_ended(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    program_state *state = reinterpret_cast<program_state*>(userdata);
    state->modem_audio.stop_transfer();
    return sd_bus_reply_method_return(m, "", nullptr);
}

static int handle_voice_begin(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    program_state *state = reinterpret_cast<program_state*>(userdata);
    state->modem_audio.start_voice_transfer();
    return sd_bus_reply_method_return(m, "", nullptr);
}

static int handle_voice_end(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    program_state *state = reinterpret_cast<program_state*>(userdata);
    state->modem_audio.stop_voice_transfer();
    return sd_bus_reply_method_return(m, "", nullptr);
}

int main(int argc, char **argv) {
    program_state state;

    sd_bus *bus = nullptr;
    int r;
    r = sd_event_default(&event);
    if (r < 0) {
        std::cerr << "failed to create default sd_event: " << std::strerror(-r) << std::endl;
        return -1;
    }

    r = sd_bus_default(&bus);
    if (r < 0) {
        std::cerr << "failed to open default bus: " << std::strerror(-r) << std::endl;
        return -1;
    }

    r = sd_bus_request_name(bus, "tmf.phoned.audiod1", 0);
    if (r < 0) {
        std::cerr << "call for request name failed: " << std::strerror(-r) << std::endl;
        sd_bus_close_unref(bus);
        return -1;
    }

    const static sd_bus_vtable vtable[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("StartDialtone", "", "", handle_start_dialtone, 0),
        SD_BUS_METHOD("StopDialtone", "", "", handle_stop_dialtone, 0),
        SD_BUS_VTABLE_END
    };

    r = sd_bus_add_object_vtable(bus, nullptr, "/tmf/phoned/Audio", "tmf.phoned.Audio", vtable, &state);
    if (r < 0) {
        std::cerr << "failed to add object vtable: " << std::strerror(-r) << std::endl;
        sd_bus_close_unref(bus);
        return EXIT_FAILURE;
    }

    r = sd_bus_match_signal(bus, nullptr, nullptr, "/tmf/phoned/Modem", "tmf.phoned.Modem", "start_dialtone", handle_start_dialtone, &state);
    if (r < 0) {
        std::cerr << "failed to match signal: " << std::strerror(-r) << std::endl;
        sd_bus_close_unref(bus);
        return EXIT_FAILURE;
    }

    r = sd_bus_match_signal(bus, nullptr, nullptr, "/tmf/phoned/Modem", "tmf.phoned.Modem", "stop_dialtone", handle_stop_dialtone, &state);
    if (r < 0) {
        std::cerr << "failed to match signal: " << std::strerror(-r) << std::endl;
        sd_bus_close_unref(bus);
        return EXIT_FAILURE;
    }

    r = sd_bus_match_signal(bus, nullptr, nullptr, "/tmf/phoned/Modem", "tmf.phoned.Modem", "call_started", handle_call_started, &state);
    if (r < 0) {
        std::cerr << "failed to match signal: " << std::strerror(-r) << std::endl;
        sd_bus_close_unref(bus);
        return EXIT_FAILURE;
    }

    r = sd_bus_match_signal(bus, nullptr, nullptr, "/tmf/phoned/Modem", "tmf.phoned.Modem", "call_ended", handle_call_ended, &state);
    if (r < 0) {
        std::cerr << "failed to match signal: " << std::strerror(-r) << std::endl;
        sd_bus_close_unref(bus);
        return EXIT_FAILURE;
    }

    r = sd_bus_match_signal(bus, nullptr, nullptr, "/tmf/phoned/Modem", "tmf.phoned.Modem", "voice_call_begin", handle_voice_begin, &state);
    if (r < 0) {
        std::cerr << "failed to match signal: " << std::strerror(-r) << std::endl;
        sd_bus_close_unref(bus);
        return EXIT_FAILURE;
    }

    r = sd_bus_match_signal(bus, nullptr, nullptr, "/tmf/phoned/Modem", "tmf.phoned.Modem", "voice_call_end", handle_voice_end, &state);
    if (r < 0) {
        std::cerr << "failed to match signal: " << std::strerror(-r) << std::endl;
        sd_bus_close_unref(bus);
        return EXIT_FAILURE;
    }

    sd_bus_attach_event(bus, event, 0);

    sd_event_loop(event);

    std::signal(SIGINT, sighandler);
    std::signal(SIGTERM, sighandler);

    sd_bus_close_unref(bus);
    sd_event_unref(event);

    std::cout << "Exiting" << std::endl;

    return 0;
}
