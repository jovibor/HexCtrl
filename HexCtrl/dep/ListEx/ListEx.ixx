module;
/********************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/      *
* Official git repository: https://github.com/jovibor/ListEx/       *
* This code is available under the "MIT License".                   *
********************************************************************/
#include <SDKDDKVer.h>
#include <shlwapi.h>
#include <uxtheme.h>
#include <commctrl.h>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <vector>
export module ListEx;

export namespace HEXCTRL::LISTEX {
	/****************************************************************
	* EListExSortMode - Sorting mode.                               *
	****************************************************************/
	enum class EListExSortMode : std::uint8_t {
		SORT_LEX, SORT_NUMERIC
	};

	/****************************************************************
	* LISTEXCOLOR - colors for the cell.                            *
	****************************************************************/
	struct LISTEXCOLOR {
		COLORREF clrBk { };   //Bk color.
		COLORREF clrText { }; //Text color.
	};
	using PLISTEXCOLOR = const LISTEXCOLOR*;

	/****************************************************************
	* LISTEXCOLORINFO - struct for the LISTEX_MSG_GETCOLOR message. *
	****************************************************************/
	struct LISTEXCOLORINFO {
		NMHDR       hdr { };
		int         iItem { };
		int         iSubItem { };
		LISTEXCOLOR stClr;
	};
	using PLISTEXCOLORINFO = LISTEXCOLORINFO*;

	/****************************************************************
	* LISTEXDATAINFO - struct for the LISTEX_MSG_SETDATA message.   *
	****************************************************************/
	struct LISTEXDATAINFO {
		NMHDR   hdr { };
		int     iItem { };
		int     iSubItem { };
		LPCWSTR pwszData { };        //Text that has been set for a cell.
		bool    fAllowEdit { true }; //Allow cell editing or not, in case of LISTEX_MSG_EDITBEGIN.
	};
	using PLISTEXDATAINFO = LISTEXDATAINFO*;

	/****************************************************************
	* LISTEXICONINFO - struct for the LISTEX_MSG_GETICON message.   *
	****************************************************************/
	struct LISTEXICONINFO {
		NMHDR hdr { };
		int   iItem { };
		int   iSubItem { };
		int   iIconIndex { -1 }; //Icon index in the list internal image-list.
	};
	using PLISTEXICONINFO = LISTEXICONINFO*;

	/****************************************************************
	* LISTEXTTINFO - struct for the LISTEX_MSG_GETTOOLTIP message.  *
	****************************************************************/
	struct LISTEXTTDATA { //Tooltips data.
		LPCWSTR pwszText { };    //Tooltip text.
		LPCWSTR pwszCaption { }; //Tooltip caption.
	};
	struct LISTEXTTINFO {
		NMHDR        hdr { };
		int          iItem { };
		int          iSubItem { };
		LISTEXTTDATA stData;
	};
	using PLISTEXTTINFO = LISTEXTTINFO*;

	/****************************************************************
	* LISTEXLINKINFO - struct for the LISTEX_MSG_LINKCLICK message. *
	****************************************************************/
	struct LISTEXLINKINFO {
		NMHDR   hdr { };
		int     iItem { };
		int     iSubItem { };
		POINT   ptClick { };  //A point where the link was clicked.
		LPCWSTR pwszText { }; //Link text.
	};
	using PLISTEXLINKINFO = LISTEXLINKINFO*;

	/**********************************************************************************
	* LISTEXCOLORS - All CListEx colors.                                              *
	**********************************************************************************/
	struct LISTEXCOLORS {
		COLORREF clrListText { GetSysColor(COLOR_WINDOWTEXT) };       //List text color.
		COLORREF clrListTextLink { RGB(0, 0, 200) };                  //List hyperlink text color.
		COLORREF clrListTextSel { GetSysColor(COLOR_HIGHLIGHTTEXT) }; //Selected item text color.
		COLORREF clrListTextLinkSel { RGB(250, 250, 250) };           //List hyperlink text color in selected cell.
		COLORREF clrListBkOdd { GetSysColor(COLOR_WINDOW) };          //List Bk color of the odd rows.
		COLORREF clrListBkEven { GetSysColor(COLOR_WINDOW) };         //List Bk color of the even rows.
		COLORREF clrListBkSel { GetSysColor(COLOR_HIGHLIGHT) };       //Selected item bk color.
		COLORREF clrListGrid { RGB(220, 220, 220) };                  //List grid color.
		COLORREF clrTooltipText { 0xFFFFFFFFUL };                     //Tooltip text color, 0xFFFFFFFFUL for current Theme color.
		COLORREF clrTooltipBk { 0xFFFFFFFFUL };                       //Tooltip bk color, 0xFFFFFFFFUL for current Theme color.
		COLORREF clrHdrText { GetSysColor(COLOR_WINDOWTEXT) };        //List header text color.
		COLORREF clrHdrBk { GetSysColor(COLOR_WINDOW) };              //List header bk color.
		COLORREF clrHdrHglInact { GetSysColor(COLOR_GRADIENTINACTIVECAPTION) };//Header highlight inactive.
		COLORREF clrHdrHglAct { GetSysColor(COLOR_GRADIENTACTIVECAPTION) };    //Header highlight active.
		COLORREF clrNWABk { GetSysColor(COLOR_WINDOW) };              //Bk of Non Working Area.
	};
	using PCLISTEXCOLORS = const LISTEXCOLORS*;

	/**********************************************************************************
	* LISTEXCREATE - Main initialization helper struct for CListEx::Create method.    *
	**********************************************************************************/
	struct LISTEXCREATE {
		HWND            hWndParent { };          //Parent window.
		PCLISTEXCOLORS  pColors { };             //CListEx colors.
		const LOGFONTW* pLFList { };             //CListEx LOGFONT.
		const LOGFONTW* pLFHdr { };              //Header LOGFONT.
		RECT            rect { };                //Initial rect.
		UINT            uID { };                 //CListEx Control ID.
		DWORD           dwStyle { };             //Window styles.
		DWORD           dwExStyle { };           //Extended window styles.
		DWORD           dwSizeFontList { 9 };    //List font default size in logical points.
		DWORD           dwSizeFontHdr { 9 };     //Header font default size in logical points.
		DWORD           dwTTStyleCell { };       //Cell's tooltip Window styles.
		DWORD           dwTTStyleLink { };       //Link's tooltip Window styles.
		DWORD           dwTTDelayTime { };       //Tooltip delay before showing up, in ms.
		DWORD           dwTTShowTime { 5000 };   //Tooltip show up time, in ms.
		DWORD           dwWidthGrid { 1 };       //Width of the list grid.
		DWORD           dwHdrHeight { };         //Header height.
		POINT           ptTTOffset { .x { 3 }, .y { -20 } }; //Tooltip offset from a cursor pos. Doesn't work for TTS_BALLOON.
		bool            fDialogCtrl { false };   //If it's a list within dialog?
		bool            fSortable { false };     //Is list sortable, by clicking on the header column?
		bool            fLinks { false };        //Enable links support.
		bool            fLinkUnderline { true }; //Links are displayed underlined or not.
		bool            fLinkTooltip { true };   //Show links' toolips or not.
		bool            fHighLatency { false };  //Do not redraw window until scroll thumb is released.
	};

	/********************************************
	* LISTEXHDRICON - Icon for header column.   *
	********************************************/
	struct LISTEXHDRICON {
		POINT pt { };              //Coords of the icon's top-left corner in the header item's rect.
		int   iIndex { -1 };       //Icon index in the header's image list.
		bool  fClickable { true }; //Is icon sending LISTEX_MSG_HDRICONCLICK message when clicked.
	};

	//WM_NOTIFY codes (NMHDR.code values).

	constexpr auto LISTEX_MSG_EDITBEGIN { 0x1000U };    //Edit in-place field is about to display.
	constexpr auto LISTEX_MSG_GETCOLOR { 0x1001U };     //Get cell color.
	constexpr auto LISTEX_MSG_GETICON { 0x1002U };      //Get cell icon.
	constexpr auto LISTEX_MSG_GETTOOLTIP { 0x1003U };   //Get cell tool-tip data.
	constexpr auto LISTEX_MSG_HDRICONCLICK { 0x1004U }; //Header's icon has been clicked.
	constexpr auto LISTEX_MSG_HDRRBTNDOWN { 0x1005U };  //Header's WM_RBUTTONDOWN message.
	constexpr auto LISTEX_MSG_HDRRBTNUP { 0x1006U };    //Header's WM_RBUTTONUP message.
	constexpr auto LISTEX_MSG_LINKCLICK { 0x1007U };    //Hyperlink has been clicked.
	constexpr auto LISTEX_MSG_SETDATA { 0x1008U };      //Item text has been edited/changed.

	//Setting a manifest for the ComCtl32.dll version 6.
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
}

namespace HEXCTRL::LISTEX::INTERNAL::wnd {
	auto DefSubclassProc(const MSG& msg) -> LRESULT {
		return ::DefSubclassProc(msg.hwnd, msg.message, msg.wParam, msg.lParam);
	}

	class CPoint : public POINT {
	public:
		CPoint() : POINT { } { }
		CPoint(POINT pt) : POINT { pt } { }
		CPoint(int x, int y) : POINT { .x { x }, .y { y } } { }
		operator LPPOINT() { return this; }
		operator const POINT*()const { return this; }
		bool operator==(POINT pt)const { return x == pt.x && y == pt.y; }
		bool operator!=(POINT pt)const { return !(*this == pt); }
		CPoint& operator=(POINT pt) { *this = pt; return *this; }
		CPoint operator+(POINT pt)const { return { x + pt.x, y + pt.y }; }
		CPoint operator-(POINT pt)const { return { x - pt.x, y - pt.y }; }
		void Offset(int iX, int iY) { x += iX; y += iY; }
		void Offset(POINT pt) { Offset(pt.x, pt.y); }
	};

	class CRect : public RECT {
	public:
		CRect() : RECT { } { }
		CRect(int iLeft, int iTop, int iRight, int iBottom) : RECT { .left { iLeft }, .top { iTop },
			.right { iRight }, .bottom { iBottom } } {
		}
		CRect(const RECT rc) { ::CopyRect(this, &rc); }
		CRect(LPCRECT pRC) { ::CopyRect(this, pRC); }
		CRect(POINT pt, SIZE size) { left = pt.x; right = left + size.cx; top = pt.y; bottom = top + size.cy; }
		CRect(POINT topLeft, POINT botRight) {
			left = topLeft.x; top = topLeft.y; right = botRight.x; bottom = botRight.y;
		}
		operator LPRECT() { return this; }
		operator LPCRECT()const { return this; }
		bool operator==(RECT rc)const { return ::EqualRect(this, &rc); }
		bool operator!=(RECT rc)const { return !(*this == rc); }
		CRect& operator=(RECT rc) { ::CopyRect(this, &rc); return *this; }
		[[nodiscard]] auto BottomRight()const->CPoint { return { { .x { right }, .y { bottom } } }; };
		void DeflateRect(int x, int y) { ::InflateRect(this, -x, -y); }
		void DeflateRect(SIZE size) { ::InflateRect(this, -size.cx, -size.cy); }
		void DeflateRect(LPCRECT pRC) { left += pRC->left; top += pRC->top; right -= pRC->right; bottom -= pRC->bottom; }
		void DeflateRect(int l, int t, int r, int b) { left += l; top += t; right -= r; bottom -= b; }
		[[nodiscard]] int Height()const { return bottom - top; }
		[[nodiscard]] bool IsRectEmpty()const { return ::IsRectEmpty(this); }
		[[nodiscard]] bool IsRectNull()const { return (left == 0 && right == 0 && top == 0 && bottom == 0); }
		void OffsetRect(int x, int y) { ::OffsetRect(this, x, y); }
		void OffsetRect(POINT pt) { ::OffsetRect(this, pt.x, pt.y); }
		[[nodiscard]] bool PtInRect(POINT pt)const { return ::PtInRect(this, pt); }
		void SetRect(int x1, int y1, int x2, int y2) { ::SetRect(this, x1, y1, x2, y2); }
		void SetRectEmpty() { ::SetRectEmpty(this); }
		[[nodiscard]] auto TopLeft()const->CPoint { return { { .x { left }, .y { top } } }; };
		[[nodiscard]] int Width()const { return right - left; }
	};

	class CDC {
	public:
		CDC() = default;
		CDC(HDC hDC) : m_hDC(hDC) { }
		operator HDC()const { return m_hDC; }
		void DrawFocusRect(LPCRECT pRc)const { ::DrawFocusRect(m_hDC, pRc); }
		int DrawTextW(std::wstring_view wsv, LPRECT pRect, UINT uFormat)const {
			return ::DrawTextW(m_hDC, wsv.data(), static_cast<int>(wsv.size()), pRect, uFormat);
		}
		void FillSolidRect(LPCRECT pRC, COLORREF clr)const {
			::SetBkColor(m_hDC, clr); ::ExtTextOutW(m_hDC, 0, 0, ETO_OPAQUE, pRC, nullptr, 0, nullptr);
		}
		[[nodiscard]] auto GetClipBox()const->CRect { RECT rc; ::GetClipBox(m_hDC, &rc); return rc; }
		bool LineTo(POINT pt)const { return LineTo(pt.x, pt.y); }
		bool LineTo(int x, int y)const { return ::LineTo(m_hDC, x, y); }
		bool MoveTo(POINT pt)const { return MoveTo(pt.x, pt.y); }
		bool MoveTo(int x, int y)const { return ::MoveToEx(m_hDC, x, y, nullptr); }
		bool Polygon(const POINT* pPT, int iCount)const { return ::Polygon(m_hDC, pPT, iCount); }
		auto SetTextColor(COLORREF clr)const->COLORREF { return ::SetTextColor(m_hDC, clr); }
		auto SelectObject(HGDIOBJ hObj)const->HGDIOBJ { return ::SelectObject(m_hDC, hObj); }
	protected:
		HDC m_hDC;
	};

	class CPaintDC : public CDC {
	public:
		CPaintDC(HWND hWnd) : m_hWnd(hWnd) { assert(::IsWindow(hWnd)); m_hDC = ::BeginPaint(m_hWnd, &m_PS); }
		~CPaintDC() { ::EndPaint(m_hWnd, &m_PS); }
		HDC GetHDC()const { return m_PS.hdc; }
		operator HDC()const { return m_PS.hdc; }
	private:
		PAINTSTRUCT m_PS;
		HWND m_hWnd;
	};

	class CMemDC : public CDC {
	public:
		CMemDC(HDC hDC, HWND hWnd) : m_hDCOrig(hDC) { assert(::IsWindow(hWnd)); ::GetClientRect(hWnd, &m_rc); Init(); }
		CMemDC(HDC hDC, RECT rc) : m_hDCOrig(hDC), m_rc(rc) { Init(); }
		~CMemDC();
		HDC GetHDC()const { return m_hDCMem; }
		operator HDC()const { return m_hDCMem; }
	private:
		void Init();
	private:
		HDC m_hDCOrig;
		HDC m_hDCMem;
		HBITMAP m_hBmp;
		RECT m_rc;
	};

	void CMemDC::Init()
	{
		m_hDCMem = m_hDC = ::CreateCompatibleDC(m_hDCOrig);
		const auto iWidth = m_rc.right - m_rc.left;
		const auto iHeight = m_rc.bottom - m_rc.top;
		m_hBmp = ::CreateCompatibleBitmap(m_hDCOrig, iWidth, iHeight);
		::SelectObject(m_hDCMem, m_hBmp);
		if (m_hDCMem == nullptr || m_hBmp == nullptr) {
			assert(true);
		}
	}

