/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/						    *
* This code is lisenced under the "MIT License modified with The Commons Clause"		*
* Scroll bar control class for MFC apps.												*
* The main creation purpose of this control is the innate 32-bit range limitation		*
* of the standard Windows's scrollbar control.											*
* This control works with unsigned long long data representation and thus can operate	*
* with numbers in full 64-bit range.													*
****************************************************************************************/
#include "stdafx.h"
#include "ScrollEx.h"
#include "res/HexCtrlRes.h"
#include <cmath>

using namespace HEXCTRL;
using namespace SCROLLEX64;

namespace HEXCTRL {
	/********************************************
	* Internal enums.							*
	********************************************/
	namespace SCROLLEX64 {
		enum SCROLL_STATE
		{
			FIRSTBUTTON_HOVER, FIRSTBUTTON_CLICK,
			FIRSTCHANNEL_CLICK, THUMB_HOVER,
			THUMB_CLICK, LASTCHANNEL_CLICK,
			LASTBUTTON_CLICK, LASTBUTTON_HOVER
		};
		enum SCROLL_TIMERS {
			IDT_FIRSTCLICK = 0x7ff0,
			IDT_CLICKREPEAT = 0x7ff1
		};
	}
}

/****************************************************
* CScrollEx64 class implementation.					*
****************************************************/
BEGIN_MESSAGE_MAP(CScrollEx64, CWnd)
	ON_WM_TIMER()
END_MESSAGE_MAP()

bool CScrollEx64::Create(CWnd * pWndParent, int iScrollType,
	ULONGLONG ullScrolline, ULONGLONG ullScrollPage, ULONGLONG ullScrollSizeMax)
{
	if (m_fCreated || !pWndParent || (iScrollType != SB_VERT && iScrollType != SB_HORZ))
		return false;

	if (!CWnd::CreateEx(0, AfxRegisterWndClass(0), nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, 0))
		return false;

	m_iScrollType = iScrollType;
	m_pwndParent = pWndParent;

	UINT uiBmp;
	if (IsVert())
	{
		uiBmp = IDB_HEXCTRL_SCROLL_V;
		m_uiScrollBarSizeWH = GetSystemMetrics(SM_CXVSCROLL);
	}
	else
	{
		uiBmp = IDB_HEXCTRL_SCROLL_H;
		m_uiScrollBarSizeWH = GetSystemMetrics(SM_CXHSCROLL);
	}

	if (!m_bmpArrows.LoadBitmapW(uiBmp))
		return false;

	m_fCreated = true;
	SetScrollSizes(ullScrolline, ullScrollPage, ullScrollSizeMax);

	return true;
}

void CScrollEx64::AddSibling(CScrollEx64* pSibling)
{
	if (pSibling)
		m_pSibling = pSibling;
}

bool CScrollEx64::IsSiblingVisible()
{
	if (m_pSibling)
		return m_pSibling->IsVisible();

	return false;
}

void CScrollEx64::SetScrollSizes(ULONGLONG ullScrolline, ULONGLONG ullScrollPage, ULONGLONG ullScrollSizeMax)
{
	if (!m_fCreated)
		return;

	m_ullScrollLine = ullScrolline;
	m_ullScrollPage = ullScrollPage;
	m_ullScrollSizeMax = ullScrollSizeMax;

	CWnd* pWnd = GetParent();
	if (pWnd)
		pWnd->SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
}

ULONGLONG CScrollEx64::SetScrollPos(ULONGLONG ullNewPos)
{
	if (!m_fCreated)
		return 0;

	m_ullScrollPosPrev = m_ullScrollPosCur;
	m_ullScrollPosCur = ullNewPos;

	CRect rc = GetParentRect();
	int iScreenSize = 0;
	if (IsVert())
		iScreenSize = rc.Height();
	else
		iScreenSize = rc.Width();

	ULONGLONG ullMax;
	if (iScreenSize > m_ullScrollSizeMax)
		ullMax = 0;
	else
		ullMax = m_ullScrollSizeMax - iScreenSize;

	if (m_ullScrollPosCur > ullMax)
		m_ullScrollPosCur = ullMax;

	SendParentScrollMsg();
	DrawScrollBar();

	return m_ullScrollPosPrev;
}

