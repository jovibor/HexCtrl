/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/                         *
* This is a Hex Control for MFC applications.                                           *
* Official git repository of the project: https://github.com/jovibor/HexCtrl/           *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
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
#include "stdafx.h"
#include <cmath>
#include <cassert>
#include "CScrollEx.h"
#include "../res/HexCtrlRes.h"

using namespace HEXCTRL::INTERNAL::SCROLLEX;

namespace HEXCTRL::INTERNAL {
	namespace SCROLLEX {
		enum class EState : DWORD
		{
			STATE_DEFAULT,
			FIRSTARROW_HOVER, FIRSTARROW_CLICK,
			FIRSTCHANNEL_CLICK,
			THUMB_HOVER, THUMB_CLICK,
			LASTCHANNEL_CLICK,
			LASTARROW_CLICK, LASTARROW_HOVER
		};
		enum class ETimer : UINT_PTR {
			IDT_FIRSTCLICK = 0x7ff0,
			IDT_CLICKREPEAT = 0x7ff1
		};

		constexpr auto THUMB_POS_MAX = 0x7fffffff;
	}
}

/****************************************************
* CScrollEx class implementation.					*
****************************************************/
BEGIN_MESSAGE_MAP(CScrollEx, CWnd)
	ON_WM_TIMER()
END_MESSAGE_MAP()

