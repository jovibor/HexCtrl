module;
/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include <SDKDDKVer.h>
#include <windows.h>
#include <cassert>
#include <cmath>
#include <limits>
#include <memory>
export module HEXCTRL.CHexScroll;

import HEXCTRL.HexUtility;

namespace HEXCTRL::INTERNAL {
	export class CHexScroll final {
	public:
		void AddSibling(CHexScroll* pSibling);
		bool Create(HWND hWndParent, bool fVert, HINSTANCE hInstRes, UINT uIDArrow, ULONGLONG ullScrolline,
			ULONGLONG ullScrollPage, ULONGLONG ullScrollSizeMax);
		bool Create(HWND hWndParent, bool fVert, HBITMAP hArrow, ULONGLONG ullScrolline,
			ULONGLONG ullScrollPage, ULONGLONG ullScrollSizeMax);
		void DestroyWindow();
		[[nodiscard]] auto GetScrollPos()const->ULONGLONG;
		[[nodiscard]] auto GetScrollPosDelta()const->LONGLONG;
		[[nodiscard]] auto GetScrollLineSize()const->ULONGLONG;
		[[nodiscard]] auto GetScrollPageSize()const->ULONGLONG;
		[[nodiscard]] auto IsThumbReleased()const->bool;
		[[nodiscard]] auto IsVisible()const->bool;

		/************************************************************************************
		* CALLBACK METHODS:                                                                 *
		* These methods must be called from the corresponding methods of the parent window. *
		************************************************************************************/
		void OnLButtonUp();
		void OnMouseMove(POINT pt);
		void OnNcActivate()const;
		void OnNcCalcSize(NCCALCSIZE_PARAMS* pCSP);
		void OnNcPaint()const;
		void OnSetCursor(UINT uHitTest, UINT uMsg);
		/************************************************************************************
		* END OF THE CALLBACK METHODS.                                                      *
		************************************************************************************/

