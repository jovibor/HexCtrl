/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
module;
#include <SDKDDKVer.h>
#include "../HexCtrl.h"
#include <Windows.h>
#include <commctrl.h>
#include <intrin.h>
#include <algorithm>
#include <bit>
#include <cassert>
#include <cwctype>
#include <format>
#include <locale>
#include <optional>
#include <source_location>
#include <string>
#include <unordered_map>
#include <vector>
export module HEXCTRL:HexUtility;

export import HexCtrl_StrToNum;
export import HexCtrl_ListEx;

namespace HEXCTRL::INTERNAL::ut { //Utility methods and stuff.
	//Time calculation constants and structs.
	constexpr auto g_uFTTicksPerMS = 10000U;            //Number of 100ns intervals in a milli-second.
	constexpr auto g_uFTTicksPerSec = 10000000U;        //Number of 100ns intervals in a second.
	constexpr auto g_uHoursPerDay = 24U;                //24 hours per day.
	constexpr auto g_uSecondsPerHour = 3600U;           //3600 seconds per hour.
	constexpr auto g_uFileTime1582OffsetDays = 6653U;   //FILETIME is based upon 1 Jan 1601 whilst GUID time is from 15 Oct 1582. Add 6653 days to convert to GUID time.
	constexpr auto g_ulFileTime1970_LOW = 0xd53e8000U;  //1st Jan 1970 as FILETIME.
	constexpr auto g_ulFileTime1970_HIGH = 0x019db1deU; //Used for Unix and Java times.
	constexpr auto g_ullUnixEpochDiff = 11644473600ULL; //Number of ticks from FILETIME epoch of 1st Jan 1601 to Unix epoch of 1st Jan 1970.

	[[nodiscard]] auto StrToWstr(std::string_view sv, UINT uCodePage = CP_UTF8) -> std::wstring
	{
		const auto iSize = ::MultiByteToWideChar(uCodePage, 0, sv.data(), static_cast<int>(sv.size()), nullptr, 0);
		std::wstring wstr(iSize, 0);
		::MultiByteToWideChar(uCodePage, 0, sv.data(), static_cast<int>(sv.size()), wstr.data(), iSize);
		return wstr;
	}

	[[nodiscard]] auto WstrToStr(std::wstring_view wsv, UINT uCodePage = CP_UTF8) -> std::string
	{
		const auto iSize = ::WideCharToMultiByte(uCodePage, 0, wsv.data(), static_cast<int>(wsv.size()), nullptr, 0, nullptr, nullptr);
		std::string str(iSize, 0);
		::WideCharToMultiByte(uCodePage, 0, wsv.data(), static_cast<int>(wsv.size()), str.data(), iSize, nullptr, nullptr);
		return str;
	}

#if defined(DEBUG) || defined(_DEBUG)
	void DBG_REPORT(const wchar_t* pMsg, const std::source_location& loc = std::source_location::current()) {
		::_wassert(pMsg, StrToWstr(loc.file_name()).data(), loc.line());
	}

	void DBG_REPORT_NOT_CREATED(const std::source_location& loc = std::source_location::current()) {
		DBG_REPORT(L"Not created yet!", loc);
	}

	void DBG_REPORT_NO_DATA_SET(const std::source_location& loc = std::source_location::current()) {
		DBG_REPORT(L"No data set.", loc);
	}
#else
	void DBG_REPORT([[maybe_unused]] const wchar_t*) { }
	void DBG_REPORT_NOT_CREATED() { }
	void DBG_REPORT_NO_DATA_SET() { }
#endif

	//Get data from IHexCtrl's given offset converted to a necessary type.
	template<typename T>
	[[nodiscard]] T GetIHexTData(const IHexCtrl& refHexCtrl, ULONGLONG ullOffset)
	{
		const auto spnData = refHexCtrl.GetData({ .ullOffset { ullOffset }, .ullSize { sizeof(T) } });
		assert(!spnData.empty());
		return *reinterpret_cast<T*>(spnData.data());
	}

	//Set data of a necessary type to IHexCtrl's given offset.
	template<typename T>
	void SetIHexTData(IHexCtrl& refHexCtrl, ULONGLONG ullOffset, T tData)
	{
		if (ullOffset + sizeof(T) > refHexCtrl.GetDataSize()) { //Data overflow check.
			DBG_REPORT(L"Data overflow occurs.");
			return;
		}

		refHexCtrl.ModifyData({ .eModifyMode { EHexModifyMode::MODIFY_ONCE },
			.spnData { reinterpret_cast<std::byte*>(&tData), sizeof(T) },
			.vecSpan { { ullOffset, sizeof(T) } } });
	}

	void SetIHexTData(IHexCtrl& refHexCtrl, ULONGLONG ullOffset, SpanCByte spnData)
	{
		if (ullOffset + spnData.size() > refHexCtrl.GetDataSize()) { //Data overflow check.
			DBG_REPORT(L"Data overflow occurs.");
			return;
		}

		refHexCtrl.ModifyData({ .eModifyMode { EHexModifyMode::MODIFY_ONCE },
			.spnData { spnData }, .vecSpan { { ullOffset, spnData.size() } } });
	}

	template<typename T> concept TSize1248 = (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);

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

#if defined(_M_IX86) || defined(_M_X64)
	template<typename T> concept TVec128 = (std::is_same_v<T, __m128> || std::is_same_v<T, __m128i> || std::is_same_v<T, __m128d>);
	template<typename T> concept TVec256 = (std::is_same_v<T, __m256> || std::is_same_v<T, __m256i> || std::is_same_v<T, __m256d>);

