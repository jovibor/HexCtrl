/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
****************************************************************************************/
#pragma once
#include <afxdialogex.h>
#include <string>

namespace HEXCTRL::INTERNAL
{
	class CHexDlgCallback final : public CDialogEx
	{
	public:
		explicit CHexDlgCallback(std::wstring_view wstrOperName,
			ULONGLONG ullProgBarMin, ULONGLONG ullProgBarMax, CWnd* pParent = nullptr);
		[[nodiscard]] bool IsCanceled()const;
		void SetProgress(ULONGLONG ullProgCurr);
		void ExitDlg();
	protected:
		BOOL OnInitDialog()override;
		void DoDataExchange(CDataExchange* pDX)override;
		void OnBtnCancel();
		afx_msg void OnTimer(UINT_PTR nIDEvent);
		DECLARE_MESSAGE_MAP()
	private:
		CProgressCtrl m_stProgBar;
		ULONGLONG m_ullProgBarMin { };
		ULONGLONG m_ullProgBarMax { };
		ULONGLONG m_ullProgBarCurr { };
		ULONGLONG m_ullThousandth { };   //One thousandth part.
		bool m_fCancel { false };
		std::wstring m_wstrOperName { };
		const UINT_PTR IDT_EXITCHECK { 0x1 };
	};
}