	CMemDC::~CMemDC()
	{
		const auto iWidth = m_rc.right - m_rc.left;
		const auto iHeight = m_rc.bottom - m_rc.top;
		::BitBlt(m_hDCOrig, m_rc.left, m_rc.top, iWidth, iHeight, m_hDCMem, m_rc.left, m_rc.top, SRCCOPY);
		::DeleteObject(m_hBmp);
		::DeleteDC(m_hDCMem);
	}

#define	GET_X_LPARAM(lParam) (static_cast<int>(static_cast<short>(LOWORD(lParam))))
#define	GET_Y_LPARAM(lParam) (static_cast<int>(static_cast<short>(HIWORD(lParam))))
}

namespace HEXCTRL::LISTEX::INTERNAL {
	class CListExHdr final {
	public:
		void DeleteColumn(int iIndex);
		[[nodiscard]] auto GetClientRect()const->RECT;
		[[nodiscard]] int GetColumnDataAlign(int iIndex)const;
		[[nodiscard]] auto GetHiddenCount()const->UINT;
		[[nodiscard]] auto GetImageList(int iList = HDSIL_NORMAL)const->HIMAGELIST;
		bool GetItem(int iPos, HDITEMW* pHDI)const;
		[[nodiscard]] int GetItemCount()const;
		[[nodiscard]] auto GetItemRect(int iIndex)const->RECT;
		void HideColumn(int iIndex, bool fHide);
		[[nodiscard]] bool IsColumnHidden(int iIndex)const; //Column index.
		[[nodiscard]] bool IsColumnSortable(int iIndex)const;
		[[nodiscard]] bool IsColumnEditable(int iIndex)const;
		auto ProcessMsg(const MSG& msg) -> LRESULT;
		void RedrawWindow()const;
		void SetHeight(DWORD dwHeight);
		void SetFont(const LOGFONTW& lf);
		void SetColor(const LISTEXCOLORS& lcs);
		void SetColumnColor(int iColumn, COLORREF clrBk, COLORREF clrText);
		void SetColumnDataAlign(int iColumn, int iAlign);
		void SetColumnIcon(int iColumn, const LISTEXHDRICON& stIcon);
		void SetColumnSortable(int iColumn, bool fSortable);
		void SetColumnEditable(int iColumn, bool fEditable);
		void SetImageList(HIMAGELIST pList, int iList = HDSIL_NORMAL);
		void SetItem(int nPos, const HDITEMW& item);
		void SetSortable(bool fSortable);
		void SetSortArrow(int iColumn, bool fAscending);
		void SubclassHeader(HWND hWndHeader);
	private:
		struct HIDDEN;
		struct HDRICON { //Header column icon.
			LISTEXHDRICON stIcon;               //Icon data struct.
			bool          fLMPressed { false }; //Left mouse button pressed atm.
		};
		struct COLUMNDATA {
			UINT uID { };    //Column ID
			HDRICON icon;    //Column icon.
			LISTEXCOLOR clr; //Column colors, text/bk.
			int  iDataAlign { LVCFMT_LEFT };
			bool fEditable { false };
			bool fSortable { true }; //By default columns are sortable, unless explicitly set to false.
		};
		void AddColumnData(const COLUMNDATA& data);
		[[nodiscard]] UINT ColumnIndexToID(int iIndex)const; //Returns unique column ID. Must be > 0.
		[[nodiscard]] int ColumnIDToIndex(UINT uID)const;
		[[nodiscard]] auto GetParent() -> HWND;
		[[nodiscard]] auto GetHdrColor(UINT ID)const->PLISTEXCOLOR;
		[[nodiscard]] auto GetColumnData(UINT uID) -> COLUMNDATA*;
		[[nodiscard]] auto GetColumnData(UINT uID)const->const COLUMNDATA*;
		[[nodiscard]] auto GetHdrIcon(UINT ID) -> HDRICON*;
		[[nodiscard]] auto GetListParent() -> HWND;
		[[nodiscard]] int HitTest(HDHITTESTINFO hhti);
		[[nodiscard]] bool IsEditable(UINT ID)const;
		[[nodiscard]] auto IsHidden(UINT ID)const->const HIDDEN*; //Internal ColumnID.
		[[nodiscard]] bool IsSortable(UINT ID)const;
		[[nodiscard]] bool IsWindow()const;
		auto OnDestroy() -> LRESULT;
		void OnDrawItem(HDC hDC, int iItem, RECT rc, bool fPressed, bool fHighl);
		auto OnLayout(const MSG& msg) -> LRESULT;
		auto OnLButtonDown(const MSG& msg) -> LRESULT;
		auto OnLButtonUp(const MSG& msg) -> LRESULT;
		auto OnPaint() -> LRESULT;
		auto OnRButtonUp(const MSG& msg) -> LRESULT;
		auto OnRButtonDown(const MSG& msg) -> LRESULT;
		static auto CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
			UINT_PTR uIDSubclass, DWORD_PTR dwRefData)->LRESULT;
	private:
		HWND m_hWnd { };         //Header window.
		HFONT m_hFntHdr { };
		COLORREF m_clrBkNWA { }; //Bk of non working area.
		COLORREF m_clrText { };
		COLORREF m_clrBk { };
		COLORREF m_clrHglInactive { };
		COLORREF m_clrHglActive { };
		DWORD m_dwHeaderHeight { 19 }; //Standard (default) height.
		UINT m_uSortColumn { 0 };   //ColumnID to draw sorting triangle at. 0 is to avoid triangle before first clicking.
		std::vector<HIDDEN> m_vecHidden; //Hidden columns.
		std::vector<COLUMNDATA> m_vecColumnData;
		bool m_fSortable { false }; //List-is-sortable global flog. Need to draw sortable triangle or not?
		bool m_fSortAscending { };  //Sorting type.
		bool m_fLMousePressed { };
	};

	//Hidden columns.
	struct CListExHdr::HIDDEN {
		UINT uID { };
		int iPrevPos { };
		int iPrevWidth { };
	};
}

using namespace HEXCTRL::LISTEX::INTERNAL;

void CListExHdr::DeleteColumn(int iIndex)
{
	if (const auto ID = ColumnIndexToID(iIndex); ID > 0) {
		std::erase_if(m_vecHidden, [=](const HIDDEN& ref) { return ref.uID == ID; });
		std::erase_if(m_vecColumnData, [=](const COLUMNDATA& ref) { return ref.uID == ID; });
	}
}

auto CListExHdr::GetClientRect()const->RECT
{
	assert(IsWindow());
	RECT rc;
	::GetClientRect(m_hWnd, &rc);
	return rc;
}

UINT CListExHdr::GetHiddenCount()const
{
	return static_cast<UINT>(m_vecHidden.size());
}

auto CListExHdr::GetImageList(int iList)const->HIMAGELIST
{
	assert(IsWindow());
	return reinterpret_cast<HIMAGELIST>(::SendMessageW(m_hWnd, HDM_GETIMAGELIST, iList, 0L));
}

bool CListExHdr::GetItem(int iPos, HDITEMW* pHDI)const
{
	assert(IsWindow());
	return ::SendMessageW(m_hWnd, HDM_GETITEM, iPos, reinterpret_cast<LPARAM>(pHDI));
}

int CListExHdr::GetItemCount()const
{
	assert(IsWindow());
	return static_cast<int>(::SendMessageW(m_hWnd, HDM_GETITEMCOUNT, 0, 0L));
}

auto CListExHdr::GetItemRect(int iIndex)const->RECT
{
	RECT rc;
	::SendMessageW(m_hWnd, HDM_GETITEMRECT, iIndex, reinterpret_cast<LPARAM>(&rc));
	return rc;
}

int CListExHdr::GetColumnDataAlign(int iIndex)const
{
	if (const auto pData = GetColumnData(ColumnIndexToID(iIndex)); pData != nullptr) {
		return pData->iDataAlign;
	}

	return -1;
}

void CListExHdr::HideColumn(int iIndex, bool fHide)
{
	const auto iColumnsCount = GetItemCount();
	if (iIndex >= iColumnsCount) {
		return;
	}

	const auto ID = ColumnIndexToID(iIndex);
	std::vector<int> vecInt(iColumnsCount, 0);
	ListView_GetColumnOrderArray(GetParent(), iColumnsCount, vecInt.data());
	HDITEMW hdi { .mask { HDI_WIDTH } };
	GetItem(iIndex, &hdi);

	if (fHide) { //Hide column.
		HIDDEN* pHidden;
		if (const auto it = std::find_if(m_vecHidden.begin(), m_vecHidden.end(),
			[=](const HIDDEN& ref) { return ref.uID == ID; }); it == m_vecHidden.end()) {
			pHidden = &m_vecHidden.emplace_back(HIDDEN { .uID { ID } });
		}
		else { pHidden = &*it; }

		pHidden->iPrevWidth = hdi.cxy;
		if (const auto iter = std::find(vecInt.begin(), vecInt.end(), iIndex); iter != vecInt.end()) {
			pHidden->iPrevPos = static_cast<int>(iter - vecInt.begin());
			std::rotate(iter, iter + 1, vecInt.end()); //Moving hiding column to the end of the column array.
		}

		ListView_SetColumnOrderArray(GetParent(), iColumnsCount, vecInt.data());
		hdi.cxy = 0;
		SetItem(iIndex, hdi);
	}
	else { //Show column.
		const auto pHidden = IsHidden(ID);
		if (pHidden == nullptr) { //No such column is hidden.
			return;
		}

		if (const auto iterRight = std::find(vecInt.rbegin(), vecInt.rend(), iIndex); iterRight != vecInt.rend()) {
			const auto iterMid = iterRight + 1;
			const auto iterEnd = vecInt.rend() - pHidden->iPrevPos;
			if (iterMid < iterEnd) {
				std::rotate(iterRight, iterMid, iterEnd); //Moving hidden column id back to its previous place.
			}
		}

		ListView_SetColumnOrderArray(GetParent(), iColumnsCount, vecInt.data());
		hdi.cxy = pHidden->iPrevWidth;
		SetItem(iIndex, hdi);
		std::erase_if(m_vecHidden, [=](const HIDDEN& ref) { return ref.uID == ID; });
	}

	RedrawWindow();
}

bool CListExHdr::IsColumnHidden(int iIndex)const
{
	return IsHidden(ColumnIndexToID(iIndex)) != nullptr;
}

bool CListExHdr::IsColumnSortable(int iIndex)const
{
	return IsSortable(ColumnIndexToID(iIndex));
}

bool CListExHdr::IsColumnEditable(int iIndex)const
{
	return IsEditable(ColumnIndexToID(iIndex));
}

auto CListExHdr::ProcessMsg(const MSG& msg)->LRESULT
{
	switch (msg.message) {
	case HDM_LAYOUT: return OnLayout(msg);
	case WM_DESTROY: return OnDestroy();
	case WM_LBUTTONDOWN: return OnLButtonDown(msg);
	case WM_LBUTTONUP: return OnLButtonUp(msg);
	case WM_PAINT: return OnPaint();
	case WM_RBUTTONDOWN: return OnRButtonDown(msg);
	case WM_RBUTTONUP: return OnRButtonUp(msg);
	default: return DefSubclassProc(msg.hwnd, msg.message, msg.wParam, msg.lParam);
	}
}

