/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "../../res/HexCtrlRes.h"
#include "../HexUtility.h"
#include "CHexDlgCodepage.h"
#include <algorithm>
#include <cassert>
#include <format>

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

BEGIN_MESSAGE_MAP(CHexDlgCodepage, CDialogEx)
	ON_WM_ACTIVATE()
	ON_NOTIFY(LVN_GETDISPINFOW, IDC_HEXCTRL_CODEPAGE_LIST, &CHexDlgCodepage::OnListGetDispInfo)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_HEXCTRL_CODEPAGE_LIST, &CHexDlgCodepage::OnListItemChanged)
	ON_NOTIFY(LISTEX::LISTEX_MSG_GETCOLOR, IDC_HEXCTRL_CODEPAGE_LIST, &CHexDlgCodepage::OnListGetColor)
	ON_NOTIFY(LISTEX::LISTEX_MSG_LINKCLICK, IDC_HEXCTRL_CODEPAGE_LIST, &CHexDlgCodepage::OnListLinkClick)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CHexDlgCodepage::AddCP(std::wstring_view wsv)
{
	if (const auto optCPID = stn::StrToUInt(wsv); optCPID) {
		if (CPINFOEXW stCP; GetCPInfoExW(*optCPID, 0, &stCP) != FALSE) {
			m_vecCodePage.emplace_back(static_cast<int>(*optCPID), stCP.CodePageName, stCP.MaxCharSize);
		}
	}
}

auto CHexDlgCodepage::GetDlgData()const->std::uint64_t
{
	if (!IsWindow(m_hWnd)) {
		return { };
	}

	return { };
}

void CHexDlgCodepage::Initialize(IHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	if (pHexCtrl == nullptr)
		return;

	m_pHexCtrl = pHexCtrl;
}

auto CHexDlgCodepage::SetDlgData(std::uint64_t /*ullData*/)->HWND
{
	if (!IsWindow(m_hWnd)) {
		Create(IDD_HEXCTRL_CODEPAGE, CWnd::FromHandle(m_pHexCtrl->GetWindowHandle(EHexWnd::WND_MAIN)));
	}

	return m_hWnd;
}

BOOL CHexDlgCodepage::ShowWindow(int nCmdShow)
{
	if (!IsWindow(m_hWnd)) {
		Create(IDD_HEXCTRL_CODEPAGE, CWnd::FromHandle(m_pHexCtrl->GetWindowHandle(EHexWnd::WND_MAIN)));
	}

	return CDialogEx::ShowWindow(nCmdShow);
}


//Private methods.

void CHexDlgCodepage::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BOOL CHexDlgCodepage::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_pListMain->CreateDialogCtrl(IDC_HEXCTRL_CODEPAGE_LIST, this);
	m_pListMain->SetExtendedStyle(LVS_EX_HEADERDRAGDROP);
	m_pListMain->SetSortable(true);
	m_pListMain->InsertColumn(0, L"Code page", LVCFMT_LEFT, 80);
	m_pListMain->InsertColumn(1, L"Name", LVCFMT_LEFT, 280);
	m_pListMain->InsertColumn(2, L"Max chars", LVCFMT_LEFT, 80);

	m_vecCodePage.emplace_back(-1, L"<link=\"https://en.wikipedia.org/wiki/ASCII\">ASCII 7-bit</link> (default)", 1);
	m_vecCodePage.emplace_back(0, L"Windows Internal UTF-16 (wchar_t)", 4);
	m_pThis = this;
	EnumSystemCodePagesW(EnumCodePagesProc, CP_INSTALLED);
	m_pListMain->SetItemCountEx(static_cast<int>(m_vecCodePage.size()), LVSICF_NOSCROLL);

	if (const auto pLayout = GetDynamicLayout(); pLayout != nullptr) {
		pLayout->SetMinSize({ 0, 0 });
	}

	return TRUE;
}

