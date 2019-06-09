#pragma once
#include <memory>
#include <string>

namespace phoned {
     class audio {
     public:
         audio(const std::string &audio_path);
	  ~audio();

	  void start_dialtone();
	  void pause_dialtone();
	  void stop_dialtone();

     private:
	  struct Data;
	  std::unique_ptr<Data> m_data;
     };
}
