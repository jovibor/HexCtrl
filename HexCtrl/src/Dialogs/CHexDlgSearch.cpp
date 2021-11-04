/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
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

//#define SEARCH_IO_TESTING

namespace HEXCTRL::INTERNAL
{
	enum class CHexDlgSearch::EMode : std::uint8_t
	{
		SEARCH_HEXBYTES, SEARCH_ASCII, SEARCH_UTF8, SEARCH_WCHAR,
		SEARCH_BYTE, SEARCH_WORD, SEARCH_DWORD, SEARCH_QWORD,
		SEARCH_FLOAT, SEARCH_DOUBLE, SEARCH_FILETIME, SEARCH_READONLY//REMOVELATER:
	};

	enum class ECmpType : std::uint16_t //Flags for the instantiations of templated MemCmp<>.
	{
		TYPE_CHAR_LOOP = 0x0001,
		TYPE_WCHAR_LOOP = 0x0002,
		TYPE_CASE_INSENSITIVE = 0x0004,
		TYPE_WILDCARD = 0x0008,
		TYPE_INT8 = 0x0010,
		TYPE_INT16 = 0x0020,
		TYPE_INT32 = 0x0040,
		TYPE_INT64 = 0x0080
	};

	enum class EMenuID : std::uint16_t
	{
		IDM_SEARCH_ADDBKM = 0x8000, IDM_SEARCH_SELECTALL = 0x8001, IDM_SEARCH_CLEARALL = 0x8002
	};

	struct CHexDlgSearch::SFINDRESULT
	{
		bool fFound { };
		bool fCanceled { }; //Search was canceled by pressing "Cancel".
		operator bool()const { return fFound; };
	};

	struct CHexDlgSearch::STHREADRUN
	{
		ULONGLONG ullStart { };
		ULONGLONG ullEnd { };
		ULONGLONG ullChunkSize { };
		ULONGLONG ullMemChunks { };
		ULONGLONG ullMemToAcquire { };
		ULONGLONG ullEndSentinel { };
		CHexDlgCallback* pDlgClbk { };
		std::span<std::byte> spnSearch { };
		bool fForward { };
		bool fBigStep { };
		bool fCanceled { };
		bool fDlgExit { };
		bool fResult { };
	};

	constexpr auto SEARCH_SIZE_LIMIT = 256U;
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
	ON_NOTIFY(LVN_GETDISPINFOW, IDC_HEXCTRL_SEARCH_LIST_MAIN, &CHexDlgSearch::OnListGetDispInfo)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_HEXCTRL_SEARCH_LIST_MAIN, &CHexDlgSearch::OnListItemChanged)
	ON_NOTIFY(NM_RCLICK, IDC_HEXCTRL_SEARCH_LIST_MAIN, &CHexDlgSearch::OnListRClick)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL CHexDlgSearch::Create(UINT nIDTemplate, CWnd* pParent, IHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	if (pHexCtrl == nullptr)
		return FALSE;

	m_pHexCtrl = pHexCtrl;

	return CDialogEx::Create(nIDTemplate, pParent);
}

bool CHexDlgSearch::IsSearchAvail()const
{
	const auto* const pHexCtrl = GetHexCtrl();
	return !(m_wstrTextSearch.empty() || !pHexCtrl->IsDataSet() || m_ullOffsetCurr >= pHexCtrl->GetDataSize());
}

void CHexDlgSearch::SearchNextPrev(bool fForward)
{
	m_iDirection = fForward ? 1 : -1;
	m_fReplace = false;
	m_fAll = false;
	Search();
}

