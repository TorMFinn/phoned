#include <skimpbuffers/BusConnection.hpp>
#include <csignal>
#include <thread>

#include "handset.hpp"

bool quit;

void sighandler(int signum) {
  quit = true;
}

int main(int argc, char **argv) {
  auto bus = skimpbuffers::BusConnection::Create("phone");
  phoned::handset h;

  h.state_changed.connect([&bus](bool state) {
			    if (state) {
			      bus->WriteString("handset_down", "/phone/state_change/handset");
			    } else {
			      bus->WriteString("handset_up", "/phone/state_change/handset");
			    }
			  });

    quit = false;
    while(not quit) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return EXIT_SUCCESS;
}