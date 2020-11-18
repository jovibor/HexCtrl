/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
****************************************************************************************/
#pragma once
#include <afxcontrolbars.h>

namespace HEXCTRL::INTERNAL
{
	class CHexPropGridCtrl final : public CMFCPropertyGridCtrl
	{
	public:
		CMFCPropertyGridProperty* GetCurrentProp();
	private:
		void OnPropertyChanged(CMFCPropertyGridProperty* pProp)const override;
		void OnChangeSelection(CMFCPropertyGridProperty* pNewSel, CMFCPropertyGridProperty* pOldSel)override;
	private:
		CMFCPropertyGridProperty* m_pNewSel { };
	};
	constexpr UINT MSG_PROPGRIDCTRL_SELCHANGED = 0xF002U; //Code for NMHDR, when selection changed.
}