void CHexDlgCodepage::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if (nState == WA_ACTIVE || nState == WA_CLICKACTIVE) {
		if (m_pHexCtrl->IsCreated()) {
			m_pListMain->SetItemState(-1, 0, LVIS_SELECTED);
			if (const auto iter = std::find_if(m_vecCodePage.begin(), m_vecCodePage.end(),
				[this](const SCODEPAGE& ref) { return ref.iCPID == m_pHexCtrl->GetCodepage(); });
				iter != m_vecCodePage.end()) {
				const auto iItem = static_cast<int>(iter - m_vecCodePage.begin());
				m_pListMain->SetItemState(iItem, LVIS_SELECTED, LVIS_SELECTED);
				m_pListMain->EnsureVisible(iItem, FALSE);
			}
		}
	}

	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
}

BOOL CHexDlgCodepage::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	if (const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(lParam); pNMI->hdr.idFrom == IDC_HEXCTRL_CODEPAGE_LIST) {
		switch (pNMI->hdr.code) {
		case LVN_COLUMNCLICK:
			SortList();
			return TRUE;
		default:
			break;
		}
	}

	return CDialogEx::OnNotify(wParam, lParam, pResult);
}

void CHexDlgCodepage::OnListGetDispInfo(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pDispInfo = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
	const auto pItem = &pDispInfo->item;

	if (pItem->mask & LVIF_TEXT) {
		const auto nItemID = static_cast<std::size_t>(pItem->iItem);
		switch (pItem->iSubItem) {
		case 0: //Code page ID.
			*std::format_to(pItem->pszText, L"{}", m_vecCodePage[nItemID].iCPID) = L'\0';
			break;
		case 1: //Name.
			pItem->pszText = m_vecCodePage[nItemID].wstrName.data();
			break;
		case 2: //Max chars.
			*std::format_to(pItem->pszText, L"{}", m_vecCodePage[nItemID].uMaxChars) = L'\0';
			break;
		default:
			break;
		}
	}
}

void CHexDlgCodepage::OnListItemChanged(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	if (const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
		pNMI->iItem != -1 && pNMI->iSubItem != -1 && (pNMI->uNewState & LVIS_SELECTED)) {
		m_pHexCtrl->SetCodepage(m_vecCodePage[static_cast<std::size_t>(pNMI->iItem)].iCPID);
	}
}

void CHexDlgCodepage::OnListGetColor(NMHDR* pNMHDR, LRESULT* pResult)
{
	if (const auto pLCI = reinterpret_cast<LISTEX::PLISTEXCOLORINFO>(pNMHDR);
		m_vecCodePage[static_cast<std::size_t>(pLCI->iItem)].uMaxChars > 1) {
		pLCI->stClr = { RGB(200, 80, 80), RGB(255, 255, 255) };
		*pResult = TRUE;
		return;
	}
}

void CHexDlgCodepage::OnListLinkClick(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto* const pLLI = reinterpret_cast<LISTEX::PLISTEXLINKINFO>(pNMHDR);
	ShellExecuteW(nullptr, L"open", pLLI->pwszText, nullptr, nullptr, SW_SHOWNORMAL);
}

void CHexDlgCodepage::OnDestroy()
{
	CDialogEx::OnDestroy();

	m_vecCodePage.clear();
}

void CHexDlgCodepage::SortList()
{
	const auto iColumn = m_pListMain->GetSortColumn();
	const auto fAscending = m_pListMain->GetSortAscending();
	std::sort(m_vecCodePage.begin() + 1, m_vecCodePage.end(),
		[iColumn, fAscending](const SCODEPAGE& st1, const SCODEPAGE& st2) {
			int iCompare { };
			switch (iColumn) {
			case 0: //CP ID.
				iCompare = st1.iCPID != st2.iCPID ? (st1.iCPID < st2.iCPID ? -1 : 1) : 0;
				break;
			case 1: //CP name.
				iCompare = st1.wstrName.compare(st2.wstrName);
				break;
			case 2: //Max chars.
				iCompare = st1.uMaxChars != st2.uMaxChars ? (st1.uMaxChars < st2.uMaxChars ? -1 : 1) : 0;
				break;
			default:
				break;
			}

			return fAscending ? iCompare < 0 : iCompare > 0;
		});

	m_pListMain->RedrawWindow();
}

BOOL CHexDlgCodepage::EnumCodePagesProc(LPWSTR pwszCP)
{
	m_pThis->AddCP(pwszCP);

	return TRUE;
}