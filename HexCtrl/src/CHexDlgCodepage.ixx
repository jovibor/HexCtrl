/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
module;
#include <SDKDDKVer.h>
#include "../res/HexCtrlRes.h"
#include "../HexCtrl.h"
#include <Windows.h>
#include <commctrl.h>
#include <algorithm>
#include <format>
export module HEXCTRL:CHexDlgCodepage;

import :HexUtility;

namespace HEXCTRL::INTERNAL {
	class CHexDlgCodepage final {
	public:
		void AddCP(std::wstring_view wsv);
		void CreateDlg()const;
		void DestroyDlg();
		[[nodiscard]] auto GetHWND()const -> HWND;
		void Initialize(IHexCtrl* pHexCtrl, HINSTANCE hInstRes);
		[[nodiscard]] bool PreTranslateMsg(MSG* pMsg);
		[[nodiscard]] auto ProcessMsg(const MSG& msg) -> INT_PTR;
		void SetDlgProperties(std::uint64_t u64Flags);
		void ShowWindow(int iCmdShow);
	private:
		[[nodiscard]] bool IsNoEsc()const;
		auto OnActivate(const MSG& msg) -> INT_PTR;
		void OnCancel();
		auto OnClose() -> INT_PTR;
		auto OnCommand(const MSG& msg) -> INT_PTR;
		auto OnDestroy() -> INT_PTR;
		auto OnDrawItem(const MSG& msg) -> INT_PTR;
		auto OnInitDialog(const MSG& msg) -> INT_PTR;
		auto OnMeasureItem(const MSG& msg) -> INT_PTR;
		auto OnNotify(const MSG& msg) -> INT_PTR;
		void OnNotifyListGetColor(NMHDR* pNMHDR);
		void OnNotifyListGetDispInfo(NMHDR* pNMHDR);
		void OnNotifyListItemChanged(NMHDR* pNMHDR);
		void OnNotifyListLinkClick(NMHDR* pNMHDR);
		auto OnSize(const MSG& msg) -> INT_PTR;
		void SortList();
		static BOOL CALLBACK EnumCodePagesProc(LPWSTR pwszCP);
	private:
		struct CODEPAGE {
			int iCPID { };
			std::wstring wstrName;
			UINT uMaxChars { };
		};
		inline static CHexDlgCodepage* m_pThis { };
		HINSTANCE m_hInstRes { };
		GDIUT::CWnd m_Wnd;
		GDIUT::CDynLayout m_DynLayout;
		LISTEX::CListEx m_ListEx;
		std::vector<CODEPAGE> m_vecCodePage;
		IHexCtrl* m_pHexCtrl { };
		std::uint64_t m_u64Flags { }; //Data from SetDlgProperties.
	};
}

using namespace HEXCTRL::INTERNAL;

void CHexDlgCodepage::AddCP(std::wstring_view wsv)
{
	if (const auto optCPID = stn::StrToUInt32(wsv); optCPID) {
		if (CPINFOEXW stCP; ::GetCPInfoExW(*optCPID, 0, &stCP) != FALSE) {
			m_vecCodePage.emplace_back(static_cast<int>(*optCPID), stCP.CodePageName, stCP.MaxCharSize);
		}
	}
}

void CHexDlgCodepage::CreateDlg()const
{
	//m_Wnd is set in the OnInitDialog().
	if (const auto hWnd = ::CreateDialogParamW(m_hInstRes, MAKEINTRESOURCEW(IDD_HEXCTRL_CODEPAGE),
		m_pHexCtrl->GetWndHandle(EHexWnd::WND_MAIN), GDIUT::DlgProc<CHexDlgCodepage>, reinterpret_cast<LPARAM>(this));
		hWnd == nullptr) {
		ut::DBG_REPORT(L"CreateDialogParamW failed.");
	}
}

void CHexDlgCodepage::DestroyDlg()
{
	if (m_Wnd.IsWindow()) {
		m_Wnd.DestroyWindow();
	}
}

auto CHexDlgCodepage::GetHWND()const->HWND
{
	return m_Wnd;
}

void CHexDlgCodepage::Initialize(IHexCtrl* pHexCtrl, HINSTANCE hInstRes)
{
	if (pHexCtrl == nullptr || hInstRes == nullptr) {
		ut::DBG_REPORT(L"Initialize == nullptr");
		return;
	}

	m_pHexCtrl = pHexCtrl;
	m_hInstRes = hInstRes;
}

