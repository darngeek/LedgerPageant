#include "agent.h"

#include "logger.h"
#include "base64.h"
#include "ledger_device.h"

// decompressionn of pubkey
#include <cryptopp/cryptlib.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/ecp.h>
#include <cryptopp/hex.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>

// dongle / response
constexpr uint16_t CODE_SUCCESS = 0x9000;
constexpr uint16_t CODE_USER_REJECTED = 0x6985;
constexpr uint16_t CODE_INVALID_PARAM = 0x6b01;
constexpr uint16_t CODE_NO_STATUS_RESULT = CODE_SUCCESS + 1;

// nist256
const uint8_t SSH_NIST256_DER_OCTET = 0x04;
const std::string SSH_NIST256_KEY_PREFIX = "ecdsa-sha2-";
const std::string SSH_NIST256_CURVE_NAME = "nistp256";
const std::string SSH_NIST256_KEY_TYPE = SSH_NIST256_KEY_PREFIX + SSH_NIST256_CURVE_NAME;

constexpr size_t headerSize = 5;
constexpr size_t headerExtra = 2;
constexpr size_t dataSize = packet_size - (headerSize + headerExtra);

agent::agent(ledger_device* inLedgerDevice)
    :mLedgerDevice(inLedgerDevice)
{

}

agent::~agent() {

}

byte_array agent::decompressPubKey(byte_array& inCompressedKey) {
    /*
        eventho romain is using NIST256p1
        https://tools.ietf.org/search/rfc4492 indicates we can use:
        secp256r1   |  prime256v1   |   NIST P-256
    */

    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PublicKey _pubKey;
    _pubKey.AccessGroupParameters().Initialize(CryptoPP::ASN1::secp256r1());

    CryptoPP::ArraySource compressedSource(inCompressedKey.get().data(), inCompressedKey.size(), true);

    CryptoPP::ECP::Point point;
    _pubKey.GetGroupParameters().GetCurve().DecodePoint(point, compressedSource, compressedSource.MaxRetrievable());

    byte_array pointMash;

    uint32_t numXbytes = point.x.ByteCount();
    for (uint32_t i = 1; i <= numXbytes; ++i) {
        pointMash.push_back(point.x.GetByte(numXbytes - i));
    }

    uint32_t numYbytes = point.y.ByteCount();
    for (uint32_t i = 1; i <= numYbytes; ++i) {
        pointMash.push_back(point.y.GetByte(numYbytes - i));
    }

    return pointMash;
}

byte_array agent::wrapApdu(APDU& command) {
    constexpr uint32_t packetSize = 64;

    byte_array commandBytes = command.to_bytes();

    uint16_t sequenceIdx = 0;
    uint32_t offset = 0;

    byte_array result;
    result.push_back((uint16_t)APDU_CHANNEL);
    result.push_back((uint8_t)APDU_TAG);
    result.push_back((uint16_t)sequenceIdx);
    result.push_back((uint16_t)commandBytes.size());

    sequenceIdx++;

    // TODO: commandLen and packetsize are likely the same, not sure where header extra comes from...
    const uint32_t dataSize = packetSize - headerSize - headerExtra;
    uint32_t blockSize = commandBytes.size();
    if (commandBytes.size() > dataSize) {
        blockSize = dataSize;
    }

    for (uint32_t i = 0; i < blockSize; ++i) {
        result.push_back(commandBytes[i + offset]);
    }
    offset = offset + blockSize;

    while (offset != commandBytes.size()) {
        result.push_back((uint16_t)APDU_CHANNEL);
        result.push_back((uint8_t)APDU_TAG);
        result.push_back((uint16_t)sequenceIdx);

        sequenceIdx++;

        blockSize = commandBytes.size() - offset;
        if ((commandBytes.size() - offset) > packetSize - 3 - headerExtra) {
            blockSize = packetSize - 3 - headerExtra;
        }

        for (uint32_t i = 0; i < blockSize; ++i) {
            result.push_back(command.to_bytes()[i + offset]);
        }

        offset = offset + blockSize;

        // padding with 0 to meet packet size
        while (result.size() % packetSize != 0) {
            result.push_back((uint8_t)0);
        }
    }

    return result;
}