void CListExHdr::RedrawWindow()const
{
	assert(IsWindow());
	::RedrawWindow(m_hWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
}

void CListExHdr::SetFont(const LOGFONTW& lf)
{
	::DeleteObject(m_hFntHdr);
	m_hFntHdr = ::CreateFontIndirectW(&lf);

	//If new font's height is higher than current height (m_dwHeaderHeight), we adjust current height as well.
	TEXTMETRICW tm;
	const auto hDC = ::GetDC(m_hWnd);
	::SelectObject(hDC, m_hFntHdr);
	::GetTextMetricsW(hDC, &tm);
	::ReleaseDC(m_hWnd, hDC);
	const DWORD dwHeightFont = tm.tmHeight + tm.tmExternalLeading + 4;
	if (dwHeightFont > m_dwHeaderHeight) {
		SetHeight(dwHeightFont);
	}
}

void CListExHdr::SetHeight(DWORD dwHeight)
{
	m_dwHeaderHeight = dwHeight;
}

void CListExHdr::SetColor(const LISTEXCOLORS& lcs)
{
	m_clrText = lcs.clrHdrText;
	m_clrBk = lcs.clrHdrBk;
	m_clrBkNWA = lcs.clrNWABk;
	m_clrHglInactive = lcs.clrHdrHglInact;
	m_clrHglActive = lcs.clrHdrHglAct;

	RedrawWindow();
}

void CListExHdr::SetColumnColor(int iColumn, COLORREF clrBk, COLORREF clrText)
{
	const auto ID = ColumnIndexToID(iColumn);
	assert(ID > 0);
	if (ID == 0) {
		return;
	}

	if (clrText == -1) {
		clrText = m_clrText;
	}

	if (const auto pData = GetColumnData(ID); pData != nullptr) {
		pData->clr = { .clrBk { clrBk }, .clrText { clrText } };
	}
	else {
		AddColumnData({ .uID { ID }, .clr { .clrBk { clrBk }, .clrText { clrText } } });
	}

	RedrawWindow();
}

void CListExHdr::SetColumnDataAlign(int iColumn, int iAlign)
{
	const auto ID = ColumnIndexToID(iColumn);
	if (ID == 0) {
		assert(true);
		return;
	}

	if (const auto pData = GetColumnData(ID); pData != nullptr) {
		pData->iDataAlign = iAlign;
	}
	else {
		AddColumnData({ .uID { ID }, .clr { .clrBk { m_clrBk }, .clrText { m_clrText } }, .iDataAlign { iAlign } });
	}
}

void CListExHdr::SetColumnIcon(int iColumn, const LISTEXHDRICON& stIcon)
{
	const auto ID = ColumnIndexToID(iColumn);
	assert(ID > 0);
	if (ID == 0) {
		return;
	}

	if (const auto pData = GetColumnData(ID); pData != nullptr) {
		pData->icon.stIcon = stIcon;
	}
	else {
		AddColumnData({ .uID { ID }, .icon { .stIcon { stIcon } }, .clr { .clrBk { m_clrBk }, .clrText { m_clrText } } });
	}

	RedrawWindow();
}

void CListExHdr::SetColumnSortable(int iColumn, bool fSortable)
{
	const auto ID = ColumnIndexToID(iColumn);
	if (ID == 0) {
		assert(true);
		return;
	}

	if (const auto pData = GetColumnData(ID); pData != nullptr) {
		pData->fSortable = fSortable;
	}
	else {
		AddColumnData({ .uID { ID }, .clr { .clrBk { m_clrBk }, .clrText { m_clrText } }, .fSortable { fSortable } });
	}
}

void CListExHdr::SetColumnEditable(int iColumn, bool fEditable)
{
	const auto ID = ColumnIndexToID(iColumn);
	if (ID == 0) {
		assert(true);
		return;
	}

	if (const auto pData = GetColumnData(ID); pData != nullptr) {
		pData->fEditable = fEditable;
	}
	else {
		AddColumnData({ .uID { ID }, .clr { .clrBk { m_clrBk }, .clrText { m_clrText } }, .fEditable { fEditable } });
	}
}

void CListExHdr::SetImageList(HIMAGELIST pList, int iList)
{
	::SendMessageW(m_hWnd, HDM_SETIMAGELIST, iList, reinterpret_cast<LPARAM>(pList));
}

void CListExHdr::SetSortable(bool fSortable)
{
	m_fSortable = fSortable;
	RedrawWindow();
}

void CListExHdr::SetSortArrow(int iColumn, bool fAscending)
{
	UINT ID { 0 };
	if (iColumn >= 0) {
		ID = ColumnIndexToID(iColumn);
		assert(ID > 0);
		if (ID == 0) {
			return;
		}
	}

	m_uSortColumn = ID;
	m_fSortAscending = fAscending;
	RedrawWindow();
}

void CListExHdr::SubclassHeader(HWND hWndHeader)
{
	assert(hWndHeader != nullptr);
	::SetWindowSubclass(hWndHeader, SubclassProc, reinterpret_cast<UINT_PTR>(this), 0);
	m_hWnd = hWndHeader;
}


//CListExHdr private methods:

void CListExHdr::AddColumnData(const COLUMNDATA& data)
{
	m_vecColumnData.emplace_back(data);
}

UINT CListExHdr::ColumnIndexToID(int iIndex)const
{
	//Each column has unique internal identifier in HDITEMW::lParam.
	HDITEMW hdi { .mask { HDI_LPARAM } };
	const auto ret = GetItem(iIndex, &hdi);

	return ret ? static_cast<UINT>(hdi.lParam) : 0;
}

int CListExHdr::ColumnIDToIndex(UINT uID)const
{
	for (int iterColumns = 0; iterColumns < GetItemCount(); ++iterColumns) {
		HDITEMW hdi { .mask { HDI_LPARAM } };
		GetItem(iterColumns, &hdi);
		if (static_cast<UINT>(hdi.lParam) == uID) {
			return iterColumns;
		}
	}

	return -1;
}

auto CListExHdr::GetParent()->HWND
{
	assert(IsWindow());
	return ::GetParent(m_hWnd);
}

auto CListExHdr::GetHdrColor(UINT ID)const->PLISTEXCOLOR
{
	const auto pData = GetColumnData(ID);
	return pData != nullptr ? &pData->clr : nullptr;
}

auto CListExHdr::GetColumnData(UINT uID)->COLUMNDATA*
{
	const auto iter = std::find_if(m_vecColumnData.begin(), m_vecColumnData.end(), [=](const COLUMNDATA& ref) {
		return ref.uID == uID; });

	return iter == m_vecColumnData.end() ? nullptr : &*iter;
}

auto CListExHdr::GetColumnData(UINT uID)const->const COLUMNDATA*
{
	const auto iter = std::find_if(m_vecColumnData.begin(), m_vecColumnData.end(), [=](const COLUMNDATA& ref) {
		return ref.uID == uID; });

	return iter == m_vecColumnData.end() ? nullptr : &*iter;
}

auto CListExHdr::GetHdrIcon(UINT ID)->CListExHdr::HDRICON*
{
	if (GetImageList() == nullptr) {
		return nullptr;
	}

	const auto pData = GetColumnData(ID);
	return pData != nullptr && pData->icon.stIcon.iIndex != -1 ? &pData->icon : nullptr;
}

void CListExHdr::SetItem(int nPos, const HDITEMW& item)
{
	assert(IsWindow());
	::SendMessageW(m_hWnd, HDM_SETITEM, nPos, reinterpret_cast<LPARAM>(&item));
}

auto CListExHdr::GetListParent()->HWND
{
	const auto hWnd = GetParent();
	assert(hWnd != nullptr);
	return ::GetParent(hWnd);
}

int CListExHdr::HitTest(const HDHITTESTINFO hhti)
{
	assert(IsWindow());
	return static_cast<int>(::SendMessageW(m_hWnd, HDM_HITTEST, 0, reinterpret_cast<LPARAM>(&hhti)));
}

auto CListExHdr::IsHidden(UINT ID)const->const HIDDEN*
{
	const auto it = std::find_if(m_vecHidden.begin(), m_vecHidden.end(), [=](const HIDDEN& ref) { return ref.uID == ID; });
	return it != m_vecHidden.end() ? &*it : nullptr;
}

bool CListExHdr::IsSortable(UINT ID)const
{
	const auto pData = GetColumnData(ID);
	return pData == nullptr || pData->fSortable; //It's sortable unless found explicitly as false.
}

bool CListExHdr::IsWindow()const
{
	return ::IsWindow(m_hWnd);
}

bool CListExHdr::IsEditable(UINT ID)const
{
	const auto pData = GetColumnData(ID);
	return pData != nullptr && pData->fEditable; //It's editable only if found explicitly as true.
}

auto CListExHdr::OnDestroy()->LRESULT
{
	m_vecHidden.clear();
	m_vecColumnData.clear();

	return 0;
}

void CListExHdr::OnDrawItem(HDC hDC, int iItem, RECT rc, bool fPressed, bool fHighl)
{
	const wnd::CRect rcOrig(rc);
	const wnd::CDC dc(hDC);
	//Non working area after last column. Or if column is resized to zero width.
	if (iItem < 0 || rcOrig.IsRectEmpty()) {
		dc.FillSolidRect(rcOrig, m_clrBkNWA);
		return;
	}

	const auto ID = ColumnIndexToID(iItem);
	const auto pClr = GetHdrColor(ID);
	const auto clrText { pClr != nullptr ? pClr->clrText : m_clrText };
	const auto clrBk { fHighl ? (fPressed ? m_clrHglActive : m_clrHglInactive)
		: (pClr != nullptr ? pClr->clrBk : m_clrBk) };

	dc.FillSolidRect(rcOrig, clrBk);
	dc.SetTextColor(clrText);
	dc.SelectObject(m_hFntHdr);

	//Set item's text buffer first char to zero, then getting item's text and Draw it.
	wchar_t warrHdrText[MAX_PATH];
	warrHdrText[0] = L'\0';
	HDITEMW hdItem { .mask { HDI_FORMAT | HDI_TEXT }, .pszText { warrHdrText }, .cchTextMax { MAX_PATH } };
	GetItem(iItem, &hdItem);

	//Draw icon for column, if any.
	auto iTextIndentLeft { 5 }; //Left text indent.
	if (const auto pIcon = GetHdrIcon(ID); pIcon != nullptr) { //If column has an icon.
		const auto pImgList = GetImageList(LVSIL_NORMAL);
		int iCX { };
		int iCY { };
		::ImageList_GetIconSize(pImgList, &iCX, &iCY); //Icon dimensions.
		const auto pt = rcOrig.TopLeft() + pIcon->stIcon.pt;
		::ImageList_DrawEx(pImgList, pIcon->stIcon.iIndex, dc, pt.x, pt.y, 0, 0, CLR_NONE, CLR_NONE, ILD_NORMAL);
		iTextIndentLeft += pIcon->stIcon.pt.x + iCX;
	}

	UINT uFormat { DT_LEFT };
	switch (hdItem.fmt & HDF_JUSTIFYMASK) {
	case HDF_CENTER:
		uFormat = DT_CENTER;
		break;
	case HDF_RIGHT:
		uFormat = DT_RIGHT;
		break;
	default:
		break;
	}

	constexpr auto iTextIndentRight { 4 };
	wnd::CRect rcText { rcOrig.left + iTextIndentLeft, rcOrig.top, rcOrig.right - iTextIndentRight, rcOrig.bottom };
	if (std::wstring_view(warrHdrText).find(L'\n') != std::wstring_view::npos) {
		//If it's multiline text, first — calculate rect for the text,
		//with DT_CALCRECT flag (not drawing anything),
		//and then calculate rect for final vertical text alignment.
		wnd::CRect rcCalcText;
		dc.DrawTextW(warrHdrText, rcCalcText, uFormat | DT_CALCRECT);
		rcText.top = rcText.Height() / 2 - rcCalcText.Height() / 2;
	}
	else {
		uFormat |= DT_VCENTER | DT_SINGLELINE;
	}

	//Draw Header Text.
	dc.DrawTextW(warrHdrText, rcText, uFormat);

	//Draw sortable triangle (arrow).
	if (m_fSortable && IsSortable(ID) && ID == m_uSortColumn) {
		static const auto penArrow = ::CreatePen(PS_SOLID, 1, RGB(90, 90, 90));
		dc.SelectObject(penArrow);
		dc.SelectObject(::GetSysColorBrush(COLOR_3DFACE));
		if (m_fSortAscending) { //Draw the UP arrow.
			const POINT arrPt[] { { rcOrig.right - 10, 3 },
				{ rcOrig.right - 15, 8 }, { rcOrig.right - 5, 8 } };
			dc.Polygon(arrPt, 3);
		}
		else { //Draw the DOWN arrow.
			const POINT arrPt[] { { rcOrig.right - 10, 8 },
				{ rcOrig.right - 15, 3 }, { rcOrig.right - 5, 3 } };
			dc.Polygon(arrPt, 3);
		}
	}

	static const auto penGrid = ::CreatePen(PS_SOLID, 2, GetSysColor(COLOR_3DFACE));
	dc.SelectObject(penGrid);
	dc.MoveTo(rcOrig.TopLeft());
	dc.LineTo(rcOrig.left, rcOrig.bottom);
	if (iItem == GetItemCount() - 1) { //Last item.
		dc.MoveTo(rcOrig.right, rcOrig.top);
		dc.LineTo(rcOrig.BottomRight());
	}
}

auto CListExHdr::OnLayout(const MSG& msg)->LRESULT
{
	wnd::DefSubclassProc(msg);
	const auto pHDL = reinterpret_cast<LPHDLAYOUT>(msg.lParam);
	pHDL->pwpos->cy = m_dwHeaderHeight;	//New header height.
	pHDL->prc->top = m_dwHeaderHeight;  //Decreasing list's height begining by the new header's height.

	return TRUE;
}

auto CListExHdr::OnLButtonDown(const MSG& msg)->LRESULT
{
	wnd::DefSubclassProc(msg);
	const POINT pt { .x { GET_X_LPARAM(msg.lParam) }, .y { GET_Y_LPARAM(msg.lParam) } };
	if (const auto iItem = HitTest({ .pt { pt } }); iItem >= 0) {
		m_fLMousePressed = true;
		SetCapture(m_hWnd);
		const auto ID = ColumnIndexToID(iItem);
		if (const auto pIcon = GetHdrIcon(ID); pIcon != nullptr
			&& pIcon->stIcon.fClickable && IsHidden(ID) == nullptr) {
			int iCX { };
			int iCY { };
			ImageList_GetIconSize(GetImageList(), &iCX, &iCY);
			const wnd::CRect rcColumn = GetItemRect(iItem);

			const auto ptIcon = rcColumn.TopLeft() + pIcon->stIcon.pt;
			if (const wnd::CRect rcIcon(ptIcon, SIZE { iCX, iCY }); rcIcon.PtInRect(pt)) {
				pIcon->fLMPressed = true;
				return 0; //Do not invoke default handler.
			}
		}
	}
	return wnd::DefSubclassProc(msg);
}

auto CListExHdr::OnLButtonUp(const MSG& msg)->LRESULT
{
	wnd::DefSubclassProc(msg);
	m_fLMousePressed = false;
	ReleaseCapture();
	RedrawWindow();

	const POINT point { .x { GET_X_LPARAM(msg.lParam) }, .y { GET_Y_LPARAM(msg.lParam) } };
	if (const auto iItem = HitTest({ .pt { point } }); iItem >= 0) {
		for (auto& ref : m_vecColumnData) {
			if (!ref.icon.fLMPressed || ref.icon.stIcon.iIndex == -1) {
				continue;
			}

			int iCX { };
			int iCY { };
			ImageList_GetIconSize(GetImageList(), &iCX, &iCY);
			const wnd::CRect rcColumn = GetItemRect(iItem);
			const auto pt = rcColumn.TopLeft() + ref.icon.stIcon.pt;
			const wnd::CRect rcIcon(pt, SIZE { iCX, iCY });

			if (rcIcon.PtInRect(point)) {
				for (auto& iterData : m_vecColumnData) {
					iterData.icon.fLMPressed = false; //Remove fLMPressed flag from all columns.
				}

				const auto hParent = GetParent();
				const auto uCtrlId = static_cast<UINT>(::GetDlgCtrlID(hParent));
				const NMHEADERW hdr { .hdr { hParent, uCtrlId, LISTEX_MSG_HDRICONCLICK }, .iItem { ColumnIDToIndex(ref.uID) } };
				::SendMessageW(GetListParent(), WM_NOTIFY, static_cast<WPARAM>(uCtrlId), reinterpret_cast<LPARAM>(&hdr));
			}
			break;
		}
	}

	return 0;
}

auto CListExHdr::OnPaint()->LRESULT
{
	const wnd::CPaintDC dc(m_hWnd);
	const wnd::CMemDC memDC(dc, m_hWnd);
	wnd::CRect rcClient;
	::GetClientRect(m_hWnd, rcClient);
	wnd::CRect rectItem;
	const auto iItems = GetItemCount();
	LONG lMax = 0;

	for (int iItem = 0; iItem < iItems; ++iItem) {
		POINT ptCur;
		::GetCursorPos(&ptCur);
		::ScreenToClient(m_hWnd, &ptCur);
		const auto iHit = HitTest({ .pt { ptCur } });
		const auto fHighl = iHit == iItem;
		const auto fPressed = m_fLMousePressed && fHighl;
		rectItem = GetItemRect(iItem);
		OnDrawItem(memDC, iItem, rectItem, fPressed, fHighl);
		lMax = (std::max)(lMax, rectItem.right);
	}

	// Draw "tail border":
	if (iItems == 0) {
		rectItem = rcClient;
		rectItem.right++;
	}
	else {
		rectItem.left = lMax;
		rectItem.right = rcClient.right + 1;
	}

	OnDrawItem(memDC, -1, rectItem, FALSE, FALSE);

	return 0;
}

auto CListExHdr::OnRButtonDown(const MSG& msg)->LRESULT
{
	const POINT pt { .x { GET_X_LPARAM(msg.lParam) }, .y { GET_Y_LPARAM(msg.lParam) } };
	const auto hParent = GetParent();
	const auto uCtrlId = static_cast<UINT>(::GetDlgCtrlID(hParent));
	const auto iItem = HitTest({ .pt { pt } });
	NMHEADERW hdr { .hdr { hParent, uCtrlId, LISTEX_MSG_HDRRBTNDOWN }, .iItem { iItem } };
	SendMessageW(GetListParent(), WM_NOTIFY, static_cast<WPARAM>(uCtrlId), reinterpret_cast<LPARAM>(&hdr));

	return wnd::DefSubclassProc(msg);
}

auto CListExHdr::OnRButtonUp(const MSG& msg)->LRESULT
{
	const POINT pt { .x { GET_X_LPARAM(msg.lParam) }, .y { GET_Y_LPARAM(msg.lParam) } };
	const auto hParent = GetParent();
	const auto uCtrlId = static_cast<UINT>(GetDlgCtrlID(hParent));
	const auto iItem = HitTest({ .pt { pt } });
	NMHEADERW hdr { { hParent, uCtrlId, LISTEX_MSG_HDRRBTNUP } };
	hdr.iItem = iItem;
	SendMessageW(GetListParent(), WM_NOTIFY, static_cast<WPARAM>(uCtrlId), reinterpret_cast<LPARAM>(&hdr));

	return wnd::DefSubclassProc(msg);
}

auto CListExHdr::SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIDSubclass, DWORD_PTR /*dwRefData*/) -> LRESULT {
	if (uMsg == WM_NCDESTROY) {
		::RemoveWindowSubclass(hWnd, SubclassProc, uIDSubclass);
	}

	const auto pHdr = reinterpret_cast<CListExHdr*>(uIDSubclass);
	return pHdr->ProcessMsg({ .hwnd { hWnd }, .message { uMsg }, .wParam { wParam }, .lParam { lParam } });
}


