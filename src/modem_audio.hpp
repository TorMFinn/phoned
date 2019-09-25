#pragma once
#include <string>
#include <memory>

namespace phoned {

  class modem_audio {
  public:
    modem_audio();
    ~modem_audio();

    // Opens the serial port for audio read/write.
    bool init_audio(const std::string &port, int baudrate);

    // Starts transferring audio towards the serial device
    void start_audio_transfer();

    // Stops the audio transfer
    void stop_audio_transfer();

  private:
    struct Data;
    std::unique_ptr<Data> m_data;
  };
}