BOOL CHexDlgSearch::ShowWindow(int nCmdShow)
{
	if (nCmdShow == SW_SHOW)
	{
		int iChkStatus { BST_UNCHECKED };
		const auto* const pHexCtrl = GetHexCtrl();
		if (auto vecSel = pHexCtrl->GetSelection(); vecSel.size() == 1 && vecSel.back().ullSize > 1)
			iChkStatus = BST_CHECKED;

		m_stCheckSel.SetCheck(iChkStatus);
		m_stEditStart.EnableWindow(iChkStatus == BST_CHECKED ? 0 : 1);
		BringWindowToTop();
	}

	return CDialogEx::ShowWindow(nCmdShow);
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

void CHexDlgSearch::AddToList(ULONGLONG ullOffset)
{
	int iHighlight { -1 };
	if (auto iter = std::find(m_vecSearchRes.begin(), m_vecSearchRes.end(), ullOffset);
		iter == m_vecSearchRes.end()) //Max-found search occurences.
	{
		if (m_vecSearchRes.size() < static_cast<size_t>(m_dwFoundLimit))
		{
			m_vecSearchRes.emplace_back(ullOffset);
			iHighlight = static_cast<int>(m_vecSearchRes.size());
			m_pListMain->SetItemCountEx(iHighlight--);
		}
	}
	else
		iHighlight = static_cast<int>(iter - m_vecSearchRes.begin());

	if (iHighlight != -1)
	{
		m_pListMain->SetItemState(-1, 0, LVIS_SELECTED);
		m_pListMain->SetItemState(iHighlight, LVIS_SELECTED, LVIS_SELECTED);
		m_pListMain->EnsureVisible(iHighlight, TRUE);
	}
}

void CHexDlgSearch::ClearList()
{
	m_pListMain->SetItemCountEx(0);
	m_vecSearchRes.clear();
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

auto CHexDlgSearch::Finder(ULONGLONG& ullStart, ULONGLONG ullEnd, std::span<std::byte> spnSearch,
	bool fForward, CHexDlgCallback* pDlgClbk, bool fDlgExit)->SFINDRESULT
{	//ullStart will keep index of found occurence, if any.

	const auto nSizeSearch = spnSearch.size();
	assert(nSizeSearch <= SEARCH_SIZE_LIMIT);
	if (nSizeSearch > SEARCH_SIZE_LIMIT)
		return { false, false };

	assert(ullStart + nSizeSearch <= m_ullEndSentinel);
	if (ullStart + nSizeSearch > m_ullEndSentinel)
		return { false, false };

	constexpr auto iSizeQuick { 1024 * 256 }; //256KB.
	const auto ullSizeTotal = m_ullEndSentinel - (fForward ? ullStart : ullEnd); //Depends on search direction.
	const auto pHexCtrl = GetHexCtrl();
	const auto ullStep = m_ullStep; //Search step.

	STHREADRUN stThread;
	stThread.ullStart = ullStart;
	stThread.ullEndSentinel = m_ullEndSentinel;
	stThread.ullEnd = ullEnd;
	stThread.pDlgClbk = pDlgClbk;
	stThread.fDlgExit = fDlgExit;
	stThread.fForward = fForward;
	stThread.spnSearch = spnSearch;

	if (!pHexCtrl->IsVirtual())
	{
		stThread.ullMemToAcquire = ullSizeTotal;
		stThread.ullChunkSize = ullSizeTotal - nSizeSearch;
		stThread.ullMemChunks = 1;
	}
	else
	{
		stThread.ullMemToAcquire = pHexCtrl->GetCacheSize();
		if (stThread.ullMemToAcquire > ullSizeTotal)
			stThread.ullMemToAcquire = ullSizeTotal;
		stThread.ullChunkSize = stThread.ullMemToAcquire - nSizeSearch;

		if (ullStep > stThread.ullChunkSize) //For very big steps.
		{
			stThread.ullMemChunks = ullSizeTotal > ullStep ? ullSizeTotal / ullStep
				+ ((ullSizeTotal % ullStep) ? 1 : 0) : 1;
			stThread.fBigStep = true;
		}
		else
			stThread.ullMemChunks = ullSizeTotal > stThread.ullChunkSize ? ullSizeTotal / stThread.ullChunkSize
			+ ((ullSizeTotal % stThread.ullChunkSize) ? 1 : 0) : 1;
	}

	if (pDlgClbk == nullptr && ullSizeTotal > iSizeQuick) //Showing "Cancel" dialog only when data > iSizeQuick
	{
		CHexDlgCallback dlgClbk(L"Searching...", fForward ? ullStart : ullEnd, fForward ? ullEnd : ullStart);
		stThread.pDlgClbk = &dlgClbk;
		std::thread thrd(m_pfnThread, this, &stThread);
		dlgClbk.DoModal();
		thrd.join();
	}
	else
		(this->*m_pfnThread)(&stThread);

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

void CHexDlgSearch::HexCtrlHighlight(const std::vector<HEXSPAN>& vecSel)
{
	const auto pHexCtrl = GetHexCtrl();
	pHexCtrl->SetSelection(vecSel, true, m_fSelection); //Highlight selection?

	if (!pHexCtrl->IsOffsetVisible(vecSel.back().ullOffset))
		pHexCtrl->GoToOffset(vecSel.back().ullOffset);
}

template<std::uint16_t uiType>
bool CHexDlgSearch::MemCmp(const std::byte* pBuf1, const std::byte* pBuf2, size_t nSize)const
{
	using enum ECmpType;
	if constexpr (uiType == 999) //REMOVELATER:
	{
		volatile auto ch = static_cast<char>(*pBuf1); //Attempt to trick optimizer.
		(void)ch;

		return false;
	}
	else if constexpr ((uiType & static_cast<std::uint16_t>(TYPE_INT8)) > 0)
	{
		if (*reinterpret_cast<const std::uint8_t*>(pBuf1) != *reinterpret_cast<const std::uint8_t*>(pBuf2))
			return m_fInverted;

		return !m_fInverted;
	}
	else if constexpr ((uiType & static_cast<std::uint16_t>(TYPE_INT16)) > 0)
	{
		if (*reinterpret_cast<const std::uint16_t*>(pBuf1) != *reinterpret_cast<const std::uint16_t*>(pBuf2))
			return m_fInverted;

		return !m_fInverted;
	}
	else if constexpr ((uiType & static_cast<std::uint16_t>(TYPE_INT32)) > 0)
	{
		if (*reinterpret_cast<const std::uint32_t*>(pBuf1) != *reinterpret_cast<const std::uint32_t*>(pBuf2))
			return m_fInverted;

		return !m_fInverted;
	}
	else if constexpr ((uiType & static_cast<std::uint16_t>(TYPE_INT64)) > 0)
	{
		if (*reinterpret_cast<const std::uint64_t*>(pBuf1) != *reinterpret_cast<const std::uint64_t*>(pBuf2))
			return m_fInverted;

		return !m_fInverted;
	}
	else if constexpr ((uiType & static_cast<std::uint16_t>(TYPE_CHAR_LOOP)) > 0)
	{
		for (auto i { 0U }; i < nSize; ++i, ++pBuf1, ++pBuf2)
		{
			if constexpr ((uiType & static_cast<std::uint16_t>(TYPE_WILDCARD)) > 0)
			{
				if (*pBuf2 == m_uWildcard) //Checking for wildcard match.
					continue;
			}

			if constexpr ((uiType & static_cast<std::uint16_t>(TYPE_CASE_INSENSITIVE)) > 0)
			{
				auto ch = static_cast<char>(*pBuf1);
				if (ch >= 0x41 && ch <= 0x5A) //IsUpper.
					ch += 32;
				if (ch != static_cast<char>(*pBuf2))
					return m_fInverted;
			}
			else
			{
				if (*pBuf1 != *pBuf2)
					return m_fInverted;
			}
		}
		return !m_fInverted;
	}
	else if constexpr ((uiType & static_cast<std::uint16_t>(TYPE_WCHAR_LOOP)) > 0)
	{
		auto pBuf1wch = reinterpret_cast<const wchar_t*>(pBuf1);
		auto pBuf2wch = reinterpret_cast<const wchar_t*>(pBuf2);
		for (auto i { 0U }; i < nSize / sizeof(wchar_t); ++i, ++pBuf1wch, ++pBuf2wch)
		{
			if constexpr ((uiType & static_cast<std::uint16_t>(TYPE_WILDCARD)) > 0)
			{
				if (*pBuf2wch == static_cast<wchar_t>(m_uWildcard)) //Checking for wildcard match.
					continue;
			}

			if constexpr ((uiType & static_cast<std::uint16_t>(TYPE_CASE_INSENSITIVE)) > 0)
			{
				auto wch = *pBuf1wch;
				if (wch >= 0x41 && wch <= 0x5A) //IsUpper.
					wch += 32;
				if (wch != *pBuf2wch)
					return m_fInverted;
			}
			else
			{
				if (*pBuf1wch != *pBuf2wch)
					return m_fInverted;
			}
		}
		return !m_fInverted;
	}
}

BOOL CHexDlgSearch::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_stBrushDefault.CreateSolidBrush(m_clrBkTextArea);
	constexpr auto iTextLimit { 512 };
	m_stComboSearch.LimitText(iTextLimit);
	m_stComboReplace.LimitText(iTextLimit);

	auto iIndex = m_stComboMode.AddString(L"Hex Bytes");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_HEXBYTES));
	m_stComboMode.SetCurSel(iIndex);
	iIndex = m_stComboMode.AddString(L"ASCII Text");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_ASCII));
	iIndex = m_stComboMode.AddString(L"UTF-8 Text");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_UTF8));
	iIndex = m_stComboMode.AddString(L"UTF-16 Text");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_WCHAR));
	iIndex = m_stComboMode.AddString(L"Char value");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_BYTE));
	iIndex = m_stComboMode.AddString(L"Short value");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_WORD));
	iIndex = m_stComboMode.AddString(L"Int32 value");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_DWORD));
	iIndex = m_stComboMode.AddString(L"Int64 value");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_QWORD));
	iIndex = m_stComboMode.AddString(L"Float value");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_FLOAT));
	iIndex = m_stComboMode.AddString(L"Double value");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_DOUBLE));
	iIndex = m_stComboMode.AddString(L"FILETIME struct");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_FILETIME));

