/****************************************************************************************
* Copyright � 2018-2020 Jovibor https://github.com/jovibor/                             *
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
		if (ul == 0 && (pEndPtr == pcsz || *pEndPtr != '\0'))
			return false;

		return true;
	}

	bool WCharsToUll(const wchar_t* pwcsz, unsigned long long& ull, bool fHex)
	{
		wchar_t* pEndPtr;
		int iRadix = fHex ? 16 : 10;
		if ((wcsstr(pwcsz, L"0x") == pwcsz) || (wcsstr(pwcsz, L"0X") == pwcsz))
			iRadix = 16; //NB: Avoid radix=0 because this may be treated as octal
		ull = std::wcstoull(pwcsz, &pEndPtr, iRadix);
		if ((ull == 0 && (pEndPtr == pwcsz || *pEndPtr != '\0'))
			|| (ull == ULLONG_MAX && errno == ERANGE))
			return false;

		return true;
	}

	bool WCharsToll(const wchar_t* pwcsz, long long& ll, bool fHex)
	{
		wchar_t* pEndPtr;
		int iRadix = fHex ? 16 : 10;
		if ((wcsstr(pwcsz, L"0x") == pwcsz) || (wcsstr(pwcsz, L"0X") == pwcsz))
			iRadix = 16; //NB: Avoid radix=0 because this may be treated as octal
		ll = std::wcstoll(pwcsz, &pEndPtr, iRadix);
		if ((ll == 0 && (pEndPtr == pwcsz || *pEndPtr != '\0'))
			|| ((ll == LLONG_MAX || ll == LLONG_MIN) && errno == ERANGE))
			return false;

		return true;
	}

	std::string WstrToStr(std::wstring_view wstr)
	{
		int iSize = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
		std::string str(iSize, 0);
		WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), &str[0], iSize, nullptr, nullptr);

		return str;
	}

	std::wstring StrToWstr(std::string_view str)
	{
		int iSize = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0);
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