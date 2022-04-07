#include "phoned/modem.hpp"
#include <iostream>
#include <csignal>
#include <thread>
#include <systemd/sd-bus.h>
#include <cstring>
#include <poll.h>
#include <mutex>

bool quit;

void sighandler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        quit = true;
    }
}

static int dial_number_handler(sd_bus_message *m, void *userdata, sd_bus_error *err) {
    // Call handsetd and ask if handset is actually lifted or not
    sd_bus_message *reply = nullptr;
    sd_bus_error call_error = SD_BUS_ERROR_NULL;
    const char *state_string;
    int call_ok = sd_bus_call_method(sd_bus_message_get_bus(m),
                                    "tmf.phoned.handsetd1", 
                                    "/tmf/phoned/Handset", 
                                    "tmf.phoned.Handset", 
                                    "GetHandsetState",
                                    &call_error,
                                    &reply, "");
    if (call_ok < 0) {
        std::cerr << "failed to get handset state: " << std::strerror(-call_ok) << std::endl;
        return sd_bus_reply_method_return(m, "b", false);
    } 

    int r = sd_bus_message_read(reply, "s", &state_string);
    if (r < 0) {
        std::cerr << "failed to read message reply after call to GetHandsetState: " << std::strerror(-r) << std::endl;
        return sd_bus_reply_method_return(m, "b", false);
    }
    if (std::string(state_string) == "lifted") {
        phoned::modem* modem = reinterpret_cast<phoned::modem*>(userdata);
        const char *number;
        sd_bus_message_read(m, "s", &number);
        modem->dial(std::string(number));
        return sd_bus_reply_method_return(m, "b", true);
    }

    return sd_bus_reply_method_return(m, "b", false);
}

static int handle_handset_change(sd_bus_message *m, void *userdata, sd_bus_error *err) {
    phoned::modem* modem = reinterpret_cast<phoned::modem*>(userdata);
    const char *handset_state;
    int r;
    r = sd_bus_message_read(m, "s", &handset_state);

    if (std::string(handset_state) == "lifted") {
        if (modem->call_incoming()) {
            modem->answer_call();
        } else {
            int r;
            r = sd_bus_emit_signal(sd_bus_message_get_bus(m), "/tmf/phoned/Modem", "tmf.phoned.Modem", "start_dialtone", "");
            if (r < 0) {
                std::cerr << "failed to emit signal start_dialtone: " << std::strerror(-r) << std::endl;
            }
        }
    } else {
        if (modem->call_in_progress()) {
            modem->hangup();
        } else {
            int r;
            r = sd_bus_emit_signal(sd_bus_message_get_bus(m), "/tmf/phoned/Modem", "tmf.phoned.Modem", "stop_dialtone", "");
            if (r < 0) {
                std::cerr << "failed to emit signal stop_dialtone: " << std::strerror(-r) << std::endl;
            }
        }
    }
    return sd_bus_reply_method_return(m, "");
}

static int hangup_handler(sd_bus_message *m, void *userdata, sd_bus_error *err) {
    return sd_bus_reply_method_return(m, "b", 1);
}

const sd_bus_vtable vtable[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD("DialNumber", "s", "b", dial_number_handler, 0),
    SD_BUS_METHOD("Hangup", "", "b", hangup_handler, 0),
    SD_BUS_SIGNAL("call_incoming", "", 0),
    SD_BUS_SIGNAL("call_missed", "", 0),
    SD_BUS_SIGNAL("call_started", "", 0),
    SD_BUS_SIGNAL("call_ended", "", 0),
    SD_BUS_SIGNAL("voice_call_begin", "", 0),
    SD_BUS_SIGNAL("voice_call_end", "", 0),
    SD_BUS_SIGNAL("start_dialtone", "", 0),
    SD_BUS_SIGNAL("stop_dialtone", "", 0),
    SD_BUS_VTABLE_END
};

