#pragma once

#include <memory>
#include <string>

struct serial_connection_info {
     std::string port_path;
     std::string baudrate;
};

class audio_sndio {
public:
     audio_sndio(const serial_connection_info& info);
     ~audio_sndio();

     void start_audio_transfer();
     void stop_audio_transfer();

private:
     struct Data;
     std::unique_ptr<Data> m_data;
};
