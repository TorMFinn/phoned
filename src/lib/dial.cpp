#include "phoned/dial.hpp"
#include <cstring>
#include <linux/gpio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <thread>
#include <iostream>
#include <mutex>
#include <chrono>

using namespace phoned;

const int GPIO_DIAL_ENABLE = 17;
const int GPIO_DIAL_COUNT = 27;

struct dial::Data {
    Data() {
        init_gpio();

        enable_read_thd = std::thread([this]() {
            int r;
            struct gpiohandle_data data;
            while(not quit_read) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                r = ioctl(dial_enable_req.fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data);
                if (r == -1) {
                    std::cerr << "failed to read gpio state: " << std::strerror(-r) << std::endl;
                    continue;
                }
                auto now = std::chrono::steady_clock::now();
                {
                    if (data.values[0] == 1 && not enable_is_active) {
                        // Enable event reading
                        std::scoped_lock lock(enable_mutex);
                        enable_is_active = true;
                    } else if (data.values[0] == 0 && enable_is_active) {
                        enable_is_active = false;
                        if (counted_value >= 10) {
                            counted_value = 0;
                        }
                        digit_entered_callback(counted_value);
                        time_last_input = now;
                        number_buffer += std::to_string(counted_value);
                        counted_value = 0;
                    }
                }
                if (std::chrono::duration_cast<std::chrono::seconds>(now - time_last_input).count() >= 4 && data.values[0] == 0) {
                    if (not number_buffer.empty()) {
                        number_entered_callback(number_buffer);
                        number_buffer.clear();
                    }
                }
            }
        });

        count_read_thd = std::thread([this]() {
            int r;
            while(not quit_read) {
                struct gpioevent_data event;
                r = read(dial_count_req.fd, &event, sizeof(event));
                if (r == -1) {
                    if (errno == -EAGAIN) {
                        continue;
                    } else {
                        std::cerr << "failed to read event: " << -errno << std::endl;
                        break;
                    }
                }

                if (r != sizeof(event)) {
                    std::cerr << "reading event failed" << std::endl;
                    break;
                }

                switch(event.id) {
                    case GPIOEVENT_EVENT_RISING_EDGE:
                    {
                        std::scoped_lock lock(enable_mutex);
                        if (enable_is_active) {
                            counted_value++;
                        }
                    }
                    break;
                    default:
                    break;
                }
            }
        });
    } 

    ~Data() {
        quit_read = true;
        if (enable_read_thd.joinable()) {
            enable_read_thd.join();
        }
        if (count_read_thd.joinable()) {
            count_read_thd.join();
        }
    }

    void init_gpio() {
        gpio_fd = open("/dev/gpiochip0", 0);
        if (gpio_fd == -1) {
            throw std::runtime_error(std::string("failed to open gpiochip0: ") + std::strerror(errno));
        }

        int r;
        dial_enable_req.lineoffset = GPIO_DIAL_ENABLE;
        dial_enable_req.handleflags = GPIOHANDLE_REQUEST_INPUT;
        dial_enable_req.eventflags = GPIOEVENT_REQUEST_BOTH_EDGES;

        dial_count_req.lineoffset = GPIO_DIAL_COUNT;
        dial_count_req.handleflags = GPIOHANDLE_REQUEST_INPUT;
        dial_count_req.eventflags = GPIOEVENT_REQUEST_RISING_EDGE;

        r = ioctl(gpio_fd, GPIO_GET_LINEEVENT_IOCTL, &dial_enable_req);
        if (r == -1) {
            throw std::runtime_error("failed to get ioctl for device");
        }

        r = ioctl(gpio_fd, GPIO_GET_LINEEVENT_IOCTL, &dial_count_req);
        if (r == -1) {
            throw std::runtime_error("failed to get ioctl for device");
        }
    }

    struct gpioevent_request dial_enable_req;
    struct gpioevent_request dial_count_req;
    std::string number_buffer;

    std::function<void (const std::string&)> number_entered_callback;
    std::function<void (int)> digit_entered_callback;

    int gpio_fd;
    int counted_value;
    std::thread enable_read_thd;
    std::thread count_read_thd;
    std::mutex enable_mutex;
    bool quit_read;
    bool enable_is_active;
    std::chrono::time_point<std::chrono::steady_clock> time_last_input;
};

dial::dial() 
: m_data(new Data()) {
}

dial::~dial() {

}

void dial::set_number_entered_callback(std::function<void (const std::string&)> callback) {
    m_data->number_entered_callback = callback;
}

void dial::set_digit_entered_callback(std::function<void (int)> callback) {
    m_data->digit_entered_callback = callback;
}
