/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../../HexCtrl.h"

import HEXCTRL.HexUtility;

namespace HEXCTRL::INTERNAL {
	class CHexDlgGoTo final {
	public:
		void CreateDlg();
		void DestroyWindow();
		void Initialize(IHexCtrl* pHexCtrl);
		[[nodiscard]] auto GetHWND()const->HWND;
		[[nodiscard]] bool IsRepeatAvail()const;
		[[nodiscard]] bool PreTranslateMsg(MSG* pMsg);
		[[nodiscard]] auto ProcessMsg(const MSG& stMsg) -> INT_PTR;
		void Repeat(bool fFwd = true); //fFwd: true - forward, false - backward.
		void SetDlgProperties(std::uint64_t u64Flags);
		void ShowWindow(int iCmdShow);
	private:
		enum class EGoMode : std::uint8_t;
		[[nodiscard]] auto GetHexCtrl()const->IHexCtrl*;
		[[nodiscard]] auto GetGoMode()const->EGoMode;
		void GoTo(bool fForward);
		[[nodiscard ]] bool IsNoEsc()const;
		auto OnActivate(const MSG& stMsg) -> INT_PTR;
		void OnCancel();
		auto OnClose() -> INT_PTR;
		auto OnCommand(const MSG& stMsg) -> INT_PTR;
		auto OnDestroy() -> INT_PTR;
		auto OnInitDialog(const MSG& stMsg) -> INT_PTR;
		void OnOK();
		void UpdateComboMode();
	private:
		wnd::CWnd m_Wnd;
		wnd::CWndCombo m_WndCmbMode;
		IHexCtrl* m_pHexCtrl { };
		std::uint64_t m_u64Flags { }; //Data from SetDlgProperties.
		bool m_fRepeat { false };     //Is repeat available.
	};
}