/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#pragma once
#include <afxdialogex.h>
#include <string>

namespace HEXCTRL::INTERNAL
{
	class CHexDlgCallback final : public CDialogEx
	{
	public:
		explicit CHexDlgCallback(std::wstring_view wstrOperName, CWnd* pParent = nullptr);
		[[nodiscard]] bool IsCanceled()const;
		void Cancel();
	protected:
		BOOL OnInitDialog()override;
		void DoDataExchange(CDataExchange* pDX)override;
		afx_msg void OnCancel()override;
		BOOL ContinueModal()override;
		DECLARE_MESSAGE_MAP()
	private:
		bool m_fCancel { false };
		std::wstring m_wstrOperName { };
	};
}