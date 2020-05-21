/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This file contributed by Data Synergy UK. No rights reserved                          *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#pragma once
#include "../../res/HexCtrlRes.h"
#include <afxcontrolbars.h>  //Standard MFC's controls header.

namespace HEXCTRL::INTERNAL
{
	/********************************************
	* CHexDlgCodePage class declaration.			*
	********************************************/
	class CHexDlgCodePage final : public CDialogEx
	{
	public:
		explicit CHexDlgCodePage(CWnd* pParent) : CDialogEx(IDD_HEXCTRL_CODEPAGE, pParent) {}
	protected:
		BOOL OnInitDialog()override;
		void OnCbnSelChange();
		DECLARE_MESSAGE_MAP()
	public:
		void AddCodePage(LPWSTR lpCodePageString);
		int GetSelectedCodepage();
	public:
		inline static CHexDlgCodePage* m_pHexDlgCodepage = nullptr;
	private:
		int m_nCodepage = -1;
	};
}