/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/						    *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* This is a Hex control for MFC apps, implemented as CWnd derived class.			    *
* The usage is quite simple:														    *
* 1. Construct CHexCtrl object — HEXCTRL::CHexCtrl myHex;								*
* 2. Call myHex.Create member function to create an instance.   					    *
* 3. Call myHex.SetData method to set the data and its size to display as hex.	        *
****************************************************************************************/
#include "stdafx.h"
#include "CHexDlgAbout.h"

using namespace HEXCTRL;

namespace HEXCTRL {
	namespace INTERNAL {
		constexpr auto WSTR_HEXCTRL_VERSION = L"Hex Control for MFC, v2.2.14";
	};
}

/****************************************************
* CHexDlgAbout class implementation.				*
****************************************************/

BEGIN_MESSAGE_MAP(CHexDlgAbout, CDialogEx)
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

BOOL CHexDlgAbout::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	GetDlgItem(IDC_HEXCTRL_ABOUT_STATIC_VERSION)->SetWindowTextW(INTERNAL::WSTR_HEXCTRL_VERSION);

	return TRUE;
}