bool CScrollEx::Create(CWnd * pWndParent, int iScrollType,
	ULONGLONG ullScrolline, ULONGLONG ullScrollPage, ULONGLONG ullScrollSizeMax)
{
	assert(!m_fCreated); //Already created
	assert(pWndParent);
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

void CScrollEx::AddSibling(CScrollEx* pSibling)
{
	m_pSibling = pSibling;
}

bool CScrollEx::IsVisible() const
{
	return m_fVisible;
}

CWnd * CScrollEx::GetParent() const
{
	return m_pwndParent;
}

void CScrollEx::SetScrollSizes(ULONGLONG ullScrolline, ULONGLONG ullScrollPage, ULONGLONG ullScrollSizeMax)
{
	assert(m_fCreated);
	if (!m_fCreated)
		return;

	m_ullScrollLine = ullScrolline;
	m_ullScrollPage = ullScrollPage;
	m_ullScrollSizeMax = ullScrollSizeMax;

	CWnd* pWnd = GetParent();
	if (pWnd) //To repaint NC area.
		pWnd->SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
}

ULONGLONG CScrollEx::SetScrollPos(ULONGLONG ullNewPos)
{
	assert(m_fCreated);
	if (!m_fCreated)
		return 0;

	m_ullScrollPosPrev = m_ullScrollPosCur;
	if (m_ullScrollPosCur == ullNewPos)
		return m_ullScrollPosPrev;

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

void CScrollEx::ScrollLineDown()
{
	ULONGLONG ullCur = GetScrollPos();
	ULONGLONG ullNew;
	if (ULONGLONG_MAX - ullCur < m_ullScrollLine) //To avoid overflow.
		ullNew = ULONGLONG_MAX;
	else
		ullNew = ullCur + m_ullScrollLine;
	SetScrollPos(ullNew);
}

void CScrollEx::ScrollLineRight()
{
	ScrollLineDown();
}

void CScrollEx::ScrollLineUp()
{
	ULONGLONG ullCur = GetScrollPos();
	ULONGLONG ullNew;
	if (m_ullScrollLine > ullCur) //To avoid overflow.
		ullNew = 0;
	else
		ullNew = ullCur - m_ullScrollLine;
	SetScrollPos(ullNew);
}

void CScrollEx::ScrollLineLeft()
{
	ScrollLineUp();
}

void CScrollEx::ScrollPageDown()
{
	ULONGLONG ullCur = GetScrollPos();
	ULONGLONG ullNew;
	if (ULONGLONG_MAX - ullCur < m_ullScrollPage) //To avoid overflow.
		ullNew = ULONGLONG_MAX;
	else
		ullNew = ullCur + m_ullScrollPage;
	SetScrollPos(ullNew);
}

void CScrollEx::ScrollPageLeft()
{
	ScrollPageUp();
}

void CScrollEx::ScrollPageRight()
{
	ScrollPageDown();
}

void CScrollEx::ScrollPageUp()
{
	ULONGLONG ullCur = GetScrollPos();
	ULONGLONG ullNew;
	if (m_ullScrollPage > ullCur) //To avoid overflow.
		ullNew = 0;
	else
		ullNew = ullCur - m_ullScrollPage;
	SetScrollPos(ullNew);
}

void CScrollEx::ScrollHome()
{
	SetScrollPos(0);
}

void CScrollEx::ScrollEnd()
{
	SetScrollPos(m_ullScrollSizeMax);
}

ULONGLONG CScrollEx::GetScrollPos()const
{
	if (!m_fCreated)
		return 0;

	return m_ullScrollPosCur;
}

LONGLONG CScrollEx::GetScrollPosDelta()const
{
	if (!m_fCreated)
		return 0;

	return LONGLONG(m_ullScrollPosCur - m_ullScrollPosPrev);
}

ULONGLONG CScrollEx::GetScrollLineSize()const
{
	return m_ullScrollLine;
}

ULONGLONG CScrollEx::GetScrollPageSize()const
{
	return m_ullScrollPage;
}

void CScrollEx::SetScrollPageSize(ULONGLONG ullSize)
{
	m_ullScrollPage = ullSize;
}

BOOL CScrollEx::OnNcActivate(BOOL /*bActive*/)const
{
	if (!m_fCreated)
		return FALSE;

	//To repaint NC area.
	GetParent()->SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

	return TRUE;
}

void CScrollEx::OnNcCalcSize(BOOL /*bCalcValidRects*/, NCCALCSIZE_PARAMS * lpncsp)
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

void CScrollEx::OnNcPaint()const
{
	if (!m_fCreated)
		return;

	DrawScrollBar();
}

void CScrollEx::OnSetCursor(CWnd * /*pWnd*/, UINT nHitTest, UINT message)
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
			CWnd* pParent = GetParent();
			pParent->SetFocus();

			if (GetThumbRect(true).PtInRect(pt))
			{
				m_ptCursorCur = pt;
				m_enState = EState::THUMB_CLICK;
				pParent->SetCapture();
			}
			else if (GetFirstArrowRect(true).PtInRect(pt))
			{
				ScrollLineUp();
				m_enState = EState::FIRSTARROW_CLICK;
				pParent->SetCapture();
				SetTimer((UINT_PTR)ETimer::IDT_FIRSTCLICK, m_iTimerFirstClick, nullptr);
			}
			else if (GetLastArrowRect(true).PtInRect(pt))
			{
				ScrollLineDown();
				m_enState = EState::LASTARROW_CLICK;
				pParent->SetCapture();
				SetTimer((UINT_PTR)ETimer::IDT_FIRSTCLICK, m_iTimerFirstClick, nullptr);
			}
			else if (GetFirstChannelRect(true).PtInRect(pt))
			{
				ScrollPageUp();
				m_enState = EState::FIRSTCHANNEL_CLICK;
				pParent->SetCapture();
				SetTimer((UINT_PTR)ETimer::IDT_FIRSTCLICK, m_iTimerFirstClick, nullptr);
			}
			else if (GetLastChannelRect(true).PtInRect(pt))
			{
				ScrollPageDown();
				m_enState = EState::LASTCHANNEL_CLICK;
				pParent->SetCapture();
				SetTimer((UINT_PTR)ETimer::IDT_FIRSTCLICK, m_iTimerFirstClick, nullptr);
			}
		}
	}
	break;
	}
}

