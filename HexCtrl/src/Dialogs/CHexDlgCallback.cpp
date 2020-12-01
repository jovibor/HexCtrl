/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
****************************************************************************************/
#include "stdafx.h"
#include "../../res/HexCtrlRes.h"
#include "CHexDlgCallback.h"

using namespace HEXCTRL::INTERNAL;

BEGIN_MESSAGE_MAP(CHexDlgCallback, CDialogEx)
	ON_COMMAND(IDCANCEL, &CHexDlgCallback::OnBtnCancel)
	ON_WM_TIMER()
END_MESSAGE_MAP()

CHexDlgCallback::CHexDlgCallback(std::wstring_view wstrOperName, CWnd* pParent)
	: CDialogEx(IDD_HEXCTRL_CALLBACK, pParent), m_wstrOperName(wstrOperName)
{
}

BOOL CHexDlgCallback::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetWindowTextW(m_wstrOperName.data());
	GetDlgItem(IDC_HEXCTRL_CALLBACK_STATIC_OPERNAME)->SetWindowTextW(m_wstrOperName.data());
	SetTimer(IDT_EXITCHECK, 150, nullptr);

	return TRUE;
}

void CHexDlgCallback::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

void CHexDlgCallback::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == IDT_EXITCHECK && m_fCancel)
	{
		OnCancel();
		KillTimer(IDT_EXITCHECK);
	}

	CDialogEx::OnTimer(nIDEvent);
}

void CHexDlgCallback::OnBtnCancel()
{
	m_fCancel = true;
}

bool CHexDlgCallback::IsCanceled()const
{
	return m_fCancel;
}

void CHexDlgCallback::ExitDlg()
{
	OnBtnCancel();
}