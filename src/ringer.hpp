/**
 * Creates an audio signal to the onboard buzzer
 */

#pragma once
#include <memory>

namespace phoned {
     class ringer {
     public:
	  ringer();
	  ~ringer();

	  void start_ring();
	  void stop_ring();

     private:
	  struct Data;
	  std::unique_ptr<Data> m_data;
     };
}
