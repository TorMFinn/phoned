#include "phoned/modem_audio.hpp"
#include <gstreamer-1.0/gst/gst.h>
#include <termios.h>
#include <cstring>
#include <iostream>
#include "fcntl.h"
#include "unistd.h"
#include "poll.h"
#include <mutex>

using namespace phoned;

struct modem_audio::Data {
    Data(const std::string &device, int baud) {
        serial_device = device;
        baudrate = baud;
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

        GstElement *pulsesrc = gst_bin_get_by_name(GST_BIN(record_pipeline), "pulsesrc0");
        if (pulsesrc == nullptr) {
            throw std::runtime_error("failed to get pulsesrc from pipeline");
        }

        GstElement *pulsesink = gst_bin_get_by_name(GST_BIN(playback_pipeline), "pulsesink0");
        if (pulsesink == nullptr) {
            throw std::runtime_error("failed to get pulsesink from pipeline");
        }

        g_object_set(fdsrc, "fd", serial_fd, "blocksize", 256, nullptr);
        g_object_set(fdsink, "fd", serial_fd, "blocksize", 256, nullptr);

        //g_object_set(pulsesrc, "blocksize", 128, nullptr);
        //g_object_set(pulsesink, "blocksize", 128, nullptr);
    }

    void init_serial() {
        int ret;
        serial_fd = open(serial_device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
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

    bool process;
    int serial_fd;
    int baudrate;
    std::mutex fd_mutex;
    std::string serial_device;
    GstElement* record_pipeline;
    GstElement* playback_pipeline;
    GstElement* appsrc;
    GstElement* appsink;
};

modem_audio::modem_audio(const std::string &serial_device, int baudrate) 
: m_data(new Data(serial_device, baudrate)) {
}

modem_audio::~modem_audio() {
    stop_transfer();
}

void modem_audio::start_voice_transfer() {

}

void modem_audio::stop_voice_transfer() {

}

void modem_audio::start_transfer() {
    m_data->init_serial();
    m_data->init_gst_fds();
    std::cout << "Starting transfer" << std::endl;
    gst_element_set_state(m_data->record_pipeline, GST_STATE_PLAYING);
    gst_element_set_state(m_data->playback_pipeline, GST_STATE_PLAYING);
}

void modem_audio::stop_transfer() {
    std::cout << "Stopping transfer" << std::endl;
    gst_element_set_state(m_data->record_pipeline, GST_STATE_NULL);
    gst_element_set_state(m_data->playback_pipeline, GST_STATE_NULL);
    close(m_data->serial_fd);
}