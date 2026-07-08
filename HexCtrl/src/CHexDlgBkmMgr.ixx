/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
module;
#include <SDKDDKVer.h>
#include "../HexCtrl.h"
#include "res/HexCtrlRes.h"
#include <Windows.h>
#include <commctrl.h>
#include <algorithm>
#include <format>
#include <numeric>
#include <vector>
export module HEXCTRL:CHexDlgBkmMgr;

import :HexUtility;

namespace HEXCTRL::INTERNAL {
	class CHexDlgBkmMgr final : public IHexBookmarks {
	public:
		auto AddBkm(const HEXBKM& bkm) -> ULONGLONG override;
		void CreateDlg()const;
		void DestroyDlg();
		[[nodiscard]] auto GetAllBkms() -> SpanHexBkm override;
		[[nodiscard]] auto GetByID(ULONGLONG ullID) -> PHEXBKM override;
		[[nodiscard]] auto GetByIndex(ULONGLONG ullIndex) -> PHEXBKM override;
		[[nodiscard]] auto GetCount() -> ULONGLONG override;
		[[nodiscard]] auto GetCurrent()const -> ULONGLONG;
		[[nodiscard]] auto GetDlgItemHandle(EHexDlgItem eItem)const -> HWND;
		[[nodiscard]] auto GetHWND()const -> HWND;
		void GoBookmark(ULONGLONG ullIndex);
		void GoNext();
		void GoPrev();
		[[nodiscard]] bool HasBkmAtOffset(ULONGLONG ullOffset)const;
		[[nodiscard]] bool HasBookmarks()const;
		[[nodiscard]] auto HitTest(ULONGLONG ullOffset) -> PHEXBKM override;
		[[nodiscard]] auto HitTest(ULONGLONG ullOffset)const -> PHEXBKM;
		void Initialize(IHexCtrl &HexCtrl, HINSTANCE hInstRes);
		[[nodiscard]] bool IsShowTooltips()const;
		[[nodiscard]] bool IsVirtual()const;
		[[nodiscard]] bool PreTranslateMsg(MSG* pMsg);
		[[nodiscard]] auto ProcessMsg(const MSG& msg) -> INT_PTR;
		void RemoveAll()override;
		void RemoveByOffset(ULONGLONG ullOffset);
		void RemoveByID(ULONGLONG ullID)override;
		void SetDlgProperties(std::uint64_t u64Flags);
		void SetVirtualBkm(IHexVirtBookmarks* pVirtBkm)override;
		void ShowWindow(int iCmdShow);
		void Update(ULONGLONG ullID, const HEXBKM& bkm);
	private:
		enum class EMenuID : std::uint16_t;
		[[nodiscard]] auto GetHexCtrl()const -> IHexCtrl*;
		[[nodiscard]] bool IsNoEsc()const;
		[[nodiscard]] bool IsShowAsHex()const;
		void OnCancel();
		void OnCheckHex();
		void RedrawHexCtrl();
		void RemoveBkmByID(std::uint64_t ullID);
		void UpdateListCount(bool fPreserveSelected = false);
		auto WMClose() -> INT_PTR;
		auto WMCommand(const MSG& msg) -> INT_PTR;
		auto WMDestroy() -> INT_PTR;
		auto WMDPIChanged(const MSG& msg) -> INT_PTR;
		auto WMDrawItem(const MSG& msg) -> INT_PTR;
		auto WMGetDPIScaledSize(const MSG& msg) -> INT_PTR;
		auto WMInitDialog(const MSG& msg) -> INT_PTR;
		auto WMMeasureItem(const MSG& msg) -> INT_PTR;
		auto WMNotify(const MSG& msg) -> INT_PTR;
		void WMNotifyListColumnClick();
		void WMNotifyListDblClick(NMHDR* pNMHDR);
		void WMNotifyListGetColor(NMHDR* pNMHDR);
		void WMNotifyListGetDispInfo(NMHDR* pNMHDR);
		void WMNotifyListItemChanged(NMHDR* pNMHDR);
		void WMNotifyListRClick(NMHDR* pNMHDR);
		void WMNotifyListSetData(NMHDR* pNMHDR);
	private:
		HINSTANCE m_hInstRes { };
		GDIUT::CWnd m_Wnd;
		GDIUT::CWndBtn m_WndBtnHex; //Check-box "Hex numbers".
		GDIUT::CWndBtn m_WndBtnTT;  //Check-box "Show tooltips".
		GDIUT::CMenu m_menuList;
		GDIUT::CDynLayout m_DynLayout;
		std::vector<HEXBKM> m_vecBookmarks; //Bookmarks data.
		LISTEX::CListEx m_ListEx;
		IHexCtrl* m_pHexCtrl { };
		IHexVirtBookmarks* m_pVirtBkm { };
		std::uint64_t m_u64IndexCurr { }; //Current bookmark's position index, to move next/prev.
		std::uint64_t m_u64Flags { }; //Data from SetDlgProperties.
	};
}