void CScrollEx::OnMouseMove(UINT /*nFlags*/, CPoint point)
{
	if (!m_fCreated || !IsThumbDragging())
		return;

	CRect rc = GetScrollWorkAreaRect(true);
	int iCurrPos = GetThumbPos();
	int iNewPos;

	if (IsVert())
	{
		if (point.y < rc.top)
		{
			iNewPos = 0;
			m_ptCursorCur.y = rc.top;
		}
		else if (point.y > rc.bottom)
		{
			iNewPos = THUMB_POS_MAX;
			m_ptCursorCur.y = rc.bottom;
		}
		else
		{
			iNewPos = iCurrPos + (point.y - m_ptCursorCur.y);
			m_ptCursorCur.y = point.y;
		}
	}
	else
	{
		if (point.x < rc.left)
		{
			iNewPos = 0;
			m_ptCursorCur.x = rc.left;
		}
		else if (point.x > rc.right)
		{
			iNewPos = THUMB_POS_MAX;
			m_ptCursorCur.x = rc.right;
		}
		else
		{
			iNewPos = iCurrPos + (point.x - m_ptCursorCur.x);
			m_ptCursorCur.x = point.x;
		}
	}

	if (iNewPos != iCurrPos)  //Set new thumb pos only if it has been changed.
		SetThumbPos(iNewPos);
}

void CScrollEx::OnLButtonUp(UINT /*nFlags*/, CPoint /*point*/)
{
	if (!m_fCreated)
		return;

	if (m_enState != EState::STATE_DEFAULT)
	{
		m_enState = EState::STATE_DEFAULT;
		KillTimer((UINT_PTR)ETimer::IDT_FIRSTCLICK);
		KillTimer((UINT_PTR)ETimer::IDT_CLICKREPEAT);
		ReleaseCapture();
		DrawScrollBar();
	}
}

void CScrollEx::DrawScrollBar()const
{
	if (!IsVisible())
		return;

	CWnd* hwndParent = GetParent();
	CWindowDC parentDC(hwndParent);

	CDC dcMem;
	CBitmap bitmap;
	dcMem.CreateCompatibleDC(&parentDC);
	CRect rc = GetParentRect(false);
	bitmap.CreateCompatibleBitmap(&parentDC, rc.Width(), rc.Height());
	dcMem.SelectObject(&bitmap);
	CDC* pDC = &dcMem;

	CRect rcSNC = GetScrollRect(true);		//Scroll bar with any additional non client area, to fill it below.
	pDC->FillSolidRect(&rcSNC, m_clrBkNC);	//Scroll bar with NC Bk.
	CRect rcS = GetScrollRect();
	pDC->FillSolidRect(&rcS, m_clrBkScrollBar); //Scroll bar Bk.
	DrawArrows(pDC);
	DrawThumb(pDC);

	//Copy drawn Scrollbar from dcMem to parent window.
	parentDC.BitBlt(rcSNC.left, rcSNC.top, rcSNC.Width(), rcSNC.Height(), &dcMem, rcSNC.left, rcSNC.top, SRCCOPY);
}

