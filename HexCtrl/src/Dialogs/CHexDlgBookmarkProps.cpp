/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/                         *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#include "stdafx.h"
#include "CHexDlgBookmarkProps.h"
#include <cassert>
#include <numeric>
#include"../Helper.h"

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

BEGIN_MESSAGE_MAP(CHexDlgBookmarkProps, CDialogEx)
END_MESSAGE_MAP()

INT_PTR CHexDlgBookmarkProps::DoModal(HEXBOOKMARKSTRUCT* phbs)
{
	assert(phbs);
	if (!phbs)
		return -1;

	m_pHBS = phbs;

	return CDialogEx::DoModal();
}

void CHexDlgBookmarkProps::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BOOL CHexDlgBookmarkProps::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CMFCColorButton* pClr;
	pClr = (CMFCColorButton*)GetDlgItem(IDC_HEXCTRL_BOOKMARKMGR_DLGEDIT_COLOR_BK);
	pClr->SetColor(m_pHBS->clrBk);
	pClr = (CMFCColorButton*)GetDlgItem(IDC_HEXCTRL_BOOKMARKMGR_DLGEDIT_COLOR_TEXT);
	pClr->SetColor(m_pHBS->clrText);

	wchar_t pwszBuff[32];
	if (!m_pHBS->vecSpan.empty())
	{
		m_ullOffset = m_pHBS->vecSpan.front().ullOffset;
		m_ullSize = std::accumulate(m_pHBS->vecSpan.begin(), m_pHBS->vecSpan.end(), ULONGLONG { },
			[](auto ullTotal, const HEXSPANSTRUCT& ref) {return ullTotal + ref.ullSize; });
	}
	else
	{
		m_ullOffset = 0;
		m_ullSize = 1;
		m_pHBS->vecSpan.emplace_back(HEXSPANSTRUCT { m_ullOffset, m_ullSize });
	}
	swprintf_s(pwszBuff, L"0x%llX", m_ullOffset);
	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_HEXCTRL_BOOKMARKMGR_DLGEDIT_EDIT_OFFSET);
	pEdit->SetWindowTextW(pwszBuff);

	swprintf_s(pwszBuff, L"0x%llX", m_ullSize);
	pEdit = (CEdit*)GetDlgItem(IDC_HEXCTRL_BOOKMARKMGR_DLGEDIT_EDIT_LENGTH);
	pEdit->SetWindowTextW(pwszBuff);
	pEdit = (CEdit*)GetDlgItem(IDC_HEXCTRL_BOOKMARKMGR_DLGEDIT_EDIT_DESCR);
	pEdit->SetWindowTextW(m_pHBS->wstrDesc.data());

	return TRUE;
}

void CHexDlgBookmarkProps::OnOK()
{
	CMFCColorButton* pClrBtn;
	pClrBtn = (CMFCColorButton*)GetDlgItem(IDC_HEXCTRL_BOOKMARKMGR_DLGEDIT_COLOR_BK);
	m_pHBS->clrBk = pClrBtn->GetColor();
	pClrBtn = (CMFCColorButton*)GetDlgItem(IDC_HEXCTRL_BOOKMARKMGR_DLGEDIT_COLOR_TEXT);
	m_pHBS->clrText = pClrBtn->GetColor();

	wchar_t pwszBuff[512];
	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_HEXCTRL_BOOKMARKMGR_DLGEDIT_EDIT_OFFSET);
	pEdit->GetWindowTextW(pwszBuff, 32);
	ULONGLONG llOffset;
	if (!WCharsToUll(pwszBuff, llOffset))
	{
		MessageBoxW(L"Wrong number format!", L"Format Error", MB_ICONERROR);
		return;
	}
	pEdit = (CEdit*)GetDlgItem(IDC_HEXCTRL_BOOKMARKMGR_DLGEDIT_EDIT_LENGTH);
	pEdit->GetWindowTextW(pwszBuff, 32);
	ULONGLONG llSize;
	if (!WCharsToUll(pwszBuff, llSize))
	{
		MessageBoxW(L"Wrong number format!", L"Format Error", MB_ICONERROR);
		return;
	}
	if (llSize == 0)
	{
		MessageBoxW(L"Length can not be zero!", L"Format Error", MB_ICONERROR);
		return;
	}
	if (m_ullOffset != llOffset || m_ullSize != llSize)
	{
		m_pHBS->vecSpan.clear();
		m_pHBS->vecSpan.emplace_back(HEXSPANSTRUCT { llOffset, llSize });
	}

	pEdit = (CEdit*)GetDlgItem(IDC_HEXCTRL_BOOKMARKMGR_DLGEDIT_EDIT_DESCR);
	pEdit->GetWindowTextW(pwszBuff, 512);
	m_pHBS->wstrDesc = pwszBuff;

	CDialogEx::OnOK();
}