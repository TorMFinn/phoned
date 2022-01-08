#pragma once
#include <memory>
#include <functional>

namespace phoned {

    enum class ModemEvent {
        CALL_INCOMING,
        CALL_MISSED,
        CALL_ENDED,
        CALL_STARTED,
        SMS_RECEIVED,
        MMS_RECEIVED
    };

    struct CallEventData {
        std::string number;
    };

    using ModemEventHandler = std::function<void (ModemEvent, void*)>;

    class Modem {
        public:
        Modem(const std::string &serial_device, int baudrate);
        ~Modem();

        void Dial(const std::string &number);
        void Hangup();
        void AnswerCall();

        bool CallInProgress();
        bool CallIncoming();
        bool HasDialtone();

        void SetModemEventHandler(ModemEventHandler handler);

        private:
        struct Data;
        std::unique_ptr<Data> m_data;
    };
}
