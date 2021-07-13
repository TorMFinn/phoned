#include "phoned/modem.hpp"
#include <unistd.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/gpio.h>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <cstring>
#include <thread>
#include <syslog.h>
#include <mutex>

using namespace phoned;

const int BUFSIZE = 1024;
const int MODEM_PWR_PIN = 6;
const int MODEM_FLIGHT_MODE_PIN = 4;

struct modem::Data {
    Data(const std::string &serial_device, int baudrate) {
        init_serial(serial_device, baudrate);
        init_gpio();
        power_modem();

        read_thd = std::thread([this]() {
            int ret;
            struct pollfd fds[1];
            fds[0].fd = serial_fd;
            fds[0].events = POLLIN;

            do_read = true;
            char buf[BUFSIZE];
            while(do_read) {
                ret = poll(fds, 1, 500);
                if (ret > 0) {
                    if (fds[0].revents & POLLIN) {
                        size_t bytes_read;
                        {
                            std::scoped_lock lock(port_mutex);
                            bytes_read = read(serial_fd, buf, BUFSIZE);
                        }
                        if (bytes_read < 0) {
                            syslog(LOG_ERR, "MODEM: error reading from file descriptor: %s", std::strerror(errno));
                            break;
                        } else if (bytes_read > 0) {
                            auto msg = std::string(buf, bytes_read);
                            msg.erase(std::remove(msg.begin(), msg.end(), '\n'), msg.end());
                            if (not msg.empty()) {
                                handle_message(msg);
                            }
                        }
                    }
                }
            }
        });
    }

    ~Data() {
        do_read = false;
        if (read_thd.joinable()) {
            read_thd.join();
        }
        close(serial_fd);
    }

    void init_gpio() {
        gpio_fd = open("/dev/gpiochip0", 0);
        if (gpio_fd == -1) {
            throw std::runtime_error(
                std::string("failed to open gpiochip0: ") + std::strerror(errno));
        }

        modem_pwr_ctrl.lineoffset = MODEM_PWR_PIN;
        modem_pwr_ctrl.handleflags = GPIOHANDLE_REQUEST_OUTPUT;
        strcpy(modem_pwr_ctrl.consumer_label, "phoned-modemd");
        modem_flight_ctrl.lineoffset = MODEM_FLIGHT_MODE_PIN;
        modem_flight_ctrl.handleflags = GPIOHANDLE_REQUEST_OUTPUT;
        strcpy(modem_flight_ctrl.consumer_label, "phoned-modemd");
        int r = ioctl(gpio_fd, GPIO_GET_LINEHANDLE_IOCTL, &modem_pwr_ctrl);
        if (r < 0) {
            throw std::runtime_error(
                std::string("failed to get modem_pwr linehandle: ") +
                std::strerror(errno));
        }
        r = ioctl(gpio_fd, GPIO_GET_LINEHANDLE_IOCTL, &modem_flight_ctrl);
        if (r < 0) {
            throw std::runtime_error(
                std::string("failed to get modem_flight linehandle: ") +
                std::strerror(errno));
        }
    }

    void power_modem() {
        struct gpiohandle_data pwr_data;
        struct gpiohandle_data flightmode_data;
        int r = ioctl(modem_pwr_ctrl.fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &pwr_data);
        if (r < 0) {
            std::cerr << "Failed to get line value modem_pwr" << std::endl;
        }
        // Setting low enables the modem
        pwr_data.values[0] = 0;
        r = ioctl(modem_pwr_ctrl.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &pwr_data);
        if (r < 0) {
            std::cout << "failed to set modem pwr ioctl" << std::endl;
        } else {
            std::cout << "enabled modem pwr" << std::endl;
        }

        r = ioctl(modem_flight_ctrl.fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &flightmode_data);
        if (r < 0) {
            std::cerr << "Failed to get line value flightmode" << std::endl;
        }

        flightmode_data.values[0] = 0;
        r = ioctl(modem_flight_ctrl.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &flightmode_data);
        if (r < 0) {
            std::cout << "failed to set modem flightmode ioctl" << std::endl;
        } else {
            std::cout << "disabled flightmode" << std::endl;
        }

        while(!write_command("AT\r\n", "OK"));
        std::cout << "Modem powered" << std::endl;
    }

    bool write_command(std::string cmd, std::string expected_response) {
        int bytes_written = write(serial_fd, cmd.c_str(), cmd.size());
        if (bytes_written < 0) {
            std::cerr << "failed to write: " << cmd << std::strerror(errno) << std::endl;
            return false;
        }
        
        char buf[256];
        int bytes_read = read(serial_fd, &buf, 256);
        if (bytes_read > 0) {
            std::string rsp(buf, bytes_read);
            if (rsp.find(expected_response) != rsp.npos) {
                return true;
            }
        } else {
            std::cerr << "failed to read: " << std::strerror(errno) << std::endl;
        }
        return false;
    }

