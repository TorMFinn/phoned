#include "phoned/handset.hpp"
#include <cstring>
#include <functional>
#include <atomic>
#include <iostream>
#include <thread>

#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/gpio.h>
#include <sys/ioctl.h>
#include <sys/types.h>

using namespace phoned;

const char *gpio_device = "/dev/gpiochip0";
const int handset_gpio_num = 14;

struct phoned::handset::Data {
    // This is the default gpio for the handset
    std::function<void (handset::handset_state state)> state_change_callback;
    std::thread gpio_read_thd;
    std::atomic_bool quit_read;
    int gpio_fd;

    Data () {
        gpio_fd = open_gpio_device();
        if (gpio_fd > 0) {
            gpio_read_thd = std::thread([&](){
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

                quit_read = false;
                while(not quit_read) {
                    struct gpioevent_data event;
                    ret = read(req.fd, &event, sizeof(event));
                    if (ret == -1) {
                        if (errno == -EAGAIN) {
                            std::cerr << "nothing to read" << std::endl;
                            continue;
                        } else {
                            std::cerr << "error reading: " << std::strerror(-ret) << std::endl;
                            break;
                        }
                    }

                    if (ret != sizeof(event)) {
                        std::cerr << "reading event failed: " << std::strerror(-ret) << std::endl;
                        break;
                    }

                    switch(event.id) {
                    case GPIOEVENT_EVENT_RISING_EDGE:
                        std::cout << "Rising event" << std::endl;
                        break;
                    case GPIOEVENT_EVENT_FALLING_EDGE:
                        std::cout << "Falling edge" << std::endl;
                        break;
                    default:
                        std::cout << "Unknown event" << std::endl;
                        break;
                    }
                }


            });
        }
    }

    ~Data () {
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

