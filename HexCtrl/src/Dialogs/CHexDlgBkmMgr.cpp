/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "../../res/HexCtrlRes.h"
#include "../HexUtility.h"
#include "CHexDlgBkmMgr.h"
#include <algorithm>
#include <cassert>
#include <format>
#include <numeric>

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

namespace HEXCTRL::INTERNAL
{
	enum class CHexDlgBkmMgr::EMenuID : std::uint16_t {
		IDM_BKMMGR_REMOVE = 0x8001, IDM_BKMMGR_REMOVEALL = 0x8002
	};
};

BEGIN_MESSAGE_MAP(CHexDlgBkmMgr, CDialogEx)
	ON_WM_ACTIVATE()
	ON_BN_CLICKED(IDC_HEXCTRL_BKMMGR_CHK_HEX, &CHexDlgBkmMgr::OnCheckHex)
	ON_BN_CLICKED(IDC_HEXCTRL_BKMMGR_CHK_MINMAX, &CHexDlgBkmMgr::OnCheckMinMax)
	ON_BN_CLICKED(IDC_HEXCTRL_BKMMGR_BTN_SAVE, &CHexDlgBkmMgr::OnBtnSave)
	ON_NOTIFY(LVN_GETDISPINFOW, IDC_HEXCTRL_BKMMGR_LIST, &CHexDlgBkmMgr::OnListGetDispInfo)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_HEXCTRL_BKMMGR_LIST, &CHexDlgBkmMgr::OnListItemChanged)
	ON_NOTIFY(NM_CLICK, IDC_HEXCTRL_BKMMGR_LIST, &CHexDlgBkmMgr::OnListLClick)
	ON_NOTIFY(NM_DBLCLK, IDC_HEXCTRL_BKMMGR_LIST, &CHexDlgBkmMgr::OnListDblClick)
	ON_NOTIFY(NM_RCLICK, IDC_HEXCTRL_BKMMGR_LIST, &CHexDlgBkmMgr::OnListRClick)
	ON_NOTIFY(LISTEX::LISTEX_MSG_GETCOLOR, IDC_HEXCTRL_BKMMGR_LIST, &CHexDlgBkmMgr::OnListGetColor)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CHexDlgBkmMgr::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HEXCTRL_BKMMGR_CHK_MINMAX, m_btnMinMax);
	DDX_Control(pDX, IDC_HEXCTRL_BKMMGR_EDIT_OFFSET, m_editOffset);
	DDX_Control(pDX, IDC_HEXCTRL_BKMMGR_EDIT_SIZE, m_editSize);
	DDX_Control(pDX, IDC_HEXCTRL_BKMMGR_EDIT_DESCR, m_editDescr);
	DDX_Control(pDX, IDC_HEXCTRL_BKMMGR_EDIT_DESCR, m_editDescr);
	DDX_Control(pDX, IDC_HEXCTRL_BKMMGR_CLR_BK, m_clrBk);
	DDX_Control(pDX, IDC_HEXCTRL_BKMMGR_CLR_TXT, m_clrTxt);
}

BOOL CHexDlgBkmMgr::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_pListMain->CreateDialogCtrl(IDC_HEXCTRL_BKMMGR_LIST, this);
	m_pListMain->SetSortable(true);
	m_pListMain->InsertColumn(0, L"\u2116", LVCFMT_LEFT, 40);
	m_pListMain->InsertColumn(1, L"Offset", LVCFMT_LEFT, 80);
	m_pListMain->InsertColumn(2, L"Size", LVCFMT_LEFT, 80);
	m_pListMain->InsertColumn(3, L"Description", LVCFMT_LEFT, 250);
	m_pListMain->InsertColumn(4, L"Colors", LVCFMT_LEFT, 65);
	m_pListMain->SetExtendedStyle(LVS_EX_HEADERDRAGDROP);

	m_stMenuList.CreatePopupMenu();
	m_stMenuList.AppendMenuW(MF_BYPOSITION, static_cast<UINT_PTR>(EMenuID::IDM_BKMMGR_REMOVE), L"Remove");
	m_stMenuList.AppendMenuW(MF_SEPARATOR);
	m_stMenuList.AppendMenuW(MF_BYPOSITION, static_cast<UINT_PTR>(EMenuID::IDM_BKMMGR_REMOVEALL), L"Remove All");

	if (const auto pChkHex = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_BKMMGR_CHK_HEX)); pChkHex) {
		pChkHex->SetCheck(BST_CHECKED);
	}

	CRect rcWnd;
	m_btnMinMax.GetWindowRect(rcWnd);
	m_hBITMAPMinMax = static_cast<HBITMAP>(LoadImageW(AfxGetInstanceHandle(),
		MAKEINTRESOURCEW(IDB_HEXCTRL_SCROLL_ARROW), IMAGE_BITMAP, rcWnd.Width(), rcWnd.Height(), 0));
	m_btnMinMax.SetBitmap(m_hBITMAPMinMax); //Set arrow bitmap to the min-max checkbox.
	m_btnMinMax.SetCheck(BST_CHECKED);

	OnCheckMinMax(); //Dialog starts in "closed" mode.

	return TRUE;
}

