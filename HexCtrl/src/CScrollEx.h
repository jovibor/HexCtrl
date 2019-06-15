/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/                         *
* This is a Hex Control for MFC applications.                                           *
* Official git repository of the project: https://github.com/jovibor/HexCtrl/           *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information, or any questions, visit the project's official repository.      *
****************************************************************************************/
/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/						    *
* This code is available under the "MIT License modified with The Commons Clause"		*
* Scroll bar control class for MFC apps.												*
* The main creation purpose of this control is the innate 32-bit range limitation		*
* of the standard Windows' scrollbars.													*
* This control works with unsigned long long data representation and thus can operate	*
* with numbers in full 64-bit range.													*
****************************************************************************************/
#pragma once
#include <afxwin.h>

namespace HEXCTRL {
	namespace SCROLLEX {
		//Forward declaration.
		enum class ENSTATE : DWORD;

		class CScrollEx : public CWnd
		{
		public:
			CScrollEx() {}
			~CScrollEx() {}
			bool Create(CWnd* pWnd, int iScrollType, ULONGLONG ullScrolline, ULONGLONG ullScrollPage, ULONGLONG ullScrollSizeMax);
			void AddSibling(CScrollEx* pSibling);
			bool IsVisible()const;
			CWnd* GetParent()const;
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
			ULONGLONG GetScrollPos()const;
			LONGLONG GetScrollPosDelta()const;
			ULONGLONG GetScrollLineSize()const;
			ULONGLONG GetScrollPageSize()const;
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
			void DrawScrollBar()const;									//Draws the whole Scrollbar.
			void DrawArrows(CDC* pDC)const;								//Draws arrows.
			void DrawThumb(CDC* pDC)const;								//Draws the Scroll thumb.
			CRect GetScrollRect(bool fWithNCArea = false)const;			//Scroll's whole rect.
			CRect GetScrollWorkAreaRect(bool fClientCoord = false)const;//Rect without arrows.
			UINT GetScrollSizeWH()const;								//Scroll size in pixels, width or height.
			UINT GetScrollWorkAreaSizeWH()const;						//Scroll size (WH) without arrows.
			CRect GetThumbRect(bool fClientCoord = false)const;
			UINT GetThumbSizeWH()const;
			UINT GetThumbPos()const;
			void SetThumbPos(int iPos);
			long double GetThumbScrollingSize()const;
			CRect GetFirstArrowRect(bool fClientCoord = false)const;
			CRect GetLastArrowRect(bool fClientCoord = false)const;
			CRect GetFirstChannelRect(bool fClientCoord = false)const;
			CRect GetLastChannelRect(bool fClientCoord = false)const;
			CRect GetParentRect(bool fClient = true)const;
			int GetTopDelta()const;	//Difference between parent window's Window and Client area. Very important in hit testing.
			int GetLeftDelta()const;
			bool IsVert()const;											//Is vertical or horizontal scrollbar.
			bool IsThumbDragging()const;								//Is the thumb currently dragged by mouse.
			bool IsSiblingVisible()const;								//Is sibling scrollbar currently visible or not.
			void SendParentScrollMsg()const;							//Sends the WM_(V/H)SCROLL to the parent window.
			afx_msg void OnTimer(UINT_PTR nIDEvent);
		protected:
			CWnd* m_pwndParent { };										//Parent window.
			CScrollEx* m_pSibling { };									//Sibling scrollbar, added with AddSibling.
			UINT m_uiScrollBarSizeWH { };								//Scrollbar size (width if vertical, height if horz).
			int m_iScrollType { };										//Scrollbar type - horizontal or vertical.
			ENSTATE m_enState { };										//Current state.
			const COLORREF m_clrBkNC { GetSysColor(COLOR_3DFACE) };		//Bk color of the non client area. 
			const COLORREF m_clrBkScrollBar { RGB(241, 241, 241) };		//Color of the scrollbar.
			const COLORREF m_clrThumb { RGB(192, 192, 192) };			//Scroll thumb color.
			CPoint m_ptCursorCur { };									//Cursor's current position.
			ULONGLONG m_ullScrollPosCur { 0 };							//Current scroll position.
			ULONGLONG m_ullScrollPosPrev { };							//Previous scroll position.
			ULONGLONG m_ullScrollLine { };								//Size of one line scroll, when clicking arrow.
			ULONGLONG m_ullScrollPage { };								//Size of page scroll, when clicking channel.
			ULONGLONG m_ullScrollSizeMax { };							//Maximum scroll size (limit).
			const unsigned m_uiThumbSizeMin { 15 };						//Minimum thumb size.
			const int m_iTimerFirstClick { 200 };						//Millisec for WM_TIMER for first channel click.
			const int m_iTimerRepeat { 50 };							//Millisec for repeat when click and hold on channel.
			CBitmap m_bmpArrows;										//Bitmap of the arrows.
			const unsigned m_uiFirstArrowOffset { 0 };					//Offset of the first arrow, in pixels, at arrows bitmap.
			const unsigned m_uiLastArrowOffset { 18 };					//Offset of the last arrow, in pixels, at arrows bitmap.
			const unsigned m_uiArrowSize { 17 };						//Arrow size in pixels.
			bool m_fCreated { false };									//Is created or not.
			bool m_fVisible { false };									//Is visible at the moment or not.
		};
	};
};