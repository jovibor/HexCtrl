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

namespace HEXCTRL::INTERNAL::LISTEX {

	struct CELLCOLOR
	{
		COLORREF clrBk;
		COLORREF clrText;
	};

	/********************************************
	* CListEx class declaration.                *
	********************************************/
	class CListEx : public IListEx
	{
	public:
		DECLARE_DYNAMIC(CListEx)
		CListEx() = default;
		~CListEx() = default;
		bool Create(const LISTEXCREATESTRUCT& lcs)override;
		void CreateDialogCtrl(UINT uCtrlID, CWnd* pwndDlg)override;
		static int CALLBACK DefCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
		BOOL DeleteAllItems()override;
		BOOL DeleteItem(int nItem)override;
		void Destroy()override;
		ULONGLONG GetCellData(int iItem, int iSubitem)override;
		UINT GetFontSize()override;
		int GetSortColumn()const override;
		bool GetSortAscending()const override;
		bool IsCreated()const override;
		void SetCellColor(int iItem, int iSubitem, COLORREF clrBk, COLORREF clrText)override;
		void SetCellData(int iItem, int iSubitem, ULONGLONG ullData)override;
		void SetCellMenu(int iItem, int iSubitem, CMenu* pMenu)override;
		void SetCellTooltip(int iItem, int iSubitem, const wchar_t* pwszTooltip, const wchar_t* pwszCaption = nullptr)override;
		void SetColor(const LISTEXCOLORSTRUCT& lcs)override;
		void SetFont(const LOGFONTW* pLogFontNew)override;
		void SetFontSize(UINT uiSize)override;
		void SetHeaderHeight(DWORD dwHeight)override;
		void SetHeaderFont(const LOGFONTW* pLogFontNew)override;
		void SetHeaderColumnColor(DWORD nColumn, COLORREF clr)override;
		void SetListMenu(CMenu* pMenu)override;
		void SetSortable(bool fSortable, int (CALLBACK *pfCompareFunc)(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) = nullptr)override;
		DECLARE_MESSAGE_MAP()
	protected:
		CListExHdr& GetHeaderCtrl() { return m_stListHeader; }
		void InitHeader();
		bool HasCellColor(int iItem, int iSubitem, COLORREF& clrBk, COLORREF& clrText);
		bool HasTooltip(int iItem, int iSubitem, std::wstring** ppwstrText = nullptr, std::wstring** ppwstrCaption = nullptr);
		bool HasMenu(int iItem, int iSubitem, CMenu** ppMenu = nullptr);
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
		bool m_fCreated { false };
		CListExHdr m_stListHeader;
		LISTEXCOLORSTRUCT m_stColor { };
		CFont m_fontList;
		CPen m_penGrid;
		HWND m_hwndTt { };
		TOOLINFO m_stToolInfo { };
		bool m_fTtShown { false };
		LVHITTESTINFO m_stCurrCell { };
		DWORD m_dwGridWidth { 1 };		//Grid width.
		CMenu* m_pListMenu { };			//List global menu, if set.
		std::unordered_map<int, std::unordered_map<int,
			std::tuple<std::wstring/*tip text*/, std::wstring/*caption text*/>>> m_umapCellTt { }; //Cell's tooltips.
		std::unordered_map<int, std::unordered_map<int, CMenu*>> m_umapCellMenu { };			   //Cell's menus.
		std::unordered_map<int, std::unordered_map<int, ULONGLONG>> m_umapCellData { };            //Cell's custom data.
		std::unordered_map<int, std::unordered_map<int, CELLCOLOR>> m_umapCellColor { };            //Cell's colors.
		NMITEMACTIVATE m_stNMII { };
		const ULONG_PTR ID_TIMER_TOOLTIP { 0x01 };
		int m_iSortColumn { };
		int (CALLBACK *m_pfCompareFunc)(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) { nullptr };
		bool m_fSortable { false };
		bool m_fSortAscending { };
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