using namespace HEXCTRL::INTERNAL;

enum class CHexDlgBkmMgr::EMenuID : std::uint16_t {
	IDM_BKMMGR_REMOVE = 0x8001, IDM_BKMMGR_REMOVEALL
};

auto CHexDlgBkmMgr::AddBkm(const HEXBKM& bkm)->ULONGLONG
{
	ULONGLONG ullID;
	if (IsVirtual()) {
		ullID = m_pVirtBkm->OnHexBkmAdd(bkm);
	}
	else {
		ullID = 1; //Bookmarks' ID starts from 1.
		if (const auto it = std::max_element(m_vecBookmarks.begin(), m_vecBookmarks.end(),
			[](const HEXBKM& lhs, const HEXBKM& rhs) {
				return lhs.ullID < rhs.ullID; }); it != m_vecBookmarks.end()) {
			ullID = it->ullID + 1; //Increasing next bookmark's ID by 1.
		}
		m_vecBookmarks.emplace_back(bkm.vecSpan, bkm.wstrDesc, ullID, bkm.ullData, bkm.stClr);
	}

	UpdateListCount(true);

	return ullID;
}

void CHexDlgBkmMgr::CreateDlg()const
{
	//m_Wnd is set in the WMInitDialog().
	if (const auto hWnd = ::CreateDialogParamW(m_hInstRes, MAKEINTRESOURCEW(IDD_HEXCTRL_BKMMGR),
		m_pHexCtrl->GetWndHandle(EHexWnd::WND_MAIN), GDIUT::DlgProc<CHexDlgBkmMgr>, reinterpret_cast<LPARAM>(this));
		hWnd == nullptr) {
		ut::DBG_REPORT(L"CreateDialogParamW failed.");
	}
}

void CHexDlgBkmMgr::DestroyDlg()
{
	if (m_Wnd.IsWindow()) {
		m_Wnd.DestroyWindow();
	}
}

auto CHexDlgBkmMgr::GetAllBkms()->SpanHexBkm
{
	if (IsVirtual()) {
		return { };
	}

	return m_vecBookmarks;
}

auto CHexDlgBkmMgr::GetByID(ULONGLONG ullID)->PHEXBKM
{
	if (IsVirtual()) {
		return m_pVirtBkm->OnHexBkmGetByID(ullID);
	}

	if (const auto it = std::find_if(m_vecBookmarks.begin(), m_vecBookmarks.end(),
		[ullID](const HEXBKM& ref) { return ullID == ref.ullID; }); it != m_vecBookmarks.end()) {
		return  &*it;
	}

	return { };
}

auto CHexDlgBkmMgr::GetByIndex(ULONGLONG ullIndex)->PHEXBKM
{
	if (IsVirtual()) {
		return m_pVirtBkm->OnHexBkmGetByIndex(ullIndex);
	}

	if (ullIndex < m_vecBookmarks.size()) {
		return &m_vecBookmarks[static_cast<std::size_t>(ullIndex)];
	}

	return { };
}

auto CHexDlgBkmMgr::GetCount()->ULONGLONG
{
	return IsVirtual() ? m_pVirtBkm->OnHexBkmGetCount() : m_vecBookmarks.size();
}

auto CHexDlgBkmMgr::GetCurrent()const->ULONGLONG
{
	return m_u64IndexCurr;
}

auto CHexDlgBkmMgr::GetDlgItemHandle(EHexDlgItem eItem)const->HWND
{
	if (!m_Wnd.IsWindow()) {
		return { };
	}

	using enum EHexDlgItem;
	switch (eItem) {
	case BKMMGR_CHK_HEX:
		return m_WndBtnHex;
	case BKMMGR_CHK_TT:
		return m_WndBtnTT;
	default:
		return { };
	}
}

auto CHexDlgBkmMgr::GetHWND()const->HWND
{
	return m_Wnd;
}

