/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "../../res/HexCtrlRes.h"
#include "CHexDlgSearch.h"
#include <algorithm>
#include <cassert>
#include <commctrl.h>
#include <cwctype>
#include <format>
#include <intrin.h>
#include <limits>
#include <string>
#include <thread>
#include <type_traits>

using namespace HEXCTRL::INTERNAL;

enum class CHexDlgSearch::ESearchMode : std::uint8_t {
	MODE_HEXBYTES, MODE_TEXT, MODE_NUMBERS, MODE_STRUCT
};

enum class CHexDlgSearch::ESearchType : std::uint8_t {
	HEXBYTES, TEXT_ASCII, TEXT_UTF8, TEXT_UTF16, NUM_INT8, NUM_UINT8, NUM_INT16, NUM_UINT16,
	NUM_INT32, NUM_UINT32, NUM_INT64, NUM_UINT64, NUM_FLOAT, NUM_DOUBLE, STRUCT_FILETIME
};

enum class CHexDlgSearch::EMenuID : std::uint16_t {
	IDM_SEARCH_ADDBKM = 0x8000, IDM_SEARCH_SELECTALL, IDM_SEARCH_CLEARALL
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
	ULONGLONG ullChunks { };         //How many memory chunks to search in.
	ULONGLONG ullChunkSize { };      //Size of one chunk.
	ULONGLONG ullChunkMaxOffset { }; //Maximum offset to start search from, in the chunk.
	CHexDlgProgress* pDlgProg { };
	IHexCtrl* pHexCtrl { };
	SpanCByte spnFind;
	bool fBigStep { };
	bool fInverted { };
};

void CHexDlgSearch::ClearData()
{
	if (!m_Wnd.IsWindow())
		return;

	ClearList();
}

void CHexDlgSearch::CreateDlg()
{
	//m_Wnd is set in the OnInitDialog().
	if (const auto hWnd = ::CreateDialogParamW(m_hInstRes, MAKEINTRESOURCEW(IDD_HEXCTRL_SEARCH),
		m_pHexCtrl->GetWndHandle(EHexWnd::WND_MAIN), wnd::DlgProc<CHexDlgSearch>, reinterpret_cast<LPARAM>(this));
		hWnd == nullptr) {
		ut::DBG_REPORT(L"CreateDialogParamW failed.");
	}
}

void CHexDlgSearch::DestroyDlg()
{
	if (m_Wnd.IsWindow()) {
		m_Wnd.DestroyWindow();
	}
}

auto CHexDlgSearch::GetDlgItemHandle(EHexDlgItem eItem)const->HWND
{
	if (!m_Wnd.IsWindow()) {
		return { };
	}

	using enum EHexDlgItem;
	switch (eItem) {
	case SEARCH_COMBO_FIND:
		return m_WndCmbFind;
	case SEARCH_COMBO_REPLACE:
		return m_WndCmbReplace;
	case SEARCH_EDIT_START:
		return m_WndEditStart;
	case SEARCH_EDIT_STEP:
		return m_WndEditStep;
	case SEARCH_EDIT_RNGBEG:
		return m_WndEditRngBegin;
	case SEARCH_EDIT_RNGEND:
		return m_WndEditRngEnd;
	case SEARCH_EDIT_LIMIT:
		return m_WndEditLimit;
	default:
		return { };
	}
}

auto CHexDlgSearch::GetHWND()const->HWND
{
	return m_Wnd;
}

void CHexDlgSearch::Initialize(IHexCtrl* pHexCtrl, HINSTANCE hInstRes)
{
	if (pHexCtrl == nullptr || hInstRes == nullptr) {
		ut::DBG_REPORT(L"Initialize == nullptr");
		return;
	}

	m_pHexCtrl = pHexCtrl;
	m_hInstRes = hInstRes;
}

bool CHexDlgSearch::IsSearchAvail()const
{
	return m_Wnd.IsWindow() && GetHexCtrl()->IsDataSet();
}

bool CHexDlgSearch::PreTranslateMsg(MSG* pMsg)
{
	return m_Wnd.IsDlgMessage(pMsg);
}

auto CHexDlgSearch::ProcessMsg(const MSG& msg)->INT_PTR
{
	switch (msg.message) {
	case WM_ACTIVATE: return OnActivate(msg);
	case WM_CLOSE: return OnClose();
	case WM_COMMAND: return OnCommand(msg);
	case WM_CTLCOLORSTATIC: return OnCtlClrStatic(msg);
	case WM_DESTROY: return OnDestroy();
	case WM_DRAWITEM: return OnDrawItem(msg);
	case WM_INITDIALOG: return OnInitDialog(msg);
	case WM_MEASUREITEM: return OnMeasureItem(msg);
	case WM_NOTIFY: return OnNotify(msg);
	default:
		return 0;
	}
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

void CHexDlgSearch::ShowWindow(int iCmdShow)
{
	if (!m_Wnd.IsWindow()) {
		CreateDlg();
	}

	m_Wnd.ShowWindow(iCmdShow);
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
			m_ListEx.SetItemCountEx(iHighlight--);
		}
	}
	else {
		iHighlight = static_cast<int>(iter - m_vecSearchRes.begin());
	}

	if (iHighlight != -1) {
		m_ListEx.SetItemState(-1, 0, LVIS_SELECTED);
		m_ListEx.SetItemState(iHighlight, LVIS_SELECTED, LVIS_SELECTED);
		m_ListEx.EnsureVisible(iHighlight, TRUE);
	}
}

void CHexDlgSearch::CalcMemChunks(SEARCHFUNCDATA& refData)const
{
	const auto nSizeSearch = refData.spnFind.size();
	if (refData.ullStartFrom + nSizeSearch > GetSentinel()) {
		refData.ullChunks = { };
		refData.ullChunkSize = { };
		refData.ullChunkMaxOffset = { };
		return;
	}

	const auto ullSizeTotal = GetSearchRngSize(); //Depends on search direction.
	const auto pHexCtrl = refData.pHexCtrl;
	const auto ullStep = refData.ullStep;
	ULONGLONG ullChunks;
	ULONGLONG ullChunkSize;
	ULONGLONG ullChunkMaxOffset;
	bool fBigStep { false };

	if (!pHexCtrl->IsVirtual()) {
		ullChunks = 1;
		ullChunkSize = ullSizeTotal;
		ullChunkMaxOffset = ullSizeTotal - nSizeSearch;
	}
	else {
		ullChunkSize = (std::min)(static_cast<ULONGLONG>(pHexCtrl->GetCacheSize()), ullSizeTotal);
		ullChunkMaxOffset = ullChunkSize - nSizeSearch;
		if (ullStep > ullChunkMaxOffset) { //For very big steps.
			ullChunks = ullSizeTotal > ullStep ? (ullSizeTotal / ullStep) + ((ullSizeTotal % ullStep) ? 1 : 0) : 1;
			fBigStep = true;
		}
		else {
			ullChunks = ullSizeTotal > ullChunkMaxOffset ? (ullSizeTotal / ullChunkMaxOffset)
				+ ((ullSizeTotal % ullChunkMaxOffset) ? 1 : 0) : 1;
		}
	}

	refData.ullChunks = ullChunks;
	refData.ullChunkSize = ullChunkSize;
	refData.ullChunkMaxOffset = ullChunkMaxOffset;
	refData.fBigStep = fBigStep;
}

void CHexDlgSearch::ClearComboType()
{
	m_WndCmbType.SetRedraw(FALSE);
	for (auto iIndex = m_WndCmbType.GetCount() - 1; iIndex >= 0; --iIndex) {
		m_WndCmbType.DeleteString(iIndex);
	}
	m_WndCmbType.SetRedraw(TRUE);
}

void CHexDlgSearch::ClearList()
{
	m_ListEx.SetItemCountEx(0);
	m_vecSearchRes.clear();
}

void CHexDlgSearch::ComboSearchFill(LPCWSTR pwsz)
{
	//Insert text into ComboBox only if it's not already there.
	if (m_WndCmbFind.FindStringExact(0, pwsz) == CB_ERR) {
		//Keep max 50 strings in list.
		if (m_WndCmbFind.GetCount() == 50) {
			m_WndCmbFind.DeleteString(49);
		}
		m_WndCmbFind.InsertString(0, pwsz);
	}
}

void CHexDlgSearch::ComboReplaceFill(LPCWSTR pwsz)
{
	//Insert wstring into ComboBox only if it's not already presented.
	if (m_WndCmbReplace.FindStringExact(0, pwsz) == CB_ERR) {
		//Keep max 50 strings in list.
		if (m_WndCmbReplace.GetCount() == 50) {
			m_WndCmbReplace.DeleteString(49);
		}
		m_WndCmbReplace.InsertString(0, pwsz);
	}
}

