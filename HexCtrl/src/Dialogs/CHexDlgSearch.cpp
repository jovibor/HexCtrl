/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "../../res/HexCtrlRes.h"
#include "../HexUtility.h"
#include "CHexDlgCallback.h"
#include "CHexDlgSearch.h"
#include <algorithm>
#include <bit>
#include <cassert>
#include <cwctype>
#include <format>
#include <limits>
#include <string>
#include <thread>

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

namespace HEXCTRL::INTERNAL
{
	enum class CHexDlgSearch::EMode : std::uint8_t {
		SEARCH_HEXBYTES, SEARCH_ASCII, SEARCH_UTF8, SEARCH_UTF16,
		SEARCH_INT8, SEARCH_INT16, SEARCH_INT32, SEARCH_INT64,
		SEARCH_FLOAT, SEARCH_DOUBLE, SEARCH_FILETIME
	};

	enum class CHexDlgSearch::ECmpType : std::uint16_t { //Flags for the instantiations of templated MemCmp<>.
		TYPE_CHAR_LOOP = 0x0001,
		TYPE_WCHAR_LOOP = 0x0002,
		TYPE_CASE_INSENSITIVE = 0x0004,
		TYPE_WILDCARD = 0x0008,
		TYPE_INT8 = 0x0010,
		TYPE_INT16 = 0x0020,
		TYPE_INT32 = 0x0040,
		TYPE_INT64 = 0x0080
	};

	enum class CHexDlgSearch::EMenuID : std::uint16_t {
		IDM_SEARCH_ADDBKM = 0x8000, IDM_SEARCH_SELECTALL = 0x8001, IDM_SEARCH_CLEARALL = 0x8002
	};

	struct CHexDlgSearch::SFINDRESULT {
		bool fFound { };
		bool fCanceled { }; //Search was canceled by pressing "Cancel".
		operator bool()const { return fFound; };
	};

	struct CHexDlgSearch::STHREADRUN {
		ULONGLONG ullStart { };
		ULONGLONG ullEnd { };
		ULONGLONG ullLoopChunkSize { }; //Actual data size for a `for()` loop.
		ULONGLONG ullMemToAcquire { };  //Size of a memory to acquire, to search in.
		ULONGLONG ullMemChunks { };     //How many such memory chunks, to search in.
		ULONGLONG ullSizeSentinel { };  //Maximum size that search can't cross
		CHexDlgCallback* pDlgClbk { };
		SpanCByte spnSearch { };
		bool fForward { };
		bool fBigStep { };
		bool fCanceled { };
		bool fDlgExit { };
		bool fResult { };
	};
};

BEGIN_MESSAGE_MAP(CHexDlgSearch, CDialogEx)
	ON_WM_ACTIVATE()
	ON_WM_CLOSE()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_BTN_SEARCH_F, &CHexDlgSearch::OnButtonSearchF)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_BTN_SEARCH_B, &CHexDlgSearch::OnButtonSearchB)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_BTN_FINDALL, &CHexDlgSearch::OnButtonFindAll)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_BTN_REPLACE, &CHexDlgSearch::OnButtonReplace)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_BTN_REPLACE_ALL, &CHexDlgSearch::OnButtonReplaceAll)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_CHECK_SEL, &CHexDlgSearch::OnCheckSel)
	ON_CBN_SELCHANGE(IDC_HEXCTRL_SEARCH_COMBO_MODE, &CHexDlgSearch::OnComboModeSelChange)
	ON_CBN_EDITUPDATE(IDC_HEXCTRL_SEARCH_COMBO_SEARCH, &CHexDlgSearch::UpdateSearchReplaceControls)
	ON_CBN_EDITUPDATE(IDC_HEXCTRL_SEARCH_COMBO_REPLACE, &CHexDlgSearch::UpdateSearchReplaceControls)
	ON_NOTIFY(LVN_GETDISPINFOW, IDC_HEXCTRL_SEARCH_LIST_MAIN, &CHexDlgSearch::OnListGetDispInfo)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_HEXCTRL_SEARCH_LIST_MAIN, &CHexDlgSearch::OnListItemChanged)
	ON_NOTIFY(NM_RCLICK, IDC_HEXCTRL_SEARCH_LIST_MAIN, &CHexDlgSearch::OnListRClick)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

auto CHexDlgSearch::GetDlgData()const->std::uint64_t
{
	if (!IsWindow(m_hWnd)) {
		return { };
	}

	return { };
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
	m_iDirection = fForward ? 1 : -1;
	m_fReplace = false;
	m_fAll = false;
	m_fSearchNext = true;
	Search();
}

auto CHexDlgSearch::SetDlgData(std::uint64_t /*ullData*/)->HWND
{
	if (!IsWindow(m_hWnd)) {
		Create(IDD_HEXCTRL_SEARCH, CWnd::FromHandle(m_pHexCtrl->GetWindowHandle(EHexWnd::WND_MAIN)));
	}

	return m_hWnd;
}

BOOL CHexDlgSearch::ShowWindow(int nCmdShow)
{
	if (!IsWindow(m_hWnd)) {
		Create(IDD_HEXCTRL_SEARCH, CWnd::FromHandle(m_pHexCtrl->GetWindowHandle(EHexWnd::WND_MAIN)));
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

void CHexDlgSearch::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_CHECK_SEL, m_stCheckSel);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_CHECK_WILDCARD, m_stCheckWcard);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_CHECK_BE, m_stCheckBE);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_CHECK_MATCHCASE, m_stCheckMatchC);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_COMBO_SEARCH, m_stComboSearch);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_COMBO_REPLACE, m_stComboReplace);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_COMBO_MODE, m_stComboMode);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_EDIT_START, m_stEditStart);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_EDIT_STEP, m_stEditStep);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_EDIT_LIMIT, m_stEditLimit);
}

void CHexDlgSearch::ClearList()
{
	m_pListMain->SetItemCountEx(0);
	m_vecSearchRes.clear();
}

void CHexDlgSearch::ComboSearchFill(LPCWSTR pwsz)
{
	//Insert wstring into ComboBox only if it's not already presented.
	if (m_stComboSearch.FindStringExact(0, pwsz) == CB_ERR) {
		//Keep max 50 strings in list.
		if (m_stComboSearch.GetCount() == 50) {
			m_stComboSearch.DeleteString(49);
		}
		m_stComboSearch.InsertString(0, pwsz);
	}
}

void CHexDlgSearch::ComboReplaceFill(LPCWSTR pwsz)
{
	//Insert wstring into ComboBox only if it's not already presented.
	if (m_stComboReplace.FindStringExact(0, pwsz) == CB_ERR) {
		//Keep max 50 strings in list.
		if (m_stComboReplace.GetCount() == 50) {
			m_stComboReplace.DeleteString(49);
		}
		m_stComboReplace.InsertString(0, pwsz);
	}
}

