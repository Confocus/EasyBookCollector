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