void CScrollEx64::ScrollLineDown()
{
	ULONGLONG ullCur = GetScrollPos();
	ULONGLONG ullNew;
	if (ULONGLONG_MAX - ullCur < m_ullScrollLine) //To avoid overflow.
		ullNew = ULONGLONG_MAX;
	else
		ullNew = ullCur + m_ullScrollLine;
	SetScrollPos(ullNew);
}

void CScrollEx64::ScrollLineRight()
{
	ScrollLineDown();
}

void CScrollEx64::ScrollLineUp()
{
	ULONGLONG ullCur = GetScrollPos();
	ULONGLONG ullNew;
	if (m_ullScrollLine > ullCur) //To avoid overflow.
		ullNew = 0;
	else
		ullNew = ullCur - m_ullScrollLine;
	SetScrollPos(ullNew);
}

void CScrollEx64::ScrollLineLeft()
{
	ScrollLineUp();
}

void CScrollEx64::ScrollPageDown()
{
	ULONGLONG ullCur = GetScrollPos();
	ULONGLONG ullNew;
	if (ULONGLONG_MAX - ullCur < m_ullScrollPage) //To avoid overflow.
		ullNew = ULONGLONG_MAX;
	else
		ullNew = ullCur + m_ullScrollPage;
	SetScrollPos(ullNew);

}

void CScrollEx64::ScrollPageLeft()
{
	ScrollPageUp();
}

void CScrollEx64::ScrollPageRight()
{
	ScrollPageDown();
}

void CScrollEx64::ScrollPageUp()
{
	ULONGLONG ullCur = GetScrollPos();
	ULONGLONG ullNew;
	if (m_ullScrollPage > ullCur) //To avoid overflow.
		ullNew = 0;
	else
		ullNew = ullCur - m_ullScrollPage;
	SetScrollPos(ullNew);
}

void CScrollEx64::ScrollHome()
{
	SetScrollPos(0);
}

void CScrollEx64::ScrollEnd()
{
	SetScrollPos(m_ullScrollSizeMax);
}

ULONGLONG CScrollEx64::GetScrollPos()
{
	if (!m_fCreated)
		return 0;

	return m_ullScrollPosCur;
}

LONGLONG CScrollEx64::GetScrollPosDelta()
{
	if (!m_fCreated)
		return 0;

	return LONGLONG(m_ullScrollPosCur - m_ullScrollPosPrev);
}

ULONGLONG CScrollEx64::GetScrollLineSize()
{
	return m_ullScrollLine;
}

ULONGLONG CScrollEx64::GetScrollPageSize()
{
	return m_ullScrollPage;
}

void CScrollEx64::SetScrollPageSize(ULONGLONG ullSize)
{
	m_ullScrollPage = ullSize;
}

BOOL CScrollEx64::OnNcActivate(BOOL bActive)
{
	if (!m_fCreated)
		return FALSE;

	//To repaint NC area.
	GetParent()->SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

	return TRUE;
}

void CScrollEx64::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
	if (!m_fCreated)
		return;

	CRect rc = lpncsp->rgrc[0];
	ULONGLONG ullCurPos = GetScrollPos();
	if (IsVert())
	{
		UINT uiHeight = rc.Height();
		if (IsSiblingVisible())
			uiHeight = rc.Height() - m_uiScrollBarSizeWH;

		if (uiHeight < m_ullScrollSizeMax)
		{
			m_fVisible = true;
			if (ullCurPos + uiHeight > m_ullScrollSizeMax)
				SetScrollPos(m_ullScrollSizeMax - uiHeight);
			else
				DrawScrollBar();
			lpncsp->rgrc[0].right -= m_uiScrollBarSizeWH;
		}
		else
		{
			SetScrollPos(0);
			m_fVisible = false;
		}
	}
	else
	{
		UINT uiWidth = rc.Width();
		if (IsSiblingVisible())
			uiWidth = rc.Width() - m_uiScrollBarSizeWH;

		if (uiWidth < m_ullScrollSizeMax)
		{
			m_fVisible = true;
			if (ullCurPos + uiWidth > m_ullScrollSizeMax)
				SetScrollPos(m_ullScrollSizeMax - uiWidth);
			else
				DrawScrollBar();
			lpncsp->rgrc[0].bottom -= m_uiScrollBarSizeWH;
		}
		else
		{
			SetScrollPos(0);
			m_fVisible = false;
		}
	}
}

