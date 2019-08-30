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
#include "../../verinfo/version.h"

#define STR_HELPER(x) #x
#define TO_STR(x) STR_HELPER(x)
#define TO_WSTR_HELPER(x) L##x
#define TO_WSTR(x) TO_WSTR_HELPER(x)

#ifdef _WIN64
#define HEXCTRL_VERSION_WSTR TO_WSTR(PRODUCT_DESC)", v" TO_STR(MAJOR_VERSION) "." TO_STR(MINOR_VERSION) "." TO_STR(MAINTENANCE_VERSION) " (x64)"
#else
#define HEXCTRL_VERSION_WSTR TO_WSTR(PRODUCT_DESC)", v" TO_STR(MAJOR_VERSION) "." TO_STR(MINOR_VERSION) "." TO_STR(MAINTENANCE_VERSION)
#endif

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

	GetDlgItem(IDC_HEXCTRL_ABOUT_STATIC_VERSION)->SetWindowTextW(HEXCTRL_VERSION_WSTR);

	return TRUE;
}