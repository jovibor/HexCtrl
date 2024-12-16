module;
/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include <SDKDDKVer.h>
#include <afxwin.h>
#include <cassert>
#include <cmath>
#include <memory>
export module HEXCTRL.CHexScroll;

namespace HEXCTRL::INTERNAL {
	export class CHexScroll final : public CWnd {
	public:
		void AddSibling(CHexScroll* pSibling);
		bool Create(CWnd* pParent, bool fVert, UINT uIDArrow, ULONGLONG ullScrolline,
			ULONGLONG ullScrollPage, ULONGLONG ullScrollSizeMax);
		bool Create(CWnd* pParent, bool fVert, HBITMAP hArrow, ULONGLONG ullScrolline,
			ULONGLONG ullScrollPage, ULONGLONG ullScrollSizeMax);
		[[nodiscard]] auto GetParent()const->CWnd*;
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
		void OnLButtonUp(UINT nFlags, CPoint point);
		void OnMouseMove(UINT nFlags, CPoint point);
		void OnNcActivate(BOOL bActive)const;
		void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
		void OnNcPaint()const;
		void OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
		/************************************************************************************
		* END OF THE CALLBACK METHODS.                                                      *
		************************************************************************************/

		void SetScrollSizes(ULONGLONG ullLine, ULONGLONG ullPage, ULONGLONG ullSizeMax);
		auto SetScrollPos(ULONGLONG ullNewPos) -> ULONGLONG;
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
		void DrawScrollBar()const;      //Draws the whole Scrollbar.
		void DrawArrows(CDC* pDC)const; //Draws arrows.
		void DrawThumb(CDC* pDC)const;  //Draws the Scroll thumb.
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
		[[nodiscard]] int GetTopDelta()const;       //Difference between parent window's Window and Client area. Very important in hit testing.
		[[nodiscard]] int GetLeftDelta()const;
		[[nodiscard]] bool IsVert()const;           //Is vertical or horizontal scrollbar.
		[[nodiscard]] bool IsThumbDragging()const;  //Is the thumb currently dragged by mouse.
		[[nodiscard]] bool IsSiblingVisible()const; //Is sibling scrollbar currently visible or not.
		void RedrawNC()const;
		void SendParentScrollMsg()const;            //Sends the WM_(V/H)SCROLL to the parent window.
		afx_msg void OnDestroy();
		afx_msg void OnTimer(UINT_PTR nIDEvent);
		DECLARE_MESSAGE_MAP();
	private:
		static constexpr auto m_iThumbPosMax { 0x7FFFFFFF };
		enum class EState : std::uint8_t;
		enum class ETimer : std::uint16_t;
		CWnd* m_pParent { };              //Parent window.
		CHexScroll* m_pSibling { };       //Sibling scrollbar, added with AddSibling.
		CBitmap m_bmpArrowFirst;          //Up or Left arrow bitmap.
		CBitmap m_bmpArrowLast;           //Down or Right arrow bitmap.
		EState m_eState { };              //Current state.
		CPoint m_ptCursorCur;             //Cursor's current position.
		UINT m_uiScrollBarSizeWH { };     //Scrollbar size (width if vertical, height if horz).
		int m_iArrowRCSizePx { };         //Arrow bitmap side size (it's a square) in pixels.
		ULONGLONG m_ullScrollPosCur { };  //Current scroll position.
		ULONGLONG m_ullScrollPosPrev { }; //Previous scroll position.
		ULONGLONG m_ullScrollLine { };    //Size of one line scroll, when clicking arrow.
		ULONGLONG m_ullScrollPage { };    //Size of page scroll, when clicking channel.
		ULONGLONG m_ullScrollSizeMax { }; //Maximum scroll size (limit).
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

BEGIN_MESSAGE_MAP(CHexScroll, CWnd)
	ON_WM_DESTROY()
	ON_WM_TIMER()
END_MESSAGE_MAP()

void CHexScroll::AddSibling(CHexScroll* pSibling)
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return;
	}

	m_pSibling = pSibling;
}

