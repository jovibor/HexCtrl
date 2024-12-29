/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../../HexCtrl.h"
#include <memory>

import HEXCTRL.HexUtility;

namespace HEXCTRL::INTERNAL {
	class CHexDlgOpers;    //Forward declarations.
	class CHexDlgFillData;
	class CHexDlgModify final {
	public:
		CHexDlgModify();
		~CHexDlgModify();
		void CreateDlg();
		[[nodiscard]] auto GetDlgItemHandle(EHexDlgItem eItem)const->HWND;
		[[nodiscard]] auto GetHWND()const->HWND;
		void Initialize(IHexCtrl* pHexCtrl, HINSTANCE hInstRes);
		[[nodiscard]] bool PreTranslateMsg(MSG* pMsg);
		[[nodiscard]] auto ProcessMsg(const MSG& msg) -> INT_PTR;
		void SetDlgProperties(std::uint64_t u64Flags);
		void ShowWindow(int iCmdShow, int iTab = -1);
	private:
		auto OnActivate(const MSG& msg) -> INT_PTR;
		auto OnClose() -> INT_PTR;
		auto OnDestroy() -> INT_PTR;
		auto OnInitDialog(const MSG& msg) -> INT_PTR;
		auto OnNotify(const MSG& msg) -> INT_PTR;
		void OnNotifyTabSelChanged(NMHDR* pNMHDR);
		void SetCurrentTab(int iTab);
	private:
		HINSTANCE m_hInstRes { };
		wnd::CWnd m_Wnd;              //Main window.
		wnd::CWndTab m_WndTab;        //Tab control.
		IHexCtrl* m_pHexCtrl { };
		std::uint64_t m_u64Flags { }; //Data from SetDlgProperties.
		std::unique_ptr<CHexDlgOpers> m_pDlgOpers;       //"Operations" tab dialog.
		std::unique_ptr<CHexDlgFillData> m_pDlgFillData; //"Fill with" tab dialog.
	};
}