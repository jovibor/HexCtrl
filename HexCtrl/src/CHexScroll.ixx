/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
module;
#include <SDKDDKVer.h>
#include <Windows.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <memory>
export module HEXCTRL:CHexScroll;

import :HexUtility;

namespace HEXCTRL::INTERNAL {
	class CHexScroll final {
	public:
		void AddSibling(CHexScroll* pSibling);
		bool Create(HWND hWndParent, bool fVert, COLORREF clrBar, COLORREF clrThumb, COLORREF clrArrow,
			ULONGLONG ullLine, ULONGLONG ullPage, ULONGLONG ullSizeMax);
		void DestroyWindow();
		[[nodiscard]] auto GetScrollPos()const -> ULONGLONG;
		[[nodiscard]] auto GetScrollPosDelta()const -> LONGLONG;
		[[nodiscard]] auto GetScrollLineSize()const -> ULONGLONG;
		[[nodiscard]] auto GetScrollPageSize()const -> ULONGLONG;
		[[nodiscard]] auto IsThumbReleased()const -> bool;
		[[nodiscard]] auto IsVisible()const -> bool;

		/************************************************************************************
		* CALLBACK METHODS:                                                                 *
		* These methods must be called from the corresponding methods of the parent window. *
		************************************************************************************/
		void OnLButtonUp();
		void OnMouseMove(POINT pt);
		void OnNCActivate()const;
		void OnNCCalcSize(NCCALCSIZE_PARAMS* pCSP);
		void OnNCPaint()const;
		void OnSetCursor(UINT uHitTest, UINT uMsg);
		/************************************************************************************
		* END OF THE CALLBACK METHODS.                                                      *
		************************************************************************************/

		[[nodiscard]] auto ProcessMsg(const MSG& msg) -> LRESULT;
		void SetColors(COLORREF clrBar, COLORREF clrThumb, COLORREF clrArrow);
		auto SetScrollPos(ULONGLONG ullNewPos) -> ULONGLONG;
		void SetScrollSizes(ULONGLONG ullLine, ULONGLONG ullPage, ULONGLONG ullSizeMax);
		void ScrollEnd();
		void ScrollLineUp();
		void ScrollLineDown();
		void ScrollLineLeft();
		void ScrollLineRight();
		void ScrollPageUp();
		void ScrollPageDown();
		void ScrollPageLeft();
		void ScrollPageRight();
		void ScrollHome();
		void SetScrollPageSize(ULONGLONG ullSize);
	private:
		void CreateArrows();
		void DrawScrollBar()const;     //Draw the whole Scrollbar.
		void DrawArrows(HDC hDC)const; //Draw arrows.
		void DrawThumb(HDC hDC)const;  //Draw the Scroll thumb.
		[[nodiscard]] auto GetParent()const -> GDIUT::CWnd;
		[[nodiscard]] auto GetScrollRect(bool fWithNCArea = false)const -> GDIUT::CRect;          //Scroll's whole rect.
		[[nodiscard]] auto GetScrollWorkAreaRect(bool fClientCoord = false)const -> GDIUT::CRect; //Rect without arrows.
		[[nodiscard]] auto GetScrollSizeWH()const -> UINT;         //Scroll size in pixels, width or height.
		[[nodiscard]] auto GetScrollWorkAreaSizeWH()const -> UINT; //Scroll size (WH) without arrows.
		[[nodiscard]] auto GetThumbRect(bool fClientCoord = false)const -> GDIUT::CRect;
		[[nodiscard]] auto GetThumbSizeWH()const -> UINT;
		[[nodiscard]] int GetThumbPos()const; //Current Thumb pos.
		void SetThumbPos(int iPos);
		[[nodiscard]] auto GetThumbScrollingSize()const -> double;
		[[nodiscard]] auto GetFirstArrowRect(bool fClientCoord = false)const -> GDIUT::CRect;
		[[nodiscard]] auto GetLastArrowRect(bool fClientCoord = false)const -> GDIUT::CRect;
		[[nodiscard]] auto GetFirstChannelRect(bool fClientCoord = false)const -> GDIUT::CRect;
		[[nodiscard]] auto GetLastChannelRect(bool fClientCoord = false)const -> GDIUT::CRect;
		[[nodiscard]] auto GetParentRect(bool fClient = true)const -> GDIUT::CRect;
		[[nodiscard]] int GetTopDelta()const;       //Difference between parent window's Window and Client area. Very important in hit testing.
		[[nodiscard]] int GetLeftDelta()const;
		[[nodiscard]] bool IsVert()const;           //Is vertical or horizontal scrollbar.
		[[nodiscard]] bool IsThumbDragging()const;  //Is the thumb currently dragged by mouse.
		[[nodiscard]] bool IsSiblingVisible()const; //Is sibling scrollbar currently visible or not.
		auto OnDestroy(const MSG& msg) -> LRESULT;
		auto OnTimer(const MSG& msg) -> LRESULT;
		void RedrawNC()const;
		void SendParentScrollMsg()const;            //Sends the WM_(V/H)SCROLL to the parent window.
	private:
		enum class EState : std::uint8_t;
		enum class ETimer : std::uint16_t;
		GDIUT::CWnd m_Wnd;
		GDIUT::CWnd m_WndParent;
		CHexScroll* m_pSibling { };   //Sibling scrollbar, added with AddSibling.
		HBITMAP m_hBmpArrowFirst { }; //Up or Left arrow bitmap.
		HBITMAP m_hBmpArrowLast { };  //Down or Right arrow bitmap.
		POINT m_ptCursorCurr { };     //Cursor's current position.
		ULONGLONG m_ullPosCurr { };   //Current scroll position.
		ULONGLONG m_ullPosPrev { };   //Previous scroll position.
		ULONGLONG m_ullLine { };      //Size of one line scroll, when clicking arrow.
		ULONGLONG m_ullPage { };      //Size of page scroll, when clicking channel.
		ULONGLONG m_ullSizeMax { };   //Maximum scroll size (limit).
		DWORD m_dwBarSizeWH { };      //Scrollbar size (width if vertical, height if horz).
		COLORREF m_clrBar { };        //Scroll bar color.
		COLORREF m_clrThumb { };      //Scroll thumb color.
		COLORREF m_clrArrow { };      //Scroll arrow color.
		EState m_eState { };          //Current state.
		bool m_fCreated { false };    //Main creation flag.
		bool m_fVisible { false };    //Is visible at the moment or not.
		bool m_fVert { };             //Is scrollbar vertical or horizontal?
	};
}

