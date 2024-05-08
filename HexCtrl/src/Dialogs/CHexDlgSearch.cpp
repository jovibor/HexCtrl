/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
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
#include <immintrin.h>
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
	ULONGLONG ullOffset { };
	bool      fFound { false };
	bool      fCanceled { false }; //Search was canceled by pressing "Cancel".
	explicit operator bool()const { return fFound; };
};

struct CHexDlgSearch::SEARCHFUNCDATA {
	ULONGLONG ullStartFrom { };
	ULONGLONG ullRngStart { };
	ULONGLONG ullRngEnd { };
	ULONGLONG ullStep { };
	ULONGLONG ullMemChunks { };      //How many memory chunks to search in.
	ULONGLONG ullMemToAcquire { };   //Size of a chunk.
	ULONGLONG ullChunkMaxOffset { }; //Maximum offset to search in chunk.
	CHexDlgCallback* pDlgClbk { };
	IHexCtrl* pHexCtrl { };
	SpanCByte spnFind { };
	bool fBigStep { };
	bool fInverted { };
};

BEGIN_MESSAGE_MAP(CHexDlgSearch, CDialogEx)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_BTN_SEARCHF, &CHexDlgSearch::OnButtonSearchF)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_BTN_SEARCHB, &CHexDlgSearch::OnButtonSearchB)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_BTN_FINDALL, &CHexDlgSearch::OnButtonFindAll)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_BTN_REPL, &CHexDlgSearch::OnButtonReplace)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_BTN_REPLALL, &CHexDlgSearch::OnButtonReplaceAll)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_CHK_SEL, &CHexDlgSearch::OnCheckSel)
	ON_CBN_SELCHANGE(IDC_HEXCTRL_SEARCH_COMBO_MODE, &CHexDlgSearch::OnComboModeSelChange)
	ON_CBN_SELCHANGE(IDC_HEXCTRL_SEARCH_COMBO_TYPE, &CHexDlgSearch::OnComboTypeSelChange)
	ON_CBN_EDITUPDATE(IDC_HEXCTRL_SEARCH_COMBO_FIND, &CHexDlgSearch::SetControlsState)
	ON_CBN_EDITUPDATE(IDC_HEXCTRL_SEARCH_COMBO_REPL, &CHexDlgSearch::SetControlsState)
	ON_NOTIFY(LVN_GETDISPINFOW, IDC_HEXCTRL_SEARCH_LIST, &CHexDlgSearch::OnListGetDispInfo)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_HEXCTRL_SEARCH_LIST, &CHexDlgSearch::OnListItemChanged)
	ON_NOTIFY(NM_RCLICK, IDC_HEXCTRL_SEARCH_LIST, &CHexDlgSearch::OnListRClick)
	ON_WM_ACTIVATE()
	ON_WM_CLOSE()
	ON_WM_CTLCOLOR()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

auto CHexDlgSearch::GetDlgItemHandle(EHexDlgItem eItem)const->HWND
{
	if (!IsWindow(m_hWnd)) {
		return { };
	}

	using enum EHexDlgItem;
	switch (eItem) {
	case SEARCH_COMBO_FIND:
		return m_comboFind;
	case SEARCH_COMBO_REPLACE:
		return m_comboReplace;
	case SEARCH_EDIT_START:
		return m_editStartFrom;
	case SEARCH_EDIT_STEP:
		return m_editStep;
	case SEARCH_EDIT_RNGBEG:
		return m_editRngBegin;
	case SEARCH_EDIT_RNGEND:
		return m_editRngEnd;
	case SEARCH_EDIT_LIMIT:
		return m_editLimit;
	default:
		return { };
	}
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
	return IsWindow(m_hWnd) && GetHexCtrl()->IsDataSet();
}

void CHexDlgSearch::SearchNextPrev(bool fForward)
{
	m_fForward = fForward;
	m_fReplace = false;
	m_fAll = false;
	m_fSearchNext = true;
	Prepare();
}

void CHexDlgSearch::SetDlgProperties(std::uint64_t u64Flags)
{
	m_u64Flags = u64Flags;
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
		if (m_vecSearchRes.size() < static_cast<std::size_t>(m_dwLimit)) {
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

void CHexDlgSearch::CalcMemChunks(SEARCHFUNCDATA& refData)const
{
	const auto nSizeSearch = refData.spnFind.size();
	if (refData.ullStartFrom + nSizeSearch > GetSentinel()) {
		refData.ullMemChunks = { };
		refData.ullMemToAcquire = { };
		refData.ullChunkMaxOffset = { };
		return;
	}

	const auto ullSizeTotal = GetSearchRngSize(); //Depends on search direction.
	const auto pHexCtrl = refData.pHexCtrl;
	const auto ullStep = refData.ullStep;
	ULONGLONG ullMemChunks;
	ULONGLONG ullMemToAcquire;
	ULONGLONG ullChunkMaxOffset;
	bool fBigStep { false };

	if (!pHexCtrl->IsVirtual()) {
		ullMemChunks = 1;
		ullMemToAcquire = ullSizeTotal;
		ullChunkMaxOffset = ullSizeTotal - nSizeSearch;
	}
	else {
		ullMemToAcquire = pHexCtrl->GetCacheSize();
		if (ullMemToAcquire > ullSizeTotal) {
			ullMemToAcquire = ullSizeTotal;
		}

		ullChunkMaxOffset = ullMemToAcquire - nSizeSearch;
		if (ullStep > ullChunkMaxOffset) { //For very big steps.
			ullMemChunks = ullSizeTotal > ullStep ? ullSizeTotal / ullStep + ((ullSizeTotal % ullStep) ? 1 : 0) : 1;
			fBigStep = true;
		}
		else {
			ullMemChunks = ullSizeTotal > ullChunkMaxOffset ? ullSizeTotal / ullChunkMaxOffset
				+ ((ullSizeTotal % ullChunkMaxOffset) ? 1 : 0) : 1;
		}
	}

	refData.ullMemChunks = ullMemChunks;
	refData.ullMemToAcquire = ullMemToAcquire;
	refData.ullChunkMaxOffset = ullChunkMaxOffset;
	refData.fBigStep = fBigStep;
}

void CHexDlgSearch::ClearComboType()
{
	m_comboType.SetRedraw(FALSE);
	for (auto iIndex = m_comboType.GetCount() - 1; iIndex >= 0; --iIndex) {
		m_comboType.DeleteString(iIndex);
	}
	m_comboType.SetRedraw(TRUE);
}

void CHexDlgSearch::ClearList()
{
	m_pListMain->SetItemCountEx(0);
	m_vecSearchRes.clear();
}

void CHexDlgSearch::ComboSearchFill(LPCWSTR pwsz)
{
	//Insert text into ComboBox only if it's not already there.
	if (m_comboFind.FindStringExact(0, pwsz) == CB_ERR) {
		//Keep max 50 strings in list.
		if (m_comboFind.GetCount() == 50) {
			m_comboFind.DeleteString(49);
		}
		m_comboFind.InsertString(0, pwsz);
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

auto CHexDlgSearch::CreateSearchData(CHexDlgCallback* pDlgClbk)const->SEARCHFUNCDATA
{
	SEARCHFUNCDATA stData { .ullStartFrom { GetStartFrom() }, .ullRngStart { GetRngStart() },
		.ullRngEnd { GetRngEnd() }, .ullStep { GetStep() }, .pDlgClbk { pDlgClbk }, .pHexCtrl { GetHexCtrl() },
		.spnFind { GetSearchSpan() }, .fInverted { IsInverted() }
	};

	CalcMemChunks(stData);

	return stData;
}

void CHexDlgSearch::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_CHK_SEL, m_btnSel);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_CHK_WC, m_btnWC);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_CHK_INV, m_btnInv);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_CHK_BE, m_btnBE);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_CHK_MC, m_btnMC);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_COMBO_FIND, m_comboFind);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_COMBO_REPL, m_comboReplace);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_COMBO_MODE, m_comboMode);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_COMBO_TYPE, m_comboType);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_EDIT_START, m_editStartFrom);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_EDIT_STEP, m_editStep);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_EDIT_RNGBEG, m_editRngBegin);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_EDIT_RNGEND, m_editRngEnd);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_EDIT_LIMIT, m_editLimit);
}