		[[nodiscard]] auto ProcessMsg(const MSG& stMsg) -> LRESULT;
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
		[[nodiscard]] bool CreateArrows(HBITMAP hArrow, bool fVert);
		void DrawScrollBar()const;     //Draw the whole Scrollbar.
		void DrawArrows(HDC hDC)const; //Draw arrows.
		void DrawThumb(HDC hDC)const;  //Draw the Scroll thumb.
		[[nodiscard]] auto GetParent()const->wnd::CWnd;
		[[nodiscard]] auto GetScrollRect(bool fWithNCArea = false)const->wnd::CRect;          //Scroll's whole rect.
		[[nodiscard]] auto GetScrollWorkAreaRect(bool fClientCoord = false)const->wnd::CRect; //Rect without arrows.
		[[nodiscard]] auto GetScrollSizeWH()const->UINT;                                 //Scroll size in pixels, width or height.
		[[nodiscard]] auto GetScrollWorkAreaSizeWH()const->UINT;                         //Scroll size (WH) without arrows.
		[[nodiscard]] auto GetThumbRect(bool fClientCoord = false)const->wnd::CRect;
		[[nodiscard]] auto GetThumbSizeWH()const->UINT;
		[[nodiscard]] int GetThumbPos()const;                                      //Current Thumb pos.
		void SetThumbPos(int iPos);
		[[nodiscard]] auto GetThumbScrollingSize()const->double;
		[[nodiscard]] auto GetFirstArrowRect(bool fClientCoord = false)const->wnd::CRect;
		[[nodiscard]] auto GetLastArrowRect(bool fClientCoord = false)const->wnd::CRect;
		[[nodiscard]] auto GetFirstChannelRect(bool fClientCoord = false)const->wnd::CRect;
		[[nodiscard]] auto GetLastChannelRect(bool fClientCoord = false)const->wnd::CRect;
		[[nodiscard]] auto GetParentRect(bool fClient = true)const->wnd::CRect;
		[[nodiscard]] int GetTopDelta()const;       //Difference between parent window's Window and Client area. Very important in hit testing.
		[[nodiscard]] int GetLeftDelta()const;
		[[nodiscard]] bool IsVert()const;           //Is vertical or horizontal scrollbar.
		[[nodiscard]] bool IsThumbDragging()const;  //Is the thumb currently dragged by mouse.
		[[nodiscard]] bool IsSiblingVisible()const; //Is sibling scrollbar currently visible or not.
		auto OnDestroy(const MSG& stMsg) -> LRESULT;
		auto OnTimer(const MSG& stMsg) -> LRESULT;
		void RedrawNC()const;
		void SendParentScrollMsg()const;            //Sends the WM_(V/H)SCROLL to the parent window.
	private:
		static constexpr auto m_iThumbPosMax { 0x7FFFFFFF };
		enum class EState : std::uint8_t;
		enum class ETimer : std::uint16_t;
		CHexScroll* m_pSibling { };       //Sibling scrollbar, added with AddSibling.
		wnd::CWnd m_Wnd;                  //Main window.
		wnd::CWnd m_WndParent;            //Parent window.
		HBITMAP m_hBmpArrowFirst { };     //Up or Left arrow bitmap.
		HBITMAP m_hBmpArrowLast { };      //Down or Right arrow bitmap.
		ULONGLONG m_ullScrollPosCur { };  //Current scroll position.
		ULONGLONG m_ullScrollPosPrev { }; //Previous scroll position.
		ULONGLONG m_ullScrollLine { };    //Size of one line scroll, when clicking arrow.
		ULONGLONG m_ullScrollPage { };    //Size of page scroll, when clicking channel.
		ULONGLONG m_ullScrollSizeMax { }; //Maximum scroll size (limit).
		UINT m_uiScrollBarSizeWH { };     //Scrollbar size (width if vertical, height if horz).
		POINT m_ptCursorCur { };          //Cursor's current position.
		int m_iArrowRCSizePx { };         //Arrow bitmap side size (it's a square) in pixels.
		EState m_eState { };              //Current state.
		bool m_fCreated { false };        //Main creation flag.
		bool m_fVisible { false };        //Is visible at the moment or not.
		bool m_fScrollVert { };           //Scrollbar type, horizontal or vertical.
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

bool CHexScroll::Create(HWND hWndParent, bool fVert, HINSTANCE hInstRes, UINT uIDArrow, ULONGLONG ullScrolline,
	ULONGLONG ullScrollPage, ULONGLONG ullScrollSizeMax)
{
	assert(!m_fCreated); //Already created.
	assert(hWndParent != nullptr);
	if (m_fCreated || hWndParent == nullptr) {
		return false;
	}

	const auto hBMP = static_cast<HBITMAP>(::LoadImageW(hInstRes, MAKEINTRESOURCEW(uIDArrow),
		IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
	if (hBMP == nullptr) {
		DBG_REPORT(L"LoadImageW failed.");
		return false;
	}

	const auto ret = Create(hWndParent, fVert, hBMP, ullScrolline, ullScrollPage, ullScrollSizeMax);;
	::DeleteObject(hBMP);

	return ret;
}

bool CHexScroll::Create(HWND hWndParent, bool fVert, HBITMAP hArrow, ULONGLONG ullScrolline,
	ULONGLONG ullScrollPage, ULONGLONG ullScrollSizeMax)
{
	assert(!m_fCreated); //Already created.
	assert(hWndParent != nullptr);
	if (m_fCreated || hWndParent == nullptr) {
		return false;
	}

	constexpr auto m_pwszScrollClassName { L"HexCtrl_ScrollBarWnd" };
	if (WNDCLASSEXW wc { }; GetClassInfoExW(nullptr, m_pwszScrollClassName, &wc) == FALSE) {
		wc.cbSize = sizeof(WNDCLASSEXW);
		wc.style = CS_GLOBALCLASS;
		wc.lpfnWndProc = wnd::WndProc<CHexScroll>;
		wc.lpszClassName = m_pwszScrollClassName;
		if (RegisterClassExW(&wc) == 0) {
			DBG_REPORT(L"RegisterClassExW failed.");
			return false;
		}
	}

	if (m_Wnd.Attach(::CreateWindowExW(0, m_pwszScrollClassName, nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, nullptr, this));
		m_Wnd.IsNull()) {
		DBG_REPORT(L"CreateWindowExW failed.");
		return false;
	}

	m_fScrollVert = fVert;
	m_WndParent.Attach(hWndParent);
	m_uiScrollBarSizeWH = GetSystemMetrics(fVert ? SM_CXVSCROLL : SM_CXHSCROLL);

	if (!CreateArrows(hArrow, fVert)) {
		DBG_REPORT(L"CreateArrows failed.");
		return false;
	}

	m_fCreated = true;
	SetScrollSizes(ullScrolline, ullScrollPage, ullScrollSizeMax);

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

	return m_ullScrollPosCur;
}

auto CHexScroll::GetScrollPosDelta()const->LONGLONG
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return { };
	}

	return static_cast<LONGLONG>(m_ullScrollPosCur - m_ullScrollPosPrev);
}

auto CHexScroll::GetScrollLineSize()const->ULONGLONG
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return { };
	}

