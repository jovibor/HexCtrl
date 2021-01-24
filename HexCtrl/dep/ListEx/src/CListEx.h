/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
* This is very extended and featured version of CMFCListCtrl class.                     *
* Official git repository: https://github.com/jovibor/ListEx/                           *
* This class is available under the "MIT License".                                      *
****************************************************************************************/
#pragma once
#include "../ListEx.h"
#include "CListExHdr.h"
#include <chrono>
#include <unordered_map>

namespace HEXCTRL::LISTEX::INTERNAL
{
	/********************************************
	* CListEx class declaration.                *
	********************************************/
	class CListEx final : public IListEx
	{
		struct SCOLROWCLR;
		struct SITEMDATA;
	public:
		bool Create(const LISTEXCREATESTRUCT& lcs)override;
		void CreateDialogCtrl(UINT uCtrlID, CWnd* pParent)override;
		static int CALLBACK DefCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
		BOOL DeleteAllItems()override;
		BOOL DeleteColumn(int iIndex)override;
		BOOL DeleteItem(int iItem)override;
		void Destroy()override;
		[[nodiscard]] ULONGLONG GetCellData(int iItem, int iSubItem)const override;
		[[nodiscard]] LISTEXCOLORS GetColors()const override;
		[[nodiscard]] EListExSortMode GetColumnSortMode(int iColumn)const override;
		[[nodiscard]] UINT GetFontSize()const override;
		[[nodiscard]] int GetSortColumn()const override;
		[[nodiscard]] bool GetSortAscending()const override;
		void HideColumn(int iIndex, bool fHide)override;
		int InsertColumn(int nCol, const LVCOLUMN* pColumn);
		int InsertColumn(int nCol, LPCTSTR lpszColumnHeading, int nFormat = LVCFMT_LEFT, int nWidth = -1, int nSubItem = -1);
		[[nodiscard]] bool IsCreated()const override;
		[[nodiscard]] bool IsColumnSortable(int iColumn)override;
		void ResetSort()override; //Reset all the sort by any column to its default state.
		void SetCellColor(int iItem, int iSubItem, COLORREF clrBk, COLORREF clrText)override;
		void SetCellData(int iItem, int iSubItem, ULONGLONG ullData)override;
		void SetCellIcon(int iItem, int iSubItem, int iIndex)override; //Icon index in list's image list.
		void SetCellTooltip(int iItem, int iSubItem, std::wstring_view wstrTooltip, std::wstring_view wstrCaption)override;
		void SetColors(const LISTEXCOLORS& lcs)override;
		void SetColumnColor(int iColumn, COLORREF clrBk, COLORREF clrText)override;
		void SetColumnSortMode(int iColumn, bool fSortable, EListExSortMode enSortMode = { })override;
		void SetFont(const LOGFONTW* pLogFontNew)override;
		void SetFontSize(UINT uiSize)override;
		void SetHdrColumnColor(int iColumn, COLORREF clrBk, COLORREF clrText = -1)override;
		void SetHdrColumnIcon(int iColumn, const LISTEXHDRICON& stIcon)override; //Icon for a given column.
		void SetHdrFont(const LOGFONTW* pLogFontNew)override;
		void SetHdrHeight(DWORD dwHeight)override;
		void SetHdrImageList(CImageList* pList)override;
		void SetRowColor(DWORD dwRow, COLORREF clrBk, COLORREF clrText)override;
		void SetSortable(bool fSortable, PFNLVCOMPARE pfnCompare, EListExSortMode enSortMode)override;
		DECLARE_DYNAMIC(CListEx)
		DECLARE_MESSAGE_MAP()
	protected:
		CListExHdr& GetHeaderCtrl()override { return m_stListHeader; }
		void InitHeader()override;
		auto HasColor(int iItem, int iSubItem)->std::optional<PLISTEXCOLOR>;
		auto HasTooltip(int iItem, int iSubItem)->std::optional<PLISTEXTOOLTIP>;
		int HasIcon(int iItem, int iSubItem); //Does cell have an icon associated.
		auto ParseItemText(int iItem, int iSubitem)->std::vector<SITEMDATA>;
		void TtLinkHide();
		void TtCellHide();
		void DrawItem(LPDRAWITEMSTRUCT pDIS)override;
		afx_msg void OnPaint();
		afx_msg BOOL OnEraseBkgnd(CDC* pDC);
		afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
		afx_msg void OnKillFocus(CWnd* pNewWnd);
		afx_msg void OnLButtonDown(UINT nFlags, CPoint pt);
		afx_msg void OnLButtonUp(UINT nFlags, CPoint pt);
		afx_msg void OnRButtonDown(UINT nFlags, CPoint pt);
		afx_msg void OnMouseMove(UINT nFlags, CPoint pt);
		afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
		afx_msg void OnTimer(UINT_PTR nIDEvent);
		afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
		afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
		afx_msg void MeasureItem(LPMEASUREITEMSTRUCT lpMIS);
		BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)override;
		afx_msg void OnLvnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnHdnBegindrag(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnHdnBegintrack(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnDestroy();
	private:
		CListExHdr m_stListHeader;
		LISTEXCOLORS m_stColors { };
		CFont m_fontList;               //Default list font.
		CFont m_fontListUnderline;      //Underlined list font, for links.
		CPen m_penGrid;                 //Pen for list lines between cells.
		CWnd m_stWndTtCell;             //Cells' tool-tip window.
		TTTOOLINFOW m_stTInfoCell { };  //Cells' tool-tip info struct.
		CWnd m_stWndTtLink;             //Link tool-tip window.
		TTTOOLINFOW m_stTInfoLink { };  //Link's tool-tip info struct.
		std::wstring m_wstrTtText { };  //Link's tool-tip current text.
		HCURSOR m_cursorHand { };       //Hand cursor handle.
		HCURSOR m_cursorDefault { };    //Standard (default) cursor handle.
		LVHITTESTINFO m_stCurrCell { }; //Cell's hit struct for tool-tip.
		LVHITTESTINFO m_stCurrLink { }; //Cell's link hit struct for tool-tip.
		DWORD m_dwGridWidth { 1 };		//Grid width.
		int m_iSortColumn { };          //Currently clicked header column.
		long m_lSizeFont { };           //Font size.
		PFNLVCOMPARE m_pfnCompare { nullptr };  //Pointer to user provided compare func.
		EListExSortMode m_enDefSortMode { EListExSortMode::SORT_LEX }; //Default sorting mode.
		std::unordered_map<int, std::unordered_map<int, LISTEXTOOLTIP>> m_umapCellTt { };  //Cell's tooltips.
		std::unordered_map<int, std::unordered_map<int, ULONGLONG>> m_umapCellData { };    //Cell's custom data.
		std::unordered_map<int, std::unordered_map<int, LISTEXCOLOR>> m_umapCellColor { }; //Cell's colors.
		std::unordered_map<int, SCOLROWCLR> m_umapRowColor { };                   //Row colors.
		std::unordered_map<int, SCOLROWCLR> m_umapColumnColor { };                //Column colors.
		std::unordered_map<int, std::unordered_map<int, int>> m_umapCellIcon { }; //Cell's icon.
		std::unordered_map<int, EListExSortMode> m_umapColumnSortMode { };        //Column sorting mode.
		bool m_fCreated { false };     //Is created.
		bool m_fHighLatency { };       //High latency flag.
		bool m_fSortable { false };    //Is list sortable.
		bool m_fSortAscending { };     //Sorting type (ascending, descending).
		bool m_fLinksUnderline { };    //Links are displayed underlined or not.
		bool m_fLinkTooltip { };       //Show links toolips.
		bool m_fVirtual { false };     //Whether list is virtual (LVS_OWNERDATA) or not.
		bool m_fTtCellShown { false }; //Is cell's tool-tip shown atm.
		bool m_fTtLinkShown { false }; //Is link's tool-tip shown atm.
		bool m_fLDownAtLink { false }; //Left mouse down on link.
		CRect m_rcLinkCurr { };        //Current link's rect;
	};

			/*******************Setting a manifest for ComCtl32.dll version 6.***********************/
#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif
}