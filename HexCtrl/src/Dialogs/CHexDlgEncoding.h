/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#pragma once
#include "../CHexCtrl.h"
#include "../../dep/ListEx/ListEx.h"
#include <afxdialogex.h>

namespace HEXCTRL::INTERNAL
{
	using namespace HEXCTRL::LISTEX;
	class CHexDlgEncoding final : public CDialogEx
	{
		struct SCODEPAGE
		{
			int iCPID { };
			std::wstring wstrName { };
			UINT uMaxChars { };
		};
	public:
		void AddCP(std::wstring_view wstr);
		BOOL Create(UINT nIDTemplate, CHexCtrl* pHexCtrl);
	private:
		inline static BOOL CALLBACK EnumCodePagesProc(LPWSTR pwszCP);
		void DoDataExchange(CDataExchange* pDX)override;
		BOOL OnInitDialog()override;
		afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)override;
		afx_msg void OnListGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnListItemChanged(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnListCellColor(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnListLinkClick(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnDestroy();
		DECLARE_MESSAGE_MAP()
		void SortList();
	private:
		inline static CHexDlgEncoding* m_pThis { };
		CHexCtrl* m_pHexCtrl { };
		IListExPtr m_pListMain { CreateListEx() };
		std::vector<SCODEPAGE> m_vecCodePage { };
	};
}