#include "phoned/dial.hpp"
#include "phoned/dialtone.hpp"
#include "phoned/handset.hpp"

#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

using namespace phoned;

static bool quit = false;

void sighandler(int signum) {
    if (signum == SIGTERM || signum == SIGINT)
        quit = true;
}

void handle_handset_state(dialtone &tone, handset::handset_state state) {
    if (state == handset::handset_state::down) {
        std::cout << "Down" << std::endl;
        tone.stop();
    } else {
        std::cout << "lifted" << std::endl;
        tone.start();
    }
}

void handle_digit_endered(int digit) {
    // TODO: Forward this to the modem.
    (void)digit;
}

void handle_number_entered(std::string number) {
    std::cout << "Number entered: " << number << std::endl;
}

int main(int argc, char **argv) {
    // Init phone components
    handset handset;
    dial dial;
    dialtone tone;

    handset.set_handset_state_changed_callback(
        [&](handset::handset_state state) { handle_handset_state(tone, state); });

    dial.set_digit_entered_callback(handle_digit_endered);

    std::signal(SIGINT, sighandler);
    std::signal(SIGTERM, sighandler);
    while (not quit) {
        // TODO: create break handler.
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    return 0;
}
