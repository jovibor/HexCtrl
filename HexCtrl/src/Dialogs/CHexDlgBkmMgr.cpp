/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "../../res/HexCtrlRes.h"
#include "CHexDlgBkmMgr.h"
#include "CHexDlgBkmProps.h"
#include <algorithm>
#include <cassert>
#include <format>
#include <numeric>

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

namespace HEXCTRL::INTERNAL
{
	enum class CHexDlgBkmMgr::EMenuID : std::uint16_t {
		IDM_BKMMGR_NEW = 0x8000, IDM_BKMMGR_EDIT = 0x8001,
		IDM_BKMMGR_REMOVE = 0x8002, IDM_BKMMGR_CLEARALL = 0x8003
	};
};

BEGIN_MESSAGE_MAP(CHexDlgBkmMgr, CDialogEx)
	ON_WM_ACTIVATE()
	ON_NOTIFY(LVN_GETDISPINFOW, IDC_HEXCTRL_BKMMGR_LIST, &CHexDlgBkmMgr::OnListGetDispInfo)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_HEXCTRL_BKMMGR_LIST, &CHexDlgBkmMgr::OnListItemChanged)
	ON_NOTIFY(NM_CLICK, IDC_HEXCTRL_BKMMGR_LIST, &CHexDlgBkmMgr::OnListLClick)
	ON_NOTIFY(NM_DBLCLK, IDC_HEXCTRL_BKMMGR_LIST, &CHexDlgBkmMgr::OnListDblClick)
	ON_NOTIFY(NM_RCLICK, IDC_HEXCTRL_BKMMGR_LIST, &CHexDlgBkmMgr::OnListRClick)
	ON_NOTIFY(LISTEX::LISTEX_MSG_GETCOLOR, IDC_HEXCTRL_BKMMGR_LIST, &CHexDlgBkmMgr::OnListGetColor)
	ON_BN_CLICKED(IDC_HEXCTRL_BKMMGR_CHK_HEX, &CHexDlgBkmMgr::OnClickCheckHex)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CHexDlgBkmMgr::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
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
	m_stMenuList.AppendMenuW(MF_BYPOSITION, static_cast<UINT_PTR>(EMenuID::IDM_BKMMGR_NEW), L"New");
	m_stMenuList.AppendMenuW(MF_BYPOSITION, static_cast<UINT_PTR>(EMenuID::IDM_BKMMGR_EDIT), L"Edit");
	m_stMenuList.AppendMenuW(MF_BYPOSITION, static_cast<UINT_PTR>(EMenuID::IDM_BKMMGR_REMOVE), L"Remove");
	m_stMenuList.AppendMenuW(MF_SEPARATOR);
	m_stMenuList.AppendMenuW(MF_BYPOSITION, static_cast<UINT_PTR>(EMenuID::IDM_BKMMGR_CLEARALL), L"Clear All");

	if (const auto pChk = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_BKMMGR_CHK_HEX)); pChk) {
		pChk->SetCheck(BST_CHECKED);
	}

	return TRUE;
}

void CHexDlgBkmMgr::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if (nState == WA_INACTIVE) {
		SetLayeredWindowAttributes(0, 150, LWA_ALPHA);
	}
	else {
		SetLayeredWindowAttributes(0, 255, LWA_ALPHA);
		UpdateList();
		m_pListMain->SetItemState(-1, 0, LVIS_SELECTED);
		m_pListMain->SetItemState(static_cast<int>(GetCurrent()), LVIS_SELECTED, LVIS_SELECTED);
		m_pListMain->EnsureVisible(static_cast<int>(GetCurrent()), FALSE);
	}

	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
}

