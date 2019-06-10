/**
 * Controls the state of the handset
 */

#pragma once

#include <boost/signals2.hpp>
#include <memory>

namespace phoned {

     class handset {
     public:
	  handset();
	  ~handset();

	  boost::signals2::signal<void (bool)> state_changed;

	  bool is_handset_down();
     private:
	  struct Data;
	  std::unique_ptr<Data> m_data;
     };
}
     