using namespace HEXCTRL::INTERNAL;

enum class CHexScroll::EState : std::uint8_t {
	STATE_DEFAULT, FIRSTARROW_HOVER, FIRSTARROW_CLICK, FIRSTCHANNEL_CLICK, THUMB_HOVER,
	THUMB_CLICK, LASTCHANNEL_CLICK, LASTARROW_CLICK, LASTARROW_HOVER
};

enum class CHexScroll::ETimer : std::uint16_t {
	IDT_FIRSTCLICK = 0x7FF0, IDT_CLICKREPEAT = 0x7FF1
};

void CHexScroll::AddSibling(CHexScroll* pSibling)
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return;
	}

	m_pSibling = pSibling;
}

bool CHexScroll::Create(HWND hWndParent, bool fVert, COLORREF clrBar, COLORREF clrThumb, COLORREF clrArrow,
	ULONGLONG ullLine, ULONGLONG ullPage, ULONGLONG ullSizeMax)
{
	assert(!m_fCreated);
	assert(hWndParent != nullptr);
	if (m_fCreated || hWndParent == nullptr) {
		return false;
	}

	constexpr auto pwszScrollClassName { L"HexCtrl_ScrollBarWnd" };
	if (WNDCLASSEXW wc { }; ::GetClassInfoExW(nullptr, pwszScrollClassName, &wc) == FALSE) {
		wc.cbSize = sizeof(WNDCLASSEXW);
		wc.style = CS_GLOBALCLASS;
		wc.lpfnWndProc = GDIUT::WndProc<CHexScroll>;
		wc.lpszClassName = pwszScrollClassName;
		if (::RegisterClassExW(&wc) == 0) {
			ut::DBG_REPORT(L"RegisterClassExW failed.");
			return false;
		}
	}

	if (m_Wnd.Attach(::CreateWindowExW(0, pwszScrollClassName, nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, nullptr, this));
		m_Wnd.IsNull()) {
		ut::DBG_REPORT(L"CreateWindowExW failed.");
		return false;
	}

	m_fVert = fVert;
	m_WndParent.Attach(hWndParent);
	m_dwBarSizeWH = ::GetSystemMetrics(fVert ? SM_CXVSCROLL : SM_CXHSCROLL);
	m_clrBar = clrBar;
	m_clrThumb = clrThumb;
	m_clrArrow = clrArrow;
	CreateArrows();

	m_fCreated = true;
	SetScrollSizes(ullLine, ullPage, ullSizeMax);

	return true;
}

