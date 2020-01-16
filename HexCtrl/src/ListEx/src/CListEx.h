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
#include <unordered_map>
#include <chrono>

namespace HEXCTRL::INTERNAL::LISTEX::INTERNAL
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
	* CELLCOLOR - colors for the cell.          *
	********************************************/
	struct CELLCOLOR
	{
		COLORREF clrBk;
		COLORREF clrText;
	};


	/********************************************
	* CListEx class declaration.                *
	********************************************/
	class CListEx final : public IListEx
	{
	public:
		explicit CListEx() = default;
		~CListEx() = default;
		bool Create(const LISTEXCREATESTRUCT& lcs)override;
		void CreateDialogCtrl(UINT uCtrlID, CWnd* pwndDlg)override;
		static int CALLBACK DefCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
		BOOL DeleteAllItems()override;
		BOOL DeleteColumn(int nCol)override;
		BOOL DeleteItem(int iItem)override;
		void Destroy()override;
		ULONGLONG GetCellData(int iItem, int iSubItem)const override;
		EnListExSortMode GetColumnSortMode(int iColumn)const override;
		UINT GetFontSize()const override;
		int GetSortColumn()const override;
		bool GetSortAscending()const override;
		bool IsCreated()const override;
		UINT MapIndexToID(UINT nItem)const;
		void SetCellColor(int iItem, int iSubItem, COLORREF clrBk, COLORREF clrText)override;
		void SetCellData(int iItem, int iSubItem, ULONGLONG ullData)override;
		void SetCellMenu(int iItem, int iSubItem, CMenu* pMenu)override;
		void SetCellTooltip(int iItem, int iSubItem, std::wstring_view wstrTooltip, std::wstring_view wstrCaption)override;
		void SetColor(const LISTEXCOLORSTRUCT& lcs)override;
		void SetColumnColor(int iColumn, COLORREF clrBk, COLORREF clrText)override;
		void SetColumnSortMode(int iColumn, EnListExSortMode enSortMode)override;
		void SetFont(const LOGFONTW* pLogFontNew)override;
		void SetFontSize(UINT uiSize)override;
		void SetHdrHeight(DWORD dwHeight)override;
		void SetHdrFont(const LOGFONTW* pLogFontNew)override;
		void SetHdrColumnColor(int iColumn, COLORREF clrBk, COLORREF clrText)override;
		void SetListMenu(CMenu* pMenu)override;
		void SetRowColor(DWORD dwRow, COLORREF clrBk, COLORREF clrText)override;
		void SetSortable(bool fSortable, PFNLVCOMPARE pfnCompare, EnListExSortMode enSortMode)override;
		DECLARE_DYNAMIC(CListEx)
		DECLARE_MESSAGE_MAP()
	protected:
		CListExHdr& GetHeaderCtrl() { return m_stListHeader; }
		void InitHeader();
		bool HasCellColor(int iItem, int iSubItem, COLORREF& clrBk, COLORREF& clrText);
		bool HasTooltip(int iItem, int iSubItem, std::wstring** ppwstrText = nullptr, std::wstring** ppwstrCaption = nullptr);
		bool HasMenu(int iItem, int iSubItem, CMenu** ppMenu = nullptr);
		void DrawItem(LPDRAWITEMSTRUCT);
		afx_msg void OnPaint();
		afx_msg BOOL OnEraseBkgnd(CDC* pDC);
		afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
		afx_msg void OnKillFocus(CWnd* pNewWnd);
		afx_msg void OnLButtonDown(UINT nFlags, CPoint pt);
		afx_msg void OnRButtonDown(UINT nFlags, CPoint pt);
		afx_msg void OnContextMenu(CWnd* pWnd, CPoint pt);
		virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
		afx_msg void OnMouseMove(UINT nFlags, CPoint pt);
		afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
		afx_msg void OnTimer(UINT_PTR nIDEvent);
		afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
		afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
		afx_msg void MeasureItem(LPMEASUREITEMSTRUCT lpMIS);
		afx_msg void OnHdnDividerdblclick(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnHdnBegintrack(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnHdnTrack(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnDestroy();
		afx_msg void OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult);
	private:
		CListExHdr m_stListHeader;
		LISTEXCOLORSTRUCT m_stColor { };
		CFont m_fontList;
		CPen m_penGrid;
		CWnd m_wndTt;                   //Tool-tip window.
		TTTOOLINFOW m_stToolInfo { };   //Tool-tip info struct.
		LVHITTESTINFO m_stCurrCell { };
		DWORD m_dwGridWidth { 1 };		//Grid width.
		CMenu* m_pListMenu { };			//List global menu, if set.
		std::unordered_map<int, std::unordered_map<int, CELLTOOLTIP>> m_umapCellTt { };  //Cell's tooltips.
		std::unordered_map<int, std::unordered_map<int, CMenu*>> m_umapCellMenu { };	 //Cell's menus.
		std::unordered_map<int, std::unordered_map<int, ULONGLONG>> m_umapCellData { };  //Cell's custom data.
		std::unordered_map<int, std::unordered_map<int, CELLCOLOR>> m_umapCellColor { }; //Cell's colors.
		std::unordered_map<DWORD, ROWCOLOR> m_umapRowColor { };     //Row colors.
		std::unordered_map<int, COLUMNCOLOR> m_umapColumnColor { }; //Column colors.
		std::unordered_map<int, EnListExSortMode> m_umapColumnSortMode { };              //Column sorting mode.
		NMITEMACTIVATE m_stNMII { };
		int m_iSortColumn { };
		long m_lSizeFont { };       //Font size.
		PFNLVCOMPARE m_pfnCompare { nullptr };  //Pointer to user provided compare func.
		EnListExSortMode m_enDefSortMode { EnListExSortMode::SORT_LEX }; //Default sorting mode.
		bool m_fCreated { false };  //Is created.
		bool m_fSortable { false }; //Is list sortable.
		bool m_fSortAscending { };  //Sorting type (ascending, descending).
		bool m_fVirtual { false };  //Whether list is virtual (LVS_OWNERDATA) or not.
		bool m_fTtShown { false };  //Is tool-tip shown atm.
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