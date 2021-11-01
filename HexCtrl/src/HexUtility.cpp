/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "HexUtility.h"
#include <algorithm>
#include <cwctype>
#include <format>
#include <limits>

namespace HEXCTRL::INTERNAL
{
	void UllToWchars(ULONGLONG ull, wchar_t* pwsz, size_t dwSize, bool fAsHex)
	{
		if (fAsHex)
		{
			//Converts dwSize bytes of ull to wchar_t*.
			for (size_t i = 0; i < dwSize; ++i)
			{
				pwsz[i * 2] = g_pwszHexMap[((ull >> ((dwSize - 1 - i) << 3)) >> 4) & 0x0F];
				pwsz[i * 2 + 1] = g_pwszHexMap[(ull >> ((dwSize - 1 - i) << 3)) & 0x0F];
			}
		}
		else
		{
			std::wstring_view wstrFormat { };
			switch (dwSize)
			{
			case 2:
				wstrFormat = L"%05llu";
				break;
			case 3:
				wstrFormat = L"%08llu";
				break;
			case 4:
				wstrFormat = L"%010llu";
				break;
			case 5:
				wstrFormat = L"%013llu";
				break;
			case 6:
				wstrFormat = L"%015llu";
				break;
			case 7:
				wstrFormat = L"%017llu";
				break;
			case 8:
				wstrFormat = L"%019llu";
				break;
			default:
				break;
			}
			swprintf_s(pwsz, 32, wstrFormat.data(), ull);
		}
	}

	template<typename T>
	bool wstr2num(const std::wstring& wstr, T& tData, int iBase)
	{
		wchar_t* pEndPtr;
		if constexpr (std::is_same_v<T, ULONGLONG>)
		{
			tData = std::wcstoull(wstr.data(), &pEndPtr, iBase);
			if ((tData == 0 && (pEndPtr == wstr.data() || *pEndPtr != '\0'))
				|| (tData == ULLONG_MAX && errno == ERANGE))
				return false;
		}
		else if constexpr (std::is_same_v<T, float>)
		{
			tData = wcstof(wstr.data(), &pEndPtr);
			if (tData == 0 && (pEndPtr == wstr.data() || *pEndPtr != '\0'))
				return false;
		}
		else if constexpr (std::is_same_v<T, double>)
		{
			tData = wcstod(wstr.data(), &pEndPtr);
			if (tData == 0 && (pEndPtr == wstr.data() || *pEndPtr != '\0'))
				return false;
		}
		else
		{
			const auto llData = std::wcstoll(wstr.data(), &pEndPtr, iBase);
			if ((llData == 0 && (pEndPtr == wstr.data() || *pEndPtr != L'\0'))
				|| ((llData == LLONG_MAX || llData == LLONG_MIN) && errno == ERANGE)
				|| (llData > static_cast<LONGLONG>((std::numeric_limits<T>::max)()))
				|| (llData < static_cast<LONGLONG>((std::numeric_limits<std::make_signed_t<T>>::min)())))
				return false;
			tData = static_cast<T>(llData);
		}

		return true;
	}
	//Explicit instantiations of templated func in .cpp.
	template bool wstr2num<CHAR>(const std::wstring& wstr, CHAR& t, int iBase);
	template bool wstr2num<UCHAR>(const std::wstring& wstr, UCHAR& t, int iBase);
	template bool wstr2num<SHORT>(const std::wstring& wstr, SHORT& t, int iBase);
	template bool wstr2num<USHORT>(const std::wstring& wstr, USHORT& t, int iBase);
	template bool wstr2num<LONG>(const std::wstring& wstr, LONG& t, int iBase);
	template bool wstr2num<ULONG>(const std::wstring& wstr, ULONG& t, int iBase);
	template bool wstr2num<INT>(const std::wstring& wstr, INT& t, int iBase);
	template bool wstr2num<UINT>(const std::wstring& wstr, UINT& t, int iBase);
	template bool wstr2num<LONGLONG>(const std::wstring& wstr, LONGLONG& t, int iBase);
	template bool wstr2num<ULONGLONG>(const std::wstring& wstr, ULONGLONG& t, int iBase);
	template bool wstr2num<float>(const std::wstring& wstr, float& t, int iBase);
	template bool wstr2num<double>(const std::wstring& wstr, double& t, int iBase);

