/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#pragma once
#include "../../res/HexCtrlRes.h"
#include "../CHexBookmarks.h"
#include "../ListEx/ListEx.h"
#include "CHexDlgBookmarkProps.h"
#include <afxdialogex.h>  //Standard MFC's controls header.

namespace HEXCTRL::INTERNAL
{
	using namespace HEXCTRL::INTERNAL::LISTEX;
	class CHexDlgBookmarkMgr final : public CDialogEx
	{
	public:
		explicit CHexDlgBookmarkMgr(CWnd* pParent = nullptr) : CDialogEx(IDD_HEXCTRL_BOOKMARKMGR, pParent) {}
		BOOL Create(UINT nIDTemplate, CWnd* pParent, CHexBookmarks* pBookmarks);
	protected:
		void DoDataExchange(CDataExchange* pDX)override;
		BOOL OnInitDialog()override;
		afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		BOOL OnCommand(WPARAM wParam, LPARAM lParam)override;
		BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)override;
		afx_msg void OnDestroy();
		afx_msg void OnListBkmGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnListBkmItemChanged(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnListBkmDblClick(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnListBkmRClick(NMHDR *pNMHDR, LRESULT *pResult);
		void UpdateList();
		void SortBookmarks();
		DECLARE_MESSAGE_MAP()
	private:
		IListExPtr m_List { CreateListEx() };
		CHexBookmarks* m_pBookmarks { };
		CMenu m_stMenuList;
		ULONGLONG m_ullCurrBkmID { }; //Currently selected bookmark ID.
		__time64_t m_time { };
		enum EMenuID {
			IDC_HEXCTRL_BOOKMARKMGR_MENU_NEW = 0x8000,
			IDC_HEXCTRL_BOOKMARKMGR_MENU_EDIT = 0x8001,
			IDC_HEXCTRL_BOOKMARKMGR_MENU_REMOVE = 0x8002,
			IDC_HEXCTRL_BOOKMARKMGR_MENU_CLEARALL = 0x8003
		};
	};
}