void CScrollEx64::OnNcPaint()
{
	if (!m_fCreated)
		return;

	DrawScrollBar();
}

void CScrollEx64::OnSetCursor(CWnd * pWnd, UINT nHitTest, UINT message)
{
	if (!m_fCreated || nHitTest == HTTOPLEFT || nHitTest == HTLEFT || nHitTest == HTTOPRIGHT || nHitTest == HTSIZE
		|| nHitTest == HTBOTTOMLEFT || nHitTest == HTRIGHT || nHitTest == HTBOTTOM || nHitTest == HTBOTTOMRIGHT)
		return;

	switch (message)
	{
	case WM_LBUTTONDOWN:
	{
		POINT pt;
		GetCursorPos(&pt);
		GetParent()->ScreenToClient(&pt);

		if (IsVisible())
		{
			if (GetThumbRect(true).PtInRect(pt))
			{
				m_ptCursorCur = pt;
				m_iScrollBarState = SCROLLEX64::SCROLL_STATE::THUMB_CLICK;
				GetParent()->SetCapture();
			}
			else if (GetFirstArrowRect(true).PtInRect(pt))
			{
				ScrollLineUp();
				m_iScrollBarState = SCROLLEX64::SCROLL_STATE::FIRSTBUTTON_CLICK;
				GetParent()->SetCapture();
				SetTimer(SCROLLEX64::SCROLL_TIMERS::IDT_FIRSTCLICK, m_iTimerFirstClick, nullptr);
			}
			else if (GetLastArrowRect(true).PtInRect(pt))
			{
				ScrollLineDown();
				m_iScrollBarState = SCROLLEX64::SCROLL_STATE::LASTBUTTON_CLICK;
				GetParent()->SetCapture();
				SetTimer(SCROLLEX64::SCROLL_TIMERS::IDT_FIRSTCLICK, m_iTimerFirstClick, nullptr);
			}
			else if (GetFirstChannelRect(true).PtInRect(pt))
			{
				ScrollPageUp();
				m_iScrollBarState = SCROLLEX64::SCROLL_STATE::FIRSTCHANNEL_CLICK;
				GetParent()->SetCapture();
				SetTimer(SCROLLEX64::SCROLL_TIMERS::IDT_FIRSTCLICK, m_iTimerFirstClick, nullptr);
			}
			else if (GetLastChannelRect(true).PtInRect(pt))
			{
				ScrollPageDown();
				m_iScrollBarState = SCROLLEX64::SCROLL_STATE::LASTCHANNEL_CLICK;
				GetParent()->SetCapture();
				SetTimer(SCROLLEX64::SCROLL_TIMERS::IDT_FIRSTCLICK, m_iTimerFirstClick, nullptr);
			}
		}
	}
	break;
	}
}

void CScrollEx64::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_fCreated)
		return;

	if (IsThumbDragging())
	{
		CRect rc = GetScrollWorkAreaRect(true);
		int iNewPos;

		if (IsVert())
		{
			if (point.y < rc.top)
				iNewPos = 0;
			else if (point.y > rc.bottom)
				iNewPos = 0x7FFFFFFF;
			else
				iNewPos = GetThumbPos() + (point.y - m_ptCursorCur.y);
		}
		else
		{
			if (point.x < rc.left)
				iNewPos = 0;
			else if (point.x > rc.right)
				iNewPos = 0x7FFFFFFF;
			else
				iNewPos = GetThumbPos() + (point.x - m_ptCursorCur.x);
		}

		m_ptCursorCur = point;
		SetThumbPos(iNewPos);
		SendParentScrollMsg();
	}
}

