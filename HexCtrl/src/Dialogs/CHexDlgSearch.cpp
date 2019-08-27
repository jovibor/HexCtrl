/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/                         *
* This is a Hex Control for MFC applications.                                           *
* Official git repository of the project: https://github.com/jovibor/HexCtrl/           *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#include "stdafx.h"
#include "CHexDlgSearch.h"

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

/************************************************************
* CHexDlgSearch class implementation.						*
* This class implements search routines within HexControl.	*
************************************************************/
BEGIN_MESSAGE_MAP(CHexDlgSearch, CDialogEx)
	ON_WM_ACTIVATE()
	ON_WM_CLOSE()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_BUTTON_SEARCH_F, &CHexDlgSearch::OnButtonSearchF)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_BUTTON_SEARCH_B, &CHexDlgSearch::OnButtonSearchB)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_BUTTON_REPLACE, &CHexDlgSearch::OnButtonReplace)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_BUTTON_REPLACE_ALL, &CHexDlgSearch::OnButtonReplaceAll)
	ON_COMMAND_RANGE(IDC_HEXCTRL_SEARCH_RADIO_HEX, IDC_HEXCTRL_SEARCH_RADIO_UNICODE, &CHexDlgSearch::OnRadioBnRange)
END_MESSAGE_MAP()

BOOL CHexDlgSearch::Create(UINT nIDTemplate, CHexCtrl* pHexCtrl)
{
	m_pHexCtrl = pHexCtrl;

	return CDialog::Create(nIDTemplate, m_pHexCtrl);
}

BOOL CHexDlgSearch::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_uRadioCurrent = IDC_HEXCTRL_SEARCH_RADIO_HEX;
	CheckRadioButton(IDC_HEXCTRL_SEARCH_RADIO_HEX, IDC_HEXCTRL_SEARCH_RADIO_UNICODE, m_uRadioCurrent);
	m_stBrushDefault.CreateSolidBrush(m_clrBkTextArea);

	return TRUE;
}

void CHexDlgSearch::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

void CHexDlgSearch::SearchCallback()
{
	SetActiveWindow();

	std::wstring wstrInfo(128, 0);
	if (m_stSearch.fFound)
	{
		if (m_stSearch.fReplace && m_stSearch.fAll)
		{
			swprintf_s(wstrInfo.data(), 127, L"%lu occurrence(s) replaced.", m_stSearch.dwReplaced);
			m_stSearch.dwReplaced = 0;
		}
		else if (m_stSearch.fDoCount)
			swprintf_s(wstrInfo.data(), 127, L"Found occurrence \u2116 %lu from the beginning.", m_stSearch.dwCount);
		else
			wstrInfo = L"Search found occurrence.";
	}
	else
	{
		ClearAll();
		if (m_stSearch.iWrap == 1)
			wstrInfo = L"Didn't find any occurrence, the end is reached.";
		else
			wstrInfo = L"Didn't find any occurrence, the begining is reached.";
	}

	GetDlgItem(IDC_HEXCTRL_SEARCH_STATIC_TEXTBOTTOM)->SetWindowTextW(wstrInfo.data());
}

void CHexDlgSearch::OnButtonSearchF()
{
	CStringW wstrTextSearch;
	GetDlgItemTextW(IDC_HEXCTRL_SEARCH_COMBO_SEARCH, wstrTextSearch);
	if (wstrTextSearch.IsEmpty())
		return;
	if (wstrTextSearch.Compare(m_stSearch.wstrSearch.data()) != 0)
	{
		ClearAll();
		m_stSearch.wstrSearch = wstrTextSearch;
	}

	m_stSearch.iDirection = 1;
	m_stSearch.fReplace = false;
	m_stSearch.fAll = false;
	m_stSearch.enSearchType = GetSearchMode();

	ComboSearchFill(wstrTextSearch);
	GetDlgItem(IDC_HEXCTRL_SEARCH_COMBO_SEARCH)->SetFocus();
	GetHexCtrl()->SearchCallback(m_stSearch);
	SearchCallback();
}

void CHexDlgSearch::OnButtonSearchB()
{
	CStringW wstrTextSearch;
	GetDlgItemTextW(IDC_HEXCTRL_SEARCH_COMBO_SEARCH, wstrTextSearch);
	if (wstrTextSearch.IsEmpty())
		return;

	if (wstrTextSearch.Compare(m_stSearch.wstrSearch.data()) != 0)
	{
		ClearAll();
		m_stSearch.wstrSearch = wstrTextSearch;
	}

	m_stSearch.iDirection = -1;
	m_stSearch.fReplace = false;
	m_stSearch.fAll = false;
	m_stSearch.enSearchType = GetSearchMode();

	ComboSearchFill(wstrTextSearch);
	GetDlgItem(IDC_HEXCTRL_SEARCH_COMBO_SEARCH)->SetFocus();
	GetHexCtrl()->SearchCallback(m_stSearch);
	SearchCallback();
}

void CHexDlgSearch::OnButtonReplace()
{
	CStringW wstrTextSearch;
	GetDlgItemTextW(IDC_HEXCTRL_SEARCH_COMBO_SEARCH, wstrTextSearch);
	if (wstrTextSearch.IsEmpty())
		return;
	if (wstrTextSearch.Compare(m_stSearch.wstrSearch.data()) != 0)
	{
		ClearAll();
		m_stSearch.wstrSearch = wstrTextSearch;
	}

	CStringW wstrTextReplace;
	GetDlgItemTextW(IDC_HEXCTRL_SEARCH_COMBO_REPLACE, wstrTextReplace);
	if (wstrTextReplace.IsEmpty())
		return;

	m_stSearch.wstrReplace = wstrTextReplace;
	m_stSearch.iDirection = 1;
	m_stSearch.fReplace = true;
	m_stSearch.fAll = false;
	m_stSearch.enSearchType = GetSearchMode();

	ComboSearchFill(wstrTextSearch);
	ComboReplaceFill(wstrTextReplace);
	GetDlgItem(IDC_HEXCTRL_SEARCH_COMBO_SEARCH)->SetFocus();
	GetHexCtrl()->SearchCallback(m_stSearch);
	SearchCallback();
}

