#include "phoned/modem_audio.hpp"
#include <gstreamer-1.0/gst/gst.h>
#include <termios.h>
#include <cstring>
#include <iostream>
#include "fcntl.h"
#include "unistd.h"

using namespace phoned;

struct modem_audio::Data {
    Data(const std::string &serial_device, int baudrate) {
        gst_init(nullptr, nullptr);
        GError *error;
        playback_pipeline = gst_parse_launch("fdsrc ! audio/x-raw,format=S16LE,layout=interleaved,rate=8000,channels=1 ! autoaudiosink", &error);
        if (playback_pipeline == nullptr) {
            throw std::runtime_error("failed to create recording pipeline");
        }
        record_pipeline = gst_parse_launch("pulsesrc ! audio/x-raw,format=S16LE,layout=interleaved,rate=8000,channels=1 ! fdsink", &error);
        if (record_pipeline == nullptr) {
            throw std::runtime_error("failed to create playback pipeline");
        }

        init_serial(serial_device, baudrate);

        GstElement *fdsrc = gst_bin_get_by_name(GST_BIN(playback_pipeline), "fdsrc0");
        if (fdsrc == nullptr) {
            throw std::runtime_error("failed to get fdsrc from record pipeline");
        }

        GstElement *fdsink = gst_bin_get_by_name(GST_BIN(record_pipeline), "fdsink0");
        if (fdsink == nullptr) {
            throw std::runtime_error("failed to get fdsink ");
        }

        g_object_set(fdsrc, "fd", serial_fd, "blocksize", 512, nullptr);
        g_object_set(fdsink, "fd", serial_fd, "blocksize", 512, nullptr);
    }

    ~Data() {
        gst_element_set_state(record_pipeline, GST_STATE_NULL);
        gst_element_set_state(playback_pipeline, GST_STATE_NULL);
    }

    void init_serial(const std::string &serial_device, int baudrate) {
        int ret;
        serial_fd = open(serial_device.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
        if (serial_fd == -1) {
            throw std::runtime_error(std::string("failed to open serial device: ") + std::strerror(errno));
        }

        struct termios config;
        cfmakeraw(&config);
        ret = cfsetspeed(&config, baudrate);
        if (ret < 0) {
            throw std::runtime_error(std::string("failed to set baudrate: ") + std::strerror(errno));
        }

        ret = tcsetattr(serial_fd, TCSANOW, &config);
        if (ret < 0) {
            throw std::runtime_error(std::string("failed to set termios config: ") + std::strerror(errno));
        }
    }

    int serial_fd;
    GstElement* record_pipeline;
    GstElement* playback_pipeline;
};

modem_audio::modem_audio(const std::string &serial_device, int baudrate) 
: m_data(new Data(serial_device, baudrate)) {}

modem_audio::~modem_audio() = default;

void modem_audio::start_transfer() {
    std::cout << "Starting transfer" << std::endl;
    gst_element_set_state(m_data->record_pipeline, GST_STATE_PLAYING);
    gst_element_set_state(m_data->playback_pipeline, GST_STATE_PLAYING);
}

void modem_audio::stop_transfer() {
    std::cout << "Stopping transfer" << std::endl;
    gst_element_set_state(m_data->record_pipeline, GST_STATE_PAUSED);
    gst_element_set_state(m_data->playback_pipeline, GST_STATE_PAUSED);
}