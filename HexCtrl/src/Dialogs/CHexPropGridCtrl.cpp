/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#include "stdafx.h"
#include "CHexPropGridCtrl.h"

using namespace HEXCTRL::INTERNAL;

CMFCPropertyGridProperty* CHexPropGridCtrl::GetCurrentProp()
{
	return m_pNewSel;
}

void CHexPropGridCtrl::OnPropertyChanged(CMFCPropertyGridProperty* pProp) const
{
	return CMFCPropertyGridCtrl::OnPropertyChanged(pProp);
}

void CHexPropGridCtrl::OnChangeSelection(CMFCPropertyGridProperty* pNewSel, CMFCPropertyGridProperty* /*pOldSel*/)
{
	m_pNewSel = pNewSel;
	NMHDR nmhdr { m_hWnd, static_cast<UINT>(GetDlgCtrlID()), HEXCTRL_PROPGRIDCTRL_SELCHANGED };
	GetParent()->SendMessageW(WM_NOTIFY, HEXCTRL_PROPGRIDCTRL, reinterpret_cast<LPARAM>(&nmhdr));	
}