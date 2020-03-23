#pragma once
#include <afxcontrolbars.h>

namespace HEXCTRL::INTERNAL
{
	class CHexPropGridCtrl final : public CMFCPropertyGridCtrl
	{
	public:
		CMFCPropertyGridProperty* GetCurrentProp();
	private:
		void OnPropertyChanged(CMFCPropertyGridProperty* pProp) const override;
		void OnChangeSelection(CMFCPropertyGridProperty* pNewSel, CMFCPropertyGridProperty* pOldSel)override;
	private:
		CMFCPropertyGridProperty* m_pNewSel { };
	};
	constexpr WPARAM HEXCTRL_PROPGRIDCTRL = 0xF001U;
	constexpr UINT HEXCTRL_PROPGRIDCTRL_SELCHANGED = 0xF002U; //Code for NMHDR, when selection changed.
}