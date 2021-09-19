/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../CHexBookmarks.h"
#include "../../dep/ListEx/ListEx.h"
#include <afxdialogex.h>

namespace HEXCTRL::INTERNAL
{
	using namespace HEXCTRL::LISTEX;
	class CHexDlgBkmMgr final : public CDialogEx
	{
		enum class EMenuID : std::uint16_t;
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
		afx_msg void OnListLClick(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnListDblClick(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnListRClick(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnListGetColor(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnClickRadioHexDec();
		void UpdateList();
		void SortBookmarks();
		DECLARE_MESSAGE_MAP()
	private:
		IListExPtr m_pListMain { CreateListEx() };
		CHexBookmarks* m_pBookmarks { };
		CMenu m_stMenuList;
		__time64_t m_time { };
		bool m_fShowAsHex { true };
	};
}