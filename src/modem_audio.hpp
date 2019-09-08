#pragma once
#include <string>
#include <memory>

namespace phoned {

  struct serial_connection_info {
    std::string device;
    int baudrate;
  };

  class modem_audio {
  public:
    modem_audio(const serial_connection_info &info);
    ~modem_audio();

    // Starts transferring audio towards the serial device
    void start_audio_transfer();

    // Stops the audio transfer
    void stop_audio_transfer();

  private:
    struct Data;
    std::unique_ptr<Data> m_data;
  };
}
