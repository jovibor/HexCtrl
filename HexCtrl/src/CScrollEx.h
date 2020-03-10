/****************************************************************************************
* Copyright � 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC applications.                                           *
* Official git repository of the project: https://github.com/jovibor/HexCtrl/           *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#pragma once
#include <afxwin.h>

namespace HEXCTRL::INTERNAL::SCROLLEX
{
	//Forward declaration.
	enum class EState : DWORD;

	class CScrollEx : public CWnd
	{
	public:
		explicit CScrollEx() = default;
		bool Create(CWnd* pWnd, int iScrollType, ULONGLONG ullScrolline, ULONGLONG ullScrollPage, ULONGLONG ullScrollSizeMax);
		void AddSibling(CScrollEx* pSibling);
		[[nodiscard]] bool IsVisible()const;
		[[nodiscard]] CWnd* GetParent()const;
		void SetScrollSizes(ULONGLONG ullScrolline, ULONGLONG ullScrollPage, ULONGLONG ullScrollSizeMax);
		ULONGLONG SetScrollPos(ULONGLONG);
		void ScrollLineUp();
		void ScrollLineDown();
		void ScrollLineLeft();
		void ScrollLineRight();
		void ScrollPageUp();
		void ScrollPageDown();
		void ScrollPageLeft();
		void ScrollPageRight();
		void ScrollHome();
		void ScrollEnd();
		[[nodiscard]] ULONGLONG GetScrollPos()const;
		[[nodiscard]] LONGLONG GetScrollPosDelta()const;
		[[nodiscard]] ULONGLONG GetScrollLineSize()const;
		[[nodiscard]] ULONGLONG GetScrollPageSize()const;
		void SetScrollPageSize(ULONGLONG ullSize);

		/************************************************************************
		* CALLBACK METHODS:														*
		* These methods below must be called in the corresponding methods		*
		* of the parent window.													*
		************************************************************************/
		BOOL OnNcActivate(BOOL bActive)const;
		void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
		void OnNcPaint()const;
		void OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
		void OnMouseMove(UINT nFlags, CPoint point);
		void OnLButtonUp(UINT nFlags, CPoint point);
		/************************************************************************
		* END OF THE CALLBACK METHODS.											*
		************************************************************************/
	protected:
		DECLARE_MESSAGE_MAP()
		void DrawScrollBar()const;                                   //Draws the whole Scrollbar.
		void DrawArrows(CDC* pDC)const;                              //Draws arrows.
		void DrawThumb(CDC* pDC)const;                               //Draws the Scroll thumb.
		[[nodiscard]] CRect GetScrollRect(bool fWithNCArea = false)const;          //Scroll's whole rect.
		[[nodiscard]] CRect GetScrollWorkAreaRect(bool fClientCoord = false)const; //Rect without arrows.
		[[nodiscard]] UINT GetScrollSizeWH()const;                                 //Scroll size in pixels, width or height.
		[[nodiscard]] UINT GetScrollWorkAreaSizeWH()const;                         //Scroll size (WH) without arrows.
		[[nodiscard]] CRect GetThumbRect(bool fClientCoord = false)const;
		[[nodiscard]] UINT GetThumbSizeWH()const;
		[[nodiscard]] int GetThumbPos()const;                                      //Current Thumb pos.
		void SetThumbPos(int iPos);
		[[nodiscard]] long double GetThumbScrollingSize()const;
		[[nodiscard]] CRect GetFirstArrowRect(bool fClientCoord = false)const;
		[[nodiscard]] CRect GetLastArrowRect(bool fClientCoord = false)const;
		[[nodiscard]] CRect GetFirstChannelRect(bool fClientCoord = false)const;
		[[nodiscard]] CRect GetLastChannelRect(bool fClientCoord = false)const;
		[[nodiscard]] CRect GetParentRect(bool fClient = true)const;
		[[nodiscard]] int GetTopDelta()const;	//Difference between parent window's Window and Client area. Very important in hit testing.
		[[nodiscard]] int GetLeftDelta()const;
		[[nodiscard]] bool IsVert()const;                                         //Is vertical or horizontal scrollbar.
		[[nodiscard]] bool IsThumbDragging()const;                                //Is the thumb currently dragged by mouse.
		[[nodiscard]] bool IsSiblingVisible()const;                               //Is sibling scrollbar currently visible or not.
		void SendParentScrollMsg()const;                            //Sends the WM_(V/H)SCROLL to the parent window.
		afx_msg void OnTimer(UINT_PTR nIDEvent);
		afx_msg void OnDestroy();
	protected:
		CWnd* m_pwndParent { };                                     //Parent window.
		CScrollEx* m_pSibling { };                                  //Sibling scrollbar, added with AddSibling.
		CBitmap m_bmpArrows;                                        //Bitmap of the arrows.
		UINT m_uiScrollBarSizeWH { };                               //Scrollbar size (width if vertical, height if horz).
		EState m_enState { };                                       //Current state.
		CPoint m_ptCursorCur { };                                   //Cursor's current position.
		ULONGLONG m_ullScrollPosCur { 0 };                          //Current scroll position.
		ULONGLONG m_ullScrollPosPrev { };                           //Previous scroll position.
		ULONGLONG m_ullScrollLine { };                              //Size of one line scroll, when clicking arrow.
		ULONGLONG m_ullScrollPage { };                              //Size of page scroll, when clicking channel.
		ULONGLONG m_ullScrollSizeMax { };                           //Maximum scroll size (limit).
		int m_iScrollType { };                                      //Scrollbar type - horizontal or vertical.
		const COLORREF m_clrBkNC { GetSysColor(COLOR_3DFACE) };     //Bk color of the non client area. 
		const COLORREF m_clrBkScrollBar { RGB(241, 241, 241) };     //Color of the scrollbar.
		const COLORREF m_clrThumb { RGB(192, 192, 192) };           //Scroll thumb color.
		const unsigned m_uiThumbSizeMin { 15 };                     //Minimum thumb size.
		const int m_iTimerFirstClick { 200 };                       //Millisec for WM_TIMER for first channel click.
		const int m_iTimerRepeat { 50 };                            //Millisec for repeat when click and hold on channel.
		const unsigned m_uiFirstArrowOffset { 0 };                  //Offset of the first arrow, in pixels, at arrows bitmap.
		const unsigned m_uiLastArrowOffset { 18 };                  //Offset of the last arrow, in pixels, at arrows bitmap.
		const unsigned m_uiArrowSize { 17 };                        //Arrow size in pixels.
		bool m_fCreated { false };                                  //Is created or not.
		bool m_fVisible { false };                                  //Is visible at the moment or not.
	};
}