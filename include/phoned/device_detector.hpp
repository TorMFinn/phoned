#pragma once
#include <libusb-1.0/libusb.h>

namespace phoned {
    class DeviceDetector {
        public:
        DeviceDetector();
        virtual ~DeviceDetector();

        libusb_context* GetContext();
        libusb_device_handle* DetectModem();

        private:
        libusb_context *m_ctx;

        bool device_is_modem(libusb_device *device);
    };
}