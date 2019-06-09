#include "audio.hpp"
#include <gst/gst.h>
#include <iostream>

using namespace phoned;

struct audio::Data {
     GstElement* dialtone_pipeline;

     void init() {
	  gst_init(nullptr, nullptr);

	  GError *e = nullptr;
	  dialtone_pipeline = gst_parse_launch("audiotestsrc volume=1.0 freq=425 ! autoaudiosink", &e);

	  if (dialtone_pipeline == nullptr) {
	       std::cerr << "failed to create dialtone pipeline" << std::endl;
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

audio::audio()
     : m_data(new Data()) {
     m_data->init();
}

audio::~audio() {

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
