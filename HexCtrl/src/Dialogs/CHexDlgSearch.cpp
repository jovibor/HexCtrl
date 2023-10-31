/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
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
#include <bit>
#include <cassert>
#include <cwctype>
#include <format>
#include <limits>
#include <string>
#include <thread>

import HEXCTRL.HexUtility;

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
		ULONGLONG ullLoopChunkSize { };  //Actual data size for a `for()` loop.
		ULONGLONG ullMemToAcquire { };   //Size of a memory to acquire, to search in.
		ULONGLONG ullMemChunks { };      //How many such memory chunks, to search in.
		ULONGLONG ullOffsetSentinel { }; //The maximum offset that search can't cross.
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
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_CHECK_WILDCARD, &CHexDlgSearch::OnCheckWildcard)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_CHECK_BE, &CHexDlgSearch::OnCheckBigEndian)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_CHECK_MATCHCASE, &CHexDlgSearch::OnCheckMatchCase)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_CHECK_INV, &CHexDlgSearch::OnCheckInverted)
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
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_CHECK_SEL, m_btnSel);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_CHECK_WILDCARD, m_btnWC);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_CHECK_INV, m_btnInv);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_CHECK_BE, m_btnBE);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_CHECK_MATCHCASE, m_btnMC);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_COMBO_SEARCH, m_comboSearch);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_COMBO_REPLACE, m_comboReplace);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_COMBO_MODE, m_comboMode);
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

auto CHexDlgSearch::Finder(ULONGLONG& ullStart, ULONGLONG ullEnd, SpanCByte spnSearch,
	bool fForward, CHexDlgCallback* pDlgClbk, bool fDlgExit)->SFINDRESULT
{	//ullStart will keep index of found occurence, if any.

	constexpr auto uSearchSizeLimit { 256U };
	constexpr auto iSizeQuick { 1024 * 1024 * 10 }; //10MB.

	const auto nSizeSearch = spnSearch.size();
	assert(nSizeSearch <= uSearchSizeLimit);
	if (nSizeSearch > uSearchSizeLimit)
		return { false, false };

	if (ullStart + nSizeSearch > m_ullOffsetSentinel)
		return { false, false };

	const auto ullSizeTotal = m_ullOffsetSentinel - (fForward ? ullStart : ullEnd); //Depends on search direction.
	const auto pHexCtrl = GetHexCtrl();
	const auto ullStep = m_ullStep; //Search step.

	STHREADRUN stThread { .ullStart { ullStart }, .ullEnd { ullEnd }, .ullOffsetSentinel { m_ullOffsetSentinel },
		.pDlgClbk { pDlgClbk }, .spnSearch { spnSearch }, .fForward { fForward }, .fDlgExit { fDlgExit } };

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
	return static_cast<EMode>(m_comboMode.GetItemData(m_comboMode.GetCurSel()));
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
	return m_fBigEndian;
}

bool CHexDlgSearch::IsInverted()const
{
	return m_fInverted;
}

bool CHexDlgSearch::IsMatchCase()const
{
	return m_fMatchCase;
}

bool CHexDlgSearch::IsSelection()const
{
	return m_fSelection;
}

bool CHexDlgSearch::IsWildcard()const
{
	return m_fWildcard;
}

void CHexDlgSearch::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if (nState == WA_INACTIVE) {
		SetLayeredWindowAttributes(0, 150, LWA_ALPHA);
	}
	else {
		SetLayeredWindowAttributes(0, 255, LWA_ALPHA);
		m_comboSearch.SetFocus();
		UpdateSearchReplaceControls();
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
	m_fSelection = m_btnSel.GetCheck() == BST_CHECKED;
	m_editStart.EnableWindow(!IsSelection());
	m_editEnd.EnableWindow(!IsSelection());
}

void CHexDlgSearch::OnCheckWildcard()
{
	m_fWildcard = m_btnWC.GetCheck() == BST_CHECKED;
}

void CHexDlgSearch::OnCheckBigEndian()
{
	m_fBigEndian = m_btnBE.GetCheck() == BST_CHECKED;
}

