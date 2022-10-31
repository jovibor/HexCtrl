/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "HexUtility.h"
#include <format>
#include <limits>

namespace HEXCTRL::INTERNAL
{
	auto NumStrToHex(std::wstring_view wsv, bool fWc, char chWc)->std::optional<std::string>
	{
		std::wstring wstrFilter = L"0123456789AaBbCcDdEeFf"; //Allowed characters.
		if (fWc)
			wstrFilter += chWc;

		if (wsv.find_first_not_of(wstrFilter) != std::string_view::npos)
			return std::nullopt;

		std::string strHexTmp;
		for (auto iterBegin = wsv.begin(); iterBegin != wsv.end();)
		{
			if (fWc && *iterBegin == chWc) //Skip wildcard.
			{
				++iterBegin;
				strHexTmp += chWc;
				continue;
			}

			//Extract two current wchars and pass it to StringToNum as wstring.
			const std::size_t nOffsetCurr = iterBegin - wsv.begin();
			const auto nSize = nOffsetCurr + 2 <= wsv.size() ? 2 : 1;
			if (const auto optNumber = StrToUChar(wsv.substr(nOffsetCurr, nSize), 16); optNumber)
			{
				iterBegin += nSize;
				strHexTmp += *optNumber;
			}
			else
				return std::nullopt;
		}

		return { std::move(strHexTmp) };
	}

	auto WstrToStr(std::wstring_view wsv, UINT uCodePage)->std::string
	{
		const auto iSize = WideCharToMultiByte(uCodePage, 0, wsv.data(), static_cast<int>(wsv.size()), nullptr, 0, nullptr, nullptr);
		std::string str(iSize, 0);
		WideCharToMultiByte(uCodePage, 0, wsv.data(), static_cast<int>(wsv.size()), &str[0], iSize, nullptr, nullptr);
		return str;
	}

	auto StrToWstr(std::string_view sv, UINT uCodePage)->std::wstring
	{
		const auto iSize = MultiByteToWideChar(uCodePage, 0, sv.data(), static_cast<int>(sv.size()), nullptr, 0);
		std::wstring wstr(iSize, 0);
		MultiByteToWideChar(uCodePage, 0, sv.data(), static_cast<int>(sv.size()), &wstr[0], iSize);
		return wstr;
	}

	auto StringToFileTime(std::wstring_view wsv, DWORD dwDateFormat)->std::optional<FILETIME>
	{
		std::optional<FILETIME> optFT { std::nullopt };
		if (auto optSysTime = StringToSystemTime(wsv, dwDateFormat); optSysTime)
		{
			FILETIME ftTime;
			if (SystemTimeToFileTime(&*optSysTime, &ftTime) != FALSE)
			{
				optFT = ftTime;
			}
		}

		return optFT;
	}

	auto StringToSystemTime(const std::wstring_view wsv, const DWORD dwDateFormat)->std::optional<SYSTEMTIME>
	{
		//dwDateFormat is a locale specific date format https://docs.microsoft.com/en-gb/windows/win32/intl/locale-idate

		if (wsv.empty())
			return std::nullopt;

		//Normalise the input string by replacing non-numeric characters except space with a `/`.
		//This should regardless of the current date/time separator character.
		std::wstring wstrDateTimeCooked { };
		for (const auto iter : wsv)
			wstrDateTimeCooked += (iswdigit(iter) || iter == L' ') ? iter : L'/';

		SYSTEMTIME stSysTime { };
		int iParsedArgs { };

		//Parse date component
		switch (dwDateFormat)
		{
		case 0:	//Month-Day-Year
			iParsedArgs = swscanf_s(wstrDateTimeCooked.data(), L"%2hu/%2hu/%4hu", &stSysTime.wMonth, &stSysTime.wDay, &stSysTime.wYear);
			break;
		case 1: //Day-Month-Year
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

		//Find time seperator, if present.
		if (const auto nPos = wstrDateTimeCooked.find(L' '); nPos != std::wstring::npos)
		{
			wstrDateTimeCooked = wstrDateTimeCooked.substr(nPos + 1);

			//Parse time component HH:MM:SS.mmm
			if (swscanf_s(wstrDateTimeCooked.data(), L"%2hu/%2hu/%2hu/%3hu", &stSysTime.wHour, &stSysTime.wMinute,
				&stSysTime.wSecond, &stSysTime.wMilliseconds) != 4)
				return std::nullopt;
		}

		return stSysTime;
	}

	auto FileTimeToString(FILETIME stFileTime, DWORD dwFormat, wchar_t wchSepar)->std::wstring
	{
		std::wstring wstrTime;
		if (SYSTEMTIME stSysTime { }; FileTimeToSystemTime(&stFileTime, &stSysTime) != FALSE)
			wstrTime = SystemTimeToString(stSysTime, dwFormat, wchSepar);
		else
			wstrTime = L"N/A";

		return wstrTime;
	}

	auto SystemTimeToString(SYSTEMTIME stSysTime, DWORD dwFormat, wchar_t wchSepar)->std::wstring
	{
		if (dwFormat > 2 || stSysTime.wDay == 0 || stSysTime.wDay > 31 || stSysTime.wMonth == 0 || stSysTime.wMonth > 12
			|| stSysTime.wYear > 9999 || stSysTime.wHour > 23 || stSysTime.wMinute > 59 || stSysTime.wSecond > 59
			|| stSysTime.wMilliseconds > 999)
			return L"N/A";

		std::wstring_view wstrFmt;
		switch (dwFormat)
		{
		case 0:	//0:Month/Day/Year HH:MM:SS.mmm
			wstrFmt = L"{1:02d}{7}{0:02d}{7}{2} {3:02d}:{4:02d}:{5:02d}.{6:03d}";
			break;
		case 1: //1:Day/Month/Year HH:MM:SS.mmm
			wstrFmt = L"{0:02d}{7}{1:02d}{7}{2} {3:02d}:{4:02d}:{5:02d}.{6:03d}";
			break;
		case 2: //2:Year/Month/Day HH:MM:SS.mmm
			wstrFmt = L"{2}{7}{1:02d}{7}{0:02d} {3:02d}:{4:02d}:{5:02d}.{6:03d}";
			break;
		}

		return std::vformat(wstrFmt, std::make_wformat_args(stSysTime.wDay, stSysTime.wMonth, stSysTime.wYear,
			stSysTime.wHour, stSysTime.wMinute, stSysTime.wSecond, stSysTime.wMilliseconds, wchSepar));
	}

	auto GetDateFormatString(DWORD dwDateFormat, wchar_t wcDateSeparator)->std::wstring
	{
		std::wstring_view wstrFmt;
		switch (dwDateFormat)
		{
		case 0:	//Month-Day-Year
			wstrFmt = L"MM{0}DD{0}YYYY HH:MM:SS.mmm";
			break;
		case 1: //Day-Month-Year
			wstrFmt = L"DD{0}MM{0}YYYY HH:MM:SS.mmm";
			break;
		case 2:	//Year-Month-Day
			wstrFmt = L"YYYY{0}MM{0}DD HH:MM:SS.mmm";
			break;
		default:
			assert(true);
			break;
		}
		return std::vformat(wstrFmt, std::make_wformat_args(wcDateSeparator));
	}
}