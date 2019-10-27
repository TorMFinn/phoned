#pragma once
#include <memory>
#include <functional>

namespace phoned {
    class handset {
        enum class handset_state {
            lifted,
            down
        };
    public:
        handset();
        ~handset();

        void set_handset_state_changed_callback(std::function<void (handset_state)> callback);

    private:
        struct Data;
        std::unique_ptr<Data> m_data;
    };
}
