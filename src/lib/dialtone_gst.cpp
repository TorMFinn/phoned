#include "phoned/dialtone.hpp"
#include <gst/gst.h>
#include <cmath>
#include <iostream>

using namespace phoned;

const float m_2pi = 6.28318530;

struct dialtone::Data {
    GstElement *pipeline;

    Data() {
        gst_init(nullptr, nullptr);
        GError *error;
        pipeline = gst_parse_launch("audiotestsrc freq=425 ! autoaudiosink", &error);
        if (pipeline == nullptr) {
            throw std::runtime_error("failed to create dialtone pipeline");
        }
    }

    ~Data() {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(pipeline));
    }
};

dialtone::dialtone()
    : m_data(new Data()) {
}

dialtone::~dialtone() {
}

void dialtone::start() {
    gst_element_set_state(m_data->pipeline, GST_STATE_PLAYING);
}

void dialtone::stop() {
    gst_element_set_state(m_data->pipeline, GST_STATE_PAUSED);
}
