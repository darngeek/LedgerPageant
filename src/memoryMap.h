#pragma once

#include <cstdint>
#include <wtypes.h>
#include <aclapi.h>
#include <string>
#include "bytearray.h"

constexpr uint32_t AGENT_MAX_MSGLEN = 8192;

class MemoryMap {
public:
	explicit MemoryMap(const std::string& inName);
	bool Open();
	uint32_t Seek(uint32_t inPos);
	bool Write(ByteArray& data);
	uint8_t* ReadBytes(uint32_t len);
	uint32_t ReadInt();
	void Close();
	std::string GetName();

private:
	HANDLE mFilemapHandle = INVALID_HANDLE_VALUE;
	size_t mLength = AGENT_MAX_MSGLEN;
	std::string mName;
	uint32_t mPosition = 0;
	LPVOID mDataPtr = NULL;
};
