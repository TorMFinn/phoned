#pragma once
#include <memory>

namespace phoned {
     class audio {
     public:
	  audio();
	  ~audio();

	  void start_dialtone();
	  void pause_dialtone();
	  void stop_dialtone();

     private:
	  struct Data;
	  std::unique_ptr<Data> m_data;
     };
}
