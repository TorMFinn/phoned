#include "phoned/ringer.hpp"
#include <linux/gpio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <cstring>
#include <iostream>
#include <thread>

using namespace phoned;

const uint8_t GPIO_RINGER_ENABLE_PIN = 26;

struct ringer::Data {
    Data() {
        gpio_fd = open("/dev/gpiochip0", 0);
        if (gpio_fd == -1) {
            throw std::runtime_error(std::string("failed to open gpiochip0") + std::strerror(errno));
        }

        int ret;
        ringer_output_request.lineoffsets[0] = GPIO_RINGER_ENABLE_PIN;
        ringer_output_request.lines = 1;
        ringer_output_request.flags = GPIOHANDLE_REQUEST_OUTPUT;

        ret = ioctl(gpio_fd, GPIO_GET_LINEHANDLE_IOCTL, &ringer_output_request);
        if (ret == -1) {
            throw std::runtime_error("failed to get ioctl for linehandle");
        }

        set_output_value(0);
    }
    ~Data() {
        stop_ringing();
        close(gpio_fd);
    }

    void set_output_value(int value) {
        int ret;
        struct gpiohandle_data data;
        data.values[0] = value;
        ret = ioctl(ringer_output_request.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
        if (ret == -1) {
            std::cerr << "failed to set ringer output value" << std::endl;
        }
    }

    void start_ringing() {
        if (not ringing) {
            if (ringer_thd.joinable()) {
                return;
            }
            ringer_thd = std::thread([this]() {
                ringing = true;
                while(ringing) {
                    for(int i = 0; i < 200; i++) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        toggle_output();
                        if (not ringing) {
                            return;
                        }
                    }
                    set_output_value(0);
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                }
            });
        }
    }

    void stop_ringing() {
        if (ringing) {
            ringing = false;
            if (ringer_thd.joinable()) {
                ringer_thd.join();
            }
            set_output_value(0);
        }
    }

    void toggle_output() {
        int ret;
        struct gpiohandle_data data;

        ret = ioctl(ringer_output_request.fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data);
        if (ret == -1) {
            std::cerr << "failed to get ringer output value" << std::endl;
        }
        data.values[0] = (data.values[0] == 1) ? 0 : 1;
        ret = ioctl(ringer_output_request.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
        if (ret == -1) {
            std::cerr << "failed to set ringer output value" << std::endl;
        }
    }

    int gpio_fd;
    struct gpiohandle_request ringer_output_request;
    std::thread ringer_thd;
    bool ringing = false;
};

ringer::ringer() 
: m_data(new Data()) {
    stop();
}

ringer::~ringer() = default;

void ringer::start() {
    m_data->start_ringing();
}

void ringer::stop() {
    m_data->stop_ringing();
}
