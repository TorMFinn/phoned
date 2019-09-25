#include "modem.hpp"
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <vector>
#include <mutex>
#include <thread>
#include <poll.h>
#include <atomic>

#include <boost/asio.hpp>
#include <boost/array.hpp>

using namespace phoned;

struct modem::Data {
    boost::asio::io_context io_ctx;
    std::unique_ptr<boost::asio::serial_port> port;
    std::thread io_thd;
    boost::array<char, 1024> rcv_buf;
    bool initial_call = true;

    void start_receive() {
        // Init async reader
        port->async_read_some(boost::asio::buffer(rcv_buf), std::bind(&Data::read_handler, this, std::placeholders::_1, std::placeholders::_2));
    }

    void open_port(const std::string &location) {
        port = std::make_unique<boost::asio::serial_port>(io_ctx, location);
        // The terminal is raw by default.
        port->set_option(boost::asio::serial_port_base::baud_rate(921600));
        start_receive();
    }

    int parse_cpas(const std::string &rsp) {
        ;
    }

    void read_handler(const boost::system::error_code &error, std::size_t bytes_transferred) {
        std::cout << std::string(rcv_buf.data(), bytes_transferred) << std::endl;
        start_receive();
    }
};

modem::modem(const std::string &at_port_path)
    : m_data(new Data()) {
    m_data->open_port(at_port_path);
    m_data->io_thd = std::thread([&]() {
                                     m_data->io_ctx.run();
                                 });
}

modem::~modem() {
    if (m_data->io_thd.joinable()) {
        m_data->io_ctx.stop();
        m_data->port->close();
        m_data->io_thd.join();
    }
}

void modem::dial(const std::string &number) {
    m_data->port->write_some(boost::asio::buffer("ATD"+number+";\r"));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    //if (m_data->initial_call) {
    m_data->port->write_some(boost::asio::buffer("AT+CPCMREG=1\r"));
     //   m_data->initial_call = false;
    //}
}

void modem::hangup() {
    m_data->port->write_some(boost::asio::buffer("AT+CHUP\r"));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    m_data->port->write_some(boost::asio::buffer("AT+CPCMREG=0\r"));
}

void modem::answer_call() {

}

bool modem::has_dialtone() {
    boost::array<char, 32> rcv_buf;
    m_data->port->write_some(boost::asio::buffer("AT+CPAS\r"));
    //size_t transfered = m_data->port->read_some(boost::asio::buffer(rcv_buf));
    //std::cout << "RSP: " << std::string(rcv_buf.data(),  transfered) << std::endl;
    return true;
}
