/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/						    *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/Pepper/blob/master/LICENSE                                 *
* This is a Hex control for MFC apps, implemented as CWnd derived class.			    *
* The usage is quite simple:														    *
* 1. Construct CHexCtrl object — HEXCTRL::CHexCtrl myHex;								*
* 2. Call myHex.Create member function to create an instance.   					    *
* 3. Call myHex.SetData method to set the data and its size to display as hex.	        *
****************************************************************************************/
#include "stdafx.h"
#include "HexCtrl.h"
#include "HexCtrlDlgSearch.h"

using namespace HEXCTRL;

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
	ON_COMMAND_RANGE(IDC_HEXCTRL_SEARCH_RADIO_HEX, IDC_HEXCTRL_SEARCH_RADIO_UNICODE, &CHexDlgSearch::OnRadioBnRange)
END_MESSAGE_MAP()

BOOL CHexDlgSearch::Create(UINT nIDTemplate, CHexCtrl* pwndParent)
{
	m_pParent = pwndParent;

	return CDialog::Create(nIDTemplate, m_pParent);
}

CHexCtrl* CHexDlgSearch::GetParent() const
{
	return m_pParent;
}

BOOL CHexDlgSearch::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	m_iRadioCurrent = IDC_HEXCTRL_SEARCH_RADIO_HEX;
	CheckRadioButton(IDC_HEXCTRL_SEARCH_RADIO_HEX, IDC_HEXCTRL_SEARCH_RADIO_UNICODE, m_iRadioCurrent);
	m_stBrushDefault.CreateSolidBrush(m_clrMenu);

	return TRUE;
}

void CHexDlgSearch::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

void CHexDlgSearch::SearchCallback()
{
	WCHAR wstrSearch[128];

	if (m_stSearch.fFound)
	{
		if (m_stSearch.fCount)
		{
			if (!m_stSearch.fWrap)
			{
				if (m_stSearch.iDirection == HEXCTRL_SEARCH::SEARCH_FORWARD)
					m_dwOccurrences++;
				else if (m_stSearch.iDirection == HEXCTRL_SEARCH::SEARCH_BACKWARD)
					m_dwOccurrences--;
			}
			else
				m_dwOccurrences = 1;

			swprintf_s(wstrSearch, 127, L"Found occurrence \u2116 %lu from the beginning.", m_dwOccurrences);
		}
		else
			swprintf_s(wstrSearch, 127, L"Search found occurrence.");

		m_stSearch.fSecondMatch = true;
		GetDlgItem(IDC_HEXCTRL_SEARCH_STATIC_TEXTBOTTOM)->SetWindowTextW(wstrSearch);
	}
	else
	{
		ClearAll();

		if (m_stSearch.iWrap == 1)
			swprintf_s(wstrSearch, 127, L"Didn't find any occurrence. The end is reached.");
		else
			swprintf_s(wstrSearch, 127, L"Didn't find any occurrence. The begining is reached.");

		GetDlgItem(IDC_HEXCTRL_SEARCH_STATIC_TEXTBOTTOM)->SetWindowTextW(wstrSearch);
	}
}

void CHexDlgSearch::OnButtonSearchF()
{
	CString strSearchText;
	GetDlgItemTextW(IDC_HEXCTRL_SEARCH_EDITSEARCH, strSearchText);
	if (strSearchText.IsEmpty())
		return;
	if (strSearchText.Compare(m_stSearch.wstrSearch.data()) != 0)
	{
		ClearAll();
		m_stSearch.wstrSearch = strSearchText;
	}

	switch (GetCheckedRadioButton(IDC_HEXCTRL_SEARCH_RADIO_HEX, IDC_HEXCTRL_SEARCH_RADIO_UNICODE))
	{
	case IDC_HEXCTRL_SEARCH_RADIO_HEX:
		m_stSearch.dwSearchType = HEXCTRL_SEARCH::SEARCH_HEX;
		break;
	case IDC_HEXCTRL_SEARCH_RADIO_ASCII:
		m_stSearch.dwSearchType = HEXCTRL_SEARCH::SEARCH_ASCII;
		break;
	case IDC_HEXCTRL_SEARCH_RADIO_UNICODE:
		m_stSearch.dwSearchType = HEXCTRL_SEARCH::SEARCH_UNICODE;
		break;
	}
	m_stSearch.iDirection = HEXCTRL_SEARCH::SEARCH_FORWARD;

	GetDlgItem(IDC_HEXCTRL_SEARCH_EDITSEARCH)->SetFocus();
	GetParent()->Search(m_stSearch);
}

void CHexDlgSearch::OnButtonSearchB()
{
	CString strSearchText;
	GetDlgItemTextW(IDC_HEXCTRL_SEARCH_EDITSEARCH, strSearchText);
	if (strSearchText.IsEmpty())
		return;
	if (strSearchText.Compare(m_stSearch.wstrSearch.data()) != 0)
	{
		ClearAll();
		m_stSearch.wstrSearch = strSearchText;
	}

	switch (GetCheckedRadioButton(IDC_HEXCTRL_SEARCH_RADIO_HEX, IDC_HEXCTRL_SEARCH_RADIO_UNICODE))
	{
	case IDC_HEXCTRL_SEARCH_RADIO_HEX:
		m_stSearch.dwSearchType = HEXCTRL_SEARCH::SEARCH_HEX;
		break;
	case IDC_HEXCTRL_SEARCH_RADIO_ASCII:
		m_stSearch.dwSearchType = HEXCTRL_SEARCH::SEARCH_ASCII;
		break;
	case IDC_HEXCTRL_SEARCH_RADIO_UNICODE:
		m_stSearch.dwSearchType = HEXCTRL_SEARCH::SEARCH_UNICODE;
		break;
	}
	m_stSearch.iDirection = HEXCTRL_SEARCH::SEARCH_BACKWARD;

	GetDlgItem(IDC_HEXCTRL_SEARCH_EDITSEARCH)->SetFocus();
	GetParent()->Search(m_stSearch);
}

void CHexDlgSearch::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if (nState == WA_INACTIVE)
		SetLayeredWindowAttributes(0, 150, LWA_ALPHA);
	else
	{
		SetLayeredWindowAttributes(0, 255, LWA_ALPHA);
		GetDlgItem(IDC_HEXCTRL_SEARCH_EDITSEARCH)->SetFocus();
	}

	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
}

BOOL CHexDlgSearch::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	{
		OnButtonSearchF();
		return TRUE;
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CHexDlgSearch::OnClose()
{
	ClearAll();
	CDialogEx::OnClose();
}

HBRUSH CHexDlgSearch::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (pWnd->GetDlgCtrlID() == IDC_HEXCTRL_SEARCH_STATIC_TEXTBOTTOM) {
		pDC->SetBkColor(m_clrMenu);
		pDC->SetTextColor(m_stSearch.fFound ? m_clrSearchFound : m_clrSearchFailed);
		return m_stBrushDefault;
	}

	return CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CHexDlgSearch::OnRadioBnRange(UINT nID)
{
	if (nID != m_iRadioCurrent)
		ClearAll();
	m_iRadioCurrent = nID;
}

void CHexDlgSearch::ClearAll()
{
	m_dwOccurrences = 0;
	m_stSearch.ullStartAt = 0;
	m_stSearch.fSecondMatch = false;
	m_stSearch.wstrSearch = { };
	m_stSearch.fWrap = false;
	m_stSearch.fCount = true;

	GetDlgItem(IDC_HEXCTRL_SEARCH_STATIC_TEXTBOTTOM)->SetWindowTextW(L"");
}