void CHexDlgSearch::OnCheckMatchCase()
{
	m_fMatchCase = m_btnMC.GetCheck() == BST_CHECKED;
}

void CHexDlgSearch::OnCheckInverted()
{
	m_fInverted = m_btnInv.GetCheck() == BST_CHECKED;
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
	m_btnWC.EnableWindow(fWildcard);
	m_btnBE.EnableWindow(fBigEndian);
	m_btnMC.EnableWindow(fMatchCase);

	//Assist user with required date format.
	if (eMode == EMode::SEARCH_FILETIME) {
		const auto [dwFormat, wchSepar] = GetHexCtrl()->GetDateInfo();
		const auto wstr = GetDateFormatString(dwFormat, wchSepar);
		m_comboSearch.SetCueBanner(wstr.data());
		m_comboReplace.SetCueBanner(wstr.data());
	}
	else {
		m_comboSearch.SetCueBanner(L"");
		m_comboReplace.SetCueBanner(L"");
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

	m_menuList.DestroyMenu();
}

BOOL CHexDlgSearch::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	constexpr auto iTextLimit { 512 };

	m_comboSearch.LimitText(iTextLimit);
	m_comboReplace.LimitText(iTextLimit);

	auto iIndex = m_comboMode.AddString(L"Hex Bytes");
	m_comboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_HEXBYTES));
	m_comboMode.SetCurSel(iIndex);
	iIndex = m_comboMode.AddString(L"Text ASCII");
	m_comboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_ASCII));
	iIndex = m_comboMode.AddString(L"Text UTF-8");
	m_comboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_UTF8));
	iIndex = m_comboMode.AddString(L"Text UTF-16");
	m_comboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_UTF16));
	iIndex = m_comboMode.AddString(L"Int8 value");
	m_comboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_INT8));
	iIndex = m_comboMode.AddString(L"Int16 value");
	m_comboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_INT16));
	iIndex = m_comboMode.AddString(L"Int32 value");
	m_comboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_INT32));
	iIndex = m_comboMode.AddString(L"Int64 value");
	m_comboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_INT64));
	iIndex = m_comboMode.AddString(L"Float value");
	m_comboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_FLOAT));
	iIndex = m_comboMode.AddString(L"Double value");
	m_comboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_DOUBLE));
	iIndex = m_comboMode.AddString(L"FILETIME struct");
	m_comboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_FILETIME));

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