void CHexScroll::DestroyWindow()
{
	m_Wnd.DestroyWindow();
}

auto CHexScroll::GetScrollPos()const->ULONGLONG
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return { };
	}

	return m_ullPosCurr;
}

auto CHexScroll::GetScrollPosDelta()const->LONGLONG
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return { };
	}

	return static_cast<LONGLONG>(m_ullPosCurr - m_ullPosPrev);
}

auto CHexScroll::GetScrollLineSize()const->ULONGLONG
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return { };
	}

	return m_ullLine;
}

auto CHexScroll::GetScrollPageSize()const->ULONGLONG
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return { };
	}

	return m_ullPage;
}

auto CHexScroll::IsThumbReleased()const->bool
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return false;
	}

	return m_eState != EState::THUMB_CLICK;
}

auto CHexScroll::IsVisible()const->bool
{
	if (!m_fCreated) {
		return false;
	}

	return m_fVisible;
}

void CHexScroll::OnLButtonUp()
{
	assert(m_fCreated);
	if (!m_fCreated || m_eState == EState::STATE_DEFAULT) {
		return;
	}

	m_eState = EState::STATE_DEFAULT;
	SendParentScrollMsg(); //For parent to check IsThumbReleased.
	m_Wnd.KillTimer(static_cast<UINT_PTR>(ETimer::IDT_FIRSTCLICK));
	m_Wnd.KillTimer(static_cast<UINT_PTR>(ETimer::IDT_CLICKREPEAT));
	::ReleaseCapture();
	DrawScrollBar();
}

void CHexScroll::OnMouseMove(POINT pt)
{
	assert(m_fCreated);
	if (!m_fCreated || !IsThumbDragging()) {
		return;
	}

	const auto rc = GetScrollWorkAreaRect(true);
	const auto iCurrPos = GetThumbPos();
	int iNewPos;

	if (IsVert()) {
		if (pt.y < rc.top) {
			iNewPos = 0;
			m_ptCursorCurr.y = rc.top;
		}
		else if (pt.y > rc.bottom) {
			iNewPos = (std::numeric_limits<int>::max)();
			m_ptCursorCurr.y = rc.bottom;
		}
		else {
			iNewPos = iCurrPos + (pt.y - m_ptCursorCurr.y);
			m_ptCursorCurr.y = pt.y;
		}
	}
	else {
		if (pt.x < rc.left) {
			iNewPos = 0;
			m_ptCursorCurr.x = rc.left;
		}
		else if (pt.x > rc.right) {
			iNewPos = (std::numeric_limits<int>::max)();
			m_ptCursorCurr.x = rc.right;
		}
		else {
			iNewPos = iCurrPos + (pt.x - m_ptCursorCurr.x);
			m_ptCursorCurr.x = pt.x;
		}
	}

	if (iNewPos != iCurrPos) {  //Set new thumb pos only if it has been changed.
		SetThumbPos(iNewPos);
	}
}

void CHexScroll::OnNCActivate()const
{
	if (!m_fCreated) {
		return;
	}

	RedrawNC(); //To repaint NC area.
}

void CHexScroll::OnNCCalcSize(NCCALCSIZE_PARAMS* pCSP)
{
	if (!m_fCreated) {
		return;
	}

	const GDIUT::CRect rc = pCSP->rgrc[0];
	const auto ullCurPos = GetScrollPos();
	if (IsVert()) {
		const UINT uiHeight { IsSiblingVisible() ? rc.Height() - m_dwBarSizeWH : rc.Height() };
		if (uiHeight < m_ullSizeMax) {
			m_fVisible = true;
			if (ullCurPos + uiHeight > m_ullSizeMax) {
				SetScrollPos(m_ullSizeMax - uiHeight);
			}
			else {
				DrawScrollBar();
			}
			pCSP->rgrc[0].right -= m_dwBarSizeWH;
		}
		else {
			SetScrollPos(0);
			m_fVisible = false;
		}
	}
	else {
		const UINT uiWidth { IsSiblingVisible() ? rc.Width() - m_dwBarSizeWH : rc.Width() };
		if (uiWidth < m_ullSizeMax) {
			m_fVisible = true;
			if (ullCurPos + uiWidth > m_ullSizeMax) {
				SetScrollPos(m_ullSizeMax - uiWidth);
			}
			else {
				DrawScrollBar();
			}
			pCSP->rgrc[0].bottom -= m_dwBarSizeWH;
		}
		else {
			SetScrollPos(0);
			m_fVisible = false;
		}
	}
}