	return m_ullScrollLine;
}

auto CHexScroll::GetScrollPageSize()const->ULONGLONG
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return { };
	}

	return m_ullScrollPage;
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
	ReleaseCapture();
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
			m_ptCursorCur.y = rc.top;
		}
		else if (pt.y > rc.bottom) {
			iNewPos = m_iThumbPosMax;
			m_ptCursorCur.y = rc.bottom;
		}
		else {
			iNewPos = iCurrPos + (pt.y - m_ptCursorCur.y);
			m_ptCursorCur.y = pt.y;
		}
	}
	else {
		if (pt.x < rc.left) {
			iNewPos = 0;
			m_ptCursorCur.x = rc.left;
		}
		else if (pt.x > rc.right) {
			iNewPos = m_iThumbPosMax;
			m_ptCursorCur.x = rc.right;
		}
		else {
			iNewPos = iCurrPos + (pt.x - m_ptCursorCur.x);
			m_ptCursorCur.x = pt.x;
		}
	}

	if (iNewPos != iCurrPos) {  //Set new thumb pos only if it has been changed.
		SetThumbPos(iNewPos);
	}
}

void CHexScroll::OnNcActivate()const
{
	if (!m_fCreated) {
		return;
	}

	RedrawNC(); //To repaint NC area.
}

void CHexScroll::OnNcCalcSize(NCCALCSIZE_PARAMS* pCSP)
{
	if (!m_fCreated) {
		return;
	}

	const wnd::CRect rc = pCSP->rgrc[0];
	const auto ullCurPos = GetScrollPos();
	if (IsVert()) {
		const UINT uiHeight { IsSiblingVisible() ? rc.Height() - m_uiScrollBarSizeWH : rc.Height() };
		if (uiHeight < m_ullScrollSizeMax) {
			m_fVisible = true;
			if (ullCurPos + uiHeight > m_ullScrollSizeMax) {
				SetScrollPos(m_ullScrollSizeMax - uiHeight);
			}
			else {
				DrawScrollBar();
			}
			pCSP->rgrc[0].right -= m_uiScrollBarSizeWH;
		}
		else {
			SetScrollPos(0);
			m_fVisible = false;
		}
	}
	else {
		const UINT uiWidth { IsSiblingVisible() ? rc.Width() - m_uiScrollBarSizeWH : rc.Width() };
		if (uiWidth < m_ullScrollSizeMax) {
			m_fVisible = true;
			if (ullCurPos + uiWidth > m_ullScrollSizeMax) {
				SetScrollPos(m_ullScrollSizeMax - uiWidth);
			}
			else {
				DrawScrollBar();
			}
			pCSP->rgrc[0].bottom -= m_uiScrollBarSizeWH;
		}
		else {
			SetScrollPos(0);
			m_fVisible = false;
		}
	}
}