#ifdef SEARCH_IO_TESTING
	iIndex = m_stComboMode.AddString(L"_READONLY(IO SPEED)_"); //REMOVELATER:
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_READONLY));
#endif //SEARCH_IO_TESTING

	m_pListMain->CreateDialogCtrl(IDC_HEXCTRL_SEARCH_LIST_MAIN, this);
	m_pListMain->SetExtendedStyle(LVS_EX_HEADERDRAGDROP);
	m_pListMain->InsertColumn(0, L"\u2116", 0, 40);
	m_pListMain->InsertColumn(1, L"Offset", LVCFMT_LEFT, 455);

	m_stMenuList.CreatePopupMenu();
	m_stMenuList.AppendMenuW(MF_BYPOSITION, static_cast<UINT_PTR>(EMenuID::IDM_SEARCH_ADDBKM), L"Add bookmark(s)");
	m_stMenuList.AppendMenuW(MF_BYPOSITION, static_cast<UINT_PTR>(EMenuID::IDM_SEARCH_SELECTALL), L"Select All");
	m_stMenuList.AppendMenuW(MF_SEPARATOR);
	m_stMenuList.AppendMenuW(MF_BYPOSITION, static_cast<UINT_PTR>(EMenuID::IDM_SEARCH_CLEARALL), L"Clear All");

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
	stToolInfo.uId = reinterpret_cast<UINT_PTR>(GetDlgItem(IDC_HEXCTRL_SEARCH_CHECK_WILDCARD)->m_hWnd);
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

void CHexDlgSearch::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	const auto* const pHexCtrl = GetHexCtrl();
	if (!pHexCtrl->IsCreated() || !pHexCtrl->IsDataSet())
		return;

	if (nState == WA_INACTIVE)
		SetLayeredWindowAttributes(0, 150, LWA_ALPHA);
	else
	{
		SetLayeredWindowAttributes(0, 255, LWA_ALPHA);
		m_stComboSearch.SetFocus();
		const auto fMutable = pHexCtrl->IsMutable();
		m_stComboReplace.EnableWindow(fMutable);
		GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_REPLACE)->EnableWindow(fMutable);
		GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_REPLACE_ALL)->EnableWindow(fMutable);
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

	if (eMode != m_eSearchMode)
	{
		ResetSearch();
		m_eSearchMode = eMode;
	}

	BOOL fWildcard { FALSE };
	BOOL fBigEndian { FALSE };
	BOOL fMatchCase { FALSE };
	switch (eMode)
	{
	case EMode::SEARCH_WORD:
	case EMode::SEARCH_DWORD:
	case EMode::SEARCH_QWORD:
	case EMode::SEARCH_FLOAT:
	case EMode::SEARCH_DOUBLE:
	case EMode::SEARCH_FILETIME:
		fBigEndian = TRUE;
		break;
	case EMode::SEARCH_ASCII:
	case EMode::SEARCH_WCHAR:
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
}

BOOL CHexDlgSearch::OnCommand(WPARAM wParam, LPARAM lParam)
{
	bool fMsgHere { true };
	switch (static_cast<EMenuID>(LOWORD(wParam)))
	{
	case EMenuID::IDM_SEARCH_ADDBKM:
	{
		int nItem { -1 };
		for (auto i = 0UL; i < m_pListMain->GetSelectedCount(); ++i)
		{
			nItem = m_pListMain->GetNextItem(nItem, LVNI_SELECTED);
			HEXBKM hbs { .vecSpan = { HEXSPAN { m_vecSearchRes.at(static_cast<size_t>(nItem)),
				m_fReplace ? m_spnReplace.size() : m_spnSearch.size() } }, .wstrDesc = m_wstrTextSearch };
			GetHexCtrl()->BkmAdd(hbs, false);
		}
		GetHexCtrl()->Redraw();
	}
	break;
	case EMenuID::IDM_SEARCH_SELECTALL:
		m_pListMain->SetItemState(-1, LVIS_SELECTED, LVIS_SELECTED);
		break;
	case EMenuID::IDM_SEARCH_CLEARALL:
		ClearList();
		m_fSecondMatch = false; //To be able to search from the zero offset.
		break;
	default:
		fMsgHere = false;
	}

	return fMsgHere ? TRUE : CDialogEx::OnCommand(wParam, lParam);
}

HBRUSH CHexDlgSearch::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (pWnd->GetDlgCtrlID() == IDC_HEXCTRL_SEARCH_STATIC_RESULT)
	{
		constexpr auto clrFound { RGB(0, 200, 0) };
		constexpr auto clrFailed { RGB(200, 0, 0) };

		pDC->SetBkColor(m_clrBkTextArea);
		pDC->SetTextColor(m_fFound ? clrFound : clrFailed);
		return m_stBrushDefault;
	}

	return CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CHexDlgSearch::OnDestroy()
{
	CDialogEx::OnDestroy();

	m_pListMain->DestroyWindow();
	m_stBrushDefault.DeleteObject();
	m_stMenuList.DestroyMenu();
}

