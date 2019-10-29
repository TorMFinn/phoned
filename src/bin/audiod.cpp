#include "phoned/dialtone.hpp"
#include <systemd/sd-bus-vtable.h>
#include <systemd/sd-bus.h>
#include <csignal>
#include <atomic>
#include <systemd/sd-event.h>
#include <thread>
#include <iostream>
#include <cstring>

sd_event *event;

struct program_state {
    phoned::dialtone dialtone;
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

static int handle_handset_changed(sd_bus_message* m, void *userdata, sd_bus_error *ret_error)  {
    int r;
    const char* handset_state;
    program_state *state = reinterpret_cast<program_state*>(userdata);
    r = sd_bus_message_read(m, "s", &handset_state);
    if (r < 0) {
        std::cerr << "failed to read message from signal: " << std::strerror(-r) << std::endl;
        return -1;
    }

    if (std::strcmp(handset_state, "down") == 0) {
        state->dialtone.stop();
    } else if (std::strcmp(handset_state, "lifted") == 0) {
        state->dialtone.start();
    }

    return 0;
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

    r = sd_bus_request_name(bus, "tmf.phoned1", 0);
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

    r = sd_bus_add_object_vtable(bus, nullptr, "/phoned1/Audio", "phoned1.Audio", vtable, &state);
    if (r < 0) {
        std::cerr << "failed to add object vtable: " << std::strerror(-r) << std::endl;
        sd_bus_close_unref(bus);
        return -1;
    }

    r = sd_bus_match_signal(bus, nullptr, nullptr, "/tmf/phoned/handset1", "tmf.phoned.handset1", "handset_state_change", handle_handset_changed, &state);
    if (r < 0) {
        std::cerr << "failed to match signal: " << std::strerror(-r) << std::endl;
        sd_bus_close_unref(bus);
        return -1;
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
