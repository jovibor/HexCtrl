/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/                         *
* This is a Hex Control for MFC applications.                                           *
* Official git repository of the project: https://github.com/jovibor/HexCtrl/           *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.      *
****************************************************************************************/
/****************************************************************************************
* These are some helper functions for HexCtrl.											*
****************************************************************************************/
#pragma once
#include <afxwin.h>
#include <string>

namespace HEXCTRL {
	//Converts dwSize bytes of ull to WCHAR string.
	void UllToWchars(ULONGLONG ull, wchar_t* pwsz, size_t dwSize);

	//Converts char* string to unsigned long number.
	//Basically it's a strtoul() wrapper.
	//Returns false if conversion is imposible, true otherwise.
	bool CharsToUl(const char* pcsz, unsigned long& ul);

	//Wide string to Multibyte string convertion.
	std::string WstrToStr(const std::wstring& wstr);

	//Converts every two numeric chars to one respective hex character: "56"->V(0x56), "7A"->z(0x7A)
	bool StrToHex(const std::string& strFrom, std::string& strToHex);
};