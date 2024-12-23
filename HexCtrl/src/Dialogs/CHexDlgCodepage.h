/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../../HexCtrl.h"

import HEXCTRL.HexUtility;

namespace HEXCTRL::INTERNAL {
	class CHexDlgCodepage final {
	public:
		void AddCP(std::wstring_view wsv);
		void CreateDlg();
		[[nodiscard]] auto GetHWND()const->HWND;
		void Initialize(IHexCtrl* pHexCtrl);
		[[nodiscard]] bool PreTranslateMsg(MSG* pMsg);
		[[nodiscard]] auto ProcessMsg(const MSG& stMsg) -> INT_PTR;
		void SetDlgProperties(std::uint64_t u64Flags);
		void ShowWindow(int iCmdShow);
	private:
		[[nodiscard ]] bool IsNoEsc()const;
		auto OnActivate(const MSG& stMsg) -> INT_PTR;
		void OnCancel();
		auto OnClose() -> INT_PTR;
		auto OnCommand(const MSG& stMsg) -> INT_PTR;
		auto OnDestroy() -> INT_PTR;
		auto OnDrawItem(const MSG& stMsg) -> INT_PTR;
		auto OnInitDialog(const MSG& stMsg) -> INT_PTR;
		auto OnMeasureItem(const MSG& stMsg) -> INT_PTR;
		auto OnNotify(const MSG& stMsg) -> INT_PTR;
		void OnNotifyListGetDispInfo(NMHDR* pNMHDR);
		void OnNotifyListItemChanged(NMHDR* pNMHDR);
		void OnNotifyListGetColor(NMHDR* pNMHDR);
		void OnNotifyListLinkClick(NMHDR* pNMHDR);
		auto OnSize(const MSG& stMsg) -> INT_PTR;
		void SortList();
		static BOOL CALLBACK EnumCodePagesProc(LPWSTR pwszCP);
	private:
		struct CODEPAGE {
			int iCPID { };
			std::wstring wstrName;
			UINT uMaxChars { };
		};
		inline static CHexDlgCodepage* m_pThis { };
		wnd::CWnd m_Wnd;              //Main window.
		wnd::CDynLayout m_DynLayout;
		IHexCtrl* m_pHexCtrl { };
		LISTEX::IListExPtr m_pList { LISTEX::CreateListEx() };
		std::vector<CODEPAGE> m_vecCodePage;
		std::uint64_t m_u64Flags { }; //Data from SetDlgProperties.
	};
}