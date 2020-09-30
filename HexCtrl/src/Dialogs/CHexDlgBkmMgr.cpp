/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#include "stdafx.h"
#include "../../res/HexCtrlRes.h"
#include "CHexDlgBkmMgr.h"
#include <numeric>

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

BEGIN_MESSAGE_MAP(CHexDlgBkmMgr, CDialogEx)
	ON_WM_ACTIVATE()
	ON_NOTIFY(LVN_GETDISPINFOW, IDC_HEXCTRL_BKMMGR_LIST, &CHexDlgBkmMgr::OnListGetDispInfo)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_HEXCTRL_BKMMGR_LIST, &CHexDlgBkmMgr::OnListItemChanged)
	ON_NOTIFY(NM_CLICK, IDC_HEXCTRL_BKMMGR_LIST, &CHexDlgBkmMgr::OnListItemLClick)
	ON_NOTIFY(NM_DBLCLK, IDC_HEXCTRL_BKMMGR_LIST, &CHexDlgBkmMgr::OnListDblClick)
	ON_NOTIFY(NM_RCLICK, IDC_HEXCTRL_BKMMGR_LIST, &CHexDlgBkmMgr::OnListRClick)
	ON_NOTIFY(LISTEX_MSG_CELLCOLOR, IDC_HEXCTRL_BKMMGR_LIST, &CHexDlgBkmMgr::OnListCellColor)
	ON_COMMAND(IDC_HEXCTRL_BKMMGR_RADIO_DEC, &CHexDlgBkmMgr::OnClickRadioDec)
	ON_COMMAND(IDC_HEXCTRL_BKMMGR_RADIO_HEX, &CHexDlgBkmMgr::OnClickRadioHex)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL CHexDlgBkmMgr::Create(UINT nIDTemplate, CWnd* pParent, CHexBookmarks* pBookmarks)
{
	m_pBookmarks = pBookmarks;

	return CDialogEx::Create(nIDTemplate, pParent);
}

void CHexDlgBkmMgr::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BOOL CHexDlgBkmMgr::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	if (auto pRadio = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_BKMMGR_RADIO_DEC)); pRadio)
		pRadio->SetCheck(BST_CHECKED);

	m_pListMain->CreateDialogCtrl(IDC_HEXCTRL_BKMMGR_LIST, this);
	m_pListMain->SetSortable(true);
	m_pListMain->InsertColumn(0, L"\u2116", LVCFMT_LEFT, 40);
	m_pListMain->InsertColumn(1, L"Offset", LVCFMT_CENTER, 80);
	m_pListMain->InsertColumn(2, L"Size", LVCFMT_CENTER, 80);
	m_pListMain->InsertColumn(3, L"Description", LVCFMT_LEFT, 210);
	m_pListMain->InsertColumn(4, L"Bk color", LVCFMT_CENTER, 65);
	m_pListMain->SetExtendedStyle(LVS_EX_HEADERDRAGDROP);

	m_stMenuList.CreatePopupMenu();
	m_stMenuList.AppendMenuW(MF_BYPOSITION, static_cast<UINT_PTR>(EMenuID::IDM_BKMMGR_NEW), L"New");
	m_stMenuList.AppendMenuW(MF_BYPOSITION, static_cast<UINT_PTR>(EMenuID::IDM_BKMMGR_EDIT), L"Edit");
	m_stMenuList.AppendMenuW(MF_BYPOSITION, static_cast<UINT_PTR>(EMenuID::IDM_BKMMGR_REMOVE), L"Remove");
	m_stMenuList.AppendMenuW(MF_SEPARATOR);
	m_stMenuList.AppendMenuW(MF_BYPOSITION, static_cast<UINT_PTR>(EMenuID::IDM_BKMMGR_CLEARALL), L"Clear All");

	return TRUE;
}

void CHexDlgBkmMgr::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if (nState == WA_INACTIVE)
		SetLayeredWindowAttributes(0, 150, LWA_ALPHA);
	else
	{
		SetLayeredWindowAttributes(0, 255, LWA_ALPHA);
		if (!m_time || m_time != m_pBookmarks->GetTouchTime())
			UpdateList();

		m_pListMain->SetItemState(-1, 0, LVIS_SELECTED);
		m_pListMain->SetItemState(static_cast<int>(m_pBookmarks->GetCurrent()), LVIS_SELECTED, LVIS_SELECTED);
		m_pListMain->EnsureVisible(static_cast<int>(m_pBookmarks->GetCurrent()), FALSE);
	}

	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
}

