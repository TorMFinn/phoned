#pragma once
#include <memory>

namespace phoned {
    class voice {
    public:
        voice();

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
}