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
		auto AddBkm(const HEXBKM& hbs, bool fRedraw) -> ULONGLONG override;    //Returns new bookmark Id.
		void CreateDlg()const;
		void DestroyDlg();
		[[nodiscard]] auto GetByID(ULONGLONG ullID) -> PHEXBKM override;       //Bookmark by ID.
		[[nodiscard]] auto GetByIndex(ULONGLONG ullIndex) -> PHEXBKM override; //Bookmark by index (in inner list).
		[[nodiscard]] auto GetCount() -> ULONGLONG override;
		[[nodiscard]] auto GetCurrent()const -> ULONGLONG;
		[[nodiscard]] auto GetDlgItemHandle(EHexDlgItem eItem)const -> HWND;
		[[nodiscard]] auto GetHWND()const -> HWND;
		void GoBookmark(ULONGLONG ullIndex);
		void GoNext();
		void GoPrev();
		[[nodiscard]] bool HasBookmark(ULONGLONG ullOffset)const;
		[[nodiscard]] bool HasBookmarks()const;
		[[nodiscard]] auto HitTest(ULONGLONG ullOffset) -> PHEXBKM override;
		[[nodiscard]] auto HitTest(ULONGLONG ullOffset)const -> PHEXBKM;
		void Initialize(IHexCtrl* pHexCtrl, HINSTANCE hInstRes);
		[[nodiscard]] bool IsVirtual()const;
		[[nodiscard]] bool PreTranslateMsg(MSG* pMsg);
		[[nodiscard]] auto ProcessMsg(const MSG& msg) -> INT_PTR;
		void RemoveAll()override;
		void RemoveByOffset(ULONGLONG ullOffset);
		void RemoveByID(ULONGLONG ullID)override;
		void SetDlgProperties(std::uint64_t u64Flags);
		void SetVirtual(IHexBookmarks* pVirtBkm);
		void ShowWindow(int iCmdShow);
		void Update(ULONGLONG ullID, const HEXBKM& bkm);
	private:
		enum class EMenuID : std::uint16_t;
		[[nodiscard]] auto GetHexCtrl()const -> IHexCtrl*;
		[[nodiscard]] bool IsNoEsc()const;
		[[nodiscard]] bool IsShowAsHex()const;
		void OnCancel();
		auto OnClose() -> INT_PTR;
		auto OnCommand(const MSG& msg) -> INT_PTR;
		void OnCheckHex();
		auto OnDestroy() -> INT_PTR;
		auto OnDrawItem(const MSG& msg) -> INT_PTR;
		auto OnInitDialog(const MSG& msg) -> INT_PTR;
		auto OnMeasureItem(const MSG& msg) -> INT_PTR;
		auto OnNotify(const MSG& msg) -> INT_PTR;
		void OnNotifyListColumnClick();
		void OnNotifyListDblClick(NMHDR* pNMHDR);
		void OnNotifyListGetColor(NMHDR* pNMHDR);
		void OnNotifyListGetDispInfo(NMHDR* pNMHDR);
		void OnNotifyListItemChanged(NMHDR* pNMHDR);
		void OnNotifyListRClick(NMHDR* pNMHDR);
		void OnNotifyListSetData(NMHDR* pNMHDR);
		auto OnSize(const MSG& msg) -> INT_PTR;
		void RemoveBookmark(std::uint64_t ullID);
		void UpdateListCount(bool fPreserveSelected = false);
	private:
		HINSTANCE m_hInstRes { };
		GDIUT::CWnd m_Wnd;
		GDIUT::CWndBtn m_WndBtnHex; //Check-box "Hex numbers".
		GDIUT::CMenu m_menuList;
		GDIUT::CDynLayout m_DynLayout;
		std::vector<HEXBKM> m_vecBookmarks; //Bookmarks data.
		LISTEX::CListEx m_ListEx;
		IHexCtrl* m_pHexCtrl { };
		IHexBookmarks* m_pVirtual { };
		std::uint64_t m_u64IndexCurr { }; //Current bookmark's position index, to move next/prev.
		std::uint64_t m_u64Flags { }; //Data from SetDlgProperties.
	};
}

using namespace HEXCTRL::INTERNAL;

enum class CHexDlgBkmMgr::EMenuID : std::uint16_t {
	IDM_BKMMGR_REMOVE = 0x8001, IDM_BKMMGR_REMOVEALL
};

