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

	void ToWchars(ULONGLONG ull, wchar_t* pwsz, DWORD dwSize)
	{
		const wchar_t* const pwszHexMap { L"0123456789ABCDEF" };

		//Converts dwSize bytes of ull to wchar_t*.
		for (unsigned i = 0; i < dwSize; i++)
		{
			pwsz[i * 2] = pwszHexMap[((ull >> ((dwSize - 1 - i) << 3)) & 0xF0) >> 4];
			pwsz[i * 2 + 1] = pwszHexMap[(ull >> ((dwSize - 1 - i) << 3)) & 0x0F];
		}
	}

	bool ToUl(const char* pcsz, unsigned long& ul)
	{
		char* pEndPtr;
		ul = strtoul(pcsz, &pEndPtr, 16);
		if (ul == 0 && (pEndPtr == pcsz || *pEndPtr != '\0'))
			return false;

		return true;
	}
}