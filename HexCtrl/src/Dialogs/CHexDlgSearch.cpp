/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository of the project: https://github.com/jovibor/HexCtrl/           *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#include "stdafx.h"
#include "../../res/HexCtrlRes.h"
#include "../Helper.h"
#include "CHexDlgCallback.h"
#include "CHexDlgSearch.h"
#include <cassert>
#include <thread>

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

/************************************************************
* CHexDlgSearch class implementation.                       *
* This class implements search routines within HexControl.  *
************************************************************/
BEGIN_MESSAGE_MAP(CHexDlgSearch, CDialogEx)
	ON_WM_ACTIVATE()
	ON_WM_CLOSE()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_BTN_SEARCH_F, &CHexDlgSearch::OnButtonSearchF)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_BTN_SEARCH_B, &CHexDlgSearch::OnButtonSearchB)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_BTN_FINDALL, &CHexDlgSearch::OnButtonFindAll)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_BTN_REPLACE, &CHexDlgSearch::OnButtonReplace)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_BTN_REPLACE_ALL, &CHexDlgSearch::OnButtonReplaceAll)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_CHECK_SELECTION, &CHexDlgSearch::OnCheckSel)
	ON_CBN_SELCHANGE(IDC_HEXCTRL_SEARCH_COMBO_MODE, &CHexDlgSearch::OnComboModeSelChange)
	ON_NOTIFY(LVN_GETDISPINFOW, IDC_HEXCTRL_SEARCH_LIST_MAIN, &CHexDlgSearch::OnListGetDispInfo)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_HEXCTRL_SEARCH_LIST_MAIN, &CHexDlgSearch::OnListItemChanged)
	ON_NOTIFY(NM_RCLICK, IDC_HEXCTRL_SEARCH_LIST_MAIN, &CHexDlgSearch::OnListRClick)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CHexDlgSearch::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_CHECK_SELECTION, m_stCheckSel);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_COMBO_SEARCH, m_stComboSearch);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_COMBO_REPLACE, m_stComboReplace);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_COMBO_MODE, m_stComboMode);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_EDIT_START, m_stEditStart);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_EDIT_STEP, m_stEditStep);
}

BOOL CHexDlgSearch::Create(UINT nIDTemplate, CHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	if (pHexCtrl == nullptr)
		return FALSE;

	m_pHexCtrl = pHexCtrl;

	return CDialogEx::Create(nIDTemplate, pHexCtrl);
}

BOOL CHexDlgSearch::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_stBrushDefault.CreateSolidBrush(m_clrBkTextArea);

	auto iIndex = m_stComboMode.AddString(L"Hex");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_HEX));
	m_stComboMode.SetCurSel(iIndex);
	iIndex = m_stComboMode.AddString(L"Text (ASCII)");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_ASCII));
	iIndex = m_stComboMode.AddString(L"Text (UTF-16)");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_WCHAR));
	iIndex = m_stComboMode.AddString(L"Number (BYTE)");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_BYTE));
	iIndex = m_stComboMode.AddString(L"Number (WORD)");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_WORD));
	iIndex = m_stComboMode.AddString(L"Number (DWORD)");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_DWORD));
	iIndex = m_stComboMode.AddString(L"Number (QWORD)");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_QWORD));
	iIndex = m_stComboMode.AddString(L"Number (Float)");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_FLOAT));
	iIndex = m_stComboMode.AddString(L"Number (Double)");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_DOUBLE));

	m_pListMain->CreateDialogCtrl(IDC_HEXCTRL_SEARCH_LIST_MAIN, this);
	m_pListMain->SetExtendedStyle(LVS_EX_HEADERDRAGDROP);
	m_pListMain->InsertColumn(0, L"\u2116", 0, 40);
	m_pListMain->InsertColumn(1, L"Offset", LVCFMT_LEFT, 445);

	m_stMenuList.CreatePopupMenu();
	m_stMenuList.AppendMenuW(MF_BYPOSITION, static_cast<UINT_PTR>(EMenuID::IDM_SEARCH_ADDBKM), L"Add bookmark");
	m_stMenuList.AppendMenuW(MF_BYPOSITION, static_cast<UINT_PTR>(EMenuID::IDM_SEARCH_SELECTALL), L"Select All");
	m_stMenuList.AppendMenuW(MF_SEPARATOR);
	m_stMenuList.AppendMenuW(MF_BYPOSITION, static_cast<UINT_PTR>(EMenuID::IDM_SEARCH_CLEARALL), L"Clear All");

	SetEditStep(m_ullStep);

	return TRUE;
}

