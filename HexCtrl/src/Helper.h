/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC applications.                                           *
* Official git repository of the project: https://github.com/jovibor/HexCtrl/           *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
/****************************************************************************************
* These are some helper functions for HexCtrl.											*
****************************************************************************************/
#pragma once
#include <afxwin.h>
#include <string>
#include "../verinfo/version.h"

namespace HEXCTRL {
	namespace INTERNAL {
		//Fast lookup wchar_t array.
		inline const wchar_t* const g_pwszHexMap { L"0123456789ABCDEF" };
		inline const char* const g_pszHexMap { "0123456789ABCDEF" };

		//Converts dwSize bytes of ull to WCHAR string.
		void UllToWchars(ULONGLONG ull, wchar_t* pwsz, size_t dwSize, bool fAsHex = true);

		//Converts char* string to unsigned long number. Basically it's a strtoul() wrapper.
		//Returns false if conversion is imposible, true otherwise.
		bool CharsToUl(const char* pcsz, unsigned long& ul);
		//As above but for wchars*
		bool WCharsToUll(const wchar_t* pwcsz, unsigned long long& ull, bool fHex = true);
		bool WCharsToll(const wchar_t* pwcsz, long long& ll, bool fHex = true);

		//Wide string to Multibyte string convertion.
		std::string WstrToStr(std::wstring_view wstr);

		//Multibyte string to wide string.
		std::wstring StrToWstr(std::string_view str);

		//Converts every two numeric chars to one respective hex character: "56"->V(0x56), "7A"->z(0x7A)
		bool StrToHex(const std::string& strFrom, std::string& strToHex);

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#ifdef _WIN64
		constexpr auto HEXCTRL_VERSION_WSTR = L"" STR(MAJOR_VERSION) "." STR(MINOR_VERSION) "." STR(MAINTENANCE_VERSION) " (x64)";
#else
		constexpr auto HEXCTRL_VERSION_WSTR = L"" STR(MAJOR_VERSION) "." STR(MINOR_VERSION) "." STR(MAINTENANCE_VERSION);
#endif
		constexpr auto HEXCTRL_VERSION_ULL = ULONGLONG(((ULONGLONG)MAJOR_VERSION << 48)
			| ((ULONGLONG)MINOR_VERSION << 32) | ((ULONGLONG)MAINTENANCE_VERSION << 16) | (ULONGLONG)REVISION_VERSION);
	}
};