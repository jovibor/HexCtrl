/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../../HexCtrl.h"
#include <commctrl.h>

import HEXCTRL.HexUtility;

namespace HEXCTRL::INTERNAL {
	class CHexDlgCodepage final {
	public:
		void AddCP(std::wstring_view wsv);
		void CreateDlg();
		void DestroyDlg();
		[[nodiscard]] auto GetHWND()const->HWND;
		void Initialize(IHexCtrl* pHexCtrl, HINSTANCE hInstRes);
		[[nodiscard]] bool PreTranslateMsg(MSG* pMsg);
		[[nodiscard]] auto ProcessMsg(const MSG& msg) -> INT_PTR;
		void SetDlgProperties(std::uint64_t u64Flags);
		void ShowWindow(int iCmdShow);
	private:
		[[nodiscard ]] bool IsNoEsc()const;
		auto OnActivate(const MSG& msg) -> INT_PTR;
		void OnCancel();
		auto OnClose() -> INT_PTR;
		auto OnCommand(const MSG& msg) -> INT_PTR;
		auto OnDestroy() -> INT_PTR;
		auto OnDrawItem(const MSG& msg) -> INT_PTR;
		auto OnInitDialog(const MSG& msg) -> INT_PTR;
		auto OnMeasureItem(const MSG& msg) -> INT_PTR;
		auto OnNotify(const MSG& msg) -> INT_PTR;
		void OnNotifyListGetColor(NMHDR* pNMHDR);
		void OnNotifyListGetDispInfo(NMHDR* pNMHDR);
		void OnNotifyListItemChanged(NMHDR* pNMHDR);
		void OnNotifyListLinkClick(NMHDR* pNMHDR);
		auto OnSize(const MSG& msg) -> INT_PTR;
		void SortList();
		static BOOL CALLBACK EnumCodePagesProc(LPWSTR pwszCP);
	private:
		struct CODEPAGE {
			int iCPID { };
			std::wstring wstrName;
			UINT uMaxChars { };
		};
		inline static CHexDlgCodepage* m_pThis { };
		HINSTANCE m_hInstRes { };
		wnd::CWnd m_Wnd;              //Main window.
		wnd::CDynLayout m_DynLayout;
		IHexCtrl* m_pHexCtrl { };
		LISTEX::CListEx m_ListEx;
		std::vector<CODEPAGE> m_vecCodePage;
		std::uint64_t m_u64Flags { }; //Data from SetDlgProperties.
	};
}