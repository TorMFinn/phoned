#pragma once
#include <memory>
#include <boost/signals2.hpp>

namespace phoned {
     class dial {
     public:
	  dial();
	  ~dial();

	  boost::signals2::signal<void (const std::string&)> on_number;

     private:
	  struct Data;
	  std::unique_ptr<Data> m_data;
     };
}