void CHexDlgBkmMgr::GoBookmark(ULONGLONG ullIndex)
{
	if (m_pHexCtrl == nullptr || !m_pHexCtrl->IsDataSet())
		return;

	if (const auto* const pBkm = GetByIndex(ullIndex); pBkm != nullptr) {
		m_u64IndexCurr = ullIndex;
		const auto ullOffset = pBkm->vecSpan.front().ullOffset;
		m_pHexCtrl->SetCaretPos(ullOffset);
		if (!m_pHexCtrl->IsOffsetVisible(ullOffset)) {
			m_pHexCtrl->GoToOffset(ullOffset);
		}
	}
}

void CHexDlgBkmMgr::GoNext()
{
	if (m_pHexCtrl == nullptr || !m_pHexCtrl->IsDataSet())
		return;

	if (++m_u64IndexCurr >= GetCount()) {
		m_u64IndexCurr = 0;
	}

	if (const auto* const pBkm = GetByIndex(m_u64IndexCurr); pBkm != nullptr) {
		const auto ullOffset = pBkm->vecSpan.front().ullOffset;
		m_pHexCtrl->SetCaretPos(ullOffset);
		if (!m_pHexCtrl->IsOffsetVisible(ullOffset)) {
			m_pHexCtrl->GoToOffset(ullOffset);
		}
	}
}

void CHexDlgBkmMgr::GoPrev()
{
	if (m_pHexCtrl == nullptr || !m_pHexCtrl->IsDataSet())
		return;

	if (m_u64IndexCurr == 0 || (m_u64IndexCurr - 1) >= GetCount()) {
		m_u64IndexCurr = GetCount() - 1;
	}
	else {
		--m_u64IndexCurr;
	}

	if (const auto* const pBkm = GetByIndex(m_u64IndexCurr); pBkm != nullptr) {
		const auto ullOffset = pBkm->vecSpan.front().ullOffset;
		m_pHexCtrl->SetCaretPos(ullOffset);
		if (!m_pHexCtrl->IsOffsetVisible(ullOffset)) {
			m_pHexCtrl->GoToOffset(ullOffset);
		}
	}
}

bool CHexDlgBkmMgr::HasBkmAtOffset(ULONGLONG ullOffset)const
{
	return HitTest(ullOffset) != nullptr;
}

bool CHexDlgBkmMgr::HasBookmarks()const
{
	return IsVirtual() ? m_pVirtBkm->OnHexBkmGetCount() > 0 : !m_vecBookmarks.empty();
}

auto CHexDlgBkmMgr::HitTest(ULONGLONG ullOffset)->PHEXBKM
{
	if (IsVirtual()) {
		return m_pVirtBkm->OnHexBkmHitTest(ullOffset);
	}

	if (const auto rit = std::find_if(m_vecBookmarks.rbegin(), m_vecBookmarks.rend(),
		[ullOffset](const HEXBKM& ref) { return std::any_of(ref.vecSpan.begin(), ref.vecSpan.end(),
			[ullOffset](const HEXSPAN& refV) {
				return ullOffset >= refV.ullOffset && ullOffset < (refV.ullOffset + refV.ullSize); }); });
					rit != m_vecBookmarks.rend()) {
		return &*rit;
	}

	return { };
}

auto CHexDlgBkmMgr::HitTest(ULONGLONG ullOffset)const->PHEXBKM
{
	return const_cast<CHexDlgBkmMgr*>(this)->HitTest(ullOffset);
}

void CHexDlgBkmMgr::Initialize(IHexCtrl &HexCtrl, HINSTANCE hInstRes)
{
	m_pHexCtrl = &HexCtrl;
	m_hInstRes = hInstRes;
}

bool CHexDlgBkmMgr::IsShowTooltips()const
{
	return m_WndBtnTT.IsWindow() && m_WndBtnTT.IsChecked();
}

bool CHexDlgBkmMgr::IsVirtual()const
{
	return m_pVirtBkm != nullptr;
}

bool CHexDlgBkmMgr::PreTranslateMsg(MSG* pMsg)
{
	return m_Wnd.IsDlgMessage(pMsg);
}

auto CHexDlgBkmMgr::ProcessMsg(const MSG& msg)->INT_PTR
{
	switch (msg.message) {
	case WM_CLOSE: return WMClose();
	case WM_COMMAND: return WMCommand(msg);
	case WM_DESTROY: return WMDestroy();
	case WM_DPICHANGED: return WMDPIChanged(msg);
	case WM_DRAWITEM: return WMDrawItem(msg);
	case WM_GETDPISCALEDSIZE: return WMGetDPIScaledSize(msg);
	case WM_INITDIALOG: return WMInitDialog(msg);
	case WM_MEASUREITEM: return WMMeasureItem(msg);
	case WM_NOTIFY: return WMNotify(msg);
	default:
		return 0;
	}
}