void CHexScroll::OnNcPaint()const
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
		static constexpr auto uTimerFirstClick { 200U }; //Milliseconds for WM_TIMER for first channel click.
		using enum EState; using enum ETimer;
		POINT pt;
		::GetCursorPos(&pt);
		const auto wndParent = GetParent();
		wndParent.ScreenToClient(pt);
		wndParent.SetFocus();

		if (GetThumbRect(true).PtInRect(pt)) {
			m_ptCursorCur = pt;
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

auto CHexScroll::ProcessMsg(const MSG& stMsg)->LRESULT
{
	switch (stMsg.message) {
	case WM_DESTROY: return CHexScroll::OnDestroy(stMsg);
	case WM_TIMER: return CHexScroll::OnTimer(stMsg);
	default: return wnd::DefMsgProc(stMsg);
	}
}

auto CHexScroll::SetScrollPos(ULONGLONG ullNewPos)->ULONGLONG
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return { };
	}

	m_ullScrollPosPrev = m_ullScrollPosCur;
	if (m_ullScrollPosCur == ullNewPos) {
		return m_ullScrollPosPrev;
	}

	const auto rc = GetParentRect();
	const auto ullScreenSize { static_cast<ULONGLONG>(IsVert() ? rc.Height() : rc.Width()) };
	const auto ullMax { ullScreenSize > m_ullScrollSizeMax ? 0 : m_ullScrollSizeMax - ullScreenSize };
	m_ullScrollPosCur = (std::min)(ullNewPos, ullMax);
	SendParentScrollMsg();
	DrawScrollBar();

	return m_ullScrollPosPrev;
}

void CHexScroll::SetScrollSizes(ULONGLONG ullLine, ULONGLONG ullPage, ULONGLONG ullSizeMax)
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return;
	}

	m_ullScrollLine = ullLine;
	m_ullScrollPage = ullPage;
	m_ullScrollSizeMax = ullSizeMax;

	RedrawNC(); //To repaint NC area.
}

void CHexScroll::ScrollEnd()
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return;
	}

	SetScrollPos(m_ullScrollSizeMax);
}

void CHexScroll::ScrollLineUp()
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return;
	}

	const auto ullCur = GetScrollPos();
	SetScrollPos(m_ullScrollLine > ullCur ? 0 : ullCur - m_ullScrollLine);
}

void CHexScroll::ScrollLineDown()
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return;
	}

	const auto ullCur = GetScrollPos();
	const auto ullMax = (std::numeric_limits<ULONGLONG>::max)();
	const auto ullNew { ullMax - ullCur < m_ullScrollLine ? ullMax : ullCur + m_ullScrollLine };
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
	SetScrollPos(m_ullScrollPage > ullCur ? 0 : ullCur - m_ullScrollPage);
}

void CHexScroll::ScrollPageDown()
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return;
	}

	const auto ullCur = GetScrollPos();
	const auto ullMax = (std::numeric_limits<ULONGLONG>::max)();
	SetScrollPos(ullMax - ullCur < m_ullScrollPage ? ullMax : ullCur + m_ullScrollPage);
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

	m_ullScrollPage = ullSize;
}


//Private methods.

