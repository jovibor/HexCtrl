/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#include "stdafx.h"
#include "../Helper.h"
#include "CHexDlgAbout.h"

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

#define HEXCTRL_VERSION_DESCR HEXCTRL_VERSION_WSTR "\r\nAuthor: " HEXCTRL_COPYRIGHT_NAME
	GetDlgItem(IDC_HEXCTRL_ABOUT_STATIC_VERSION)->SetWindowTextW(HEXCTRL_VERSION_DESCR);

	return TRUE;
}