void CHexScroll::OnNCPaint()const
{
	if (!m_fCreated) {
		return;
	}

	DrawScrollBar();
}

void CHexScroll::OnSetCursor(UINT uHitTest, UINT uMsg)
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return;
	}

	if (uHitTest == HTTOPLEFT || uHitTest == HTLEFT || uHitTest == HTBOTTOMLEFT
		|| uHitTest == HTTOPRIGHT || uHitTest == HTRIGHT || uHitTest == HTBOTTOMRIGHT
		|| uHitTest == HTBOTTOM || uHitTest == HTSIZE || !IsVisible()) {
		return;
	}

	switch (uMsg) {
	case WM_LBUTTONDOWN:
	{
		constexpr auto uTimerFirstClick { 200U }; //Milliseconds for WM_TIMER for first channel click.
		using enum EState; using enum ETimer;
		POINT pt;
		::GetCursorPos(&pt);
		const auto wndParent = GetParent();
		wndParent.ScreenToClient(pt);
		wndParent.SetFocus();

		if (GetThumbRect(true).PtInRect(pt)) {
			m_ptCursorCurr = pt;
			m_eState = THUMB_CLICK;
			wndParent.SetCapture();
		}
		else if (GetFirstArrowRect(true).PtInRect(pt)) {
			ScrollLineUp();
			m_eState = FIRSTARROW_CLICK;
			wndParent.SetCapture();
			m_Wnd.SetTimer(static_cast<UINT_PTR>(IDT_FIRSTCLICK), uTimerFirstClick, nullptr);
		}
		else if (GetLastArrowRect(true).PtInRect(pt)) {
			ScrollLineDown();
			m_eState = LASTARROW_CLICK;
			wndParent.SetCapture();
			m_Wnd.SetTimer(static_cast<UINT_PTR>(IDT_FIRSTCLICK), uTimerFirstClick, nullptr);
		}
		else if (GetFirstChannelRect(true).PtInRect(pt)) {
			ScrollPageUp();
			m_eState = FIRSTCHANNEL_CLICK;
			wndParent.SetCapture();
			m_Wnd.SetTimer(static_cast<UINT_PTR>(IDT_FIRSTCLICK), uTimerFirstClick, nullptr);
		}
		else if (GetLastChannelRect(true).PtInRect(pt)) {
			ScrollPageDown();
			m_eState = LASTCHANNEL_CLICK;
			wndParent.SetCapture();
			m_Wnd.SetTimer(static_cast<UINT_PTR>(IDT_FIRSTCLICK), uTimerFirstClick, nullptr);
		}
	}
	break;
	default:
		break;
	}
}

auto CHexScroll::ProcessMsg(const MSG& msg)->LRESULT
{
	switch (msg.message) {
	case WM_DESTROY: return OnDestroy(msg);
	case WM_TIMER: return OnTimer(msg);
	default: return GDIUT::DefWndProc(msg);
	}
}

void CHexScroll::SetColors(COLORREF clrBar, COLORREF clrThumb, COLORREF clrArrow)
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return;
	}

	m_clrBar = clrBar;
	m_clrThumb = clrThumb;
	m_clrArrow = clrArrow;
	CreateArrows();
	DrawScrollBar();
}

auto CHexScroll::SetScrollPos(ULONGLONG ullNewPos)->ULONGLONG
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return { };
	}

	m_ullPosPrev = m_ullPosCurr;
	if (m_ullPosCurr == ullNewPos) {
		return m_ullPosPrev;
	}

	const auto rc = GetParentRect();
	const auto ullScreenSize { static_cast<ULONGLONG>(IsVert() ? rc.Height() : rc.Width()) };
	const auto ullMax { ullScreenSize > m_ullSizeMax ? 0 : m_ullSizeMax - ullScreenSize };
	m_ullPosCurr = (std::min)(ullNewPos, ullMax);
	SendParentScrollMsg();
	DrawScrollBar();

	return m_ullPosPrev;
}

