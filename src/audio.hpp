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

	  void start_rx_line();
	  void stop_rx_line();

	  void start_tx_line();
	  void stop_tx_line();

     private:
	  struct Data;
	  std::unique_ptr<Data> m_data;
     };
}
