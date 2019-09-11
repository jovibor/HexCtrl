/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/                         *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#include "stdafx.h"
#include "CHexDlgAbout.h"
#include "../Helper.h"

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

/****************************************************
* CHexDlgAbout class implementation.				*
****************************************************/

BEGIN_MESSAGE_MAP(CHexDlgAbout, CDialogEx)
END_MESSAGE_MAP()

BOOL CHexDlgAbout::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	std::wstring wstrVer = L"Hex Control for MFC/Win32 v";
	wstrVer += HEXCTRL_VERSION_WSTR;
	GetDlgItem(IDC_HEXCTRL_ABOUT_STATIC_VERSION)->SetWindowTextW(wstrVer.data());

	return TRUE;
}