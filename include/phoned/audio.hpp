#pragma once
#include <memory>
#include <string>

namespace phoned {
    class audio {
    public:
        audio();
        ~audio();

        bool start_transfer(const std::string &device_name, int baudrate);
        void stop_transfer();
    private:
        struct Data;
        std::unique_ptr<Data> m_data;
    };
}
