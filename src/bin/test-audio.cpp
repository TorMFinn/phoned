#include <iostream>
#include <cstring>
#include <pulse/def.h>
#include <sys/poll.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <csignal>
#include <poll.h>
#include <thread>

#include <pulse/simple.h>

pa_simple *playback;
pa_simple *record;
bool quit;

void sighandler(int signum) {
    quit = true;
}

void serial_reader() {
    int serial_fd = open("/dev/ttyUSB4", O_NOCTTY | O_RDWR | O_NDELAY);
    if (serial_fd < 0) {
        std::cerr << "failed to open serial port: " << std::strerror(errno);
        goto exit_serial;
    }

    struct termios config;
    cfmakeraw(&config);

    config.c_oflag &= ~(ONLRET);

    cfsetspeed(&config, 921600);
    if (tcsetattr(serial_fd, TCSANOW, &config) < 0) {
        std::cerr << "failed to configure serial port: " << std::strerror(errno)
                  << std::endl;
        goto exit_serial;
    }

    if(tcflush(serial_fd, TCIOFLUSH) < 0) {
        std::cerr << "failed to flush buffers: " << std::strerror(errno) << std::endl;
        goto exit_serial;
    }

    std::cout << "opened serial port" << std::endl;

    struct pollfd fds;
    fds.fd = serial_fd;
    fds.events = POLLIN | POLLOUT | POLLWRBAND;
    while(!quit) {
        int poll_ret = poll(&fds, 1, 500);
        if (poll_ret > 0) {
            // Poll Result OK
            if (fds.revents & POLLIN) {
                // Data to read
                std::cout << "POLLIN" << std::endl;
                char buf[128];
                ssize_t bytes_read = read(serial_fd, buf, 128);
                if (bytes_read > 0) {
                    std::cout << "Read " << bytes_read << " byte from modem" << std::endl;
                    int error;
                    if (pa_simple_write(playback, buf, bytes_read, &error) <
                        0) {
                      std::cerr << "failed to write to pulseaudio" << std::endl;
                    }
                } else {
                    std::cout << "read error: " << bytes_read << " : " << std::strerror(errno) << std::endl;
                    break;
                }
            } else if (fds.revents & POLLOUT) {
                // Possible to write data
                std::cout << "POLLOUT" << std::endl;
                char buf[128];
                int error;
                pa_usec_t latency = pa_simple_get_latency(record, &error);
                std::cout << "Current latency: " << latency << std::endl;
                pa_simple_flush(record, &error);
                if (pa_simple_read(record, buf, 128, &error) == 0) {
                    // Got all data
                    int bytes_written = write(serial_fd, buf, 128);
                    if (bytes_written > 0) {
                        std::cerr << "Wrote : " << bytes_written << " to modem" << std::endl;
                    } else {
                        std::cerr << "failed to write data: " << bytes_written << " reason: " << std::strerror(errno) << std::endl;
                        break;
                    }
                } else {
                    std::cerr << "failed to read from pulseaudio, what now ?" << std::endl;
                }
            } else {
                // Something else
                std::cerr << "Some other flag was set, mask: " << std::hex << (unsigned) fds.revents << std::dec << std::endl;
            }
        }
    }

    exit_serial:
    std::cout << "closing serial" << std::endl;
    close(serial_fd);
}

int main() {
    pa_sample_spec ss;

    ss.format = PA_SAMPLE_S16LE;
    ss.channels = 1;
    ss.rate = 8000;

    playback = pa_simple_new(nullptr, "Audiod", PA_STREAM_PLAYBACK, nullptr,
                             "modem audio", &ss, nullptr, nullptr, nullptr);
    if (playback == nullptr) {
      std::cerr << "failed to open pulse audio simple" << std::endl;
      goto exit;
    }

    record = pa_simple_new(nullptr, "audiod_record", PA_STREAM_RECORD, nullptr,
    "modem record", &ss, nullptr, nullptr, nullptr);
    if (record == nullptr) {
        std::cerr << "failed to open pulseaudio record device" << std::endl;
        goto exit;
    }

    quit = false;
    std::signal(SIGINT, sighandler);
    while(!quit) {
        auto read_thd = std::thread([&](){
            serial_reader();
        });
        read_thd.join();
        std::cout << "joined" << std::endl;
    }

    std::cout << "exiting" << std::endl;

    exit:
      pa_simple_free(playback);
      pa_simple_free(record);
      return 0;
}