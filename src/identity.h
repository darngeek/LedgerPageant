#pragma once

#include <regex>
#include <iostream>
#include "bytearray.h"
#include <tchar.h>

#include <cryptopp\cryptlib.h>
#include <cryptopp\sha.h>
#include <cryptopp\hex.h>
#include <cryptopp\files.h>
#include <cryptopp\channels.h>

class Identity {
public:
	Identity();
	explicit Identity(const std::string& identStr);
	~Identity();

	bool FromString(std::string identStr);
	std::string ToString() const;

	ByteArray GetPathBIP32() const;

	// private:
	std::wstring name;
	std::wstring protocol;
	std::wstring user;
	std::wstring host;
	std::wstring path;
	int port;

	TCHAR regKeyName[255] = {0};
	ByteArray pubkey_cached;

private:
	ByteArray GetAddress(bool ecdh) const;
};
