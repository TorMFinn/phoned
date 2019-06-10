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

using namespace phoned;

struct modem::Data {
    int fd;
    std::thread read_thd;
    bool call_in_progress;
    std::atomic<bool> quit;
    std::mutex read_mutex;

    bool open_modem(const std::string &path) {
        struct termios config;
        fd = open(path.c_str(), O_RDWR | O_NOCTTY);
        if (fd == -1) {
            std::cerr << "failed to open modem\n"
                      << std::strerror(errno) << std::endl;
            return false;
        }

        if (not isatty(fd)) {
            std::cerr << "file is not a tty" << std::endl;
            return false;
        }

        cfmakeraw(&config);
        if (cfsetspeed(&config, B921600) < 0) {
            std::cerr << "failed to set baudrate" << std::endl;
            return false;
        }

        if(tcsetattr(fd, TCSAFLUSH, &config) < 0) {
            std::cerr << "failed to set serial config" << std::endl;
            return false;
        }

        //read_thd = std::thread(&Data::read_thd_function, this);

        return true;
    }

     void init_usb_audio() {
	  /*
	  std::string cmd = "AT+CPCMREG=1\r";
	  ssize_t written = write(fd, cmd.data(), cmd.size());
	  */
	  execute_command("AT+CPCMREG=1");
	  std::this_thread::sleep_for(std::chrono::milliseconds(200));
	  execute_command("AT+CNSN=0x0");
	  std::this_thread::sleep_for(std::chrono::milliseconds(200));
	  execute_command("AT+CNSLIM=0x0");
	  std::this_thread::sleep_for(std::chrono::milliseconds(200));
	  execute_command("AT+CNSLIM=0x0");
	  std::this_thread::sleep_for(std::chrono::milliseconds(200));
	  execute_command("AT+CTXVOL=0x8000");
	  std::cout << "started usb audio" << std::endl;
	  /*
	  std::this_thread::sleep_for(std::chrono::seconds(1));

	  std::string cmd2 = "AT+VMUTE=1\r";
	  write(fd, cmd2.data(), cmd2.size());
	  std::cout << "started usb audio" << std::endl;
	  */
     }

     void stop_usb_audio() {
	  std::string cmd = "AT+CPCMREG=0\r";
	  ssize_t written = write(fd, cmd.data(), cmd.size());
	  std::cout << "stopped usb audio" << std::endl;
     }

    /**
     * Execute a AT command and wait for OK
     */
    bool execute_command(const std::string cmd) {
        char recv_buf[1024];
        std::string full_cmd = cmd + "\r";
        read_mutex.lock();
        std::cout << "Writing: " << full_cmd << std::endl;
        write(fd, full_cmd.data(), full_cmd.size());

	/*
        // Read and wait for OK
        ssize_t amount_read = read(fd, recv_buf, 1024);
        std::string recv_buf_str(recv_buf, amount_read);

        read_mutex.unlock();
	*/
        read_mutex.unlock();
        //return recv_buf_str.find("OK") != std::string::npos;
	return true;
    }

    void read_thd_function() {
        quit = false;
        struct pollfd fds[1];
        fds[0].fd = fd;
        fds[0].events = POLLIN;
        while (not quit) {
            int r = poll(fds, 1, 1000);
            if (r > 0) {
                // Success
                read_mutex.lock();
                char buf[1024];
                ssize_t amount_read = read(fd, buf, 1024);
                std::cout << "I did read: " << std::string(buf, amount_read) << std::endl;
                read_mutex.unlock();
            }
        }
    }
};

modem::modem(const std::string &at_port_path)
    : m_data(new Data()) {
    if(not m_data->open_modem(at_port_path)) {
        throw std::runtime_error("failed to configure modem communications");
    }

    std::cout << "AT: " << std::boolalpha << m_data->execute_command("AT") << std::endl;
}

modem::~modem() {
    m_data->quit = true;
    if (m_data->read_thd.joinable()) {
        m_data->read_thd.join();
    }
    close(m_data->fd);
}

bool modem::dial(const std::string &number) {
     std::string cmd = "ATD"+number+";\r";
     ssize_t written = write(m_data->fd, cmd.data(), cmd.size());
     std::this_thread::sleep_for(std::chrono::seconds(1));
     m_data->init_usb_audio();
     std::this_thread::sleep_for(std::chrono::seconds(1));
     return written > 0;
}

void modem::hangup() {
     //m_data->execute_command("AT+CHUP");
     std::string cmd = "AT+CHUP\r";
     ssize_t written = write(m_data->fd, cmd.data(), cmd.size());
     std::this_thread::sleep_for(std::chrono::seconds(1));
     m_data->stop_usb_audio();
     std::this_thread::sleep_for(std::chrono::seconds(1));
}

bool modem::answer_call() {
    return m_data->execute_command("ATA");
}

bool modem::has_dialtone() {
     std::lock_guard<std::mutex> lock(m_data->read_mutex);
     std::string cmd = "AT+CPAS\r";
     ssize_t written = write(m_data->fd, cmd.data(), cmd.size());
     if (written > 0) {
	  // Get reply
	  char recv_buf[1024];
	  ssize_t r = read(m_data->fd, recv_buf, 1024);
	  if (r > 0) {
	       std::string str_buf(recv_buf, r);
	       return str_buf.find("+CPAS: 0") != std::string::npos;
	  }
     }

     return false;
}
     
