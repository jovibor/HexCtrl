/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "../../res/HexCtrlRes.h"
#include "CHexDlgCodepage.h"
#include <algorithm>
#include <cassert>
#include <format>

using namespace HEXCTRL::INTERNAL;

BEGIN_MESSAGE_MAP(CHexDlgCodepage, CDialogEx)
	ON_NOTIFY(LVN_GETDISPINFOW, IDC_HEXCTRL_CODEPAGE_LIST, &CHexDlgCodepage::OnListGetDispInfo)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_HEXCTRL_CODEPAGE_LIST, &CHexDlgCodepage::OnListItemChanged)
	ON_NOTIFY(LISTEX::LISTEX_MSG_GETCOLOR, IDC_HEXCTRL_CODEPAGE_LIST, &CHexDlgCodepage::OnListGetColor)
	ON_NOTIFY(LISTEX::LISTEX_MSG_LINKCLICK, IDC_HEXCTRL_CODEPAGE_LIST, &CHexDlgCodepage::OnListLinkClick)
	ON_WM_ACTIVATE()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CHexDlgCodepage::AddCP(std::wstring_view wsv)
{
	if (const auto optCPID = stn::StrToUInt32(wsv); optCPID) {
		if (CPINFOEXW stCP; GetCPInfoExW(*optCPID, 0, &stCP) != FALSE) {
			m_vecCodePage.emplace_back(static_cast<int>(*optCPID), stCP.CodePageName, stCP.MaxCharSize);
		}
	}
}

void CHexDlgCodepage::Initialize(IHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	if (pHexCtrl == nullptr)
		return;

	m_pHexCtrl = pHexCtrl;
}

bool CHexDlgCodepage::PreTranslateMsg(MSG* pMsg)
{
	if (m_hWnd == nullptr)
		return false;

	if (::IsDialogMessageW(m_hWnd, pMsg) != FALSE) { return true; }

	return false;
}

void CHexDlgCodepage::SetDlgProperties(std::uint64_t u64Flags)
{
	m_u64Flags = u64Flags;
}

BOOL CHexDlgCodepage::ShowWindow(int nCmdShow)
{
	if (!IsWindow(m_hWnd)) {
		Create(IDD_HEXCTRL_CODEPAGE, CWnd::FromHandle(m_pHexCtrl->GetWndHandle(EHexWnd::WND_MAIN)));
	}

	return CDialogEx::ShowWindow(nCmdShow);
}


//Private methods.

void CHexDlgCodepage::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

bool CHexDlgCodepage::IsNoEsc()const
{
	return m_u64Flags & HEXCTRL_FLAG_DLG_NOESC;
}

void CHexDlgCodepage::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if (!m_pHexCtrl->IsCreated())
		return;

	if (nState == WA_ACTIVE || nState == WA_CLICKACTIVE) {
		m_pList->SetItemState(-1, 0, LVIS_SELECTED);
		if (const auto iter = std::find_if(m_vecCodePage.begin(), m_vecCodePage.end(),
			[this](const CODEPAGE& ref) { return ref.iCPID == m_pHexCtrl->GetCodepage(); });
			iter != m_vecCodePage.end()) {
			const auto iItem = static_cast<int>(iter - m_vecCodePage.begin());
			m_pList->SetItemState(iItem, LVIS_SELECTED, LVIS_SELECTED);
			m_pList->EnsureVisible(iItem, FALSE);
		}
	}

	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
}

void CHexDlgCodepage::OnCancel()
{
	if (IsNoEsc()) //Not closing Dialog on Escape key.
		return;

	CDialogEx::OnCancel();
}

void CHexDlgCodepage::OnDestroy()
{
	CDialogEx::OnDestroy();

	m_vecCodePage.clear();
	m_u64Flags = { };
	m_pHexCtrl = nullptr;
}

void CHexDlgCodepage::OnClose()
{
	EndDialog(IDCANCEL);
}

BOOL CHexDlgCodepage::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_pList->Create({ .pParent { this }, .uID { IDC_HEXCTRL_CODEPAGE_LIST }, .dwSizeFontList { 10 },
		.dwSizeFontHdr { 10 }, .fDialogCtrl { true } });
	m_pList->SetExtendedStyle(LVS_EX_HEADERDRAGDROP);
	m_pList->SetSortable(true);
	m_pList->InsertColumn(0, L"Code page", LVCFMT_LEFT, 80);
	m_pList->InsertColumn(1, L"Name", LVCFMT_LEFT, 280);
	m_pList->InsertColumn(2, L"Max chars", LVCFMT_LEFT, 80);

	m_vecCodePage.emplace_back(-1, L"<link=\"https://en.wikipedia.org/wiki/ASCII\">ASCII 7-bit</link> (default)", 1);
	m_vecCodePage.emplace_back(0, L"Windows Internal UTF-16 (wchar_t)", 4);
	m_pThis = this;
	EnumSystemCodePagesW(EnumCodePagesProc, CP_INSTALLED);
	m_pList->SetItemCountEx(static_cast<int>(m_vecCodePage.size()), LVSICF_NOSCROLL);

	if (const auto pLayout = GetDynamicLayout(); pLayout != nullptr) {
		pLayout->SetMinSize({ 0, 0 });
	}

	return TRUE;
}

void CHexDlgCodepage::OnListGetDispInfo(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pDispInfo = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
	const auto pItem = &pDispInfo->item;
	if ((pItem->mask & LVIF_TEXT) == 0)
		return;

	const auto iItem = static_cast<std::size_t>(pItem->iItem);
	switch (pItem->iSubItem) {
	case 0: //Code page ID.
		*std::format_to(pItem->pszText, L"{}", m_vecCodePage[iItem].iCPID) = L'\0';
		break;
	case 1: //Name.
		pItem->pszText = m_vecCodePage[iItem].wstrName.data();
		break;
	case 2: //Max chars.
		*std::format_to(pItem->pszText, L"{}", m_vecCodePage[iItem].uMaxChars) = L'\0';
		break;
	default:
		break;
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

void CHexDlgCodepage::SortList()
{
	const auto iColumn = m_pList->GetSortColumn();
	const auto fAscending = m_pList->GetSortAscending();
	std::sort(m_vecCodePage.begin() + 1, m_vecCodePage.end(),
		[iColumn, fAscending](const CODEPAGE& st1, const CODEPAGE& st2) {
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

	m_pList->RedrawWindow();
}

BOOL CHexDlgCodepage::EnumCodePagesProc(LPWSTR pwszCP)
{
	m_pThis->AddCP(pwszCP);

	return TRUE;
}