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

namespace HEXCTRL::INTERNAL
{	
	//Default codepage (Windows-1250) when HexCtrl code page is set to -1.
	//This is mainly to prevent any non 1-byte ASCII default code pages 
	//that could be used in Windows (if CP_ACP would have been set here instead).
	constexpr UINT CODEPAGE_DEFAULT = { 1250 };
	
	//Fast lookup char/wchar_t arrays.
	inline const wchar_t* const g_pwszHexMap { L"0123456789ABCDEF" };

	//Converts dwSize bytes of ull to WCHAR string.
	void UllToWchars(ULONGLONG ull, wchar_t* pwsz, size_t dwSize, bool fAsHex = true);

	//Converts wide string to template's numeric data type.
	template<typename T>
	inline bool wstr2num(std::wstring_view wstr, T& tData, int iBase = 0);

	//Converts multibyte string to template's numeric data type.
	template<typename T>
	inline bool str2num(std::string_view str, T& tData, int iBase = 0);

	//Converts every two numeric chars to one respective hex character: "56"->V(0x56), "7A"->z(0x7A)
	bool str2hex(std::string_view str, std::string& strToHex);

	//Wide to Multibyte string convertion.
	std::string wstr2str(std::wstring_view wstr, UINT uCodePage = CP_UTF8);

	//Multibyte to Wide string convertion.
	std::wstring str2wstr(std::string_view str, UINT uCodePage = CP_UTF8);

	//Substitute all unprintable wchar symbols with dot.
	void ReplaceUnprintable(std::wstring& wstr, bool fASCII, bool fCRLFRepl = true);

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#ifdef _WIN64
	constexpr auto HEXCTRL_VERSION_WSTR = L"" STR(MAJOR_VERSION) "." STR(MINOR_VERSION) "." STR(MAINTENANCE_VERSION) " (x64)";
#else
	constexpr auto HEXCTRL_VERSION_WSTR = L"" STR(MAJOR_VERSION) "." STR(MINOR_VERSION) "." STR(MAINTENANCE_VERSION);
#endif
	constexpr auto HEXCTRL_VERSION_ULL = ULONGLONG((static_cast<ULONGLONG>(MAJOR_VERSION) << 48)
		| (static_cast<ULONGLONG>(MINOR_VERSION) << 32) | (static_cast<ULONGLONG>(MAINTENANCE_VERSION) << 16) | static_cast<ULONGLONG>(REVISION_VERSION));
};