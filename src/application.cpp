#include "application.h"

#include "logger.h"
#include "encodeUtil.h"

// dongle response
constexpr uint16_t CODE_SUCCESS = 0x9000;
constexpr uint16_t CODE_USER_REJECTED = 0x6985;
constexpr uint16_t CODE_INVALID_PARAM = 0x6b01;
constexpr uint16_t CODE_NO_STATUS_RESULT = CODE_SUCCESS + 1;

// nist256
constexpr uint8_t SSH_NIST256_DER_OCTET = 0x04;
const std::string SSH_NIST256_KEY_PREFIX = "ecdsa-sha2-";
const std::string SSH_NIST256_CURVE_NAME = "nistp256";
const std::string SSH_NIST256_KEY_TYPE = SSH_NIST256_KEY_PREFIX + SSH_NIST256_CURVE_NAME;

constexpr size_t headerSize = 5;
constexpr size_t headerExtra = 2;
constexpr size_t maxDataSize = packet_size - (headerSize + headerExtra);

// SSH
#define SSH_AGENT_FAILURE 5
#define SSH2_AGENTC_REQUEST_IDENTITIES 11
#define SSH2_AGENT_IDENTITIES_ANSWER 12
#define SSH2_AGENTC_SIGN_REQUEST 13
#define SSH2_AGENT_SIGN_RESPONSE 14


Application::Application()
	: mIsDeviceConnected(false)
	, mDevice() {
	LoadIdentities();
}

Application::~Application() {

}

bool Application::TryOpenDevice() {
	bool success = true;

	// if we arent ready prompt user to connect
	while (!mDevice.Open()) {
		LPCWSTR title = L"Ledger - Pageant";
		LPCWSTR description = L"Could not connect with SSH/PGP Agent on Ledger Nano S";

		int msgboxID = MessageBox(NULL, description, title, MB_ICONQUESTION | MB_CANCELTRYCONTINUE | MB_DEFBUTTON2);
		if (msgboxID != IDTRYAGAIN) {
			success = false;
			break;
		}
	}

	return success;
}

ByteArray Application::WrapApdu(const APDU& command) {
	constexpr uint32_t packetSize = 64;

	ByteArray commandBytes = command.AsBytes();

	uint16_t sequenceIdx = 0;
	uint32_t offset = 0;

	ByteArray result;
	result.PushBack((uint16_t)APDU_CHANNEL);
	result.PushBack((uint8_t)APDU_TAG);
	result.PushBack((uint16_t)sequenceIdx);
	result.PushBack((uint16_t)commandBytes.Size());

	sequenceIdx++;

	uint32_t blockSize = commandBytes.Size();
	if (commandBytes.Size() > maxDataSize) {
		blockSize = maxDataSize;
	}

	for (uint32_t i = 0; i < blockSize; ++i) {
		result.PushBack(commandBytes[i + offset]);
	}
	offset = offset + blockSize;

	while (offset != commandBytes.Size()) {
		result.PushBack((uint16_t)APDU_CHANNEL);
		result.PushBack((uint8_t)APDU_TAG);
		result.PushBack((uint16_t)sequenceIdx);

		sequenceIdx++;

		blockSize = commandBytes.Size() - offset;
		if ((commandBytes.Size() - offset) > packetSize - 3 - headerExtra) {
			blockSize = packetSize - 3 - headerExtra;
		}

		for (uint32_t i = 0; i < blockSize; ++i) {
			result.PushBack(command.AsBytes()[i + offset]);
		}

		offset = offset + blockSize;

		// padding with 0 to meet packet Size
		while (result.Size() % packetSize != 0) {
			result.PushBack((uint8_t)0);
		}
	}

	return result;
}