bool CHexScroll::CreateArrows(HBITMAP hArrow, bool fVert)
{
	BITMAP stBMP { };
	GetObjectW(hArrow, sizeof(BITMAP), &stBMP); //stBMP.bmBits is nullptr here.
	const auto dwWidth = static_cast<DWORD>(stBMP.bmWidth);
	const auto dwHeight = static_cast<DWORD>(stBMP.bmHeight);
	const auto dwPixels = dwWidth * dwHeight;
	const auto dwBytesBmp = stBMP.bmWidthBytes * stBMP.bmHeight;
	const auto pPixelsOrig = std::make_unique<COLORREF[]>(dwPixels);
	m_iArrowRCSizePx = stBMP.bmWidth;
	GetBitmapBits(hArrow, dwBytesBmp, pPixelsOrig.get());

	const auto lmbTranspose = [](COLORREF* pInOut, DWORD dwWidth, DWORD dwHeight) {
		for (auto itHeight = 0UL; itHeight < dwHeight; ++itHeight) { //Transpose matrix.
			for (auto j = itHeight; j < dwWidth; ++j) {
				std::swap(pInOut[(itHeight * dwHeight) + j], pInOut[(j * dwHeight) + itHeight]);
			}
		}
		};
	const auto lmbFlipVert = [](COLORREF* pInOut, DWORD dwWidth, DWORD dwHeight) {
		for (auto itWidth = 0UL; itWidth < dwWidth; ++itWidth) { //Flip matrix' columns.
			for (auto itHeight = 0UL, itHeightBack = dwHeight - 1; itHeight < itHeightBack; ++itHeight, --itHeightBack) {
				std::swap(pInOut[(itHeight * dwHeight) + itWidth], pInOut[(itHeightBack * dwWidth) + itWidth]);
			}
		}
		};
	const auto lmbFlipHorz = [](COLORREF* pInOut, DWORD dwWidth, DWORD dwHeight) {
		for (auto itHeight = 0UL; itHeight < dwHeight; ++itHeight) { //Flip matrix' rows.
			for (auto itWidth = 0UL, itWidthBack = dwWidth - 1; itWidth < itWidthBack; ++itWidth, --itWidthBack) {
				std::swap(pInOut[(itHeight * dwWidth) + itWidth], pInOut[(itHeight * dwWidth) + itWidthBack]);
			}
		}
		};

	m_hBmpArrowFirst = ::CreateBitmapIndirect(&stBMP);
	m_hBmpArrowLast = ::CreateBitmapIndirect(&stBMP);

	if (fVert) {
		::SetBitmapBits(m_hBmpArrowFirst, dwBytesBmp, pPixelsOrig.get()); //Up arrow.
		lmbFlipVert(pPixelsOrig.get(), dwWidth, dwHeight);               //Down arrow.
	}
	else {
		lmbTranspose(pPixelsOrig.get(), dwWidth, dwHeight);
		lmbFlipVert(pPixelsOrig.get(), dwWidth, dwHeight);
		::SetBitmapBits(m_hBmpArrowFirst, dwBytesBmp, pPixelsOrig.get()); //Left arrow.
		lmbFlipHorz(pPixelsOrig.get(), dwWidth, dwHeight);               //Right arrow.
	}

	SetBitmapBits(m_hBmpArrowLast, dwBytesBmp, pPixelsOrig.get());

	return true;
}

void CHexScroll::DrawScrollBar()const
{
	if (!IsVisible()) {
		return;
	}

	static const auto clrBkNC { ::GetSysColor(COLOR_3DFACE) }; //Bk color of the non client area. 
	static constexpr auto clrBkScroll = RGB(241, 241, 241);  //Scroll Bk color.

	const auto wndParent = GetParent();
	const auto hDCParent = wndParent.GetWindowDC();
	const auto hDCMem = ::CreateCompatibleDC(hDCParent);
	const auto rcWnd = GetParentRect(false);
	const auto hBMP = ::CreateCompatibleBitmap(hDCParent, rcWnd.Width(), rcWnd.Height());
	::SelectObject(hDCMem, hBMP);
	const auto rcSNC = GetScrollRect(true);	//Scroll bar with any additional non client area, to fill it below.
	wnd::FillSolidRect(hDCMem, rcSNC, clrBkNC);
	const auto rcS = GetScrollRect();
	wnd::FillSolidRect(hDCMem, rcS, clrBkScroll);
	DrawArrows(hDCMem);
	DrawThumb(hDCMem);

	//Copy drawn Scrollbar from hDCMem to the parent window (hDCParent).
	::BitBlt(hDCParent, rcSNC.left, rcSNC.top, rcSNC.Width(), rcSNC.Height(), hDCMem, rcSNC.left, rcSNC.top, SRCCOPY);

	::DeleteObject(hBMP);
	::DeleteDC(hDCMem);
	wndParent.ReleaseDC(hDCParent);
}

