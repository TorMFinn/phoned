#pragma once
#include <functional>
#include <memory>

namespace phoned {

enum class HandsetState
{
    lifted,
    down
};

class Handset {
  public:
    Handset();
    ~Handset();

    using HandsetStateChangedHandler = std::function<void(HandsetState)>;

    void SetHandsetStateChangedHandler(HandsetStateChangedHandler handler);

    HandsetState get_state();

  private:
    struct Data;
    std::unique_ptr<Data> m_data;
};
} // namespace phoned