void CScrollEx::DrawArrows(CDC * pDC)const
{
	CRect rcScroll = GetScrollRect();
	CDC compatDC;
	compatDC.CreateCompatibleDC(pDC);
	compatDC.SelectObject(m_bmpArrows);

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

void CScrollEx::DrawThumb(CDC * pDC)const
{
	CRect rcThumb = GetThumbRect();
	if (!rcThumb.IsRectNull())
		pDC->FillSolidRect(rcThumb, m_clrThumb);
}

CRect CScrollEx::GetScrollRect(bool fWithNCArea)const
{
	if (!m_fCreated)
		return { };

	CWnd* hwndParent = GetParent();
	CRect rcClient = GetParentRect();
	CRect rcWnd = GetParentRect(false);
	hwndParent->MapWindowPoints(nullptr, &rcClient);

	int iTopDelta = GetTopDelta();
	int iLeftDelta = GetLeftDelta();

	CRect rcScroll;
	if (IsVert())
	{
		rcScroll.left = rcClient.right + iLeftDelta;
		rcScroll.top = rcClient.top + iTopDelta;
		rcScroll.right = rcScroll.left + m_uiScrollBarSizeWH;
		if (fWithNCArea) //Adding difference here to gain equality in coords when call to hwndParent->ScreenToClient below.
			rcScroll.bottom = rcWnd.bottom + iTopDelta;
		else
			rcScroll.bottom = rcScroll.top + rcClient.Height();
	}
	else
	{
		rcScroll.left = rcClient.left + iLeftDelta;
		rcScroll.top = rcClient.bottom + iTopDelta;
		rcScroll.bottom = rcScroll.top + m_uiScrollBarSizeWH;
		if (fWithNCArea)
			rcScroll.right = rcWnd.right + iLeftDelta;
		else
			rcScroll.right = rcScroll.left + rcClient.Width();
	}
	hwndParent->ScreenToClient(&rcScroll);

	return rcScroll;
}

CRect CScrollEx::GetScrollWorkAreaRect(bool fClientCoord)const
{
	CRect rc = GetScrollRect();
	if (IsVert())
		rc.DeflateRect(0, m_uiScrollBarSizeWH, 0, m_uiScrollBarSizeWH);
	else
		rc.DeflateRect(m_uiScrollBarSizeWH, 0, m_uiScrollBarSizeWH, 0);

	if (fClientCoord)
		rc.OffsetRect(-GetLeftDelta(), -GetTopDelta());

	return rc;
}

UINT CScrollEx::GetScrollSizeWH()const
{
	UINT uiSizeInPixels;

	if (IsVert())
		uiSizeInPixels = GetScrollRect().Height();
	else
		uiSizeInPixels = GetScrollRect().Width();

	return uiSizeInPixels;
}

UINT CScrollEx::GetScrollWorkAreaSizeWH()const
{
	UINT uiScrollSize = GetScrollSizeWH();
	UINT uiScrollWorkArea;
	if (uiScrollSize <= m_uiArrowSize * 2)
		uiScrollWorkArea = 0;
	else
		uiScrollWorkArea = uiScrollSize - m_uiArrowSize * 2; //Minus two arrow's size.

	return uiScrollWorkArea;
}

CRect CScrollEx::GetThumbRect(bool fClientCoord)const
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
		rc.OffsetRect(-GetLeftDelta(), -GetTopDelta());

	return rc;
}

UINT CScrollEx::GetThumbSizeWH()const
{
	UINT uiScrollWorkAreaSizeWH = GetScrollWorkAreaSizeWH();
	CRect rcParent = GetParentRect();
	long double dDelta;

	UINT uiThumbSize;
	if (IsVert())
		dDelta = (long double)rcParent.Height() / m_ullScrollSizeMax;
	else
		dDelta = (long double)rcParent.Width() / m_ullScrollSizeMax;

	uiThumbSize = (UINT)std::lroundl(uiScrollWorkAreaSizeWH * dDelta);

	if (uiThumbSize < m_uiThumbSizeMin)
		uiThumbSize = m_uiThumbSizeMin;

	return uiThumbSize;
}

int CScrollEx::GetThumbPos()const
{
	ULONGLONG ullScrollPos = GetScrollPos();
	long double dThumbScrollingSize = GetThumbScrollingSize();

	int iThumbPos;
	if (ullScrollPos < dThumbScrollingSize)
		iThumbPos = 0;
	else
		iThumbPos = std::lroundl(ullScrollPos / dThumbScrollingSize);

	return iThumbPos;
}

