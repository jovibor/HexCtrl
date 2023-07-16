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
	ON_BN_CLICKED(IDC_HEXCTRL_BKMMGR_CHK_HEX, &CHexDlgBkmMgr::OnCheckHex)
	ON_NOTIFY(LVN_GETDISPINFOW, IDC_HEXCTRL_BKMMGR_LIST, &CHexDlgBkmMgr::OnListGetDispInfo)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_HEXCTRL_BKMMGR_LIST, &CHexDlgBkmMgr::OnListItemChanged)
	ON_NOTIFY(NM_DBLCLK, IDC_HEXCTRL_BKMMGR_LIST, &CHexDlgBkmMgr::OnListDblClick)
	ON_NOTIFY(NM_RCLICK, IDC_HEXCTRL_BKMMGR_LIST, &CHexDlgBkmMgr::OnListRClick)
	ON_NOTIFY(LISTEX::LISTEX_MSG_GETCOLOR, IDC_HEXCTRL_BKMMGR_LIST, &CHexDlgBkmMgr::OnListGetColor)
	ON_NOTIFY(LISTEX::LISTEX_MSG_SETDATA, IDC_HEXCTRL_BKMMGR_LIST, &CHexDlgBkmMgr::OnListSetData)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

auto CHexDlgBkmMgr::AddBkm(const HEXBKM& hbs, bool fRedraw)->ULONGLONG
{
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
	}

	UpdateListCount(true);

	if (fRedraw && m_pHexCtrl && m_pHexCtrl->IsCreated()) {
		m_pHexCtrl->Redraw();
	}

	return ullID;
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
	return static_cast<ULONGLONG>(m_llIndexCurr);
}

auto CHexDlgBkmMgr::GetDlgData()const->std::uint64_t
{
	if (!IsWindow(m_hWnd)) {
		return { };
	}

	std::uint64_t ullData { };

	if (IsShowAsHex()) {
		ullData |= HEXCTRL_FLAG_BKMMGR_HEXNUM;
	}

	return ullData;
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

void CHexDlgBkmMgr::Initialize(IHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	if (pHexCtrl == nullptr)
		return;

	m_pHexCtrl = pHexCtrl;
}

bool CHexDlgBkmMgr::IsVirtual()const
{
	return m_pVirtual != nullptr;
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
		if (const auto iter = std::find_if(m_vecBookmarks.rbegin(), m_vecBookmarks.rend(),
			[ullOffset](const HEXBKM& ref) { return std::any_of(ref.vecSpan.begin(), ref.vecSpan.end(),
				[ullOffset](const HEXSPAN& refV) {
					return ullOffset >= refV.ullOffset && ullOffset < (refV.ullOffset + refV.ullSize); }); });
			iter != m_vecBookmarks.rend()) {
			RemoveBookmark(iter->ullID);
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

auto CHexDlgBkmMgr::SetDlgData(std::uint64_t ullData)->HWND
{
	if (!IsWindow(m_hWnd)) {
		Create(IDD_HEXCTRL_BKMMGR, CWnd::FromHandle(m_pHexCtrl->GetWindowHandle(EHexWnd::WND_MAIN)));
	}

	if ((ullData & HEXCTRL_FLAG_BKMMGR_HEXNUM) > 0 != IsShowAsHex()) {
		m_btnHex.SetCheck(m_btnHex.GetCheck() == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED);
		OnCheckHex();
	}

	return m_hWnd;
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
		Create(IDD_HEXCTRL_BKMMGR, CWnd::FromHandle(m_pHexCtrl->GetWindowHandle(EHexWnd::WND_MAIN)));
	}

	return CDialogEx::ShowWindow(nCmdShow);
}

void CHexDlgBkmMgr::SortData(int iColumn, bool fAscending)
{
	//iColumn is column number in CHexDlgBkmMgr::m_pList.
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
	if (IsVirtual())
		return;

	if (const auto iter = std::find_if(m_vecBookmarks.begin(), m_vecBookmarks.end(),
		[ullID](const HEXBKM& ref) { return ullID == ref.ullID; }); iter != m_vecBookmarks.end()) {
		*iter = bkm;
	}

	if (IsWindow(m_pList->m_hWnd)) {
		m_pList->RedrawWindow();
	}

	if (m_pHexCtrl && m_pHexCtrl->IsCreated()) {
		m_pHexCtrl->Redraw();
	}
}


//Private methods.

void CHexDlgBkmMgr::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HEXCTRL_BKMMGR_CHK_HEX, m_btnHex);
}

bool CHexDlgBkmMgr::IsShowAsHex()const
{
	return m_fShowAsHex;
}