	template<typename T>
	bool str2num(const std::string& str, T& tData, int iBase)
	{
		char* pEndPtr;
		if constexpr (std::is_same_v<T, ULONGLONG>)
		{
			tData = std::strtoull(str.data(), &pEndPtr, iBase);
			if ((tData == 0 && (pEndPtr == str.data() || *pEndPtr != '\0'))
				|| (tData == ULLONG_MAX && errno == ERANGE))
				return false;
		}
		else
		{
			const auto llData = std::strtoll(str.data(), &pEndPtr, iBase);
			if ((llData == 0 && (pEndPtr == str.data() || *pEndPtr != '\0'))
				|| ((llData == LLONG_MAX || llData == LLONG_MIN) && errno == ERANGE)
				|| (llData > static_cast<LONGLONG>((std::numeric_limits<T>::max)()))
				|| (llData < static_cast<LONGLONG>((std::numeric_limits<std::make_signed_t<T>>::min)()))
				)
				return false;
			tData = static_cast<T>(llData);
		}

		return true;
	}
	//Explicit instantiations of templated func in .cpp.
	template bool str2num<UCHAR>(const std::string& str, UCHAR& t, int iBase);

	bool str2hex(std::string_view strIn, std::string& strToHex, bool fWc, char chWc)
	{
		std::string strFilter = "0123456789AaBbCcDdEeFf"; //Allowed characters.
		if (fWc)
			strFilter += chWc;
		if (strIn.find_first_not_of(strFilter) != std::string_view::npos)
			return false;

		std::string strTmp;
		for (auto iterBegin = strIn.begin(); iterBegin != strIn.end();)
		{
			if (fWc && *iterBegin == chWc) //Skip wildcard.
			{
				++iterBegin;
				strTmp += chWc;
				continue;
			}

			//Extract two current chars and pass it to str2num as string.
			const size_t nOffsetCurr = iterBegin - strIn.begin();
			const auto nSize = nOffsetCurr + 2 <= strIn.size() ? 2 : 1;
			if (unsigned char chNumber;	str2num(std::string(strIn.substr(nOffsetCurr, nSize)), chNumber, 16))
			{
				iterBegin += nSize;
				strTmp += chNumber;
			}
			else
				return false;
		}
		strToHex = std::move(strTmp);

		return true;
	}