bool CHexDlgCodepage::PreTranslateMsg(MSG* pMsg)
{
	return m_Wnd.IsDlgMessage(pMsg);
}

auto CHexDlgCodepage::ProcessMsg(const MSG& msg)->INT_PTR
{
	switch (msg.message) {
	case WM_ACTIVATE: return OnActivate(msg);
	case WM_CLOSE: return OnClose();
	case WM_COMMAND: return OnCommand(msg);
	case WM_DESTROY: return OnDestroy();
	case WM_DRAWITEM: return OnDrawItem(msg);
	case WM_INITDIALOG: return OnInitDialog(msg);
	case WM_MEASUREITEM: return OnMeasureItem(msg);
	case WM_NOTIFY: return OnNotify(msg);
	case WM_SIZE: return OnSize(msg);
	default:
		return 0;
	}
}

void CHexDlgCodepage::SetDlgProperties(std::uint64_t u64Flags)
{
	m_u64Flags = u64Flags;
}

void CHexDlgCodepage::ShowWindow(int iCmdShow)
{
	if (!m_Wnd.IsWindow()) {
		CreateDlg();
	}

	m_Wnd.ShowWindow(iCmdShow);
}


//Private methods.

bool CHexDlgCodepage::IsNoEsc()const
{
	return m_u64Flags & HEXCTRL_FLAG_DLG_NOESC;
}

auto CHexDlgCodepage::OnActivate(const MSG& msg)->INT_PTR
{
	if (m_pHexCtrl == nullptr || !m_pHexCtrl->IsCreated())
		return FALSE;

	const auto nState = LOWORD(msg.wParam);
	if (nState == WA_ACTIVE || nState == WA_CLICKACTIVE) {
		m_ListEx.SetItemState(-1, 0, LVIS_SELECTED);
		if (const auto it = std::find_if(m_vecCodePage.begin(), m_vecCodePage.end(),
			[this](const CODEPAGE& ref) { return ref.iCPID == m_pHexCtrl->GetCodepage(); });
			it != m_vecCodePage.end()) {
			const auto iItem = static_cast<int>(it - m_vecCodePage.begin());
			m_ListEx.SetItemState(iItem, LVIS_SELECTED, LVIS_SELECTED);
			m_ListEx.EnsureVisible(iItem, FALSE);
		}
	}

	return FALSE; //Default handler.
}

void CHexDlgCodepage::OnCancel()
{
	if (IsNoEsc()) //Not closing Dialog on Escape key.
		return;

	OnClose();
}

auto CHexDlgCodepage::OnClose()->INT_PTR
{
	ShowWindow(SW_HIDE);
	return TRUE;
}

auto CHexDlgCodepage::OnCommand(const MSG& msg)->INT_PTR
{
	const auto uCtrlID = LOWORD(msg.wParam);
	switch (uCtrlID) {
	case IDCANCEL: OnCancel(); break;
	default:
		return FALSE;
	}
	return TRUE;
}

auto CHexDlgCodepage::OnDestroy()->INT_PTR
{
	m_vecCodePage.clear();
	m_u64Flags = { };
	m_pHexCtrl = nullptr;
	m_DynLayout.RemoveAll();

	return TRUE;
}

auto CHexDlgCodepage::OnDrawItem(const MSG& msg)->INT_PTR
{
	const auto pDIS = reinterpret_cast<LPDRAWITEMSTRUCT>(msg.lParam);
	if (pDIS->CtlID == static_cast<UINT>(IDC_HEXCTRL_CODEPAGE_LIST)) {
		m_ListEx.DrawItem(pDIS);
	}

	return TRUE;
}