//CListEx.
namespace HEXCTRL::LISTEX {
	export class CListEx final {
	public:
		bool Create(const LISTEXCREATE& lcs);
		void CreateDialogCtrl(UINT uCtrlID, HWND hWndParent);
		bool DeleteAllItems();
		bool DeleteColumn(int iIndex);
		bool DeleteItem(int iItem);
		void DrawItem(LPDRAWITEMSTRUCT pDIS);
		bool EnsureVisible(int iItem, bool fPartialOK)const;
		[[nodiscard]] auto GetColors()const->const LISTEXCOLORS &;
		[[nodiscard]] auto GetColumnSortMode(int iColumn)const->EListExSortMode;
		[[nodiscard]] auto GetFont()const->LOGFONTW;
		[[nodiscard]] auto GetHWND()const->HWND;
		[[nodiscard]] auto GetImageList(int iList = HDSIL_NORMAL)const->HIMAGELIST;
		void GetItem(LVITEMW* pItem)const;
		[[nodiscard]] int GetItemCount()const;
		[[nodiscard]] auto GetItemRect(int iItem, int iArea)const->RECT;
		[[nodiscard]] auto GetItemText(int iItem, int iSubItem)const->std::wstring;
		[[nodiscard]] int GetNextItem(int iItem, int iFlags)const;
		[[nodiscard]] auto GetSelectedCount()const->UINT;
		[[nodiscard]] int GetSelectionMark()const;
		[[nodiscard]] int GetSortColumn()const;
		[[nodiscard]] bool GetSortAscending()const;
		[[nodiscard]] auto GetSubItemRect(int iItem, int iSubItem, int iArea)const->RECT;
		[[nodiscard]] int GetTopIndex()const;
		void HideColumn(int iIndex, bool fHide);
		bool HitTest(LVHITTESTINFO* pHTI)const;
		int InsertColumn(int iCol, const LVCOLUMNW* pColumn)const;
		int InsertColumn(int iCol, const LVCOLUMNW* pColumn, int iDataAlign, bool fEditable = false);
		int InsertColumn(int iCol, LPCWSTR pwszName, int iFormat = LVCFMT_LEFT, int iWidth = -1,
			int iSubItem = -1, int iDataAlign = LVCFMT_LEFT, bool fEditable = false);
		[[nodiscard]] bool IsCreated()const;
		[[nodiscard]] bool IsColumnSortable(int iColumn);
		[[nodiscard]] auto MapIndexToID(UINT uIndex)const->UINT;
		[[nodiscard]] auto MapIDToIndex(UINT uID)const->UINT;
		void MeasureItem(LPMEASUREITEMSTRUCT pMIS);
		auto ProcessMsg(const MSG& msg) -> LRESULT;
		void RedrawWindow()const;
		void ResetSort(); //Reset all the sort by any column to its default state.
		void Scroll(SIZE size)const;
		void SetColors(const LISTEXCOLORS& lcs);
		void SetColumn(int iCol, const LVCOLUMNW* pColumn)const;
		void SetColumnEditable(int iColumn, bool fEditable);
		void SetColumnSortMode(int iColumn, bool fSortable, EListExSortMode eSortMode = EListExSortMode::SORT_LEX);
		auto SetExtendedStyle(DWORD dwExStyle)const->DWORD;
		void SetFont(const LOGFONTW& lf);
		void SetHdrColumnColor(int iColumn, COLORREF clrBk, COLORREF clrText = -1);
		void SetHdrColumnIcon(int iColumn, const LISTEXHDRICON& stIcon); //Icon for a given column.
		void SetHdrFont(const LOGFONTW& lf);
		void SetHdrHeight(DWORD dwHeight);
		void SetHdrImageList(HIMAGELIST pList);
		auto SetImageList(HIMAGELIST hList, int iListType) -> HIMAGELIST;
		void SetItemCountEx(int iCount, DWORD dwFlags = LVSICF_NOINVALIDATEALL)const;
		bool SetItemState(int iItem, LVITEMW* pItem)const;
		bool SetItemState(int iItem, UINT uState, UINT uStateMask)const;
		void SetItemText(int iItem, int iSubItem, LPCWSTR pwszText);
		void SetSortable(bool fSortable, PFNLVCOMPARE pfnCompare = nullptr,
			EListExSortMode eSortMode = EListExSortMode::SORT_LEX);
		void SortItemsEx(PFNLVCOMPARE pfnCompare, DWORD_PTR dwData)const;
		void Update(int iItem)const;
		static auto CALLBACK DefCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)->int;
	private:
		struct ITEMDATA;
		[[nodiscard]] long GetFontSize()const;
		[[nodiscard]] auto GetHeader() -> CListExHdr& { return m_Hdr; }
		void FontSizeIncDec(bool fInc);
		[[nodiscard]] bool IsWindow()const;
		[[nodiscard]] auto GetCustomColor(int iItem, int iSubItem)const->std::optional<LISTEXCOLOR>;
		[[nodiscard]] int GetIcon(int iItem, int iSubItem)const; //Does cell have an icon associated.
		[[nodiscard]] auto GetTooltip(int iItem, int iSubItem)const->std::optional<LISTEXTTDATA>;
		auto OnCommand(const MSG& msg) -> LRESULT;
		auto OnDestroy() -> LRESULT;
		void OnEditInPlaceEnterPressed();
		void OnEditInPlaceKillFocus();
		auto OnEraseBkgnd() -> LRESULT;
		auto OnHdnBegindrag(NMHDR* pNMHDR) -> LRESULT;
		auto OnHdnBegintrack(NMHDR* pNMHDR) -> LRESULT;
		auto OnHScroll(const MSG& msg) -> LRESULT;
		auto OnLButtonDblClk(const MSG& msg) -> LRESULT;
		auto OnLButtonDown(const MSG& msg) -> LRESULT;
		auto OnLButtonUp(const MSG& msg) -> LRESULT;
		auto OnMouseMove(const MSG& msg) -> LRESULT;
		auto OnMouseWheel(const MSG& msg) -> LRESULT;
		auto OnNotify(const MSG& msg) -> LRESULT;
		void OnNotifyEditInPlace(NMHDR* pNMHDR);
		auto OnPaint() -> LRESULT;
		auto OnTimer(const MSG& msg) -> LRESULT;
		auto OnVScroll(const MSG& msg) -> LRESULT;
		auto ParseItemData(int iItem, int iSubitem) -> std::vector<ITEMDATA>;
		void RecalcMeasure()const;
		void SetFontSize(long lSize);
		void TTCellShow(bool fShow, bool fTimer = false);
		void TTLinkShow(bool fShow, bool fTimer = false);
		void TTHLShow(bool fShow, UINT uRow); //Tooltips for high latency mode.
		static auto CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
			UINT_PTR uIDSubclass, DWORD_PTR dwRefData)->LRESULT;
		static auto CALLBACK EditSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
			UINT_PTR uIDSubclass, DWORD_PTR dwRefData)->LRESULT;
	private:
		static constexpr ULONG_PTR m_uIDTCellTTActivate { 0x01 }; //Cell tool-tip activate-timer ID.
		static constexpr ULONG_PTR m_uIDTCellTTCheck { 0x02 };    //Cell tool-tip check-timer ID.
		static constexpr ULONG_PTR m_uIDTLinkTTActivate { 0x03 }; //Link tool-tip activate-timer ID.
		static constexpr ULONG_PTR m_uIDTLinkTTCheck { 0x04 };    //Link tool-tip check-timer ID.
		static constexpr auto m_uIDEditInPlace { 0x01U };         //In place edit-box ID.
		struct COLUMNDATA {
			int iIndex { };
			EListExSortMode eSortMode { };
		};
		CListExHdr m_Hdr;               //List header control.
		HWND m_hWnd { };                //Main window.
		HFONT m_hFntList { };           //Default list font.
		HFONT m_hFntListUnderline { };  //Underlined list font, for links.
		HPEN m_hPenGrid { };            //Pen for list lines between cells.
		HWND m_hWndCellTT { };          //Cells tool-tip window.
		HWND m_hWndLinkTT { };          //Links tool-tip window.
		HWND m_hWndRowTT { };           //Tooltip window for row in m_fHighLatency mode.
		HWND m_hWndEditInPlace { };     //Edit box for in-place cells editing.
		wnd::CRect m_rcLinkCurr;        //Current link's rect;
		LISTEXCOLORS m_stColors { };
		std::vector<COLUMNDATA> m_vecColumnData; //Column data.
		std::wstring m_wstrTTText;      //Tool-tip current text.
		std::wstring m_wstrTTCaption;   //Tool-tip current caption.
		std::chrono::steady_clock::time_point m_tmTT; //Start time of the tooltip.
		LVHITTESTINFO m_htiCurrCell { }; //Cells hit struct for tool-tip.
		LVHITTESTINFO m_htiCurrLink { }; //Links hit struct for tool-tip.
		LVHITTESTINFO m_htiEdit { };    //Hit struct for in-place editing.
		PFNLVCOMPARE m_pfnCompare { };  //Pointer to a user provided compare func.
		DWORD m_dwGridWidth { };        //Grid width.
		DWORD m_dwTTDelayTime { };      //Tooltip delay before showing, in milliseconds.
		DWORD m_dwTTShowTime { };       //Tooltip show up time, in milliseconds.
		POINT m_ptTTOffset { };         //Tooltip offset from the cursor point.
		UINT m_uHLItem { };             //High latency Vscroll item.
		int m_iSortColumn { -1 };       //Currently clicked header column.
		int m_iLOGPIXELSY { };          //GetDeviceCaps(LOGPIXELSY) constant.
		EListExSortMode m_eDefSortMode { EListExSortMode::SORT_LEX }; //Default sorting mode.
		bool m_fCreated { false };      //Is created.
		bool m_fHighLatency { false };  //High latency flag.
		bool m_fSortable { false };     //Is list sortable.
		bool m_fSortAsc { false };      //Sorting type (ascending, descending).
		bool m_fLinks { false };        //Enable links support.
		bool m_fLinkUnderl { false };   //Links are displayed underlined or not.
		bool m_fLinkTooltip { false };  //Show links toolips.
		bool m_fVirtual { false };      //Whether list is virtual (LVS_OWNERDATA) or not.
		bool m_fCellTTActive { false }; //Is cell's tool-tip shown atm.
		bool m_fLinkTTActive { false }; //Is link's tool-tip shown atm.
		bool m_fLDownAtLink { false };  //Left mouse down on link.
		bool m_fHLFlag { false };       //High latency Vscroll flag.
	};

	//Text and links in the cell.
	struct CListEx::ITEMDATA {
		ITEMDATA(int iIconIndex, wnd::CRect rc) : rc(rc), iIconIndex(iIconIndex) { }; //Ctor for just image index.
		ITEMDATA(std::wstring_view wsvText, std::wstring_view wsvLink, std::wstring_view wsvTitle,
			wnd::CRect rc, bool fLink = false, bool fTitle = false) :
			wstrText(wsvText), wstrLink(wsvLink), wstrTitle(wsvTitle), rc(rc), fLink(fLink), fTitle(fTitle) {
		}
		std::wstring wstrText;  //Visible text.
		std::wstring wstrLink;  //Text within link <link="textFromHere"> tag.
		std::wstring wstrTitle; //Text within title <...title="textFromHere"> tag.
		wnd::CRect rc;          //Rect text belongs to.
		int iIconIndex { -1 };  //Icon index in the image list, if any.
		bool fLink { false };   //Is it just a text (wstrLink is empty) or text with link?
		bool fTitle { false };  //Is it a link with custom title (wstrTitle is not empty)?
	};
}

using namespace HEXCTRL::LISTEX;

bool CListEx::Create(const LISTEXCREATE& lcs)
{
	assert(!IsCreated());
	if (IsCreated()) {
		return false;
	}

	HWND hWnd { }; //Main window.
	auto dwStyle = lcs.dwStyle | WS_CHILD | LVS_OWNERDRAWFIXED | LVS_REPORT;
	if (lcs.fDialogCtrl) {
		hWnd = ::GetDlgItem(lcs.hWndParent, lcs.uID);
		if (hWnd == nullptr) {
			assert(false);
			return false;
		}

		dwStyle |= GetWindowLongPtrW(hWnd, GWL_STYLE);
		SetWindowLongPtrW(hWnd, GWL_STYLE, dwStyle);
	}
	else {
		const wnd::CRect rc = lcs.rect;
		if (hWnd = ::CreateWindowExW(lcs.dwExStyle, WC_LISTVIEWW, nullptr, dwStyle, rc.left, rc.top, rc.Width(),
			rc.Height(), lcs.hWndParent, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(lcs.uID)), nullptr, this);
			hWnd == nullptr) {
			assert(false);
			return false;
		}
	}

	if (::SetWindowSubclass(hWnd, SubclassProc, reinterpret_cast<UINT_PTR>(this), 0) == FALSE) {
		assert(false);
		return false;
	}

	GetHeader().SubclassHeader(::GetDlgItem(hWnd, 0));

	m_fVirtual = dwStyle & LVS_OWNERDATA;
	m_hWnd = hWnd;

	if (lcs.pColors != nullptr) {
		m_stColors = *lcs.pColors;
	}

	m_fSortable = lcs.fSortable;
	m_fLinks = lcs.fLinks;
	m_fLinkUnderl = lcs.fLinkUnderline;
	m_fLinkTooltip = lcs.fLinkTooltip;
	m_fHighLatency = lcs.fHighLatency;
	m_dwGridWidth = lcs.dwWidthGrid;
	m_dwTTDelayTime = lcs.dwTTDelayTime;
	m_dwTTShowTime = lcs.dwTTShowTime;
	m_ptTTOffset = lcs.ptTTOffset;

	//Cell Tooltip.
	if (m_hWndCellTT = ::CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr, lcs.dwTTStyleCell, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, nullptr, nullptr); m_hWndCellTT == nullptr) {
		assert(false);
		return false;
	}

	//The TTF_TRACK flag should not be used with the TTS_BALLOON tooltips, because in this case
	//the balloon will always be positioned _below_ provided coordinates.
	TTTOOLINFOW ttiCell { .cbSize { sizeof(TTTOOLINFOW) },
		.uFlags { static_cast<UINT>(lcs.dwTTStyleCell & TTS_BALLOON ? 0 : TTF_TRACK) } };
	::SendMessageW(m_hWndCellTT, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&ttiCell));
	::SendMessageW(m_hWndCellTT, TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(400)); //to allow use of newline \n.
	if (m_stColors.clrTooltipText != 0xFFFFFFFFUL || m_stColors.clrTooltipBk != 0xFFFFFFFFUL) {
		//To prevent Windows from changing theme of cells tooltip window.
		//Without this call Windows draws Tooltips with current Theme colors, and
		//TTM_SETTIPTEXTCOLOR/TTM_SETTIPBKCOLOR have no effect.
		::SetWindowTheme(m_hWndCellTT, nullptr, L"");
		::SendMessageW(m_hWndCellTT, TTM_SETTIPTEXTCOLOR, static_cast<WPARAM>(m_stColors.clrTooltipText), 0);
		::SendMessageW(m_hWndCellTT, TTM_SETTIPBKCOLOR, static_cast<WPARAM>(m_stColors.clrTooltipBk), 0);
	}

	//Link Tooltip.
	if (m_hWndLinkTT = ::CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr, lcs.dwTTStyleLink, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, nullptr, nullptr); m_hWndLinkTT == nullptr) {
		assert(false);
		return false;
	}

	TTTOOLINFOW ttiLink { .cbSize { sizeof(TTTOOLINFOW) },
		.uFlags { static_cast<UINT>(lcs.dwTTStyleLink & TTS_BALLOON ? 0 : TTF_TRACK) } };
	::SendMessageW(m_hWndLinkTT, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&ttiLink));
	::SendMessageW(m_hWndLinkTT, TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(400)); //to allow use of newline \n.

	if (m_fHighLatency) { //Tooltip for HighLatency mode.
		if (m_hWndRowTT = ::CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr,
			TTS_NOANIMATE | TTS_NOFADE | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, nullptr, nullptr, nullptr); m_hWndRowTT == nullptr) {
			assert(false);
			return false;
		}

		TTTOOLINFOW ttiHL { .cbSize { sizeof(TTTOOLINFOW) }, .uFlags { static_cast<UINT>(TTF_TRACK) } };
		::SendMessageW(m_hWndRowTT, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&ttiHL));
	}

	const auto hDC = ::GetDC(m_hWnd);
	m_iLOGPIXELSY = ::GetDeviceCaps(hDC, LOGPIXELSY);
	NONCLIENTMETRICSW ncm { .cbSize { sizeof(NONCLIENTMETRICSW) } };
	::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0); //Get System Default UI Font.
	ncm.lfMessageFont.lfHeight = -MulDiv(lcs.dwSizeFontList, m_iLOGPIXELSY, 72);
	LOGFONTW lfList { lcs.pLFList != nullptr ? *lcs.pLFList : ncm.lfMessageFont };
	m_hFntList = ::CreateFontIndirectW(&lfList);
	lfList.lfUnderline = TRUE;
	m_hFntListUnderline = ::CreateFontIndirectW(&lfList);
	ncm.lfMessageFont.lfHeight = -MulDiv(lcs.dwSizeFontHdr, m_iLOGPIXELSY, 72);
	const LOGFONTW lfHdr { lcs.pLFHdr != nullptr ? *lcs.pLFHdr : ncm.lfMessageFont };
	const auto fntDefault = ::CreateFontIndirectW(&lfHdr);
	TEXTMETRICW tm;
	::SelectObject(hDC, fntDefault);
	::GetTextMetricsW(hDC, &tm);
	const DWORD dwHdrHeight = lcs.dwHdrHeight == 0 ?
		tm.tmHeight + tm.tmExternalLeading + MulDiv(5, m_iLOGPIXELSY, 72) : lcs.dwHdrHeight; //Header is a bit higher than list rows.
	::ReleaseDC(m_hWnd, hDC);

	m_hPenGrid = ::CreatePen(PS_SOLID, m_dwGridWidth, m_stColors.clrListGrid);
	::SetClassLongPtrW(m_hWnd, GCLP_HCURSOR, 0); //To prevent cursor from blinking.
	RecalcMeasure();

	m_fCreated = true;

	GetHeader().SetColor(m_stColors);
	GetHeader().SetSortable(lcs.fSortable);
	SetHdrHeight(dwHdrHeight);
	SetHdrFont(lfHdr);
	Update(0);

	return true;
}

