#pragma once

#include <string>

class KeyType {
public:
	KeyType() {}

	KeyType(const std::string& name, const std::string& prefix, const uint8_t der_octet, const uint8_t p2)
		: mName (name)
		, mPrefix (prefix)
		, mOctet (der_octet)
		, mP2 (p2)
	{}

	KeyType(const KeyType& other) {
		mName = other.mName;
		mPrefix = other.mPrefix;
		mOctet = other.mOctet;
		mP2 = other.mP2;
	}
	
	KeyType operator=(const KeyType& other) {
		mName = other.mName;
		mPrefix = other.mPrefix;
		mOctet = other.mOctet;
		mP2 = other.mP2;

		return *this;
	}

	KeyType(KeyType&& other) = default;

	const std::string& GetName() const { 
		return mName; 
	}

	std::string GetKeyType() const { 
		return mPrefix + mName;
	}

	uint8_t GetOctet() const {
		return mOctet; 
	}

	uint8_t GetP2() const {
		return mP2; 
	}


private:
	std::string mName;
	std::string mPrefix;
	uint8_t mOctet;
	uint8_t mP2;
};