auto CHexDlgSearch::CreateSearchData(CHexDlgProgress* pDlgProg)const->SEARCHFUNCDATA
{
	SEARCHFUNCDATA stData { .ullStartFrom { GetStartFrom() }, .ullRngStart { GetRngStart() },
		.ullRngEnd { GetRngEnd() }, .ullStep { GetStep() }, .pDlgProg { pDlgProg }, .pHexCtrl { GetHexCtrl() },
		.spnFind { GetSearchSpan() }, .fInverted { IsInverted() }
	};

	CalcMemChunks(stData);

	return stData;
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
		CHexDlgProgress dlgProg(L"Searching...", L"Found: ", GetStartFrom(), GetLastSearchOffset());
		stFuncData.pDlgProg = &dlgProg;
		const auto lmbFindAllThread = [&]() {
			auto lmbWrapper = [&]()mutable->FINDRESULT {
				CalcMemChunks(stFuncData);
				return pSearchFunc(stFuncData);
				};

			while (const auto findRes = lmbWrapper()) {
				m_vecSearchRes.emplace_back(findRes.ullOffset); //Filling the vector of Found occurences.
				dlgProg.SetCurrent(findRes.ullOffset);
				dlgProg.SetCount(m_vecSearchRes.size());

				const auto ullNext = findRes.ullOffset + GetStep();
				if (ullNext > GetLastSearchOffset() || m_vecSearchRes.size() >= m_dwLimit
					|| dlgProg.IsCanceled())
					break;

				m_ullStartFrom = stFuncData.ullStartFrom = ullNext;
			}
			dlgProg.OnCancel();
			};
		std::thread thrd(lmbFindAllThread);
		dlgProg.DoModal(m_Wnd, m_hInstRes);
		thrd.join();
	}

	if (!m_vecSearchRes.empty()) {
		m_fFound = true;
		m_dwCount = static_cast<DWORD>(m_vecSearchRes.size());
	}

	m_ListEx.SetItemCountEx(static_cast<int>(m_vecSearchRes.size()));
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
			CHexDlgProgress dlgProg(L"Searching...", L"", GetStartFrom(), GetLastSearchOffset());
			stFuncData.pDlgProg = &dlgProg;
			const auto lmbWrapper = [&]() {
				findRes = pSearchFunc(stFuncData);
				dlgProg.OnCancel();
				};
			std::thread thrd(lmbWrapper);
			dlgProg.DoModal(m_Wnd, m_hInstRes);
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
			CHexDlgProgress dlgProg(L"Searching...", L"", GetRngStart(), GetStartFrom());
			stFuncData.pDlgProg = &dlgProg;
			const auto lmbWrapper = [&]() {
				findRes = pSearchFunc(stFuncData);
				dlgProg.OnCancel();
				};
			std::thread thrd(lmbWrapper);
			dlgProg.DoModal(m_Wnd, m_hInstRes);
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

auto CHexDlgSearch::GetSearchFunc(bool fFwd, bool fDlgProg)const->PtrSearchFunc
{
	using enum EVecSize;
#if defined(_M_IX86) || defined(_M_X64)
	if (ut::HasAVX2()) {
		return fFwd ? (fDlgProg ? GetSearchFuncFwd<true, VEC256>() : GetSearchFuncFwd<false, VEC256>()) :
			(fDlgProg ? GetSearchFuncBack<true, VEC256>() : GetSearchFuncBack<false, VEC256>());
	}
#endif
	//For ARM64 and VEC128 path is the same.
	return fFwd ? (fDlgProg ? GetSearchFuncFwd<true, VEC128>() : GetSearchFuncFwd<false, VEC128>()) :
		(fDlgProg ? GetSearchFuncBack<true, VEC128>() : GetSearchFuncBack<false, VEC128>());
}

template<bool fDlgProg, CHexDlgSearch::EVecSize eVecSize>
auto CHexDlgSearch::GetSearchFuncFwd()const->PtrSearchFunc
{
	//The `fDlgProg` arg ensures that no runtime check will be performed for the 
	//'SEARCHFUNCDATA::pDlgProg == nullptr', at the hot path inside the SearchFunc function.

	using enum ESearchType; using enum EMemCmp;

	if (GetStep() == 1 && !IsWildcard()) {
		switch (GetSearchDataSize()) {
		case 1: //Special case for 1 byte data size SIMD.
			return IsInverted() ?
				SearchFuncVecFwdByte1<SEARCHTYPE(DATA_BYTE1, eVecSize, fDlgProg, false, false, true)> :
				SearchFuncVecFwdByte1<SEARCHTYPE(DATA_BYTE1, eVecSize, fDlgProg, false, false, false)>;
		case 2: //Special case for 2 bytes data size SIMD.
			return IsInverted() ?
				SearchFuncVecFwdByte2<SEARCHTYPE(DATA_BYTE2, eVecSize, fDlgProg, false, false, true)> :
				SearchFuncVecFwdByte2<SEARCHTYPE(DATA_BYTE2, eVecSize, fDlgProg, false, false, false)>;
		case 4: //Special case for 4 bytes data size SIMD.
			return IsInverted() ?
				SearchFuncVecFwdByte4<SEARCHTYPE(DATA_BYTE4, eVecSize, fDlgProg, false, false, true)> :
				SearchFuncVecFwdByte4<SEARCHTYPE(DATA_BYTE4, eVecSize, fDlgProg, false, false, false)>;
		default:
			break;
		};
	}

	switch (GetSearchType()) {
	case HEXBYTES:
		return IsWildcard() ?
			SearchFuncFwd<SEARCHTYPE(CHAR_STR, eVecSize, fDlgProg, true, true)> :
			SearchFuncFwd<SEARCHTYPE(CHAR_STR, eVecSize, fDlgProg, true, false)>;
	case TEXT_ASCII:
		if (IsMatchCase() && !IsWildcard()) {
			return SearchFuncFwd<SEARCHTYPE(CHAR_STR, eVecSize, fDlgProg, true, false)>;
		}

		if (IsMatchCase() && IsWildcard()) {
			return SearchFuncFwd<SEARCHTYPE(CHAR_STR, eVecSize, fDlgProg, true, true)>;
		}

		if (!IsMatchCase() && IsWildcard()) {
			return SearchFuncFwd<SEARCHTYPE(CHAR_STR, eVecSize, fDlgProg, false, true)>;
		}

		if (!IsMatchCase() && !IsWildcard()) {
			return SearchFuncFwd<SEARCHTYPE(CHAR_STR, eVecSize, fDlgProg, false, false)>;
		}
		break;
	case TEXT_UTF8:
		return SearchFuncFwd<SEARCHTYPE(CHAR_STR, eVecSize, fDlgProg, true, false)>; //Search UTF-8 as plain chars.
	case TEXT_UTF16:
		if (IsMatchCase() && !IsWildcard()) {
			return SearchFuncFwd<SEARCHTYPE(WCHAR_STR, eVecSize, fDlgProg, true, false)>;
		}

		if (IsMatchCase() && IsWildcard()) {
			return SearchFuncFwd<SEARCHTYPE(WCHAR_STR, eVecSize, fDlgProg, true, true)>;
		}

		if (!IsMatchCase() && IsWildcard()) {
			return SearchFuncFwd<SEARCHTYPE(WCHAR_STR, eVecSize, fDlgProg, false, true)>;
		}

		if (!IsMatchCase() && !IsWildcard()) {
			return SearchFuncFwd<SEARCHTYPE(WCHAR_STR, eVecSize, fDlgProg, false, false)>;
		}
		break;
	case NUM_INT8:
	case NUM_UINT8:
		return SearchFuncFwd<SEARCHTYPE(DATA_BYTE1, eVecSize, fDlgProg)>;
	case NUM_INT16:
	case NUM_UINT16:
		return SearchFuncFwd<SEARCHTYPE(DATA_BYTE2, eVecSize, fDlgProg)>;
	case NUM_INT32:
	case NUM_UINT32:
	case NUM_FLOAT:
		return SearchFuncFwd<SEARCHTYPE(DATA_BYTE4, eVecSize, fDlgProg)>;
	case NUM_INT64:
	case NUM_UINT64:
	case NUM_DOUBLE:
	case STRUCT_FILETIME:
		return SearchFuncFwd<SEARCHTYPE(DATA_BYTE8, eVecSize, fDlgProg)>;
	default:
		break;
	}

	return { };
}

template<bool fDlgProg, CHexDlgSearch::EVecSize eVecSize>
auto CHexDlgSearch::GetSearchFuncBack()const->PtrSearchFunc
{
	//The `fDlgProg` arg ensures that no runtime check will be performed for the 
	//'SEARCHFUNCDATA::pDlgProg == nullptr', at the hot path inside the SearchFunc function.
	using enum ESearchType; using enum EMemCmp; using enum EVecSize;

	switch (GetSearchType()) {
	case HEXBYTES:
		return IsWildcard() ?
			SearchFuncBack<SEARCHTYPE(CHAR_STR, eVecSize, fDlgProg, true, true)> :
			SearchFuncBack<SEARCHTYPE(CHAR_STR, eVecSize, fDlgProg, true, false)>;
	case TEXT_ASCII:
		if (IsMatchCase() && !IsWildcard()) {
			return SearchFuncBack<SEARCHTYPE(CHAR_STR, eVecSize, fDlgProg, true, false)>;
		}

		if (IsMatchCase() && IsWildcard()) {
			return SearchFuncBack<SEARCHTYPE(CHAR_STR, eVecSize, fDlgProg, true, true)>;
		}

		if (!IsMatchCase() && IsWildcard()) {
			return SearchFuncBack<SEARCHTYPE(CHAR_STR, eVecSize, fDlgProg, false, true)>;
		}

		if (!IsMatchCase() && !IsWildcard()) {
			return SearchFuncBack<SEARCHTYPE(CHAR_STR, eVecSize, fDlgProg, false, false)>;
		}
		break;
	case TEXT_UTF8:
		return SearchFuncBack<SEARCHTYPE(CHAR_STR, eVecSize, fDlgProg, true, false)>; //Search UTF-8 as plain chars.
	case TEXT_UTF16:
		if (IsMatchCase() && !IsWildcard()) {
			return SearchFuncBack<SEARCHTYPE(WCHAR_STR, eVecSize, fDlgProg, true, false)>;
		}

		if (IsMatchCase() && IsWildcard()) {
			return SearchFuncBack<SEARCHTYPE(WCHAR_STR, eVecSize, fDlgProg, true, true)>;
		}

		if (!IsMatchCase() && IsWildcard()) {
			return SearchFuncBack<SEARCHTYPE(WCHAR_STR, eVecSize, fDlgProg, false, true)>;
		}

		if (!IsMatchCase() && !IsWildcard()) {
			return SearchFuncBack<SEARCHTYPE(WCHAR_STR, eVecSize, fDlgProg, false, false)>;
		}
		break;
	case NUM_INT8:
	case NUM_UINT8:
		return SearchFuncBack<SEARCHTYPE(DATA_BYTE1, eVecSize, fDlgProg)>;
	case NUM_INT16:
	case NUM_UINT16:
		return SearchFuncBack<SEARCHTYPE(DATA_BYTE2, eVecSize, fDlgProg)>;
	case NUM_INT32:
	case NUM_UINT32:
	case NUM_FLOAT:
		return SearchFuncBack<SEARCHTYPE(DATA_BYTE4, eVecSize, fDlgProg)>;
	case NUM_INT64:
	case NUM_UINT64:
	case NUM_DOUBLE:
	case STRUCT_FILETIME:
		return SearchFuncBack<SEARCHTYPE(DATA_BYTE8, eVecSize, fDlgProg)>;
	default:
		break;
	}

	return { };
}

