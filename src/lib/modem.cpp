#include "phoned/modem.hpp"
#include <unistd.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <poll.h>
#include <cstring>
#include <thread>
#include <syslog.h>
#include <mutex>

using namespace phoned;

const int BUFSIZE = 1024;

struct modem::Data {
    Data(const std::string &serial_device, int baudrate) {
        init_serial(serial_device, baudrate);

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
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            stop_audio();
        } else if (msg.find("VOICE CALL: END") != msg.npos) {
            try {
                // Stops writing during the handler and allows the modem to be able to flush its buffers
                call_in_progress = false;
                call_is_incoming = false;
                call_ended_handler();
                voice_end_handler();
                std::this_thread::sleep_for(std::chrono::seconds(1));
                stop_audio();
                // Give time for audio to stop transfer
            } catch (const std::bad_function_call) {
                syslog(LOG_ERR, "MODEM: warning, no call_ended handler is set");
            }
        } else if (msg.find("VOICE CALL: BEGIN") != msg.npos) {
            try {
                call_in_progress = true;
                start_audio();
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

    bool start_audio() {
        if (not audio_started) {
            syslog(LOG_INFO, "MODEM: Sending audio transfer request");
            const std::string cmd = std::string("AT+CPCMREG=1\r\n");
            if (write_command(cmd) > 0) {
                audio_started = true;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return audio_started;
    }

    bool stop_audio() {
        if (audio_started) {
            syslog(LOG_INFO, "Sending stop audio transfer request");
            const std::string cmd = std::string("AT+CPCMREG=0\r\n");
            if (write_command(cmd) > 0) {
                audio_started = false;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return !audio_started;
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
};

modem::modem(const std::string &serial_device, int baudrate) 
: m_data(new Data(serial_device, baudrate)) {

}

modem::~modem() = default;

void modem::dial(const std::string &number) {
    if(m_data->dial_number(number)) {
        m_data->call_in_progress = true;
        m_data->start_audio();
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
