#include "phoned/dialtone.hpp"
#include <thread>
#include <iostream>

int main(int argc, char **argv) {
    phoned::dialtone dialtone;

    while(1) {
        std::cout << "starting dialtone" << std::endl;
        dialtone.start();
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::cout << "Stopping dialtone" << std::endl;
        dialtone.stop();
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    
    return 0;
}
