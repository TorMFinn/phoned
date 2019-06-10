#include "ringer.hpp"
#include <libgpio.h>
#include <iostream>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <mutex>

#define GPIO_RINGER_ENABLE 3

using namespace phoned;

struct ringer::Data {
     gpio_handle_t handle;
     std::atomic<bool> quit;
     std::atomic<bool> ring;
     std::thread ring_thd;
     std::mutex ring_mutex;
     std::condition_variable ring_cv;

     void init() {
	  handle = gpio_open(0);
	  if (handle == GPIO_INVALID_HANDLE) {
	       std::cerr << "Failed to open gpio"
			 << std::endl;
	  }

	  gpio_pin_output(handle, GPIO_RINGER_ENABLE);
	  gpio_pin_pulldown(handle, GPIO_RINGER_ENABLE);

	  ring_thd = std::thread(&Data::ringer_thd_function, this);
     }

     void ringer_thd_function() {
	  quit = false;
	  while(not quit) {
	       std::cout << "waiting" << std::endl;
	       std::unique_lock<std::mutex> lock(ring_mutex);
	       ring_cv.wait(lock);
	       std::cout << "Awoke" << std::endl;
	       if (ring) {
		    while(ring) {
			 std::cout << "ringing" << std::endl;
			 int val = gpio_pin_get(handle, GPIO_RINGER_ENABLE);
			 gpio_pin_set(handle, GPIO_RINGER_ENABLE, -val);
			 std::this_thread::sleep_for(std::chrono::milliseconds(500));
		    }
	       }
	       lock.unlock();
	       std::cout << "unlocked" << std::endl;
	  }
     }
};

ringer::ringer()
     : m_data(new Data()) {
     m_data->init();
}

ringer::~ringer() {
     stop_ring();
     m_data->ring_cv.notify_one();
     m_data->quit = true;
     if(m_data->ring_thd.joinable()) {
	  m_data->ring_thd.join();
     }
}

void ringer::start_ring() {
     m_data->ring = true;
     m_data->ring_cv.notify_one();
}

void ringer::stop_ring() {
     m_data->ring = false;
}



