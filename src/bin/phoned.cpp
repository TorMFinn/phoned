#include <iostream>
#include <cstring>
#include <csignal>
#include <systemd/sd-bus.h>

sd_event *event = nullptr;

int sighandler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        sd_event_exit(event, 0);
    }
}

static int handle_handset_signal(sd_bus_message *m, void *userdata, sd_bus_error *err) {

}

int main() {
    int r;
    sd_bus *bus = nullptr;

    r = sd_bus_default(&bus);
    if (r < 0) {
        std::cerr << "failed to open default bus: " << std::strerror(-r) << std::endl;
        return EXIT_FAILURE;
    }

    r = sd_bus_request_name(bus, "tmf.phoned1", 0);
    if (r < 0) {
        std::cerr << "failed to get requested name: " << std::endl;
        goto cleanup;
        return EXIT_FAILURE;
    }

    r = 

    cleanup:
    sd_bus_close_unref(bus);

    return r < 0 ? EXIT_FAILURE: EXIT_SUCCESS;
}