void CHexDlgBkmMgr::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if (pWndOther != nullptr && pWndOther->GetParent() == this) {
		return; //To avoid WM_ACTIVATE from child controls (e.g. color buttons).
	}

	if (nState == WA_INACTIVE) {
		SetLayeredWindowAttributes(0, 150, LWA_ALPHA);
	}
	else {
		SetLayeredWindowAttributes(0, 255, LWA_ALPHA);
		UpdateList();
		m_pListMain->SetItemState(-1, 0, LVIS_SELECTED);
		if (GetCount() > 0) {
			const auto iCurrent = static_cast<int>(GetCurrent());
			m_pListMain->SetItemState(iCurrent, LVIS_SELECTED, LVIS_SELECTED);
			m_pListMain->EnsureVisible(iCurrent, FALSE);
		}
		else {
			EnableBottomArea(false);
		}
	}

	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
}

BOOL CHexDlgBkmMgr::OnCommand(WPARAM wParam, LPARAM lParam)
{
	bool fMsgHere { true }; //Process message here, and not pass further, to parent.
	switch (static_cast<EMenuID>(LOWORD(wParam))) {
	case EMenuID::IDM_BKMMGR_REMOVE:
	{
		std::vector<PHEXBKM> vecBkm;
		int nItem { -1 };
		for (auto i = 0UL; i < m_pListMain->GetSelectedCount(); ++i) {
			nItem = m_pListMain->GetNextItem(nItem, LVNI_SELECTED);
			if (const auto pBkm = GetByIndex(nItem); pBkm != nullptr) {
				vecBkm.emplace_back(pBkm);
			}
		}

		for (const auto pBkm : vecBkm) {
			RemoveByID(pBkm->ullID);
		}

		EnableBottomArea(false);
		UpdateList();
	}
	break;
	case EMenuID::IDM_BKMMGR_REMOVEALL:
		RemoveAll();
		EnableBottomArea(false);
		UpdateList();
		break;
	default:
		fMsgHere = false;
	}

	return fMsgHere ? TRUE : CDialogEx::OnCommand(wParam, lParam);
}

void CHexDlgBkmMgr::OnCheckHex()
{
	m_fShowAsHex = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_BKMMGR_CHK_HEX))->GetCheck() == BST_CHECKED;
	m_pListMain->RedrawWindow();
}

void CHexDlgBkmMgr::OnCheckMinMax()
{
	const auto fMinimize = m_btnMinMax.GetCheck() == BST_CHECKED;
	constexpr int iIDsToHide[] { IDC_HEXCTRL_BKMMGR_GRB, IDC_HEXCTRL_BKMMGR_STATIC_OFFSET, IDC_HEXCTRL_BKMMGR_STATIC_SIZE,
		IDC_HEXCTRL_BKMMGR_STATIC_CLRBK, IDC_HEXCTRL_BKMMGR_STATIC_CLRTXT, IDC_HEXCTRL_BKMMGR_EDIT_OFFSET,
		IDC_HEXCTRL_BKMMGR_EDIT_SIZE, IDC_HEXCTRL_BKMMGR_CLR_BK, IDC_HEXCTRL_BKMMGR_CLR_TXT, IDC_HEXCTRL_BKMMGR_STATIC_DESCR,
		IDC_HEXCTRL_BKMMGR_EDIT_DESCR, IDC_HEXCTRL_BKMMGR_BTN_SAVE };

	for (const auto id : iIDsToHide) {
		GetDlgItem(id)->ShowWindow(fMinimize ? SW_HIDE : SW_SHOW);
	}

	CRect rcGRB;
	GetDlgItem(IDC_HEXCTRL_BKMMGR_GRB)->GetClientRect(rcGRB);
	const auto iHeightGRB = rcGRB.Height();

	CRect rcWnd;
	GetWindowRect(rcWnd);
	const auto iHightNew = fMinimize ? rcWnd.Height() - iHeightGRB : rcWnd.Height() + iHeightGRB;
	EnableDynamicLayoutHelper(false);
	SetWindowPos(nullptr, 0, 0, rcWnd.Width(), iHightNew, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_FRAMECHANGED);
	EnableDynamicLayoutHelper(true);
}