auto CHexDlgSearch::GetSearchMode()const->CHexDlgSearch::ESearchMode
{
	return static_cast<ESearchMode>(m_WndCmbMode.GetItemData(m_WndCmbMode.GetCurSel()));
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

	return static_cast<ESearchType>(m_WndCmbType.GetItemData(m_WndCmbType.GetCurSel()));
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
	return m_WndBtnBE.IsChecked();
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
	return m_WndBtnInv.IsChecked();
}

bool CHexDlgSearch::IsMatchCase()const
{
	return m_WndBtnMC.IsChecked();
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
	return m_WndBtnSel.IsChecked();
}

bool CHexDlgSearch::IsSmallSearch()const
{
	constexpr auto uSizeQuick { 1024U * 1024U * 50U }; //50MB without creating a new thread.
	return GetSearchRngSize() <= uSizeQuick;
}

bool CHexDlgSearch::IsWildcard()const
{
	return m_WndBtnWC.IsWindowEnabled() && m_WndBtnWC.IsChecked();
}

auto CHexDlgSearch::OnActivate(const MSG& msg)->INT_PTR
{
	if (m_pHexCtrl == nullptr || !m_pHexCtrl->IsCreated())
		return FALSE;

	const auto nState = LOWORD(msg.wParam);
	if (nState == WA_ACTIVE || nState == WA_CLICKACTIVE) {
		m_WndCmbFind.SetFocus();
		SetControlsState();
	}

	return FALSE;
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

	OnClose();
}

void CHexDlgSearch::OnCheckSel()
{
	m_WndEditStart.EnableWindow(!IsSelection());
	m_WndEditRngBegin.EnableWindow(!IsSelection());
	m_WndEditRngEnd.EnableWindow(!IsSelection());
}

auto CHexDlgSearch::OnClose()->INT_PTR
{
	ShowWindow(SW_HIDE);
	return TRUE;
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
		m_WndBtnBE.EnableWindow(true);
		const auto [dwFormat, wchSepar] = GetHexCtrl()->GetDateInfo();
		const auto wstr = ut::GetDateFormatString(dwFormat, wchSepar);
		m_WndCmbFind.SetCueBanner(wstr);
		m_WndCmbReplace.SetCueBanner(wstr);
	}
	break;
	case TEXT_ASCII:
	case TEXT_UTF16:
		m_WndBtnMC.EnableWindow(true);
		m_WndBtnWC.EnableWindow(true);
		break;
	case TEXT_UTF8:
		m_WndBtnMC.EnableWindow(false);
		m_WndBtnWC.EnableWindow(false);
		break;
	default:
		m_WndCmbFind.SetCueBanner(L"");
		m_WndCmbReplace.SetCueBanner(L"");
		break;
	}
}

auto CHexDlgSearch::OnCommand(const MSG& msg)->INT_PTR
{
	const auto uCtrlID = LOWORD(msg.wParam); //Control ID or menu ID.
	const auto uCode = HIWORD(msg.wParam);   //Control code, zero for menu.
	const auto hWndCtrl = reinterpret_cast<HWND>(msg.lParam); //Control HWND, zero for menu.

	//IDOK and IDCANCEL don't have HWND in lParam, if send as result of
	//IsDialogMessage and no button with such ID presents in the dialog.
	if (hWndCtrl != nullptr || uCtrlID == IDOK || uCtrlID == IDCANCEL) { //Notifications from controls.
		switch (uCtrlID) {
		case IDOK: OnOK(); break;
		case IDCANCEL: OnCancel(); break;
		case IDC_HEXCTRL_SEARCH_BTN_SEARCHF: OnButtonSearchF(); break;
		case IDC_HEXCTRL_SEARCH_BTN_SEARCHB: OnButtonSearchB(); break;
		case IDC_HEXCTRL_SEARCH_BTN_FINDALL: OnButtonFindAll(); break;
		case IDC_HEXCTRL_SEARCH_BTN_REPL: OnButtonReplace(); break;
		case IDC_HEXCTRL_SEARCH_BTN_REPLALL: OnButtonReplaceAll(); break;
		case IDC_HEXCTRL_SEARCH_CHK_SEL: OnCheckSel(); break;
		case IDC_HEXCTRL_SEARCH_COMBO_MODE: if (uCode == CBN_SELCHANGE) { OnComboModeSelChange(); } break;
		case IDC_HEXCTRL_SEARCH_COMBO_TYPE: if (uCode == CBN_SELCHANGE) { OnComboTypeSelChange(); } break;
		case IDC_HEXCTRL_SEARCH_COMBO_FIND: if (uCode == CBN_EDITUPDATE) { SetControlsState(); } break;
		case IDC_HEXCTRL_SEARCH_COMBO_REPL: if (uCode == CBN_EDITUPDATE) { SetControlsState(); } break;
		default: return FALSE;
		}
	}
	else {
		switch (static_cast<EMenuID>(uCtrlID)) {
		case EMenuID::IDM_SEARCH_ADDBKM:
		{
			int nItem { -1 };
			for (auto i = 0UL; i < m_ListEx.GetSelectedCount(); ++i) {
				nItem = m_ListEx.GetNextItem(nItem, LVNI_SELECTED);
				const HEXBKM hbs { .vecSpan { HEXSPAN { m_vecSearchRes.at(static_cast<std::size_t>(nItem)),
					m_fReplace ? m_vecReplaceData.size() : m_vecSearchData.size() } }, .wstrDesc { m_wstrSearch },
					.stClr { GetHexCtrl()->GetColors().clrBkBkm, GetHexCtrl()->GetColors().clrFontBkm } };
				GetHexCtrl()->GetBookmarks()->AddBkm(hbs, false);
			}
			GetHexCtrl()->Redraw();
		}
		break;
		case EMenuID::IDM_SEARCH_SELECTALL: m_ListEx.SetItemState(-1, LVIS_SELECTED, LVIS_SELECTED); break;
		case EMenuID::IDM_SEARCH_CLEARALL:
			ClearList();
			m_fSecondMatch = false; //To be able to search from the zero offset.
			break;
		default: return FALSE;
		}
	}

	return TRUE;
}

auto CHexDlgSearch::OnCtlClrStatic(const MSG& msg)->INT_PTR
{
	if (const auto hWndFrom = reinterpret_cast<HWND>(msg.lParam); hWndFrom == m_WndStatResult) {
		const auto hDC = reinterpret_cast<HDC>(msg.wParam);
		::SetTextColor(hDC, m_fFound ? RGB(0, 200, 0) : RGB(200, 0, 0));
		::SetBkColor(hDC, ::GetSysColor(COLOR_3DFACE));
		return reinterpret_cast<INT_PTR>(::GetSysColorBrush(COLOR_3DFACE));
	}

	return FALSE; //Default handler.
}

auto CHexDlgSearch::OnDestroy()->INT_PTR
{
	m_MenuList.DestroyMenu();
	m_vecSearchRes.clear();
	m_vecSearchData.clear();
	m_vecReplaceData.clear();
	m_wstrSearch.clear();
	m_wstrReplace.clear();
	m_u64Flags = { };
	m_pHexCtrl = nullptr;

	return TRUE;
}

auto CHexDlgSearch::OnDrawItem(const MSG& msg)->INT_PTR
{
	const auto pDIS = reinterpret_cast<LPDRAWITEMSTRUCT>(msg.lParam);
	if (pDIS->CtlID == static_cast<UINT>(IDC_HEXCTRL_SEARCH_LIST)) {
		m_ListEx.DrawItem(pDIS);
	}

	return TRUE;
}

