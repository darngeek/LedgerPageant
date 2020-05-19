#pragma once

#include "apdu.h"
#include "bytearray.h"
#include "identity.h"

#include <string>

class ledger_device;

class agent {
public:
    agent(ledger_device* inLedgerDevice);
    ~agent();
    
    byte_array decompressPubKey(byte_array& inCompressedKey);

    byte_array wrapApdu(APDU& command);
    byte_array unwrapApdu(byte_array& data);

    byte_array exchange(APDU& apdu, uint16_t* statusCode);

    std::string printPubKey(byte_array inKeyBlob, Identity& ident);
    byte_array get_public_key(Identity ident);
    byte_array sign(byte_array& inData, Identity& inIdentity);

private:
    ledger_device* mLedgerDevice = nullptr;
};
