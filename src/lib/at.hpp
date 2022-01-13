#pragma once
#include <optional>
#include <string>
#include <vector>

namespace phoned {

static std::vector<std::string> parse_buffer;
using ATCommandArgs = std::vector<std::string>;

enum class ATCommand
{
	INCOMING

};

struct ATCommandData {
    ATCommand cmd;
    ATCommandArgs args;
};

class ATParser {
  public:
    ATParser();
    virtual ~ATParser();

    std::optional<ATCommand> ParseChar(char ch);

  private:
    std::vector<std::string> m_parse_buffer;
};

} // namespace phoned