auto CHexDlgSearch::OnInitDialog(const MSG& msg)->INT_PTR
{
	m_Wnd.Attach(msg.hwnd);
	m_WndStatResult.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_STAT_RESULT));
	m_WndCmbFind.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_COMBO_FIND));
	m_WndCmbReplace.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_COMBO_REPL));
	m_WndCmbMode.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_COMBO_MODE));
	m_WndCmbType.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_COMBO_TYPE));
	m_WndBtnSel.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_CHK_SEL));
	m_WndBtnWC.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_CHK_WC));
	m_WndBtnInv.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_CHK_INV));
	m_WndBtnBE.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_CHK_BE));
	m_WndBtnMC.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_CHK_MC));
	m_WndEditStart.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_EDIT_START));
	m_WndEditStep.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_EDIT_STEP));
	m_WndEditRngBegin.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_EDIT_RNGBEG));
	m_WndEditRngEnd.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_EDIT_RNGEND));
	m_WndEditLimit.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_EDIT_LIMIT));

	constexpr auto iTextLimit { 512 };
	m_WndCmbFind.LimitText(iTextLimit);
	m_WndCmbReplace.LimitText(iTextLimit);

	auto iIndex = m_WndCmbMode.AddString(L"Hex Bytes");
	m_WndCmbMode.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchMode::MODE_HEXBYTES));
	m_WndCmbMode.SetCurSel(iIndex);
	iIndex = m_WndCmbMode.AddString(L"Text");
	m_WndCmbMode.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchMode::MODE_TEXT));
	iIndex = m_WndCmbMode.AddString(L"Numbers");
	m_WndCmbMode.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchMode::MODE_NUMBERS));
	iIndex = m_WndCmbMode.AddString(L"Structs");
	m_WndCmbMode.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchMode::MODE_STRUCT));

	m_ListEx.Create({ .hWndParent { m_Wnd }, .uID { IDC_HEXCTRL_SEARCH_LIST }, .dwSizeFontList { 10 },
		.dwSizeFontHdr { 10 }, .fDialogCtrl { true } });
	m_ListEx.SetExtendedStyle(LVS_EX_HEADERDRAGDROP);
	m_ListEx.InsertColumn(0, L"№", LVCFMT_LEFT, 50);
	m_ListEx.InsertColumn(1, L"Offset", LVCFMT_LEFT, 455);

	m_MenuList.CreatePopupMenu();
	m_MenuList.AppendString(static_cast<UINT_PTR>(EMenuID::IDM_SEARCH_ADDBKM), L"Add bookmark(s)");
	m_MenuList.AppendString(static_cast<UINT_PTR>(EMenuID::IDM_SEARCH_SELECTALL), L"Select All");
	m_MenuList.AppendSepar();
	m_MenuList.AppendString(static_cast<UINT_PTR>(EMenuID::IDM_SEARCH_CLEARALL), L"Clear All");

	m_WndEditStep.SetWndText(std::format(L"{}", GetStep())); //"Step" edit box text.
	m_WndEditLimit.SetWndText(std::format(L"{}", m_dwLimit).data()); //"Limit search hits" edit box text.
	m_WndEditRngBegin.SetCueBanner(L"range begin");
	m_WndEditRngEnd.SetCueBanner(L"range end");

	const auto hwndTipWC = CreateWindowExW(0, TOOLTIPS_CLASSW, nullptr, WS_POPUP | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_Wnd, nullptr, nullptr, nullptr);
	const auto hwndTipInv = CreateWindowExW(0, TOOLTIPS_CLASSW, nullptr, WS_POPUP | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_Wnd, nullptr, nullptr, nullptr);
	const auto hwndTipEndOffset = CreateWindowExW(0, TOOLTIPS_CLASSW, nullptr, WS_POPUP | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_Wnd, nullptr, nullptr, nullptr);
	if (hwndTipWC == nullptr || hwndTipInv == nullptr || hwndTipEndOffset == nullptr)
		return FALSE;

	TTTOOLINFOW stToolInfo { .cbSize { sizeof(TTTOOLINFOW) }, .uFlags { TTF_IDISHWND | TTF_SUBCLASS }, .hwnd { m_Wnd },
		.uId { reinterpret_cast<UINT_PTR>(m_WndBtnWC.GetHWND()) } }; //"Wildcard" check box tooltip.

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

	stToolInfo.uId = reinterpret_cast<UINT_PTR>(m_WndBtnInv.GetHWND()); //"Inverted" check box tooltip.
	wstrToolText = L"Search for the non-matching occurences.\r\nThat is everything that doesn't match search conditions.";
	stToolInfo.lpszText = wstrToolText.data();
	::SendMessageW(hwndTipInv, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&stToolInfo));
	::SendMessageW(hwndTipInv, TTM_SETDELAYTIME, TTDT_AUTOPOP, static_cast<LPARAM>(LOWORD(0x7FFF)));
	::SendMessageW(hwndTipInv, TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(1000));

	stToolInfo.uId = reinterpret_cast<UINT_PTR>(m_WndEditRngEnd.GetHWND()); //"Search range end" edit box tooltip.
	wstrToolText = L"Last offset for the search.\r\nEmpty means until data end.";
	stToolInfo.lpszText = wstrToolText.data();
	::SendMessageW(hwndTipEndOffset, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&stToolInfo));
	::SendMessageW(hwndTipEndOffset, TTM_SETDELAYTIME, TTDT_AUTOPOP, static_cast<LPARAM>(LOWORD(0x7FFF)));
	::SendMessageW(hwndTipEndOffset, TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(1000));

	return TRUE;
}

auto CHexDlgSearch::OnMeasureItem(const MSG& msg)->INT_PTR
{
	const auto pMIS = reinterpret_cast<LPMEASUREITEMSTRUCT>(msg.lParam);
	if (pMIS->CtlID == static_cast<UINT>(IDC_HEXCTRL_SEARCH_LIST)) {
		m_ListEx.MeasureItem(pMIS);
	}

	return TRUE;
}

auto CHexDlgSearch::OnNotify(const MSG& msg)->INT_PTR
{
	const auto pNMHDR = reinterpret_cast<NMHDR*>(msg.lParam);
	switch (pNMHDR->idFrom) {
	case IDC_HEXCTRL_SEARCH_LIST:
		switch (pNMHDR->code) {
		case LVN_GETDISPINFOW: OnNotifyListGetDispInfo(pNMHDR); break;
		case LVN_ITEMCHANGED: OnNotifyListItemChanged(pNMHDR); break;
		case NM_RCLICK: OnNotifyListRClick(pNMHDR); break;
		default: break;
		}
	default: break;
	}

	return TRUE;
}

void CHexDlgSearch::OnNotifyListGetDispInfo(NMHDR* pNMHDR)
{
	const auto* const pDispInfo = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
	const auto* const pItem = &pDispInfo->item;
	if ((pItem->mask & LVIF_TEXT) == 0)
		return;

	const auto iItem = static_cast<std::size_t>(pItem->iItem);
	switch (pItem->iSubItem) {
	case 0: //Index value.
		*std::format_to(pItem->pszText, L"{}", iItem + 1) = L'\0';
		break;
	case 1: //Offset.
		*std::format_to(pItem->pszText, L"0x{:X}", GetHexCtrl()->GetOffset(m_vecSearchRes[iItem], true)) = L'\0';
		break;
	default:
		break;
	}
}

void CHexDlgSearch::OnNotifyListItemChanged(NMHDR* pNMHDR)
{
	const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	if (pNMI->iItem < 0 || pNMI->iSubItem < 0 || !(pNMI->uNewState & LVIS_SELECTED))
		return;

	VecSpan vecSpan;
	int nItem = -1;
	for (auto i = 0UL; i < m_ListEx.GetSelectedCount(); ++i) {
		nItem = m_ListEx.GetNextItem(nItem, LVNI_SELECTED);

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
	SetEditStartFrom(GetHexCtrl()->GetOffset(ullOffset, true)); //Show virtual offset.
	m_ullStartFrom = ullOffset;
}

void CHexDlgSearch::OnNotifyListRClick(NMHDR* /*pNMHDR*/)
{
	const auto fEnabled { m_ListEx.GetItemCount() > 0 };
	m_MenuList.EnableMenuItem(static_cast<UINT>(EMenuID::IDM_SEARCH_ADDBKM), fEnabled);
	m_MenuList.EnableMenuItem(static_cast<UINT>(EMenuID::IDM_SEARCH_SELECTALL), fEnabled);
	m_MenuList.EnableMenuItem(static_cast<UINT>(EMenuID::IDM_SEARCH_CLEARALL), fEnabled);

	POINT pt;
	GetCursorPos(&pt);
	m_MenuList.TrackPopupMenu(pt.x, pt.y, m_Wnd);
}

void CHexDlgSearch::OnOK()
{
	OnButtonSearchF();
}

void CHexDlgSearch::OnSelectModeHEXBYTES()
{
	ClearComboType();
	m_WndCmbType.EnableWindow(false);
	m_WndBtnBE.EnableWindow(false);
	m_WndBtnMC.EnableWindow(true);
	m_WndBtnWC.EnableWindow(true);
}

void CHexDlgSearch::OnSelectModeNUMBERS()
{
	ClearComboType();

	m_WndCmbType.EnableWindow(true);
	auto iIndex = m_WndCmbType.AddString(L"Int8");
	m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchType::NUM_INT8));
	m_WndCmbType.SetCurSel(iIndex);
	iIndex = m_WndCmbType.AddString(L"Unsigned Int8");
	m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchType::NUM_UINT8));
	iIndex = m_WndCmbType.AddString(L"Int16");
	m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchType::NUM_INT16));
	iIndex = m_WndCmbType.AddString(L"Unsigned Int16");
	m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchType::NUM_UINT16));
	iIndex = m_WndCmbType.AddString(L"Int32");
	m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchType::NUM_INT32));
	iIndex = m_WndCmbType.AddString(L"Unsigned Int32");
	m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchType::NUM_UINT32));
	iIndex = m_WndCmbType.AddString(L"Int64");
	m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchType::NUM_INT64));
	iIndex = m_WndCmbType.AddString(L"Unsigned Int64");
	m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchType::NUM_UINT64));
	iIndex = m_WndCmbType.AddString(L"Float");
	m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchType::NUM_FLOAT));
	iIndex = m_WndCmbType.AddString(L"Double");
	m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchType::NUM_DOUBLE));

	m_WndBtnBE.EnableWindow(true);
	m_WndBtnMC.EnableWindow(false);
	m_WndBtnWC.EnableWindow(false);
}