void CHexDlgSearch::Prepare()
{
	const auto* const pHexCtrl = GetHexCtrl();
	auto ullDataSize { 0ULL }; //Actual data size for the search.
	if (m_editEnd.GetWindowTextLengthW() > 0) {
		CStringW wstrEndOffset;
		m_editEnd.GetWindowTextW(wstrEndOffset);
		const auto optEnd = stn::StrToULL(wstrEndOffset.GetString());
		if (!optEnd) {
			MessageBoxW(L"End offset is wrong.", L"Incorrect offset", MB_ICONERROR);
			return;
		}

		ullDataSize = (std::min)(pHexCtrl->GetDataSize(), *optEnd + 1); //Not more than GetDataSize().
	}
	else {
		ullDataSize = pHexCtrl->GetDataSize();
	}

	//"Search" text.
	CStringW wstrTextSearch;
	m_comboSearch.GetWindowTextW(wstrTextSearch);
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
	if (!IsSelection()) { //Do not use "Start offset" field when in the selection.
		CStringW wstrStartOffset;
		m_editStart.GetWindowTextW(wstrStartOffset);
		if (wstrStartOffset.IsEmpty()) {
			m_ullOffsetCurr = 0;
		}
		else if (const auto optStart = stn::StrToULL(wstrStartOffset.GetString()); optStart) {
			if (*optStart >= ullDataSize) {
				MessageBoxW(L"Start offset is bigger than the data size.", L"Incorrect offset", MB_ICONERROR);
				return;
			}
			m_ullOffsetCurr = *optStart;
		}
		else
			return;
	}

	//Step.
	CStringW wstrStep;
	m_editStep.GetWindowTextW(wstrStep);
	if (const auto optStep = stn::StrToULL(wstrStep.GetString()); optStep) {
		m_ullStep = *optStep;
	}
	else
		return;

	//Limit.
	CStringW wstrLimit;
	m_editLimit.GetWindowTextW(wstrLimit);
	if (const auto optLimit = stn::StrToUInt(wstrLimit.GetString()); optLimit) {
		m_dwFoundLimit = *optLimit;
	}
	else
		return;

	if (m_fReplace) {
		wchar_t warrReplace[64];
		m_comboReplace.GetWindowTextW(warrReplace, static_cast<int>(std::size(warrReplace)));
		m_wstrTextReplace = warrReplace;
		if (m_wstrTextReplace.empty()) {
			MessageBoxW(m_pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
			m_comboReplace.SetFocus();
			return;
		}
		ComboReplaceFill(warrReplace);
	}
	m_comboSearch.SetFocus();

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

	if (IsSelection()) { //Search in selection.
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
		m_ullOffsetBoundEnd = ullDataSize - m_vecSearchData.size();
		m_ullOffsetSentinel = ullDataSize;
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

	using enum ECmpType;
	if (!IsWildcard()) {
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
	if (!IsMatchCase()) { //Make the string lowercase.
		std::transform(strSearch.begin(), strSearch.end(), strSearch.begin(),
			[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
	}

	m_vecSearchData = RangeToVecBytes(strSearch);
	m_vecReplaceData = RangeToVecBytes(WstrToStr(m_wstrTextReplace, CP_ACP));

	using enum ECmpType;
	if (IsMatchCase() && !IsWildcard()) {
		m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_CHAR_LOOP)>;
	}
	else if (IsMatchCase() && IsWildcard()) {
		m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_CHAR_LOOP)
			| static_cast<std::uint16_t>(TYPE_WILDCARD)>;
	}
	else if (!IsMatchCase() && IsWildcard()) {
		m_pfnThread = &CHexDlgSearch::ThreadRun< static_cast<std::uint16_t>(TYPE_CHAR_LOOP) |
			static_cast<std::uint16_t>(TYPE_CASE_INSENSITIVE) | static_cast<std::uint16_t>(TYPE_WILDCARD)>;
	}
	else if (!IsMatchCase() && !IsWildcard()) {
		m_pfnThread = &CHexDlgSearch::ThreadRun< static_cast<std::uint16_t>(TYPE_CHAR_LOOP)
			| static_cast<std::uint16_t>(TYPE_CASE_INSENSITIVE)>;
	}

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

	using enum ECmpType;
	if (IsMatchCase() && !IsWildcard()) {
		m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_WCHAR_LOOP)>;
	}
	else if (IsMatchCase() && IsWildcard()) {
		m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_WCHAR_LOOP)
			| static_cast<std::uint16_t>(TYPE_WILDCARD)>;
	}
	else if (!IsMatchCase() && IsWildcard()) {
		m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_WCHAR_LOOP)
			| static_cast<std::uint16_t>(TYPE_CASE_INSENSITIVE) | static_cast<std::uint16_t>(TYPE_WILDCARD)>;
	}
	else if (!IsMatchCase() && !IsWildcard()) {
		m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_WCHAR_LOOP)
			| static_cast<std::uint16_t>(TYPE_CASE_INSENSITIVE)>;
	}

	return true;
}

bool CHexDlgSearch::PrepareTextUTF8()
{
	m_vecSearchData = RangeToVecBytes(WstrToStr(m_wstrTextSearch, CP_UTF8)); //Convert to UTF-8 string.
	m_vecReplaceData = RangeToVecBytes(WstrToStr(m_wstrTextReplace, CP_UTF8));

	using enum ECmpType;
	m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_CHAR_LOOP)>;

	return true;
}

