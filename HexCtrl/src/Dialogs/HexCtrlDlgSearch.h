/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/						    *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* This is a Hex control for MFC apps, implemented as CWnd derived class.			    *
* The usage is quite simple:														    *
* 1. Construct CHexCtrl object — HEXCTRL::CHexCtrl myHex;								*
* 2. Call myHex.Create member function to create an instance.   					    *
* 3. Call myHex.SetData method to set the data and its size to display as hex.	        *
****************************************************************************************/
#pragma once
#include <afxcontrolbars.h>  //Standard MFC's controls header.
#include <string>
#include "../../IHexCtrl.h"
#include "../../res/HexCtrlRes.h"

namespace HEXCTRL {
	/********************************************
	* CHexDlgSearch class definition.			*
	********************************************/
	class CHexDlgSearch : public CDialogEx
	{
	public:
		explicit CHexDlgSearch(CWnd* m_pHexCtrl = nullptr) {}
		virtual ~CHexDlgSearch() {}
		BOOL Create(UINT nIDTemplate, IHexCtrl* pHexCtrl);
protected:
		virtual void DoDataExchange(CDataExchange* pDX);
		virtual BOOL OnInitDialog();
		afx_msg void OnButtonSearchF();
		afx_msg void OnButtonSearchB();
		afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		afx_msg void OnClose();
		virtual BOOL PreTranslateMessage(MSG* pMsg);
		HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
		void OnRadioBnRange(UINT nID);
		void SearchCallback();
		void ClearAll();
		IHexCtrl* GetHexCtrl();
		DECLARE_MESSAGE_MAP()
	private:
		IHexCtrl* m_pHexCtrl { };
		HEXSEARCHSTRUCT m_stSearch { };
		DWORD m_dwOccurrences { };
		int m_iRadioCurrent { };
		COLORREF m_clrSearchFailed { RGB(200, 0, 0) };
		COLORREF m_clrSearchFound { RGB(0, 200, 0) };
		CBrush m_stBrushDefault;
		COLORREF m_clrMenu { GetSysColor(COLOR_MENU) };
	};
}