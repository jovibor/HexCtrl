/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#pragma once
#include "../CHexCtrl.h"
#include <afxdialogex.h>

namespace HEXCTRL::INTERNAL
{
	class CHexDlgGoTo final : public CDialogEx
	{
	public:
		BOOL Create(UINT nIDTemplate, CHexCtrl* pHexCtrl);
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
		[[nodiscard]] CHexCtrl* GetHexCtrl()const;
		void HexCtrlGoOffset(ULONGLONG ullOffset);
		void SetRangesText()const;
	private:
		CHexCtrl* m_pHexCtrl { };
		ULONGLONG m_ullData { };
		ULONGLONG m_ullCurrOffset { }; //Offset that was set when OnGo was called last time.
		ULONGLONG m_ullOffsetsFrom { };
		ULONGLONG m_ullOffsetsTo { };
		ULONGLONG m_ullPagesFrom { };
		ULONGLONG m_ullPagesTo { };
		int m_iRepeat { 0 };
	};
}