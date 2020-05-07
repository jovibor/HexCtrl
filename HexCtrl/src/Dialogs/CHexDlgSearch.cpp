/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository of the project: https://github.com/jovibor/HexCtrl/           *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#include "stdafx.h"
#include "../Helper.h"
#include "CHexDlgCallback.h"
#include "CHexDlgSearch.h"
#include <thread>

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
	Search();
}

bool CHexDlgSearch::IsSearchAvail()
{
	CHexCtrl* pHexCtrl = GetHexCtrl();
	if (!pHexCtrl)
		return false;

	if (m_wstrTextSearch.empty() || !pHexCtrl->IsDataSet() || m_ullOffset >= pHexCtrl->GetDataSize())
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

bool CHexDlgSearch::PrepareSearch()
{
	static const wchar_t* const wstrReplaceWarning { L"Replacing string is longer than Find string.\r\n"
		"Do you want to overwrite the bytes following search occurrence?\r\n"
		"Choosing \"No\" will cancel search." };
	static const wchar_t* const wstrWrongInput { L"Wrong input data format!" };

	auto pHexCtrl = GetHexCtrl();
	if (!pHexCtrl)
		return false;
	auto ullDataSize = pHexCtrl->GetDataSize();

	CStringW wstrTextSearch;
	GetDlgItemTextW(IDC_HEXCTRL_SEARCH_COMBO_SEARCH, wstrTextSearch);
	if (wstrTextSearch.IsEmpty())
		return false;

	if (wstrTextSearch.Compare(m_wstrTextSearch.data()) != 0)
	{
		ResetSearch();
		m_wstrTextSearch = wstrTextSearch;
	}
	ComboSearchFill(wstrTextSearch);

	if (m_fReplace)
	{
		wchar_t warrReplace[64];
		GetDlgItemTextW(IDC_HEXCTRL_SEARCH_COMBO_REPLACE, warrReplace, _countof(warrReplace));
		m_wstrTextReplace = warrReplace;
		if (m_wstrTextReplace.empty())
			return false;

		ComboReplaceFill(warrReplace);
	}
	GetDlgItem(IDC_HEXCTRL_SEARCH_COMBO_SEARCH)->SetFocus();

	switch (GetSearchMode())
	{
	case ESearchMode::SEARCH_HEX:
	{
		m_strSearch = WstrToStr(m_wstrTextSearch);
		m_strReplace = WstrToStr(m_wstrTextReplace);
		if (!StrToHex(m_strSearch, m_strSearch))
		{
			m_iWrap = 1;
			return false;
		}
		m_nSizeSearch = m_strSearch.size();
		if ((m_fReplace && !StrToHex(m_strReplace, m_strReplace)))
		{
			MessageBoxW(wstrWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		m_nSizeReplace = m_strReplace.size();
		m_pSearchData = reinterpret_cast<std::byte*>(m_strSearch.data());
		m_pReplaceData = reinterpret_cast<std::byte*>(m_strReplace.data());
	}
	break;
	case ESearchMode::SEARCH_ASCII:
	{
		m_strSearch = WstrToStr(m_wstrTextSearch);
		m_strReplace = WstrToStr(m_wstrTextReplace);
		m_nSizeSearch = m_strSearch.size();
		m_nSizeReplace = m_strReplace.size();
		m_pSearchData = reinterpret_cast<std::byte*>(m_strSearch.data());
		m_pReplaceData = reinterpret_cast<std::byte*>(m_strReplace.data());
	}
	break;
	case ESearchMode::SEARCH_UTF16:
	{
		m_nSizeSearch = m_wstrTextSearch.size() * sizeof(wchar_t);
		m_nSizeReplace = m_wstrTextReplace.size() * sizeof(wchar_t);
		m_pSearchData = reinterpret_cast<std::byte*>(m_wstrTextSearch.data());
		m_pReplaceData = reinterpret_cast<std::byte*>(m_wstrTextReplace.data());
	}
	break;
	}

	if ((reinterpret_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_SEARCH_CHECK_SELECTION)))->GetCheck() == BST_CHECKED)
	{
		auto vecSel = pHexCtrl->GetSelection();
		if (vecSel.empty()) //No selection.
			return false;

		auto ullSelSize = vecSel.front().ullSize;
		if (ullSelSize < m_nSizeSearch) //Selection is too small.
			return false;

		auto ullSelStart = vecSel.front().ullOffset;
		m_ullSearchStart = ullSelStart;
		m_ullSearchEnd = ullSelStart + ullSelSize - m_nSizeSearch;

		m_fSelection = true;
	}
	else
	{
		m_ullSearchStart = 0;
		m_ullSearchEnd = pHexCtrl->GetDataSize() - m_nSizeSearch;
		m_fSelection = false;
	}

	if (m_ullOffset + m_nSizeSearch > ullDataSize || m_ullOffset + m_nSizeReplace > ullDataSize)
	{
		m_ullOffset = 0;
		m_fSecondMatch = false;
		return false;
	}
	if (m_fReplaceWarning && m_fReplace && (m_nSizeReplace > m_nSizeSearch))
	{
		if (IDNO == MessageBoxW(wstrReplaceWarning, L"Warning", MB_YESNO | MB_ICONQUESTION | MB_TOPMOST))
			return false;

		m_fReplaceWarning = false;
	}

	Search();
	SetActiveWindow();

	return true;
}

void CHexDlgSearch::Search()
{
	auto pHexCtrl = GetHexCtrl();
	if (!pHexCtrl)
		return;
	if (m_wstrTextSearch.empty() || !pHexCtrl->IsDataSet() || m_ullOffset >= pHexCtrl->GetDataSize())
		return;

	m_fFound = false;
	ULONGLONG ullUntil = m_ullSearchEnd;

	///////////////Actual Search:////////////////////////////
	if (m_fReplace && m_fAll) //Replace All
	{
		ULONGLONG ullStart = m_ullSearchStart;
		while (true)
		{
			if (DoSearch(ullStart, ullUntil, m_pSearchData, m_nSizeSearch))
			{
				Replace(ullStart, m_pReplaceData, m_nSizeSearch, m_nSizeReplace, false);
				ullStart += m_nSizeReplace;
				m_fFound = true;
				m_dwReplaced++;
			}
			else
				break;
		}
	}
	else //Search or Replace
	{
		if (m_iDirection == 1) //Forward direction
		{
			if (m_fReplace && m_fSecondMatch)
			{
				Replace(m_ullOffset, m_pReplaceData, m_nSizeSearch, m_nSizeReplace);
				m_ullOffset += m_nSizeReplace; //Increase next search step to replaced count.
				m_dwReplaced++;
			}
			else
				m_ullOffset = m_fSecondMatch ? m_ullOffset + 1 : m_ullSearchStart;

			if (DoSearch(m_ullOffset, ullUntil, m_pSearchData, m_nSizeSearch))
			{
				m_fFound = true;
				m_fSecondMatch = true;
				m_fWrap = false;
				m_dwCount++;
			}
			if (!m_fFound && m_fSecondMatch)
			{
				m_ullOffset = m_ullSearchStart; //Starting from the beginning.
				if (DoSearch(m_ullOffset, ullUntil, m_pSearchData, m_nSizeSearch))
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
			ullUntil = m_ullSearchStart;
			if (m_fSecondMatch && m_ullOffset > 0)
			{
				m_ullOffset--;
				if (DoSearch(m_ullOffset, ullUntil, m_pSearchData, m_nSizeSearch, false))
				{
					m_fFound = true;
					m_fSecondMatch = true;
					m_fWrap = false;
					m_dwCount--;
				}
			}
			if (!m_fFound)
			{
				m_ullOffset = m_ullSearchEnd;
				if (DoSearch(m_ullOffset, ullUntil, m_pSearchData, m_nSizeSearch, false))
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
			ULONGLONG ullSelSize = m_fReplace ? m_nSizeReplace : m_nSizeSearch;
			if (m_fSelection)
			{
				//Highlight selection.
				std::vector<HEXSPANSTRUCT> vec { { ullSelIndex, ullSelSize } };
				pHexCtrl->SetSelHighlight(vec);
				pHexCtrl->GoToOffset(ullSelIndex);
				pHexCtrl->Redraw();
			}
			else
				pHexCtrl->GoToOffset(ullSelIndex, true, ullSelSize);
		}
	}
	else
	{
		ResetSearch();
		if (m_iWrap == 1)
			wstrInfo = L"Didn't find any occurrence, the end is reached.";
		else
			wstrInfo = L"Didn't find any occurrence, the begining is reached.";
	}

	GetDlgItem(IDC_HEXCTRL_SEARCH_STATIC_TEXTBOTTOM)->SetWindowTextW(wstrInfo.data());
}

bool CHexDlgSearch::DoSearch(ULONGLONG& ullStart, ULONGLONG ullEnd, std::byte* pSearch, size_t nSizeSearch, bool fForward)
{
	auto pHex = GetHexCtrl();
	auto ullMaxDataSize = pHex->GetDataSize();
	if (ullStart + nSizeSearch > ullMaxDataSize)
		return false;

	ULONGLONG ullSize = fForward ? ullEnd - ullStart : ullStart - ullEnd; //Depends on search direction
	ULONGLONG ullSizeChunk { };    //Size of the chunk to work with.
	ULONGLONG ullChunks { };
	ULONGLONG ullMemToAcquire { }; //Size of VirtualData memory for acquiring. It's bigger than ullSizeChunk.
	ULONGLONG ullOffsetSearch { };
	constexpr auto sizeQuick { 1024 * 256 }; //256KB.

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

	bool fResult { false };
	CHexDlgCallback dlg(L"Searching...");
	std::thread thrd([&]() {
		if (fForward)
		{
			ullOffsetSearch = ullStart;
			for (ULONGLONG iterChunk = 0; iterChunk < ullChunks; ++iterChunk)
			{
				if (ullOffsetSearch + ullMemToAcquire > ullMaxDataSize)
				{
					ullMemToAcquire = ullMaxDataSize - ullOffsetSearch;
					ullSizeChunk = ullMemToAcquire - nSizeSearch;
				}
				if (iterChunk > 0)
					ullOffsetSearch += ullSizeChunk;

				auto pData = pHex->GetData({ ullOffsetSearch, ullMemToAcquire });
				for (ULONGLONG i = 0; i <= ullSizeChunk; ++i)
				{
					if (memcmp(pData + i, pSearch, nSizeSearch) == 0)
					{
						ullStart = ullOffsetSearch + i;
						fResult = true;
						goto exit;
					}
					if (dlg.IsCanceled())
						goto exit;
				}
			}
		}
		else
		{
			ullOffsetSearch = ullStart - ullSizeChunk;
			for (ULONGLONG iterChunk = ullChunks; iterChunk > 0; --iterChunk)
			{
				auto pData = pHex->GetData({ ullOffsetSearch, ullMemToAcquire });
				for (auto i = static_cast<LONGLONG>(ullSizeChunk); i >= 0; --i)	//i might be negative.
				{
					if (memcmp(pData + i, pSearch, nSizeSearch) == 0)
					{
						ullStart = ullOffsetSearch + i;
						fResult = true;
						goto exit;
					}
					if (dlg.IsCanceled())
						goto exit;
				}

				if ((ullOffsetSearch - ullSizeChunk) < ullEnd
					|| (ullOffsetSearch - ullSizeChunk) > (std::numeric_limits<ULONGLONG>::max() - ullSizeChunk))
				{
					ullMemToAcquire = (ullOffsetSearch - ullEnd) + nSizeSearch;
					ullSizeChunk = ullMemToAcquire - nSizeSearch;
				}
				ullOffsetSearch -= ullSizeChunk;
			}
		}
	exit:
		dlg.Cancel();
		});
	if (ullSize > sizeQuick) //Showing "Cancel" dialog only when data > sizeQuick
		dlg.DoModal();
	thrd.join();

	return fResult;
}

void CHexDlgSearch::Replace(ULONGLONG ullIndex, std::byte* pData, size_t nSizeData, size_t nSizeReplace, bool fRedraw)
{
	MODIFYSTRUCT hms;
	hms.vecSpan.emplace_back(HEXSPANSTRUCT { ullIndex, nSizeData });
	hms.ullDataSize = nSizeReplace;
	hms.pData = pData;
	GetHexCtrl()->Modify(hms, fRedraw);
}

void CHexDlgSearch::OnButtonSearchF()
{
	m_iDirection = 1;
	m_fReplace = false;
	m_fAll = false;
	PrepareSearch();
}

void CHexDlgSearch::OnButtonSearchB()
{
	m_iDirection = -1;
	m_fReplace = false;
	m_fAll = false;
	PrepareSearch();
}

void CHexDlgSearch::OnButtonReplace()
{
	m_iDirection = 1;
	m_fReplace = true;
	m_fAll = false;
	PrepareSearch();
}

void CHexDlgSearch::OnButtonReplaceAll()
{
	m_iDirection = 1;
	m_fReplace = true;
	m_fAll = true;
	PrepareSearch();
}

void CHexDlgSearch::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if (nState == WA_INACTIVE)
		SetLayeredWindowAttributes(0, 150, LWA_ALPHA);
	else
	{
		SetLayeredWindowAttributes(0, 255, LWA_ALPHA);
		GetDlgItem(IDC_HEXCTRL_SEARCH_COMBO_SEARCH)->SetFocus();
		if (GetHexCtrl()->IsCreated() && GetHexCtrl()->IsDataSet())
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
	ResetSearch();
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
		ResetSearch();
	m_uRadioCurrent = nID;
}

void CHexDlgSearch::ResetSearch()
{
	m_ullOffset = { };
	m_dwCount = { };
	m_dwReplaced = { };
	m_iWrap = { };
	m_fWrap = { false };
	m_fSecondMatch = { false };
	m_fFound = { false };
	m_fDoCount = { true };
	GetHexCtrl()->ClearSelHighlight();

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
	auto pCombo = (CComboBox*)GetDlgItem(IDC_HEXCTRL_SEARCH_COMBO_SEARCH);
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
	auto pCombo = (CComboBox*)GetDlgItem(IDC_HEXCTRL_SEARCH_COMBO_REPLACE);
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
