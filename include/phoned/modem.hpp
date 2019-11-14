#pragma once
#include <memory>
#include <functional>

namespace phoned {
    class modem {
        public:
        modem(const std::string &serial_device, int baudrate);
        ~modem();

        void dial(const std::string &number);
        void hangup();
        void answer_call();

        bool call_in_progress();
        bool call_incoming();
        bool has_dialtone();

        void set_call_incoming_handler(std::function <void ()> handler);
        void set_call_missed_handler(std::function <void ()> handler);
        void set_call_ended_handler(std::function <void ()> handler);
        void set_call_started_handler(std::function <void ()> handler);
        void set_voice_call_begin_handler(std::function<void ()> handler);
        void set_voice_call_end_handler(std::function<void ()> handler);

        private:
        struct Data;
        std::unique_ptr<Data> m_data;
    };
}