ByteArray Application::UnwrapApdu(ByteArray& data) {
	uint32_t sequenceIdx = 0;
	uint32_t offset = 0;

	const uint32_t dataLen = data.Size();
	if (dataLen < 5 + headerExtra + 5) {
		LOG_ERR("Data size smaller than header.");
		return {};
	}

	if (data.AsShort(offset) != APDU_CHANNEL) {
		LOG_ERR("Invalid APDU channel");
		return {};
	}
	offset += 2;

	if (data.AsByte(offset) != APDU_TAG) {
		LOG_ERR("Invalid APDU tag");
		return {};
	}
	offset += 1;

	if (data.AsShort(offset) != sequenceIdx) {
		LOG_ERR("Invalid APDU sequence");
		return {};
	}
	offset += 2;

	uint32_t responseLength = data.AsShort(offset);
	offset += 2;

	if (data.Size() < 5 + headerExtra + responseLength) {
		// We're waiting for another frame
		return {};
	}

	const uint32_t packetSize = 64;
	uint32_t blockSize = responseLength;
	if (responseLength > packetSize - 5 - headerExtra) {
		blockSize = packetSize - 5 - headerExtra;
	}

	ByteArray result;
	for (uint32_t i = 0; i < blockSize; ++i) {
		result.PushBack(data[offset + i]);
	}
	offset += blockSize;

	while (result.Size() != responseLength) {
		sequenceIdx++;

		if (offset == data.Size()) {
			LOG_ERR("Invalid data size");
			return {};
		}

		if (data.AsShort(offset) != APDU_CHANNEL) {
			LOG_ERR("Invalid APDU channel");
			return {};
		}
		offset += 2;

		if (data.AsByte(offset) != APDU_TAG) {
			LOG_ERR("Invalid APDU tag");
			return {};
		}
		offset += 1;

		if (data.AsShort(offset) != sequenceIdx) {
			LOG_ERR("Invalid APDU sequence");
			return {};
		}
		offset += 2;

		blockSize = responseLength - result.Size();
		if (responseLength - result.Size() > packetSize - 3 - headerExtra) {
			blockSize = packetSize - 3 - headerExtra;
		}

		for (uint32_t i = 0; i < blockSize; ++i) {
			result.PushBack(data[offset + i]);
		}

		offset += blockSize;
	}

	return result;
}

ByteArray Application::Exchange(const APDU& apdu, uint16_t* statusCode) {
	ByteArray apdu_data = WrapApdu(apdu);

	uint32_t padSize = apdu_data.Size() % 64;
	if (padSize != 0) {
		for (uint32_t i = 0; i < (64 - padSize); ++i) {
			apdu_data.PushBack((uint8_t)0);
		}
	}

	if (apdu_data.Size() % 64 != 0) {
		LOG_ERR("padding not 64 byte aligned.");
		return {};
	}


	uint32_t offset = 0;
	while (offset != apdu_data.Size()) {
		//data = tmp[offset:offset + 64];
		//data = bytearray([0]) + data;
		ByteArray data;
		data.PushBack((uint8_t)0);
		for (uint32_t i = 0; i < 64; ++i) {
			data.PushBack(apdu_data[offset + i]);
		}

		if (mDevice.Write(data) < 0) {
			LOG_ERR("Error while writing to device");
			return {};
		}

		offset += 64;
	}

	uint32_t dataLength = 0;

	ByteArray result;
	if (mDevice.Read(result, 15) == 0) {
		return {};
	}

	uint32_t swOffset = 0;
	while (true) {
		ByteArray response = UnwrapApdu(result);
		uint32_t responseLen = response.Size();
		if (responseLen) {
			result = response;
			swOffset = responseLen - 2;
			dataLength = responseLen - 2;
			break;
		}

		ByteArray readBytes;
		mDevice.Read(readBytes);
		result.PushBack(readBytes);
	}

	// return status code
	*statusCode = result.AsShort(swOffset);

	// return
	ByteArray response;
	for (uint32_t i = 0; i < dataLength; ++i) {
		response.PushBack(result[i]);
	}

	return response;
}

void Application::LoadIdentities() {
	mIdentities = mRegistry.getSessions();
	LOG_DBG("Loaded %d identities", mIdentities.size());
}

std::vector<Identity>& Application::GetIdentities() {
	return mIdentities;
}

size_t Application::GetNumIdentities() const {
	return mIdentities.size();
}

Identity& Application::GetIdentityByIndex(size_t index) {
	return mIdentities[index];
}

size_t Application::AddIdentity(Identity& inIdent) {
	mIdentities.push_back(inIdent);
	return mIdentities.size() - 1;
}

bool Application::RemoveIdentityByIndex(size_t index) {
	if (index < mIdentities.size()) {
		if (mRegistry.removeSession(mIdentities[index])) {
			// remove from list if removed from registry
			mIdentities.erase(mIdentities.begin() + index);
			return true;
		}
	}

	return false;
}

bool Application::SaveIdentity(Identity& identity) {
	if (identity.name.empty()) {
		LOG_WARN("Empty name on identity");
		return false;
	}

	mRegistry.createSession(identity);

	std::string identName(identity.name.begin(), identity.name.end());
	LOG_DBG("Saved identity: %s", identName.c_str());
	return true;
}

uint32_t Application::GetNumLoadedKeys() {
	uint32_t numKeys = 0;
	for (uint32_t i = 0; i < mIdentities.size(); ++i) {
		Identity ident = mIdentities[i];
		if (!ident.pubkey_cached.Empty()) {
			numKeys++;
		}
	}
	return numKeys;
}

