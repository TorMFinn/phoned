#include <sndio.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <string>
#include <fcntl.h>
#include <poll.h>
#include <iostream>

int open_serial(const std::string &path) {
        struct termios config;
	int fd;
        fd = open(path.c_str(), O_RDWR, O_NOCTTY);
        if (fd == -1) {
	     printf("failed to open audio fd\n");
	     return -1;
        }

        if (not isatty(fd)) {
	     printf("file is not a tty\n");
            return -1;
        }

	if(tcgetattr(fd, &config) < 0) {
	     printf("failed to get serial termios for audio\n");
	     return -1;
	}

        cfmakeraw(&config);
        if (cfsetspeed(&config, B921600) < 0) {
	     printf("failed to set baudrate\n");
            return -1;
        }

        if(tcsetattr(fd, TCSAFLUSH, &config) < 0) {
	     printf("failed to set serial config");
            return -1;
        }

        return fd;
}

void print_buf(uint8_t *buf, size_t len, int width) {
     for (int i = 0; i < len; i++) {
	  if ((i % width) == 0) {
	       printf("\n");
	  }
	  printf("%u ", buf[i]);
     }
}

int main(int argc, char **argv) {
     struct sio_hdl *hdl_rec, *hdl_play;
     struct sio_par par;
     struct sio_par par_play;
     int fd = open_serial("/dev/cuaU0.4");

     hdl_rec = sio_open(SIO_DEVANY, SIO_REC, 0);
     if (hdl_rec == nullptr) {
	  printf("failed to open sio device");
	  return -1;
     }

     hdl_play = sio_open(SIO_DEVANY, SIO_PLAY, 0);
     if (hdl_play == nullptr) {
	  printf("failed to open sio play device\n");
	  return -1;
     }

     sio_initpar(&par);
     sio_initpar(&par_play);

     par.bits = 16;
     par.sig = 1; // Signed.
     par.le = 1; // Little Endian.
     par.rchan = 1; // One recording channel.
     par.rate = 8000; // 8khz sound.

     par_play.bits = 16;
     par_play.sig = 1; // Signed.
     par_play.le = 1; // Little Endian.
     par_play.pchan = 1; // One recording channel.
     par_play.rate = 8000; // 8khz sound.

     // Set the parameters
     if (not sio_setpar(hdl_rec, &par)) {
	  printf("failed to set parameters\n");
     }

     if (not sio_setpar(hdl_play, &par_play)) {
	  printf("failed to set parameters for playing\n");
     }

     // Retrieve new configuration
     if (not sio_getpar(hdl_rec, &par)) {
	  printf("failed to get new parameters from device\n");
     }

     if (not sio_getpar(hdl_play, &par_play)) {
	  printf("failed to get new parameters from device\n");
     }

     //size_t bufsize_rec = par.bps * par.rchan * par.round;
     //size_t bufsize_play = par_play.bps * par_play.pchan * par_play.round;

     size_t bufsize_rec = par.appbufsz;
     size_t bufsize_play = par_play.appbufsz;
     uint8_t *snd_buf = static_cast<uint8_t*>(malloc(bufsize_rec));
     uint8_t *rcv_buf = static_cast<uint8_t*>(malloc(bufsize_play));
     if (snd_buf == nullptr) {
         printf("failed to allocate bytes for buffer\n");
     }
     if (rcv_buf == nullptr) {
	  printf("failed to allocate bytes for recv buffer\n");
     }
     
     // Start reading
     if(sio_start(hdl_rec)) {
	  printf("started\n");
     }

     if (sio_start(hdl_play)) {
	  printf("started playing\n");
     }

     //tcflush(fd, TCIFLUSH);

     size_t n;
     while(1) {
         char trash[1024];
	     ssize_t r = read(fd, rcv_buf, bufsize_play);
         ssize_t t = read(fd, trash, sizeof(trash));
         printf("t: %zd\n", t);
	  if (r < bufsize_play) {
	       printf("could not read requested bytes");
	       continue;
	  }
      /*
      // fush read
      char c;
      while(read(fd, &c, 1) != EAGAIN);
      */

	  printf("read %zu\n", r);

	  n = sio_write(hdl_play, rcv_buf, bufsize_play);
	  if (n == 0) {
	       printf("failed to write audio\n");
	       break;
	  }

	  n = sio_read(hdl_rec, snd_buf, bufsize_rec);
	  if (n == 0) {
	       printf("failed to read\n");
	       break;
	  }

	  if (n > 0) {
	       ssize_t written = write(fd, snd_buf, bufsize_rec);
	       printf("Wrote %zd bytes to serial\n", written);
	  }
     }

     sio_close(hdl_rec);
     sio_close(hdl_play);
	  
     return 0;
}