void CHexScroll::DrawArrows(HDC hDC)const
{
	const auto rcScroll = GetScrollRect();
	const auto iFirstBtnOffsetDrawX = rcScroll.left;
	const auto iFirstBtnOffsetDrawY = rcScroll.top;
	int iFirstBtnWH;
	int iLastBtnWH;
	int iLastBtnOffsetDrawX;
	int iLastBtnOffsetDrawY;

	if (IsVert()) {
		iFirstBtnWH = iLastBtnWH = rcScroll.Width();
		iLastBtnOffsetDrawX = rcScroll.left;
		iLastBtnOffsetDrawY = rcScroll.bottom - rcScroll.Width();
	}
	else {
		iFirstBtnWH = iLastBtnWH = rcScroll.Height();
		iLastBtnOffsetDrawX = rcScroll.right - rcScroll.Height();
		iLastBtnOffsetDrawY = rcScroll.top;
	}

	::SetStretchBltMode(hDC, HALFTONE); //To stretch bmp at max quality, without artifacts.

	//https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-setstretchbltmode
	//After setting the HALFTONE stretching mode, an application must call the Win32 function
	//SetBrushOrgEx to set the brush origin. If it fails to do so, brush misalignment occurs.
	::SetBrushOrgEx(hDC, 0, 0, nullptr);

	const auto hDCSource = CreateCompatibleDC(hDC);
	::SelectObject(hDCSource, m_hBmpArrowFirst);	//First arrow button.
	::StretchBlt(hDC, iFirstBtnOffsetDrawX, iFirstBtnOffsetDrawY, iFirstBtnWH, iFirstBtnWH,
		hDCSource, 0, 0, m_iArrowRCSizePx, m_iArrowRCSizePx, SRCCOPY);
	::SelectObject(hDCSource, m_hBmpArrowLast); //Last arrow button.
	::StretchBlt(hDC, iLastBtnOffsetDrawX, iLastBtnOffsetDrawY, iLastBtnWH, iLastBtnWH,
		hDCSource, 0, 0, m_iArrowRCSizePx, m_iArrowRCSizePx, SRCCOPY);
	::DeleteDC(hDCSource);
}

void CHexScroll::DrawThumb(HDC hDC)const
{
	auto rcThumb = GetThumbRect();
	if (!rcThumb.IsRectNull()) {
		wnd::FillSolidRect(hDC, rcThumb, RGB(200, 200, 200)); //Scrollbar thumb color.
	}
}

auto CHexScroll::GetParent()const->wnd::CWnd
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return nullptr;
	}

	return m_WndParent;
}

