/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
****************************************************************************************/
/****************************************************************************************
* Here dwells some useful helper stuff for HexCtrl.                                     *
****************************************************************************************/
#pragma once
#include "../HexCtrl.h"
#include <afxwin.h>
#include <cassert>
#include <optional>
#include <string>

#define HEXCTRL_PRODUCT_NAME		L"Hex Control for MFC/Win32"
#define HEXCTRL_COPYRIGHT_NAME  	L"(C) 2018-2021 Jovibor"
#define HEXCTRL_VERSION_MAJOR		3
#define HEXCTRL_VERSION_MINOR		0
#define HEXCTRL_VERSION_MAINTENANCE	0

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
	[[nodiscard]] std::string wstr2str(std::wstring_view wstr, UINT uCodePage = CP_UTF8);

	//Multibyte to Wide string convertion.
	[[nodiscard]] std::wstring str2wstr(std::string_view str, UINT uCodePage = CP_UTF8);

	//Substitute all unprintable wchar symbols with dot.
	void ReplaceUnprintable(std::wstring& wstr, bool fASCII, bool fCRLFRepl = true);

	//Convert string_view into SYSTEMTIME struct.
	[[nodiscard]] auto StringToSystemTime(std::wstring_view wstr, DWORD dwDateFormat)->std::optional<SYSTEMTIME>;

	//Get data from IHexCtrl's given offset converted to necessary type.
	template<typename T>
	[[nodiscard]] T GetIHexTData(const IHexCtrl& refHexCtrl, ULONGLONG ullOffset)
	{
		T tData { };
		const auto spnData = refHexCtrl.GetData({ ullOffset, sizeof(T) });
		assert(!spnData.empty());
		if (!spnData.empty())
			tData = *reinterpret_cast<T*>(spnData.data());

		return tData;
	}

	//Set data to IHexCtrl's given offset converted to necessary type.
	template<typename T>
	void SetIHexTData(IHexCtrl& refHexCtrl, ULONGLONG ullOffset, T tData)
	{
		//Data overflow check.
		assert(ullOffset + sizeof(T) <= refHexCtrl.GetDataSize());
		if (ullOffset + sizeof(T) > refHexCtrl.GetDataSize())
			return;

		HEXMODIFY hms;
		hms.enModifyMode = EHexModifyMode::MODIFY_DEFAULT;
		hms.spnData = { reinterpret_cast<std::byte*>(&tData), sizeof(T) };
		hms.vecOffset.emplace_back(HEXOFFSET { ullOffset, sizeof(T) });
		refHexCtrl.ModifyData(hms);
	}

#define TO_WSTR_HELPER(x) L## #x
#define TO_WSTR(x) TO_WSTR_HELPER(x)
#define HEXCTRL_VERSION_WSTR_RAW HEXCTRL_PRODUCT_NAME L", v" TO_WSTR(HEXCTRL_VERSION_MAJOR) L"."\
		TO_WSTR(HEXCTRL_VERSION_MINOR) L"." TO_WSTR(HEXCTRL_VERSION_MAINTENANCE)
#ifdef _WIN64
#define HEXCTRL_VERSION_WSTR HEXCTRL_VERSION_WSTR_RAW L" (x64)"
#else
#define HEXCTRL_VERSION_WSTR HEXCTRL_VERSION_WSTR_RAW L" (x86)"
#endif
};