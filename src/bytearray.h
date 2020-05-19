#pragma once

#include <stdint.h>
#include <vector>
#include <algorithm>	// swap

class byte_array {
public:
	byte_array() {

	}

	explicit byte_array(size_t size)
		: mData(size) {
	}

	~byte_array() {

	}

	uint32_t push_back(uint8_t _byte) {
		mData.push_back(_byte);

		return 1;
	}

	uint32_t push_back(uint16_t _short) {
		mData.push_back((uint8_t)(_short >> 8u));
		mData.push_back((uint8_t)(_short));

		return 2;
	}

	uint32_t push_back(uint32_t _int) {
		mData.push_back((uint8_t)(_int >> 24u));
		mData.push_back((uint8_t)(_int >> 16u));
		mData.push_back((uint8_t)(_int >> 8u));
		mData.push_back((uint8_t)(_int));

		return 4;
	}

	uint32_t push_back(uint64_t _long) {
		mData.push_back((uint8_t)(_long >> 56u));
		mData.push_back((uint8_t)(_long >> 48u));
		mData.push_back((uint8_t)(_long >> 40u));
		mData.push_back((uint8_t)(_long >> 32u));
		mData.push_back((uint8_t)(_long >> 24u));
		mData.push_back((uint8_t)(_long >> 16u));
		mData.push_back((uint8_t)(_long >> 8u));
		mData.push_back((uint8_t)(_long));

		return 8;
	}

	uint32_t push_back(uint8_t* bytes, uint32_t bytesSize) {
		const uint32_t oldEnd = mData.size();
		mData.resize(oldEnd+bytesSize);
		memcpy(&(mData.data()[oldEnd]), bytes, bytesSize);
		return bytesSize;
	}

	uint32_t push_back(byte_array bytes) {
		const uint32_t oldEnd = mData.size();
		mData.resize(oldEnd + bytes.size());
		memcpy(&(mData.data()[oldEnd]), bytes.get().data(), bytes.size());
		return bytes.size();
	}

	size_t size() const {
		return mData.size();
	}

	bool empty() const {
		return mData.empty();
	}

	void clear() {
		mData.clear();
	}

	std::vector<uint8_t> get() const {
		return mData;
	}

	uint8_t& operator[] (const uint32_t index) {
		return mData.at(index);
	}

	byte_array& operator= (byte_array other) {
		if (*this == other) {
			return *this;
		}

		mData.swap(other.mData);

		return *this;
	}

	std::vector<uint8_t>::const_iterator begin() const {
		return mData.begin();
	}
	std::vector<uint8_t>::const_iterator end() const {
		return mData.end();
	}

	std::vector<uint8_t>::iterator begin() {
		return mData.begin();
	}
	std::vector<uint8_t>::iterator end() {
		return mData.end();
	}

	bool operator== (const byte_array& other) const {
		if (mData.size() != other.mData.size()) {
			return false;
		}

		return mData == other.mData;
	}

	bool operator!= (const byte_array& other) {
		if (mData.size() != other.mData.size()) {
			return true;
		}

		return mData != other.mData;
	}

	uint8_t asByte(size_t index = 0) const {
		return mData[index];
	}

	uint16_t asShort(size_t index = 0) const {
		return	static_cast<uint16_t>(mData[index+0]) << 8u | 
				static_cast<uint16_t>(mData[index+1]);
	}

	uint32_t asInt(size_t index = 0) const {
		return	static_cast<uint32_t>(mData[index+0]) << 24u |
				static_cast<uint32_t>(mData[index+1]) << 16u |
				static_cast<uint32_t>(mData[index+2]) << 8u |
				static_cast<uint32_t>(mData[index+3]);
	}

	uint64_t asLong(size_t index = 0) const {
		return	static_cast<uint64_t>(mData[index+0]) << 56u | 
				static_cast<uint64_t>(mData[index+1]) << 48u |
				static_cast<uint64_t>(mData[index+2]) << 40u | 
				static_cast<uint64_t>(mData[index+3]) << 32u |
				static_cast<uint64_t>(mData[index+4]) << 24u | 
				static_cast<uint64_t>(mData[index+5]) << 16u |
				static_cast<uint64_t>(mData[index+6]) << 8u | 
				static_cast<uint64_t>(mData[index+7]);
	}

	bool as_hex(char* outBuffer) {
		char* data = (char*)mData.data();
		size_t size = mData.size();

		outBuffer = (char*)malloc(size * 3);
		uint32_t buffIdx = 0;
		for (uint32_t i = 0; i < size; ++i) {
			sprintf_s(outBuffer + buffIdx, 4, "%02X ", data[i]);
			buffIdx++;
		}
	}

private:
	std::vector<uint8_t> mData;
};
