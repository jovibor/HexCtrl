/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is very extended and featured version of CMFCListCtrl class.                     *
* Official git repository: https://github.com/jovibor/ListEx/                           *
* This class is available under the "MIT License".                                      *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#pragma once
#include "../ListEx.h"
#include "CListExHdr.h"
#include <chrono>
#include <unordered_map>

namespace HEXCTRL::LISTEX::INTERNAL
{
	/********************************************
	* CELLTOOLTIP - tool-tips for the cell.     *
	********************************************/
	struct CELLTOOLTIP
	{
		std::wstring wstrText;
		std::wstring wstrCaption;
	};

	/********************************************
	* COLUMNCOLOR - colors for the column.      *
	********************************************/
	struct COLUMNCOLOR
	{
		COLORREF   clrBk { };    //Background.
		COLORREF   clrText { };  //Text.
		std::chrono::high_resolution_clock::time_point time { }; //Time when added.
	};

	/********************************************
	* ROWCOLOR - colors for the row.            *
	********************************************/
	struct ROWCOLOR
	{
		COLORREF   clrBk { };    //Background.
		COLORREF   clrText { };  //Text.
		std::chrono::high_resolution_clock::time_point time { }; //Time when added.
	};

	/********************************************
	* ITEMTEXT - text and links in the cell.    *
	********************************************/
	struct ITEMTEXT
	{
		ITEMTEXT(std::wstring_view wstrText, std::wstring_view wstrLink, std::wstring_view wstrTitle,
			CRect rect, bool fLink = false, bool fTitle = false) :
			wstrText(wstrText), wstrLink(wstrLink), wstrTitle(wstrTitle), rect(rect), fLink(fLink), fTitle(fTitle) {}
		std::wstring wstrText { };  //Visible text.
		std::wstring wstrLink { };  //Text within link <link="textFromHere"> tag.
		std::wstring wstrTitle { }; //Text within title <...title="textFromHere"> tag.
		CRect rect { };             //Rect text belongs to.
		bool fLink { false };       //Is it just a text (wstrLink is empty) or text with link?
		bool fTitle { false };      //Is it link with custom title (wstrTitle is not empty)?
	};

	/********************************************
	* CListEx class declaration.                *
	********************************************/
	class CListEx final : public IListEx
	{
	public:
		bool Create(const LISTEXCREATESTRUCT& lcs)override;
		void CreateDialogCtrl(UINT uCtrlID, CWnd* pwndDlg)override;
		static int CALLBACK DefCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
		BOOL DeleteAllItems()override;
		BOOL DeleteColumn(int nCol)override;
		BOOL DeleteItem(int iItem)override;
		void Destroy()override;
		[[nodiscard]] ULONGLONG GetCellData(int iItem, int iSubItem)const override;
		[[nodiscard]] LISTEXCOLORS GetColors()const override;
		[[nodiscard]] EListExSortMode GetColumnSortMode(int iColumn)const override;
		[[nodiscard]] UINT GetFontSize()const override;
		[[nodiscard]] int GetSortColumn()const override;
		[[nodiscard]] bool GetSortAscending()const override;
		[[nodiscard]] bool IsCreated()const override;
		[[nodiscard]] UINT MapIndexToID(UINT nItem)const;
		void SetCellColor(int iItem, int iSubItem, COLORREF clrBk, COLORREF clrText)override;
		void SetCellData(int iItem, int iSubItem, ULONGLONG ullData)override;
		void SetCellMenu(int iItem, int iSubItem, CMenu* pMenu)override;
		void SetCellTooltip(int iItem, int iSubItem, std::wstring_view wstrTooltip, std::wstring_view wstrCaption)override;
		void SetColors(const LISTEXCOLORS& lcs)override;
		void SetColumnColor(int iColumn, COLORREF clrBk, COLORREF clrText)override;
		void SetColumnSortMode(int iColumn, EListExSortMode enSortMode)override;
		void SetFont(const LOGFONTW* pLogFontNew)override;
		void SetFontSize(UINT uiSize)override;
		void SetHdrHeight(DWORD dwHeight)override;
		void SetHdrFont(const LOGFONTW* pLogFontNew)override;
		void SetHdrColumnColor(int iColumn, COLORREF clrBk, COLORREF clrText)override;
		void SetListMenu(CMenu* pMenu)override;
		void SetRowColor(DWORD dwRow, COLORREF clrBk, COLORREF clrText)override;
		void SetSortable(bool fSortable, PFNLVCOMPARE pfnCompare, EListExSortMode enSortMode)override;
		DECLARE_DYNAMIC(CListEx)
		DECLARE_MESSAGE_MAP()
	protected:
		CListExHdr& GetHeaderCtrl()override { return m_stListHeader; }
		void InitHeader()override;
		bool HasCellColor(int iItem, int iSubItem, COLORREF& clrBk, COLORREF& clrText);
		bool HasTooltip(int iItem, int iSubItem, std::wstring** ppwstrText = nullptr, std::wstring** ppwstrCaption = nullptr);
		bool HasMenu(int iItem, int iSubItem, CMenu** ppMenu = nullptr);
		std::vector<ITEMTEXT> ParseItemText(int iItem, int iSubitem);
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
		afx_msg void OnContextMenu(CWnd* pWnd, CPoint pt);
		BOOL OnCommand(WPARAM wParam, LPARAM lParam)override;
		afx_msg void OnMouseMove(UINT nFlags, CPoint pt);
		afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
		afx_msg void OnTimer(UINT_PTR nIDEvent);
		afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
		afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
		afx_msg void MeasureItem(LPMEASUREITEMSTRUCT lpMIS);
		afx_msg void OnHdnDividerdblclick(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnHdnBegintrack(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnHdnTrack(NMHDR *pNMHDR, LRESULT *pResult);
		BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)override;
		afx_msg void OnLvnColumnClick(NMHDR *pNMHDR, LRESULT *pResult);
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
		CMenu* m_pListMenu { };			//List global menu, if set.
		NMITEMACTIVATE m_stNMII { };    //Struct for SendMessage.
		int m_iSortColumn { };          //Currently clicked header column.
		long m_lSizeFont { };           //Font size.
		PFNLVCOMPARE m_pfnCompare { nullptr };  //Pointer to user provided compare func.
		EListExSortMode m_enDefSortMode { EListExSortMode::SORT_LEX }; //Default sorting mode.
		std::unordered_map<int, std::unordered_map<int, CELLTOOLTIP>> m_umapCellTt { }; //Cell's tooltips.
		std::unordered_map<int, std::unordered_map<int, CMenu*>> m_umapCellMenu { };    //Cell's menus.
		std::unordered_map<int, std::unordered_map<int, ULONGLONG>> m_umapCellData { }; //Cell's custom data.
		std::unordered_map<int, std::unordered_map<int, LISTEXCELLCOLOR>> m_umapCellColor { }; //Cell's colors.
		std::unordered_map<DWORD, ROWCOLOR> m_umapRowColor { };            //Row colors.
		std::unordered_map<int, COLUMNCOLOR> m_umapColumnColor { };        //Column colors.
		std::unordered_map<int, EListExSortMode> m_umapColumnSortMode { }; //Column sorting mode.
		bool m_fCreated { false };     //Is created.
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