void CScrollEx64::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (!m_fCreated)
		return;

	if (m_iScrollBarState > 0)
	{
		m_iScrollBarState = 0;
		KillTimer(SCROLLEX64::SCROLL_TIMERS::IDT_FIRSTCLICK);
		KillTimer(SCROLLEX64::SCROLL_TIMERS::IDT_CLICKREPEAT);
		ReleaseCapture();
		DrawScrollBar();
	}
}

void CScrollEx64::DrawScrollBar()
{
	if (!IsVisible())
		return;

	CWnd* pwndParent = GetParent();
	CWindowDC parentDC(pwndParent);

	CDC dcMem;
	CBitmap bitmap;
	dcMem.CreateCompatibleDC(&parentDC);
	CRect rc = GetParentRect(false);
	bitmap.CreateCompatibleBitmap(&parentDC, rc.Width(), rc.Height());
	dcMem.SelectObject(&bitmap);
	CDC* pDC = &dcMem;

	CRect rcSNC = GetScrollRect(true); //Scroll with non client.
	pDC->FillSolidRect(&rcSNC, m_clrBkNC); //NC Bk.
	CRect rcS = GetScrollRect();
	pDC->FillSolidRect(&rcS, m_clrBkScrollBar); //Bk.
	DrawArrows(pDC);
	DrawThumb(pDC);

	parentDC.BitBlt(rcSNC.left, rcSNC.top, rcSNC.Width(), rcSNC.Height(), &dcMem, rcSNC.left, rcSNC.top, SRCCOPY);
}

void CScrollEx64::DrawArrows(CDC * pDC)
{
	CRect rcScroll = GetScrollRect();
	CDC compatDC;
	compatDC.CreateCompatibleDC(pDC);
	compatDC.SelectObject(&m_bmpArrows);

	int iFirstBtnOffsetDrawX, iFirstBtnOffsetDrawY, iFirstBtnWH, iFirstBtnBmpOffsetX, iFirstBtnBmpOffsetY;
	int iLastBtnOffsetDrawX, iLastBtnOffsetDrawY, iLastBtnWH, iLastBtnBmpOffsetX, iLastBtnBmpOffsetY;
	if (IsVert())
	{
		//First arrow button: offsets in bitmap to take from
		//and screen coords to print to.
		iFirstBtnBmpOffsetX = 0;
		iFirstBtnBmpOffsetY = 0;
		iFirstBtnOffsetDrawX = rcScroll.left;
		iFirstBtnOffsetDrawY = rcScroll.top;
		iFirstBtnWH = rcScroll.Width();

		iLastBtnBmpOffsetX = 0;
		iLastBtnBmpOffsetY = m_uiLastArrowOffset;
		iLastBtnOffsetDrawX = rcScroll.left;
		iLastBtnOffsetDrawY = rcScroll.bottom - rcScroll.Width();
		iLastBtnWH = rcScroll.Width();
	}
	else
	{
		iFirstBtnBmpOffsetX = 0;
		iFirstBtnBmpOffsetY = 0;
		iFirstBtnOffsetDrawX = rcScroll.left;
		iFirstBtnOffsetDrawY = rcScroll.top;
		iFirstBtnWH = rcScroll.Height();

		iLastBtnBmpOffsetX = m_uiLastArrowOffset;
		iLastBtnBmpOffsetY = 0;
		iLastBtnOffsetDrawX = rcScroll.right - rcScroll.Height();
		iLastBtnOffsetDrawY = rcScroll.top;
		iLastBtnWH = rcScroll.Height();
	}
	//First arrow button.
	pDC->StretchBlt(iFirstBtnOffsetDrawX, iFirstBtnOffsetDrawY, iFirstBtnWH, iFirstBtnWH,
		&compatDC, iFirstBtnBmpOffsetX, iFirstBtnBmpOffsetY, m_uiArrowSize, m_uiArrowSize, SRCCOPY);

	//Last arrow button.
	pDC->StretchBlt(iLastBtnOffsetDrawX, iLastBtnOffsetDrawY, iLastBtnWH, iLastBtnWH,
		&compatDC, iLastBtnBmpOffsetX, iLastBtnBmpOffsetY, m_uiArrowSize, m_uiArrowSize, SRCCOPY);
}

