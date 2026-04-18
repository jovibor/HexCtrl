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
#include <cmath>
#include <cwctype>
#include <format>
#include <locale>
#include <optional>
#include <source_location>
#include <string>
#include <unordered_map>
#include <vector>
export module HEXCTRL:HexUtility;

#pragma comment(lib, "MSImg32") //AlphaBlend.

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
	[[nodiscard]] T GetIHexTData(const IHexCtrl& HexCtrl, ULONGLONG ullOffset)
	{
		const auto spnData = HexCtrl.GetData({ .ullOffset { ullOffset }, .ullSize { sizeof(T) } });
		assert(!spnData.empty());
		return *reinterpret_cast<T*>(spnData.data());
	}

	//Set data of a necessary type to IHexCtrl's given offset.
	template<typename T>
	void SetIHexTData(IHexCtrl& HexCtrl, ULONGLONG ullOffset, T tData)
	{
		if (ullOffset + sizeof(T) > HexCtrl.GetDataSize()) { //Data overflow check.
			DBG_REPORT(L"Data overflow occurs.");
			return;
		}

		HexCtrl.ModifyData({ .eModifyMode { EHexModifyMode::MODIFY_ONCE },
			.spnData { reinterpret_cast<std::byte*>(&tData), sizeof(T) },
			.vecSpan { { ullOffset, sizeof(T) } } });
	}

	void SetIHexTData(IHexCtrl& HexCtrl, ULONGLONG ullOffset, SpanCByte spnData)
	{
		if (ullOffset + spnData.size() > HexCtrl.GetDataSize()) { //Data overflow check.
			DBG_REPORT(L"Data overflow occurs.");
			return;
		}

		HexCtrl.ModifyData({ .eModifyMode { EHexModifyMode::MODIFY_ONCE },
			.spnData { spnData }, .vecSpan { { ullOffset, spnData.size() } } });
	}

	template<typename T> concept TSize1248 = (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);

	template<TSize1248 T>
	[[nodiscard]] constexpr T ByteSwap(T tData)noexcept
	{
		//Since a swapping-data type can be any type of 2, 4, or 8 bytes size,
		//we first bit_cast swapping-data to an integral type of the same size,
		//then byte-swapping and then bit_cast to the original type back.
		if constexpr (sizeof(T) == sizeof(std::uint8_t)) { //1 byte.
			return tData;
		}
		else if constexpr (sizeof(T) == sizeof(std::uint16_t)) { //2 bytes.
			auto u16Data = std::bit_cast<std::uint16_t>(tData);
			if (std::is_constant_evaluated()) {
				u16Data = static_cast<std::uint16_t>((u16Data << 8) | (u16Data >> 8));
				return std::bit_cast<T>(u16Data);
			}
			return std::bit_cast<T>(_byteswap_ushort(u16Data));
		}
		else if constexpr (sizeof(T) == sizeof(std::uint32_t)) { //4 bytes.
			auto u32Data = std::bit_cast<std::uint32_t>(tData);
			if (std::is_constant_evaluated()) {
				u32Data = (u32Data << 24) | ((u32Data << 8) & 0x00FF'0000U)
					| ((u32Data >> 8) & 0x0000'FF00U) | (u32Data >> 24);
				return std::bit_cast<T>(u32Data);
			}
			return std::bit_cast<T>(_byteswap_ulong(u32Data));
		}
		else if constexpr (sizeof(T) == sizeof(std::uint64_t)) { //8 bytes.
			auto u64Data = std::bit_cast<std::uint64_t>(tData);
			if (std::is_constant_evaluated()) {
				u64Data = (u64Data << 56) | ((u64Data << 40) & 0x00FF'0000'0000'0000ULL)
					| ((u64Data << 24) & 0x0000'FF00'0000'0000ULL) | ((u64Data << 8) & 0x0000'00FF'0000'0000ULL)
					| ((u64Data >> 8) & 0x0000'0000'FF00'0000ULL) | ((u64Data >> 24) & 0x0000'0000'00FF'0000ULL)
					| ((u64Data >> 40) & 0x0000'0000'0000'FF00ULL) | (u64Data >> 56);
				return std::bit_cast<T>(u64Data);
			}
			return std::bit_cast<T>(_byteswap_uint64(u64Data));
		}
	}

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
			const std::size_t uzOffsetCurr = it - wsv.begin();
			const auto iSize = uzOffsetCurr + 2 <= wsv.size() ? 2 : 1;
			if (const auto optNumber = stn::StrToUInt8(wsv.substr(uzOffsetCurr, iSize), 16); optNumber) {
				it += iSize;
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
		if (const auto uzPos = wstrDateTimeCooked.find(L' '); uzPos != std::wstring::npos) {
			wstrDateTimeCooked = wstrDateTimeCooked.substr(uzPos + 1);

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
	[[nodiscard]] auto RangeToVecBytes(const T& TData) -> std::vector<std::byte>
	{
		const std::byte* pBegin;
		const std::byte* pEnd;
		if constexpr (std::is_same_v<T, std::string>) {
			pBegin = reinterpret_cast<const std::byte*>(TData.data());
			pEnd = pBegin + TData.size();
		}
		else if constexpr (std::is_same_v<T, std::wstring>) {
			pBegin = reinterpret_cast<const std::byte*>(TData.data());
			pEnd = pBegin + TData.size() * sizeof(wchar_t);
		}
		else {
			pBegin = reinterpret_cast<const std::byte*>(&TData);
			pEnd = pBegin + sizeof(TData);
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

	//Replicates GET_X_LPARAM macro from windowsx.h.
	[[nodiscard]] constexpr int GetXLPARAM(LPARAM lParam) {
		return static_cast<int>(static_cast<short>(static_cast<DWORD_PTR>(lParam) & 0xFFFFU));
	}

	//Replicates GET_Y_LPARAM macro from windowsx.h.
	[[nodiscard]] constexpr int GetYLPARAM(LPARAM lParam) {
		return GetXLPARAM(static_cast<DWORD_PTR>(lParam) >> 16);
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

	[[nodiscard]] auto GetDPIScaleForHWND(HWND hWnd) -> float {
		return static_cast<float>(::GetDpiForWindow(hWnd)) / USER_DEFAULT_SCREEN_DPI; //High-DPI scale factor for window.
	}

	//Get GDI font size in points from the size in pixels.
	[[nodiscard]] auto FontPointsFromPixels(long iSizePixels) -> float {
		constexpr auto flPointsInPixel = 72.F / USER_DEFAULT_SCREEN_DPI;
		return std::abs(iSizePixels) * flPointsInPixel;
	}

	//Get GDI font size in pixels from the size in points.
	[[nodiscard]] auto FontPixelsFromPoints(float flSizePoints) -> long {
		constexpr auto flPixelsInPoint = USER_DEFAULT_SCREEN_DPI / 72.F;
		return std::lround(flSizePoints * flPixelsInPoint);
	}

	//iDirection: 1:UP, -1:DOWN, -2:LEFT, 2:RIGHT.
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

	class CDynLayout final {
	public:
		//Ratio settings, for how much to move or to resize child item when parent is resized.
		struct ItemRatio {
			[[nodiscard]] bool IsNull()const { return flXRatio == 0.F && flYRatio == 0.F; };
			float flXRatio { }; float flYRatio { };
		};
		struct MoveRatio : public ItemRatio { }; //To differentiate move from size in the AddItem.
		struct SizeRatio : public ItemRatio { };

		CDynLayout() = default;
		CDynLayout(HWND hWndHost) : m_hWndHost(hWndHost) { }
		void AddItem(int iIDItem, MoveRatio move, SizeRatio size);
		void AddItem(HWND hWndItem, MoveRatio move, SizeRatio size);
		void Enable(bool fTrack);
		bool LoadFromResource(HINSTANCE hInstRes, const wchar_t* pwszNameResource);
		bool LoadFromResource(HINSTANCE hInstRes, UINT uNameResource);
		void OnSize(int iWidth, int iHeight)const; //Should be hooked into the host window's WM_SIZE handler.
		void RemoveAll() { m_vecItems.clear(); }
		void SetHost(HWND hWnd) { assert(hWnd != nullptr); m_hWndHost = hWnd; }

		//Static helper methods to use in the AddItem.
		[[nodiscard]] static MoveRatio MoveNone() { return { }; }
		[[nodiscard]] static MoveRatio MoveHorz(int iXRatio) {
			return { { .flXRatio { ToFlRatio(iXRatio) } } };
		}
		[[nodiscard]] static MoveRatio MoveVert(int iYRatio) {
			return { { .flYRatio { ToFlRatio(iYRatio) } } };
		}
		[[nodiscard]] static MoveRatio MoveHorzAndVert(int iXRatio, int iYRatio) {
			return { { .flXRatio { ToFlRatio(iXRatio) }, .flYRatio { ToFlRatio(iYRatio) } } };
		}
		[[nodiscard]] static SizeRatio SizeNone() { return { }; }
		[[nodiscard]] static SizeRatio SizeHorz(int iXRatio) {
			return { { .flXRatio { ToFlRatio(iXRatio) } } };
		}
		[[nodiscard]] static SizeRatio SizeVert(int iYRatio) {
			return { { .flYRatio { ToFlRatio(iYRatio) } } };
		}
		[[nodiscard]] static SizeRatio SizeHorzAndVert(int iXRatio, int iYRatio) {
			return { { .flXRatio { ToFlRatio(iXRatio) }, .flYRatio { ToFlRatio(iYRatio) } } };
		}
	private:
		[[nodiscard]] static auto ToFlRatio(int iRatio) -> float {
			return std::clamp(iRatio, 0, 100) / 100.F;
		}
		struct ItemData {
			HWND hWnd { };   //Item window.
			RECT rcOrig { }; //Item original window rect after EnableTrack(true).
			MoveRatio move;  //How much to move the item.
			SizeRatio size;  //How much to resize the item.
		};
		HWND m_hWndHost { };   //Host window.
		RECT m_rcHostOrig { }; //Host original client area rect after EnableTrack(true).
		std::vector<ItemData> m_vecItems; //All items to resize/move.
		bool m_fTrack { };
	};

	void CDynLayout::AddItem(int iIDItem, MoveRatio move, SizeRatio size) {
		AddItem(::GetDlgItem(m_hWndHost, iIDItem), move, size);
	}

	void CDynLayout::AddItem(HWND hWndItem, MoveRatio move, SizeRatio size) {
		assert(hWndItem != nullptr);
		if (hWndItem == nullptr)
			return;

		if (move.IsNull() && size.IsNull())
			return;

		m_vecItems.emplace_back(ItemData { .hWnd { hWndItem }, .move { move }, .size { size } });
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

	bool CDynLayout::LoadFromResource(HINSTANCE hInstRes, const wchar_t* pwszNameResource) {
		assert(pwszNameResource != nullptr);
		if (pwszNameResource == nullptr)
			return false;

		assert(m_hWndHost != nullptr);
		if (m_hWndHost == nullptr)
			return false;

		const auto hDlgLayout = ::FindResourceW(hInstRes, pwszNameResource, L"AFX_DIALOG_LAYOUT");
		if (hDlgLayout == nullptr) { //No such resource found in the hInstRes.
			return false;
		}

		const auto hResData = ::LoadResource(hInstRes, hDlgLayout);
		assert(hResData != nullptr);
		if (hResData == nullptr)
			return false;

		const auto pResData = ::LockResource(hResData);
		assert(pResData != nullptr);
		if (pResData == nullptr)
			return false;

		const auto dwSizeRes = ::SizeofResource(hInstRes, hDlgLayout);
		const auto* pDataBegin = reinterpret_cast<WORD*>(pResData);
		const auto* const pDataEnd = reinterpret_cast<WORD*>(reinterpret_cast<std::byte*>(pResData) + dwSizeRes);

		assert(*pDataBegin == 0);
		if (*pDataBegin != 0) //First WORD must be zero, it's a header (version number).
			return false;

		++pDataBegin; //Past first WORD is the actual data.
		auto hWndChild = ::GetWindow(m_hWndHost, GW_CHILD); //First child window in the host window.
		while (pDataBegin + 4 <= pDataEnd) { //Actual AFX_DIALOG_LAYOUT data.
			if (hWndChild == nullptr)
				break;

			const auto wXMoveRatio = *pDataBegin++;
			const auto wYMoveRatio = *pDataBegin++;
			const auto wXSizeRatio = *pDataBegin++;
			const auto wYSizeRatio = *pDataBegin++;
			AddItem(hWndChild, MoveHorzAndVert(wXMoveRatio, wYMoveRatio), SizeHorzAndVert(wXSizeRatio, wYSizeRatio));
			hWndChild = ::GetWindow(hWndChild, GW_HWNDNEXT);
		}

		return true;
	}

	bool CDynLayout::LoadFromResource(HINSTANCE hInstRes, UINT uNameResource) {
		return LoadFromResource(hInstRes, MAKEINTRESOURCEW(uNameResource));
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
			.right { iRight }, .bottom { iBottom } } { }
		CRect(RECT rc) { ::CopyRect(this, &rc); }
		CRect(LPCRECT pRC) { ::CopyRect(this, pRC); }
		CRect(POINT pt, SIZE size) : RECT { .left { pt.x }, .top { pt.y }, .right { pt.x + size.cx },
			.bottom { pt.y + size.cy } } { }
		CRect(POINT topLeft, POINT botRight) : RECT { .left { topLeft.x }, .top { topLeft.y },
			.right { botRight.x }, .bottom { botRight.y } } { }
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
		CWnd& operator=(HWND hWnd) { Attach(hWnd); return *this; };
		CWnd& operator=(CWnd) = delete;
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
			assert(IsWindow()); return reinterpret_cast<HFONT>(SendMsg(WM_GETFONT, 0, 0));
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
		void SetRedraw(bool fRedraw)const { assert(IsWindow()); SendMsg(WM_SETREDRAW, fRedraw, 0); }
		bool ShowWindow(int iCmdShow)const { assert(IsWindow()); return ::ShowWindow(m_hWnd, iCmdShow); }
		[[nodiscard]] static auto FromHandle(HWND hWnd) -> CWnd { return hWnd; }
		[[nodiscard]] static auto GetFocus() -> CWnd { return ::GetFocus(); }
	protected:
		HWND m_hWnd { }; //Windows window handle.
	};

	class CWndBtn final : public CWnd {
	public:
		[[nodiscard]] bool IsChecked()const { assert(IsWindow()); return SendMsg(BM_GETCHECK, 0, 0); }
		void SetBitmap(HBITMAP hBmp)const {
			assert(IsWindow()); SendMsg(BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(hBmp));
		}
		void SetCheck(bool fCheck)const { assert(IsWindow()); SendMsg(BM_SETCHECK, fCheck, 0); }
	};

	class CWndEdit final : public CWnd {
	public:
		void SetCueBanner(LPCWSTR pwszText, bool fDrawIfFocus = false)const {
			assert(IsWindow()); SendMsg(EM_SETCUEBANNER, fDrawIfFocus, reinterpret_cast<LPARAM>(pwszText));
		}
		void SetLimitText(int iSize) { assert(IsWindow()); SendMsg(EM_LIMITTEXT, iSize); }
	};

	class CWndCombo final : public CWnd {
	public:
		int AddString(const std::wstring& wstr)const { return AddString(wstr.data()); }
		int AddString(LPCWSTR pwszStr)const {
			assert(IsWindow());
			return static_cast<int>(SendMsg(CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(pwszStr)));
		}
		int DeleteString(int iIndex)const {
			assert(IsWindow()); return static_cast<int>(SendMsg(CB_DELETESTRING, iIndex, 0));
		}
		[[nodiscard]] int FindStringExact(int iIndex, LPCWSTR pwszStr)const {
			assert(IsWindow());
			return static_cast<int>(SendMsg(CB_FINDSTRINGEXACT, iIndex, reinterpret_cast<LPARAM>(pwszStr)));
		}
		[[nodiscard]] auto GetComboBoxInfo()const -> COMBOBOXINFO {
			assert(IsWindow());	COMBOBOXINFO cbi { .cbSize { sizeof(COMBOBOXINFO) } };
			::GetComboBoxInfo(GetHWND(), &cbi); return cbi;
		}
		[[nodiscard]] int GetCount()const {
			assert(IsWindow()); return static_cast<int>(SendMsg(CB_GETCOUNT, 0, 0));
		}
		[[nodiscard]] int GetCurSel()const {
			assert(IsWindow()); return static_cast<int>(SendMsg(CB_GETCURSEL, 0, 0));
		}
		[[nodiscard]] auto GetItemData(int iIndex)const -> DWORD_PTR {
			assert(IsWindow()); return SendMsg(CB_GETITEMDATA, iIndex, 0);
		}
		[[nodiscard]] bool HasString(LPCWSTR pwszStr)const { return FindStringExact(-1, pwszStr) != CB_ERR; };
		[[nodiscard]] bool HasString(const std::wstring& wstr)const { return HasString(wstr.data()); };
		int InsertString(int iIndex, const std::wstring& wstr)const { return InsertString(iIndex, wstr.data()); }
		int InsertString(int iIndex, LPCWSTR pwszStr)const {
			assert(IsWindow());
			return static_cast<int>(SendMsg(CB_INSERTSTRING, iIndex, reinterpret_cast<LPARAM>(pwszStr)));
		}
		void LimitText(int iMaxChars)const { assert(IsWindow()); SendMsg(CB_LIMITTEXT, iMaxChars, 0); }
		void ResetContent()const { assert(IsWindow()); SendMsg(CB_RESETCONTENT, 0, 0); }
		void SetCueBanner(const std::wstring& wstr)const { SetCueBanner(wstr.data()); }
		void SetCueBanner(LPCWSTR pwszText)const {
			assert(IsWindow()); SendMsg(CB_SETCUEBANNER, 0, reinterpret_cast<LPARAM>(pwszText));
		}
		void SetCurSel(int iIndex)const { assert(IsWindow()); SendMsg(CB_SETCURSEL, iIndex, 0); }
		void SetItemData(int iIndex, DWORD_PTR dwData)const {
			assert(IsWindow()); SendMsg(CB_SETITEMDATA, iIndex, static_cast<LPARAM>(dwData));
		}
	};

	class CWndTree final : public CWnd {
	public:
		void DeleteAllItems()const { DeleteItem(TVI_ROOT); };
		void DeleteItem(HTREEITEM hItem)const {
			assert(IsWindow()); SendMsg(TVM_DELETEITEM, 0, reinterpret_cast<LPARAM>(hItem));
		}
		void Expand(HTREEITEM hItem, UINT uCode)const {
			assert(IsWindow()); SendMsg(TVM_EXPAND, uCode, reinterpret_cast<LPARAM>(hItem));
		}
		void GetItem(TVITEMW* pItem)const {
			assert(IsWindow()); SendMsg(TVM_GETITEM, 0, reinterpret_cast<LPARAM>(pItem));
		}
		[[nodiscard]] auto GetItemData(HTREEITEM hItem)const -> DWORD_PTR {
			TVITEMW item { .mask { TVIF_PARAM }, .hItem { hItem } }; GetItem(&item); return item.lParam;
		}
		[[nodiscard]] auto GetNextItem(HTREEITEM hItem, UINT uCode)const -> HTREEITEM {
			assert(IsWindow()); return reinterpret_cast<HTREEITEM>
				(SendMsg(TVM_GETNEXTITEM, uCode, reinterpret_cast<LPARAM>(hItem)));
		}
		[[nodiscard]] auto GetNextSiblingItem(HTREEITEM hItem)const -> HTREEITEM { return GetNextItem(hItem, TVGN_NEXT); }
		[[nodiscard]] auto GetParentItem(HTREEITEM hItem)const -> HTREEITEM { return GetNextItem(hItem, TVGN_PARENT); }
		[[nodiscard]] auto GetRootItem()const -> HTREEITEM { return GetNextItem(nullptr, TVGN_ROOT); }
		[[nodiscard]] auto GetSelectedItem()const -> HTREEITEM { return GetNextItem(nullptr, TVGN_CARET); }
		[[nodiscard]] auto HitTest(TVHITTESTINFO* pHTI)const -> HTREEITEM {
			assert(IsWindow());
			return reinterpret_cast<HTREEITEM>(SendMsg(TVM_HITTEST, 0, reinterpret_cast<LPARAM>(pHTI)));
		}
		[[nodiscard]] auto HitTest(POINT pt, UINT* pFlags = nullptr)const -> HTREEITEM {
			TVHITTESTINFO hti { .pt { pt } }; const auto ret = HitTest(&hti);
			if (pFlags != nullptr) { *pFlags = hti.flags; }
			return ret;
		}
		auto InsertItem(LPTVINSERTSTRUCTW pTIS)const -> HTREEITEM {
			assert(IsWindow()); return reinterpret_cast<HTREEITEM>
				(SendMsg(TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(pTIS)));
		}
		void SelectItem(HTREEITEM hItem)const {
			assert(IsWindow()); SendMsg(TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(hItem));
		}
	};

	class CWndProgBar final : public CWnd {
	public:
		int SetPos(int iPos)const {
			assert(IsWindow()); return static_cast<int>(SendMsg(PBM_SETPOS, iPos, 0UL));
		}
		void SetRange(int iLower, int iUpper)const {
			assert(IsWindow());
			SendMsg(PBM_SETRANGE32, static_cast<WPARAM>(iLower), static_cast<LPARAM>(iUpper));
		}
	};

	class CWndTab final : public CWnd {
	public:
		[[nodiscard]] int GetCurSel()const {
			assert(IsWindow()); return static_cast<int>(SendMsg(TCM_GETCURSEL, 0, 0L));
		}
		[[nodiscard]] auto GetItemRect(int iItem)const -> CRect {
			assert(IsWindow());
			RECT rc { }; SendMsg(TCM_GETITEMRECT, iItem, reinterpret_cast<LPARAM>(&rc));
			return rc;
		}
		auto InsertItem(int iItem, TCITEMW* pItem)const -> LONG {
			assert(IsWindow());
			return static_cast<LONG>(SendMsg(TCM_INSERTITEMW, iItem, reinterpret_cast<LPARAM>(pItem)));
		}
		auto InsertItem(int iItem, LPCWSTR pwszStr)const -> LONG {
			TCITEMW tci { .mask { TCIF_TEXT }, .pszText { const_cast<LPWSTR>(pwszStr) } };
			return InsertItem(iItem, &tci);
		}
		int SetCurSel(int iItem)const {
			assert(IsWindow()); return static_cast<int>(SendMsg(TCM_SETCURSEL, iItem, 0L));
		}
	};

	class CMenu final {
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
		void SetItemInfo(UINT uItem, LPCMENUITEMINFOW pMII, bool fByID = true)const {
			assert(IsMenu()); ::SetMenuItemInfoW(m_hMenu, uItem, !fByID, pMII);
		}
		void SetItemInfo(UINT uItem, const MENUITEMINFOW& mii, bool fByID = true)const {
			SetItemInfo(uItem, &mii, fByID);
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

namespace HEXCTRL::INTERNAL::simd {
#if defined(_M_IX86) || defined(_M_X64)
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

	template<typename T> concept TVec128 = (std::is_same_v<T, __m128> || std::is_same_v<T, __m128i> || std::is_same_v<T, __m128d>);
	template<typename T> concept TVec256 = (std::is_same_v<T, __m256> || std::is_same_v<T, __m256i> || std::is_same_v<T, __m256d>);

	template<ut::TSize1248 TIntegral, TVec128 TVec>	//Bytes swap inside vector types: __m128, __m128i, __m128d.
	[[nodiscard]] auto __vectorcall ByteSwapVec(const TVec m128T) -> TVec
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
			for (int i = 0; i < std::size(flData); ++i) {
				flData[i] = ut::ByteSwap(flData[i]);
			}
			return _mm_load_ps(flData); //Returning local array as __m128.
		}
		else if constexpr (std::is_same_v<TVec, __m128d>) { //Doubles.
			alignas(16) double dblData[2];
			_mm_store_pd(dblData, m128T); //Loading m128T into local double array.
			for (int i = 0; i < std::size(dblData); ++i) {
				dblData[i] = ut::ByteSwap(dblData[i]);
			}
			return _mm_load_pd(dblData); //Returning local array as __m128d.
		}
	}

	template<ut::TSize1248 TIntegral, TVec256 TVec>	//Bytes swap inside vector types: __m256, __m256i, __m256d.
	[[nodiscard]] auto __vectorcall ByteSwapVec(const TVec m256T) -> TVec
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
			for (int i = 0; i < std::size(flData); ++i) {
				flData[i] = ut::ByteSwap(flData[i]);
			}
			return _mm256_load_ps(flData); //Returning local array as __m256.
		}
		else if constexpr (std::is_same_v<TVec, __m256d>) { //Doubles.
			alignas(32) double dblData[4];
			_mm256_store_pd(dblData, m256T); //Loading m256T into local double array.
			for (int i = 0; i < std::size(dblData); ++i) {
				dblData[i] = ut::ByteSwap(dblData[i]);
			}
			return _mm256_load_pd(dblData); //Returning local array as __m256d.
		}
	}
#endif // ^^^ _M_IX86 || _M_X64

	enum class EVecType : std::uint8_t {
		VECTOR_128 = 16U, //SSE4.2, ARM64NEON.
		VECTOR_256 = 32U  //AVX2.
	};

	[[nodiscard]] auto GetVectorType() -> EVecType {
		using enum EVecType;
	#if defined(_M_IX86) || defined(_M_X64)
		return HasAVX2() ? VECTOR_256 : VECTOR_128;
	#elif defined(_M_ARM64) //^^^ _M_IX86 || _M_X64 / vvv _M_ARM64
		return VECTOR_128;
	#endif //^^^ _M_ARM64
	}

	[[nodiscard]] constexpr auto VecTypeToSize(EVecType eVecType) -> std::uint32_t {
		using enum EVecType;
		switch (eVecType) {
		case VECTOR_128: return 16U;
		case VECTOR_256: return 32U;
		default:
			return 0U;
		};
	}

//MemCmp*.
#if defined(_M_IX86) || defined(_M_X64)
	template<EVecType eVecType, bool fEqual = true>
	[[nodiscard]] __forceinline auto MemCmpEQ1(const std::byte* pWhere, std::uint8_t u8What)noexcept -> std::uint64_t {
		if constexpr (eVecType == EVecType::VECTOR_128) {
			const auto m128iWhere = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pWhere));
			const auto m128iWhat = _mm_set1_epi8(static_cast<char>(u8What));
			const auto m128iResult = _mm_cmpeq_epi8(m128iWhere, m128iWhat);
			std::uint32_t u32Mask = _mm_movemask_epi8(m128iResult);

			if constexpr (!fEqual) {
				u32Mask ^= 0xFFFF;
			}
			if (u32Mask == 0) {
				return 0xFFFFFFFFU;
			}

			return std::countr_zero(u32Mask);
		}
		else if constexpr (eVecType == EVecType::VECTOR_256) {
			const auto m256iWhere = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pWhere));
			const auto m256iWhat = _mm256_set1_epi8(static_cast<char>(u8What));
			const auto m256iResult = _mm256_cmpeq_epi8(m256iWhere, m256iWhat);
			std::uint32_t u32Mask = _mm256_movemask_epi8(m256iResult);

			if constexpr (!fEqual) { u32Mask ^= 0xFFFFFFFFU; }
			if (u32Mask == 0) {
				return 0xFFFFFFFFU;
			}

			return std::countr_zero(u32Mask);
		}
	}

	template<EVecType eVecType, bool fEqual = true>
	[[nodiscard]] __forceinline auto MemCmpEQ2(const std::byte* pWhere, std::uint16_t u16What)noexcept -> std::uint64_t {
		if constexpr (eVecType == EVecType::VECTOR_128) {
			//Bytes:  0x01020304050607080910111213141516
			const auto m128iWhere0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pWhere));
			//Become: 0x02030405060708091011121314151600 after right-shift the vector.
			const auto m128iWhere1 = _mm_srli_si128(m128iWhere0, 1); //Shifting-right the whole vector by 1 byte.
			const auto m128iWhat = _mm_set1_epi16(u16What);
			const auto m128iResult0 = _mm_cmpeq_epi16(m128iWhere0, m128iWhat);
			const auto m128iResult1 = _mm_cmpeq_epi16(m128iWhere1, m128iWhat); //Comparing both loads simultaneously.
			std::uint32_t u32Mask0 = _mm_movemask_epi8(m128iResult0);
			//Shifting-left the mask by 1 to compensate previous 1 byte right-shift by _mm_srli_si128.
			std::uint32_t u32Mask1 = _mm_movemask_epi8(m128iResult1) << 1;

			//Inverting only the first 16 bits, the mask size (16 * 8 = 128vec).
			if constexpr (!fEqual) {
				u32Mask0 ^= 0xFFFFU;
				u32Mask1 ^= 0xFFFFU;
			}

			//Setting the zero bit to 0, because it's always 0 after left-shifting the mask above.
			//Setting the last bit to 0, because two last bytes don't participate in the search, in a right-shifted vector.
			//We've already left-shifted the mask, so zeroing remaining bit.

			u32Mask1 &= 0b01111111'11111110;

			if (u32Mask0 == 0 && u32Mask1 == 0) {
				return 0xFFFFFFFFU;
			}

			const auto iRes0 = std::countr_zero(u32Mask0);
			const auto iRes1 = std::countr_zero(u32Mask1);
			return (std::min)(iRes0, iRes1); //>15 here means not found, all in mask are zeros.
		}
		else if constexpr (eVecType == EVecType::VECTOR_256) {
			const auto m256iWhere0 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pWhere));
			const auto m256iWhere1 = _mm256_srli_si256(m256iWhere0, 1);
			const auto m256iWhat = _mm256_set1_epi16(u16What);
			const auto m256iResult0 = _mm256_cmpeq_epi16(m256iWhere0, m256iWhat);
			const auto m256iResult1 = _mm256_cmpeq_epi16(m256iWhere1, m256iWhat);
			std::uint32_t u32Mask0 = static_cast<std::uint32_t>(_mm256_movemask_epi8(m256iResult0));
			std::uint32_t u32Mask1 = static_cast<std::uint32_t>(_mm256_movemask_epi8(m256iResult1)) << 1;

			//Inverting all 32 bits (32 * 8 = 256vec).
			if constexpr (!fEqual) {
				u32Mask0 ^= 0xFFFFFFFFU;
				u32Mask1 ^= 0xFFFFFFFFU;
			}

			u32Mask1 &= 0b01111111'11111111'11111111'11111110;

			if (u32Mask0 == 0 && u32Mask1 == 0) {
				return 0xFFFFFFFFU;
			}

			const auto iRes0 = std::countr_zero(u32Mask0);
			const auto iRes1 = std::countr_zero(u32Mask1);
			return (std::min)(iRes0, iRes1); //>31 here means not found, all in mask are zeros.
		}
	}

	template<EVecType eVecType, bool fEqual = true>
	[[nodiscard]] __forceinline auto MemCmpEQ4(const std::byte* pWhere, std::uint32_t u32What)noexcept -> std::uint64_t {
		if constexpr (eVecType == EVecType::VECTOR_128) {
			const auto m128iWhere0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pWhere));
			const auto m128iWhere1 = _mm_srli_si128(m128iWhere0, 1);
			const auto m128iWhere2 = _mm_srli_si128(m128iWhere0, 2);
			const auto m128iWhere3 = _mm_srli_si128(m128iWhere0, 3);
			const auto m128iWhat = _mm_set1_epi32(u32What);
			const auto m128iResult0 = _mm_cmpeq_epi32(m128iWhere0, m128iWhat);
			const auto m128iResult1 = _mm_cmpeq_epi32(m128iWhere1, m128iWhat);
			const auto m128iResult2 = _mm_cmpeq_epi32(m128iWhere2, m128iWhat);
			const auto m128iResult3 = _mm_cmpeq_epi32(m128iWhere3, m128iWhat);
			std::uint32_t u32Mask0 = _mm_movemask_epi8(m128iResult0);
			std::uint32_t u32Mask1 = _mm_movemask_epi8(m128iResult1) << 1;
			std::uint32_t u32Mask2 = _mm_movemask_epi8(m128iResult2) << 2;
			std::uint32_t u32Mask3 = _mm_movemask_epi8(m128iResult3) << 3;

			if constexpr (!fEqual) {
				u32Mask0 ^= 0xFFFFU;
				u32Mask1 ^= 0xFFFFU;
				u32Mask2 ^= 0xFFFFU;
				u32Mask3 ^= 0xFFFFU;
			}

			//Setting the first bits in the masks to 0, because they're always 0 after left-shifting the masks above.
			//Setting the last bits to 0, because four last bytes in the right-shifted vector don't participate in the search.
			//We've already left-shifted the mask, so zeroing remaining bits.

			u32Mask1 &= 0b00011111'11111110;
			u32Mask2 &= 0b00111111'11111100;
			u32Mask3 &= 0b01111111'11111000;

			if (u32Mask0 == 0 && u32Mask1 == 0 && u32Mask2 == 0 && u32Mask3 == 0) {
				return 0xFFFFFFFFU;
			}

			const auto iRes0 = std::countr_zero(u32Mask0);
			const auto iRes1 = std::countr_zero(u32Mask1);
			const auto iRes2 = std::countr_zero(u32Mask2);
			const auto iRes3 = std::countr_zero(u32Mask3);
			return (std::min)(iRes0, (std::min)(iRes1, (std::min)(iRes2, iRes3)));
		}
		else if constexpr (eVecType == EVecType::VECTOR_256) {
			const auto m256iWhere0 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pWhere));
			const auto m256iWhere1 = _mm256_srli_si256(m256iWhere0, 1);
			const auto m256iWhere2 = _mm256_srli_si256(m256iWhere0, 2);
			const auto m256iWhere3 = _mm256_srli_si256(m256iWhere0, 3);
			const auto m256iWhat = _mm256_set1_epi32(u32What);
			const auto m256iResult0 = _mm256_cmpeq_epi32(m256iWhere0, m256iWhat);
			const auto m256iResult1 = _mm256_cmpeq_epi32(m256iWhere1, m256iWhat);
			const auto m256iResult2 = _mm256_cmpeq_epi32(m256iWhere2, m256iWhat);
			const auto m256iResult3 = _mm256_cmpeq_epi32(m256iWhere3, m256iWhat);
			std::uint32_t u32Mask0 = _mm256_movemask_epi8(m256iResult0);
			std::uint32_t u32Mask1 = _mm256_movemask_epi8(m256iResult1) << 1;
			std::uint32_t u32Mask2 = _mm256_movemask_epi8(m256iResult2) << 2;
			std::uint32_t u32Mask3 = _mm256_movemask_epi8(m256iResult3) << 3;

			if constexpr (!fEqual) {
				u32Mask0 ^= 0xFFFFFFFFU;
				u32Mask1 ^= 0xFFFFFFFFU;
				u32Mask2 ^= 0xFFFFFFFFU;
				u32Mask3 ^= 0xFFFFFFFFU;
			}

			u32Mask1 &= 0b00011111'11111111'11111111'11111110;
			u32Mask2 &= 0b00111111'11111111'11111111'11111100;
			u32Mask3 &= 0b01111111'11111111'11111111'11111000;

			if (u32Mask0 == 0 && u32Mask1 == 0 && u32Mask2 == 0 && u32Mask3 == 0) {
				return 0xFFFFFFFFU;
			}

			const auto iRes0 = std::countr_zero(u32Mask0);
			const auto iRes1 = std::countr_zero(u32Mask1);
			const auto iRes2 = std::countr_zero(u32Mask2);
			const auto iRes3 = std::countr_zero(u32Mask3);
			return (std::min)(iRes0, (std::min)(iRes1, (std::min)(iRes2, iRes3)));
		}
	}
