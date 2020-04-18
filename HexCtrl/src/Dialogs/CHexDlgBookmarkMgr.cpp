/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#include "stdafx.h"
#include "CHexDlgBookmarkMgr.h"
#include <algorithm>
#include <numeric>

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

BEGIN_MESSAGE_MAP(CHexDlgBookmarkMgr, CDialogEx)
	ON_WM_ACTIVATE()
	ON_NOTIFY(LVN_GETDISPINFOW, IDC_HEXCTRL_BOOKMARKMGR_LIST, &CHexDlgBookmarkMgr::OnListBkmGetDispInfo)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_HEXCTRL_BOOKMARKMGR_LIST, &CHexDlgBookmarkMgr::OnListBkmItemChanged)
	ON_NOTIFY(NM_CLICK, IDC_HEXCTRL_BOOKMARKMGR_LIST, &CHexDlgBookmarkMgr::OnListBkmItemLClick)
	ON_NOTIFY(NM_DBLCLK, IDC_HEXCTRL_BOOKMARKMGR_LIST, &CHexDlgBookmarkMgr::OnListBkmDblClick)
	ON_NOTIFY(NM_RCLICK, IDC_HEXCTRL_BOOKMARKMGR_LIST, &CHexDlgBookmarkMgr::OnListBkmRClick)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL CHexDlgBookmarkMgr::Create(UINT nIDTemplate, CWnd* pParent, CHexBookmarks* pBookmarks)
{
	m_pBookmarks = pBookmarks;

	return CDialogEx::Create(nIDTemplate, pParent);
}

void CHexDlgBookmarkMgr::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BOOL CHexDlgBookmarkMgr::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_stList->CreateDialogCtrl(IDC_HEXCTRL_BOOKMARKMGR_LIST, this);
	m_stList->SetSortable(true);
	m_stList->InsertColumn(0, L"\u2116", LVCFMT_RIGHT, 40);
	m_stList->InsertColumn(1, L"Offset", LVCFMT_RIGHT, 80);
	m_stList->InsertColumn(2, L"Size", LVCFMT_RIGHT, 80);
	m_stList->InsertColumn(3, L"Description", LVCFMT_LEFT, 210);
	m_stList->InsertColumn(4, L"Bk color", LVCFMT_LEFT, 65);
	m_stList->SetExtendedStyle(LVS_EX_HEADERDRAGDROP);

	m_stMenuList.CreatePopupMenu();
	m_stMenuList.AppendMenuW(MF_BYPOSITION, IDC_HEXCTRL_BOOKMARKMGR_MENU_NEW, L"New");
	m_stMenuList.AppendMenuW(MF_BYPOSITION, IDC_HEXCTRL_BOOKMARKMGR_MENU_EDIT, L"Edit");
	m_stMenuList.AppendMenuW(MF_BYPOSITION, IDC_HEXCTRL_BOOKMARKMGR_MENU_REMOVE, L"Remove");
	m_stMenuList.AppendMenuW(MF_SEPARATOR);
	m_stMenuList.AppendMenuW(MF_BYPOSITION, IDC_HEXCTRL_BOOKMARKMGR_MENU_CLEARALL, L"Clear all");

	return TRUE;
}

void CHexDlgBookmarkMgr::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if (nState == WA_INACTIVE)
		SetLayeredWindowAttributes(0, 150, LWA_ALPHA);
	else
	{
		SetLayeredWindowAttributes(0, 255, LWA_ALPHA);
		if (!m_time || m_time != m_pBookmarks->GetTouchTime())
			UpdateList();

		m_stList->SetItemState(-1, 0, LVIS_SELECTED);
		m_stList->SetItemState(static_cast<int>(m_pBookmarks->GetCurrent()), LVIS_SELECTED, LVIS_SELECTED);
		m_stList->EnsureVisible(static_cast<int>(m_pBookmarks->GetCurrent()), FALSE);
	}

	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
}

