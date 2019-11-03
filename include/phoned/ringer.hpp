#pragma once
#include <memory>

namespace phoned {
    class ringer {
        public:
        ringer();
        ~ringer();

        void start();
        void stop();

        private:
        struct Data;
        std::unique_ptr<Data> m_data;
    };
}