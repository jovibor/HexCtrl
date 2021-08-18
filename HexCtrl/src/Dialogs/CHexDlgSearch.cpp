/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
****************************************************************************************/
#include "stdafx.h"
#include "../../res/HexCtrlRes.h"
#include "../HexUtility.h"
#include "CHexDlgCallback.h"
#include "CHexDlgSearch.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cwctype>
#include <limits>
#include <string>
#include <thread>
#undef min
#undef max

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

namespace HEXCTRL::INTERNAL
{
	enum class CHexDlgSearch::EMode : WORD
	{
		SEARCH_HEX, SEARCH_ASCII, SEARCH_WCHAR,
		SEARCH_BYTE, SEARCH_WORD, SEARCH_DWORD, SEARCH_QWORD,
		SEARCH_FLOAT, SEARCH_DOUBLE, SEARCH_FILETIME
	};

	enum class EMenuID : WORD
	{
		IDM_SEARCH_ADDBKM = 0x8000, IDM_SEARCH_SELECTALL = 0x8001, IDM_SEARCH_CLEARALL = 0x8002
	};

	struct CHexDlgSearch::SFIND
	{
		bool fFound { };
		bool fCanceled { }; //Search was canceled by pressing "Cancel".
		operator bool()const { return fFound; };
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

auto CHexDlgSearch::Finder(ULONGLONG& ullStart, ULONGLONG ullEnd,
	std::byte* pSearch, size_t nSizeSearch,
	ULONGLONG ullEndSentinel, bool fForward,
	CHexDlgCallback* pDlgClbk, bool fDlgExit)->SFIND
{
	//ullStart will return index of found occurence, if any.

	if (ullStart + nSizeSearch > ullEndSentinel)
		return { false, false };

	const auto ullSizeTotal = fForward ? ullEnd - ullStart : ullStart - ullEnd; //Depends on search direction.
	const ULONGLONG ullStep { m_ullStep }; //Search step.
	ULONGLONG ullSizeChunk { };    //Size of the chunk to work with.
	ULONGLONG ullChunks { };       //How many chunks.
	ULONGLONG ullMemToAcquire { }; //Size of VirtualData memory for acquiring. It's bigger than ullSizeChunk.
	ULONGLONG ullOffsetSearch { };
	constexpr auto iSizeQuick { 1024 * 256 }; //256KB.
	bool fBigStep { false };
	const auto pHexCtrl = GetHexCtrl();

	if (!pHexCtrl->IsVirtual())
	{
		ullSizeChunk = ullSizeTotal;
		ullMemToAcquire = ullSizeTotal;
		ullChunks = 1;
	}
	else
	{
		ullMemToAcquire = pHexCtrl->GetCacheSize();
		if (ullMemToAcquire > ullSizeTotal + nSizeSearch)
			ullMemToAcquire = ullSizeTotal + nSizeSearch;
		ullSizeChunk = ullMemToAcquire - nSizeSearch;

		if (ullStep > ullSizeChunk) //For very big steps.
		{
			ullChunks = ullSizeTotal > ullStep ? ullSizeTotal / ullStep + ((ullSizeTotal % ullStep) ? 1 : 0) : 1;
			fBigStep = true;
		}
		else
			ullChunks = ullSizeTotal > ullSizeChunk ? ullSizeTotal / ullSizeChunk + ((ullSizeTotal % ullSizeChunk) ? 1 : 0) : 1;
	}

	bool fResult { false };
	bool fCanceled { false };
	const auto lmbSearch = [&](CHexDlgCallback* pDlgClbk = nullptr)
	{
		if (fForward) //Forward search.
		{
			ullOffsetSearch = ullStart;
			for (auto iterChunk = 0ULL; iterChunk < ullChunks; ++iterChunk)
			{
				const auto spnData = pHexCtrl->GetData({ ullOffsetSearch, ullMemToAcquire });
				assert(!spnData.empty());

				for (auto iterData = 0ULL; iterData <= ullSizeChunk; iterData += ullStep)
				{
					if (MemCmp(spnData.data() + iterData, pSearch, nSizeSearch))
					{
						ullStart = ullOffsetSearch + iterData;
						fResult = true;
						goto FOUND;
					}

					if (pDlgClbk != nullptr)
					{
						if (pDlgClbk->IsCanceled())
						{
							fCanceled = true;
							return; //Breaking out from all level loops.
						}
						pDlgClbk->SetProgress(ullOffsetSearch + iterData);
					}
				}

				if (fBigStep)
					ullOffsetSearch += ullStep;
				else
					ullOffsetSearch += ullSizeChunk;
				if (ullOffsetSearch + ullMemToAcquire > ullEndSentinel)
				{
					ullMemToAcquire = ullEndSentinel - ullOffsetSearch;
					ullSizeChunk = ullMemToAcquire - nSizeSearch;
				}
			}
		}
		else //Backward search.
		{
			ullOffsetSearch = ullStart - ullSizeChunk;
			for (auto iterChunk = ullChunks; iterChunk > 0; --iterChunk)
			{
				const auto spnData = pHexCtrl->GetData({ ullOffsetSearch, ullMemToAcquire });
				assert(!spnData.empty());

				for (auto iterData = static_cast<LONGLONG>(ullSizeChunk); iterData >= 0; iterData -= ullStep) //iterData might be negative.
				{
					if (MemCmp(spnData.data() + iterData, pSearch, nSizeSearch))
					{
						ullStart = ullOffsetSearch + iterData;
						fResult = true;
						goto FOUND;
					}

					if (pDlgClbk != nullptr)
					{
						if (pDlgClbk->IsCanceled())
						{
							fCanceled = true;
							return; //Breaking out from all level loops.
						}
						pDlgClbk->SetProgress(ullStart - (ullOffsetSearch + iterData));
					}
				}

				if (fBigStep)
				{
					if ((ullOffsetSearch - ullStep) < ullEnd
						|| (ullOffsetSearch - ullStep) > (std::numeric_limits<ULONGLONG>::max() - ullStep))
						break; //Lower bound reached.

					ullOffsetSearch -= ullSizeChunk;
				}
				else
				{
					if ((ullOffsetSearch - ullSizeChunk) < ullEnd
						|| (ullOffsetSearch - ullSizeChunk) > (std::numeric_limits<ULONGLONG>::max() - ullSizeChunk))
					{
						ullMemToAcquire = (ullOffsetSearch - ullEnd) + nSizeSearch;
						ullSizeChunk = ullMemToAcquire - nSizeSearch;
					}
					ullOffsetSearch -= ullSizeChunk;
				}
			}
		}

	FOUND:
		if (fDlgExit && pDlgClbk != nullptr)
			pDlgClbk->ExitDlg();
	};

	if (pDlgClbk == nullptr && ullSizeTotal > iSizeQuick) //Showing "Cancel" dialog only when data > iSizeQuick
	{
		CHexDlgCallback dlgClbk(L"Searching...", fForward ? ullStart : ullEnd, fForward ? ullEnd : ullStart);
		std::thread thrd(lmbSearch, &dlgClbk);
		dlgClbk.DoModal();
		thrd.join();
	}
	else
		lmbSearch(pDlgClbk);

	return { fResult, fCanceled };
}

IHexCtrl* CHexDlgSearch::GetHexCtrl()const
{
	return m_pHexCtrl;
}

auto CHexDlgSearch::GetSearchMode()const->CHexDlgSearch::EMode
{
	return static_cast<EMode>(m_stComboMode.GetItemData(m_stComboMode.GetCurSel()));
}

void CHexDlgSearch::HexCtrlHighlight(const std::vector<HEXOFFSET>& vecSel)
{
	const auto pHexCtrl = GetHexCtrl();
	pHexCtrl->SetSelection(vecSel, true, m_fSelection); //Highlight selection?

	if (!pHexCtrl->IsOffsetVisible(vecSel.back().ullOffset))
		pHexCtrl->GoToOffset(vecSel.back().ullOffset);
}

bool CHexDlgSearch::MemCmp(const std::byte* pBuf1, const std::byte* pBuf2, size_t nSize)const
{
	const std::byte* pData1 { pBuf1 };
	std::string strUpper;
	std::wstring wstrUpper;

	if (!m_fMatchCase)
	{
		switch (m_eSearchMode)
		{
		case EMode::SEARCH_ASCII:
			strUpper.resize(nSize);
			std::transform(reinterpret_cast<const char*>(pBuf1), reinterpret_cast<const char*>(pBuf1 + nSize),
				strUpper.begin(), [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
			pData1 = reinterpret_cast<std::byte*>(strUpper.data());
			break;
		case EMode::SEARCH_WCHAR:
			wstrUpper.resize(nSize / sizeof(wchar_t));
			std::transform(reinterpret_cast<const wchar_t*>(pBuf1), reinterpret_cast<const wchar_t*>(pBuf1 + nSize),
				wstrUpper.begin(), [](wchar_t wch) { return static_cast<wchar_t>(std::towupper(wch)); });
			pData1 = reinterpret_cast<std::byte*>(wstrUpper.data());
			break;
		default:
			break;
		}
	}

	if (m_fWildcard)
	{
		for (auto i { 0ULL }; i < nSize; ++i, ++pData1, ++pBuf2)
		{
			if (*pBuf2 == m_uWildcard) //Checking for wildcard match.
				continue;

			if (*pData1 != *pBuf2)
				return m_fInverted;
		}
		return !m_fInverted;
	}

	const auto iResult = std::memcmp(pData1, pBuf2, nSize);

	return m_fInverted ? iResult != 0 : iResult == 0;
}

BOOL CHexDlgSearch::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_stBrushDefault.CreateSolidBrush(m_clrBkTextArea);
	constexpr auto iTextLimit { 512 };
	m_stComboSearch.LimitText(iTextLimit);
	m_stComboReplace.LimitText(iTextLimit);

	auto iIndex = m_stComboMode.AddString(L"Hex Bytes");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_HEX));
	m_stComboMode.SetCurSel(iIndex);
	iIndex = m_stComboMode.AddString(L"ASCII Text");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_ASCII));
	iIndex = m_stComboMode.AddString(L"UTF-16 Text");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_WCHAR));
	iIndex = m_stComboMode.AddString(L"Byte: (u)int8");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_BYTE));
	iIndex = m_stComboMode.AddString(L"Short: (u)int16");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_WORD));
	iIndex = m_stComboMode.AddString(L"Int: (u)int32");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_DWORD));
	iIndex = m_stComboMode.AddString(L"Int64: (u)int64");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_QWORD));
	iIndex = m_stComboMode.AddString(L"Float");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_FLOAT));
	iIndex = m_stComboMode.AddString(L"Double");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_DOUBLE));
	iIndex = m_stComboMode.AddString(L"FILETIME");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_FILETIME));

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
	wchar_t buff[32] { };
	swprintf_s(buff, std::size(buff), L"%llu", m_ullStep);
	m_stEditStep.SetWindowTextW(buff);

	//"Limit search hit" edit box text.
	swprintf_s(buff, std::size(buff), L"%u", m_dwFoundLimit);
	m_stEditLimit.SetWindowTextW(buff);

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

