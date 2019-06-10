#include "audio.hpp"
#include <gst/gst.h>
#include <iostream>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>

using namespace phoned;

struct audio::Data {
    GstElement* dialtone_pipeline;
    GstElement* rx_pipeline;
    GstElement* tx_pipeline;
     GstElement* tx_valve;
     GstElement* rx_valve;

    int audio_fd;

    bool open_audio_fd(const std::string &path) {
        struct termios config;
        audio_fd = open(path.c_str(), O_RDWR, O_NOCTTY);
        if (audio_fd == -1) {
            std::cerr << "failed to open audio fd\n"
                      << std::strerror(errno) << std::endl;
            return false;
        }

        if (not isatty(audio_fd)) {
            std::cerr << "file is not a tty" << std::endl;
            return false;
        }

	if(tcgetattr(audio_fd, &config) < 0) {
	     std::cerr << "failed to get serial termios for audio"
		       << std::endl;
	     return false;
	}

        cfmakeraw(&config);
        if (cfsetspeed(&config, B230400) < 0) {
            std::cerr << "failed to set baudrate" << std::endl;
            return false;
        }

        if(tcsetattr(audio_fd, TCSAFLUSH, &config) < 0) {
            std::cerr << "failed to set serial config" << std::endl;
            return false;
        }

        return true;
    }

    void init() {
        gst_init(nullptr, nullptr);

        GError *e = nullptr;
        dialtone_pipeline = gst_parse_launch("audiotestsrc volume=1.0 freq=425 ! autoaudiosink", &e);

        if (dialtone_pipeline == nullptr) {
            std::cerr << "failed to create dialtone pipeline" << std::endl;
        }

        e = nullptr;
        rx_pipeline = gst_parse_launch("fdsrc name=fd ! audio/x-raw,format=S16LE,layout=interleaved,rate=8000,channels=1 ! audioresample ! valve drop=true name=valve ! autoaudiosink", &e);

        if (rx_pipeline) {
            GstElement *fdsrc = gst_bin_get_by_name(GST_BIN(rx_pipeline), "fd");
	    rx_valve = gst_bin_get_by_name(GST_BIN(rx_pipeline), "valve");
            g_object_set(fdsrc, "fd", audio_fd, nullptr);
	    gst_element_set_state(rx_pipeline, GST_STATE_PLAYING);
        } else {
            std::cerr << "failed to create rx_audio_pipeline" << std::endl;
        }

        e = nullptr;
        tx_pipeline = gst_parse_launch("autoaudiosrc ! audio/x-raw,format=S16LE,layout=interleaved,rate=8000,channels=1 ! valve drop=true name=valve ! fdsink name=fd max-lateness=1000000000", &e);
        if (tx_pipeline) {
            std::cout << "setting up tx pipeline" << std::endl;
            GstElement *fdsink = gst_bin_get_by_name(GST_BIN(tx_pipeline), "fd");
	    tx_valve = gst_bin_get_by_name(GST_BIN(tx_pipeline), "valve");
            g_object_set(fdsink, "fd", audio_fd, nullptr);
	    gst_element_set_state(tx_pipeline, GST_STATE_PLAYING);
        } else {
            std::cerr << "failed to create tx_audio_pipeline" << std::endl;
        }
    }

    void start_dialtone() {
        if (dialtone_pipeline) {
            gst_element_set_state(dialtone_pipeline, GST_STATE_PLAYING);
        }
    }

    void pause_dialtone() {
        if (dialtone_pipeline) {
            gst_element_set_state(dialtone_pipeline, GST_STATE_PAUSED);
        }
    }

    void stop_dialtone() {
        if (dialtone_pipeline) {
            gst_element_set_state(dialtone_pipeline, GST_STATE_NULL);
        }
    }
};

audio::audio(const std::string &audio_path)
    : m_data(new Data()) {
    if(not m_data->open_audio_fd(audio_path)) {
        throw std::runtime_error("failed to create audio serial line");
    }
    m_data->init();
}

audio::~audio() {
     m_data->stop_dialtone();
     gst_element_set_state(m_data->rx_pipeline, GST_STATE_NULL);
     gst_element_set_state(m_data->tx_pipeline, GST_STATE_NULL);
}

void audio::start_dialtone() {
    m_data->start_dialtone();
}

void audio::pause_dialtone() {
    m_data->pause_dialtone();
}

void audio::stop_dialtone() {
    m_data->stop_dialtone();
}

void audio::start_rx_line() {
     if (m_data->rx_pipeline){
	  //gst_element_set_state(m_data->rx_pipeline, GST_STATE_PLAYING);
	  g_object_set(m_data->rx_valve, "drop", false, nullptr);
     }
}

void audio::stop_rx_line() {
     if (m_data->rx_pipeline){
	  //gst_element_set_state(m_data->rx_pipeline, GST_STATE_PAUSED);
	  g_object_set(m_data->rx_valve, "drop", true, nullptr);
     }
}

void audio::start_tx_line() {
     if (m_data->tx_pipeline) {
	  //gst_element_set_state(m_data->tx_pipeline, GST_STATE_PLAYING);
	  g_object_set(m_data->tx_valve, "drop", false, nullptr);
     }
}

void audio::stop_tx_line() {
     if (m_data->tx_pipeline) {
	  //gst_element_set_state(m_data->tx_pipeline, GST_STATE_PAUSED);
	  g_object_set(m_data->tx_valve, "drop", true, nullptr);
     }
}
