/****************************************************************************************
* Copyright © 2018-2024 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "../../dep/StrToNum/StrToNum/StrToNum.h"
#include "../../res/HexCtrlRes.h"
#include "CHexDlgCallback.h"
#include "CHexDlgSearch.h"
#include <algorithm>
#include <cassert>
#include <cwctype>
#include <format>
#include <limits>
#include <string>
#include <thread>
#include <type_traits>

using namespace HEXCTRL::INTERNAL;

enum class CHexDlgSearch::ESearchMode : std::uint8_t {
	MODE_HEXBYTES, MODE_TEXT, MODE_NUMBERS, MODE_STRUCT
};

enum class CHexDlgSearch::ESearchType : std::uint8_t {
	HEXBYTES, TEXT_ASCII, TEXT_UTF8, TEXT_UTF16,
	NUM_INT8, NUM_UINT8, NUM_INT16, NUM_UINT16,
	NUM_INT32, NUM_UINT32, NUM_INT64, NUM_UINT64,
	NUM_FLOAT, NUM_DOUBLE, STRUCT_FILETIME
};

enum class CHexDlgSearch::EMenuID : std::uint16_t {
	IDM_SEARCH_ADDBKM = 0x8000, IDM_SEARCH_SELECTALL = 0x8001, IDM_SEARCH_CLEARALL = 0x8002
};

struct CHexDlgSearch::FINDRESULT {
	ULONGLONG ullOffset;
	bool      fFound { };
	bool      fCanceled { }; //Search was canceled by pressing "Cancel".
	operator bool()const { return fFound; };
};

struct CHexDlgSearch::SEARCHFUNCDATA {
	ULONGLONG ullStart { };
	ULONGLONG ullEnd { };
	ULONGLONG ullLoopChunkSize { };  //Actual data size for a `for()` loop.
	ULONGLONG ullMemToAcquire { };   //Size of a memory to acquire, to search in.
	ULONGLONG ullMemChunks { };      //How many such memory chunks, to search in.
	ULONGLONG ullOffsetSentinel { }; //The maximum offset that search can't cross.
	LONGLONG llStep { };             //Search step, must be signed here.
	IHexCtrl* pHexCtrl { };
	CHexDlgCallback* pDlgClbk { };
	SpanCByte spnFind { };
	bool fBigStep { };
	bool fInverted { };
	bool fCanceled { };
	bool fCancelDlgOnEnd { }; //"OnCancel" the dialog at the end of the search.
	bool fResult { };
};

BEGIN_MESSAGE_MAP(CHexDlgSearch, CDialogEx)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_BTN_SEARCH_F, &CHexDlgSearch::OnButtonSearchF)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_BTN_SEARCH_B, &CHexDlgSearch::OnButtonSearchB)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_BTN_FINDALL, &CHexDlgSearch::OnButtonFindAll)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_BTN_REPLACE, &CHexDlgSearch::OnButtonReplace)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_BTN_REPLACE_ALL, &CHexDlgSearch::OnButtonReplaceAll)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_CHECK_SEL, &CHexDlgSearch::OnCheckSel)
	ON_CBN_SELCHANGE(IDC_HEXCTRL_SEARCH_COMBO_MODE, &CHexDlgSearch::OnComboModeSelChange)
	ON_CBN_SELCHANGE(IDC_HEXCTRL_SEARCH_COMBO_TYPE, &CHexDlgSearch::OnComboTypeSelChange)
	ON_CBN_EDITUPDATE(IDC_HEXCTRL_SEARCH_COMBO_SEARCH, &CHexDlgSearch::UpdateSearchReplaceControls)
	ON_CBN_EDITUPDATE(IDC_HEXCTRL_SEARCH_COMBO_REPLACE, &CHexDlgSearch::UpdateSearchReplaceControls)
	ON_NOTIFY(LVN_GETDISPINFOW, IDC_HEXCTRL_SEARCH_LIST_MAIN, &CHexDlgSearch::OnListGetDispInfo)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_HEXCTRL_SEARCH_LIST_MAIN, &CHexDlgSearch::OnListItemChanged)
	ON_NOTIFY(NM_RCLICK, IDC_HEXCTRL_SEARCH_LIST_MAIN, &CHexDlgSearch::OnListRClick)
	ON_WM_ACTIVATE()
	ON_WM_CLOSE()
	ON_WM_CTLCOLOR()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

auto CHexDlgSearch::GetDlgData()const->std::uint64_t
{
	if (!IsWindow(m_hWnd)) {
		return { };
	}

	std::uint64_t ullData { };

	if (IsNoEsc()) {
		ullData |= HEXCTRL_FLAG_NOESC;
	}

	return ullData;
}

void CHexDlgSearch::Initialize(IHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	if (pHexCtrl == nullptr)
		return;

	m_pHexCtrl = pHexCtrl;
}

bool CHexDlgSearch::IsSearchAvail()const
{
	const auto* const pHexCtrl = GetHexCtrl();
	return pHexCtrl->IsDataSet() && !m_wstrTextSearch.empty() && m_ullOffsetCurr < pHexCtrl->GetDataSize();
}

void CHexDlgSearch::SearchNextPrev(bool fForward)
{
	m_fForward = fForward;
	m_fReplace = false;
	m_fAll = false;
	m_fSearchNext = true;
	Search();
}

auto CHexDlgSearch::SetDlgData(std::uint64_t ullData, bool fCreate)->HWND
{
	m_u64DlgData = ullData;

	if (!IsWindow(m_hWnd)) {
		if (fCreate) {
			Create(IDD_HEXCTRL_SEARCH, CWnd::FromHandle(m_pHexCtrl->GetWndHandle(EHexWnd::WND_MAIN)));
		}
	}
	else {
		ApplyDlgData();
	}

	return m_hWnd;
}

BOOL CHexDlgSearch::ShowWindow(int nCmdShow)
{
	if (!IsWindow(m_hWnd)) {
		Create(IDD_HEXCTRL_SEARCH, CWnd::FromHandle(m_pHexCtrl->GetWndHandle(EHexWnd::WND_MAIN)));
	}

	return CDialogEx::ShowWindow(nCmdShow);
}


//Private methods.

void CHexDlgSearch::AddToList(ULONGLONG ullOffset)
{
	int iHighlight { -1 };
	if (const auto iter = std::find(m_vecSearchRes.begin(), m_vecSearchRes.end(), ullOffset);
		iter == m_vecSearchRes.end()) { //Max-found search occurences.
		if (m_vecSearchRes.size() < static_cast<std::size_t>(m_dwFoundLimit)) {
			m_vecSearchRes.emplace_back(ullOffset);
			iHighlight = static_cast<int>(m_vecSearchRes.size());
			m_pListMain->SetItemCountEx(iHighlight--);
		}
	}
	else {
		iHighlight = static_cast<int>(iter - m_vecSearchRes.begin());
	}

	if (iHighlight != -1) {
		m_pListMain->SetItemState(-1, 0, LVIS_SELECTED);
		m_pListMain->SetItemState(iHighlight, LVIS_SELECTED, LVIS_SELECTED);
		m_pListMain->EnsureVisible(iHighlight, TRUE);
	}
}

void CHexDlgSearch::ApplyDlgData()
{
}

void CHexDlgSearch::ClearComboType()
{
	m_comboType.SetRedraw(FALSE);
	for (auto iIndex = m_comboType.GetCount() - 1; iIndex >= 0; --iIndex) {
		m_comboType.DeleteString(iIndex);
	}
	m_comboType.SetRedraw(TRUE);
}

void CHexDlgSearch::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_CHECK_SEL, m_btnSel);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_CHECK_WILDCARD, m_btnWC);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_CHECK_INV, m_btnInv);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_CHECK_BE, m_btnBE);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_CHECK_MATCHCASE, m_btnMC);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_COMBO_SEARCH, m_comboSearch);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_COMBO_REPLACE, m_comboReplace);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_COMBO_MODE, m_comboMode);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_COMBO_TYPE, m_comboType);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_EDIT_START, m_editStart);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_EDIT_END, m_editEnd);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_EDIT_STEP, m_editStep);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_EDIT_LIMIT, m_editLimit);
}

void CHexDlgSearch::ClearList()
{
	m_pListMain->SetItemCountEx(0);
	m_vecSearchRes.clear();
}

void CHexDlgSearch::ComboSearchFill(LPCWSTR pwsz)
{
	//Insert wstring into ComboBox only if it's not already presented.
	if (m_comboSearch.FindStringExact(0, pwsz) == CB_ERR) {
		//Keep max 50 strings in list.
		if (m_comboSearch.GetCount() == 50) {
			m_comboSearch.DeleteString(49);
		}
		m_comboSearch.InsertString(0, pwsz);
	}
}

void CHexDlgSearch::ComboReplaceFill(LPCWSTR pwsz)
{
	//Insert wstring into ComboBox only if it's not already presented.
	if (m_comboReplace.FindStringExact(0, pwsz) == CB_ERR) {
		//Keep max 50 strings in list.
		if (m_comboReplace.GetCount() == 50) {
			m_comboReplace.DeleteString(49);
		}
		m_comboReplace.InsertString(0, pwsz);
	}
}

auto CHexDlgSearch::GetHexCtrl()const->IHexCtrl*
{
	return m_pHexCtrl;
}

template<bool fDlgClbck>
auto CHexDlgSearch::GetSearchFuncFwd()const->PtrSearchFunc
{
	//The `fDlgClbck` arg ensures that no runtime check will be performed for the 
	//'SEARCHFUNCDATA::pDlgClbk == nullptr', at the hot path inside the SearchFunc function.
	using enum ESearchType; using enum EMemCmp;
	switch (GetSearchType()) {
	case HEXBYTES:
		if (m_vecSearchData.size() == 1 && m_ullStep == 1 && !IsWildcard()) { //Special case for 1byte data size.
			return IsInverted() ?
				SearchFuncFwd<SEARCHTYPE(DATA_BYTE1, fDlgClbck, false, false, true, true)> :
				SearchFuncFwd<SEARCHTYPE(DATA_BYTE1, fDlgClbck, false, false, true, false)>;
		}
		else {
			return IsWildcard() ?
				SearchFuncFwd<SEARCHTYPE(CHAR_STR, fDlgClbck, true, true)> :
				SearchFuncFwd<SEARCHTYPE(CHAR_STR, fDlgClbck, true, false)>;
		}
	case TEXT_ASCII:
		if (m_vecSearchData.size() == 1 && m_ullStep == 1 && !IsWildcard()) { //Special case for 1byte data size.
			return IsInverted() ?
				SearchFuncFwd<SEARCHTYPE(DATA_BYTE1, fDlgClbck, false, false, true, true)> :
				SearchFuncFwd<SEARCHTYPE(DATA_BYTE1, fDlgClbck, false, false, true, false)>;
		}
		else {
			if (IsMatchCase() && !IsWildcard()) {
				return SearchFuncFwd<SEARCHTYPE(CHAR_STR, fDlgClbck, true, false)>;
			}

			if (IsMatchCase() && IsWildcard()) {
				return SearchFuncFwd<SEARCHTYPE(CHAR_STR, fDlgClbck, true, true)>;
			}

			if (!IsMatchCase() && IsWildcard()) {
				return SearchFuncFwd<SEARCHTYPE(CHAR_STR, fDlgClbck, false, true)>;
			}

			if (!IsMatchCase() && !IsWildcard()) {
				return SearchFuncFwd<SEARCHTYPE(CHAR_STR, fDlgClbck, false, false)>;
			}
		}
		break;
	case TEXT_UTF8:
		return SearchFuncFwd<SEARCHTYPE(CHAR_STR, fDlgClbck, true, false)>; //Search UTF-8 as plain chars.
	case TEXT_UTF16:
		if (IsMatchCase() && !IsWildcard()) {
			return SearchFuncFwd<SEARCHTYPE(WCHAR_STR, fDlgClbck, true, false)>;
		}

		if (IsMatchCase() && IsWildcard()) {
			return SearchFuncFwd<SEARCHTYPE(WCHAR_STR, fDlgClbck, true, true)>;
		}

		if (!IsMatchCase() && IsWildcard()) {
			return SearchFuncFwd<SEARCHTYPE(WCHAR_STR, fDlgClbck, false, true)>;
		}

		if (!IsMatchCase() && !IsWildcard()) {
			return SearchFuncFwd<SEARCHTYPE(WCHAR_STR, fDlgClbck, false, false)>;
		}
		break;
	case NUM_INT8:
	case NUM_UINT8:
		if (m_ullStep == 1) {
			return IsInverted() ?
				SearchFuncFwd<SEARCHTYPE(DATA_BYTE1, fDlgClbck, false, false, true, true)> :
				SearchFuncFwd<SEARCHTYPE(DATA_BYTE1, fDlgClbck, false, false, true, false)>;
		}
		else {
			return SearchFuncFwd<SEARCHTYPE(DATA_BYTE1, fDlgClbck)>;
		}
		break;
	case NUM_INT16:
	case NUM_UINT16:
		return SearchFuncFwd<SEARCHTYPE(DATA_BYTE2, fDlgClbck)>;
	case NUM_INT32:
	case NUM_UINT32:
	case NUM_FLOAT:
		return SearchFuncFwd<SEARCHTYPE(DATA_BYTE4, fDlgClbck)>;
	case NUM_INT64:
	case NUM_UINT64:
	case NUM_DOUBLE:
	case STRUCT_FILETIME:
		return SearchFuncFwd<SEARCHTYPE(DATA_BYTE8, fDlgClbck)>;
	default:
		break;
	}

	return { };
}

template<bool fDlgClbck>
auto CHexDlgSearch::GetSearchFuncBack()const->PtrSearchFunc
{
	//The `fDlgClbck` arg ensures that no runtime check will be performed for the 
	//'SEARCHFUNCDATA::pDlgClbk == nullptr', at the hot path inside the SearchFunc function.
	using enum ESearchType; using enum EMemCmp;
	switch (GetSearchType()) {
	case HEXBYTES:
		if (m_vecSearchData.size() == 1 && m_ullStep == 1 && !IsWildcard()) { //Special case for 1byte data size.
			return IsInverted() ?
				SearchFuncBack<SEARCHTYPE(DATA_BYTE1, fDlgClbck, false, false, true, true)> :
				SearchFuncBack<SEARCHTYPE(DATA_BYTE1, fDlgClbck, false, false, true, false)>;
		}
		else {
			return IsWildcard() ?
				SearchFuncBack<SEARCHTYPE(CHAR_STR, fDlgClbck, true, true)> :
				SearchFuncBack<SEARCHTYPE(CHAR_STR, fDlgClbck, true, false)>;
		}
	case TEXT_ASCII:
		if (m_vecSearchData.size() == 1 && m_ullStep == 1 && !IsWildcard()) { //Special case for 1byte data size.
			return IsInverted() ?
				SearchFuncBack<SEARCHTYPE(DATA_BYTE1, fDlgClbck, false, false, true, true)> :
				SearchFuncBack<SEARCHTYPE(DATA_BYTE1, fDlgClbck, false, false, true, false)>;
		}
		else {
			if (IsMatchCase() && !IsWildcard()) {
				return SearchFuncBack<SEARCHTYPE(CHAR_STR, fDlgClbck, true, false)>;
			}

			if (IsMatchCase() && IsWildcard()) {
				return SearchFuncBack<SEARCHTYPE(CHAR_STR, fDlgClbck, true, true)>;
			}

			if (!IsMatchCase() && IsWildcard()) {
				return SearchFuncBack<SEARCHTYPE(CHAR_STR, fDlgClbck, false, true)>;
			}

			if (!IsMatchCase() && !IsWildcard()) {
				return SearchFuncBack<SEARCHTYPE(CHAR_STR, fDlgClbck, false, false)>;
			}
		}
		break;
	case TEXT_UTF8:
		return SearchFuncBack<SEARCHTYPE(CHAR_STR, fDlgClbck, true, false)>; //Search UTF-8 as plain chars.
	case TEXT_UTF16:
		if (IsMatchCase() && !IsWildcard()) {
			return SearchFuncBack<SEARCHTYPE(WCHAR_STR, fDlgClbck, true, false)>;
		}

		if (IsMatchCase() && IsWildcard()) {
			return SearchFuncBack<SEARCHTYPE(WCHAR_STR, fDlgClbck, true, true)>;
		}

		if (!IsMatchCase() && IsWildcard()) {
			return SearchFuncBack<SEARCHTYPE(WCHAR_STR, fDlgClbck, false, true)>;
		}

		if (!IsMatchCase() && !IsWildcard()) {
			return SearchFuncBack<SEARCHTYPE(WCHAR_STR, fDlgClbck, false, false)>;
		}
		break;
	case NUM_INT8:
	case NUM_UINT8:
		if (m_ullStep == 1) {
			return IsInverted() ?
				SearchFuncBack<SEARCHTYPE(DATA_BYTE1, fDlgClbck, false, false, true, true)> :
				SearchFuncBack<SEARCHTYPE(DATA_BYTE1, fDlgClbck, false, false, true, false)>;
		}
		else {
			return SearchFuncBack<SEARCHTYPE(DATA_BYTE1, fDlgClbck)>;
		}
		break;
	case NUM_INT16:
	case NUM_UINT16:
		return SearchFuncBack<SEARCHTYPE(DATA_BYTE2, fDlgClbck)>;
	case NUM_INT32:
	case NUM_UINT32:
	case NUM_FLOAT:
		return SearchFuncBack<SEARCHTYPE(DATA_BYTE4, fDlgClbck)>;
	case NUM_INT64:
	case NUM_UINT64:
	case NUM_DOUBLE:
	case STRUCT_FILETIME:
		return SearchFuncBack<SEARCHTYPE(DATA_BYTE8, fDlgClbck)>;
	default:
		break;
	}

	return { };
}

auto CHexDlgSearch::GetSearchFunc(bool fFwd, bool fDlgClbck)const->PtrSearchFunc
{
	return fFwd ? (fDlgClbck ? GetSearchFuncFwd<true>() : GetSearchFuncFwd<false>()) :
		(fDlgClbck ? GetSearchFuncBack<true>() : GetSearchFuncBack<false>());
}

auto CHexDlgSearch::GetSearchMode()const->CHexDlgSearch::ESearchMode
{
	return static_cast<ESearchMode>(m_comboMode.GetItemData(m_comboMode.GetCurSel()));
}

auto CHexDlgSearch::GetSearchType()const->ESearchType
{
	if (GetSearchMode() == ESearchMode::MODE_HEXBYTES) {
		return ESearchType::HEXBYTES;
	}

	return static_cast<ESearchType>(m_comboType.GetItemData(m_comboType.GetCurSel()));
}

void CHexDlgSearch::HexCtrlHighlight(const VecSpan& vecSel)
{
	const auto pHexCtrl = GetHexCtrl();
	pHexCtrl->SetSelection(vecSel, true, IsSelection()); //Highlight selection?

	if (!pHexCtrl->IsOffsetVisible(vecSel.back().ullOffset)) {
		pHexCtrl->GoToOffset(vecSel.back().ullOffset);
	}
}

bool CHexDlgSearch::IsBigEndian()const
{
	return m_btnBE.GetCheck() == BST_CHECKED;
}

bool CHexDlgSearch::IsForward()const
{
	return m_fForward;
}

bool CHexDlgSearch::IsInverted()const
{
	return m_btnInv.GetCheck() == BST_CHECKED;
}

bool CHexDlgSearch::IsMatchCase()const
{
	return m_btnMC.GetCheck() == BST_CHECKED;
}

bool CHexDlgSearch::IsNoEsc()const
{
	return m_u64DlgData & HEXCTRL_FLAG_NOESC;
}

bool CHexDlgSearch::IsSelection()const
{
	return m_btnSel.GetCheck() == BST_CHECKED;
}

bool CHexDlgSearch::IsWildcard()const
{
	return m_btnWC.GetCheck() == BST_CHECKED;
}

void CHexDlgSearch::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if (!m_pHexCtrl->IsCreated())
		return;

	if (nState == WA_ACTIVE || nState == WA_CLICKACTIVE) {
		m_comboSearch.SetFocus();
		UpdateSearchReplaceControls();
	}

	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
}

void CHexDlgSearch::OnButtonSearchF()
{
	m_fForward = true;
	m_fReplace = false;
	m_fAll = false;
	Prepare();
}

void CHexDlgSearch::OnButtonSearchB()
{
	m_fForward = false;
	m_fReplace = false;
	m_fAll = false;
	Prepare();
}

void CHexDlgSearch::OnButtonFindAll()
{
	m_fForward = true;
	m_fReplace = false;
	m_fAll = true;
	Prepare();
}

void CHexDlgSearch::OnButtonReplace()
{
	m_fForward = true;
	m_fReplace = true;
	m_fAll = false;
	Prepare();
}

void CHexDlgSearch::OnButtonReplaceAll()
{
	m_fForward = true;
	m_fReplace = true;
	m_fAll = true;
	Prepare();
}

void CHexDlgSearch::OnCancel()
{
	if (IsNoEsc()) //Not closing Dialog on Escape key.
		return;

	CDialogEx::OnCancel();
}

void CHexDlgSearch::OnCheckSel()
{
	m_editStart.EnableWindow(!IsSelection());
	m_editEnd.EnableWindow(!IsSelection());
}

void CHexDlgSearch::OnClose()
{
	EndDialog(IDCANCEL);
}

void CHexDlgSearch::OnComboModeSelChange()
{
	const auto eMode = GetSearchMode();
	if (eMode != m_eSearchMode) {
		ResetSearch();
		m_eSearchMode = eMode;
	}

	using enum ESearchMode;
	switch (eMode) {
	case MODE_HEXBYTES:
		OnSelectModeHEXBYTES();
		break;
	case MODE_TEXT:
		OnSearchModeTEXT();
		break;
	case MODE_NUMBERS:
		OnSelectModeNUMBERS();
		break;
	case MODE_STRUCT:
		OnSelectModeSTRUCT();
		break;
	default:
		break;
	}

	OnComboTypeSelChange();
}

void CHexDlgSearch::OnComboTypeSelChange()
{
	using enum ESearchType;
	const auto eType = GetSearchType();
	switch (eType) {
	case STRUCT_FILETIME:
	{
		m_btnBE.EnableWindow(TRUE);
		const auto [dwFormat, wchSepar] = GetHexCtrl()->GetDateInfo();
		const auto wstr = GetDateFormatString(dwFormat, wchSepar);
		m_comboSearch.SetCueBanner(wstr.data());
		m_comboReplace.SetCueBanner(wstr.data());
	}
	break;
	case TEXT_ASCII:
	case TEXT_UTF16:
		m_btnMC.EnableWindow(TRUE);
		m_btnWC.EnableWindow(TRUE);
		break;
	case TEXT_UTF8:
		m_btnMC.EnableWindow(FALSE);
		m_btnWC.EnableWindow(FALSE);
		break;
	default:
		m_comboSearch.SetCueBanner(L"");
		m_comboReplace.SetCueBanner(L"");
		break;
	}
}

BOOL CHexDlgSearch::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (static_cast<EMenuID>(LOWORD(wParam))) {
	case EMenuID::IDM_SEARCH_ADDBKM:
	{
		int nItem { -1 };
		for (auto i = 0UL; i < m_pListMain->GetSelectedCount(); ++i) {
			nItem = m_pListMain->GetNextItem(nItem, LVNI_SELECTED);
			const HEXBKM hbs { .vecSpan { HEXSPAN { m_vecSearchRes.at(static_cast<std::size_t>(nItem)),
				m_fReplace ? m_vecReplaceData.size() : m_vecSearchData.size() } }, .wstrDesc { m_wstrTextSearch },
				.stClr { GetHexCtrl()->GetColors().clrBkBkm, GetHexCtrl()->GetColors().clrFontBkm } };
			GetHexCtrl()->GetBookmarks()->AddBkm(hbs, false);
		}
		GetHexCtrl()->Redraw();
	}
	return TRUE;
	case EMenuID::IDM_SEARCH_SELECTALL:
		m_pListMain->SetItemState(-1, LVIS_SELECTED, LVIS_SELECTED);
		return TRUE;
	case EMenuID::IDM_SEARCH_CLEARALL:
		ClearList();
		m_fSecondMatch = false; //To be able to search from the zero offset.
		return TRUE;
	default:
		break;
	}

	return CDialogEx::OnCommand(wParam, lParam);
}

auto CHexDlgSearch::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)->HBRUSH
{
	const auto hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	if (pWnd->GetDlgCtrlID() == IDC_HEXCTRL_SEARCH_STATIC_RESULT) {
		pDC->SetTextColor(m_fFound ? RGB(0, 200, 0) : RGB(200, 0, 0));
	}

	return hbr;
}

void CHexDlgSearch::OnDestroy()
{
	CDialogEx::OnDestroy();

	m_menuList.DestroyMenu();
	m_vecSearchRes.clear();
	m_vecSearchData.clear();
	m_vecReplaceData.clear();
	m_wstrTextSearch.clear();
	m_wstrTextReplace.clear();
	m_u64DlgData = { };
}

BOOL CHexDlgSearch::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	static constexpr auto iTextLimit { 512 };

	m_comboSearch.LimitText(iTextLimit);
	m_comboReplace.LimitText(iTextLimit);

	auto iIndex = m_comboMode.AddString(L"Hex Bytes");
	m_comboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchMode::MODE_HEXBYTES));
	m_comboMode.SetCurSel(iIndex);
	iIndex = m_comboMode.AddString(L"Text");
	m_comboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchMode::MODE_TEXT));
	iIndex = m_comboMode.AddString(L"Numbers");
	m_comboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchMode::MODE_NUMBERS));
	iIndex = m_comboMode.AddString(L"Structs");
	m_comboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchMode::MODE_STRUCT));

	m_pListMain->CreateDialogCtrl(IDC_HEXCTRL_SEARCH_LIST_MAIN, this);
	m_pListMain->SetExtendedStyle(LVS_EX_HEADERDRAGDROP);
	m_pListMain->InsertColumn(0, L"\u2116", LVCFMT_LEFT, 40);
	m_pListMain->InsertColumn(1, L"Offset", LVCFMT_LEFT, 455);

	m_menuList.CreatePopupMenu();
	m_menuList.AppendMenuW(MF_STRING, static_cast<UINT_PTR>(EMenuID::IDM_SEARCH_ADDBKM), L"Add bookmark(s)");
	m_menuList.AppendMenuW(MF_STRING, static_cast<UINT_PTR>(EMenuID::IDM_SEARCH_SELECTALL), L"Select All");
	m_menuList.AppendMenuW(MF_SEPARATOR);
	m_menuList.AppendMenuW(MF_STRING, static_cast<UINT_PTR>(EMenuID::IDM_SEARCH_CLEARALL), L"Clear All");

	m_editStep.SetWindowTextW(std::format(L"{}", m_ullStep).data()); //"Step" edit box text.
	m_editLimit.SetWindowTextW(std::format(L"{}", m_dwFoundLimit).data()); //"Limit search hits" edit box text.
	m_editStart.SetCueBanner(L"start offset");
	m_editEnd.SetCueBanner(L"end offset");

	const auto hwndTipWC = CreateWindowExW(0, TOOLTIPS_CLASS, nullptr, WS_POPUP | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, nullptr, nullptr, nullptr);
	const auto hwndTipInv = CreateWindowExW(0, TOOLTIPS_CLASS, nullptr, WS_POPUP | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, nullptr, nullptr, nullptr);
	const auto hwndTipEndOffset = CreateWindowExW(0, TOOLTIPS_CLASS, nullptr, WS_POPUP | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, nullptr, nullptr, nullptr);
	if (hwndTipWC == nullptr || hwndTipInv == nullptr || hwndTipEndOffset == nullptr)
		return FALSE;

	TTTOOLINFOW stToolInfo { .cbSize { sizeof(TTTOOLINFOW) }, .uFlags { TTF_IDISHWND | TTF_SUBCLASS }, .hwnd { m_hWnd },
		.uId { reinterpret_cast<UINT_PTR>(m_btnWC.m_hWnd) } }; //"Wildcard" check box tooltip.

	std::wstring wstrToolText;
	wstrToolText += L"Use ";
	wstrToolText += static_cast<wchar_t>(m_uWildcard);
	wstrToolText += L" character to match any symbol, or any byte if in \"Hex Bytes\" search mode.\r\n";
	wstrToolText += L"Example:\r\n";
	wstrToolText += L"  Hex Bytes: 11?11 will match: 112211, 113311, 114411, 119711, etc...\r\n";
	wstrToolText += L"  ASCII Text: sa??le will match: sample, saAAle, saxale, saZble, etc...\r\n";
	stToolInfo.lpszText = wstrToolText.data();
	::SendMessageW(hwndTipWC, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&stToolInfo));
	::SendMessageW(hwndTipWC, TTM_SETDELAYTIME, TTDT_AUTOPOP, static_cast<LPARAM>(LOWORD(0x7FFF)));
	::SendMessageW(hwndTipWC, TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(1000));

	stToolInfo.uId = reinterpret_cast<UINT_PTR>(m_btnInv.m_hWnd); //"Inverted" check box tooltip.
	wstrToolText = L"Search for the non-matching occurences.\r\nThat is everything that doesn't match search conditions.";
	stToolInfo.lpszText = wstrToolText.data();
	::SendMessageW(hwndTipInv, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&stToolInfo));
	::SendMessageW(hwndTipInv, TTM_SETDELAYTIME, TTDT_AUTOPOP, static_cast<LPARAM>(LOWORD(0x7FFF)));
	::SendMessageW(hwndTipInv, TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(1000));

	stToolInfo.uId = reinterpret_cast<UINT_PTR>(m_editEnd.m_hWnd); //"End offset" edit box tooltip.
	wstrToolText = L"The last offset for the search.\r\nEmpty means the whole data.";
	stToolInfo.lpszText = wstrToolText.data();
	::SendMessageW(hwndTipEndOffset, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&stToolInfo));
	::SendMessageW(hwndTipEndOffset, TTM_SETDELAYTIME, TTDT_AUTOPOP, static_cast<LPARAM>(LOWORD(0x7FFF)));
	::SendMessageW(hwndTipEndOffset, TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(1000));

	m_locale = std::locale("en_US.UTF-8");

	ApplyDlgData();

	return TRUE;
}

void CHexDlgSearch::OnListGetDispInfo(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto* const pDispInfo = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
	const auto* const pItem = &pDispInfo->item;

	if (pItem->mask & LVIF_TEXT) {
		const auto nItemID = static_cast<std::size_t>(pItem->iItem);
		switch (pItem->iSubItem) {
		case 0: //Index value.
			*std::format_to(pItem->pszText, L"{}", nItemID + 1) = L'\0';
			break;
		case 1: //Offset.
			*std::format_to(pItem->pszText, L"0x{:X}", m_vecSearchRes[nItemID]) = L'\0';
			break;
		default:
			break;
		}
	}
}

void CHexDlgSearch::OnListItemChanged(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	if (const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
		pNMI->iItem != -1 && pNMI->iSubItem != -1 && (pNMI->uNewState & LVIS_SELECTED)) {
		SetEditStartAt(m_vecSearchRes[static_cast<std::size_t>(pNMI->iItem)]);

		VecSpan vecSpan;
		int nItem = -1;
		for (auto i = 0UL; i < m_pListMain->GetSelectedCount(); ++i) {
			nItem = m_pListMain->GetNextItem(nItem, LVNI_SELECTED);

			//Do not yet add selected (clicked) item (in multiselect), will add it after the loop,
			//so that it's always last in vec, to highlight it in HexCtrlHighlight.
			if (pNMI->iItem != nItem) {
				vecSpan.emplace_back(m_vecSearchRes.at(static_cast<std::size_t>(nItem)),
					m_fReplace ? m_vecReplaceData.size() : m_vecSearchData.size());
			}
		}
		vecSpan.emplace_back(m_vecSearchRes.at(static_cast<std::size_t>(pNMI->iItem)),
			m_fReplace ? m_vecReplaceData.size() : m_vecSearchData.size());

		HexCtrlHighlight(vecSpan);
	}
}

void CHexDlgSearch::OnListRClick(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	const auto fEnabled { m_pListMain->GetItemCount() > 0 };
	m_menuList.EnableMenuItem(static_cast<UINT>(EMenuID::IDM_SEARCH_ADDBKM), (fEnabled ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
	m_menuList.EnableMenuItem(static_cast<UINT>(EMenuID::IDM_SEARCH_SELECTALL), (fEnabled ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
	m_menuList.EnableMenuItem(static_cast<UINT>(EMenuID::IDM_SEARCH_CLEARALL), (fEnabled ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);

	POINT pt;
	GetCursorPos(&pt);
	m_menuList.TrackPopupMenuEx(TPM_LEFTALIGN, pt.x, pt.y, this, nullptr);
}

void CHexDlgSearch::OnOK()
{
	OnButtonSearchF();
}

void CHexDlgSearch::OnSelectModeHEXBYTES()
{
	ClearComboType();
	m_comboType.EnableWindow(FALSE);
	m_btnBE.EnableWindow(FALSE);
	m_btnMC.EnableWindow(TRUE);
	m_btnWC.EnableWindow(TRUE);
}

void CHexDlgSearch::OnSelectModeNUMBERS()
{
	ClearComboType();

	m_comboType.EnableWindow(TRUE);
	auto iIndex = m_comboType.AddString(L"Int8");
	m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchType::NUM_INT8));
	m_comboType.SetCurSel(iIndex);
	iIndex = m_comboType.AddString(L"Unsigned Int8");
	m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchType::NUM_UINT8));
	iIndex = m_comboType.AddString(L"Int16");
	m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchType::NUM_INT16));
	iIndex = m_comboType.AddString(L"Unsigned Int16");
	m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchType::NUM_UINT16));
	iIndex = m_comboType.AddString(L"Int32");
	m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchType::NUM_INT32));
	iIndex = m_comboType.AddString(L"Unsigned Int32");
	m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchType::NUM_UINT32));
	iIndex = m_comboType.AddString(L"Int64");
	m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchType::NUM_INT64));
	iIndex = m_comboType.AddString(L"Unsigned Int64");
	m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchType::NUM_UINT64));
	iIndex = m_comboType.AddString(L"Float");
	m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchType::NUM_FLOAT));
	iIndex = m_comboType.AddString(L"Double");
	m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchType::NUM_DOUBLE));

	m_btnBE.EnableWindow(TRUE);
	m_btnMC.EnableWindow(FALSE);
	m_btnWC.EnableWindow(FALSE);
}

void CHexDlgSearch::OnSelectModeSTRUCT()
{
	ClearComboType();

	m_comboType.EnableWindow(TRUE);
	auto iIndex = m_comboType.AddString(L"FILETIME");
	m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchType::STRUCT_FILETIME));
	m_comboType.SetCurSel(iIndex);

	m_btnMC.EnableWindow(FALSE);
	m_btnWC.EnableWindow(FALSE);
}

void CHexDlgSearch::OnSearchModeTEXT()
{
	ClearComboType();

	m_comboType.EnableWindow(TRUE);
	auto iIndex = m_comboType.AddString(L"ASCII");
	m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchType::TEXT_ASCII));
	m_comboType.SetCurSel(iIndex);
	iIndex = m_comboType.AddString(L"UTF-8");
	m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchType::TEXT_UTF8));
	iIndex = m_comboType.AddString(L"UTF-16");
	m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchType::TEXT_UTF16));

	m_btnBE.EnableWindow(FALSE);
}

void CHexDlgSearch::Prepare()
{
	static constexpr auto uSearchSizeLimit { 256U };
	const auto pHexCtrl = GetHexCtrl();
	const auto ullDataSize = pHexCtrl->GetDataSize();

	//"Search" text.
	const auto iSearchSize = m_comboSearch.GetWindowTextLengthW();
	if (iSearchSize == 0 || iSearchSize > uSearchSizeLimit) {
		m_comboSearch.SetFocus();
		return;
	}

	CStringW wstrTextSearch;
	m_comboSearch.GetWindowTextW(wstrTextSearch);
	bool fNewSearchStr { false };
	if (wstrTextSearch.Compare(m_wstrTextSearch.data()) != 0) {
		ResetSearch();
		m_wstrTextSearch = wstrTextSearch;
		fNewSearchStr = true;
	}
	ComboSearchFill(wstrTextSearch);

	auto ullSearchDataSize { 0ULL }; //Actual search data size.
	//"Start/end" offsets.
	if (!IsSelection()) { //Do not use "Start/End offset" when in the selection.
		ULONGLONG ullStart;
		ULONGLONG ullEnd;
		if (m_editStart.GetWindowTextLengthW() == 0) {
			ullStart = 0;
		}
		else {
			CStringW wstrStartOffset;
			m_editStart.GetWindowTextW(wstrStartOffset);
			const auto optStart = stn::StrToUInt64(wstrStartOffset.GetString());
			if (!optStart) {
				m_editStart.SetFocus();
				MessageBoxW(L"Start offset is incorrect.", L"Incorrect offset", MB_ICONERROR);
				return;
			}

			ullStart = *optStart;
			if (ullStart >= ullDataSize) {
				m_editStart.SetFocus();
				MessageBoxW(L"Start offset is bigger than the data size.", L"Incorrect offset", MB_ICONERROR);
				return;
			}
		}

		//"End offset"
		if (m_editEnd.GetWindowTextLengthW() == 0) {
			ullEnd = ullDataSize - 1;
		}
		else {
			CStringW wstrEndOffset;
			m_editEnd.GetWindowTextW(wstrEndOffset);
			const auto optEnd = stn::StrToUInt64(wstrEndOffset.GetString());
			if (!optEnd) {
				m_editEnd.SetFocus();
				MessageBoxW(L"End offset is incorrect.", L"Incorrect offset", MB_ICONERROR);
				return;
			}

			ullEnd = *optEnd;
			if (ullEnd < ullStart) {
				m_editEnd.SetFocus();
				MessageBoxW(L"End offset can't be less than Start.", L"Incorrect offset", MB_ICONERROR);
				return;
			}
		}

		m_ullOffsetCurr = ullStart;
		ullSearchDataSize = ullEnd + 1;
	}
	else { ullSearchDataSize = ullDataSize; }


	//"Step".
	if (m_editStep.GetWindowTextLengthW() > 0) {
		CStringW wstrStep;
		m_editStep.GetWindowTextW(wstrStep);
		if (const auto optStep = stn::StrToUInt64(wstrStep.GetString()); optStep) {
			m_ullStep = *optStep;
		}
		else {
			m_editStep.SetFocus();
			MessageBoxW(L"Incorrect step size.", L"Incorrect step", MB_ICONERROR);
			return;
		}
	}
	else {
		m_editStep.SetFocus();
		MessageBoxW(L"Step size must be at least one.", L"Incorrect step", MB_ICONERROR);
		return;
	}

	//"Limit".
	if (m_editLimit.GetWindowTextLengthW() > 0) {
		CStringW wstrLimit;
		m_editLimit.GetWindowTextW(wstrLimit);
		if (const auto optLimit = stn::StrToUInt32(wstrLimit.GetString()); optLimit) {
			m_dwFoundLimit = *optLimit;
		}
		else {
			m_editLimit.SetFocus();
			MessageBoxW(L"Incorrect limit size.", L"Incorrect limit", MB_ICONERROR);
			return;
		}
	}
	else {
		m_editLimit.SetFocus();
		MessageBoxW(L"Limit size must be at least one.", L"Incorrect limit", MB_ICONERROR);
		return;
	}

	if (m_fReplace) {
		CStringW wstrReplace;
		m_comboReplace.GetWindowTextW(wstrReplace);
		m_wstrTextReplace = wstrReplace;
		if (m_wstrTextReplace.empty()) {
			m_comboReplace.SetFocus();
			MessageBoxW(m_pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
			return;
		}

		ComboReplaceFill(wstrReplace);
	}
	m_comboSearch.SetFocus();

	bool fSuccess { false };
	switch (GetSearchMode()) {
	case ESearchMode::MODE_HEXBYTES:
		fSuccess = PrepareHexBytes();
		break;
	default:
		using enum ESearchType;
		switch (GetSearchType()) {
		case TEXT_ASCII:
			fSuccess = PrepareTextASCII();
			break;
		case TEXT_UTF8:
			fSuccess = PrepareTextUTF8();
			break;
		case TEXT_UTF16:
			fSuccess = PrepareTextUTF16();
			break;
		case NUM_INT8:
			fSuccess = PrepareNumber<std::int8_t>();
			break;
		case NUM_UINT8:
			fSuccess = PrepareNumber<std::uint8_t>();
			break;
		case NUM_INT16:
			fSuccess = PrepareNumber<std::int16_t>();
			break;
		case NUM_UINT16:
			fSuccess = PrepareNumber<std::uint16_t>();
			break;
		case NUM_INT32:
			fSuccess = PrepareNumber<std::int32_t>();
			break;
		case NUM_UINT32:
			fSuccess = PrepareNumber<std::uint32_t>();
			break;
		case NUM_INT64:
			fSuccess = PrepareNumber<std::int64_t>();
			break;
		case NUM_UINT64:
			fSuccess = PrepareNumber<std::uint64_t>();
			break;
		case NUM_FLOAT:
			fSuccess = PrepareNumber<float>();
			break;
		case NUM_DOUBLE:
			fSuccess = PrepareNumber<double>();
			break;
		case STRUCT_FILETIME:
			fSuccess = PrepareFILETIME();
			break;
		default:
			break;
		}
		break;
	}
	if (!fSuccess)
		return;

	if (IsSelection()) { //Search in a selection.
		const auto vecSel = pHexCtrl->GetSelection();
		if (vecSel.empty()) //No selection.
			return;

		const auto& refFront = vecSel.front();

		//If the current selection differs from the previous one, or if FindAll, or new search string.
		if (fNewSearchStr || m_stSelSpan.ullOffset != refFront.ullOffset
			|| m_stSelSpan.ullSize != refFront.ullSize || m_fAll) {
			ResetSearch();
			m_stSelSpan = refFront;
			m_ullOffsetCurr = refFront.ullOffset;
		}

		const auto ullSelSize = refFront.ullSize;
		if (ullSelSize < m_vecSearchData.size()) //Selection is too small.
			return;

		m_ullOffsetBoundBegin = refFront.ullOffset;
		m_ullOffsetBoundEnd = m_ullOffsetBoundBegin + ullSelSize - m_vecSearchData.size();
		m_ullOffsetSentinel = m_ullOffsetBoundBegin + ullSelSize;
	}
	else { //Search in a whole data.
		m_ullOffsetBoundBegin = 0;
		m_ullOffsetBoundEnd = ullSearchDataSize - m_vecSearchData.size();
		m_ullOffsetSentinel = ullSearchDataSize;
	}

	if (m_ullOffsetCurr + m_vecSearchData.size() > ullSearchDataSize
		|| m_ullOffsetCurr + m_vecReplaceData.size() > ullSearchDataSize) {
		m_ullOffsetCurr = 0;
		m_fSecondMatch = false;
		return;
	}

	if (m_fReplace && (m_vecReplaceData.size() > m_vecSearchData.size())) {
		static constexpr auto wstrReplaceWarning { L"The replacing data is longer than searching data.\r\n"
			"Do you want to overwrite bytes following search occurrence?\r\nChoosing No will cancel replace." };
		if (MessageBoxW(wstrReplaceWarning, L"Warning", MB_YESNO | MB_ICONQUESTION | MB_TOPMOST) == IDNO)
			return;
	}

	Search();
	SetActiveWindow();
}

bool CHexDlgSearch::PrepareHexBytes()
{
	static constexpr auto pwszWrongInput { L"Unacceptable input character.\r\nAllowed characters are: 0123456789AaBbCcDdEeFf" };
	auto optData = NumStrToHex(m_wstrTextSearch, IsWildcard() ? static_cast<char>(m_uWildcard) : 0);
	if (!optData) {
		m_iWrap = 1;
		MessageBoxW(pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}

	m_vecSearchData = RangeToVecBytes(*optData);

	if (m_fReplace) {
		auto optDataRepl = NumStrToHex(m_wstrTextReplace);
		if (!optDataRepl) {
			MessageBoxW(pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
			m_comboReplace.SetFocus();
			return false;
		}

		m_vecReplaceData = RangeToVecBytes(*optDataRepl);
	}

	return true;
}

bool CHexDlgSearch::PrepareTextASCII()
{
	auto strSearch = WstrToStr(m_wstrTextSearch, CP_ACP); //Convert to the system default Windows ANSI code page.
	if (!IsMatchCase()) { //Make the string lowercase.
		std::transform(strSearch.begin(), strSearch.end(), strSearch.begin(),
			[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
	}

	m_vecSearchData = RangeToVecBytes(strSearch);
	m_vecReplaceData = RangeToVecBytes(WstrToStr(m_wstrTextReplace, CP_ACP));

	return true;
}

bool CHexDlgSearch::PrepareTextUTF16()
{
	auto m_wstrSearch = m_wstrTextSearch;
	if (!IsMatchCase()) { //Make the string lowercase.
		std::transform(m_wstrSearch.begin(), m_wstrSearch.end(), m_wstrSearch.begin(), std::towlower);
	}

	m_vecSearchData = RangeToVecBytes(m_wstrSearch);
	m_vecReplaceData = RangeToVecBytes(m_wstrTextReplace);

	return true;
}

bool CHexDlgSearch::PrepareTextUTF8()
{
	m_vecSearchData = RangeToVecBytes(WstrToStr(m_wstrTextSearch, CP_UTF8)); //Convert to UTF-8 string.
	m_vecReplaceData = RangeToVecBytes(WstrToStr(m_wstrTextReplace, CP_UTF8));

	return true;
}

template<typename T> requires TSize1248<T>
bool CHexDlgSearch::PrepareNumber()
{
	const auto optData = stn::StrToNum<T>(m_wstrTextSearch);
	if (!optData) {
		MessageBoxW(m_pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}

	T tData = *optData;
	T tDataRep { };

	if (m_fReplace) {
		const auto optDataRepl = stn::StrToNum<T>(m_wstrTextReplace);
		if (!optDataRepl) {
			MessageBoxW(m_pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		tDataRep = *optDataRepl;
	}

	if (IsBigEndian()) {
		tData = ByteSwap(tData);
		tDataRep = ByteSwap(tDataRep);
	}

	m_vecSearchData = RangeToVecBytes(tData);
	m_vecReplaceData = RangeToVecBytes(tDataRep);

	return true;
}

bool CHexDlgSearch::PrepareFILETIME()
{
	const auto [dwFormat, wchSepar] = GetHexCtrl()->GetDateInfo();
	const std::wstring wstrErr = L"Wrong FILETIME format.\r\nA correct format is: " + GetDateFormatString(dwFormat, wchSepar);
	const auto optFTSearch = StringToFileTime(m_wstrTextSearch, dwFormat);
	if (!optFTSearch) {
		MessageBoxW(wstrErr.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}

	FILETIME ftSearch = *optFTSearch;
	FILETIME ftReplace { };

	if (m_fReplace) {
		const auto optFTReplace = StringToFileTime(m_wstrTextReplace, dwFormat);
		if (!optFTReplace) {
			MessageBoxW(wstrErr.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		ftReplace = *optFTReplace;
	}

	if (IsBigEndian()) {
		ftSearch.dwLowDateTime = ByteSwap(ftSearch.dwLowDateTime);
		ftSearch.dwHighDateTime = ByteSwap(ftSearch.dwHighDateTime);
		ftReplace.dwLowDateTime = ByteSwap(ftReplace.dwLowDateTime);
		ftReplace.dwHighDateTime = ByteSwap(ftReplace.dwHighDateTime);
	}

	m_vecSearchData = RangeToVecBytes(ftSearch);
	m_vecReplaceData = RangeToVecBytes(ftReplace);

	return true;
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
	ClearList();
	GetHexCtrl()->SetSelection({ }, false, true); //Clear selection highlight.
	GetDlgItem(IDC_HEXCTRL_SEARCH_STATIC_RESULT)->SetWindowTextW(L"");
}

void CHexDlgSearch::Search()
{
	static constexpr auto uSizeQuick { 1024 * 1024 * 50U }; //50MB without creating a new thread.
	const auto pHexCtrl = GetHexCtrl();
	if (m_wstrTextSearch.empty() || !pHexCtrl->IsDataSet() || m_ullOffsetCurr >= pHexCtrl->GetDataSize())
		return;

	m_fFound = false;
	const auto ullEnd = IsForward() ? m_ullOffsetBoundEnd : m_ullOffsetBoundBegin;
	const auto ullSizeTotal = m_ullOffsetBoundEnd - m_ullOffsetBoundBegin;
	FINDRESULT stFind { };
	FINDERDATA stFinder { .pHexCtrl { pHexCtrl }, .spnFind { m_vecSearchData }, .ullStart { m_ullOffsetCurr },
		.ullEnd { ullEnd }, .ullStep { m_ullStep }, .ullOffsetSentinel { m_ullOffsetSentinel },
		.fForward { IsForward() }, .fInverted { IsInverted() }, .fCancelDlgOnEnd { true } };

	const auto lmbFindFwdSmall = [&](FINDERDATA& refFinder) {
		refFinder.pSearchFunc = GetSearchFunc(true, false);
		refFinder.fCancelDlgOnEnd = false;

		if (stFind = Finder(refFinder); stFind) {
			m_fFound = true;
			m_fSecondMatch = true;
			++m_dwCount;
			m_ullOffsetCurr = stFind.ullOffset;
		}

		if (!m_fFound) {
			m_iWrap = 1;
			if (m_fSecondMatch && !stFind.fCanceled) {
				m_ullOffsetCurr = m_ullOffsetBoundBegin; //Starting from the beginning.
				refFinder.ullStart = m_ullOffsetCurr;
				if (stFind = Finder(refFinder); stFind) {
					m_fFound = true;
					m_fDoCount = true;
					m_dwCount = 1;
					m_ullOffsetCurr = stFind.ullOffset;
				}
			}
		}
		};
	const auto lmbFindFwdThread = [&](FINDERDATA& refFinder) {
		refFinder.pSearchFunc = GetSearchFunc(true, true);
		refFinder.fCancelDlgOnEnd = true;

		//Just a tiny wrapper lambda, to be able to get stFind result out of the thread.
		//By passing Finder straight to a thread, we'll not be able to get the returned result.
		const auto lmbWrapper = [&](FINDERDATA& refFinder) {
			stFind = Finder(refFinder); };

		refFinder.ullStart = m_ullOffsetCurr;
		refFinder.pSearchFunc = GetSearchFunc(true, true);
		CHexDlgCallback dlgClbk(L"Searching...", L"", refFinder.ullStart, ullEnd, this);
		refFinder.pDlgClbk = &dlgClbk;
		std::thread thrd(lmbWrapper, std::ref(refFinder));
		dlgClbk.DoModal();
		thrd.join();

		if (stFind) {
			m_fFound = true;
			m_fSecondMatch = true;
			++m_dwCount;
			m_ullOffsetCurr = stFind.ullOffset;
		}

		if (!m_fFound) {
			m_iWrap = 1;
			if (m_fSecondMatch && !stFind.fCanceled) {
				m_ullOffsetCurr = m_ullOffsetBoundBegin; //Starting from the beginning.
				refFinder.ullStart = m_ullOffsetCurr;
				CHexDlgCallback dlgClbkSecond(L"Searching...", L"", refFinder.ullStart, ullEnd, this);
				refFinder.pDlgClbk = &dlgClbkSecond;
				std::thread thrdSecond(lmbWrapper, std::ref(refFinder));
				dlgClbkSecond.DoModal();
				thrdSecond.join();

				if (stFind) {
					m_fFound = true;
					m_fDoCount = true;
					m_dwCount = 1;
					m_ullOffsetCurr = stFind.ullOffset;
				}
			}
		}
		};

	pHexCtrl->SetRedraw(false);

	if (m_fReplace) {
		if (m_fAll) { //Replace All
			ClearList();
			m_dwCount = 0;
			m_dwReplaced = 0;

			const auto lmbReplaceAllSmall = [&vecSearchRes = m_vecSearchRes, &vecReplaceData = m_vecReplaceData,
				dwFoundLimit = m_dwFoundLimit](FINDERDATA& refFinder) {
				refFinder.fCancelDlgOnEnd = false;
				const auto sSizeRepl = vecReplaceData.size();
				while (const auto res = Finder(refFinder)) {
					if (res.ullOffset + sSizeRepl > refFinder.ullOffsetSentinel)
						break;

					Replace(refFinder.pHexCtrl, res.ullOffset, vecReplaceData);
					vecSearchRes.emplace_back(res.ullOffset); //Filling the vector of Replaced occurences.
					refFinder.ullStart = res.ullOffset + (sSizeRepl <= refFinder.ullStep ?
						refFinder.ullStep : sSizeRepl);
					if (refFinder.ullStart > refFinder.ullEnd || vecSearchRes.size() >= dwFoundLimit) {
						break;
					}
				}
				};
			const auto lmbReplaceAllThread = [&vecSearchRes = m_vecSearchRes, &vecReplaceData = m_vecReplaceData,
				dwFoundLimit = m_dwFoundLimit](FINDERDATA& refFinder) {
				refFinder.fCancelDlgOnEnd = false;
				const auto sSizeRepl = vecReplaceData.size();
				while (const auto res = Finder(refFinder)) {
					if (res.ullOffset + sSizeRepl > refFinder.ullOffsetSentinel)
						break;

					Replace(refFinder.pHexCtrl, res.ullOffset, vecReplaceData);
					vecSearchRes.emplace_back(res.ullOffset); //Filling the vector of Replaced occurences.
					refFinder.ullStart = res.ullOffset + (sSizeRepl <= refFinder.ullStep ?
						refFinder.ullStep : sSizeRepl);
					refFinder.pDlgClbk->SetCount(vecSearchRes.size());
					if (refFinder.ullStart > refFinder.ullEnd || vecSearchRes.size() >= dwFoundLimit
						|| refFinder.pDlgClbk->IsCanceled()) {
						break;
					}
				}
				refFinder.pDlgClbk->OnCancel();
				};

			if (ullSizeTotal <= uSizeQuick) {
				stFinder.pSearchFunc = GetSearchFunc(true, false);
				lmbReplaceAllSmall(stFinder);
			}
			else {
				stFinder.pSearchFunc = GetSearchFunc(true, true);
				CHexDlgCallback dlgClbk(L"Replacing...", L"Replaced: ", m_ullOffsetCurr, ullEnd, this);
				stFinder.pDlgClbk = &dlgClbk;
				std::thread thrd(lmbReplaceAllThread, std::ref(stFinder));
				dlgClbk.DoModal();
				thrd.join();
			}

			if (!m_vecSearchRes.empty()) {
				m_fFound = true;
				m_dwCount = m_dwReplaced = static_cast<DWORD>(m_vecSearchRes.size());
			}
			m_pListMain->SetItemCountEx(static_cast<int>(m_vecSearchRes.size()));

		}
		else if (IsForward()) { //Forward only.
			if (ullSizeTotal <= uSizeQuick) { //Fast, non-thread search.
				lmbFindFwdSmall(stFinder);
			}
			else {
				lmbFindFwdThread(stFinder);
			}

			if (m_fFound) {
				const auto sSizeRepl = m_vecReplaceData.size();
				if (stFind.ullOffset + sSizeRepl <= stFinder.ullOffsetSentinel) {
					Replace(pHexCtrl, m_ullOffsetCurr, m_vecReplaceData);
					++m_dwReplaced;
				}
			}
		}
	}
	else { //Search.
		if (m_fAll) {
			ClearList(); //Clearing all results.
			m_dwCount = 0;
			const auto lmbFindAllSmall = [&vecSearchRes = m_vecSearchRes, dwFoundLimit = m_dwFoundLimit]
			(FINDERDATA& refFinder) {
				refFinder.fCancelDlgOnEnd = false;
				while (const auto res = Finder(refFinder)) {
					vecSearchRes.emplace_back(res.ullOffset); //Filling the vector of Found occurences.
					refFinder.ullStart = res.ullOffset + refFinder.ullStep;
					if (refFinder.ullStart > refFinder.ullEnd || vecSearchRes.size() >= dwFoundLimit)
						break;
				}
				};
			const auto lmbFindAllThread = [&vecSearchRes = m_vecSearchRes, dwFoundLimit = m_dwFoundLimit]
			(FINDERDATA& refFinder) {
				refFinder.fCancelDlgOnEnd = false;
				while (const auto res = Finder(refFinder)) {
					vecSearchRes.emplace_back(res.ullOffset); //Filling the vector of Found occurences.
					refFinder.ullStart = res.ullOffset + refFinder.ullStep;
					refFinder.pDlgClbk->SetCount(vecSearchRes.size());
					if (refFinder.ullStart > refFinder.ullEnd || vecSearchRes.size() >= dwFoundLimit
						|| refFinder.pDlgClbk->IsCanceled())
						break;
				}
				refFinder.pDlgClbk->OnCancel();
				};

			if (ullSizeTotal <= uSizeQuick) {
				stFinder.pSearchFunc = GetSearchFunc(true, false);
				lmbFindAllSmall(stFinder);
			}
			else {
				stFinder.pSearchFunc = GetSearchFunc(true, true);
				CHexDlgCallback dlgClbk(L"Searching...", L"Found: ", m_ullOffsetCurr, ullEnd, this);
				stFinder.pDlgClbk = &dlgClbk;
				std::thread thrd(lmbFindAllThread, std::ref(stFinder));
				dlgClbk.DoModal();
				thrd.join();
			}

			if (!m_vecSearchRes.empty()) {
				m_fFound = true;
				m_dwCount = static_cast<DWORD>(m_vecSearchRes.size());
			}
			m_pListMain->SetItemCountEx(static_cast<int>(m_vecSearchRes.size()));
		}
		else {
			const auto lmbFindBackSmall = [&](FINDERDATA& refFinder) {
				refFinder.pSearchFunc = GetSearchFunc(false, false);
				refFinder.fCancelDlgOnEnd = false;
				const auto ullNext = m_ullOffsetCurr - m_ullStep;
				if (m_fSecondMatch && ullNext < m_ullOffsetCurr && ullNext >= m_ullOffsetBoundBegin) {
					m_ullOffsetCurr = ullNext;
					refFinder.ullStart = m_ullOffsetCurr;
					if (stFind = Finder(refFinder); stFind) {
						m_fFound = true;
						--m_dwCount;
						m_ullOffsetCurr = stFind.ullOffset;
					}
				}

				if (!m_fFound) {
					m_iWrap = -1;
					if (!stFind.fCanceled) {
						m_ullOffsetCurr = m_ullOffsetBoundEnd;
						refFinder.ullStart = m_ullOffsetCurr;
						if (stFind = Finder(refFinder); stFind) {
							m_fFound = true;
							m_fSecondMatch = true;
							m_fDoCount = false;
							m_dwCount = 1;
							m_ullOffsetCurr = stFind.ullOffset;
						}
					}
				}
				};
			const auto lmbFindBackThread = [&](FINDERDATA& refFinder) {
				refFinder.pSearchFunc = GetSearchFunc(false, true);
				refFinder.fCancelDlgOnEnd = true;
				const auto lmbWrapper = [&](FINDERDATA& refFinder) {
					stFind = Finder(refFinder); };

				const auto ullNext = m_ullOffsetCurr - m_ullStep;
				if (m_fSecondMatch && ullNext < m_ullOffsetCurr && ullNext >= m_ullOffsetBoundBegin) {
					m_ullOffsetCurr = ullNext;
					refFinder.ullStart = m_ullOffsetCurr;
					CHexDlgCallback dlgClbk(L"Searching...", L"", ullEnd, refFinder.ullStart, this);
					stFinder.pDlgClbk = &dlgClbk;
					std::thread thrd(lmbWrapper, std::ref(stFinder));
					dlgClbk.DoModal();
					thrd.join();

					if (stFind) {
						m_fFound = true;
						--m_dwCount;
						m_ullOffsetCurr = stFind.ullOffset;
					}
				}

				if (!m_fFound) {
					m_iWrap = -1;
					if (!stFind.fCanceled) {
						m_ullOffsetCurr = m_ullOffsetBoundEnd;
						refFinder.ullStart = m_ullOffsetCurr;
						CHexDlgCallback dlgClbkSecond(L"Searching...", L"", ullEnd, refFinder.ullStart, this);
						stFinder.pDlgClbk = &dlgClbkSecond;
						std::thread thrdSecond(lmbWrapper, std::ref(stFinder));
						dlgClbkSecond.DoModal();
						thrdSecond.join();

						if (stFind) {
							m_fFound = true;
							m_fSecondMatch = true;
							m_fDoCount = false;
							m_dwCount = 1;
							m_ullOffsetCurr = stFind.ullOffset;
						}
					}
				}
				};

			if (ullSizeTotal <= uSizeQuick) { //Fast, non-thread search.
				if (IsForward()) {
					//If anything was previously found we increase m_ullOffsetCurr by m_ullStep.
					m_ullOffsetCurr += m_fSecondMatch ? m_ullStep : 0;
					stFinder.ullStart = m_ullOffsetCurr;
					lmbFindFwdSmall(stFinder);
				}
				else {
					lmbFindBackSmall(stFinder);
				}
			}
			else {
				if (IsForward()) {
					m_ullOffsetCurr += m_fSecondMatch ? m_ullStep : 0;
					stFinder.ullStart = m_ullOffsetCurr;
					lmbFindFwdThread(stFinder);
				}
				else {
					lmbFindBackThread(stFinder);
				}
			}
		}
	}

	pHexCtrl->SetRedraw(true);

	std::wstring wstrInfo;
	if (m_fFound) {
		if (m_fAll) {
			if (m_fReplace) {
				wstrInfo = std::format(m_locale, L"{:L} occurrence(s) replaced.", m_dwReplaced);
				m_dwReplaced = 0;
				pHexCtrl->Redraw(); //Redraw in case of Replace all.
			}
			else {
				wstrInfo = std::format(m_locale, L"Found {:L} occurrences.", m_dwCount);
				m_dwCount = 0;
			}
		}
		else {
			if (m_fDoCount) {
				wstrInfo = std::format(m_locale, L"Found occurrence \u2116 {:L} from the beginning.", m_dwCount);
			}
			else {
				wstrInfo = L"Search found occurrence.";
			}

			HexCtrlHighlight({ { m_ullOffsetCurr, m_fReplace ? m_vecReplaceData.size() : m_vecSearchData.size() } });
			AddToList(m_ullOffsetCurr);

			if (m_fReplace) { //Increase next search step to replaced or m_ullStep amount.
				m_ullOffsetCurr += m_vecReplaceData.size() <= m_ullStep ? m_ullStep : m_vecReplaceData.size();
			}

			SetEditStartAt(m_ullOffsetCurr);
		}
	}
	else {
		if (stFind.fCanceled) {
			wstrInfo = L"Didn't find any occurrence, search was canceled.";
		}
		else {
			if (m_iWrap == 1) {
				wstrInfo = L"Didn't find any occurrence, the end is reached.";
			}
			else if (m_iWrap == -1) {
				wstrInfo = L"Didn't find any occurrence, the begining is reached.";
			}

			ResetSearch();
		}
	}

	GetDlgItem(IDC_HEXCTRL_SEARCH_STATIC_RESULT)->SetWindowTextW(wstrInfo.data());
	if (!m_fSearchNext) {
		SetForegroundWindow();
		SetFocus();
	}
	else { m_fSearchNext = false; }
}

void CHexDlgSearch::SetEditStartAt(ULONGLONG ullOffset)
{
	m_editStart.SetWindowTextW(std::format(L"0x{:X}", ullOffset).data());
}

void CHexDlgSearch::UpdateSearchReplaceControls()
{
	const auto* const pHexCtrl = GetHexCtrl();
	if (!pHexCtrl->IsCreated() || !pHexCtrl->IsDataSet())
		return;

	const auto fMutable = pHexCtrl->IsMutable();
	const auto fSearchEnabled = m_comboSearch.GetWindowTextLengthW() > 0;
	const auto fReplaceEnabled = fMutable && fSearchEnabled && m_comboReplace.GetWindowTextLengthW() > 0;

	m_comboReplace.EnableWindow(fMutable);
	GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_SEARCH_F)->EnableWindow(fSearchEnabled);
	GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_SEARCH_B)->EnableWindow(fSearchEnabled);
	GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_FINDALL)->EnableWindow(fSearchEnabled);
	GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_REPLACE)->EnableWindow(fReplaceEnabled);
	GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_REPLACE_ALL)->EnableWindow(fReplaceEnabled);
}


//Static functions.

void CHexDlgSearch::Replace(IHexCtrl* pHexCtrl, ULONGLONG ullIndex, const SpanCByte spnReplace)
{
	pHexCtrl->ModifyData({ .eModifyMode { EHexModifyMode::MODIFY_ONCE }, .spnData { spnReplace },
		.vecSpan { { ullIndex, spnReplace.size() } } });
}

auto CHexDlgSearch::Finder(const FINDERDATA& stFinder)->FINDRESULT
{
	const auto nSizeSearch = stFinder.spnFind.size();
	if (stFinder.ullStart + nSizeSearch > stFinder.ullOffsetSentinel) {
		if (stFinder.pDlgClbk != nullptr && stFinder.fCancelDlgOnEnd) {
			stFinder.pDlgClbk->OnCancel();
		}
		return { { }, false, false };
	}

	//Depends on search direction.
	const auto ullSizeTotal = stFinder.ullOffsetSentinel - (stFinder.fForward ? stFinder.ullStart : stFinder.ullEnd);
	const auto pHexCtrl = stFinder.pHexCtrl;
	const auto ullStep = stFinder.ullStep; //Search step.
	ULONGLONG ullLoopChunkSize;
	ULONGLONG ullMemToAcquire;
	ULONGLONG ullMemChunks;
	bool fBigStep { false };

	if (!pHexCtrl->IsVirtual()) {
		ullMemToAcquire = ullSizeTotal;
		ullLoopChunkSize = ullSizeTotal - nSizeSearch;
		ullMemChunks = 1;
	}
	else {
		ullMemToAcquire = pHexCtrl->GetCacheSize();
		if (ullMemToAcquire > ullSizeTotal) {
			ullMemToAcquire = ullSizeTotal;
		}

		ullLoopChunkSize = ullMemToAcquire - nSizeSearch;
		if (ullStep > ullLoopChunkSize) { //For very big steps.
			ullMemChunks = ullSizeTotal > ullStep ? ullSizeTotal / ullStep + ((ullSizeTotal % ullStep) ? 1 : 0) : 1;
			fBigStep = true;
		}
		else {
			ullMemChunks = ullSizeTotal > ullLoopChunkSize ? ullSizeTotal / ullLoopChunkSize
				+ ((ullSizeTotal % ullLoopChunkSize) ? 1 : 0) : 1;
		}
	}

	SEARCHFUNCDATA stSearch { .ullStart { stFinder.ullStart }, .ullEnd { stFinder.ullEnd },
		.ullLoopChunkSize { ullLoopChunkSize }, .ullMemToAcquire { ullMemToAcquire }, .ullMemChunks { ullMemChunks },
		.ullOffsetSentinel { stFinder.ullOffsetSentinel }, .llStep { static_cast<LONGLONG>(ullStep) },
		.pHexCtrl { stFinder.pHexCtrl }, .pDlgClbk { stFinder.pDlgClbk }, .spnFind { stFinder.spnFind },
		.fBigStep { fBigStep }, .fInverted { stFinder.fInverted }, .fCancelDlgOnEnd { stFinder.fCancelDlgOnEnd } };

	stFinder.pSearchFunc(&stSearch);

	return { stSearch.ullStart, stSearch.fResult, stSearch.fCanceled };
}

int CHexDlgSearch::MemCmpSIMDEQByte1(const __m128i* pWhere, std::byte bWhat)
{
	const auto m128iWhere = _mm_loadu_si128(pWhere);
	const auto m128iWhat = _mm_set1_epi8(static_cast<char>(bWhat));
	const auto m128iResult = _mm_cmpeq_epi8(m128iWhere, m128iWhat);
	const auto uiMask = static_cast<std::uint32_t>(_mm_movemask_epi8(m128iResult));
	return std::countr_zero(uiMask); //>15 here means not found, all in mask are zeros.
}

int CHexDlgSearch::MemCmpSIMDEQByte1Inv(const __m128i* pWhere, std::byte bWhat)
{
	const auto m128iWhere = _mm_loadu_si128(pWhere);
	const auto m128iWhat = _mm_set1_epi8(static_cast<char>(bWhat));
	const auto m128iResult = _mm_cmpeq_epi8(m128iWhere, m128iWhat);
	const auto iMask = static_cast<std::uint32_t>(_mm_movemask_epi8(m128iResult));
	return std::countr_zero(~iMask); //>15 here means not found, all in mask are ones.
}

template<CHexDlgSearch::SEARCHTYPE stType>
bool CHexDlgSearch::MemCmp(const std::byte* pWhere, const std::byte* pWhat, std::size_t nSize)
{
	using enum EMemCmp;
	if constexpr (stType.eMemCmp == DATA_BYTE1) {
		return *reinterpret_cast<const std::uint8_t*>(pWhere) == *reinterpret_cast<const std::uint8_t*>(pWhat);
	}
	else if constexpr (stType.eMemCmp == DATA_BYTE2) {
		return *reinterpret_cast<const std::uint16_t*>(pWhere) == *reinterpret_cast<const std::uint16_t*>(pWhat);
	}
	else if constexpr (stType.eMemCmp == DATA_BYTE4) {
		return *reinterpret_cast<const std::uint32_t*>(pWhere) == *reinterpret_cast<const std::uint32_t*>(pWhat);
	}
	else if constexpr (stType.eMemCmp == DATA_BYTE8) {
		return *reinterpret_cast<const std::uint64_t*>(pWhere) == *reinterpret_cast<const std::uint64_t*>(pWhat);
	}
	else if constexpr (stType.eMemCmp == CHAR_STR) {
		for (std::size_t i { 0 }; i < nSize; ++i, ++pWhere, ++pWhat) {
			if constexpr (stType.fWildcard) {
				if (*pWhat == m_uWildcard) //Checking for wildcard match.
					continue;
			}

			if constexpr (!stType.fMatchCase) {
				auto ch = static_cast<char>(*pWhere);
				if (ch >= 'A' && ch <= 'Z') { //If it's a capital letter.
					ch += 32; //Lowering this letter ('a' - 'A' = 32).
				}

				if (ch != static_cast<char>(*pWhat))
					return false;
			}
			else {
				if (*pWhere != *pWhat)
					return false;
			}
		}
		return true;
	}
	else if constexpr (stType.eMemCmp == WCHAR_STR) {
		auto pBuf1wch = reinterpret_cast<const wchar_t*>(pWhere);
		auto pBuf2wch = reinterpret_cast<const wchar_t*>(pWhat);
		for (std::size_t i { 0 }; i < (nSize / sizeof(wchar_t)); ++i, ++pBuf1wch, ++pBuf2wch) {
			if constexpr (stType.fWildcard) {
				if (*pBuf2wch == static_cast<wchar_t>(m_uWildcard)) //Checking for wildcard match.
					continue;
			}

			if constexpr (!stType.fMatchCase) {
				auto wch = *pBuf1wch;
				if (wch >= 'A' && wch <= 'Z') { //If it's a capital letter.
					wch += 32; //Lowering this letter ('a' - 'A' = 32).
				}

				if (wch != *pBuf2wch)
					return false;
			}
			else {
				if (*pBuf1wch != *pBuf2wch)
					return false;
			}
		}
		return true;
	}
}

template<CHexDlgSearch::SEARCHTYPE stType>
void CHexDlgSearch::SearchFuncFwd(SEARCHFUNCDATA* pSearch)
{
	//Members locality is important for the best performance of the tight search loop below.
	static constexpr auto LOOP_UNROLL_SIZE = 8U; //How many comparisons we do at one loop cycle.
	const auto ullEnd = pSearch->ullEnd;
	const auto ullMemChunks = pSearch->ullMemChunks;
	const auto ullOffsetSentinel = pSearch->ullOffsetSentinel;
	const auto llStep = pSearch->llStep;
	const auto pHexCtrl = pSearch->pHexCtrl;
	const auto pDlgClbk = pSearch->pDlgClbk;
	const auto pDataSearch = pSearch->spnFind.data();
	const auto nSizeSearch = pSearch->spnFind.size();
	const auto fBigStep = pSearch->fBigStep;
	const auto fInverted = pSearch->fInverted;
	auto ullOffsetSearch = pSearch->ullStart;
	auto ullLoopChunkSize = pSearch->ullLoopChunkSize;
	auto ullMemToAcquire = pSearch->ullMemToAcquire;

	using enum EMemCmp;
	const auto lmbEnd = [pSearch = pSearch]() {
		if constexpr (stType.fDlgClbck) {
			if (pSearch->fCancelDlgOnEnd) {
				pSearch->pDlgClbk->OnCancel();
			}
			else {
				//If MemCmp returns true at the very first hit, we don't reach the SetProgress in the main loop.
				//It might be the case for the "Find/Replace All" for example.
				//If the data all the same (e.g. 0xCDCDCDCD...) then MemCmp would return true every single step.
				pSearch->pDlgClbk->SetProgress(pSearch->ullStart);
			}
		}
		};
	const auto lmbFound = [&](ULONGLONG ullStart) {
		pSearch->ullStart = ullStart;
		pSearch->fResult = true;
		lmbEnd();
		};

	for (auto itChunk = 0ULL; itChunk < ullMemChunks; ++itChunk) {
		const auto spnData = pHexCtrl->GetData({ ullOffsetSearch, ullMemToAcquire });
		assert(!spnData.empty());

		//Branch for the fast SIMD search.
		if constexpr (stType.eMemCmp == DATA_BYTE1 && stType.fSIMD) {
			for (auto ullOffsetData = 0ULL; ullOffsetData <= ullLoopChunkSize; ullOffsetData += sizeof(__m128i)) {
				if ((ullOffsetData + sizeof(__m128i)) <= ullLoopChunkSize) {
					if constexpr (!stType.fInverted) { //SIMD forward not inverted.
						if (const auto iRes = MemCmpSIMDEQByte1(
							reinterpret_cast<const __m128i*>(spnData.data() + ullOffsetData), *pDataSearch);
							iRes < sizeof(__m128i)) {
							return lmbFound(ullOffsetSearch + ullOffsetData + iRes);
						}
					}
					else if constexpr (stType.fInverted) { //SIMD forward inverted.
						if (const auto iRes = MemCmpSIMDEQByte1Inv(
							reinterpret_cast<const __m128i*>(spnData.data() + ullOffsetData), *pDataSearch);
							iRes < sizeof(__m128i)) {
							return lmbFound(ullOffsetSearch + ullOffsetData + iRes);
						}
					}
				}
				else {
					for (auto i = 0; i <= ullLoopChunkSize - ullOffsetData; ++i) {
						if (MemCmp<stType>(spnData.data() + ullOffsetData + i, pDataSearch, 1) == !fInverted) {
							return lmbFound(ullOffsetSearch + ullOffsetData + i);
						}
					}
				}

				if constexpr (stType.fDlgClbck) {
					if (pDlgClbk->IsCanceled()) {
						pSearch->fCanceled = true;
						return;
					}
					pDlgClbk->SetProgress(ullOffsetSearch + ullOffsetData);
				}
			}
		}
		else { //Branch for the classical search.
			//Unrolling the loop, making LOOP_UNROLL_SIZE comparisons at one cycle.
			for (auto ullOffsetData = 0ULL; ullOffsetData <= ullLoopChunkSize; ullOffsetData += llStep * LOOP_UNROLL_SIZE) {
				//First memory comparison is always unconditional.
				if (MemCmp<stType>(spnData.data() + ullOffsetData, pDataSearch, nSizeSearch) == !fInverted) {
					return lmbFound(ullOffsetSearch + ullOffsetData);
				}

				//This branch allows to avoid the "ullOffsetData + llStep * 'nstep' <= ullLoopChunkSize" check every cycle.
				if ((ullOffsetData + llStep * (LOOP_UNROLL_SIZE - 1)) <= ullLoopChunkSize) {
					if (MemCmp<stType>(spnData.data() + ullOffsetData + llStep * 1, pDataSearch, nSizeSearch) == !fInverted) {
						return lmbFound(ullOffsetSearch + ullOffsetData + llStep * 1);
					}
					if (MemCmp<stType>(spnData.data() + ullOffsetData + llStep * 2, pDataSearch, nSizeSearch) == !fInverted) {
						return lmbFound(ullOffsetSearch + ullOffsetData + llStep * 2);
					}
					if (MemCmp<stType>(spnData.data() + ullOffsetData + llStep * 3, pDataSearch, nSizeSearch) == !fInverted) {
						return lmbFound(ullOffsetSearch + ullOffsetData + llStep * 3);
					}
					if (MemCmp<stType>(spnData.data() + ullOffsetData + llStep * 4, pDataSearch, nSizeSearch) == !fInverted) {
						return lmbFound(ullOffsetSearch + ullOffsetData + llStep * 4);
					}
					if (MemCmp<stType>(spnData.data() + ullOffsetData + llStep * 5, pDataSearch, nSizeSearch) == !fInverted) {
						return lmbFound(ullOffsetSearch + ullOffsetData + llStep * 5);
					}
					if (MemCmp<stType>(spnData.data() + ullOffsetData + llStep * 6, pDataSearch, nSizeSearch) == !fInverted) {
						return lmbFound(ullOffsetSearch + ullOffsetData + llStep * 6);
					}
					if (MemCmp<stType>(spnData.data() + ullOffsetData + llStep * 7, pDataSearch, nSizeSearch) == !fInverted) {
						return lmbFound(ullOffsetSearch + ullOffsetData + llStep * 7);
					}
				}
				//In this branch we check every comparison for the ullLoopChunkSize exceeding.
				//The one less comparison is needed here, otherwise we would've hit the branch above.
				else {
					if ((ullOffsetData + llStep * 1 <= ullLoopChunkSize)
						&& (MemCmp<stType>(spnData.data() + ullOffsetData + llStep * 1, pDataSearch, nSizeSearch) == !fInverted)) {
						return lmbFound(ullOffsetSearch + ullOffsetData + llStep * 1);
					}
					if ((ullOffsetData + llStep * 2 <= ullLoopChunkSize)
						&& (MemCmp<stType>(spnData.data() + ullOffsetData + llStep * 2, pDataSearch, nSizeSearch) == !fInverted)) {
						return lmbFound(ullOffsetSearch + ullOffsetData + llStep * 2);
					}
					if ((ullOffsetData + llStep * 3 <= ullLoopChunkSize)
						&& (MemCmp<stType>(spnData.data() + ullOffsetData + llStep * 3, pDataSearch, nSizeSearch) == !fInverted)) {
						return lmbFound(ullOffsetSearch + ullOffsetData + llStep * 3);
					}
					if ((ullOffsetData + llStep * 4 <= ullLoopChunkSize)
						&& (MemCmp<stType>(spnData.data() + ullOffsetData + llStep * 4, pDataSearch, nSizeSearch) == !fInverted)) {
						return lmbFound(ullOffsetSearch + ullOffsetData + llStep * 4);
					}
					if ((ullOffsetData + llStep * 5 <= ullLoopChunkSize)
						&& (MemCmp<stType>(spnData.data() + ullOffsetData + llStep * 5, pDataSearch, nSizeSearch) == !fInverted)) {
						return lmbFound(ullOffsetSearch + ullOffsetData + llStep * 5);
					}
					if ((ullOffsetData + llStep * 6 <= ullLoopChunkSize)
						&& (MemCmp<stType>(spnData.data() + ullOffsetData + llStep * 6, pDataSearch, nSizeSearch) == !fInverted)) {
						return lmbFound(ullOffsetSearch + ullOffsetData + llStep * 6);
					}
				}

				if constexpr (stType.fDlgClbck) {
					if (pDlgClbk->IsCanceled()) {
						pSearch->fCanceled = true;
						return;
					}
					pDlgClbk->SetProgress(ullOffsetSearch + ullOffsetData);
				}
			}
		}

		if (fBigStep) [[unlikely]] {
			if ((ullOffsetSearch + llStep) > ullEnd)
				break; //Upper bound reached.

			ullOffsetSearch += llStep;
			}
		else {
			ullOffsetSearch += ullLoopChunkSize;
		}

		if (ullOffsetSearch + ullMemToAcquire > ullOffsetSentinel) {
			ullMemToAcquire = ullOffsetSentinel - ullOffsetSearch;
			ullLoopChunkSize = ullMemToAcquire - nSizeSearch;
		}
	}

	lmbEnd();
}

template<CHexDlgSearch::SEARCHTYPE stType>
void CHexDlgSearch::SearchFuncBack(SEARCHFUNCDATA* pSearch)
{
	//Members locality is important for the best performance of the tight search loop below.
	static constexpr auto LOOP_UNROLL_SIZE = 8U; //How many comparisons we do at one loop cycle.
	const auto ullStart = pSearch->ullStart;
	const auto ullEnd = pSearch->ullEnd;
	const auto ullMemChunks = pSearch->ullMemChunks;
	const auto llStep = pSearch->llStep;
	const auto pHexCtrl = pSearch->pHexCtrl;
	const auto pDlgClbk = pSearch->pDlgClbk;
	const auto pDataSearch = pSearch->spnFind.data();
	const auto nSizeSearch = pSearch->spnFind.size();
	const auto fBigStep = pSearch->fBigStep;
	const auto fInverted = pSearch->fInverted;
	auto ullOffsetSearch = pSearch->ullStart - pSearch->ullLoopChunkSize;
	auto ullLoopChunkSize = pSearch->ullLoopChunkSize;
	auto ullMemToAcquire = pSearch->ullMemToAcquire;

	const auto lmbEnd = [pSearch = pSearch]() {
		if constexpr (stType.fDlgClbck) {
			if (pSearch->fCancelDlgOnEnd) {
				pSearch->pDlgClbk->OnCancel();
			}
			else {
				//If MemCmp returns true at the very first hit, we don't reach the SetProgress in the main loop.
				//It might be the case for the "Find/Replace All" for example.
				//If the data all the same (e.g. 0xCDCDCDCD...) then MemCmp would return true every single step.
				pSearch->pDlgClbk->SetProgress(pSearch->ullStart);
			}
		}
		};
	const auto lmbFound = [&](ULONGLONG ullStart) {
		pSearch->ullStart = ullStart;
		pSearch->fResult = true;
		lmbEnd();
		};

	if (ullOffsetSearch < ullEnd || ullOffsetSearch >((std::numeric_limits<ULONGLONG>::max)() - ullLoopChunkSize)) {
		ullMemToAcquire = (ullStart - ullEnd) + nSizeSearch;
		ullLoopChunkSize = ullMemToAcquire - nSizeSearch;
		ullOffsetSearch = ullEnd;
	}

	for (auto itChunk = ullMemChunks; itChunk > 0; --itChunk) {
		const auto spnData = pHexCtrl->GetData({ ullOffsetSearch, ullMemToAcquire });
		assert(!spnData.empty());
		for (auto llOffsetData = static_cast<LONGLONG>(ullLoopChunkSize); llOffsetData >= 0;
			llOffsetData -= llStep * LOOP_UNROLL_SIZE) { //llOffsetData might be negative.
			//First memory comparison is always unconditional.
			if (MemCmp<stType>(spnData.data() + llOffsetData, pDataSearch, nSizeSearch) == !fInverted) {
				return lmbFound(ullOffsetSearch + llOffsetData);
			}

			if ((llOffsetData - llStep * (LOOP_UNROLL_SIZE - 1)) >= 0) {
				if (MemCmp<stType>(spnData.data() + (llOffsetData - llStep * 1), pDataSearch, nSizeSearch) == !fInverted) {
					return lmbFound(ullOffsetSearch + (llOffsetData - llStep * 1));
				}
				if (MemCmp<stType>(spnData.data() + (llOffsetData - llStep * 2), pDataSearch, nSizeSearch) == !fInverted) {
					return lmbFound(ullOffsetSearch + (llOffsetData - llStep * 2));
				}
				if (MemCmp<stType>(spnData.data() + (llOffsetData - llStep * 3), pDataSearch, nSizeSearch) == !fInverted) {
					return lmbFound(ullOffsetSearch + (llOffsetData - llStep * 3));
				}
				if (MemCmp<stType>(spnData.data() + (llOffsetData - llStep * 4), pDataSearch, nSizeSearch) == !fInverted) {
					return lmbFound(ullOffsetSearch + (llOffsetData - llStep * 4));
				}
				if (MemCmp<stType>(spnData.data() + (llOffsetData - llStep * 5), pDataSearch, nSizeSearch) == !fInverted) {
					return lmbFound(ullOffsetSearch + (llOffsetData - llStep * 5));
				}
				if (MemCmp<stType>(spnData.data() + (llOffsetData - llStep * 6), pDataSearch, nSizeSearch) == !fInverted) {
					return lmbFound(ullOffsetSearch + (llOffsetData - llStep * 6));
				}
				if (MemCmp<stType>(spnData.data() + (llOffsetData - llStep * 7), pDataSearch, nSizeSearch) == !fInverted) {
					return lmbFound(ullOffsetSearch + (llOffsetData - llStep * 7));
				}
			}
			else {
				if ((llOffsetData - llStep * 1 >= 0)
					&& (MemCmp<stType>(spnData.data() + (llOffsetData - llStep * 1), pDataSearch, nSizeSearch) == !fInverted)) {
					return lmbFound(ullOffsetSearch + (llOffsetData - llStep * 1));
				}
				if ((llOffsetData - llStep * 2 >= 0)
					&& (MemCmp<stType>(spnData.data() + (llOffsetData - llStep * 2), pDataSearch, nSizeSearch) == !fInverted)) {
					return lmbFound(ullOffsetSearch + (llOffsetData - llStep * 2));
				}
				if ((llOffsetData - llStep * 3 >= 0)
					&& (MemCmp<stType>(spnData.data() + (llOffsetData - llStep * 3), pDataSearch, nSizeSearch) == !fInverted)) {
					return lmbFound(ullOffsetSearch + (llOffsetData - llStep * 3));
				}
				if ((llOffsetData - llStep * 4 >= 0)
					&& (MemCmp<stType>(spnData.data() + (llOffsetData - llStep * 4), pDataSearch, nSizeSearch) == !fInverted)) {
					return lmbFound(ullOffsetSearch + (llOffsetData - llStep * 4));
				}
				if ((llOffsetData - llStep * 5 >= 0)
					&& (MemCmp<stType>(spnData.data() + (llOffsetData - llStep * 5), pDataSearch, nSizeSearch) == !fInverted)) {
					return lmbFound(ullOffsetSearch + (llOffsetData - llStep * 5));
				}
				if ((llOffsetData - llStep * 6 >= 0)
					&& (MemCmp<stType>(spnData.data() + (llOffsetData - llStep * 6), pDataSearch, nSizeSearch) == !fInverted)) {
					return lmbFound(ullOffsetSearch + (llOffsetData - llStep * 6));
				}
			}

			if constexpr (stType.fDlgClbck) {
				if (pDlgClbk->IsCanceled()) {
					pSearch->fCanceled = true;
					return;
				}
				pDlgClbk->SetProgress(ullStart - (ullOffsetSearch + llOffsetData));
			}
		}

		if (fBigStep) [[unlikely]] {
			if ((ullOffsetSearch - llStep) < ullEnd || (ullOffsetSearch - llStep) > ((std::numeric_limits<ULONGLONG>::max)() - llStep))
				break; //Lower bound reached.

			ullOffsetSearch -= ullLoopChunkSize;
			}
		else {
			if ((ullOffsetSearch - ullLoopChunkSize) < ullEnd || ((ullOffsetSearch - ullLoopChunkSize) >
				((std::numeric_limits<ULONGLONG>::max)() - ullLoopChunkSize))) {
				ullMemToAcquire = (ullOffsetSearch - ullEnd) + nSizeSearch;
				ullLoopChunkSize = ullMemToAcquire - nSizeSearch;
				ullOffsetSearch = ullEnd;
			}
			else {
				ullOffsetSearch -= ullLoopChunkSize;
			}
		}
	}

	lmbEnd();
}