void CHexDlgBkmMgr::RemoveAll()
{
	IsVirtual() ? m_pVirtBkm->OnHexBkmRemoveAll() : m_vecBookmarks.clear();
	UpdateListCount();
}

void CHexDlgBkmMgr::RemoveByOffset(ULONGLONG ullOffset)
{
	if (IsVirtual()) {
		if (const auto* const pBkm = m_pVirtBkm->OnHexBkmHitTest(ullOffset); pBkm != nullptr) {
			m_pVirtBkm->OnHexBkmRemoveByID(pBkm->ullID);
		}
	}
	else {
		if (m_vecBookmarks.empty())
			return;

		//Searching from the end, to remove last added bookmark if there are few at the given offset.
		if (const auto it = std::find_if(m_vecBookmarks.rbegin(), m_vecBookmarks.rend(),
			[ullOffset](const HEXBKM& ref) { return std::any_of(ref.vecSpan.begin(), ref.vecSpan.end(),
				[ullOffset](const HEXSPAN& refV) {
					return ullOffset >= refV.ullOffset && ullOffset < (refV.ullOffset + refV.ullSize); }); });
			it != m_vecBookmarks.rend()) {
			RemoveBkmByID(it->ullID);
		}
	}

	UpdateListCount();
}

void CHexDlgBkmMgr::RemoveByID(ULONGLONG ullID)
{
	if (IsVirtual()) {
		m_pVirtBkm->OnHexBkmRemoveByID(ullID);
	}
	else {
		if (const auto pBkm = GetByID(ullID); pBkm != nullptr) {
			RemoveBkmByID(pBkm->ullID);
		}
	}

	UpdateListCount();
}

void CHexDlgBkmMgr::SetDlgProperties(std::uint64_t u64Flags)
{
	m_u64Flags = u64Flags;
}

void CHexDlgBkmMgr::SetVirtualBkm(IHexVirtBookmarks* pVirtBkm)
{
	m_pVirtBkm = pVirtBkm;
	UpdateListCount();
}

void CHexDlgBkmMgr::ShowWindow(int iCmdShow)
{
	if (!m_Wnd.IsWindow()) {
		CreateDlg();
	}

	m_Wnd.ShowWindow(iCmdShow);
}

void CHexDlgBkmMgr::Update(ULONGLONG ullID, const HEXBKM& bkm)
{
	if (IsVirtual())
		return;

	if (const auto it = std::find_if(m_vecBookmarks.begin(), m_vecBookmarks.end(),
		[ullID](const HEXBKM& ref) { return ullID == ref.ullID; }); it != m_vecBookmarks.end()) {
		*it = bkm;
	}

	UpdateListCount();
}


//Private methods.

auto CHexDlgBkmMgr::GetHexCtrl()const->IHexCtrl*
{
	return m_pHexCtrl;
}

bool CHexDlgBkmMgr::IsNoEsc()const
{
	return m_u64Flags & HEXCTRL_FLAG_DLG_NOESC;
}

bool CHexDlgBkmMgr::IsShowAsHex()const
{
	return m_WndBtnHex.IsChecked();
}

void CHexDlgBkmMgr::OnCancel()
{
	if (IsNoEsc()) //Not closing Dialog on Escape key.
		return;

	WMClose();
}

void CHexDlgBkmMgr::OnCheckHex()
{
	m_ListEx.RedrawWindow();
}
void CHexDlgBkmMgr::RedrawHexCtrl()
{
	if (m_pHexCtrl != nullptr && m_pHexCtrl->IsDataSet()) {
		m_pHexCtrl->Redraw();
	}
}

void CHexDlgBkmMgr::RemoveBkmByID(std::uint64_t ullID)
{
	if (const auto it = std::find_if(m_vecBookmarks.begin(), m_vecBookmarks.end(),
		[ullID](const HEXBKM& bkm) { return ullID == bkm.ullID; }); it != m_vecBookmarks.end()) {
		m_vecBookmarks.erase(it);
	}
}

