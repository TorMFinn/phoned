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
        virtual bool Open(const std::string& serial_device, int baudrate) = 0;
        virtual void Dial(const std::string &number) = 0;
        virtual void Hangup() = 0;
        virtual void AnswerCall() = 0;

        virtual bool CallInProgress() = 0;
        virtual bool CallIncoming() = 0;
        virtual bool HasDialtone() = 0;

        void SetModemEventHandler(ModemEventHandler handler) {
            m_event_handler = handler;
        }

        protected:
        ModemEventHandler m_event_handler;
    };

    using ModemPtr = std::shared_ptr<Modem>;
}