void CScrollEx64::DrawThumb(CDC* pDC)
{
	CRect rcThumb = GetThumbRect();
	if (!rcThumb.IsRectNull())
		pDC->FillSolidRect(rcThumb, m_clrThumb);
}

CRect CScrollEx64::GetScrollRect(bool fWithNCArea)
{
	if (!m_fCreated)
		return 0;

	CWnd* pwndParent = GetParent();
	CRect rcClient = GetParentRect();
	CRect rcWnd = GetParentRect(false);
	pwndParent->MapWindowPoints(nullptr, &rcClient);

	m_iTopDelta = rcClient.top - rcWnd.top;
	m_iLeftDelta = rcClient.left - rcWnd.left;

	CRect rcScroll;
	if (IsVert())
	{
		rcScroll.left = rcClient.right + m_iLeftDelta;
		rcScroll.top = rcClient.top + m_iTopDelta;
		rcScroll.right = rcScroll.left + m_uiScrollBarSizeWH;
		if (fWithNCArea)
			rcScroll.bottom = rcWnd.bottom + m_iTopDelta;
		else
			rcScroll.bottom = rcScroll.top + rcClient.Height();
	}
	else
	{
		rcScroll.left = rcClient.left + m_iLeftDelta;
		rcScroll.top = rcClient.bottom + m_iTopDelta;
		rcScroll.bottom = rcScroll.top + m_uiScrollBarSizeWH;
		if (fWithNCArea)
			rcScroll.right = rcWnd.right + m_iLeftDelta;
		else
			rcScroll.right = rcScroll.left + rcClient.Width();
	}
	pwndParent->ScreenToClient(&rcScroll);

	return rcScroll;
}

CRect CScrollEx64::GetScrollWorkAreaRect(bool fClientCoord)
{
	CRect rc = GetScrollRect();
	if (IsVert())
		rc.DeflateRect(0, m_uiScrollBarSizeWH, 0, m_uiScrollBarSizeWH);
	else
		rc.DeflateRect(m_uiScrollBarSizeWH, 0, m_uiScrollBarSizeWH, 0);

	if (fClientCoord)
		rc.OffsetRect(-m_iLeftDelta, -m_iTopDelta);

	return rc;
}

UINT CScrollEx64::GetScrollSizeWH()
{
	UINT uiSizeInPixels;

	if (IsVert())
		uiSizeInPixels = GetScrollRect().Height();
	else
		uiSizeInPixels = GetScrollRect().Width();

	return uiSizeInPixels;
}

UINT CScrollEx64::GetScrollWorkAreaSizeWH()
{
	UINT uiScrollSize = GetScrollSizeWH();
	UINT uiScrollWorkArea;
	if (uiScrollSize <= m_uiArrowSize * 2)
		uiScrollWorkArea = 0;
	else
		uiScrollWorkArea = uiScrollSize - m_uiArrowSize * 2; //Minus two arrow's size.

	return uiScrollWorkArea;
}

CRect CScrollEx64::GetThumbRect(bool fClientCoord)
{
	CRect rc { };
	UINT uiThumbSize = GetThumbSizeWH();
	if (!uiThumbSize)
		return rc;

	CRect rcScrollWA = GetScrollWorkAreaRect();
	if (IsVert())
	{
		rc.left = rcScrollWA.left;
		rc.top = rcScrollWA.top + GetThumbPos();
		rc.right = rc.left + m_uiScrollBarSizeWH;
		rc.bottom = rc.top + uiThumbSize;
		if (rc.bottom > rcScrollWA.bottom)
			rc.bottom = rcScrollWA.bottom;
	}
	else
	{
		rc.left = rcScrollWA.left + GetThumbPos();
		rc.top = rcScrollWA.top;
		rc.right = rc.left + uiThumbSize;
		rc.bottom = rc.top + m_uiScrollBarSizeWH;
	}
	if (fClientCoord)
		rc.OffsetRect(-m_iLeftDelta, -m_iTopDelta);

	return rc;
}

