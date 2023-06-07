/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../dep/StrToNum/StrToNum/StrToNum.h"
#include "../HexCtrl.h"
#include <afxwin.h>
#include <cassert>
#include <optional>
#include <string>

namespace HEXCTRL::INTERNAL
{
	//Time calculation constants and structs.
	constexpr auto g_uFTTicksPerMS = 10000U;             //Number of 100ns intervals in a milli-second
	constexpr auto g_uFTTicksPerSec = 10000000UL;        //Number of 100ns intervals in a second
	constexpr auto g_uHoursPerDay = 24U;                 //24 hours per day
	constexpr auto g_uSecondsPerHour = 3600U;            //3600 seconds per hour
	constexpr auto g_uFileTime1582OffsetDays = 6653U;    //FILETIME is based upon 1 Jan 1601 whilst GUID time is from 15 Oct 1582. Add 6653 days to convert to GUID time
	constexpr auto g_ulFileTime1970_LOW = 0xd53e8000UL;  //1st Jan 1970 as FILETIME
	constexpr auto g_ulFileTime1970_HIGH = 0x019db1deUL; //Used for Unix and Java times
	constexpr auto g_ullUnixEpochDiff = 11644473600ULL;  //Number of ticks from FILETIME epoch of 1st Jan 1601 to Unix epoch of 1st Jan 1970

	//Converts every two numeric wchars to one respective hex character: "56"->V(0x56), "7A"->z(0x7A), etc...
	//chWc - a wildcard if any.
	[[nodiscard]] auto NumStrToHex(std::wstring_view wsv, char chWc = 0) -> std::optional<std::string>;

	//Wide to Multibyte string convertion.
	[[nodiscard]] auto WstrToStr(std::wstring_view wsv, UINT uCodePage = CP_UTF8) -> std::string;

	//Multibyte to Wide string convertion.
	[[nodiscard]] auto StrToWstr(std::string_view sv, UINT uCodePage = CP_UTF8) -> std::wstring;

	//Convert string into FILETIME struct.
	[[nodiscard]] auto StringToFileTime(std::wstring_view wsv, DWORD dwFormat) -> std::optional<FILETIME>;

	//Convert string into SYSTEMTIME struct.
	[[nodiscard]] auto StringToSystemTime(std::wstring_view wsv, DWORD dwFormat) -> std::optional<SYSTEMTIME>;

	//Convert FILETIME struct to a readable string.
	[[nodiscard]] auto FileTimeToString(FILETIME stFileTime, DWORD dwFormat, wchar_t wchSepar) -> std::wstring;

	//Convert SYSTEMTIME struct to a readable string.
	[[nodiscard]] auto SystemTimeToString(SYSTEMTIME stSysTime, DWORD dwFormat, wchar_t wchSepar) -> std::wstring;

	//String of date/time in given format with given date separator.
	[[nodiscard]] auto GetDateFormatString(DWORD dwFormat, wchar_t wchSepar) -> std::wstring;

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
		//Since a swapping-data type can be any type of 2, 4, or 8 bytes size,
		//we first bit_cast swapping-data to an integral type of the same size,
		//then byte-swapping and then bit_cast to the original type back.
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
				ulData = (ulData << 24) | ((ulData << 8) & 0x00FF'0000UL)
					| ((ulData >> 8) & 0x0000'FF00UL) | (ulData >> 24);
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

	template <class T>
	[[nodiscard]] constexpr T BitReverse(T tData) {
		T tReversed { };
		constexpr auto iBitsCount = sizeof(T) * 8;
		for (auto i = 0; i < iBitsCount; ++i, tData >>= 1) {
			tReversed = (tReversed << 1) | (tData & 1);
		}
		return tReversed;
	}
};