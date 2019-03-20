/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/						    *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                 *
* This is a Hex control for MFC apps, implemented as CWnd derived class.			    *
* The usage is quite simple:														    *
* 1. Construct CHexCtrl object — HEXCTRL::CHexCtrl myHex;								*
* 2. Call myHex.Create member function to create an instance.   					    *
* 3. Call myHex.SetData method to set the data and its size to display as hex.	        *
****************************************************************************************/
#include "stdafx.h"
#include "HexCtrlDlgAbout.h"

using namespace HEXCTRL;

namespace HEXCTRL {
	namespace HEXCTRL_INTERNAL {
		constexpr auto HEXCTRL_VERSION_WSTR = L"Hex Control for MFC, v2.2.3";
		constexpr auto HEXCTRL_LINKGITHUB_WSTR = L"https://github.com/jovibor/HexCtrl";
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

	//To prevent cursor from blinking
	SetClassLongPtrW(m_hWnd, GCLP_HCURSOR, 0);

	m_fontDefault.CreateStockObject(DEFAULT_GUI_FONT);
	LOGFONTW lf;
	m_fontDefault.GetLogFont(&lf);
	lf.lfUnderline = TRUE;
	m_fontUnderline.CreateFontIndirectW(&lf);

	m_stBrushDefault.CreateSolidBrush(m_clrMenu);

	m_curHand = LoadCursor(nullptr, IDC_HAND);
	m_curArrow = LoadCursor(nullptr, IDC_ARROW);

	GetDlgItem(IDC_HEXCTRL_ABOUT_STATIC_VERSION)->SetWindowTextW(HEXCTRL_INTERNAL::HEXCTRL_VERSION_WSTR);
	GetDlgItem(IDC_HEXCTRL_ABOUT_STATIC_LINKGITHUB)->SetWindowTextW(HEXCTRL_INTERNAL::HEXCTRL_LINKGITHUB_WSTR);

	return TRUE;
}

void CHexDlgAbout::OnMouseMove(UINT nFlags, CPoint point)
{
	CWnd* pWnd = ChildWindowFromPoint(point);
	if (!pWnd)
		return;

	if (m_fGithubLink == (pWnd->GetDlgCtrlID() == IDC_HEXCTRL_ABOUT_STATIC_LINKGITHUB))
	{
		m_fGithubLink = !m_fGithubLink;
		GetDlgItem(IDC_HEXCTRL_ABOUT_STATIC_LINKGITHUB)->RedrawWindow();
		SetCursor(m_fGithubLink ? m_curArrow : m_curHand);
	}

	CDialogEx::OnMouseMove(nFlags, point);
}

void CHexDlgAbout::OnLButtonDown(UINT nFlags, CPoint point)
{
	CWnd* pWnd = ChildWindowFromPoint(point);

	if (!pWnd)
		return;

	if (pWnd->GetDlgCtrlID() == IDC_HEXCTRL_ABOUT_STATIC_LINKGITHUB)
		ShellExecute(nullptr, L"open", HEXCTRL_INTERNAL::HEXCTRL_LINKGITHUB_WSTR, nullptr, nullptr, NULL);

	CDialogEx::OnLButtonDown(nFlags, point);
}

HBRUSH CHexDlgAbout::OnCtlColor(CDC * pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (pWnd->GetDlgCtrlID() == IDC_HEXCTRL_ABOUT_STATIC_LINKGITHUB)
	{
		pDC->SetBkColor(m_clrMenu);
		pDC->SetTextColor(RGB(0, 0, 210));
		pDC->SelectObject(m_fGithubLink ? &m_fontDefault : &m_fontUnderline);
		return m_stBrushDefault;
	}

	return CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
}