int main() {
    phoned::modem modem("/dev/ttyS0", 115200);
    sd_bus *bus = nullptr;
    std::mutex bus_mutex;
    struct pollfd p;
    int ret;
    ret = sd_bus_default(&bus);
    if (ret < 0) {
        std::cerr << "failed to open dbus connection: " << std::strerror(-ret) << std::endl;
        return -1;
    }

    ret = sd_bus_request_name(bus, "tmf.phoned.modemd1", 0);
    if (ret < 0) {
        std::cerr << "failed to get requested name: " << std::strerror(-ret) << std::endl;
        goto cleanup;
    }

    ret = sd_bus_add_object_vtable(bus, nullptr, "/tmf/phoned/Modem", "tmf.phoned.Modem", vtable, &modem);
    if (ret < 0) {
        std::cerr << "failed to add object vtable: " << std::strerror(-ret) << std::endl;
        goto cleanup;
    }

    ret = sd_bus_match_signal(bus, nullptr, nullptr, "/tmf/phoned/Handset", "tmf.phoned.Handset", "handset_state_change", handle_handset_change, &modem);
    if (ret < 0) {
        std::cerr << "failed to add signal match for handset_change: " << std::strerror(-ret) << std::endl;
        goto cleanup;
    }

    ret = sd_bus_match_signal(bus, nullptr, nullptr, "/tmf/phoned/Dial", "tmf.phoned.Dial", "number_entered", dial_number_handler, &modem);
    if (ret < 0) {
        std::cerr << "failed to add signal match for number_entered: " << std::strerror(-ret) << std::endl;
        goto cleanup;
    }

    modem.set_call_incoming_handler([&bus]() -> void {
        int r;
        r = sd_bus_emit_signal(bus, "/tmf/phoned/Modem", "tmf.phoned.Modem", "call_incoming", "");
        if (r < 0) {
            std::cerr << "failed to emit call_incoming signal: " << std::strerror(-r) << std::endl;
        }
    });

    modem.set_call_missed_handler([&bus]() -> void {
        int r;
        r = sd_bus_emit_signal(bus, "/tmf/phoned/Modem", "tmf.phoned.Modem", "call_missed", "");
        if (r < 0) {
            std::cerr << "failed to emit call_missed signal: " << std::strerror(-r) << std::endl;
        }
    });

    modem.set_call_started_handler([&bus]() -> void {
        int r;
        r = sd_bus_emit_signal(bus, "/tmf/phoned/Modem", "tmf.phoned.Modem", "call_started", "");
        if (r < 0) {
            std::cerr << "failed to emit call_started signal: " << std::strerror(-r) << std::endl;
        }
        r = sd_bus_emit_signal(bus, "/tmf/phoned/Modem", "tmf.phoned.Modem", "stop_dialtone", "");
        if (r < 0) {
            std::cerr << "failed to emit stop_dialtone signal: " << std::strerror(-r) << std::endl;
        }
    });

    modem.set_call_ended_handler([&bus]() -> void {
        int r;
        r = sd_bus_emit_signal(bus, "/tmf/phoned/Modem", "tmf.phoned.Modem", "call_ended", "");
        if (r < 0) {
            std::cerr << "failed to emit call_ended signal: " << std::strerror(-r) << std::endl;
        }
    });

    modem.set_voice_call_begin_handler([&bus]() -> void {
        int r;
        r = sd_bus_emit_signal(bus, "/tmf/phoned/Modem", "tmf.phoned.Modem", "voice_call_begin", "");
        if (r < 0 ) {
            std::cerr << "failed to emit voice_call_begin signal: " << std::strerror(-r) << std::endl;
        }
    });

    modem.set_voice_call_end_handler([&bus]() -> void {
        int r;
        r = sd_bus_emit_signal(bus, "/tmf/phoned/Modem", "tmf.phoned.Modem", "voice_call_end", "");
        if (r < 0) {
            std::cerr << "failed to emit voice_call_end signal: " << std::strerror(-r) << std::endl;
        }
    });

    quit = false;
    std::signal(SIGINT, sighandler);
    std::signal(SIGTERM, sighandler);
    p = {
        .fd = sd_bus_get_fd(bus),
    };
    int r;
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

cleanup: 
    sd_bus_close_unref(bus);

    return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
