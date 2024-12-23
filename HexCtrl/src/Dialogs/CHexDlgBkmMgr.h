/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../../HexCtrl.h"
#include <vector>

import HEXCTRL.HexUtility;

namespace HEXCTRL::INTERNAL {
	class CHexDlgBkmMgr final : public IHexBookmarks {
	public:
		auto AddBkm(const HEXBKM& hbs, bool fRedraw) -> ULONGLONG override;    //Returns new bookmark Id.
		void CreateDlg();
		[[nodiscard]] auto GetHWND()const->HWND;
		[[nodiscard]] auto GetByID(ULONGLONG ullID) -> PHEXBKM override;       //Bookmark by ID.
		[[nodiscard]] auto GetByIndex(ULONGLONG ullIndex) -> PHEXBKM override; //Bookmark by index (in inner list).
		[[nodiscard]] auto GetCount() -> ULONGLONG override;
		[[nodiscard]] auto GetCurrent()const->ULONGLONG;
		[[nodiscard]] auto GetDlgItemHandle(EHexDlgItem eItem)const->HWND;
		void GoBookmark(ULONGLONG ullIndex);
		void GoNext();
		void GoPrev();
		[[nodiscard]] bool HasBookmarks()const;
		[[nodiscard]] auto HitTest(ULONGLONG ullOffset) -> PHEXBKM override;
		void Initialize(IHexCtrl* pHexCtrl);
		[[nodiscard]] bool IsVirtual()const;
		[[nodiscard]] bool PreTranslateMsg(MSG* pMsg);
		[[nodiscard]] auto ProcessMsg(const MSG& stMsg) -> INT_PTR;
		void RemoveAll()override;
		void RemoveByOffset(ULONGLONG ullOffset);
		void RemoveByID(ULONGLONG ullID)override;
		void SetDlgProperties(std::uint64_t u64Flags);
		void SetVirtual(IHexBookmarks* pVirtBkm);
		void ShowWindow(int iCmdShow);
		void SortData(int iColumn, bool fAscending);
		void Update(ULONGLONG ullID, const HEXBKM& bkm);
	private:
		enum class EMenuID : std::uint16_t;
		[[nodiscard]] auto GetHexCtrl()const->IHexCtrl*;
		[[nodiscard]] bool IsNoEsc()const;
		[[nodiscard]] bool IsShowAsHex()const;
		void OnCancel();
		auto OnClose() -> INT_PTR;
		auto OnCommand(const MSG& stMsg) -> INT_PTR;
		void OnCheckHex();
		auto OnDestroy() -> INT_PTR;
		auto OnDrawItem(const MSG& stMsg) -> INT_PTR;
		void OnOK();
		auto OnInitDialog(const MSG& stMsg) -> INT_PTR;
		auto OnMeasureItem(const MSG& stMsg) -> INT_PTR;
		auto OnNotify(const MSG& stMsg) -> INT_PTR;
		void OnNotifyListGetDispInfo(NMHDR* pNMHDR);
		void OnNotifyListItemChanged(NMHDR* pNMHDR);
		void OnNotifyListDblClick(NMHDR* pNMHDR);
		void OnNotifyListRClick(NMHDR* pNMHDR);
		void OnNotifyListGetColor(NMHDR* pNMHDR);
		void OnNotifyListSetData(NMHDR* pNMHDR);
		auto OnSize(const MSG& stMsg) -> INT_PTR;
		void RemoveBookmark(std::uint64_t ullID);
		void SortBookmarks();
		void UpdateListCount(bool fPreserveSelected = false);
	private:
		wnd::CWnd m_Wnd;              //Main window.
		wnd::CWndBtn m_WndBtnHex;     //Check-box "Hex numbers".
		wnd::CMenu m_menuList;
		wnd::CDynLayout m_DynLayout;
		std::vector<HEXBKM> m_vecBookmarks; //Bookmarks data.
		IHexCtrl* m_pHexCtrl { };
		IHexBookmarks* m_pVirtual { };
		LISTEX::IListExPtr m_pList { LISTEX::CreateListEx() };
		LONGLONG m_llIndexCurr { };   //Current bookmark's position index, to move next/prev.
		std::uint64_t m_u64Flags { }; //Data from SetDlgProperties.
	};
}