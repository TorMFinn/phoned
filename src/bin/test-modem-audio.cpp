#include "phoned/modem_audio.hpp"
#include <iostream>

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "missing parameter\n"
                  << "test-modem-audio: <port> <baudrate>" << std::endl;
        return EXIT_FAILURE;
    }

    phoned::modem_audio audio(std::string(argv[1]), std::stoi(argv[2]));

    std::string input;
    while(true) {
        std::cout << "\nenter command:\n"
                  << "start\n"
                  << "stop\n"
                  << "quit\n\n" << std::endl;
        std::cin >> input;

        if (input == "start") {
            audio.start_transfer();
        } else if (input == "stop") {
            audio.stop_transfer();
        } else if (input == "quit") {
            break;
        }
    }
}
