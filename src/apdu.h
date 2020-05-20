#pragma once

#include "bytearray.h"

constexpr uint16_t APDU_CHANNEL = 0x0101;
constexpr uint8_t APDU_TAG = 0x05;

class APDU {
public:
	APDU(uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, const ByteArray& data)
		: mInstructionClass(cla)
		, mInstruction(ins)
		, mParameter1(p1)
		, mParameter2(p2)
		, mPayloadSize((uint8_t)data.Size() & 0xFF) {
		memcpy(mPayload, data.Get().data(), data.Size());
	}

	~APDU() {

	}

	ByteArray AsBytes() const {
		ByteArray out;
		out.PushBack(mInstructionClass);
		out.PushBack(mInstruction);
		out.PushBack(mParameter1);
		out.PushBack(mParameter2);
		out.PushBack(mPayloadSize);

		if (mPayloadSize > 0) {
			out.PushBack((uint8_t*)mPayload, mPayloadSize);
		}

		return out;
	}

private:
	uint8_t mInstructionClass = 0x00;
	uint8_t mInstruction = 0x00;
	uint8_t mParameter1 = 0x00;
	uint8_t mParameter2 = 0x00;
	uint8_t mPayloadSize = 0;
	uint8_t mPayload[0xFF];
};