void CHexDlgSearch::OnListGetDispInfo(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto* const pDispInfo = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
	const auto* const pItem = &pDispInfo->item;

	if (pItem->mask & LVIF_TEXT)
	{
		const auto nItemID = static_cast<size_t>(pItem->iItem);
		switch (pItem->iSubItem)
		{
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
		pNMI->iItem != -1 && pNMI->iSubItem != -1 && (pNMI->uNewState & LVIS_SELECTED))
	{
		SetEditStartAt(m_vecSearchRes[static_cast<size_t>(pNMI->iItem)]);

		std::vector<HEXSPAN> vecSpan { };
		int nItem = -1;
		for (auto i = 0UL; i < m_pListMain->GetSelectedCount(); ++i)
		{
			nItem = m_pListMain->GetNextItem(nItem, LVNI_SELECTED);

			//Do not yet add selected (clicked) item (in multiselect), will add it after the loop,
			//so that it's always last in vec, to highlight it in HexCtrlHighlight.
			if (pNMI->iItem != nItem)
				vecSpan.emplace_back(m_vecSearchRes.at(static_cast<size_t>(nItem)),
					m_fReplace ? m_spnReplace.size() : m_spnSearch.size());
		}
		vecSpan.emplace_back(m_vecSearchRes.at(static_cast<size_t>(pNMI->iItem)),
			m_fReplace ? m_spnReplace.size() : m_spnSearch.size());

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
	if (wstrTextSearch.Compare(m_wstrTextSearch.data()) != 0)
	{
		ResetSearch();
		m_wstrTextSearch = wstrTextSearch;
	}
	ComboSearchFill(wstrTextSearch);

	//Start at.
	if (!m_fSelection) //Do not use "Start at" field when in the selection.
	{
		CStringW wstrStart;
		m_stEditStart.GetWindowTextW(wstrStart);
		if (wstrStart.IsEmpty())
			m_ullOffsetCurr = 0;
		else if (!wstr2num(wstrStart.GetString(), m_ullOffsetCurr))
			return;
	}

	//Step.
	CStringW wstrStep;
	m_stEditStep.GetWindowTextW(wstrStep);
	if (wstrStep.IsEmpty() || !wstr2num(wstrStep.GetString(), m_ullStep))
		return;

	//Limit.
	CStringW wstrLimit;
	m_stEditLimit.GetWindowTextW(wstrLimit);
	if (wstrStep.IsEmpty() || !wstr2num(wstrLimit.GetString(), m_dwFoundLimit))
		return;

	if (m_fReplace)
	{
		wchar_t warrReplace[64];
		GetDlgItemTextW(IDC_HEXCTRL_SEARCH_COMBO_REPLACE, warrReplace, static_cast<int>(std::size(warrReplace)));
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
	case EMode::SEARCH_HEXBYTES:
		fSuccess = PrepareHexBytes();
		break;
	case EMode::SEARCH_ASCII:
		fSuccess = PrepareASCII();
		break;
	case EMode::SEARCH_UTF8:
		fSuccess = PrepareUTF8();
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
	case EMode::SEARCH_FILETIME:
		fSuccess = PrepareFILETIME();
		break;
#ifdef SEARCH_IO_TESTING
	case EMode::SEARCH_READONLY: //REMOVELATER:
		fSuccess = true;
		m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(999)>;
		break;
#endif
	}
	if (!fSuccess)
		return;

	if (m_fSelection) //Search in selection.
	{
		const auto vecSel = pHexCtrl->GetSelection();
		if (vecSel.empty()) //No selection.
			return;

		auto& refFront = vecSel.front();

		//If the current selection differs from the previous one, or if FindAll.
		if (m_stSelSpan.ullOffset != refFront.ullOffset || m_stSelSpan.ullSize != refFront.ullSize || m_fAll)
		{
			ClearList();
			m_dwCount = 0;
			m_fSecondMatch = false;
			m_stSelSpan = refFront;
			m_ullOffsetCurr = refFront.ullOffset;
		}

		const auto ullSelSize = refFront.ullSize;
		if (ullSelSize < m_spnSearch.size()) //Selection is too small.
			return;

		m_ullBoundBegin = refFront.ullOffset;
		m_ullBoundEnd = m_ullBoundBegin + ullSelSize - m_spnSearch.size();
		m_ullEndSentinel = m_ullBoundBegin + ullSelSize;
	}
	else //Search in whole data.
	{
		m_ullBoundBegin = 0;
		m_ullBoundEnd = ullDataSize - m_spnSearch.size();
		m_ullEndSentinel = ullDataSize;
	}

	if (m_ullOffsetCurr + m_spnSearch.size() > ullDataSize || m_ullOffsetCurr + m_spnReplace.size() > ullDataSize)
	{
		m_ullOffsetCurr = 0;
		m_fSecondMatch = false;
		return;
	}

	if (m_fReplaceWarn && m_fReplace && (m_spnReplace.size() > m_spnSearch.size()))
	{
		constexpr auto wstrReplaceWarning { L"The replacing string is longer than searching string.\r\n"
			"Do you want to overwrite the bytes following search occurrence?\r\n"
			"Choosing \"No\" will cancel search." };
		if (IDNO == MessageBoxW(wstrReplaceWarning, L"Warning", MB_YESNO | MB_ICONQUESTION | MB_TOPMOST))
			return;
		m_fReplaceWarn = false;
	}

	Search();
	SetActiveWindow();
}

bool CHexDlgSearch::PrepareHexBytes()
{
	constexpr auto wstrWrongInput { L"Unacceptable input character.\r\nAllowed characters are: 0123456789AaBbCcDdEeFf" };
	m_fMatchCase = false;
	m_strSearch = wstr2str(m_wstrTextSearch);
	m_strReplace = wstr2str(m_wstrTextReplace);
	if (!str2hex(m_strSearch, m_strSearch, m_fWildcard, static_cast<char>(m_uWildcard)))
	{
		m_iWrap = 1;
		MessageBoxW(wstrWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}

	if ((m_fReplace && !str2hex(m_strReplace, m_strReplace)))
	{
		MessageBoxW(wstrWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		m_stComboReplace.SetFocus();
		return false;
	}

	m_spnSearch = { reinterpret_cast<std::byte*>(m_strSearch.data()), m_strSearch.size() };
	m_spnReplace = { reinterpret_cast<std::byte*>(m_strReplace.data()), m_strReplace.size() };

	using enum ECmpType;
	if (!m_fWildcard)
		m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_CHAR_LOOP)>;
	else
		m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_CHAR_LOOP) |
		static_cast<std::uint16_t>(TYPE_WILDCARD)>;

	return true;
}

bool CHexDlgSearch::PrepareASCII()
{
	m_strSearch = wstr2str(m_wstrTextSearch, CP_ACP); //Convert to the system default Windows ANSI code page.
	if (!m_fMatchCase) //Make the string lowercase.
		std::transform(m_strSearch.begin(), m_strSearch.end(), m_strSearch.begin(),
			[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

	m_strReplace = wstr2str(m_wstrTextReplace, CP_ACP);
	m_spnSearch = { reinterpret_cast<std::byte*>(m_strSearch.data()), m_strSearch.size() };
	m_spnReplace = { reinterpret_cast<std::byte*>(m_strReplace.data()), m_strReplace.size() };

	using enum ECmpType;
	if (m_fMatchCase && !m_fWildcard)
		m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_CHAR_LOOP)>;
	else if (m_fMatchCase && m_fWildcard)
		m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_CHAR_LOOP)
		| static_cast<std::uint16_t>(TYPE_WILDCARD)>;
	else if (!m_fMatchCase && m_fWildcard)
		m_pfnThread = &CHexDlgSearch::ThreadRun< static_cast<std::uint16_t>(TYPE_CHAR_LOOP) |
		static_cast<std::uint16_t>(TYPE_CASE_INSENSITIVE) | static_cast<std::uint16_t>(TYPE_WILDCARD)>;
	else if (!m_fMatchCase && !m_fWildcard)
		m_pfnThread = &CHexDlgSearch::ThreadRun< static_cast<std::uint16_t>(TYPE_CHAR_LOOP)
		| static_cast<std::uint16_t>(TYPE_CASE_INSENSITIVE)>;

	return true;
}

bool CHexDlgSearch::PrepareWCHAR()
{
	m_wstrSearch = m_wstrTextSearch;
	if (!m_fMatchCase) //Make the string lowercase.
		std::transform(m_wstrSearch.begin(), m_wstrSearch.end(), m_wstrSearch.begin(), std::towlower);

	m_wstrReplace = m_wstrTextReplace;
	m_spnSearch = { reinterpret_cast<std::byte*>(m_wstrSearch.data()), m_wstrSearch.size() * sizeof(wchar_t) };
	m_spnReplace = { reinterpret_cast<std::byte*>(m_wstrReplace.data()), m_wstrReplace.size() * sizeof(wchar_t) };

	using enum ECmpType;
	if (m_fMatchCase && !m_fWildcard)
		m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_WCHAR_LOOP)>;
	else if (m_fMatchCase && m_fWildcard)
		m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_WCHAR_LOOP)
		| static_cast<std::uint16_t>(TYPE_WILDCARD)>;
	else if (!m_fMatchCase && m_fWildcard)
		m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_WCHAR_LOOP)
		| static_cast<std::uint16_t>(TYPE_CASE_INSENSITIVE) | static_cast<std::uint16_t>(TYPE_WILDCARD)>;
	else if (!m_fMatchCase && !m_fWildcard)
		m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_WCHAR_LOOP)
		| static_cast<std::uint16_t>(TYPE_CASE_INSENSITIVE)>;

	return true;
}

