#pragma once

#include <vector>
#include "apdu.h"
#include "identity.h"
#include "memoryMap.h"
#include "ledger_device.h"
#include "registryInterface.h"

class Application {
public:
	Application();
	~Application();

	// Device
	bool TryOpenDevice();
	ByteArray WrapApdu(const APDU& command);
	ByteArray UnwrapApdu(ByteArray& data);
	ByteArray Exchange(const APDU& apdu, uint16_t* statusCode);

	// Identity
	void LoadIdentities();
	std::vector<Identity>& GetIdentities();
	size_t GetNumIdentities() const;
	Identity& GetIdentityByIndex(size_t index);
	size_t AddIdentity(Identity& inIdent);
	bool RemoveIdentityByIndex(size_t index);
	bool SaveIdentity(Identity& identity);

	// Public Key
	uint32_t GetNumLoadedKeys();
	ByteArray GetPubKeyFor(const Identity& identity);
	std::string GetPubKeyStrFor(const ByteArray& keyBlob, const Identity& identity);

	// FileMap
	MemoryMap& GetOrCreateMap(const std::string& identifier);
	bool HandleMemoryMap(MemoryMap& inMap);
	void PresentPubKeys(MemoryMap& map);
	void SignMap(MemoryMap& map, Identity& ident);

private:
	bool mIsDeviceConnected = false;
	Device mDevice;
	RegistryInterface mRegistry;

	std::vector<MemoryMap> mMemoryMaps;
	std::vector<Identity> mIdentities;
};