void CHexDlgSearch::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if (nState == WA_INACTIVE)
		SetLayeredWindowAttributes(0, 150, LWA_ALPHA);
	else
	{
		SetLayeredWindowAttributes(0, 255, LWA_ALPHA);
		m_stComboSearch.SetFocus();

		auto pHexCtrl = GetHexCtrl();
		if (pHexCtrl->IsCreated() && pHexCtrl->IsDataSet())
		{
			bool fMutable = pHexCtrl->IsMutable();
			m_stComboReplace.EnableWindow(fMutable);
			GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_REPLACE)->EnableWindow(fMutable);
			GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_REPLACE_ALL)->EnableWindow(fMutable);
		}
	}

	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
}

void CHexDlgSearch::OnOK()
{
	OnButtonSearchF();
}

void CHexDlgSearch::OnCancel()
{
	GetHexCtrl()->SetFocus();

	CDialogEx::OnCancel();
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

void CHexDlgSearch::OnButtonFindAll()
{
	m_iDirection = 1;
	m_fReplace = false;
	m_fAll = true;
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

BOOL CHexDlgSearch::OnCommand(WPARAM wParam, LPARAM lParam)
{
	bool fHere { true };
	switch (static_cast<EMenuID>(LOWORD(wParam)))
	{
	case EMenuID::IDM_SEARCH_ADDBKM:
	{		
		int nItem { -1 };
		for (auto i = 0UL; i < m_pListMain->GetSelectedCount(); ++i)
		{
			HEXBKMSTRUCT hbs{ };
			nItem = m_pListMain->GetNextItem(nItem, LVNI_SELECTED);
			hbs.vecSpan.emplace_back(HEXSPANSTRUCT { m_vecSearchRes.at(static_cast<size_t>(nItem)),
				m_fReplace ? m_nSizeReplace : m_nSizeSearch });

			GetHexCtrl()->BkmAdd(hbs, true);
		}		
	}
	break;
	case EMenuID::IDM_SEARCH_SELECTALL:
		m_pListMain->SetItemState(-1, LVIS_SELECTED, LVIS_SELECTED);
		break;
	case EMenuID::IDM_SEARCH_CLEARALL:
		ClearList();
		break;
	default:
		fHere = false;
	}

	if (fHere)
		return TRUE;

	return CDialogEx::OnCommand(wParam, lParam);
}

void CHexDlgSearch::OnCheckSel()
{
	m_stEditStart.EnableWindow(m_stCheckSel.GetCheck() == BST_CHECKED ? 0 : 1);
}

void CHexDlgSearch::OnComboModeSelChange()
{
	if (auto eMode = GetSearchMode(); eMode != m_eModeCurr)
	{
		ResetSearch();
		m_eModeCurr = eMode;
	}
}

void CHexDlgSearch::OnListGetDispInfo(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pDispInfo = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
	const auto pItem = &pDispInfo->item;

	if (pItem->mask & LVIF_TEXT)
	{
		const auto nItemID = static_cast<size_t>(pItem->iItem);
		const auto nMaxLengh = static_cast<size_t>(pItem->cchTextMax);

		switch (pItem->iSubItem)
		{
		case 0: //Index number.
			swprintf_s(pItem->pszText, nMaxLengh, L"%zd", nItemID + 1);
			break;
		case 1: //Offset.
			swprintf_s(pItem->pszText, nMaxLengh, L"0x%llX", m_vecSearchRes[nItemID]);
			break;
		}
	}
}

void CHexDlgSearch::OnListItemChanged(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	if (const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
		pNMI->iItem != -1 && pNMI->iSubItem != -1 && (pNMI->uNewState & LVIS_SELECTED))
	{
		SetEditStartAt(m_vecSearchRes[static_cast<size_t>(pNMI->iItem)]);

		std::vector<HEXSPANSTRUCT> vecSpan { };
		int nItem = -1;
		for (auto i = 0UL; i < m_pListMain->GetSelectedCount(); ++i)
		{
			nItem = m_pListMain->GetNextItem(nItem, LVNI_SELECTED);

			//Do not yet add selected (clicked) item (in multiselect), will add it after the loop,
			//so that it's always last in vec, to highlight it in HexCtrlHighlight.
			if (pNMI->iItem != nItem)
				vecSpan.emplace_back(HEXSPANSTRUCT { m_vecSearchRes.at(static_cast<size_t>(nItem)),
					m_fReplace ? m_nSizeReplace : m_nSizeSearch });
		}
		vecSpan.emplace_back(HEXSPANSTRUCT { m_vecSearchRes.at(static_cast<size_t>(pNMI->iItem)),
			m_fReplace ? m_nSizeReplace : m_nSizeSearch });

		HexCtrlHighlight(vecSpan);
	}
}

void CHexDlgSearch::OnListRClick(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	bool fEnabled { m_pListMain->GetItemCount() > 0 };
	m_stMenuList.EnableMenuItem(static_cast<UINT_PTR>(EMenuID::IDM_SEARCH_ADDBKM), (fEnabled ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
	m_stMenuList.EnableMenuItem(static_cast<UINT_PTR>(EMenuID::IDM_SEARCH_SELECTALL), (fEnabled ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
	m_stMenuList.EnableMenuItem(static_cast<UINT_PTR>(EMenuID::IDM_SEARCH_CLEARALL), (fEnabled ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);

	POINT pt;
	GetCursorPos(&pt);
	m_stMenuList.TrackPopupMenuEx(TPM_LEFTALIGN, pt.x, pt.y, this, nullptr);
}

HBRUSH CHexDlgSearch::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (pWnd->GetDlgCtrlID() == IDC_HEXCTRL_SEARCH_STATIC_TEXTBOTTOM)
	{
		pDC->SetBkColor(m_clrBkTextArea);
		pDC->SetTextColor(m_fFound ? m_clrSearchFound : m_clrSearchFailed);
		return m_stBrushDefault;
	}

	return CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CHexDlgSearch::AddToList(ULONGLONG ullOffset)
{
	int iHighlight;
	if (auto iter = std::find(m_vecSearchRes.begin(), m_vecSearchRes.end(), ullOffset);
		iter == m_vecSearchRes.end() && m_vecSearchRes.size() < static_cast<size_t>(m_dwMaxSearch)) //Max found search occurences.
	{
		m_vecSearchRes.emplace_back(ullOffset);
		iHighlight = static_cast<int>(m_vecSearchRes.size());
		m_pListMain->SetItemCountEx(iHighlight);
		iHighlight -= 1;
	}
	else
		iHighlight = static_cast<int>(iter - m_vecSearchRes.begin());

	m_pListMain->SetItemState(-1, 0, LVIS_SELECTED);
	m_pListMain->SetItemState(iHighlight, LVIS_SELECTED, LVIS_SELECTED);
	m_pListMain->EnsureVisible(iHighlight, TRUE);
}

void CHexDlgSearch::ClearList()
{
	m_pListMain->SetItemCountEx(0);
	m_vecSearchRes.clear();
}

void CHexDlgSearch::HexCtrlHighlight(const std::vector<HEXSPANSTRUCT>& vecSel)
{
	auto pHexCtrl = GetHexCtrl();

	if (m_fSelection) //Highlight selection.
		pHexCtrl->SetSelHighlight(vecSel);
	else
		pHexCtrl->SetSelection(vecSel);

	if (!pHexCtrl->IsOffsetVisible(vecSel.back().ullOffset))
		pHexCtrl->GoToOffset(vecSel.back().ullOffset);
}

void CHexDlgSearch::Search(bool fForward)
{
	m_iDirection = fForward ? 1 : -1;
	m_fReplace = false;
	m_fAll = false;
	Search();
}

bool CHexDlgSearch::IsSearchAvail()
{
	auto pHexCtrl = GetHexCtrl();
	return !(m_wstrTextSearch.empty() || !pHexCtrl->IsDataSet() || m_ullOffsetCurr >= pHexCtrl->GetDataSize());
}

BOOL CHexDlgSearch::ShowWindow(int nCmdShow)
{
	if (nCmdShow == SW_SHOW)
	{
		int iChkStatus { BST_UNCHECKED };
		if (auto vecSel = m_pHexCtrl->GetSelection(); vecSel.size() == 1 && vecSel.back().ullSize > 1)
			iChkStatus = BST_CHECKED;

		m_stCheckSel.SetCheck(iChkStatus);
		m_stEditStart.EnableWindow(iChkStatus == BST_CHECKED ? 0 : 1);
		BringWindowToTop();
	}

	return CDialogEx::ShowWindow(nCmdShow);
}

void CHexDlgSearch::PrepareSearch()
{
	auto pHexCtrl = GetHexCtrl();
	auto ullDataSize = pHexCtrl->GetDataSize();

	//"Search" text.
	CStringW wstrTextSearch;
	m_stComboSearch.GetWindowTextW(wstrTextSearch);
	if (wstrTextSearch.IsEmpty())
		return;
	if (wstrTextSearch.Compare(m_wstrTextSearch.data()) != 0)
	{
		ResetSearch();
		m_wstrTextSearch = wstrTextSearch;
	}
	ComboSearchFill(wstrTextSearch);

	//Start at.
	CStringW wstrStart;
	ULONGLONG ullOffsetCurr { };
	m_stEditStart.GetWindowTextW(wstrStart);
	if (!wstrStart.IsEmpty() && !wstr2num(wstrStart.GetString(), ullOffsetCurr))
		return;
	m_ullOffsetCurr = ullOffsetCurr;

	//Step.
	CStringW wstrStep;
	ULONGLONG ullStep;
	m_stEditStep.GetWindowTextW(wstrStep);
	if (wstrStep.IsEmpty() || !wstr2num(wstrStep.GetString(), ullStep))
		return;
	m_ullStep = ullStep;

	if (m_fReplace)
	{
		wchar_t warrReplace[64];
		GetDlgItemTextW(IDC_HEXCTRL_SEARCH_COMBO_REPLACE, warrReplace, _countof(warrReplace));
		m_wstrTextReplace = warrReplace;
		if (m_wstrTextReplace.empty())
		{
			MessageBoxW(m_wstrWrongInput.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
			m_stComboReplace.SetFocus();
			return;
		}
		ComboReplaceFill(warrReplace);
	}
	m_stComboSearch.SetFocus();

	bool fSuccess { false };
	switch (GetSearchMode())
	{
	case EMode::SEARCH_HEX:
		fSuccess = PrepareHex();
		break;
	case EMode::SEARCH_ASCII:
		fSuccess = PrepareASCII();
		break;
	case EMode::SEARCH_WCHAR:
		fSuccess = PrepareWCHAR();
		break;
	case EMode::SEARCH_BYTE:
		fSuccess = PrepareBYTE();
		break;
	case EMode::SEARCH_WORD:
		fSuccess = PrepareWORD();
		break;
	case EMode::SEARCH_DWORD:
		fSuccess = PrepareDWORD();
		break;
	case EMode::SEARCH_QWORD:
		fSuccess = PrepareQWORD();
		break;
	case EMode::SEARCH_FLOAT:
		fSuccess = PrepareFloat();
		break;
	case EMode::SEARCH_DOUBLE:
		fSuccess = PrepareDouble();
		break;
	}
	if (!fSuccess)
		return;

	//Search in selection.
	if (m_stCheckSel.GetCheck() == BST_CHECKED)
	{
		auto vecSel = pHexCtrl->GetSelection();
		if (vecSel.empty()) //No selection.
			return;

		auto& refFront = vecSel.front();
		//If the current selection differs from the previous one.
		if (m_stSelSpan.ullOffset != refFront.ullOffset ||
			m_stSelSpan.ullSize != refFront.ullSize)
		{
			ClearList();
			m_dwCount = 0;
			m_fSecondMatch = false;
			m_stSelSpan = refFront;
			m_ullOffsetCurr = refFront.ullOffset;
		}

		auto ullSelSize = refFront.ullSize;
		if (ullSelSize < m_nSizeSearch) //Selection is too small.
			return;

		m_ullOffsetStart = refFront.ullOffset;
		m_ullOffsetEnd = m_ullOffsetStart + ullSelSize - m_nSizeSearch;
		m_ullEndSentinel = m_ullOffsetStart + ullSelSize;
		m_fSelection = true;
	}
	else //Search in whole data.
	{
		m_ullOffsetStart = 0;
		m_ullOffsetEnd = ullDataSize - m_nSizeSearch;
		m_ullEndSentinel = pHexCtrl->GetDataSize();
		m_fSelection = false;
	}

	if (m_ullOffsetCurr + m_nSizeSearch > ullDataSize || m_ullOffsetCurr + m_nSizeReplace > ullDataSize)
	{
		m_ullOffsetCurr = 0;
		m_fSecondMatch = false;
		return;
	}

	static bool fReplaceWarning { true }; //Show Replace string size exceeds Warning or not.
	if (fReplaceWarning && m_fReplace && (m_nSizeReplace > m_nSizeSearch))
	{
		static std::wstring_view wstrReplaceWarning { L"The replacing string is longer than Find string.\r\n"
			"Do you want to overwrite the bytes following search occurrence?\r\n"
			"Choosing \"No\" will cancel search." };
		if (IDNO == MessageBoxW(wstrReplaceWarning.data(), L"Warning", MB_YESNO | MB_ICONQUESTION | MB_TOPMOST))
			return;
		fReplaceWarning = false;
	}

	Search();
	SetActiveWindow();
}

bool CHexDlgSearch::PrepareHex()
{
	m_strSearch = wstr2str(m_wstrTextSearch);
	m_strReplace = wstr2str(m_wstrTextReplace);
	if (!str2hex(m_strSearch, m_strSearch))
	{
		m_iWrap = 1;
		MessageBoxW(m_wstrWrongInput.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}
	m_nSizeSearch = m_strSearch.size();

	if ((m_fReplace && !str2hex(m_strReplace, m_strReplace)))
	{
		MessageBoxW(m_wstrWrongInput.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		m_stComboReplace.SetFocus();
		return false;
	}
	m_nSizeReplace = m_strReplace.size();
	m_pSearchData = reinterpret_cast<std::byte*>(m_strSearch.data());
	m_pReplaceData = reinterpret_cast<std::byte*>(m_strReplace.data());

	return true;
}

bool CHexDlgSearch::PrepareASCII()
{
	m_strSearch = wstr2str(m_wstrTextSearch);
	m_strReplace = wstr2str(m_wstrTextReplace);
	m_nSizeSearch = m_strSearch.size();
	m_nSizeReplace = m_strReplace.size();
	m_pSearchData = reinterpret_cast<std::byte*>(m_strSearch.data());
	m_pReplaceData = reinterpret_cast<std::byte*>(m_strReplace.data());

	return true;
}

bool CHexDlgSearch::PrepareWCHAR()
{
	m_nSizeSearch = m_wstrTextSearch.size() * sizeof(wchar_t);
	m_nSizeReplace = m_wstrTextReplace.size() * sizeof(wchar_t);
	m_pSearchData = reinterpret_cast<std::byte*>(m_wstrTextSearch.data());
	m_pReplaceData = reinterpret_cast<std::byte*>(m_wstrTextReplace.data());

	return true;
}

bool CHexDlgSearch::PrepareBYTE()
{
	BYTE bData { };
	BYTE bDataRet { };
	if (!wstr2num(m_wstrTextSearch, bData) || (m_fReplace && !wstr2num(m_wstrTextReplace, bDataRet)))
	{
		MessageBoxW(m_wstrWrongInput.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}
	m_strSearch = bData;
	m_strReplace = bDataRet;
	m_nSizeSearch = m_strSearch.size();
	m_nSizeReplace = m_strReplace.size();
	m_pSearchData = reinterpret_cast<std::byte*>(m_strSearch.data());
	m_pReplaceData = reinterpret_cast<std::byte*>(m_strReplace.data());

	return true;
}

bool CHexDlgSearch::PrepareWORD()
{
	WORD wData { };
	WORD wDataRet { };
	if (!wstr2num(m_wstrTextSearch, wData) || (m_fReplace && !wstr2num(m_wstrTextReplace, wDataRet)))
	{
		MessageBoxW(m_wstrWrongInput.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}

	m_strSearch.assign(reinterpret_cast<char*>(&wData), sizeof(WORD));
	m_strReplace.assign(reinterpret_cast<char*>(&wDataRet), sizeof(WORD));
	m_nSizeSearch = m_strSearch.size();
	m_nSizeReplace = m_strReplace.size();
	m_pSearchData = reinterpret_cast<std::byte*>(m_strSearch.data());
	m_pReplaceData = reinterpret_cast<std::byte*>(m_strReplace.data());

	return true;
}

bool CHexDlgSearch::PrepareDWORD()
{
	DWORD dwData { };
	DWORD dwDataRet { };
	if (!wstr2num(m_wstrTextSearch, dwData) || (m_fReplace && !wstr2num(m_wstrTextReplace, dwDataRet)))
	{
		MessageBoxW(m_wstrWrongInput.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}

	m_strSearch.assign(reinterpret_cast<char*>(&dwData), sizeof(DWORD));
	m_strReplace.assign(reinterpret_cast<char*>(&dwDataRet), sizeof(DWORD));
	m_nSizeSearch = m_strSearch.size();
	m_nSizeReplace = m_strReplace.size();
	m_pSearchData = reinterpret_cast<std::byte*>(m_strSearch.data());
	m_pReplaceData = reinterpret_cast<std::byte*>(m_strReplace.data());

	return true;
}

bool CHexDlgSearch::PrepareQWORD()
{
	QWORD qwData { };
	QWORD qwDataRet { };
	if (!wstr2num(m_wstrTextSearch, qwData) || (m_fReplace && !wstr2num(m_wstrTextReplace, qwDataRet)))
	{
		MessageBoxW(m_wstrWrongInput.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}

	m_strSearch.assign(reinterpret_cast<char*>(&qwData), sizeof(QWORD));
	m_strReplace.assign(reinterpret_cast<char*>(&qwDataRet), sizeof(QWORD));
	m_nSizeSearch = m_strSearch.size();
	m_nSizeReplace = m_strReplace.size();
	m_pSearchData = reinterpret_cast<std::byte*>(m_strSearch.data());
	m_pReplaceData = reinterpret_cast<std::byte*>(m_strReplace.data());

	return true;
}

bool CHexDlgSearch::PrepareFloat()
{
	float flData { };
	float flDataRet { };
	if (!wstr2num(m_wstrTextSearch, flData) || (m_fReplace && !wstr2num(m_wstrTextReplace, flDataRet)))
	{
		MessageBoxW(m_wstrWrongInput.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}

	m_strSearch.assign(reinterpret_cast<char*>(&flData), sizeof(float));
	m_strReplace.assign(reinterpret_cast<char*>(&flDataRet), sizeof(float));
	m_nSizeSearch = m_strSearch.size();
	m_nSizeReplace = m_strReplace.size();
	m_pSearchData = reinterpret_cast<std::byte*>(m_strSearch.data());
	m_pReplaceData = reinterpret_cast<std::byte*>(m_strReplace.data());

	return true;
}

bool CHexDlgSearch::PrepareDouble()
{
	double ddData { };
	double ddDataRet { };
	if (!wstr2num(m_wstrTextSearch, ddData) || (m_fReplace && !wstr2num(m_wstrTextReplace, ddDataRet)))
	{
		MessageBoxW(m_wstrWrongInput.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}

	m_strSearch.assign(reinterpret_cast<char*>(&ddData), sizeof(double));
	m_strReplace.assign(reinterpret_cast<char*>(&ddDataRet), sizeof(double));
	m_nSizeSearch = m_strSearch.size();
	m_nSizeReplace = m_strReplace.size();
	m_pSearchData = reinterpret_cast<std::byte*>(m_strSearch.data());
	m_pReplaceData = reinterpret_cast<std::byte*>(m_strReplace.data());

	return true;
}

void CHexDlgSearch::Search()
{
	auto pHexCtrl = GetHexCtrl();
	if (m_wstrTextSearch.empty() || !pHexCtrl->IsDataSet() || m_ullOffsetCurr >= pHexCtrl->GetDataSize())
		return;

	m_fFound = false;
	auto ullUntil = m_ullOffsetEnd;
	auto lmbFindForward = [&]()
	{
		if (Find(m_ullOffsetCurr, ullUntil, m_pSearchData, m_nSizeSearch, m_ullEndSentinel))
		{
			m_fFound = true;
			m_fSecondMatch = true;
			m_dwCount++;
		}
		if (!m_fFound && m_fSecondMatch)
		{
			m_ullOffsetCurr = m_ullOffsetStart; //Starting from the beginning.
			if (Find(m_ullOffsetCurr, ullUntil, m_pSearchData, m_nSizeSearch, m_ullEndSentinel))
			{
				m_fFound = true;
				m_fSecondMatch = true;
				m_fDoCount = true;
				m_dwCount = 1;
			}
			m_iWrap = 1;
		}
	};
	auto lmbFindBackward = [&]()
	{
		ullUntil = m_ullOffsetStart;
		if (m_fSecondMatch && m_ullOffsetCurr - m_ullStep < m_ullOffsetCurr)
		{
			m_ullOffsetCurr -= m_ullStep;
			if (Find(m_ullOffsetCurr, ullUntil, m_pSearchData, m_nSizeSearch, m_ullEndSentinel, false))
			{
				m_fFound = true;
				m_dwCount--;
			}
		}

		if (!m_fFound)
		{
			m_ullOffsetCurr = m_ullOffsetEnd;
			if (Find(m_ullOffsetCurr, ullUntil, m_pSearchData, m_nSizeSearch, m_ullEndSentinel, false))
			{
				m_fFound = true;
				m_fSecondMatch = true;
				m_iWrap = -1;
				m_fDoCount = false;
				m_dwCount = 1;
			}
		}
	};

	if (m_fReplace) //Replace
	{
		if (m_fAll) //Replace All
		{
			auto ullStart = m_ullOffsetStart;
			auto lmbSearch = [&](CHexDlgCallback* pDlg)
			{
				while (true)
				{
					if (Find(ullStart, ullUntil, m_pSearchData, m_nSizeSearch, m_ullEndSentinel, true, false))
					{
						Replace(ullStart, m_pReplaceData, m_nSizeSearch, m_nSizeReplace, false, false);
						ullStart += m_nSizeReplace <= m_ullStep ? m_ullStep : m_nSizeReplace;
						m_fFound = true;
						m_dwReplaced++;
						if (ullStart > ullUntil)
							break;
					}
					else
						break;

					if (pDlg->IsCanceled())
						goto exit;
				}
			exit:
				pDlg->Cancel();
			};

			CHexDlgCallback dlg(L"Searching...");
			std::thread thrd(lmbSearch, &dlg);
			dlg.DoModal();
			thrd.join();
			if (m_fFound) //Notifying parent just once about data change/replace.
				pHexCtrl->ParentNotify(HEXCTRL_MSG_SETDATA);
		}
		else if (m_iDirection == 1 && m_fSecondMatch) //Forward only direction.
		{
			Replace(m_ullOffsetCurr, m_pReplaceData, m_nSizeSearch, m_nSizeReplace);

			//Increase next search step to replaced or m_ullStep amount.
			m_ullOffsetCurr += m_nSizeReplace <= m_ullStep ? m_ullStep : m_nSizeReplace;
			m_dwReplaced++;
		}

		if (!m_fAll)
			lmbFindForward();
	}
	else //Search.
	{
		if (m_fAll) //Find All
		{
			ClearList(); //Clearing all results.
			m_dwCount = 0;
			auto ullStart = m_ullOffsetStart;
			auto lmbSearch = [&](CHexDlgCallback* pDlg)
			{
				while (true)
				{
					if (Find(ullStart, ullUntil, m_pSearchData, m_nSizeSearch, m_ullEndSentinel, true, false))
					{
						m_vecSearchRes.emplace_back(ullStart); //Filling the vector of Found occurences.
						ullStart += m_ullStep;
						m_fFound = true;
						++m_dwCount;
						if (ullStart > ullUntil || m_dwCount >= m_dwMaxSearch) //Find no more than m_dwMaxSearch.
							break;
					}
					else
						break;

					if (pDlg->IsCanceled())
						goto exit;
				}
			exit:
				m_pListMain->SetItemCountEx(static_cast<int>(m_dwCount));
				pDlg->Cancel();
			};

			CHexDlgCallback dlg(L"Searching...");
			std::thread thrd(lmbSearch, &dlg);
			dlg.DoModal();
			thrd.join();
		}
		else
		{
			if (m_iDirection == 1) //Forward direction.
			{
				//If previously anything was found we increase m_ullOffsetCurr by m_ullStep.
				m_ullOffsetCurr = m_fSecondMatch ? m_ullOffsetCurr + m_ullStep : m_ullOffsetCurr;
				lmbFindForward();
			}
			else if (m_iDirection == -1) //Backward direction
			{
				lmbFindBackward();
			}
		}
	}

	std::wstring wstrInfo(128, 0);
	if (m_fFound)
	{
		if (m_fAll)
		{
			if (m_fReplace)
			{
				swprintf_s(wstrInfo.data(), wstrInfo.size(), L"%lu occurrence(s) replaced.", m_dwReplaced);
				m_dwReplaced = 0;
			}
			else
			{
				swprintf_s(wstrInfo.data(), wstrInfo.size(), L"Found %lu occurrences.", m_dwCount);
				m_dwCount = 0;
			}
			pHexCtrl->RedrawWindow();
		}
		else
		{
			if (m_fDoCount)
				swprintf_s(wstrInfo.data(), wstrInfo.size(), L"Found occurrence \u2116 %lu from the beginning.", m_dwCount);
			else
				wstrInfo = L"Search found occurrence.";

			HexCtrlHighlight({ { m_ullOffsetCurr, m_fReplace ? m_nSizeReplace : m_nSizeSearch } });
			AddToList(m_ullOffsetCurr);
			SetEditStartAt(m_ullOffsetCurr);
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

bool CHexDlgSearch::Find(ULONGLONG& ullStart, ULONGLONG ullEnd, std::byte* pSearch,
	size_t nSizeSearch, ULONGLONG ullEndSentinel, bool fForward, bool fThread)
{
	if (ullStart + nSizeSearch > ullEndSentinel)
		return false;

	ULONGLONG ullSize = fForward ? ullEnd - ullStart : ullStart - ullEnd; //Depends on search direction
	ULONGLONG ullSizeChunk { };    //Size of the chunk to work with.
	ULONGLONG ullChunks { };
	ULONGLONG ullMemToAcquire { }; //Size of VirtualData memory for acquiring. It's bigger than ullSizeChunk.
	ULONGLONG ullOffsetSearch { };
	constexpr auto sizeQuick { 1024 * 256 }; //256KB.
	auto pHexCtrl = GetHexCtrl();

	switch (pHexCtrl->GetDataMode())
	{
	case EHexDataMode::DATA_MEMORY:
		ullSizeChunk = ullSize;
		ullMemToAcquire = ullSize;
		ullChunks = 1;
		break;
	case EHexDataMode::DATA_MSG:
	case EHexDataMode::DATA_VIRTUAL:
	{
		ullMemToAcquire = pHexCtrl->GetCacheSize();
		if (ullMemToAcquire > ullSize + nSizeSearch)
			ullMemToAcquire = ullSize + nSizeSearch;
		ullSizeChunk = ullMemToAcquire - nSizeSearch;
		if (ullSize > ullSizeChunk)
			ullChunks = (ullSize % ullSizeChunk) ? ullSize / ullSizeChunk + 1 : ullSize / ullSizeChunk;
		else
			ullChunks = 1;
	}
	break;
	}

	bool fResult { false };
	auto lmbSearch = [&](CHexDlgCallback* pDlg = nullptr)
	{
		if (fForward)
		{
			ullOffsetSearch = ullStart;
			for (ULONGLONG iterChunk = 0; iterChunk < ullChunks; ++iterChunk)
			{
				if (ullOffsetSearch + ullMemToAcquire > ullEndSentinel)
				{
					ullMemToAcquire = ullEndSentinel - ullOffsetSearch;
					ullSizeChunk = ullMemToAcquire - nSizeSearch;
				}
				if (iterChunk > 0)
					ullOffsetSearch += ullSizeChunk;

				auto pData = pHexCtrl->GetData({ ullOffsetSearch, ullMemToAcquire });
				for (ULONGLONG i = 0; i <= ullSizeChunk; i += m_ullStep)
				{
					if (memcmp(pData + i, pSearch, nSizeSearch) == 0)
					{
						ullStart = ullOffsetSearch + i;
						fResult = true;
						goto exit;
					}
					if (pDlg != nullptr && pDlg->IsCanceled())
						goto exit;
				}
			}
		}
		else
		{
			ullOffsetSearch = ullStart - ullSizeChunk;
			for (ULONGLONG iterChunk = ullChunks; iterChunk > 0; --iterChunk)
			{
				auto pData = pHexCtrl->GetData({ ullOffsetSearch, ullMemToAcquire });
				for (auto i = static_cast<LONGLONG>(ullSizeChunk); i >= 0; i -= m_ullStep)	//i might be negative.
				{
					if (memcmp(pData + i, pSearch, nSizeSearch) == 0)
					{
						ullStart = ullOffsetSearch + i;
						fResult = true;
						goto exit;
					}
					if (pDlg != nullptr && pDlg->IsCanceled())
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
		if (pDlg != nullptr)
			pDlg->Cancel();
	};

	if (fThread && ullSize > sizeQuick) //Showing "Cancel" dialog only when data > sizeQuick
	{
		CHexDlgCallback dlg(L"Searching...");
		std::thread thrd(lmbSearch, &dlg);
		dlg.DoModal();
		thrd.join();
	}
	else
		lmbSearch();

	return fResult;
}

void CHexDlgSearch::Replace(ULONGLONG ullIndex, std::byte* pData, size_t nSizeData, size_t nSizeReplace,
	bool fRedraw, bool fParentNtfy)
{
	SMODIFY hms;
	hms.vecSpan.emplace_back(HEXSPANSTRUCT { ullIndex, nSizeData });
	hms.ullDataSize = nSizeReplace;
	hms.pData = pData;
	hms.fParentNtfy = fParentNtfy;
	hms.fRedraw = fRedraw;
	GetHexCtrl()->Modify(hms);
}

CHexCtrl* CHexDlgSearch::GetHexCtrl()const
{
	return m_pHexCtrl;
}

CHexDlgSearch::EMode CHexDlgSearch::GetSearchMode()
{
	return static_cast<EMode>(m_stComboMode.GetItemData(m_stComboMode.GetCurSel()));
}

void CHexDlgSearch::ComboSearchFill(LPCWSTR pwsz)
{
	//Insert wstring into ComboBox only if it's not already presented.
	if (m_stComboSearch.FindStringExact(0, pwsz) == CB_ERR)
	{
		//Keep max 50 strings in list.
		if (m_stComboSearch.GetCount() == 50)
			m_stComboSearch.DeleteString(49);
		m_stComboSearch.InsertString(0, pwsz);
	}
}

void CHexDlgSearch::ComboReplaceFill(LPCWSTR pwsz)
{
	//Insert wstring into ComboBox only if it's not already presented.
	if (m_stComboReplace.FindStringExact(0, pwsz) == CB_ERR)
	{
		//Keep max 50 strings in list.
		if (m_stComboReplace.GetCount() == 50)
			m_stComboReplace.DeleteString(49);
		m_stComboReplace.InsertString(0, pwsz);
	}
}

void CHexDlgSearch::SetEditStartAt(ULONGLONG ullOffset)
{
	wchar_t buff[32] { };
	swprintf_s(buff, _countof(buff), L"0x%llX", ullOffset);
	m_stEditStart.SetWindowTextW(buff);
}

void CHexDlgSearch::SetEditStep(ULONGLONG ullStep)
{
	wchar_t buff[32] { };
	swprintf_s(buff, _countof(buff), L"%llu", ullStep);
	m_stEditStep.SetWindowTextW(buff);
}

void CHexDlgSearch::ResetSearch()
{
	m_ullOffsetCurr = { };
	m_dwCount = { };
	m_dwReplaced = { };
	m_iWrap = { };
	m_fSecondMatch = { false };
	m_fFound = { false };
	m_fDoCount = { true };
	m_pListMain->SetItemCountEx(0);
	m_vecSearchRes.clear();
	GetHexCtrl()->ClearSelHighlight();
	GetDlgItem(IDC_HEXCTRL_SEARCH_STATIC_TEXTBOTTOM)->SetWindowTextW(L"");
}

void CHexDlgSearch::OnDestroy()
{
	CDialogEx::OnDestroy();

	m_pListMain->DestroyWindow();
	m_stBrushDefault.DeleteObject();
	m_stMenuList.DestroyMenu();
}