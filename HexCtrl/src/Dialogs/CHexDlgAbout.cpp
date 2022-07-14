/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "../HexUtility.h"
#include "CHexDlgAbout.h"
#include <string>

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

BEGIN_MESSAGE_MAP(CHexDlgAbout, CDialogEx)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL CHexDlgAbout::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	std::wstring wstrDescr = WSTR_HEXCTRL_FULL_VERSION;
	wstrDescr += L"\r\nAuthor: ";
	wstrDescr += WSTR_HEXCTRL_COPYRIGHT_NAME;
	GetDlgItem(IDC_HEXCTRL_ABOUT_STATIC_VERSION)->SetWindowTextW(wstrDescr.data());

	auto pDC = GetDC();
	const auto iLOGPIXELSY = GetDeviceCaps(pDC->m_hDC, LOGPIXELSY);
	ReleaseDC(pDC);

	const auto fScale = iLOGPIXELSY / 96.0f; //Scale factor for HighDPI displays.
	const auto iSizeIcon = static_cast<int>(32 * fScale);
	m_bmpLogo = static_cast<HBITMAP>(LoadImageW(AfxGetInstanceHandle(),
		MAKEINTRESOURCEW(IDB_HEXCTRL_LOGO), IMAGE_BITMAP, iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	static_cast<CStatic*>(GetDlgItem(IDC_HEXCTRL_ABOUT_LOGO))->SetBitmap(m_bmpLogo);

	return TRUE;
}

void CHexDlgAbout::OnDestroy()
{
	DeleteObject(m_bmpLogo);

	CDialogEx::OnDestroy();
}