auto CHexDlgBkmMgr::AddBkm(const HEXBKM& hbs, bool fRedraw)->ULONGLONG
{
	ULONGLONG ullID;
	if (m_pVirtual) {
		ullID = m_pVirtual->AddBkm(hbs, fRedraw);
	}
	else {
		ullID = 1; //Bookmarks' ID starts from 1.
		if (const auto it = std::max_element(m_vecBookmarks.begin(), m_vecBookmarks.end(),
			[](const HEXBKM& lhs, const HEXBKM& rhs) {
				return lhs.ullID < rhs.ullID; }); it != m_vecBookmarks.end()) {
			ullID = it->ullID + 1; //Increasing next bookmark's ID by 1.
		}
		m_vecBookmarks.emplace_back(hbs.vecSpan, hbs.wstrDesc, ullID, hbs.ullData, hbs.stClr);
	}

	UpdateListCount(true);

	if (fRedraw && m_pHexCtrl != nullptr && m_pHexCtrl->IsCreated()) {
		m_pHexCtrl->Redraw();
	}

	return ullID;
}

void CHexDlgBkmMgr::CreateDlg()const
{
	//m_Wnd is set in the OnInitDialog().
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

auto CHexDlgBkmMgr::GetByID(ULONGLONG ullID)->PHEXBKM
{
	PHEXBKM pBkm { };
	if (m_pVirtual) {
		pBkm = m_pVirtual->GetByID(ullID);
	}
	else if (const auto it = std::find_if(m_vecBookmarks.begin(), m_vecBookmarks.end(),
		[ullID](const HEXBKM& ref) { return ullID == ref.ullID; }); it != m_vecBookmarks.end()) {
		pBkm = &*it;
	}

	return pBkm;
}

auto CHexDlgBkmMgr::GetByIndex(ULONGLONG ullIndex)->PHEXBKM
{
	PHEXBKM pBkm { };
	if (m_pVirtual) {
		pBkm = m_pVirtual->GetByIndex(ullIndex);
	}
	else if (ullIndex < m_vecBookmarks.size()) {
		pBkm = &m_vecBookmarks[static_cast<std::size_t>(ullIndex)];
	}

	return pBkm;
}

auto CHexDlgBkmMgr::GetCount()->ULONGLONG
{
	return IsVirtual() ? m_pVirtual->GetCount() : m_vecBookmarks.size();
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

bool CHexDlgBkmMgr::HasBookmark(ULONGLONG ullOffset)const
{
	return HitTest(ullOffset) != nullptr;
}

bool CHexDlgBkmMgr::HasBookmarks()const
{
	return IsVirtual() ? m_pVirtual->GetCount() > 0 : !m_vecBookmarks.empty();
}

auto CHexDlgBkmMgr::HitTest(ULONGLONG ullOffset)->PHEXBKM
{
	PHEXBKM pBkm { };
	if (m_pVirtual) {
		pBkm = m_pVirtual->HitTest(ullOffset);
	}
	else {
		if (const auto rit = std::find_if(m_vecBookmarks.rbegin(), m_vecBookmarks.rend(),
			[ullOffset](const HEXBKM& ref) { return std::any_of(ref.vecSpan.begin(), ref.vecSpan.end(),
				[ullOffset](const HEXSPAN& refV) {
					return ullOffset >= refV.ullOffset && ullOffset < (refV.ullOffset + refV.ullSize); }); });
					rit != m_vecBookmarks.rend()) {
			pBkm = &*rit;
		}
	}

	return pBkm;
}

auto CHexDlgBkmMgr::HitTest(ULONGLONG ullOffset)const->PHEXBKM
{
	return const_cast<CHexDlgBkmMgr*>(this)->HitTest(ullOffset);
}

void CHexDlgBkmMgr::Initialize(IHexCtrl* pHexCtrl, HINSTANCE hInstRes)
{
	if (pHexCtrl == nullptr || hInstRes == nullptr) {
		ut::DBG_REPORT(L"Initialize == nullptr");
		return;
	}

	m_pHexCtrl = pHexCtrl;
	m_hInstRes = hInstRes;
}

bool CHexDlgBkmMgr::IsVirtual()const
{
	return m_pVirtual != nullptr;
}

bool CHexDlgBkmMgr::PreTranslateMsg(MSG* pMsg)
{
	return m_Wnd.IsDlgMessage(pMsg);
}

auto CHexDlgBkmMgr::ProcessMsg(const MSG& msg)->INT_PTR
{
	switch (msg.message) {
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

void CHexDlgBkmMgr::RemoveAll()
{
	if (m_pVirtual) {
		m_pVirtual->RemoveAll();
	}
	else {
		m_vecBookmarks.clear();
	}

	UpdateListCount();

	if (m_pHexCtrl && m_pHexCtrl->IsCreated()) {
		m_pHexCtrl->Redraw();
	}
}

void CHexDlgBkmMgr::RemoveByOffset(ULONGLONG ullOffset)
{
	if (m_pVirtual) {
		if (const auto* const pBkm = m_pVirtual->HitTest(ullOffset); pBkm != nullptr) {
			m_pVirtual->RemoveByID(pBkm->ullID);
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
			RemoveBookmark(it->ullID);
		}
	}

	UpdateListCount();

	if (m_pHexCtrl && m_pHexCtrl->IsCreated()) {
		m_pHexCtrl->Redraw();
	}
}

void CHexDlgBkmMgr::RemoveByID(ULONGLONG ullID)
{
	if (m_pVirtual) {
		m_pVirtual->RemoveByID(ullID);
	}
	else {
		if (const auto pBkm = GetByID(ullID); pBkm != nullptr) {
			RemoveBookmark(pBkm->ullID);
		}
	}

	UpdateListCount();

	if (m_pHexCtrl && m_pHexCtrl->IsCreated()) {
		m_pHexCtrl->Redraw();
	}
}

void CHexDlgBkmMgr::SetDlgProperties(std::uint64_t u64Flags)
{
	m_u64Flags = u64Flags;
}

void CHexDlgBkmMgr::SetVirtual(IHexBookmarks* pVirtBkm)
{
	if (pVirtBkm == this) { //To avoid setting self as a Virtual Bookmarks handler.
		ut::DBG_REPORT(L"pVirtBkm == this");
		return;
	}

	m_pVirtual = pVirtBkm;
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

	if (::IsWindow(m_ListEx.GetHWND())) {
		m_ListEx.RedrawWindow();
	}

	if (m_pHexCtrl && m_pHexCtrl->IsCreated()) {
		m_pHexCtrl->Redraw();
	}
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

	OnClose();
}

auto CHexDlgBkmMgr::OnClose()->INT_PTR
{
	ShowWindow(SW_HIDE);
	return TRUE;
}

auto CHexDlgBkmMgr::OnCommand(const MSG& msg) -> INT_PTR
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
			int nItem { -1 };
			for (auto i = 0UL; i < uSelCount; ++i) {
				nItem = m_ListEx.GetNextItem(nItem, LVNI_SELECTED);
				vecIndex.insert(vecIndex.begin(), nItem); //Last indexes go first.
			}

			for (const auto iIndexBkm : vecIndex) {
				if (const auto pBkm = GetByIndex(iIndexBkm); pBkm != nullptr) {
					RemoveBookmark(pBkm->ullID);
				}
			}

			UpdateListCount();

			if (m_pHexCtrl && m_pHexCtrl->IsCreated()) {
				m_pHexCtrl->Redraw();
			}
		}
		break;
		case EMenuID::IDM_BKMMGR_REMOVEALL: RemoveAll(); break;
		default:
			return FALSE;
		}
	}

	return TRUE;
}

