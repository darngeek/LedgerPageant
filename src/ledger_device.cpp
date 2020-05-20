#include "ledger_device.h"

#include "logger.h"
#include <chrono>   // get tick
#include <thread>   // sleep

#define LEDGER_VID 0x2c97
#define LEDGER_USAGE_PAGE 0xffa0
#define NANOS_PID 0x1005

Device::Device() {
	LOG_DBG("HIDAPI Init.");
	hid_init();
}

Device::~Device() {
	LOG_DBG("HIDAPI Exit.");
	Close();
	hid_exit();
}

bool Device::Open() {
	hid_device* result = hid_open(LEDGER_VID, NANOS_PID, NULL);
	if (result != NULL) {
		mDevice = result;
		return true;
	}

	return false;
}

void Device::Close() {
	hid_close(mDevice);
}

size_t Device::Read(ByteArray& out) {

	hid_set_nonblocking(mDevice, 0);

	mReadBuffer.Clear();

	// read into vector, push back onto buffer
	std::vector<uint8_t> buf(packet_size);
	size_t n = hid_read(mDevice, buf.data(), packet_size);
	for (uint32_t i = 0; i < packet_size; ++i) {
		mReadBuffer.PushBack(buf[i]);
	}

	if (n == 0) {
		auto err = hid_error(mDevice);
		wprintf(err);
		return 0;
	}

	if (n != packet_size) {
		return 0;
	}

	hid_set_nonblocking(mDevice, 1);

	return out.PushBack(mReadBuffer);
}

uint32_t Device::Read(ByteArray& outBuffer, uint32_t timeout) {
	std::chrono::high_resolution_clock::time_point startTimeout = std::chrono::high_resolution_clock::now();

	hid_set_nonblocking(mDevice, 0);

	int32_t readByteLen = 0;
	while (readByteLen == 0) {
		readByteLen = this->Read(outBuffer);

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

	hid_set_nonblocking(mDevice, 1);

	return readByteLen;
}

int Device::Write(const ByteArray& inBuffer) {
	return hid_write(mDevice, inBuffer.Get().data(), inBuffer.Size());
}
