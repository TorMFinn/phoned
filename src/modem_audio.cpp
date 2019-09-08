#include "modem_audio.hpp"

#include <termios.h>
#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include <sndio.h>
#include <fcntl.h>
#include <thread>

using namespace phoned;

struct modem_audio::Data {
  int serial_fd;

  struct sio_hdl *hdl_play, *hdl_rec;
  struct sio_par par_play, par_rec;

  size_t bufsize_sio;

  uint8_t* sndbuf;
  uint8_t* rcvbuf;

  std::thread process_thd;
  std::atomic_bool quit_audio;
  std::atomic_bool audio_running = false;

  void open_serial_port(const serial_connection_info& info) {
    serial_fd = open(info.device.c_str(), O_RDWR, O_NOCTTY);
    if (serial_fd <= 0) {
      throw std::runtime_error("failed to open audio serial port");
    }

    struct termios cfg;
    cfmakeraw(&cfg);

    cfg.c_cc[VMIN] = bufsize_sio;
    if(cfsetspeed(&cfg, 3686400) < 0) {
      printf("failed to set baudrate\n");
    }

    if (tcsetattr(serial_fd, TCSANOW, &cfg) <0 ) {
      printf("failed to set serial config");
    }
  }

  void init_sndio() {
    hdl_play = sio_open(SIO_DEVANY, SIO_PLAY, 0);
    if (hdl_play == nullptr) {
      throw std::runtime_error("failed to create sio play device");
    }

    hdl_rec = sio_open(SIO_DEVANY, SIO_REC, 0);
    if (hdl_rec == nullptr) {
      throw std::runtime_error("failed to create sio record device");
    }

    sio_initpar(&par_play);
    sio_initpar(&par_rec);
    par_play.bits = 16;
    par_play.sig = 1;
    par_play.le = 1;
    par_play.pchan = 1;
    par_play.rate = 8000;

    par_rec.bits = 16;
    par_rec.sig = 1;
    par_rec.le = 1;
    par_rec.rchan = 1;
    par_rec.rate = 8000;

    if (not sio_setpar(hdl_play, &par_play)) {
      throw std::runtime_error("failed to set sio parameters");
    }

    if (not sio_setpar(hdl_rec, &par_rec)) {
      throw std::runtime_error("failed to set sio parameters");
    }

    if (not sio_getpar(hdl_play, &par_play)) {
      throw std::runtime_error("failed to get sio parameters");
    }

    if (not sio_getpar(hdl_rec, &par_rec)) {
      throw std::runtime_error("failed to get sio parameters");
    }

    // This is the optimal buffer size for the desired parameters
    bufsize_sio = par_rec.appbufsz;

    sndbuf = (uint8_t*) malloc(bufsize_sio);
    rcvbuf = (uint8_t*) malloc(bufsize_sio);

    if (sndbuf == nullptr || rcvbuf == nullptr) {
      throw std::runtime_error("failed to allocate sound buffers");
    }
  }

  void process_data_thd() {
    struct pollfd fds[1];
    fds[0].fd = serial_fd;
    fds[0].events = POLLIN | POLLOUT | POLLPRI;
    while(not quit_audio) {
      int r = poll(fds, 1, 100);
      if (r < 0) {
	// error
      }
      else if (r > 0) {
	if (fds[0].revents & POLLIN) {
	  // Can retrieve audio
	  size_t received = read(serial_fd, rcvbuf, bufsize_sio);
	  printf("received %zu bytes from modem\n", received);
	  if (received > 0) {
	    size_t n = sio_write(hdl_play, rcvbuf, bufsize_sio);
	    if( n <= 0) {
	      printf("Failed to write data to sndio device");
	    }
	  }
	}
	if (fds[0].revents & POLLOUT) {
	  size_t a_read = sio_read(hdl_rec, sndbuf, bufsize_sio);
	  if (a_read > 0) {
	    size_t written = write(serial_fd, sndbuf, a_read);
	    printf("wrote %zu bytes to serial audio port\n", written);
	  }
	}
      }
    }
  }

  void close_port() {
    close(serial_fd);
  }

};

modem_audio::modem_audio(const serial_connection_info &info)
  : m_data(new Data()) {
  m_data->init_sndio();
  m_data->open_serial_port(info);
}

modem_audio::~modem_audio() {
  stop_audio_transfer();
  m_data->close_port();
}

void modem_audio::start_audio_transfer() {
  if(sio_start(m_data->hdl_play) == 0) {
    printf("failed to start sio device");
    return;
  }

  if(sio_start(m_data->hdl_rec) == 0) {
    printf("failed to start sio device");
    return;
  }

  m_data->process_thd = std::thread([&]() {
    m_data->quit_audio = false;
    m_data->process_data_thd();
  });

  m_data->audio_running = true;
}

void modem_audio::stop_audio_transfer() {
    if (not m_data->audio_running) {
        printf("audio is not running, no reason to stop");
        return;
    }
  if (m_data->process_thd.joinable()) {
    m_data->process_thd.join();
  }

  if(sio_stop(m_data->hdl_rec)) {
    printf("failed to stop sndio handle\n");
  }

  if(sio_stop(m_data->hdl_play)) {
    printf("failed to stop sndio handle\n");
  }
  m_data->audio_running = false;
}
  
