#include "phoned/audio.hpp"
#include <csignal>
#include <thread>

bool quit = false;

void sighandler(int signum) {
    (void)signum;
    quit = true;
}

int main() {
    phoned::audio audio;

    quit = false;
    std::signal(SIGTERM, sighandler);
    std::signal(SIGINT, sighandler);

    audio.start_transfer("/dev/ttyUSB4", 921600);
    while (!quit) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    audio.stop_transfer();
    return 0;
}