void CHexDlgSearch::OnOK()
{
	OnButtonSearchF();
}

void CHexDlgSearch::OnCancel()
{
	::SetFocus(GetHexCtrl()->GetWindowHandle(EHexWnd::WND_MAIN));

	CDialogEx::OnCancel();
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
			HEXBKM hbs { };
			nItem = m_pListMain->GetNextItem(nItem, LVNI_SELECTED);
			hbs.vecOffset.emplace_back(HEXOFFSET { m_vecSearchRes.at(static_cast<size_t>(nItem)),
				m_fReplace ? m_nSizeReplace : m_nSizeSearch });
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

void CHexDlgSearch::OnDestroy()
{
	CDialogEx::OnDestroy();

	m_pListMain->DestroyWindow();
	m_stBrushDefault.DeleteObject();
	m_stMenuList.DestroyMenu();
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
	case EMode::SEARCH_HEX:
		fWildcard = TRUE;
		break;
	default:
		break;
	}
	m_stCheckWcard.EnableWindow(fWildcard);
	m_stCheckBE.EnableWindow(fBigEndian);
	m_stCheckMatchC.EnableWindow(fMatchCase);
}

void CHexDlgSearch::OnListGetDispInfo(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto* const pDispInfo = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
	const auto* const pItem = &pDispInfo->item;

	if (pItem->mask & LVIF_TEXT)
	{
		const auto nItemID = static_cast<size_t>(pItem->iItem);
		const auto nMaxLengh = static_cast<size_t>(pItem->cchTextMax);

		switch (pItem->iSubItem)
		{
		case 0: //Index number.
			swprintf_s(pItem->pszText, nMaxLengh, L"%zu", nItemID + 1);
			break;
		case 1: //Offset.
			swprintf_s(pItem->pszText, nMaxLengh, L"0x%llX", m_vecSearchRes[nItemID]);
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

		std::vector<HEXOFFSET> vecOffset { };
		int nItem = -1;
		for (auto i = 0UL; i < m_pListMain->GetSelectedCount(); ++i)
		{
			nItem = m_pListMain->GetNextItem(nItem, LVNI_SELECTED);

			//Do not yet add selected (clicked) item (in multiselect), will add it after the loop,
			//so that it's always last in vec, to highlight it in HexCtrlHighlight.
			if (pNMI->iItem != nItem)
				vecOffset.emplace_back(HEXOFFSET { m_vecSearchRes.at(static_cast<size_t>(nItem)),
					m_fReplace ? m_nSizeReplace : m_nSizeSearch });
		}
		vecOffset.emplace_back(HEXOFFSET { m_vecSearchRes.at(static_cast<size_t>(pNMI->iItem)),
			m_fReplace ? m_nSizeReplace : m_nSizeSearch });

		HexCtrlHighlight(vecOffset);
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

HBRUSH CHexDlgSearch::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (pWnd->GetDlgCtrlID() == IDC_HEXCTRL_SEARCH_STATIC_RESULT)
	{
		pDC->SetBkColor(m_clrBkTextArea);
		pDC->SetTextColor(m_fFound ? m_clrSearchFound : m_clrSearchFailed);
		return m_stBrushDefault;
	}

	return CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
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
	case EMode::SEARCH_FILETIME:
		fSuccess = PrepareFILETIME();
		break;
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
		if (ullSelSize < m_nSizeSearch) //Selection is too small.
			return;

		m_ullBoundBegin = refFront.ullOffset;
		m_ullBoundEnd = m_ullBoundBegin + ullSelSize - m_nSizeSearch;
		m_ullEndSentinel = m_ullBoundBegin + ullSelSize;
	}
	else //Search in whole data.
	{
		m_ullBoundBegin = 0;
		m_ullBoundEnd = ullDataSize - m_nSizeSearch;
		m_ullEndSentinel = ullDataSize;
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
		static std::wstring_view wstrReplaceWarning { L"The replacing string is longer than searching string.\r\n"
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
	if (!str2hex(m_strSearch, m_strSearch, m_fWildcard, static_cast<char>(m_uWildcard)))
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
	if (!m_fMatchCase)
		std::transform(m_strSearch.begin(), m_strSearch.end(), m_strSearch.begin(),
			[](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });

	m_strReplace = wstr2str(m_wstrTextReplace);
	m_nSizeSearch = m_strSearch.size();
	m_nSizeReplace = m_strReplace.size();
	m_pSearchData = reinterpret_cast<std::byte*>(m_strSearch.data());
	m_pReplaceData = reinterpret_cast<std::byte*>(m_strReplace.data());

	return true;
}

bool CHexDlgSearch::PrepareWCHAR()
{
	m_wstrSearch = m_wstrTextSearch;
	if (!m_fMatchCase)
		std::transform(m_wstrSearch.begin(), m_wstrSearch.end(), m_wstrSearch.begin(), std::towupper);

	if (m_fWildcard)
	{
		//Constructing one wchar_t consisting with two m_uWildcard symbols.
		const wchar_t wDblWildcard = static_cast<short>(m_uWildcard) << 8 | static_cast<short>(m_uWildcard);
		std::replace(m_wstrSearch.begin(), m_wstrSearch.end(), static_cast<wchar_t>(m_uWildcard), wDblWildcard);
	}

	m_wstrReplace = m_wstrTextReplace;
	m_nSizeSearch = m_wstrSearch.size() * sizeof(wchar_t);
	m_nSizeReplace = m_wstrReplace.size() * sizeof(wchar_t);
	m_pSearchData = reinterpret_cast<std::byte*>(m_wstrSearch.data());
	m_pReplaceData = reinterpret_cast<std::byte*>(m_wstrReplace.data());

	return true;
}

bool CHexDlgSearch::PrepareBYTE()
{
	BYTE bData { };
	BYTE bDataRep { };
	if (!wstr2num(m_wstrTextSearch, bData) || (m_fReplace && !wstr2num(m_wstrTextReplace, bDataRep)))
	{
		MessageBoxW(m_wstrWrongInput.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}
	m_strSearch = bData;
	m_strReplace = bDataRep;
	m_nSizeSearch = m_strSearch.size();
	m_nSizeReplace = m_strReplace.size();
	m_pSearchData = reinterpret_cast<std::byte*>(m_strSearch.data());
	m_pReplaceData = reinterpret_cast<std::byte*>(m_strReplace.data());

	return true;
}

bool CHexDlgSearch::PrepareWORD()
{
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
	m_nSizeSearch = m_strSearch.size();
	m_nSizeReplace = m_strReplace.size();

	return true;
}

bool CHexDlgSearch::PrepareDWORD()
{
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
	m_nSizeSearch = m_strSearch.size();
	m_nSizeReplace = m_strReplace.size();
	m_pSearchData = reinterpret_cast<std::byte*>(m_strSearch.data());
	m_pReplaceData = reinterpret_cast<std::byte*>(m_strReplace.data());

	return true;
}

bool CHexDlgSearch::PrepareQWORD()
{
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
	m_nSizeSearch = m_strSearch.size();
	m_nSizeReplace = m_strReplace.size();
	m_pSearchData = reinterpret_cast<std::byte*>(m_strSearch.data());
	m_pReplaceData = reinterpret_cast<std::byte*>(m_strReplace.data());

	return true;
}

bool CHexDlgSearch::PrepareFloat()
{
	float flData { };
	float flDataRep { };
	if (!wstr2num(m_wstrTextSearch, flData) || (m_fReplace && !wstr2num(m_wstrTextReplace, flDataRep)))
	{
		MessageBoxW(m_wstrWrongInput.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}

	if (m_fBigEndian)
	{
		auto lData = _byteswap_ulong(*reinterpret_cast<unsigned long*>(&flData));
		flData = *reinterpret_cast<float*>(&lData);
		auto lDataRep = _byteswap_ulong(*reinterpret_cast<unsigned long*>(&flDataRep));
		flDataRep = *reinterpret_cast<float*>(&lDataRep);
	}

	m_strSearch.assign(reinterpret_cast<char*>(&flData), sizeof(float));
	m_strReplace.assign(reinterpret_cast<char*>(&flDataRep), sizeof(float));
	m_nSizeSearch = m_strSearch.size();
	m_nSizeReplace = m_strReplace.size();
	m_pSearchData = reinterpret_cast<std::byte*>(m_strSearch.data());
	m_pReplaceData = reinterpret_cast<std::byte*>(m_strReplace.data());

	return true;
}

bool CHexDlgSearch::PrepareDouble()
{
	double ddData { };
	double ddDataRep { };
	if (!wstr2num(m_wstrTextSearch, ddData) || (m_fReplace && !wstr2num(m_wstrTextReplace, ddDataRep)))
	{
		MessageBoxW(m_wstrWrongInput.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}

	if (m_fBigEndian)
	{
		auto llData = _byteswap_uint64(*reinterpret_cast<unsigned long long*>(&ddData));
		ddData = *reinterpret_cast<double*>(&llData);
		auto llDataRep = _byteswap_uint64(*reinterpret_cast<unsigned long long*>(&ddDataRep));
		ddDataRep = *reinterpret_cast<double*>(&llDataRep);
	}

	m_strSearch.assign(reinterpret_cast<char*>(&ddData), sizeof(double));
	m_strReplace.assign(reinterpret_cast<char*>(&ddDataRep), sizeof(double));
	m_nSizeSearch = m_strSearch.size();
	m_nSizeReplace = m_strReplace.size();
	m_pSearchData = reinterpret_cast<std::byte*>(m_strSearch.data());
	m_pReplaceData = reinterpret_cast<std::byte*>(m_strReplace.data());

	return true;
}

bool CHexDlgSearch::PrepareFILETIME()
{
	DWORD dwDateFormat { };
	if (!GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IDATE | LOCALE_RETURN_NUMBER,
		reinterpret_cast<LPWSTR>(&dwDateFormat), sizeof(dwDateFormat)))
		dwDateFormat = 1;

	const auto optSysTimeSearch = StringToSystemTime(m_wstrTextSearch, dwDateFormat);
	const auto optSysTimeReplace = StringToSystemTime(m_wstrTextReplace, dwDateFormat);
	FILETIME stFileTimeSearch { };
	FILETIME stFileTimeReplace { };
	if (!optSysTimeSearch || !SystemTimeToFileTime(&*optSysTimeSearch, &stFileTimeSearch)
			|| (m_fReplace && (!optSysTimeReplace || !SystemTimeToFileTime(&*optSysTimeReplace, &stFileTimeReplace))))
	{
		wchar_t wSeparator[2] { L'/' };
		std::wstring_view wstrFormat { };
		switch (dwDateFormat)
		{
		case 0:	//Month-Day-Year
			wstrFormat = L"MM%sDD%sYYYY HH:MM:SS.mmm";
			break;
		case 2:	//Year-Month-Day
			wstrFormat = L"YYYY%sMM%sDD HH:MM:SS.mmm";
			break;
		default: //Day-Month-Year (default)
			wstrFormat = L"DD%sMM%YYYY HH:MM:SS.mmm";
		}

		WCHAR buff[32];
		swprintf_s(buff, std::size(buff), wstrFormat.data(), wSeparator, wSeparator);
		std::wstring wstr = L"Wrong FILETIME format.\r\nA correct format is: " + std::wstring(buff);
		MessageBoxW(wstr.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);

		return false;
	}

	if (m_fBigEndian)
	{
		stFileTimeSearch.dwLowDateTime = _byteswap_ulong(stFileTimeSearch.dwLowDateTime);
		stFileTimeSearch.dwHighDateTime = _byteswap_ulong(stFileTimeSearch.dwHighDateTime);
		stFileTimeReplace.dwLowDateTime = _byteswap_ulong(stFileTimeReplace.dwLowDateTime);
		stFileTimeReplace.dwHighDateTime = _byteswap_ulong(stFileTimeReplace.dwHighDateTime);
	}

	m_strSearch.assign(reinterpret_cast<char*>(&stFileTimeSearch), sizeof(FILETIME));
	m_strReplace.assign(reinterpret_cast<char*>(&stFileTimeReplace), sizeof(FILETIME));
	m_nSizeSearch = m_strSearch.size();
	m_nSizeReplace = m_strReplace.size();
	m_pSearchData = reinterpret_cast<std::byte*>(m_strSearch.data());
	m_pReplaceData = reinterpret_cast<std::byte*>(m_strReplace.data());

	return true;
}

void CHexDlgSearch::Replace(ULONGLONG ullIndex, std::byte* pData, size_t nSizeData, size_t nSizeReplace)const
{
	HEXMODIFY hms;
	hms.vecOffset.emplace_back(HEXOFFSET { ullIndex, nSizeData });
	hms.spnData = { pData, nSizeReplace };
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
	SFIND stFind;

	const auto lmbFindForward = [&]()
	{
		if (stFind = Finder(m_ullOffsetCurr, ullUntil, m_pSearchData, m_nSizeSearch, m_ullEndSentinel);
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
				if (Finder(m_ullOffsetCurr, ullUntil, m_pSearchData, m_nSizeSearch, m_ullEndSentinel))
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
			if (stFind = Finder(m_ullOffsetCurr, ullUntil, m_pSearchData, m_nSizeSearch, m_ullEndSentinel, false);
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
				if (Finder(m_ullOffsetCurr, ullUntil, m_pSearchData, m_nSizeSearch, m_ullEndSentinel, false))
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
					if (Finder(ullStart, ullUntil, m_pSearchData, m_nSizeSearch, m_ullEndSentinel, true, pDlgClbk, false))
					{
						Replace(ullStart, m_pReplaceData, m_nSizeSearch, m_nSizeReplace);
						ullStart += m_nSizeReplace <= m_ullStep ? m_ullStep : m_nSizeReplace;
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

			CHexDlgCallback dlgClbk(L"Searching...", ullStart, ullUntil, this);
			std::thread thrd(lmbReplaceAll, &dlgClbk);
			dlgClbk.DoModal();
			thrd.join();
		}
		else if (m_iDirection == 1) //Forward only direction.
		{
			lmbFindForward();
			if (m_fFound)
			{
				Replace(m_ullOffsetCurr, m_pReplaceData, m_nSizeSearch, m_nSizeReplace);
				//Increase next search step to replaced or m_ullStep amount.
				m_ullOffsetCurr += m_nSizeReplace <= m_ullStep ? m_ullStep : m_nSizeReplace;
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
					if (Finder(ullStart, ullUntil, m_pSearchData, m_nSizeSearch, m_ullEndSentinel, true, pDlgClbk, false))
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

	std::wstring wstrInfo(128, 0);
	if (m_fFound)
	{
		if (m_fAll)
		{
			if (m_fReplace)
			{
				swprintf_s(wstrInfo.data(), wstrInfo.size(), L"%lu occurrence(s) replaced.", m_dwReplaced);
				m_dwReplaced = 0;
				pHexCtrl->Redraw(); //Redraw in case of Replace all.
			}
			else
			{
				swprintf_s(wstrInfo.data(), wstrInfo.size(), L"Found %lu occurrences.", m_dwCount);
				m_dwCount = 0;
			}
		}
		else
		{
			if (m_fDoCount)
				swprintf_s(wstrInfo.data(), wstrInfo.size(), L"Found occurrence \u2116 %lu from the beginning.", m_dwCount);
			else
				wstrInfo = L"Search found occurrence.";

			HexCtrlHighlight({ { ullOffsetFound, m_fReplace ? m_nSizeReplace : m_nSizeSearch } });
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
}

void CHexDlgSearch::SetEditStartAt(ULONGLONG ullOffset)
{
	wchar_t buff[32] { };
	swprintf_s(buff, std::size(buff), L"0x%llX", ullOffset);
	m_stEditStart.SetWindowTextW(buff);
}