bool CHexScroll::Create(CWnd* pParent, bool fVert, UINT uIDArrow, ULONGLONG ullScrolline,
	ULONGLONG ullScrollPage, ULONGLONG ullScrollSizeMax)
{
	assert(!m_fCreated); //Already created.
	assert(pParent != nullptr);
	if (m_fCreated || pParent == nullptr) {
		return false;
	}

	CBitmap bmpArrow; //Bitmap of the arrow.
	if (!bmpArrow.LoadBitmapW(uIDArrow)) {
		return false;
	}

	return Create(pParent, fVert, bmpArrow, ullScrolline, ullScrollPage, ullScrollSizeMax);
}

bool CHexScroll::Create(CWnd* pParent, bool fVert, HBITMAP hArrow, ULONGLONG ullScrolline,
	ULONGLONG ullScrollPage, ULONGLONG ullScrollSizeMax)
{
	assert(!m_fCreated); //Already created.
	assert(pParent != nullptr);
	if (m_fCreated || pParent == nullptr) {
		return false;
	}

	if (CreateEx(0, AfxRegisterWndClass(0U), nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr) == FALSE) {
		return false;
	}

	m_fScrollVert = fVert;
	m_pParent = pParent;
	m_uiScrollBarSizeWH = GetSystemMetrics(fVert ? SM_CXVSCROLL : SM_CXHSCROLL);

	if (!CreateArrows(hArrow, fVert)) {
		return false;
	}

	m_fCreated = true;
	SetScrollSizes(ullScrolline, ullScrollPage, ullScrollSizeMax);

	return true;
}

auto CHexScroll::GetParent()const->CWnd*
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return nullptr;
	}

	return m_pParent;
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

void CHexScroll::OnLButtonUp(UINT /*nFlags*/, CPoint /*point*/)
{
	assert(m_fCreated);
	if (!m_fCreated || m_eState == EState::STATE_DEFAULT) {
		return;
	}

	m_eState = EState::STATE_DEFAULT;
	SendParentScrollMsg(); //For parent to check IsThumbReleased.
	KillTimer(static_cast<UINT_PTR>(ETimer::IDT_FIRSTCLICK));
	KillTimer(static_cast<UINT_PTR>(ETimer::IDT_CLICKREPEAT));
	ReleaseCapture();
	DrawScrollBar();
}

void CHexScroll::OnMouseMove(UINT /*nFlags*/, CPoint point)
{
	assert(m_fCreated);
	if (!m_fCreated || !IsThumbDragging()) {
		return;
	}

	const auto rc = GetScrollWorkAreaRect(true);
	const auto iCurrPos = GetThumbPos();
	int iNewPos;

	if (IsVert()) {
		if (point.y < rc.top) {
			iNewPos = 0;
			m_ptCursorCur.y = rc.top;
		}
		else if (point.y > rc.bottom) {
			iNewPos = m_iThumbPosMax;
			m_ptCursorCur.y = rc.bottom;
		}
		else {
			iNewPos = iCurrPos + (point.y - m_ptCursorCur.y);
			m_ptCursorCur.y = point.y;
		}
	}
	else {
		if (point.x < rc.left) {
			iNewPos = 0;
			m_ptCursorCur.x = rc.left;
		}
		else if (point.x > rc.right) {
			iNewPos = m_iThumbPosMax;
			m_ptCursorCur.x = rc.right;
		}
		else {
			iNewPos = iCurrPos + (point.x - m_ptCursorCur.x);
			m_ptCursorCur.x = point.x;
		}
	}

	if (iNewPos != iCurrPos) {  //Set new thumb pos only if it has been changed.
		SetThumbPos(iNewPos);
	}
}

void CHexScroll::OnNcActivate(BOOL /*bActive*/)const
{
	if (!m_fCreated) {
		return;
	}

	RedrawNC(); //To repaint NC area.
}

