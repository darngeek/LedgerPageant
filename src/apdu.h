#pragma once

#include "bytearray.h"

constexpr uint16_t APDU_CHANNEL = 0x0101;
constexpr uint8_t APDU_TAG = 0x05;

class APDU {
public:
    APDU(uint8_t _cla, uint8_t _ins, uint8_t _p1, uint8_t _p2, byte_array _data) 
        : cla (_cla)
        , ins (_ins)
        , p1 (_p1)
        , p2 (_p2)
        , payload_size((uint8_t)_data.size() & 0xFF)
    {
        memcpy(payload, _data.get().data(), _data.size());
    }

    ~APDU() {

    }

    byte_array to_bytes() const {
        byte_array out;
        out.push_back(cla);
        out.push_back(ins);
        out.push_back(p1);
        out.push_back(p2);
        out.push_back(payload_size);

        if (payload_size > 0) {
            out.push_back((uint8_t*)payload, payload_size);
        }

        return out;
    }

private:
    uint8_t cla = 0;
    uint8_t ins = 0;
    uint8_t p1 = 0;
    uint8_t p2 = 0;
    uint8_t payload_size = 0;
    uint8_t payload[0xFF];
};