void CHexDlgBkmMgr::OnBtnSave()
{
	if (m_iBkmCurr < 0)
		return;

	CStringW cstr;
	m_editOffset.GetWindowTextW(cstr);
	const auto optOffset = stn::StrToULL(cstr.GetString());
	if (!optOffset) {
		MessageBoxW(L"Invalid offset format.", L"Format error", MB_ICONERROR);
		return;
	}

	m_editSize.GetWindowTextW(cstr);
	const auto optSize = stn::StrToULL(cstr.GetString());
	if (!optSize) {
		MessageBoxW(L"Invalid size format.", L"Format error", MB_ICONERROR);
		return;
	}
	if (*optSize == 0) {
		MessageBoxW(L"Size can't be zero!", L"Format error", MB_ICONERROR);
		return;
	}

	const auto pBkm = GetByIndex(m_iBkmCurr);
	if (pBkm == nullptr)
		return;

	pBkm->clrBk = m_clrBk.GetColor();
	pBkm->clrText = m_clrTxt.GetColor();

	const auto ullOffsetCurr = pBkm->vecSpan.front().ullOffset;
	const auto ullSizeCurr = std::accumulate(pBkm->vecSpan.begin(), pBkm->vecSpan.end(), 0ULL,
		[](auto ullTotal, const HEXSPAN& ref) { return ullTotal + ref.ullSize; });
	if (ullOffsetCurr != *optOffset || ullSizeCurr != *optSize) {
		pBkm->vecSpan.clear();
		pBkm->vecSpan.emplace_back(*optOffset, *optSize);
	}

	m_editDescr.GetWindowTextW(cstr);
	pBkm->wstrDesc = cstr;

	m_pHexCtrl->Redraw();
}

void CHexDlgBkmMgr::OnDestroy()
{
	CDialogEx::OnDestroy();

	RemoveAll();
	m_stMenuList.DestroyMenu();
	DeleteObject(m_hBITMAPMinMax);
}

void CHexDlgBkmMgr::EnableBottomArea(bool fEnable)
{
	constexpr int iIDsToDisable[] { IDC_HEXCTRL_BKMMGR_EDIT_OFFSET, IDC_HEXCTRL_BKMMGR_EDIT_SIZE,
		IDC_HEXCTRL_BKMMGR_CLR_BK, IDC_HEXCTRL_BKMMGR_CLR_TXT, IDC_HEXCTRL_BKMMGR_EDIT_DESCR, IDC_HEXCTRL_BKMMGR_BTN_SAVE };

	for (const auto id : iIDsToDisable) {
		GetDlgItem(id)->EnableWindow(fEnable);
	}

	if (!fEnable) {
		m_iBkmCurr = -1;
	}
}

