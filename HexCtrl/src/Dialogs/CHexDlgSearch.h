/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/                         *
* This is a Hex Control for MFC applications.                                           *
* Official git repository of the project: https://github.com/jovibor/HexCtrl/           *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#pragma once
#include <afxcontrolbars.h>  //Standard MFC's controls header.
#include "../CHexCtrl.h"
#include "../../res/HexCtrlRes.h"

namespace HEXCTRL {
	namespace INTERNAL {
		/********************************************
		* CHexDlgSearch class definition.			*
		********************************************/
		class CHexDlgSearch : public CDialogEx
		{
		public:
			explicit CHexDlgSearch() : CDialogEx(IDD_HEXCTRL_SEARCH) {}
			virtual ~CHexDlgSearch() {}
			BOOL Create(UINT nIDTemplate, CHexCtrl* pHexCtrl);
		protected:
			virtual void DoDataExchange(CDataExchange* pDX);
			virtual BOOL OnInitDialog();
			afx_msg void OnButtonSearchF();
			afx_msg void OnButtonSearchB();
			afx_msg void OnButtonReplace();
			afx_msg void OnButtonReplaceAll();
			afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
			afx_msg void OnCancel();
			virtual BOOL PreTranslateMessage(MSG* pMsg);
			HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
			void OnRadioBnRange(UINT nID);
			void SearchCallback();
			void ClearAll();
			CHexCtrl* GetHexCtrl()const;
			ESearchType GetSearchType();                //Returns current search type.
			DECLARE_MESSAGE_MAP()
		private:
			CHexCtrl* m_pHexCtrl { };
			SEARCHSTRUCT m_stSearch { };
			int m_iRadioCurrent { };
			const COLORREF m_clrSearchFailed { RGB(200, 0, 0) };
			const COLORREF m_clrSearchFound { RGB(0, 200, 0) };
			const COLORREF m_clrBkTextArea { GetSysColor(COLOR_MENU) };
			CBrush m_stBrushDefault;
		};
	}
}