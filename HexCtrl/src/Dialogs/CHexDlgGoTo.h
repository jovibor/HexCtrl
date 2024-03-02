/****************************************************************************************
* Copyright © 2018-2024 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../../HexCtrl.h"
#include <afxdialogex.h>

namespace HEXCTRL::INTERNAL {
	class CHexDlgGoTo final : public CDialogEx {
	public:
		[[nodiscard]] auto GetDlgData()const->std::uint64_t;
		void Initialize(IHexCtrl* pHexCtrl);
		[[nodiscard]] bool IsRepeatAvail()const;
		void Repeat(bool fFwd = true); //fFwd: true - forward, false - backward.
		auto SetDlgData(std::uint64_t ullData, bool fCreate) -> HWND;
		BOOL ShowWindow(int nCmdShow);
	private:
		enum class EGoMode : std::uint8_t;
		void ApplyDlgData();
		void DoDataExchange(CDataExchange* pDX)override;
		[[nodiscard]] auto GetHexCtrl()const->IHexCtrl*;
		[[nodiscard]] auto GetGoMode()const->EGoMode;
		void GoTo(bool fForward);
		[[nodiscard ]] bool IsNoEsc()const;
		afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		void OnCancel()override;
		afx_msg void OnClose();
		afx_msg void OnDestroy();
		BOOL OnInitDialog()override;
		void OnOK()override;
		void UpdateComboMode();
		DECLARE_MESSAGE_MAP();
	private:
		IHexCtrl* m_pHexCtrl { };
		std::uint64_t m_u64DlgData { }; //Data from SetDlgData.
		CComboBox m_comboMode;
		bool m_fRepeat { false }; //Is repeat available.
	};
}