bool CHexDlgSearch::PrepareINT8()
{
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

	if (IsBigEndian()) {
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

	if (IsBigEndian()) {
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

	if (IsBigEndian()) {
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

	if (IsBigEndian()) {
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

	if (IsBigEndian()) {
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

	if (IsBigEndian()) {
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
	auto ullUntil = m_ullOffsetBoundEnd;
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
				m_ullOffsetCurr = m_ullOffsetBoundBegin; //Starting from the beginning.
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
		ullUntil = m_ullOffsetBoundBegin;
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
				m_ullOffsetCurr = m_ullOffsetBoundEnd;
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
	m_editStart.SetWindowTextW(std::format(L"0x{:X}", ullOffset).data());
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
				if (MemCmp<uCmpType>(spnData.data() + iterData, pDataSearch, nSizeSearch) == !IsInverted()) {
					pStThread->ullStart = ullOffsetSearch + iterData;
					pStThread->fResult = true;
					goto FOUND;
				}
				if ((iterData + llStep <= pStThread->ullLoopChunkSize)
					&& (MemCmp<uCmpType>(spnData.data() + iterData + llStep, pDataSearch, nSizeSearch) == !IsInverted())) {
					pStThread->ullStart = ullOffsetSearch + iterData + llStep;
					pStThread->fResult = true;
					goto FOUND;
				}
				if ((iterData + llStep * 2 <= pStThread->ullLoopChunkSize)
					&& (MemCmp<uCmpType>(spnData.data() + iterData + llStep * 2, pDataSearch, nSizeSearch) == !IsInverted())) {
					pStThread->ullStart = ullOffsetSearch + iterData + llStep * 2;
					pStThread->fResult = true;
					goto FOUND;
				}
				if ((iterData + llStep * 3 <= pStThread->ullLoopChunkSize)
					&& (MemCmp<uCmpType>(spnData.data() + iterData + llStep * 3, pDataSearch, nSizeSearch) == !IsInverted())) {
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

			if (ullOffsetSearch + pStThread->ullMemToAcquire > pStThread->ullOffsetSentinel) {
				pStThread->ullMemToAcquire = pStThread->ullOffsetSentinel - ullOffsetSearch;
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
				if (MemCmp<uCmpType>(spnData.data() + iterData, pDataSearch, nSizeSearch) == !IsInverted()) {
					pStThread->ullStart = ullOffsetSearch + iterData;
					pStThread->fResult = true;
					goto FOUND;
				}
				if ((iterData - llStep >= 0)
					&& (MemCmp<uCmpType>(spnData.data() + (iterData - llStep), pDataSearch, nSizeSearch) == !IsInverted())) {
					pStThread->ullStart = ullOffsetSearch + iterData - llStep;
					pStThread->fResult = true;
					goto FOUND;
				}
				if ((iterData - llStep * 2 >= 0)
					&& (MemCmp<uCmpType>(spnData.data() + (iterData - llStep * 2), pDataSearch, nSizeSearch) == !IsInverted())) {
					pStThread->ullStart = ullOffsetSearch + iterData - llStep * 2;
					pStThread->fResult = true;
					goto FOUND;
				}
				if ((iterData - llStep * 3 >= 0)
					&& (MemCmp<uCmpType>(spnData.data() + (iterData - llStep * 3), pDataSearch, nSizeSearch) == !IsInverted())) {
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
	const auto fSearchEnabled = m_comboSearch.GetWindowTextLengthW() > 0;
	const auto fReplaceEnabled = fMutable && fSearchEnabled && m_comboReplace.GetWindowTextLengthW() > 0;

	m_comboReplace.EnableWindow(fMutable);
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

auto CHexDlgSearch::RangeToVecBytes(const std::string& str)->std::vector<std::byte>
{
	const auto pBegin = reinterpret_cast<const std::byte*>(str.data());
	const auto pEnd = pBegin + str.size();

	return { pBegin, pEnd };
}

auto CHexDlgSearch::RangeToVecBytes(const std::wstring& wstr)->std::vector<std::byte>
{
	const auto pBegin = reinterpret_cast<const std::byte*>(wstr.data());
	const auto pEnd = pBegin + wstr.size() * sizeof(wchar_t);

	return { pBegin, pEnd };
}

template<typename T>
auto CHexDlgSearch::RangeToVecBytes(T tData)->std::vector<std::byte>
{
	const auto pBegin = reinterpret_cast<std::byte*>(&tData);
	const auto pEnd = pBegin + sizeof(tData);

	return { pBegin, pEnd };
}