void CHexDlgSearch::OnSelectModeSTRUCT()
{
	ClearComboType();

	m_WndCmbType.EnableWindow(true);
	auto iIndex = m_WndCmbType.AddString(L"FILETIME");
	m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchType::STRUCT_FILETIME));
	m_WndCmbType.SetCurSel(iIndex);

	m_WndBtnMC.EnableWindow(false);
	m_WndBtnWC.EnableWindow(false);
}

void CHexDlgSearch::OnSearchModeTEXT()
{
	ClearComboType();

	m_WndCmbType.EnableWindow(true);
	auto iIndex = m_WndCmbType.AddString(L"ASCII");
	m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchType::TEXT_ASCII));
	m_WndCmbType.SetCurSel(iIndex);
	iIndex = m_WndCmbType.AddString(L"UTF-8");
	m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchType::TEXT_UTF8));
	iIndex = m_WndCmbType.AddString(L"UTF-16");
	m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(ESearchType::TEXT_UTF16));

	m_WndBtnBE.EnableWindow(false);
}

void CHexDlgSearch::Prepare()
{
	if (!m_Wnd.IsWindow())
		return;

	constexpr auto uSearchSizeLimit { 256U };
	const auto pHexCtrl = GetHexCtrl();
	if (!pHexCtrl->IsDataSet())
		return;

	const auto ullHexDataSize = pHexCtrl->GetDataSize();

	//"Search" text.
	const auto iSearchSize = m_WndCmbFind.GetWndTextSize();
	if (iSearchSize == 0 || iSearchSize > uSearchSizeLimit) {
		m_WndCmbFind.SetFocus();
		return;
	}

	const auto wstrSearch = m_WndCmbFind.GetWndText();
	if (wstrSearch != m_wstrSearch) {
		ResetSearch();
		m_wstrSearch = wstrSearch;
	}

	//"Replace" text.
	if (IsReplace()) {
		if (m_WndCmbReplace.IsWndTextEmpty()) {
			m_WndCmbReplace.SetFocus();
			MessageBoxW(m_Wnd, m_pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
			return;
		}

		m_wstrReplace = m_WndCmbReplace.GetWndText();
		ComboReplaceFill(m_wstrReplace.data());
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

	ComboSearchFill(wstrSearch.data()); //Adding current search text to the search combo-box.

	ULONGLONG ullStartFrom;
	ULONGLONG ullRngStart;
	ULONGLONG ullRngEnd;

	//"Search range start/end" offsets.
	if (!IsSelection()) { //Do not use "Search range start/end offsets" when in selection.
		//"Search range start offset".
		if (m_WndEditRngBegin.IsWndTextEmpty()) {
			ullRngStart = 0;
		}
		else {
			const auto optRngStart = stn::StrToUInt64(m_WndEditRngBegin.GetWndText());
			if (!optRngStart) {
				m_WndEditRngBegin.SetFocus();
				MessageBoxW(m_Wnd, L"Search range start offset is incorrect.", L"Incorrect offset", MB_ICONERROR);
				return;
			}

			ullRngStart = pHexCtrl->GetOffset(*optRngStart, false); //Always use flat offset.
		}

		//"Search range end offset".
		if (m_WndEditRngEnd.IsWndTextEmpty()) {
			ullRngEnd = ullHexDataSize - 1; //The last possible referensable offset.
		}
		else {
			const auto optRngEnd = stn::StrToUInt64(m_WndEditRngEnd.GetWndText());
			if (!optRngEnd) {
				m_WndEditRngEnd.SetFocus();
				MessageBoxW(m_Wnd, L"Search range end offset is incorrect.", L"Incorrect offset", MB_ICONERROR);
				return;
			}

			ullRngEnd = pHexCtrl->GetOffset(*optRngEnd, false); //Always use flat offset.
		}

		//"Start from".
		if (m_WndEditStart.IsWndTextEmpty()) {
			ullStartFrom = ullRngStart; //If empty field.
		}
		else {
			const auto optStartFrom = stn::StrToUInt64(m_WndEditStart.GetWndText());
			if (!optStartFrom) {
				m_WndEditStart.SetFocus();
				MessageBoxW(m_Wnd, L"Start offset is incorrect.", L"Incorrect offset", MB_ICONERROR);
				return;
			}

			ullStartFrom = pHexCtrl->GetOffset(*optStartFrom, false); //Always use flat offset.
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
		m_WndEditRngEnd.SetFocus();
		MessageBoxW(m_Wnd, L"Search range is out of data bounds.", L"Incorrect offset", MB_ICONERROR);
		return;
	}

	//"Start from" check to fit within the search range.
	if (ullStartFrom < ullRngStart || ullStartFrom > ullRngEnd) {
		m_WndEditStart.SetFocus();
		MessageBoxW(m_Wnd, L"Start offset is not within the search range.", L"Incorrect offset", MB_ICONERROR);
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
	if (!m_WndEditStep.IsWndTextEmpty()) {
		if (const auto optStep = stn::StrToUInt64(m_WndEditStep.GetWndText()); optStep) {
			ullStep = *optStep;
		}
		else {
			m_WndEditStep.SetFocus();
			MessageBoxW(m_Wnd, L"Incorrect step size.", L"Incorrect step", MB_ICONERROR);
			return;
		}
	}
	else {
		m_WndEditStep.SetFocus();
		MessageBoxW(m_Wnd, L"Step size must be at least one.", L"Incorrect step", MB_ICONERROR);
		return;
	}

	//"Limit".
	DWORD dwLimit;
	if (!m_WndEditLimit.IsWndTextEmpty()) {
		if (const auto optLimit = stn::StrToUInt32(m_WndEditLimit.GetWndText()); optLimit) {
			dwLimit = *optLimit;
		}
		else {
			m_WndEditLimit.SetFocus();
			MessageBoxW(m_Wnd, L"Incorrect limit size.", L"Incorrect limit", MB_ICONERROR);
			return;
		}
	}
	else {
		m_WndEditLimit.SetFocus();
		MessageBoxW(m_Wnd, L"Limit size must be at least one.", L"Incorrect limit", MB_ICONERROR);
		return;
	}

	if (IsReplace() && GetReplaceDataSize() > GetSearchDataSize()) {
		static constexpr auto wstrReplaceWarning { L"The replacing data is longer than search data.\r\n"
			"Do you want to overwrite bytes following search occurrence?\r\nChoosing No will cancel replace." };
		if (MessageBoxW(m_Wnd, wstrReplaceWarning, L"Warning", MB_YESNO | MB_ICONQUESTION | MB_TOPMOST) == IDNO)
			return;
	}

	m_ullRngBegin = ullRngStart;
	m_ullRngEnd = ullRngEnd;
	m_ullStartFrom = ullStartFrom;
	m_ullStep = ullStep;
	m_dwLimit = dwLimit;

	Search();
	m_Wnd.SetActiveWindow();
}

bool CHexDlgSearch::PrepareHexBytes()
{
	static constexpr auto pwszWrongInput { L"Unacceptable input character.\r\nAllowed characters are: 0123456789AaBbCcDdEeFf" };
	auto optData = ut::NumStrToHex(m_wstrSearch, IsWildcard() ? static_cast<char>(m_uWildcard) : 0);
	if (!optData) {
		m_iWrap = 1;
		MessageBoxW(m_Wnd, pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}

	m_vecSearchData = ut::RangeToVecBytes(*optData);

	if (m_fReplace) {
		auto optDataRepl = ut::NumStrToHex(m_wstrReplace);
		if (!optDataRepl) {
			MessageBoxW(m_Wnd, pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
			m_WndCmbReplace.SetFocus();
			return false;
		}

		m_vecReplaceData = ut::RangeToVecBytes(*optDataRepl);
	}

	return true;
}

bool CHexDlgSearch::PrepareTextASCII()
{
	auto strSearch = ut::WstrToStr(m_wstrSearch, CP_ACP); //Convert to the system default Windows ANSI code page.
	if (!IsMatchCase()) { //Make the string lowercase.
		std::transform(strSearch.begin(), strSearch.end(), strSearch.begin(),
			[](char ch) { return static_cast<char>(std::tolower(ch)); });
	}

	m_vecSearchData = ut::RangeToVecBytes(strSearch);
	m_vecReplaceData = ut::RangeToVecBytes(ut::WstrToStr(m_wstrReplace, CP_ACP));

	return true;
}

bool CHexDlgSearch::PrepareTextUTF16()
{
	auto wstrSearch = m_wstrSearch;
	if (!IsMatchCase()) { //Make the string lowercase.
		std::transform(wstrSearch.begin(), wstrSearch.end(), wstrSearch.begin(),
			[](wchar_t wch) { return static_cast<wchar_t>(std::tolower(wch)); });
	}

	m_vecSearchData = ut::RangeToVecBytes(wstrSearch);
	m_vecReplaceData = ut::RangeToVecBytes(m_wstrReplace);

	return true;
}

bool CHexDlgSearch::PrepareTextUTF8()
{
	m_vecSearchData = ut::RangeToVecBytes(ut::WstrToStr(m_wstrSearch, CP_UTF8)); //Convert to UTF-8 string.
	m_vecReplaceData = ut::RangeToVecBytes(ut::WstrToStr(m_wstrReplace, CP_UTF8));

	return true;
}

template<typename T> requires ut::TSize1248<T>
bool CHexDlgSearch::PrepareNumber()
{
	const auto optData = stn::StrToNum<T>(m_wstrSearch);
	if (!optData) {
		m_WndCmbFind.SetFocus();
		MessageBoxW(m_Wnd, m_pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}

	T tData = *optData;
	T tDataRep { };

	if (m_fReplace) {
		const auto optDataRepl = stn::StrToNum<T>(m_wstrReplace);
		if (!optDataRepl) {
			m_WndCmbReplace.SetFocus();
			MessageBoxW(m_Wnd, m_pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		tDataRep = *optDataRepl;
	}

	if (IsBigEndian()) {
		tData = ut::ByteSwap(tData);
		tDataRep = ut::ByteSwap(tDataRep);
	}

	m_vecSearchData = ut::RangeToVecBytes(tData);
	m_vecReplaceData = ut::RangeToVecBytes(tDataRep);

	return true;
}

bool CHexDlgSearch::PrepareFILETIME()
{
	const auto [dwFormat, wchSepar] = GetHexCtrl()->GetDateInfo();
	const std::wstring wstrErr = L"Wrong FILETIME format.\r\nA correct format is: " + ut::GetDateFormatString(dwFormat, wchSepar);
	const auto optFTSearch = ut::StringToFileTime(m_wstrSearch, dwFormat);
	if (!optFTSearch) {
		MessageBoxW(m_Wnd, wstrErr.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}

	FILETIME ftSearch = *optFTSearch;
	FILETIME ftReplace { };

	if (m_fReplace) {
		const auto optFTReplace = ut::StringToFileTime(m_wstrReplace, dwFormat);
		if (!optFTReplace) {
			MessageBoxW(m_Wnd, wstrErr.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		ftReplace = *optFTReplace;
	}

	if (IsBigEndian()) {
		ftSearch.dwLowDateTime = ut::ByteSwap(ftSearch.dwLowDateTime);
		ftSearch.dwHighDateTime = ut::ByteSwap(ftSearch.dwHighDateTime);
		ftReplace.dwLowDateTime = ut::ByteSwap(ftReplace.dwLowDateTime);
		ftReplace.dwHighDateTime = ut::ByteSwap(ftReplace.dwHighDateTime);
	}

	m_vecSearchData = ut::RangeToVecBytes(ftSearch);
	m_vecReplaceData = ut::RangeToVecBytes(ftReplace);

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
		CHexDlgProgress dlgProg(L"Replacing...", L"Replaced: ", GetStartFrom(), GetLastSearchOffset());
		const auto lmbReplaceAllThread = [&]() {
			auto stFuncData = CreateSearchData(&dlgProg);
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
				dlgProg.SetCurrent(findRes.ullOffset);
				dlgProg.SetCount(m_vecSearchRes.size());

				const auto ullNext = findRes.ullOffset + (sSizeRepl <= stFuncData.ullStep ?
					stFuncData.ullStep : sSizeRepl);
				if (ullNext > GetLastSearchOffset() || m_vecSearchRes.size() >= m_dwLimit
					|| dlgProg.IsCanceled()) {
					break;
				}

				m_ullStartFrom = stFuncData.ullStartFrom = ullNext;
			}
			dlgProg.OnCancel();
			};

		std::thread thrd(lmbReplaceAllThread);
		dlgProg.DoModal(m_Wnd, m_hInstRes);
		thrd.join();
	}

	if (!m_vecSearchRes.empty()) {
		m_fFound = true;
		m_dwCount = m_dwReplaced = static_cast<DWORD>(m_vecSearchRes.size());
	}
	m_ListEx.SetItemCountEx(static_cast<int>(m_vecSearchRes.size()));
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
	m_WndStatResult.SetWndText(L"");
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
				wstrInfo = std::format(ut::GetLocale(), L"{:L} occurrence(s) replaced.", m_dwReplaced);
				m_dwReplaced = 0;
				GetHexCtrl()->Redraw(); //Redraw in case of Replace all.
			}
			else {
				wstrInfo = std::format(ut::GetLocale(), L"Found {:L} occurrences.", m_dwCount);
				m_dwCount = 0;
			}
		}
		else {
			if (m_fDoCount) {
				wstrInfo = std::format(ut::GetLocale(), L"Found occurrence № {:L} from the beginning.", m_dwCount);
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

			SetEditStartFrom(GetHexCtrl()->GetOffset(ullStartFrom, true));
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
	m_WndStatResult.SetWndText(wstrInfo);

	if (!m_fSearchNext) {
		m_Wnd.SetForegroundWindow();
		m_Wnd.SetFocus();
	}
	else { m_fSearchNext = false; }
}

void CHexDlgSearch::SetControlsState()
{
	const auto pHexCtrl = GetHexCtrl();
	if (!pHexCtrl->IsCreated() || !pHexCtrl->IsDataSet())
		return;

	const auto fMutable = pHexCtrl->IsMutable();
	const auto fSearchEnabled = !m_WndCmbFind.IsWndTextEmpty();
	const auto fReplaceEnabled = fMutable && fSearchEnabled && !m_WndCmbReplace.IsWndTextEmpty();

	m_WndCmbReplace.EnableWindow(fMutable);
	wnd::CWnd::FromHandle(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_SEARCHF)).EnableWindow(fSearchEnabled);
	wnd::CWnd::FromHandle(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_SEARCHB)).EnableWindow(fSearchEnabled);
	wnd::CWnd::FromHandle(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_FINDALL)).EnableWindow(fSearchEnabled);
	wnd::CWnd::FromHandle(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_REPL)).EnableWindow(fReplaceEnabled);
	wnd::CWnd::FromHandle(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_REPLALL)).EnableWindow(fReplaceEnabled);
}

void CHexDlgSearch::SetEditStartFrom(ULONGLONG ullOffset)
{
	m_WndEditStart.SetWndText(std::format(L"0x{:X}", ullOffset));
}


//Static functions.

void CHexDlgSearch::Replace(IHexCtrl* pHexCtrl, ULONGLONG ullIndex, SpanCByte spnReplace)
{
	pHexCtrl->ModifyData({ .eModifyMode { EHexModifyMode::MODIFY_ONCE }, .spnData { spnReplace },
		.vecSpan { { ullIndex, spnReplace.size() } } });
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
	const auto pDlgProg = refSearch.pDlgProg;
	const auto pDataSearch = refSearch.spnFind.data();
	const auto nSizeSearch = refSearch.spnFind.size();
	const auto ullEnd = ullOffsetSentinel - nSizeSearch;
	const auto fBigStep = refSearch.fBigStep;
	const auto fInverted = refSearch.fInverted;
	const auto ullChunks = refSearch.ullChunks;
	auto ullChunkSize = refSearch.ullChunkSize;
	auto ullChunkMaxOffset = refSearch.ullChunkMaxOffset;
	auto ullOffsetSearch = refSearch.ullStartFrom;

	for (auto itChunk = 0ULL; itChunk < ullChunks; ++itChunk) {
		const auto spnData = pHexCtrl->GetData({ ullOffsetSearch, ullChunkSize });
		assert(!spnData.empty());
		assert(spnData.size() >= ullChunkSize);

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

			if constexpr (stType.fDlgProg) {
				if (pDlgProg->IsCanceled()) {
					return { { }, false, true };
				}
				pDlgProg->SetCurrent(ullOffsetSearch + ullOffsetData);
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

		if (ullOffsetSearch + ullChunkSize > ullOffsetSentinel) {
			ullChunkSize = ullOffsetSentinel - ullOffsetSearch;
			ullChunkMaxOffset = ullChunkSize - nSizeSearch;
		}
	}

	return { };
}

#if defined(_M_IX86) || defined(_M_X64)
template<CHexDlgSearch::EVecSize eVecSize>
int CHexDlgSearch::MemCmpVecEQByte1(const std::byte* pWhere, std::byte bWhat)
{
	if constexpr (eVecSize == EVecSize::VEC128) {
		const auto m128iWhere = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pWhere));
		const auto m128iWhat = _mm_set1_epi8(static_cast<char>(bWhat));
		const auto m128iResult = _mm_cmpeq_epi8(m128iWhere, m128iWhat);
		const auto uiMask = static_cast<std::uint32_t>(_mm_movemask_epi8(m128iResult));
		return std::countr_zero(uiMask); //>15 here means not found, all in mask are zeros.
	}
	else if constexpr (eVecSize == EVecSize::VEC256) {
		const auto m256iWhere = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pWhere));
		const auto m256iWhat = _mm256_set1_epi8(static_cast<char>(bWhat));
		const auto m256iResult = _mm256_cmpeq_epi8(m256iWhere, m256iWhat);
		const auto uiMask = static_cast<std::uint32_t>(_mm256_movemask_epi8(m256iResult));
		return std::countr_zero(uiMask); //>31 here means not found, all in mask are zeros.
	}
}

template<CHexDlgSearch::EVecSize eVecSize>
int CHexDlgSearch::MemCmpVecNEQByte1(const std::byte* pWhere, std::byte bWhat)
{
	if constexpr (eVecSize == EVecSize::VEC128) {
		const auto m128iWhere = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pWhere));
		const auto m128iWhat = _mm_set1_epi8(static_cast<char>(bWhat));
		const auto m128iResult = _mm_cmpeq_epi8(m128iWhere, m128iWhat);
		const auto iMask = static_cast<std::uint32_t>(_mm_movemask_epi8(m128iResult));
		return std::countr_zero(~iMask);
	}
	else if constexpr (eVecSize == EVecSize::VEC256) {
		const auto m256iWhere = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pWhere));
		const auto m256iWhat = _mm256_set1_epi8(static_cast<char>(bWhat));
		const auto m256iResult = _mm256_cmpeq_epi8(m256iWhere, m256iWhat);
		const auto iMask = static_cast<std::uint32_t>(_mm256_movemask_epi8(m256iResult));
		return std::countr_zero(~iMask);
	}
}