void CHexDlgBkmMgr::UpdateListCount(bool fPreserveSelected)
{
	if (!m_Wnd.IsWindow()) {
		return;
	}

	m_ListEx.SetItemState(-1, 0, LVIS_SELECTED);
	m_ListEx.SetItemCountEx(static_cast<int>(GetCount()), LVSICF_NOSCROLL);

	if (fPreserveSelected) {
		if (GetCount() > 0) {
			const auto iCurrent = static_cast<int>(GetCurrent());
			m_ListEx.SetItemState(iCurrent, LVIS_SELECTED, LVIS_SELECTED);
			m_ListEx.EnsureVisible(iCurrent, FALSE);
		}
	}
}

auto CHexDlgBkmMgr::WMClose()->INT_PTR
{
	ShowWindow(SW_HIDE);
	return TRUE;
}

auto CHexDlgBkmMgr::WMCommand(const MSG& msg) -> INT_PTR
{
	const auto uCtrlID = LOWORD(msg.wParam); //Control ID or menu ID.
	const auto uCode = HIWORD(msg.wParam);   //Control code, zero for menu.
	const auto hWndCtrl = reinterpret_cast<HWND>(msg.lParam); //Control HWND, zero for menu.

	//IDOK and IDCANCEL don't have HWND in lParam, if send as result of
	//IsDialogMessage and no button with such ID presents in the dialog.
	if (hWndCtrl != nullptr || uCtrlID == IDOK || uCtrlID == IDCANCEL) { //Notifications from controls.
		if (uCode != BN_CLICKED) { return FALSE; }
		switch (uCtrlID) {
		case IDCANCEL: OnCancel(); break;
		case IDC_HEXCTRL_BKMMGR_CHK_HEX: OnCheckHex(); break;
		default: return FALSE;
		}
	}
	else {
		switch (static_cast<EMenuID>(uCtrlID)) {
		case EMenuID::IDM_BKMMGR_REMOVE:
		{
			const auto uSelCount = m_ListEx.GetSelectedCount();
			std::vector<int> vecIndex;
			vecIndex.reserve(uSelCount);
			int iItem { -1 };
			for (auto i = 0UL; i < uSelCount; ++i) {
				iItem = m_ListEx.GetNextItem(iItem, LVNI_SELECTED);
				vecIndex.insert(vecIndex.begin(), iItem); //Last indexes go first.
			}

			for (const auto iIndexBkm : vecIndex) {
				if (const auto pBkm = GetByIndex(iIndexBkm); pBkm != nullptr) {
					RemoveBkmByID(pBkm->ullID);
				}
			}

			UpdateListCount();
			RedrawHexCtrl();
		}
		break;
		case EMenuID::IDM_BKMMGR_REMOVEALL:
			RemoveAll();
			RedrawHexCtrl();
			break;
		default:
			return FALSE;
		}
	}

	return TRUE;
}

auto CHexDlgBkmMgr::WMDestroy()->INT_PTR
{
	RemoveAll();
	m_menuList.DestroyMenu();
	m_u64Flags = { };
	m_DynLayout.RemoveAll();

	return TRUE;
}

auto CHexDlgBkmMgr::WMDPIChanged([[maybe_unused]] const MSG& msg)->INT_PTR
{
	m_DynLayout.Enable(true);
	return 0;
}

auto CHexDlgBkmMgr::WMDrawItem(const MSG& msg)->INT_PTR
{
	const auto pDIS = reinterpret_cast<LPDRAWITEMSTRUCT>(msg.lParam);
	if (pDIS->CtlID == static_cast<UINT>(IDC_HEXCTRL_BKMMGR_LIST)) {
		m_ListEx.DrawItem(pDIS);
	}

	return TRUE;
}

auto CHexDlgBkmMgr::WMGetDPIScaledSize([[maybe_unused]] const MSG& msg)->INT_PTR
{
	//This message is sent to top-level windows with a DPI_AWARENESS_CONTEXT
	//of Per Monitor v2 before a WM_DPICHANGED message is sent.
	//We use it to temporarily disable all dynamic layout resizes,
	//to re-enable it later in the WM_DPICHANGED handler.

	m_DynLayout.Enable(false);
	return 0;
}