void CListEx::CreateDialogCtrl(UINT uCtrlID, HWND hWndParent)
{
	LISTEXCREATE lcs;
	lcs.hWndParent = hWndParent;
	lcs.uID = uCtrlID;
	lcs.fDialogCtrl = true;

	Create(lcs);
}

bool CListEx::DeleteAllItems()
{
	assert(IsCreated());
	if (!IsCreated()) {
		return FALSE;
	}

	::DestroyWindow(m_hWndEditInPlace);

	return ::SendMessageW(m_hWnd, LVM_DELETEALLITEMS, 0, 0L) != 0;
}

bool CListEx::DeleteColumn(int iIndex)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return FALSE;
	}

	std::erase_if(m_vecColumnData, [=](const COLUMNDATA& ref) { return ref.iIndex == iIndex; });
	GetHeader().DeleteColumn(iIndex);

	return ::SendMessageW(m_hWnd, LVM_DELETECOLUMN, iIndex, 0) != 0;
}

bool CListEx::DeleteItem(int iItem)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return FALSE;
	}

	return ::SendMessageW(m_hWnd, LVM_DELETEITEM, iItem, 0L) != 0;
}

bool CListEx::EnsureVisible(int iItem, bool fPartialOK)const
{
	assert(IsCreated());
	if (!IsCreated()) {
		return { };
	}

	return static_cast<bool>(::SendMessageW(m_hWnd, LVM_ENSUREVISIBLE, iItem, MAKELPARAM(fPartialOK, 0)));
}

auto CListEx::GetColors()const->const LISTEXCOLORS& {
	return m_stColors;
}

auto CListEx::GetColumnSortMode(int iColumn)const->EListExSortMode
{
	assert(IsCreated());

	if (const auto it = std::find_if(m_vecColumnData.begin(), m_vecColumnData.end(),
		[=](const COLUMNDATA& ref) { return ref.iIndex == iColumn; }); it != m_vecColumnData.end()) {
		return it->eSortMode;
	}

	return m_eDefSortMode;
}

auto CListEx::GetFont()const->LOGFONTW
{
	assert(IsCreated());
	if (!IsCreated()) {
		return { };
	}

	LOGFONTW lf { };
	::GetObjectW(m_hFntList, sizeof(lf), &lf);

	return lf;
}

auto CListEx::GetHWND()const->HWND
{
	if (!IsCreated()) {
		return { };
	}

	return m_hWnd;
}

auto CListEx::GetImageList(int iList)const->HIMAGELIST
{
	assert(IsCreated());
	if (!IsCreated()) {
		return { };
	}

	return reinterpret_cast<HIMAGELIST>(::SendMessageW(m_hWnd, LVM_GETIMAGELIST, iList, 0L));
}

void CListEx::GetItem(LVITEMW* pItem)const
{
	assert(IsCreated());
	if (!IsCreated()) {
		return;
	}

	::SendMessageW(m_hWnd, LVM_GETITEMW, 0, reinterpret_cast<LPARAM>(pItem));
}

int CListEx::GetItemCount()const
{
	assert(IsCreated());
	if (!IsCreated()) {
		return -1;
	}

	return static_cast<int>(::SendMessageW(m_hWnd, LVM_GETITEMCOUNT, 0, 0L));
}

auto CListEx::GetItemRect(int iItem, int iArea)const->RECT
{
	assert(IsCreated());
	if (!IsCreated()) {
		return { };
	}

	assert(iArea == LVIR_BOUNDS || iArea == LVIR_ICON || iArea == LVIR_LABEL || iArea == LVIR_SELECTBOUNDS);

	RECT rc { .left { iArea } };
	const bool ret = ::SendMessageW(m_hWnd, LVM_GETITEMRECT, iItem, reinterpret_cast<LPARAM>(&rc));

	return ret ? rc : RECT { };
}

auto CListEx::GetItemText(int iItem, int iSubItem)const->std::wstring
{
	assert(IsCreated());
	if (!IsCreated()) {
		return { };
	}

	wchar_t buff[256];
	LVITEMW item { .mask { LVIF_TEXT }, .iItem { iItem }, .iSubItem { iSubItem },
		.pszText { buff }, .cchTextMax { 256 } };
	GetItem(&item);
	return item.pszText;
}

int CListEx::GetNextItem(int iItem, int iFlags)const
{
	assert(IsCreated());
	if (!IsCreated()) {
		return { };
	}

	return static_cast<int>(::SendMessageW(m_hWnd, LVM_GETNEXTITEM, iItem, MAKELPARAM(iFlags, 0)));
}

auto CListEx::GetSelectedCount()const->UINT
{
	assert(IsCreated());
	if (!IsCreated()) {
		return { };
	}

	return static_cast<UINT>(::SendMessageW(m_hWnd, LVM_GETSELECTEDCOUNT, 0, 0L));
}

int CListEx::GetSelectionMark()const
{
	assert(IsCreated());
	if (!IsCreated()) {
		return { };
	}

	return static_cast<int>(::SendMessageW(m_hWnd, LVM_GETSELECTIONMARK, 0, 0));
}

long CListEx::GetFontSize()const
{
	assert(IsCreated());
	if (!IsCreated()) {
		return { };
	}

	LOGFONTW lf { };
	::GetObjectW(m_hFntList, sizeof(lf), &lf);

	return lf.lfHeight;
}

int CListEx::GetSortColumn()const
{
	assert(IsCreated());
	if (!IsCreated()) {
		return -1;
	}

	return m_iSortColumn;
}

bool CListEx::GetSortAscending()const
{
	assert(IsCreated());
	if (!IsCreated()) {
		return false;
	}

	return m_fSortAsc;
}

auto CListEx::GetSubItemRect(int iItem, int iSubItem, int iArea)const->RECT
{
	assert(IsCreated());
	if (!IsCreated()) {
		return { };
	}

	assert(iArea == LVIR_BOUNDS || iArea == LVIR_ICON || iArea == LVIR_LABEL || iArea == LVIR_SELECTBOUNDS);

	RECT rc { .left { iArea }, .top { iSubItem } };
	const bool ret = ::SendMessageW(m_hWnd, LVM_GETSUBITEMRECT, iItem, reinterpret_cast<LPARAM>(&rc));

	return ret ? rc : RECT { };
}

int CListEx::GetTopIndex()const
{
	assert(IsCreated());
	if (!IsCreated()) {
		return { };
	}

	return static_cast<int>(::SendMessageW(m_hWnd, LVM_GETTOPINDEX, 0, 0));
}

void CListEx::HideColumn(int iIndex, bool fHide)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return;
	}

	GetHeader().HideColumn(iIndex, fHide);
	RedrawWindow();
}

bool CListEx::HitTest(LVHITTESTINFO* pHTI)const
{
	assert(IsCreated());
	if (!IsCreated()) {
		return false;
	}

	return ::SendMessageW(m_hWnd, LVM_SUBITEMHITTEST, 0, reinterpret_cast<LPARAM>(pHTI)) != -1;
}

int CListEx::InsertColumn(int iCol, const LVCOLUMNW* pColumn)const
{
	return static_cast<int>(::SendMessageW(m_hWnd, LVM_INSERTCOLUMN, iCol, reinterpret_cast<LPARAM>(pColumn)));
}

int CListEx::InsertColumn(int iCol, const LVCOLUMNW* pColumn, int iDataAlign, bool fEditable)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return -1;
	}

	auto& refHdr = GetHeader();
	const auto nHiddenCount = refHdr.GetHiddenCount();

	//Checking that the new column ID (nCol) not greater than the count of 
	//the header items minus count of the already hidden columns.
	if (nHiddenCount > 0 && iCol >= static_cast<int>(refHdr.GetItemCount() - nHiddenCount)) {
		iCol = refHdr.GetItemCount() - nHiddenCount;
	}

	const auto iNewIndex = InsertColumn(iCol, pColumn);

	//Assigning each column a unique internal random identifier.
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<unsigned int> distrib(1, (std::numeric_limits<unsigned int>::max)());
	refHdr.SetItem(iNewIndex, { .mask { HDI_LPARAM }, .lParam { static_cast<LPARAM>(distrib(gen)) } });
	refHdr.SetColumnDataAlign(iNewIndex, iDataAlign);

	//First (zero index) column is always left-aligned by default, no matter what the pColumn->fmt is set to.
	//To change the alignment a user must explicitly call the SetColumn after the InsertColumn.
	//This call here is just to remove that absurd limitation.
	const LVCOLUMNW stCol { .mask { LVCF_FMT }, .fmt { pColumn->fmt } };
	SetColumn(iNewIndex, &stCol);

	//All new columns are not editable by default.
	if (fEditable) {
		SetColumnEditable(iNewIndex, true);
	}

	return iNewIndex;
}

int CListEx::InsertColumn(int iCol, LPCWSTR pwszName, int iFormat, int iWidth, int iSubItem, int iDataAlign, bool fEditable)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return -1;
	}

	const LVCOLUMNW lvcol { .mask { LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_TEXT }, .fmt { iFormat },
		.cx { iWidth }, .pszText { const_cast<LPWSTR>(pwszName) }, .iSubItem { iSubItem } };
	return InsertColumn(iCol, &lvcol, iDataAlign, fEditable);
}

bool CListEx::IsCreated()const
{
	return m_fCreated;
}

bool CListEx::IsColumnSortable(int iColumn)
{
	return GetHeader().IsColumnSortable(iColumn);
}

auto CListEx::MapIndexToID(UINT uIndex)const->UINT
{
	assert(IsWindow());
	return static_cast<UINT>(::SendMessageW(m_hWnd, LVM_MAPINDEXTOID, static_cast<WPARAM>(uIndex), 0));
}

auto CListEx::MapIDToIndex(UINT uID)const->UINT
{
	assert(IsWindow());
	return static_cast<UINT>(::SendMessageW(m_hWnd, LVM_MAPIDTOINDEX, static_cast<WPARAM>(uID), 0));
}

auto CListEx::ProcessMsg(const MSG& msg)->LRESULT
{
	switch (msg.message) {
	case WM_COMMAND: return OnCommand(msg);
	case WM_DESTROY: return OnDestroy();
	case WM_ERASEBKGND: return OnEraseBkgnd();
	case WM_HSCROLL: return OnHScroll(msg);
	case WM_LBUTTONDBLCLK: return OnLButtonDblClk(msg);
	case WM_LBUTTONDOWN: return OnLButtonDown(msg);
	case WM_LBUTTONUP: return OnLButtonUp(msg);
	case WM_MOUSEMOVE: return OnMouseMove(msg);
	case WM_MOUSEWHEEL: return OnMouseWheel(msg);
	case WM_NOTIFY: return OnNotify(msg);
	case WM_PAINT: return OnPaint();
	case WM_TIMER: return OnTimer(msg);
	case WM_VSCROLL: return OnVScroll(msg);
	default: return wnd::DefSubclassProc(msg);
	}
}

void CListEx::ResetSort()
{
	m_iSortColumn = -1;
	GetHeader().SetSortArrow(-1, false);
}

void CListEx::Scroll(SIZE size)const
{
	::SendMessageW(m_hWnd, LVM_SCROLL, size.cx, size.cy);
}

void CListEx::SetColors(const LISTEXCOLORS& lcs)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return;
	}

	m_stColors = lcs;
	GetHeader().SetColor(lcs);
	RedrawWindow();
}

void CListEx::SetColumn(int iCol, const LVCOLUMNW* pColumn)const
{
	assert(IsCreated());
	if (!IsCreated()) {
		return;
	}

	::SendMessageW(m_hWnd, LVM_SETCOLUMN, iCol, reinterpret_cast<LPARAM>(pColumn));
}

void CListEx::SetColumnEditable(int iColumn, bool fEditable)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return;
	}

	GetHeader().SetColumnEditable(iColumn, fEditable);
}

void CListEx::SetColumnSortMode(int iColumn, bool fSortable, EListExSortMode eSortMode)
{
	if (const auto it = std::find_if(m_vecColumnData.begin(), m_vecColumnData.end(),
		[=](const COLUMNDATA& ref) { return ref.iIndex == iColumn; }); it != m_vecColumnData.end()) {
		it->eSortMode = eSortMode;
	}
	else { m_vecColumnData.emplace_back(COLUMNDATA { .iIndex { iColumn }, .eSortMode { eSortMode } }); }

	GetHeader().SetColumnSortable(iColumn, fSortable);
}

auto CListEx::SetExtendedStyle(DWORD dwExStyle)const->DWORD
{
	assert(IsCreated());
	if (!IsCreated()) {
		return { };
	}

	return static_cast<DWORD>(::SendMessageW(m_hWnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwExStyle));
}

void CListEx::SetFont(const LOGFONTW& lf)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return;
	}

	::DeleteObject(m_hFntList);
	m_hFntList = ::CreateFontIndirectW(&lf);
	LOGFONTW lfu { lf };
	lfu.lfUnderline = TRUE;
	::DeleteObject(m_hFntListUnderline);
	m_hFntListUnderline = ::CreateFontIndirectW(&lfu);

	RecalcMeasure();
	Update(0);

	if (::IsWindow(m_hWndEditInPlace)) { //If m_hWndEditInPlace is active, ammend its rect.
		wnd::CRect rcCell = GetSubItemRect(m_htiEdit.iItem, m_htiEdit.iSubItem, LVIR_BOUNDS);
		if (m_htiEdit.iSubItem == 0) { //Clicked on item (first column).
			auto rcLabel = GetItemRect(m_htiEdit.iItem, LVIR_LABEL);
			rcCell.right = rcLabel.right;
		}
		::SetWindowPos(m_hWndEditInPlace, nullptr, rcCell.left, rcCell.top, rcCell.Width(), rcCell.Height(), SWP_NOZORDER);
		::SendMessageW(m_hWndEditInPlace, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFntList), FALSE);
	}
}