auto CHexDlgSearch::Finder(ULONGLONG& ullStart, ULONGLONG ullEnd, SpanCByte spnSearch,
	bool fForward, CHexDlgCallback* pDlgClbk, bool fDlgExit)->SFINDRESULT
{	//ullStart will keep index of found occurence, if any.

	constexpr auto uSearchSizeLimit { 256U };
	constexpr auto iSizeQuick { 1024 * 1024 * 10 }; //10MB.

	const auto nSizeSearch = spnSearch.size();
	assert(nSizeSearch <= uSearchSizeLimit);
	if (nSizeSearch > uSearchSizeLimit)
		return { false, false };

	if (ullStart + nSizeSearch > m_ullSizeSentinel)
		return { false, false };

	const auto ullSizeTotal = m_ullSizeSentinel - (fForward ? ullStart : ullEnd); //Depends on search direction.
	const auto pHexCtrl = GetHexCtrl();
	const auto ullStep = m_ullStep; //Search step.

	STHREADRUN stThread;
	stThread.ullStart = ullStart;
	stThread.ullEnd = ullEnd;
	stThread.ullSizeSentinel = m_ullSizeSentinel;
	stThread.pDlgClbk = pDlgClbk;
	stThread.fDlgExit = fDlgExit;
	stThread.fForward = fForward;
	stThread.spnSearch = spnSearch;

	if (!pHexCtrl->IsVirtual()) {
		stThread.ullMemToAcquire = ullSizeTotal;
		stThread.ullLoopChunkSize = ullSizeTotal - nSizeSearch;
		stThread.ullMemChunks = 1;
	}
	else {
		stThread.ullMemToAcquire = pHexCtrl->GetCacheSize();
		if (stThread.ullMemToAcquire > ullSizeTotal) {
			stThread.ullMemToAcquire = ullSizeTotal;
		}
		stThread.ullLoopChunkSize = stThread.ullMemToAcquire - nSizeSearch;

		if (ullStep > stThread.ullLoopChunkSize) { //For very big steps.
			stThread.ullMemChunks = ullSizeTotal > ullStep ? ullSizeTotal / ullStep
				+ ((ullSizeTotal % ullStep) ? 1 : 0) : 1;
			stThread.fBigStep = true;
		}
		else {
			stThread.ullMemChunks = ullSizeTotal > stThread.ullLoopChunkSize ? ullSizeTotal / stThread.ullLoopChunkSize
				+ ((ullSizeTotal % stThread.ullLoopChunkSize) ? 1 : 0) : 1;
		}
	}

	if (pDlgClbk == nullptr && ullSizeTotal > iSizeQuick) { //Showing "Cancel" dialog only when data > iSizeQuick
		CHexDlgCallback dlgClbk(L"Searching...", fForward ? ullStart : ullEnd, fForward ? ullEnd : ullStart);
		stThread.pDlgClbk = &dlgClbk;
		std::thread thrd(m_pfnThread, this, &stThread);
		dlgClbk.DoModal();
		thrd.join();
	}
	else {
		(this->*m_pfnThread)(&stThread);
	}

	ullStart = stThread.ullStart;

	return { stThread.fResult, stThread.fCanceled };
}

IHexCtrl* CHexDlgSearch::GetHexCtrl()const
{
	return m_pHexCtrl;
}

auto CHexDlgSearch::GetSearchMode()const->CHexDlgSearch::EMode
{
	return static_cast<EMode>(m_stComboMode.GetItemData(m_stComboMode.GetCurSel()));
}

void CHexDlgSearch::HexCtrlHighlight(const VecSpan& vecSel)
{
	const auto pHexCtrl = GetHexCtrl();
	pHexCtrl->SetSelection(vecSel, true, m_fSelection); //Highlight selection?

	if (!pHexCtrl->IsOffsetVisible(vecSel.back().ullOffset)) {
		pHexCtrl->GoToOffset(vecSel.back().ullOffset);
	}
}

void CHexDlgSearch::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if (nState == WA_INACTIVE) {
		SetLayeredWindowAttributes(0, 150, LWA_ALPHA);
	}
	else {
		SetLayeredWindowAttributes(0, 255, LWA_ALPHA);
		m_stComboSearch.SetFocus();
		UpdateSearchReplaceControls();

		const auto iChecked { GetHexCtrl()->HasSelection() ? BST_CHECKED : BST_UNCHECKED };
		m_stCheckSel.SetCheck(iChecked);
		m_stEditStart.EnableWindow(iChecked);
	}

	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
}

void CHexDlgSearch::OnButtonSearchF()
{
	m_iDirection = 1;
	m_fReplace = false;
	m_fAll = false;
	Prepare();
}

void CHexDlgSearch::OnButtonSearchB()
{
	m_iDirection = -1;
	m_fReplace = false;
	m_fAll = false;
	Prepare();
}

void CHexDlgSearch::OnButtonFindAll()
{
	m_iDirection = 1;
	m_fReplace = false;
	m_fAll = true;
	Prepare();
}

void CHexDlgSearch::OnButtonReplace()
{
	m_iDirection = 1;
	m_fReplace = true;
	m_fAll = false;
	Prepare();
}

void CHexDlgSearch::OnButtonReplaceAll()
{
	m_iDirection = 1;
	m_fReplace = true;
	m_fAll = true;
	Prepare();
}

void CHexDlgSearch::OnCancel()
{
	::SetFocus(GetHexCtrl()->GetWindowHandle(EHexWnd::WND_MAIN));

	CDialogEx::OnCancel();
}

void CHexDlgSearch::OnCheckSel()
{
	m_stEditStart.EnableWindow(m_stCheckSel.GetCheck() == BST_CHECKED ? 0 : 1);
}