void CHexScroll::SetScrollSizes(ULONGLONG ullLine, ULONGLONG ullPage, ULONGLONG ullSizeMax)
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return;
	}

	m_ullLine = ullLine;
	m_ullPage = ullPage;
	m_ullSizeMax = ullSizeMax;

	RedrawNC(); //To repaint NC area.
}

void CHexScroll::ScrollEnd()
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return;
	}

	SetScrollPos(m_ullSizeMax);
}

void CHexScroll::ScrollLineUp()
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return;
	}

	const auto ullCur = GetScrollPos();
	SetScrollPos(m_ullLine > ullCur ? 0 : ullCur - m_ullLine);
}

void CHexScroll::ScrollLineDown()
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return;
	}

	const auto ullCur = GetScrollPos();
	const auto ullMax = (std::numeric_limits<ULONGLONG>::max)();
	const auto ullNew { ullMax - ullCur < m_ullLine ? ullMax : ullCur + m_ullLine };
	SetScrollPos(ullNew);
}

void CHexScroll::ScrollLineLeft()
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return;
	}

	ScrollLineUp();
}

void CHexScroll::ScrollLineRight()
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return;
	}

	ScrollLineDown();
}

void CHexScroll::ScrollPageUp()
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return;
	}

	const auto ullCur = GetScrollPos();
	SetScrollPos(m_ullPage > ullCur ? 0 : ullCur - m_ullPage);
}

void CHexScroll::ScrollPageDown()
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return;
	}

	const auto ullCur = GetScrollPos();
	const auto ullMax = (std::numeric_limits<ULONGLONG>::max)();
	SetScrollPos(ullMax - ullCur < m_ullPage ? ullMax : ullCur + m_ullPage);
}

void CHexScroll::ScrollPageLeft()
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return;
	}

	ScrollPageUp();
}

void CHexScroll::ScrollPageRight()
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return;
	}

	ScrollPageDown();
}

void CHexScroll::ScrollHome()
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return;
	}

	SetScrollPos(0);
}

void CHexScroll::SetScrollPageSize(ULONGLONG ullSize)
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return;
	}

	m_ullPage = ullSize;
}


//Private methods.

void CHexScroll::CreateArrows()
{
	::DeleteObject(m_hBmpArrowFirst);
	::DeleteObject(m_hBmpArrowLast);
	const auto wndParent = GetParent();
	const auto hDC = wndParent.GetWindowDC();
	m_hBmpArrowFirst = GDIUT::CreateArrowBitmap(hDC, m_dwBarSizeWH, m_dwBarSizeWH, m_fVert ? 1 : -2, m_clrBar, m_clrArrow);
	m_hBmpArrowLast = GDIUT::CreateArrowBitmap(hDC, m_dwBarSizeWH, m_dwBarSizeWH, m_fVert ? -1 : 2, m_clrBar, m_clrArrow);
	wndParent.ReleaseDC(hDC);
}

void CHexScroll::DrawScrollBar()const
{
	if (!IsVisible()) {
		return;
	}

	const auto wndParent = GetParent();
	const auto hDCParent = wndParent.GetWindowDC();
	const GDIUT::CDC dcMem = ::CreateCompatibleDC(hDCParent);
	const auto rcWnd = GetParentRect(false);
	const auto hBMP = ::CreateCompatibleBitmap(hDCParent, rcWnd.Width(), rcWnd.Height());
	dcMem.SelectObject(hBMP);
	const auto rcSNC = GetScrollRect(true);	//Scroll bar with any additional non client area.
	dcMem.FillSolidRect(rcSNC, m_clrBar);
	const auto rcS = GetScrollRect();
	dcMem.FillSolidRect(rcS, m_clrBar);
	DrawArrows(dcMem);
	DrawThumb(dcMem);

	//Copy the drawn scrollbar from the dcMem to the parent window's DC.
	::BitBlt(hDCParent, rcSNC.left, rcSNC.top, rcSNC.Width(), rcSNC.Height(), dcMem, rcSNC.left, rcSNC.top, SRCCOPY);
	::DeleteObject(hBMP);
	::DeleteDC(dcMem);
	wndParent.ReleaseDC(hDCParent);
}