#elif defined(_M_ARM64) //^^^ _M_IX86 || _M_X64 / vvv _M_ARM64
	//Convert __n128 mask of 16 8-bit values into 16 4-bit values.
	[[nodiscard]] auto GetMaskU8(__n128 n128i)noexcept -> std::uint64_t {
		const auto n64Res = vshrn_n_u16(vreinterpretq_u16_u8(n128i), 4);
		return vget_lane_u64(vreinterpret_u64_u8(n64Res), 0);
	}

	//Convert __n128 mask of 8 16-bit values into 8 8-bit values.
	[[nodiscard]] auto GetMaskU16(__n128 n128i)noexcept -> std::uint64_t {
		const auto n64Res = vshrn_n_u32(vreinterpretq_u32_u16(n128i), 8);
		return vget_lane_u64(vreinterpret_u64_u16(n64Res), 0);
	}

	//Convert __n128 mask of 4 32-bit values into 4 16-bit values.
	[[nodiscard]] auto GetMaskU32(__n128 n128i)noexcept -> std::uint64_t {
		const auto n64Res = vshrn_n_u64(vreinterpretq_u64_u32(n128i), 16);
		return vget_lane_u64(vreinterpret_u64_u32(n64Res), 0);
	}

	template<EVecType eVecType, bool fEqual = true>
	[[nodiscard]] __forceinline auto MemCmpEQ1(const std::byte* pWhere, std::uint8_t u8What)noexcept -> std::uint64_t {
		for (auto i = 0U; i < 16U; ++i) { //16 is a number of distinct uint8_t numbers in a 128-bit vector.
			const auto u8Data = *reinterpret_cast<const std::uint8_t*>(pWhere + i);
			if constexpr (fEqual) {
				if (u8Data == u8What) {
					return i;
				}
			}
			else {
				if (u8Data != u8What) {
					return i;
				}
			}
		}

		return 0xFFFFFFFFU;

	/*	const auto n128iWhere = vld1q_u8(pWhere);
		const auto n128iWhat = vdupq_n_u8(u8What);
		const auto n128iResult = vceqq_u8(n128iWhere, n128iWhat);
		std::uint64_t u64Mask = GetMaskU8(n128iResult);

		if constexpr (!fEqual) { u64Mask ^= 0xFFFFFFFFU; }
		if (u64Mask == 0) { return 0xFFFFFFFFU; }

		return _CountTrailingZeros64(u64Mask) >> 2;*/
	}

	template<EVecType eVecType, bool fEqual = true>
	[[nodiscard]] __forceinline auto MemCmpEQ2(const std::byte* pWhere, std::uint16_t u16What)noexcept -> std::uint64_t {
		for (auto i = 0U; i < 15U; ++i) { //15 is a number of distinct uint16_t numbers in a 128-bit vector.
			const auto u16Data = *reinterpret_cast<const std::uint16_t*>(pWhere + i);
			if constexpr (fEqual) {
				if (u16Data == u16What) {
					return i;
				}
			}
			else {
				if (u16Data != u16What) {
					return i;
				}
			}
		}

		return 0xFFFFFFFFU;
	}

	template<EVecType eVecType, bool fEqual = true>
	[[nodiscard]] __forceinline auto MemCmpEQ4(const std::byte* pWhere, std::uint32_t u32What)noexcept -> std::uint64_t {
		for (auto i = 0U; i < 13U; ++i) { //13 is a number of distinct uint32_t numbers in a 128-bit vector.
			const auto u32Data = *reinterpret_cast<const std::uint32_t*>(pWhere + i);
			if constexpr (fEqual) {
				if (u32Data == u32What) {
					return i;
				}
			}
			else {
				if (u32Data != u32What) {
					return i;
				}
			}
		}

		return 0xFFFFFFFFU;
	}