	template<TSize1248 TIntegral, TVec128 TVec>	//Bytes swap inside vector types: __m128, __m128i, __m128d.
	[[nodiscard]] auto ByteSwapVec(const TVec m128T) -> TVec
	{
		if constexpr (std::is_same_v<TVec, __m128i>) { //Integrals.
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
		else if constexpr (std::is_same_v<TVec, __m128>) { //Floats.
			alignas(16) float flData[4];
			_mm_store_ps(flData, m128T); //Loading m128T into local float array.
			const auto m128iData = _mm_load_si128(reinterpret_cast<__m128i*>(flData)); //Loading array as __m128i (convert).
			const auto m128iMask = _mm_setr_epi8(3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12);
			const auto m128iSwapped = _mm_shuffle_epi8(m128iData, m128iMask); //Swapping bytes.
			_mm_store_si128(reinterpret_cast<__m128i*>(flData), m128iSwapped); //Loading m128iSwapped back into local array.
			return _mm_load_ps(flData); //Returning local array as __m128.
		}
		else if constexpr (std::is_same_v<TVec, __m128d>) { //Doubles.
			alignas(16) double dbllData[2];
			_mm_store_pd(dbllData, m128T); //Loading m128T into local double array.
			const auto m128iData = _mm_load_si128(reinterpret_cast<__m128i*>(dbllData)); //Loading array as __m128i (convert).
			const auto m128iMask = _mm_setr_epi8(7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8);
			const auto m128iSwapped = _mm_shuffle_epi8(m128iData, m128iMask); //Swapping bytes.
			_mm_store_si128(reinterpret_cast<__m128i*>(dbllData), m128iSwapped); //Loading m128iSwapped back into local array.
			return _mm_load_pd(dbllData); //Returning local array as __m128d.
		}
	}

	template<TSize1248 TIntegral, TVec256 TVec>	//Bytes swap inside vector types: __m256, __m256i, __m256d.
	[[nodiscard]] auto ByteSwapVec(const TVec m256T) -> TVec
	{
		if constexpr (std::is_same_v<TVec, __m256i>) { //Integrals.
			if constexpr (sizeof(TIntegral) == sizeof(std::uint8_t)) { //1 bytes.
				return m256T;
			}
			else if constexpr (sizeof(TIntegral) == sizeof(std::uint16_t)) { //2 bytes.
				const auto m256iMask = _mm256_setr_epi8(1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14, 17, 16,
					19, 18, 21, 20, 23, 22, 25, 24, 27, 26, 29, 28, 31, 30);
				return _mm256_shuffle_epi8(m256T, m256iMask);
			}
			else if constexpr (sizeof(TIntegral) == sizeof(std::uint32_t)) { //4 bytes.
				const auto m256iMask = _mm256_setr_epi8(3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12,
					19, 18, 17, 16, 23, 22, 21, 20, 27, 26, 25, 24, 31, 30, 29, 28);
				return _mm256_shuffle_epi8(m256T, m256iMask);
			}
			else if constexpr (sizeof(TIntegral) == sizeof(std::uint64_t)) { //8 bytes.
				const auto m256iMask = _mm256_setr_epi8(7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8,
					23, 22, 21, 20, 19, 18, 17, 16, 31, 30, 29, 28, 27, 26, 25, 24);
				return _mm256_shuffle_epi8(m256T, m256iMask);
			}
		}
		else if constexpr (std::is_same_v<TVec, __m256>) { //Floats.
			alignas(32) float flData[8];
			_mm256_store_ps(flData, m256T); //Loading m256T into local float array.
			const auto m256iData = _mm256_load_si256(reinterpret_cast<__m256i*>(flData)); //Loading array as __m256i (convert).
			const auto m256iMask = _mm256_setr_epi8(3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12,
					19, 18, 17, 16, 23, 22, 21, 20, 27, 26, 25, 24, 31, 30, 29, 28);
			const auto m256iSwapped = _mm256_shuffle_epi8(m256iData, m256iMask); //Swapping bytes.
			_mm256_store_si256(reinterpret_cast<__m256i*>(flData), m256iSwapped); //Loading m256iSwapped back into local array.
			return _mm256_load_ps(flData); //Returning local array as __m256.
		}
		else if constexpr (std::is_same_v<TVec, __m256d>) { //Doubles.
			alignas(32) double dbllData[4];
			_mm256_store_pd(dbllData, m256T); //Loading m256T into local double array.
			const auto m256iData = _mm256_load_si256(reinterpret_cast<__m256i*>(dbllData)); //Loading array as __m256i (convert).
			const auto m256iMask = _mm256_setr_epi8(7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8,
					23, 22, 21, 20, 19, 18, 17, 16, 31, 30, 29, 28, 27, 26, 25, 24);
			const auto m256iSwapped = _mm256_shuffle_epi8(m256iData, m256iMask); //Swapping bytes.
			_mm256_store_si256(reinterpret_cast<__m256i*>(dbllData), m256iSwapped); //Loading m256iSwapped back into local array.
			return _mm256_load_pd(dbllData); //Returning local array as __m256d.
		}
	}

	[[nodiscard]] bool HasAVX2() {
		const static bool fHasAVX2 = []() {
			int arrInfo[4] { };
			::__cpuid(arrInfo, 0);
			if (arrInfo[0] < 7)
				return false;

			::__cpuid(arrInfo, 7);
			return (arrInfo[1] & 0b00100000) != 0;
			}();
		return fHasAVX2;
	}
#endif // ^^^ _M_IX86 || _M_X64

	template<TSize1248 T> [[nodiscard]] constexpr T BitReverse(T tData)noexcept {
		T tReversed { };
		constexpr auto iBitsCount = sizeof(T) * 8;
		for (auto i = 0U; i < iBitsCount; ++i, tData >>= 1) {
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
		for (auto it = wsv.begin(); it != wsv.end();) {
			if (fWc && static_cast<char>(*it) == chWc) { //Skip wildcard.
				++it;
				strHexTmp += chWc;
				continue;
			}

			//Extract two current wchars and pass it to StringToNum as wstring.
			const std::size_t nOffsetCurr = it - wsv.begin();
			const auto nSize = nOffsetCurr + 2 <= wsv.size() ? 2 : 1;
			if (const auto optNumber = stn::StrToUInt8(wsv.substr(nOffsetCurr, nSize), 16); optNumber) {
				it += nSize;
				strHexTmp += *optNumber;
			}
			else
				return std::nullopt;
		}

		return { std::move(strHexTmp) };
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
			iParsedArgs = ::swscanf_s(wstrDateTimeCooked.data(), L"%2hu/%2hu/%4hu",
				&stSysTime.wMonth, &stSysTime.wDay, &stSysTime.wYear);
			break;
		case 1: //Day-Month-Year.
			iParsedArgs = ::swscanf_s(wstrDateTimeCooked.data(), L"%2hu/%2hu/%4hu",
				&stSysTime.wDay, &stSysTime.wMonth, &stSysTime.wYear);
			break;
		case 2:	//Year-Month-Day.
			iParsedArgs = ::swscanf_s(wstrDateTimeCooked.data(), L"%4hu/%2hu/%2hu",
				&stSysTime.wYear, &stSysTime.wMonth, &stSysTime.wDay);
			break;
		default:
			DBG_REPORT(L"Wrong format.");
			return std::nullopt;
		}
		if (iParsedArgs != 3)
			return std::nullopt;

		//Find time seperator, if present.
		if (const auto nPos = wstrDateTimeCooked.find(L' '); nPos != std::wstring::npos) {
			wstrDateTimeCooked = wstrDateTimeCooked.substr(nPos + 1);

			//Parse time component HH:MM:SS.mmm.
			if (::swscanf_s(wstrDateTimeCooked.data(), L"%2hu/%2hu/%2hu/%3hu", &stSysTime.wHour, &stSysTime.wMinute,
				&stSysTime.wSecond, &stSysTime.wMilliseconds) != 4)
				return std::nullopt;
		}

		return stSysTime;
	}

	[[nodiscard]] auto StringToFileTime(std::wstring_view wsv, DWORD dwFormat) -> std::optional<FILETIME>
	{
		std::optional<FILETIME> optFT { std::nullopt };
		if (auto optSysTime = StringToSystemTime(wsv, dwFormat); optSysTime) {
			if (FILETIME ftTime; ::SystemTimeToFileTime(&*optSysTime, &ftTime) != FALSE) {
				optFT = ftTime;
			}
		}
		return optFT;
	}

	[[nodiscard]] auto SystemTimeToString(SYSTEMTIME st, DWORD dwFormat, wchar_t wchSepar) -> std::wstring
	{
		if (dwFormat > 2 || st.wDay == 0 || st.wDay > 31 || st.wMonth == 0 || st.wMonth > 12
			|| st.wYear > 9999 || st.wHour > 23 || st.wMinute > 59 || st.wSecond > 59 || st.wMilliseconds > 999)
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

		return std::vformat(wsvFmt, std::make_wformat_args(st.wDay, st.wMonth, st.wYear,
			st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, wchSepar));
	}

	[[nodiscard]] auto FileTimeToString(FILETIME ft, DWORD dwFormat, wchar_t wchSepar) -> std::wstring
	{
		if (SYSTEMTIME stSysTime; ::FileTimeToSystemTime(&ft, &stSysTime) != FALSE) {
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
			DBG_REPORT(L"Wrong format.");
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
		else {
			pBegin = reinterpret_cast<const std::byte*>(&refData);
			pEnd = pBegin + sizeof(refData);
		}

		return { pBegin, pEnd };
	}

	[[nodiscard]] auto GetLocale() -> std::locale {
		static const auto loc { std::locale("en_US.UTF-8") };
		return loc;
	}

	//Returns hInstance of the current module, whether it is a .exe or .dll.
	[[nodiscard]] auto GetCurrModuleHinst() -> HINSTANCE {
		HINSTANCE hInst { };
		::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<LPCWSTR>(&GetCurrModuleHinst), &hInst);
		return hInst;
	};

	//Get font points size from the size in pixels.
	[[nodiscard]] constexpr auto FontPointsFromPixels(float flSizePixels) -> float {
		constexpr auto flPointsInPixel = 72.F / USER_DEFAULT_SCREEN_DPI;
		return flSizePixels * flPointsInPixel;
	}

	//Get font pixels size from the size in points.
	[[nodiscard]] constexpr auto FontPixelsFromPoints(float flSizePoints) -> float {
		constexpr auto flPixelsInPoint = USER_DEFAULT_SCREEN_DPI / 72.F;
		return flSizePoints * flPixelsInPoint;
	}

	//Replicates GET_X_LPARAM macro from windowsx.h.
	[[nodiscard]] constexpr int GetXLPARAM(LPARAM lParam) {
		return (static_cast<int>(static_cast<short>(static_cast<WORD>((static_cast<DWORD_PTR>(lParam)) & 0xFFFFU))));
	}

	//Replicates GET_Y_LPARAM macro from windowsx.h.
	[[nodiscard]] constexpr int GetYLPARAM(LPARAM lParam) {
		return GetXLPARAM(lParam >> 16);
	}
}

namespace HEXCTRL::INTERNAL::GDIUT { //Windows GDI related stuff.
	auto DefWndProc(const MSG& msg) -> LRESULT {
		return ::DefWindowProcW(msg.hwnd, msg.message, msg.wParam, msg.lParam);
	}

