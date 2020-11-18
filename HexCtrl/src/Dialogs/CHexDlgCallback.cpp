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
	ON_COMMAND(IDCANCEL, &CHexDlgCallback::OnCancel)
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

	return TRUE;
}

void CHexDlgCallback::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

void CHexDlgCallback::OnCancel()
{
	m_fCancel = true;
}

bool CHexDlgCallback::IsCanceled()const
{
	return m_fCancel;
}

void CHexDlgCallback::Cancel()
{
	OnCancel();
}

BOOL CHexDlgCallback::ContinueModal()
{
	//Returning FALSE from here means exiting from the modal loop.

	//Although it's explicitly stated that this function must return zero 
	//to exit modal loop state, it may ASSERT sometimes if FALSE is returned
	//too early, at very early stage of dialog initialization.
	//https://docs.microsoft.com/en-us/cpp/mfc/reference/cwnd-class?view=vs-2017&redirectedfrom=MSDN#continuemodal
	//To overcome this, PostMessageW(WM_COMMAND, IDOK) instead of return FALSE seems to work ok.
	if (m_fCancel)
		PostMessageW(WM_COMMAND, IDOK);
		//return FALSE;

	return CDialogEx::ContinueModal();
}