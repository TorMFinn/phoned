#include "dial.hpp"
#include <sys/types.h>
#include <libgpio.h>
#include <exception>

Dial::Dial(const gpio_config &cfg) {
    if(not configure_gpio(cfg)) {
        throw std::runtime_error("could not configure gpio");
    }
}

Dial::~Dial() {
     if (gpio_handle_ != GPIO_INVALID_HANDLE) {
	  gpio_close(gpio_handle_);
     }
}

bool Dial::configure_gpio(const gpio_config &cfgconst gpio_config &cfg) {
     gpio_handle_ = kk

     ;
}

void Dial::OnNumberReady(const std::function<void (std::string)> &callback) {
     callback_ = callback;
}