auto CHexDlgBkmMgr::WMInitDialog(const MSG& msg)->INT_PTR
{
	m_Wnd.Attach(msg.hwnd);
	m_WndBtnHex.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_BKMMGR_CHK_HEX));
	m_WndBtnTT.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_BKMMGR_CHK_TT));

	m_ListEx.Create({ .hWndParent { m_Wnd }, .uID { IDC_HEXCTRL_BKMMGR_LIST }, .flSizeFontList { 10.F },
		.flSizeFontHdr { 10.F }, .fDialogCtrl { true }, .fSortable { true } });
	m_ListEx.SetExtendedStyle(LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT);
	const auto flDPIScale = GDIUT::GetDPIScaleForHWND(m_ListEx);
	m_ListEx.InsertColumn(0, L"№", LVCFMT_LEFT, std::lround(40 * flDPIScale));
	m_ListEx.InsertColumn(1, L"Offset", LVCFMT_LEFT, std::lround(80 * flDPIScale), -1, 0, true);
	m_ListEx.InsertColumn(2, L"Size", LVCFMT_LEFT, std::lround(50 * flDPIScale), -1, 0, true);
	m_ListEx.InsertColumn(3, L"Description", LVCFMT_LEFT, std::lround(250 * flDPIScale), -1, 0, true);
	m_ListEx.InsertColumn(4, L"Bk", LVCFMT_LEFT, std::lround(40 * flDPIScale));
	m_ListEx.InsertColumn(5, L"Text", LVCFMT_LEFT, std::lround(40 * flDPIScale));

	m_menuList.CreatePopupMenu();
	m_menuList.AppendString(static_cast<UINT_PTR>(EMenuID::IDM_BKMMGR_REMOVE), L"Remove");
	m_menuList.AppendSepar();
	m_menuList.AppendString(static_cast<UINT_PTR>(EMenuID::IDM_BKMMGR_REMOVEALL), L"Remove All");

	m_WndBtnHex.SetCheck(true);
	m_WndBtnTT.SetCheck(true);

	m_DynLayout.Initialize(m_Wnd);
	m_DynLayout.AddItem(IDC_HEXCTRL_BKMMGR_LIST, GDIUT::CDynLayout::MoveNone(), GDIUT::CDynLayout::SizeHorzAndVert(100, 100));
	m_DynLayout.AddItem(IDC_HEXCTRL_BKMMGR_CHK_HEX, GDIUT::CDynLayout::MoveVert(100), GDIUT::CDynLayout::SizeNone());
	m_DynLayout.AddItem(IDC_HEXCTRL_BKMMGR_CHK_TT, GDIUT::CDynLayout::MoveVert(100), GDIUT::CDynLayout::SizeNone());
	m_DynLayout.Enable(true);

	UpdateListCount();

	return TRUE;
}

auto CHexDlgBkmMgr::WMMeasureItem(const MSG& msg)->INT_PTR
{
	const auto pMIS = reinterpret_cast<LPMEASUREITEMSTRUCT>(msg.lParam);
	if (pMIS->CtlID == static_cast<UINT>(IDC_HEXCTRL_BKMMGR_LIST)) {
		m_ListEx.MeasureItem(pMIS);
	}

	return TRUE;
}

auto CHexDlgBkmMgr::WMNotify(const MSG& msg)->INT_PTR
{
	const auto pNMHDR = reinterpret_cast<NMHDR*>(msg.lParam);
	switch (pNMHDR->idFrom) {
	case IDC_HEXCTRL_BKMMGR_LIST:
		switch (pNMHDR->code) {
		case LVN_COLUMNCLICK: WMNotifyListColumnClick(); break;
		case LVN_GETDISPINFOW: WMNotifyListGetDispInfo(pNMHDR); break;
		case LVN_ITEMCHANGED: WMNotifyListItemChanged(pNMHDR); break;
		case NM_DBLCLK: WMNotifyListDblClick(pNMHDR); break;
		case NM_RCLICK: WMNotifyListRClick(pNMHDR); break;
		case LISTEX::LISTEX_MSG_GETCOLOR: WMNotifyListGetColor(pNMHDR); break;
		case LISTEX::LISTEX_MSG_SETDATA: WMNotifyListSetData(pNMHDR); break;
		default: break;
		}
	default: break;
	}

	return TRUE;
}

