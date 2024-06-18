module;
/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include <SDKDDKVer.h>
#include "../dep/StrToNum/StrToNum.h"
#include "../HexCtrl.h"
#include <afxwin.h>
#include <bit>
#include <cassert>
#include <cwctype>
#include <format>
#include <immintrin.h>
#include <locale>
#include <optional>
#include <source_location>
#include <string>
export module HEXCTRL.HexUtility;

export namespace HEXCTRL::INTERNAL {
	//Time calculation constants and structs.
	constexpr auto g_uFTTicksPerMS = 10000U;             //Number of 100ns intervals in a milli-second.
	constexpr auto g_uFTTicksPerSec = 10000000UL;        //Number of 100ns intervals in a second.
	constexpr auto g_uHoursPerDay = 24U;                 //24 hours per day.
	constexpr auto g_uSecondsPerHour = 3600U;            //3600 seconds per hour.
	constexpr auto g_uFileTime1582OffsetDays = 6653U;    //FILETIME is based upon 1 Jan 1601 whilst GUID time is from 15 Oct 1582. Add 6653 days to convert to GUID time.
	constexpr auto g_ulFileTime1970_LOW = 0xd53e8000UL;  //1st Jan 1970 as FILETIME.
	constexpr auto g_ulFileTime1970_HIGH = 0x019db1deUL; //Used for Unix and Java times.
	constexpr auto g_ullUnixEpochDiff = 11644473600ULL;  //Number of ticks from FILETIME epoch of 1st Jan 1601 to Unix epoch of 1st Jan 1970.

	//Get data from IHexCtrl's given offset converted to a necessary type.
	template<typename T>
	[[nodiscard]] T GetIHexTData(const IHexCtrl& refHexCtrl, ULONGLONG ullOffset)
	{
		const auto spnData = refHexCtrl.GetData({ ullOffset, sizeof(T) });
		assert(!spnData.empty());
		return *reinterpret_cast<T*>(spnData.data());
	}

	//Set data of a necessary type to IHexCtrl's given offset.
	template<typename T>
	void SetIHexTData(IHexCtrl& refHexCtrl, ULONGLONG ullOffset, T tData)
	{
		//Data overflow check.
		assert(ullOffset + sizeof(T) <= refHexCtrl.GetDataSize());
		if (ullOffset + sizeof(T) > refHexCtrl.GetDataSize())
			return;

		refHexCtrl.ModifyData({ .eModifyMode { EHexModifyMode::MODIFY_ONCE },
			.spnData { reinterpret_cast<std::byte*>(&tData), sizeof(T) },
			.vecSpan { { ullOffset, sizeof(T) } } });
	}

	template<typename T> concept TSize1248 = (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);
	template<typename T> concept TSIMD = (std::is_same_v<T, __m128> || std::is_same_v<T, __m128i> || std::is_same_v<T, __m128d>);

