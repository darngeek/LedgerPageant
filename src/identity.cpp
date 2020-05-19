#include "identity.h"

#include "logger.h"

Identity::Identity() {
}

Identity::Identity(std::string identStr) {
    if (!fromUrl(identStr)) {
        LOG_ERR("Invalid identity string");
    }
}

Identity::~Identity() {

}

bool Identity::fromUrl(std::string identStr) {
    std::wregex identity_regex(L"^(?:(\\w+)://)?(?:(.*?)@)?([^:/]*)(?::(\\d+))?(?:/(.*))?$");
    std::wsmatch base_match;
    std::wstring identWstr = std::wstring(identStr.begin(), identStr.end());
    auto regexthingie = std::regex_match(identWstr, base_match, identity_regex);

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

const std::string Identity::toUrl() {
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

    if (port != 0) {
        result += L":" + port;
    }

    if (!path.empty()) {
        result += L"/" + path;
    }

    return std::string(result.begin(), result.end());
}

static std::string uint32_tobytes_le(uint32_t inInt) {
    std::string result;
    result.reserve(4);

    result.push_back((inInt >> 0) & 0xFF);
    result.push_back((inInt >> 8) & 0xFF);
    result.push_back((inInt >> 16) & 0xFF);
    result.push_back((inInt >> 24) & 0xFF);

    return result;
}

static std::string makeSha256(std::string msg) {
    std::string digest;
    CryptoPP::SHA256 sha256;
    CryptoPP::HashFilter f3(sha256, new CryptoPP::StringSink(digest));

    CryptoPP::ChannelSwitch cs;
    cs.AddDefaultRoute(f3);

    CryptoPP::StringSource ss(msg, true /*pumpAll*/, new CryptoPP::Redirector(cs));

    return digest;
}

byte_array Identity::get_address(bool ecdh) {
    uint32_t index = 0; // TODO: index = (int)self.identity_dict.get('index', 0))

    std::string addr = uint32_tobytes_le(index) + toUrl();
    std::string digest = makeSha256(addr);
    
    constexpr uint32_t hardened_mask = 0x80000000;
    constexpr uint8_t hardened_mask_byte = 0x80;

    byte_array address;

    uint32_t addr_0 = ecdh ? 17 : 13;
    address.push_back(hardened_mask | addr_0);

    // read first 4 unsigned long (16 bytes) from digest bytes (endian flip)
    for (uint32_t i = 0; i < 16; i+=4) {
        address.push_back((uint8_t)(hardened_mask_byte | (uint8_t)digest[i+3]));
        address.push_back((uint8_t)digest[i+2]);
        address.push_back((uint8_t)digest[i+1]);
        address.push_back((uint8_t)digest[i+0]);
    }

    return address;
}

byte_array Identity::getBip32Path() {
    byte_array path = get_address(false);
    uint8_t numInts = (uint8_t)std::floor((path.size() + 1) / 4);

    byte_array result;
    result.push_back(numInts);
    result.push_back(path);

    return result;
}