void CListEx::SetHdrHeight(DWORD dwHeight)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return;
	}

	GetHeader().SetHeight(dwHeight);
	Update(0);
	GetHeader().RedrawWindow();
}

void CListEx::SetHdrImageList(HIMAGELIST pList)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return;
	}

	GetHeader().SetImageList(pList);
}

auto CListEx::SetImageList(HIMAGELIST hList, int iListType)->HIMAGELIST
{
	assert(IsCreated());
	if (!IsCreated()) {
		return { };
	}

	return reinterpret_cast<HIMAGELIST>(::SendMessageW(m_hWnd, LVM_SETIMAGELIST, iListType, reinterpret_cast<LPARAM>(hList)));
}

void CListEx::SetItemCountEx(int iCount, DWORD dwFlags)const
{
	assert(IsCreated());
	if (!IsCreated()) {
		return;
	}

	assert(dwFlags == 0 || m_fVirtual);
	::SendMessageW(m_hWnd, LVM_SETITEMCOUNT, iCount, dwFlags);
}

bool CListEx::SetItemState(int iItem, LVITEMW* pItem)const
{
	assert(IsCreated());
	if (!IsCreated()) {
		return false;
	}

	return static_cast<bool>(::SendMessageW(m_hWnd, LVM_SETITEMSTATE, iItem, reinterpret_cast<LPARAM>(pItem)));
}

bool CListEx::SetItemState(int iItem, UINT uState, UINT uStateMask)const
{
	assert(IsCreated());
	if (!IsCreated()) {
		return false;
	}

	LVITEMW lvi { .state { uState }, .stateMask { uStateMask } };
	return SetItemState(iItem, &lvi);
}

void CListEx::SetItemText(int iItem, int iSubItem, LPCWSTR pwszText)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return;
	}

	LVITEMW lvi { .iSubItem { iSubItem }, .pszText { const_cast<LPWSTR>(pwszText) } };
	::SendMessageW(m_hWnd, LVM_SETITEMTEXTW, iItem, reinterpret_cast<LPARAM>(&lvi));
}

void CListEx::SetHdrFont(const LOGFONTW& lf)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return;
	}

	GetHeader().SetFont(lf);
	Update(0);
	GetHeader().RedrawWindow();
}

void CListEx::SetHdrColumnColor(int iColumn, COLORREF clrBk, COLORREF clrText)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return;
	}

	GetHeader().SetColumnColor(iColumn, clrBk, clrText);
	Update(0);
	GetHeader().RedrawWindow();
}

void CListEx::SetHdrColumnIcon(int iColumn, const LISTEXHDRICON& stIcon)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return;
	}

	GetHeader().SetColumnIcon(iColumn, stIcon);
}

void CListEx::SetSortable(bool fSortable, PFNLVCOMPARE pfnCompare, EListExSortMode eSortMode)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return;
	}

	m_fSortable = fSortable;
	m_pfnCompare = pfnCompare;
	m_eDefSortMode = eSortMode;

	GetHeader().SetSortable(fSortable);
}

void CListEx::SortItemsEx(PFNLVCOMPARE pfnCompare, DWORD_PTR dwData)const
{
	assert(IsCreated());
	if (!IsCreated()) {
		return;
	}

	assert(!m_fVirtual);
	::SendMessageW(m_hWnd, LVM_SORTITEMSEX, dwData, reinterpret_cast<LPARAM>(pfnCompare));
}

void CListEx::Update(int iItem)const
{
	assert(IsCreated());
	if (!IsCreated()) {
		return;
	}

	::SendMessageW(m_hWnd, LVM_UPDATE, iItem, 0L);
}

int CALLBACK CListEx::DefCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	const auto* const pListCtrl = reinterpret_cast<CListEx*>(lParamSort);
	const auto iSortColumn = pListCtrl->GetSortColumn();
	const auto eSortMode = pListCtrl->GetColumnSortMode(iSortColumn);
	const auto wstrItem1 = pListCtrl->GetItemText(static_cast<int>(lParam1), iSortColumn);
	const auto wstrItem2 = pListCtrl->GetItemText(static_cast<int>(lParam2), iSortColumn);

	int iCompare { };
	switch (eSortMode) {
	case EListExSortMode::SORT_LEX:
		iCompare = wstrItem1.compare(wstrItem2);
		break;
	case EListExSortMode::SORT_NUMERIC:
	{
		LONGLONG llData1 { }, llData2 { };
		StrToInt64ExW(wstrItem1.data(), STIF_SUPPORT_HEX, &llData1);
		StrToInt64ExW(wstrItem2.data(), STIF_SUPPORT_HEX, &llData2);
		iCompare = llData1 != llData2 ? (llData1 - llData2 < 0 ? -1 : 1) : 0;
	}
	break;
	}

	int iResult = 0;
	if (pListCtrl->GetSortAscending()) {
		if (iCompare < 0) {
			iResult = -1;
		}
		else if (iCompare > 0) {
			iResult = 1;
		}
	}
	else {
		if (iCompare < 0) {
			iResult = 1;
		}
		else if (iCompare > 0) {
			iResult = -1;
		}
	}

	return iResult;
}


//CListEx private methods:

void CListEx::DrawItem(LPDRAWITEMSTRUCT pDIS)
{
	if (pDIS->hwndItem != m_hWnd)
		return;

	const auto iItem = pDIS->itemID;
	constexpr auto iTextIndentTop = 1; //To compensate what is added in the MeasureItem.
	constexpr auto iTextIndentLeft = 2;

	switch (pDIS->itemAction) {
	case ODA_SELECT:
	case ODA_DRAWENTIRE:
	{
		const wnd::CDC cdc(pDIS->hDC);
		const auto clrBkCurrRow = (iItem % 2) ? m_stColors.clrListBkEven : m_stColors.clrListBkOdd;
		const auto& refHdr = GetHeader();
		const auto iColumns = refHdr.GetItemCount();
		for (auto iSubitem = 0; iSubitem < iColumns; ++iSubitem) {
			if (refHdr.IsColumnHidden(iSubitem)) {
				continue;
			}

			COLORREF clrText;
			COLORREF clrBk;
			COLORREF clrTextLink;

			//Subitems' draw routine.
			//Colors depending on whether subitem selected or not, and has tooltip or not.
			if (pDIS->itemState & ODS_SELECTED) {
				clrText = m_stColors.clrListTextSel;
				clrBk = m_stColors.clrListBkSel;
				clrTextLink = m_stColors.clrListTextLinkSel;
			}
			else {
				clrTextLink = m_stColors.clrListTextLink;

				if (const auto optClr = GetCustomColor(iItem, iSubitem); optClr) {
					//Check for default colors (-1).
					clrText = optClr->clrText == -1 ? m_stColors.clrListText : optClr->clrText;
					clrBk = optClr->clrBk == -1 ? clrBkCurrRow : optClr->clrBk;
				}
				else {
					clrText = m_stColors.clrListText;
					clrBk = clrBkCurrRow;
				}
			}

			wnd::CRect rcBounds = GetSubItemRect(iItem, iSubitem, LVIR_BOUNDS);
			cdc.FillSolidRect(rcBounds, clrBk);

			for (const auto& itItemData : ParseItemData(iItem, iSubitem)) {
				if (itItemData.iIconIndex > -1) {
					::ImageList_DrawEx(GetImageList(LVSIL_NORMAL), itItemData.iIconIndex, cdc,
						itItemData.rc.left, itItemData.rc.top, 0, 0, CLR_NONE, CLR_NONE, ILD_NORMAL);
					continue;
				}

				if (itItemData.fLink) {
					cdc.SetTextColor(clrTextLink);
					if (m_fLinkUnderl) {
						cdc.SelectObject(m_hFntListUnderline);
					}
				}
				else {
					cdc.SetTextColor(clrText);
					cdc.SelectObject(m_hFntList);
				}

				::ExtTextOutW(cdc, itItemData.rc.left + iTextIndentLeft, itItemData.rc.top + iTextIndentTop,
					ETO_CLIPPED, rcBounds, itItemData.wstrText.data(), static_cast<UINT>(itItemData.wstrText.size()), nullptr);
			}

			//Drawing subitem's rect lines.
			//If SetExtendedStyle(LVS_EX_GRIDLINES) is set, lines are drawn automatically but through whole client rect.
			if (m_dwGridWidth > 0) {
				cdc.SelectObject(m_hPenGrid);
				cdc.MoveTo(rcBounds.TopLeft());
				cdc.LineTo(rcBounds.right, rcBounds.top);   //Top line.
				cdc.MoveTo(rcBounds.TopLeft());
				cdc.LineTo(rcBounds.left, rcBounds.bottom); //Left line.
				cdc.MoveTo(rcBounds.left, rcBounds.bottom);
				cdc.LineTo(rcBounds.BottomRight());         //Bottom line.
				if (iSubitem == iColumns - 1) { //Drawing a right line only for the last column.
					rcBounds.right -= 1; //To overcome a glitch with a last line disappearing if resizing a header.
					cdc.MoveTo(rcBounds.right, rcBounds.top);
					cdc.LineTo(rcBounds.BottomRight());     //Right line.
				}
			}

			//Draw focus rect (marquee).
			if ((pDIS->itemState & ODS_FOCUS) && !(pDIS->itemState & ODS_SELECTED)) {
				cdc.DrawFocusRect(rcBounds);
			}
		}
	}
	break;
	default:
		break;
	}
}

void CListEx::FontSizeIncDec(bool fInc)
{
	const auto lFontSize = MulDiv(-GetFontSize(), 72, m_iLOGPIXELSY) + (fInc ? 1 : -1);
	SetFontSize(lFontSize);
}

auto CListEx::GetCustomColor(int iItem, int iSubItem)const->std::optional<LISTEXCOLOR>
{
	if (iItem < 0 || iSubItem < 0) {
		return std::nullopt;
	}

	//Asking parent for color.
	const auto iCtrlID = ::GetDlgCtrlID(m_hWnd);
	const LISTEXCOLORINFO lci { .hdr { .hwndFrom { m_hWnd }, .idFrom { static_cast<UINT>(iCtrlID) },
		.code { LISTEX_MSG_GETCOLOR } }, .iItem { iItem }, .iSubItem { iSubItem },
		.stClr { .clrBk { 0xFFFFFFFFUL }, .clrText { 0xFFFFFFFFUL } } };
	::SendMessageW(GetParent(m_hWnd), WM_NOTIFY, static_cast<WPARAM>(iCtrlID), reinterpret_cast<LPARAM>(&lci));
	if (lci.stClr.clrBk != -1 || lci.stClr.clrText != -1) {
		return lci.stClr;
	}

	return std::nullopt;
}

int CListEx::GetIcon(int iItem, int iSubItem)const
{
	if (GetImageList(LVSIL_NORMAL) == nullptr) {
		return -1; //-1 is the default, when no image for cell is set.
	}

	//Asking parent for the icon index in image list.
	const auto uCtrlID = static_cast<UINT>(::GetDlgCtrlID(m_hWnd));
	const LISTEXICONINFO lii { .hdr { .hwndFrom { m_hWnd }, .idFrom { uCtrlID }, .code { LISTEX_MSG_GETICON } },
		.iItem { iItem }, .iSubItem { iSubItem } };
	::SendMessageW(GetParent(m_hWnd), WM_NOTIFY, static_cast<WPARAM>(uCtrlID), reinterpret_cast<LPARAM>(&lii));
	return lii.iIconIndex; //By default it's -1, meaning no icon.
}

auto CListEx::GetTooltip(int iItem, int iSubItem)const->std::optional<LISTEXTTDATA>
{
	if (iItem < 0 || iSubItem < 0) {
		return std::nullopt;
	}

	//Asking parent for tooltip.
	const auto iCtrlID = ::GetDlgCtrlID(m_hWnd);
	const LISTEXTTINFO ltti { .hdr { .hwndFrom { m_hWnd }, .idFrom { static_cast<UINT>(iCtrlID) },
		.code { LISTEX_MSG_GETTOOLTIP } }, .iItem { iItem }, .iSubItem { iSubItem } };
	if (::SendMessageW(::GetParent(m_hWnd), WM_NOTIFY, static_cast<WPARAM>(iCtrlID), reinterpret_cast<LPARAM>(&ltti));
		ltti.stData.pwszText != nullptr) { //If tooltip text was set by the Parent.
		return ltti.stData;
	}

	return std::nullopt;
}

bool CListEx::IsWindow()const
{
	return ::IsWindow(m_hWnd);
}

void CListEx::MeasureItem(LPMEASUREITEMSTRUCT pMIS)
{
	if (pMIS->CtlID != static_cast<UINT>(GetDlgCtrlID(m_hWnd)))
		return;

	//Set row height according to current font's height.
	const auto hDC = ::GetDC(m_hWnd);
	::SelectObject(hDC, m_hFntList);
	TEXTMETRICW tm;
	::GetTextMetricsW(hDC, &tm);
	::ReleaseDC(m_hWnd, hDC);
	pMIS->itemHeight = tm.tmHeight + tm.tmExternalLeading + 2;
}

auto CListEx::OnCommand(const MSG& msg)->LRESULT
{
	const auto uCtrlID = LOWORD(msg.wParam); //Control ID.
	const auto uCode = HIWORD(msg.wParam);
	if (uCtrlID == m_uIDEditInPlace && uCode == EN_KILLFOCUS) {
		OnEditInPlaceKillFocus();
		return 0;
	}

	return 1;
}

auto CListEx::OnDestroy()->LRESULT
{
	::DestroyWindow(m_hWndCellTT);
	::DestroyWindow(m_hWndLinkTT);
	::DestroyWindow(m_hWndRowTT);
	::DestroyWindow(m_hWndEditInPlace);
	::DeleteObject(m_hFntList);
	::DeleteObject(m_hFntListUnderline);
	::DeleteObject(m_hPenGrid);
	m_vecColumnData.clear();
	m_fCreated = false;

	return 0;
}

void CListEx::OnEditInPlaceEnterPressed()
{
	wchar_t buffText[256];
	::GetWindowTextW(m_hWndEditInPlace, buffText, 256);
	if (m_fVirtual) {
		const auto uCtrlId = static_cast<UINT>(::GetDlgCtrlID(m_hWnd));
		const LISTEXDATAINFO ldi { .hdr { .hwndFrom { m_hWnd }, .idFrom { uCtrlId }, .code { LISTEX_MSG_SETDATA } },
			.iItem { m_htiEdit.iItem }, .iSubItem { m_htiEdit.iSubItem }, .pwszData { buffText } };
		::SendMessageW(::GetParent(m_hWnd), WM_NOTIFY, static_cast<WPARAM>(uCtrlId), reinterpret_cast<LPARAM>(&ldi));
	}
	else {
		SetItemText(m_htiEdit.iItem, m_htiEdit.iSubItem, buffText);
	}

	OnEditInPlaceKillFocus();
}

void CListEx::OnEditInPlaceKillFocus()
{
	::DestroyWindow(m_hWndEditInPlace);
	RedrawWindow();
}

auto CListEx::OnEraseBkgnd()->LRESULT
{
	return TRUE;
}

auto CListEx::OnHdnBegindrag(NMHDR* pNMHDR)->LRESULT
{
	const auto phdr = reinterpret_cast<LPNMHEADERW>(pNMHDR);
	if (GetHeader().IsColumnHidden(phdr->iItem)) {
		return TRUE;
	}

	return FALSE;
}