void CHexDlgSearch::OnComboModeSelChange()
{
	const auto eMode = GetSearchMode();

	if (eMode != m_eSearchMode) {
		ResetSearch();
		m_eSearchMode = eMode;
	}

	BOOL fWildcard { FALSE };
	BOOL fBigEndian { FALSE };
	BOOL fMatchCase { FALSE };
	switch (eMode) {
	case EMode::SEARCH_INT16:
	case EMode::SEARCH_INT32:
	case EMode::SEARCH_INT64:
	case EMode::SEARCH_FLOAT:
	case EMode::SEARCH_DOUBLE:
	case EMode::SEARCH_FILETIME:
		fBigEndian = TRUE;
		break;
	case EMode::SEARCH_ASCII:
	case EMode::SEARCH_UTF16:
		fMatchCase = TRUE;
		fWildcard = TRUE;
		break;
	case EMode::SEARCH_HEXBYTES:
		fWildcard = TRUE;
		break;
	default:
		break;
	}
	m_stCheckWcard.EnableWindow(fWildcard);
	m_stCheckBE.EnableWindow(fBigEndian);
	m_stCheckMatchC.EnableWindow(fMatchCase);

	//Assist user with required date format.
	if (eMode == EMode::SEARCH_FILETIME) {
		const auto [dwFormat, wchSepar] = GetHexCtrl()->GetDateInfo();
		const auto wstr = GetDateFormatString(dwFormat, wchSepar);
		m_stComboSearch.SetCueBanner(wstr.data());
		m_stComboReplace.SetCueBanner(wstr.data());
	}
	else {
		m_stComboSearch.SetCueBanner(L"");
		m_stComboReplace.SetCueBanner(L"");
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
			const HEXBKM hbs { .vecSpan = { HEXSPAN { m_vecSearchRes.at(static_cast<std::size_t>(nItem)),
				m_fReplace ? m_vecReplaceData.size() : m_vecSearchData.size() } }, .wstrDesc = m_wstrTextSearch };
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

HBRUSH CHexDlgSearch::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
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

	m_stMenuList.DestroyMenu();
}

BOOL CHexDlgSearch::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	constexpr auto iTextLimit { 512 };

	m_stComboSearch.LimitText(iTextLimit);
	m_stComboReplace.LimitText(iTextLimit);

	auto iIndex = m_stComboMode.AddString(L"Hex Bytes");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_HEXBYTES));
	m_stComboMode.SetCurSel(iIndex);
	iIndex = m_stComboMode.AddString(L"Text ASCII");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_ASCII));
	iIndex = m_stComboMode.AddString(L"Text UTF-8");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_UTF8));
	iIndex = m_stComboMode.AddString(L"Text UTF-16");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_UTF16));
	iIndex = m_stComboMode.AddString(L"Int8 value");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_INT8));
	iIndex = m_stComboMode.AddString(L"Int16 value");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_INT16));
	iIndex = m_stComboMode.AddString(L"Int32 value");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_INT32));
	iIndex = m_stComboMode.AddString(L"Int64 value");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_INT64));
	iIndex = m_stComboMode.AddString(L"Float value");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_FLOAT));
	iIndex = m_stComboMode.AddString(L"Double value");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_DOUBLE));
	iIndex = m_stComboMode.AddString(L"FILETIME struct");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_FILETIME));

	m_pListMain->CreateDialogCtrl(IDC_HEXCTRL_SEARCH_LIST_MAIN, this);
	m_pListMain->SetExtendedStyle(LVS_EX_HEADERDRAGDROP);
	m_pListMain->InsertColumn(0, L"\u2116", LVCFMT_LEFT, 40);
	m_pListMain->InsertColumn(1, L"Offset", LVCFMT_LEFT, 455);

	m_stMenuList.CreatePopupMenu();
	m_stMenuList.AppendMenuW(MF_STRING, static_cast<UINT_PTR>(EMenuID::IDM_SEARCH_ADDBKM), L"Add bookmark(s)");
	m_stMenuList.AppendMenuW(MF_STRING, static_cast<UINT_PTR>(EMenuID::IDM_SEARCH_SELECTALL), L"Select All");
	m_stMenuList.AppendMenuW(MF_SEPARATOR);
	m_stMenuList.AppendMenuW(MF_STRING, static_cast<UINT_PTR>(EMenuID::IDM_SEARCH_CLEARALL), L"Clear All");

	//"Step" edit box text.
	m_stEditStep.SetWindowTextW(std::format(L"{}", m_ullStep).data());

	//"Limit search hit" edit box text.
	m_stEditLimit.SetWindowTextW(std::format(L"{}", m_dwFoundLimit).data());

	const auto hwndTipWC = CreateWindowExW(0, TOOLTIPS_CLASS, nullptr, WS_POPUP | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, nullptr, nullptr, nullptr);
	const auto hwndTipInv = CreateWindowExW(0, TOOLTIPS_CLASS, nullptr, WS_POPUP | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, nullptr, nullptr, nullptr);
	if (hwndTipWC == nullptr || hwndTipInv == nullptr)
		return FALSE;

	TTTOOLINFOW stToolInfo { };
	stToolInfo.cbSize = sizeof(TTTOOLINFOW);
	stToolInfo.hwnd = m_hWnd;
	stToolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;

	//"Wildcard" check box tooltip.
	stToolInfo.uId = reinterpret_cast<UINT_PTR>(m_stCheckWcard.m_hWnd);
	std::wstring wstrToolText { };
	//"Wildcard" tooltip text.
	wstrToolText += L"Use ";
	wstrToolText += static_cast<wchar_t>(m_uWildcard);
	wstrToolText += L" character to match any symbol, or any byte if in \"Hex Bytes\" search mode.\r\n";
	wstrToolText += L"Example:\r\n";
	wstrToolText += L"  Hex Bytes: 11?11 will match: 112211, 113311, 114411, 119711, etc...\r\n";
	wstrToolText += L"  ASCII Text: sa??le will match: sample, saAAle, saxale, saZble, etc...\r\n";
	stToolInfo.lpszText = wstrToolText.data();
	::SendMessageW(hwndTipWC, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&stToolInfo));
	::SendMessageW(hwndTipWC, TTM_SETDELAYTIME, TTDT_AUTOPOP, static_cast<LPARAM>(LOWORD(0x7FFF)));
	::SendMessageW(hwndTipWC, TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(1000));

	stToolInfo.uId = reinterpret_cast<UINT_PTR>(GetDlgItem(IDC_HEXCTRL_SEARCH_CHECK_INV)->m_hWnd);
	//"Inverted" tooltip text.
	wstrToolText = L"Search for the non-matching occurences.\r\nThat is everything that doesn't match search conditions.";
	stToolInfo.lpszText = wstrToolText.data();
	::SendMessageW(hwndTipInv, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&stToolInfo));
	::SendMessageW(hwndTipInv, TTM_SETDELAYTIME, TTDT_AUTOPOP, static_cast<LPARAM>(LOWORD(0x7FFF)));
	::SendMessageW(hwndTipInv, TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(1000));

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
	m_stMenuList.EnableMenuItem(static_cast<UINT>(EMenuID::IDM_SEARCH_ADDBKM), (fEnabled ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
	m_stMenuList.EnableMenuItem(static_cast<UINT>(EMenuID::IDM_SEARCH_SELECTALL), (fEnabled ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
	m_stMenuList.EnableMenuItem(static_cast<UINT>(EMenuID::IDM_SEARCH_CLEARALL), (fEnabled ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);

	POINT pt;
	GetCursorPos(&pt);
	m_stMenuList.TrackPopupMenuEx(TPM_LEFTALIGN, pt.x, pt.y, this, nullptr);
}

void CHexDlgSearch::OnOK()
{
	OnButtonSearchF();
}

void CHexDlgSearch::Prepare()
{
	const auto* const pHexCtrl = GetHexCtrl();
	const auto ullDataSize = pHexCtrl->GetDataSize();
	m_fWildcard = m_stCheckWcard.GetCheck() == BST_CHECKED;
	m_fSelection = m_stCheckSel.GetCheck() == BST_CHECKED;
	m_fBigEndian = m_stCheckBE.GetCheck() == BST_CHECKED;
	m_fMatchCase = m_stCheckMatchC.GetCheck() == BST_CHECKED;
	m_fInverted = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_SEARCH_CHECK_INV))->GetCheck() == BST_CHECKED;

	//"Search" text.
	CStringW wstrTextSearch;
	m_stComboSearch.GetWindowTextW(wstrTextSearch);
	if (wstrTextSearch.IsEmpty())
		return;

	bool fNewSearchStr { false };
	if (wstrTextSearch.Compare(m_wstrTextSearch.data()) != 0) {
		ResetSearch();
		m_wstrTextSearch = wstrTextSearch;
		fNewSearchStr = true;
	}
	ComboSearchFill(wstrTextSearch);

	//Start at.
	if (!m_fSelection) { //Do not use "Start at" field when in the selection.
		CStringW wstrStart;
		m_stEditStart.GetWindowTextW(wstrStart);
		if (wstrStart.IsEmpty()) {
			m_ullOffsetCurr = 0;
		}
		else if (const auto optStart = stn::StrToULL(wstrStart.GetString()); optStart) {
			if (*optStart >= ullDataSize) {
				MessageBoxW(L"\"Start search at\" offset is bigger than the data size!", L"Incorrect offset", MB_ICONERROR);
				return;
			}
			m_ullOffsetCurr = *optStart;
		}
		else
			return;
	}

	//Step.
	CStringW wstrStep;
	m_stEditStep.GetWindowTextW(wstrStep);
	if (const auto optStep = stn::StrToULL(wstrStep.GetString()); optStep) {
		m_ullStep = *optStep;
	}
	else
		return;

	//Limit.
	CStringW wstrLimit;
	m_stEditLimit.GetWindowTextW(wstrLimit);
	if (const auto optLimit = stn::StrToUInt(wstrLimit.GetString()); optLimit) {
		m_dwFoundLimit = *optLimit;
	}
	else
		return;

	if (m_fReplace) {
		wchar_t warrReplace[64];
		m_stComboReplace.GetWindowTextW(warrReplace, static_cast<int>(std::size(warrReplace)));
		m_wstrTextReplace = warrReplace;
		if (m_wstrTextReplace.empty()) {
			MessageBoxW(m_pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
			m_stComboReplace.SetFocus();
			return;
		}
		ComboReplaceFill(warrReplace);
	}
	m_stComboSearch.SetFocus();

	bool fSuccess { false };
	switch (GetSearchMode()) {
	case EMode::SEARCH_HEXBYTES:
		fSuccess = PrepareHexBytes();
		break;
	case EMode::SEARCH_ASCII:
		fSuccess = PrepareTextASCII();
		break;
	case EMode::SEARCH_UTF8:
		fSuccess = PrepareTextUTF8();
		break;
	case EMode::SEARCH_UTF16:
		fSuccess = PrepareTextUTF16();
		break;
	case EMode::SEARCH_INT8:
		fSuccess = PrepareINT8();
		break;
	case EMode::SEARCH_INT16:
		fSuccess = PrepareINT16();
		break;
	case EMode::SEARCH_INT32:
		fSuccess = PrepareINT32();
		break;
	case EMode::SEARCH_INT64:
		fSuccess = PrepareINT64();
		break;
	case EMode::SEARCH_FLOAT:
		fSuccess = PrepareFloat();
		break;
	case EMode::SEARCH_DOUBLE:
		fSuccess = PrepareDouble();
		break;
	case EMode::SEARCH_FILETIME:
		fSuccess = PrepareFILETIME();
		break;
	}
	if (!fSuccess)
		return;

	if (m_fSelection) { //Search in selection.
		const auto vecSel = pHexCtrl->GetSelection();
		if (vecSel.empty()) //No selection.
			return;

		auto& refFront = vecSel.front();

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

		m_ullBoundBegin = refFront.ullOffset;
		m_ullBoundEnd = m_ullBoundBegin + ullSelSize - m_vecSearchData.size();
		m_ullSizeSentinel = m_ullBoundBegin + ullSelSize;
	}
	else { //Search in a whole data.
		m_ullBoundBegin = 0;
		m_ullBoundEnd = ullDataSize - m_vecSearchData.size();
		m_ullSizeSentinel = ullDataSize;
	}

	if (m_ullOffsetCurr + m_vecSearchData.size() > ullDataSize || m_ullOffsetCurr + m_vecReplaceData.size() > ullDataSize) {
		m_ullOffsetCurr = 0;
		m_fSecondMatch = false;
		return;
	}

	if (m_fReplaceWarn && m_fReplace && (m_vecReplaceData.size() > m_vecSearchData.size())) {
		static constexpr auto wstrReplaceWarning { L"The replacing string is longer than searching string.\r\n"
			"Do you want to overwrite the bytes following search occurrence?\r\nChoosing \"No\" will cancel search." };
		if (IDNO == MessageBoxW(wstrReplaceWarning, L"Warning", MB_YESNO | MB_ICONQUESTION | MB_TOPMOST))
			return;

		m_fReplaceWarn = false;
	}

	Search();
	SetActiveWindow();
}

bool CHexDlgSearch::PrepareHexBytes()
{
	static constexpr auto pwszWrongInput { L"Unacceptable input character.\r\nAllowed characters are: 0123456789AaBbCcDdEeFf" };
	m_fMatchCase = false;
	auto optData = NumStrToHex(m_wstrTextSearch, m_fWildcard ? static_cast<char>(m_uWildcard) : 0);
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
			m_stComboReplace.SetFocus();
			return false;
		}
		m_vecReplaceData = RangeToVecBytes(*optDataRepl);
	}

	using enum ECmpType;
	if (!m_fWildcard) {
		m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_CHAR_LOOP)>;
	}
	else {
		m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_CHAR_LOOP) |
			static_cast<std::uint16_t>(TYPE_WILDCARD)>;
	}

	return true;
}