UINT CScrollEx64::GetThumbSizeWH()
{
	UINT uiScrollWorkAreaSizeWH = GetScrollWorkAreaSizeWH();
	CRect rcParent = GetParentRect();
	long double dDelta;

	UINT uiThumbSize;
	if (IsVert())
		//dDelta = (long double)uiScrollWorkAreaSizeWH / m_ullScrollSizeMax;
		dDelta = (long double)rcParent.Height() / m_ullScrollSizeMax;
	else
		dDelta = (long double)rcParent.Width() / m_ullScrollSizeMax;

	uiThumbSize = (UINT)std::lroundl(uiScrollWorkAreaSizeWH * dDelta);

	if (uiThumbSize < m_uiThumbSizeMin)
		uiThumbSize = m_uiThumbSizeMin;

	return uiThumbSize;
}

UINT CScrollEx64::GetThumbPos()
{
	ULONGLONG ullScrollPos = GetScrollPos();
	long double dThumbScrollingSize = GetThumbScrollingSize();

	UINT uiThumbPos;
	if (ullScrollPos < dThumbScrollingSize)
		uiThumbPos = 0;
	else
		uiThumbPos = (UINT)std::lroundl(ullScrollPos / dThumbScrollingSize);

	return uiThumbPos;
}

long double CScrollEx64::GetThumbScrollingSize()
{
	if (!m_fCreated)
		return 0;

	UINT uiWAWOThumb = GetScrollWorkAreaSizeWH() - GetThumbSizeWH(); //Work area without thumb.
	int iPage;
	if (IsVert())
		iPage = GetParentRect().Height();
	else
		iPage = GetParentRect().Width();

	return (m_ullScrollSizeMax - iPage) / (long double)uiWAWOThumb;
}

void CScrollEx64::SetThumbPos(int iPos)
{
	CRect rcWorkArea = GetScrollWorkAreaRect();
	UINT uiThumbSize = GetThumbSizeWH();
	ULONGLONG ullNewScrollPos;

	if (iPos < 0)
		ullNewScrollPos = 0;
	else if (iPos == 0x7FFFFFFF)
		ullNewScrollPos = m_ullScrollSizeMax;
	else
	{
		if (IsVert())
		{
			if (iPos + (int)uiThumbSize > rcWorkArea.Height())
				iPos = rcWorkArea.Height() - uiThumbSize;
		}
		else
		{
			if (iPos + (int)uiThumbSize > rcWorkArea.Width())
				iPos = rcWorkArea.Width() - uiThumbSize;
		}
		ullNewScrollPos = (ULONGLONG)std::llroundl(iPos * GetThumbScrollingSize());
	}
	SetScrollPos(ullNewScrollPos);
}

CRect CScrollEx64::GetFirstArrowRect(bool fClientCoord)
{
	CRect rc = GetScrollRect();
	if (IsVert())
		rc.bottom = rc.top + m_uiArrowSize;
	else
		rc.right = rc.left + m_uiArrowSize;

	if (fClientCoord)
		rc.OffsetRect(-m_iLeftDelta, -m_iTopDelta);

	return rc;
}

CRect CScrollEx64::GetLastArrowRect(bool fClientCoord)
{
	CRect rc = GetScrollRect();
	if (IsVert())
		rc.top = rc.bottom - m_uiArrowSize;
	else
		rc.left = rc.right - m_uiArrowSize;

	if (fClientCoord)
		rc.OffsetRect(-m_iLeftDelta, -m_iTopDelta);

	return rc;
}

