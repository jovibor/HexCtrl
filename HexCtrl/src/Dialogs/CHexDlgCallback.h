/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
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
	private:
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
		long long m_llTicks { 0LL }; //How many ticks have passed since dialog beginning.
		int m_iTicksInSecond { };    //How many ticks in one second.
	};
}