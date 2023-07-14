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
	class CHexDlgFillData final : public CDialogEx {
	public:
		void Create(CWnd* pParent, IHexCtrl* pHexCtrl);
		void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	private:
		enum class EFillType : std::uint8_t; //Forward declaration.
		void DoDataExchange(CDataExchange* pDX)override;
		[[nodiscard]] auto GetFillType()const->EFillType;
		void OnCancel()override;
		BOOL OnCommand(WPARAM wParam, LPARAM lParam)override;
		BOOL OnInitDialog()override;
		void OnOK()override;
		DECLARE_MESSAGE_MAP();
	private:
		IHexCtrl* m_pHexCtrl { };
		CComboBox m_stComboType; //Fill type combo-box.
		CComboBox m_stComboData; //Data combo-box.
	};
}