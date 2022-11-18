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
#include <deque>

namespace HEXCTRL::INTERNAL
{
	class CHexDlgBkmMgr final : public CDialogEx, public IHexBookmarks
	{
	public:
		BOOL Create(UINT nIDTemplate, CWnd* pParent, IHexCtrl* pHexCtrl);
		ULONGLONG AddBkm(const HEXBKM& hbs, bool fRedraw)override; //Returns new bookmark Id.
		void ClearAll()override;
		[[nodiscard]] auto GetByID(ULONGLONG ullID) -> PHEXBKM override;       //Bookmark by ID.
		[[nodiscard]] auto GetByIndex(ULONGLONG ullIndex) -> PHEXBKM override; //Bookmark by index (in inner list).
		[[nodiscard]] ULONGLONG GetCount()override;
		[[nodiscard]] ULONGLONG GetCurrent()const;
		void GoBookmark(ULONGLONG ullIndex);
		void GoNext();
		void GoPrev();
		[[nodiscard]] bool HasBookmarks()const;
		[[nodiscard]] auto HitTest(ULONGLONG ullOffset) -> PHEXBKM override;
		[[nodiscard]] bool IsVirtual()const;
		void RemoveByOffset(ULONGLONG ullOffset);
		void RemoveByID(ULONGLONG ullID)override;
		void SetVirtual(IHexBookmarks* pVirtBkm);
		void SortData(int iColumn, bool fAscending);
		void Update(ULONGLONG ullID, const HEXBKM& stBookmark);
	private:
		enum class EMenuID : std::uint16_t;
		void DoDataExchange(CDataExchange* pDX)override;
		BOOL OnInitDialog()override;
		afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		BOOL OnCommand(WPARAM wParam, LPARAM lParam)override;
		BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)override;
		afx_msg void OnClickCheckHex();
		afx_msg void OnListGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnListItemChanged(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnListLClick(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnListDblClick(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnListRClick(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnListGetColor(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnDestroy();
		void UpdateList();
		void SortBookmarks();
		DECLARE_MESSAGE_MAP();
	private:
		std::deque<HEXBKM> m_deqBookmarks;
		IHexCtrl* m_pHexCtrl { };
		IHexBookmarks* m_pVirtual { };
		LONGLONG m_llIndexCurr { }; //Current bookmark position index, to move next/prev.
		LISTEX::IListExPtr m_pListMain { LISTEX::CreateListEx() };
		CMenu m_stMenuList;
		LISTEX::LISTEXCOLOR m_stCellClr { };
		bool m_fShowAsHex { true };
	};
}