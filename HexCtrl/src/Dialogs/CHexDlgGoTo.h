/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../../HexCtrl.h"
#include <afxdialogex.h>

namespace HEXCTRL::INTERNAL
{
	class CHexDlgGoTo final : public CDialogEx {
	public:
		[[nodiscard]] auto GetDlgData()const->std::uint64_t;
		void Initialize(IHexCtrl* pHexCtrl);
		[[nodiscard]] bool IsRepeatAvail()const;
		void Repeat(bool fFwd = true); //fFwd: true - forward, false - backward.
		auto SetDlgData(std::uint64_t ullData) -> HWND;
		BOOL ShowWindow(int nCmdShow);
	private:
		void DoDataExchange(CDataExchange* pDX)override;
		[[nodiscard]] auto GetHexCtrl()const->IHexCtrl*;
		void HexCtrlGoOffset(ULONGLONG ullOffset);
		afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		BOOL OnInitDialog()override;
		void OnOK()override;
		afx_msg void OnRadioRangeAddrType(UINT uID);
		void SetRangesText()const;
		DECLARE_MESSAGE_MAP();
	private:
		IHexCtrl* m_pHexCtrl { };
		ULONGLONG m_ullData { };
		ULONGLONG m_ullCurrOffset { }; //Offset that was set when OnOK was called last time.
		ULONGLONG m_ullOffsetsFrom { };
		ULONGLONG m_ullOffsetsTo { };
		ULONGLONG m_ullPagesFrom { };
		ULONGLONG m_ullPagesTo { };
		int m_iRepeat { 0 };
	};
}