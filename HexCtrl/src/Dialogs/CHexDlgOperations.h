/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#pragma once
#include "../../res/HexCtrlRes.h"
#include "../CHexCtrl.h"
#include "CHexEdit.h"
#include <afxdialogex.h>  //Standard MFC's controls header.

namespace HEXCTRL::INTERNAL
{
	class CHexDlgOperations final : public CDialogEx
	{
	public:
		explicit CHexDlgOperations(CWnd* pParent = nullptr) : CDialogEx(IDD_HEXCTRL_OPERATIONS, pParent) {}
		BOOL Create(UINT nIDTemplate, CHexCtrl* pHexCtrl);
	protected:
		void DoDataExchange(CDataExchange* pDX)override;
		BOOL OnInitDialog()override;
		BOOL OnCommand(WPARAM wParam, LPARAM lParam)override;
		BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)override;
		void OnOK()override;
		DECLARE_MESSAGE_MAP()
	private:
		[[nodiscard]] CHexCtrl* GetHexCtrl()const;
	private:
		CHexCtrl* m_pHexCtrl { };
		CHexEdit m_editOR;
		CHexEdit m_editXOR;
		CHexEdit m_editAND;
		CHexEdit m_editSHL;
		CHexEdit m_editSHR;
		CHexEdit m_editAdd;
		CHexEdit m_editSub;
		CHexEdit m_editMul;
		CHexEdit m_editDiv;
	};
}