CRect CScrollEx64::GetFirstChannelRect(bool fClientCoord)
{
	CRect rcThumb = GetThumbRect();
	CRect rcArrow = GetFirstArrowRect();
	CRect rc;
	if (IsVert())
		rc.SetRect(rcArrow.left, rcArrow.bottom, rcArrow.right, rcThumb.top);
	else
		rc.SetRect(rcArrow.right, rcArrow.top, rcThumb.left, rcArrow.bottom);

	if (fClientCoord)
		rc.OffsetRect(-m_iLeftDelta, -m_iTopDelta);

	return rc;
}

CRect CScrollEx64::GetLastChannelRect(bool fClientCoord)
{
	CRect rcThumb = GetThumbRect();
	CRect rcArrow = GetLastArrowRect();
	CRect rc;
	if (IsVert())
		rc.SetRect(rcArrow.left, rcThumb.bottom, rcArrow.right, rcArrow.top);
	else
		rc.SetRect(rcThumb.left, rcArrow.top, rcArrow.left, rcArrow.bottom);

	if (fClientCoord)
		rc.OffsetRect(-m_iLeftDelta, -m_iTopDelta);

	return rc;
}

CRect CScrollEx64::GetParentRect(bool fClient)
{
	CRect rc;
	if (fClient)
		GetParent()->GetClientRect(&rc);
	else
		GetParent()->GetWindowRect(&rc);

	return rc;
}

bool CScrollEx64::IsVert()
{
	return m_iScrollType == SB_VERT ? true : false;
}

bool CScrollEx64::IsThumbDragging()
{
	return m_iScrollBarState == SCROLLEX64::SCROLL_STATE::THUMB_CLICK ? true : false;
}

void CScrollEx64::ResetTimers()
{
	if (m_iScrollBarState > 0)
	{
		m_iScrollBarState = 0;
		KillTimer(SCROLLEX64::SCROLL_TIMERS::IDT_FIRSTCLICK);
		KillTimer(SCROLLEX64::SCROLL_TIMERS::IDT_CLICKREPEAT);
	}
}

void CScrollEx64::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent)
	{
	case SCROLLEX64::SCROLL_TIMERS::IDT_FIRSTCLICK:
		KillTimer(SCROLLEX64::SCROLL_TIMERS::IDT_FIRSTCLICK);
		SetTimer(SCROLLEX64::SCROLL_TIMERS::IDT_CLICKREPEAT, m_iTimerRepeat, nullptr);
		break;
	case SCROLLEX64::SCROLL_TIMERS::IDT_CLICKREPEAT:
	{
		switch (m_iScrollBarState)
		{
		case SCROLLEX64::SCROLL_STATE::FIRSTBUTTON_CLICK:
			ScrollLineUp();
			break;
		case SCROLLEX64::SCROLL_STATE::LASTBUTTON_CLICK:
			ScrollLineDown();
			break;
		case SCROLLEX64::SCROLL_STATE::FIRSTCHANNEL_CLICK:
		{
			CPoint pt;	GetCursorPos(&pt);
			CRect rc = GetThumbRect(true);
			GetParent()->ClientToScreen(rc);
			if (IsVert()) {
				if (pt.y < rc.top)
					ScrollPageUp();
			}
			else {
				if (pt.x < rc.left)
					ScrollPageUp();
			}
		}
		break;
		case SCROLLEX64::SCROLL_STATE::LASTCHANNEL_CLICK:
			CPoint pt;	GetCursorPos(&pt);
			CRect rc = GetThumbRect(true);
			GetParent()->ClientToScreen(rc);
			if (IsVert()) {
				if (pt.y > rc.bottom)
					ScrollPageDown();
			}
			else {
				if (pt.x > rc.right)
					ScrollPageDown();
			}
		}
	}
	break;
	}

	CWnd::OnTimer(nIDEvent);
}

void CScrollEx64::SendParentScrollMsg()
{
	if (!m_fCreated)
		return;

	UINT uiMsg = IsVert() ? WM_VSCROLL : WM_HSCROLL;
	GetParent()->SendMessageW(uiMsg);
}