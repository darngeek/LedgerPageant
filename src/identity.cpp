#include "identity.h"

#include "window.h"
#include "application.h"
#include "encodeUtil.h"
#include "stringUtil.h"
#include "logger.h"

Identity::Identity() {
}

Identity::Identity(const std::string& identStr) {
	if (!FromString(identStr)) {
		LOG_ERR("Invalid identity string");
	}
}

Identity::~Identity() {

}

void Identity::InitKeyType(std::string keyTypeStr) {
	Application* app = Window::GetPtr()->GetApplication();
	keyType = app->GetKeyTypeByIndex(app->GetKeyTypeIndexByName(keyTypeStr));
}

bool Identity::FromString(std::string identStr) {
	std::wregex identity_regex(L"^(?:(\\w+)://)?(?:(.*?)@)?([^:/]*)(?::(\\d+))?(?:/(.*))?$");
	std::wsmatch base_match;
	std::wstring identWstr = stringUtil::s2ws(identStr);

	std::regex_match(identWstr, base_match, identity_regex);

	if (base_match.size() != 6) {
		return false;
	}

	protocol = base_match[1].str();
	user = base_match[2].str();
	host = base_match[3].str();
	port = std::stoi(base_match[4].str());
	path = base_match[5].str();

	return true;
}

std::string Identity::ToString() const {
	std::wstring result = L"";

	if (!protocol.empty()) {
		result += protocol + L"://";
	}
	else {
		// default ssh assumed
		result += L"ssh://";
	}

	if (!user.empty()) {
		result += user + L"@";
	}

	result += host;

	if (port > 0) {
		result += L":" + port;
	}/* else {
		if (protocol.empty() || protocol == L"ssh") {
			// default :22 assumed when no protocol or ssh
			result += L":22";
		}
	}*/

	if (!path.empty()) {
		result += L"/" + path;
	}

	return stringUtil::ws2s(result);
}

ByteArray Identity::GetAddress(bool ecdh) const {
	std::string addr;
	addr.push_back(0x00);
	addr.push_back(0x00);
	addr.push_back(0x00);
	addr.push_back(0x00);
	addr += ToString();
	std::string digest = encodeUtils::makeSha256(addr);

	constexpr uint32_t hardened_mask = 0x80000000;
	constexpr uint8_t hardened_mask_byte = 0x80;

	ByteArray address;

	uint32_t addr_0 = ecdh ? 17 : 13;
	address.PushBack(hardened_mask | addr_0);

	for (uint32_t i = 0; i < 16; i += 4) {
		address.PushBack((uint8_t)(hardened_mask_byte | (uint8_t)digest[i + 3]));
		address.PushBack((uint8_t)digest[i + 2]);
		address.PushBack((uint8_t)digest[i + 1]);
		address.PushBack((uint8_t)digest[i + 0]);
	}

	return address;
}

ByteArray Identity::GetPathBIP32() const {
	ByteArray path = GetAddress(false);
	uint8_t numInts = (uint8_t)std::floor((path.Size() + 1) / 4);

	ByteArray result;
	result.PushBack(numInts);
	result.PushBack(path);

	return result;
}
