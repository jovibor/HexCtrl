/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC applications.                                           *
* Official git repository of the project: https://github.com/jovibor/HexCtrl/           *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
/****************************************************************************************
* These are some helper functions for HexCtrl.											*
****************************************************************************************/
#include "stdafx.h"
#include "Helper.h"

#undef min
#undef max

namespace HEXCTRL::INTERNAL {

	void UllToWchars(ULONGLONG ull, wchar_t* pwsz, size_t dwSize, bool fAsHex)
	{
		if (fAsHex)
		{
			//Converts dwSize bytes of ull to wchar_t*.
			for (size_t i = 0; i < dwSize; i++)
			{
				pwsz[i * 2] = g_pwszHexMap[((ull >> ((dwSize - 1 - i) << 3)) >> 4) & 0x0F];
				pwsz[i * 2 + 1] = g_pwszHexMap[(ull >> ((dwSize - 1 - i) << 3)) & 0x0F];
			}
		}
		else
		{
			std::wstring wstrFormat;
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
			}
			swprintf_s(pwsz, 32, wstrFormat.data(), ull);
		}
	}

	bool CharsToUl(const char* pcsz, unsigned long& ul)
	{
		char* pEndPtr;
		ul = strtoul(pcsz, &pEndPtr, 16);
		return !(ul == 0 && (pEndPtr == pcsz || *pEndPtr != '\0'));
	}

	template<typename T>bool wstr2num(std::wstring_view wstr, T& tData, int iBase)
	{
		wchar_t* pEndPtr;
		if constexpr (std::is_same_v<T, ULONGLONG>)
		{
			tData = std::wcstoull(wstr.data(), &pEndPtr, iBase);
			if ((tData == 0 && (pEndPtr == wstr.data() || *pEndPtr != '\0'))
				|| (tData == ULLONG_MAX && errno == ERANGE))
				return false;
		}
		else
		{
			auto llData = std::wcstoll(wstr.data(), &pEndPtr, iBase);
			if ((llData == 0 && (pEndPtr == wstr.data() || *pEndPtr != '\0'))
				|| ((llData == LLONG_MAX || llData == LLONG_MIN) && errno == ERANGE)
				|| (llData > static_cast<LONGLONG>(std::numeric_limits<T>::max()))
				|| (llData < static_cast<LONGLONG>(std::numeric_limits<T>::min()))
				)
				return false;
			tData = static_cast<T>(llData);
		}

		return true;
	}
	//Explicit instantiations of templated func in .cpp.
	template bool wstr2num<CHAR>(std::wstring_view wstr, CHAR& t, int iBase);
	template bool wstr2num<UCHAR>(std::wstring_view wstr, UCHAR& t, int iBase);
	template bool wstr2num<SHORT>(std::wstring_view wstr, SHORT& t, int iBase);
	template bool wstr2num<USHORT>(std::wstring_view wstr, USHORT& t, int iBase);
	template bool wstr2num<LONG>(std::wstring_view wstr, LONG& t, int iBase);
	template bool wstr2num<ULONG>(std::wstring_view wstr, ULONG& t, int iBase);
	template bool wstr2num<LONGLONG>(std::wstring_view wstr, LONGLONG& t, int iBase);
	template bool wstr2num<ULONGLONG>(std::wstring_view wstr, ULONGLONG& t, int iBase);

	std::string WstrToStr(std::wstring_view wstr)
	{
		int iSize = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
		std::string str(iSize, 0);
		WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), &str[0], iSize, nullptr, nullptr);

		return str;
	}

	std::wstring StrToWstr(std::string_view str)
	{
		int iSize = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);
		std::wstring wstr(iSize, 0);
		MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), &wstr[0], iSize);
		return wstr;
	}

	bool StrToHex(const std::string& strFrom, std::string& strToHex)
	{
		size_t dwIterations = strFrom.size() / 2 + strFrom.size() % 2;
		std::string strTmp;
		for (size_t i = 0; i < dwIterations; i++)
		{
			std::string strToUL; //String to hold currently extracted two letters.

			if (i + 2 <= strFrom.size())
				strToUL = strFrom.substr(i * 2, 2);
			else
				strToUL = strFrom.substr(i * 2, 1);

			unsigned long ulNumber;
			if (!CharsToUl(strToUL.data(), ulNumber))
				return false;

			strTmp += static_cast<unsigned char>(ulNumber);
		}
		strToHex = std::move(strTmp);
		strToHex.shrink_to_fit();

		return true;
	}
}