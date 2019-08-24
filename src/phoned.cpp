//#include "dialtone.hpp"
#include "modem_audio.hpp"
#include "modem.hpp"
#include "handset.hpp"
#include "dial.hpp"
#include "ringer.hpp"
#include <memory>
#include <iostream>
#include <thread>
#include <csignal>

std::unique_ptr<phoned::modem_audio> audio;
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
	  //audio->stop_dialtone();
	  //phoned::stop_dialtone();
	  modem->dial("99163563");
          std::this_thread::sleep_for(std::chrono::seconds(1));
          audio->start_audio_transfer();
     }
}

void on_handset_changed(bool down) {
    handset_down = down;
    if (handset_down) {
      //audio->stop_dialtone();
      //phoned::stop_dialtone();
	  std::cout << "handset down" << std::endl;
	      modem->hangup();
          audio->stop_audio_transfer();
          std::this_thread::sleep_for(std::chrono::seconds(1));
     } else {
        std::cout << "handset up" << std::endl;
	  if (modem->has_dialtone()) {
	    //audio->start_dialtone();
	    //phoned::start_dialtone();
	  }
     }
}

int main(int argc, char **argv) {
  phoned::serial_connection_info info = {"/dev/cuaU0.4", 921600};
  audio = std::make_unique<phoned::modem_audio>(info);
     modem = std::make_unique<phoned::modem>("/dev/cuaU0.2");
     dial = std::make_unique<phoned::dial>();
     dial->on_number.connect(on_number);

     phoned::handset handset;
     handset.state_changed.connect(on_handset_changed);

     // Get initial state
     on_handset_changed(handset.is_handset_down());

     /*
     phoned::ringer ringer;
     std::this_thread::sleep_for(std::chrono::seconds(1));
     ringer.start_ring();
     */

     std::signal(SIGINT, sighandler);
     std::signal(SIGTERM, sighandler);
     quit = false;
     while(not quit) {
	  std::this_thread::sleep_for(std::chrono::seconds(1));
     }

     //ringer.stop_ring();

     return 0;
}