void CHexDlgBkmMgr::WMNotifyListColumnClick()
{
	if (IsVirtual())
		return;

	//Sort bookmarks according to a clicked column.
	const auto iColumn = m_ListEx.GetSortColumn();
	const auto fAscending = m_ListEx.GetSortAscending();
	std::sort(m_vecBookmarks.begin(), m_vecBookmarks.end(),
		[iColumn, fAscending](const HEXBKM& st1, const HEXBKM& st2) {
			int iCompare { };
			switch (iColumn) {
			case 0:
				break;
			case 1: //Offset.
				if (!st1.vecSpan.empty() && !st2.vecSpan.empty()) {
					const auto ullOffset1 = st1.vecSpan.front().ullOffset;
					const auto ullOffset2 = st2.vecSpan.front().ullOffset;
					iCompare = ullOffset1 != ullOffset2 ? (ullOffset1 < ullOffset2 ? -1 : 1) : 0;
				}
				break;
			case 2: //Size.
				if (!st1.vecSpan.empty() && !st2.vecSpan.empty()) {
					auto ullSize1 = std::reduce(st1.vecSpan.begin(), st1.vecSpan.end(), 0ULL,
						[](auto ullTotal, const HEXSPAN& ref) { return ullTotal + ref.ullSize; });
					auto ullSize2 = std::reduce(st2.vecSpan.begin(), st2.vecSpan.end(), 0ULL,
						[](auto ullTotal, const HEXSPAN& ref) { return ullTotal + ref.ullSize; });
					iCompare = ullSize1 != ullSize2 ? (ullSize1 < ullSize2 ? -1 : 1) : 0;
				}
				break;
			case 3: //Description.
				iCompare = st1.wstrDesc.compare(st2.wstrDesc);
				break;
			default:
				break;
			}

			return fAscending ? iCompare < 0 : iCompare > 0;
	});

	if (::IsWindow(m_ListEx.GetHWND())) {
		m_ListEx.RedrawWindow();
	}
}

void CHexDlgBkmMgr::WMNotifyListDblClick(NMHDR* pNMHDR)
{
	const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	if (pNMI->iSubItem != 4 && pNMI->iSubItem != 5) {
		return;
	}

	const auto pBkm = GetByIndex(static_cast<std::size_t>(pNMI->iItem));
	if (pBkm == nullptr) {
		return;
	}

	COLORREF* pClr { };
	switch (pNMI->iSubItem) {
	case 4: //Bk color.
		pClr = &pBkm->stClr.clrBk;
		break;
	case 5: //Text color.
		pClr = &pBkm->stClr.clrText;
		break;
	default:
		return;
	}

	COLORREF arrClr[16] { };
	CHOOSECOLORW stChclr { .lStructSize { sizeof(CHOOSECOLORW) }, .rgbResult { *pClr },
		.lpCustColors { arrClr }, .Flags { CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT } };
	if (::ChooseColorW(&stChclr) == FALSE) {
		return;
	}

	*pClr = stChclr.rgbResult;
	m_pHexCtrl->Redraw();
}

void CHexDlgBkmMgr::WMNotifyListGetColor(NMHDR* pNMHDR)
{
	const auto pLCI = reinterpret_cast<LISTEX::PLISTEXCOLORINFO>(pNMHDR);

	switch (pLCI->iSubItem) {
	case 4: //Bk color.
		if (const auto* const pBkm = GetByIndex(static_cast<std::size_t>(pLCI->iItem)); pBkm != nullptr) {
			pLCI->stClr.clrBk = pBkm->stClr.clrBk;
			return;
		}
		break;
	case 5: //Text color.
		if (const auto* const pBkm = GetByIndex(static_cast<std::size_t>(pLCI->iItem)); pBkm != nullptr) {
			pLCI->stClr.clrBk = pBkm->stClr.clrText;
			return;
		}
		break;
	default:
		break;
	}
}

void CHexDlgBkmMgr::WMNotifyListGetDispInfo(NMHDR* pNMHDR)
{
	const auto pDispInfo = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
	const auto pItem = &pDispInfo->item;
	if ((pItem->mask & LVIF_TEXT) == 0)
		return;

	const auto iItem = pItem->iItem;
	const auto pBkm = GetByIndex(static_cast<ULONGLONG>(iItem));
	if (pBkm == nullptr)
		return;

	ULONGLONG ullOffset { 0 };
	ULONGLONG ullSize { 0 };
	switch (pItem->iSubItem) {
	case 0: //Index number.
		*std::format_to(pItem->pszText, L"{}", iItem + 1) = L'\0';
		break;
	case 1: //Offset.
		if (!pBkm->vecSpan.empty()) {
			ullOffset = GetHexCtrl()->GetOffset(pBkm->vecSpan.front().ullOffset, true); //Display virtual offset.
		}
		*std::vformat_to(pItem->pszText, IsShowAsHex() ? L"0x{:X}" : L"{}", std::make_wformat_args(ullOffset)) = L'\0';
		break;
	case 2: //Size.
		if (!pBkm->vecSpan.empty()) {
			ullSize = std::reduce(pBkm->vecSpan.begin(), pBkm->vecSpan.end(), 0ULL,
				[](auto ullTotal, const HEXSPAN& ref) { return ullTotal + ref.ullSize; });
		}
		*std::vformat_to(pItem->pszText, IsShowAsHex() ? L"0x{:X}" : L"{}", std::make_wformat_args(ullSize)) = L'\0';
		break;
	case 3: //Description.
		pItem->pszText = pBkm->wstrDesc.data();
		break;
	default:
		break;
	}
}

