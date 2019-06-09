#include "audio.hpp"
#include "handset.hpp"
#include <memory>
#include <iostream>
#include <thread>

std::unique_ptr<phoned::audio> audio;

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
     audio = std::make_unique<phoned::audio>();

     phoned::handset handset;
     handset.state_changed.connect(on_headset_changed);

     while(1) {
	  std::this_thread::sleep_for(std::chrono::seconds(1));
     }

     return 0;
}