BOOL CHexDlgBkmMgr::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (static_cast<EMenuID>(LOWORD(wParam))) {
	case EMenuID::IDM_BKMMGR_REMOVE:
	{
		const auto uSelCount = m_pList->GetSelectedCount();
		std::vector<int> vecIndex;
		vecIndex.reserve(uSelCount);
		int nItem { -1 };
		for (auto i = 0UL; i < uSelCount; ++i) {
			nItem = m_pList->GetNextItem(nItem, LVNI_SELECTED);
			vecIndex.insert(vecIndex.begin(), nItem); //Last indexes go first.
		}

		for (const auto i : vecIndex) {
			if (const auto pBkm = GetByIndex(i); pBkm != nullptr) {
				RemoveBookmark(pBkm->ullID);
			}
		}

		UpdateListCount();

		if (m_pHexCtrl && m_pHexCtrl->IsCreated()) {
			m_pHexCtrl->Redraw();
		}
	}
	return TRUE;
	case EMenuID::IDM_BKMMGR_REMOVEALL:
		RemoveAll();
		return TRUE;
	default:
		break;
	}

	return CDialogEx::OnCommand(wParam, lParam);
}

void CHexDlgBkmMgr::OnCheckHex()
{
	m_fShowAsHex = m_btnHex.GetCheck() == BST_CHECKED;
	m_pList->RedrawWindow();
}

void CHexDlgBkmMgr::OnDestroy()
{
	CDialogEx::OnDestroy();

	RemoveAll();
	m_stMenuList.DestroyMenu();
}

BOOL CHexDlgBkmMgr::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_pList->CreateDialogCtrl(IDC_HEXCTRL_BKMMGR_LIST, this);
	m_pList->SetExtendedStyle(LVS_EX_HEADERDRAGDROP);
	m_pList->SetSortable(true);
	m_pList->InsertColumn(0, L"\u2116", LVCFMT_LEFT, 40);
	m_pList->InsertColumn(1, L"Offset", LVCFMT_LEFT, 80, -1, 0, true);
	m_pList->InsertColumn(2, L"Size", LVCFMT_LEFT, 50, -1, 0, true);
	m_pList->InsertColumn(3, L"Description", LVCFMT_LEFT, 250, -1, 0, true);
	m_pList->InsertColumn(4, L"Bk", LVCFMT_LEFT, 30);
	m_pList->InsertColumn(5, L"Text", LVCFMT_LEFT, 30);

	m_stMenuList.CreatePopupMenu();
	m_stMenuList.AppendMenuW(MF_STRING, static_cast<UINT_PTR>(EMenuID::IDM_BKMMGR_REMOVE), L"Remove");
	m_stMenuList.AppendMenuW(MF_SEPARATOR);
	m_stMenuList.AppendMenuW(MF_STRING, static_cast<UINT_PTR>(EMenuID::IDM_BKMMGR_REMOVEALL), L"Remove All");

	m_btnHex.SetCheck(BST_CHECKED);

	UpdateListCount();

	if (const auto pDL = GetDynamicLayout(); pDL != nullptr) {
		pDL->SetMinSize({ 0, 0 });
	}

	return TRUE;
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
		*std::vformat_to(pItem->pszText, IsShowAsHex() ? L"0x{:X}" : L"{}", std::make_wformat_args(ullOffset)) = L'\0';
		break;
	case 2: //Size.
		if (!pBkm->vecSpan.empty()) {
			ullSize = std::accumulate(pBkm->vecSpan.begin(), pBkm->vecSpan.end(), 0ULL,
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

void CHexDlgBkmMgr::OnListItemChanged(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	//Go selected bookmark only with keyboard arrows and LMouse clicks.
	//Does not trigger (LVN_ITEMCHANGED event) when updating bookmark: !(pLCI->uNewState & LVIS_SELECTED)
	if (const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
		pNMI->iItem >= 0 && pNMI->iSubItem >= 0 && (pNMI->uNewState & LVIS_SELECTED)) {
		GoBookmark(static_cast<ULONGLONG>(pNMI->iItem));
	}
}

void CHexDlgBkmMgr::OnListDblClick(NMHDR* pNMHDR, LRESULT* /*pResult*/)
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
		pClr = &pBkm->clrBk;
		break;
	case 5: //Text color.
		pClr = &pBkm->clrText;
		break;
	default:
		return;
	}

	CMFCColorDialog dlg(*pClr);
	if (dlg.DoModal() != IDOK) {
		return;
	}

	*pClr = dlg.GetColor();
	m_pHexCtrl->Redraw();
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
		(m_pList->GetItemCount() > 0 ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);

	POINT pt;
	GetCursorPos(&pt);
	m_stMenuList.TrackPopupMenuEx(TPM_LEFTALIGN, pt.x, pt.y, this, nullptr);
}

