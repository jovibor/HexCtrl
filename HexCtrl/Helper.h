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

namespace HEXCTRL {
	//Converts dwSize bytes of ull to WCHAR string.
	void ToWchars(ULONGLONG ull, wchar_t* pwsz, DWORD dwSize);

	//Converts char* string to unsigned long number.
	//Returns false if conversion is imposible, true otherwise.
	bool ToUl(const char* pcsz, unsigned long& ul);
};