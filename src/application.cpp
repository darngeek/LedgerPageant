#include "application.h"

#include "logger.h"
#include "registryInterface.h"
#include "apdu.h"

#define SSH_AGENT_FAILURE 5
#define SSH2_AGENTC_REQUEST_IDENTITIES 11
#define SSH2_AGENT_IDENTITIES_ANSWER 12
#define SSH2_AGENTC_SIGN_REQUEST 13
#define SSH2_AGENT_SIGN_RESPONSE 14


Application::Application()
: isDeviceConnected(false)
, mDevice()
, mAgent(&mDevice)
{
	// startup sequence
	importIdentities();

	// initial check for device and app
	mDevice.updateIsDevicePresent();
}

Application::~Application() {
	
}

bool Application::isDeviceReady()  {
	bool success = true;

	mDevice.updateIsDevicePresent();

	// if we arent ready prompt user to connect
	while(!mDevice.isDeviceReady()) {
		LPCWSTR title = L"Ledger - Pageant";
		LPCWSTR description;
		if (!mDevice.isDeviceAppReady()) {
			description = L"Could not connect with SSH/PGP Agent on Ledger Nano S";
		}
		else {
			description = L"Could not connect with Ledger Nano S";
		}

		int msgboxID = MessageBox(NULL, description, title, MB_ICONQUESTION | MB_CANCELTRYCONTINUE | MB_DEFBUTTON2);
		if (msgboxID == IDTRYAGAIN) {
			mDevice.updateIsDevicePresent();
		}
		else {
			success = false;
			break;
		}
	}

	return success;
}

void Application::importIdentities() {
	mIdentities = regInt.getSessions();
	LOG_DBG("Loaded %d identities", mIdentities.size());
}

std::vector<Identity>& Application::getIdentities() {
	return mIdentities;
}

size_t Application::getNumIdentities() const {
	return mIdentities.size();
}

Identity& Application::getIdentityByIndex(size_t index) {
	return mIdentities[index];
}

size_t Application::addIdentity(Identity& inIdent) {
	mIdentities.push_back(inIdent);
	return mIdentities.size() - 1;
}

bool Application::removeIdentityByIndex(size_t index) {
	if (index < mIdentities.size()) {
		if (regInt.removeSession(mIdentities[index])) {
			// remove from list if removed from registry
			mIdentities.erase(mIdentities.begin() + index);
			return true;
		}
	}

	return false;
}

bool Application::writeIdentity(Identity& identity) {
	if (identity.name.empty()) {
		LOG_WARN("Empty name on identity");
		return false;
	}

	int index = regInt.createSession(identity);
	std::string identName(identity.name.begin(), identity.name.end());
	LOG_DBG("Saved identity: %s", identName.c_str());
	return true;
}

byte_array Application::getPubKeyFor(Identity& identity) {
	if (!isDeviceReady()) {
		return {};
	}

	return mAgent.get_public_key(identity);
}

std::string Application::getPubKeyStrFor(byte_array& keyBlob, Identity& identity) {
	return mAgent.printPubKey(keyBlob, identity);
}

uint32_t Application::getNumLoadedKeys() {
	uint32_t numKeys = 0;
	for (uint32_t i = 0; i < mIdentities.size(); ++i) {
		Identity ident = mIdentities[i];
		if (!ident.pubkey_cached.empty()) {
			numKeys++;
		}
	}
	return numKeys;
}

void Application::presentPubKeys(MemoryMap& inMap) {
	// count identities that have keys loaded:
	uint32_t numKeys = getNumLoadedKeys();
	if (numKeys == 0) {
		int msgboxID = MessageBox(NULL, L"No Identities have keys loaded, Please load Keys to continue.", L"Ledger Pageant", MB_ICONEXCLAMATION | MB_OK | MB_DEFBUTTON2);
		if (msgboxID == IDOK) {
			return;
		}
	}

	if (!isDeviceReady()) {
		return;
	}

	byte_array response;
	response.push_back((uint8_t)SSH2_AGENT_IDENTITIES_ANSWER);
	response.push_back((uint32_t)numKeys);

	for (uint32_t i = 0; i < mIdentities.size(); ++i) {
		Identity ident = mIdentities[i];

		// skip unloaded
		if (ident.pubkey_cached.empty()) {
			continue;
		}

		// key
		// byte_array pubKey = getPubKeyFor(ident);
		response.push_back((uint32_t)ident.pubkey_cached.size());
		response.push_back((uint8_t*)ident.pubkey_cached.get().data(), ident.pubkey_cached.size());

		// comment
		std::string pathComment = ident.toUrl();
		response.push_back((uint32_t)pathComment.length());
		response.push_back((uint8_t*)pathComment.data(), pathComment.length());
	}

	byte_array agent_response;
	agent_response.push_back((uint32_t)response.size());
	agent_response.push_back((uint8_t*)response.get().data(), response.size());

	inMap.seek(0);
	inMap.write((uint8_t*)agent_response.get().data(), agent_response.size());
}

