#pragma once
#include <string>
#include <stdint.h>
#include <functional>
#include <libgpio.h>

struct gpio_config {
     std::string gpiodevice;
     uint8_t pin_start_count;
     uint8_t pin_count;
};

class Dial {
     Dial(const gpio_config &cfg);
     ~Dial();

     void OnNumberReady(const std::function<void (const std::string&)> &callback);

private:
     std::function<void (const std::string&)> callback_;
     gpio_handle_t gpio_handle_;
     bool configure_gpio(const gpio_config &cfg);
};
