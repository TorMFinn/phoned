#pragma once
#include <memory>

namespace phoned {
class dialtone {
public:
  dialtone();
  ~dialtone();

  // Start the dialtone signal
  void start();

  // Stop the dialtone signal
  void stop();

private:
  struct Data;
  std::unique_ptr<Data> m_data;
};
} // namespace phoned