bool CHexDlgSearch::PrepareUTF8()
{
	m_fMatchCase = false;
	m_fWildcard = false;
	m_strSearch = wstr2str(m_wstrTextSearch, CP_UTF8); //Convert to UTF-8 string.
	m_strReplace = wstr2str(m_wstrTextReplace, CP_UTF8);
	m_spnSearch = { reinterpret_cast<std::byte*>(m_strSearch.data()), m_strSearch.size() };
	m_spnReplace = { reinterpret_cast<std::byte*>(m_strReplace.data()), m_strReplace.size() };

	using enum ECmpType;
	m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_CHAR_LOOP)>;

	return true;
}

bool CHexDlgSearch::PrepareBYTE()
{
	m_fMatchCase = false;
	m_fWildcard = false;
	BYTE bData { };
	BYTE bDataRep { };
	if (!wstr2num(m_wstrTextSearch, bData) || (m_fReplace && !wstr2num(m_wstrTextReplace, bDataRep)))
	{
		MessageBoxW(m_wstrWrongInput.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}
	m_strSearch = bData;
	m_strReplace = bDataRep;
	m_spnSearch = { reinterpret_cast<std::byte*>(m_strSearch.data()), m_strSearch.size() };
	m_spnReplace = { reinterpret_cast<std::byte*>(m_strReplace.data()), m_strReplace.size() };

	using enum ECmpType;
	m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_INT8)>;

	return true;
}

bool CHexDlgSearch::PrepareWORD()
{
	m_fMatchCase = false;
	m_fWildcard = false;
	WORD wData { };
	WORD wDataRep { };
	if (!wstr2num(m_wstrTextSearch, wData) || (m_fReplace && !wstr2num(m_wstrTextReplace, wDataRep)))
	{
		MessageBoxW(m_wstrWrongInput.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}

	if (m_fBigEndian)
	{
		wData = _byteswap_ushort(wData);
		wDataRep = _byteswap_ushort(wDataRep);
	}

	m_strSearch.assign(reinterpret_cast<char*>(&wData), sizeof(WORD));
	m_strReplace.assign(reinterpret_cast<char*>(&wDataRep), sizeof(WORD));
	m_spnSearch = { reinterpret_cast<std::byte*>(m_strSearch.data()), m_strSearch.size() };
	m_spnReplace = { reinterpret_cast<std::byte*>(m_strReplace.data()), m_strReplace.size() };

	using enum ECmpType;
	m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_INT16)>;

	return true;
}

