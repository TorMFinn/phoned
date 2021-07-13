#include "phoned/handset.hpp"
#include "phoned/dialtone.hpp"
#include "phoned/ringer.hpp"
#include "phoned/dial.hpp"

#include <csignal>
#include <thread>
#include <iostream>

bool quit = false;
void sighandler(int signum) {
    (void) signum;
    quit = true;
}

class phoned_app {
public:
    phoned_app() {
        handset.set_handset_state_changed_callback([&](phoned::handset::handset_state state) {
            handle_handset_state(state);
        });

        // Initialize with the default handset state
        handle_handset_state(handset.get_state());

        dial.set_number_entered_callback([&](const std::string &number) {
            handle_number_endered(number);
        });
    }
    ~phoned_app() = default;

private:
    void handle_handset_state(phoned::handset::handset_state state) {
        if (state == phoned::handset::handset_state::lifted) {
            // If call is not active, then we activate the dialtone
            dialtone.start();
        } else {
            dialtone.stop();
        }
    }

    void handle_number_endered(const std::string &number) {
        std::cout << "Entered number: " << number << std::endl;
        if (handset.get_state() == phoned::handset::handset_state::lifted) {
            // Start Call
            dialtone.stop();
        }
    }

    phoned::handset handset;
    phoned::dialtone dialtone;
    phoned::ringer ringer;
    phoned::dial dial;
};

int main(int argc, char** argv) {
    phoned_app app;

    std::signal(SIGTERM, sighandler);
    std::signal(SIGINT, sighandler);
    while (!quit) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Exiting" << std::endl;

    return 0;
}
