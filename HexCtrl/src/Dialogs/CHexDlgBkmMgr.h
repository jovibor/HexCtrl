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
	/********************************************
	* CHexDlgBkmMgr class declaration.          *
	********************************************/
	class CHexDlgBkmMgr final : public CDialogEx
	{
		enum class EMenuID : WORD {
			IDM_BKMMGR_NEW = 0x8000, IDM_BKMMGR_EDIT = 0x8001,
			IDM_BKMMGR_REMOVE = 0x8002, IDM_BKMMGR_CLEARALL = 0x8003
		};
	public:
		BOOL Create(UINT nIDTemplate, CWnd* pParent, CHexBookmarks* pBookmarks);
		void SetDisplayMode(bool fShowAsHex);
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
		afx_msg void OnClickRadioDec();
		afx_msg void OnClickRadioHex();
		void UpdateList();
		void SortBookmarks();
		DECLARE_MESSAGE_MAP()
	private:
		IListExPtr m_pListMain { CreateListEx() };
		CHexBookmarks* m_pBookmarks { };
		CMenu m_stMenuList;
		__time64_t m_time { };
		bool m_fShowAsHex{ false };
	};
}