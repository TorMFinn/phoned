#pragma once
#include <functional>
#include <memory>

namespace phoned {

enum class DialEvent { NUMBER_ENTERED, DIGIT_ENTERED };

struct NumberEnteredData {
  std::string number;
};

struct DigitEnteredData {
  int digit;
};

using DialEventHandler = std::function<void(DialEvent, void *)>;

class Dial {
public:
  Dial();
  ~Dial();

  void SetDialEventHandler(DialEventHandler handler);

private:
  struct Data;
  std::unique_ptr<Data> m_data;
};
} // namespace phoned