void CHexDlgBkmMgr::EnableDynamicLayoutHelper(bool fEnable)
{
	if (fEnable && IsDynamicLayoutEnabled())
		return;

	EnableDynamicLayout(fEnable ? TRUE : FALSE);

	if (fEnable) {
		const auto pLayout = GetDynamicLayout();
		pLayout->Create(this);

		pLayout->AddItem(IDC_HEXCTRL_BKMMGR_LIST, CMFCDynamicLayout::MoveNone(),
				CMFCDynamicLayout::SizeHorizontalAndVertical(100, 100));
		pLayout->AddItem(IDC_HEXCTRL_BKMMGR_CHK_HEX, CMFCDynamicLayout::MoveVertical(100), CMFCDynamicLayout::SizeNone());
		pLayout->AddItem(IDC_HEXCTRL_BKMMGR_CHK_MINMAX, CMFCDynamicLayout::MoveHorizontalAndVertical(50, 100),
			CMFCDynamicLayout::SizeNone());
		pLayout->AddItem(IDOK, CMFCDynamicLayout::MoveHorizontalAndVertical(100, 100), CMFCDynamicLayout::SizeNone());
		pLayout->AddItem(IDC_HEXCTRL_BKMMGR_GRB, CMFCDynamicLayout::MoveVertical(100), CMFCDynamicLayout::SizeHorizontal(100));
		pLayout->AddItem(IDC_HEXCTRL_BKMMGR_STATIC_OFFSET, CMFCDynamicLayout::MoveVertical(100), CMFCDynamicLayout::SizeNone());
		pLayout->AddItem(IDC_HEXCTRL_BKMMGR_STATIC_SIZE, CMFCDynamicLayout::MoveVertical(100), CMFCDynamicLayout::SizeNone());
		pLayout->AddItem(IDC_HEXCTRL_BKMMGR_STATIC_CLRBK, CMFCDynamicLayout::MoveVertical(100), CMFCDynamicLayout::SizeNone());
		pLayout->AddItem(IDC_HEXCTRL_BKMMGR_STATIC_CLRTXT, CMFCDynamicLayout::MoveVertical(100), CMFCDynamicLayout::SizeNone());
		pLayout->AddItem(IDC_HEXCTRL_BKMMGR_EDIT_OFFSET, CMFCDynamicLayout::MoveVertical(100), CMFCDynamicLayout::SizeNone());
		pLayout->AddItem(IDC_HEXCTRL_BKMMGR_EDIT_SIZE, CMFCDynamicLayout::MoveVertical(100), CMFCDynamicLayout::SizeNone());
		pLayout->AddItem(IDC_HEXCTRL_BKMMGR_CLR_BK, CMFCDynamicLayout::MoveVertical(100), CMFCDynamicLayout::SizeNone());
		pLayout->AddItem(IDC_HEXCTRL_BKMMGR_CLR_TXT, CMFCDynamicLayout::MoveVertical(100), CMFCDynamicLayout::SizeNone());
		pLayout->AddItem(IDC_HEXCTRL_BKMMGR_STATIC_DESCR, CMFCDynamicLayout::MoveVertical(100), CMFCDynamicLayout::SizeNone());
		pLayout->AddItem(IDC_HEXCTRL_BKMMGR_EDIT_DESCR, CMFCDynamicLayout::MoveVertical(100), CMFCDynamicLayout::SizeNone());
		pLayout->AddItem(IDC_HEXCTRL_BKMMGR_BTN_SAVE, CMFCDynamicLayout::MoveHorizontalAndVertical(100, 100),
			CMFCDynamicLayout::SizeNone());
	}
}

void CHexDlgBkmMgr::FillBkmData(int iBkmIndex)
{
	m_iBkmCurr = iBkmIndex;
	const auto pBkm = GetByIndex(iBkmIndex);
	m_clrBk.SetColor(pBkm->clrBk);
	m_clrTxt.SetColor(pBkm->clrText);

	auto ullOffset { 0ULL };
	auto ullSize { 0ULL };
	if (!pBkm->vecSpan.empty()) {
		ullOffset = pBkm->vecSpan.front().ullOffset;
		ullSize = std::accumulate(pBkm->vecSpan.begin(), pBkm->vecSpan.end(), 0ULL,
			[](auto ullTotal, const HEXSPAN& ref) { return ullTotal + ref.ullSize; });
	}

	m_editOffset.SetWindowTextW(std::vformat(m_fShowAsHex ? L"0x{:X}" : L"{}", std::make_wformat_args(ullOffset)).data());
	m_editSize.SetWindowTextW(std::vformat(m_fShowAsHex ? L"0x{:X}" : L"{}", std::make_wformat_args(ullSize)).data());
	m_editDescr.SetWindowTextW(pBkm->wstrDesc.data());
}

BOOL CHexDlgBkmMgr::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	if (const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(lParam); pNMI->hdr.idFrom == IDC_HEXCTRL_BKMMGR_LIST) {
		switch (pNMI->hdr.code) {
		case LVN_COLUMNCLICK: //ON_NOTIFY(LVN_COLUMNCLICK...) macro doesn't seem to work, for no obvious reason.
			if (!IsVirtual()) {
				SortBookmarks();
			}
			break;
		default:
			break;
		}
	}

	return CDialogEx::OnNotify(wParam, lParam, pResult);
}

