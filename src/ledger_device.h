#pragma once

#include "hidapi\hidapi\hidapi.h"
#include "bytearray.h"

constexpr size_t packet_size = 64;

class ledger_device {
public:
    ledger_device();
    ~ledger_device();

    bool updateIsDevicePresent();
    bool isDeviceReady() const;
    bool isDeviceAppReady() const;

    size_t read(byte_array& outBuffer);
    size_t write(const byte_array& inBuffer);
    
    uint32_t read_timeout(byte_array& outBuffer, uint32_t timeout);

private:
    bool mDeviceReady = false;
    bool mDeviceAppReady = false;
    hid_device* m_dev = nullptr;
    hid_device_info* dev_info = nullptr;
    byte_array m_buffer; // TODO: used as buffer for last read value, then copying a bunch of times
};
