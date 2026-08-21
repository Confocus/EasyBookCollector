#pragma once
#include <Windows.h>
#include <iostream>

namespace PublicFuncs {
	std::wstring UTF8ToWString(const char* utf8, int length);
	std::wstring Trim(std::wstring_view s);
}