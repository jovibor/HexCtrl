/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/						    *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* This is a Hex control for MFC apps, implemented as CWnd derived class.			    *
****************************************************************************************/
/****************************************************************************************
* These are some helper functions for HexCtrl.											*
****************************************************************************************/
#include "stdafx.h"
#include "Helper.h"

namespace HEXCTRL {

	void UllToWchars(ULONGLONG ull, wchar_t* pwsz, size_t dwSize)
	{
		const wchar_t* const pwszHexMap { L"0123456789ABCDEF" };

		//Converts dwSize bytes of ull to wchar_t*.
		for (size_t i = 0; i < dwSize; i++)
		{
			pwsz[i * 2] = pwszHexMap[((ull >> ((dwSize - 1 - i) << 3)) & 0xF0) >> 4];
			pwsz[i * 2 + 1] = pwszHexMap[(ull >> ((dwSize - 1 - i) << 3)) & 0x0F];
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

	std::string WstrToStr(const std::wstring & wstr)
	{
		int iSize = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), nullptr, 0, nullptr, nullptr);
		std::string str(iSize, 0);
		WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], iSize, nullptr, nullptr);

		return str;
	}

	bool NumStrToHex(const std::string & strNum, std::string & strHex)
	{
		size_t dwIterations = strNum.size() / 2 + strNum.size() % 2;

		for (size_t i = 0; i < dwIterations; i++)
		{
			std::string strToUL; //String to hold currently extracted two letters.

			if (i + 2 <= strNum.size())
				strToUL = strNum.substr(i * 2, 2);
			else
				strToUL = strNum.substr(i * 2, 1);

			unsigned long ulNumber;
			if (!CharsToUl(strToUL.data(), ulNumber))
				return false;

			strHex += (unsigned char)ulNumber;
		}

		return true;
	}
}