void CHexScroll::DrawArrows(HDC hDC)const
{
	const auto rcScroll = GetScrollRect();
	const auto iFirstBtnOffsetDrawX = rcScroll.left;
	const auto iFirstBtnOffsetDrawY = rcScroll.top;
	const auto iLastBtnOffsetDrawX = IsVert() ? rcScroll.left : rcScroll.right - rcScroll.Height();
	const auto iLastBtnOffsetDrawY = IsVert() ? rcScroll.bottom - rcScroll.Width() : rcScroll.top;
	const auto hDCMem = ::CreateCompatibleDC(hDC);
	::SelectObject(hDCMem, m_hBmpArrowFirst);
	::BitBlt(hDC, iFirstBtnOffsetDrawX, iFirstBtnOffsetDrawY, m_dwBarSizeWH, m_dwBarSizeWH, hDCMem, 0, 0, SRCCOPY);
	::SelectObject(hDCMem, m_hBmpArrowLast);
	::BitBlt(hDC, iLastBtnOffsetDrawX, iLastBtnOffsetDrawY, m_dwBarSizeWH, m_dwBarSizeWH, hDCMem, 0, 0, SRCCOPY);
	::DeleteDC(hDCMem);
}

void CHexScroll::DrawThumb(HDC hDC)const
{
	const auto rcThumb = GetThumbRect();
	if (!rcThumb.IsRectNull()) {
		GDIUT::CDC(hDC).FillSolidRect(rcThumb, m_clrThumb);
	}
}

auto CHexScroll::GetParent()const->GDIUT::CWnd
{
	return m_WndParent;
}

auto CHexScroll::GetScrollRect(bool fWithNCArea)const->GDIUT::CRect
{
	const auto wndParent = GetParent();
	auto rcClient = GetParentRect();
	wndParent.MapWindowPoints(nullptr, rcClient);
	const auto rcWnd = GetParentRect(false);
	const auto iTopDelta = GetTopDelta();
	const auto iLeftDelta = GetLeftDelta();

	GDIUT::CRect rcScroll;
	if (IsVert()) {
		rcScroll.left = rcClient.right + iLeftDelta;
		rcScroll.top = rcClient.top + iTopDelta;
		rcScroll.right = rcScroll.left + m_dwBarSizeWH;
		if (fWithNCArea) { //Adding difference here to gain equality in coords when call to ScreenToClient below.
			rcScroll.bottom = rcWnd.bottom + iTopDelta;
		}
		else {
			rcScroll.bottom = rcScroll.top + rcClient.Height();
		}
	}
	else {
		rcScroll.left = rcClient.left + iLeftDelta;
		rcScroll.top = rcClient.bottom + iTopDelta;
		rcScroll.bottom = rcScroll.top + m_dwBarSizeWH;
		if (fWithNCArea) {
			rcScroll.right = rcWnd.right + iLeftDelta;
		}
		else {
			rcScroll.right = rcScroll.left + rcClient.Width();
		}
	}
	wndParent.ScreenToClient(rcScroll);

	return rcScroll;
}

auto CHexScroll::GetScrollWorkAreaRect(bool fClientCoord)const->GDIUT::CRect
{
	auto rc = GetScrollRect();
	if (IsVert()) {
		rc.DeflateRect(0, m_dwBarSizeWH, 0, m_dwBarSizeWH);
	}
	else {
		rc.DeflateRect(m_dwBarSizeWH, 0, m_dwBarSizeWH, 0);
	}

	if (fClientCoord) {
		rc.OffsetRect(-GetLeftDelta(), -GetTopDelta());
	}

	return rc;
}

auto CHexScroll::GetScrollSizeWH()const->UINT
{
	return IsVert() ? GetScrollRect().Height() : GetScrollRect().Width();
}

auto CHexScroll::GetScrollWorkAreaSizeWH()const->UINT
{
	const auto uiScrollSize = GetScrollSizeWH();
	return uiScrollSize <= m_dwBarSizeWH * 2 ? 0 : uiScrollSize - (m_dwBarSizeWH * 2); //Minus two arrow's size.
}

auto CHexScroll::GetThumbRect(bool fClientCoord)const->GDIUT::CRect
{
	GDIUT::CRect rc;
	const auto uiThumbSize = GetThumbSizeWH();
	if (!uiThumbSize) {
		return rc;
	}

	const auto rcScrollWA = GetScrollWorkAreaRect();
	if (IsVert()) {
		rc.left = rcScrollWA.left;
		rc.top = rcScrollWA.top + GetThumbPos();
		rc.right = rc.left + m_dwBarSizeWH;
		rc.bottom = rc.top + uiThumbSize;
		rc.bottom = (std::min)(rc.top + static_cast<LONG>(uiThumbSize), rcScrollWA.bottom);
	}
	else {
		rc.left = rcScrollWA.left + GetThumbPos();
		rc.top = rcScrollWA.top;
		rc.right = rc.left + uiThumbSize;
		rc.bottom = rc.top + m_dwBarSizeWH;
	}
	if (fClientCoord) {
		rc.OffsetRect(-GetLeftDelta(), -GetTopDelta());
	}

	return rc;
}

