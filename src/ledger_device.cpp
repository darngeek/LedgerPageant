#include "ledger_device.h"

#include "logger.h"
#include <chrono>   // get tick
#include <thread>   // sleep

#define LEDGER_VID 0x2c97
#define LEDGER_USAGE_PAGE 0xffa0
#define NANOS_PID 0x0001

ledger_device::ledger_device() {
    LOG_DBG("HIDAPI Init.");
    hid_init();
}

ledger_device::~ledger_device() {
    LOG_DBG("HIDAPI Exit.");
    hid_exit();
}

bool ledger_device::updateIsDevicePresent() {
    mDeviceAppReady = false;

    std::vector<hid_device_info*> devs;
    hid_device_info* current = hid_enumerate(0, 0);

    while (current != nullptr) {
        if (current->vendor_id == LEDGER_VID) {
            if (current->usage_page == LEDGER_USAGE_PAGE || current->interface_number == 0) {

                // Win32 specific? Installation specific?
                if (current->product_id == 0x1005) {
                    // app open:
                    mDeviceAppReady = true;
                    devs.push_back(current);
                }
                else {
                    mDeviceAppReady = false;
                }
            }
        }

        current = current->next;
    }

    if (devs.size() == 1) {
        dev_info = *devs.begin();
        m_dev = hid_open_path(dev_info->path);
        hid_set_nonblocking(m_dev, 1);

        if (m_dev == nullptr) {
            LOG_ERR("Failed to open HID device: %s", dev_info->path);
            return false;
        }

        mDeviceReady = true;
        return true;
    }

    return false;
}

bool ledger_device::isDeviceReady() const {
    return mDeviceReady;
}

bool ledger_device::isDeviceAppReady() const {
    return mDeviceAppReady;
}

size_t ledger_device::read(byte_array& out) {

    hid_set_nonblocking(m_dev, 0);

    m_buffer.clear();

    // TODO: unsure why cant read onto buffer.
    std::vector<uint8_t> buf(packet_size);
    size_t n = hid_read(m_dev, buf.data(), packet_size);
    // read into vector, push back onto buffer
    for (uint32_t i = 0; i < packet_size; ++i) {
        m_buffer.push_back(buf[i]);
    }

    if (n < 0) {
        auto err = hid_error(m_dev);
        wprintf(err);
        return 0;
    }

    if (n != packet_size) {
        return 0;
    }

    hid_set_nonblocking(m_dev, 1);

    return out.push_back(m_buffer);
}

size_t ledger_device::write(const byte_array& inBuffer) {
    return hid_write(m_dev, inBuffer.get().data(), inBuffer.size());
}

uint32_t ledger_device::read_timeout(byte_array& outBuffer, uint32_t timeout) {
    std::chrono::high_resolution_clock::time_point startTimeout = std::chrono::high_resolution_clock::now();

    hid_set_nonblocking(m_dev, 0);

    int32_t readByteLen = 0;
    while (readByteLen == 0) {
        readByteLen = this->read(outBuffer);

        // repeat while read not finished:
        if (readByteLen == 0) {
            if (std::chrono::high_resolution_clock::now() - startTimeout > std::chrono::seconds(timeout)) {
                LOG_ERR("read timeout (forgot to push button?)");
                return 0;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        else if (readByteLen == -1) {
            LOG_ERR("Error reading from device");
            return 0;
        }
    }

    hid_set_nonblocking(m_dev, 1);

    return readByteLen;
}