	template<typename T>
	auto CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)->LRESULT
	{
		//Different IHexCtrl objects will share the same WndProc<ExactTypeHere> function.
		//Hence, the map is needed to differentiate these objects. 
		//The DlgProc<T> works absolutely the same way.
		static std::unordered_map<HWND, T*> uMap;

		//CREATESTRUCTW::lpCreateParams always possesses `this` pointer, passed to the CreateWindowExW
		//function as lpParam. We save it to the static uMap to have access to this->ProcessMsg() method.
		if (uMsg == WM_CREATE) {
			const auto lpCS = reinterpret_cast<LPCREATESTRUCTW>(lParam);
			if (lpCS->lpCreateParams != nullptr) {
				uMap[hWnd] = reinterpret_cast<T*>(lpCS->lpCreateParams);
			}
		}

		if (const auto it = uMap.find(hWnd); it != uMap.end()) {
			const auto ret = it->second->ProcessMsg({ .hwnd { hWnd }, .message { uMsg },
				.wParam { wParam }, .lParam { lParam } });
			if (uMsg == WM_NCDESTROY) { //Remove hWnd from the map on window destruction.
				uMap.erase(it);
			}
			return ret;
		}

		return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}

	template<typename T>
	auto CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)->INT_PTR {
		//DlgProc should return zero for all non-processed messages.
		//In that case messages will be processed by Windows default dialog proc.
		//Non-processed messages should not be passed neither to DefWindowProcW nor to DefDlgProcW.
		//Processed messages should return any non-zero value, depending on message type.

		static std::unordered_map<HWND, T*> uMap;

		//DialogBoxParamW and CreateDialogParamW dwInitParam arg is sent with WM_INITDIALOG as lParam.
		if (uMsg == WM_INITDIALOG) {
			if (lParam != 0) {
				uMap[hWnd] = reinterpret_cast<T*>(lParam);
			}
		}

		if (const auto it = uMap.find(hWnd); it != uMap.end()) {
			const auto ret = it->second->ProcessMsg({ .hwnd { hWnd }, .message { uMsg },
				.wParam { wParam }, .lParam { lParam } });
			if (uMsg == WM_NCDESTROY) { //Remove hWnd from the map on dialog destruction.
				uMap.erase(it);
			}
			return ret;
		}

		return 0;
	}

	[[nodiscard]] auto GetDPIForHWND(HWND hWnd) -> UINT {
		//const auto hDC = ::GetDC(hWnd);
		//const auto iLOGPIXELSY = ::GetDeviceCaps(hDC, LOGPIXELSY);
		//::ReleaseDC(hWnd, hDC);
		const auto iLOGPIXELSY = ::GetDpiForWindow(hWnd); //Available only since "Windows 10, version 1607".
		return iLOGPIXELSY;
	}

	[[nodiscard]] auto GetDPIScaleForHWND(HWND hWnd) -> float {
		return static_cast<float>(GetDPIForHWND(hWnd)) / USER_DEFAULT_SCREEN_DPI; //High-DPI scale factor for window.
	}

	//iDirection: -2:LEFT, 1:UP, 2:RIGHT, -1:DOWN.
	[[nodiscard]] auto CreateArrowBitmap(HDC hDC, DWORD dwWidth, DWORD dwHeight, int iDirection, COLORREF clrBk, COLORREF clrArrow) -> HBITMAP {
		//Make the width and height even numbers, to make it easier drawing an isosceles triangle.
		const int iWidth = dwWidth - (dwWidth % 2);
		const int iHeight = dwHeight - (dwHeight % 2);

		POINT ptArrow[3] { }; //Arrow coordinates within the bitmap.
		switch (iDirection) {
		case -2: //LEFT.
			ptArrow[0] = { .x { iWidth / 4 }, .y { iHeight / 2 } };
			ptArrow[1] = { .x { iWidth / 2 }, .y { iHeight / 4 } };
			ptArrow[2] = { .x { iWidth / 2 }, .y { iHeight - (iHeight / 4) } };
			break;
		case 1: //UP.
			ptArrow[0] = { .x { iWidth / 2 }, .y { iHeight / 4 } };
			ptArrow[1] = { .x { iWidth / 4 }, .y { iHeight / 2 } };
			ptArrow[2] = { .x { iWidth - (iWidth / 4) }, .y { iHeight / 2 } };
			break;
		case 2: //RIGHT.
			ptArrow[0] = { .x { iWidth / 2 }, .y { iHeight / 4 } };
			ptArrow[1] = { .x { iWidth / 2 }, .y { iHeight - (iHeight / 4) } };
			ptArrow[2] = { .x { iWidth - (iWidth / 4) }, .y { iHeight / 2 } };
			break;
		case -1: //DOWN.
			ptArrow[0] = { .x { iWidth / 4 }, .y { iHeight / 2 } };
			ptArrow[1] = { .x { iWidth - (iWidth / 4) }, .y { iHeight / 2 } };
			ptArrow[2] = { .x { iWidth / 2 }, .y { iHeight - (iHeight / 4) } };
			break;
		default:
			break;
		}

		const auto hdcMem = ::CreateCompatibleDC(hDC);
		const auto hBMPArrow = ::CreateCompatibleBitmap(hDC, dwWidth, dwHeight);
		::SelectObject(hdcMem, hBMPArrow);
		const RECT rcFill { .right { static_cast<int>(dwWidth) }, .bottom { static_cast<int>(dwHeight) } };
		::SetBkColor(hdcMem, clrBk); //SetBkColor+ExtTextOutW=FillSolidRect behavior.
		::ExtTextOutW(hdcMem, 0, 0, ETO_OPAQUE, &rcFill, nullptr, 0, nullptr);
		const auto hBrushArrow = ::CreateSolidBrush(clrArrow);
		::SelectObject(hdcMem, hBrushArrow);
		const auto hPenArrow = ::CreatePen(PS_SOLID, 1, clrArrow);
		::SelectObject(hdcMem, hPenArrow);
		::Polygon(hdcMem, ptArrow, 3); //Draw an arrow.
		::DeleteObject(hBrushArrow);
		::DeleteObject(hPenArrow);
		::DeleteDC(hdcMem);

		return hBMPArrow;
	}

	//Get 32bit ARGB bitmap with premultiplied alpha from HICON.
	[[nodiscard]] auto BitmapFromIcon(HICON hIcon) -> HBITMAP {
		ICONINFO ii;
		if (::GetIconInfo(hIcon, &ii) == FALSE)
			return { };

		::DeleteObject(ii.hbmMask); //Deleting icon's BW mask bitmap.
		BITMAP bmIconClr;
		::GetObjectW(ii.hbmColor, sizeof(BITMAP), &bmIconClr);
		const auto iBmpHeight = std::abs(bmIconClr.bmHeight);
		const auto iBmpWidth = std::abs(bmIconClr.bmWidth);
		BITMAPINFO bi { .bmiHeader { .biSize { sizeof(BITMAPINFOHEADER) }, .biWidth { iBmpWidth },
			.biHeight { iBmpHeight }, .biPlanes { 1 }, .biBitCount { 32 }, .biCompression { BI_RGB } } };
		RGBQUAD* pARGB { }; //Newly created DI bitmap's data pointer.
		const auto hDCMem = ::CreateCompatibleDC(nullptr);
		const auto hBmp32 = ::CreateDIBSection(hDCMem, &bi, DIB_RGB_COLORS, reinterpret_cast<void**>(&pARGB), nullptr, 0);
		::GetDIBits(hDCMem, ii.hbmColor, 0, iBmpHeight, pARGB, &bi, DIB_RGB_COLORS);
		::DeleteDC(hDCMem);
		::DeleteObject(ii.hbmColor);

		if (hBmp32 == nullptr || pARGB == nullptr)
			return { };

		const auto dwSizePx = static_cast<DWORD>(iBmpHeight * iBmpWidth);
		if (bmIconClr.bmBitsPixel == 32) { //Premultiply alpha for all channels.
			for (auto i = 0U; i < dwSizePx; ++i) {
				pARGB[i].rgbBlue = static_cast<BYTE>(static_cast<DWORD>(pARGB[i].rgbBlue) * pARGB[i].rgbReserved / 255);
				pARGB[i].rgbGreen = static_cast<BYTE>(static_cast<DWORD>(pARGB[i].rgbGreen) * pARGB[i].rgbReserved / 255);
				pARGB[i].rgbRed = static_cast<BYTE>(static_cast<DWORD>(pARGB[i].rgbRed) * pARGB[i].rgbReserved / 255);
			}
		}
		else { //If the icon is not 32bit, merely set bitmap alpha channel to fully opaque.
			for (auto i = 0U; i < dwSizePx; ++i) {
				pARGB[i].rgbReserved = 255;
			}
		}

		return hBmp32;
	}

	class CDynLayout final {
	public:
		//Ratio settings, for how much to move or resize an item when parent is resized.
		struct ItemRatio { float flXRatio { }; float flYRatio { }; };
		struct MoveRatio : public ItemRatio { }; //To differentiate move from size in the AddItem.
		struct SizeRatio : public ItemRatio { };

		void AddItem(int iIDItem, MoveRatio move, SizeRatio size);
		void Enable(bool fTrack);
		void OnSize(int iWidth, int iHeight)const; //Should be hooked into the host window WM_SIZE handler.
		void RemoveAll() { m_vecItems.clear(); }
		void SetHost(HWND hWnd) { assert(hWnd != nullptr); m_hWndHost = hWnd; }

		//Static helper methods to use in the AddItem.
		[[nodiscard]] static MoveRatio MoveNone() { return { }; }
		[[nodiscard]] static MoveRatio MoveHorz(int iXRatio) {
			iXRatio = std::clamp(iXRatio, 0, 100); return { { .flXRatio { iXRatio / 100.F } } };
		}
		[[nodiscard]] static MoveRatio MoveVert(int iYRatio) {
			iYRatio = std::clamp(iYRatio, 0, 100); return { { .flYRatio { iYRatio / 100.F } } };
		}
		[[nodiscard]] static MoveRatio MoveHorzAndVert(int iXRatio, int iYRatio) {
			iXRatio = std::clamp(iXRatio, 0, 100); iYRatio = std::clamp(iYRatio, 0, 100);
			return { { .flXRatio { iXRatio / 100.F }, .flYRatio { iYRatio / 100.F } } };
		}
		[[nodiscard]] static SizeRatio SizeNone() { return { }; }
		[[nodiscard]] static SizeRatio SizeHorz(int iXRatio) {
			iXRatio = std::clamp(iXRatio, 0, 100); return { { .flXRatio { iXRatio / 100.F } } };
		}
		[[nodiscard]] static SizeRatio SizeVert(int iYRatio) {
			iYRatio = std::clamp(iYRatio, 0, 100); return { { .flYRatio { iYRatio / 100.F } } };
		}
		[[nodiscard]] static SizeRatio SizeHorzAndVert(int iXRatio, int iYRatio) {
			iXRatio = std::clamp(iXRatio, 0, 100); iYRatio = std::clamp(iYRatio, 0, 100);
			return { { .flXRatio { iXRatio / 100.F }, .flYRatio { iYRatio / 100.F } } };
		}
	private:
		struct ItemData {
			HWND hWnd { };   //Item window.
			RECT rcOrig { }; //Item original client area rect after EnableTrack(true).
			MoveRatio move;  //How much to move the item.
			SizeRatio size;  //How much to resize the item.
		};
		HWND m_hWndHost { };   //Host window.
		RECT m_rcHostOrig { }; //Host original client area rect after EnableTrack(true).
		std::vector<ItemData> m_vecItems; //All items to resize/move.
		bool m_fTrack { };
	};

	void CDynLayout::AddItem(int iIDItem, MoveRatio move, SizeRatio size) {
		const auto hWnd = ::GetDlgItem(m_hWndHost, iIDItem);
		assert(hWnd != nullptr);
		if (hWnd != nullptr) {
			m_vecItems.emplace_back(ItemData { .hWnd { hWnd }, .move { move }, .size { size } });
		}
	}

	void CDynLayout::Enable(bool fTrack) {
		m_fTrack = fTrack;
		if (m_fTrack) {
			::GetClientRect(m_hWndHost, &m_rcHostOrig);
			for (auto& [hWnd, rc, move, size] : m_vecItems) {
				::GetWindowRect(hWnd, &rc);
				::ScreenToClient(m_hWndHost, reinterpret_cast<LPPOINT>(&rc));
				::ScreenToClient(m_hWndHost, reinterpret_cast<LPPOINT>(&rc) + 1);
			}
		}
	}

	void CDynLayout::OnSize(int iWidth, int iHeight)const {
		if (!m_fTrack)
			return;

		const auto iHostWidth = m_rcHostOrig.right - m_rcHostOrig.left;
		const auto iHostHeight = m_rcHostOrig.bottom - m_rcHostOrig.top;
		const auto iDeltaX = iWidth - iHostWidth;
		const auto iDeltaY = iHeight - iHostHeight;

		const auto hDWP = ::BeginDeferWindowPos(static_cast<int>(m_vecItems.size()));
		for (const auto& [hWnd, rc, move, size] : m_vecItems) {
			const auto iNewLeft = static_cast<int>(rc.left + (iDeltaX * move.flXRatio));
			const auto iNewTop = static_cast<int>(rc.top + (iDeltaY * move.flYRatio));
			const auto iOrigWidth = rc.right - rc.left;
			const auto iOrigHeight = rc.bottom - rc.top;
			const auto iNewWidth = static_cast<int>(iOrigWidth + (iDeltaX * size.flXRatio));
			const auto iNewHeight = static_cast<int>(iOrigHeight + (iDeltaY * size.flYRatio));
			::DeferWindowPos(hDWP, hWnd, nullptr, iNewLeft, iNewTop, iNewWidth, iNewHeight, SWP_NOZORDER | SWP_NOACTIVATE);
		}
		::EndDeferWindowPos(hDWP);
	}

	class CPoint final : public POINT {
	public:
		CPoint() : POINT { } { }
		CPoint(POINT pt) : POINT { pt } { }
		CPoint(int x, int y) : POINT { .x { x }, .y { y } } { }
		~CPoint() = default;
		operator LPPOINT() { return this; }
		operator const POINT*()const { return this; }
		bool operator==(CPoint rhs)const { return x == rhs.x && y == rhs.y; }
		bool operator==(POINT pt)const { return x == pt.x && y == pt.y; }
		friend bool operator==(POINT pt, CPoint rhs) { return rhs == pt; }
		CPoint operator+(POINT pt)const { return { x + pt.x, y + pt.y }; }
		CPoint operator-(POINT pt)const { return { x - pt.x, y - pt.y }; }
		void Offset(int iX, int iY) { x += iX; y += iY; }
		void Offset(POINT pt) { Offset(pt.x, pt.y); }
	};

	class CRect final : public RECT {
	public:
		CRect() : RECT { } { }
		CRect(int iLeft, int iTop, int iRight, int iBottom) : RECT { .left { iLeft }, .top { iTop },
			.right { iRight }, .bottom { iBottom } } {
		}
		CRect(RECT rc) { ::CopyRect(this, &rc); }
		CRect(LPCRECT pRC) { ::CopyRect(this, pRC); }
		CRect(POINT pt, SIZE size) : RECT { .left { pt.x }, .top { pt.y }, .right { pt.x + size.cx },
			.bottom { pt.y + size.cy } } {
		}
		CRect(POINT topLeft, POINT botRight) : RECT { .left { topLeft.x }, .top { topLeft.y },
			.right { botRight.x }, .bottom { botRight.y } } {
		}
		~CRect() = default;
		operator LPRECT() { return this; }
		operator LPCRECT()const { return this; }
		bool operator==(CRect rhs)const { return ::EqualRect(this, rhs); }
		bool operator==(RECT rc)const { return ::EqualRect(this, &rc); }
		friend bool operator==(RECT rc, CRect rhs) { return rhs == rc; }
		CRect& operator=(RECT rc) { ::CopyRect(this, &rc); return *this; }
		[[nodiscard]] auto BottomRight()const -> CPoint { return { { .x { right }, .y { bottom } } }; };
		void DeflateRect(int x, int y) { ::InflateRect(this, -x, -y); }
		void DeflateRect(SIZE size) { ::InflateRect(this, -size.cx, -size.cy); }
		void DeflateRect(LPCRECT pRC) { left += pRC->left; top += pRC->top; right -= pRC->right; bottom -= pRC->bottom; }
		void DeflateRect(int l, int t, int r, int b) { left += l; top += t; right -= r; bottom -= b; }
		[[nodiscard]] int Height()const { return bottom - top; }
		[[nodiscard]] bool IsRectEmpty()const { return ::IsRectEmpty(this); }
		[[nodiscard]] bool IsRectNull()const { return (left == 0 && right == 0 && top == 0 && bottom == 0); }
		void OffsetRect(int x, int y) { ::OffsetRect(this, x, y); }
		void OffsetRect(POINT pt) { ::OffsetRect(this, pt.x, pt.y); }
		[[nodiscard]] bool PtInRect(POINT pt)const { return ::PtInRect(this, pt); }
		void SetRect(int x1, int y1, int x2, int y2) { ::SetRect(this, x1, y1, x2, y2); }
		void SetRectEmpty() { ::SetRectEmpty(this); }
		[[nodiscard]] auto TopLeft()const -> CPoint { return { { .x { left }, .y { top } } }; };
		[[nodiscard]] int Width()const { return right - left; }
	};

	class CDC {
	public:
		CDC() = default;
		CDC(HDC hDC) : m_hDC(hDC) { }
		~CDC() = default;
		operator HDC()const { return m_hDC; }
		void AbortDoc()const { ::AbortDoc(m_hDC); }
		int AlphaBlend(int iX, int iY, int iWidth, int iHeight, HDC hDCSrc,
			int iXSrc, int iYSrc, int iWidthSrc, int iHeightSrc, BYTE bSrcAlpha = 255, BYTE bAlphaFormat = AC_SRC_ALPHA)const {
			const BLENDFUNCTION bf { .SourceConstantAlpha { bSrcAlpha }, .AlphaFormat { bAlphaFormat } };
			return ::AlphaBlend(m_hDC, iX, iY, iWidth, iHeight, hDCSrc, iXSrc, iYSrc, iWidthSrc, iHeightSrc, bf);
		}
		BOOL BitBlt(int iX, int iY, int iWidth, int iHeight, HDC hDCSource, int iXSource, int iYSource, DWORD dwROP)const {
			//When blitting from a monochrome bitmap to a color one, the black color in the monohrome bitmap 
			//becomes the destination DC’s text color, and the white color in the monohrome bitmap 
			//becomes the destination DC’s background color, when using SRCCOPY mode.
			return ::BitBlt(m_hDC, iX, iY, iWidth, iHeight, hDCSource, iXSource, iYSource, dwROP);
		}
		[[nodiscard]] auto CreateCompatibleBitmap(int iWidth, int iHeight)const -> HBITMAP {
			return ::CreateCompatibleBitmap(m_hDC, iWidth, iHeight);
		}
		[[nodiscard]] CDC CreateCompatibleDC()const { return ::CreateCompatibleDC(m_hDC); }
		void DeleteDC()const { ::DeleteDC(m_hDC); }
		bool DrawFrameControl(LPRECT pRC, UINT uType, UINT uState)const {
			return ::DrawFrameControl(m_hDC, pRC, uType, uState);
		}
		bool DrawFrameControl(int iX, int iY, int iWidth, int iHeight, UINT uType, UINT uState)const {
			RECT rc { .left { iX }, .top { iY }, .right { iX + iWidth }, .bottom { iY + iHeight } };
			return DrawFrameControl(&rc, uType, uState);
		}
		void DrawImage(HBITMAP hBmp, int iX, int iY, int iWidth, int iHeight)const {
			const auto dcMem = CreateCompatibleDC();
			dcMem.SelectObject(hBmp);
			BITMAP bm; ::GetObjectW(hBmp, sizeof(BITMAP), &bm);

			//Only 32bit bitmaps can have alpha channel.
			//If destination and source bitmaps do not have the same color format, 
			//AlphaBlend converts the source bitmap to match the destination bitmap.
			//AlphaBlend works with both, DI (DeviceIndependent) and DD (DeviceDependent), bitmaps.
			AlphaBlend(iX, iY, iWidth, iHeight, dcMem, 0, 0, iWidth, iHeight, 255, bm.bmBitsPixel == 32 ? AC_SRC_ALPHA : 0);
			dcMem.DeleteDC();
		}
		[[nodiscard]] HDC GetHDC()const { return m_hDC; }
		void GetTextMetricsW(LPTEXTMETRICW pTM)const { ::GetTextMetricsW(m_hDC, pTM); }
		auto SetBkColor(COLORREF clr)const -> COLORREF { return ::SetBkColor(m_hDC, clr); }
		void DrawEdge(LPRECT pRC, UINT uEdge, UINT uFlags)const { ::DrawEdge(m_hDC, pRC, uEdge, uFlags); }
		void DrawFocusRect(LPCRECT pRc)const { ::DrawFocusRect(m_hDC, pRc); }
		int DrawTextW(std::wstring_view wsv, LPRECT pRect, UINT uFormat)const {
			return DrawTextW(wsv.data(), static_cast<int>(wsv.size()), pRect, uFormat);
		}
		int DrawTextW(LPCWSTR pwszText, int iSize, LPRECT pRect, UINT uFormat)const {
			return ::DrawTextW(m_hDC, pwszText, iSize, pRect, uFormat);
		}
		int EndDoc()const { return ::EndDoc(m_hDC); }
		int EndPage()const { return ::EndPage(m_hDC); }
		void FillSolidRect(LPCRECT pRC, COLORREF clr)const {
			::SetBkColor(m_hDC, clr); ::ExtTextOutW(m_hDC, 0, 0, ETO_OPAQUE, pRC, nullptr, 0, nullptr);
		}
		[[nodiscard]] auto GetClipBox()const -> CRect { RECT rc; ::GetClipBox(m_hDC, &rc); return rc; }
		bool LineTo(POINT pt)const { return LineTo(pt.x, pt.y); }
		bool LineTo(int x, int y)const { return ::LineTo(m_hDC, x, y); }
		bool MoveTo(POINT pt)const { return MoveTo(pt.x, pt.y); }
		bool MoveTo(int x, int y)const { return ::MoveToEx(m_hDC, x, y, nullptr); }
		bool Polygon(const POINT* pPT, int iCount)const { return ::Polygon(m_hDC, pPT, iCount); }
		int SetDIBits(HBITMAP hBmp, UINT uStartLine, UINT uLines, const void* pBits, const BITMAPINFO* pBMI, UINT uClrUse)const {
			return ::SetDIBits(m_hDC, hBmp, uStartLine, uLines, pBits, pBMI, uClrUse);
		}
		int SetDIBitsToDevice(int iX, int iY, DWORD dwWidth, DWORD dwHeight, int iXSrc, int iYSrc, UINT uStartLine, UINT uLines,
			const void* pBits, const BITMAPINFO* pBMI, UINT uClrUse)const {
			return ::SetDIBitsToDevice(m_hDC, iX, iY, dwWidth, dwHeight, iXSrc, iYSrc, uStartLine, uLines, pBits, pBMI, uClrUse);
		}
		int SetMapMode(int iMode)const { return ::SetMapMode(m_hDC, iMode); }
		auto SetTextColor(COLORREF clr)const -> COLORREF { return ::SetTextColor(m_hDC, clr); }
		auto SetViewportOrg(int iX, int iY)const -> POINT { POINT pt; ::SetViewportOrgEx(m_hDC, iX, iY, &pt); return pt; }
		auto SelectObject(HGDIOBJ hObj)const -> HGDIOBJ { return ::SelectObject(m_hDC, hObj); }
		int StartDocW(const DOCINFOW* pDI)const { return ::StartDocW(m_hDC, pDI); }
		int StartPage()const { return ::StartPage(m_hDC); }
		void TextOutW(int iX, int iY, LPCWSTR pwszText, int iSize)const { ::TextOutW(m_hDC, iX, iY, pwszText, iSize); }
		void TextOutW(int iX, int iY, std::wstring_view wsv)const {
			TextOutW(iX, iY, wsv.data(), static_cast<int>(wsv.size()));
		}
	protected:
		HDC m_hDC;
	};

	class CPaintDC final : public CDC {
	public:
		CPaintDC(HWND hWnd) : m_hWnd(hWnd) { assert(::IsWindow(hWnd)); m_hDC = ::BeginPaint(m_hWnd, &m_PS); }
		~CPaintDC() { ::EndPaint(m_hWnd, &m_PS); }
	private:
		PAINTSTRUCT m_PS;
		HWND m_hWnd;
	};

	class CMemDC final : public CDC {
	public:
		CMemDC(HDC hDC, RECT rc) : m_hDCOrig(hDC), m_rc(rc) {
			m_hDC = ::CreateCompatibleDC(m_hDCOrig);
			assert(m_hDC != nullptr);
			const auto iWidth = m_rc.right - m_rc.left;
			const auto iHeight = m_rc.bottom - m_rc.top;
			m_hBmp = ::CreateCompatibleBitmap(m_hDCOrig, iWidth, iHeight);
			assert(m_hBmp != nullptr);
			::SelectObject(m_hDC, m_hBmp);
		}
		~CMemDC() {
			const auto iWidth = m_rc.right - m_rc.left;
			const auto iHeight = m_rc.bottom - m_rc.top;
			::BitBlt(m_hDCOrig, m_rc.left, m_rc.top, iWidth, iHeight, m_hDC, m_rc.left, m_rc.top, SRCCOPY);
			::DeleteObject(m_hBmp);
			::DeleteDC(m_hDC);
		}
	private:
		HDC m_hDCOrig;
		HBITMAP m_hBmp;
		RECT m_rc;
	};

	class CWnd {
	public:
		CWnd() = default;
		CWnd(HWND hWnd) { Attach(hWnd); }
		~CWnd() = default;
		CWnd& operator=(CWnd) = delete;
		CWnd& operator=(HWND) = delete;
		operator HWND()const { return m_hWnd; }
		[[nodiscard]] bool operator==(const CWnd& rhs)const { return m_hWnd == rhs.m_hWnd; }
		[[nodiscard]] bool operator==(HWND hWnd)const { return m_hWnd == hWnd; }
		void Attach(HWND hWnd) { m_hWnd = hWnd; } //Can attach to nullptr as well.
		void CheckRadioButton(int iIDFirst, int iIDLast, int iIDCheck)const {
			assert(IsWindow()); ::CheckRadioButton(m_hWnd, iIDFirst, iIDLast, iIDCheck);
		}
		[[nodiscard]] auto ChildWindowFromPoint(POINT pt)const -> CWnd {
			assert(IsWindow()); return ::ChildWindowFromPoint(m_hWnd, pt);
		}
		void ClientToScreen(LPPOINT pPT)const { assert(IsWindow()); ::ClientToScreen(m_hWnd, pPT); }
		void ClientToScreen(LPRECT pRC)const {
			ClientToScreen(reinterpret_cast<LPPOINT>(pRC)); ClientToScreen((reinterpret_cast<LPPOINT>(pRC)) + 1);
		}
		void DestroyWindow() { assert(IsWindow()); ::DestroyWindow(m_hWnd); m_hWnd = nullptr; }
		void Detach() { m_hWnd = nullptr; }
		[[nodiscard]] auto GetClientRect()const -> CRect {
			assert(IsWindow()); RECT rc; ::GetClientRect(m_hWnd, &rc); return rc;
		}
		bool EnableWindow(bool fEnable)const { assert(IsWindow()); return ::EnableWindow(m_hWnd, fEnable); }
		void EndDialog(INT_PTR iResult)const { assert(IsWindow()); ::EndDialog(m_hWnd, iResult); }
		[[nodiscard]] int GetCheckedRadioButton(int iIDFirst, int iIDLast)const {
			assert(IsWindow()); for (int iID = iIDFirst; iID <= iIDLast; ++iID) {
				if (::IsDlgButtonChecked(m_hWnd, iID) != 0) { return iID; }
			} return 0;
		}
		[[nodiscard]] auto GetDC()const -> HDC { assert(IsWindow()); return ::GetDC(m_hWnd); }
		[[nodiscard]] int GetDlgCtrlID()const { assert(IsWindow()); return ::GetDlgCtrlID(m_hWnd); }
		[[nodiscard]] auto GetDlgItem(int iIDCtrl)const -> CWnd { assert(IsWindow()); return ::GetDlgItem(m_hWnd, iIDCtrl); }
		[[nodiscard]] auto GetHFont()const -> HFONT {
			assert(IsWindow()); return reinterpret_cast<HFONT>(::SendMessageW(m_hWnd, WM_GETFONT, 0, 0));
		}
		[[nodiscard]] auto GetHWND()const -> HWND { return m_hWnd; }
		[[nodiscard]] auto GetLogFont()const -> std::optional<LOGFONTW> {
			if (const auto hFont = GetHFont(); hFont != nullptr) {
				LOGFONTW lf { }; ::GetObjectW(hFont, sizeof(lf), &lf); return lf;
			}
			return std::nullopt;
		}
		[[nodiscard]] auto GetParent()const -> CWnd { assert(IsWindow()); return ::GetParent(m_hWnd); }
		[[nodiscard]] auto GetScrollInfo(bool fVert, UINT uMask = SIF_ALL)const -> SCROLLINFO {
			assert(IsWindow()); SCROLLINFO si { .cbSize { sizeof(SCROLLINFO) }, .fMask { uMask } };
			::GetScrollInfo(m_hWnd, fVert, &si); return si;
		}
		[[nodiscard]] auto GetScrollPos(bool fVert)const -> int { return GetScrollInfo(fVert, SIF_POS).nPos; }
		[[nodiscard]] auto GetWindowDC()const -> HDC { assert(IsWindow()); return ::GetWindowDC(m_hWnd); }
		[[nodiscard]] auto GetWindowLongPTR(int iIndex)const {
			assert(IsWindow()); return ::GetWindowLongPtrW(m_hWnd, iIndex);
		}
		[[nodiscard]] auto GetWindowRect()const -> CRect {
			assert(IsWindow()); RECT rc; ::GetWindowRect(m_hWnd, &rc); return rc;
		}
		[[nodiscard]] auto GetWindowStyles()const { return static_cast<DWORD>(GetWindowLongPTR(GWL_STYLE)); }
		[[nodiscard]] auto GetWindowStylesEx()const { return static_cast<DWORD>(GetWindowLongPTR(GWL_EXSTYLE)); }
		[[nodiscard]] auto GetWndText()const -> std::wstring {
			assert(IsWindow()); wchar_t buff[256]; ::GetWindowTextW(m_hWnd, buff, std::size(buff)); return buff;
		}
		[[nodiscard]] auto GetWndTextSize()const -> DWORD { assert(IsWindow()); return ::GetWindowTextLengthW(m_hWnd); }
		void Invalidate(bool fErase)const { assert(IsWindow()); ::InvalidateRect(m_hWnd, nullptr, fErase); }
		[[nodiscard]] bool IsDlgMessage(MSG* pMsg)const { return ::IsDialogMessageW(m_hWnd, pMsg); }
		[[nodiscard]] bool IsNull()const { return m_hWnd == nullptr; }
		[[nodiscard]] bool IsWindow()const { return ::IsWindow(m_hWnd); }
		[[nodiscard]] bool IsWindowEnabled()const { assert(IsWindow()); return ::IsWindowEnabled(m_hWnd); }
		[[nodiscard]] bool IsWindowVisible()const { assert(IsWindow()); return ::IsWindowVisible(m_hWnd); }
		[[nodiscard]] bool IsWndTextEmpty()const { return GetWndTextSize() == 0; }
		void KillTimer(UINT_PTR uID)const {
			assert(IsWindow()); ::KillTimer(m_hWnd, uID);
		}
		int MapWindowPoints(HWND hWndTo, LPRECT pRC)const {
			assert(IsWindow()); return ::MapWindowPoints(m_hWnd, hWndTo, reinterpret_cast<LPPOINT>(pRC), 2);
		}
		bool RedrawWindow(LPCRECT pRC = nullptr, HRGN hrgn = nullptr, UINT uFlags = RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE)const {
			assert(IsWindow()); return static_cast<bool>(::RedrawWindow(m_hWnd, pRC, hrgn, uFlags));
		}
		int ReleaseDC(HDC hDC)const { assert(IsWindow()); return ::ReleaseDC(m_hWnd, hDC); }
		void ScreenToClient(LPPOINT pPT)const { assert(IsWindow()); ::ScreenToClient(m_hWnd, pPT); }
		void ScreenToClient(POINT& pt)const { ScreenToClient(&pt); }
		void ScreenToClient(LPRECT pRC)const {
			ScreenToClient(reinterpret_cast<LPPOINT>(pRC)); ScreenToClient(reinterpret_cast<LPPOINT>(pRC) + 1);
		}
		auto SendMsg(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0)const -> LRESULT {
			assert(IsWindow()); return ::SendMessageW(m_hWnd, uMsg, wParam, lParam);
		}
		void SetActiveWindow()const { assert(IsWindow()); ::SetActiveWindow(m_hWnd); }
		auto SetCapture()const -> CWnd { assert(IsWindow()); return ::SetCapture(m_hWnd); }
		auto SetClassLongPTR(int iIndex, LONG_PTR dwNewLong)const -> ULONG_PTR {
			assert(IsWindow()); return ::SetClassLongPtrW(m_hWnd, iIndex, dwNewLong);
		}
		void SetFocus()const { assert(IsWindow()); ::SetFocus(m_hWnd); }
		void SetForegroundWindow()const { assert(IsWindow()); ::SetForegroundWindow(m_hWnd); }
		void SetScrollInfo(bool fVert, const SCROLLINFO& si)const {
			assert(IsWindow()); ::SetScrollInfo(m_hWnd, fVert, &si, TRUE);
		}
		void SetScrollPos(bool fVert, int iPos)const {
			const SCROLLINFO si { .cbSize { sizeof(SCROLLINFO) }, .fMask { SIF_POS }, .nPos { iPos } };
			SetScrollInfo(fVert, si);
		}
		auto SetTimer(UINT_PTR uID, UINT uElapse, TIMERPROC pFN = nullptr)const -> UINT_PTR {
			assert(IsWindow()); return ::SetTimer(m_hWnd, uID, uElapse, pFN);
		}
		void SetWindowPos(HWND hWndAfter, int iX, int iY, int iWidth, int iHeight, UINT uFlags)const {
			assert(IsWindow()); ::SetWindowPos(m_hWnd, hWndAfter, iX, iY, iWidth, iHeight, uFlags);
		}
		auto SetWndClassLong(int iIndex, LONG_PTR dwNewLong)const -> ULONG_PTR {
			assert(IsWindow()); return ::SetClassLongPtrW(m_hWnd, iIndex, dwNewLong);
		}
		void SetWndText(LPCWSTR pwszStr)const { assert(IsWindow()); ::SetWindowTextW(m_hWnd, pwszStr); }
		void SetWndText(const std::wstring& wstr)const { SetWndText(wstr.data()); }
		void SetRedraw(bool fRedraw)const { assert(IsWindow()); ::SendMessageW(m_hWnd, WM_SETREDRAW, fRedraw, 0); }
		bool ShowWindow(int iCmdShow)const { assert(IsWindow()); return ::ShowWindow(m_hWnd, iCmdShow); }
		[[nodiscard]] static auto FromHandle(HWND hWnd) -> CWnd { return hWnd; }
		[[nodiscard]] static auto GetFocus() -> CWnd { return ::GetFocus(); }
	protected:
		HWND m_hWnd { }; //Windows window handle.
	};

	class CWndBtn final : public CWnd {
	public:
		[[nodiscard]] bool IsChecked()const { assert(IsWindow()); return ::SendMessageW(m_hWnd, BM_GETCHECK, 0, 0); }
		void SetBitmap(HBITMAP hBmp)const {
			assert(IsWindow()); ::SendMessageW(m_hWnd, BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(hBmp));
		}
		void SetCheck(bool fCheck)const { assert(IsWindow()); ::SendMessageW(m_hWnd, BM_SETCHECK, fCheck, 0); }
	};

	class CWndEdit final : public CWnd {
	public:
		void SetCueBanner(LPCWSTR pwszText, bool fDrawIfFocus = false)const {
			assert(IsWindow());
			::SendMessageW((m_hWnd), EM_SETCUEBANNER, static_cast<WPARAM>(fDrawIfFocus), reinterpret_cast<LPARAM>(pwszText));
		}
	};

	class CWndCombo final : public CWnd {
	public:
		int AddString(const std::wstring& wstr)const { return AddString(wstr.data()); }
		int AddString(LPCWSTR pwszStr)const {
			assert(IsWindow());
			return static_cast<int>(::SendMessageW(m_hWnd, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(pwszStr)));
		}
		int DeleteString(int iIndex)const {
			assert(IsWindow()); return static_cast<int>(::SendMessageW(m_hWnd, CB_DELETESTRING, iIndex, 0));
		}
		[[nodiscard]] int FindStringExact(int iIndex, LPCWSTR pwszStr)const {
			assert(IsWindow());
			return static_cast<int>(::SendMessageW(m_hWnd, CB_FINDSTRINGEXACT, iIndex, reinterpret_cast<LPARAM>(pwszStr)));
		}
		[[nodiscard]] int GetCount()const {
			assert(IsWindow()); return static_cast<int>(::SendMessageW(m_hWnd, CB_GETCOUNT, 0, 0));
		}
		[[nodiscard]] int GetCurSel()const {
			assert(IsWindow()); return static_cast<int>(::SendMessageW(m_hWnd, CB_GETCURSEL, 0, 0));
		}
		[[nodiscard]] auto GetItemData(int iIndex)const -> DWORD_PTR {
			assert(IsWindow()); return ::SendMessageW(m_hWnd, CB_GETITEMDATA, iIndex, 0);
		}
		[[nodiscard]] bool HasString(LPCWSTR pwszStr)const { return FindStringExact(0, pwszStr) != CB_ERR; };
		[[nodiscard]] bool HasString(const std::wstring& wstr)const { return HasString(wstr.data()); };
		int InsertString(int iIndex, const std::wstring& wstr)const { return InsertString(iIndex, wstr.data()); }
		int InsertString(int iIndex, LPCWSTR pwszStr)const {
			assert(IsWindow());
			return static_cast<int>(::SendMessageW(m_hWnd, CB_INSERTSTRING, iIndex, reinterpret_cast<LPARAM>(pwszStr)));
		}
		void LimitText(int iMaxChars)const { assert(IsWindow()); ::SendMessageW(m_hWnd, CB_LIMITTEXT, iMaxChars, 0); }
		void ResetContent()const { assert(IsWindow()); ::SendMessageW(m_hWnd, CB_RESETCONTENT, 0, 0); }
		void SetCueBanner(const std::wstring& wstr)const { SetCueBanner(wstr.data()); }
		void SetCueBanner(LPCWSTR pwszText)const {
			assert(IsWindow()); ::SendMessageW(m_hWnd, CB_SETCUEBANNER, 0, reinterpret_cast<LPARAM>(pwszText));
		}
		void SetCurSel(int iIndex)const { assert(IsWindow()); ::SendMessageW(m_hWnd, CB_SETCURSEL, iIndex, 0); }
		void SetItemData(int iIndex, DWORD_PTR dwData)const {
			assert(IsWindow()); ::SendMessageW(m_hWnd, CB_SETITEMDATA, iIndex, static_cast<LPARAM>(dwData));
		}
	};

	class CWndTree final : public CWnd {
	public:
		void DeleteAllItems()const { DeleteItem(TVI_ROOT); };
		void DeleteItem(HTREEITEM hItem)const {
			assert(IsWindow()); ::SendMessageW(m_hWnd, TVM_DELETEITEM, 0, reinterpret_cast<LPARAM>(hItem));
		}
		void Expand(HTREEITEM hItem, UINT uCode)const {
			assert(IsWindow()); ::SendMessageW(m_hWnd, TVM_EXPAND, uCode, reinterpret_cast<LPARAM>(hItem));
		}
		void GetItem(TVITEMW* pItem)const {
			assert(IsWindow()); ::SendMessageW(m_hWnd, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(pItem));
		}
		[[nodiscard]] auto GetItemData(HTREEITEM hItem)const -> DWORD_PTR {
			TVITEMW item { .mask { TVIF_PARAM }, .hItem { hItem } }; GetItem(&item); return item.lParam;
		}
		[[nodiscard]] auto GetNextItem(HTREEITEM hItem, UINT uCode)const -> HTREEITEM {
			assert(IsWindow()); return reinterpret_cast<HTREEITEM>
				(::SendMessageW(m_hWnd, TVM_GETNEXTITEM, uCode, reinterpret_cast<LPARAM>(hItem)));
		}
		[[nodiscard]] auto GetNextSiblingItem(HTREEITEM hItem)const -> HTREEITEM { return GetNextItem(hItem, TVGN_NEXT); }
		[[nodiscard]] auto GetParentItem(HTREEITEM hItem)const -> HTREEITEM { return GetNextItem(hItem, TVGN_PARENT); }
		[[nodiscard]] auto GetRootItem()const -> HTREEITEM { return GetNextItem(nullptr, TVGN_ROOT); }
		[[nodiscard]] auto GetSelectedItem()const -> HTREEITEM { return GetNextItem(nullptr, TVGN_CARET); }
		[[nodiscard]] auto HitTest(TVHITTESTINFO* pHTI)const -> HTREEITEM {
			assert(IsWindow());
			return reinterpret_cast<HTREEITEM>(::SendMessageW(m_hWnd, TVM_HITTEST, 0, reinterpret_cast<LPARAM>(pHTI)));
		}
		[[nodiscard]] auto HitTest(POINT pt, UINT* pFlags = nullptr)const -> HTREEITEM {
			TVHITTESTINFO hti { .pt { pt } }; const auto ret = HitTest(&hti);
			if (pFlags != nullptr) { *pFlags = hti.flags; }
			return ret;
		}
		auto InsertItem(LPTVINSERTSTRUCTW pTIS)const -> HTREEITEM {
			assert(IsWindow()); return reinterpret_cast<HTREEITEM>
				(::SendMessageW(m_hWnd, TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(pTIS)));
		}
		void SelectItem(HTREEITEM hItem)const {
			assert(IsWindow()); ::SendMessageW(m_hWnd, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(hItem));
		}
	};

	class CWndProgBar final : public CWnd {
	public:
		int SetPos(int iPos)const {
			assert(IsWindow()); return static_cast<int>(::SendMessageW(m_hWnd, PBM_SETPOS, iPos, 0UL));
		}
		void SetRange(int iLower, int iUpper)const {
			assert(IsWindow());
			::SendMessageW(m_hWnd, PBM_SETRANGE32, static_cast<WPARAM>(iLower), static_cast<LPARAM>(iUpper));
		}
	};

	class CWndTab final : public CWnd {
	public:
		[[nodiscard]] int GetCurSel()const {
			assert(IsWindow()); return static_cast<int>(::SendMessageW(m_hWnd, TCM_GETCURSEL, 0, 0L));
		}
		[[nodiscard]] auto GetItemRect(int nItem)const -> CRect {
			assert(IsWindow());
			RECT rc { }; ::SendMessageW(m_hWnd, TCM_GETITEMRECT, nItem, reinterpret_cast<LPARAM>(&rc));
			return rc;
		}
		auto InsertItem(int iItem, TCITEMW* pItem)const -> LONG {
			assert(IsWindow());
			return static_cast<LONG>(::SendMessageW(m_hWnd, TCM_INSERTITEMW, iItem, reinterpret_cast<LPARAM>(pItem)));
		}
		auto InsertItem(int iItem, LPCWSTR pwszStr)const -> LONG {
			TCITEMW tci { .mask { TCIF_TEXT }, .pszText { const_cast<LPWSTR>(pwszStr) } };
			return InsertItem(iItem, &tci);
		}
		int SetCurSel(int iItem)const {
			assert(IsWindow()); return static_cast<int>(::SendMessageW(m_hWnd, TCM_SETCURSEL, iItem, 0L));
		}
	};

	class CMenu {
	public:
		CMenu() = default;
		CMenu(HMENU hMenu) { Attach(hMenu); }
		~CMenu() = default;
		CMenu operator=(const CWnd&) = delete;
		CMenu operator=(HMENU) = delete;
		operator HMENU()const { return m_hMenu; }
		void AppendItem(UINT uFlags, UINT_PTR uIDItem, LPCWSTR pNameItem)const {
			assert(IsMenu()); ::AppendMenuW(m_hMenu, uFlags, uIDItem, pNameItem);
		}
		void AppendSepar()const { AppendItem(MF_SEPARATOR, 0, nullptr); }
		void AppendString(UINT_PTR uIDItem, LPCWSTR pNameItem)const { AppendItem(MF_STRING, uIDItem, pNameItem); }
		void Attach(HMENU hMenu) { m_hMenu = hMenu; }
		void CreatePopupMenu() { Attach(::CreatePopupMenu()); }
		void DestroyMenu() { assert(IsMenu()); ::DestroyMenu(m_hMenu); m_hMenu = nullptr; }
		void Detach() { m_hMenu = nullptr; }
		void EnableItem(UINT uIDItem, bool fEnable, bool fByID = true)const {
			assert(IsMenu()); ::EnableMenuItem(m_hMenu, uIDItem, (fEnable ? MF_ENABLED : MF_GRAYED) |
				(fByID ? MF_BYCOMMAND : MF_BYPOSITION));
		}
		[[nodiscard]] auto GetHMENU()const -> HMENU { return m_hMenu; }
		[[nodiscard]] auto GetItemBitmap(UINT uID, bool fByID = true)const -> HBITMAP {
			return GetItemInfo(uID, MIIM_BITMAP, fByID).hbmpItem;
		}
		[[nodiscard]] auto GetItemBitmapCheck(UINT uID, bool fByID = true)const -> HBITMAP {
			return GetItemInfo(uID, MIIM_CHECKMARKS, fByID).hbmpChecked;
		}
		[[nodiscard]] auto GetItemID(int iPos)const -> UINT {
			assert(IsMenu()); return ::GetMenuItemID(m_hMenu, iPos);
		}
		bool GetItemInfo(UINT uID, LPMENUITEMINFOW pMII, bool fByID = true)const {
			assert(IsMenu()); return ::GetMenuItemInfoW(m_hMenu, uID, !fByID, pMII) != FALSE;
		}
		[[nodiscard]] auto GetItemInfo(UINT uID, UINT uMask, bool fByID = true)const -> MENUITEMINFOW {
			MENUITEMINFOW mii { .cbSize { sizeof(MENUITEMINFOW) }, .fMask { uMask } };
			GetItemInfo(uID, &mii, fByID); return mii;
		}
		[[nodiscard]] auto GetItemState(UINT uID, bool fByID = true)const -> UINT {
			assert(IsMenu()); return ::GetMenuState(m_hMenu, uID, fByID ? MF_BYCOMMAND : MF_BYPOSITION);
		}
		[[nodiscard]] auto GetItemType(UINT uID, bool fByID = true)const -> UINT {
			return GetItemInfo(uID, MIIM_FTYPE, fByID).fType;
		}
		[[nodiscard]] auto GetItemWstr(UINT uID, bool fByID = true)const -> std::wstring {
			wchar_t buff[128]; MENUITEMINFOW mii { .cbSize { sizeof(MENUITEMINFOW) }, .fMask { MIIM_STRING },
				.dwTypeData { buff }, .cch { 128 } }; return GetItemInfo(uID, &mii, fByID) ? buff : std::wstring { };
		}
		[[nodiscard]] int GetItemsCount()const {
			assert(IsMenu()); return ::GetMenuItemCount(m_hMenu);
		}
		[[nodiscard]] auto GetSubMenu(int iPos)const -> CMenu { assert(IsMenu()); return ::GetSubMenu(m_hMenu, iPos); };
		[[nodiscard]] bool IsItemChecked(UINT uIDItem, bool fByID = true)const {
			return GetItemState(uIDItem, fByID) & MF_CHECKED;
		}
		[[nodiscard]] bool IsItemSepar(UINT uPos)const { return GetItemState(uPos, false) & MF_SEPARATOR; }
		[[nodiscard]] bool IsMenu()const { return ::IsMenu(m_hMenu); }
		bool LoadMenuW(HINSTANCE hInst, LPCWSTR pwszName) { m_hMenu = ::LoadMenuW(hInst, pwszName); return IsMenu(); }
		bool LoadMenuW(HINSTANCE hInst, UINT uMenuID) { return LoadMenuW(hInst, MAKEINTRESOURCEW(uMenuID)); }
		void SetItemBitmap(UINT uItem, HBITMAP hBmp, bool fByID = true)const {
			const MENUITEMINFOW mii { .cbSize { sizeof(MENUITEMINFOW) }, .fMask { MIIM_BITMAP }, .hbmpItem { hBmp } };
			SetItemInfo(uItem, &mii, fByID);
		}
		void SetItemBitmapCheck(UINT uItem, HBITMAP hBmp, bool fByID = true)const {
			::SetMenuItemBitmaps(m_hMenu, uItem, fByID ? MF_BYCOMMAND : MF_BYPOSITION, nullptr, hBmp);
		}
		void SetItemCheck(UINT uIDItem, bool fCheck, bool fByID = true)const {
			assert(IsMenu()); ::CheckMenuItem(m_hMenu, uIDItem, (fCheck ? MF_CHECKED : MF_UNCHECKED) |
				(fByID ? MF_BYCOMMAND : MF_BYPOSITION));
		}
		void SetItemData(UINT uItem, ULONG_PTR dwData, bool fByID = true)const {
			const MENUITEMINFOW mii { .cbSize { sizeof(MENUITEMINFOW) }, .fMask { MIIM_DATA }, .dwItemData { dwData } };
			SetItemInfo(uItem, &mii, fByID);
		}
		void SetItemInfo(UINT uItem, LPCMENUITEMINFO pMII, bool fByID = true)const {
			assert(IsMenu()); ::SetMenuItemInfoW(m_hMenu, uItem, !fByID, pMII);
		}
		void SetItemType(UINT uItem, UINT uType, bool fByID = true)const {
			const MENUITEMINFOW mii { .cbSize { sizeof(MENUITEMINFOW) }, .fMask { MIIM_FTYPE }, .fType { uType } };
			SetItemInfo(uItem, &mii, fByID);
		}
		void SetItemWstr(UINT uItem, const std::wstring& wstr, bool fByID = true)const {
			const MENUITEMINFOW mii { .cbSize { sizeof(MENUITEMINFOW) }, .fMask { MIIM_STRING },
				.dwTypeData { const_cast<LPWSTR>(wstr.data()) } };
			SetItemInfo(uItem, &mii, fByID);
		}
		BOOL TrackPopupMenu(int iX, int iY, HWND hWndOwner, UINT uFlags = TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON)const {
			assert(IsMenu()); return ::TrackPopupMenuEx(m_hMenu, uFlags, iX, iY, hWndOwner, nullptr);
		}
	private:
		HMENU m_hMenu { }; //Windows menu handle.
	};
};