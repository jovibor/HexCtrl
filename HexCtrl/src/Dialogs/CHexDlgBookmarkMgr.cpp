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
#include <numeric>

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

namespace HEXCTRL::INTERNAL {
	constexpr auto IDC_HEXCTRL_BOOKMARKMGR_MENU_NEW = 0x8000;
	constexpr auto IDC_HEXCTRL_BOOKMARKMGR_MENU_EDIT = 0x8001;
	constexpr auto IDC_HEXCTRL_BOOKMARKMGR_MENU_REMOVE = 0x8002;
	constexpr auto IDC_HEXCTRL_BOOKMARKMGR_MENU_CLEARALL = 0x8003;
}

BEGIN_MESSAGE_MAP(CHexDlgBookmarkMgr, CDialogEx)
	ON_WM_ACTIVATE()
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
	m_List->InsertColumn(0, L"\u2116", LVCFMT_RIGHT, 30);
	m_List->InsertColumn(1, L"Offset", LVCFMT_RIGHT, 80);
	m_List->InsertColumn(2, L"Size", LVCFMT_RIGHT, 80);
	m_List->InsertColumn(3, L"Description", LVCFMT_LEFT, 215);
	m_List->InsertColumn(4, L"Bk color", LVCFMT_LEFT, 80);
	m_List->SetColumnSortMode(0, EnListExSortMode::SORT_NUMERIC);
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
		switch (pNMI->hdr.code)
		{
		case NM_RCLICK:
		{
			bool fEnabled;
			if (pNMI->iItem == -1 || pNMI->iSubItem == -1)
				fEnabled = false;
			else
			{
				fEnabled = true;
				m_dwCurrBkmId = (DWORD)m_List->GetItemData(pNMI->iItem);
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

			m_pBookmarks->GoBookmark((DWORD)m_List->GetItemData(pNMI->iItem));
		}
		break;
		case NM_DBLCLK:
			if (pNMI->iItem == -1 || pNMI->iSubItem == -1)
				break;

			m_dwCurrBkmId = (DWORD)m_List->GetItemData(pNMI->iItem);
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
		auto optBkm = m_pBookmarks->GetBookmark(m_dwCurrBkmId);
		if (optBkm.has_value())
		{
			CHexDlgBookmarkProps dlgBkmEdit;
			if (dlgBkmEdit.DoModal(&*optBkm) == IDOK)
			{
				m_pBookmarks->Update(optBkm->dwID, *optBkm);
				UpdateList();
			}
		}
	}
	break;
	case IDC_HEXCTRL_BOOKMARKMGR_MENU_REMOVE:
		m_pBookmarks->RemoveId(m_dwCurrBkmId);
		m_List->DeleteItem(m_iCurrListId);
		UpdateList();
		break;
	case IDC_HEXCTRL_BOOKMARKMGR_MENU_CLEARALL:
		m_pBookmarks->ClearAll();
		UpdateList();
		break;
	}

	return CDialogEx::OnCommand(wParam, lParam);
}

void CHexDlgBookmarkMgr::UpdateList()
{
	m_List->DeleteAllItems();
	int listindex { };
	WCHAR wstr[32];

	m_List->SetRedraw(FALSE);
	for (const auto& iter : *m_pBookmarks->GetData())
	{
		swprintf_s(wstr, 8, L"%i", listindex + 1);
		m_List->InsertItem(listindex, wstr);

		ULONGLONG ullOffset { 0 };
		ULONGLONG ullSize { 0 };
		if (!iter.vecSpan.empty())
		{
			ullOffset = iter.vecSpan.front().ullOffset;
			ullSize = std::accumulate(iter.vecSpan.begin(), iter.vecSpan.end(), ULONGLONG { },
				[](auto ullTotal, const HEXSPANSTRUCT& ref) {return ullTotal + ref.ullSize; });
		}
		swprintf_s(wstr, 32, L"0x%llX", ullOffset);
		m_List->SetItemText(listindex, 1, wstr);
		swprintf_s(wstr, 32, L"0x%llX", ullSize);
		m_List->SetItemText(listindex, 2, wstr);
		m_List->SetItemText(listindex, 3, iter.wstrDesc.data());
		m_List->SetCellColor(listindex, 4, iter.clrBk);
		m_List->SetItemData(listindex, (DWORD_PTR)iter.dwID);

		++listindex;
	}
	m_List->SetRedraw(TRUE);
	m_time = m_pBookmarks->GetTouchTime();
}

void CHexDlgBookmarkMgr::OnDestroy()
{
	CDialogEx::OnDestroy();

	m_List->DestroyWindow();
	m_stMenuList.DestroyMenu();
}