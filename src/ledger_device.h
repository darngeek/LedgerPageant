#pragma once

#include "hidapi\hidapi\hidapi.h"
#include "bytearray.h"

constexpr size_t packet_size = 64;

class Device {
public:
	Device();
	~Device();

	bool Open();
	void Close();

	size_t Read(ByteArray& outBuffer);
	uint32_t Read(ByteArray& outBuffer, uint32_t timeout);

	int Write(const ByteArray& inBuffer);
	
private:
	bool mDeviceAppReady = false;
	hid_device* mDevice = nullptr;
	ByteArray mReadBuffer;
};
