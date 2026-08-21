#include "publicfuncs.h"
std::wstring PublicFuncs::UTF8ToWString(const char* utf8, int length)
{
	if (!utf8 || length <= 0)
		return {};

	int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8, length, nullptr, 0);
	std::wstring result(wlen, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, utf8, length, &result[0], wlen);
	return result;
}

std::wstring PublicFuncs::Trim(std::wstring_view s)
{
	auto l = s.find_first_not_of(L" \t\n\r");
	auto r = s.find_last_not_of(L" \t\n\r");
	if (l == s.npos) return {};
	return std::wstring(s.substr(l, r - l + 1));
}
