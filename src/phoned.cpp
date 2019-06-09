#include "audio.hpp"
#include "modem.hpp"
#include "handset.hpp"
#include <memory>
#include <iostream>
#include <thread>
#include <csignal>

std::unique_ptr<phoned::audio> audio;
std::unique_ptr<phoned::modem> modem;

bool quit;

void sighandler(int signum) {
    if (signum == SIGTERM || signum == SIGINT) {
        quit = true;
    }
}

void on_headset_changed(bool state) {
     if (state) {
	  audio->pause_dialtone();
	  std::cout << "handset down" << std::endl;
     } else {
	  audio->start_dialtone();
	  std::cout << "handset up" << std::endl;
     }
}

int main(int argc, char **argv) {
     audio = std::make_unique<phoned::audio>("/dev/ttyUSB4");
     //modem = std::make_unique<phoned::modem>("/dev/ttyUSB2");

     /*
     phoned::handset handset;
     handset.state_changed.connect(on_headset_changed);
     */

     //modem->dial("45420341");

     std::signal(SIGINT, sighandler);
     std::signal(SIGTERM, sighandler);
     quit = false;
     while(not quit) {
	  std::this_thread::sleep_for(std::chrono::seconds(1));
     }

     return 0;
}
