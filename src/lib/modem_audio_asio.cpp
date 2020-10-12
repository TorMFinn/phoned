#include "phoned/audio.hpp"
#include "phoned/modem_audio.hpp"
#define BOOST_ASIO_DISABLE_CONCEPTS
#include <boost/asio/io_context.hpp>
#include <boost/asio/serial_port.hpp>

#include <pulse/simple.h>
#include <pulse/error.h>

#include <thread>
#include <iostream>
#include <syslog.h>

using namespace phoned;

struct modem_audio::Data {
    // Boost Asio stash
    boost::asio::io_context io_ctx;
    boost::asio::serial_port serial_port;
    std::thread io_thd;

    std::string serial_device;
    int baudrate;

    phoned::audio audio;

    Data()
        : serial_port(io_ctx) {
        io_thd = std::thread([&](){
            // Work guard, so that run() does not quit before doing first task
            boost::asio::io_context::work work(io_ctx);
            io_ctx.run();
        });

        audio.set_audio_sample_callback([&](const std::vector<uint8_t> &sample) {
            if (write_data(sample) > 0) {
                std::cout << "Wrote audio to modem" << std::endl;
            }
        });

        audio.start();
    }

    ~Data() {
        if (io_thd.joinable()) {
            io_ctx.stop();
            io_thd.join();
        }
    }

    bool open_serial_port(){
        try {
            std::cout << "Opening serial port" << std::endl;
            serial_port.open(serial_device);
            serial_port.set_option(boost::asio::serial_port_base::baud_rate(baudrate));
            start_receive();
            std::cout << "Port opened" << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "Failed to open serial port: " << e.what() << std::endl;
            return false;
        }
        return true;
    }

    void disconnect() {
        serial_port.close();
    }

    void start_receive() {
        uint8_t recv_buf[audio.get_sample_buffer_size()];
        serial_port.async_read_some(boost::asio::buffer(recv_buf, audio.get_sample_buffer_size()),
                                    [&](const boost::system::error_code &err, std::size_t bytes_received) {
            if (err) {
                syslog(LOG_ERR, "ModemAudio: failed to read from port: %s", err.message().c_str());
                std::cerr << "failed to read from port: " << err.message() << std::endl;
                // Disconnect port
                return;
            }

            std::cout << "received data from modem: " << bytes_received << std::endl;
            audio.write_sample(std::vector<uint8_t>(recv_buf, recv_buf + audio.get_sample_buffer_size()));
            start_receive();
        });
    }

    ssize_t write_data(const std::vector<uint8_t> &data) {
        if (serial_port.is_open()) {
            return serial_port.write_some(boost::asio::buffer(data));
        }
        return 0;
    }
};


modem_audio::modem_audio(const std::string &serial_device, int baudrate)
    : m_data(new Data()) {
    m_data->serial_device = serial_device;
    m_data->baudrate = baudrate;
}

modem_audio::~modem_audio() = default;

void modem_audio::start_voice_transfer() {
    if (m_data->serial_port.is_open() == false) {
        m_data->open_serial_port();
    }
}

void modem_audio::stop_voice_transfer() {
    m_data->disconnect();
}

void modem_audio::start_transfer() {
    start_voice_transfer();
}

void modem_audio::stop_transfer() {
    stop_voice_transfer();
}
