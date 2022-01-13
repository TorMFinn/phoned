#include "phoned/simcom7500x.hpp"
#include <boost/asio.hpp>

#include <array>
#include <iostream>

using namespace boost::asio;

namespace phoned {
struct SIMCOM7500X::Data {
    boost::asio::io_context io_ctx;
    boost::asio::serial_port serial_port;

    std::array<char, 256> read_buffer;

    std::function<void(const std::string &)> read_handler;
    ModemEventHandler *event_handler;

    Data() : serial_port(io_ctx) {
        boost::asio::io_context::work work(io_ctx);
        io_ctx.run();
    }

    void start_reading() {
        serial_port.async_read_some(boost::asio::buffer(read_buffer),
                                    [&](const boost::system::error_code &error, std::size_t bytes_read) {
                                        if (error) {
                                            std::cerr << "failed to read from port: " << error.message() << std::endl;
                                        }

                                        std::string modem_output(read_buffer.data(), read_buffer.data() + bytes_read);
                                        std::cout << "read: " << modem_output << " from modem" << std::endl;

                                        // Check if there is some event that has been received.
                                        check_for_event(modem_output);

                                        // If a command has been sent, we call the read handler.
                                        if (read_handler) {
                                            read_handler(modem_output);
                                        }

                                        // Do next read.
                                        start_reading();
                                    });
    }

    void check_for_event(const std::string &msg) {
        if (msg.find("CALL_INCOMING") != std::string::npos) {
            event_handler->operator()(ModemEvent::CALL_INCOMING, nullptr);
        } else if (msg.find("CALL_MISSED") != std::string::npos) {
            event_handler->operator()(ModemEvent::CALL_MISSED, nullptr);
        } else if (msg.find("CALL_STARTED") != std::string::npos) {
            event_handler->operator()(ModemEvent::CALL_STARTED, nullptr);
        }
    }
};

SIMCOM7500X::SIMCOM7500X() : m_data(new Data()) {
    m_data->event_handler = &m_event_handler;
}

SIMCOM7500X::~SIMCOM7500X() = default;

bool SIMCOM7500X::Open(const std::string &serial_device, int baudrate) {
    try {
        m_data->serial_port.open(serial_device);

        // Asio configures port as RAW by default, so no need to set
        // extra options.
        m_data->serial_port.set_option(serial_port_base::baud_rate(baudrate));
        m_data->start_reading();
    } catch (boost::system::system_error error) {
        std::cerr << "Failed to open serial port: " << error.what();
        return false;
    }

    return true;
}

void SIMCOM7500X::Dial(const std::string &number) {
    const std::string cmd = "ATD" + number + ";";
    boost::asio::write(m_data->serial_port, boost::asio::buffer(cmd));
}

void SIMCOM7500X::Hangup() {
    const std::string cmd = "ATH\r\n";
    boost::asio::write(m_data->serial_port, boost::asio::buffer(cmd));
}

void SIMCOM7500X::AnswerCall() {
    const std::string cmd = "ATA\r\n";
    boost::asio::write(m_data->serial_port, boost::asio::buffer(cmd));
}

bool SIMCOM7500X::CallInProgress() {
    return false;
}

bool SIMCOM7500X::CallIncoming() {
    return false;
}

bool SIMCOM7500X::HasDialtone() {
    return true;
}

} // namespace phoned