bool CHexDlgSearch::PrepareTextASCII()
{
	auto strSearch = WstrToStr(m_wstrTextSearch, CP_ACP); //Convert to the system default Windows ANSI code page.
	if (!m_fMatchCase) { //Make the string lowercase.
		std::transform(strSearch.begin(), strSearch.end(), strSearch.begin(),
			[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
	}

	m_vecSearchData = RangeToVecBytes(strSearch);
	m_vecReplaceData = RangeToVecBytes(WstrToStr(m_wstrTextReplace, CP_ACP));

	using enum ECmpType;
	if (m_fMatchCase && !m_fWildcard) {
		m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_CHAR_LOOP)>;
	}
	else if (m_fMatchCase && m_fWildcard) {
		m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_CHAR_LOOP)
			| static_cast<std::uint16_t>(TYPE_WILDCARD)>;
	}
	else if (!m_fMatchCase && m_fWildcard) {
		m_pfnThread = &CHexDlgSearch::ThreadRun< static_cast<std::uint16_t>(TYPE_CHAR_LOOP) |
			static_cast<std::uint16_t>(TYPE_CASE_INSENSITIVE) | static_cast<std::uint16_t>(TYPE_WILDCARD)>;
	}
	else if (!m_fMatchCase && !m_fWildcard) {
		m_pfnThread = &CHexDlgSearch::ThreadRun< static_cast<std::uint16_t>(TYPE_CHAR_LOOP)
			| static_cast<std::uint16_t>(TYPE_CASE_INSENSITIVE)>;
	}

	return true;
}