void CHexDlgSearch::FindAll()
{
	ClearList(); //Clearing all results.
	m_dwCount = 0;
	const auto pSearchFunc = GetSearchFunc(true, !IsSmallSearch());
	auto stFuncData = CreateSearchData();

	if (IsSmallSearch()) {
		auto lmbWrapper = [&]()mutable->FINDRESULT {
			CalcMemChunks(stFuncData);
			return pSearchFunc(stFuncData);
			};

		while (const auto findRes = lmbWrapper()) {
			m_vecSearchRes.emplace_back(findRes.ullOffset); //Filling the vector of Found occurences.

			const auto ullNext = findRes.ullOffset + GetStep();
			if (ullNext > GetLastSearchOffset() || m_vecSearchRes.size() >= m_dwLimit)
				break;

			m_ullStartFrom = stFuncData.ullStartFrom = ullNext;
		}
	}
	else {
		CHexDlgCallback dlgClbk(L"Searching...", L"Found: ", GetStartFrom(), GetLastSearchOffset(), this);
		stFuncData.pDlgClbk = &dlgClbk;
		const auto lmbFindAllThread = [&]() {
			auto lmbWrapper = [&]()mutable->FINDRESULT {
				CalcMemChunks(stFuncData);
				return pSearchFunc(stFuncData);
				};

			while (const auto findRes = lmbWrapper()) {
				m_vecSearchRes.emplace_back(findRes.ullOffset); //Filling the vector of Found occurences.
				dlgClbk.SetProgress(findRes.ullOffset);
				dlgClbk.SetCount(m_vecSearchRes.size());

				const auto ullNext = findRes.ullOffset + GetStep();
				if (ullNext > GetLastSearchOffset() || m_vecSearchRes.size() >= m_dwLimit
					|| dlgClbk.IsCanceled())
					break;

				m_ullStartFrom = stFuncData.ullStartFrom = ullNext;
			}
			dlgClbk.OnCancel();
			};
		std::thread thrd(lmbFindAllThread);
		dlgClbk.DoModal();
		thrd.join();
	}

	if (!m_vecSearchRes.empty()) {
		m_fFound = true;
		m_dwCount = static_cast<DWORD>(m_vecSearchRes.size());
	}

	m_pListMain->SetItemCountEx(static_cast<int>(m_vecSearchRes.size()));
}

void CHexDlgSearch::FindForward()
{
	FINDRESULT findRes;
	auto lmbFind = [&]() {
		const auto pSearchFunc = GetSearchFunc(true, !IsSmallSearch());
		auto stFuncData = CreateSearchData();

		if (IsSmallSearch()) {
			findRes = pSearchFunc(stFuncData);
		}
		else {
			CHexDlgCallback dlgClbk(L"Searching...", L"", GetStartFrom(), GetLastSearchOffset(), this);
			stFuncData.pDlgClbk = &dlgClbk;
			const auto lmbWrapper = [&]() {
				findRes = pSearchFunc(stFuncData);
				dlgClbk.OnCancel();
				};
			std::thread thrd(lmbWrapper);
			dlgClbk.DoModal();
			thrd.join();
		}
		};

	if (lmbFind(); findRes) {
		m_fSecondMatch = true;
		++m_dwCount;
		m_ullStartFrom = findRes.ullOffset;
	}

	if (!findRes) {
		m_iWrap = 1;
		if (m_fSecondMatch && !findRes.fCanceled) {
			m_ullStartFrom = GetRngStart(); //Starting from the beginning.
			if (lmbFind(); findRes) {
				m_fDoCount = true;
				m_dwCount = 1;
				m_ullStartFrom = findRes.ullOffset;
			}
		}
	}
	m_fFound = findRes.fFound;
}

void CHexDlgSearch::FindBackward()
{
	FINDRESULT findRes;
	auto lmbFind = [&]() {
		const auto pSearchFunc = GetSearchFunc(false, !IsSmallSearch());
		auto stFuncData = CreateSearchData();

		if (IsSmallSearch()) {
			findRes = pSearchFunc(stFuncData);
		}
		else {
			CHexDlgCallback dlgClbk(L"Searching...", L"", GetRngStart(), GetStartFrom(), this);
			stFuncData.pDlgClbk = &dlgClbk;
			const auto lmbWrapper = [&]() {
				findRes = pSearchFunc(stFuncData);
				dlgClbk.OnCancel();
				};
			std::thread thrd(lmbWrapper);
			dlgClbk.DoModal();
			thrd.join();
		}
		};

	const auto ullNext = m_ullStartFrom - GetStep();
	if (m_fSecondMatch && ullNext < m_ullStartFrom && ullNext >= GetRngStart()) {
		m_ullStartFrom = ullNext;
		if (lmbFind(); findRes) {
			--m_dwCount;
			m_ullStartFrom = findRes.ullOffset;
		}
	}

	if (!findRes) {
		m_iWrap = -1;
		if (!findRes.fCanceled) {
			m_ullStartFrom = GetLastSearchOffset(); //Starting from the end.
			if (lmbFind(); findRes) {
				m_fSecondMatch = true;
				m_fDoCount = false;
				m_dwCount = 1;
				m_ullStartFrom = findRes.ullOffset;
			}
		}
	}
	m_fFound = findRes.fFound;
}

auto CHexDlgSearch::GetHexCtrl()const->IHexCtrl*
{
	return m_pHexCtrl;
}

auto CHexDlgSearch::GetLastSearchOffset()const->ULONGLONG
{
	return GetSentinel() - GetSearchDataSize();
}

auto CHexDlgSearch::GetReplaceSpan()const->SpanCByte
{
	return m_vecReplaceData;
}

auto CHexDlgSearch::GetReplaceDataSize()const->DWORD
{
	return static_cast<DWORD>(m_vecReplaceData.size());
}

auto CHexDlgSearch::GetRngStart()const->ULONGLONG
{
	return m_ullRngBegin;
}

auto CHexDlgSearch::GetRngEnd()const->ULONGLONG
{
	return m_ullRngEnd;
}

auto CHexDlgSearch::GetRngSize()const->ULONGLONG
{
	return m_ullRngEnd - m_ullRngBegin + 1;
}

auto CHexDlgSearch::GetSearchDataSize()const->DWORD
{
	return static_cast<DWORD>(m_vecSearchData.size());
}

auto CHexDlgSearch::GetSearchFunc(bool fFwd, bool fDlgClbck)const->PtrSearchFunc
{
	return fFwd ? (fDlgClbck ? GetSearchFuncFwd<true>() : GetSearchFuncFwd<false>()) :
		(fDlgClbck ? GetSearchFuncBack<true>() : GetSearchFuncBack<false>());
}

