#include "phoned/dial.hpp"
#include "phoned/dialtone.hpp"
#include "phoned/handset.hpp"
#include "phoned/simcom7500x.hpp"

#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <thread>

using namespace phoned;

static bool quit = false;

void sighandler(int signum) {
    if (signum == SIGTERM || signum == SIGINT)
        quit = true;
}

void handle_handset_state(Dialtone &tone, HandsetState state) {
    if (state == HandsetState::down) {
        std::cout << "Down" << std::endl;
        tone.Stop();
    } else {
        std::cout << "lifted" << std::endl;
        tone.Start();
    }
}

void dial_handler(DialEvent event, void *event_data) {
    switch (event) {
    case DialEvent::DIGIT_ENTERED: {
        auto data = reinterpret_cast<DigitEnteredData *>(event_data);
        std::cout << "Digit entered: " << data->digit << std::endl;
    } break;
    case DialEvent::NUMBER_ENTERED: {
        auto number = reinterpret_cast<NumberEnteredData *>(event_data);
        std::cout << "Number entered: " << number->number << std::endl;
        break;
    }
    }
}

void modem_handler(ModemEvent event, void *event_data) {
    switch (event) {
    case ModemEvent::CALL_INCOMING:
        std::cout << "Call incoming" << std::endl;
        break;
    case ModemEvent::CALL_MISSED:
        std::cout << "Call missed" << std::endl;
        break;
    case ModemEvent::CALL_ENDED:
        std::cout << "Call ended" << std::endl;
        break;
    case ModemEvent::CALL_STARTED:
        std::cout << "Call Started" << std::endl;
        break;
    case ModemEvent::MMS_RECEIVED:
        std::cout << "MMS Received" << std::endl;
        break;
    case ModemEvent::SMS_RECEIVED:
        std::cout << "SMS Received" << std::endl;
        break;
    }
}

int main(int argc, char **argv) {
    // Init phone components
    Handset handset;
    Dial dial;
    Dialtone tone;

    handset.SetHandsetStateChangedHandler([&](HandsetState state) {
        handle_handset_state(tone, state);
    });

    dial.SetDialEventHandler(dial_handler);

    ModemPtr modem = std::make_shared<SIMCOM7500X>();
    if (!modem->Open("/dev/ttyUSB2", 115200)) {
        std::cerr << "failed to open modem." << std::endl;
        return EXIT_FAILURE;
    }

    std::signal(SIGINT, sighandler);
    std::signal(SIGTERM, sighandler);
    while (not quit) {
        // TODO: create break handler.
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    return 0;
}
