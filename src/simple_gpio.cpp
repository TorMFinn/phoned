#include <sys/types.h>
#include <err.h>
#include <libgpio.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <string>

int main() {
     gpio_handle_t handle;
     handle = gpio_open(0);

     if (handle == GPIO_INVALID_HANDLE) {
	  err(1, "gpio_open failed");
     }

     // Configure pins as input
     gpio_pin_input(handle, 22);
     gpio_pin_pulldown(handle, 22);
     gpio_pin_input(handle, 17);
     gpio_pin_pulldown(handle, 17);

     int num_count = 0;
     std::chrono::time_point<std::chrono::steady_clock> start_time;
     std::string number = "";
     while(1) {
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
		    std::cout << number << std::endl;
		    number = "";
	       }
	  }
	  std::this_thread::sleep_for(std::chrono::milliseconds(10));
     }

    return 0;
}