template<bool fDlgClbck>
auto CHexDlgSearch::GetSearchFuncFwd()const->PtrSearchFunc
{
	//The `fDlgClbck` arg ensures that no runtime check will be performed for the 
	//'SEARCHFUNCDATA::pDlgClbk == nullptr', at the hot path inside the SearchFunc function.

	using enum ESearchType; using enum EMemCmp;
	if (GetStep() == 1 && !IsWildcard()) {
		switch (GetSearchDataSize()) {
		case 1: //Special case for 1 byte data size SIMD.
			return IsInverted() ?
				SearchFuncFwdByte1<SEARCHTYPE(DATA_BYTE1, fDlgClbck, false, false, true)> :
				SearchFuncFwdByte1<SEARCHTYPE(DATA_BYTE1, fDlgClbck, false, false, false)>;
		case 2: //Special case for 2 bytes data size SIMD.
			return IsInverted() ?
				SearchFuncFwdByte2<SEARCHTYPE(DATA_BYTE2, fDlgClbck, false, false, true)> :
				SearchFuncFwdByte2<SEARCHTYPE(DATA_BYTE2, fDlgClbck, false, false, false)>;
		case 4: //Special case for 4 bytes data size SIMD.
			return IsInverted() ?
				SearchFuncFwdByte4<SEARCHTYPE(DATA_BYTE4, fDlgClbck, false, false, true)> :
				SearchFuncFwdByte4<SEARCHTYPE(DATA_BYTE4, fDlgClbck, false, false, false)>;
		default:
			break;
		};
	}

	switch (GetSearchType()) {
	case HEXBYTES:
		return IsWildcard() ?
			SearchFuncFwd<SEARCHTYPE(CHAR_STR, fDlgClbck, true, true)> :
			SearchFuncFwd<SEARCHTYPE(CHAR_STR, fDlgClbck, true, false)>;
	case TEXT_ASCII:
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
		return SearchFuncFwd<SEARCHTYPE(DATA_BYTE1, fDlgClbck)>;
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
		return IsWildcard() ?
			SearchFuncBack<SEARCHTYPE(CHAR_STR, fDlgClbck, true, true)> :
			SearchFuncBack<SEARCHTYPE(CHAR_STR, fDlgClbck, true, false)>;
	case TEXT_ASCII:
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
		return SearchFuncBack<SEARCHTYPE(DATA_BYTE1, fDlgClbck)>;
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

auto CHexDlgSearch::GetSearchMode()const->CHexDlgSearch::ESearchMode
{
	return static_cast<ESearchMode>(m_comboMode.GetItemData(m_comboMode.GetCurSel()));
}

auto CHexDlgSearch::GetSearchRngSize()const->ULONGLONG
{
	return IsForward() ? GetSentinel() - GetStartFrom() :
		(GetStartFrom() - GetRngStart()) + GetSearchDataSize();
}

auto CHexDlgSearch::GetSearchSpan()const->SpanCByte
{
	return m_vecSearchData;
}

auto CHexDlgSearch::GetSearchType()const->ESearchType
{
	if (GetSearchMode() == ESearchMode::MODE_HEXBYTES) {
		return ESearchType::HEXBYTES;
	}

	return static_cast<ESearchType>(m_comboType.GetItemData(m_comboType.GetCurSel()));
}

auto CHexDlgSearch::GetSentinel()const->ULONGLONG
{
	return m_ullRngEnd + 1; //This offset is non-dereferenceable.
}

auto CHexDlgSearch::GetStartFrom()const->ULONGLONG
{
	return m_ullStartFrom;
}

auto CHexDlgSearch::GetStep()const->ULONGLONG
{
	return m_ullStep;
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

bool CHexDlgSearch::IsFreshSearch()const
{
	return m_fFreshSearch;
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
	return m_u64Flags & HEXCTRL_FLAG_DLG_NOESC;
}

bool CHexDlgSearch::IsReplace()const
{
	return m_fReplace;
}

bool CHexDlgSearch::IsSelection()const
{
	return m_btnSel.GetCheck() == BST_CHECKED;
}

bool CHexDlgSearch::IsSmallSearch()const
{
	constexpr auto uSizeQuick { 1024U * 1024U * 50U }; //50MB without creating a new thread.
	return GetSearchRngSize() <= uSizeQuick;
}

bool CHexDlgSearch::IsWildcard()const
{
	return m_btnWC.IsWindowEnabled() && m_btnWC.GetCheck() == BST_CHECKED;
}

void CHexDlgSearch::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if (!m_pHexCtrl->IsCreated())
		return;

	if (nState == WA_ACTIVE || nState == WA_CLICKACTIVE) {
		m_comboFind.SetFocus();
		SetControlsState();
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
	m_editStartFrom.EnableWindow(!IsSelection());
	m_editRngBegin.EnableWindow(!IsSelection());
	m_editRngEnd.EnableWindow(!IsSelection());
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
		m_comboFind.SetCueBanner(wstr.data());
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
		m_comboFind.SetCueBanner(L"");
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
				m_fReplace ? m_vecReplaceData.size() : m_vecSearchData.size() } }, .wstrDesc { m_wstrSearch },
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

	if (pWnd->GetDlgCtrlID() == IDC_HEXCTRL_SEARCH_STAT_RESULT) {
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
	m_wstrSearch.clear();
	m_wstrReplace.clear();
	m_u64Flags = { };
	m_pHexCtrl = nullptr;
}

BOOL CHexDlgSearch::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	static constexpr auto iTextLimit { 512 };

	m_comboFind.LimitText(iTextLimit);
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

	m_pListMain->CreateDialogCtrl(IDC_HEXCTRL_SEARCH_LIST, this);
	m_pListMain->SetExtendedStyle(LVS_EX_HEADERDRAGDROP);
	m_pListMain->InsertColumn(0, L"№", LVCFMT_LEFT, 50);
	m_pListMain->InsertColumn(1, L"Offset", LVCFMT_LEFT, 455);

	m_menuList.CreatePopupMenu();
	m_menuList.AppendMenuW(MF_STRING, static_cast<UINT_PTR>(EMenuID::IDM_SEARCH_ADDBKM), L"Add bookmark(s)");
	m_menuList.AppendMenuW(MF_STRING, static_cast<UINT_PTR>(EMenuID::IDM_SEARCH_SELECTALL), L"Select All");
	m_menuList.AppendMenuW(MF_SEPARATOR);
	m_menuList.AppendMenuW(MF_STRING, static_cast<UINT_PTR>(EMenuID::IDM_SEARCH_CLEARALL), L"Clear All");

	m_editStep.SetWindowTextW(std::format(L"{}", GetStep()).data()); //"Step" edit box text.
	m_editLimit.SetWindowTextW(std::format(L"{}", m_dwLimit).data()); //"Limit search hits" edit box text.
	m_editRngBegin.SetCueBanner(L"range begin");
	m_editRngEnd.SetCueBanner(L"range end");

	const auto hwndTipWC = CreateWindowExW(0, TOOLTIPS_CLASSW, nullptr, WS_POPUP | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, nullptr, nullptr, nullptr);
	const auto hwndTipInv = CreateWindowExW(0, TOOLTIPS_CLASSW, nullptr, WS_POPUP | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, nullptr, nullptr, nullptr);
	const auto hwndTipEndOffset = CreateWindowExW(0, TOOLTIPS_CLASSW, nullptr, WS_POPUP | TTS_ALWAYSTIP,
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

	stToolInfo.uId = reinterpret_cast<UINT_PTR>(m_editRngEnd.m_hWnd); //"Search range end" edit box tooltip.
	wstrToolText = L"Last offset for the search.\r\nEmpty means until data end.";
	stToolInfo.lpszText = wstrToolText.data();
	::SendMessageW(hwndTipEndOffset, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&stToolInfo));
	::SendMessageW(hwndTipEndOffset, TTM_SETDELAYTIME, TTDT_AUTOPOP, static_cast<LPARAM>(LOWORD(0x7FFF)));
	::SendMessageW(hwndTipEndOffset, TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(1000));

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
	const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	if (pNMI->iItem < 0 || pNMI->iSubItem < 0 || !(pNMI->uNewState & LVIS_SELECTED))
		return;

	VecSpan vecSpan;
	int nItem = -1;
	for (auto i = 0UL; i < m_pListMain->GetSelectedCount(); ++i) {
		nItem = m_pListMain->GetNextItem(nItem, LVNI_SELECTED);

		//Do not yet add selected (clicked) item (in multiselect), will add it after the loop,
		//so that it's always last in the vecSpan to highlight it in HexCtrlHighlight.
		if (pNMI->iItem != nItem) {
			vecSpan.emplace_back(m_vecSearchRes.at(static_cast<std::size_t>(nItem)),
				m_fReplace ? m_vecReplaceData.size() : m_vecSearchData.size());
		}
	}

	const auto ullOffset = m_vecSearchRes[static_cast<std::size_t>(pNMI->iItem)];
	vecSpan.emplace_back(ullOffset, m_fReplace ? m_vecReplaceData.size() : m_vecSearchData.size());
	HexCtrlHighlight(vecSpan);
	SetEditStartFrom(ullOffset);
	m_ullStartFrom = ullOffset;
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
	if (!IsWindow(m_hWnd))
		return;

	constexpr auto uSearchSizeLimit { 256U };
	const auto pHexCtrl = GetHexCtrl();
	if (!pHexCtrl->IsDataSet())
		return;

	const auto ullHexDataSize = pHexCtrl->GetDataSize();

	//"Search" text.
	const auto iSearchSize = m_comboFind.GetWindowTextLengthW();
	if (iSearchSize == 0 || iSearchSize > uSearchSizeLimit) {
		m_comboFind.SetFocus();
		return;
	}

	CStringW wstrSearch;
	m_comboFind.GetWindowTextW(wstrSearch);
	if (wstrSearch.Compare(m_wstrSearch.data()) != 0) {
		ResetSearch();
		m_wstrSearch = wstrSearch;
	}

	//"Replace" text.
	if (IsReplace()) {
		if (m_comboReplace.GetWindowTextLengthW() == 0) {
			m_comboReplace.SetFocus();
			MessageBoxW(m_pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
			return;
		}

		CStringW wstrReplace;
		m_comboReplace.GetWindowTextW(wstrReplace);
		m_wstrReplace = wstrReplace;
		ComboReplaceFill(wstrReplace);
	}

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

	ComboSearchFill(wstrSearch); //Adding current search text to the search combo-box.

	ULONGLONG ullStartFrom;
	ULONGLONG ullRngStart;
	ULONGLONG ullRngEnd;

	//"Search range start/end" offsets.
	if (!IsSelection()) { //Do not use "Search range start/end offsets" when in selection.
		//"Search range start offset".
		if (m_editRngBegin.GetWindowTextLengthW() == 0) {
			ullRngStart = 0;
		}
		else {
			CStringW wstrRngStart;
			m_editRngBegin.GetWindowTextW(wstrRngStart);
			const auto optRngStart = stn::StrToUInt64(wstrRngStart.GetString());
			if (!optRngStart) {
				m_editRngBegin.SetFocus();
				MessageBoxW(L"Search range start offset is incorrect.", L"Incorrect offset", MB_ICONERROR);
				return;
			}

			ullRngStart = *optRngStart;
		}

		//"Search range end offset".
		if (m_editRngEnd.GetWindowTextLengthW() == 0) {
			ullRngEnd = ullHexDataSize - 1; //The last possible referensable offset.
		}
		else {
			CStringW wstrRngEnd;
			m_editRngEnd.GetWindowTextW(wstrRngEnd);
			const auto optRngEnd = stn::StrToUInt64(wstrRngEnd.GetString());
			if (!optRngEnd) {
				m_editRngEnd.SetFocus();
				MessageBoxW(L"Search range end offset is incorrect.", L"Incorrect offset", MB_ICONERROR);
				return;
			}

			ullRngEnd = *optRngEnd;
		}

		//"Start from".
		if (m_editStartFrom.GetWindowTextLengthW() == 0) {
			ullStartFrom = ullRngStart; //If empty field.
		}
		else {
			CStringW wstrStartFrom;
			m_editStartFrom.GetWindowTextW(wstrStartFrom);
			const auto optStartFrom = stn::StrToUInt64(wstrStartFrom.GetString());
			if (!optStartFrom) {
				m_editStartFrom.SetFocus();
				MessageBoxW(L"Start offset is incorrect.", L"Incorrect offset", MB_ICONERROR);
				return;
			}

			ullStartFrom = *optStartFrom;
		}
	}
	else {
		const auto vecSel = pHexCtrl->GetSelection();
		if (vecSel.empty()) //No selection.
			return;

		const auto& refSelFront = vecSel.front();
		ullRngStart = refSelFront.ullOffset;
		ullRngEnd = ullRngStart + refSelFront.ullSize - 1;
		assert(ullRngStart <= ullRngEnd);

		if (IsFreshSearch() || ullRngStart != GetRngStart() || ullRngEnd != GetRngEnd() || m_fAll) {
			ResetSearch();
			ullStartFrom = ullRngStart;
			m_fFreshSearch = false;
		}
		else {
			ullStartFrom = GetStartFrom();
		}
	}

	//"Range start/end" out of bounds check.
	if (ullRngStart > ullRngEnd || ullRngStart >= ullHexDataSize || ullRngEnd >= ullHexDataSize) {
		m_editRngEnd.SetFocus();
		MessageBoxW(L"Search range is out of data bounds.", L"Incorrect offset", MB_ICONERROR);
		return;
	}

	//"Start from" check to fit within the search range.
	if (ullStartFrom < ullRngStart || ullStartFrom > ullRngEnd) {
		m_editStartFrom.SetFocus();
		MessageBoxW(L"Start offset is not within the search range.", L"Incorrect offset", MB_ICONERROR);
		return;
	}

	//"Start from" plus search data size is beyond the search range end.
	if ((ullStartFrom + GetSearchDataSize()) > (ullRngEnd + 1)) {
		ResetSearch();
		return;
	}

	//If "Start from" edit box was changed we reset all the search.
	if (m_fSecondMatch && ullStartFrom != m_ullStartFrom) {
		ResetSearch();
	}

	//"Step".
	ULONGLONG ullStep;
	if (m_editStep.GetWindowTextLengthW() > 0) {
		CStringW wstrStep;
		m_editStep.GetWindowTextW(wstrStep);
		if (const auto optStep = stn::StrToUInt64(wstrStep.GetString()); optStep) {
			ullStep = *optStep;
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
	DWORD dwLimit;
	if (m_editLimit.GetWindowTextLengthW() > 0) {
		CStringW wstrLimit;
		m_editLimit.GetWindowTextW(wstrLimit);
		if (const auto optLimit = stn::StrToUInt32(wstrLimit.GetString()); optLimit) {
			dwLimit = *optLimit;
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

	if (IsReplace() && GetReplaceDataSize() > GetSearchDataSize()) {
		static constexpr auto wstrReplaceWarning { L"The replacing data is longer than search data.\r\n"
			"Do you want to overwrite bytes following search occurrence?\r\nChoosing No will cancel replace." };
		if (MessageBoxW(wstrReplaceWarning, L"Warning", MB_YESNO | MB_ICONQUESTION | MB_TOPMOST) == IDNO)
			return;
	}

	m_ullRngBegin = ullRngStart;
	m_ullRngEnd = ullRngEnd;
	m_ullStartFrom = ullStartFrom;
	m_ullStep = ullStep;
	m_dwLimit = dwLimit;

	Search();
	SetActiveWindow();
}

bool CHexDlgSearch::PrepareHexBytes()
{
	static constexpr auto pwszWrongInput { L"Unacceptable input character.\r\nAllowed characters are: 0123456789AaBbCcDdEeFf" };
	auto optData = NumStrToHex(m_wstrSearch, IsWildcard() ? static_cast<char>(m_uWildcard) : 0);
	if (!optData) {
		m_iWrap = 1;
		MessageBoxW(pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}

	m_vecSearchData = RangeToVecBytes(*optData);

	if (m_fReplace) {
		auto optDataRepl = NumStrToHex(m_wstrReplace);
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
	auto strSearch = WstrToStr(m_wstrSearch, CP_ACP); //Convert to the system default Windows ANSI code page.
	if (!IsMatchCase()) { //Make the string lowercase.
		std::transform(strSearch.begin(), strSearch.end(), strSearch.begin(),
			[](char ch) { return static_cast<char>(std::tolower(ch)); });
	}

	m_vecSearchData = RangeToVecBytes(strSearch);
	m_vecReplaceData = RangeToVecBytes(WstrToStr(m_wstrReplace, CP_ACP));

	return true;
}

bool CHexDlgSearch::PrepareTextUTF16()
{
	auto wstrSearch = m_wstrSearch;
	if (!IsMatchCase()) { //Make the string lowercase.
		std::transform(wstrSearch.begin(), wstrSearch.end(), wstrSearch.begin(),
			[](wchar_t wch) { return static_cast<wchar_t>(std::tolower(wch)); });
	}

	m_vecSearchData = RangeToVecBytes(wstrSearch);
	m_vecReplaceData = RangeToVecBytes(m_wstrReplace);

	return true;
}

bool CHexDlgSearch::PrepareTextUTF8()
{
	m_vecSearchData = RangeToVecBytes(WstrToStr(m_wstrSearch, CP_UTF8)); //Convert to UTF-8 string.
	m_vecReplaceData = RangeToVecBytes(WstrToStr(m_wstrReplace, CP_UTF8));

	return true;
}

template<typename T> requires TSize1248<T>
bool CHexDlgSearch::PrepareNumber()
{
	const auto optData = stn::StrToNum<T>(m_wstrSearch);
	if (!optData) {
		m_comboFind.SetFocus();
		MessageBoxW(m_pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}

	T tData = *optData;
	T tDataRep { };

	if (m_fReplace) {
		const auto optDataRepl = stn::StrToNum<T>(m_wstrReplace);
		if (!optDataRepl) {
			m_comboReplace.SetFocus();
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
	const auto optFTSearch = StringToFileTime(m_wstrSearch, dwFormat);
	if (!optFTSearch) {
		MessageBoxW(wstrErr.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}

	FILETIME ftSearch = *optFTSearch;
	FILETIME ftReplace { };

	if (m_fReplace) {
		const auto optFTReplace = StringToFileTime(m_wstrReplace, dwFormat);
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

void CHexDlgSearch::ReplaceAll()
{
	ClearList();
	m_dwCount = 0;
	m_dwReplaced = 0;

	if (IsSmallSearch()) {
		const auto pSearchFunc = GetSearchFunc(true, false);
		auto stFuncData = CreateSearchData();
		auto lmbWrapper = [&]()mutable->FINDRESULT {
			CalcMemChunks(stFuncData);
			return pSearchFunc(stFuncData);
			};

		const auto sSizeRepl = m_vecReplaceData.size();
		while (const auto findRes = lmbWrapper()) {
			if (findRes.ullOffset + sSizeRepl > GetSentinel())
				break;

			Replace(stFuncData.pHexCtrl, findRes.ullOffset, GetReplaceSpan());
			m_vecSearchRes.emplace_back(findRes.ullOffset); //Filling the vector of Found occurences.

			const auto ullNext = findRes.ullOffset + (sSizeRepl <= stFuncData.ullStep ?
				stFuncData.ullStep : sSizeRepl);
			if (ullNext > GetLastSearchOffset() || m_vecSearchRes.size() >= m_dwLimit) {
				break;
			}

			m_ullStartFrom = stFuncData.ullStartFrom = ullNext;
		}
	}
	else {
		const auto pSearchFunc = GetSearchFunc(true, true);
		CHexDlgCallback dlgClbk(L"Replacing...", L"Replaced: ", GetStartFrom(), GetLastSearchOffset(), this);
		const auto lmbReplaceAllThread = [&]() {
			auto stFuncData = CreateSearchData(&dlgClbk);
			auto lmbWrapper = [&]()mutable->FINDRESULT {
				CalcMemChunks(stFuncData);
				return pSearchFunc(stFuncData);
				};

			const auto sSizeRepl = m_vecReplaceData.size();
			while (const auto findRes = lmbWrapper()) {
				if (findRes.ullOffset + sSizeRepl > GetSentinel())
					break;

				Replace(stFuncData.pHexCtrl, findRes.ullOffset, m_vecReplaceData);
				m_vecSearchRes.emplace_back(findRes.ullOffset); //Filling the vector of Replaced occurences.
				dlgClbk.SetProgress(findRes.ullOffset);
				dlgClbk.SetCount(m_vecSearchRes.size());

				const auto ullNext = findRes.ullOffset + (sSizeRepl <= stFuncData.ullStep ?
					stFuncData.ullStep : sSizeRepl);
				if (ullNext > GetLastSearchOffset() || m_vecSearchRes.size() >= m_dwLimit
					|| dlgClbk.IsCanceled()) {
					break;
				}

				m_ullStartFrom = stFuncData.ullStartFrom = ullNext;
			}
			dlgClbk.OnCancel();
			};

		std::thread thrd(lmbReplaceAllThread);
		dlgClbk.DoModal();
		thrd.join();
	}

	if (!m_vecSearchRes.empty()) {
		m_fFound = true;
		m_dwCount = m_dwReplaced = static_cast<DWORD>(m_vecSearchRes.size());
	}
	m_pListMain->SetItemCountEx(static_cast<int>(m_vecSearchRes.size()));
}

void CHexDlgSearch::ResetSearch()
{
	m_fFreshSearch = true;
	m_ullStartFrom = { };
	m_dwCount = { };
	m_dwReplaced = { };
	m_iWrap = { };
	m_fSecondMatch = { false };
	m_fFound = { false };
	m_fDoCount = { true };
	ClearList();
	GetHexCtrl()->SetSelection({ }, false, true); //Clear selection highlight.
	GetDlgItem(IDC_HEXCTRL_SEARCH_STAT_RESULT)->SetWindowTextW(L"");
}

void CHexDlgSearch::Search()
{
	const auto pHexCtrl = GetHexCtrl();
	m_fFound = false;

	pHexCtrl->SetRedraw(false);
	if (m_fReplace) {
		if (m_fAll) { //Replace All
			ReplaceAll();
		}
		else if (IsForward()) { //Forward only.
			FindForward();
			if (m_fFound) {
				const auto sSizeRepl = m_vecReplaceData.size();
				if (GetStartFrom() + sSizeRepl <= GetSentinel()) {
					Replace(pHexCtrl, GetStartFrom(), GetReplaceSpan());
					++m_dwReplaced;
				}
			}
		}
	}
	else { //Search.
		if (m_fAll) {
			FindAll();
		}
		else {
			if (IsForward()) {
				m_ullStartFrom += m_fSecondMatch ? GetStep() : 0;
				FindForward();
			}
			else {
				FindBackward();
			}
		}
	}
	pHexCtrl->SetRedraw(true);

	std::wstring wstrInfo;
	if (m_fFound) {
		if (m_fAll) {
			if (m_fReplace) {
				wstrInfo = std::format(GetLocale(), L"{:L} occurrence(s) replaced.", m_dwReplaced);
				m_dwReplaced = 0;
				GetHexCtrl()->Redraw(); //Redraw in case of Replace all.
			}
			else {
				wstrInfo = std::format(GetLocale(), L"Found {:L} occurrences.", m_dwCount);
				m_dwCount = 0;
			}
		}
		else {
			if (m_fDoCount) {
				wstrInfo = std::format(GetLocale(), L"Found occurrence № {:L} from the beginning.", m_dwCount);
			}
			else {
				wstrInfo = L"Search found occurrence.";
			}

			auto ullStartFrom = GetStartFrom();
			HexCtrlHighlight({ { ullStartFrom, m_fReplace ? GetReplaceDataSize() : GetSearchDataSize() } });
			AddToList(ullStartFrom);

			if (m_fReplace) { //Increase next search step to replaced or m_ullStep amount.
				ullStartFrom += m_vecReplaceData.size() <= GetStep() ? GetStep() : m_vecReplaceData.size();
			}

			SetEditStartFrom(ullStartFrom);
		}
	}
	else {
		if (m_iWrap >= 0) { //==0 or ==1
			wstrInfo = L"Didn't find any occurrence, the end is reached.";
		}
		else if (m_iWrap == -1) {
			wstrInfo = L"Didn't find any occurrence, the begining is reached.";
		}

		ResetSearch();
	}
	GetDlgItem(IDC_HEXCTRL_SEARCH_STAT_RESULT)->SetWindowTextW(wstrInfo.data());

	if (!m_fSearchNext) {
		SetForegroundWindow();
		SetFocus();
	}
	else { m_fSearchNext = false; }
}

void CHexDlgSearch::SetControlsState()
{
	const auto pHexCtrl = GetHexCtrl();
	if (!pHexCtrl->IsCreated() || !pHexCtrl->IsDataSet())
		return;

	const auto fMutable = pHexCtrl->IsMutable();
	const auto fSearchEnabled = m_comboFind.GetWindowTextLengthW() > 0;
	const auto fReplaceEnabled = fMutable && fSearchEnabled && m_comboReplace.GetWindowTextLengthW() > 0;

	m_comboReplace.EnableWindow(fMutable);
	GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_SEARCHF)->EnableWindow(fSearchEnabled);
	GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_SEARCHB)->EnableWindow(fSearchEnabled);
	GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_FINDALL)->EnableWindow(fSearchEnabled);
	GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_REPL)->EnableWindow(fReplaceEnabled);
	GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_REPLALL)->EnableWindow(fReplaceEnabled);
}

void CHexDlgSearch::SetEditStartFrom(ULONGLONG ullOffset)
{
	m_editStartFrom.SetWindowTextW(std::format(L"0x{:X}", ullOffset).data());
}


//Static functions.

void CHexDlgSearch::Replace(IHexCtrl* pHexCtrl, ULONGLONG ullIndex, SpanCByte spnReplace)
{
	pHexCtrl->ModifyData({ .eModifyMode { EHexModifyMode::MODIFY_ONCE }, .spnData { spnReplace },
		.vecSpan { { ullIndex, spnReplace.size() } } });
}

int CHexDlgSearch::MemCmpVecEQByte1(const __m128i* pWhere, std::byte bWhat)
{
	const auto m128iWhere = _mm_loadu_si128(pWhere);
	const auto m128iWhat = _mm_set1_epi8(static_cast<char>(bWhat));
	const auto m128iResult = _mm_cmpeq_epi8(m128iWhere, m128iWhat);
	const auto uiMask = static_cast<std::uint32_t>(_mm_movemask_epi8(m128iResult));
	return std::countr_zero(uiMask); //>15 here means not found, all in mask are zeros.
}

int CHexDlgSearch::MemCmpVecNEQByte1(const __m128i* pWhere, std::byte bWhat)
{
	const auto m128iWhere = _mm_loadu_si128(pWhere);
	const auto m128iWhat = _mm_set1_epi8(static_cast<char>(bWhat));
	const auto m128iResult = _mm_cmpeq_epi8(m128iWhere, m128iWhat);
	const auto iMask = static_cast<std::uint32_t>(_mm_movemask_epi8(m128iResult));
	return std::countr_zero(~iMask);
}

int CHexDlgSearch::MemCmpVecEQByte2(const __m128i* pWhere, std::uint16_t ui16What)
{
	const auto m128iWhere0 = _mm_loadu_si128(pWhere);
	const auto m128iWhere1 = _mm_srli_si128(m128iWhere0, 1); //Shifting-right the whole vector by 1 byte.
	const auto m128iWhat = _mm_set1_epi16(ui16What);
	const auto m128iResult0 = _mm_cmpeq_epi16(m128iWhere0, m128iWhat);
	const auto m128iResult1 = _mm_cmpeq_epi16(m128iWhere1, m128iWhat); //Comparing both loads simultaneously.
	const auto uiMask0 = static_cast<std::uint32_t>(_mm_movemask_epi8(m128iResult0));
	//Shifting-left the mask by 1 to compensate previous 1 byte right-shift by _mm_srli_si128.
	const auto uiMask1 = static_cast<std::uint32_t>(_mm_movemask_epi8(m128iResult1)) << 1;
	const auto iRes0 = std::countr_zero(uiMask0);
	//Setting the last bit (of 16) to zero, to avoid false positives when searching for `0000` and last byte is `00`.
	const auto iRes1 = std::countr_zero(uiMask1 & 0b01111111'11111110);
	return (std::min)(iRes0, iRes1); //>15 here means not found, all in mask are zeros.
}

int CHexDlgSearch::MemCmpVecNEQByte2(const __m128i* pWhere, std::uint16_t ui16What)
{
	const auto m128iWhere0 = _mm_loadu_si128(pWhere);
	const auto m128iWhere1 = _mm_srli_si128(m128iWhere0, 1);
	const auto m128iWhat = _mm_set1_epi16(ui16What);
	const auto m128iResult0 = _mm_cmpeq_epi16(m128iWhere0, m128iWhat);
	const auto m128iResult1 = _mm_cmpeq_epi16(m128iWhere1, m128iWhat);
	const auto uiMask0 = static_cast<std::uint32_t>(_mm_movemask_epi8(m128iResult0));
	const auto uiMask1 = static_cast<std::uint32_t>(_mm_movemask_epi8(m128iResult1)) << 1;
	const auto iRes0 = std::countr_zero(~uiMask0);
	//Setting the first bit (of 16), after inverting, to zero, because it's always 0 after the left-shifting above.
	const auto iRes1 = std::countr_zero((~uiMask1) & 0b01111111'11111110);
	return (std::min)(iRes0, iRes1);
}

int CHexDlgSearch::MemCmpVecEQByte4(const __m128i* pWhere, std::uint32_t ui32What)
{
	const auto m128iWhere0 = _mm_loadu_si128(pWhere);
	const auto m128iWhere1 = _mm_srli_si128(m128iWhere0, 1);
	const auto m128iWhere2 = _mm_srli_si128(m128iWhere0, 2);
	const auto m128iWhere3 = _mm_srli_si128(m128iWhere0, 3);
	const auto m128iWhat = _mm_set1_epi32(ui32What);
	const auto m128iResult0 = _mm_cmpeq_epi32(m128iWhere0, m128iWhat);
	const auto m128iResult1 = _mm_cmpeq_epi32(m128iWhere1, m128iWhat);
	const auto m128iResult2 = _mm_cmpeq_epi32(m128iWhere2, m128iWhat);
	const auto m128iResult3 = _mm_cmpeq_epi32(m128iWhere3, m128iWhat);
	const auto uiMask0 = static_cast<std::uint32_t>(_mm_movemask_epi8(m128iResult0));
	const auto uiMask1 = static_cast<std::uint32_t>(_mm_movemask_epi8(m128iResult1)) << 1;
	const auto uiMask2 = static_cast<std::uint32_t>(_mm_movemask_epi8(m128iResult2)) << 2;
	const auto uiMask3 = static_cast<std::uint32_t>(_mm_movemask_epi8(m128iResult3)) << 3;
	const auto iRes0 = std::countr_zero(uiMask0);
	const auto iRes1 = std::countr_zero(uiMask1 & 0b00011111'11111110);
	const auto iRes2 = std::countr_zero(uiMask2 & 0b00111111'11111100);
	const auto iRes3 = std::countr_zero(uiMask3 & 0b01111111'11111000);
	return (std::min)(iRes0, (std::min)(iRes1, (std::min)(iRes2, iRes3)));
}

int CHexDlgSearch::MemCmpVecNEQByte4(const __m128i* pWhere, std::uint32_t ui32What)
{
	const auto m128iWhere0 = _mm_loadu_si128(pWhere);
	const auto m128iWhere1 = _mm_srli_si128(m128iWhere0, 1);
	const auto m128iWhere2 = _mm_srli_si128(m128iWhere0, 2);
	const auto m128iWhere3 = _mm_srli_si128(m128iWhere0, 3);
	const auto m128iWhat = _mm_set1_epi32(ui32What);
	const auto m128iResult0 = _mm_cmpeq_epi32(m128iWhere0, m128iWhat);
	const auto m128iResult1 = _mm_cmpeq_epi32(m128iWhere1, m128iWhat);
	const auto m128iResult2 = _mm_cmpeq_epi32(m128iWhere2, m128iWhat);
	const auto m128iResult3 = _mm_cmpeq_epi32(m128iWhere3, m128iWhat);
	const auto uiMask0 = static_cast<std::uint32_t>(_mm_movemask_epi8(m128iResult0));
	const auto uiMask1 = static_cast<std::uint32_t>(_mm_movemask_epi8(m128iResult1)) << 1;
	const auto uiMask2 = static_cast<std::uint32_t>(_mm_movemask_epi8(m128iResult2)) << 2;
	const auto uiMask3 = static_cast<std::uint32_t>(_mm_movemask_epi8(m128iResult3)) << 3;
	const auto iRes0 = std::countr_zero(~uiMask0);
	const auto iRes1 = std::countr_zero((~uiMask1) & 0b00011111'11111110);
	const auto iRes2 = std::countr_zero((~uiMask2) & 0b00111111'11111100);
	const auto iRes3 = std::countr_zero((~uiMask3) & 0b01111111'11111000);
	return (std::min)(iRes0, (std::min)(iRes1, (std::min)(iRes2, iRes3)));
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
auto CHexDlgSearch::SearchFuncFwd(const SEARCHFUNCDATA& refSearch)->FINDRESULT
{
	//Members locality is important for the best performance of the tight search loop below.
	static constexpr auto LOOP_UNROLL_SIZE = 8U; //How many comparisons we do at one loop cycle.
	const auto ullOffsetSentinel = refSearch.ullRngEnd + 1;
	const auto ullStep = refSearch.ullStep;
	const auto pHexCtrl = refSearch.pHexCtrl;
	const auto pDlgClbk = refSearch.pDlgClbk;
	const auto pDataSearch = refSearch.spnFind.data();
	const auto nSizeSearch = refSearch.spnFind.size();
	const auto ullEnd = ullOffsetSentinel - nSizeSearch;
	const auto fBigStep = refSearch.fBigStep;
	const auto fInverted = refSearch.fInverted;
	const auto ullMemChunks = refSearch.ullMemChunks;
	auto ullMemToAcquire = refSearch.ullMemToAcquire;
	auto ullChunkMaxOffset = refSearch.ullChunkMaxOffset;
	auto ullOffsetSearch = refSearch.ullStartFrom;

	for (auto itChunk = 0ULL; itChunk < ullMemChunks; ++itChunk) {
		const auto spnData = pHexCtrl->GetData({ ullOffsetSearch, ullMemToAcquire });
		assert(!spnData.empty());
		assert(spnData.size() >= ullMemToAcquire);

		//Unrolling the loop, making LOOP_UNROLL_SIZE comparisons at one cycle.
		for (auto ullOffsetData = 0ULL; ullOffsetData <= ullChunkMaxOffset; ullOffsetData += ullStep * LOOP_UNROLL_SIZE) {
			//First memory comparison is always unconditional.
			if (MemCmp<stType>(spnData.data() + ullOffsetData, pDataSearch, nSizeSearch) == !fInverted) {
				return { ullOffsetSearch + ullOffsetData, true, false };
			}

			//This branch allows to avoid the "ullOffsetData + ullStep * 'nstep' <= ullChunkMaxOffset" check every cycle.
			if ((ullOffsetData + ullStep * (LOOP_UNROLL_SIZE - 1)) <= ullChunkMaxOffset) {
				if (MemCmp<stType>(spnData.data() + ullOffsetData + ullStep * 1, pDataSearch, nSizeSearch) == !fInverted) {
					return { ullOffsetSearch + ullOffsetData + ullStep * 1, true, false };
				}
				if (MemCmp<stType>(spnData.data() + ullOffsetData + ullStep * 2, pDataSearch, nSizeSearch) == !fInverted) {
					return { ullOffsetSearch + ullOffsetData + ullStep * 2, true, false };
				}
				if (MemCmp<stType>(spnData.data() + ullOffsetData + ullStep * 3, pDataSearch, nSizeSearch) == !fInverted) {
					return { ullOffsetSearch + ullOffsetData + ullStep * 3, true, false };
				}
				if (MemCmp<stType>(spnData.data() + ullOffsetData + ullStep * 4, pDataSearch, nSizeSearch) == !fInverted) {
					return { ullOffsetSearch + ullOffsetData + ullStep * 4, true, false };
				}
				if (MemCmp<stType>(spnData.data() + ullOffsetData + ullStep * 5, pDataSearch, nSizeSearch) == !fInverted) {
					return { ullOffsetSearch + ullOffsetData + ullStep * 5, true, false };
				}
				if (MemCmp<stType>(spnData.data() + ullOffsetData + ullStep * 6, pDataSearch, nSizeSearch) == !fInverted) {
					return { ullOffsetSearch + ullOffsetData + ullStep * 6, true, false };
				}
				if (MemCmp<stType>(spnData.data() + ullOffsetData + ullStep * 7, pDataSearch, nSizeSearch) == !fInverted) {
					return { ullOffsetSearch + ullOffsetData + ullStep * 7, true, false };
				}
			}
			//In this branch we check every comparison for the ullChunkMaxOffset exceeding.
			//The one less comparison is needed here, otherwise we would've hit the branch above.
			else {
				if ((ullOffsetData + ullStep * 1 <= ullChunkMaxOffset)
					&& (MemCmp<stType>(spnData.data() + ullOffsetData + ullStep * 1, pDataSearch, nSizeSearch) == !fInverted)) {
					return { ullOffsetSearch + ullOffsetData + ullStep * 1, true, false };
				}
				if ((ullOffsetData + ullStep * 2 <= ullChunkMaxOffset)
					&& (MemCmp<stType>(spnData.data() + ullOffsetData + ullStep * 2, pDataSearch, nSizeSearch) == !fInverted)) {
					return { ullOffsetSearch + ullOffsetData + ullStep * 2, true, false };
				}
				if ((ullOffsetData + ullStep * 3 <= ullChunkMaxOffset)
					&& (MemCmp<stType>(spnData.data() + ullOffsetData + ullStep * 3, pDataSearch, nSizeSearch) == !fInverted)) {
					return { ullOffsetSearch + ullOffsetData + ullStep * 3, true, false };
				}
				if ((ullOffsetData + ullStep * 4 <= ullChunkMaxOffset)
					&& (MemCmp<stType>(spnData.data() + ullOffsetData + ullStep * 4, pDataSearch, nSizeSearch) == !fInverted)) {
					return { ullOffsetSearch + ullOffsetData + ullStep * 4, true, false };
				}
				if ((ullOffsetData + ullStep * 5 <= ullChunkMaxOffset)
					&& (MemCmp<stType>(spnData.data() + ullOffsetData + ullStep * 5, pDataSearch, nSizeSearch) == !fInverted)) {
					return { ullOffsetSearch + ullOffsetData + ullStep * 5, true, false };
				}
				if ((ullOffsetData + ullStep * 6 <= ullChunkMaxOffset)
					&& (MemCmp<stType>(spnData.data() + ullOffsetData + ullStep * 6, pDataSearch, nSizeSearch) == !fInverted)) {
					return { ullOffsetSearch + ullOffsetData + ullStep * 6, true, false };
				}
			}

			if constexpr (stType.fDlgClbck) {
				if (pDlgClbk->IsCanceled()) {
					return { { }, false, true };
				}
				pDlgClbk->SetProgress(ullOffsetSearch + ullOffsetData);
			}
		}

		if (fBigStep) [[unlikely]] {
			if ((ullOffsetSearch + ullStep) > ullEnd)
				break; //Upper bound reached.

			ullOffsetSearch += ullStep;
			}
		else {
			ullOffsetSearch += ullChunkMaxOffset;
		}

		if (ullOffsetSearch + ullMemToAcquire > ullOffsetSentinel) {
			ullMemToAcquire = ullOffsetSentinel - ullOffsetSearch;
			ullChunkMaxOffset = ullMemToAcquire - nSizeSearch;
		}
	}

	return { };
}

template<CHexDlgSearch::SEARCHTYPE stType>
auto CHexDlgSearch::SearchFuncFwdByte1(const SEARCHFUNCDATA& refSearch)->FINDRESULT
{
	//Members locality is important for the best performance of the tight search loop below.
	const auto ullOffsetSentinel = refSearch.ullRngEnd + 1;
	const auto ullStep = refSearch.ullStep;
	const auto pHexCtrl = refSearch.pHexCtrl;
	const auto pDlgClbk = refSearch.pDlgClbk;
	const auto pDataSearch = refSearch.spnFind.data();
	const auto nSizeSearch = refSearch.spnFind.size();
	const auto ullEnd = ullOffsetSentinel - nSizeSearch;
	const auto fBigStep = refSearch.fBigStep;
	const auto fInverted = refSearch.fInverted;
	const auto ullMemChunks = refSearch.ullMemChunks;
	auto ullMemToAcquire = refSearch.ullMemToAcquire;
	auto ullChunkMaxOffset = refSearch.ullChunkMaxOffset;
	auto ullOffsetSearch = refSearch.ullStartFrom;

	for (auto itChunk = 0ULL; itChunk < ullMemChunks; ++itChunk) {
		const auto spnData = pHexCtrl->GetData({ ullOffsetSearch, ullMemToAcquire });
		assert(!spnData.empty());
		assert(spnData.size() >= ullMemToAcquire);

		for (auto ullOffsetData = 0ULL; ullOffsetData <= ullChunkMaxOffset; ullOffsetData += sizeof(__m128i)) {
			if ((ullOffsetData + sizeof(__m128i)) <= ullChunkMaxOffset) {
				if constexpr (!stType.fInverted) { //SIMD forward not inverted.
					if (const auto iRes = MemCmpVecEQByte1(
						reinterpret_cast<const __m128i*>(spnData.data() + ullOffsetData), *pDataSearch);
						iRes < sizeof(__m128i)) {
						return { ullOffsetSearch + ullOffsetData + iRes, true, false };
					}
				}
				else if constexpr (stType.fInverted) { //SIMD forward inverted.
					if (const auto iRes = MemCmpVecNEQByte1(
						reinterpret_cast<const __m128i*>(spnData.data() + ullOffsetData), *pDataSearch);
						iRes < sizeof(__m128i)) {
						return { ullOffsetSearch + ullOffsetData + iRes, true, false };
					}
				}
			}
			else {
				for (auto i = 0; i <= ullChunkMaxOffset - ullOffsetData; ++i) {
					if (MemCmp<stType>(spnData.data() + ullOffsetData + i, pDataSearch, 0) == !fInverted) {
						return { ullOffsetSearch + ullOffsetData + i, true, false };
					}
				}
			}

			if constexpr (stType.fDlgClbck) {
				if (pDlgClbk->IsCanceled()) {
					return { { }, false, true };
				}
				pDlgClbk->SetProgress(ullOffsetSearch + ullOffsetData);
			}
		}

		if (fBigStep) [[unlikely]] {
			if ((ullOffsetSearch + ullStep) > ullEnd)
				break; //Upper bound reached.

			ullOffsetSearch += ullStep;
			}
		else {
			ullOffsetSearch += ullChunkMaxOffset;
		}

		if (ullOffsetSearch + ullMemToAcquire > ullOffsetSentinel) {
			ullMemToAcquire = ullOffsetSentinel - ullOffsetSearch;
			ullChunkMaxOffset = ullMemToAcquire - nSizeSearch;
		}
	}

	return { };
}

template<CHexDlgSearch::SEARCHTYPE stType>
auto CHexDlgSearch::SearchFuncFwdByte2(const SEARCHFUNCDATA& refSearch)->FINDRESULT
{
	//Members locality is important for the best performance of the tight search loop below.
	const auto ullOffsetSentinel = refSearch.ullRngEnd + 1;
	const auto ullStep = refSearch.ullStep;
	const auto pHexCtrl = refSearch.pHexCtrl;
	const auto pDlgClbk = refSearch.pDlgClbk;
	const auto pDataSearch = refSearch.spnFind.data();
	const auto nSizeSearch = refSearch.spnFind.size();
	const auto ullEnd = ullOffsetSentinel - nSizeSearch;
	const auto fBigStep = refSearch.fBigStep;
	const auto fInverted = refSearch.fInverted;
	const auto ullMemChunks = refSearch.ullMemChunks;
	auto ullMemToAcquire = refSearch.ullMemToAcquire;
	auto ullChunkMaxOffset = refSearch.ullChunkMaxOffset;
	auto ullOffsetSearch = refSearch.ullStartFrom;

	for (auto itChunk = 0ULL; itChunk < ullMemChunks; ++itChunk) {
		const auto spnData = pHexCtrl->GetData({ ullOffsetSearch, ullMemToAcquire });
		assert(!spnData.empty());
		assert(spnData.size() >= ullMemToAcquire);

		//Next cycle offset is "sizeof(__m128i) - 1", to get the data that crosses the vector.
		for (auto ullOffsetData = 0ULL; ullOffsetData <= ullChunkMaxOffset; ullOffsetData += (sizeof(__m128i) - 1)) {
			if ((ullOffsetData + sizeof(__m128i)) <= ullChunkMaxOffset) {
				if constexpr (!stType.fInverted) { //SIMD forward not inverted.
					if (const auto iRes = MemCmpVecEQByte2(
						reinterpret_cast<const __m128i*>(spnData.data() + ullOffsetData),
						*reinterpret_cast<const std::uint16_t*>(pDataSearch));
						iRes < sizeof(__m128i)) {
						return { ullOffsetSearch + ullOffsetData + iRes, true, false };
					}
				}
				else if constexpr (stType.fInverted) { //SIMD forward inverted.
					if (const auto iRes = MemCmpVecNEQByte2(
						reinterpret_cast<const __m128i*>(spnData.data() + ullOffsetData),
						*reinterpret_cast<const std::uint16_t*>(pDataSearch));
						iRes < sizeof(__m128i)) {
						return { ullOffsetSearch + ullOffsetData + iRes, true, false };
					}
				}
			}
			else {
				for (auto i = 0; i <= ullChunkMaxOffset - ullOffsetData; ++i) {
					if (MemCmp<stType>(spnData.data() + ullOffsetData + i, pDataSearch, 0) == !fInverted) {
						return { ullOffsetSearch + ullOffsetData + i, true, false };
					}
				}
			}

			if constexpr (stType.fDlgClbck) {
				if (pDlgClbk->IsCanceled()) {
					return { { }, false, true };
				}
				pDlgClbk->SetProgress(ullOffsetSearch + ullOffsetData);
			}
		}

		if (fBigStep) [[unlikely]] {
			if ((ullOffsetSearch + ullStep) > ullEnd)
				break; //Upper bound reached.

			ullOffsetSearch += ullStep;
			}
		else {
			ullOffsetSearch += ullChunkMaxOffset;
		}

		if (ullOffsetSearch + ullMemToAcquire > ullOffsetSentinel) {
			ullMemToAcquire = ullOffsetSentinel - ullOffsetSearch;
			ullChunkMaxOffset = ullMemToAcquire - nSizeSearch;
		}
	}

	return { };
}

template<CHexDlgSearch::SEARCHTYPE stType>
auto CHexDlgSearch::SearchFuncFwdByte4(const SEARCHFUNCDATA& refSearch)->FINDRESULT
{
	//Members locality is important for the best performance of the tight search loop below.
	const auto ullOffsetSentinel = refSearch.ullRngEnd + 1;
	const auto ullStep = refSearch.ullStep;
	const auto pHexCtrl = refSearch.pHexCtrl;
	const auto pDlgClbk = refSearch.pDlgClbk;
	const auto pDataSearch = refSearch.spnFind.data();
	const auto nSizeSearch = refSearch.spnFind.size();
	const auto ullEnd = ullOffsetSentinel - nSizeSearch;
	const auto fBigStep = refSearch.fBigStep;
	const auto fInverted = refSearch.fInverted;
	const auto ullMemChunks = refSearch.ullMemChunks;
	auto ullMemToAcquire = refSearch.ullMemToAcquire;
	auto ullChunkMaxOffset = refSearch.ullChunkMaxOffset;
	auto ullOffsetSearch = refSearch.ullStartFrom;

	for (auto itChunk = 0ULL; itChunk < ullMemChunks; ++itChunk) {
		const auto spnData = pHexCtrl->GetData({ ullOffsetSearch, ullMemToAcquire });
		assert(!spnData.empty());
		assert(spnData.size() >= ullMemToAcquire);

		//Next cycle offset is "sizeof(__m128i) - 3", to get the data that crosses the vector.
		for (auto ullOffsetData = 0ULL; ullOffsetData <= ullChunkMaxOffset; ullOffsetData += (sizeof(__m128i) - 3)) {
			if ((ullOffsetData + sizeof(__m128i)) <= ullChunkMaxOffset) {
				if constexpr (!stType.fInverted) { //SIMD forward not inverted.
					if (const auto iRes = MemCmpVecEQByte4(
						reinterpret_cast<const __m128i*>(spnData.data() + ullOffsetData),
						*reinterpret_cast<const std::uint32_t*>(pDataSearch));
						iRes < sizeof(__m128i)) {
						return { ullOffsetSearch + ullOffsetData + iRes, true, false };
					}
				}
				else if constexpr (stType.fInverted) { //SIMD forward inverted.
					if (const auto iRes = MemCmpVecNEQByte4(
						reinterpret_cast<const __m128i*>(spnData.data() + ullOffsetData),
						*reinterpret_cast<const std::uint32_t*>(pDataSearch));
						iRes < sizeof(__m128i)) {
						return { ullOffsetSearch + ullOffsetData + iRes, true, false };
					}
				}
			}
			else {
				for (auto i = 0; i <= ullChunkMaxOffset - ullOffsetData; ++i) {
					if (MemCmp<stType>(spnData.data() + ullOffsetData + i, pDataSearch, 0) == !fInverted) {
						return { ullOffsetSearch + ullOffsetData + i, true, false };
					}
				}
			}

			if constexpr (stType.fDlgClbck) {
				if (pDlgClbk->IsCanceled()) {
					return { { }, false, true };
				}
				pDlgClbk->SetProgress(ullOffsetSearch + ullOffsetData);
			}
		}

		if (fBigStep) [[unlikely]] {
			if ((ullOffsetSearch + ullStep) > ullEnd)
				break; //Upper bound reached.

			ullOffsetSearch += ullStep;
			}
		else {
			ullOffsetSearch += ullChunkMaxOffset;
		}

		if (ullOffsetSearch + ullMemToAcquire > ullOffsetSentinel) {
			ullMemToAcquire = ullOffsetSentinel - ullOffsetSearch;
			ullChunkMaxOffset = ullMemToAcquire - nSizeSearch;
		}
	}

	return { };
}

template<CHexDlgSearch::SEARCHTYPE stType>
auto CHexDlgSearch::SearchFuncBack(const SEARCHFUNCDATA& refSearch)->FINDRESULT
{
	//Members locality is important for the best performance of the tight search loop below.
	static constexpr auto LOOP_UNROLL_SIZE = 8U; //How many comparisons we do at one loop cycle.
	const auto ullStartFrom = refSearch.ullStartFrom;
	const auto ullEnd = refSearch.ullRngStart;
	//Step is signed here to make the "llOffsetData - llStep" arithmetic also signed.
	const auto llStep = static_cast<std::int64_t>(refSearch.ullStep);
	const auto pHexCtrl = refSearch.pHexCtrl;
	const auto pDlgClbk = refSearch.pDlgClbk;
	const auto pDataSearch = refSearch.spnFind.data();
	const auto nSizeSearch = refSearch.spnFind.size();
	const auto fBigStep = refSearch.fBigStep;
	const auto fInverted = refSearch.fInverted;
	const auto ullMemChunks = refSearch.ullMemChunks;
	auto ullMemToAcquire = refSearch.ullMemToAcquire;
	auto ullChunkMaxOffset = refSearch.ullChunkMaxOffset;
	auto ullOffsetSearch = refSearch.ullStartFrom - refSearch.ullChunkMaxOffset;

	if (ullOffsetSearch < ullEnd || ullOffsetSearch >((std::numeric_limits<ULONGLONG>::max)() - ullChunkMaxOffset)) {
		ullMemToAcquire = (ullStartFrom - ullEnd) + nSizeSearch;
		ullChunkMaxOffset = ullMemToAcquire - nSizeSearch;
		ullOffsetSearch = ullEnd;
	}

	for (auto itChunk = ullMemChunks; itChunk > 0; --itChunk) {
		const auto spnData = pHexCtrl->GetData({ ullOffsetSearch, ullMemToAcquire });
		assert(!spnData.empty());
		assert(spnData.size() >= ullMemToAcquire);

		for (auto llOffsetData = static_cast<LONGLONG>(ullChunkMaxOffset); llOffsetData >= 0;
			llOffsetData -= llStep * LOOP_UNROLL_SIZE) { //llOffsetData might be negative.
			//First memory comparison is always unconditional.
			if (MemCmp<stType>(spnData.data() + llOffsetData, pDataSearch, nSizeSearch) == !fInverted) {
				return { ullOffsetSearch + llOffsetData, true, false };
			}

			if ((llOffsetData - llStep * (LOOP_UNROLL_SIZE - 1)) >= 0) {
				if (MemCmp<stType>(spnData.data() + (llOffsetData - llStep * 1), pDataSearch, nSizeSearch) == !fInverted) {
					return { ullOffsetSearch + (llOffsetData - llStep * 1), true, false };
				}
				if (MemCmp<stType>(spnData.data() + (llOffsetData - llStep * 2), pDataSearch, nSizeSearch) == !fInverted) {
					return { ullOffsetSearch + (llOffsetData - llStep * 2), true, false };
				}
				if (MemCmp<stType>(spnData.data() + (llOffsetData - llStep * 3), pDataSearch, nSizeSearch) == !fInverted) {
					return { ullOffsetSearch + (llOffsetData - llStep * 3), true, false };
				}
				if (MemCmp<stType>(spnData.data() + (llOffsetData - llStep * 4), pDataSearch, nSizeSearch) == !fInverted) {
					return { ullOffsetSearch + (llOffsetData - llStep * 4), true, false };
				}
				if (MemCmp<stType>(spnData.data() + (llOffsetData - llStep * 5), pDataSearch, nSizeSearch) == !fInverted) {
					return { ullOffsetSearch + (llOffsetData - llStep * 5), true, false };
				}
				if (MemCmp<stType>(spnData.data() + (llOffsetData - llStep * 6), pDataSearch, nSizeSearch) == !fInverted) {
					return { ullOffsetSearch + (llOffsetData - llStep * 6), true, false };
				}
				if (MemCmp<stType>(spnData.data() + (llOffsetData - llStep * 7), pDataSearch, nSizeSearch) == !fInverted) {
					return { ullOffsetSearch + (llOffsetData - llStep * 7), true, false };
				}
			}
			else {
				if ((llOffsetData - llStep * 1 >= 0)
					&& (MemCmp<stType>(spnData.data() + (llOffsetData - llStep * 1), pDataSearch, nSizeSearch) == !fInverted)) {
					return { ullOffsetSearch + (llOffsetData - llStep * 1), true, false };
				}
				if ((llOffsetData - llStep * 2 >= 0)
					&& (MemCmp<stType>(spnData.data() + (llOffsetData - llStep * 2), pDataSearch, nSizeSearch) == !fInverted)) {
					return { ullOffsetSearch + (llOffsetData - llStep * 2), true, false };
				}
				if ((llOffsetData - llStep * 3 >= 0)
					&& (MemCmp<stType>(spnData.data() + (llOffsetData - llStep * 3), pDataSearch, nSizeSearch) == !fInverted)) {
					return { ullOffsetSearch + (llOffsetData - llStep * 3), true, false };
				}
				if ((llOffsetData - llStep * 4 >= 0)
					&& (MemCmp<stType>(spnData.data() + (llOffsetData - llStep * 4), pDataSearch, nSizeSearch) == !fInverted)) {
					return { ullOffsetSearch + (llOffsetData - llStep * 4), true, false };
				}
				if ((llOffsetData - llStep * 5 >= 0)
					&& (MemCmp<stType>(spnData.data() + (llOffsetData - llStep * 5), pDataSearch, nSizeSearch) == !fInverted)) {
					return { ullOffsetSearch + (llOffsetData - llStep * 5), true, false };
				}
				if ((llOffsetData - llStep * 6 >= 0)
					&& (MemCmp<stType>(spnData.data() + (llOffsetData - llStep * 6), pDataSearch, nSizeSearch) == !fInverted)) {
					return { ullOffsetSearch + (llOffsetData - llStep * 6), true, false };
				}
			}

			if constexpr (stType.fDlgClbck) {
				if (pDlgClbk->IsCanceled()) {
					return { { }, false, true };
				}
				pDlgClbk->SetProgress(ullStartFrom - (ullOffsetSearch + llOffsetData));
			}
		}

		if (fBigStep) [[unlikely]] {
			if ((ullOffsetSearch - llStep) < ullEnd || (ullOffsetSearch - llStep) > ((std::numeric_limits<ULONGLONG>::max)() - llStep))
				break; //Lower bound reached.

			ullOffsetSearch -= ullChunkMaxOffset;
			}
		else {
			if ((ullOffsetSearch - ullChunkMaxOffset) < ullEnd || ((ullOffsetSearch - ullChunkMaxOffset) >
				((std::numeric_limits<ULONGLONG>::max)() - ullChunkMaxOffset))) {
				ullMemToAcquire = (ullOffsetSearch - ullEnd) + nSizeSearch;
				ullChunkMaxOffset = ullMemToAcquire - nSizeSearch;
				ullOffsetSearch = ullEnd;
			}
			else {
				ullOffsetSearch -= ullChunkMaxOffset;
			}
		}
	}

	return { };
}