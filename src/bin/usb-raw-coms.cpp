#include "phoned/device_detector.hpp"
#include <iostream>
#include <libusb-1.0/libusb.h>

int main() {
    phoned::DeviceDetector detector;

    auto modem = detector.DetectModem();
    if (modem != nullptr) {
        std::cout << "FOUND MODEM :)" << std::endl;
    }

    libusb_device *modem_dev = libusb_get_device(modem);
    if (modem_dev == nullptr) {
        std::cerr << "failed to get device for modem handle" << std::endl;
        return -1;
    }

    libusb_device_descriptor desc;
    libusb_get_device_descriptor(modem_dev, &desc);

    std::cout << "Modem has " << (unsigned) desc.bNumConfigurations << " possible configurations" << std::endl;
    libusb_config_descriptor *conf_desc;
    libusb_get_config_descriptor(modem_dev, 0, &conf_desc);

    std::cout << "Config 0 has " << (unsigned) conf_desc->bNumInterfaces << " interfaces" << std::endl;
    for (uint8_t i; i < conf_desc->bNumInterfaces; i++) {
        std::cout << "Interface " << (unsigned) i << " class type " << std::hex << (unsigned) conf_desc->interface[i].altsetting->bInterfaceClass << std::endl;
    }

    int r = libusb_claim_interface(modem, 1); // AT PORT
    if (r != LIBUSB_SUCCESS) {
        std::cerr << "failed to claim AT port" << std::endl;
    }

    std::cout << "GOT AT PORT" << std::endl;

    
    return 0;
}