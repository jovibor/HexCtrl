/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include <afxdialogex.h>
#include <string>

namespace HEXCTRL::INTERNAL {
	class CHexDlgCallback final : public CDialogEx {
	public:
		explicit CHexDlgCallback(std::wstring_view wsvOperName, std::wstring_view wsvCountName,
			ULONGLONG ullMin, ULONGLONG ullMax, CWnd* pParent = nullptr);
		[[nodiscard]] bool IsCanceled()const;
		void OnCancel()override;
		void SetCount(ULONGLONG ullCount);
		void SetCurrent(ULONGLONG ullCurr); //Sets current data in the whole data diapason.
	private:
		void DoDataExchange(CDataExchange* pDX)override;
		afx_msg void OnClose();
		afx_msg auto OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) -> HBRUSH;
		BOOL OnInitDialog()override;
		afx_msg void OnTimer(UINT_PTR nIDEvent);
		DECLARE_MESSAGE_MAP();
	private:
		static constexpr UINT_PTR m_uIDTCancelCheck { 0x1 };
		static constexpr auto m_uElapse { 100U }; //Milliseconds for the timer.
		CProgressCtrl m_stProgBar;
		std::wstring m_wstrOperName;
		std::wstring m_wstrCountName; //Count name (e.g. Found, Replaced, etc...).
		ULONGLONG m_ullMin { };       //Minimum data amount. 
		ULONGLONG m_ullPrev { };      //Previous data in bytes.
		ULONGLONG m_ullCurr { };      //Current data amount processed.
		ULONGLONG m_ullMax { };       //Max data amount.
		ULONGLONG m_ullThousands { }; //How many thousands in the whole data diapason.
		ULONGLONG m_ullCount { };     //Count of found/replaced items.
		bool m_fCancel { false };     //"Cancel" button pressed.
	};
}