/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
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
	ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL CHexDlgSearch::Create(UINT nIDTemplate, CHexCtrl* pHexCtrl)
{
	m_pHexCtrl = pHexCtrl;

	return CDialogEx::Create(nIDTemplate, pHexCtrl);
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

bool CHexDlgSearch::IsSearchAvail()
{
	CHexCtrl* pHexCtrl = GetHexCtrl();
	if (!pHexCtrl)
		return false;

	if (m_wstrSearch.empty() || !pHexCtrl->IsDataSet() || m_ullOffset >= pHexCtrl->GetDataSize())
		return false;

	return true;
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

bool CHexDlgSearch::DoSearch(ULONGLONG& ullOffset, ULONGLONG ullUntil, const unsigned char* pSearch, size_t nSizeSearch, bool fForward)
{
	auto pHex = GetHexCtrl();
	ULONGLONG ullDataSize = pHex->GetDataSize();
	if (ullOffset + nSizeSearch > ullDataSize)
		return false;

	ULONGLONG ullSize = fForward ? ullUntil - ullOffset : ullOffset - ullUntil; //Depends on search direction
	ULONGLONG ullSizeChunk { };    //Size of the chunk to work with.
	ULONGLONG ullChunks { };
	ULONGLONG ullMemToAcquire { }; //Size of VirtualData memory for acquiring. It's bigger than ullSizeChunk.
	ULONGLONG ullOffsetSearch { };

	switch (pHex->GetDataMode())
	{
	case EHexDataMode::DATA_MEMORY:
		ullSizeChunk = ullSize;
		ullMemToAcquire = ullSizeChunk;
		ullChunks = 1;
		break;
	case EHexDataMode::DATA_MSG:
	case EHexDataMode::DATA_VIRTUAL:
	{
		ullMemToAcquire = pHex->GetCacheSize();
		if (ullMemToAcquire > ullSize + nSizeSearch)
			ullMemToAcquire = ullSize + nSizeSearch;
		ullSizeChunk = ullMemToAcquire - nSizeSearch;
		if (ullSize > ullSizeChunk)
			ullChunks = ullSize % ullSizeChunk ? ullSize / ullSizeChunk + 1 : ullSize / ullSizeChunk;
		else
			ullChunks = 1;
	}
	break;
	}

	if (fForward)
	{
		ullOffsetSearch = ullOffset;
		for (ULONGLONG iterChunk = 0; iterChunk < ullChunks; ++iterChunk)
		{
			if (ullOffsetSearch + ullMemToAcquire > ullDataSize)
			{
				ullMemToAcquire = ullDataSize - ullOffsetSearch;
				ullSizeChunk = ullMemToAcquire - nSizeSearch;
			}
			if (iterChunk > 0)
				ullOffsetSearch += ullSizeChunk;

			auto pData = pHex->GetData({ ullOffsetSearch, ullMemToAcquire });
			for (ULONGLONG i = 0; i <= ullSizeChunk; ++i)
			{
				if (memcmp(pData + i, pSearch, nSizeSearch) == 0)
				{
					ullOffset = ullOffsetSearch + i;
					return true;
				}
			}
		}
	}
	else
	{
		ullOffsetSearch = ullOffset - ullSizeChunk;
		for (ULONGLONG iterChunk = ullChunks; iterChunk > 0; --iterChunk)
		{
			auto pData = pHex->GetData({ ullOffsetSearch, ullMemToAcquire });
			for (auto i = static_cast<LONGLONG>(ullSizeChunk); i >= 0; --i)	//i might be negative.
			{
				if (memcmp(pData + i, pSearch, nSizeSearch) == 0)
				{
					ullOffset = ullOffsetSearch + i;
					return true;
				}
			}

			if ((ullOffsetSearch - ullSizeChunk) < ullUntil
				|| (ullOffsetSearch - ullSizeChunk) > (std::numeric_limits<ULONGLONG>::max() - ullSizeChunk))
			{
				ullMemToAcquire = (ullOffsetSearch - ullUntil) + nSizeSearch;
				ullSizeChunk = ullMemToAcquire - nSizeSearch;
			}
			ullOffsetSearch -= ullSizeChunk;
		}
	}

	return false;
}

void CHexDlgSearch::Search()
{
	CHexCtrl* pHexCtrl = GetHexCtrl();
	if (!pHexCtrl)
		return;

	auto ullDataSize = pHexCtrl->GetDataSize();
	if (m_wstrSearch.empty() || !pHexCtrl->IsDataSet() || m_ullOffset >= ullDataSize)
		return;

	m_fFound = false;
	size_t nSizeSearch { };
	const BYTE* pSearch { };
	size_t nSizeReplace { };
	PBYTE pReplace { };
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
		strReplace = WstrToStr(wstrReplace);
		strSearch = WstrToStr(m_wstrSearch);
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
		pSearch = reinterpret_cast<PBYTE>(strSearch.data());
		pReplace = reinterpret_cast<PBYTE>(strReplace.data());
	}
	break;
	case ESearchMode::SEARCH_ASCII:
	{
		strSearch = WstrToStr(m_wstrSearch);
		strReplace = WstrToStr(wstrReplace);
		nSizeSearch = strSearch.size();
		nSizeReplace = strReplace.size();
		pSearch = reinterpret_cast<PBYTE>(strSearch.data());
		pReplace = reinterpret_cast<PBYTE>(strReplace.data());
	}
	break;
	case ESearchMode::SEARCH_UTF16:
	{
		nSizeSearch = m_wstrSearch.size() * sizeof(wchar_t);
		nSizeReplace = wstrReplace.size() * sizeof(wchar_t);
		pSearch = reinterpret_cast<PBYTE>(m_wstrSearch.data());
		pReplace = reinterpret_cast<PBYTE>(wstrReplace.data());
	}
	break;
	}

	if ((nSizeSearch + m_ullOffset) > ullDataSize || (nSizeReplace + m_ullOffset) > ullDataSize)
	{
		m_ullOffset = 0;
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
			if (DoSearch(ullStart, ullUntil, pSearch, nSizeSearch))
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
				SearchReplace(m_ullOffset, pReplace, nSizeSearch, nSizeReplace);
				m_ullOffset += nSizeReplace; //Increase next search step to replaced count.
				m_dwReplaced++;
			}
			else
				m_ullOffset = m_fSecondMatch ? m_ullOffset + 1 : 0;

			if (DoSearch(m_ullOffset, ullUntil, pSearch, nSizeSearch))
			{
				m_fFound = true;
				m_fSecondMatch = true;
				m_fWrap = false;
				m_dwCount++;
			}
			if (!m_fFound && m_fSecondMatch)
			{
				m_ullOffset = 0; //Starting from the beginning.
				if (DoSearch(m_ullOffset, ullUntil, pSearch, nSizeSearch))
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
			if (m_fSecondMatch && m_ullOffset > 0)
			{
				m_ullOffset--;
				if (DoSearch(m_ullOffset, 0, pSearch, nSizeSearch, false))
				{
					m_fFound = true;
					m_fSecondMatch = true;
					m_fWrap = false;
					m_dwCount--;
				}
			}
			if (!m_fFound)
			{
				m_ullOffset = ullDataSize - nSizeSearch;
				if (DoSearch(m_ullOffset, 0, pSearch, nSizeSearch, false))
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

			ULONGLONG ullSelIndex = m_ullOffset;
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

void CHexDlgSearch::SearchReplace(ULONGLONG ullIndex, PBYTE pData, size_t nSizeData, size_t nSizeReplace, bool fRedraw)
{
	MODIFYSTRUCT hms;
	hms.vecSpan.emplace_back(HEXSPANSTRUCT { ullIndex, nSizeData });
	hms.ullDataSize = nSizeReplace;
	hms.pData = reinterpret_cast<std::byte*>(pData);
	GetHexCtrl()->Modify(hms, fRedraw);
}

void CHexDlgSearch::OnButtonSearchF()
{
	CStringW wstrTextSearch;
	GetDlgItemTextW(IDC_HEXCTRL_SEARCH_COMBO_SEARCH, wstrTextSearch);
	if (wstrTextSearch.IsEmpty())
		return;
	if (wstrTextSearch.Compare(m_wstrSearch.data()) != 0)
	{
		ClearAll();
		m_wstrSearch = wstrTextSearch;
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

	if (wstrTextSearch.Compare(m_wstrSearch.data()) != 0)
	{
		ClearAll();
		m_wstrSearch = wstrTextSearch;
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
	if (wstrTextSearch.Compare(m_wstrSearch.data()) != 0)
	{
		ClearAll();
		m_wstrSearch = wstrTextSearch;
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
	if (wstrTextSearch.Compare(m_wstrSearch.data()) != 0)
	{
		ClearAll();
		m_wstrSearch = wstrTextSearch;
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
	m_ullOffset = { };
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

void CHexDlgSearch::OnDestroy()
{
	CDialogEx::OnDestroy();

	m_stBrushDefault.DeleteObject();
}