#endif //^^^ _M_ARM64

//ModifyOperVec.
#if defined(_M_IX86) || defined(_M_X64)
	void OperVec128_i8(std::byte* pData, const HEXMODIFY& hms) {
		auto m128iData = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pData));
		alignas(16) std::int8_t i8Data[16];
		_mm_store_si128(reinterpret_cast<__m128i*>(i8Data), m128iData);
		assert(!hms.spnData.empty());
		const auto i8Oper = *reinterpret_cast<const std::int8_t*>(hms.spnData.data());
		const auto m128iOper = _mm_set1_epi8(i8Oper);

		using enum EHexOperMode;
		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m128iData = _mm_add_epi8(m128iData, m128iOper);
			break;
		case OPER_SUB:
			m128iData = _mm_sub_epi8(m128iData, m128iOper);
			break;
		case OPER_MUL:
		{
			const auto m128iEven = _mm_mullo_epi16(m128iData, m128iOper);
			const auto m128iOdd = _mm_mullo_epi16(_mm_srli_epi16(m128iData, 8), _mm_srli_epi16(m128iOper, 8));
			m128iData = _mm_or_si128(_mm_slli_epi16(m128iOdd, 8), _mm_srli_epi16(_mm_slli_epi16(m128iEven, 8), 8));
		}
		break;
		case OPER_DIV:
			assert(i8Oper > 0);
			m128iData = _mm_setr_epi8(i8Data[0] / i8Oper, i8Data[1] / i8Oper, i8Data[2] / i8Oper, i8Data[3] / i8Oper,
				i8Data[4] / i8Oper, i8Data[5] / i8Oper, i8Data[6] / i8Oper, i8Data[7] / i8Oper,
				i8Data[8] / i8Oper, i8Data[9] / i8Oper, i8Data[10] / i8Oper, i8Data[11] / i8Oper,
				i8Data[12] / i8Oper, i8Data[13] / i8Oper, i8Data[14] / i8Oper, i8Data[15] / i8Oper);
			break;
		case OPER_MIN:
			m128iData = _mm_max_epi8(m128iData, m128iOper); //SSE4.1
			break;
		case OPER_MAX:
			m128iData = _mm_min_epi8(m128iData, m128iOper); //SSE4.1
			break;
		case OPER_SWAP: //No need for the int8_t.
			break;
		case OPER_OR:
			m128iData = _mm_or_si128(m128iData, m128iOper);
			break;
		case OPER_XOR:
			m128iData = _mm_xor_si128(m128iData, m128iOper);
			break;
		case OPER_AND:
			m128iData = _mm_and_si128(m128iData, m128iOper);
			break;
		case OPER_NOT:
			//_mm_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
			m128iData = _mm_xor_si128(m128iData, _mm_cmpeq_epi64(m128iData, m128iData)); //SSE4.1
			break;
		case OPER_SHL:
			m128iData = _mm_setr_epi8(i8Data[0] << i8Oper, i8Data[1] << i8Oper, i8Data[2] << i8Oper, i8Data[3] << i8Oper,
				i8Data[4] << i8Oper, i8Data[5] << i8Oper, i8Data[6] << i8Oper, i8Data[7] << i8Oper,
				i8Data[8] << i8Oper, i8Data[9] << i8Oper, i8Data[10] << i8Oper, i8Data[11] << i8Oper,
				i8Data[12] << i8Oper, i8Data[13] << i8Oper, i8Data[14] << i8Oper, i8Data[15] << i8Oper);
			break;
		case OPER_SHR:
			m128iData = _mm_setr_epi8(i8Data[0] >> i8Oper, i8Data[1] >> i8Oper, i8Data[2] >> i8Oper, i8Data[3] >> i8Oper,
				i8Data[4] >> i8Oper, i8Data[5] >> i8Oper, i8Data[6] >> i8Oper, i8Data[7] >> i8Oper,
				i8Data[8] >> i8Oper, i8Data[9] >> i8Oper, i8Data[10] >> i8Oper, i8Data[11] >> i8Oper,
				i8Data[12] >> i8Oper, i8Data[13] >> i8Oper, i8Data[14] >> i8Oper, i8Data[15] >> i8Oper);
			break;
		case OPER_ROTL:
			m128iData = _mm_setr_epi8(std::rotl(static_cast<std::uint8_t>(i8Data[0]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[1]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[2]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[3]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[4]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[5]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[6]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[7]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[8]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[9]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[10]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[11]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[12]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[13]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[14]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[15]), i8Oper));
			break;
		case OPER_ROTR:
			m128iData = _mm_setr_epi8(std::rotr(static_cast<std::uint8_t>(i8Data[0]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[1]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[2]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[3]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[4]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[5]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[6]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[7]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[8]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[9]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[10]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[11]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[12]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[13]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[14]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[15]), i8Oper));
			break;
		case OPER_BITREV:
			m128iData = _mm_setr_epi8(ut::BitReverse(i8Data[0]), ut::BitReverse(i8Data[1]), ut::BitReverse(i8Data[2]),
				ut::BitReverse(i8Data[3]), ut::BitReverse(i8Data[4]), ut::BitReverse(i8Data[5]), ut::BitReverse(i8Data[6]),
				ut::BitReverse(i8Data[7]), ut::BitReverse(i8Data[8]), ut::BitReverse(i8Data[9]), ut::BitReverse(i8Data[10]),
				ut::BitReverse(i8Data[11]), ut::BitReverse(i8Data[12]), ut::BitReverse(i8Data[13]), ut::BitReverse(i8Data[14]),
				ut::BitReverse(i8Data[15]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported int8_t operation.");
			break;
		}

		_mm_storeu_si128(reinterpret_cast<__m128i*>(pData), m128iData);
	}

	void OperVec128_u8(std::byte* pData, const HEXMODIFY& hms) {
		auto m128iData = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pData));
		alignas(16) std::uint8_t u8Data[16];
		_mm_store_si128(reinterpret_cast<__m128i*>(u8Data), m128iData);
		assert(!hms.spnData.empty());
		const auto u8Oper = *reinterpret_cast<const std::uint8_t*>(hms.spnData.data());
		const auto m128iOper = _mm_set1_epi8(u8Oper);

		using enum EHexOperMode;
		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m128iData = _mm_add_epi8(m128iData, m128iOper);
			break;
		case OPER_SUB:
			m128iData = _mm_sub_epi8(m128iData, m128iOper);
			break;
		case OPER_MUL:
			m128iData = _mm_setr_epi8(u8Data[0] * u8Oper, u8Data[1] * u8Oper, u8Data[2] * u8Oper,
				u8Data[3] * u8Oper, u8Data[4] * u8Oper, u8Data[5] * u8Oper, u8Data[6] * u8Oper,
				u8Data[7] * u8Oper, u8Data[8] * u8Oper, u8Data[9] * u8Oper, u8Data[10] * u8Oper,
				u8Data[11] * u8Oper, u8Data[12] * u8Oper, u8Data[13] * u8Oper, u8Data[14] * u8Oper,
				u8Data[15] * u8Oper);
			break;
		case OPER_DIV:
			assert(u8Oper > 0);
			m128iData = _mm_setr_epi8(u8Data[0] / u8Oper, u8Data[1] / u8Oper, u8Data[2] / u8Oper,
				u8Data[3] / u8Oper, u8Data[4] / u8Oper, u8Data[5] / u8Oper, u8Data[6] / u8Oper,
				u8Data[7] / u8Oper, u8Data[8] / u8Oper, u8Data[9] / u8Oper, u8Data[10] / u8Oper,
				u8Data[11] / u8Oper, u8Data[12] / u8Oper, u8Data[13] / u8Oper, u8Data[14] / u8Oper,
				u8Data[15] / u8Oper);
			break;
		case OPER_MIN:
			m128iData = _mm_max_epu8(m128iData, m128iOper); //SSE4.1
			break;
		case OPER_MAX:
			m128iData = _mm_min_epu8(m128iData, m128iOper); //SSE4.1
			break;
		case OPER_SWAP:	//No need for the uint8_t.
			break;
		case OPER_OR:
			m128iData = _mm_or_si128(m128iData, m128iOper);
			break;
		case OPER_XOR:
			m128iData = _mm_xor_si128(m128iData, m128iOper);
			break;
		case OPER_AND:
			m128iData = _mm_and_si128(m128iData, m128iOper);
			break;
		case OPER_NOT:
			//_mm_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
			m128iData = _mm_xor_si128(m128iData, _mm_cmpeq_epi64(m128iData, m128iData)); //SSE4.1
			break;
		case OPER_SHL:
			m128iData = _mm_setr_epi8(u8Data[0] << u8Oper, u8Data[1] << u8Oper, u8Data[2] << u8Oper, u8Data[3] << u8Oper,
				u8Data[4] << u8Oper, u8Data[5] << u8Oper, u8Data[6] << u8Oper, u8Data[7] << u8Oper,
				u8Data[8] << u8Oper, u8Data[9] << u8Oper, u8Data[10] << u8Oper, u8Data[11] << u8Oper,
				u8Data[12] << u8Oper, u8Data[13] << u8Oper, u8Data[14] << u8Oper, u8Data[15] << u8Oper);
			break;
		case OPER_SHR:
			m128iData = _mm_setr_epi8(u8Data[0] >> u8Oper, u8Data[1] >> u8Oper, u8Data[2] >> u8Oper, u8Data[3] >> u8Oper,
				u8Data[4] >> u8Oper, u8Data[5] >> u8Oper, u8Data[6] >> u8Oper, u8Data[7] >> u8Oper,
				u8Data[8] >> u8Oper, u8Data[9] >> u8Oper, u8Data[10] >> u8Oper, u8Data[11] >> u8Oper,
				u8Data[12] >> u8Oper, u8Data[13] >> u8Oper, u8Data[14] >> u8Oper, u8Data[15] >> u8Oper);
			break;
		case OPER_ROTL:
			m128iData = _mm_setr_epi8(std::rotl(u8Data[0], u8Oper), std::rotl(u8Data[1], u8Oper),
				std::rotl(u8Data[2], u8Oper), std::rotl(u8Data[3], u8Oper), std::rotl(u8Data[4], u8Oper),
				std::rotl(u8Data[5], u8Oper), std::rotl(u8Data[6], u8Oper), std::rotl(u8Data[7], u8Oper),
				std::rotl(u8Data[8], u8Oper), std::rotl(u8Data[9], u8Oper), std::rotl(u8Data[10], u8Oper),
				std::rotl(u8Data[11], u8Oper), std::rotl(u8Data[12], u8Oper), std::rotl(u8Data[13], u8Oper),
				std::rotl(u8Data[14], u8Oper), std::rotl(u8Data[15], u8Oper));
			break;
		case OPER_ROTR:
			m128iData = _mm_setr_epi8(std::rotr(u8Data[0], u8Oper), std::rotr(u8Data[1], u8Oper),
				std::rotr(u8Data[2], u8Oper), std::rotr(u8Data[3], u8Oper), std::rotr(u8Data[4], u8Oper),
				std::rotr(u8Data[5], u8Oper), std::rotr(u8Data[6], u8Oper), std::rotr(u8Data[7], u8Oper),
				std::rotr(u8Data[8], u8Oper), std::rotr(u8Data[9], u8Oper), std::rotr(u8Data[10], u8Oper),
				std::rotr(u8Data[11], u8Oper), std::rotr(u8Data[12], u8Oper), std::rotr(u8Data[13], u8Oper),
				std::rotr(u8Data[14], u8Oper), std::rotr(u8Data[15], u8Oper));
			break;
		case OPER_BITREV:
			m128iData = _mm_setr_epi8(ut::BitReverse(u8Data[0]), ut::BitReverse(u8Data[1]), ut::BitReverse(u8Data[2]),
				ut::BitReverse(u8Data[3]), ut::BitReverse(u8Data[4]), ut::BitReverse(u8Data[5]), ut::BitReverse(u8Data[6]),
				ut::BitReverse(u8Data[7]), ut::BitReverse(u8Data[8]), ut::BitReverse(u8Data[9]), ut::BitReverse(u8Data[10]),
				ut::BitReverse(u8Data[11]), ut::BitReverse(u8Data[12]), ut::BitReverse(u8Data[13]), ut::BitReverse(u8Data[14]),
				ut::BitReverse(u8Data[15]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported uint8_t operation.");
			break;
		}

		_mm_storeu_si128(reinterpret_cast<__m128i*>(pData), m128iData);
	}

	void OperVec128_i16(std::byte* pData, const HEXMODIFY& hms) {
		auto m128iData = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pData));
		if (hms.fBigEndian) {
			m128iData = ByteSwapVec<std::int16_t>(_mm_loadu_si128(reinterpret_cast<const __m128i*>(pData)));
		}

		alignas(16) std::int16_t i16Data[8];
		_mm_store_si128(reinterpret_cast<__m128i*>(i16Data), m128iData);
		assert(!hms.spnData.empty());
		const auto i16Oper = *reinterpret_cast<const std::int16_t*>(hms.spnData.data());
		const auto m128iOper = _mm_set1_epi16(i16Oper);

		using enum EHexOperMode;
		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m128iData = _mm_add_epi16(m128iData, m128iOper);
			break;
		case OPER_SUB:
			m128iData = _mm_sub_epi16(m128iData, m128iOper);
			break;
		case OPER_MUL:
			m128iData = _mm_mullo_epi16(m128iData, m128iOper);
			break;
		case OPER_DIV:
			assert(i16Oper > 0);
			m128iData = _mm_setr_epi16(i16Data[0] / i16Oper, i16Data[1] / i16Oper, i16Data[2] / i16Oper,
				i16Data[3] / i16Oper, i16Data[4] / i16Oper, i16Data[5] / i16Oper, i16Data[6] / i16Oper, i16Data[7] / i16Oper);
			break;
		case OPER_MIN:
			m128iData = _mm_max_epi16(m128iData, m128iOper);
			break;
		case OPER_MAX:
			m128iData = _mm_min_epi16(m128iData, m128iOper);
			break;
		case OPER_SWAP:
			m128iData = ByteSwapVec<std::int16_t>(m128iData);
			break;
		case OPER_OR:
			m128iData = _mm_or_si128(m128iData, m128iOper);
			break;
		case OPER_XOR:
			m128iData = _mm_xor_si128(m128iData, m128iOper);
			break;
		case OPER_AND:
			m128iData = _mm_and_si128(m128iData, m128iOper);
			break;
		case OPER_NOT:
			//_mm_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
			m128iData = _mm_xor_si128(m128iData, _mm_cmpeq_epi64(m128iData, m128iData)); //SSE4.1
			break;
		case OPER_SHL:
			m128iData = _mm_slli_epi16(m128iData, i16Oper);
			break;
		case OPER_SHR:
			m128iData = _mm_srai_epi16(m128iData, i16Oper); //Arithmetic shift.
			break;
		case OPER_ROTL:
			m128iData = _mm_setr_epi16(std::rotl(static_cast<std::uint16_t>(i16Data[0]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[1]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[2]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[3]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[4]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[5]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[6]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[7]), i16Oper));
			break;
		case OPER_ROTR:
			m128iData = _mm_setr_epi16(std::rotr(static_cast<std::uint16_t>(i16Data[0]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[1]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[2]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[3]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[4]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[5]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[6]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[7]), i16Oper));
			break;
		case OPER_BITREV:
			m128iData = _mm_setr_epi16(ut::BitReverse(i16Data[0]), ut::BitReverse(i16Data[1]), ut::BitReverse(i16Data[2]),
				ut::BitReverse(i16Data[3]), ut::BitReverse(i16Data[4]), ut::BitReverse(i16Data[5]), ut::BitReverse(i16Data[6]),
				ut::BitReverse(i16Data[7]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported int16_t operation.");
			break;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m128iData = ByteSwapVec<std::int16_t>(m128iData);
		}

		_mm_storeu_si128(reinterpret_cast<__m128i*>(pData), m128iData);
	}

	void OperVec128_u16(std::byte* pData, const HEXMODIFY& hms) {
		auto m128iData = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pData));
		if (hms.fBigEndian) {
			m128iData = ByteSwapVec<std::uint16_t>(_mm_loadu_si128(reinterpret_cast<const __m128i*>(pData)));
		}

		alignas(16) std::uint16_t u16Data[8];
		_mm_store_si128(reinterpret_cast<__m128i*>(u16Data), m128iData);
		assert(!hms.spnData.empty());
		const auto u16Oper = *reinterpret_cast<const std::uint16_t*>(hms.spnData.data());
		const auto m128iOper = _mm_set1_epi16(u16Oper);

		using enum EHexOperMode;
		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m128iData = _mm_add_epi16(m128iData, m128iOper);
			break;
		case OPER_SUB:
			m128iData = _mm_sub_epi16(m128iData, m128iOper);
			break;
		case OPER_MUL:
			m128iData = _mm_setr_epi16(u16Data[0] * u16Oper, u16Data[1] * u16Oper, u16Data[2] * u16Oper,
				u16Data[3] * u16Oper, u16Data[4] * u16Oper, u16Data[5] * u16Oper, u16Data[6] * u16Oper,
				u16Data[7] * u16Oper);
			break;
		case OPER_DIV:
			assert(u16Oper > 0);
			m128iData = _mm_setr_epi16(u16Data[0] / u16Oper, u16Data[1] / u16Oper, u16Data[2] / u16Oper,
				u16Data[3] / u16Oper, u16Data[4] / u16Oper, u16Data[5] / u16Oper, u16Data[6] / u16Oper,
				u16Data[7] / u16Oper);
			break;
		case OPER_MIN:
			m128iData = _mm_max_epu16(m128iData, m128iOper);
			break;
		case OPER_MAX:
			m128iData = _mm_min_epu16(m128iData, m128iOper);
			break;
		case OPER_SWAP:
			m128iData = ByteSwapVec<std::uint16_t>(m128iData);
			break;
		case OPER_OR:
			m128iData = _mm_or_si128(m128iData, m128iOper);
			break;
		case OPER_XOR:
			m128iData = _mm_xor_si128(m128iData, m128iOper);
			break;
		case OPER_AND:
			m128iData = _mm_and_si128(m128iData, m128iOper);
			break;
		case OPER_NOT:
			//_mm_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
			m128iData = _mm_xor_si128(m128iData, _mm_cmpeq_epi64(m128iData, m128iData)); //SSE4.1
			break;
		case OPER_SHL:
			m128iData = _mm_slli_epi16(m128iData, u16Oper);
			break;
		case OPER_SHR:
			m128iData = _mm_srli_epi16(m128iData, u16Oper); //Logical shift.
			break;
		case OPER_ROTL:
			m128iData = _mm_setr_epi16(std::rotl(u16Data[0], u16Oper), std::rotl(u16Data[1], u16Oper),
				std::rotl(u16Data[2], u16Oper), std::rotl(u16Data[3], u16Oper), std::rotl(u16Data[4], u16Oper),
				std::rotl(u16Data[5], u16Oper), std::rotl(u16Data[6], u16Oper), std::rotl(u16Data[7], u16Oper));
			break;
		case OPER_ROTR:
			m128iData = _mm_setr_epi16(std::rotr(u16Data[0], u16Oper), std::rotr(u16Data[1], u16Oper),
				std::rotr(u16Data[2], u16Oper), std::rotr(u16Data[3], u16Oper), std::rotr(u16Data[4], u16Oper),
				std::rotr(u16Data[5], u16Oper), std::rotr(u16Data[6], u16Oper), std::rotr(u16Data[7], u16Oper));
			break;
		case OPER_BITREV:
			m128iData = _mm_setr_epi16(ut::BitReverse(u16Data[0]), ut::BitReverse(u16Data[1]), ut::BitReverse(u16Data[2]),
				ut::BitReverse(u16Data[3]), ut::BitReverse(u16Data[4]), ut::BitReverse(u16Data[5]), ut::BitReverse(u16Data[6]),
				ut::BitReverse(u16Data[7]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported uint16_t operation.");
			break;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m128iData = ByteSwapVec<std::uint16_t>(m128iData);
		}

		_mm_storeu_si128(reinterpret_cast<__m128i*>(pData), m128iData);
	}

	void OperVec128_i32(std::byte* pData, const HEXMODIFY& hms) {
		auto m128iData = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pData));
		if (hms.fBigEndian) {
			m128iData = ByteSwapVec<std::int32_t>(_mm_loadu_si128(reinterpret_cast<const __m128i*>(pData)));
		}

		alignas(16) std::int32_t i32Data[4];
		_mm_store_si128(reinterpret_cast<__m128i*>(i32Data), m128iData);
		assert(!hms.spnData.empty());
		const auto i32Oper = *reinterpret_cast<const std::int32_t*>(hms.spnData.data());
		const auto m128iOper = _mm_set1_epi32(i32Oper);

		using enum EHexOperMode;
		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m128iData = _mm_add_epi32(m128iData, m128iOper);
			break;
		case OPER_SUB:
			m128iData = _mm_sub_epi32(m128iData, m128iOper);
			break;
		case OPER_MUL:
			m128iData = _mm_mullo_epi32(m128iData, m128iOper);
			break;
		case OPER_DIV:
			assert(i32Oper > 0);
			m128iData = _mm_setr_epi32(i32Data[0] / i32Oper, i32Data[1] / i32Oper, i32Data[2] / i32Oper,
				i32Data[3] / i32Oper);
			break;
		case OPER_MIN:
			m128iData = _mm_max_epi32(m128iData, m128iOper);
			break;
		case OPER_MAX:
			m128iData = _mm_min_epi32(m128iData, m128iOper);
			break;
		case OPER_SWAP:
			m128iData = ByteSwapVec<std::int32_t>(m128iData);
			break;
		case OPER_OR:
			m128iData = _mm_or_si128(m128iData, m128iOper);
			break;
		case OPER_XOR:
			m128iData = _mm_xor_si128(m128iData, m128iOper);
			break;
		case OPER_AND:
			m128iData = _mm_and_si128(m128iData, m128iOper);
			break;
		case OPER_NOT:
			//_mm_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
			m128iData = _mm_xor_si128(m128iData, _mm_cmpeq_epi64(m128iData, m128iData)); //SSE4.1
			break;
		case OPER_SHL:
			m128iData = _mm_slli_epi32(m128iData, i32Oper);
			break;
		case OPER_SHR:
			m128iData = _mm_srai_epi32(m128iData, i32Oper); //Arithmetic shift.
			break;
		case OPER_ROTL:
			m128iData = _mm_setr_epi32(std::rotl(static_cast<std::uint32_t>(i32Data[0]), i32Oper),
				std::rotl(static_cast<std::uint32_t>(i32Data[1]), i32Oper),
				std::rotl(static_cast<std::uint32_t>(i32Data[2]), i32Oper),
				std::rotl(static_cast<std::uint32_t>(i32Data[3]), i32Oper));
			break;
		case OPER_ROTR:
			m128iData = _mm_setr_epi32(std::rotr(static_cast<std::uint32_t>(i32Data[0]), i32Oper),
				std::rotr(static_cast<std::uint32_t>(i32Data[1]), i32Oper),
				std::rotr(static_cast<std::uint32_t>(i32Data[2]), i32Oper),
				std::rotr(static_cast<std::uint32_t>(i32Data[3]), i32Oper));
			break;
		case OPER_BITREV:
			m128iData = _mm_setr_epi32(ut::BitReverse(i32Data[0]), ut::BitReverse(i32Data[1]), ut::BitReverse(i32Data[2]),
				ut::BitReverse(i32Data[3]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported int32_t operation.");
			break;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m128iData = ByteSwapVec<std::int32_t>(m128iData);
		}

		_mm_storeu_si128(reinterpret_cast<__m128i*>(pData), m128iData);
	}

	void OperVec128_u32(std::byte* pData, const HEXMODIFY& hms) {
		auto m128iData = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pData));
		if (hms.fBigEndian) {
			m128iData = ByteSwapVec<std::uint32_t>(_mm_loadu_si128(reinterpret_cast<const __m128i*>(pData)));
		}

		alignas(16) std::uint32_t u32Data[4];
		_mm_store_si128(reinterpret_cast<__m128i*>(u32Data), m128iData);
		assert(!hms.spnData.empty());
		const auto u32Oper = *reinterpret_cast<const std::uint32_t*>(hms.spnData.data());
		const auto m128iOper = _mm_set1_epi32(u32Oper);

		using enum EHexOperMode;
		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m128iData = _mm_add_epi32(m128iData, m128iOper);
			break;
		case OPER_SUB:
			m128iData = _mm_sub_epi32(m128iData, m128iOper);
			break;
		case OPER_MUL:
			m128iData = _mm_setr_epi32(u32Data[0] * u32Oper, u32Data[1] * u32Oper, u32Data[2] * u32Oper,
				u32Data[3] * u32Oper);
			break;
		case OPER_DIV:
			assert(u32Oper > 0);
			m128iData = _mm_setr_epi32(u32Data[0] / u32Oper, u32Data[1] / u32Oper, u32Data[2] / u32Oper,
				u32Data[3] / u32Oper);
			break;
		case OPER_MIN:
			m128iData = _mm_max_epu32(m128iData, m128iOper); //SSE4.1
			break;
		case OPER_MAX:
			m128iData = _mm_min_epu32(m128iData, m128iOper); //SSE4.1
			break;
		case OPER_SWAP:
			m128iData = ByteSwapVec<std::uint32_t>(m128iData);
			break;
		case OPER_OR:
			m128iData = _mm_or_si128(m128iData, m128iOper);
			break;
		case OPER_XOR:
			m128iData = _mm_xor_si128(m128iData, m128iOper);
			break;
		case OPER_AND:
			m128iData = _mm_and_si128(m128iData, m128iOper);
			break;
		case OPER_NOT:
			//_mm_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
			m128iData = _mm_xor_si128(m128iData, _mm_cmpeq_epi64(m128iData, m128iData)); //SSE4.1
			break;
		case OPER_SHL:
			m128iData = _mm_slli_epi32(m128iData, u32Oper);
			break;
		case OPER_SHR:
			m128iData = _mm_srli_epi32(m128iData, u32Oper); //Logical shift.
			break;
		case OPER_ROTL:
			m128iData = _mm_setr_epi32(std::rotl(u32Data[0], u32Oper), std::rotl(u32Data[1], u32Oper),
				std::rotl(u32Data[2], u32Oper), std::rotl(u32Data[3], u32Oper));
			break;
		case OPER_ROTR:
			m128iData = _mm_setr_epi32(std::rotr(u32Data[0], u32Oper), std::rotr(u32Data[1], u32Oper),
				std::rotr(u32Data[2], u32Oper), std::rotr(u32Data[3], u32Oper));
			break;
		case OPER_BITREV:
			m128iData = _mm_setr_epi32(ut::BitReverse(u32Data[0]), ut::BitReverse(u32Data[1]), ut::BitReverse(u32Data[2]),
				ut::BitReverse(u32Data[3]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported uint32_t operation.");
			break;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m128iData = ByteSwapVec<std::uint32_t>(m128iData);
		}

		_mm_storeu_si128(reinterpret_cast<__m128i*>(pData), m128iData);
	}

	void OperVec128_i64(std::byte* pData, const HEXMODIFY& hms) {
		auto m128iData = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pData));
		if (hms.fBigEndian) {
			m128iData = ByteSwapVec<std::int64_t>(_mm_loadu_si128(reinterpret_cast<const __m128i*>(pData)));
		}

		alignas(16) std::int64_t i64Data[2];
		_mm_store_si128(reinterpret_cast<__m128i*>(i64Data), m128iData);
		assert(!hms.spnData.empty());
		const auto i64Oper = *reinterpret_cast<const std::int64_t*>(hms.spnData.data());
		const auto m128iOper = _mm_set1_epi64x(i64Oper);

		using enum EHexOperMode;
		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m128iData = _mm_add_epi64(m128iData, m128iOper);
			break;
		case OPER_SUB:
			m128iData = _mm_sub_epi64(m128iData, m128iOper);
			break;
		case OPER_MUL:
			m128iData = _mm_setr_epi64x(i64Data[0] * i64Oper, i64Data[1] * i64Oper);
			break;
		case OPER_DIV:
			assert(i64Oper > 0);
			m128iData = _mm_setr_epi64x(i64Data[0] / i64Oper, i64Data[1] / i64Oper);
			break;
		case OPER_MIN:
			m128iData = _mm_setr_epi64x((std::max)(i64Data[0], i64Oper), (std::max)(i64Data[1], i64Oper));
			break;
		case OPER_MAX:
			m128iData = _mm_setr_epi64x((std::min)(i64Data[0], i64Oper), (std::min)(i64Data[1], i64Oper));
			break;
		case OPER_SWAP:
			m128iData = ByteSwapVec<std::int64_t>(m128iData);
			break;
		case OPER_OR:
			m128iData = _mm_or_si128(m128iData, m128iOper);
			break;
		case OPER_XOR:
			m128iData = _mm_xor_si128(m128iData, m128iOper);
			break;
		case OPER_AND:
			m128iData = _mm_and_si128(m128iData, m128iOper);
			break;
		case OPER_NOT:
			//_mm_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
			m128iData = _mm_xor_si128(m128iData, _mm_cmpeq_epi64(m128iData, m128iData)); //SSE4.1
			break;
		case OPER_SHL:
			m128iData = _mm_slli_epi64(m128iData, static_cast<int>(i64Oper));
			break;
		case OPER_SHR:
			m128iData = _mm_setr_epi64x(i64Data[0] >> i64Oper, i64Data[1] >> i64Oper);
			break;
		case OPER_ROTL:
			m128iData = _mm_setr_epi64x(std::rotl(static_cast<std::uint64_t>(i64Data[0]), static_cast<const int>(i64Oper)),
				std::rotl(static_cast<std::uint64_t>(i64Data[1]), static_cast<const int>(i64Oper)));
			break;
		case OPER_ROTR:
			m128iData = _mm_setr_epi64x(std::rotr(static_cast<std::uint64_t>(i64Data[0]), static_cast<const int>(i64Oper)),
				std::rotr(static_cast<std::uint64_t>(i64Data[1]), static_cast<const int>(i64Oper)));
			break;
		case OPER_BITREV:
			m128iData = _mm_setr_epi64x(ut::BitReverse(i64Data[0]), ut::BitReverse(i64Data[1]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported int64_t operation.");
			break;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m128iData = ByteSwapVec<std::int64_t>(m128iData);
		}

		_mm_storeu_si128(reinterpret_cast<__m128i*>(pData), m128iData);
	}

	void OperVec128_u64(std::byte* pData, const HEXMODIFY& hms) {
		auto m128iData = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pData));
		if (hms.fBigEndian) {
			m128iData = ByteSwapVec<std::uint64_t>(_mm_loadu_si128(reinterpret_cast<const __m128i*>(pData)));
		}

		alignas(16) std::uint64_t u64Data[2];
		_mm_store_si128(reinterpret_cast<__m128i*>(u64Data), m128iData);
		assert(!hms.spnData.empty());
		const auto u64Oper = *reinterpret_cast<const std::uint64_t*>(hms.spnData.data());
		const auto m128iOper = _mm_set1_epi64x(u64Oper);

		using enum EHexOperMode;
		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m128iData = _mm_add_epi64(m128iData, m128iOper);
			break;
		case OPER_SUB:
			m128iData = _mm_sub_epi64(m128iData, m128iOper);
			break;
		case OPER_MUL:
			m128iData = _mm_setr_epi64x(u64Data[0] * u64Oper, u64Data[1] * u64Oper);
			break;
		case OPER_DIV:
			assert(u64Oper > 0);
			m128iData = _mm_setr_epi64x(u64Data[0] / u64Oper, u64Data[1] / u64Oper);
			break;
		case OPER_MIN:
			m128iData = _mm_setr_epi64x((std::max)(u64Data[0], u64Oper), (std::max)(u64Data[1], u64Oper));
			break;
		case OPER_MAX:
			m128iData = _mm_setr_epi64x((std::min)(u64Data[0], u64Oper), (std::min)(u64Data[1], u64Oper));
			break;
		case OPER_SWAP:
			m128iData = ByteSwapVec<std::uint64_t>(m128iData);
			break;
		case OPER_OR:
			m128iData = _mm_or_si128(m128iData, m128iOper);
			break;
		case OPER_XOR:
			m128iData = _mm_xor_si128(m128iData, m128iOper);
			break;
		case OPER_AND:
			m128iData = _mm_and_si128(m128iData, m128iOper);
			break;
		case OPER_NOT:
			//_mm_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
			m128iData = _mm_xor_si128(m128iData, _mm_cmpeq_epi64(m128iData, m128iData)); //SSE4.1
			break;
		case OPER_SHL:
			m128iData = _mm_slli_epi64(m128iData, static_cast<int>(u64Oper));
			break;
		case OPER_SHR:
			m128iData = _mm_srli_epi64(m128iData, static_cast<int>(u64Oper)); //Logical shift.
			break;
		case OPER_ROTL:
			m128iData = _mm_setr_epi64x(std::rotl(u64Data[0], static_cast<const int>(u64Oper)),
				std::rotl(u64Data[1], static_cast<const int>(u64Oper)));
			break;
		case OPER_ROTR:
			m128iData = _mm_setr_epi64x(std::rotr(u64Data[0], static_cast<const int>(u64Oper)),
				std::rotr(u64Data[1], static_cast<const int>(u64Oper)));
			break;
		case OPER_BITREV:
			m128iData = _mm_setr_epi64x(ut::BitReverse(u64Data[0]), ut::BitReverse(u64Data[1]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported uint64_t operation.");
			break;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m128iData = ByteSwapVec<std::uint64_t>(m128iData);
		}

		_mm_storeu_si128(reinterpret_cast<__m128i*>(pData), m128iData);
	}

	void OperVec128_f(std::byte* pData, const HEXMODIFY& hms) {
		auto m128Data = _mm_loadu_ps(reinterpret_cast<const float*>(pData));
		if (hms.fBigEndian) {
			m128Data = ByteSwapVec<float>(_mm_loadu_ps(reinterpret_cast<const float*>(pData)));
		}

		assert(std::isfinite(*reinterpret_cast<const float*>(hms.spnData.data())));
		const auto m128Oper = _mm_set_ps1(*reinterpret_cast<const float*>(hms.spnData.data()));

		using enum EHexOperMode;
		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m128Data = _mm_add_ps(m128Data, m128Oper);
			break;
		case OPER_SUB:
			m128Data = _mm_sub_ps(m128Data, m128Oper);
			break;
		case OPER_MUL:
			m128Data = _mm_mul_ps(m128Data, m128Oper);
			break;
		case OPER_DIV:
			assert(*reinterpret_cast<const float*>(hms.spnData.data()) > 0.F);
			m128Data = _mm_div_ps(m128Data, m128Oper);
			break;
		case OPER_MIN:
			m128Data = _mm_max_ps(m128Data, m128Oper);
			break;
		case OPER_MAX:
			m128Data = _mm_min_ps(m128Data, m128Oper);
			break;
		case OPER_SWAP:
			m128Data = ByteSwapVec<float>(m128Data);
			break;
		default:
			ut::DBG_REPORT(L"Unsupported float operation.");
			return;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m128Data = ByteSwapVec<float>(m128Data);
		}

		_mm_storeu_ps(reinterpret_cast<float*>(pData), m128Data);
	}

	void OperVec128_d(std::byte* pData, const HEXMODIFY& hms) {
		auto m128dData = _mm_loadu_pd(reinterpret_cast<const double*>(pData));
		if (hms.fBigEndian) {
			m128dData = ByteSwapVec<double>(_mm_loadu_pd(reinterpret_cast<const double*>(pData)));
		}

		assert(std::isfinite(*reinterpret_cast<const double*>(hms.spnData.data())));
		const auto m128dOper = _mm_set1_pd(*reinterpret_cast<const double*>(hms.spnData.data()));

		using enum EHexOperMode;
		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m128dData = _mm_add_pd(m128dData, m128dOper);
			break;
		case OPER_SUB:
			m128dData = _mm_sub_pd(m128dData, m128dOper);
			break;
		case OPER_MUL:
			m128dData = _mm_mul_pd(m128dData, m128dOper);
			break;
		case OPER_DIV:
			assert(*reinterpret_cast<const double*>(hms.spnData.data()) > 0.);
			m128dData = _mm_div_pd(m128dData, m128dOper);
			break;
		case OPER_MIN:
			m128dData = _mm_max_pd(m128dData, m128dOper);
			break;
		case OPER_MAX:
			m128dData = _mm_min_pd(m128dData, m128dOper);
			break;
		case OPER_SWAP:
			m128dData = ByteSwapVec<double>(m128dData);
			break;
		default:
			ut::DBG_REPORT(L"Unsupported double operation.");
			return;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m128dData = ByteSwapVec<double>(m128dData);
		}

		_mm_storeu_pd(reinterpret_cast<double*>(pData), m128dData);
	}

	void OperVec256_i8(std::byte* pData, const HEXMODIFY& hms) {
		auto m256iData = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pData));
		alignas(32) std::int8_t i8Data[32];
		_mm256_store_si256(reinterpret_cast<__m256i*>(i8Data), m256iData);
		assert(!hms.spnData.empty());
		const auto i8Oper = *reinterpret_cast<const std::int8_t*>(hms.spnData.data());
		const auto m256iOper = _mm256_set1_epi8(i8Oper);

		using enum EHexOperMode;
		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m256iData = _mm256_add_epi8(m256iData, m256iOper);
			break;
		case OPER_SUB:
			m256iData = _mm256_sub_epi8(m256iData, m256iOper);
			break;
		case OPER_MUL:
		{
			const auto m256iEven = _mm256_mullo_epi16(m256iData, m256iOper);
			const auto m256iOdd = _mm256_mullo_epi16(_mm256_srli_epi16(m256iData, 8), _mm256_srli_epi16(m256iOper, 8));
			m256iData = _mm256_or_si256(_mm256_slli_epi16(m256iOdd, 8), _mm256_srli_epi16(_mm256_slli_epi16(m256iEven, 8), 8));
		}
		break;
		case OPER_DIV:
			assert(i8Oper > 0);
			m256iData = _mm256_setr_epi8(i8Data[0] / i8Oper, i8Data[1] / i8Oper, i8Data[2] / i8Oper, i8Data[3] / i8Oper,
				i8Data[4] / i8Oper, i8Data[5] / i8Oper, i8Data[6] / i8Oper, i8Data[7] / i8Oper, i8Data[8] / i8Oper,
				i8Data[9] / i8Oper, i8Data[10] / i8Oper, i8Data[11] / i8Oper, i8Data[12] / i8Oper, i8Data[13] / i8Oper,
				i8Data[14] / i8Oper, i8Data[15] / i8Oper, i8Data[16] / i8Oper, i8Data[17] / i8Oper, i8Data[18] / i8Oper,
				i8Data[19] / i8Oper, i8Data[20] / i8Oper, i8Data[21] / i8Oper, i8Data[22] / i8Oper, i8Data[23] / i8Oper,
				i8Data[24] / i8Oper, i8Data[25] / i8Oper, i8Data[26] / i8Oper, i8Data[27] / i8Oper, i8Data[28] / i8Oper,
				i8Data[29] / i8Oper, i8Data[30] / i8Oper, i8Data[31] / i8Oper);
			break;
		case OPER_MIN:
			m256iData = _mm256_max_epi8(m256iData, m256iOper);
			break;
		case OPER_MAX:
			m256iData = _mm256_min_epi8(m256iData, m256iOper);
			break;
		case OPER_SWAP: //No need for the int8_t.
			break;
		case OPER_OR:
			m256iData = _mm256_or_si256(m256iData, m256iOper);
			break;
		case OPER_XOR:
			m256iData = _mm256_xor_si256(m256iData, m256iOper);
			break;
		case OPER_AND:
			m256iData = _mm256_and_si256(m256iData, m256iOper);
			break;
		case OPER_NOT:
			//_mm256_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
			m256iData = _mm256_xor_si256(m256iData, _mm256_cmpeq_epi64(m256iData, m256iData));
			break;
		case OPER_SHL:
			m256iData = _mm256_setr_epi8(i8Data[0] << i8Oper, i8Data[1] << i8Oper, i8Data[2] << i8Oper,
				i8Data[3] << i8Oper, i8Data[4] << i8Oper, i8Data[5] << i8Oper, i8Data[6] << i8Oper,
				i8Data[7] << i8Oper, i8Data[8] << i8Oper, i8Data[9] << i8Oper, i8Data[10] << i8Oper,
				i8Data[11] << i8Oper, i8Data[12] << i8Oper, i8Data[13] << i8Oper, i8Data[14] << i8Oper,
				i8Data[15] << i8Oper, i8Data[16] << i8Oper, i8Data[17] << i8Oper, i8Data[18] << i8Oper,
				i8Data[19] << i8Oper, i8Data[20] << i8Oper, i8Data[21] << i8Oper, i8Data[22] << i8Oper,
				i8Data[23] << i8Oper, i8Data[24] << i8Oper, i8Data[25] << i8Oper, i8Data[26] << i8Oper,
				i8Data[27] << i8Oper, i8Data[28] << i8Oper, i8Data[29] << i8Oper, i8Data[30] << i8Oper,
				i8Data[31] << i8Oper);
			break;
		case OPER_SHR:
			m256iData = _mm256_setr_epi8(i8Data[0] >> i8Oper, i8Data[1] >> i8Oper, i8Data[2] >> i8Oper,
				i8Data[3] >> i8Oper, i8Data[4] >> i8Oper, i8Data[5] >> i8Oper, i8Data[6] >> i8Oper,
				i8Data[7] >> i8Oper, i8Data[8] >> i8Oper, i8Data[9] >> i8Oper, i8Data[10] >> i8Oper,
				i8Data[11] >> i8Oper, i8Data[12] >> i8Oper, i8Data[13] >> i8Oper, i8Data[14] >> i8Oper,
				i8Data[15] >> i8Oper, i8Data[16] >> i8Oper, i8Data[17] >> i8Oper, i8Data[18] >> i8Oper,
				i8Data[19] >> i8Oper, i8Data[20] >> i8Oper, i8Data[21] >> i8Oper, i8Data[22] >> i8Oper,
				i8Data[23] >> i8Oper, i8Data[24] >> i8Oper, i8Data[25] >> i8Oper, i8Data[26] >> i8Oper,
				i8Data[27] >> i8Oper, i8Data[28] >> i8Oper, i8Data[29] >> i8Oper, i8Data[30] >> i8Oper,
				i8Data[31] >> i8Oper);
			break;
		case OPER_ROTL:
			m256iData = _mm256_setr_epi8(std::rotl(static_cast<std::uint8_t>(i8Data[0]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[1]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[2]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[3]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[4]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[5]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[6]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[7]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[8]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[9]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[10]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[11]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[12]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[13]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[14]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[15]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[16]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[17]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[18]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[19]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[20]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[21]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[22]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[23]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[24]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[25]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[26]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[27]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[28]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[29]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[30]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[31]), i8Oper));
			break;
		case OPER_ROTR:
			m256iData = _mm256_setr_epi8(std::rotr(static_cast<std::uint8_t>(i8Data[0]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[1]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[2]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[3]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[4]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[5]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[6]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[7]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[8]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[9]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[10]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[11]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[12]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[13]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[14]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[15]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[16]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[17]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[18]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[19]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[20]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[21]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[22]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[23]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[24]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[25]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[26]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[27]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[28]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[29]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[30]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[31]), i8Oper));
			break;
		case OPER_BITREV:
			m256iData = _mm256_setr_epi8(ut::BitReverse(i8Data[0]), ut::BitReverse(i8Data[1]), ut::BitReverse(i8Data[2]),
				ut::BitReverse(i8Data[3]), ut::BitReverse(i8Data[4]), ut::BitReverse(i8Data[5]), ut::BitReverse(i8Data[6]),
				ut::BitReverse(i8Data[7]), ut::BitReverse(i8Data[8]), ut::BitReverse(i8Data[9]), ut::BitReverse(i8Data[10]),
				ut::BitReverse(i8Data[11]), ut::BitReverse(i8Data[12]), ut::BitReverse(i8Data[13]), ut::BitReverse(i8Data[14]),
				ut::BitReverse(i8Data[15]), ut::BitReverse(i8Data[16]), ut::BitReverse(i8Data[17]), ut::BitReverse(i8Data[18]),
				ut::BitReverse(i8Data[19]), ut::BitReverse(i8Data[20]), ut::BitReverse(i8Data[21]), ut::BitReverse(i8Data[22]),
				ut::BitReverse(i8Data[23]), ut::BitReverse(i8Data[24]), ut::BitReverse(i8Data[25]), ut::BitReverse(i8Data[26]),
				ut::BitReverse(i8Data[27]), ut::BitReverse(i8Data[28]), ut::BitReverse(i8Data[29]), ut::BitReverse(i8Data[30]),
				ut::BitReverse(i8Data[31]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported int8_t operation.");
			break;
		}

		_mm256_storeu_si256(reinterpret_cast<__m256i*>(pData), m256iData);
	}

	void OperVec256_u8(std::byte* pData, const HEXMODIFY& hms) {
		auto m256iData = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pData));
		alignas(32) std::uint8_t u8Data[32];
		_mm256_store_si256(reinterpret_cast<__m256i*>(u8Data), m256iData);
		assert(!hms.spnData.empty());
		const auto u8Oper = *reinterpret_cast<const std::uint8_t*>(hms.spnData.data());
		const auto m256iOper = _mm256_set1_epi8(u8Oper);

		using enum EHexOperMode;
		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m256iData = _mm256_add_epi8(m256iData, m256iOper);
			break;
		case OPER_SUB:
			m256iData = _mm256_sub_epi8(m256iData, m256iOper);
			break;
		case OPER_MUL:
			m256iData = _mm256_setr_epi8(u8Data[0] * u8Oper, u8Data[1] * u8Oper, u8Data[2] * u8Oper,
				u8Data[3] * u8Oper, u8Data[4] * u8Oper, u8Data[5] * u8Oper, u8Data[6] * u8Oper,
				u8Data[7] * u8Oper, u8Data[8] * u8Oper, u8Data[9] * u8Oper, u8Data[10] * u8Oper,
				u8Data[11] * u8Oper, u8Data[12] * u8Oper, u8Data[13] * u8Oper, u8Data[14] * u8Oper,
				u8Data[15] * u8Oper, u8Data[16] * u8Oper, u8Data[17] * u8Oper, u8Data[18] * u8Oper,
				u8Data[19] * u8Oper, u8Data[20] * u8Oper, u8Data[21] * u8Oper, u8Data[22] * u8Oper,
				u8Data[23] * u8Oper, u8Data[24] * u8Oper, u8Data[25] * u8Oper, u8Data[26] * u8Oper,
				u8Data[27] * u8Oper, u8Data[28] * u8Oper, u8Data[29] * u8Oper, u8Data[30] * u8Oper,
				u8Data[31] * u8Oper);
			break;
		case OPER_DIV:
			assert(u8Oper > 0);
			m256iData = _mm256_setr_epi8(u8Data[0] / u8Oper, u8Data[1] / u8Oper, u8Data[2] / u8Oper,
				u8Data[3] / u8Oper, u8Data[4] / u8Oper, u8Data[5] / u8Oper, u8Data[6] / u8Oper,
				u8Data[7] / u8Oper, u8Data[8] / u8Oper, u8Data[9] / u8Oper, u8Data[10] / u8Oper,
				u8Data[11] / u8Oper, u8Data[12] / u8Oper, u8Data[13] / u8Oper, u8Data[14] / u8Oper,
				u8Data[15] / u8Oper, u8Data[16] / u8Oper, u8Data[17] / u8Oper, u8Data[18] / u8Oper,
				u8Data[19] / u8Oper, u8Data[20] / u8Oper, u8Data[21] / u8Oper, u8Data[22] / u8Oper,
				u8Data[23] / u8Oper, u8Data[24] / u8Oper, u8Data[25] / u8Oper, u8Data[26] / u8Oper,
				u8Data[27] / u8Oper, u8Data[28] / u8Oper, u8Data[29] / u8Oper, u8Data[30] / u8Oper,
				u8Data[31] / u8Oper);
			break;
		case OPER_MIN:
			m256iData = _mm256_max_epu8(m256iData, m256iOper);
			break;
		case OPER_MAX:
			m256iData = _mm256_min_epu8(m256iData, m256iOper);
			break;
		case OPER_SWAP:	//No need for the uint8_t.
			break;
		case OPER_OR:
			m256iData = _mm256_or_si256(m256iData, m256iOper);
			break;
		case OPER_XOR:
			m256iData = _mm256_xor_si256(m256iData, m256iOper);
			break;
		case OPER_AND:
			m256iData = _mm256_and_si256(m256iData, m256iOper);
			break;
		case OPER_NOT:
			//_mm256_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
			m256iData = _mm256_xor_si256(m256iData, _mm256_cmpeq_epi64(m256iData, m256iData));
			break;
		case OPER_SHL:
			m256iData = _mm256_setr_epi8(u8Data[0] << u8Oper, u8Data[1] << u8Oper, u8Data[2] << u8Oper,
				u8Data[3] << u8Oper, u8Data[4] << u8Oper, u8Data[5] << u8Oper, u8Data[6] << u8Oper,
				u8Data[7] << u8Oper, u8Data[8] << u8Oper, u8Data[9] << u8Oper, u8Data[10] << u8Oper,
				u8Data[11] << u8Oper, u8Data[12] << u8Oper, u8Data[13] << u8Oper, u8Data[14] << u8Oper,
				u8Data[15] << u8Oper, u8Data[16] << u8Oper, u8Data[17] << u8Oper, u8Data[18] << u8Oper,
				u8Data[19] << u8Oper, u8Data[20] << u8Oper, u8Data[21] << u8Oper, u8Data[22] << u8Oper,
				u8Data[23] << u8Oper, u8Data[24] << u8Oper, u8Data[25] << u8Oper, u8Data[26] << u8Oper,
				u8Data[27] << u8Oper, u8Data[28] << u8Oper, u8Data[29] << u8Oper, u8Data[30] << u8Oper,
				u8Data[31] << u8Oper);
			break;
		case OPER_SHR:
			m256iData = _mm256_setr_epi8(u8Data[0] >> u8Oper, u8Data[1] >> u8Oper, u8Data[2] >> u8Oper,
				u8Data[3] >> u8Oper, u8Data[4] >> u8Oper, u8Data[5] >> u8Oper, u8Data[6] >> u8Oper,
				u8Data[7] >> u8Oper, u8Data[8] >> u8Oper, u8Data[9] >> u8Oper, u8Data[10] >> u8Oper,
				u8Data[11] >> u8Oper, u8Data[12] >> u8Oper, u8Data[13] >> u8Oper, u8Data[14] >> u8Oper,
				u8Data[15] >> u8Oper, u8Data[16] >> u8Oper, u8Data[17] >> u8Oper, u8Data[18] >> u8Oper,
				u8Data[19] >> u8Oper, u8Data[20] >> u8Oper, u8Data[21] >> u8Oper, u8Data[22] >> u8Oper,
				u8Data[23] >> u8Oper, u8Data[24] >> u8Oper, u8Data[25] >> u8Oper, u8Data[26] >> u8Oper,
				u8Data[27] >> u8Oper, u8Data[28] >> u8Oper, u8Data[29] >> u8Oper, u8Data[30] >> u8Oper,
				u8Data[31] >> u8Oper);
			break;
		case OPER_ROTL:
			m256iData = _mm256_setr_epi8(std::rotl(u8Data[0], u8Oper), std::rotl(u8Data[1], u8Oper),
				std::rotl(u8Data[2], u8Oper), std::rotl(u8Data[3], u8Oper), std::rotl(u8Data[4], u8Oper),
				std::rotl(u8Data[5], u8Oper), std::rotl(u8Data[6], u8Oper), std::rotl(u8Data[7], u8Oper),
				std::rotl(u8Data[8], u8Oper), std::rotl(u8Data[9], u8Oper), std::rotl(u8Data[10], u8Oper),
				std::rotl(u8Data[11], u8Oper), std::rotl(u8Data[12], u8Oper), std::rotl(u8Data[13], u8Oper),
				std::rotl(u8Data[14], u8Oper), std::rotl(u8Data[15], u8Oper), std::rotl(u8Data[16], u8Oper),
				std::rotl(u8Data[17], u8Oper), std::rotl(u8Data[18], u8Oper), std::rotl(u8Data[19], u8Oper),
				std::rotl(u8Data[20], u8Oper), std::rotl(u8Data[21], u8Oper), std::rotl(u8Data[22], u8Oper),
				std::rotl(u8Data[23], u8Oper), std::rotl(u8Data[24], u8Oper), std::rotl(u8Data[25], u8Oper),
				std::rotl(u8Data[26], u8Oper), std::rotl(u8Data[27], u8Oper), std::rotl(u8Data[28], u8Oper),
				std::rotl(u8Data[29], u8Oper), std::rotl(u8Data[30], u8Oper), std::rotl(u8Data[31], u8Oper));
			break;
		case OPER_ROTR:
			m256iData = _mm256_setr_epi8(std::rotr(u8Data[0], u8Oper), std::rotr(u8Data[1], u8Oper),
				std::rotr(u8Data[2], u8Oper), std::rotr(u8Data[3], u8Oper), std::rotr(u8Data[4], u8Oper),
				std::rotr(u8Data[5], u8Oper), std::rotr(u8Data[6], u8Oper), std::rotr(u8Data[7], u8Oper),
				std::rotr(u8Data[8], u8Oper), std::rotr(u8Data[9], u8Oper), std::rotr(u8Data[10], u8Oper),
				std::rotr(u8Data[11], u8Oper), std::rotr(u8Data[12], u8Oper), std::rotr(u8Data[13], u8Oper),
				std::rotr(u8Data[14], u8Oper), std::rotr(u8Data[15], u8Oper), std::rotr(u8Data[16], u8Oper),
				std::rotr(u8Data[17], u8Oper), std::rotr(u8Data[18], u8Oper), std::rotr(u8Data[19], u8Oper),
				std::rotr(u8Data[20], u8Oper), std::rotr(u8Data[21], u8Oper), std::rotr(u8Data[22], u8Oper),
				std::rotr(u8Data[23], u8Oper), std::rotr(u8Data[24], u8Oper), std::rotr(u8Data[25], u8Oper),
				std::rotr(u8Data[26], u8Oper), std::rotr(u8Data[27], u8Oper), std::rotr(u8Data[28], u8Oper),
				std::rotr(u8Data[29], u8Oper), std::rotr(u8Data[30], u8Oper), std::rotr(u8Data[31], u8Oper));
			break;
		case OPER_BITREV:
			m256iData = _mm256_setr_epi8(ut::BitReverse(u8Data[0]), ut::BitReverse(u8Data[1]), ut::BitReverse(u8Data[2]),
				ut::BitReverse(u8Data[3]), ut::BitReverse(u8Data[4]), ut::BitReverse(u8Data[5]), ut::BitReverse(u8Data[6]),
				ut::BitReverse(u8Data[7]), ut::BitReverse(u8Data[8]), ut::BitReverse(u8Data[9]), ut::BitReverse(u8Data[10]),
				ut::BitReverse(u8Data[11]), ut::BitReverse(u8Data[12]), ut::BitReverse(u8Data[13]), ut::BitReverse(u8Data[14]),
				ut::BitReverse(u8Data[15]), ut::BitReverse(u8Data[16]), ut::BitReverse(u8Data[17]), ut::BitReverse(u8Data[18]),
				ut::BitReverse(u8Data[19]), ut::BitReverse(u8Data[20]), ut::BitReverse(u8Data[21]), ut::BitReverse(u8Data[22]),
				ut::BitReverse(u8Data[23]), ut::BitReverse(u8Data[24]), ut::BitReverse(u8Data[25]), ut::BitReverse(u8Data[26]),
				ut::BitReverse(u8Data[27]), ut::BitReverse(u8Data[28]), ut::BitReverse(u8Data[29]), ut::BitReverse(u8Data[30]),
				ut::BitReverse(u8Data[31]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported uint8_t operation.");
			break;
		}

		_mm256_storeu_si256(reinterpret_cast<__m256i*>(pData), m256iData);
	}

	void OperVec256_i16(std::byte* pData, const HEXMODIFY& hms) {
		auto m256iData = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pData));
		if (hms.fBigEndian) {
			m256iData = ByteSwapVec<std::int16_t>(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(pData)));
		}

		alignas(32) std::int16_t i16Data[16];
		_mm256_store_si256(reinterpret_cast<__m256i*>(i16Data), m256iData);
		assert(!hms.spnData.empty());
		const auto i16Oper = *reinterpret_cast<const std::int16_t*>(hms.spnData.data());
		const auto m256iOper = _mm256_set1_epi16(i16Oper);

		using enum EHexOperMode;
		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m256iData = _mm256_add_epi16(m256iData, m256iOper);
			break;
		case OPER_SUB:
			m256iData = _mm256_sub_epi16(m256iData, m256iOper);
			break;
		case OPER_MUL:
			m256iData = _mm256_mullo_epi16(m256iData, m256iOper);
			break;
		case OPER_DIV:
			assert(i16Oper > 0);
			m256iData = _mm256_setr_epi16(i16Data[0] / i16Oper, i16Data[1] / i16Oper, i16Data[2] / i16Oper,
				i16Data[3] / i16Oper, i16Data[4] / i16Oper, i16Data[5] / i16Oper, i16Data[6] / i16Oper,
				i16Data[7] / i16Oper, i16Data[8] / i16Oper, i16Data[9] / i16Oper, i16Data[10] / i16Oper,
				i16Data[11] / i16Oper, i16Data[12] / i16Oper, i16Data[13] / i16Oper, i16Data[14] / i16Oper,
				i16Data[15] / i16Oper);
			break;
		case OPER_MIN:
			m256iData = _mm256_max_epi16(m256iData, m256iOper);
			break;
		case OPER_MAX:
			m256iData = _mm256_min_epi16(m256iData, m256iOper);
			break;
		case OPER_SWAP:
			m256iData = ByteSwapVec<std::int16_t>(m256iData);
			break;
		case OPER_OR:
			m256iData = _mm256_or_si256(m256iData, m256iOper);
			break;
		case OPER_XOR:
			m256iData = _mm256_xor_si256(m256iData, m256iOper);
			break;
		case OPER_AND:
			m256iData = _mm256_and_si256(m256iData, m256iOper);
			break;
		case OPER_NOT:
			//_mm256_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
			m256iData = _mm256_xor_si256(m256iData, _mm256_cmpeq_epi64(m256iData, m256iData));
			break;
		case OPER_SHL:
			m256iData = _mm256_slli_epi16(m256iData, i16Oper);
			break;
		case OPER_SHR:
			m256iData = _mm256_srai_epi16(m256iData, i16Oper); //Arithmetic shift.
			break;
		case OPER_ROTL:
			m256iData = _mm256_setr_epi16(std::rotl(static_cast<std::uint16_t>(i16Data[0]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[1]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[2]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[3]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[4]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[5]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[6]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[7]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[8]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[9]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[10]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[11]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[12]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[13]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[14]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[15]), i16Oper));
			break;
		case OPER_ROTR:
			m256iData = _mm256_setr_epi16(std::rotr(static_cast<std::uint16_t>(i16Data[0]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[1]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[2]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[3]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[4]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[5]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[6]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[7]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[8]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[9]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[10]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[11]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[12]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[13]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[14]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[15]), i16Oper));
			break;
		case OPER_BITREV:
			m256iData = _mm256_setr_epi16(ut::BitReverse(i16Data[0]), ut::BitReverse(i16Data[1]), ut::BitReverse(i16Data[2]),
				ut::BitReverse(i16Data[3]), ut::BitReverse(i16Data[4]), ut::BitReverse(i16Data[5]), ut::BitReverse(i16Data[6]),
				ut::BitReverse(i16Data[7]), ut::BitReverse(i16Data[8]), ut::BitReverse(i16Data[9]), ut::BitReverse(i16Data[10]),
				ut::BitReverse(i16Data[11]), ut::BitReverse(i16Data[12]), ut::BitReverse(i16Data[13]), ut::BitReverse(i16Data[14]),
				ut::BitReverse(i16Data[15]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported int16_t operation.");
			break;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m256iData = ByteSwapVec<std::int16_t>(m256iData);
		}

		_mm256_storeu_si256(reinterpret_cast<__m256i*>(pData), m256iData);
	}

	void OperVec256_u16(std::byte* pData, const HEXMODIFY& hms) {
		auto m256iData = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pData));
		if (hms.fBigEndian) {
			m256iData = ByteSwapVec<std::uint16_t>(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(pData)));
		}

		alignas(32) std::uint16_t u16Data[16];
		_mm256_store_si256(reinterpret_cast<__m256i*>(u16Data), m256iData);
		assert(!hms.spnData.empty());
		const auto u16Oper = *reinterpret_cast<const std::uint16_t*>(hms.spnData.data());
		const auto m256iOper = _mm256_set1_epi16(u16Oper);

		using enum EHexOperMode;
		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m256iData = _mm256_add_epi16(m256iData, m256iOper);
			break;
		case OPER_SUB:
			m256iData = _mm256_sub_epi16(m256iData, m256iOper);
			break;
		case OPER_MUL:
			m256iData = _mm256_setr_epi16(u16Data[0] * u16Oper, u16Data[1] * u16Oper, u16Data[2] * u16Oper,
				u16Data[3] * u16Oper, u16Data[4] * u16Oper, u16Data[5] * u16Oper, u16Data[6] * u16Oper,
				u16Data[7] * u16Oper, u16Data[8] * u16Oper, u16Data[9] * u16Oper, u16Data[10] * u16Oper,
				u16Data[11] * u16Oper, u16Data[12] * u16Oper, u16Data[13] * u16Oper, u16Data[14] * u16Oper,
				u16Data[15] * u16Oper);
			break;
		case OPER_DIV:
			assert(u16Oper > 0);
			m256iData = _mm256_setr_epi16(u16Data[0] / u16Oper, u16Data[1] / u16Oper, u16Data[2] / u16Oper,
				u16Data[3] / u16Oper, u16Data[4] / u16Oper, u16Data[5] / u16Oper, u16Data[6] / u16Oper,
				u16Data[7] / u16Oper, u16Data[8] / u16Oper, u16Data[9] / u16Oper, u16Data[10] / u16Oper,
				u16Data[11] / u16Oper, u16Data[12] / u16Oper, u16Data[13] / u16Oper, u16Data[14] / u16Oper,
				u16Data[15] / u16Oper);
			break;
		case OPER_MIN:
			m256iData = _mm256_max_epu16(m256iData, m256iOper);
			break;
		case OPER_MAX:
			m256iData = _mm256_min_epu16(m256iData, m256iOper);
			break;
		case OPER_SWAP:
			m256iData = ByteSwapVec<std::uint16_t>(m256iData);
			break;
		case OPER_OR:
			m256iData = _mm256_or_si256(m256iData, m256iOper);
			break;
		case OPER_XOR:
			m256iData = _mm256_xor_si256(m256iData, m256iOper);
			break;
		case OPER_AND:
			m256iData = _mm256_and_si256(m256iData, m256iOper);
			break;
		case OPER_NOT:
			//_mm256_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
			m256iData = _mm256_xor_si256(m256iData, _mm256_cmpeq_epi64(m256iData, m256iData));
			break;
		case OPER_SHL:
			m256iData = _mm256_slli_epi16(m256iData, u16Oper);
			break;
		case OPER_SHR:
			m256iData = _mm256_srli_epi16(m256iData, u16Oper); //Logical shift.
			break;
		case OPER_ROTL:
			m256iData = _mm256_setr_epi16(std::rotl(u16Data[0], u16Oper), std::rotl(u16Data[1], u16Oper),
				std::rotl(u16Data[2], u16Oper), std::rotl(u16Data[3], u16Oper), std::rotl(u16Data[4], u16Oper),
				std::rotl(u16Data[5], u16Oper), std::rotl(u16Data[6], u16Oper), std::rotl(u16Data[7], u16Oper),
				std::rotl(u16Data[8], u16Oper), std::rotl(u16Data[9], u16Oper), std::rotl(u16Data[10], u16Oper),
				std::rotl(u16Data[11], u16Oper), std::rotl(u16Data[12], u16Oper), std::rotl(u16Data[13], u16Oper),
				std::rotl(u16Data[14], u16Oper), std::rotl(u16Data[15], u16Oper));
			break;
		case OPER_ROTR:
			m256iData = _mm256_setr_epi16(std::rotr(u16Data[0], u16Oper), std::rotr(u16Data[1], u16Oper),
				std::rotr(u16Data[2], u16Oper), std::rotr(u16Data[3], u16Oper), std::rotr(u16Data[4], u16Oper),
				std::rotr(u16Data[5], u16Oper), std::rotr(u16Data[6], u16Oper), std::rotr(u16Data[7], u16Oper),
				std::rotr(u16Data[8], u16Oper), std::rotr(u16Data[9], u16Oper), std::rotr(u16Data[10], u16Oper),
				std::rotr(u16Data[11], u16Oper), std::rotr(u16Data[12], u16Oper), std::rotr(u16Data[13], u16Oper),
				std::rotr(u16Data[14], u16Oper), std::rotr(u16Data[15], u16Oper));
			break;
		case OPER_BITREV:
			m256iData = _mm256_setr_epi16(ut::BitReverse(u16Data[0]), ut::BitReverse(u16Data[1]), ut::BitReverse(u16Data[2]),
				ut::BitReverse(u16Data[3]), ut::BitReverse(u16Data[4]), ut::BitReverse(u16Data[5]), ut::BitReverse(u16Data[6]),
				ut::BitReverse(u16Data[7]), ut::BitReverse(u16Data[8]), ut::BitReverse(u16Data[9]), ut::BitReverse(u16Data[10]),
				ut::BitReverse(u16Data[11]), ut::BitReverse(u16Data[12]), ut::BitReverse(u16Data[13]), ut::BitReverse(u16Data[14]),
				ut::BitReverse(u16Data[15]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported uint16_t operation.");
			break;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m256iData = ByteSwapVec<std::uint16_t>(m256iData);
		}

		_mm256_storeu_si256(reinterpret_cast<__m256i*>(pData), m256iData);
	}

	void OperVec256_i32(std::byte* pData, const HEXMODIFY& hms) {
		auto m256iData = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pData));
		if (hms.fBigEndian) {
			m256iData = ByteSwapVec<std::int32_t>(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(pData)));
		}

		alignas(32) std::int32_t i32Data[8];
		_mm256_store_si256(reinterpret_cast<__m256i*>(i32Data), m256iData);
		assert(!hms.spnData.empty());
		const auto i32Oper = *reinterpret_cast<const std::int32_t*>(hms.spnData.data());
		const auto m256iOper = _mm256_set1_epi32(i32Oper);

		using enum EHexOperMode;
		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m256iData = _mm256_add_epi32(m256iData, m256iOper);
			break;
		case OPER_SUB:
			m256iData = _mm256_sub_epi32(m256iData, m256iOper);
			break;
		case OPER_MUL:
			m256iData = _mm256_mullo_epi32(m256iData, m256iOper);
			break;
		case OPER_DIV:
			assert(i32Oper > 0);
			m256iData = _mm256_setr_epi32(i32Data[0] / i32Oper, i32Data[1] / i32Oper, i32Data[2] / i32Oper,
				i32Data[3] / i32Oper, i32Data[4] / i32Oper, i32Data[5] / i32Oper, i32Data[6] / i32Oper,
				i32Data[7] / i32Oper);
			break;
		case OPER_MIN:
			m256iData = _mm256_max_epi32(m256iData, m256iOper);
			break;
		case OPER_MAX:
			m256iData = _mm256_min_epi32(m256iData, m256iOper);
			break;
		case OPER_SWAP:
			m256iData = ByteSwapVec<std::int32_t>(m256iData);
			break;
		case OPER_OR:
			m256iData = _mm256_or_si256(m256iData, m256iOper);
			break;
		case OPER_XOR:
			m256iData = _mm256_xor_si256(m256iData, m256iOper);
			break;
		case OPER_AND:
			m256iData = _mm256_and_si256(m256iData, m256iOper);
			break;
		case OPER_NOT:
			//_mm256_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
			m256iData = _mm256_xor_si256(m256iData, _mm256_cmpeq_epi64(m256iData, m256iData));
			break;
		case OPER_SHL:
			m256iData = _mm256_slli_epi32(m256iData, i32Oper);
			break;
		case OPER_SHR:
			m256iData = _mm256_srai_epi32(m256iData, i32Oper); //Arithmetic shift.
			break;
		case OPER_ROTL:
			m256iData = _mm256_setr_epi32(std::rotl(static_cast<std::uint32_t>(i32Data[0]), i32Oper),
				std::rotl(static_cast<std::uint32_t>(i32Data[1]), i32Oper),
				std::rotl(static_cast<std::uint32_t>(i32Data[2]), i32Oper),
				std::rotl(static_cast<std::uint32_t>(i32Data[3]), i32Oper),
				std::rotl(static_cast<std::uint32_t>(i32Data[4]), i32Oper),
				std::rotl(static_cast<std::uint32_t>(i32Data[5]), i32Oper),
				std::rotl(static_cast<std::uint32_t>(i32Data[6]), i32Oper),
				std::rotl(static_cast<std::uint32_t>(i32Data[7]), i32Oper));
			break;
		case OPER_ROTR:
			m256iData = _mm256_setr_epi32(std::rotr(static_cast<std::uint32_t>(i32Data[0]), i32Oper),
				std::rotr(static_cast<std::uint32_t>(i32Data[1]), i32Oper),
				std::rotr(static_cast<std::uint32_t>(i32Data[2]), i32Oper),
				std::rotr(static_cast<std::uint32_t>(i32Data[3]), i32Oper),
				std::rotr(static_cast<std::uint32_t>(i32Data[4]), i32Oper),
				std::rotr(static_cast<std::uint32_t>(i32Data[5]), i32Oper),
				std::rotr(static_cast<std::uint32_t>(i32Data[6]), i32Oper),
				std::rotr(static_cast<std::uint32_t>(i32Data[7]), i32Oper));
			break;
		case OPER_BITREV:
			m256iData = _mm256_setr_epi32(ut::BitReverse(i32Data[0]), ut::BitReverse(i32Data[1]), ut::BitReverse(i32Data[2]),
				ut::BitReverse(i32Data[3]), ut::BitReverse(i32Data[4]), ut::BitReverse(i32Data[5]), ut::BitReverse(i32Data[6]),
				ut::BitReverse(i32Data[7]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported int32_t operation.");
			break;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m256iData = ByteSwapVec<std::int32_t>(m256iData);
		}

		_mm256_storeu_si256(reinterpret_cast<__m256i*>(pData), m256iData);
	}

	void OperVec256_u32(std::byte* pData, const HEXMODIFY& hms) {
		auto m256iData = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pData));
		if (hms.fBigEndian) {
			m256iData = ByteSwapVec<std::uint32_t>(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(pData)));
		}

		alignas(32) std::uint32_t u32Data[8];
		_mm256_store_si256(reinterpret_cast<__m256i*>(u32Data), m256iData);
		assert(!hms.spnData.empty());
		const auto u32Oper = *reinterpret_cast<const std::uint32_t*>(hms.spnData.data());
		const auto m256iOper = _mm256_set1_epi32(u32Oper);

		using enum EHexOperMode;
		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m256iData = _mm256_add_epi32(m256iData, m256iOper);
			break;
		case OPER_SUB:
			m256iData = _mm256_sub_epi32(m256iData, m256iOper);
			break;
		case OPER_MUL:
			m256iData = _mm256_setr_epi32(u32Data[0] * u32Oper, u32Data[1] * u32Oper, u32Data[2] * u32Oper,
				u32Data[3] * u32Oper, u32Data[4] * u32Oper, u32Data[5] * u32Oper, u32Data[6] * u32Oper,
				u32Data[7] * u32Oper);
			break;
		case OPER_DIV:
			assert(u32Oper > 0);
			m256iData = _mm256_setr_epi32(u32Data[0] / u32Oper, u32Data[1] / u32Oper, u32Data[2] / u32Oper,
				u32Data[3] / u32Oper, u32Data[4] / u32Oper, u32Data[5] / u32Oper, u32Data[6] / u32Oper,
				u32Data[7] / u32Oper);
			break;
		case OPER_MIN:
			m256iData = _mm256_max_epu32(m256iData, m256iOper);
			break;
		case OPER_MAX:
			m256iData = _mm256_min_epu32(m256iData, m256iOper);
			break;
		case OPER_SWAP:
			m256iData = ByteSwapVec<std::uint32_t>(m256iData);
			break;
		case OPER_OR:
			m256iData = _mm256_or_si256(m256iData, m256iOper);
			break;
		case OPER_XOR:
			m256iData = _mm256_xor_si256(m256iData, m256iOper);
			break;
		case OPER_AND:
			m256iData = _mm256_and_si256(m256iData, m256iOper);
			break;
		case OPER_NOT:
			//_mm256_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
			m256iData = _mm256_xor_si256(m256iData, _mm256_cmpeq_epi64(m256iData, m256iData));
			break;
		case OPER_SHL:
			m256iData = _mm256_slli_epi32(m256iData, u32Oper);
			break;
		case OPER_SHR:
			m256iData = _mm256_srli_epi32(m256iData, u32Oper); //Logical shift.
			break;
		case OPER_ROTL:
			m256iData = _mm256_setr_epi32(std::rotl(u32Data[0], u32Oper), std::rotl(u32Data[1], u32Oper),
				std::rotl(u32Data[2], u32Oper), std::rotl(u32Data[3], u32Oper), std::rotl(u32Data[4], u32Oper),
				std::rotl(u32Data[5], u32Oper), std::rotl(u32Data[6], u32Oper), std::rotl(u32Data[7], u32Oper));
			break;
		case OPER_ROTR:
			m256iData = _mm256_setr_epi32(std::rotr(u32Data[0], u32Oper), std::rotr(u32Data[1], u32Oper),
				std::rotr(u32Data[2], u32Oper), std::rotr(u32Data[3], u32Oper), std::rotr(u32Data[4], u32Oper),
				std::rotr(u32Data[5], u32Oper), std::rotr(u32Data[6], u32Oper), std::rotr(u32Data[7], u32Oper));
			break;
		case OPER_BITREV:
			m256iData = _mm256_setr_epi32(ut::BitReverse(u32Data[0]), ut::BitReverse(u32Data[1]), ut::BitReverse(u32Data[2]),
				ut::BitReverse(u32Data[3]), ut::BitReverse(u32Data[4]), ut::BitReverse(u32Data[5]), ut::BitReverse(u32Data[6]),
				ut::BitReverse(u32Data[7]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported uint32_t operation.");
			break;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m256iData = ByteSwapVec<std::uint32_t>(m256iData);
		}

		_mm256_storeu_si256(reinterpret_cast<__m256i*>(pData), m256iData);
	};

	void OperVec256_i64(std::byte* pData, const HEXMODIFY& hms) {
		auto m256iData = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pData));
		if (hms.fBigEndian) {
			m256iData = ByteSwapVec<std::int64_t>(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(pData)));
		}

		alignas(32) std::int64_t i64Data[4];
		_mm256_store_si256(reinterpret_cast<__m256i*>(i64Data), m256iData);
		assert(!hms.spnData.empty());
		const auto i64Oper = *reinterpret_cast<const std::int64_t*>(hms.spnData.data());
		const auto m256iOper = _mm256_set1_epi64x(i64Oper);

		using enum EHexOperMode;
		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m256iData = _mm256_add_epi64(m256iData, m256iOper);
			break;
		case OPER_SUB:
			m256iData = _mm256_sub_epi64(m256iData, m256iOper);
			break;
		case OPER_MUL:
			m256iData = _mm256_setr_epi64x(i64Data[0] * i64Oper, i64Data[1] * i64Oper, i64Data[2] * i64Oper,
				i64Data[3] * i64Oper);
			break;
		case OPER_DIV:
			assert(i64Oper > 0);
			m256iData = _mm256_setr_epi64x(i64Data[0] / i64Oper, i64Data[1] / i64Oper, i64Data[2] / i64Oper,
				i64Data[3] / i64Oper);
			break;
		case OPER_MIN:
			m256iData = _mm256_setr_epi64x((std::max)(i64Data[0], i64Oper), (std::max)(i64Data[1], i64Oper),
				(std::max)(i64Data[2], i64Oper), (std::max)(i64Data[3], i64Oper));
			break;
		case OPER_MAX:
			m256iData = _mm256_setr_epi64x((std::min)(i64Data[0], i64Oper), (std::min)(i64Data[1], i64Oper),
				(std::min)(i64Data[2], i64Oper), (std::min)(i64Data[3], i64Oper));
			break;
		case OPER_SWAP:
			m256iData = ByteSwapVec<std::int64_t>(m256iData);
			break;
		case OPER_OR:
			m256iData = _mm256_or_si256(m256iData, m256iOper);
			break;
		case OPER_XOR:
			m256iData = _mm256_xor_si256(m256iData, m256iOper);
			break;
		case OPER_AND:
			m256iData = _mm256_and_si256(m256iData, m256iOper);
			break;
		case OPER_NOT:
			//_mm256_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
			m256iData = _mm256_xor_si256(m256iData, _mm256_cmpeq_epi64(m256iData, m256iData));
			break;
		case OPER_SHL:
			m256iData = _mm256_slli_epi64(m256iData, static_cast<int>(i64Oper));
			break;
		case OPER_SHR:
			m256iData = _mm256_setr_epi64x(i64Data[0] >> i64Oper, i64Data[1] >> i64Oper, i64Data[2] >> i64Oper,
				i64Data[3] >> i64Oper);
			break;
		case OPER_ROTL:
			m256iData = _mm256_setr_epi64x(std::rotl(static_cast<std::uint64_t>(i64Data[0]), static_cast<const int>(i64Oper)),
					std::rotl(static_cast<std::uint64_t>(i64Data[1]), static_cast<const int>(i64Oper)),
					std::rotl(static_cast<std::uint64_t>(i64Data[2]), static_cast<const int>(i64Oper)),
					std::rotl(static_cast<std::uint64_t>(i64Data[3]), static_cast<const int>(i64Oper)));
			break;
		case OPER_ROTR:
			m256iData = _mm256_setr_epi64x(std::rotr(static_cast<std::uint64_t>(i64Data[0]), static_cast<const int>(i64Oper)),
				std::rotr(static_cast<std::uint64_t>(i64Data[1]), static_cast<const int>(i64Oper)),
				std::rotr(static_cast<std::uint64_t>(i64Data[2]), static_cast<const int>(i64Oper)),
				std::rotr(static_cast<std::uint64_t>(i64Data[3]), static_cast<const int>(i64Oper)));
			break;
		case OPER_BITREV:
			m256iData = _mm256_setr_epi64x(ut::BitReverse(i64Data[0]), ut::BitReverse(i64Data[1]), ut::BitReverse(i64Data[2]),
				ut::BitReverse(i64Data[3]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported int64_t operation.");
			break;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m256iData = ByteSwapVec<std::int64_t>(m256iData);
		}

		_mm256_storeu_si256(reinterpret_cast<__m256i*>(pData), m256iData);
	}

	void OperVec256_u64(std::byte* pData, const HEXMODIFY& hms) {
		auto m256iData = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pData));
		if (hms.fBigEndian) {
			m256iData = ByteSwapVec<std::uint64_t>(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(pData)));
		}

		alignas(32) std::uint64_t u64Data[4];
		_mm256_store_si256(reinterpret_cast<__m256i*>(u64Data), m256iData);
		assert(!hms.spnData.empty());
		const auto u64Oper = *reinterpret_cast<const std::uint64_t*>(hms.spnData.data());
		const auto m256iOper = _mm256_set1_epi64x(u64Oper);

		using enum EHexOperMode;
		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m256iData = _mm256_add_epi64(m256iData, m256iOper);
			break;
		case OPER_SUB:
			m256iData = _mm256_sub_epi64(m256iData, m256iOper);
			break;
		case OPER_MUL:
			m256iData = _mm256_setr_epi64x(u64Data[0] * u64Oper, u64Data[1] * u64Oper, u64Data[2] * u64Oper,
				u64Data[3] * u64Oper);
			break;
		case OPER_DIV:
			assert(u64Oper > 0);
			m256iData = _mm256_setr_epi64x(u64Data[0] / u64Oper, u64Data[1] / u64Oper, u64Data[2] / u64Oper,
				u64Data[3] / u64Oper);
			break;
		case OPER_MIN:
			m256iData = _mm256_setr_epi64x((std::max)(u64Data[0], u64Oper), (std::max)(u64Data[1], u64Oper),
				(std::max)(u64Data[2], u64Oper), (std::max)(u64Data[3], u64Oper));
			break;
		case OPER_MAX:
			m256iData = _mm256_setr_epi64x((std::min)(u64Data[0], u64Oper), (std::min)(u64Data[1], u64Oper),
				 (std::min)(u64Data[2], u64Oper), (std::min)(u64Data[3], u64Oper));
			break;
		case OPER_SWAP:
			m256iData = ByteSwapVec<std::uint64_t>(m256iData);
			break;
		case OPER_OR:
			m256iData = _mm256_or_si256(m256iData, m256iOper);
			break;
		case OPER_XOR:
			m256iData = _mm256_xor_si256(m256iData, m256iOper);
			break;
		case OPER_AND:
			m256iData = _mm256_and_si256(m256iData, m256iOper);
			break;
		case OPER_NOT:
			//_mm256_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
			m256iData = _mm256_xor_si256(m256iData, _mm256_cmpeq_epi64(m256iData, m256iData));
			break;
		case OPER_SHL:
			m256iData = _mm256_slli_epi64(m256iData, static_cast<int>(u64Oper));
			break;
		case OPER_SHR:
			m256iData = _mm256_srli_epi64(m256iData, static_cast<int>(u64Oper)); //Logical shift.
			break;
		case OPER_ROTL:
			m256iData = _mm256_setr_epi64x(std::rotl(u64Data[0], static_cast<const int>(u64Oper)),
				std::rotl(u64Data[1], static_cast<const int>(u64Oper)),
				std::rotl(u64Data[2], static_cast<const int>(u64Oper)),
				std::rotl(u64Data[3], static_cast<const int>(u64Oper)));
			break;
		case OPER_ROTR:
			m256iData = _mm256_setr_epi64x(std::rotr(u64Data[0], static_cast<const int>(u64Oper)),
				std::rotr(u64Data[1], static_cast<const int>(u64Oper)),
				std::rotr(u64Data[2], static_cast<const int>(u64Oper)),
				std::rotr(u64Data[3], static_cast<const int>(u64Oper)));
			break;
		case OPER_BITREV:
			m256iData = _mm256_setr_epi64x(ut::BitReverse(u64Data[0]), ut::BitReverse(u64Data[1]), ut::BitReverse(u64Data[2]),
				ut::BitReverse(u64Data[3]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported uint64_t operation.");
			break;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m256iData = ByteSwapVec<std::uint64_t>(m256iData);
		}

		_mm256_storeu_si256(reinterpret_cast<__m256i*>(pData), m256iData);
	}

	void OperVec256_f(std::byte* pData, const HEXMODIFY& hms) {
		auto m256Data = _mm256_loadu_ps(reinterpret_cast<const float*>(pData));
		if (hms.fBigEndian) {
			m256Data = ByteSwapVec<float>(_mm256_loadu_ps(reinterpret_cast<const float*>(pData)));
		}

		assert(std::isfinite(*reinterpret_cast<const float*>(hms.spnData.data())));
		const auto m256Oper = _mm256_set1_ps(*reinterpret_cast<const float*>(hms.spnData.data()));

		using enum EHexOperMode;
		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m256Data = _mm256_add_ps(m256Data, m256Oper);
			break;
		case OPER_SUB:
			m256Data = _mm256_sub_ps(m256Data, m256Oper);
			break;
		case OPER_MUL:
			m256Data = _mm256_mul_ps(m256Data, m256Oper);
			break;
		case OPER_DIV:
			assert(*reinterpret_cast<const float*>(hms.spnData.data()) > 0.F);
			m256Data = _mm256_div_ps(m256Data, m256Oper);
			break;
		case OPER_MIN:
			m256Data = _mm256_max_ps(m256Data, m256Oper);
			break;
		case OPER_MAX:
			m256Data = _mm256_min_ps(m256Data, m256Oper);
			break;
		case OPER_SWAP:
			m256Data = ByteSwapVec<float>(m256Data);
			break;
		default:
			ut::DBG_REPORT(L"Unsupported float operation.");
			return;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m256Data = ByteSwapVec<float>(m256Data);
		}

		_mm256_storeu_ps(reinterpret_cast<float*>(pData), m256Data);
	}

	void OperVec256_d(std::byte* pData, const HEXMODIFY& hms) {
		auto m256dData = _mm256_loadu_pd(reinterpret_cast<const double*>(pData));
		if (hms.fBigEndian) {
			m256dData = ByteSwapVec<double>(_mm256_loadu_pd(reinterpret_cast<const double*>(pData)));
		}

		assert(std::isfinite(*reinterpret_cast<const double*>(hms.spnData.data())));
		const auto m256dOper = _mm256_set1_pd(*reinterpret_cast<const double*>(hms.spnData.data()));

		using enum EHexOperMode;
		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m256dData = _mm256_add_pd(m256dData, m256dOper);
			break;
		case OPER_SUB:
			m256dData = _mm256_sub_pd(m256dData, m256dOper);
			break;
		case OPER_MUL:
			m256dData = _mm256_mul_pd(m256dData, m256dOper);
			break;
		case OPER_DIV:
			assert(*reinterpret_cast<const double*>(hms.spnData.data()) > 0.);
			m256dData = _mm256_div_pd(m256dData, m256dOper);
			break;
		case OPER_MIN:
			m256dData = _mm256_max_pd(m256dData, m256dOper);
			break;
		case OPER_MAX:
			m256dData = _mm256_min_pd(m256dData, m256dOper);
			break;
		case OPER_SWAP:
			m256dData = ByteSwapVec<double>(m256dData);
			break;
		default:
			ut::DBG_REPORT(L"Unsupported double operation.");
			return;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m256dData = ByteSwapVec<double>(m256dData);
		}

		_mm256_storeu_pd(reinterpret_cast<double*>(pData), m256dData);
	}

	template<EVecType eVecType>
	void ModifyOperVec(std::byte* pData, const HEXMODIFY& hms, [[maybe_unused]] SpanCByte)
	{
		assert(pData != nullptr);
		using enum EHexDataType;

		if constexpr (eVecType == EVecType::VECTOR_128) {
			switch (hms.eDataType) {
			case DATA_INT8:
				OperVec128_i8(pData, hms);
				break;
			case DATA_UINT8:
				OperVec128_u8(pData, hms);
				break;
			case DATA_INT16:
				OperVec128_i16(pData, hms);
				break;
			case DATA_UINT16:
				OperVec128_u16(pData, hms);
				break;
			case DATA_INT32:
				OperVec128_i32(pData, hms);
				break;
			case DATA_UINT32:
				OperVec128_u32(pData, hms);
				break;
			case DATA_INT64:
				OperVec128_i64(pData, hms);
				break;
			case DATA_UINT64:
				OperVec128_u64(pData, hms);
				break;
			case DATA_FLOAT:
				OperVec128_f(pData, hms);
				break;
			case DATA_DOUBLE:
				OperVec128_d(pData, hms);
				break;
			default:
				break;
			}
		}
		else if constexpr (eVecType == EVecType::VECTOR_256) {
			switch (hms.eDataType) {
			case DATA_INT8:
				OperVec256_i8(pData, hms);
				break;
			case DATA_UINT8:
				OperVec256_u8(pData, hms);
				break;
			case DATA_INT16:
				OperVec256_i16(pData, hms);
				break;
			case DATA_UINT16:
				OperVec256_u16(pData, hms);
				break;
			case DATA_INT32:
				OperVec256_i32(pData, hms);
				break;
			case DATA_UINT32:
				OperVec256_u32(pData, hms);
				break;
			case DATA_INT64:
				OperVec256_i64(pData, hms);
				break;
			case DATA_UINT64:
				OperVec256_u64(pData, hms);
				break;
			case DATA_FLOAT:
				OperVec256_f(pData, hms);
				break;
			case DATA_DOUBLE:
				OperVec256_d(pData, hms);
				break;
			default:
				break;
			}
		}
	}
