#pragma once
#include <memory>
#include <string>

namespace phoned {
     class audio {
     public:
         audio(const std::string &audio_path);
	  ~audio();

	  void start_dialtone();
	  void stop_dialtone();

	  void start_audio_transfer();
	  void stop_audio_transfer();

     private:
	  struct Data;
	  std::unique_ptr<Data> m_data;
     };
}