auto CListEx::OnHdnBegintrack(NMHDR* pNMHDR)->LRESULT
{
	const auto phdr = reinterpret_cast<LPNMHEADERW>(pNMHDR);
	if (GetHeader().IsColumnHidden(phdr->iItem)) {
		return TRUE;
	}

	return FALSE;
}

auto CListEx::OnHScroll(const MSG& msg)->LRESULT
{
	GetHeader().RedrawWindow();
	wnd::DefSubclassProc(msg);

	return 0;
}

auto CListEx::OnLButtonDblClk(const MSG& msg)->LRESULT
{
	const POINT pt { .x { GET_X_LPARAM(msg.lParam) }, .y { GET_Y_LPARAM(msg.lParam) } };
	m_htiEdit.pt = pt;
	HitTest(&m_htiEdit);
	if (m_htiEdit.iItem < 0 || m_htiEdit.iSubItem < 0 || !GetHeader().IsColumnEditable(m_htiEdit.iSubItem)) {
		return wnd::DefSubclassProc(msg);
	}

	const auto uCtrlId = static_cast<UINT>(::GetDlgCtrlID(m_hWnd));
	const LISTEXDATAINFO ldi { .hdr { .hwndFrom { m_hWnd }, .idFrom { uCtrlId }, .code { LISTEX_MSG_EDITBEGIN } },
		.iItem { m_htiEdit.iItem }, .iSubItem { m_htiEdit.iSubItem } };
	::SendMessageW(::GetParent(m_hWnd), WM_NOTIFY, static_cast<WPARAM>(uCtrlId), reinterpret_cast<LPARAM>(&ldi));
	if (!ldi.fAllowEdit) { //User explicitly declined to display edit-box.
		return wnd::DefSubclassProc(msg);
	}

	//Get Column data alignment.
	const auto iAlignment = GetHeader().GetColumnDataAlign(m_htiEdit.iSubItem);
	const DWORD dwStyle = iAlignment == LVCFMT_LEFT ? ES_LEFT : (iAlignment == LVCFMT_RIGHT ? ES_RIGHT : ES_CENTER);
	auto rcCell = GetSubItemRect(m_htiEdit.iItem, m_htiEdit.iSubItem, LVIR_BOUNDS);
	if (m_htiEdit.iSubItem == 0) { //Clicked on item (first column).
		rcCell.right = GetItemRect(m_htiEdit.iItem, LVIR_LABEL).right;
	}

	const auto wstrText = GetItemText(m_htiEdit.iItem, m_htiEdit.iSubItem);
	::DestroyWindow(m_hWndEditInPlace);
	const auto iWidth = rcCell.right - rcCell.left;
	const auto iHeight = rcCell.bottom - rcCell.top;
	m_hWndEditInPlace = ::CreateWindowExW(0, WC_EDITW, nullptr, dwStyle | WS_BORDER | WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
		rcCell.left, rcCell.top, iWidth, iHeight, m_hWnd, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(m_uIDEditInPlace)),
		nullptr, nullptr);
	::SetWindowSubclass(m_hWndEditInPlace, EditSubclassProc, reinterpret_cast<UINT_PTR>(this), 0);
	::SendMessageW(m_hWndEditInPlace, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFntList), FALSE);
	::SetWindowTextW(m_hWndEditInPlace, wstrText.data());
	::SetFocus(m_hWndEditInPlace);

	return wnd::DefSubclassProc(msg);
}

auto CListEx::OnLButtonDown(const MSG& msg)->LRESULT
{
	const POINT pt { .x { GET_X_LPARAM(msg.lParam) }, .y { GET_Y_LPARAM(msg.lParam) } };
	bool fLinkDown { false };
	LVHITTESTINFO hti { .pt { pt } };
	HitTest(&hti);
	if (hti.iItem >= 0 && hti.iSubItem >= 0) {
		const auto vecText = ParseItemData(hti.iItem, hti.iSubItem);
		if (const auto iterFind = std::find_if(vecText.begin(), vecText.end(), [&](const ITEMDATA& item) {
			return item.fLink && item.rc.PtInRect(pt); }); iterFind != vecText.end()) {
			m_fLDownAtLink = true;
			m_rcLinkCurr = iterFind->rc;
			fLinkDown = true;
		}
	}

	if (!fLinkDown) {
		return wnd::DefSubclassProc(msg);
	}

	return 0;
}

auto CListEx::OnLButtonUp(const MSG& msg)->LRESULT
{
	bool fLinkUp { false };
	if (m_fLDownAtLink) {
		const POINT pt { .x { GET_X_LPARAM(msg.lParam) }, .y { GET_Y_LPARAM(msg.lParam) } };
		LVHITTESTINFO hti { .pt { pt } };
		HitTest(&hti);
		if (hti.iItem < 0 || hti.iSubItem < 0) {
			m_fLDownAtLink = false;
			return 0;
		}

		const auto vecText = ParseItemData(hti.iItem, hti.iSubItem);
		if (const auto iterFind = std::find_if(vecText.begin(), vecText.end(), [&](const ITEMDATA& item) {
			return item.fLink && item.rc == m_rcLinkCurr; }); iterFind != vecText.end()) {
			m_rcLinkCurr.SetRectEmpty();
			fLinkUp = true;
			const auto uCtrlId = static_cast<UINT>(::GetDlgCtrlID(m_hWnd));
			const LISTEXLINKINFO lli { .hdr { .hwndFrom { m_hWnd }, .idFrom { uCtrlId }, .code { LISTEX_MSG_LINKCLICK } },
				.iItem { hti.iItem }, .iSubItem { hti.iSubItem }, .ptClick { pt }, .pwszText { iterFind->wstrLink.data() } };
			::SendMessageW(::GetParent(m_hWnd), WM_NOTIFY, static_cast<WPARAM>(uCtrlId), reinterpret_cast<LPARAM>(&lli));
		}
	}

	m_fLDownAtLink = false;
	if (!fLinkUp) {
		return wnd::DefSubclassProc(msg);
	}

	return 0;
}

auto CListEx::OnMouseWheel(const MSG& msg)->LRESULT
{
	const auto zDelta = GET_WHEEL_DELTA_WPARAM(msg.wParam);
	const auto nFlags = GET_KEYSTATE_WPARAM(msg.wParam);
	if (nFlags == MK_CONTROL) {
		FontSizeIncDec(zDelta > 0);
		return 0;
	}

	GetHeader().RedrawWindow();

	return wnd::DefSubclassProc(msg);
}

auto CListEx::OnMouseMove(const MSG& msg)->LRESULT
{
	static const auto hCurArrow = static_cast<HCURSOR>(LoadImageW(nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0,
		LR_DEFAULTSIZE | LR_SHARED));
	static const auto hCurHand = static_cast<HCURSOR>(LoadImageW(nullptr, IDC_HAND, IMAGE_CURSOR, 0, 0,
		LR_DEFAULTSIZE | LR_SHARED));
	const POINT pt { .x { GET_X_LPARAM(msg.lParam) }, .y { GET_Y_LPARAM(msg.lParam) } };

	LVHITTESTINFO hti { .pt { pt } };
	HitTest(&hti);
	bool fLinkRect { false }; //Cursor at link's rect area?
	if (hti.iItem >= 0 && hti.iSubItem >= 0) {
		auto vecText = ParseItemData(hti.iItem, hti.iSubItem);     //Non const to allow std::move(itLink->wstrLink).
		auto itLink = std::find_if(vecText.begin(), vecText.end(), //Non const to allow std::move(itLink->wstrLink).
			[&](const ITEMDATA& item) {	return item.fLink && item.rc.PtInRect(pt); });
		if (itLink != vecText.end()) {
			fLinkRect = true;
			if (m_fLinkTooltip && !m_fLDownAtLink && m_rcLinkCurr != itLink->rc) {
				TTLinkShow(false);
				m_fLinkTTActive = true;
				m_rcLinkCurr = itLink->rc;
				m_htiCurrLink.iItem = hti.iItem;
				m_htiCurrLink.iSubItem = hti.iSubItem;
				m_wstrTTText = itLink->fTitle ? std::move(itLink->wstrTitle) : std::move(itLink->wstrLink);
				if (m_dwTTDelayTime > 0) {
					::SetTimer(m_hWnd, m_uIDTLinkTTActivate, m_dwTTDelayTime, nullptr); //Activate link's tooltip after delay.
				}
				else { TTLinkShow(true); } //Activate immediately.
			}
		}
	}

	SetCursor(fLinkRect ? hCurHand : hCurArrow);

	if (fLinkRect) { //Link's rect is under the cursor.
		if (m_fCellTTActive) { //If there is a cell's tooltip atm, hide it.
			TTCellShow(false);
		}
		return 0; //Do not process further, cursor is on the link's rect.
	}

	m_rcLinkCurr.SetRectEmpty(); //If cursor is not over any link.
	m_fLDownAtLink = false;

	if (m_fLinkTTActive) { //If there was a link's tool-tip shown, hide it.
		TTLinkShow(false);
	}

	if (const auto optTT = GetTooltip(hti.iItem, hti.iSubItem); optTT) {
		//Check if cursor is still in the same cell's rect. If so - just leave.
		if (m_htiCurrCell.iItem != hti.iItem || m_htiCurrCell.iSubItem != hti.iSubItem) {
			m_fCellTTActive = true;
			m_htiCurrCell.iItem = hti.iItem;
			m_htiCurrCell.iSubItem = hti.iSubItem;
			m_wstrTTText = optTT->pwszText; //Tooltip text.
			m_wstrTTCaption = optTT->pwszCaption ? optTT->pwszCaption : L""; //Tooltip caption.
			if (m_dwTTDelayTime > 0) {
				::SetTimer(m_hWnd, m_uIDTCellTTActivate, m_dwTTDelayTime, nullptr); //Activate cell's tooltip after delay.
			}
			else { TTCellShow(true); }; //Activate immediately.
		}
	}
	else {
		if (m_fCellTTActive) {
			TTCellShow(false);
		}
		else {
			m_htiCurrCell.iItem = -1;
			m_htiCurrCell.iSubItem = -1;
		}
	}

	return 0;
}

auto CListEx::OnNotify(const MSG& msg)->LRESULT
{
	if (!m_fCreated) {
		return wnd::DefSubclassProc(msg);
	}

	//HDN_ITEMCLICK messages should be handled here first, to set m_fSortAscending 
	//and m_iSortColumn. And only then this message goes to parent window in form
	//of HDN_ITEMCLICK and LVN_COLUMNCLICK.
	//If we execute this code in LVN_COLUMNCLICK handler, it will be handled only
	//AFTER the parent window handles LVN_COLUMNCLICK.
	//Briefly: CListEx::OnLvnColumnClick fires up only AFTER LVN_COLUMNCLICK sent to the parent.

	const auto pNMHDR = reinterpret_cast<NMHDR*>(msg.lParam);
	const auto pNMLV = reinterpret_cast<LPNMHEADERW>(msg.lParam);
	switch (pNMHDR->idFrom) {
	case 0: //Header control.
		switch (pNMHDR->code) {
		case HDN_BEGINDRAG: return OnHdnBegindrag(pNMHDR);
		case HDN_BEGINTRACKA:
		case HDN_BEGINTRACKW:
			return OnHdnBegintrack(pNMHDR);
		case HDN_ITEMCLICKA:
		case HDN_ITEMCLICKW:
			if (m_fSortable) {
				if (IsColumnSortable(pNMLV->iItem)) {
					m_fSortAsc = pNMLV->iItem == m_iSortColumn ? !m_fSortAsc : true;
					GetHeader().SetSortArrow(pNMLV->iItem, m_fSortAsc);
					m_iSortColumn = pNMLV->iItem;
				}
				else {
					m_iSortColumn = -1;
				}

				if (!m_fVirtual) {
					SortItemsEx(m_pfnCompare ? m_pfnCompare : DefCompareFunc, reinterpret_cast<DWORD_PTR>(this));
					return wnd::DefSubclassProc(msg);
				}
			}break;
		default: break;
		}
		break;
	case m_uIDEditInPlace: //Edit in-place control.
		switch (pNMHDR->code) {
		case VK_RETURN:
		case VK_ESCAPE:
			OnNotifyEditInPlace(pNMHDR);
			break;
		default: break;
		}
	default: break;
	}

	return wnd::DefSubclassProc(msg);
}

void CListEx::OnNotifyEditInPlace(NMHDR* pNMHDR)
{
	switch (pNMHDR->code) {
	case VK_RETURN:
		OnEditInPlaceEnterPressed();
		break;
	case VK_ESCAPE:
		OnEditInPlaceKillFocus();
		break;
	default:
		break;
	}
}

auto CListEx::OnPaint()->LRESULT
{
	const wnd::CPaintDC dc(m_hWnd);
	wnd::CRect rcClient;
	::GetClientRect(m_hWnd, rcClient);
	const wnd::CRect rcHdr = GetHeader().GetClientRect();
	rcClient.top += rcHdr.Height();
	rcClient.bottom += rcHdr.Height();
	if (rcClient.IsRectEmpty()) {
		return 0;
	}

	const wnd::CMemDC dcMem(dc, rcClient); //To avoid flickering drawing to CMemDC, excluding list header area.
	::GetClipBox(dcMem, rcClient);
	dcMem.FillSolidRect(rcClient, m_stColors.clrNWABk);

	return DefSubclassProc(m_hWnd, WM_PAINT, reinterpret_cast<WPARAM>(dcMem.GetHDC()), 0);
}

auto CListEx::OnTimer(const MSG& msg)->LRESULT
{
	const auto nIDEvent = static_cast<UINT_PTR>(msg.wParam);
	POINT ptScreen;
	::GetCursorPos(&ptScreen);
	POINT ptClient = ptScreen;
	::ScreenToClient(m_hWnd, &ptClient);
	LVHITTESTINFO hitInfo { .pt { ptClient } };
	HitTest(&hitInfo);

	switch (nIDEvent) {
	case m_uIDTCellTTActivate:
		::KillTimer(m_hWnd, m_uIDTCellTTActivate);
		if (m_htiCurrCell.iItem == hitInfo.iItem && m_htiCurrCell.iSubItem == hitInfo.iSubItem) {
			TTCellShow(true);
		}
		break;
	case m_uIDTLinkTTActivate:
		::KillTimer(m_hWnd, m_uIDTLinkTTActivate);
		if (m_rcLinkCurr.PtInRect(ptClient)) {
			TTLinkShow(true);
		}
		break;
	case m_uIDTCellTTCheck:
	{
		//If cursor has left cell's rect, or time run out.
		const auto msElapsed = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - m_tmTT).count();
		if (m_htiCurrCell.iItem != hitInfo.iItem || m_htiCurrCell.iSubItem != hitInfo.iSubItem) {
			TTCellShow(false);
		}
		else if (msElapsed >= m_dwTTShowTime) {
			TTCellShow(false, true);
		}
	}
	break;
	case m_uIDTLinkTTCheck:
	{
		//If cursor has left link subitem's rect, or time run out.
		const auto msElapsed = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - m_tmTT).count();
		if (m_htiCurrLink.iItem != hitInfo.iItem || m_htiCurrLink.iSubItem != hitInfo.iSubItem) {
			TTLinkShow(false);
		}
		else if (msElapsed >= m_dwTTShowTime) {
			TTLinkShow(false, true);
		}
	}
	break;
	default:
		wnd::DefSubclassProc(msg);
		break;
	}

	return 0;
}