ByteArray Application::GetPubKeyFor(const Identity& identity) {
	if (!TryOpenDevice()) {
		return {};
	}

	ByteArray path = identity.GetPathBIP32();

	// APDU for public key ssh
	APDU dataApdu(0x80, 0x02, 0x00, 0x01, path);

	uint16_t status = CODE_SUCCESS;
	ByteArray response = Exchange(dataApdu, &status);

	std::string possibleCause = "";
	if (status != 0x9000 && (status & 0xFF00) != 0x6100 && (status & 0xFF00) != 0x6C00) {
		possibleCause = "Unknown reason, Ledger not connected?";
		if (status == 0x6982)
			possibleCause = "Have you uninstalled the existing CA with resetCustomCA first?";
		if (status == 0x6985) {
			possibleCause = "Condition of use not satisfied (denied by the user?)";
			return ByteArray();
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
		return ByteArray();
	}

	ByteArray blob;

	// if (SSH_NIST256_CURVE_NAME)
	const uint32_t readOffset = 1;
	if ((response[64 + readOffset] & 1) != 0) {
		// indicate compression with 0x03
		blob.PushBack((uint8_t)0x03);
	}
	else {
		blob.PushBack((uint8_t)0x02);
	}

	// copy 32 response bytes
	for (uint32_t idx = 1; idx < 33; ++idx) {
		blob.PushBack(response[idx + readOffset]);
	}

	ByteArray keyinfo;
	// SSH_NIST256_KEY_TYPE
	keyinfo.PushBack(SSH_NIST256_KEY_TYPE.length());
	keyinfo.PushBack((uint8_t*)SSH_NIST256_KEY_TYPE.data(), SSH_NIST256_KEY_TYPE.length());
	// curve_name
	keyinfo.PushBack(SSH_NIST256_CURVE_NAME.length());
	keyinfo.PushBack((uint8_t*)SSH_NIST256_CURVE_NAME.data(), SSH_NIST256_CURVE_NAME.length());
	// key_blob
	ByteArray pubkeyDecompressed = encodeUtils::decompressPubKey(blob);
	keyinfo.PushBack(pubkeyDecompressed.Size() + 1);
	keyinfo.PushBack((uint8_t)SSH_NIST256_DER_OCTET);
	keyinfo.PushBack(pubkeyDecompressed);

	return keyinfo;
}

std::string Application::GetPubKeyStrFor(const ByteArray& keyBlob, const Identity& identity) {
	return SSH_NIST256_KEY_TYPE + " " + encodeUtils::encodeBase64(keyBlob.AsString()) + " <" + identity.ToString() + ">";
}

MemoryMap& Application::GetOrCreateMap(const std::string& identifier) {
	std::vector<MemoryMap>::iterator iterator = mMemoryMaps.begin();
	std::vector<MemoryMap>::iterator itEnd = mMemoryMaps.end();
	for (iterator; iterator != itEnd; ++iterator) {
		if ((*iterator).GetName() == identifier) {
			return *iterator;
		}
	}

	mMemoryMaps.push_back(MemoryMap(identifier));
	return mMemoryMaps.back();
}

bool Application::HandleMemoryMap(MemoryMap& inMap) {
	inMap.Open();
	inMap.Seek(0);

	uint32_t operationOffset = inMap.ReadInt();
	if (operationOffset == 0) {
		LOG_DBG("No identity was accepted");
		return false;
	}
	else if (operationOffset > 1) {
		// Unclear where this clamp comes from
		operationOffset = 1;
	}

	uint8_t operation = *inMap.ReadBytes(operationOffset);
	if (operation == SSH2_AGENTC_REQUEST_IDENTITIES) {
		PresentPubKeys(inMap);
		return true;
	}
	else if (operation == SSH2_AGENTC_SIGN_REQUEST) {
		bool foundKey = false;
		Identity ident;

		// first data after the 13 is the public key itself.
		for (uint32_t i = 0; i < GetNumIdentities(); ++i) {
			// start at offset
			inMap.Seek(4 + operationOffset);

			uint32_t keyLen = inMap.ReadInt();

			// if this identity is loaded (has a key)
			Identity curIdent = GetIdentityByIndex(i);
			if (!curIdent.pubkey_cached.Empty()) {
				// read map for key:
				ByteArray mapArray;
				mapArray.PushBack(inMap.ReadBytes(keyLen), keyLen);
				if (curIdent.pubkey_cached == mapArray) {
					foundKey = true;
					ident = curIdent;
					break;
				}
			}
		}

		if (!foundKey) {
			LOG_ERR("Error: accepted key not found.");
			ByteArray response;
			response.PushBack((uint8_t)SSH_AGENT_FAILURE);

			ByteArray agent_response;
			agent_response.PushBack((uint32_t)response.Size());
			agent_response.PushBack((uint8_t*)response.Get().data(), response.Size());

			inMap.Seek(0);
			inMap.Write(agent_response);
			return false;
		}

		std::string identName(ident.name.begin(), ident.name.end());
		LOG_DBG("Identity %s was accepted", identName.c_str());

		SignMap(inMap, ident);

		inMap.Close();
		return true;
	}
	else {
		LOG_DBG("Unknown Operation %s", operation);
	}

	return false;
}

void Application::PresentPubKeys(MemoryMap& inMap) {
	// count identities that have keys loaded:
	uint32_t numKeys = GetNumLoadedKeys();
	if (numKeys == 0) {
		int msgboxID = MessageBox(NULL, L"No Identities have keys loaded, Please load Keys to continue.", L"Ledger Pageant", MB_ICONEXCLAMATION | MB_OK | MB_DEFBUTTON2);
		if (msgboxID == IDOK) {
			return;
		}
	}

	if (!TryOpenDevice()) {
		return;
	}

	ByteArray response;
	response.PushBack((uint8_t)SSH2_AGENT_IDENTITIES_ANSWER);
	response.PushBack((uint32_t)numKeys);

	for (uint32_t i = 0; i < mIdentities.size(); ++i) {
		Identity ident = mIdentities[i];

		// skip unloaded
		if (ident.pubkey_cached.Empty()) {
			continue;
		}

		// key
		response.PushBack((uint32_t)ident.pubkey_cached.Size());
		response.PushBack((uint8_t*)ident.pubkey_cached.Get().data(), ident.pubkey_cached.Size());

		// comment
		std::string pathComment = ident.ToString();
		response.PushBack((uint32_t)pathComment.length());
		response.PushBack((uint8_t*)pathComment.data(), pathComment.length());
	}

	ByteArray agent_response;
	agent_response.PushBack((uint32_t)response.Size());
	agent_response.PushBack((uint8_t*)response.Get().data(), response.Size());

	inMap.Seek(0);
	inMap.Write(agent_response);
}

void Application::SignMap(MemoryMap& inMap, Identity& ident) {
	// read challenge from map
	size_t challenge_size = (size_t)inMap.ReadInt();
	uint8_t* challenge = inMap.ReadBytes(challenge_size);

	ByteArray challenge_blob;
	challenge_blob.PushBack(challenge, challenge_size);

	// >
	uint32_t offset = 0;
	ByteArray signature;
	uint32_t chunk_size = 0;
	uint32_t challengeLen = challenge_blob.Size();
	while (offset != challengeLen) {
		ByteArray response_data;
		if (offset == 0) {
			ByteArray dongle_path = ident.GetPathBIP32();

			for (uint32_t i = 0; i < dongle_path.Size(); ++i) {
				response_data.PushBack(dongle_path[i]);
			}
		}

		if ((challengeLen - offset) > (255 - response_data.Size())) {
			chunk_size = (255 - response_data.Size());
		}
		else {
			chunk_size = challengeLen - offset;
		}

		for (uint32_t i = offset; i < (offset + chunk_size); ++i) {
			response_data.PushBack(challenge_blob[i]);
		}

		offset += chunk_size;

		// challenge response apdu:
		APDU dataApdu(0x80, 0x04, 0x00, 0x81, response_data);

		uint16_t status = CODE_SUCCESS;
		signature = Exchange(dataApdu, &status);
		if (status != CODE_SUCCESS) {
			return;
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

	ByteArray encoded_signature_value;
	encoded_signature_value.PushBack((uint32_t)r.size());
	encoded_signature_value.PushBack((uint8_t*)r.data(), r.size());

	encoded_signature_value.PushBack((uint32_t)s.size());
	encoded_signature_value.PushBack((uint8_t*)s.data(), s.size());

	ByteArray encoded_signature;
	const std::string keyType(SSH_NIST256_KEY_TYPE);
	encoded_signature.PushBack((uint32_t)keyType.length());
	encoded_signature.PushBack((uint8_t*)keyType.c_str(), keyType.size());

	encoded_signature.PushBack((uint32_t)encoded_signature_value.Size());
	encoded_signature.PushBack((uint8_t*)encoded_signature_value.Get().data(), encoded_signature_value.Size());

	// < response
	ByteArray response;
	response.PushBack((uint8_t)SSH2_AGENT_SIGN_RESPONSE);
	response.PushBack((uint32_t)encoded_signature.Size());
	response.PushBack((uint8_t*)encoded_signature.Get().data(), encoded_signature.Size());

	ByteArray agent_response;
	agent_response.PushBack((uint32_t)response.Size());
	agent_response.PushBack((uint8_t*)response.Get().data(), response.Size());

	inMap.Seek(0);
	inMap.Write(agent_response);
}