void CHexDlgBkmMgr::OnListGetDispInfo(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pDispInfo = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
	const auto pItem = &pDispInfo->item;

	if ((pItem->mask & LVIF_TEXT) == 0)
		return;

	const auto iItemID = pItem->iItem;
	const auto pBkm = GetByIndex(static_cast<ULONGLONG>(iItemID));
	if (pBkm == nullptr)
		return;

	ULONGLONG ullOffset { 0 };
	ULONGLONG ullSize { 0 };
	switch (pItem->iSubItem) {
	case 0: //Index number.
		*std::format_to(pItem->pszText, L"{}", iItemID + 1) = L'\0';
		break;
	case 1: //Offset.
		if (!pBkm->vecSpan.empty()) {
			ullOffset = pBkm->vecSpan.front().ullOffset;
		}
		*std::vformat_to(pItem->pszText, m_fShowAsHex ? L"0x{:X}" : L"{}", std::make_wformat_args(ullOffset)) = L'\0';
		break;
	case 2: //Size.
		if (!pBkm->vecSpan.empty()) {
			ullSize = std::accumulate(pBkm->vecSpan.begin(), pBkm->vecSpan.end(), 0ULL,
				[](auto ullTotal, const HEXSPAN& ref) { return ullTotal + ref.ullSize; });
		}
		*std::vformat_to(pItem->pszText, m_fShowAsHex ? L"0x{:X}" : L"{}", std::make_wformat_args(ullSize)) = L'\0';
		break;
	case 3: //Description.
		pItem->pszText = pBkm->wstrDesc.data();
		break;
	case 4: //Color.
		*std::format_to(pItem->pszText, L"#Text") = L'\0';
		break;
	default:
		break;
	}
}

void CHexDlgBkmMgr::OnListItemChanged(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	//Go selected bookmark only with keyboard arrows and lmouse clicks.
	//Does not trigger (LVN_ITEMCHANGED event) when updating bookmark: !(pNMI->uNewState & LVIS_SELECTED)
	if (const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
		pNMI->iItem >= 0 && pNMI->iSubItem >= 0 && (pNMI->uNewState & LVIS_SELECTED)) {
		GoBookmark(static_cast<ULONGLONG>(pNMI->iItem));
		FillBkmData(pNMI->iItem);
		EnableBottomArea(true);
	}
}

void CHexDlgBkmMgr::OnListLClick(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	if (const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
		pNMI->iItem < 0 || pNMI->iSubItem < 0) {
		EnableBottomArea(false);
	}
}

void CHexDlgBkmMgr::OnListDblClick(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	if (const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
		pNMI->iItem >= 0 && pNMI->iSubItem >= 0) {
		if (m_btnMinMax.GetCheck() == BST_CHECKED) { //If minimized.
			m_btnMinMax.SetCheck(BST_UNCHECKED);
			OnCheckMinMax();
		}
	}
}

