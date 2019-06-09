#include "dial.hpp"
#include <libgpio.h>
#include <atomic>
#include <thread>
#include <functional>
#include <iostream>

#define GPIO_DIAL_PIN_ENABLE 22
#define GPIO_DIAL_PIN_SENSE 17

using namespace phoned;

struct dial::Data {
     gpio_handle_t handle;
     std::thread sense_thd;
     std::atomic<bool> quit;
     std::function<void (const std::string&)> num_cb;

     void init() {
	  handle = gpio_open(0);
	  if (handle == GPIO_INVALID_HANDLE) {
	       //err(1, "gpio_open failed");
	  }

	  gpio_pin_input(handle, GPIO_DIAL_PIN_ENABLE);
	  gpio_pin_pulldown(handle, GPIO_DIAL_PIN_ENABLE);
	  gpio_pin_input(handle, GPIO_DIAL_PIN_SENSE);
	  gpio_pin_pulldown(handle, GPIO_DIAL_PIN_SENSE);

	  sense_thd = std::thread(&Data::sense_function, this);
     }

     void sense_function() {
	  quit = false;
	  int num_count = 0;
	  std::chrono::time_point<std::chrono::steady_clock> start_time;
	  std::string number = "";
	  while(not quit) {
	       int enable_count = gpio_pin_get(handle, 17);
	       int num_pin = gpio_pin_get(handle, 22);
 
	       if (enable_count == 1) {
		    start_time = std::chrono::steady_clock::now();
		    if (num_pin == 0) {
			 std::this_thread::sleep_for(std::chrono::milliseconds(49));
			 if(gpio_pin_get(handle, 22) == 0) {
			      num_count++;
			 }
		    }
	       } else {
		    if (num_count > 0) {
			 if (num_count == 10) {
			      num_count = 0;
			 }
			 number += std::to_string(num_count);
		    }
		    num_count = 0;

		    auto end = std::chrono::steady_clock::now();
		    auto dt = std::chrono::duration_cast<std::chrono::seconds>(end - start_time);
		    if (dt.count() >= 3 && not number.empty()) {
			 num_cb(number);
			 number = "";
		    }
	       }
	       std::this_thread::sleep_for(std::chrono::milliseconds(10));
	  }
     }
};

dial::dial()
     : m_data(new Data()) {
     m_data->num_cb = [&](const std::string& number) {
			   on_number(number);
		      };
     m_data->init();
}

dial::~dial() {
     m_data->quit = true;
     if (m_data->sense_thd.joinable()) {
	  m_data->sense_thd.join();
     }
}