template<CHexDlgSearch::EVecSize eVecSize>
int CHexDlgSearch::MemCmpVecEQByte2(const std::byte* pWhere, std::uint16_t ui16What)
{
	if constexpr (eVecSize == EVecSize::VEC128) {
		const auto m128iWhere0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pWhere));
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
	else if constexpr (eVecSize == EVecSize::VEC256) {
		const auto m256iWhere0 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pWhere));
		const auto m256iWhere1 = _mm256_srli_si256(m256iWhere0, 1); //Shifting-right the whole vector by 1 byte.
		const auto m256iWhat = _mm256_set1_epi16(ui16What);
		const auto m256iResult0 = _mm256_cmpeq_epi16(m256iWhere0, m256iWhat);
		const auto m256iResult1 = _mm256_cmpeq_epi16(m256iWhere1, m256iWhat); //Comparing both loads simultaneously.
		const auto uiMask0 = static_cast<std::uint32_t>(_mm256_movemask_epi8(m256iResult0));
		//Shifting-left the mask by 1 to compensate previous 1 byte right-shift by _mm256_srli_si256.
		const auto uiMask1 = static_cast<std::uint32_t>(_mm256_movemask_epi8(m256iResult1)) << 1;
		const auto iRes0 = std::countr_zero(uiMask0);
		//Setting the last bit (of 32) to zero, to avoid false positives when searching for `0000` and last byte is `00`.
		const auto iRes1 = std::countr_zero(uiMask1 & 0b01111111'11111111'11111111'11111110);
		return (std::min)(iRes0, iRes1); //>31 here means not found, all in mask are zeros.
	}
}

