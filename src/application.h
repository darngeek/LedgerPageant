#pragma once

#include <vector>
#include "identity.h"
#include "memoryMap.h"
#include "ledger_device.h"
#include "agent.h"
#include "registryInterface.h"

class Application {
public:
	Application();
	~Application();

	// device
	bool isDeviceReady();

	// identity
	void importIdentities();
	std::vector<Identity>& getIdentities();
	size_t getNumIdentities() const;
	Identity& getIdentityByIndex(size_t index);
	size_t addIdentity(Identity& inIdent);
	bool removeIdentityByIndex(size_t index);
	bool writeIdentity(Identity& identity);

	// key
	uint32_t getNumLoadedKeys();
	byte_array getPubKeyFor(Identity& identity);
	std::string getPubKeyStrFor(byte_array& keyBlob, Identity& identity);
	void presentPubKeys(MemoryMap& map);
	void sign(MemoryMap& map, Identity& ident);

	// copy-data
	MemoryMap& getOrCreateMap(const std::string& identifier);
	bool handleMemoryMap(MemoryMap& inMap);

private:
	bool isDeviceConnected;
	ledger_device mDevice;
	agent mAgent;
	RegistryInterface regInt;

	std::vector<MemoryMap> mMemoryMaps;
	std::vector<Identity> mIdentities;
};