byte_array agent::unwrapApdu(byte_array& data) {
    uint32_t sequenceIdx = 0;
    uint32_t offset = 0;

    const uint32_t dataLen = data.size();
    if (dataLen < 5 + headerExtra + 5) {
        LOG_ERR("Data size smaller than header.");
        return {};
    }

    if (data.asShort(offset) != APDU_CHANNEL) {
        LOG_ERR("Invalid APDU channel");
        return {};
    }
    offset += 2;

    if (data.asByte(offset) != APDU_TAG) {
        LOG_ERR("Invalid APDU tag");
        return {};
    }
    offset += 1;

    if (data.asShort(offset) != sequenceIdx) {
        LOG_ERR("Invalid APDU sequence");
        return {};
    }
    offset += 2;

    uint32_t responseLength = data.asShort(offset);
    offset += 2;

    if (data.size() < 5 + headerExtra + responseLength) {
        // We're waiting for another frame
        return {};
    }

    const uint32_t packetSize = 64;
    uint32_t blockSize = responseLength;
    if (responseLength > packetSize - 5 - headerExtra) {
        blockSize = packetSize - 5 - headerExtra;
    }

    byte_array result;
    for (uint32_t i = 0; i < blockSize; ++i) {
        result.push_back(data[offset + i]);
    }
    offset += blockSize;

    while (result.size() != responseLength) {
        sequenceIdx++;

        if (offset == data.size()) {
            LOG_ERR("Invalid data size");
            return {};
        }

        if (data.asShort(offset) != APDU_CHANNEL) {
            LOG_ERR("Invalid APDU channel");
            return {};
        }
        offset += 2;

        if (data.asByte(offset) != APDU_TAG) {
            LOG_ERR("Invalid APDU tag");
            return {};
        }
        offset += 1;

        if (data.asShort(offset) != sequenceIdx) {
            LOG_ERR("Invalid APDU sequence");
            return {};
        }
        offset += 2;

        blockSize = responseLength - result.size();
        if (responseLength - result.size() > packetSize - 3 - headerExtra) {
            blockSize = packetSize - 3 - headerExtra;
        }

        for (uint32_t i = 0; i < blockSize; ++i) {
            result.push_back(data[offset + i]);
        }

        offset += blockSize;
    }

    return result;
}

byte_array agent::exchange(APDU& apdu, uint16_t* statusCode) {

    // TODO: why the copy
    byte_array apdu_data = wrapApdu(apdu);
    byte_array tmp = apdu_data;

    uint32_t padSize = apdu_data.size() % 64;
    if (padSize != 0) {
        for (uint32_t i = 0; i < (64 - padSize); ++i) {
            tmp.push_back((uint8_t)0);
        }
    }

    if (tmp.size() % 64 != 0) {
        LOG_ERR("padding not 64 byte aligned.");
        return {};
    }


    uint32_t offset = 0;
    while (offset != tmp.size()) {
        //data = tmp[offset:offset + 64];
        //data = bytearray([0]) + data;
        byte_array data;
        data.push_back((uint8_t)0);
        for (uint32_t i = 0; i < 64; ++i) {
            data.push_back(tmp[offset + i]);
        }

        if (mLedgerDevice->write(data) < 0) {
            LOG_ERR("Error while writing to device");
            return {};
        }

        offset += 64;
    }

    uint32_t dataLength = 0;
    uint32_t dataStart = 2;

    byte_array result;
    if (mLedgerDevice->read_timeout(result, 15) == 0) {
        return {};
    }

    uint32_t swOffset = 0;
    while (true) {
        byte_array response = unwrapApdu(result);
        uint32_t responseLen = response.size();
        if (responseLen) {
            result = response;
            swOffset = responseLen - 2;
            dataLength = responseLen - 2;
            break;
        }

        byte_array readBytes;
        mLedgerDevice->read(readBytes);
        result.push_back(readBytes);
    }

    // return status code
    *statusCode = result.asShort(swOffset);

    // return
    byte_array response;
    for (uint32_t i = 0; i < dataLength; ++i) {
        response.push_back(result[i]);
    }

    return response;
}

std::string agent::printPubKey(byte_array inKeyBlob, Identity& ident) {
    std::string blobStr(inKeyBlob.begin(), inKeyBlob.end());
    return SSH_NIST256_KEY_TYPE + " " + base64_encode(blobStr) + " <" + ident.toUrl() + ">";
}

