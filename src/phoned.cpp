#include "audio.hpp"
#include "modem.hpp"
#include "handset.hpp"
#include "dial.hpp"
#include <memory>
#include <iostream>
#include <thread>
#include <csignal>

std::unique_ptr<phoned::audio> audio;
std::unique_ptr<phoned::modem> modem;
std::unique_ptr<phoned::dial> dial;

bool quit;
bool handset_down = true;

void sighandler(int signum) {
    if (signum == SIGTERM || signum == SIGINT) {
        quit = true;
    }
}

void on_number(const std::string &number) {
     std::cout << "is handset down: " << std::boolalpha << std::endl;
     std::cout << "number: " << number << std::endl;
     if (not handset_down) {
	  std::cout << "dialing" << std::endl;
	  audio->stop_dialtone();
	  audio->start_rx_line();
	  modem->dial(number);
     }
}

void on_headset_changed(bool state) {
     handset_down = state;
     if (state) {
	  audio->stop_dialtone();
	  std::cout << "handset down" << std::endl;
	  modem->hangup();
	  //audio->stop_rx_line();
     } else {
	  if (modem->has_dialtone()) {
	       audio->start_dialtone();
	       std::cout << "handset up" << std::endl;
	  }
     }
}

int main(int argc, char **argv) {
     audio = std::make_unique<phoned::audio>("/dev/cuaU0.4");
     modem = std::make_unique<phoned::modem>("/dev/cuaU0.2");
     dial = std::make_unique<phoned::dial>();
     dial->on_number.connect(on_number);

     phoned::handset handset;
     handset.state_changed.connect(on_headset_changed);

     std::signal(SIGINT, sighandler);
     std::signal(SIGTERM, sighandler);
     quit = false;
     while(not quit) {
	  std::this_thread::sleep_for(std::chrono::seconds(1));
     }

     return 0;
}