void CHexDlgBkmMgr::OnListRClick(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	bool fEnabled { false };
	if (const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
		pNMI->iItem >= 0 && pNMI->iSubItem >= 0) {
		fEnabled = true;
	}

	//Edit menu enabled only when one item selected.
	m_stMenuList.EnableMenuItem(static_cast<UINT>(EMenuID::IDM_BKMMGR_REMOVE), (fEnabled ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
	m_stMenuList.EnableMenuItem(static_cast<UINT>(EMenuID::IDM_BKMMGR_REMOVEALL),
		(m_pListMain->GetItemCount() > 0 ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);

	POINT pt;
	GetCursorPos(&pt);
	m_stMenuList.TrackPopupMenuEx(TPM_LEFTALIGN, pt.x, pt.y, this, nullptr);
}

void CHexDlgBkmMgr::OnListGetColor(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	if (const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR); pNMI->iSubItem == 4) {
		if (const auto* const pBkm = GetByIndex(static_cast<size_t>(pNMI->iItem)); pBkm != nullptr) {
			m_stCellClr.clrBk = pBkm->clrBk;
			m_stCellClr.clrText = pBkm->clrText;
			pNMI->lParam = reinterpret_cast<LPARAM>(&m_stCellClr);
		}
	}
}

void CHexDlgBkmMgr::UpdateList()
{
	m_pListMain->SetItemCountEx(static_cast<int>(GetCount()), LVSICF_NOSCROLL);
}

void CHexDlgBkmMgr::SortBookmarks()
{
	//Sort bookmarks according to clicked column.
	SortData(m_pListMain->GetSortColumn(), m_pListMain->GetSortAscending());
	m_pListMain->RedrawWindow();
}

auto CHexDlgBkmMgr::AddBkm(const HEXBKM& hbs, bool fRedraw)->ULONGLONG
{
	if (m_pHexCtrl == nullptr || !m_pHexCtrl->IsDataSet())
		return 0;

	ULONGLONG ullID { static_cast<ULONGLONG>(-1) };
	if (m_pVirtual) {
		ullID = m_pVirtual->AddBkm(hbs, fRedraw);
	}
	else {
		ullID = 1; //Bookmarks' ID starts from 1.

		if (const auto iter = std::max_element(m_vecBookmarks.begin(), m_vecBookmarks.end(),
			[](const HEXBKM& ref1, const HEXBKM& ref2) {
				return ref1.ullID < ref2.ullID; }); iter != m_vecBookmarks.end()) {
			ullID = iter->ullID + 1; //Increasing next bookmark's ID by 1.
		}

		m_vecBookmarks.emplace_back(hbs.vecSpan, hbs.wstrDesc, ullID, hbs.ullData, hbs.clrBk, hbs.clrText);

		if (fRedraw) {
			m_pHexCtrl->Redraw();
		}
	}

	return ullID;
}

void CHexDlgBkmMgr::RemoveAll()
{
	if (m_pVirtual) {
		m_pVirtual->RemoveAll();
	}
	else {
		m_vecBookmarks.clear();
	}

	if (m_pHexCtrl && m_pHexCtrl->IsCreated()) {
		m_pHexCtrl->Redraw();
	}
}

auto CHexDlgBkmMgr::GetByID(ULONGLONG ullID)->PHEXBKM
{
	PHEXBKM pBkm { };
	if (m_pVirtual) {
		pBkm = m_pVirtual->GetByID(ullID);
	}
	else if (const auto iter = std::find_if(m_vecBookmarks.begin(), m_vecBookmarks.end(),
		[ullID](const HEXBKM& ref) { return ullID == ref.ullID; }); iter != m_vecBookmarks.end()) {
		pBkm = &*iter;
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
		pBkm = &m_vecBookmarks[static_cast<size_t>(ullIndex)];
	}

	return pBkm;
}

auto CHexDlgBkmMgr::GetCount()->ULONGLONG
{
	return IsVirtual() ? m_pVirtual->GetCount() : m_vecBookmarks.size();
}

auto CHexDlgBkmMgr::GetCurrent()const->ULONGLONG
{
	return static_cast<ULONGLONG>(m_llIndexCurr);
}

void CHexDlgBkmMgr::GoBookmark(ULONGLONG ullIndex)
{
	if (m_pHexCtrl == nullptr || !m_pHexCtrl->IsDataSet())
		return;

	if (const auto* const pBkm = GetByIndex(ullIndex); pBkm != nullptr) {
		m_llIndexCurr = static_cast<LONGLONG>(ullIndex);
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

	if (++m_llIndexCurr >= static_cast<LONGLONG>(GetCount())) {
		m_llIndexCurr = 0;
	}

	if (const auto* const pBkm = GetByIndex(m_llIndexCurr); pBkm != nullptr) {
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

	if (--m_llIndexCurr; m_llIndexCurr < 0 || m_llIndexCurr >= static_cast<LONGLONG>(GetCount())) {
		m_llIndexCurr = static_cast<LONGLONG>(GetCount()) - 1;
	}

	if (const auto* const pBkm = GetByIndex(m_llIndexCurr); pBkm != nullptr) {
		const auto ullOffset = pBkm->vecSpan.front().ullOffset;
		m_pHexCtrl->SetCaretPos(ullOffset);
		if (!m_pHexCtrl->IsOffsetVisible(ullOffset)) {
			m_pHexCtrl->GoToOffset(ullOffset);
		}
	}
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
		if (const auto rIter = std::find_if(m_vecBookmarks.rbegin(), m_vecBookmarks.rend(),
			[ullOffset](const HEXBKM& ref) { return std::any_of(ref.vecSpan.begin(), ref.vecSpan.end(),
				[ullOffset](const HEXSPAN& refV) {
					return ullOffset >= refV.ullOffset && ullOffset < (refV.ullOffset + refV.ullSize); }); });
					rIter != m_vecBookmarks.rend()) {
			pBkm = &*rIter;
		}
	}

	return pBkm;
}

void CHexDlgBkmMgr::Initialize(UINT nIDTemplate, IHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	assert(nIDTemplate > 0);
	if (pHexCtrl == nullptr)
		return;

	m_nIDTemplate = nIDTemplate;
	m_pHexCtrl = pHexCtrl;
}

bool CHexDlgBkmMgr::IsVirtual()const
{
	return m_pVirtual != nullptr;
}

void CHexDlgBkmMgr::RemoveByOffset(ULONGLONG ullOffset)
{
	if (m_pHexCtrl == nullptr || !m_pHexCtrl->IsDataSet())
		return;

	if (m_pVirtual) {
		if (const auto* const pBkm = m_pVirtual->HitTest(ullOffset); pBkm != nullptr) {
			m_pVirtual->RemoveByID(pBkm->ullID);
		}
	}
	else {
		if (m_vecBookmarks.empty())
			return;

		//Searching from the end, to remove last added bookmark if few at the given offset.
		if (const auto rIter = std::find_if(m_vecBookmarks.rbegin(), m_vecBookmarks.rend(),
			[ullOffset](const HEXBKM& ref) { return std::any_of(ref.vecSpan.begin(), ref.vecSpan.end(),
				[ullOffset](const HEXSPAN& refV) {
					return ullOffset >= refV.ullOffset && ullOffset < (refV.ullOffset + refV.ullSize); }); });
					rIter != m_vecBookmarks.rend()) {
			m_vecBookmarks.erase(std::next(rIter).base());
		}
	}

	m_pHexCtrl->Redraw();
}

void CHexDlgBkmMgr::RemoveByID(ULONGLONG ullID)
{
	if (m_pHexCtrl == nullptr || !m_pHexCtrl->IsDataSet())
		return;

	if (m_pVirtual) {
		m_pVirtual->RemoveByID(ullID);
	}
	else {
		if (m_vecBookmarks.empty())
			return;

		if (const auto iter = std::find_if(m_vecBookmarks.begin(), m_vecBookmarks.end(),
			[ullID](const HEXBKM& ref) { return ullID == ref.ullID; }); iter != m_vecBookmarks.end()) {
			m_vecBookmarks.erase(iter);
		}
	}

	m_pHexCtrl->Redraw();
}

void CHexDlgBkmMgr::SetVirtual(IHexBookmarks* pVirtBkm)
{
	assert(pVirtBkm != this);
	if (pVirtBkm == this) //To avoid setting self as a Virtual Bookmarks handler.
		return;

	m_pVirtual = pVirtBkm;
}

BOOL CHexDlgBkmMgr::ShowWindow(int nCmdShow)
{
	if (!IsWindow(m_hWnd)) {
		Create(m_nIDTemplate, CWnd::FromHandle(m_pHexCtrl->GetWindowHandle(EHexWnd::WND_MAIN)));
	}

	return CDialogEx::ShowWindow(nCmdShow);
}

void CHexDlgBkmMgr::SortData(int iColumn, bool fAscending)
{
	//iColumn is column number in CHexDlgBkmMgr::m_pListMain.
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
					auto ullSize1 = std::accumulate(st1.vecSpan.begin(), st1.vecSpan.end(), 0ULL,
						[](auto ullTotal, const HEXSPAN& ref) { return ullTotal + ref.ullSize; });
					auto ullSize2 = std::accumulate(st2.vecSpan.begin(), st2.vecSpan.end(), 0ULL,
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
}

void CHexDlgBkmMgr::Update(ULONGLONG ullID, const HEXBKM& bkm)
{
	if (m_pHexCtrl == nullptr || !m_pHexCtrl->IsDataSet() || IsVirtual() || m_vecBookmarks.empty())
		return;

	if (const auto iter = std::find_if(m_vecBookmarks.begin(), m_vecBookmarks.end(),
		[ullID](const HEXBKM& ref) { return ullID == ref.ullID; }); iter != m_vecBookmarks.end()) {
		*iter = bkm;
		m_pHexCtrl->Redraw();
	}
}