BOOL CHexDlgBookmarkMgr::OnCommand(WPARAM wParam, LPARAM lParam)
{
	bool fHere { true }; //Process message here, and not pass further, to parent.
	switch (LOWORD(wParam))
	{
	case IDC_HEXCTRL_BOOKMARKMGR_MENU_NEW:
	{
		HEXBOOKMARKSTRUCT hbs;
		CHexDlgBookmarkProps dlgBkmEdit;
		if (dlgBkmEdit.DoModal(hbs) == IDOK)
		{
			m_pBookmarks->Add(hbs);
			UpdateList();
		}
	}
	break;
	case IDC_HEXCTRL_BOOKMARKMGR_MENU_EDIT:
	{
		if (auto pBkm = m_pBookmarks->GetByIndex(m_ullCurrBkmIndex); pBkm != nullptr)
		{
			CHexDlgBookmarkProps dlgBkmEdit;
			auto stBkm = *pBkm; //Pass a copy to dlgBkmEdit to avoid changing the original, from list.
			if (dlgBkmEdit.DoModal(stBkm) == IDOK)
			{
				m_pBookmarks->Update(pBkm->ullID, stBkm);
				UpdateList();
			}
		}
	}
	break;
	case IDC_HEXCTRL_BOOKMARKMGR_MENU_REMOVE:
		if (auto pBkm = m_pBookmarks->GetByIndex(m_ullCurrBkmIndex); pBkm != nullptr)
		{
			m_pBookmarks->RemoveByID(pBkm->ullID);
			UpdateList();
		}
		break;
	case IDC_HEXCTRL_BOOKMARKMGR_MENU_CLEARALL:
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

BOOL CHexDlgBookmarkMgr::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
	if (pNMI->hdr.idFrom == IDC_HEXCTRL_BOOKMARKMGR_LIST)
	{
		switch (pNMI->hdr.code)
		{
		case LVN_COLUMNCLICK: //ON_NOTIFY(LVN_COLUMNCLICK...) macro doesn't seem work, for no obvious reason.
			if (!m_pBookmarks->IsVirtual())
				SortBookmarks();
			break;
		case LISTEX_MSG_CELLCOLOR: //Dynamically set cells colors.
			if (pNMI->iSubItem == 4)
			{
				if (auto pBkm = m_pBookmarks->GetByIndex(static_cast<size_t>(pNMI->iItem)); pBkm != nullptr)
				{
					static LISTEXCELLCOLOR stCellClr;
					stCellClr.clrBk = pBkm->clrBk;
					stCellClr.clrText = pBkm->clrText;
					pNMI->lParam = reinterpret_cast<LPARAM>(&stCellClr);
				}
			}
			break;
		}
	}

	return CDialogEx::OnNotify(wParam, lParam, pResult);
}

void CHexDlgBookmarkMgr::OnListBkmGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult)
{
	auto pDispInfo = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
	auto pItem = &pDispInfo->item;

	if (pItem->mask & LVIF_TEXT)
	{
		auto iItemID = pItem->iItem;
		auto iMaxLengh = pItem->cchTextMax;
		const auto pBkm = m_pBookmarks->GetByIndex(static_cast<ULONGLONG>(iItemID));
		if (pBkm == nullptr)
			return;

		ULONGLONG ullOffset { 0 };
		ULONGLONG ullSize { 0 };
		switch (pItem->iSubItem)
		{
		case 0: //Index number.
			swprintf_s(pItem->pszText, static_cast<size_t>(iMaxLengh), L"%d", iItemID + 1);
			break;
		case 1: //Offset
			if (!pBkm->vecSpan.empty())
				ullOffset = pBkm->vecSpan.front().ullOffset;
			swprintf_s(pItem->pszText, static_cast<size_t>(iMaxLengh), L"0x%llX", ullOffset);
			break;
		case 2: //Size.
			if (!pBkm->vecSpan.empty())
				ullSize = std::accumulate(pBkm->vecSpan.begin(), pBkm->vecSpan.end(), 0ULL,
					[](auto ullTotal, const HEXSPANSTRUCT& ref) {return ullTotal + ref.ullSize; });
			swprintf_s(pItem->pszText, static_cast<size_t>(iMaxLengh), L"0x%llX", ullSize);
			break;
		case 3: //Description
			pItem->pszText = const_cast<wchar_t*>(pBkm->wstrDesc.data());
			break;
		}
	}
	*pResult = 0;
}

void CHexDlgBookmarkMgr::OnListBkmItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	//Go selected bookmark only with keyboard arrows and lmouse clicks.
	//Does not trigger (LVN_ITEMCHANGED event) when updating bookmark: !(pNMI->uNewState & LVIS_SELECTED)
	if (pNMI->iItem != -1 && pNMI->iSubItem != -1 && (pNMI->uNewState & LVIS_SELECTED))
		m_pBookmarks->GoBookmark(static_cast<ULONGLONG>(pNMI->iItem));

	*pResult = 0;
}

