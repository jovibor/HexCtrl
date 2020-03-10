/****************************************************************************************
* Copyright � 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#pragma once
#include <afxcontrolbars.h>  //Standard MFC's controls header.
#include "../CHexCtrl.h"
#include "../../res/HexCtrlRes.h"

namespace HEXCTRL::INTERNAL
{
	class CHexDlgFillWith final : public CDialogEx
	{
	public:
		explicit CHexDlgFillWith(CWnd* pParent = nullptr) : CDialogEx(IDD_HEXCTRL_FILLWITHDATA, pParent) {}
		BOOL Create(UINT nIDTemplate, CHexCtrl* pHexCtrl);
	protected:
		void DoDataExchange(CDataExchange* pDX)override;
		BOOL OnInitDialog()override;
		void OnOK()override;
		DECLARE_MESSAGE_MAP()
	private:
		[[nodiscard]] CHexCtrl* GetHexCtrl()const;
	private:
		CHexCtrl* m_pHexCtrl { };
	};
}