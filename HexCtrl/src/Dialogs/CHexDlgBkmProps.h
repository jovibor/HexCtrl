/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../../HexCtrlDefs.h"
#include "../../res/HexCtrlRes.h"
#include <afxcontrolbars.h>

namespace HEXCTRL::INTERNAL
{
	class CHexDlgBkmProps final : public CDialogEx
	{
	public:
		explicit CHexDlgBkmProps(CWnd* pParent = nullptr) : CDialogEx(IDD_HEXCTRL_BKMPROPS, pParent) {}
		INT_PTR DoModal(HEXBKM& hbs, bool fShowAsHex);
	private:
		void DoDataExchange(CDataExchange* pDX)override;
		BOOL OnInitDialog()override;
		void OnOK()override;
		DECLARE_MESSAGE_MAP()
	private:
		PHEXBKM m_pHBS { };
		ULONGLONG m_ullOffset { };  //Current offset to compare on exit.
		ULONGLONG m_ullSize { };
		bool m_fShowAsHex { };
	};
}