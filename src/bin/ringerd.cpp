#include "phoned/ringer.hpp"
#include <systemd/sd-bus.h>
#include <iostream>
#include <cstring>
#include <csignal>

sd_event *event = nullptr;

void sighandler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        if (event != nullptr) {
            sd_event_exit(event, 0);
        }
    }
}

static int handle_start_ringer(sd_bus_message *m, void *userdata, sd_bus_error *err) {
    phoned::ringer* ringer = reinterpret_cast<phoned::ringer*>(userdata);
    ringer->start();
    return sd_bus_reply_method_return(m, "");
}

static int handle_stop_ringer(sd_bus_message *m, void *userdata, sd_bus_error *err) {
    phoned::ringer* ringer = reinterpret_cast<phoned::ringer*>(userdata);
    ringer->stop();
    return sd_bus_reply_method_return(m, "");
}

static int handle_incoming_call(sd_bus_message *m, void *userdata, sd_bus_error *err) {
    return handle_start_ringer(m, userdata, err);
}

static int handle_missed_call(sd_bus_message *m, void *userdata, sd_bus_error *err) {
    return handle_stop_ringer(m, userdata, err);
}

static int handle_call_started(sd_bus_message *m, void *userdata, sd_bus_error *err) {
    std::cout << "CALL STARTED " << std::endl;
    return handle_stop_ringer(m, userdata, err);
}

int main(int argc, char **argv) {
    sd_bus *bus = nullptr;
    int ret;
    ret = sd_bus_default(&bus);
    if (ret < 0) {
        std::cerr << "failed to open dbus connection: " << std::strerror(-ret) << std::endl;
        return -1;
    }

    ret = sd_bus_request_name(bus, "tmf.phoned.ringerd1", 0);
    if (ret < 0) {
        std::cerr << "failed to get requested bus name: " << std::strerror(-ret) << std::endl;
        sd_bus_close_unref(bus);
        return -1;
    }

    const sd_bus_vtable vtable[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("StartRinger", "", "", handle_start_ringer, 0),
        SD_BUS_METHOD("StopRinger", "", "", handle_stop_ringer, 0),
        SD_BUS_VTABLE_END
    };

    phoned::ringer ringer;

    ret = sd_bus_add_object_vtable(bus, nullptr, "/tmf/phoned/Ringer", "tmf.phoned.Ringer", vtable, &ringer);
    if (ret < 0) {
        std::cerr << "failed to add object vtable: " << std::strerror(-ret) << std::endl;
        sd_bus_close_unref(bus);
        return EXIT_FAILURE;
    }

    ret = sd_bus_match_signal(bus, nullptr, nullptr, "/tmf/phoned/Modem", "tmf.phoned.Modem", "call_incoming", handle_incoming_call, &ringer);
    if (ret < 0) {
        std::cerr << "failed to add signal match, incoming_call: " << std::strerror(-ret) << std::endl;
        sd_bus_close_unref(bus);
        return EXIT_FAILURE;
    }

    ret = sd_bus_match_signal(bus, nullptr, nullptr, "/tmf/phoned/Modem", "tmf.phoned.Modem", "call_missed", handle_missed_call, &ringer);
    if (ret < 0) {
        std::cerr << "failed to add signal match, missed_call: " << std::strerror(-ret) << std::endl;
        sd_bus_close_unref(bus);
        return EXIT_FAILURE;
    }

    ret = sd_bus_match_signal(bus, nullptr, nullptr, "/tmf/phoned/Modem", "tmf.phoned.Modem", "call_started", handle_call_started, &ringer);
    if (ret < 0) {
        std::cerr << "failed to add signal match, call_started: " << std::strerror(-ret) << std::endl;
        sd_bus_close_unref(bus);
        return EXIT_FAILURE;
    }

    ret = sd_event_default(&event);
    if (ret < 0) {
        std::cerr << "failed to create sd_event: " << std::strerror(-ret) << std::endl;
        sd_bus_close_unref(bus);
        return -1;
    }

    sd_bus_attach_event(bus, event, SD_EVENT_PRIORITY_NORMAL);
    sd_event_loop(event);

    sd_bus_close_unref(bus);
    sd_event_unref(event);

    return 0;
}