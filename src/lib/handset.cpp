#include "phoned/handset.hpp"
#include <atomic>
#include <cstring>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

#include <fcntl.h>
#include <linux/gpio.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

using namespace phoned;

const char *gpio_device = "/dev/gpiochip0";
const int handset_gpio_num = 13;

struct phoned::Handset::Data {
    // This is the default gpio for the handset
    std::function<void(HandsetState state)> state_changed_handler;
    std::thread gpio_read_thd;
    std::atomic_bool quit_read;
    HandsetState saved_state;
    int gpio_fd;
    bool reading_switch = false;
    std::mutex state_mutex;

    Data() {
        gpio_fd = open_gpio_device();
        if (gpio_fd > 0) {
            gpio_read_thd = std::thread([&]() {
                int ret;
                struct gpioevent_request req;
                struct gpiohandle_data data;

                req.lineoffset = handset_gpio_num;
                req.handleflags = GPIOHANDLE_REQUEST_INPUT;
                req.eventflags = GPIOEVENT_REQUEST_BOTH_EDGES;

                ret = ioctl(gpio_fd, GPIO_GET_LINEEVENT_IOCTL, &req);
                if (ret == -1) {
                    std::cerr << "failed to issue GET EVENT IOCTL: " << std::strerror(-ret) << std::endl;
                    return;
                }

                // Read initial state
                ret = ioctl(req.fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data);
                if (ret == -1) {
                    std::cerr << "failed to read gpio state: " << std::strerror(-ret) << std::endl;
                    return;
                }

                // Notify with initial state
                saved_state = value_to_state(data.values[0]);

                quit_read = false;
                while (not quit_read) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    std::scoped_lock lock(state_mutex);
                    ret = ioctl(req.fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data);
                    if (ret == -1) {
                        std::cerr << "failed to read gpio state: " << std::strerror(-ret) << std::endl;
                        continue;
                    }
                    update_state(data.values[0]);
                }
            });
        }
    }

    void update_state(int value) {
        auto new_state = value_to_state(value);
        if (new_state != saved_state) {
            try {
                state_changed_handler(new_state);
                saved_state = new_state;
            } catch (const std::exception &e) {
                ;
            }
        }
    }

    HandsetState value_to_state(int value) {
        if (value == 0) {
            return HandsetState::down;
        }

        return HandsetState::lifted;
    }

    ~Data() {
        quit_read = true;
        if (gpio_read_thd.joinable()) {
            gpio_read_thd.join();
        }

        close(gpio_fd);
    }

    int open_gpio_device() {
        int fd = open("/dev/gpiochip0", 0);
        if (fd == -1) {
            std::cerr << "failed to open gpiochip0: " << std::strerror(-errno) << std::endl;
            return -1;
        }

        return fd;
    }
};

Handset::Handset() : m_data(new Data()) {
}

Handset::~Handset() {
}

void Handset::SetHandsetStateChangedHandler(HandsetStateChangedHandler handler) {
    m_data->state_changed_handler = handler;
    HandsetState state;
    {
        std::scoped_lock lock(m_data->state_mutex);
        state = m_data->saved_state;
    }
    if (m_data->state_changed_handler) {
        m_data->state_changed_handler(state);
    }
}

HandsetState Handset::get_state() {
    std::scoped_lock lock(m_data->state_mutex);
    return m_data->saved_state;
}