auto CListEx::OnVScroll(const MSG& msg)->LRESULT
{
	const auto nSBCode = LOWORD(msg.wParam);
	if (m_fVirtual && m_fHighLatency) {
		if (nSBCode != SB_THUMBTRACK) {
			//If there was SB_THUMBTRACK message previously, calculate the scroll amount (up/down)
			//by multiplying item's row height by difference between current (top) and nPos row.
			//Scroll may be negative therefore.
			if (m_fHLFlag) {
				const wnd::CRect rc = GetItemRect(m_uHLItem, LVIR_LABEL);
				const SIZE size(0, (m_uHLItem - GetTopIndex()) * rc.Height());
				Scroll(size);
				m_fHLFlag = false;
			}
			TTHLShow(false, 0);
			wnd::DefSubclassProc(msg);
		}
		else {
			m_fHLFlag = true;
			SCROLLINFO si { };
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_ALL;
			::GetScrollInfo(m_hWnd, SB_VERT, &si);
			m_uHLItem = si.nTrackPos; //si.nTrackPos is in fact a row number.
			TTHLShow(true, m_uHLItem);
		}
	}
	else {
		wnd::DefSubclassProc(msg);
	}

	return 0;
}

auto CListEx::ParseItemData(int iItem, int iSubitem)->std::vector<CListEx::ITEMDATA>
{
	constexpr auto iIndentRc { 4 };
	const auto wstrText = GetItemText(iItem, iSubitem);
	const std::wstring_view wsvText = wstrText;
	wnd::CRect rcTextOrig = GetSubItemRect(iItem, iSubitem, LVIR_LABEL); //Original rect of the subitem's text.
	if (iSubitem > 0) { //Not needed for item itself (not subitem).
		rcTextOrig.left += iIndentRc;
	}

	std::vector<ITEMDATA> vecData;
	int iImageWidth { 0 };

	if (const auto iIndex = GetIcon(iItem, iSubitem); iIndex > -1) { //If cell has icon.
		IMAGEINFO stII;
		ImageList_GetImageInfo(GetImageList(LVSIL_NORMAL), iIndex, &stII);
		iImageWidth = stII.rcImage.right - stII.rcImage.left;
		const auto iImgHeight = stII.rcImage.bottom - stII.rcImage.top;
		const auto iImgTop = iImgHeight < rcTextOrig.Height() ?
			rcTextOrig.top + (rcTextOrig.Height() / 2 - iImgHeight / 2) : rcTextOrig.top;
		const RECT rcImage(rcTextOrig.left, iImgTop, rcTextOrig.left + iImageWidth, iImgTop + iImgHeight);
		vecData.emplace_back(iIndex, rcImage);
		rcTextOrig.left += iImageWidth + iIndentRc; //Offset rect for image width.
	}

	std::size_t nPosCurr { 0 }; //Current position in the parsed string.
	wnd::CRect rcTextCurr; //Current rect.
	const auto hDC = GetDC(m_hWnd);

	while (nPosCurr != std::wstring_view::npos) {
		static constexpr std::wstring_view wsvTagLink { L"<link=" };
		static constexpr std::wstring_view wsvTagFirstClose { L">" };
		static constexpr std::wstring_view wsvTagLast { L"</link>" };
		static constexpr std::wstring_view wsvTagTitle { L"title=" };
		static constexpr std::wstring_view wsvQuote { L"\"" };

		//Searching the string for a <link=...></link> pattern.
		if (const std::size_t nPosTagLink { wsvText.find(wsvTagLink, nPosCurr) }, //Start position of the opening tag "<link=".
			nPosLinkOpenQuote { wsvText.find(wsvQuote, nPosTagLink) }, //Position of the (link="<-) open quote.
			//Position of the (link=""<-) close quote.
			nPosLinkCloseQuote { wsvText.find(wsvQuote, nPosLinkOpenQuote + wsvQuote.size()) },
			//Start position of the opening tag's closing bracket ">".
			nPosTagFirstClose { wsvText.find(wsvTagFirstClose, nPosLinkCloseQuote + wsvQuote.size()) },
			//Start position of the enclosing tag "</link>".
			nPosTagLast { wsvText.find(wsvTagLast, nPosTagFirstClose + wsvTagFirstClose.size()) };
			m_fLinks && nPosTagLink != std::wstring_view::npos && nPosLinkOpenQuote != std::wstring_view::npos
			&& nPosLinkCloseQuote != std::wstring_view::npos && nPosTagFirstClose != std::wstring_view::npos
			&& nPosTagLast != std::wstring_view::npos) {
			::SelectObject(hDC, m_hFntList);
			SIZE size;

			//Any text before found tag.
			if (nPosTagLink > nPosCurr) {
				const auto wsvTextBefore = wsvText.substr(nPosCurr, nPosTagLink - nPosCurr);
				::GetTextExtentPoint32W(hDC, wsvTextBefore.data(), static_cast<int>(wsvTextBefore.size()), &size);
				if (rcTextCurr.IsRectNull()) {
					rcTextCurr.SetRect(rcTextOrig.left, rcTextOrig.top, rcTextOrig.left + size.cx, rcTextOrig.bottom);
				}
				else {
					rcTextCurr.left = rcTextCurr.right;
					rcTextCurr.right += size.cx;
				}
				vecData.emplace_back(wsvTextBefore, L"", L"", rcTextCurr);
			}

			//The clickable/linked text, that between <link=...>textFromHere</link> tags.
			const auto wsvTextBetweenTags = wsvText.substr(nPosTagFirstClose + wsvTagFirstClose.size(),
				nPosTagLast - (nPosTagFirstClose + wsvTagFirstClose.size()));
			::GetTextExtentPoint32W(hDC, wsvTextBetweenTags.data(), static_cast<int>(wsvTextBetweenTags.size()), &size);

			if (rcTextCurr.IsRectNull()) {
				rcTextCurr.SetRect(rcTextOrig.left, rcTextOrig.top, rcTextOrig.left + size.cx, rcTextOrig.bottom);
			}
			else {
				rcTextCurr.left = rcTextCurr.right;
				rcTextCurr.right += size.cx;
			}

			//Link tag text (linkID) between quotes: <link="textFromHere">
			const auto wsvTextLink = wsvText.substr(nPosLinkOpenQuote + wsvQuote.size(),
				nPosLinkCloseQuote - nPosLinkOpenQuote - wsvQuote.size());
			nPosCurr = nPosLinkCloseQuote + wsvQuote.size();

			//Searching for title "<link=...title="">" tag.
			bool fTitle { false };
			std::wstring_view wsvTextTitle { };

			if (const std::size_t nPosTagTitle { wsvText.find(wsvTagTitle, nPosCurr) }, //Position of the (title=) tag beginning.
				nPosTitleOpenQuote { wsvText.find(wsvQuote, nPosTagTitle) },  //Position of the (title="<-) opening quote.
				nPosTitleCloseQuote { wsvText.find(wsvQuote, nPosTitleOpenQuote + wsvQuote.size()) }; //Position of the (title=""<-) closing quote.
				nPosTagTitle != std::wstring_view::npos && nPosTitleOpenQuote != std::wstring_view::npos
				&& nPosTitleCloseQuote != std::wstring_view::npos) {
				//Title tag text between quotes: <...title="textFromHere">
				wsvTextTitle = wsvText.substr(nPosTitleOpenQuote + wsvQuote.size(),
					nPosTitleCloseQuote - nPosTitleOpenQuote - wsvQuote.size());
				fTitle = true;
			}

			vecData.emplace_back(wsvTextBetweenTags, wsvTextLink, wsvTextTitle, rcTextCurr, true, fTitle);
			nPosCurr = nPosTagLast + wsvTagLast.size();
		}
		else {
			const auto wsvTextAfter = wsvText.substr(nPosCurr, wsvText.size() - nPosCurr);
			::SelectObject(hDC, m_hFntList);
			SIZE size;
			::GetTextExtentPoint32W(hDC, wsvTextAfter.data(), static_cast<int>(wsvTextAfter.size()), &size);

			if (rcTextCurr.IsRectNull()) {
				rcTextCurr.SetRect(rcTextOrig.left, rcTextOrig.top, rcTextOrig.left + size.cx, rcTextOrig.bottom);
			}
			else {
				rcTextCurr.left = rcTextCurr.right;
				rcTextCurr.right += size.cx;
			}

			vecData.emplace_back(wsvTextAfter, L"", L"", rcTextCurr);
			nPosCurr = std::wstring_view::npos;
		}
	}
	ReleaseDC(m_hWnd, hDC);

	//Depending on column's data alignment we adjust rects coords to render properly.
	switch (GetHeader().GetColumnDataAlign(iSubitem)) {
	case LVCFMT_LEFT: //Do nothing, by default it's already left aligned.
		break;
	case LVCFMT_CENTER:
	{
		int iWidthTotal { };
		for (const auto& iter : vecData) {
			iWidthTotal += iter.rc.Width();
		}

		if (iWidthTotal < rcTextOrig.Width()) {
			const auto iRcOffset = (rcTextOrig.Width() - iWidthTotal) / 2;
			for (auto& iter : vecData) {
				if (iter.iIconIndex == -1) { //Offset only rects with text, not icons rects.
					iter.rc.OffsetRect(iRcOffset, 0);
				}
			}
		}
	}
	break;
	case LVCFMT_RIGHT:
	{
		int iWidthTotal { };
		for (const auto& iter : vecData) {
			iWidthTotal += iter.rc.Width();
		}

		const auto iWidthRect = rcTextOrig.Width() + iImageWidth - iIndentRc;
		if (iWidthTotal < iWidthRect) {
			const auto iRcOffset = iWidthRect - iWidthTotal;
			for (auto& iter : vecData) {
				if (iter.iIconIndex == -1) { //Offset only rects with text, not icons rects.
					iter.rc.OffsetRect(iRcOffset, 0);
				}
			}
		}
	}
	break;
	default:
		break;
	}

	return vecData;
}

void CListEx::RedrawWindow()const
{
	assert(IsWindow());
	::RedrawWindow(m_hWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
}

void CListEx::RecalcMeasure()const
{
	//To get WM_MEASUREITEM after changing the font.
	wnd::CRect rc;
	GetWindowRect(m_hWnd, rc);
	const WINDOWPOS wp { .hwnd { m_hWnd }, .cx { rc.Width() }, .cy { rc.Height() },
		.flags { SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER } };
	::SendMessageW(m_hWnd, WM_WINDOWPOSCHANGED, 0, reinterpret_cast<LPARAM>(&wp));
}

void CListEx::SetFontSize(long lSize)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return;
	}

	//Prevent font size from being too small or too big.
	if (lSize < 4 || lSize > 64) {
		return;
	}

	LOGFONTW lf { };
	::GetObjectW(m_hFntList, sizeof(lf), &lf);
	lf.lfHeight = -MulDiv(lSize, m_iLOGPIXELSY, 72);
	SetFont(lf);
}

void CListEx::TTCellShow(bool fShow, bool fTimer)
{
	TTTOOLINFOW ttiCell { .cbSize { sizeof(TTTOOLINFOW) }, .lpszText { m_wstrTTText.data() } };
	if (fShow) {
		wnd::CPoint ptCurr;
		GetCursorPos(&ptCurr);
		ptCurr.Offset(m_ptTTOffset);
		::SendMessageW(m_hWndCellTT, TTM_TRACKPOSITION, 0, static_cast<LPARAM>(MAKELONG(ptCurr.x, ptCurr.y)));
		::SendMessageW(m_hWndCellTT, TTM_SETTITLEW, TTI_NONE, reinterpret_cast<LPARAM>(m_wstrTTCaption.data()));
		::SendMessageW(m_hWndCellTT, TTM_UPDATETIPTEXTW, 0, reinterpret_cast<LPARAM>(&ttiCell));
		::SendMessageW(m_hWndCellTT, TTM_TRACKACTIVATE, TRUE, reinterpret_cast<LPARAM>(&ttiCell));
		m_tmTT = std::chrono::high_resolution_clock::now();
		::SetTimer(m_hWnd, m_uIDTCellTTCheck, 300, nullptr); //Timer to check whether mouse has left subitem's rect.
	}
	else {
		::KillTimer(m_hWnd, m_uIDTCellTTCheck);

		//When hiding tooltip by timer we not nullify the m_htiCurrCell.
		//Otherwise tooltip will be shown again after mouse movement,
		//even if cursor didn't leave current cell area.
		if (!fTimer) {
			m_htiCurrCell.iItem = -1;
			m_htiCurrCell.iSubItem = -1;
		}

		m_fCellTTActive = false;
		::SendMessageW(m_hWndCellTT, TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&ttiCell));
	}
}

void CListEx::TTLinkShow(bool fShow, bool fTimer)
{
	TTTOOLINFOW ttiLink { .cbSize { sizeof(TTTOOLINFOW) }, .lpszText { m_wstrTTText.data() } };
	if (fShow) {
		wnd::CPoint ptCurr;
		GetCursorPos(&ptCurr);
		ptCurr.Offset(m_ptTTOffset);
		::SendMessageW(m_hWndLinkTT, TTM_TRACKPOSITION, 0, static_cast<LPARAM>(MAKELONG(ptCurr.x, ptCurr.y)));
		::SendMessageW(m_hWndLinkTT, TTM_UPDATETIPTEXTW, 0, reinterpret_cast<LPARAM>(&ttiLink));
		::SendMessageW(m_hWndLinkTT, TTM_TRACKACTIVATE, TRUE, reinterpret_cast<LPARAM>(&ttiLink));
		m_tmTT = std::chrono::high_resolution_clock::now();
		::SetTimer(m_hWnd, m_uIDTLinkTTCheck, 300, nullptr); //Timer to check whether mouse has left link's rect.
	}
	else {
		::KillTimer(m_hWnd, m_uIDTLinkTTCheck);
		if (!fTimer) {
			m_htiCurrLink.iItem = -1;
			m_htiCurrLink.iSubItem = -1;
			m_rcLinkCurr.SetRectEmpty();
		}

		m_fLinkTTActive = false;
		::SendMessageW(m_hWndLinkTT, TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&ttiLink));
	}
}

void CListEx::TTHLShow(bool fShow, UINT uRow)
{
	if (fShow) {
		POINT ptCurr;
		GetCursorPos(&ptCurr);
		wchar_t warrOffset[32];
		*std::format_to(warrOffset, L"Row: {}", uRow) = L'\0';
		TTTOOLINFOW ttiHL { .cbSize { sizeof(TTTOOLINFOW) }, .lpszText { warrOffset } };
		::SendMessageW(m_hWndRowTT, TTM_TRACKPOSITION, 0, static_cast<LPARAM>(MAKELONG(ptCurr.x - 5, ptCurr.y - 20)));
		::SendMessageW(m_hWndRowTT, TTM_UPDATETIPTEXTW, 0, reinterpret_cast<LPARAM>(&ttiHL));
	}

	TTTOOLINFOW ttiHL { .cbSize { sizeof(TTTOOLINFOW) }, };
	::SendMessageW(m_hWndRowTT, TTM_TRACKACTIVATE, fShow, reinterpret_cast<LPARAM>(&ttiHL));
}

auto CListEx::SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIDSubclass, DWORD_PTR /*dwRefData*/) -> LRESULT {
	if (uMsg == WM_NCDESTROY) {
		::RemoveWindowSubclass(hWnd, SubclassProc, uIDSubclass);
	}

	const auto pListEx = reinterpret_cast<CListEx*>(uIDSubclass);
	return pListEx->ProcessMsg({ .hwnd { hWnd }, .message { uMsg }, .wParam { wParam }, .lParam { lParam } });
}

auto CListEx::EditSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR /*dwRefData*/)->LRESULT
{
	switch (uMsg) {
	case WM_GETDLGCODE:
		return DLGC_WANTALLKEYS;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE || wParam == VK_RETURN) {
			const NMHDR hdr { .hwndFrom { hWnd }, .idFrom { m_uIDEditInPlace }, .code { static_cast<UINT>(wParam) } };
			::SendMessageW(::GetParent(hWnd), WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&hdr));
		}
		break;
	case WM_NCDESTROY:
		RemoveWindowSubclass(hWnd, EditSubclassProc, uIdSubclass);
		break;
	default:
		break;
	}

	return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
}