/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC applications.                                           *
* Official git repository of the project: https://github.com/jovibor/HexCtrl/           *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
****************************************************************************************/
/****************************************************************************************
* Here dwells some useful helper stuff for HexCtrl.                                     *
****************************************************************************************/
#pragma once
#include <afxwin.h>
#include <string>

#define HEXCTRL_PRODUCT_NAME			"Hex Control for MFC/Win32"
#define HEXCTRL_COPYRIGHT_NAME  		"(C) 2018-2021 Jovibor"
#define HEXCTRL_VERSION_MAJOR			2
#define HEXCTRL_VERSION_MINOR			19
#define HEXCTRL_VERSION_MAINTENANCE		2

namespace HEXCTRL::INTERNAL
{
	//Default codepage (Windows-1250) when HexCtrl code page is set to -1.
	//This is mainly to prevent any non 1-byte ASCII default code pages 
	//that could be set in Windows (if CP_ACP would have been set here instead).
	constexpr auto HEXCTRL_CODEPAGE_DEFAULT { 1250U };

	//Fast lookup wchar_t array.
	constexpr const wchar_t* const g_pwszHexMap { L"0123456789ABCDEF" };

	//Converts dwSize bytes of ull to WCHAR string.
	void UllToWchars(ULONGLONG ull, wchar_t* pwsz, size_t dwSize, bool fAsHex = true);

	//Converts wide string to template's numeric data type.
	template<typename T>
	bool wstr2num(const std::wstring& wstr, T& tData, int iBase = 0);

	//Converts multibyte string to template's numeric data type.
	template<typename T>
	bool str2num(const std::string& str, T& tData, int iBase = 0);

	//Converts every two numeric chars to one respective hex character: "56"->V(0x56), "7A"->z(0x7A)
	//fWc means that wildcards are allowed, uWc - the wildcard.
	bool str2hex(const std::string& str, std::string& strToHex, bool fWc = false, char chWc = '?');

	//Wide to Multibyte string convertion.
	std::string wstr2str(std::wstring_view wstr, UINT uCodePage = CP_UTF8);

	//Multibyte to Wide string convertion.
	std::wstring str2wstr(std::string_view str, UINT uCodePage = CP_UTF8);

	//Substitute all unprintable wchar symbols with dot.
	void ReplaceUnprintable(std::wstring& wstr, bool fASCII, bool fCRLFRepl = true);

#define TO_STR_HELPER(x) #x
#define TO_STR(x) TO_STR_HELPER(x)
#ifdef _WIN64
#define HEXCTRL_VERSION_WSTR HEXCTRL_PRODUCT_NAME L", v" TO_STR(HEXCTRL_VERSION_MAJOR) "."\
		TO_STR(HEXCTRL_VERSION_MINOR) "." TO_STR(HEXCTRL_VERSION_MAINTENANCE) " (x64)"
#else
#define HEXCTRL_VERSION_WSTR HEXCTRL_PRODUCT_NAME L", v" TO_STR(HEXCTRL_VERSION_MAJOR) "."\
		TO_STR(HEXCTRL_VERSION_MINOR) "." TO_STR(HEXCTRL_VERSION_MAINTENANCE) " (x86)"
#endif
};