	std::string wstr2str(std::wstring_view wstr, UINT uCodePage)
	{
		const auto iSize = WideCharToMultiByte(uCodePage, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
		std::string str(iSize, 0);
		WideCharToMultiByte(uCodePage, 0, wstr.data(), static_cast<int>(wstr.size()), &str[0], iSize, nullptr, nullptr);

		return str;
	}

	std::wstring str2wstr(std::string_view str, UINT uCodePage)
	{
		const auto iSize = MultiByteToWideChar(uCodePage, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);
		std::wstring wstr(iSize, 0);
		MultiByteToWideChar(uCodePage, 0, str.data(), static_cast<int>(str.size()), &wstr[0], iSize);

		return wstr;
	}

	void ReplaceUnprintable(std::wstring& wstr, bool fASCII, bool fCRLF)
	{
		//If fASCII is true, then only wchars in 0x1F<...<0x7F range are considered printable.
		//If fCRLF is false, then CR(0x0D) and LF(0x0A) wchars remain untouched.
		if (fASCII)
		{
			std::replace_if(wstr.begin(), wstr.end(), [=](wchar_t wch) //All non ASCII.
				{return (wch <= 0x1F || wch >= 0x7F) && (fCRLF || (wch != 0x0D && wch != 0x0A)); }, L'.');
		}
		else
		{
			std::replace_if(wstr.begin(), wstr.end(), [=](wchar_t wch) //All non printable wchars.
				{return !std::iswprint(wch) && (fCRLF || (wch != 0x0D && wch != 0x0A)); }, L'.');
		}
	}

	auto StringToFileTime(std::wstring_view wstr, DWORD dwDateFormat)->std::optional<FILETIME>
	{
		std::optional<FILETIME> optFT { std::nullopt };
		if (auto optSysTime = StringToSystemTime(wstr, dwDateFormat); optSysTime)
		{
			FILETIME ftTime;
			if (SystemTimeToFileTime(&optSysTime.value(), &ftTime) != FALSE)
			{
				optFT = ftTime;
			}
		}

		return optFT;
	}

	auto StringToSystemTime(const std::wstring_view wstr, const DWORD dwDateFormat)->std::optional<SYSTEMTIME>
	{
		//dwDateFormat is a locale specific date format https://docs.microsoft.com/en-gb/windows/win32/intl/locale-idate

		if (wstr.empty())
			return std::nullopt;

		//Normalise the input string by replacing non-numeric characters except space with a `/`.
		//This should regardless of the current date/time separator character.
		std::wstring wstrDateTimeCooked { };
		for (const auto iter : wstr)
			wstrDateTimeCooked += (iswdigit(iter) || iter == L' ') ? iter : L'/';

		SYSTEMTIME stSysTime { };
		int iParsedArgs { };

		//Parse date component
		switch (dwDateFormat)
		{
		case 0:	//Month-Day-Year
			iParsedArgs = swscanf_s(wstrDateTimeCooked.data(), L"%2hu/%2hu/%4hu", &stSysTime.wMonth, &stSysTime.wDay, &stSysTime.wYear);
			break;
		case 1: //Day-Month-Year (default)
			iParsedArgs = swscanf_s(wstrDateTimeCooked.data(), L"%2hu/%2hu/%4hu", &stSysTime.wDay, &stSysTime.wMonth, &stSysTime.wYear);
			break;
		case 2:	//Year-Month-Day 
			iParsedArgs = swscanf_s(wstrDateTimeCooked.data(), L"%4hu/%2hu/%2hu", &stSysTime.wYear, &stSysTime.wMonth, &stSysTime.wDay);
			break;
		default:
			assert(true);
			return std::nullopt;
		}
		if (iParsedArgs != 3)
			return std::nullopt;

		//Find time seperator (if present).
		if (const auto nPos = wstrDateTimeCooked.find(L' '); nPos != std::wstring::npos)
			wstrDateTimeCooked = wstrDateTimeCooked.substr(nPos + 1);

		//Parse time component.
		if (swscanf_s(wstrDateTimeCooked.data(), L"%2hu/%2hu/%2hu/%3hu", &stSysTime.wHour, &stSysTime.wMinute,
			&stSysTime.wSecond, &stSysTime.wMilliseconds) != 4)
			return std::nullopt;

		return stSysTime;
	}

	auto FileTimeToString(const FILETIME& stFileTime, DWORD dwDateFormat)->std::wstring
	{
		std::wstring wstrTime;
		if (SYSTEMTIME stSysTime { }; FileTimeToSystemTime(&stFileTime, &stSysTime) != FALSE)
			wstrTime = SystemTimeToString(stSysTime, dwDateFormat);

		return wstrTime;
	}

	auto SystemTimeToString(const SYSTEMTIME & stSysTime, DWORD dwDateFormat) -> std::wstring
	{
		if (dwDateFormat > 2 || stSysTime.wDay == 0 || stSysTime.wDay > 31 || stSysTime.wMonth == 0 || stSysTime.wMonth > 12
			|| stSysTime.wYear > 9999 || stSysTime.wHour > 23 || stSysTime.wMinute > 59 || stSysTime.wSecond > 59
			|| stSysTime.wMilliseconds > 999)
			return L"N/A";

		std::wstring_view wstrFmt;
		switch (dwDateFormat)
		{
		case 0:	//0:Month/Day/Year HH:MM:SS.mmm
			wstrFmt = L"{1:02d}/{0:02d}/{2} {3:02d}:{4:02d}:{5:02d}.{6:03d}";
			break;
		case 1: //1:Day/Month/Year HH:MM:SS.mmm
			wstrFmt = L"{0:02d}/{1:02d}/{2} {3:02d}:{4:02d}:{5:02d}.{6:03d}";
			break;
		case 2: //2:Year/Month/Day HH:MM:SS.mmm
			wstrFmt = L"{2}/{1:02d}/{0:02d} {3:02d}:{4:02d}:{5:02d}.{6:03d}";
			break;
		}

		return std::format(wstrFmt, stSysTime.wDay, stSysTime.wMonth, stSysTime.wYear,
			stSysTime.wHour, stSysTime.wMinute, stSysTime.wSecond, stSysTime.wMilliseconds);
	}
}