BOOL CHexDlgBkmMgr::OnCommand(WPARAM wParam, LPARAM lParam)
{
	bool fHere { true }; //Process message here, and not pass further, to parent.
	switch (static_cast<EMenuID>(LOWORD(wParam)))
	{
	case EMenuID::IDM_BKMMGR_NEW:
	{
		HEXBKMSTRUCT hbs;
		CHexDlgBkmProps dlgBkmEdit;
		if (dlgBkmEdit.DoModal(hbs, m_fShowAsHex) == IDOK)
		{
			m_pBookmarks->Add(hbs);
			UpdateList();
		}
	}
	break;
	case EMenuID::IDM_BKMMGR_EDIT:
	{
		if (m_pListMain->GetSelectedCount() > 1)
			break;

		auto nItem = m_pListMain->GetNextItem(-1, LVNI_SELECTED);
		if (auto pBkm = m_pBookmarks->GetByIndex(nItem); pBkm != nullptr)
		{
			CHexDlgBkmProps dlgBkmEdit;
			auto stBkm = *pBkm; //Pass a copy to dlgBkmEdit to avoid changing the original, from list.
			if (dlgBkmEdit.DoModal(stBkm, m_fShowAsHex) == IDOK)
			{
				m_pBookmarks->Update(pBkm->ullID, stBkm);
				UpdateList();
			}
		}
	}
	break;
	case EMenuID::IDM_BKMMGR_REMOVE:
	{
		std::vector<HEXBKMSTRUCT*> vecBkm { };
		int nItem { -1 };
		for (auto i = 0UL; i < m_pListMain->GetSelectedCount(); ++i)
		{
			nItem = m_pListMain->GetNextItem(nItem, LVNI_SELECTED);
			if (auto pBkm = m_pBookmarks->GetByIndex(nItem); pBkm != nullptr)
				vecBkm.emplace_back(pBkm);
		}
		for (const auto& iter : vecBkm)
			m_pBookmarks->RemoveByID(iter->ullID);

		UpdateList();
	}
	break;
	case EMenuID::IDM_BKMMGR_CLEARALL:
		m_pBookmarks->ClearAll();
		UpdateList();
		break;
	default:
		fHere = false;
	}

	if (fHere)
		return TRUE;

	return CDialogEx::OnCommand(wParam, lParam);
}

BOOL CHexDlgBkmMgr::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	if (const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(lParam); pNMI->hdr.idFrom == IDC_HEXCTRL_BKMMGR_LIST)
	{
		switch (pNMI->hdr.code)
		{
		case LVN_COLUMNCLICK: //ON_NOTIFY(LVN_COLUMNCLICK...) macro doesn't seem to work, for no obvious reason.
			if (!m_pBookmarks->IsVirtual())
				SortBookmarks();
			break;
		}
	}

	return CDialogEx::OnNotify(wParam, lParam, pResult);
}

void CHexDlgBkmMgr::OnListGetDispInfo(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pDispInfo = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
	const auto pItem = &pDispInfo->item;

	if (pItem->mask & LVIF_TEXT)
	{
		const auto iItemID = pItem->iItem;
		const auto nMaxLengh = static_cast<size_t>(pItem->cchTextMax);
		const auto pBkm = m_pBookmarks->GetByIndex(static_cast<ULONGLONG>(iItemID));
		if (pBkm == nullptr)
			return;

		ULONGLONG ullOffset { 0 };
		ULONGLONG ullSize { 0 };
		switch (pItem->iSubItem)
		{
		case 0: //Index number.
			if (m_fShowAsHex)
				swprintf_s(pItem->pszText, nMaxLengh, L"0x%x", iItemID + 1);
			else
				swprintf_s(pItem->pszText, nMaxLengh, L"%d", iItemID + 1);
			break;
		case 1: //Offset
			if (!pBkm->vecSpan.empty())
				ullOffset = pBkm->vecSpan.front().ullOffset;
			if (m_fShowAsHex)
				swprintf_s(pItem->pszText, nMaxLengh, L"0x%llX", ullOffset);
			else
				swprintf_s(pItem->pszText, nMaxLengh, L"%llu", ullOffset);
			break;
		case 2: //Size.
			if (!pBkm->vecSpan.empty())
				ullSize = std::accumulate(pBkm->vecSpan.begin(), pBkm->vecSpan.end(), 0ULL,
					[](auto ullTotal, const HEXSPANSTRUCT& ref) {return ullTotal + ref.ullSize; });
			if (m_fShowAsHex)
				swprintf_s(pItem->pszText, nMaxLengh, L"0x%llX", ullSize);
			else
				swprintf_s(pItem->pszText, nMaxLengh, L"%llu", ullSize);
			break;
		case 3: //Description
			pItem->pszText = const_cast<wchar_t*>(pBkm->wstrDesc.data());
			break;
		}
	}
}