void CHexDlgSearch::OnButtonReplaceAll()
{
	CStringW wstrTextSearch;
	GetDlgItemTextW(IDC_HEXCTRL_SEARCH_COMBO_SEARCH, wstrTextSearch);
	if (wstrTextSearch.IsEmpty())
		return;
	if (wstrTextSearch.Compare(m_stSearch.wstrSearch.data()) != 0)
	{
		ClearAll();
		m_stSearch.wstrSearch = wstrTextSearch;
	}

	CStringW wstrTextReplace;
	GetDlgItemTextW(IDC_HEXCTRL_SEARCH_COMBO_REPLACE, wstrTextReplace);
	if (wstrTextReplace.IsEmpty())
		return;

	m_stSearch.fReplace = true;
	m_stSearch.fAll = true;
	m_stSearch.wstrReplace = wstrTextReplace;
	m_stSearch.iDirection = 1;
	m_stSearch.enSearchType = GetSearchMode();

	ComboSearchFill(wstrTextSearch);
	ComboReplaceFill(wstrTextReplace);
	GetDlgItem(IDC_HEXCTRL_SEARCH_COMBO_SEARCH)->SetFocus();
	GetHexCtrl()->SearchCallback(m_stSearch);
	SearchCallback();
}

void CHexDlgSearch::OnActivate(UINT nState, CWnd * pWndOther, BOOL bMinimized)
{
	if (nState == WA_INACTIVE)
		SetLayeredWindowAttributes(0, 150, LWA_ALPHA);
	else
	{
		SetLayeredWindowAttributes(0, 255, LWA_ALPHA);
		GetDlgItem(IDC_HEXCTRL_SEARCH_COMBO_SEARCH)->SetFocus();

		bool fMutable = GetHexCtrl()->IsMutable();
		GetDlgItem(IDC_HEXCTRL_SEARCH_COMBO_REPLACE)->EnableWindow(fMutable);
		GetDlgItem(IDC_HEXCTRL_SEARCH_BUTTON_REPLACE)->EnableWindow(fMutable);
		GetDlgItem(IDC_HEXCTRL_SEARCH_BUTTON_REPLACE_ALL)->EnableWindow(fMutable);
	}

	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
}

BOOL CHexDlgSearch::PreTranslateMessage(MSG * pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	{
		OnButtonSearchF();
		return TRUE;
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CHexDlgSearch::OnCancel()
{
	ClearAll();
	GetHexCtrl()->SetFocus();

	CDialogEx::OnCancel();
}

HBRUSH CHexDlgSearch::OnCtlColor(CDC * pDC, CWnd * pWnd, UINT nCtlColor)
{
	if (pWnd->GetDlgCtrlID() == IDC_HEXCTRL_SEARCH_STATIC_TEXTBOTTOM)
	{
		pDC->SetBkColor(m_clrBkTextArea);
		pDC->SetTextColor(m_stSearch.fFound ? m_clrSearchFound : m_clrSearchFailed);
		return m_stBrushDefault;
	}

	return CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CHexDlgSearch::OnRadioBnRange(UINT nID)
{
	if (nID != m_uRadioCurrent)
		ClearAll();
	m_uRadioCurrent = nID;
}

void CHexDlgSearch::ClearAll()
{
	m_stSearch = { };
	GetDlgItem(IDC_HEXCTRL_SEARCH_STATIC_TEXTBOTTOM)->SetWindowTextW(L"");
}

CHexCtrl* CHexDlgSearch::GetHexCtrl()const
{
	return m_pHexCtrl;
}

ESearchMode CHexDlgSearch::GetSearchMode()
{
	ESearchMode enSearch { };
	switch (GetCheckedRadioButton(IDC_HEXCTRL_SEARCH_RADIO_HEX, IDC_HEXCTRL_SEARCH_RADIO_UNICODE))
	{
	case IDC_HEXCTRL_SEARCH_RADIO_HEX:
		enSearch = ESearchMode::SEARCH_HEX;
		break;
	case IDC_HEXCTRL_SEARCH_RADIO_ASCII:
		enSearch = ESearchMode::SEARCH_ASCII;
		break;
	case IDC_HEXCTRL_SEARCH_RADIO_UNICODE:
		enSearch = ESearchMode::SEARCH_UTF16;
		break;
	}

	return enSearch;
}

void CHexDlgSearch::ComboSearchFill(LPCWSTR pwsz)
{
	CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_HEXCTRL_SEARCH_COMBO_SEARCH);
	if (!pCombo)
		return;

	//Insert wstring into ComboBox only if it's not already presented.
	if (pCombo->FindStringExact(0, pwsz) == CB_ERR)
	{
		//Keep max 50 strings in list.
		if (pCombo->GetCount() == 50)
			pCombo->DeleteString(49);
		pCombo->InsertString(0, pwsz);
	}
}

void CHexDlgSearch::ComboReplaceFill(LPCWSTR pwsz)
{
	CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_HEXCTRL_SEARCH_COMBO_REPLACE);
	if (!pCombo)
		return;

	//Insert wstring into ComboBox only if it's not already presented.
	if (pCombo->FindStringExact(0, pwsz) == CB_ERR)
	{
		//Keep max 50 strings in list.
		if (pCombo->GetCount() == 50)
			pCombo->DeleteString(49);
		pCombo->InsertString(0, pwsz);
	}
}
