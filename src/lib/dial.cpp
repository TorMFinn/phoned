#include "phoned/dial.hpp"
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <linux/gpio.h>
#include <mutex>
#include <stdexcept>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <syslog.h>
#include <thread>
#include <unistd.h>

using namespace phoned;

const int GPIO_DIAL_ENABLE = 17;
const int GPIO_DIAL_COUNT = 27;

using namespace std::chrono;

struct Dial::Data {

  ~Data() {
    quit_read = true;
    if (enable_read_thd.joinable()) {
      enable_read_thd.join();
    }
    if (count_read_thd.joinable()) {
      count_read_thd.join();
    }
  }

  Data() {
    init_gpio();

    enable_read_thd = std::thread([this]() {
      while (not quit_read) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        auto enable_count_state = read_enable_raw();
        if (enable_count_state) {
          do_counting = true;
        } else if (!enable_count_state && do_counting) {
          do_counting = false;
          std::cout << "counted value: " << counted_digit << std::endl;
          update_number(counted_digit);
          time_since_last_digit = steady_clock::now();
          counted_digit = 0;
        } else if (!enable_count_state) {
          if (duration_cast<seconds>(steady_clock::now() -
                                     time_since_last_digit) >= 4s) {
            finish_number();
          }
        }
      }
    });

    count_read_thd = std::thread([this]() {
      bool current_pin_state = false;
      bool prev_pin_state = false;
      bool dial_state = true;
      bool prev_dial_state = false;
      time_point<high_resolution_clock> time_last_change;
      while (not quit_read) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        current_pin_state = read_count_raw();
        if (current_pin_state != prev_pin_state) {
          time_last_change = high_resolution_clock::now();
        }

        if (duration_cast<milliseconds>(high_resolution_clock::now() -
                                        time_last_change) > 5ms) {
          if (dial_state != current_pin_state) {
            dial_state = current_pin_state;
            if (dial_state) {
              if (do_counting) {
                counted_digit++;
              }
            }
          }
        }

        prev_pin_state = current_pin_state;
      }
    });
  }

  void update_number(int digit) {
    if (digit == 0) {
      // Not valid input
      // means we activated count but did not do a digit entry
      return;
    } else if (digit == 10) {
      // 10 counts represents the 0 on the dial.
      digit = 0;
    }
    std::thread([&, digit]() {
      DigitEnteredData data;
      data.digit = digit;

      if (dial_event_handler) {
        dial_event_handler(DialEvent::DIGIT_ENTERED, std::move(&data));
      }
    }).detach();
    number_buffer += std::to_string(digit);
  }

  void finish_number() {
    if (!number_buffer.empty()) {
      std::cout << "number is: " << number_buffer << std::endl;
      NumberEnteredData data;
      data.number = number_buffer;
      if (dial_event_handler) {
        dial_event_handler(DialEvent::DIGIT_ENTERED, std::move(&data));
      }
      number_buffer.clear();
    }
  }

  bool debounce_low() {
    int low_counts = 0;
    for (int i = 0; i < 10; i++) {
      if (!read_count_raw()) {
        low_counts++;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    // std::cout << "low counts: " << low_counts << std::endl;
    if (low_counts >= 1) {
      return true;
    }
    return false;
  }

  bool debounce_high() {
    int high_counts = 0;
    for (int i = 0; i < 10; i++) {
      if (read_count_raw()) {
        high_counts++;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    // std::cout << "high counts" << high_counts << std::endl;
    if (high_counts > 5) {
      return true;
    }
    return false;
  }

  bool read_count_raw() {
    struct gpiohandle_data data;
    int r = ioctl(dial_count_req.fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data);
    if (r < 0) {
      syslog(LOG_ERR, "DIAL: failed to read count pin: %s", std::strerror(-r));
      return false;
    }
    // Active Low
    return data.values[0] == 0;
  }

  bool read_enable_raw() {
    struct gpiohandle_data data;
    int r = ioctl(dial_enable_req.fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data);
    if (r < 0) {
      syslog(LOG_ERR, "DIAL: failed to read count pin: %s", std::strerror(-r));
      return false;
    }
    // Active Low
    return data.values[0] == 0;
  }

  void init_gpio() {
    gpio_fd = open("/dev/gpiochip0", 0);
    if (gpio_fd == -1) {
      throw std::runtime_error(std::string("failed to open gpiochip0: ") +
                               std::strerror(errno));
    }

    int r;
    dial_enable_req.lineoffset = GPIO_DIAL_ENABLE;
    dial_enable_req.handleflags = GPIOHANDLE_REQUEST_INPUT;
    dial_enable_req.eventflags = GPIOEVENT_REQUEST_BOTH_EDGES;

    dial_count_req.lineoffset = GPIO_DIAL_COUNT;
    dial_count_req.handleflags = GPIOHANDLE_REQUEST_INPUT;
    dial_count_req.eventflags = GPIOEVENT_REQUEST_BOTH_EDGES;

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
  time_point<steady_clock> time_since_last_digit;

  DialEventHandler dial_event_handler;

  int gpio_fd;
  int count_pin_fd;
  int counted_digit;
  std::thread enable_read_thd;
  std::thread count_read_thd;
  bool quit_read;
  bool do_counting;
};

Dial::Dial() : m_data(new Data()) {}

Dial::~Dial() {}

void Dial::SetDialEventHandler(DialEventHandler handler) {
  m_data->dial_event_handler = handler;
}
