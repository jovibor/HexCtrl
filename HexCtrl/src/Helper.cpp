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
#include <algorithm>
#include <cwctype>

#undef min
#undef max

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
				|| (llData > static_cast<LONGLONG>(std::numeric_limits<T>::max()))
				|| (llData < static_cast<LONGLONG>(std::numeric_limits<std::make_signed_t<T>>::min())))
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
				|| (llData > static_cast<LONGLONG>(std::numeric_limits<T>::max()))
				|| (llData < static_cast<LONGLONG>(std::numeric_limits<std::make_signed_t<T>>::min()))
				)
				return false;
			tData = static_cast<T>(llData);
		}

		return true;
	}
	//Explicit instantiations of templated func in .cpp.
	template bool str2num<UCHAR>(const std::string& str, UCHAR& t, int iBase);

	bool str2hex(const std::string& str, std::string& strToHex, bool fWc, char chWc)
	{
		std::string strTmp;
		for (auto iterBegin = str.begin(); iterBegin != str.end();)
		{
			if (fWc && *iterBegin == chWc) //Skip wildcard.
			{
				++iterBegin;
				strTmp += chWc;
				continue;
			}

			//Extract two current chars and pass it to str2num as string.
			const size_t nOffsetCurr = iterBegin - str.begin();
			const auto nSize = nOffsetCurr + 2 <= str.size() ? 2 : 1;
			if (unsigned char chNumber;	str2num(str.substr(nOffsetCurr, nSize), chNumber, 16))
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

	void ReplaceUnprintable(std::wstring& wstr, bool fASCII, bool fCRLFRepl)
	{
		//If fASCII is true, then only wchars in 0x1F<...<0x7F range are considered printable.
		//If fCRLFRepl is false, then CR(0x0D) and LF(0x0A) wchars remain untouched.
		if (fASCII)
		{
			std::replace_if(wstr.begin(), wstr.end(), [=](wchar_t wch) //All non ASCII.
				{return (wch <= 0x1F || wch >= 0x7F) && (fCRLFRepl || (wch != 0x0D && wch != 0x0A)); }, L'.');
		}
		else
		{
			std::replace_if(wstr.begin(), wstr.end(), [=](wchar_t wch) //All non printable wchars.
				{return !std::iswprint(wch) && (fCRLFRepl || (wch != 0x0D && wch != 0x0A)); }, L'.');
		}
	}
}