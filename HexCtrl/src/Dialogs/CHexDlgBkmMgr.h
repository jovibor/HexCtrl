/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#pragma once
#include "../CHexBookmarks.h"
#include "../ListEx/ListEx.h"
#include "CHexDlgBkmProps.h"
#include <afxdialogex.h>

namespace HEXCTRL::INTERNAL
{
	using namespace HEXCTRL::LISTEX;
	class CHexDlgBkmMgr final : public CDialogEx
	{
	public:
		BOOL Create(UINT nIDTemplate, CWnd* pParent, CHexBookmarks* pBookmarks);
	protected:
		void DoDataExchange(CDataExchange* pDX)override;
		BOOL OnInitDialog()override;
		afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		BOOL OnCommand(WPARAM wParam, LPARAM lParam)override;
		BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)override;
		afx_msg void OnDestroy();
		afx_msg void OnListGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnListItemChanged(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnListItemLClick(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnListDblClick(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnListRClick(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnListCellColor(NMHDR *pNMHDR, LRESULT *pResult);
		void UpdateList();
		void SortBookmarks();
		DECLARE_MESSAGE_MAP()
	private:
		IListExPtr m_pListMain { CreateListEx() };
		CHexBookmarks* m_pBookmarks { };
		CMenu m_stMenuList;
		ULONGLONG m_ullCurrBkmIndex { }; //Currently selected bookmark index.
		__time64_t m_time { };
		enum EMenuID {
			IDC_HEXCTRL_BKMMGR_MENU_NEW = 0x8000,
			IDC_HEXCTRL_BKMMGR_MENU_EDIT = 0x8001,
			IDC_HEXCTRL_BKMMGR_MENU_REMOVE = 0x8002,
			IDC_HEXCTRL_BKMMGR_MENU_CLEARALL = 0x8003
		};
	};
}