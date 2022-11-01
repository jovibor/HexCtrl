/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/

//Here dwells some useful helper stuff for HexCtrl.
#pragma once
#include "../dep/StrToNum/StrToNum.h"
#include "../HexCtrl.h"
#include <afxwin.h>
#include <cassert>
#include <optional>
#include <string>

#define HEXCTRL_PRODUCT_NAME		L"Hex Control for MFC/Win32"
#define HEXCTRL_COPYRIGHT_NAME  	L"(C) 2018-2023 Jovibor"
#define HEXCTRL_VERSION_MAJOR		3
#define HEXCTRL_VERSION_MINOR		2
#define HEXCTRL_VERSION_MAINTENANCE	0

#define TO_WSTR_HELPER(x) L## #x
#define TO_WSTR(x) TO_WSTR_HELPER(x)
#define HEXCTRL_FULL_VERSION_RAW HEXCTRL_PRODUCT_NAME L", v" TO_WSTR(HEXCTRL_VERSION_MAJOR) L"."\
		TO_WSTR(HEXCTRL_VERSION_MINOR) L"." TO_WSTR(HEXCTRL_VERSION_MAINTENANCE)

namespace HEXCTRL::INTERNAL
{
	//HexCtrl version and name constants for exporting.
	constexpr auto WSTR_HEXCTRL_PRODUCT_NAME = HEXCTRL_PRODUCT_NAME;
	constexpr auto WSTR_HEXCTRL_COPYRIGHT_NAME = HEXCTRL_COPYRIGHT_NAME;
	constexpr auto ID_HEXCTRL_VERSION_MAJOR = HEXCTRL_VERSION_MAJOR;
	constexpr auto ID_HEXCTRL_VERSION_MINOR = HEXCTRL_VERSION_MINOR;
	constexpr auto ID_HEXCTRL_VERSION_MAINTENANCE = HEXCTRL_VERSION_MAINTENANCE;
#ifdef _WIN64
	constexpr auto WSTR_HEXCTRL_FULL_VERSION = HEXCTRL_FULL_VERSION_RAW L" (x64)";
#else
	constexpr auto WSTR_HEXCTRL_FULL_VERSION = HEXCTRL_FULL_VERSION_RAW L" (x86)";
#endif

	using namespace stn; //To enable StrTo* methods internally.

	//Converts every two numeric wchars to one respective hex character: "56"->V(0x56), "7A"->z(0x7A)
	//fWc means that wildcards are allowed, chWc - the wildcard.
	[[nodiscard]] auto NumStrToHex(std::wstring_view wsv, bool fWc = false, char chWc = '?')->std::optional<std::string>;

	//Wide to Multibyte string convertion.
	[[nodiscard]] auto WstrToStr(std::wstring_view wsv, UINT uCodePage = CP_UTF8)->std::string;

	//Multibyte to Wide string convertion.
	[[nodiscard]] auto StrToWstr(std::string_view sv, UINT uCodePage = CP_UTF8)->std::wstring;

	//Convert string into FILETIME struct.
	[[nodiscard]] auto StringToFileTime(std::wstring_view wsv, DWORD dwFormat)->std::optional<FILETIME>;

	//Convert string into SYSTEMTIME struct.
	[[nodiscard]] auto StringToSystemTime(std::wstring_view wsv, DWORD dwFormat)->std::optional<SYSTEMTIME>;

	//Convert FILETIME struct to a readable string.
	[[nodiscard]] auto FileTimeToString(FILETIME stFileTime, DWORD dwFormat, wchar_t wchSepar)->std::wstring;

	//Convert SYSTEMTIME struct to a readable string.
	[[nodiscard]] auto SystemTimeToString(SYSTEMTIME stSysTime, DWORD dwFormat, wchar_t wchSepar)->std::wstring;

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

	//False value for static_assert.
	template <class>
	inline constexpr bool StaticAssertFalse = false;

	//Byte swap for types of 2, 4, or 8 size.
	template <class T>
	[[nodiscard]] constexpr T ByteSwap(T tData)noexcept
	{
		//Since a converting type can be any type of 2, 4, or 8 size length,
		//we first bit_cast this type to a corresponding integral type of equal size.
		//Then byteswapping and then bit_cast to the original type back before return.
		if constexpr (sizeof(T) == sizeof(unsigned char)) {
			return tData;
		}
		else if constexpr (sizeof(T) == sizeof(unsigned short)) {
			auto wData = std::bit_cast<unsigned short>(tData);
			if (std::is_constant_evaluated()) {
				wData = static_cast<unsigned short>((wData << 8) | (wData >> 8));
				return std::bit_cast<T>(wData);
			}
			return std::bit_cast<T>(_byteswap_ushort(wData));
		}
		else if constexpr (sizeof(T) == sizeof(unsigned long)) {
			auto ulData = std::bit_cast<unsigned long>(tData);
			if (std::is_constant_evaluated()) {
				ulData = (ulData << 24) | ((ulData << 8) & 0x00FF'0000UL) | ((ulData >> 8) & 0x0000'FF00UL) | (ulData >> 24);
				return std::bit_cast<T>(ulData);
			}
			return std::bit_cast<T>(_byteswap_ulong(ulData));
		}
		else if constexpr (sizeof(T) == sizeof(unsigned long long)) {
			auto ullData = std::bit_cast<unsigned long long>(tData);
			if (std::is_constant_evaluated()) {
				ullData = (ullData << 56) | ((ullData << 40) & 0x00FF'0000'0000'0000ULL)
					| ((ullData << 24) & 0x0000'FF00'0000'0000ULL) | ((ullData << 8) & 0x0000'00FF'0000'0000ULL)
					| ((ullData >> 8) & 0x0000'0000'FF00'0000ULL) | ((ullData >> 24) & 0x0000'0000'00FF'0000ULL)
					| ((ullData >> 40) & 0x0000'0000'0000'FF00ULL) | (ullData >> 56);
				return std::bit_cast<T>(ullData);
			}
			return std::bit_cast<T>(_byteswap_uint64(ullData));
		}
		else {
			static_assert(StaticAssertFalse<T>, "Unexpected data size.");
		}
	}
};