#pragma once
#include <string>
#include <memory>

namespace phoned {
    class modem_audio {
        public:
        modem_audio(const std::string &serial_device, int baudrate);
        ~modem_audio();

        void start_transfer();
        void stop_transfer();

        private:
        struct Data;
        std::unique_ptr<Data> m_data;
    };
}