/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/						    *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                 *
* This is a Hex control for MFC apps, implemented as CWnd derived class.			    *
* The usage is quite simple:														    *
* 1. Construct CHexCtrl object — HEXCTRL::CHexCtrl myHex;								*
* 2. Call myHex.Create member function to create an instance.   					    *
* 3. Call myHex.SetData method to set the data and its size to display as hex.	        *
****************************************************************************************/
#pragma once
#include <afxcontrolbars.h>  //Standard MFC's controls header.
#include "../../res/HexCtrlRes.h"

namespace HEXCTRL {
	/********************************************
	* CHexDlgAbout class definition.			*
	********************************************/
	class CHexDlgAbout : public CDialogEx
	{
	public:
		explicit CHexDlgAbout(CWnd* m_pHexCtrl = nullptr) : CDialogEx(IDD_HEXCTRL_ABOUT) {}
		virtual ~CHexDlgAbout() {}
	protected:
		virtual BOOL OnInitDialog() override;
		DECLARE_MESSAGE_MAP()
	};
}