bool CHexDlgSearch::PrepareDWORD()
{
	m_fMatchCase = false;
	m_fWildcard = false;
	DWORD dwData { };
	DWORD dwDataRep { };
	if (!wstr2num(m_wstrTextSearch, dwData) || (m_fReplace && !wstr2num(m_wstrTextReplace, dwDataRep)))
	{
		MessageBoxW(m_wstrWrongInput.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}

	if (m_fBigEndian)
	{
		dwData = _byteswap_ulong(dwData);
		dwDataRep = _byteswap_ulong(dwDataRep);
	}

	m_strSearch.assign(reinterpret_cast<char*>(&dwData), sizeof(DWORD));
	m_strReplace.assign(reinterpret_cast<char*>(&dwDataRep), sizeof(DWORD));
	m_spnSearch = { reinterpret_cast<std::byte*>(m_strSearch.data()), m_strSearch.size() };
	m_spnReplace = { reinterpret_cast<std::byte*>(m_strReplace.data()), m_strReplace.size() };

	using enum ECmpType;
	m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_INT32)>;

	return true;
}

bool CHexDlgSearch::PrepareQWORD()
{
	m_fMatchCase = false;
	m_fWildcard = false;
	QWORD qwData { };
	QWORD qwDataRep { };
	if (!wstr2num(m_wstrTextSearch, qwData) || (m_fReplace && !wstr2num(m_wstrTextReplace, qwDataRep)))
	{
		MessageBoxW(m_wstrWrongInput.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}

	if (m_fBigEndian)
	{
		qwData = _byteswap_uint64(qwData);
		qwDataRep = _byteswap_uint64(qwDataRep);
	}

	m_strSearch.assign(reinterpret_cast<char*>(&qwData), sizeof(QWORD));
	m_strReplace.assign(reinterpret_cast<char*>(&qwDataRep), sizeof(QWORD));
	m_spnSearch = { reinterpret_cast<std::byte*>(m_strSearch.data()), m_strSearch.size() };
	m_spnReplace = { reinterpret_cast<std::byte*>(m_strReplace.data()), m_strReplace.size() };

	using enum ECmpType;
	m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_INT64)>;

	return true;
}

bool CHexDlgSearch::PrepareFloat()
{
	m_fMatchCase = false;
	m_fWildcard = false;
	float flData { };
	float flDataRep { };
	if (!wstr2num(m_wstrTextSearch, flData) || (m_fReplace && !wstr2num(m_wstrTextReplace, flDataRep)))
	{
		MessageBoxW(m_wstrWrongInput.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}

	if (m_fBigEndian)
	{
		flData = std::bit_cast<float>(_byteswap_ulong(std::bit_cast<unsigned long>(flData)));
		flDataRep = std::bit_cast<float>(_byteswap_ulong(std::bit_cast<unsigned long>(flDataRep)));
	}

	m_strSearch.assign(reinterpret_cast<char*>(&flData), sizeof(float));
	m_strReplace.assign(reinterpret_cast<char*>(&flDataRep), sizeof(float));
	m_spnSearch = { reinterpret_cast<std::byte*>(m_strSearch.data()), m_strSearch.size() };
	m_spnReplace = { reinterpret_cast<std::byte*>(m_strReplace.data()), m_strReplace.size() };

	using enum ECmpType;
	m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_INT32)>;

	return true;
}

bool CHexDlgSearch::PrepareDouble()
{
	m_fMatchCase = false;
	m_fWildcard = false;
	double ddData { };
	double ddDataRep { };
	if (!wstr2num(m_wstrTextSearch, ddData) || (m_fReplace && !wstr2num(m_wstrTextReplace, ddDataRep)))
	{
		MessageBoxW(m_wstrWrongInput.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}

	if (m_fBigEndian)
	{
		ddData = std::bit_cast<double>(_byteswap_uint64(std::bit_cast<unsigned long long>(ddData)));
		ddDataRep = std::bit_cast<double>(_byteswap_uint64(std::bit_cast<unsigned long long>(ddDataRep)));
	}

	m_strSearch.assign(reinterpret_cast<char*>(&ddData), sizeof(double));
	m_strReplace.assign(reinterpret_cast<char*>(&ddDataRep), sizeof(double));
	m_spnSearch = { reinterpret_cast<std::byte*>(m_strSearch.data()), m_strSearch.size() };
	m_spnReplace = { reinterpret_cast<std::byte*>(m_strReplace.data()), m_strReplace.size() };

	using enum ECmpType;
	m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_INT64)>;

	return true;
}

bool CHexDlgSearch::PrepareFILETIME()
{
	m_fMatchCase = false;
	m_fWildcard = false;
	DWORD dwDateFormat { GetHexCtrl()->GetDateInfo() };

	const auto optFTSearch = StringToFileTime(m_wstrTextSearch, dwDateFormat);
	const auto optFTReplace = StringToFileTime(m_wstrTextReplace, dwDateFormat);
	if (!optFTSearch || (m_fReplace && !optFTReplace))
	{
		std::wstring wstr = L"Wrong FILETIME format.\r\nA correct format is: " + GetDateFormatString(dwDateFormat);
		MessageBoxW(wstr.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);

		return false;
	}

	FILETIME stFTSearch { *optFTSearch };
	FILETIME stFTReplace { *optFTReplace };
	if (m_fBigEndian)
	{
		stFTSearch.dwLowDateTime = _byteswap_ulong(stFTSearch.dwLowDateTime);
		stFTSearch.dwHighDateTime = _byteswap_ulong(stFTSearch.dwHighDateTime);
		stFTReplace.dwLowDateTime = _byteswap_ulong(stFTReplace.dwLowDateTime);
		stFTReplace.dwHighDateTime = _byteswap_ulong(stFTReplace.dwHighDateTime);
	}

	m_strSearch.assign(reinterpret_cast<char*>(&stFTSearch), sizeof(FILETIME));
	m_strReplace.assign(reinterpret_cast<char*>(&stFTReplace), sizeof(FILETIME));
	m_spnSearch = { reinterpret_cast<std::byte*>(m_strSearch.data()), m_strSearch.size() };
	m_spnReplace = { reinterpret_cast<std::byte*>(m_strReplace.data()), m_strReplace.size() };

	using enum ECmpType;
	m_pfnThread = &CHexDlgSearch::ThreadRun<static_cast<std::uint16_t>(TYPE_INT64)>;

	return true;
}

