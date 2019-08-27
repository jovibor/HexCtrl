/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/                         *
* This is a Hex Control for MFC applications.                                           *
* Official git repository of the project: https://github.com/jovibor/HexCtrl/           *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
/****************************************************************************************
* These are some helper functions for HexCtrl.											*
****************************************************************************************/
#include "stdafx.h"
#include "Helper.h"

namespace HEXCTRL {
	namespace INTERNAL {

		void UllToWchars(ULONGLONG ull, wchar_t* pwsz, size_t dwSize)
		{
			//Converts dwSize bytes of ull to wchar_t*.
			for (size_t i = 0; i < dwSize; i++)
			{
				pwsz[i * 2] = g_pwszHexMap[((ull >> ((dwSize - 1 - i) << 3)) >> 4) & 0x0F];
				pwsz[i * 2 + 1] = g_pwszHexMap[(ull >> ((dwSize - 1 - i) << 3)) & 0x0F];
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

		std::string WstrToStr(const std::wstring& wstr)
		{
			int iSize = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), nullptr, 0, nullptr, nullptr);
			std::string str(iSize, 0);
			WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], iSize, nullptr, nullptr);

			return str;
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

				strTmp += (unsigned char)ulNumber;
			}
			strToHex = std::move(strTmp);
			strToHex.shrink_to_fit();

			return true;
		}
	}
}