auto CHexScroll::GetThumbSizeWH()const->UINT
{
	constexpr auto uThumbSizeMin = 15U; //Minimum allowed thumb size.
	const auto uiScrollWorkAreaSizeWH = GetScrollWorkAreaSizeWH();
	const auto rcParent = GetParentRect();
	const double dDelta { IsVert() ? static_cast<double>(rcParent.Height()) / m_ullSizeMax :
		static_cast<double>(rcParent.Width()) / m_ullSizeMax };
	const auto uiThumbSize { static_cast<UINT>(std::lroundl(uiScrollWorkAreaSizeWH * dDelta)) };

	return uiThumbSize < uThumbSizeMin ? uThumbSizeMin : uiThumbSize;
}

int CHexScroll::GetThumbPos()const
{
	const auto ullScrollPos = GetScrollPos();
	const auto dThumbScrollingSize = GetThumbScrollingSize();

	return ullScrollPos < dThumbScrollingSize ? 0 : std::lroundl(ullScrollPos / dThumbScrollingSize);
}

auto CHexScroll::GetThumbScrollingSize()const->double
{
	const auto uiWAWOThumb = GetScrollWorkAreaSizeWH() - GetThumbSizeWH(); //Work area without thumb.
	const auto iPage { IsVert() ? GetParentRect().Height() : GetParentRect().Width() };

	return (m_ullSizeMax - iPage) / static_cast<double>(uiWAWOThumb);
}

auto CHexScroll::GetFirstArrowRect(bool fClientCoord)const->GDIUT::CRect
{
	auto rc = GetScrollRect();
	if (IsVert()) {
		rc.bottom = rc.top + m_dwBarSizeWH;
	}
	else {
		rc.right = rc.left + m_dwBarSizeWH;
	}

	if (fClientCoord) {
		rc.OffsetRect(-GetLeftDelta(), -GetTopDelta());
	}

	return rc;
}

auto CHexScroll::GetLastArrowRect(bool fClientCoord)const->GDIUT::CRect
{
	auto rc = GetScrollRect();
	if (IsVert()) {
		rc.top = rc.bottom - m_dwBarSizeWH;
	}
	else {
		rc.left = rc.right - m_dwBarSizeWH;
	}

	if (fClientCoord) {
		rc.OffsetRect(-GetLeftDelta(), -GetTopDelta());
	}

	return rc;
}

auto CHexScroll::GetFirstChannelRect(bool fClientCoord)const->GDIUT::CRect
{
	const auto rcThumb = GetThumbRect();
	const auto rcArrow = GetFirstArrowRect();
	GDIUT::CRect rc;
	if (IsVert()) {
		rc.SetRect(rcArrow.left, rcArrow.bottom, rcArrow.right, rcThumb.top);
	}
	else {
		rc.SetRect(rcArrow.right, rcArrow.top, rcThumb.left, rcArrow.bottom);
	}

	if (fClientCoord) {
		rc.OffsetRect(-GetLeftDelta(), -GetTopDelta());
	}

	return rc;
}

auto CHexScroll::GetLastChannelRect(bool fClientCoord)const->GDIUT::CRect
{
	const auto rcThumb = GetThumbRect();
	const auto rcArrow = GetLastArrowRect();
	GDIUT::CRect rc;
	if (IsVert()) {
		rc.SetRect(rcArrow.left, rcThumb.bottom, rcArrow.right, rcArrow.top);
	}
	else {
		rc.SetRect(rcThumb.left, rcArrow.top, rcArrow.left, rcArrow.bottom);
	}

	if (fClientCoord) {
		rc.OffsetRect(-GetLeftDelta(), -GetTopDelta());
	}

	return rc;
}

auto CHexScroll::GetParentRect(bool fClient)const->GDIUT::CRect
{
	const auto wndParent = GetParent();
	return fClient ? wndParent.GetClientRect() : wndParent.GetWindowRect();
}