	//Bytes swap for types of 2, 4, or 8 byte size.
	template<TSize1248 T> [[nodiscard]] constexpr T ByteSwap(T tData)noexcept
	{
		//Since a swapping-data type can be any type of 2, 4, or 8 bytes size,
		//we first bit_cast swapping-data to an integral type of the same size,
		//then byte-swapping and then bit_cast to the original type back.
		if constexpr (sizeof(T) == sizeof(std::uint8_t)) { //1 byte.
			return tData;
		}
		else if constexpr (sizeof(T) == sizeof(std::uint16_t)) { //2 bytes.
			auto wData = std::bit_cast<std::uint16_t>(tData);
			if (std::is_constant_evaluated()) {
				wData = static_cast<std::uint16_t>((wData << 8) | (wData >> 8));
				return std::bit_cast<T>(wData);
			}
			return std::bit_cast<T>(_byteswap_ushort(wData));
		}
		else if constexpr (sizeof(T) == sizeof(std::uint32_t)) { //4 bytes.
			auto ulData = std::bit_cast<std::uint32_t>(tData);
			if (std::is_constant_evaluated()) {
				ulData = (ulData << 24) | ((ulData << 8) & 0x00FF'0000U)
					| ((ulData >> 8) & 0x0000'FF00U) | (ulData >> 24);
				return std::bit_cast<T>(ulData);
			}
			return std::bit_cast<T>(_byteswap_ulong(ulData));
		}
		else if constexpr (sizeof(T) == sizeof(std::uint64_t)) { //8 bytes.
			auto ullData = std::bit_cast<std::uint64_t>(tData);
			if (std::is_constant_evaluated()) {
				ullData = (ullData << 56) | ((ullData << 40) & 0x00FF'0000'0000'0000ULL)
					| ((ullData << 24) & 0x0000'FF00'0000'0000ULL) | ((ullData << 8) & 0x0000'00FF'0000'0000ULL)
					| ((ullData >> 8) & 0x0000'0000'FF00'0000ULL) | ((ullData >> 24) & 0x0000'0000'00FF'0000ULL)
					| ((ullData >> 40) & 0x0000'0000'0000'FF00ULL) | (ullData >> 56);
				return std::bit_cast<T>(ullData);
			}
			return std::bit_cast<T>(_byteswap_uint64(ullData));
		}
	}

	//Bytes swap inside SIMD types: __m128, __m128i, __m128d.
	template<TSize1248 TIntegral, TSIMD T>
	[[nodiscard]] auto ByteSwapVec(const T m128T) -> T
	{
		if constexpr (std::is_same_v<T, __m128i>) { //Integrals.
			if constexpr (sizeof(TIntegral) == sizeof(std::uint8_t)) { //1 bytes.
				return m128T;
			}
			else if constexpr (sizeof(TIntegral) == sizeof(std::uint16_t)) { //2 bytes.
				const auto m128iMask = _mm_setr_epi8(1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14);
				return _mm_shuffle_epi8(m128T, m128iMask);
			}
			else if constexpr (sizeof(TIntegral) == sizeof(std::uint32_t)) { //4 bytes.
				const auto m128iMask = _mm_setr_epi8(3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12);
				return _mm_shuffle_epi8(m128T, m128iMask);
			}
			else if constexpr (sizeof(TIntegral) == sizeof(std::uint64_t)) { //8 bytes.
				const auto m128iMask = _mm_setr_epi8(7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8);
				return _mm_shuffle_epi8(m128T, m128iMask);
			}
		}
		else if constexpr (std::is_same_v<T, __m128>) { //Floats.
			alignas(16) float flData[4];
			_mm_store_ps(flData, m128T); //Loading m128T into local float array.
			const auto m128iData = _mm_load_si128(reinterpret_cast<__m128i*>(flData)); //Loading array as __m128i (convert).
			const auto m128iMask = _mm_setr_epi8(3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12);
			const auto m128iSwapped = _mm_shuffle_epi8(m128iData, m128iMask); //Swapping bytes.
			_mm_store_si128(reinterpret_cast<__m128i*>(flData), m128iSwapped); //Loading m128iSwapped back into local array.
			return _mm_load_ps(flData); //Returning local array as __m128.
		}
		else if constexpr (std::is_same_v<T, __m128d>) { //Doubles.
			alignas(16) double dbllData[2];
			_mm_store_pd(dbllData, m128T); //Loading m128T into local double array.
			const auto m128iData = _mm_load_si128(reinterpret_cast<__m128i*>(dbllData)); //Loading array as __m128i (convert).
			const auto m128iMask = _mm_setr_epi8(7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8);
			const auto m128iSwapped = _mm_shuffle_epi8(m128iData, m128iMask); //Swapping bytes.
			_mm_store_si128(reinterpret_cast<__m128i*>(dbllData), m128iSwapped); //Loading m128iSwapped back into local array.
			return _mm_load_pd(dbllData); //Returning local array as __m128d.
		}
	}

	template<TSize1248 T> [[nodiscard]] constexpr T BitReverse(T tData) {
		T tReversed { };
		constexpr auto iBitsCount = sizeof(T) * 8;
		for (auto i = 0; i < iBitsCount; ++i, tData >>= 1) {
			tReversed = (tReversed << 1) | (tData & 1);
		}
		return tReversed;
	}

	//Converts every two numeric wchars to one respective hex character: "56"->V(0x56), "7A"->z(0x7A), etc...
	//chWc - a wildcard if any.
	[[nodiscard]] auto NumStrToHex(std::wstring_view wsv, char chWc = 0) -> std::optional<std::string>
	{
		const auto fWc = chWc != 0; //Is wildcard used?
		std::wstring wstrFilter = L"0123456789AaBbCcDdEeFf"; //Allowed characters.

		if (fWc) {
			wstrFilter += chWc;
		}

		if (wsv.find_first_not_of(wstrFilter) != std::wstring_view::npos)
			return std::nullopt;

		std::string strHexTmp;
		for (auto iterBegin = wsv.begin(); iterBegin != wsv.end();) {
			if (fWc && static_cast<char>(*iterBegin) == chWc) { //Skip wildcard.
				++iterBegin;
				strHexTmp += chWc;
				continue;
			}

			//Extract two current wchars and pass it to StringToNum as wstring.
			const std::size_t nOffsetCurr = iterBegin - wsv.begin();
			const auto nSize = nOffsetCurr + 2 <= wsv.size() ? 2 : 1;
			if (const auto optNumber = stn::StrToUInt8(wsv.substr(nOffsetCurr, nSize), 16); optNumber) {
				iterBegin += nSize;
				strHexTmp += *optNumber;
			}
			else
				return std::nullopt;
		}

		return { std::move(strHexTmp) };
	}

	[[nodiscard]] auto WstrToStr(std::wstring_view wsv, UINT uCodePage = CP_UTF8) -> std::string
	{
		const auto iSize = WideCharToMultiByte(uCodePage, 0, wsv.data(), static_cast<int>(wsv.size()), nullptr, 0, nullptr, nullptr);
		std::string str(iSize, 0);
		WideCharToMultiByte(uCodePage, 0, wsv.data(), static_cast<int>(wsv.size()), str.data(), iSize, nullptr, nullptr);
		return str;
	}

	[[nodiscard]] auto StrToWstr(std::string_view sv, UINT uCodePage = CP_UTF8) -> std::wstring
	{
		const auto iSize = MultiByteToWideChar(uCodePage, 0, sv.data(), static_cast<int>(sv.size()), nullptr, 0);
		std::wstring wstr(iSize, 0);
		MultiByteToWideChar(uCodePage, 0, sv.data(), static_cast<int>(sv.size()), wstr.data(), iSize);
		return wstr;
	}

	[[nodiscard]] auto StringToSystemTime(std::wstring_view wsv, DWORD dwFormat) -> std::optional<SYSTEMTIME>
	{
		//dwFormat is a locale specific date format https://docs.microsoft.com/en-gb/windows/win32/intl/locale-idate

		if (wsv.empty())
			return std::nullopt;

		//Normalise the input string by replacing non-numeric characters except space with a `/`.
		//This should regardless of the current date/time separator character.
		std::wstring wstrDateTimeCooked;
		for (const auto wch : wsv) {
			wstrDateTimeCooked += (std::iswdigit(wch) || wch == L' ') ? wch : L'/';
		}

		SYSTEMTIME stSysTime { };
		int iParsedArgs { };

		//Parse date component.
		switch (dwFormat) {
		case 0:	//Month-Day-Year.
			iParsedArgs = swscanf_s(wstrDateTimeCooked.data(), L"%2hu/%2hu/%4hu", &stSysTime.wMonth, &stSysTime.wDay, &stSysTime.wYear);
			break;
		case 1: //Day-Month-Year.
			iParsedArgs = swscanf_s(wstrDateTimeCooked.data(), L"%2hu/%2hu/%4hu", &stSysTime.wDay, &stSysTime.wMonth, &stSysTime.wYear);
			break;
		case 2:	//Year-Month-Day.
			iParsedArgs = swscanf_s(wstrDateTimeCooked.data(), L"%4hu/%2hu/%2hu", &stSysTime.wYear, &stSysTime.wMonth, &stSysTime.wDay);
			break;
		default:
			assert(true);
			return std::nullopt;
		}
		if (iParsedArgs != 3)
			return std::nullopt;

		//Find time seperator, if present.
		if (const auto nPos = wstrDateTimeCooked.find(L' '); nPos != std::wstring::npos) {
			wstrDateTimeCooked = wstrDateTimeCooked.substr(nPos + 1);

			//Parse time component HH:MM:SS.mmm.
			if (swscanf_s(wstrDateTimeCooked.data(), L"%2hu/%2hu/%2hu/%3hu", &stSysTime.wHour, &stSysTime.wMinute,
				&stSysTime.wSecond, &stSysTime.wMilliseconds) != 4)
				return std::nullopt;
		}

		return stSysTime;
	}

	[[nodiscard]] auto StringToFileTime(std::wstring_view wsv, DWORD dwFormat) -> std::optional<FILETIME>
	{
		std::optional<FILETIME> optFT { std::nullopt };
		if (auto optSysTime = StringToSystemTime(wsv, dwFormat); optSysTime) {
			if (FILETIME ftTime; SystemTimeToFileTime(&*optSysTime, &ftTime) != FALSE) {
				optFT = ftTime;
			}
		}
		return optFT;
	}

	[[nodiscard]] auto SystemTimeToString(SYSTEMTIME stSysTime, DWORD dwFormat, wchar_t wchSepar) -> std::wstring
	{
		if (dwFormat > 2 || stSysTime.wDay == 0 || stSysTime.wDay > 31 || stSysTime.wMonth == 0 || stSysTime.wMonth > 12
			|| stSysTime.wYear > 9999 || stSysTime.wHour > 23 || stSysTime.wMinute > 59 || stSysTime.wSecond > 59
			|| stSysTime.wMilliseconds > 999)
			return L"N/A";

		std::wstring_view wsvFmt;
		switch (dwFormat) {
		case 0:	//0:Month/Day/Year HH:MM:SS.mmm.
			wsvFmt = L"{1:02d}{7}{0:02d}{7}{2} {3:02d}:{4:02d}:{5:02d}.{6:03d}";
			break;
		case 1: //1:Day/Month/Year HH:MM:SS.mmm.
			wsvFmt = L"{0:02d}{7}{1:02d}{7}{2} {3:02d}:{4:02d}:{5:02d}.{6:03d}";
			break;
		case 2: //2:Year/Month/Day HH:MM:SS.mmm.
			wsvFmt = L"{2}{7}{1:02d}{7}{0:02d} {3:02d}:{4:02d}:{5:02d}.{6:03d}";
			break;
		default:
			break;
		}

		return std::vformat(wsvFmt, std::make_wformat_args(stSysTime.wDay, stSysTime.wMonth, stSysTime.wYear,
			stSysTime.wHour, stSysTime.wMinute, stSysTime.wSecond, stSysTime.wMilliseconds, wchSepar));
	}

	[[nodiscard]] auto FileTimeToString(FILETIME stFileTime, DWORD dwFormat, wchar_t wchSepar) -> std::wstring
	{
		if (SYSTEMTIME stSysTime; FileTimeToSystemTime(&stFileTime, &stSysTime) != FALSE) {
			return SystemTimeToString(stSysTime, dwFormat, wchSepar);
		}

		return L"N/A";
	}

	//String of a date/time in the given format with the given date separator.
	[[nodiscard]] auto GetDateFormatString(DWORD dwFormat, wchar_t wchSepar) -> std::wstring
	{
		std::wstring_view wsvFmt;
		switch (dwFormat) {
		case 0:	//Month-Day-Year.
			wsvFmt = L"MM{0}DD{0}YYYY HH:MM:SS.mmm";
			break;
		case 1: //Day-Month-Year.
			wsvFmt = L"DD{0}MM{0}YYYY HH:MM:SS.mmm";
			break;
		case 2:	//Year-Month-Day.
			wsvFmt = L"YYYY{0}MM{0}DD HH:MM:SS.mmm";
			break;
		default:
			assert(true);
			break;
		}

		return std::vformat(wsvFmt, std::make_wformat_args(wchSepar));
	}

	template<typename T>
	[[nodiscard]] auto RangeToVecBytes(const T& refData) -> std::vector<std::byte>
	{
		const std::byte* pBegin;
		const std::byte* pEnd;
		if constexpr (std::is_same_v<T, std::string>) {
			pBegin = reinterpret_cast<const std::byte*>(refData.data());
			pEnd = pBegin + refData.size();
		}
		else if constexpr (std::is_same_v<T, std::wstring>) {
			pBegin = reinterpret_cast<const std::byte*>(refData.data());
			pEnd = pBegin + refData.size() * sizeof(wchar_t);
		}
		else if constexpr (std::is_same_v<T, CStringW>) {
			pBegin = reinterpret_cast<const std::byte*>(refData.GetString());
			pEnd = pBegin + refData.GetLength() * sizeof(wchar_t);
		}
		else {
			pBegin = reinterpret_cast<const std::byte*>(&refData);
			pEnd = pBegin + sizeof(refData);
		}

		return { pBegin, pEnd };
	}

	[[nodiscard]] auto GetLocale() -> std::locale {
		static std::locale loc { std::locale("en_US.UTF-8") };
		return loc;
	}

#if defined(DEBUG) || defined(_DEBUG)
	void DBG_REPORT(const wchar_t* pMsg, const std::source_location& loc = std::source_location::current()) {
		_wassert(pMsg, StrToWstr(loc.file_name()).data(), loc.line());
	}
#else
	void DBG_REPORT([[maybe_unused]] const wchar_t*) {}
#endif
}