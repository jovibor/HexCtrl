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
#include "../res/HexCtrlRes.h"

namespace HEXCTRL {
	/********************************************
	* CHexDlgAbout class definition.			*
	********************************************/
	class CHexDlgAbout : public CDialogEx
	{
	public:
		explicit CHexDlgAbout(CWnd* m_pParent = nullptr) : CDialogEx(IDD_HEXCTRL_ABOUT) {}
		virtual ~CHexDlgAbout() {}
	protected:
		virtual BOOL OnInitDialog() override;
		afx_msg void OnMouseMove(UINT nFlags, CPoint point);
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
		HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
		DECLARE_MESSAGE_MAP()
	private:
		bool m_fGithubLink { true };
		HCURSOR m_curHand { };
		HCURSOR m_curArrow { };
		CFont m_fontDefault;
		CFont m_fontUnderline;
		CBrush m_stBrushDefault;
		COLORREF m_clrMenu { GetSysColor(COLOR_MENU) };
	};
}