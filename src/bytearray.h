#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <algorithm>	// swap

class ByteArray {
public:
	ByteArray() {

	}

	explicit ByteArray(size_t size)
		: mData(size) {
	}

	~ByteArray() {

	}

	uint32_t PushBack(uint8_t _byte) {
		mData.push_back(_byte);

		return 1;
	}

	uint32_t PushBack(uint16_t _short) {
		mData.push_back((uint8_t)(_short >> 8u));
		mData.push_back((uint8_t)(_short));

		return 2;
	}

	uint32_t PushBack(uint32_t _int) {
		mData.push_back((uint8_t)(_int >> 24u));
		mData.push_back((uint8_t)(_int >> 16u));
		mData.push_back((uint8_t)(_int >> 8u));
		mData.push_back((uint8_t)(_int));

		return 4;
	}

	uint32_t PushBack(uint64_t _long) {
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

	uint32_t PushBack(uint8_t* bytes, uint32_t bytesSize = -1) {
		if (bytesSize == -1) {
			return 0;
		}

		const uint32_t oldEnd = mData.size();
		mData.resize(oldEnd + bytesSize);
		memcpy(&(mData.data()[oldEnd]), bytes, bytesSize);
		return bytesSize;
	}

	uint32_t PushBack(const ByteArray& bytes) {
		const uint32_t oldEnd = mData.size();
		mData.resize(oldEnd + bytes.Size());
		memcpy(&(mData.data()[oldEnd]), bytes.Get().data(), bytes.Size());
		return bytes.Size();
	}

	size_t Size() const {
		return mData.size();
	}

	bool Empty() const {
		return mData.empty();
	}

	void Clear() {
		mData.clear();
	}

	const std::vector<uint8_t>& Get() const {
		return mData;
	}

	std::vector<uint8_t>& Get() {
		return mData;
	}

	uint8_t& operator[] (const uint32_t index) {
		return mData.at(index);
	}

	ByteArray& operator= (ByteArray other) {
		if (*this == other) {
			return *this;
		}

		mData.swap(other.mData);

		return *this;
	}

	bool operator== (const ByteArray& other) const {
		if (mData.size() != other.mData.size()) {
			return false;
		}

		return mData == other.mData;
	}

	bool operator!= (const ByteArray& other) {
		if (mData.size() != other.mData.size()) {
			return true;
		}

		return mData != other.mData;
	}

	uint8_t AsByte(size_t index = 0) const {
		return mData[index];
	}
	
	std::string AsString() const {
		std::string str(mData.begin(), mData.end());
		return str;
	}

	uint16_t AsShort(size_t index = 0) const {
		return	static_cast<uint16_t>(mData[index + 0]) << 8u |
			static_cast<uint16_t>(mData[index + 1]);
	}

	uint32_t AsInt(size_t index = 0) const {
		return	static_cast<uint32_t>(mData[index + 0]) << 24u |
			static_cast<uint32_t>(mData[index + 1]) << 16u |
			static_cast<uint32_t>(mData[index + 2]) << 8u |
			static_cast<uint32_t>(mData[index + 3]);
	}

	uint64_t AsLong(size_t index = 0) const {
		return	static_cast<uint64_t>(mData[index + 0]) << 56u |
			static_cast<uint64_t>(mData[index + 1]) << 48u |
			static_cast<uint64_t>(mData[index + 2]) << 40u |
			static_cast<uint64_t>(mData[index + 3]) << 32u |
			static_cast<uint64_t>(mData[index + 4]) << 24u |
			static_cast<uint64_t>(mData[index + 5]) << 16u |
			static_cast<uint64_t>(mData[index + 6]) << 8u |
			static_cast<uint64_t>(mData[index + 7]);
	}

private:
	std::vector<uint8_t> mData;
};
