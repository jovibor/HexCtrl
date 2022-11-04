/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "CHexPropGridCtrl.h"

using namespace HEXCTRL::INTERNAL;

void CHexPropGridCtrl::OnChangeSelection(CMFCPropertyGridProperty* pNewProp, CMFCPropertyGridProperty* /*pOldProp*/)
{
	GetParent()->SendMessageW(WM_PROPGRID_PROPERTY_SELECTED, GetDlgCtrlID(), reinterpret_cast<LPARAM>(pNewProp));
}