int CHexScroll::GetTopDelta()const
{
	auto rcClient = GetParentRect();
	GetParent().MapWindowPoints(nullptr, rcClient);

	return rcClient.top - GetParentRect(false).top;
}

int CHexScroll::GetLeftDelta()const
{
	auto rcClient = GetParentRect();
	GetParent().MapWindowPoints(nullptr, rcClient);

	return rcClient.left - GetParentRect(false).left;
}

bool CHexScroll::IsVert()const
{
	return m_fVert;
}

bool CHexScroll::IsThumbDragging()const
{
	return m_eState == EState::THUMB_CLICK;
}

bool CHexScroll::IsSiblingVisible()const
{
	return m_pSibling ? m_pSibling->IsVisible() : false;
}

auto CHexScroll::OnDestroy(const MSG& msg)->LRESULT
{
	::DeleteObject(m_hBmpArrowFirst);
	::DeleteObject(m_hBmpArrowLast);
	m_hBmpArrowFirst = nullptr;
	m_hBmpArrowLast = nullptr;
	m_fCreated = false;

	return GDIUT::DefWndProc(msg);
}

auto CHexScroll::OnTimer(const MSG& msg)->LRESULT
{
	constexpr auto uTimerRepeat { 50U }; //Milliseconds for repeat when click and hold on channel.
	using enum EState; using enum ETimer;

	switch (msg.wParam) {
	case (static_cast<UINT_PTR>(IDT_FIRSTCLICK)):
		m_Wnd.KillTimer(static_cast<UINT_PTR>(IDT_FIRSTCLICK));
		m_Wnd.SetTimer(static_cast<UINT_PTR>(IDT_CLICKREPEAT), uTimerRepeat, nullptr);
		break;
	case (static_cast<UINT_PTR>(IDT_CLICKREPEAT)):
		switch (m_eState) {
		case FIRSTARROW_CLICK:
			ScrollLineUp();
			break;
		case LASTARROW_CLICK:
			ScrollLineDown();
			break;
		case FIRSTCHANNEL_CLICK:
		{
			POINT pt;
			::GetCursorPos(&pt);
			auto rcThumb = GetThumbRect(true);
			GetParent().ClientToScreen(rcThumb);
			if (IsVert()) {
				if (pt.y < rcThumb.top) {
					ScrollPageUp();
				}
			}
			else {
				if (pt.x < rcThumb.left) {
					ScrollPageUp();
				}
			}
		}
		break;
		case LASTCHANNEL_CLICK:
		{
			POINT pt;
			::GetCursorPos(&pt);
			auto rcThumb = GetThumbRect(true);
			GetParent().ClientToScreen(rcThumb);
			if (IsVert()) {
				if (pt.y > rcThumb.bottom) {
					ScrollPageDown();
				}
			}
			else {
				if (pt.x > rcThumb.right) {
					ScrollPageDown();
				}
			}
		}
		break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return 0;
}

void CHexScroll::RedrawNC()const
{
	//To repaint NC area.
	if (const auto wndParent = GetParent(); !wndParent.IsNull()) {
		wndParent.SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}
}

void CHexScroll::SendParentScrollMsg()const
{
	GetParent().SendMsg(IsVert() ? WM_VSCROLL : WM_HSCROLL);
}

void CHexScroll::SetThumbPos(int iPos)
{
	if (iPos == GetThumbPos()) {
		return;
	}

	const auto rcWorkArea = GetScrollWorkAreaRect();
	const auto uiThumbSize = GetThumbSizeWH();
	ULONGLONG ullNewScrollPos;

	if (iPos < 0) {
		ullNewScrollPos = 0;
	}
	else if (iPos == (std::numeric_limits<int>::max)()) {
		ullNewScrollPos = m_ullSizeMax;
	}
	else {
		if (IsVert()) {
			if (iPos + static_cast<int>(uiThumbSize) > rcWorkArea.Height()) {
				iPos = rcWorkArea.Height() - uiThumbSize;
			}
		}
		else {
			if (iPos + static_cast<int>(uiThumbSize) > rcWorkArea.Width()) {
				iPos = rcWorkArea.Width() - uiThumbSize;
			}
		}
		ullNewScrollPos = static_cast<ULONGLONG>(std::llroundl(iPos * GetThumbScrollingSize()));
	}

	SetScrollPos(ullNewScrollPos);
}