/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../../HexCtrl.h"
#include <afxdialogex.h>

namespace HEXCTRL::INTERNAL
{
	class CHexDlgOpers final : public CDialogEx
	{
	public:
		BOOL Create(UINT nIDTemplate, CWnd* pParent, IHexCtrl* pHexCtrl);
	private:
		void DoDataExchange(CDataExchange* pDX)override;
		BOOL OnInitDialog()override;
		afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		BOOL OnCommand(WPARAM wParam, LPARAM lParam)override;
		BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)override;
		void OnOK()override;
		[[nodiscard]] EHexOperMode GetOperMode()const;
		void CheckWndAvail()const;
		DECLARE_MESSAGE_MAP()
	private:
		IHexCtrl* m_pHexCtrl { };
		CComboBox m_stComboOper;  //Data operation combo-box.
	};
}