BOOL CHexDlgBkmMgr::OnCommand(WPARAM wParam, LPARAM lParam)
{
	bool fMsgHere { true }; //Process message here, and not pass further, to parent.
	switch (static_cast<EMenuID>(LOWORD(wParam))) {
	case EMenuID::IDM_BKMMGR_NEW:
	{
		HEXBKM hbs;
		CHexDlgBkmProps dlgBkmEdit;
		if (dlgBkmEdit.DoModal(hbs, m_fShowAsHex) == IDOK) {
			AddBkm(hbs, true);
			UpdateList();
		}
	}
	break;
	case EMenuID::IDM_BKMMGR_EDIT:
	{
		if (m_pListMain->GetSelectedCount() > 1)
			break;

		const auto nItem = m_pListMain->GetNextItem(-1, LVNI_SELECTED);
		if (const auto* const pBkm = GetByIndex(nItem); pBkm != nullptr) {
			CHexDlgBkmProps dlgBkmEdit;
			auto stBkm = *pBkm; //Pass a copy to dlgBkmEdit to avoid changing the original, from list.
			if (dlgBkmEdit.DoModal(stBkm, m_fShowAsHex) == IDOK) {
				Update(pBkm->ullID, stBkm);
				UpdateList();
			}
		}
	}
	break;
	case EMenuID::IDM_BKMMGR_REMOVE:
	{
		std::vector<PHEXBKM> vecBkm { };
		int nItem { -1 };
		for (auto i = 0UL; i < m_pListMain->GetSelectedCount(); ++i) {
			nItem = m_pListMain->GetNextItem(nItem, LVNI_SELECTED);
			if (const auto pBkm = GetByIndex(nItem); pBkm != nullptr)
				vecBkm.emplace_back(pBkm);
		}
		for (const auto& iter : vecBkm) {
			RemoveByID(iter->ullID);
		}

		UpdateList();
	}
	break;
	case EMenuID::IDM_BKMMGR_CLEARALL:
		ClearAll();
		UpdateList();
		break;
	default:
		fMsgHere = false;
	}

	return fMsgHere ? TRUE : CDialogEx::OnCommand(wParam, lParam);
}

void CHexDlgBkmMgr::OnClickCheckHex()
{
	m_fShowAsHex = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_BKMMGR_CHK_HEX))->GetCheck() == BST_CHECKED;
	m_pListMain->RedrawWindow();
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
		pNMI->iItem != -1 && pNMI->iSubItem != -1 && (pNMI->uNewState & LVIS_SELECTED)) {
		GoBookmark(static_cast<ULONGLONG>(pNMI->iItem));
	}
}

void CHexDlgBkmMgr::OnListLClick(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	if (const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR); pNMI->iItem != -1 && pNMI->iSubItem != -1) {
		GoBookmark(static_cast<ULONGLONG>(pNMI->iItem));
	}
}

void CHexDlgBkmMgr::OnListDblClick(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	if (const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR); pNMI->iItem != -1 && pNMI->iSubItem != -1) {
		SendMessageW(WM_COMMAND, static_cast<WPARAM>(EMenuID::IDM_BKMMGR_EDIT));
	}
}

