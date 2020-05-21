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
#include <cwctype>

#undef min
#undef max

namespace HEXCTRL::INTERNAL {

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

	template<typename T>
	bool wstr2num(std::wstring_view wstr, T& tData, int iBase)
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
	template bool wstr2num<int>(std::wstring_view wstr, int& t, int iBase);
	template bool wstr2num<CHAR>(std::wstring_view wstr, CHAR& t, int iBase);
	template bool wstr2num<UCHAR>(std::wstring_view wstr, UCHAR& t, int iBase);
	template bool wstr2num<SHORT>(std::wstring_view wstr, SHORT& t, int iBase);
	template bool wstr2num<USHORT>(std::wstring_view wstr, USHORT& t, int iBase);
	template bool wstr2num<LONG>(std::wstring_view wstr, LONG& t, int iBase);
	template bool wstr2num<ULONG>(std::wstring_view wstr, ULONG& t, int iBase);
	template bool wstr2num<LONGLONG>(std::wstring_view wstr, LONGLONG& t, int iBase);
	template bool wstr2num<ULONGLONG>(std::wstring_view wstr, ULONGLONG& t, int iBase);

	template<typename T>
	bool str2num(std::string_view str, T& tData, int iBase)
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
			auto llData = std::strtoll(str.data(), &pEndPtr, iBase);
			if ((llData == 0 && (pEndPtr == str.data() || *pEndPtr != '\0'))
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
	template bool str2num<UCHAR>(std::string_view str, UCHAR& t, int iBase);

	bool StrToHex(std::string_view str, std::string& strToHex)
	{
		size_t dwIterations = str.size() / 2 + str.size() % 2;
		std::string strTmp;
		for (size_t i = 0; i < dwIterations; ++i)
		{
			std::string strToUL; //String to hold currently extracted two letters.

			if (i + 2 <= str.size())
				strToUL = str.substr(i * 2, 2);
			else
				strToUL = str.substr(i * 2, 1);

			unsigned char chNumber;
			if (!str2num(strToUL.data(), chNumber, 16))
				return false;

			strTmp += chNumber;
		}
		strToHex = std::move(strTmp);
		strToHex.shrink_to_fit();

		return true;
	}

	std::string WstrToStr(std::wstring_view wstr, UINT uCodePage)
	{
		auto iSize = WideCharToMultiByte(uCodePage, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
		std::string str(iSize, 0);
		WideCharToMultiByte(uCodePage, 0, wstr.data(), static_cast<int>(wstr.size()), &str[0], iSize, nullptr, nullptr);

		return str;
	}

	std::wstring StrToWstr(std::string_view str, UINT uCodePage)
	{
		auto iSize = MultiByteToWideChar(uCodePage, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);
		std::wstring wstr(iSize, 0);
		MultiByteToWideChar(uCodePage, 0, str.data(), static_cast<int>(str.size()), &wstr[0], iSize);

		return wstr;
	}

	void ReplaceUnprintable(std::wstring& wstr, bool fASCII, bool fCRLFRepl)
	{
		//If fASCII is true, then only wchars in 0x1F<...<0x7f range are considered printable.
		//If fCRLFRepl is false, then CR(0x0D) and LF(0x0A) wchars remain untouched.
		if (fASCII)
		{
			for (auto& iter : wstr)
				if ((iter <= 0x1F || iter >= 0x7f) && (fCRLFRepl || (iter != 0x0D && iter != 0x0A))) //All non ASCII.
					iter = L'.';
		}
		else
		{
			for (auto& iter : wstr)
				if (!std::iswprint(iter) && (fCRLFRepl || (iter != 0x0D && iter != 0x0A))) //All non printable wchars.
					iter = L'.';
		}
	}
}