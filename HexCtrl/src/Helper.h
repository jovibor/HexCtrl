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
#include "version.h"
#include <afxwin.h>
#include <string>

namespace HEXCTRL::INTERNAL {
	//Fast lookup char/wchar_t arrays.
	inline const wchar_t* const g_pwszHexMap { L"0123456789ABCDEF" };
	inline const char* const g_pszHexMap { "0123456789ABCDEF" };

	//Converts dwSize bytes of ull to WCHAR string.
	void UllToWchars(ULONGLONG ull, wchar_t* pwsz, size_t dwSize, bool fAsHex = true);

	//Converts wide string to template's numeric data type.
	template<typename T>
	inline bool wstr2num(std::wstring_view wstr, T& tData, int iBase = 0);
	
	//Converts multibyte string to template's numeric data type.
	template<typename T>
	inline bool str2num(std::string_view str, T& tData, int iBase = 0);

	//Converts every two numeric chars to one respective hex character: "56"->V(0x56), "7A"->z(0x7A)
	bool StrToHex(std::string_view str, std::string& strToHex);

	//Wide to Multibyte string convertion.
	std::string WstrToStr(std::wstring_view wstr);

	//Multibyte to Wide string convertion.
	std::wstring StrToWstr(std::string_view str);

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#ifdef _WIN64
	constexpr auto HEXCTRL_VERSION_WSTR = L"" STR(MAJOR_VERSION) "." STR(MINOR_VERSION) "." STR(MAINTENANCE_VERSION) " (x64)";
#else
	constexpr auto HEXCTRL_VERSION_WSTR = L"" STR(MAJOR_VERSION) "." STR(MINOR_VERSION) "." STR(MAINTENANCE_VERSION);
#endif
	constexpr auto HEXCTRL_VERSION_ULL = ULONGLONG(((ULONGLONG)MAJOR_VERSION << 48)
		| ((ULONGLONG)MINOR_VERSION << 32) | ((ULONGLONG)MAINTENANCE_VERSION << 16) | (ULONGLONG)REVISION_VERSION);
};