auto CHexScroll::GetScrollRect(bool fWithNCArea)const->wnd::CRect
{
	if (!m_fCreated) {
		return { };
	}

	const auto wndParent = GetParent();
	auto rcClient = GetParentRect();
	wndParent.MapWindowPoints(nullptr, rcClient);
	const auto rcWnd = GetParentRect(false);
	const auto iTopDelta = GetTopDelta();
	const auto iLeftDelta = GetLeftDelta();

	wnd::CRect rcScroll;
	if (IsVert()) {
		rcScroll.left = rcClient.right + iLeftDelta;
		rcScroll.top = rcClient.top + iTopDelta;
		rcScroll.right = rcScroll.left + m_uiScrollBarSizeWH;
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
		rcScroll.bottom = rcScroll.top + m_uiScrollBarSizeWH;
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

auto CHexScroll::GetScrollWorkAreaRect(bool fClientCoord)const->wnd::CRect
{
	auto rc = GetScrollRect();
	if (IsVert()) {
		rc.DeflateRect(0, m_uiScrollBarSizeWH, 0, m_uiScrollBarSizeWH);
	}
	else {
		rc.DeflateRect(m_uiScrollBarSizeWH, 0, m_uiScrollBarSizeWH, 0);
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

	return uiScrollSize <= m_uiScrollBarSizeWH * 2 ? 0 : uiScrollSize - (m_uiScrollBarSizeWH * 2); //Minus two arrow's size.
}

auto CHexScroll::GetThumbRect(bool fClientCoord)const->wnd::CRect
{
	wnd::CRect rc;
	const auto uiThumbSize = GetThumbSizeWH();
	if (!uiThumbSize) {
		return rc;
	}

	const auto rcScrollWA = GetScrollWorkAreaRect();
	if (IsVert()) {
		rc.left = rcScrollWA.left;
		rc.top = rcScrollWA.top + GetThumbPos();
		rc.right = rc.left + m_uiScrollBarSizeWH;
		rc.bottom = rc.top + uiThumbSize;
		rc.bottom = (std::min)(rc.top + static_cast<LONG>(uiThumbSize), rcScrollWA.bottom);
	}
	else {
		rc.left = rcScrollWA.left + GetThumbPos();
		rc.top = rcScrollWA.top;
		rc.right = rc.left + uiThumbSize;
		rc.bottom = rc.top + m_uiScrollBarSizeWH;
	}
	if (fClientCoord) {
		rc.OffsetRect(-GetLeftDelta(), -GetTopDelta());
	}

	return rc;
}

auto CHexScroll::GetThumbSizeWH()const->UINT
{
	static constexpr auto uThumbSizeMin = 15U; //Minimum allowed thumb size.
	const auto uiScrollWorkAreaSizeWH = GetScrollWorkAreaSizeWH();
	const auto rcParent = GetParentRect();
	const double dDelta { IsVert() ? static_cast<double>(rcParent.Height()) / m_ullScrollSizeMax :
		static_cast<double>(rcParent.Width()) / m_ullScrollSizeMax };
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
	if (!m_fCreated) {
		return 0;
	}

	const auto uiWAWOThumb = GetScrollWorkAreaSizeWH() - GetThumbSizeWH(); //Work area without thumb.
	const auto iPage { IsVert() ? GetParentRect().Height() : GetParentRect().Width() };

	return (m_ullScrollSizeMax - iPage) / static_cast<double>(uiWAWOThumb);
}

auto CHexScroll::GetFirstArrowRect(bool fClientCoord)const->wnd::CRect
{
	auto rc = GetScrollRect();
	if (IsVert()) {
		rc.bottom = rc.top + m_uiScrollBarSizeWH;
	}
	else {
		rc.right = rc.left + m_uiScrollBarSizeWH;
	}

	if (fClientCoord) {
		rc.OffsetRect(-GetLeftDelta(), -GetTopDelta());
	}

	return rc;
}

auto CHexScroll::GetLastArrowRect(bool fClientCoord)const->wnd::CRect
{
	auto rc = GetScrollRect();
	if (IsVert()) {
		rc.top = rc.bottom - m_uiScrollBarSizeWH;
	}
	else {
		rc.left = rc.right - m_uiScrollBarSizeWH;
	}

	if (fClientCoord) {
		rc.OffsetRect(-GetLeftDelta(), -GetTopDelta());
	}

	return rc;
}

auto CHexScroll::GetFirstChannelRect(bool fClientCoord)const->wnd::CRect
{
	const auto rcThumb = GetThumbRect();
	const auto rcArrow = GetFirstArrowRect();
	wnd::CRect rc;
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

auto CHexScroll::GetLastChannelRect(bool fClientCoord)const->wnd::CRect
{
	const auto rcThumb = GetThumbRect();
	const auto rcArrow = GetLastArrowRect();
	wnd::CRect rc;
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

auto CHexScroll::GetParentRect(bool fClient)const->wnd::CRect
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
	return m_fScrollVert;
}

bool CHexScroll::IsThumbDragging()const
{
	return m_eState == EState::THUMB_CLICK;
}

bool CHexScroll::IsSiblingVisible()const
{
	return m_pSibling ? m_pSibling->IsVisible() : false;
}

auto CHexScroll::OnDestroy(const MSG& stMsg)->LRESULT
{
	::DeleteObject(m_hBmpArrowFirst);
	::DeleteObject(m_hBmpArrowLast);
	m_hBmpArrowFirst = nullptr;
	m_hBmpArrowLast = nullptr;
	m_Wnd.Detach();
	m_WndParent.Detach();
	m_fCreated = false;

	return wnd::DefMsgProc(stMsg);
}

auto CHexScroll::OnTimer(const MSG& stMsg)->LRESULT
{
	static constexpr auto uTimerRepeat { 50U }; //Milliseconds for repeat when click and hold on channel.
	const auto nIDEvent = static_cast<UINT_PTR>(stMsg.wParam);
	using enum EState; using enum ETimer;

	switch (nIDEvent) {
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
			GetCursorPos(&pt);
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
			GetCursorPos(&pt);
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
	if (!m_fCreated) {
		return;
	}

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
	else if (iPos == m_iThumbPosMax) {
		ullNewScrollPos = m_ullScrollSizeMax;
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