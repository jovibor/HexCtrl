/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include <afxcontrolbars.h>

namespace HEXCTRL::INTERNAL
{
	class CHexPropGridCtrl final : public CMFCPropertyGridCtrl
	{
	private:
		void OnChangeSelection(CMFCPropertyGridProperty* pNewProp, CMFCPropertyGridProperty* pOldProp)override;
	};
	constexpr auto WM_PROPGRID_PROPERTY_SELECTED = 0x0401U; //Message to parent when new property selected.
}