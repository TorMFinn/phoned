/**
 * Modem driver for calling, SIM7500E
 */

#pragma once
#include <string>
#include <memory>
#include <boost/signals2.hpp>

namespace phoned {
    class modem {
    public:
        modem(const std::string &at_port_path);
        ~modem();

        boost::signals2::signal<void ()> incoming_call;
        //boost::signals2::signal<void ()> call_finnished;

        bool dial(const std::string &number);

        /** 
         * Hangup the current call
         */
        void hangup();

        /**
         * answers an incomming call
         */
        bool answer_call();

    private:
        struct Data;
        std::unique_ptr<Data> m_data;
    };
}