#elif defined(_M_ARM64) //^^^ _M_IX86 || _M_X64 / vvv _M_ARM64
	template<typename T>
	void OperScalarT(T* pData, const HEXMODIFY& hms) {
		constexpr auto uChunksData = 16U / sizeof(T);
		if (hms.fBigEndian) {
			for (auto i = 0U; i < uChunksData; ++i) {
				pData[i] = ut::ByteSwap(pData[i]);
			}
		}

		assert(!hms.spnData.empty());
		const T tOper = *reinterpret_cast<const T*>(hms.spnData.data());

		using enum EHexOperMode;
		if constexpr (std::is_integral_v<T>) { //Operations only for integral types.
			switch (hms.eOperMode) {
			case OPER_OR:
				for (auto i = 0U; i < uChunksData; ++i) {
					pData[i] |= tOper;
				}
				break;
			case OPER_XOR:
				for (auto i = 0U; i < uChunksData; ++i) {
					pData[i] ^= tOper;
				}
				break;
			case OPER_AND:
				for (auto i = 0U; i < uChunksData; ++i) {
					pData[i] &= tOper;
				}
				break;
			case OPER_NOT:
				for (auto i = 0U; i < uChunksData; ++i) {
					pData[i] = ~pData[i];
				}
				break;
			case OPER_SHL:
				for (auto i = 0U; i < uChunksData; ++i) {
					pData[i] <<= tOper;
				}
				break;
			case OPER_SHR:
				for (auto i = 0U; i < uChunksData; ++i) {
					pData[i] >>= tOper;
				}
				break;
			case OPER_ROTL:
				for (auto i = 0U; i < uChunksData; ++i) {
					pData[i] = std::rotl(static_cast<std::make_unsigned_t<T>>(pData[i]), static_cast<const int>(tOper));
				}
				break;
			case OPER_ROTR:
				for (auto i = 0U; i < uChunksData; ++i) {
					pData[i] = std::rotr(static_cast<std::make_unsigned_t<T>>(pData[i]), static_cast<const int>(tOper));
				}
				break;
			case OPER_BITREV:
				for (auto i = 0U; i < uChunksData; ++i) {
					pData[i] = ut::BitReverse(pData[i]);
				}
				break;
			default:
				break;
			}
		}

		switch (hms.eOperMode) { //Operations for integral and floating types.
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			for (auto i = 0U; i < uChunksData; ++i) {
				pData[i] += tOper;
			}
			break;
		case OPER_SUB:
			for (auto i = 0U; i < uChunksData; ++i) {
				pData[i] -= tOper;
			}
			break;
		case OPER_MUL:
			for (auto i = 0U; i < uChunksData; ++i) {
				pData[i] *= tOper;
			}
			break;
		case OPER_DIV:
			assert(tOper > 0);
			for (auto i = 0U; i < uChunksData; ++i) {
				pData[i] /= tOper;
			}
			break;
		case OPER_MIN:
			for (auto i = 0U; i < uChunksData; ++i) {
				if constexpr (std::is_floating_point_v<T>) {
					pData[i] = std::fmax(pData[i], tOper);
				}
				else {
					pData[i] = (std::max)(pData[i], tOper);
				}
			}
			break;
		case OPER_MAX:
			for (auto i = 0U; i < uChunksData; ++i) {
				if constexpr (std::is_floating_point_v<T>) {
					pData[i] = std::fmin(pData[i], tOper);
				}
				else {
					pData[i] = (std::min)(pData[i], tOper);
				}
			}
			break;
		case OPER_SWAP:
			for (auto i = 0U; i < uChunksData; ++i) {
				pData[i] = ut::ByteSwap(pData[i]);
			}
			break;
		default:
			break;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			for (auto i = 0U; i < uChunksData; ++i) {
				pData[i] = ut::ByteSwap(pData[i]);
			}
		}
	}

	template<EVecType eVecType>
	void ModifyOperVec(std::byte* pData, const HEXMODIFY& hms, [[maybe_unused]] SpanCByte)
	{
		assert(pData != nullptr);
		using enum EHexDataType;

		switch (hms.eDataType) {
		case DATA_INT8:
			OperScalarT(reinterpret_cast<std::int8_t*>(pData), hms);
			break;
		case DATA_UINT8:
			OperScalarT(reinterpret_cast<std::uint8_t*>(pData), hms);
			break;
		case DATA_INT16:
			OperScalarT(reinterpret_cast<std::int16_t*>(pData), hms);
			break;
		case DATA_UINT16:
			OperScalarT(reinterpret_cast<std::uint16_t*>(pData), hms);
			break;
		case DATA_INT32:
			OperScalarT(reinterpret_cast<std::int32_t*>(pData), hms);
			break;
		case DATA_UINT32:
			OperScalarT(reinterpret_cast<std::uint32_t*>(pData), hms);
			break;
		case DATA_INT64:
			OperScalarT(reinterpret_cast<std::int64_t*>(pData), hms);
			break;
		case DATA_UINT64:
			OperScalarT(reinterpret_cast<std::uint64_t*>(pData), hms);
			break;
		case DATA_FLOAT:
			OperScalarT(reinterpret_cast<float*>(pData), hms);
			break;
		case DATA_DOUBLE:
			OperScalarT(reinterpret_cast<double*>(pData), hms);
			break;
		default:
			break;
		}
	}
#endif //^^^ _M_ARM64
}