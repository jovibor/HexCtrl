/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../../HexCtrl.h"
#include <afxdialogex.h>
#include "CHexDlgFillData.h"
#include "CHexDlgOpers.h"

namespace HEXCTRL::INTERNAL
{
	class CHexDlgModify final : public CDialogEx {
	public:
		[[nodiscard]] auto GetDlgData()const->std::uint64_t;
		void Initialize(IHexCtrl* pHexCtrl);
		auto SetDlgData(std::uint64_t ullData) -> HWND;
		BOOL ShowWindow(int nCmdShow, int iTab);
	private:
		afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		void DoDataExchange(CDataExchange* pDX)override;
		BOOL OnInitDialog()override;
		afx_msg void OnTabSelChanged(NMHDR* pNMHDR, LRESULT* pResult);
		void SetCurrentTab(int iTab);
		DECLARE_MESSAGE_MAP();
	private:
		IHexCtrl* m_pHexCtrl { };
		CTabCtrl m_tabMain;
		const std::unique_ptr<CHexDlgFillData> m_pDlgFillData { std::make_unique<CHexDlgFillData>() }; //"Fill with" tab dialog.
		const std::unique_ptr<CHexDlgOpers> m_pDlgOpers { std::make_unique<CHexDlgOpers>() }; //"Operations" tab dialog.
	};
}