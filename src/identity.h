#pragma once

#include <regex>
#include <iostream>
#include "bytearray.h"
#include <tchar.h>

// CryptoPP:
#include <cryptopp\cryptlib.h>
#include <cryptopp\sha.h>
#include <cryptopp\hex.h>
#include <cryptopp\files.h>
#include <cryptopp\channels.h>

class Identity {
public:
    Identity();
    Identity(std::string identStr);
    ~Identity();

    bool fromUrl(std::string identStr);
    const std::string toUrl();

    byte_array getBip32Path();

    byte_array get_address(bool ecdh);

// private:
    std::wstring name;
    std::wstring protocol;
    std::wstring user;
    std::wstring host;
    std::wstring path;
    int port;

    TCHAR regKeyName[255];
    byte_array pubkey_cached;
};
