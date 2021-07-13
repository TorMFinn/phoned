#include "phoned/audio.hpp"
#include <gstreamer-1.0/gst/gst.h>
#include <termios.h>
#include <cstring>
#include <iostream>
#include "fcntl.h"
#include "unistd.h"
#include "poll.h"
#include <mutex>

using namespace phoned;

struct audio::Data {
    Data() {
        gst_init(nullptr, nullptr);
        GError *error;
        playback_pipeline = gst_parse_launch("fdsrc ! audio/x-raw,format=S16LE,layout=interleaved,rate=8000,channels=1 ! audioresample ! pulsesink", &error);
        if (playback_pipeline == nullptr) {
            throw std::runtime_error("failed to create recording pipeline");
        }

        record_pipeline = gst_parse_launch("pulsesrc ! audio/x-raw,format=S16LE,layout=interleaved,rate=8000,channels=1 ! fdsink", &error);
        if (record_pipeline == nullptr) {
            throw std::runtime_error("failed to create playback pipeline");
        }
    }

    ~Data() {
    }

    void init_gst_fds() {
        GstElement *fdsrc = gst_bin_get_by_name(GST_BIN(playback_pipeline), "fdsrc0");
        if (fdsrc == nullptr) {
            throw std::runtime_error("failed to get fdsrc from record pipeline");
        }

        GstElement *fdsink = gst_bin_get_by_name(GST_BIN(record_pipeline), "fdsink0");
        if (fdsink == nullptr) {
            throw std::runtime_error("failed to get fdsink ");
        }

        g_object_set(fdsrc, "fd", serial_fd, "blocksize", 256, nullptr);
        g_object_set(fdsink, "fd", serial_fd, "blocksize", 256, nullptr);
    }

    bool open_serial_port(const std::string &device, int baudrate) {
        serial_fd = open(device.c_str(), O_RDWR);
        if (serial_fd < 0) {
            std::cerr << "failed to open serial device: " << std::strerror(errno)
                      << std::endl;
            return false;
        }

        // Set Raw terminal options
        struct termios serial_settings = {};
        cfmakeraw(&serial_settings);
        cfsetspeed(&serial_settings, baudrate);

        if (tcsetattr(serial_fd, TCSANOW, &serial_settings) < 0) {
            std::cerr << "failed to set serial port settings: "
                      << std::strerror(errno) << std::endl;
            return false;
        }

        return true;
    }

    void close_port() {
        close(serial_fd);
    }

    bool process;
    int serial_fd;
    int baudrate;
    std::string serial_device;
    GstElement* record_pipeline;
    GstElement* playback_pipeline;
};

audio::audio() 
    : m_data(new Data()) {}

audio::~audio() {
    stop_transfer();
}

bool audio::start_transfer(const std::string &device_name, int baudrate) {
    if (m_data->open_serial_port(device_name, baudrate)) {
        m_data->init_gst_fds();
        gst_element_set_state(m_data->record_pipeline, GST_STATE_PLAYING);
        gst_element_set_state(m_data->playback_pipeline, GST_STATE_PLAYING);
        return true;
    }
    return false;
}

void audio::stop_transfer() {
    std::cout << "Stopping transfer" << std::endl;
    m_data->close_port();
    gst_element_set_state(m_data->record_pipeline, GST_STATE_NULL);
    gst_element_set_state(m_data->playback_pipeline, GST_STATE_NULL);
    close(m_data->serial_fd);
}
