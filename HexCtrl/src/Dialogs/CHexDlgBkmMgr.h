/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../../dep/ListEx/ListEx.h"
#include "../../HexCtrl.h"
#include <afxdialogex.h>
#include <vector>

namespace HEXCTRL::INTERNAL
{
	class CHexDlgBkmMgr final : public CDialogEx, public IHexBookmarks {
	public:
		auto AddBkm(const HEXBKM& hbs, bool fRedraw) -> ULONGLONG override; //Returns new bookmark Id.
		[[nodiscard]] auto GetByID(ULONGLONG ullID) -> PHEXBKM override;       //Bookmark by ID.
		[[nodiscard]] auto GetByIndex(ULONGLONG ullIndex) -> PHEXBKM override; //Bookmark by index (in inner list).
		[[nodiscard]] auto GetCount() -> ULONGLONG override;
		[[nodiscard]] auto GetCurrent()const->ULONGLONG;
		[[nodiscard]] auto GetDlgData()const->std::uint64_t;
		void GoBookmark(ULONGLONG ullIndex);
		void GoNext();
		void GoPrev();
		[[nodiscard]] bool HasBookmarks()const;
		[[nodiscard]] auto HitTest(ULONGLONG ullOffset) -> PHEXBKM override;
		void Initialize(IHexCtrl* pHexCtrl);
		[[nodiscard]] bool IsVirtual()const;
		void RemoveAll()override;
		void RemoveByOffset(ULONGLONG ullOffset);
		void RemoveByID(ULONGLONG ullID)override;
		auto SetDlgData(std::uint64_t ullData) -> HWND;
		void SetVirtual(IHexBookmarks* pVirtBkm);
		BOOL ShowWindow(int nCmdShow);
		void SortData(int iColumn, bool fAscending);
		void Update(ULONGLONG ullID, const HEXBKM& bkm);
	private:
		enum class EMenuID : std::uint16_t;
		void DoDataExchange(CDataExchange* pDX)override;
		[[nodiscard]] bool IsShowAsHex()const;
		BOOL OnCommand(WPARAM wParam, LPARAM lParam)override;
		afx_msg void OnCheckHex();
		afx_msg void OnDestroy();
		BOOL OnInitDialog()override;
		afx_msg void OnListGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnListItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnListDblClick(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnListRClick(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnListGetColor(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnListSetData(NMHDR* pNMHDR, LRESULT* pResult);
		BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)override;
		void OnOK()override;
		void RemoveBookmark(std::uint64_t ullID);
		void SortBookmarks();
		void UpdateListCount(bool fPreserveSelected = false);
		DECLARE_MESSAGE_MAP();
	private:
		std::vector<HEXBKM> m_vecBookmarks; //Bookmarks data.
		IHexCtrl* m_pHexCtrl { };
		IHexBookmarks* m_pVirtual { };
		LISTEX::IListExPtr m_pList { LISTEX::CreateListEx() };
		LONGLONG m_llIndexCurr { }; //Current bookmark's position index, to move next/prev.
		CMenu m_stMenuList;
		CButton m_btnHex;           //Check-box "Hex numbers".
		bool m_fShowAsHex { true };
	};
}