void CHexDlgBkmMgr::OnCheckHex()
{
	m_ListEx.RedrawWindow();
}

auto CHexDlgBkmMgr::OnDestroy()->INT_PTR
{
	RemoveAll();
	m_menuList.DestroyMenu();
	m_u64Flags = { };
	m_DynLayout.RemoveAll();

	return TRUE;
}

auto CHexDlgBkmMgr::OnDrawItem(const MSG& msg)->INT_PTR
{
	const auto pDIS = reinterpret_cast<LPDRAWITEMSTRUCT>(msg.lParam);
	if (pDIS->CtlID == static_cast<UINT>(IDC_HEXCTRL_BKMMGR_LIST)) {
		m_ListEx.DrawItem(pDIS);
	}

	return TRUE;
}

auto CHexDlgBkmMgr::OnInitDialog(const MSG& msg)->INT_PTR
{
	m_Wnd.Attach(msg.hwnd);
	m_WndBtnHex.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_BKMMGR_CHK_HEX));

	m_ListEx.Create({ .hWndParent { m_Wnd }, .uID { IDC_HEXCTRL_BKMMGR_LIST }, .dwSizeFontList { 10 },
		.dwSizeFontHdr { 10 }, .fDialogCtrl { true }, .fSortable { true } });
	m_ListEx.SetExtendedStyle(LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT);
	m_ListEx.InsertColumn(0, L"№", LVCFMT_LEFT, 40);
	m_ListEx.InsertColumn(1, L"Offset", LVCFMT_LEFT, 80, -1, 0, true);
	m_ListEx.InsertColumn(2, L"Size", LVCFMT_LEFT, 50, -1, 0, true);
	m_ListEx.InsertColumn(3, L"Description", LVCFMT_LEFT, 250, -1, 0, true);
	m_ListEx.InsertColumn(4, L"Bk", LVCFMT_LEFT, 40);
	m_ListEx.InsertColumn(5, L"Text", LVCFMT_LEFT, 40);

	m_menuList.CreatePopupMenu();
	m_menuList.AppendString(static_cast<UINT_PTR>(EMenuID::IDM_BKMMGR_REMOVE), L"Remove");
	m_menuList.AppendSepar();
	m_menuList.AppendString(static_cast<UINT_PTR>(EMenuID::IDM_BKMMGR_REMOVEALL), L"Remove All");

	m_WndBtnHex.SetCheck(BST_CHECKED);

	UpdateListCount();

	m_DynLayout.SetHost(m_Wnd);
	m_DynLayout.AddItem(IDC_HEXCTRL_BKMMGR_LIST, GDIUT::CDynLayout::MoveNone(), GDIUT::CDynLayout::SizeHorzAndVert(100, 100));
	m_DynLayout.AddItem(IDC_HEXCTRL_BKMMGR_CHK_HEX, GDIUT::CDynLayout::MoveVert(100), GDIUT::CDynLayout::SizeNone());
	m_DynLayout.Enable(true);

	return TRUE;
}

