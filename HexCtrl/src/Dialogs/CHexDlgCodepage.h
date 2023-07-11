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
	class CHexDlgCodepage final : public CDialogEx {
	public:
		void AddCP(std::wstring_view wsv);
		[[nodiscard]] auto GetDlgData()const->std::uint64_t;
		void Initialize(IHexCtrl* pHexCtrl);
		auto SetDlgData(std::uint64_t ullData) -> HWND;
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
		LISTEX::IListExPtr m_pListMain { LISTEX::CreateListEx() };
		std::vector<SCODEPAGE> m_vecCodePage { };
	};
}