template<CHexDlgSearch::EVecSize eVecSize>
int CHexDlgSearch::MemCmpVecNEQByte2(const std::byte* pWhere, std::uint16_t ui16What)
{
	if constexpr (eVecSize == EVecSize::VEC128) {
		const auto m128iWhere0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pWhere));
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
	else if constexpr (eVecSize == EVecSize::VEC256) {
		const auto m256iWhere0 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pWhere));
		const auto m256iWhere1 = _mm256_srli_si256(m256iWhere0, 1);
		const auto m256iWhat = _mm256_set1_epi16(ui16What);
		const auto m256iResult0 = _mm256_cmpeq_epi16(m256iWhere0, m256iWhat);
		const auto m256iResult1 = _mm256_cmpeq_epi16(m256iWhere1, m256iWhat);
		const auto uiMask0 = static_cast<std::uint32_t>(_mm256_movemask_epi8(m256iResult0));
		const auto uiMask1 = static_cast<std::uint32_t>(_mm256_movemask_epi8(m256iResult1)) << 1;
		const auto iRes0 = std::countr_zero(~uiMask0);
		//Setting the first bit (of 32), after inverting, to zero, because it's always 0 after the left-shifting above.
		const auto iRes1 = std::countr_zero((~uiMask1) & 0b01111111'11111111'11111111'11111110);
		return (std::min)(iRes0, iRes1);
	}
}