auto CHexDlgBkmMgr::OnMeasureItem(const MSG& msg)->INT_PTR
{
	const auto pMIS = reinterpret_cast<LPMEASUREITEMSTRUCT>(msg.lParam);
	if (pMIS->CtlID == static_cast<UINT>(IDC_HEXCTRL_BKMMGR_LIST)) {
		m_ListEx.MeasureItem(pMIS);
	}

	return TRUE;
}

auto CHexDlgBkmMgr::OnNotify(const MSG& msg)->INT_PTR
{
	const auto pNMHDR = reinterpret_cast<NMHDR*>(msg.lParam);
	switch (pNMHDR->idFrom) {
	case IDC_HEXCTRL_BKMMGR_LIST:
		switch (pNMHDR->code) {
		case LVN_COLUMNCLICK: OnNotifyListColumnClick(); break;
		case LVN_GETDISPINFOW: OnNotifyListGetDispInfo(pNMHDR); break;
		case LVN_ITEMCHANGED: OnNotifyListItemChanged(pNMHDR); break;
		case NM_DBLCLK: OnNotifyListDblClick(pNMHDR); break;
		case NM_RCLICK: OnNotifyListRClick(pNMHDR); break;
		case LISTEX::LISTEX_MSG_GETCOLOR: OnNotifyListGetColor(pNMHDR); break;
		case LISTEX::LISTEX_MSG_SETDATA: OnNotifyListSetData(pNMHDR); break;
		default: break;
		}
	default: break;
	}

	return TRUE;
}

void CHexDlgBkmMgr::OnNotifyListColumnClick()
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

void CHexDlgBkmMgr::OnNotifyListDblClick(NMHDR* pNMHDR)
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

void CHexDlgBkmMgr::OnNotifyListGetColor(NMHDR* pNMHDR)
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

void CHexDlgBkmMgr::OnNotifyListGetDispInfo(NMHDR* pNMHDR)
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

void CHexDlgBkmMgr::OnNotifyListItemChanged(NMHDR* pNMHDR)
{
	//Go selected bookmark only with keyboard arrows and LMouse clicks.
	//Does not trigger (LVN_ITEMCHANGED event) when updating bookmark: !(pLCI->uNewState & LVIS_SELECTED)
	if (const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
		pNMI->iItem >= 0 && pNMI->iSubItem >= 0 && (pNMI->uNewState & LVIS_SELECTED)) {
		GoBookmark(static_cast<ULONGLONG>(pNMI->iItem));
	}
}

void CHexDlgBkmMgr::OnNotifyListRClick(NMHDR* pNMHDR)
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

void CHexDlgBkmMgr::OnNotifyListSetData(NMHDR* pNMHDR)
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

auto CHexDlgBkmMgr::OnSize(const MSG& msg)->INT_PTR
{
	const auto wWidth = LOWORD(msg.lParam);
	const auto wHeight = HIWORD(msg.lParam);
	m_DynLayout.OnSize(wWidth, wHeight);
	return TRUE;
}

void CHexDlgBkmMgr::RemoveBookmark(std::uint64_t ullID)
{
	if (const auto it = std::find_if(m_vecBookmarks.begin(), m_vecBookmarks.end(),
		[ullID](const HEXBKM& ref) { return ullID == ref.ullID; }); it != m_vecBookmarks.end()) {
		m_vecBookmarks.erase(it);
	}
}

void CHexDlgBkmMgr::UpdateListCount(bool fPreserveSelected)
{
	if (!::IsWindow(m_ListEx.GetHWND())) {
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