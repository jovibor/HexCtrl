/****************************************************************************************
* Copyright © 2018-2024 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../../HexCtrl.h"
#include <afxdialogex.h>
#include <memory>

namespace HEXCTRL::INTERNAL {
	class CHexDlgOpers;    //Forward declarations.
	class CHexDlgFillData;
	class CHexDlgModify final : public CDialogEx {
	public:
		CHexDlgModify();
		~CHexDlgModify();
		[[nodiscard]] auto GetDlgData()const->std::uint64_t;
		void Initialize(IHexCtrl* pHexCtrl);
		auto SetDlgData(std::uint64_t ullData) -> HWND;
		BOOL ShowWindow(int nCmdShow, int iTab);
	private:
		void DoDataExchange(CDataExchange* pDX)override;
		afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		BOOL OnInitDialog()override;
		afx_msg void OnTabSelChanged(NMHDR* pNMHDR, LRESULT* pResult);
		void SetCurrentTab(int iTab);
		DECLARE_MESSAGE_MAP();
	private:
		IHexCtrl* m_pHexCtrl { };
		CTabCtrl m_tabMain;
		std::unique_ptr<CHexDlgOpers> m_pDlgOpers;       //"Operations" tab dialog.
		std::unique_ptr<CHexDlgFillData> m_pDlgFillData; //"Fill with" tab dialog.
	};
}