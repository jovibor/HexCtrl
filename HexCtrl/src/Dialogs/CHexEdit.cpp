/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#include "stdafx.h"
#include "CHexEdit.h"

using namespace HEXCTRL::INTERNAL;

BEGIN_MESSAGE_MAP(CHexEdit, CEdit)
	ON_WM_KEYDOWN()
	ON_WM_GETDLGCODE()
END_MESSAGE_MAP()

void CHexEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	UINT uCode { };
	switch (nChar)
	{
	case VK_RETURN:
		uCode = VK_RETURN;
		break;
	case VK_TAB:
		GetParent()->PostMessageW(WM_NEXTDLGCTL);
		uCode = VK_TAB;
		break;
	case VK_UP:
		uCode = VK_UP;
		GetParent()->PostMessageW(WM_NEXTDLGCTL, TRUE);
		break;
	case VK_DOWN:
		GetParent()->PostMessageW(WM_NEXTDLGCTL);
		uCode = VK_DOWN;
		break;
	case VK_ESCAPE:
		uCode = VK_ESCAPE;
		break;
	}
	if (uCode)
	{
		NMHDR nmhdr { m_hWnd, (UINT)GetDlgCtrlID(), uCode };
		GetParent()->SendMessageW(WM_NOTIFY, HEXCTRL_EDITCTRL, (LPARAM)&nmhdr);
	}

	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}

UINT CHexEdit::OnGetDlgCode()
{
	return DLGC_WANTALLKEYS;
}