void CHexDlgBkmMgr::WMNotifyListItemChanged(NMHDR* pNMHDR)
{
	//Go selected bookmark only with keyboard arrows and LMouse clicks.
	//Does not trigger (LVN_ITEMCHANGED event) when updating bookmark: !(pLCI->uNewState & LVIS_SELECTED)
	if (const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
		pNMI->iItem >= 0 && pNMI->iSubItem >= 0 && (pNMI->uNewState & LVIS_SELECTED)) {
		GoBookmark(static_cast<ULONGLONG>(pNMI->iItem));
	}
}

void CHexDlgBkmMgr::WMNotifyListRClick(NMHDR* pNMHDR)
{
	bool fEnabled { false };
	if (const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
		pNMI->iItem >= 0 && pNMI->iSubItem >= 0) {
		fEnabled = true;
	}

	//Edit menu enabled only when one item selected.
	m_menuList.EnableItem(static_cast<UINT>(EMenuID::IDM_BKMMGR_REMOVE), fEnabled);
	m_menuList.EnableItem(static_cast<UINT>(EMenuID::IDM_BKMMGR_REMOVEALL), m_ListEx.GetItemCount() > 0);

	POINT pt;
	::GetCursorPos(&pt);
	m_menuList.TrackPopupMenu(pt.x, pt.y, m_Wnd);
}

void CHexDlgBkmMgr::WMNotifyListSetData(NMHDR* pNMHDR)
{
	const auto pLDI = reinterpret_cast<LISTEX::PLISTEXDATAINFO>(pNMHDR);
	const auto pBkm = GetByIndex(static_cast<std::size_t>(pLDI->iItem));
	if (pBkm == nullptr) {
		return;
	}

	switch (pLDI->iSubItem) {
	case 1: //Offset.
	{
		const auto optOffset = stn::StrToUInt64(pLDI->pwszData);
		if (!optOffset) {
			::MessageBoxW(m_Wnd, L"Invalid offset format.", L"Format error", MB_ICONERROR);
			return;
		}

		const auto ullOffset = GetHexCtrl()->GetOffset(*optOffset, false); //Always use flat offset.
		const auto ullOffsetCurr = pBkm->vecSpan.front().ullOffset;
		if (ullOffsetCurr != ullOffset) {
			const auto ullSizeCurr = std::reduce(pBkm->vecSpan.begin(), pBkm->vecSpan.end(), 0ULL,
				[](auto ullTotal, const HEXSPAN& ref) { return ullTotal + ref.ullSize; });
			pBkm->vecSpan.clear();
			pBkm->vecSpan.emplace_back(ullOffset, ullSizeCurr);
		}
	}
	break;
	case 2: //Size.
	{
		const auto optSize = stn::StrToUInt64(pLDI->pwszData);
		if (!optSize) {
			::MessageBoxW(m_Wnd, L"Invalid size format.", L"Format error", MB_ICONERROR);
			return;
		}

		if (*optSize == 0) {
			::MessageBoxW(m_Wnd, L"Size can't be zero.", L"Size error", MB_ICONERROR);
			return;
		}

		const auto ullSizeCurr = std::reduce(pBkm->vecSpan.begin(), pBkm->vecSpan.end(), 0ULL,
			[](auto ullTotal, const HEXSPAN& ref) { return ullTotal + ref.ullSize; });
		if (ullSizeCurr != *optSize) {
			const auto ullOffsetCurr = pBkm->vecSpan.front().ullOffset;
			pBkm->vecSpan.clear();
			pBkm->vecSpan.emplace_back(ullOffsetCurr, *optSize);
		}
	}
	break;
	case 3: //Description.
		pBkm->wstrDesc = pLDI->pwszData;
		break;
	default:
		return;
	}

	m_pHexCtrl->Redraw();
}