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
	ON_NOTIFY(LVN_GETDISPINFOW, IDC_HEXCTRL_BOOKMARKMGR_LIST, &CHexDlgBookmarkMgr::OnListBkmsGetDispInfo)
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

	m_List->CreateDialogCtrl(IDC_HEXCTRL_BOOKMARKMGR_LIST, this);
	m_List->SetSortable(true);
	m_List->InsertColumn(0, L"\u2116", LVCFMT_RIGHT, 40);
	m_List->InsertColumn(1, L"Offset", LVCFMT_RIGHT, 80);
	m_List->InsertColumn(2, L"Size", LVCFMT_RIGHT, 80);
	m_List->InsertColumn(3, L"Description", LVCFMT_LEFT, 210);
	m_List->InsertColumn(4, L"Bk color", LVCFMT_LEFT, 65);
	//	m_List->SetColumnSortMode(0, EnListExSortMode::SORT_NUMERIC);
	m_List->SetExtendedStyle(LVS_EX_HEADERDRAGDROP);

	m_stMenuList.CreatePopupMenu();
	m_stMenuList.AppendMenuW(MF_BYPOSITION, IDC_HEXCTRL_BOOKMARKMGR_MENU_NEW, L"New");
	m_stMenuList.AppendMenuW(MF_BYPOSITION, IDC_HEXCTRL_BOOKMARKMGR_MENU_EDIT, L"Edit");
	m_stMenuList.AppendMenuW(MF_BYPOSITION, IDC_HEXCTRL_BOOKMARKMGR_MENU_REMOVE, L"Remove");
	m_stMenuList.AppendMenuW(MF_SEPARATOR);
	m_stMenuList.AppendMenuW(MF_BYPOSITION, IDC_HEXCTRL_BOOKMARKMGR_MENU_CLEARALL, L"Clear all");

	return TRUE;
}

