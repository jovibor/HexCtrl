/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#pragma once
#include <afxcontrolbars.h>  //Standard MFC's controls header.
#include "../CHexBookmarks.h"
#include "CHexDlgBookmarkProps.h"
#include "../../res/HexCtrlRes.h"
#include "../ListEx/ListEx.h"

namespace HEXCTRL::INTERNAL
{
	using namespace HEXCTRL::INTERNAL::LISTEX;
	class CHexDlgBookmarkMgr final : public CDialogEx
	{
	public:
		explicit CHexDlgBookmarkMgr(CWnd* pParent = nullptr) : CDialogEx(IDD_HEXCTRL_BOOKMARKMGR, pParent) {}
		virtual ~CHexDlgBookmarkMgr() {}
		BOOL Create(UINT nIDTemplate, CWnd* pParent, CHexBookmarks* pBookmarks);
	protected:
		virtual void DoDataExchange(CDataExchange* pDX);
		virtual BOOL OnInitDialog();
		virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
		afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
		DECLARE_MESSAGE_MAP()
	private:
		void UpdateList();
	private:
		IListExPtr m_List { CreateListEx() };
		CHexBookmarks* m_pBookmarks { };
		CMenu m_stMenuList;
		DWORD m_dwCurrBkmId { }; //Currently selected bookmark Id.
		int m_iCurrListId { };   //Currently selected list Id.
		std::time_t m_time { };
	public:
		afx_msg void OnDestroy();
	};
}