void CHexDlgBkmMgr::OnListRClick(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	bool fEnabled { false };
	if (const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR); pNMI->iItem != -1 && pNMI->iSubItem != -1) {
		fEnabled = true;
	}

	//Edit menu enabled only when one item selected.
	m_stMenuList.EnableMenuItem(static_cast<UINT>(EMenuID::IDM_BKMMGR_EDIT),
		(fEnabled && (m_pListMain->GetSelectedCount() == 1) ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
	m_stMenuList.EnableMenuItem(static_cast<UINT>(EMenuID::IDM_BKMMGR_REMOVE), (fEnabled ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
	m_stMenuList.EnableMenuItem(static_cast<UINT>(EMenuID::IDM_BKMMGR_CLEARALL),
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

void CHexDlgBkmMgr::OnDestroy()
{
	CDialogEx::OnDestroy();

	ClearAll();
	m_stMenuList.DestroyMenu();
}

ULONGLONG CHexDlgBkmMgr::AddBkm(const HEXBKM& hbs, bool fRedraw)
{
	if (m_pHexCtrl == nullptr || !m_pHexCtrl->IsDataSet())
		return 0;

	ULONGLONG ullID { static_cast<ULONGLONG>(-1) };
	if (m_pVirtual) {
		ullID = m_pVirtual->AddBkm(hbs, fRedraw);
	}
	else {
		ullID = 1; //Bookmarks' ID starts from 1.

		if (const auto iter = std::max_element(m_deqBookmarks.begin(), m_deqBookmarks.end(),
			[](const HEXBKM& ref1, const HEXBKM& ref2) {
				return ref1.ullID < ref2.ullID; }); iter != m_deqBookmarks.end()) {
			ullID = iter->ullID + 1; //Increasing next bookmark's ID by 1.
		}

		m_deqBookmarks.emplace_back(hbs.vecSpan, hbs.wstrDesc, ullID, hbs.ullData, hbs.clrBk, hbs.clrText);

		if (fRedraw) {
			m_pHexCtrl->Redraw();
		}
	}

	return ullID;
}

void CHexDlgBkmMgr::ClearAll()
{
	if (m_pVirtual) {
		m_pVirtual->ClearAll();
	}
	else {
		m_deqBookmarks.clear();
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
	else if (const auto iter = std::find_if(m_deqBookmarks.begin(), m_deqBookmarks.end(),
		[ullID](const HEXBKM& ref) { return ullID == ref.ullID; }); iter != m_deqBookmarks.end()) {
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
	else if (ullIndex < m_deqBookmarks.size()) {
		pBkm = &m_deqBookmarks[static_cast<size_t>(ullIndex)];
	}

	return pBkm;
}

ULONGLONG CHexDlgBkmMgr::GetCount()
{
	return IsVirtual() ? m_pVirtual->GetCount() : m_deqBookmarks.size();
}

ULONGLONG CHexDlgBkmMgr::GetCurrent()const
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
	return IsVirtual() ? m_pVirtual->GetCount() > 0 : !m_deqBookmarks.empty();
}

auto CHexDlgBkmMgr::HitTest(ULONGLONG ullOffset)->PHEXBKM
{
	PHEXBKM pBkm { };
	if (m_pVirtual) {
		pBkm = m_pVirtual->HitTest(ullOffset);
	}
	else {
		if (const auto rIter = std::find_if(m_deqBookmarks.rbegin(), m_deqBookmarks.rend(),
			[ullOffset](const HEXBKM& ref) { return std::any_of(ref.vecSpan.begin(), ref.vecSpan.end(),
				[ullOffset](const HEXSPAN& refV) {
					return ullOffset >= refV.ullOffset && ullOffset < (refV.ullOffset + refV.ullSize); }); });
					rIter != m_deqBookmarks.rend()) {
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
		if (m_deqBookmarks.empty())
			return;

		//Searching from the end, to remove last added bookmark if few at the given offset.
		if (const auto rIter = std::find_if(m_deqBookmarks.rbegin(), m_deqBookmarks.rend(),
			[ullOffset](const HEXBKM& ref) { return std::any_of(ref.vecSpan.begin(), ref.vecSpan.end(),
				[ullOffset](const HEXSPAN& refV) {
					return ullOffset >= refV.ullOffset && ullOffset < (refV.ullOffset + refV.ullSize); }); });
					rIter != m_deqBookmarks.rend()) {
			m_deqBookmarks.erase(std::next(rIter).base());
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
		if (m_deqBookmarks.empty())
			return;

		if (const auto iter = std::find_if(m_deqBookmarks.begin(), m_deqBookmarks.end(),
			[ullID](const HEXBKM& ref) { return ullID == ref.ullID; }); iter != m_deqBookmarks.end()) {
			m_deqBookmarks.erase(iter);
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
	std::sort(m_deqBookmarks.begin(), m_deqBookmarks.end(),
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

	return fAscending ? iCompare < 0 : iCompare > 0; });
}

void CHexDlgBkmMgr::Update(ULONGLONG ullID, const HEXBKM& stBookmark)
{
	if (m_pHexCtrl == nullptr || !m_pHexCtrl->IsDataSet() || IsVirtual() || m_deqBookmarks.empty())
		return;

	if (const auto iter = std::find_if(m_deqBookmarks.begin(), m_deqBookmarks.end(),
		[ullID](const HEXBKM& ref) { return ullID == ref.ullID; }); iter != m_deqBookmarks.end()) {
		*iter = stBookmark;
		m_pHexCtrl->Redraw();
	}
}