BOOL CHexDlgBookmarkMgr::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
	if (pNMI->hdr.idFrom == IDC_HEXCTRL_BOOKMARKMGR_LIST)
	{
		auto refData = m_pBookmarks->GetData();
		ULONGLONG ullBkmID { };
		if (pNMI->iItem >= 0 && pNMI->iItem < static_cast<int>(refData->size()))
			ullBkmID = refData->at(static_cast<size_t>(pNMI->iItem)).ullID;

		switch (pNMI->hdr.code)
		{
		case LVM_MAPINDEXTOID:
			pNMI->lParam = static_cast<LPARAM>(ullBkmID);
			break;
		case LVN_COLUMNCLICK:
			SortBookmarks();
			break;
		case NM_RCLICK:
		{
			bool fEnabled;
			if (pNMI->iItem == -1 || pNMI->iSubItem == -1)
				fEnabled = false;
			else
			{
				fEnabled = true;
				m_ullCurrBkmId = ullBkmID;
				m_iCurrListId = pNMI->iItem;
			}
			m_stMenuList.EnableMenuItem(IDC_HEXCTRL_BOOKMARKMGR_MENU_EDIT, (fEnabled ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
			m_stMenuList.EnableMenuItem(IDC_HEXCTRL_BOOKMARKMGR_MENU_REMOVE, (fEnabled ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
			m_stMenuList.EnableMenuItem(IDC_HEXCTRL_BOOKMARKMGR_MENU_CLEARALL,
				(m_List->GetItemCount() > 0 ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);

			POINT pt;
			GetCursorPos(&pt);
			m_stMenuList.TrackPopupMenuEx(TPM_LEFTALIGN, pt.x, pt.y, this, nullptr);
		}
		break;
		case LVN_ITEMCHANGED:
		case NM_CLICK:
		{
			//Go selected bookmark only with keyboard arrows and lmouse clicks.
			//Does not trigger (LVN_ITEMCHANGED event) when updating bookmark: !(pNMI->uNewState & LVIS_SELECTED)
			if (pNMI->iItem == -1 || pNMI->iSubItem == -1 || !(pNMI->uNewState & LVIS_SELECTED))
				break;

			m_pBookmarks->GoBookmark(ullBkmID);
		}
		break;
		case NM_DBLCLK:
			if (pNMI->iItem == -1 || pNMI->iSubItem == -1)
				break;

			m_ullCurrBkmId = ullBkmID;
			m_iCurrListId = pNMI->iItem;
			SendMessageW(WM_COMMAND, IDC_HEXCTRL_BOOKMARKMGR_MENU_EDIT);
			break;
		}
	}

	return CDialogEx::OnNotify(wParam, lParam, pResult);
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
		HEXBOOKMARKSTRUCT hbkms;
		CHexDlgBookmarkProps dlgBkmEdit;
		if (dlgBkmEdit.DoModal(&hbkms) == IDOK)
		{
			m_pBookmarks->Add(hbkms);
			UpdateList();
		}
	}
	break;
	case IDC_HEXCTRL_BOOKMARKMGR_MENU_EDIT:
	{
		auto optBkm = m_pBookmarks->GetBookmark(m_ullCurrBkmId);
		if (optBkm.has_value())
		{
			CHexDlgBookmarkProps dlgBkmEdit;
			if (dlgBkmEdit.DoModal(&*optBkm) == IDOK)
			{
				m_pBookmarks->Update(optBkm->ullID, *optBkm);
				UpdateList();
			}
		}
	}
	break;
	case IDC_HEXCTRL_BOOKMARKMGR_MENU_REMOVE:
		m_pBookmarks->RemoveId(m_ullCurrBkmId);
		m_List->DeleteItem(m_iCurrListId);
		UpdateList();
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

void CHexDlgBookmarkMgr::UpdateList()
{
	const auto refData = m_pBookmarks->GetData();
	m_List->SetItemCountEx(static_cast<int>(refData->size()), LVSICF_NOSCROLL);
	int listindex { 0 };
	for (const auto& iter : *refData)
	{
		m_List->SetCellColor(listindex++, 4, iter.clrBk);
	}

	m_time = m_pBookmarks->GetTouchTime();
}

void CHexDlgBookmarkMgr::OnDestroy()
{
	CDialogEx::OnDestroy();

	m_List->DestroyWindow();
	m_stMenuList.DestroyMenu();
}

void CHexDlgBookmarkMgr::OnListBkmsGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult)
{
	auto pDispInfo = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
	auto pItem = &pDispInfo->item;

	if (pItem->mask & LVIF_TEXT)
	{
		auto pBkm = m_pBookmarks->GetData();
		auto iItemID = pItem->iItem;
		auto iMaxLengh = pItem->cchTextMax;
		auto& refData = pBkm->at(static_cast<size_t>(iItemID));
		ULONGLONG ullOffset { 0 };
		ULONGLONG ullSize { 0 };
		switch (pItem->iSubItem)
		{
		case 0: //Index number.
			swprintf_s(pItem->pszText, static_cast<size_t>(iMaxLengh), L"%d", iItemID + 1);
			break;
		case 1: //Offset
			if (!refData.vecSpan.empty())
				ullOffset = refData.vecSpan.front().ullOffset;
			swprintf_s(pItem->pszText, static_cast<size_t>(iMaxLengh), L"0x%llX", ullOffset);
			break;
		case 2: //Size.
			if (!refData.vecSpan.empty())
				ullSize = std::accumulate(refData.vecSpan.begin(), refData.vecSpan.end(), 0ULL,
					[](auto ullTotal, const HEXSPANSTRUCT& ref) {return ullTotal + ref.ullSize; });
			swprintf_s(pItem->pszText, static_cast<size_t>(iMaxLengh), L"0x%llX", ullSize);
			break;
		case 3: //Description
			pItem->pszText = const_cast<wchar_t*>(refData.wstrDesc.data());
			break;
		}
	}
	*pResult = 0;
}

void CHexDlgBookmarkMgr::SortBookmarks()
{
	auto refData = const_cast<std::deque<HEXBOOKMARKSTRUCT>*>(m_pBookmarks->GetData());
	const auto iColumn = m_List->GetSortColumn();
	const auto fAscending = m_List->GetSortAscending();

	//Sorts bookmarks according to clicked column.
	std::sort(refData->begin(), refData->end(), [iColumn, fAscending](const HEXBOOKMARKSTRUCT& st1, const HEXBOOKMARKSTRUCT& st2)
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

	m_List->RedrawWindow();
}