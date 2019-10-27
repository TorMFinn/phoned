#include "phoned/audiod_dbus.hpp"
#include <systemd/sd-bus.h>
#include <thread>
#include <iostream>
#include <cstring>

using namespace phoned::dbus;

struct audiod::Data {
    sd_bus *bus = nullptr;
    std::thread bus_thd;

    std::function<void ()> start_dialtone_callback;
    std::function<void ()> stop_dialtone_callback;
    std::function<void ()> start_audio_transfer_callback;
    std::function<void ()> stop_audio_transfer_callback;

    Data() {
        bus_thd = std::thread([&]() {
            int r;
            r = sd_bus_default(&bus);
            if (r < 0) {
                throw std::runtime_error(std::string("Failed to open default bus: ") + std::strerror(-r));
            }
        });
    }

    ~Data() {

    }

    static int start_dialtone_handler(sd_bus* bus, void *userdata, sd_bus_error *ret_error) {

    }

    static int stop_dialtone_handler(sd_bus* bus, void *userdata, sd_bus_error *ret_error) {

    }

    static int start_audio_transfer_handler(sd_bus* bus, void *userdata, sd_bus_error *ret_error) {

    }

    static int stop_audio_transfer_handler(sd_bus* bus, void *userdata, sd_bus_error *ret_error) {

    }
};

audiod::audiod()
    : m_data(new Data()) {
}

audiod::~audiod() {
}

void audiod::set_dialtone_start_callback(std::function<void ()> callback) {
    m_data->start_dialtone_callback = callback;
}

void audiod::set_dialtone_stop_callback(std::function<void ()> callback) {
    m_data->stop_dialtone_callback = callback;
}

void audiod::set_start_audio_transfer_callback(std::function<void ()> callback) {
    m_data->start_audio_transfer_callback = callback;
}

void audiod::set_stop_audio_transfer_callback(std::function<void ()> callback) {
    m_data->stop_audio_transfer_callback = callback;
}
