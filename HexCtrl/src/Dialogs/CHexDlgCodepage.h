/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../../HexCtrl.h"
#include "../../dep/ListEx/ListEx.h"
#include <afxdialogex.h>

namespace HEXCTRL::INTERNAL
{
	using namespace HEXCTRL::LISTEX;
	class CHexDlgCodepage final : public CDialogEx
	{
	public:
		void AddCP(std::wstring_view wsv);
		void Initialize(IHexCtrl* pHexCtrl);
		BOOL ShowWindow(int nCmdShow);
	private:
		void DoDataExchange(CDataExchange* pDX)override;
		BOOL OnInitDialog()override;
		afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)override;
		afx_msg void OnListGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnListItemChanged(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnListGetColor(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnListLinkClick(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnDestroy();
		void SortList();
		static BOOL CALLBACK EnumCodePagesProc(LPWSTR pwszCP);
		DECLARE_MESSAGE_MAP();
	private:
		struct SCODEPAGE {
			int iCPID { };
			std::wstring wstrName { };
			UINT uMaxChars { };
		};
		inline static CHexDlgCodepage* m_pThis { };
		IHexCtrl* m_pHexCtrl { };
		IListExPtr m_pListMain { CreateListEx() };
		std::vector<SCODEPAGE> m_vecCodePage { };
	};
}