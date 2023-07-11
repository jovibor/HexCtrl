/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../../HexCtrl.h"
#include <afxdialogex.h>
#include <unordered_map>

namespace HEXCTRL::INTERNAL
{
	class CHexDlgOpers final : public CDialogEx {
	public:
		void Create(CWnd* pParent, IHexCtrl* pHexCtrl);
		void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	private:
		void DoDataExchange(CDataExchange* pDX)override;
		BOOL OnInitDialog()override;
		void OnCancel()override;
		BOOL OnCommand(WPARAM wParam, LPARAM lParam)override;
		void OnOK()override;
		void CheckWndAvail()const;
		[[nodiscard]] auto GetOperMode()const->EHexOperMode;
		[[nodiscard]] auto GetDataSize()const->EHexDataSize;
		void SetOKButtonName();
		DECLARE_MESSAGE_MAP();
	private:
		IHexCtrl* m_pHexCtrl { };
		CComboBox m_stComboOper; //Operation combo-box.
		CComboBox m_stComboSize; //Data size combo-box.
		using enum EHexOperMode;
		inline static const std::unordered_map<EHexOperMode, std::wstring_view> m_mapNames {
			{ OPER_ASSIGN, L"Assign" }, { OPER_ADD, L"Add" }, { OPER_SUB, L"Subtract" },
			{ OPER_MUL, L"Multiply" }, { OPER_DIV, L"Divide" }, { OPER_CEIL, L"Ceiling" },
			{ OPER_FLOOR, L"Floor" }, { OPER_OR, L"OR" }, { OPER_XOR, L"XOR" }, { OPER_AND, L"AND" },
			{ OPER_NOT, L"NOT" }, { OPER_SHL, L"SHL" }, { OPER_SHR, L"SHR" }, { OPER_ROTL, L"ROTL" },
			{ OPER_ROTR, L"ROTR" }, { OPER_SWAP, L"Swap Bytes" }, { OPER_BITREV, L"Reverse Bits" }
		};
	};
}