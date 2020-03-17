#pragma once
#include <afxcontrolbars.h>

namespace HEXCTRL::INTERNAL
{
	class CHexPropGridCtrl final : public CMFCPropertyGridCtrl
	{
	public:
		CMFCPropertyGridProperty* GetCurrentProp();
	private:
		void OnPropertyChanged(CMFCPropertyGridProperty* pProp) const;
		void OnChangeSelection(CMFCPropertyGridProperty* pNewSel, CMFCPropertyGridProperty* pOldSel);
	private:
		CMFCPropertyGridProperty* m_pNewSel { };
	};
	constexpr WPARAM HEXCTRL_PROPGRIDCTRL = 0xF001u;
	constexpr UINT HEXCTRL_PROPGRIDCTRL_SELCHANGED = 0xF002; //Code for NMHDR, when selection changed.
}