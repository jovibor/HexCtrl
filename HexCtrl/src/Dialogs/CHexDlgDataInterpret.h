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
#include "../CHexCtrl.h"
#include "../../res/HexCtrlRes.h"
#include "CHexEdit.h"

namespace HEXCTRL::INTERNAL
{
	class CHexDlgDataInterpret final : public CDialogEx
	{
	public:
		explicit CHexDlgDataInterpret(CWnd* pParent = nullptr) : CDialogEx(IDD_HEXCTRL_DATAINTERPRET, pParent) {}
		~CHexDlgDataInterpret() = default;
		BOOL Create(UINT nIDTemplate, CHexCtrl* pHexCtrl);
		ULONGLONG GetSize();
		void InspectOffset(ULONGLONG ullOffset);
		BOOL ShowWindow(int nCmdShow);
	protected:
		BOOL OnCommand(WPARAM wParam, LPARAM lParam)override;
		void OnOK()override;
		afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		void DoDataExchange(CDataExchange* pDX)override;
		BOOL OnInitDialog()override;
		afx_msg void OnClose();
		BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)override;
		void UpdateHexCtrl();
		DECLARE_MESSAGE_MAP()
	private:
		CHexCtrl* m_pHexCtrl { };
		bool m_fVisible { false };
		CHexEdit m_edit8sign;
		CHexEdit m_edit8unsign;
		CHexEdit m_edit16sign;
		CHexEdit m_edit16unsign;
		CHexEdit m_edit32sign;
		CHexEdit m_edit32unsign;
		CHexEdit m_edit64sign;
		CHexEdit m_edit64unsign;
		CHexEdit m_editFloat;
		CHexEdit m_editDouble;
		CHexEdit m_editTime32;
		CHexEdit m_editTime64;
		std::vector<CHexEdit*> m_vec16 { };
		std::vector<CHexEdit*> m_vec32 { };
		std::vector<CHexEdit*> m_vec64 { };
		UINT_PTR m_dwCurrID { };
		ULONGLONG m_ullOffset { };
		ULONGLONG m_ullSize { };
	};
}