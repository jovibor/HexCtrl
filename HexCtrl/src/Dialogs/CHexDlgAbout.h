/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../../res/HexCtrlRes.h"
#include <afxcontrolbars.h>

namespace HEXCTRL::INTERNAL
{
	class CHexDlgAbout final : public CDialogEx
	{
	public:
		explicit CHexDlgAbout(CWnd* pParent) : CDialogEx(IDD_HEXCTRL_ABOUT, pParent) {}
	protected:
		BOOL OnInitDialog()override;
		DECLARE_MESSAGE_MAP()
	};
}