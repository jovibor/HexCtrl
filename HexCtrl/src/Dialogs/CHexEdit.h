/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#pragma once
#include <afxcontrolbars.h>  //Standard MFC's controls header.

namespace HEXCTRL::INTERNAL
{
	class CHexEdit final : public CEdit
	{
	public:
		explicit CHexEdit() = default;
		~CHexEdit() = default;
	protected:
		afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg UINT OnGetDlgCode();
		DECLARE_MESSAGE_MAP()
	};
	constexpr WPARAM HEXCTRL_EDITCTRL = 0xFFFFu;
}