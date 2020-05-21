/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This file contributed by Data Synergy UK. No rights reserved                          *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#include "stdafx.h"
#include "../Helper.h"
#include "CHexDlgCodepage.h"

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

/****************************************************
* CHexDlgCodePage class implementation              *
****************************************************/

BEGIN_MESSAGE_MAP(CHexDlgCodePage, CDialogEx)
	ON_CBN_SELCHANGE(IDC_HEXCTRL_CODEPAGE_COMBO, CHexDlgCodePage::OnCbnSelChange)
END_MESSAGE_MAP()

BOOL CALLBACK EnumCodePagesProc(LPWSTR lpCodePageString)
{	
	if (CHexDlgCodePage::m_pHexDlgCodepage)
		CHexDlgCodePage::m_pHexDlgCodepage->AddCodePage(lpCodePageString);

	return true;
}

BOOL CHexDlgCodePage::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	
	CHexDlgCodePage::m_pHexDlgCodepage = this;
	EnumSystemCodePages(EnumCodePagesProc, CP_INSTALLED);	
	GetDlgItem(IDOK)->EnableWindow(false);

	return TRUE;
}

void CHexDlgCodePage::AddCodePage(LPWSTR lpCodePageString)
{
	auto pCombo = reinterpret_cast<CComboBox*>(GetDlgItem(IDC_HEXCTRL_CODEPAGE_COMBO));
	if (pCombo)
	{
		int nCodePage{};
		if (wstr2num(lpCodePageString, nCodePage))
		{
			CPINFOEX stCPInfo{};
			if (GetCPInfoEx(nCodePage, 0, &stCPInfo))
			{
				int nIndex = pCombo->AddString(stCPInfo.CodePageName);
				if (nIndex >= 0)
					pCombo->SetItemData(nIndex, (DWORD_PTR)nCodePage);
			}
		}
	}
}

void CHexDlgCodePage::OnCbnSelChange()
{
	auto pCombo = reinterpret_cast<CComboBox*>(GetDlgItem(IDC_HEXCTRL_CODEPAGE_COMBO));
	if (pCombo)
	{
		int nIndex = pCombo->GetCurSel();
		if (nIndex >= 0)
			m_nCodepage=(int)pCombo->GetItemData(nIndex);
	}

	GetDlgItem(IDOK)->EnableWindow(m_nCodepage > 0);
}

int CHexDlgCodePage::GetSelectedCodepage()
{
	return m_nCodepage;
}