void CHexScroll::OnNcCalcSize(BOOL /*bCalcValidRects*/, NCCALCSIZE_PARAMS* lpncsp)
{
	if (!m_fCreated) {
		return;
	}

	const CRect rc = lpncsp->rgrc[0];
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
			lpncsp->rgrc[0].right -= m_uiScrollBarSizeWH;
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
			lpncsp->rgrc[0].bottom -= m_uiScrollBarSizeWH;
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

void CHexScroll::OnSetCursor(CWnd* /*pWnd*/, UINT nHitTest, UINT message)
{
	assert(m_fCreated);
	if (!m_fCreated) {
		return;
	}

	if (nHitTest == HTTOPLEFT || nHitTest == HTLEFT || nHitTest == HTBOTTOMLEFT
		|| nHitTest == HTTOPRIGHT || nHitTest == HTRIGHT || nHitTest == HTBOTTOMRIGHT
		|| nHitTest == HTBOTTOM || nHitTest == HTSIZE || !IsVisible()) {
		return;
	}

	switch (message) {
	case WM_LBUTTONDOWN:
	{
		const auto pParent = GetParent();
		if (pParent == nullptr) {
			return;
		}

		POINT pt;
		GetCursorPos(&pt);
		pParent->ScreenToClient(&pt);
		pParent->SetFocus();
		static constexpr auto uTimerFirstClick { 200U }; //Milliseconds for WM_TIMER for first channel click.
		using enum EState; using enum ETimer;

		if (GetThumbRect(true).PtInRect(pt)) {
			m_ptCursorCur = pt;
			m_eState = THUMB_CLICK;
			pParent->SetCapture();
		}
		else if (GetFirstArrowRect(true).PtInRect(pt)) {
			ScrollLineUp();
			m_eState = FIRSTARROW_CLICK;
			pParent->SetCapture();
			SetTimer(static_cast<UINT_PTR>(IDT_FIRSTCLICK), uTimerFirstClick, nullptr);
		}
		else if (GetLastArrowRect(true).PtInRect(pt)) {
			ScrollLineDown();
			m_eState = LASTARROW_CLICK;
			pParent->SetCapture();
			SetTimer(static_cast<UINT_PTR>(IDT_FIRSTCLICK), uTimerFirstClick, nullptr);
		}
		else if (GetFirstChannelRect(true).PtInRect(pt)) {
			ScrollPageUp();
			m_eState = FIRSTCHANNEL_CLICK;
			pParent->SetCapture();
			SetTimer(static_cast<UINT_PTR>(IDT_FIRSTCLICK), uTimerFirstClick, nullptr);
		}
		else if (GetLastChannelRect(true).PtInRect(pt)) {
			ScrollPageDown();
			m_eState = LASTCHANNEL_CLICK;
			pParent->SetCapture();
			SetTimer(static_cast<UINT_PTR>(IDT_FIRSTCLICK), uTimerFirstClick, nullptr);
		}
	}
	break;
	default:
		break;
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
	const auto ullNew { ULONGLONG_MAX - ullCur < m_ullScrollLine ? ULONGLONG_MAX : ullCur + m_ullScrollLine };
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
	SetScrollPos(ULONGLONG_MAX - ullCur < m_ullScrollPage ? ULONGLONG_MAX : ullCur + m_ullScrollPage);
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


/********************************************************************
* Private methods.
********************************************************************/

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

	m_bmpArrowFirst.CreateBitmapIndirect(&stBMP);
	m_bmpArrowLast.CreateBitmapIndirect(&stBMP);

	if (fVert) {
		m_bmpArrowFirst.SetBitmapBits(dwBytesBmp, pPixelsOrig.get()); //Up arrow.
		lmbFlipVert(pPixelsOrig.get(), dwWidth, dwHeight);              //Down arrow.
	}
	else {
		lmbTranspose(pPixelsOrig.get(), dwWidth, dwHeight);
		lmbFlipVert(pPixelsOrig.get(), dwWidth, dwHeight);
		m_bmpArrowFirst.SetBitmapBits(dwBytesBmp, pPixelsOrig.get()); //Left arrow.
		lmbFlipHorz(pPixelsOrig.get(), dwWidth, dwHeight);              //Right arrow.
	}

	m_bmpArrowLast.SetBitmapBits(dwBytesBmp, pPixelsOrig.get());

	return true;
}

void CHexScroll::DrawScrollBar()const
{
	if (!IsVisible()) {
		return;
	}

	static const auto clrBkNC { GetSysColor(COLOR_3DFACE) }; //Bk color of the non client area. 
	const auto pParent = GetParent();
	CWindowDC dcParent(pParent);
	CDC dcMem;
	dcMem.CreateCompatibleDC(&dcParent);
	CBitmap bitmap;
	const auto rcWnd = GetParentRect(false);
	bitmap.CreateCompatibleBitmap(&dcParent, rcWnd.Width(), rcWnd.Height());
	dcMem.SelectObject(&bitmap);
	const auto pDC = &dcMem;
	const auto rcSNC = GetScrollRect(true);	//Scroll bar with any additional non client area, to fill it below.
	pDC->FillSolidRect(rcSNC, clrBkNC);	//Scroll bar with NC Bk.
	const auto rcS = GetScrollRect();
	pDC->FillSolidRect(rcS, RGB(241, 241, 241)); //Scroll bar Bk.
	DrawArrows(pDC);
	DrawThumb(pDC);

	//Copy drawn Scrollbar from dcMem to the parent window.
	dcParent.BitBlt(rcSNC.left, rcSNC.top, rcSNC.Width(), rcSNC.Height(), &dcMem, rcSNC.left, rcSNC.top, SRCCOPY);
}

void CHexScroll::DrawArrows(CDC* pDC)const
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

	pDC->SetStretchBltMode(HALFTONE); //To stretch bmp at max quality, without artifacts.

	//https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-setstretchbltmode
	//After setting the HALFTONE stretching mode, an application must call the Win32 function
	//SetBrushOrgEx to set the brush origin. If it fails to do so, brush misalignment occurs.
	SetBrushOrgEx(pDC->m_hDC, 0, 0, nullptr);

	CDC dcSource;
	dcSource.CreateCompatibleDC(pDC);
	dcSource.SelectObject(m_bmpArrowFirst);	//First arrow button.
	pDC->StretchBlt(iFirstBtnOffsetDrawX, iFirstBtnOffsetDrawY, iFirstBtnWH, iFirstBtnWH,
		&dcSource, 0, 0, m_iArrowRCSizePx, m_iArrowRCSizePx, SRCCOPY);
	dcSource.SelectObject(m_bmpArrowLast); //Last arrow button.
	pDC->StretchBlt(iLastBtnOffsetDrawX, iLastBtnOffsetDrawY, iLastBtnWH, iLastBtnWH,
		&dcSource, 0, 0, m_iArrowRCSizePx, m_iArrowRCSizePx, SRCCOPY);
}

void CHexScroll::DrawThumb(CDC* pDC)const
{
	auto rcThumb = GetThumbRect();
	if (!rcThumb.IsRectNull()) {
		pDC->FillSolidRect(rcThumb, RGB(200, 200, 200)); //Scrollbar thumb color.
	}
}

CRect CHexScroll::GetScrollRect(bool fWithNCArea)const
{
	if (!m_fCreated) {
		return { };
	}

	const auto* const pParent = GetParent();
	auto rcClient = GetParentRect();
	pParent->MapWindowPoints(nullptr, rcClient);
	const auto rcWnd = GetParentRect(false);
	const auto iTopDelta = GetTopDelta();
	const auto iLeftDelta = GetLeftDelta();

	CRect rcScroll;
	if (IsVert()) {
		rcScroll.left = rcClient.right + iLeftDelta;
		rcScroll.top = rcClient.top + iTopDelta;
		rcScroll.right = rcScroll.left + m_uiScrollBarSizeWH;
		if (fWithNCArea) { //Adding difference here to gain equality in coords when call to pParent->ScreenToClient below.
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
	pParent->ScreenToClient(&rcScroll);

	return rcScroll;
}

CRect CHexScroll::GetScrollWorkAreaRect(bool fClientCoord)const
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

UINT CHexScroll::GetScrollSizeWH()const
{
	return IsVert() ? GetScrollRect().Height() : GetScrollRect().Width();
}

UINT CHexScroll::GetScrollWorkAreaSizeWH()const
{
	const auto uiScrollSize = GetScrollSizeWH();

	return uiScrollSize <= m_uiScrollBarSizeWH * 2 ? 0 : uiScrollSize - (m_uiScrollBarSizeWH * 2); //Minus two arrow's size.
}

CRect CHexScroll::GetThumbRect(bool fClientCoord)const
{
	CRect rc { };
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

UINT CHexScroll::GetThumbSizeWH()const
{
	static constexpr auto uThumbSizeMin = 15U; //Minimum allowed thumb size.
	const auto uiScrollWorkAreaSizeWH = GetScrollWorkAreaSizeWH();
	const auto rcParent = GetParentRect();
	const long double dDelta { IsVert() ? static_cast<long double>(rcParent.Height()) / m_ullScrollSizeMax :
		static_cast<long double>(rcParent.Width()) / m_ullScrollSizeMax };
	const auto uiThumbSize { static_cast<UINT>(std::lroundl(uiScrollWorkAreaSizeWH * dDelta)) };

	return uiThumbSize < uThumbSizeMin ? uThumbSizeMin : uiThumbSize;
}

int CHexScroll::GetThumbPos()const
{
	const auto ullScrollPos = GetScrollPos();
	const auto dThumbScrollingSize = GetThumbScrollingSize();

	return ullScrollPos < dThumbScrollingSize ? 0 : std::lroundl(ullScrollPos / dThumbScrollingSize);
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

long double CHexScroll::GetThumbScrollingSize()const
{
	if (!m_fCreated) {
		return 0;
	}

	const auto uiWAWOThumb = GetScrollWorkAreaSizeWH() - GetThumbSizeWH(); //Work area without thumb.
	const auto iPage { IsVert() ? GetParentRect().Height() : GetParentRect().Width() };

	return (m_ullScrollSizeMax - iPage) / static_cast<long double>(uiWAWOThumb);
}

CRect CHexScroll::GetFirstArrowRect(bool fClientCoord)const
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

CRect CHexScroll::GetLastArrowRect(bool fClientCoord)const
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

CRect CHexScroll::GetFirstChannelRect(bool fClientCoord)const
{
	const auto rcThumb = GetThumbRect();
	const auto rcArrow = GetFirstArrowRect();
	CRect rc;
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

CRect CHexScroll::GetLastChannelRect(bool fClientCoord)const
{
	const auto rcThumb = GetThumbRect();
	const auto rcArrow = GetLastArrowRect();
	CRect rc;
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

CRect CHexScroll::GetParentRect(bool fClient)const
{
	CRect rc;
	if (fClient) {
		GetParent()->GetClientRect(rc);
	}
	else {
		GetParent()->GetWindowRect(rc);
	}

	return rc;
}

int CHexScroll::GetTopDelta()const
{
	auto rcClient = GetParentRect();
	GetParent()->MapWindowPoints(nullptr, rcClient);

	return rcClient.top - GetParentRect(false).top;
}

int CHexScroll::GetLeftDelta()const
{
	auto rcClient = GetParentRect();
	GetParent()->MapWindowPoints(nullptr, rcClient);

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

void CHexScroll::RedrawNC()const
{
	//To repaint NC area.
	if (const auto pWnd = GetParent(); pWnd != nullptr) {
		pWnd->SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}
}

void CHexScroll::SendParentScrollMsg()const
{
	if (!m_fCreated) {
		return;
	}

	GetParent()->SendMessageW(IsVert() ? WM_VSCROLL : WM_HSCROLL);
}

void CHexScroll::OnTimer(UINT_PTR nIDEvent)
{
	static constexpr auto uTimerRepeat { 50U }; //Milliseconds for repeat when click and hold on channel.
	using enum EState; using enum ETimer;

	switch (nIDEvent) {
	case (static_cast<UINT_PTR>(IDT_FIRSTCLICK)):
		KillTimer(static_cast<UINT_PTR>(IDT_FIRSTCLICK));
		SetTimer(static_cast<UINT_PTR>(IDT_CLICKREPEAT), uTimerRepeat, nullptr);
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
			CPoint pt;
			GetCursorPos(&pt);
			CRect rc = GetThumbRect(true);
			GetParent()->ClientToScreen(rc);
			if (IsVert()) {
				if (pt.y < rc.top) {
					ScrollPageUp();
				}
			}
			else {
				if (pt.x < rc.left) {
					ScrollPageUp();
				}
			}
		}
		break;
		case LASTCHANNEL_CLICK:
		{
			CPoint pt;
			GetCursorPos(&pt);
			CRect rc = GetThumbRect(true);
			GetParent()->ClientToScreen(rc);
			if (IsVert()) {
				if (pt.y > rc.bottom) {
					ScrollPageDown();
				}
			}
			else {
				if (pt.x > rc.right) {
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

	CWnd::OnTimer(nIDEvent);
}

void CHexScroll::OnDestroy()
{
	m_bmpArrowFirst.DeleteObject();
	m_bmpArrowLast.DeleteObject();
	m_pParent = nullptr;
	m_fCreated = false;

	CWnd::OnDestroy();
}