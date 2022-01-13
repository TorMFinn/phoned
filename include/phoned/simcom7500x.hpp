#pragma once
#include "phoned/modem.hpp"

namespace phoned {
class SIMCOM7500X : public Modem {
  public:
    SIMCOM7500X();
    virtual ~SIMCOM7500X();

    bool Open(const std::string &serial_device, int baudrate) override;
    void Dial(const std::string &number) override;
    void Hangup() override;
    void AnswerCall() override;

    bool CallInProgress() override;
    bool CallIncoming() override;
    bool HasDialtone() override;

    private:
    struct Data;
    std::unique_ptr<Data> m_data;
};
} // namespace phoned