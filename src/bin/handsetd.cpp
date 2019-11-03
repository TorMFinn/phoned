#include "phoned/handset.hpp"
#include <systemd/sd-bus-vtable.h>
#include <systemd/sd-event.h>
#include <thread>
#include <iostream>
#include <mutex>
#include <cstring>
#include <csignal>
#include <poll.h>
#include <atomic>
#include <systemd/sd-bus.h>

std::atomic_bool quit_program = false;

void sighandler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        quit_program = true;
    }
}

static int get_state_handler(sd_bus_message *m, void *userdata, sd_bus_error* err) {
    phoned::handset *handset = reinterpret_cast<phoned::handset*>(userdata);
    auto state = handset->get_state();
    if (state == phoned::handset::handset_state::lifted) {
        return sd_bus_reply_method_return(m, "s", "lifted");
    } 
    return sd_bus_reply_method_return(m, "s", "down");
}

int main(int argc, char**argv) {
    phoned::handset handset;
    std::mutex bus_mutex;

    sd_bus *bus = nullptr;
    int r;
    r = sd_bus_default(&bus);
    if (r < 0) {
        std::cerr << "failed to create default bus: " << std::strerror(-r) << std::endl;
        return -1;
    }

    r = sd_bus_request_name(bus, "tmf.phoned.handset1", 0);
    if (r < 0) {
        std::cerr << "failed to get requested name" << std::endl;
        sd_bus_close_unref(bus);
        return -1;
    }

    const static sd_bus_vtable vtable[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("GetHandsetState", "", "s", get_state_handler, 0),
        SD_BUS_SIGNAL("handset_state_change", "s", 0),
        SD_BUS_VTABLE_END
    };

    r = sd_bus_add_object_vtable(bus, nullptr, "/tmf/phoned/handset1", "tmf.phoned.handset1", vtable, &handset);
    if (r < 0) {
        std::cerr << "failed to add object vtable: " << std::strerror(-r) << std::endl;
        sd_bus_close_unref(bus);
        return -1;
    }

    handset.set_handset_state_changed_callback([&bus, &bus_mutex](phoned::handset::handset_state state) {
        if (state == phoned::handset::handset_state::down) {
            std::scoped_lock lock(bus_mutex);
            int r = sd_bus_emit_signal(bus, "/tmf/phoned/handset1", "tmf.phoned.handset1", "handset_state_change", "s", "down");
            if (r < 0) {
                std::cerr << "failed to emit signal: " << std::strerror(-r) << std::endl;
            }
        } else {
            std::scoped_lock lock(bus_mutex);
            bool state = false;
            int r = sd_bus_emit_signal(bus, "/tmf/phoned/handset1", "tmf.phoned.handset1", "handset_state_change", "s", "lifted");
            if (r < 0) {
                std::cerr << "failed to emit signal: " << std::strerror(-r) << std::endl;
            }
        }
    });

    std::signal(SIGINT, sighandler);
    std::signal(SIGTERM, sighandler);
    struct pollfd p = {
        .fd = sd_bus_get_fd(bus),
    };
    while(not quit_program) {
        int r;
        {
            std::scoped_lock lock(bus_mutex);
            r = sd_bus_process(bus, nullptr);
            if( r > 0) {
                continue;
            }
        }
        p.events = sd_bus_get_events(bus);
        r = poll(&p, 1, 500);
        if (r > 0) {
            continue;
        }
    }

    sd_bus_close(bus);
    sd_bus_unref(bus);

    return 0;
}