bool CHexDlgSearch::PrepareTextUTF16()
{
	auto m_wstrSearch = m_wstrTextSearch;
	if (!m_fMatchCase) { //Make the string lowercase.
		std::transform(m_wstrSearch.begin(), m_wstrSearch.end(), m_wstrSearch.begin(), std::towlower);
	}

	m_vecSearchData = RangeToVecBytes(m_wstrSearch);
	m_vecReplaceData = RangeToVecBytes(m_wstrTextReplace);

	using enum ECmpType;
	if (m_fMatchCase && !m_fWildcard) {
		m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_WCHAR_LOOP)>;
	}
	else if (m_fMatchCase && m_fWildcard) {
		m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_WCHAR_LOOP)
			| static_cast<std::uint16_t>(TYPE_WILDCARD)>;
	}
	else if (!m_fMatchCase && m_fWildcard) {
		m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_WCHAR_LOOP)
			| static_cast<std::uint16_t>(TYPE_CASE_INSENSITIVE) | static_cast<std::uint16_t>(TYPE_WILDCARD)>;
	}
	else if (!m_fMatchCase && !m_fWildcard) {
		m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_WCHAR_LOOP)
			| static_cast<std::uint16_t>(TYPE_CASE_INSENSITIVE)>;
	}

	return true;
}

bool CHexDlgSearch::PrepareTextUTF8()
{
	m_fMatchCase = false;
	m_fWildcard = false;
	m_vecSearchData = RangeToVecBytes(WstrToStr(m_wstrTextSearch, CP_UTF8)); //Convert to UTF-8 string.
	m_vecReplaceData = RangeToVecBytes(WstrToStr(m_wstrTextReplace, CP_UTF8));

	using enum ECmpType;
	m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_CHAR_LOOP)>;

	return true;
}