void CHexDlgSearch::Replace(ULONGLONG ullIndex, const std::span<std::byte> spnReplace)const
{
	HEXMODIFY hms;
	hms.enModifyMode = EHexModifyMode::MODIFY_ONCE;
	hms.vecSpan.emplace_back(ullIndex, spnReplace.size());
	hms.spnData = spnReplace;
	GetHexCtrl()->ModifyData(hms);
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

	const auto lmbFindForward = [&]()
	{
		if (stFind = Finder(m_ullOffsetCurr, ullUntil, m_spnSearch);
			stFind.fFound)
		{
			m_fFound = true;
			m_fSecondMatch = true;
			m_dwCount++;
			ullOffsetFound = m_ullOffsetCurr;
		}
		if (!m_fFound)
		{
			m_iWrap = 1;

			if (m_fSecondMatch && !stFind.fCanceled)
			{
				m_ullOffsetCurr = m_ullBoundBegin; //Starting from the beginning.
				if (Finder(m_ullOffsetCurr, ullUntil, m_spnSearch))
				{
					m_fFound = true;
					m_fDoCount = true;
					m_dwCount = 1;
					ullOffsetFound = m_ullOffsetCurr;
				}
			}
		}
	};
	const auto lmbFindBackward = [&]()
	{
		ullUntil = m_ullBoundBegin;
		if (m_fSecondMatch && m_ullOffsetCurr - m_ullStep < m_ullOffsetCurr)
		{
			m_ullOffsetCurr -= m_ullStep;
			if (stFind = Finder(m_ullOffsetCurr, ullUntil, m_spnSearch, false);
				stFind.fFound)
			{
				m_fFound = true;
				m_dwCount--;
				ullOffsetFound = m_ullOffsetCurr;
			}
		}
		if (!m_fFound)
		{
			m_iWrap = -1;

			if (!stFind.fCanceled)
			{
				m_ullOffsetCurr = m_ullBoundEnd;
				if (Finder(m_ullOffsetCurr, ullUntil, m_spnSearch, false))
				{
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

	if (m_fReplace) //Replace
	{
		if (m_fAll) //Replace All
		{
			auto ullStart = m_ullOffsetCurr;
			const auto lmbReplaceAll = [&](CHexDlgCallback* pDlgClbk)
			{
				if (pDlgClbk == nullptr)
					return;

				while (true)
				{
					if (Finder(ullStart, ullUntil, m_spnSearch, true, pDlgClbk, false))
					{
						Replace(ullStart, m_spnReplace);
						ullStart += m_spnReplace.size() <= m_ullStep ? m_ullStep : m_spnReplace.size();
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
		else if (m_iDirection == 1) //Forward only direction.
		{
			lmbFindForward();
			if (m_fFound)
			{
				Replace(m_ullOffsetCurr, m_spnReplace);
				//Increase next search step to replaced or m_ullStep amount.
				m_ullOffsetCurr += m_spnReplace.size() <= m_ullStep ? m_ullStep : m_spnReplace.size();
				++m_dwReplaced;
			}
		}
	}
	else //Search.
	{
		if (m_fAll)
		{
			ClearList(); //Clearing all results.
			m_dwCount = 0;
			auto ullStart = m_ullOffsetCurr;
			const auto lmbSearchAll = [&](CHexDlgCallback* pDlgClbk)
			{
				if (pDlgClbk == nullptr)
					return;

				while (true)
				{
					if (Finder(ullStart, ullUntil, m_spnSearch, true, pDlgClbk, false))
					{
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

	pHexCtrl->SetRedraw(true);

	std::wstring wstrInfo;
	if (m_fFound)
	{
		if (m_fAll)
		{
			if (m_fReplace)
			{
				wstrInfo = std::format(L"{} occurrence(s) replaced.", m_dwReplaced);
				m_dwReplaced = 0;
				pHexCtrl->Redraw(); //Redraw in case of Replace all.
			}
			else
			{
				wstrInfo = std::format(L"Found {} occurrences.", m_dwCount);
				m_dwCount = 0;
			}
		}
		else
		{
			if (m_fDoCount)
				wstrInfo = std::format(L"Found occurrence \u2116 {} from the beginning.", m_dwCount);
			else
				wstrInfo = L"Search found occurrence.";

			HexCtrlHighlight({ { ullOffsetFound, m_fReplace ? m_spnReplace.size() : m_spnSearch.size() } });
			AddToList(ullOffsetFound);
			SetEditStartAt(m_ullOffsetCurr);
		}
	}
	else
	{
		if (stFind.fCanceled)
			wstrInfo = L"Didn't find any occurrence, search was canceled.";
		else
		{
			if (m_iWrap == 1)
				wstrInfo = L"Didn't find any occurrence, the end is reached.";
			else if (m_iWrap == -1)
				wstrInfo = L"Didn't find any occurrence, the begining is reached.";
			ResetSearch();
		}
	}

	GetDlgItem(IDC_HEXCTRL_SEARCH_STATIC_RESULT)->SetWindowTextW(wstrInfo.data());
	SetForegroundWindow();
	SetFocus();
}

void CHexDlgSearch::SetEditStartAt(ULONGLONG ullOffset)
{
	m_stEditStart.SetWindowTextW(std::format(L"0x{:X}", ullOffset).data());
}

template<std::uint16_t uiType>
void CHexDlgSearch::ThreadRun(STHREADRUN* pStThread)
{
	const auto pDataSearch = pStThread->spnSearch.data();
	const auto nSizeSearch = pStThread->spnSearch.size();
	const auto pHexCtrl = GetHexCtrl();
	const auto llStep = static_cast<LONGLONG>(m_ullStep); //Search step, must be signed here.
	constexpr auto LOOP_UNROLL_SIZE = 4; //How many comparisons we do at one loop step.

	if (pStThread->fForward) //Forward search.
	{
		auto ullOffsetSearch = pStThread->ullStart;
		for (auto iterChunk = 0ULL; iterChunk < pStThread->ullMemChunks; ++iterChunk)
		{
			const auto spnData = pHexCtrl->GetData({ ullOffsetSearch, pStThread->ullMemToAcquire });
			assert(!spnData.empty());

			for (auto iterData = 0ULL; iterData <= pStThread->ullChunkSize; iterData += llStep * LOOP_UNROLL_SIZE)
			{
				//Unrolling the loop, making LOOP_UNROLL_SIZE comparisons at a time.
				if (MemCmp<uiType>(spnData.data() + iterData, pDataSearch, nSizeSearch))
				{
					pStThread->ullStart = ullOffsetSearch + iterData;
					pStThread->fResult = true;
					goto FOUND;
				}
				if ((iterData + llStep <= pStThread->ullChunkSize)
					&& MemCmp<uiType>(spnData.data() + iterData + llStep, pDataSearch, nSizeSearch))
				{
					pStThread->ullStart = ullOffsetSearch + iterData + llStep;
					pStThread->fResult = true;
					goto FOUND;
				}
				if ((iterData + llStep * 2 <= pStThread->ullChunkSize)
					&& MemCmp<uiType>(spnData.data() + iterData + llStep * 2, pDataSearch, nSizeSearch))
				{
					pStThread->ullStart = ullOffsetSearch + iterData + llStep * 2;
					pStThread->fResult = true;
					goto FOUND;
				}
				if ((iterData + llStep * 3 <= pStThread->ullChunkSize)
					&& MemCmp<uiType>(spnData.data() + iterData + llStep * 3, pDataSearch, nSizeSearch))
				{
					pStThread->ullStart = ullOffsetSearch + iterData + llStep * 3;
					pStThread->fResult = true;
					goto FOUND;
				}

				if (pStThread->pDlgClbk != nullptr)
				{
					if (pStThread->pDlgClbk->IsCanceled())
					{
						pStThread->fCanceled = true;
						return; //Breaking out from all level loops.
					}
					pStThread->pDlgClbk->SetProgress(ullOffsetSearch + iterData);
				}
			}

			if (pStThread->fBigStep)
			{
				if ((ullOffsetSearch + llStep) > pStThread->ullEnd)
					break; //Upper bound reached.

				ullOffsetSearch += llStep;
			}
			else
			{
				ullOffsetSearch += pStThread->ullChunkSize;
			}

			if (ullOffsetSearch + pStThread->ullMemToAcquire > pStThread->ullEndSentinel)
			{
				pStThread->ullMemToAcquire = pStThread->ullEndSentinel - ullOffsetSearch;
				pStThread->ullChunkSize = pStThread->ullMemToAcquire - nSizeSearch;
			}
		}
	}
	else //Backward search.
	{
		ULONGLONG ullOffsetSearch;
		if ((pStThread->ullStart - pStThread->ullChunkSize) < pStThread->ullEnd
			|| ((pStThread->ullStart - pStThread->ullChunkSize) >
				((std::numeric_limits<ULONGLONG>::max)() - pStThread->ullChunkSize)))
		{
			pStThread->ullMemToAcquire = (pStThread->ullStart - pStThread->ullEnd) + nSizeSearch;
			pStThread->ullChunkSize = pStThread->ullMemToAcquire - nSizeSearch;
			ullOffsetSearch = pStThread->ullEnd;
		}
		else
			ullOffsetSearch = pStThread->ullStart - pStThread->ullChunkSize;

		for (auto iterChunk = pStThread->ullMemChunks; iterChunk > 0; --iterChunk)
		{
			const auto spnData = pHexCtrl->GetData({ ullOffsetSearch, pStThread->ullMemToAcquire });
			assert(!spnData.empty());

			for (auto iterData = static_cast<LONGLONG>(pStThread->ullChunkSize); iterData >= 0; iterData -= llStep * LOOP_UNROLL_SIZE) //iterData might be negative.
			{
				if (MemCmp<uiType>(spnData.data() + iterData, pDataSearch, nSizeSearch))
				{
					pStThread->ullStart = ullOffsetSearch + iterData;
					pStThread->fResult = true;
					goto FOUND;
				}
				if ((iterData - llStep >= 0)
					&& MemCmp<uiType>(spnData.data() + (iterData - llStep), pDataSearch, nSizeSearch))
				{
					pStThread->ullStart = ullOffsetSearch + iterData - llStep;
					pStThread->fResult = true;
					goto FOUND;
				}
				if ((iterData - llStep * 2 >= 0)
					&& MemCmp<uiType>(spnData.data() + (iterData - llStep * 2), pDataSearch, nSizeSearch))
				{
					pStThread->ullStart = ullOffsetSearch + iterData - llStep * 2;
					pStThread->fResult = true;
					goto FOUND;
				}
				if ((iterData - llStep * 3 >= 0)
					&& MemCmp<uiType>(spnData.data() + (iterData - llStep * 3), pDataSearch, nSizeSearch))
				{
					pStThread->ullStart = ullOffsetSearch + iterData - llStep * 3;
					pStThread->fResult = true;
					goto FOUND;
				}

				if (pStThread->pDlgClbk != nullptr)
				{
					if (pStThread->pDlgClbk->IsCanceled())
					{
						pStThread->fCanceled = true;
						return; //Breaking out from all level loops.
					}
					pStThread->pDlgClbk->SetProgress(pStThread->ullStart - (ullOffsetSearch + iterData));
				}
			}

			if (pStThread->fBigStep)
			{
				if ((ullOffsetSearch - llStep) < pStThread->ullEnd
					|| (ullOffsetSearch - llStep) > ((std::numeric_limits<ULONGLONG>::max)() - llStep))
					break; //Lower bound reached.

				ullOffsetSearch -= pStThread->ullChunkSize;
			}
			else
			{
				if ((ullOffsetSearch - pStThread->ullChunkSize) < pStThread->ullEnd
					|| ((ullOffsetSearch - pStThread->ullChunkSize) >
						((std::numeric_limits<ULONGLONG>::max)() - pStThread->ullChunkSize)))
				{
					pStThread->ullMemToAcquire = (ullOffsetSearch - pStThread->ullEnd) + nSizeSearch;
					pStThread->ullChunkSize = pStThread->ullMemToAcquire - nSizeSearch;
					ullOffsetSearch = pStThread->ullEnd;
				}
				else
					ullOffsetSearch -= pStThread->ullChunkSize;
			}
		}
	}

FOUND:
	if (pStThread->pDlgClbk != nullptr)
	{
		if (pStThread->fDlgExit)
			pStThread->pDlgClbk->ExitDlg();
		else {
			//If MemCmp returns true at the very first hit, we don't reach the SetProgress in the main loop.
			//It might be the case for the "Find/Replace All" for example.
			//If the data all the same (e.g. 0xCDCDCDCD...) then MemCmp would return true every single step.
			pStThread->pDlgClbk->SetProgress(pStThread->ullStart);
		}
	}
}