void Application::sign(MemoryMap& inMap, Identity& ident) {
	// read challenge from map
	uint8_t* d = (uint8_t*)inMap.read(4);
	size_t challenge_size = (d[0] << 24) | (d[1] << 16) | (d[2] << 8) | d[3];
	uint8_t* challenge = (uint8_t*)inMap.read(challenge_size);

	byte_array challenge_blob;
	challenge_blob.push_back(challenge, challenge_size);

	byte_array encoded_signature = mAgent.sign(challenge_blob, ident);
	    
    byte_array response;
    response.push_back((uint8_t)SSH2_AGENT_SIGN_RESPONSE);
    response.push_back((uint32_t)encoded_signature.size());
    response.push_back((uint8_t*)encoded_signature.get().data(), encoded_signature.size());

    byte_array agent_response;
    agent_response.push_back((uint32_t)response.size());
    agent_response.push_back((uint8_t*)response.get().data(), response.size());

    inMap.seek(0);
    inMap.write((uint8_t*)agent_response.get().data(), agent_response.size());
}

MemoryMap& Application::getOrCreateMap(const std::string& identifier) {
	std::vector<MemoryMap>::iterator iterator = mMemoryMaps.begin();
	std::vector<MemoryMap>::iterator itEnd = mMemoryMaps.end();
	for (iterator; iterator != itEnd; iterator++) {
		if ((*iterator).getName() == identifier) {
			return *iterator;
		}
	}

	mMemoryMaps.push_back(MemoryMap(identifier));
	return mMemoryMaps.back();
}

bool Application::handleMemoryMap(MemoryMap& inMap) {
	inMap.open();
	inMap.seek(0);

	// read accepted identity index: first 4 bytes
	uint8_t* d = (uint8_t*)inMap.read(4);
	uint32_t operationOffset = (d[0] << 24) | (d[1] << 16) | (d[2] << 8) | d[3];

	if (operationOffset == 0) {
		LOG_DBG("No identity was accepted");
		return false;
	}
	else if(operationOffset > 1){
		// Unclear where this clamp comes from
		operationOffset = 1;
	}

	uint8_t* operation = (uint8_t*)inMap.read(operationOffset);

	if (*operation == SSH2_AGENTC_REQUEST_IDENTITIES) {
		presentPubKeys(inMap);
		return true;
	}
	else if (*operation == SSH2_AGENTC_SIGN_REQUEST) {
		bool foundKey = false;
		Identity ident;

		// first data after the 13 is the public key itself.
		for (uint32_t i = 0; i < getNumIdentities(); ++i) {
			// start at offset
			inMap.seek(4+operationOffset);

			// read len:
			uint8_t* d = (uint8_t*)inMap.read(4);
			uint32_t keyLen = (d[0] << 24) | (d[1] << 16) | (d[2] << 8) | d[3];

			// if this identity is loaded (has a key)
			Identity curIdent = getIdentityByIndex(i);
			if (!curIdent.pubkey_cached.empty()) {
				// read map for key:
				byte_array mapArray;
				mapArray.push_back(inMap.read(keyLen), keyLen);
				if (curIdent.pubkey_cached == mapArray) {
					foundKey = true;
					ident = curIdent;
					break;
				}
			}
		}

		if (!foundKey) {
			LOG_ERR("Error: accepted key not found.");
			byte_array response;
			response.push_back((uint8_t)SSH_AGENT_FAILURE);

			byte_array agent_response;
			agent_response.push_back((uint32_t)response.size());
			agent_response.push_back((uint8_t*)response.get().data(), response.size());

			inMap.seek(0);
			inMap.write((uint8_t*)agent_response.get().data(), agent_response.size());
			return false;
		}

		std::string identName(ident.name.begin(), ident.name.end());
		LOG_DBG("Identity %s was accepted", identName.c_str());
		
		sign(inMap, ident);

		inMap.close();
		return true;
	}
	else {
		LOG_DBG("Unknown Operation %s", *operation);
	}

	return false;
}