bool CHexDlgSearch::PrepareINT8()
{
	m_fMatchCase = false;
	m_fWildcard = false;
	const auto optData = stn::StrToUChar(m_wstrTextSearch);
	if (!optData) {
		MessageBoxW(m_pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}
	m_vecSearchData = RangeToVecBytes(*optData);

	if (m_fReplace) {
		const auto optDataRepl = stn::StrToUChar(m_wstrTextReplace);
		if (!optDataRepl) {
			MessageBoxW(m_pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		m_vecReplaceData = RangeToVecBytes(*optDataRepl);
	}

	using enum ECmpType;
	m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_INT8)>;

	return true;
}

bool CHexDlgSearch::PrepareINT16()
{
	m_fMatchCase = false;
	m_fWildcard = false;
	const auto optData = stn::StrToUShort(m_wstrTextSearch);
	if (!optData) {
		MessageBoxW(m_pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}
	unsigned short wData = *optData;
	unsigned short wDataRep { 0 };

	if (m_fReplace) {
		const auto optDataRepl = stn::StrToUShort(m_wstrTextReplace);
		if (!optDataRepl) {
			MessageBoxW(m_pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		wDataRep = *optDataRepl;
	}

	if (m_fBigEndian) {
		wData = ByteSwap(wData);
		wDataRep = ByteSwap(wDataRep);
	}

	m_vecSearchData = RangeToVecBytes(wData);
	m_vecReplaceData = RangeToVecBytes(wDataRep);

	using enum ECmpType;
	m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_INT16)>;

	return true;
}

bool CHexDlgSearch::PrepareINT32()
{
	m_fMatchCase = false;
	m_fWildcard = false;
	const auto optData = stn::StrToUInt(m_wstrTextSearch);
	if (!optData) {
		MessageBoxW(m_pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}
	unsigned int dwData = *optData;
	unsigned int dwDataRep { 0 };

	if (m_fReplace) {
		const auto optDataRepl = stn::StrToUInt(m_wstrTextReplace);
		if (!optDataRepl) {
			MessageBoxW(m_pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		dwDataRep = *optDataRepl;
	}

	if (m_fBigEndian) {
		dwData = ByteSwap(dwData);
		dwDataRep = ByteSwap(dwDataRep);
	}

	m_vecSearchData = RangeToVecBytes(dwData);
	m_vecReplaceData = RangeToVecBytes(dwDataRep);

	using enum ECmpType;
	m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_INT32)>;

	return true;
}

bool CHexDlgSearch::PrepareINT64()
{
	m_fMatchCase = false;
	m_fWildcard = false;
	const auto optData = stn::StrToULL(m_wstrTextSearch);
	if (!optData) {
		MessageBoxW(m_pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}
	unsigned long long ullData = *optData;
	unsigned long long ullDataRep { 0 };

	if (m_fReplace) {
		const auto optDataRepl = stn::StrToULL(m_wstrTextReplace);
		if (!optDataRepl) {
			MessageBoxW(m_pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		ullDataRep = *optDataRepl;
	}

	if (m_fBigEndian) {
		ullData = ByteSwap(ullData);
		ullDataRep = ByteSwap(ullDataRep);
	}

	m_vecSearchData = RangeToVecBytes(ullData);
	m_vecReplaceData = RangeToVecBytes(ullDataRep);
	using enum ECmpType;
	m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_INT64)>;

	return true;
}

bool CHexDlgSearch::PrepareFloat()
{
	m_fMatchCase = false;
	m_fWildcard = false;
	const auto optData = stn::StrToFloat(m_wstrTextSearch);
	if (!optData) {
		MessageBoxW(m_pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}
	float flData = *optData;
	float flDataRep { 0.0F };

	if (m_fReplace) {
		const auto optDataRepl = stn::StrToFloat(m_wstrTextReplace);
		if (!optDataRepl) {
			MessageBoxW(m_pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		flDataRep = *optDataRepl;
	}

	if (m_fBigEndian) {
		flData = ByteSwap(flData);
		flDataRep = ByteSwap(flDataRep);
	}

	m_vecSearchData = RangeToVecBytes(flData);
	m_vecReplaceData = RangeToVecBytes(flDataRep);

	using enum ECmpType;
	m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_INT32)>;

	return true;
}

bool CHexDlgSearch::PrepareDouble()
{
	m_fMatchCase = false;
	m_fWildcard = false;
	const auto optData = stn::StrToDouble(m_wstrTextSearch);
	if (!optData) {
		MessageBoxW(m_pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}
	double dblData = *optData;
	double dblDataRep { 0.0F };

	if (m_fReplace) {
		const auto optDataRepl = stn::StrToDouble(m_wstrTextReplace);
		if (!optDataRepl) {
			MessageBoxW(m_pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		dblDataRep = *optDataRepl;
	}

	if (m_fBigEndian) {
		dblData = ByteSwap(dblData);
		dblDataRep = ByteSwap(dblDataRep);
	}

	m_vecSearchData = RangeToVecBytes(dblData);
	m_vecReplaceData = RangeToVecBytes(dblDataRep);

	using enum ECmpType;
	m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_INT64)>;

	return true;
}

bool CHexDlgSearch::PrepareFILETIME()
{
	m_fMatchCase = false;
	m_fWildcard = false;
	const auto [dwFormat, wchSepar] = GetHexCtrl()->GetDateInfo();
	const std::wstring wstrErr = L"Wrong FILETIME format.\r\nA correct format is: " + GetDateFormatString(dwFormat, wchSepar);
	const auto optFTSearch = StringToFileTime(m_wstrTextSearch, dwFormat);
	if (!optFTSearch) {
		MessageBoxW(wstrErr.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}
	FILETIME stFTSearch = *optFTSearch;
	FILETIME stFTReplace { };

	if (m_fReplace) {
		const auto optFTReplace = StringToFileTime(m_wstrTextReplace, dwFormat);
		if (!optFTReplace) {
			MessageBoxW(wstrErr.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		stFTReplace = *optFTReplace;
	}

	if (m_fBigEndian) {
		stFTSearch.dwLowDateTime = ByteSwap(stFTSearch.dwLowDateTime);
		stFTSearch.dwHighDateTime = ByteSwap(stFTSearch.dwHighDateTime);
		stFTReplace.dwLowDateTime = ByteSwap(stFTReplace.dwLowDateTime);
		stFTReplace.dwHighDateTime = ByteSwap(stFTReplace.dwHighDateTime);
	}

	m_vecSearchData = RangeToVecBytes(stFTSearch);
	m_vecReplaceData = RangeToVecBytes(stFTReplace);

	using enum ECmpType;
	m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_INT64)>;

	return true;
}

void CHexDlgSearch::Replace(ULONGLONG ullIndex, const SpanCByte spnReplace)const
{
	GetHexCtrl()->ModifyData({ .enModifyMode { EHexModifyMode::MODIFY_ONCE }, .spnData { spnReplace },
		.vecSpan { { ullIndex, spnReplace.size() } } });
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
	const auto pHexCtrl = GetHexCtrl();
	if (m_wstrTextSearch.empty() || !pHexCtrl->IsDataSet() || m_ullOffsetCurr >= pHexCtrl->GetDataSize())
		return;

	m_fFound = false;
	auto ullUntil = m_ullBoundEnd;
	auto ullOffsetFound { 0ULL };
	SFINDRESULT stFind;

	const auto lmbFindForward = [&]() {
		if (stFind = Finder(m_ullOffsetCurr, ullUntil, m_vecSearchData); stFind.fFound) {
			m_fFound = true;
			m_fSecondMatch = true;
			m_dwCount++;
			ullOffsetFound = m_ullOffsetCurr;
		}
		if (!m_fFound) {
			m_iWrap = 1;

			if (m_fSecondMatch && !stFind.fCanceled) {
				m_ullOffsetCurr = m_ullBoundBegin; //Starting from the beginning.
				if (Finder(m_ullOffsetCurr, ullUntil, m_vecSearchData)) {
					m_fFound = true;
					m_fDoCount = true;
					m_dwCount = 1;
					ullOffsetFound = m_ullOffsetCurr;
				}
			}
		}
		};
	const auto lmbFindBackward = [&]() {
		ullUntil = m_ullBoundBegin;
		if (m_fSecondMatch && m_ullOffsetCurr - m_ullStep < m_ullOffsetCurr) {
			m_ullOffsetCurr -= m_ullStep;
			if (stFind = Finder(m_ullOffsetCurr, ullUntil, m_vecSearchData, false);
				stFind.fFound) {
				m_fFound = true;
				m_dwCount--;
				ullOffsetFound = m_ullOffsetCurr;
			}
		}
		if (!m_fFound) {
			m_iWrap = -1;

			if (!stFind.fCanceled) {
				m_ullOffsetCurr = m_ullBoundEnd;
				if (Finder(m_ullOffsetCurr, ullUntil, m_vecSearchData, false)) {
					m_fFound = true;
					m_fSecondMatch = true;
					m_fDoCount = false;
					m_dwCount = 1;
					ullOffsetFound = m_ullOffsetCurr;
				}
			}
		}
		};

	pHexCtrl->SetRedraw(false);

	if (m_fReplace) { //Replace
		if (m_fAll) { //Replace All
			auto ullStart = m_ullOffsetCurr;
			const auto lmbReplaceAll = [&](CHexDlgCallback* pDlgClbk) {
				if (pDlgClbk == nullptr)
					return;

				while (true) {
					if (Finder(ullStart, ullUntil, m_vecSearchData, true, pDlgClbk, false)) {
						Replace(ullStart, m_vecReplaceData);
						ullStart += m_vecReplaceData.size() <= m_ullStep ? m_ullStep : m_vecReplaceData.size();
						m_fFound = true;
						++m_dwReplaced;
						if (ullStart > ullUntil || m_dwReplaced >= m_dwFoundLimit || pDlgClbk->IsCanceled())
							break;
					}
					else
						break;
				}

				pDlgClbk->ExitDlg();
				};

			CHexDlgCallback dlgClbk(L"Replacing...", ullStart, ullUntil, this);
			std::thread thrd(lmbReplaceAll, &dlgClbk);
			dlgClbk.DoModal();
			thrd.join();
		}
		else if (m_iDirection == 1) { //Forward only direction.
			lmbFindForward();
			if (m_fFound) {
				Replace(m_ullOffsetCurr, m_vecReplaceData);
				//Increase next search step to replaced or m_ullStep amount.
				m_ullOffsetCurr += m_vecReplaceData.size() <= m_ullStep ? m_ullStep : m_vecReplaceData.size();
				++m_dwReplaced;
			}
		}
	}
	else { //Search.
		if (m_fAll) {
			ClearList(); //Clearing all results.
			m_dwCount = 0;
			auto ullStart = m_ullOffsetCurr;
			const auto lmbSearchAll = [&](CHexDlgCallback* pDlgClbk) {
				if (pDlgClbk == nullptr)
					return;

				while (true) {
					if (Finder(ullStart, ullUntil, m_vecSearchData, true, pDlgClbk, false)) {
						m_vecSearchRes.emplace_back(ullStart); //Filling the vector of Found occurences.
						ullStart += m_ullStep;
						m_fFound = true;
						++m_dwCount;

						if (ullStart > ullUntil || m_dwCount >= m_dwFoundLimit || pDlgClbk->IsCanceled()) //Find no more than m_dwFoundLimit.
							break;
					}
					else
						break;
				}

				pDlgClbk->ExitDlg();
				m_pListMain->SetItemCountEx(static_cast<int>(m_dwCount));
				};

			CHexDlgCallback dlgClbk(L"Searching...", ullStart, ullUntil, this);
			std::thread thrd(lmbSearchAll, &dlgClbk);
			dlgClbk.DoModal();
			thrd.join();
		}
		else {
			if (m_iDirection == 1) { //Forward direction.
				//If previously anything was found we increase m_ullOffsetCurr by m_ullStep.
				m_ullOffsetCurr = m_fSecondMatch ? m_ullOffsetCurr + m_ullStep : m_ullOffsetCurr;
				lmbFindForward();
			}
			else if (m_iDirection == -1) { //Backward direction.
				lmbFindBackward();
			}
		}
	}

	pHexCtrl->SetRedraw(true);

	std::wstring wstrInfo;
	if (m_fFound) {
		if (m_fAll) {
			if (m_fReplace) {
				wstrInfo = std::format(L"{} occurrence(s) replaced.", m_dwReplaced);
				m_dwReplaced = 0;
				pHexCtrl->Redraw(); //Redraw in case of Replace all.
			}
			else {
				wstrInfo = std::format(L"Found {} occurrences.", m_dwCount);
				m_dwCount = 0;
			}
		}
		else {
			if (m_fDoCount) {
				wstrInfo = std::format(L"Found occurrence \u2116 {} from the beginning.", m_dwCount);
			}
			else {
				wstrInfo = L"Search found occurrence.";
			}

			HexCtrlHighlight({ { ullOffsetFound, m_fReplace ? m_vecReplaceData.size() : m_vecSearchData.size() } });
			AddToList(ullOffsetFound);
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
	m_stEditStart.SetWindowTextW(std::format(L"0x{:X}", ullOffset).data());
}

template<std::uint16_t uCmpType>
void CHexDlgSearch::ThreadRun(STHREADRUN* pStThread)
{
	const auto pDataSearch = pStThread->spnSearch.data();
	const auto nSizeSearch = pStThread->spnSearch.size();
	const auto pHexCtrl = GetHexCtrl();
	const auto llStep = static_cast<LONGLONG>(m_ullStep); //Search step, must be signed here.
	constexpr auto LOOP_UNROLL_SIZE = 4; //How many comparisons we do at one loop step.

	if (pStThread->fForward) { //Forward search.
		auto ullOffsetSearch = pStThread->ullStart;
		for (auto iterChunk = 0ULL; iterChunk < pStThread->ullMemChunks; ++iterChunk) {
			const auto spnData = pHexCtrl->GetData({ ullOffsetSearch, pStThread->ullMemToAcquire });
			assert(!spnData.empty());

			for (auto iterData = 0ULL; iterData <= pStThread->ullLoopChunkSize; iterData += llStep * LOOP_UNROLL_SIZE) {
				//Unrolling the loop, making LOOP_UNROLL_SIZE comparisons at a time.
				if (MemCmp<uCmpType>(spnData.data() + iterData, pDataSearch, nSizeSearch) == !m_fInverted) {
					pStThread->ullStart = ullOffsetSearch + iterData;
					pStThread->fResult = true;
					goto FOUND;
				}
				if ((iterData + llStep <= pStThread->ullLoopChunkSize)
					&& (MemCmp<uCmpType>(spnData.data() + iterData + llStep, pDataSearch, nSizeSearch) == !m_fInverted)) {
					pStThread->ullStart = ullOffsetSearch + iterData + llStep;
					pStThread->fResult = true;
					goto FOUND;
				}
				if ((iterData + llStep * 2 <= pStThread->ullLoopChunkSize)
					&& (MemCmp<uCmpType>(spnData.data() + iterData + llStep * 2, pDataSearch, nSizeSearch) == !m_fInverted)) {
					pStThread->ullStart = ullOffsetSearch + iterData + llStep * 2;
					pStThread->fResult = true;
					goto FOUND;
				}
				if ((iterData + llStep * 3 <= pStThread->ullLoopChunkSize)
					&& (MemCmp<uCmpType>(spnData.data() + iterData + llStep * 3, pDataSearch, nSizeSearch) == !m_fInverted)) {
					pStThread->ullStart = ullOffsetSearch + iterData + llStep * 3;
					pStThread->fResult = true;
					goto FOUND;
				}

				if (pStThread->pDlgClbk != nullptr) {
					if (pStThread->pDlgClbk->IsCanceled()) {
						pStThread->fCanceled = true;
						return; //Breaking out from all level loops.
					}
					pStThread->pDlgClbk->SetProgress(ullOffsetSearch + iterData);
				}
			}

			if (pStThread->fBigStep) {
				if ((ullOffsetSearch + llStep) > pStThread->ullEnd)
					break; //Upper bound reached.

				ullOffsetSearch += llStep;
			}
			else {
				ullOffsetSearch += pStThread->ullLoopChunkSize;
			}

			if (ullOffsetSearch + pStThread->ullMemToAcquire > pStThread->ullSizeSentinel) {
				pStThread->ullMemToAcquire = pStThread->ullSizeSentinel - ullOffsetSearch;
				pStThread->ullLoopChunkSize = pStThread->ullMemToAcquire - nSizeSearch;
			}
		}
	}
	else { //Backward search.
		ULONGLONG ullOffsetSearch;
		if ((pStThread->ullStart - pStThread->ullLoopChunkSize) < pStThread->ullEnd
			|| ((pStThread->ullStart - pStThread->ullLoopChunkSize) >
				((std::numeric_limits<ULONGLONG>::max)() - pStThread->ullLoopChunkSize))) {
			pStThread->ullMemToAcquire = (pStThread->ullStart - pStThread->ullEnd) + nSizeSearch;
			pStThread->ullLoopChunkSize = pStThread->ullMemToAcquire - nSizeSearch;
			ullOffsetSearch = pStThread->ullEnd;
		}
		else {
			ullOffsetSearch = pStThread->ullStart - pStThread->ullLoopChunkSize;
		}

		for (auto iterChunk = pStThread->ullMemChunks; iterChunk > 0; --iterChunk) {
			const auto spnData = pHexCtrl->GetData({ ullOffsetSearch, pStThread->ullMemToAcquire });
			assert(!spnData.empty());

			for (auto iterData = static_cast<LONGLONG>(pStThread->ullLoopChunkSize); iterData >= 0;
				iterData -= llStep * LOOP_UNROLL_SIZE) { //iterData might be negative.
				if (MemCmp<uCmpType>(spnData.data() + iterData, pDataSearch, nSizeSearch) == !m_fInverted) {
					pStThread->ullStart = ullOffsetSearch + iterData;
					pStThread->fResult = true;
					goto FOUND;
				}
				if ((iterData - llStep >= 0)
					&& (MemCmp<uCmpType>(spnData.data() + (iterData - llStep), pDataSearch, nSizeSearch) == !m_fInverted)) {
					pStThread->ullStart = ullOffsetSearch + iterData - llStep;
					pStThread->fResult = true;
					goto FOUND;
				}
				if ((iterData - llStep * 2 >= 0)
					&& (MemCmp<uCmpType>(spnData.data() + (iterData - llStep * 2), pDataSearch, nSizeSearch) == !m_fInverted)) {
					pStThread->ullStart = ullOffsetSearch + iterData - llStep * 2;
					pStThread->fResult = true;
					goto FOUND;
				}
				if ((iterData - llStep * 3 >= 0)
					&& (MemCmp<uCmpType>(spnData.data() + (iterData - llStep * 3), pDataSearch, nSizeSearch) == !m_fInverted)) {
					pStThread->ullStart = ullOffsetSearch + iterData - llStep * 3;
					pStThread->fResult = true;
					goto FOUND;
				}

				if (pStThread->pDlgClbk != nullptr) {
					if (pStThread->pDlgClbk->IsCanceled()) {
						pStThread->fCanceled = true;
						return; //Breaking out from all level loops.
					}
					pStThread->pDlgClbk->SetProgress(pStThread->ullStart - (ullOffsetSearch + iterData));
				}
			}

			if (pStThread->fBigStep) {
				if ((ullOffsetSearch - llStep) < pStThread->ullEnd
					|| (ullOffsetSearch - llStep) > ((std::numeric_limits<ULONGLONG>::max)() - llStep))
					break; //Lower bound reached.

				ullOffsetSearch -= pStThread->ullLoopChunkSize;
			}
			else {
				if ((ullOffsetSearch - pStThread->ullLoopChunkSize) < pStThread->ullEnd
					|| ((ullOffsetSearch - pStThread->ullLoopChunkSize) >
						((std::numeric_limits<ULONGLONG>::max)() - pStThread->ullLoopChunkSize))) {
					pStThread->ullMemToAcquire = (ullOffsetSearch - pStThread->ullEnd) + nSizeSearch;
					pStThread->ullLoopChunkSize = pStThread->ullMemToAcquire - nSizeSearch;
					ullOffsetSearch = pStThread->ullEnd;
				}
				else {
					ullOffsetSearch -= pStThread->ullLoopChunkSize;
				}
			}
		}
	}

FOUND:
	if (pStThread->pDlgClbk != nullptr) {
		if (pStThread->fDlgExit) {
			pStThread->pDlgClbk->ExitDlg();
		}
		else {
			//If MemCmp returns true at the very first hit, we don't reach the SetProgress in the main loop.
			//It might be the case for the "Find/Replace All" for example.
			//If the data all the same (e.g. 0xCDCDCDCD...) then MemCmp would return true every single step.
			pStThread->pDlgClbk->SetProgress(pStThread->ullStart);
		}
	}
}

void CHexDlgSearch::UpdateSearchReplaceControls()
{
	const auto* const pHexCtrl = GetHexCtrl();
	if (!pHexCtrl->IsCreated() || !pHexCtrl->IsDataSet())
		return;

	const auto fMutable = pHexCtrl->IsMutable();
	m_stComboReplace.EnableWindow(fMutable);

	CStringW wstrTextSearch;
	m_stComboSearch.GetWindowTextW(wstrTextSearch);
	const auto fSearchEnabled = !wstrTextSearch.IsEmpty();

	CStringW wstrTextReplace;
	m_stComboReplace.GetWindowTextW(wstrTextReplace);
	const auto fReplaceEnabled = fMutable && fSearchEnabled && !wstrTextReplace.IsEmpty();

	GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_SEARCH_F)->EnableWindow(fSearchEnabled);
	GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_SEARCH_B)->EnableWindow(fSearchEnabled);
	GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_FINDALL)->EnableWindow(fSearchEnabled);
	GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_REPLACE)->EnableWindow(fReplaceEnabled);
	GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_REPLACE_ALL)->EnableWindow(fReplaceEnabled);
}

template<std::uint16_t uCmpType>
bool CHexDlgSearch::MemCmp(const std::byte* pBuf1, const std::byte* pBuf2, std::size_t nSize)
{
	using enum ECmpType;
	if constexpr ((uCmpType & static_cast<std::uint16_t>(TYPE_INT8)) > 0) {
		return *reinterpret_cast<const std::uint8_t*>(pBuf1) == *reinterpret_cast<const std::uint8_t*>(pBuf2);
	}
	else if constexpr ((uCmpType & static_cast<std::uint16_t>(TYPE_INT16)) > 0) {
		return *reinterpret_cast<const std::uint16_t*>(pBuf1) == *reinterpret_cast<const std::uint16_t*>(pBuf2);
	}
	else if constexpr ((uCmpType & static_cast<std::uint16_t>(TYPE_INT32)) > 0) {
		return *reinterpret_cast<const std::uint32_t*>(pBuf1) == *reinterpret_cast<const std::uint32_t*>(pBuf2);
	}
	else if constexpr ((uCmpType & static_cast<std::uint16_t>(TYPE_INT64)) > 0) {
		return *reinterpret_cast<const std::uint64_t*>(pBuf1) == *reinterpret_cast<const std::uint64_t*>(pBuf2);
	}
	else if constexpr ((uCmpType & static_cast<std::uint16_t>(TYPE_CHAR_LOOP)) > 0) {
		for (std::size_t i { 0 }; i < nSize; ++i, ++pBuf1, ++pBuf2) {
			if constexpr ((uCmpType & static_cast<std::uint16_t>(TYPE_WILDCARD)) > 0) {
				if (*pBuf2 == m_uWildcard) //Checking for wildcard match.
					continue;
			}

			if constexpr ((uCmpType & static_cast<std::uint16_t>(TYPE_CASE_INSENSITIVE)) > 0) {
				auto ch = static_cast<char>(*pBuf1);
				if (ch >= 0x41 && ch <= 0x5A) { //IsUpper.
					ch += 32;
				}

				if (ch != static_cast<char>(*pBuf2))
					return false;
			}
			else {
				if (*pBuf1 != *pBuf2)
					return false;
			}
		}
		return true;
	}
	else if constexpr ((uCmpType & static_cast<std::uint16_t>(TYPE_WCHAR_LOOP)) > 0) {
		auto pBuf1wch = reinterpret_cast<const wchar_t*>(pBuf1);
		auto pBuf2wch = reinterpret_cast<const wchar_t*>(pBuf2);
		for (std::size_t i { 0 }; i < (nSize / sizeof(wchar_t)); ++i, ++pBuf1wch, ++pBuf2wch) {
			if constexpr ((uCmpType & static_cast<std::uint16_t>(TYPE_WILDCARD)) > 0) {
				if (*pBuf2wch == static_cast<wchar_t>(m_uWildcard)) //Checking for wildcard match.
					continue;
			}

			if constexpr ((uCmpType & static_cast<std::uint16_t>(TYPE_CASE_INSENSITIVE)) > 0) {
				auto wch = *pBuf1wch;
				if (wch >= 0x41 && wch <= 0x5A) { //IsUpper.
					wch += 32;
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

std::vector<std::byte> CHexDlgSearch::RangeToVecBytes(const std::string& str)
{
	const auto pBegin = reinterpret_cast<const std::byte*>(str.data());
	const auto pEnd = pBegin + str.size();

	return std::vector<std::byte>(pBegin, pEnd);
}

std::vector<std::byte> CHexDlgSearch::RangeToVecBytes(const std::wstring& wstr)
{
	const auto pBegin = reinterpret_cast<const std::byte*>(wstr.data());
	const auto pEnd = pBegin + wstr.size() * sizeof(wchar_t);

	return std::vector<std::byte>(pBegin, pEnd);
}

template<typename T>
std::vector<std::byte> CHexDlgSearch::RangeToVecBytes(T tData)
{
	const auto pBegin = reinterpret_cast<std::byte*>(&tData);
	const auto pEnd = pBegin + sizeof(tData);

	return std::vector<std::byte>(pBegin, pEnd);
}