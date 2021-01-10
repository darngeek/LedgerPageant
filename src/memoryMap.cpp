#include "memoryMap.h"
#include "stringUtil.h"

MemoryMap::MemoryMap(const std::string& inName)
	: mName(inName) {
}

bool MemoryMap::Open() {
	std::wstring stemp = stringUtil::s2ws(mName);
	mFilemapHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, mLength, stemp.c_str());

	if (mFilemapHandle == INVALID_HANDLE_VALUE) {
		return false;
	}

	mDataPtr = MapViewOfFile(mFilemapHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	return true;
}

uint32_t MemoryMap::Seek(uint32_t inPos) {
	mPosition = inPos;
	return mPosition;
}

bool MemoryMap::Write(ByteArray& data) {
	uint32_t size = data.Size();
	if (mPosition + size >= mLength) {
		return false;
	}

	uint8_t* dest = (uint8_t*)mDataPtr + mPosition;
	RtlMoveMemory(dest, data.Get().data(), size);

	mPosition += size;
	return true;
}

uint8_t* MemoryMap::ReadBytes(uint32_t len) {
	uint8_t* out = (uint8_t*)calloc(len, sizeof(uint8_t));

	uint8_t* source = (uint8_t*)mDataPtr + mPosition;
	size_t length = len;
	RtlMoveMemory(out, source, length);

	mPosition += len;
	return out;
}

uint32_t MemoryMap::ReadInt() {
	uint8_t* bytes = ReadBytes(4);
	return  (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
}

void MemoryMap::Close() {
	UnmapViewOfFile(mDataPtr);
	CloseHandle(mFilemapHandle);
}

std::string MemoryMap::GetName() {
	return mName;
}