void CScrollEx::SetThumbPos(int iPos)
{
	if (iPos == GetThumbPos())
		return;

	CRect rcWorkArea = GetScrollWorkAreaRect();
	UINT uiThumbSize = GetThumbSizeWH();
	ULONGLONG ullNewScrollPos;

	if (iPos < 0)
		ullNewScrollPos = 0;
	else if (iPos == THUMB_POS_MAX)
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

long double CScrollEx::GetThumbScrollingSize()const
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

CRect CScrollEx::GetFirstArrowRect(bool fClientCoord)const
{
	CRect rc = GetScrollRect();
	if (IsVert())
		rc.bottom = rc.top + m_uiArrowSize;
	else
		rc.right = rc.left + m_uiArrowSize;

	if (fClientCoord)
		rc.OffsetRect(-GetLeftDelta(), -GetTopDelta());

	return rc;
}

CRect CScrollEx::GetLastArrowRect(bool fClientCoord)const
{
	CRect rc = GetScrollRect();
	if (IsVert())
		rc.top = rc.bottom - m_uiArrowSize;
	else
		rc.left = rc.right - m_uiArrowSize;

	if (fClientCoord)
		rc.OffsetRect(-GetLeftDelta(), -GetTopDelta());

	return rc;
}

CRect CScrollEx::GetFirstChannelRect(bool fClientCoord)const
{
	CRect rcThumb = GetThumbRect();
	CRect rcArrow = GetFirstArrowRect();
	CRect rc;
	if (IsVert())
		rc.SetRect(rcArrow.left, rcArrow.bottom, rcArrow.right, rcThumb.top);
	else
		rc.SetRect(rcArrow.right, rcArrow.top, rcThumb.left, rcArrow.bottom);

	if (fClientCoord)
		rc.OffsetRect(-GetLeftDelta(), -GetTopDelta());

	return rc;
}

CRect CScrollEx::GetLastChannelRect(bool fClientCoord)const
{
	CRect rcThumb = GetThumbRect();
	CRect rcArrow = GetLastArrowRect();
	CRect rc;
	if (IsVert())
		rc.SetRect(rcArrow.left, rcThumb.bottom, rcArrow.right, rcArrow.top);
	else
		rc.SetRect(rcThumb.left, rcArrow.top, rcArrow.left, rcArrow.bottom);

	if (fClientCoord)
		rc.OffsetRect(-GetLeftDelta(), -GetTopDelta());

	return rc;
}

CRect CScrollEx::GetParentRect(bool fClient)const
{
	CRect rc;
	if (fClient)
		GetParent()->GetClientRect(&rc);
	else
		GetParent()->GetWindowRect(&rc);

	return rc;
}

int CScrollEx::GetTopDelta() const
{
	CRect rcClient = GetParentRect();
	GetParent()->MapWindowPoints(nullptr, &rcClient);

	return rcClient.top - GetParentRect(false).top;
}

int CScrollEx::GetLeftDelta() const
{
	CRect rcClient = GetParentRect();
	GetParent()->MapWindowPoints(nullptr, &rcClient);

	return rcClient.left - GetParentRect(false).left;
}

bool CScrollEx::IsVert()const
{
	return m_iScrollType == SB_VERT;
}

bool CScrollEx::IsThumbDragging()const
{
	return m_enState == EState::THUMB_CLICK;
}

bool CScrollEx::IsSiblingVisible()const
{
	if (m_pSibling)
		return m_pSibling->IsVisible();

	return false;
}


void CScrollEx::SendParentScrollMsg()const
{
	if (!m_fCreated)
		return;

	GetParent()->SendMessageW(IsVert() ? WM_VSCROLL : WM_HSCROLL);
}

void CScrollEx::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent)
	{
	case (UINT_PTR)ETimer::IDT_FIRSTCLICK:
		KillTimer((UINT_PTR)ETimer::IDT_FIRSTCLICK);
		SetTimer((UINT_PTR)ETimer::IDT_CLICKREPEAT, m_iTimerRepeat, nullptr);
		break;
	case (UINT_PTR)ETimer::IDT_CLICKREPEAT:
	{
		switch (m_enState)
		{
		case EState::FIRSTARROW_CLICK:
			ScrollLineUp();
			break;
		case EState::LASTARROW_CLICK:
			ScrollLineDown();
			break;
		case EState::FIRSTCHANNEL_CLICK:
		{
			CPoint pt;
			GetCursorPos(&pt);
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
		case EState::LASTCHANNEL_CLICK:
			CPoint pt;
			GetCursorPos(&pt);
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
			break;
		}
	}
	break;
	}

	CWnd::OnTimer(nIDEvent);
}