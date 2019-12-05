/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/                         *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository of the project: https://github.com/jovibor/HexCtrl/           *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#include "stdafx.h"
#include "CHexDlgSearch.h"
#include "../Helper.h"

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

	return CDialogEx::Create(nIDTemplate, m_pHexCtrl);
}

void CHexDlgSearch::Search(bool fForward)
{
	if (fForward)
		m_iDirection = 1;
	else
		m_iDirection = -1;

	m_fReplace = false;
	m_fAll = false;
	enSearchType = GetSearchMode();
	Search();
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

bool CHexDlgSearch::DoSearch(const unsigned char* pWhere, ULONGLONG& ullStart, ULONGLONG ullUntil, const unsigned char* pSearch, size_t nSize, bool fForward)
{
	if (fForward)
	{
		for (ULONGLONG i = ullStart; i <= ullUntil; ++i)
		{
			if (memcmp(pWhere + i, pSearch, nSize) == 0)
			{
				ullStart = i;
				return true;
			}
		}
	}
	else
	{
		for (LONGLONG i = (LONGLONG)m_ullIndex; i >= (LONGLONG)ullUntil; --i)
		{
			if (memcmp(pWhere + i, pSearch, nSize) == 0)
			{
				ullStart = i;
				return true;
			}
		}
	}

	return false;
}

void CHexDlgSearch::Search()
{
	CHexCtrl* pHexCtrl = GetHexCtrl();
	if (!pHexCtrl)
		return;

	ULONGLONG ullDataSize;
	PBYTE pData = pHexCtrl->GetData(&ullDataSize);
	if (wstrSearch.empty() || !pHexCtrl->IsDataSet() || m_ullIndex >= ullDataSize)
		return;

	m_fFound = false;
	size_t nSizeSearch { };
	const BYTE* pSearch { };
	size_t nSizeReplace { };
	const BYTE* pReplace { };
	std::string strSearch;
	std::string strReplace;
	static const wchar_t* const wstrReplaceWarning { L"Replacing string is longer than Find string.\r\n"
		"Do you want to overwrite the bytes following search occurrence?\r\n"
		"Choosing \"No\" will cancel search." };
	static const wchar_t* const wstrWrongInput { L"Wrong input data format!" };

	switch (enSearchType)
	{
	case ESearchMode::SEARCH_HEX:
	{
		strSearch = WstrToStr(wstrSearch);
		strReplace = WstrToStr(wstrReplace);
		if (!StrToHex(strSearch, strSearch))
		{
			m_iWrap = 1;
			return;
		}
		nSizeSearch = strSearch.size();
		if ((m_fReplace && !StrToHex(strReplace, strReplace)))
		{
			MessageBoxW(wstrWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
			return;
		}
		nSizeReplace = strReplace.size();
		pSearch = (PBYTE)strSearch.data();
		pReplace = (PBYTE)strReplace.data();
	}
	break;
	case ESearchMode::SEARCH_ASCII:
	{
		strSearch = WstrToStr(wstrSearch);
		strReplace = WstrToStr(wstrReplace);
		nSizeSearch = strSearch.size();
		nSizeReplace = strReplace.size();
		pSearch = (PBYTE)strSearch.data();
		pReplace = (PBYTE)strReplace.data();
	}
	break;
	case ESearchMode::SEARCH_UTF16:
	{
		nSizeSearch = wstrSearch.size() * sizeof(wchar_t);
		nSizeReplace = wstrReplace.size() * sizeof(wchar_t);
		pSearch = (PBYTE)wstrSearch.data();
		pReplace = (PBYTE)wstrReplace.data();
	}
	break;
	}

	if ((nSizeSearch + m_ullIndex) > ullDataSize || (nSizeReplace + m_ullIndex) > ullDataSize)
	{
		m_ullIndex = 0;
		m_fSecondMatch = false;
		return;
	}
	if (m_fReplace && (nSizeReplace > nSizeSearch) && m_fReplaceWarning)
	{
		if (IDNO == MessageBoxW(wstrReplaceWarning, L"Warning", MB_YESNO | MB_ICONQUESTION | MB_TOPMOST))
			return;
		else
			m_fReplaceWarning = false;
	}

	ULONGLONG ullUntil = ullDataSize - nSizeSearch;
	///////////////Actual Search:////////////////////////////
	if (m_fReplace && m_fAll) //SearchReplace All
	{
		ULONGLONG ullStart = 0;
		while (true)
		{
			if (DoSearch(pData, ullStart, ullUntil, pSearch, nSizeSearch))
			{
				SearchReplace(ullStart, pReplace, nSizeSearch, nSizeReplace, false);
				ullStart += nSizeReplace;
				m_fFound = true;
				m_dwReplaced++;
			}
			else
				break;
		}
	}
	else //Search or SearchReplace
	{
		if (m_iDirection == 1) //Forward direction
		{
			if (m_fReplace && m_fSecondMatch)
			{
				SearchReplace(m_ullIndex, pReplace, nSizeSearch, nSizeReplace);
				m_ullIndex += nSizeReplace; //Increase next search step to replaced count.
				m_dwReplaced++;
			}
			else
				m_ullIndex = m_fSecondMatch ? m_ullIndex + 1 : 0;

			if (DoSearch(pData, m_ullIndex, ullUntil, pSearch, nSizeSearch))
			{
				m_fFound = true;
				m_fSecondMatch = true;
				m_fWrap = false;
				m_dwCount++;
			}
			if (!m_fFound && m_fSecondMatch)
			{
				m_ullIndex = 0; //Starting from the beginning.
				if (DoSearch(pData, m_ullIndex, ullUntil, pSearch, nSizeSearch))
				{
					m_fFound = true;
					m_fSecondMatch = true;
					m_fWrap = true;
					m_fDoCount = true;
					m_dwCount = 1;
				}
				m_iWrap = 1;
			}
		}
		else if (m_iDirection == -1) //Backward direction
		{
			if (m_fSecondMatch && m_ullIndex > 0)
			{
				m_ullIndex--;
				if (DoSearch(pData, m_ullIndex, 0, pSearch, nSizeSearch, false))
				{
					m_fFound = true;
					m_fSecondMatch = true;
					m_fWrap = false;
					m_dwCount--;
				}
			}
			if (!m_fFound)
			{
				m_ullIndex = ullDataSize - nSizeSearch;
				if (DoSearch(pData, m_ullIndex, 0, pSearch, nSizeSearch, false))
				{
					m_fFound = true;
					m_fSecondMatch = true;
					m_fWrap = true;
					m_iWrap = -1;
					m_fDoCount = false;
					m_dwCount = 1;
				}
			}
		}
	}

	std::wstring wstrInfo(128, 0);
	if (m_fFound)
	{
		if (m_fReplace && m_fAll)
		{
			swprintf_s(wstrInfo.data(), 127, L"%lu occurrence(s) replaced.", m_dwReplaced);
			m_dwReplaced = 0;
			pHexCtrl->RedrawWindow();
		}
		else
		{
			if (m_fDoCount)
				swprintf_s(wstrInfo.data(), 127, L"Found occurrence \u2116 %lu from the beginning.", m_dwCount);
			else
				wstrInfo = L"Search found occurrence.";

			ULONGLONG ullSelIndex = m_ullIndex;
			ULONGLONG ullSelSize = m_fReplace ? nSizeReplace : nSizeSearch;
			pHexCtrl->SetSelection(ullSelIndex, ullSelIndex, ullSelSize, 1, true, true);
		}
	}
	else
	{
		ClearAll();
		if (m_iWrap == 1)
			wstrInfo = L"Didn't find any occurrence, the end is reached.";
		else
			wstrInfo = L"Didn't find any occurrence, the begining is reached.";
	}

	GetDlgItem(IDC_HEXCTRL_SEARCH_STATIC_TEXTBOTTOM)->SetWindowTextW(wstrInfo.data());
}

void CHexDlgSearch::SearchReplace(ULONGLONG ullIndex, const BYTE* pData, size_t nSizeData, size_t nSizeReplace, bool fRedraw)
{
	HEXMODIFYSTRUCT hms;
	hms.vecSpan.emplace_back(HEXSPANSTRUCT { ullIndex, nSizeData });
	hms.ullDataSize = nSizeReplace;
	hms.pData = pData;
	GetHexCtrl()->ModifyData(hms, fRedraw);
}

void CHexDlgSearch::OnButtonSearchF()
{
	CStringW wstrTextSearch;
	GetDlgItemTextW(IDC_HEXCTRL_SEARCH_COMBO_SEARCH, wstrTextSearch);
	if (wstrTextSearch.IsEmpty())
		return;
	if (wstrTextSearch.Compare(wstrSearch.data()) != 0)
	{
		ClearAll();
		wstrSearch = wstrTextSearch;
	}

	m_iDirection = 1;
	m_fReplace = false;
	m_fAll = false;
	enSearchType = GetSearchMode();

	ComboSearchFill(wstrTextSearch);
	GetDlgItem(IDC_HEXCTRL_SEARCH_COMBO_SEARCH)->SetFocus();
	Search();
	SetActiveWindow();
}

void CHexDlgSearch::OnButtonSearchB()
{
	CStringW wstrTextSearch;
	GetDlgItemTextW(IDC_HEXCTRL_SEARCH_COMBO_SEARCH, wstrTextSearch);
	if (wstrTextSearch.IsEmpty())
		return;

	if (wstrTextSearch.Compare(wstrSearch.data()) != 0)
	{
		ClearAll();
		wstrSearch = wstrTextSearch;
	}

	m_iDirection = -1;
	m_fReplace = false;
	m_fAll = false;
	enSearchType = GetSearchMode();

	ComboSearchFill(wstrTextSearch);
	GetDlgItem(IDC_HEXCTRL_SEARCH_COMBO_SEARCH)->SetFocus();
	Search();
	SetActiveWindow();
}

void CHexDlgSearch::OnButtonReplace()
{
	CStringW wstrTextSearch;
	GetDlgItemTextW(IDC_HEXCTRL_SEARCH_COMBO_SEARCH, wstrTextSearch);
	if (wstrTextSearch.IsEmpty())
		return;
	if (wstrTextSearch.Compare(wstrSearch.data()) != 0)
	{
		ClearAll();
		wstrSearch = wstrTextSearch;
	}

	CStringW wstrTextReplace;
	GetDlgItemTextW(IDC_HEXCTRL_SEARCH_COMBO_REPLACE, wstrTextReplace);
	if (wstrTextReplace.IsEmpty())
		return;

	wstrReplace = wstrTextReplace;
	m_iDirection = 1;
	m_fReplace = true;
	m_fAll = false;
	enSearchType = GetSearchMode();

	ComboSearchFill(wstrTextSearch);
	ComboReplaceFill(wstrTextReplace);
	GetDlgItem(IDC_HEXCTRL_SEARCH_COMBO_SEARCH)->SetFocus();
	Search();
	SetActiveWindow();
}

void CHexDlgSearch::OnButtonReplaceAll()
{
	CStringW wstrTextSearch;
	GetDlgItemTextW(IDC_HEXCTRL_SEARCH_COMBO_SEARCH, wstrTextSearch);
	if (wstrTextSearch.IsEmpty())
		return;
	if (wstrTextSearch.Compare(wstrSearch.data()) != 0)
	{
		ClearAll();
		wstrSearch = wstrTextSearch;
	}

	CStringW wstrTextReplace;
	GetDlgItemTextW(IDC_HEXCTRL_SEARCH_COMBO_REPLACE, wstrTextReplace);
	if (wstrTextReplace.IsEmpty())
		return;

	m_fReplace = true;
	m_fAll = true;
	wstrReplace = wstrTextReplace;
	m_iDirection = 1;
	enSearchType = GetSearchMode();

	ComboSearchFill(wstrTextSearch);
	ComboReplaceFill(wstrTextReplace);
	GetDlgItem(IDC_HEXCTRL_SEARCH_COMBO_SEARCH)->SetFocus();
	Search();
	SetActiveWindow();
}

void CHexDlgSearch::OnActivate(UINT nState, CWnd * pWndOther, BOOL bMinimized)
{
	if (nState == WA_INACTIVE)
		SetLayeredWindowAttributes(0, 150, LWA_ALPHA);
	else
	{
		SetLayeredWindowAttributes(0, 255, LWA_ALPHA);
		GetDlgItem(IDC_HEXCTRL_SEARCH_COMBO_SEARCH)->SetFocus();
		if (GetHexCtrl()->IsCreated())
		{
			bool fMutable = GetHexCtrl()->IsMutable();
			GetDlgItem(IDC_HEXCTRL_SEARCH_COMBO_REPLACE)->EnableWindow(fMutable);
			GetDlgItem(IDC_HEXCTRL_SEARCH_BUTTON_REPLACE)->EnableWindow(fMutable);
			GetDlgItem(IDC_HEXCTRL_SEARCH_BUTTON_REPLACE_ALL)->EnableWindow(fMutable);
		}
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
		pDC->SetTextColor(m_fFound ? m_clrSearchFound : m_clrSearchFailed);
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
	m_ullIndex = { };
	m_dwCount = { };
	m_dwReplaced = { };
	m_iDirection = { };
	m_iWrap = { };
	m_fWrap = { false };
	m_fSecondMatch = { false };
	m_fFound = { false };
	m_fDoCount = { true };
	m_fReplace = { false };
	m_fAll = { false };

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