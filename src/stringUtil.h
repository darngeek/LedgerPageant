#pragma once

#include <string>

namespace stringUtil {
	std::wstring s2ws(const std::string& s);
	std::string ws2s(const std::wstring& s);
}
