#pragma once
#include <memory>
#include <functional>

namespace phoned::dbus {
    class audiod {
    public:
        audiod();
        ~audiod();

        void set_dialtone_start_callback(std::function<void ()> callback);
        void set_dialtone_stop_callback(std::function<void ()> callback);

        void set_start_audio_transfer_callback(std::function<void ()> callback);
        void set_stop_audio_transfer_callback(std::function<void ()> callback);

    private:
        struct Data;
        std::unique_ptr<Data> m_data;
    };
}
