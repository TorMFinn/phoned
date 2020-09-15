#include "phoned/device_detector.hpp"
#include <libusb-1.0/libusb.h>
#include <stdexcept>
#include <iostream>

namespace phoned {
    DeviceDetector::DeviceDetector() {
        int r = libusb_init(&m_ctx);
        if (r != LIBUSB_SUCCESS) {
            throw std::runtime_error("failed to initialize libusb context");
        }
    }

    DeviceDetector::~DeviceDetector() {
        libusb_exit(m_ctx);
    }

    libusb_device_handle* DeviceDetector::DetectModem() {
        libusb_device **device_list;
        ssize_t num_devices = libusb_get_device_list(m_ctx, &device_list);
        std::cout << "Found " << num_devices << " devices" << std::endl;
        for (ssize_t idx = 0; idx < num_devices; idx++) {
            libusb_device *device = device_list[idx];
            if (device_is_modem(device)) {
                std::cout << "FOUND MODEM" << std::endl;
                libusb_device_handle *handle;
                int r = libusb_open(device, &handle);
                if (r == LIBUSB_SUCCESS) {
                    return handle;
                } else {
                    std::cerr << "Failed to open device" << std::endl;
                    return nullptr;
                }
            }
        }
        libusb_free_device_list(device_list, 1);
        return nullptr;
    }

    bool DeviceDetector::device_is_modem(libusb_device* device) {
        libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(device, &desc);
        if (r != LIBUSB_SUCCESS) {
            std::cerr << "failed to get USB device descriptor" << std::endl;
            return false;
        }
        if (desc.idProduct == 0x9001 && desc.idVendor == 0x1e0e) {
            // SIMCOM 7500E device
            std::cout << "DETECTED MODEM SIMCOM7500" << std::endl;
            return true;
        }
        return false;
    }
}