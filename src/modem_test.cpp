#include "modem.hpp"
#include <iostream>

int main() {
    phoned::modem modem("/dev/ttyUSB2");

    //modem.dial("45420341");

    return 0;
}
