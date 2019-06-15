#include "audio.hpp"
#include <gst/gst.h>
#include <iostream>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <sndio.h>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <thread>

using namespace phoned;
using namespace boost::asio;

struct audio::Data {
     GstElement* dialtone_pipeline;
     std::string port;
     int audio_fd;
    bool io_started = false;

     // sndio data
     struct sio_hdl *hdl_rec, *hdl_play;
     struct sio_par par_rec;
     struct sio_par par_play;

     size_t bufsize_rec;
     size_t bufsize_play;

     uint8_t *snd_buf;
     uint8_t *rcv_buf;

     // Boost asio data
     boost::asio::io_context io_ctx;
     boost::asio::serial_port serial;

    bool call_in_progress;

    std::thread write_thd;

    Data()
        : serial(io_ctx) {
    }

     void init_sndio() {
	  hdl_rec = sio_open(SIO_DEVANY, SIO_REC, 0);
	  if (hdl_rec == nullptr) {
	       std::cerr << "failed to open sio record device"
			 << std::endl;
	  }

	  hdl_play = sio_open(SIO_DEVANY, SIO_PLAY, 0);
	  if (hdl_play == nullptr) {
	       std::cerr << "failed to open sio play device"
			 << std::endl;
	  }

	  sio_initpar(&par_rec);
	  sio_initpar(&par_play);

	  par_rec.bits = 16;
	  par_rec.sig = 1; // Signed.
	  par_rec.le = 1; // Little Endian.
	  par_rec.rchan = 1; // One recording channel.
	  par_rec.rate = 8000; // 8khz sound.

	  par_play.bits = 16;
	  par_play.sig = 1; // Signed.
	  par_play.le = 1; // Little Endian.
	  par_play.pchan = 1; // One recording channel.
	  par_play.rate = 8000; // 8khz sound.

	  // Set the parameters
	  if (not sio_setpar(hdl_rec, &par_rec)) {
	       printf("failed to set parameters\n");
	  }

	  if (not sio_setpar(hdl_play, &par_play)) {
	       printf("failed to set parameters for playing\n");
	  }

	  // Retrieve new configuration
	  if (not sio_getpar(hdl_rec, &par_rec)) {
	       printf("failed to get new parameters from device\n");
	  }

	  if (not sio_getpar(hdl_play, &par_play)) {
	       printf("failed to get new parameters from device\n");
	  }

	  // set the bufsizes
	  bufsize_rec = par_rec.appbufsz;
	  bufsize_play = par_play.appbufsz;

	  // allocate buffers
	  snd_buf = static_cast<uint8_t*>(malloc(bufsize_rec));
	  if (snd_buf == nullptr) {
	       std::cerr << "failed to allocate bytes for send buffer" << std::endl;
	  }
	  rcv_buf = static_cast<uint8_t*>(malloc(bufsize_play));

	  if (rcv_buf == nullptr) {
	       std::cerr << "failed to allocate bytes for receive buffer" << std::endl;
	  }

     }

     void close_fd() {
	  close(audio_fd);
     }

    void write_thread_function() {
        size_t n;
        while(call_in_progress) {
            n = sio_read(hdl_rec, snd_buf, bufsize_rec);
            if (n > 0 ) {
	      //std::cout << "writing audio to modem: " << n << std::endl;
	      //serial.write_some(boost::asio::buffer(snd_buf, n));
                serial.async_write_some(boost::asio::buffer(snd_buf, n),
                                        [](const boost::system::error_code& error, std::size_t bytes_transferred) {
					  std::cout << "Wrote audio to modem" << std::endl;
					});
            }
	    std::this_thread::sleep_for(std::chrono::milliseconds(70));
        }
    }

    void handle_receive(const boost::system::error_code& e,
                             std::size_t bytes_transferred) {
        std::cout << "received " << bytes_transferred << std::endl;
        if (bytes_transferred == bufsize_play) {
            // send audio to play device
            size_t n = sio_write(hdl_play, rcv_buf, bufsize_play);
            if (n == 0) {
                printf("failed to write audio\n");
            }
        }

        start_receive();
    }

     bool open_audio_port() {
         try {
	  // configure with default settings
          serial.open(port);
	  serial.set_option(serial_port_base::baud_rate(921600));

         } catch (const std::exception& e) {
             std::cerr << "failed to open audio port: " << e.what() << std::endl;
             return false;
         }

         return true;
     }

    void start_receive() {
        boost::asio::async_read(serial, boost::asio::buffer(rcv_buf, bufsize_play), boost::asio::transfer_at_least(bufsize_play), std::bind(&audio::Data::handle_receive, this, std::placeholders::_1, std::placeholders::_2));
        if (not io_started) {
          std::thread([&]() {
                          std::cout << "Starting io_ctx" << std::endl;
                          if(io_ctx.stopped()) {
                              io_ctx.restart();
                              io_ctx.run();
                          } else {
                          io_ctx.run();
                          }
                          std::cout << "IO CTX Stopped" << std::endl;
                      }).detach();
          io_started = true;
        }
    }

     void init_dialtone() {
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

     void stop_dialtone() {
	  if (dialtone_pipeline) {
	       gst_element_set_state(dialtone_pipeline, GST_STATE_NULL);
	  }
     }
};

audio::audio(const std::string &audio_path)
     : m_data(new Data()) {
     m_data->port = audio_path;
     m_data->init_dialtone();
     m_data->init_sndio();
     //m_data->open_audio_port();
}

audio::~audio() {
     m_data->stop_dialtone();
}

void audio::start_dialtone() {
     m_data->start_dialtone();
}

void audio::stop_dialtone() {
     m_data->stop_dialtone();
}

void audio::start_audio_transfer() {
     // Start reading
     if(sio_start(m_data->hdl_rec)) {
	  printf("started\n");
     }

     if (sio_start(m_data->hdl_play)) {
	  printf("started playing\n");
     }

     std::cout << "starting receive" << std::endl;
     m_data->open_audio_port();
     m_data->call_in_progress = true;
     m_data->start_receive();

     m_data->write_thd = std::thread([&](){
                                         m_data->write_thread_function();
                                     });
}

void audio::stop_audio_transfer() {
    if (m_data->call_in_progress) {
    std::cout << "stopping receive " << std::endl;
    m_data->call_in_progress = false;
    if(m_data->write_thd.joinable()) {
        std::cout << "Stopping thread" << std::endl;
        m_data->write_thd.join();
    } else {
        std::cout << "not joinable" << std::endl;
    }

    if (sio_stop(m_data->hdl_rec) ) {
        std::cout << "Stopped recording" << std::endl;
    }

    if (sio_stop(m_data->hdl_play)) {
        std::cout << "Stopped playing" << std::endl;
    }

    m_data->serial.cancel();
    m_data->io_ctx.stop();
    m_data->serial.close();
    m_data->io_started = false;
    }
}