void CHexDlgBookmarkMgr::OnListBkmItemLClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	if (pNMI->iItem != -1 && pNMI->iSubItem != -1)
		m_pBookmarks->GoBookmark(static_cast<ULONGLONG>(pNMI->iItem));

	*pResult = 0;
}

void CHexDlgBookmarkMgr::OnListBkmDblClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	if (pNMI->iItem != -1 && pNMI->iSubItem != -1)
	{
		m_ullCurrBkmIndex = static_cast<ULONGLONG>(pNMI->iItem);
		SendMessageW(WM_COMMAND, IDC_HEXCTRL_BOOKMARKMGR_MENU_EDIT);
	}

	*pResult = 0;
}

void CHexDlgBookmarkMgr::OnListBkmRClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	bool fEnabled { false };
	if (pNMI->iItem != -1 && pNMI->iSubItem != -1)
	{
		fEnabled = true;
		m_ullCurrBkmIndex = static_cast<ULONGLONG>(pNMI->iItem);
	}
	m_stMenuList.EnableMenuItem(IDC_HEXCTRL_BOOKMARKMGR_MENU_EDIT, (fEnabled ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
	m_stMenuList.EnableMenuItem(IDC_HEXCTRL_BOOKMARKMGR_MENU_REMOVE, (fEnabled ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
	m_stMenuList.EnableMenuItem(IDC_HEXCTRL_BOOKMARKMGR_MENU_CLEARALL,
		(m_stList->GetItemCount() > 0 ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);

	POINT pt;
	GetCursorPos(&pt);
	m_stMenuList.TrackPopupMenuEx(TPM_LEFTALIGN, pt.x, pt.y, this, nullptr);

	*pResult = 0;
}

void CHexDlgBookmarkMgr::UpdateList()
{
	m_stList->SetItemCountEx(static_cast<int>(m_pBookmarks->GetCount()), LVSICF_NOSCROLL);
	m_time = m_pBookmarks->GetTouchTime();
}

void CHexDlgBookmarkMgr::SortBookmarks()
{
	auto pData = m_pBookmarks->GetData();
	const auto iColumn = m_stList->GetSortColumn();
	const auto fAscending = m_stList->GetSortAscending();

	//Sorts bookmarks according to clicked column.
	std::sort(pData->begin(), pData->end(), [iColumn, fAscending](const HEXBOOKMARKSTRUCT& st1, const HEXBOOKMARKSTRUCT& st2)
	{
		int iCompare { };
		switch (iColumn)
		{
		case 0:
			break;
		case 1: //Offset.
		{
			ULONGLONG ullOffset1 { 0 };
			ULONGLONG ullOffset2 { 0 };
			if (!st1.vecSpan.empty() && !st2.vecSpan.empty())
			{
				ullOffset1 = st1.vecSpan.front().ullOffset;
				ullOffset2 = st2.vecSpan.front().ullOffset;
				iCompare = ullOffset1 < ullOffset2 ? -1 : 1;
			}
		}
		break;
		case 2: //Size.
		{
			ULONGLONG ullSize1 { 0 };
			ULONGLONG ullSize2 { 0 };
			if (!st1.vecSpan.empty() && !st2.vecSpan.empty())
			{
				ullSize1 = std::accumulate(st1.vecSpan.begin(), st1.vecSpan.end(), 0ULL,
					[](auto ullTotal, const HEXSPANSTRUCT& ref) {return ullTotal + ref.ullSize; });
				ullSize2 = std::accumulate(st2.vecSpan.begin(), st2.vecSpan.end(), 0ULL,
					[](auto ullTotal, const HEXSPANSTRUCT& ref) {return ullTotal + ref.ullSize; });
				iCompare = ullSize1 < ullSize2 ? -1 : 1;
			}
		}
		break;
		case 3: //Description.
			iCompare = st1.wstrDesc.compare(st2.wstrDesc);
			break;
		}

		bool fResult { false };
		if (fAscending)
		{
			if (iCompare < 0)
				fResult = true;
		}
		else
		{
			if (iCompare > 0)
				fResult = true;
		}

		return fResult;
	});

	m_stList->RedrawWindow();
}

void CHexDlgBookmarkMgr::OnDestroy()
{
	CDialogEx::OnDestroy();

	m_stList->DestroyWindow();
	m_stMenuList.DestroyMenu();
}