void CHexDlgBkmMgr::OnListItemChanged(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	//Go selected bookmark only with keyboard arrows and lmouse clicks.
	//Does not trigger (LVN_ITEMCHANGED event) when updating bookmark: !(pNMI->uNewState & LVIS_SELECTED)
	if (const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
		pNMI->iItem != -1 && pNMI->iSubItem != -1 && (pNMI->uNewState & LVIS_SELECTED))
		m_pBookmarks->GoBookmark(static_cast<ULONGLONG>(pNMI->iItem));
}

void CHexDlgBkmMgr::OnListItemLClick(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	if (const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR); pNMI->iItem != -1 && pNMI->iSubItem != -1)
		m_pBookmarks->GoBookmark(static_cast<ULONGLONG>(pNMI->iItem));
}

void CHexDlgBkmMgr::OnListDblClick(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	if (const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR); pNMI->iItem != -1 && pNMI->iSubItem != -1)
		SendMessageW(WM_COMMAND, static_cast<UINT_PTR>(EMenuID::IDM_BKMMGR_EDIT));
}

void CHexDlgBkmMgr::OnListRClick(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	bool fEnabled { false };
	if (const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR); pNMI->iItem != -1 && pNMI->iSubItem != -1)
		fEnabled = true;

	//Edit menu enabled only when one item selected.
	m_stMenuList.EnableMenuItem(static_cast<UINT_PTR>(EMenuID::IDM_BKMMGR_EDIT),
		(fEnabled && (m_pListMain->GetSelectedCount() == 1) ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
	m_stMenuList.EnableMenuItem(static_cast<UINT_PTR>(EMenuID::IDM_BKMMGR_REMOVE), (fEnabled ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
	m_stMenuList.EnableMenuItem(static_cast<UINT_PTR>(EMenuID::IDM_BKMMGR_CLEARALL),
		(m_pListMain->GetItemCount() > 0 ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);

	POINT pt;
	GetCursorPos(&pt);
	m_stMenuList.TrackPopupMenuEx(TPM_LEFTALIGN, pt.x, pt.y, this, nullptr);
}

void CHexDlgBkmMgr::OnListCellColor(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	if (const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR); pNMI->iSubItem == 4)
	{
		if (auto pBkm = m_pBookmarks->GetByIndex(static_cast<size_t>(pNMI->iItem)); pBkm != nullptr)
		{
			static LISTEXCELLCOLOR stCellClr;
			stCellClr.clrBk = pBkm->clrBk;
			stCellClr.clrText = pBkm->clrText;
			pNMI->lParam = reinterpret_cast<LPARAM>(&stCellClr);
		}
	}
}

void CHexDlgBkmMgr::UpdateList()
{
	m_pListMain->SetItemCountEx(static_cast<int>(m_pBookmarks->GetCount()), LVSICF_NOSCROLL);
	m_time = m_pBookmarks->GetTouchTime();
}

void CHexDlgBkmMgr::SortBookmarks()
{
	//Sort bookmarks according to clicked column.
	m_pBookmarks->SortData(m_pListMain->GetSortColumn(), m_pListMain->GetSortAscending());
	m_pListMain->RedrawWindow();
}

void CHexDlgBkmMgr::OnDestroy()
{
	CDialogEx::OnDestroy();

	m_pListMain->DestroyWindow();
	m_stMenuList.DestroyMenu();
}

void CHexDlgBkmMgr::OnClickRadioDec()
{
	m_fShowAsHex = false;
	m_pListMain->RedrawWindow();
}

void CHexDlgBkmMgr::OnClickRadioHex()
{
	m_fShowAsHex = true;
	m_pListMain->RedrawWindow();
}

void CHexDlgBkmMgr::SetDisplayMode(bool fShowAsHex)
{
	m_fShowAsHex = fShowAsHex;

	auto pHexRadio = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_BKMMGR_RADIO_HEX));
	auto pDecRadio = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_BKMMGR_RADIO_DEC));
	if (pHexRadio && pDecRadio)
	{
		pHexRadio->SetCheck(m_fShowAsHex ? BST_CHECKED : BST_UNCHECKED);
		pDecRadio->SetCheck(m_fShowAsHex ? BST_UNCHECKED : BST_CHECKED);
	}
	
	m_pListMain->RedrawWindow();
}