byte_array agent::get_public_key(Identity ident) {
    const uint32_t timeout = 5;

    byte_array path = ident.getBip32Path();
    uint8_t pathLen = (uint8_t)path.size();

    // APDU for public key ssh
    APDU dataApdu(0x80, 0x02, 0x00, 0x01, path);

    uint16_t status = CODE_SUCCESS;
    byte_array response = exchange(dataApdu, &status);

    std::string possibleCause = "";
    if (status != 0x9000 && (status & 0xFF00) != 0x6100 && (status & 0xFF00) != 0x6C00) {
        possibleCause = "Unknown reason, Ledger not connected?";
        if (status == 0x6982)
            possibleCause = "Have you uninstalled the existing CA with resetCustomCA first?";
        if (status == 0x6985) {
            possibleCause = "Condition of use not satisfied (denied by the user?)";
            return byte_array();
        }
        if (status == 0x6a84 or status == 0x6a85)
            possibleCause = "Not enough space?";
        if (status == 0x6a83)
            possibleCause = "Maybe this app requires a library to be installed first?";
        if (status == 0x6484)
            possibleCause = "Are you using the correct targetId?";
        if (status == 0x6d00)
            possibleCause = "Unexpected state of device: verify that the right application is opened?";
        if (status == 0x6e00)
            possibleCause = "Unexpected state of device: verify that the right application is opened?";

        LOG_ERR("Exchange status error: %s", possibleCause.c_str());
        return byte_array();
    }

    byte_array blob;

    if (SSH_NIST256_CURVE_NAME == "nistp256") {
        const uint32_t readOffset = 1;
        if ((response[64 + readOffset] & 1) != 0) {
            // indicate compression with 0x03
            blob.push_back((uint8_t)0x03);
        }
        else {
            blob.push_back((uint8_t)0x02);
        }

        // copy 32 response bytes
        for (uint32_t idx = 1; idx < 33; ++idx) {
            blob.push_back(response[idx + readOffset]);
        }
    }
    else {
        LOG_ERR("shouldnt happen, unimpl");
        byte_array keyX;
        for (uint32_t idx = 1; idx <= 32; ++idx) {
            keyX.push_back(response[idx]);
        }

        byte_array keyY;
        for (uint32_t idx = 33; idx < response.size(); ++idx) {
            keyY.push_back(response[idx]);
        }
        if ((keyX[31] & 1) != 0) {
            keyY[31] |= 0x80; // harden (marked with ')
        }

        blob.push_back((uint8_t)0);
        blob.push_back(keyY);
    }

    byte_array keyinfo;
    // SSH_NIST256_KEY_TYPE
    keyinfo.push_back(SSH_NIST256_KEY_TYPE.length());
    keyinfo.push_back((uint8_t*)SSH_NIST256_KEY_TYPE.data(), SSH_NIST256_KEY_TYPE.length());
    // curve_name
    keyinfo.push_back(SSH_NIST256_CURVE_NAME.length());
    keyinfo.push_back((uint8_t*)SSH_NIST256_CURVE_NAME.data(), SSH_NIST256_CURVE_NAME.length());
    // key_blob
    byte_array pubkeyDecompressed = decompressPubKey(blob);
    keyinfo.push_back(pubkeyDecompressed.size() + 1);
    keyinfo.push_back((uint8_t)SSH_NIST256_DER_OCTET);
    keyinfo.push_back(pubkeyDecompressed);

    return keyinfo;
}

byte_array agent::sign(byte_array& inChallenge, Identity& inIdentity) {
    uint32_t offset = 0;
    byte_array signature;
    uint32_t chunk_size = 0;
    uint32_t challengeLen = inChallenge.size();
    while (offset != challengeLen) {
        byte_array response_data;
        if (offset == 0) {
            byte_array dongle_path = inIdentity.getBip32Path();

            for (uint32_t i = 0; i < dongle_path.size(); ++i) {
                response_data.push_back(dongle_path[i]);
            }
        }

        if ((challengeLen - offset) > (255 - response_data.size())) {
            chunk_size = (255 - response_data.size());
        }
        else {
            chunk_size = challengeLen - offset;
        }

        for (uint32_t i = offset; i < (offset + chunk_size); ++i) {
            response_data.push_back(inChallenge[i]);
        }

        offset += chunk_size;

        // challenge response apdu:
        APDU dataApdu(0x80, 0x04, 0x00, 0x81, response_data);

        uint16_t status = CODE_SUCCESS;
        signature = exchange(dataApdu, &status);
        if (status != CODE_SUCCESS) {
            return {};
        }
    }

    offset = 3;

    uint32_t length = signature[offset];
    // LOG_DBG("R Signature length: %d", length);

    std::vector<uint8_t> r;
    for (uint32_t i = offset + 1; i < offset + 1 + length; ++i) {
        /*
        // ledger-agent does this, but this breaks...

        // Python
            if r[0] == 0:
                r = r[1:]
        // C++
            if (i == offset + 1) {
                if (signature[i] == 0) {
                    LOG_DBG("skipstart");
                    //continue;
                }
            }
        */

        r.push_back(signature[i]);
    }

    offset = offset + 1 + length + 1;

    length = signature[offset];
    // LOG_DBG("S Signature length: %d", length);

    std::vector<uint8_t> s;
    for (uint32_t i = offset + 1; i < offset + 1 + length; ++i) {
        if (i == offset + 1) {
            if (signature[i] == 0) {
                // LOG_DBG("skipstart");
                continue;
            }
        }

        s.push_back(signature[i]);
    }

    byte_array encoded_signature_value;
    encoded_signature_value.push_back((uint32_t)r.size());
    encoded_signature_value.push_back((uint8_t*)r.data(), r.size());

    encoded_signature_value.push_back((uint32_t)s.size());
    encoded_signature_value.push_back((uint8_t*)s.data(), s.size());

    byte_array encoded_signature;
    const std::string keyType(SSH_NIST256_KEY_TYPE);
    encoded_signature.push_back((uint32_t)keyType.length());
    encoded_signature.push_back((uint8_t*)keyType.c_str(), keyType.size());

    encoded_signature.push_back((uint32_t)encoded_signature_value.size());
    encoded_signature.push_back((uint8_t*)encoded_signature_value.get().data(), encoded_signature_value.size());

    return encoded_signature;
}