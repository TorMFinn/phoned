#include "phoned/dial.hpp"
#include <csignal>
#include <thread>
#include <iostream>
#include <systemd/sd-bus.h>
#include <cstring>
#include <mutex>
#include <poll.h>

bool quit;
std::mutex bus_mutex;
sd_bus *bus = nullptr;

void sighandler(int signum) {
    if (signum == SIGTERM || signum == SIGINT) {
        quit = true;
    }
}

void number_entered_callback(const std::string &number) {
    std::scoped_lock lock(bus_mutex);
    int r;
    r = sd_bus_emit_signal(bus, "/tmf/phoned/Dial", "tmf.phoned.Dial", "number_entered", "s", number.c_str());
    if (r < 0) {
        std::cerr << "failed to emit number_entered signal: " << std::strerror(-r) << std::endl;
    }
}

void digit_entered_callback(int digit) {
    std::scoped_lock lock(bus_mutex);
    int r;
    r = sd_bus_emit_signal(bus, "/tmf/phoned/Dial", "tmf.phoned.Dial", "digit_entered", "y", digit);
    if (r < 0) {
        std::cerr << "failed to emit digit_entered signal: " << std::strerror(-r) << std::endl;
    }
}

int main(int argc, char**argv) {
    int r;
    r = sd_bus_default(&bus);
    if (r < 0) {
        std::cerr << "call to sd_bus_default failed: " << std::strerror(-r) << std::endl;
        return -1;
    }

    r = sd_bus_request_name(bus, "tmf.phoned1.diald", 0);
    if (r < 0) {
        std::cerr << "failed to get requested bus name: " << std::strerror(-r) << std::endl;
        sd_bus_close_unref(bus);
        return -1;
    }

    const static sd_bus_vtable vtable[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_SIGNAL("number_entered", "s", 0),
        SD_BUS_SIGNAL("digit_entered", "y", 0),
        SD_BUS_VTABLE_END
    };

    r = sd_bus_add_object_vtable(bus, nullptr, "/tmf/phoned/Dial", "tmf.phoned.Dial", vtable, nullptr);
    if (r < 0) {
        std::cerr << "failed to add object vtable: " << std::strerror(-r) << std::endl;
        sd_bus_close_unref(bus);
        return -1;
    }

    phoned::dial dial;
    dial.set_digit_entered_callback(digit_entered_callback);
    dial.set_number_entered_callback(number_entered_callback);

    quit = false;
    struct pollfd p = {
        .fd = sd_bus_get_fd(bus),
    };

    while(not quit) {
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

    sd_bus_close_unref(bus);

    return 0;
}