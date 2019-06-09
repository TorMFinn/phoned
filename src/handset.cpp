// GPIO Numbers
#define HANDSET_SWITCH_FEED 4
#define HANDSET_SWITCH_READ 14

#include "handset.hpp"
#include <libgpio.h>
#include <thread>
#include <atomic>
#include <iostream>
#include <sys/types.h>
#include <err.h>
#include <functional>

using namespace phoned;

struct handset::Data {
     gpio_handle_t handle;
     std::thread m_thd;
     std::atomic<bool> quit;
     std::function<void (int state)> state_cb;
     bool old_state = false;
     bool initial_read = true;

     ~Data() {
	  quit = true;
	  if (m_thd.joinable()) {
	       m_thd.join();
	  }
     }

     void init() {
	  handle = gpio_open(0);
	  if (handle == GPIO_INVALID_HANDLE) {
	       err(1, "gpio_open failed");
	  }

	  gpio_pin_output(handle, HANDSET_SWITCH_FEED);
	  gpio_pin_pullup(handle, HANDSET_SWITCH_FEED);
	  // Set feed high and let it remain like such.
	  gpio_pin_set(handle, HANDSET_SWITCH_FEED, 1);

	  gpio_pin_input(handle, HANDSET_SWITCH_READ);
	  gpio_pin_pulldown(handle, HANDSET_SWITCH_READ);

	  m_thd = std::thread(&handset::Data::read_thd, this);
     }

     void read_thd() {
	  quit = false;
	  while(not quit) {
	       std::this_thread::sleep_for(std::chrono::milliseconds(50));
	       bool state = static_cast<bool>(gpio_pin_get(handle, HANDSET_SWITCH_READ));

	       if (state != old_state) {
		    state_cb(state);
	       }
	       /*
	       else if (initial_read) {
		    state_cb(state);
		    initial_read = false;
	       }
	       */
	       old_state = state;
	  }
     }
};

handset::handset()
     : m_data(new Data()) {
     m_data->init();

     m_data->state_cb = [&](bool state) {
			     state_changed(state);
			};
}

handset::~handset() {
     ;
}