auto CHexDlgCodepage::OnInitDialog(const MSG& msg)->INT_PTR
{
	m_Wnd.Attach(msg.hwnd);
	m_ListEx.Create({ .hWndParent { m_Wnd }, .uID { IDC_HEXCTRL_CODEPAGE_LIST }, .dwSizeFontList { 10 },
		.dwSizeFontHdr { 9 }, .fDialogCtrl { true }, .fLinks { true } });
	m_ListEx.SetExtendedStyle(LVS_EX_HEADERDRAGDROP);
	m_ListEx.SetSortable(true);
	m_ListEx.InsertColumn(0, L"Code page", LVCFMT_LEFT, 80);
	m_ListEx.InsertColumn(1, L"Name", LVCFMT_LEFT, 280);
	m_ListEx.InsertColumn(2, L"Max chars", LVCFMT_LEFT, 80);

	m_vecCodePage.emplace_back(-1, L"<link=\"https://en.wikipedia.org/wiki/ASCII\">ASCII 7-bit</link> (default)", 1);
	m_vecCodePage.emplace_back(0, L"Windows Internal UTF-16 (wchar_t)", 4);
	m_pThis = this;
	::EnumSystemCodePagesW(EnumCodePagesProc, CP_INSTALLED);
	m_ListEx.SetItemCountEx(static_cast<int>(m_vecCodePage.size()), LVSICF_NOSCROLL);

	m_DynLayout.SetHost(m_Wnd);
	m_DynLayout.AddItem(IDC_HEXCTRL_CODEPAGE_LIST, GDIUT::CDynLayout::MoveNone(), GDIUT::CDynLayout::SizeHorzAndVert(100, 100));
	m_DynLayout.Enable(true);

	return TRUE;
}

auto CHexDlgCodepage::OnMeasureItem(const MSG& msg)->INT_PTR
{
	const auto pMIS = reinterpret_cast<LPMEASUREITEMSTRUCT>(msg.lParam);
	if (pMIS->CtlID == static_cast<UINT>(IDC_HEXCTRL_CODEPAGE_LIST)) {
		m_ListEx.MeasureItem(pMIS);
	}

	return TRUE;
}

auto CHexDlgCodepage::OnNotify(const MSG& msg)->INT_PTR
{
	const auto pNMHDR = reinterpret_cast<NMHDR*>(msg.lParam);
	if (pNMHDR->idFrom == IDC_HEXCTRL_CODEPAGE_LIST) {
		switch (pNMHDR->code) {
		case LVN_COLUMNCLICK: SortList(); break;
		case LVN_GETDISPINFOW: OnNotifyListGetDispInfo(pNMHDR); break;
		case LVN_ITEMCHANGED: OnNotifyListItemChanged(pNMHDR); break;
		case LISTEX::LISTEX_MSG_GETCOLOR: OnNotifyListGetColor(pNMHDR); break;
		case LISTEX::LISTEX_MSG_LINKCLICK: OnNotifyListLinkClick(pNMHDR); break;
		default: break;
		}
	}

	return TRUE;
}

void CHexDlgCodepage::OnNotifyListGetColor(NMHDR* pNMHDR)
{
	if (const auto pLCI = reinterpret_cast<LISTEX::PLISTEXCOLORINFO>(pNMHDR);
		m_vecCodePage[static_cast<std::size_t>(pLCI->iItem)].uMaxChars > 1) {
		pLCI->stClr = { RGB(200, 80, 80), RGB(255, 255, 255) };
		return;
	}
}

void CHexDlgCodepage::OnNotifyListGetDispInfo(NMHDR* pNMHDR)
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

void CHexDlgCodepage::OnNotifyListItemChanged(NMHDR* pNMHDR)
{
	if (const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
		pNMI->iItem != -1 && pNMI->iSubItem != -1 && (pNMI->uNewState & LVIS_SELECTED)) {
		m_pHexCtrl->SetCodepage(m_vecCodePage[static_cast<std::size_t>(pNMI->iItem)].iCPID);
	}
}

void CHexDlgCodepage::OnNotifyListLinkClick(NMHDR* pNMHDR)
{
	const auto* const pLLI = reinterpret_cast<LISTEX::PLISTEXLINKINFO>(pNMHDR);
	::ShellExecuteW(nullptr, L"open", pLLI->pwszText, nullptr, nullptr, SW_SHOWNORMAL);
}

auto CHexDlgCodepage::OnSize(const MSG& msg)->INT_PTR
{
	const auto wWidth = LOWORD(msg.lParam);
	const auto wHeight = HIWORD(msg.lParam);
	m_DynLayout.OnSize(wWidth, wHeight);
	return TRUE;
}

void CHexDlgCodepage::SortList()
{
	const auto iColumn = m_ListEx.GetSortColumn();
	const auto fAscending = m_ListEx.GetSortAscending();
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

	m_ListEx.RedrawWindow();
}

BOOL CHexDlgCodepage::EnumCodePagesProc(LPWSTR pwszCP)
{
	m_pThis->AddCP(pwszCP);
	return TRUE;
}