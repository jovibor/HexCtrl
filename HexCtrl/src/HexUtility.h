/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
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
#define HEXCTRL_VERSION_MINOR		1
#define HEXCTRL_VERSION_MAINTENANCE	0

#define TO_WSTR_HELPER(x) L## #x
#define TO_WSTR(x) TO_WSTR_HELPER(x)
#define HEXCTRL_FULL_VERSION_RAW HEXCTRL_PRODUCT_NAME L", v" TO_WSTR(HEXCTRL_VERSION_MAJOR) L"."\
		TO_WSTR(HEXCTRL_VERSION_MINOR) L"." TO_WSTR(HEXCTRL_VERSION_MAINTENANCE)

namespace HEXCTRL::INTERNAL
{
	//HexCtrl version and name constants for exporting.
	constexpr auto WSTR_HEXCTRL_PRODUCT_NAME = L"" HEXCTRL_PRODUCT_NAME;
	constexpr auto WSTR_HEXCTRL_COPYRIGHT_NAME = L"" HEXCTRL_COPYRIGHT_NAME;
	constexpr auto ID_HEXCTRL_VERSION_MAJOR = HEXCTRL_VERSION_MAJOR;
	constexpr auto ID_HEXCTRL_VERSION_MINOR = HEXCTRL_VERSION_MINOR;
	constexpr auto ID_HEXCTRL_VERSION_MAINTENANCE = HEXCTRL_VERSION_MAINTENANCE;
#ifdef _WIN64
	constexpr auto WSTR_HEXCTRL_FULL_VERSION = HEXCTRL_FULL_VERSION_RAW L" (x64)";
#else
	constexpr auto WSTR_HEXCTRL_FULL_VERSION = HEXCTRL_FULL_VERSION_RAW L" (x86)";
#endif

	//Converts wide string to a templated arithmetic data type.
	template<typename TArithmetic> requires std::is_arithmetic_v<TArithmetic>
	[[nodiscard]] auto StringToNum(const std::wstring& wstr, int iBase = 0)->std::optional<TArithmetic>;

	//Converts every two numeric wchars to one respective hex character: "56"->V(0x56), "7A"->z(0x7A)
	//fWc means that wildcards are allowed, chWc - the wildcard.
	[[nodiscard]] auto StringToHex(std::wstring_view wstr, bool fWc = false, char chWc = '?')->std::optional<std::string>;

	//Wide to Multibyte string convertion.
	[[nodiscard]] std::string WstrToStr(std::wstring_view wstr, UINT uCodePage = CP_UTF8);

	//Multibyte to Wide string convertion.
	[[nodiscard]] std::wstring StrToWstr(std::string_view str, UINT uCodePage = CP_UTF8);

	//Convert string into FILETIME struct.
	[[nodiscard]] auto StringToFileTime(std::wstring_view wstr, DWORD dwFormat)->std::optional<FILETIME>;

	//Convert string into SYSTEMTIME struct.
	[[nodiscard]] auto StringToSystemTime(std::wstring_view wstr, DWORD dwFormat)->std::optional<SYSTEMTIME>;

	//Convert FILETIME struct to a readable string.
	[[nodiscard]] auto FileTimeToString(const FILETIME& stFileTime, DWORD dwFormat, wchar_t wchSepar)->std::wstring;

	//Convert SYSTEMTIME struct to a readable string.
	[[nodiscard]] auto SystemTimeToString(const SYSTEMTIME& stSysTime, DWORD dwFormat, wchar_t wchSepar)->std::wstring;

	//String of date/time in given format with given date separator.
	[[nodiscard]] auto GetDateFormatString(DWORD dwFormat, wchar_t wchSepar)->std::wstring;

	//Get data from IHexCtrl's given offset converted to a necessary type.
	template<typename T>
	[[nodiscard]] inline T GetIHexTData(const IHexCtrl& refHexCtrl, ULONGLONG ullOffset)
	{
		const auto spnData = refHexCtrl.GetData({ ullOffset, sizeof(T) });
		assert(!spnData.empty());
		return *reinterpret_cast<T*>(spnData.data());
	}

	//Set data of a necessary type to IHexCtrl's given offset.
	template<typename T>
	inline void SetIHexTData(IHexCtrl& refHexCtrl, ULONGLONG ullOffset, T tData)
	{
		//Data overflow check.
		assert(ullOffset + sizeof(T) <= refHexCtrl.GetDataSize());
		if (ullOffset + sizeof(T) > refHexCtrl.GetDataSize())
			return;

		refHexCtrl.ModifyData({ .enModifyMode = EHexModifyMode::MODIFY_ONCE,
			.spnData = { reinterpret_cast<std::byte*>(&tData), sizeof(T) },
			.vecSpan = { { ullOffset, sizeof(T) } } });
	}
};