void CHexDlgBkmMgr::OnListGetColor(NMHDR* pNMHDR, LRESULT* pResult)
{
	const auto pLCI = reinterpret_cast<LISTEX::PLISTEXCOLORINFO>(pNMHDR);

	switch (pLCI->iSubItem) {
	case 4: //Bk color.
		if (const auto* const pBkm = GetByIndex(static_cast<std::size_t>(pLCI->iItem)); pBkm != nullptr) {
			pLCI->stClr.clrBk = pBkm->clrBk;
			*pResult = TRUE;
			return;
		}
		break;
	case 5: //Text color.
		if (const auto* const pBkm = GetByIndex(static_cast<std::size_t>(pLCI->iItem)); pBkm != nullptr) {
			pLCI->stClr.clrBk = pBkm->clrText;
			*pResult = TRUE;
			return;
		}
		break;
	default:
		break;
	}
}

void CHexDlgBkmMgr::OnListSetData(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pLDI = reinterpret_cast<LISTEX::PLISTEXDATAINFO>(pNMHDR);
	const auto pBkm = GetByIndex(static_cast<std::size_t>(pLDI->iItem));
	if (pBkm == nullptr) {
		return;
	}

	switch (pLDI->iSubItem) {
	case 1: //Offset.
	{
		const auto optOffset = stn::StrToULL(pLDI->pwszData);
		if (!optOffset) {
			MessageBoxW(L"Invalid offset format.", L"Format error", MB_ICONERROR);
			return;
		}

		const auto ullOffsetCurr = pBkm->vecSpan.front().ullOffset;
		if (ullOffsetCurr != *optOffset) {
			const auto ullSizeCurr = std::accumulate(pBkm->vecSpan.begin(), pBkm->vecSpan.end(), 0ULL,
				[](auto ullTotal, const HEXSPAN& ref) { return ullTotal + ref.ullSize; });
			pBkm->vecSpan.clear();
			pBkm->vecSpan.emplace_back(*optOffset, ullSizeCurr);
		}
	}
	break;
	case 2: //Size.
	{
		const auto optSize = stn::StrToULL(pLDI->pwszData);
		if (!optSize) {
			MessageBoxW(L"Invalid size format.", L"Format error", MB_ICONERROR);
			return;
		}

		if (*optSize == 0) {
			MessageBoxW(L"Size can't be zero!", L"Size error", MB_ICONERROR);
			return;
		}

		const auto ullSizeCurr = std::accumulate(pBkm->vecSpan.begin(), pBkm->vecSpan.end(), 0ULL,
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

BOOL CHexDlgBkmMgr::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	if (const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(lParam); pNMI->hdr.idFrom == IDC_HEXCTRL_BKMMGR_LIST) {
		switch (pNMI->hdr.code) {
		//ON_NOTIFY(LVN_COLUMNCLICK, ...) macro doesn't work for CMFCListCtrl in neither virtual nor default mode.
		//But it works for vanilla CListCtrl. Obviously it's MFC quirks.
		case LVN_COLUMNCLICK:
			if (!IsVirtual()) {
				SortBookmarks();
			}
			return TRUE;
		default:
			break;
		}
	}

	return CDialogEx::OnNotify(wParam, lParam, pResult);
}

void CHexDlgBkmMgr::OnOK()
{
	//Empty to avoid dialog closing on Enter.
}

void CHexDlgBkmMgr::RemoveBookmark(std::uint64_t ullID)
{
	if (const auto iter = std::find_if(m_vecBookmarks.begin(), m_vecBookmarks.end(),
		[ullID](const HEXBKM& ref) { return ullID == ref.ullID; }); iter != m_vecBookmarks.end()) {
		m_vecBookmarks.erase(iter);
	}
}

void CHexDlgBkmMgr::SortBookmarks()
{
	//Sort bookmarks according to clicked column.
	SortData(m_pList->GetSortColumn(), m_pList->GetSortAscending());

	if (IsWindow(m_pList->m_hWnd)) {
		m_pList->RedrawWindow();
	}
}

void CHexDlgBkmMgr::UpdateListCount(bool fPreserveSelected)
{
	if (!IsWindow(m_pList->m_hWnd)) {
		return;
	}

	m_pList->SetItemState(-1, 0, LVIS_SELECTED);
	m_pList->SetItemCountEx(static_cast<int>(GetCount()), LVSICF_NOSCROLL);

	if (fPreserveSelected) {
		if (GetCount() > 0) {
			const auto iCurrent = static_cast<int>(GetCurrent());
			m_pList->SetItemState(iCurrent, LVIS_SELECTED, LVIS_SELECTED);
			m_pList->EnsureVisible(iCurrent, FALSE);
		}
	}
}