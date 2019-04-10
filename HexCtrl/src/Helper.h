/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/						    *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* This is a Hex control for MFC apps, implemented as CWnd derived class.			    *
****************************************************************************************/
/****************************************************************************************
* These are some helper functions for HexCtrl.											*
****************************************************************************************/
#pragma once
#include <afxwin.h>
#include <string>

namespace HEXCTRL {
	//Converts dwSize bytes of ull to WCHAR string.
	void UllToWchars(ULONGLONG ull, wchar_t* pwsz, DWORD dwSize);

	//Converts char* string to unsigned long number.
	//Returns false if conversion is imposible, true otherwise.
	bool CharsToUl(const char* pcsz, unsigned long& ul);

	//Wide string to Multibyte string convertion.
	std::string WstrToStr(std::wstring& wstr);

	//Converts every two numbers from string to one respective character (56->V, 78->x).
	bool NumStrToHex(std::string& strNum, std::string& strHex);
};