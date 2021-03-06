/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
****************************************************************************************/
#pragma once
#include "../../HexCtrl.h"
#include <afxdialogex.h>

namespace HEXCTRL::INTERNAL
{
	class CHexDlgGoTo final : public CDialogEx
	{
	public:
		BOOL Create(UINT nIDTemplate, CWnd* pParent, IHexCtrl* pHexCtrl);
		[[nodiscard]] bool IsRepeatAvail()const;
		void Repeat(bool fFwd = true); //fFwd: true - forward, false - backward.
	protected:
		void DoDataExchange(CDataExchange* pDX)override;
		BOOL OnInitDialog()override;
		afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		void OnOK()override;
		afx_msg void OnClickRadioAbs();
		afx_msg void OnClickRadioFwdCurr();
		afx_msg void OnClickRadioBackCurr();
		afx_msg void OnClickRadioBackEnd();
		DECLARE_MESSAGE_MAP()
	private:
		[[nodiscard]] IHexCtrl* GetHexCtrl()const;
		void HexCtrlGoOffset(ULONGLONG ullOffset);
		void SetRangesText()const;
	private:
		IHexCtrl* m_pHexCtrl { };
		ULONGLONG m_ullData { };
		ULONGLONG m_ullCurrOffset { }; //Offset that was set when OnGo was called last time.
		ULONGLONG m_ullOffsetsFrom { };
		ULONGLONG m_ullOffsetsTo { };
		ULONGLONG m_ullPagesFrom { };
		ULONGLONG m_ullPagesTo { };
		int m_iRepeat { 0 };
	};
}