    void handle_message(const std::string &msg) {
        syslog(LOG_DEBUG, "MODEM: AT Receive: %s", msg.c_str());
        if (msg == "OK") {
            return;
        }

        if (msg.find("+CRING: VOICE") != msg.npos || msg.find("RING") != msg.npos) {
            try {
                call_is_incoming = true;
                call_incoming_handler();
            } catch (const std::bad_function_call) {
                syslog(LOG_ERR, "MODEM: no call_incoming handler is set");
            }
        } else if (msg.find("MISSED_CALL") != msg.npos) {
            try {
                call_in_progress = false; // New, may not need this?
                call_is_incoming = false;
                call_missed_handler();
            } catch (const std::bad_function_call) {
                syslog(LOG_ERR, "MODEM: no call_missed handler is set");
            }
        } else if (msg.find("BUSY") != msg.npos) {
            call_in_progress = false;
            call_ended_handler();
        } else if (msg.find("VOICE CALL: END") != msg.npos) {
            try {
                // Stops writing during the handler and allows the modem to be able to flush its buffers
                call_in_progress = false;
                call_is_incoming = false;
                call_ended_handler();
                voice_end_handler();
            } catch (const std::bad_function_call) {
                syslog(LOG_ERR, "MODEM: warning, no call_ended handler is set");
            }
        } else if (msg.find("VOICE CALL: BEGIN") != msg.npos) {
            try {
                call_in_progress = true;
                call_started_handler();
                voice_begin_handler();
            } catch (const std::bad_function_call) {
                syslog(LOG_ERR, "MODEM: no call_started handler is set");
            }
        }
    }

    bool dial_number(const std::string &number) {
        const std::string cmd = std::string("ATD" + number + ";\r\n");
        return write_command(cmd) > 0;
    }

    ssize_t write_command(const std::string &command) {
        // Sleep between commands as reccomended by SIMCOM.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::scoped_lock lock(port_mutex);
        syslog(LOG_DEBUG, "MODEM: Writing command: %s", command.c_str());
        int bytes_written = write(serial_fd, command.c_str(), command.size());
        if (bytes_written <= 0) {
            syslog(LOG_ERR, "MODEM: failed to write command: %s", std::strerror(errno));
        }
        return bytes_written;
    }

    void init_serial(const std::string &serial_device, int baudrate) {
        int ret;
        serial_fd = open(serial_device.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
        if (serial_fd == -1) {
            throw std::runtime_error(std::string("failed to open serial device: ") + std::strerror(errno));
        }

        struct termios config;
        cfmakeraw(&config);
        ret = cfsetspeed(&config, baudrate);
        if (ret < 0) {
            throw std::runtime_error(std::string("failed to set baudrate: ") + std::strerror(errno));
        }

        ret = tcsetattr(serial_fd, TCSANOW, &config);
        if (ret < 0) {
            throw std::runtime_error(std::string("failed to set termios config: ") + std::strerror(errno));
        }
    }

    int serial_fd;
    bool do_read;
    bool call_is_incoming = false;
    bool call_in_progress = false;
    bool audio_started = false;
    std::thread read_thd;
    std::mutex port_mutex;

    std::function <void ()> call_incoming_handler;
    std::function <void ()> call_missed_handler;
    std::function <void ()> call_ended_handler;
    std::function <void ()> call_started_handler;
    std::function <void ()> voice_begin_handler;
    std::function <void ()> voice_end_handler;

    // modem power controls
    int gpio_fd;
    struct gpioevent_request modem_pwr_ctrl;
    struct gpioevent_request modem_flight_ctrl;
};

modem::modem(const std::string &serial_device, int baudrate) 
: m_data(new Data(serial_device, baudrate)) {

}

modem::~modem() = default;

void modem::dial(const std::string &number) {
    if(m_data->dial_number(number)) {
        m_data->call_in_progress = true;
        // Call now so that audiod will get the signal to start audio transfer
        m_data->call_started_handler();
    }
}

void modem::answer_call() {
    m_data->write_command("ATA\r\n");
}

void modem::hangup() {
    if(m_data->write_command("AT+CHUP\r\n") > 0) {
        m_data->call_in_progress = false;
        m_data->call_is_incoming = false;
    }
}

bool modem::has_dialtone(){
    return true;
}

bool modem::call_incoming() {
    return m_data->call_is_incoming;
}

bool modem::call_in_progress() {
    return m_data->call_in_progress;
}

void modem::set_call_incoming_handler(std::function<void ()> handler) {
    m_data->call_incoming_handler = handler;
}

void modem::set_call_missed_handler(std::function<void ()> handler) {
    m_data->call_missed_handler = handler;
}

void modem::set_call_ended_handler(std::function<void ()> handler) {
    m_data->call_ended_handler = handler;
}

void modem::set_call_started_handler(std::function<void ()> handler) {
    m_data->call_started_handler = handler;
}

void modem::set_voice_call_begin_handler(std::function<void ()> handler) {
    m_data->voice_begin_handler = handler;
}

void modem::set_voice_call_end_handler(std::function<void ()> handler) {
    m_data->voice_end_handler = handler;
}
