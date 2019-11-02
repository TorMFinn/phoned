#pragma once
#include <memory>
#include <functional>

namespace phoned {
    class dial {
        public:
        dial();
        ~dial();

        void set_number_entered_callback(std::function<void (const std::string&)> callback);
        void set_digit_entered_callback(std::function<void (int)> callback);

        private:
        struct Data;
        std::unique_ptr<Data> m_data;
    };
}