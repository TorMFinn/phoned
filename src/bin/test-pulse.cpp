#include <iostream>
#include "phoned/audio.hpp"
#include <csignal>
#include <thread>

bool quit = false;

void sighandler(int signum) {
    quit = true;
}

int main(int argc, char** argv) {
    phoned::audio a;

    a.start();

    std::signal(SIGINT, sighandler);
    while(!quit) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
