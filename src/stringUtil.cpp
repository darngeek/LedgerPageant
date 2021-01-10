#include "stringUtil.h"

#include "window.h"

namespace stringUtil {
	std::wstring s2ws(const std::string& s) {
		size_t slength = s.length() + 1;
		size_t len = ::MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0); 
		std::wstring r(len, L'\0');
		::MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, &r[0], len);
		return r;
	}

	std::string ws2s(const std::wstring& s) {
		size_t slength = s.length();// + 1;
		size_t wlength = ::WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, 0, 0, 0, 0); 
		std::string r(wlength, '\0');
		::WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, &r[0], wlength, 0, 0); 
		return r;
	}
}