template<CHexDlgSearch::EVecSize eVecSize>
int CHexDlgSearch::MemCmpVecEQByte4(const std::byte* pWhere, std::uint32_t ui32What)
{
	if constexpr (eVecSize == EVecSize::VEC128) {
		const auto m128iWhere0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pWhere));
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
	else if constexpr (eVecSize == EVecSize::VEC256) {
		const auto m256iWhere0 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pWhere));
		const auto m256iWhere1 = _mm256_srli_si256(m256iWhere0, 1);
		const auto m256iWhere2 = _mm256_srli_si256(m256iWhere0, 2);
		const auto m256iWhere3 = _mm256_srli_si256(m256iWhere0, 3);
		const auto m256iWhat = _mm256_set1_epi32(ui32What);
		const auto m256iResult0 = _mm256_cmpeq_epi32(m256iWhere0, m256iWhat);
		const auto m256iResult1 = _mm256_cmpeq_epi32(m256iWhere1, m256iWhat);
		const auto m256iResult2 = _mm256_cmpeq_epi32(m256iWhere2, m256iWhat);
		const auto m256iResult3 = _mm256_cmpeq_epi32(m256iWhere3, m256iWhat);
		const auto uiMask0 = static_cast<std::uint32_t>(_mm256_movemask_epi8(m256iResult0));
		const auto uiMask1 = static_cast<std::uint32_t>(_mm256_movemask_epi8(m256iResult1)) << 1;
		const auto uiMask2 = static_cast<std::uint32_t>(_mm256_movemask_epi8(m256iResult2)) << 2;
		const auto uiMask3 = static_cast<std::uint32_t>(_mm256_movemask_epi8(m256iResult3)) << 3;
		const auto iRes0 = std::countr_zero(uiMask0);
		const auto iRes1 = std::countr_zero(uiMask1 & 0b00011111'11111111'11111111'11111110);
		const auto iRes2 = std::countr_zero(uiMask2 & 0b00111111'11111111'11111111'11111100);
		const auto iRes3 = std::countr_zero(uiMask3 & 0b01111111'11111111'11111111'11111000);
		return (std::min)(iRes0, (std::min)(iRes1, (std::min)(iRes2, iRes3)));
	}
}

template<CHexDlgSearch::EVecSize eVecSize>
int CHexDlgSearch::MemCmpVecNEQByte4(const std::byte* pWhere, std::uint32_t ui32What)
{
	if constexpr (eVecSize == EVecSize::VEC128) {
		const auto m128iWhere0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pWhere));
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
	else if constexpr (eVecSize == EVecSize::VEC256) {
		const auto m256iWhere0 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pWhere));
		const auto m256iWhere1 = _mm256_srli_si256(m256iWhere0, 1);
		const auto m256iWhere2 = _mm256_srli_si256(m256iWhere0, 2);
		const auto m256iWhere3 = _mm256_srli_si256(m256iWhere0, 3);
		const auto m256iWhat = _mm256_set1_epi32(ui32What);
		const auto m256iResult0 = _mm256_cmpeq_epi32(m256iWhere0, m256iWhat);
		const auto m256iResult1 = _mm256_cmpeq_epi32(m256iWhere1, m256iWhat);
		const auto m256iResult2 = _mm256_cmpeq_epi32(m256iWhere2, m256iWhat);
		const auto m256iResult3 = _mm256_cmpeq_epi32(m256iWhere3, m256iWhat);
		const auto uiMask0 = static_cast<std::uint32_t>(_mm256_movemask_epi8(m256iResult0));
		const auto uiMask1 = static_cast<std::uint32_t>(_mm256_movemask_epi8(m256iResult1)) << 1;
		const auto uiMask2 = static_cast<std::uint32_t>(_mm256_movemask_epi8(m256iResult2)) << 2;
		const auto uiMask3 = static_cast<std::uint32_t>(_mm256_movemask_epi8(m256iResult3)) << 3;
		const auto iRes0 = std::countr_zero(~uiMask0);
		const auto iRes1 = std::countr_zero((~uiMask1) & 0b00011111'11111111'11111111'11111110);
		const auto iRes2 = std::countr_zero((~uiMask2) & 0b00111111'11111111'11111111'11111100);
		const auto iRes3 = std::countr_zero((~uiMask3) & 0b01111111'11111111'11111111'11111000);
		return (std::min)(iRes0, (std::min)(iRes1, (std::min)(iRes2, iRes3)));
	}
}

template<CHexDlgSearch::SEARCHTYPE stType>
auto CHexDlgSearch::SearchFuncVecFwdByte1(const SEARCHFUNCDATA& refSearch)->FINDRESULT
{
	//Members locality is important for the best performance of the tight search loop below.
	constexpr auto iVecSize = static_cast<int>(stType.eVecSize); //Vector size 128/256.
	const auto ullOffsetSentinel = refSearch.ullRngEnd + 1;
	const auto ullStep = refSearch.ullStep;
	const auto pHexCtrl = refSearch.pHexCtrl;
	const auto pDlgProg = refSearch.pDlgProg;
	const auto pDataSearch = refSearch.spnFind.data();
	const auto nSizeSearch = refSearch.spnFind.size();
	const auto ullEnd = ullOffsetSentinel - nSizeSearch;
	const auto fBigStep = refSearch.fBigStep;
	const auto fInverted = refSearch.fInverted;
	const auto ullChunks = refSearch.ullChunks;
	auto ullChunkSize = refSearch.ullChunkSize;
	auto ullChunkMaxOffset = refSearch.ullChunkMaxOffset;
	auto ullOffsetSearch = refSearch.ullStartFrom;

	for (auto itChunk = 0ULL; itChunk < ullChunks; ++itChunk) {
		const auto spnData = pHexCtrl->GetData({ ullOffsetSearch, ullChunkSize });
		assert(!spnData.empty());
		assert(spnData.size() >= ullChunkSize);

		for (auto ullOffsetData = 0ULL; ullOffsetData <= ullChunkMaxOffset; ullOffsetData += iVecSize) {
			if ((ullOffsetData + iVecSize) <= ullChunkMaxOffset) {
				if constexpr (!stType.fInverted) { //SIMD forward not inverted.
					if (const auto iRes = MemCmpVecEQByte1<stType.eVecSize>(spnData.data() + ullOffsetData, *pDataSearch);
						iRes < iVecSize) {
						return { ullOffsetSearch + ullOffsetData + iRes, true, false };
					}
				}
				else if constexpr (stType.fInverted) { //SIMD forward inverted.
					if (const auto iRes = MemCmpVecNEQByte1<stType.eVecSize>(spnData.data() + ullOffsetData, *pDataSearch);
						iRes < iVecSize) {
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

			if constexpr (stType.fDlgProg) {
				if (pDlgProg->IsCanceled()) {
					return { { }, false, true };
				}
				pDlgProg->SetCurrent(ullOffsetSearch + ullOffsetData);
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

		if (ullOffsetSearch + ullChunkSize > ullOffsetSentinel) {
			ullChunkSize = ullOffsetSentinel - ullOffsetSearch;
			ullChunkMaxOffset = ullChunkSize - nSizeSearch;
		}
	}

	return { };
}

template<CHexDlgSearch::SEARCHTYPE stType>
auto CHexDlgSearch::SearchFuncVecFwdByte2(const SEARCHFUNCDATA& refSearch)->FINDRESULT
{
	//Members locality is important for the best performance of the tight search loop below.
	constexpr auto iVecSize = static_cast<int>(stType.eVecSize);
	const auto ullOffsetSentinel = refSearch.ullRngEnd + 1;
	const auto ullStep = refSearch.ullStep;
	const auto pHexCtrl = refSearch.pHexCtrl;
	const auto pDlgProg = refSearch.pDlgProg;
	const auto pDataSearch = refSearch.spnFind.data();
	const auto nSizeSearch = refSearch.spnFind.size();
	const auto ullEnd = ullOffsetSentinel - nSizeSearch;
	const auto fBigStep = refSearch.fBigStep;
	const auto fInverted = refSearch.fInverted;
	const auto ullChunks = refSearch.ullChunks;
	auto ullChunkSize = refSearch.ullChunkSize;
	auto ullChunkMaxOffset = refSearch.ullChunkMaxOffset;
	auto ullOffsetSearch = refSearch.ullStartFrom;

	for (auto itChunk = 0ULL; itChunk < ullChunks; ++itChunk) {
		const auto spnData = pHexCtrl->GetData({ ullOffsetSearch, ullChunkSize });
		assert(!spnData.empty());
		assert(spnData.size() >= ullChunkSize);

		//Next cycle offset is "iVecSize - 1", to get the data that crosses the vector.
		for (auto ullOffsetData = 0ULL; ullOffsetData <= ullChunkMaxOffset; ullOffsetData += (iVecSize - 1)) {
			if ((ullOffsetData + iVecSize) <= ullChunkMaxOffset) {
				if constexpr (!stType.fInverted) { //SIMD forward not inverted.
					if (const auto iRes = MemCmpVecEQByte2<stType.eVecSize>(spnData.data() + ullOffsetData,
						*reinterpret_cast<const std::uint16_t*>(pDataSearch));
						iRes < iVecSize) {
						return { ullOffsetSearch + ullOffsetData + iRes, true, false };
					}
				}
				else if constexpr (stType.fInverted) { //SIMD forward inverted.
					if (const auto iRes = MemCmpVecNEQByte2<stType.eVecSize>(spnData.data() + ullOffsetData,
						*reinterpret_cast<const std::uint16_t*>(pDataSearch));
						iRes < iVecSize) {
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

			if constexpr (stType.fDlgProg) {
				if (pDlgProg->IsCanceled()) {
					return { { }, false, true };
				}
				pDlgProg->SetCurrent(ullOffsetSearch + ullOffsetData);
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

		if (ullOffsetSearch + ullChunkSize > ullOffsetSentinel) {
			ullChunkSize = ullOffsetSentinel - ullOffsetSearch;
			ullChunkMaxOffset = ullChunkSize - nSizeSearch;
		}
	}

	return { };
}

template<CHexDlgSearch::SEARCHTYPE stType>
auto CHexDlgSearch::SearchFuncVecFwdByte4(const SEARCHFUNCDATA& refSearch)->FINDRESULT
{
	//Members locality is important for the best performance of the tight search loop below.
	constexpr auto iVecSize = static_cast<int>(stType.eVecSize);
	const auto ullOffsetSentinel = refSearch.ullRngEnd + 1;
	const auto ullStep = refSearch.ullStep;
	const auto pHexCtrl = refSearch.pHexCtrl;
	const auto pDlgProg = refSearch.pDlgProg;
	const auto pDataSearch = refSearch.spnFind.data();
	const auto nSizeSearch = refSearch.spnFind.size();
	const auto ullEnd = ullOffsetSentinel - nSizeSearch;
	const auto fBigStep = refSearch.fBigStep;
	const auto fInverted = refSearch.fInverted;
	const auto ullChunks = refSearch.ullChunks;
	auto ullChunkSize = refSearch.ullChunkSize;
	auto ullChunkMaxOffset = refSearch.ullChunkMaxOffset;
	auto ullOffsetSearch = refSearch.ullStartFrom;

	for (auto itChunk = 0ULL; itChunk < ullChunks; ++itChunk) {
		const auto spnData = pHexCtrl->GetData({ ullOffsetSearch, ullChunkSize });
		assert(!spnData.empty());
		assert(spnData.size() >= ullChunkSize);

		//Next cycle offset is "iVecSize - 3", to get the data that crosses the vector.
		for (auto ullOffsetData = 0ULL; ullOffsetData <= ullChunkMaxOffset; ullOffsetData += (iVecSize - 3)) {
			if ((ullOffsetData + iVecSize) <= ullChunkMaxOffset) {
				if constexpr (!stType.fInverted) { //SIMD forward not inverted.
					if (const auto iRes = MemCmpVecEQByte4<stType.eVecSize>(spnData.data() + ullOffsetData,
						*reinterpret_cast<const std::uint32_t*>(pDataSearch));
						iRes < iVecSize) {
						return { ullOffsetSearch + ullOffsetData + iRes, true, false };
					}
				}
				else if constexpr (stType.fInverted) { //SIMD forward inverted.
					if (const auto iRes = MemCmpVecNEQByte4<stType.eVecSize>(spnData.data() + ullOffsetData,
						*reinterpret_cast<const std::uint32_t*>(pDataSearch));
						iRes < iVecSize) {
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

			if constexpr (stType.fDlgProg) {
				if (pDlgProg->IsCanceled()) {
					return { { }, false, true };
				}
				pDlgProg->SetCurrent(ullOffsetSearch + ullOffsetData);
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

		if (ullOffsetSearch + ullChunkSize > ullOffsetSentinel) {
			ullChunkSize = ullOffsetSentinel - ullOffsetSearch;
			ullChunkMaxOffset = ullChunkSize - nSizeSearch;
		}
	}

	return { };
}
#elif defined(_M_ARM64)  //^^^ _M_IX86 || _M_X64 / vvv _M_ARM64
template<CHexDlgSearch::SEARCHTYPE stType>
auto CHexDlgSearch::SearchFuncVecFwdByte1(const SEARCHFUNCDATA& refSearch)->FINDRESULT
{
	return CHexDlgSearch::SearchFuncFwd<stType>(refSearch);
}

template<CHexDlgSearch::SEARCHTYPE stType>
auto CHexDlgSearch::SearchFuncVecFwdByte2(const SEARCHFUNCDATA& refSearch)->FINDRESULT
{
	return CHexDlgSearch::SearchFuncFwd<stType>(refSearch);
}

template<CHexDlgSearch::SEARCHTYPE stType>
auto CHexDlgSearch::SearchFuncVecFwdByte4(const SEARCHFUNCDATA& refSearch)->FINDRESULT
{
	return CHexDlgSearch::SearchFuncFwd<stType>(refSearch);
}
#endif //^^^ _M_ARM64

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
	const auto pDlgProg = refSearch.pDlgProg;
	const auto pDataSearch = refSearch.spnFind.data();
	const auto nSizeSearch = refSearch.spnFind.size();
	const auto fBigStep = refSearch.fBigStep;
	const auto fInverted = refSearch.fInverted;
	const auto ullChunks = refSearch.ullChunks;
	auto ullChunkSize = refSearch.ullChunkSize;
	auto ullChunkMaxOffset = refSearch.ullChunkMaxOffset;
	auto ullOffsetSearch = refSearch.ullStartFrom - refSearch.ullChunkMaxOffset;

	if (ullOffsetSearch < ullEnd || ullOffsetSearch >((std::numeric_limits<ULONGLONG>::max)() - ullChunkMaxOffset)) {
		ullChunkSize = (ullStartFrom - ullEnd) + nSizeSearch;
		ullChunkMaxOffset = ullChunkSize - nSizeSearch;
		ullOffsetSearch = ullEnd;
	}

	for (auto itChunk = ullChunks; itChunk > 0; --itChunk) {
		const auto spnData = pHexCtrl->GetData({ ullOffsetSearch, ullChunkSize });
		assert(!spnData.empty());
		assert(spnData.size() >= ullChunkSize);

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

			if constexpr (stType.fDlgProg) {
				if (pDlgProg->IsCanceled()) {
					return { { }, false, true };
				}
				pDlgProg->SetCurrent(ullStartFrom - (ullOffsetSearch + llOffsetData));
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
				ullChunkSize = (ullOffsetSearch - ullEnd) + nSizeSearch;
				ullChunkMaxOffset = ullChunkSize - nSizeSearch;
				ullOffsetSearch = ullEnd;
			}
			else {
				ullOffsetSearch -= ullChunkMaxOffset;
			}
		}
	}

	return { };
}