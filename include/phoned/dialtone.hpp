#include <memory>

namespace phoned {
class Dialtone {
public:
  Dialtone();
  ~Dialtone();

  // Start the dialtone signal
  void Start();

  // Stop the dialtone signal
  void Stop();

private:
  struct Data;
  std::unique_ptr<Data> m_data;
};
} // namespace phoned
