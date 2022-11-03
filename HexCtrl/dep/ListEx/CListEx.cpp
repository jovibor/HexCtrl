/****************************************************************************************
* Copyright © 2018-2022 Jovibor https://github.com/jovibor/                             *
* This is very extended and featured version of CMFCListCtrl class.                     *
* Official git repository: https://github.com/jovibor/ListEx/                           *
* This code is available under the "MIT License".                                       *
****************************************************************************************/
#include "stdafx.h"
#include "ListEx.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <limits>
#include <optional>
#include <random>
#include <unordered_map>

namespace HEXCTRL::LISTEX::INTERNAL
{
	class CListExHdr final : public CMFCHeaderCtrl
	{
	public:
		explicit CListExHdr();
		~CListExHdr()final = default;
		void DeleteColumn(int iIndex);
		[[nodiscard]] UINT GetHiddenCount()const;
		void HideColumn(int iIndex, bool fHide);
		[[nodiscard]] bool IsColumnHidden(int iIndex); //Column index.
		[[nodiscard]] bool IsColumnSortable(int iIndex)const;
		void SetHeight(DWORD dwHeight);
		void SetFont(const LOGFONTW* pLogFontNew);
		void SetColor(const LISTEXCOLORS& lcs);
		void SetColumnColor(int iColumn, COLORREF clrBk, COLORREF clrText);
		void SetColumnIcon(int iColumn, const LISTEXHDRICON& stIcon);
		void SetColumnSortable(int iColumn, bool fSortable);
		void SetSortable(bool fSortable);
		void SetSortArrow(int iColumn, bool fAscending);
	private:
		struct SHDRCOLOR;
		struct SHDRICON;
		struct SHIDDEN;
		[[nodiscard]] UINT ColumnIndexToID(int iIndex)const; //Returns unique column ID. Must be > 0.
		[[nodiscard]] int ColumnIDToIndex(UINT uID)const;
		[[nodiscard]] auto HasColor(UINT ID)->SHDRCOLOR*;
		[[nodiscard]] auto HasIcon(UINT ID)->SHDRICON*;
		[[nodiscard]] auto IsHidden(UINT ID)->std::optional<SHIDDEN*>; //Internal ColumnID.
		[[nodiscard]] bool IsSortable(UINT ID)const;
		afx_msg void OnDestroy();
		afx_msg void OnDrawItem(CDC* pDC, int iItem, CRect rcOrig, BOOL bIsPressed, BOOL bIsHighlighted)override;
		afx_msg LRESULT OnLayout(WPARAM wParam, LPARAM lParam);
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
		afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
		afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
		DECLARE_MESSAGE_MAP()
	private:
		CFont m_fontHdr;
		CPen m_penGrid;
		CPen m_penLight;
		CPen m_penShadow;
		COLORREF m_clrBkNWA { }; //Bk of non working area.
		COLORREF m_clrText { };
		COLORREF m_clrBk { };
		COLORREF m_clrHglInactive { };
		COLORREF m_clrHglActive { };
		DWORD m_dwHeaderHeight { 19 }; //Standard (default) height.
		std::unordered_map<UINT, SHDRCOLOR> m_umapColors { }; //Colors for columns.
		std::unordered_map<UINT, SHDRICON> m_umapIcons { };   //Icons for columns.
		std::unordered_map<UINT, bool> m_umapIsSort { };      //Is column sortable.
		std::unordered_map<UINT, SHIDDEN> m_umapHidden { };   //Hidden columns.
		UINT m_uSortColumn { 0 };   //ColumnID to draw sorting triangle at. 0 is to avoid triangle before first clicking.
		bool m_fSortable { false }; //List-is-sortable global flog. Need to draw sortable triangle or not?
		bool m_fSortAscending { };  //Sorting type.
	};

	class CListEx final : public IListEx
	{
	public:
		bool Create(const LISTEXCREATE& lcs)override;
		void CreateDialogCtrl(UINT uCtrlID, CWnd* pParent)override;
		static int CALLBACK DefCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
		BOOL DeleteAllItems()override;
		BOOL DeleteColumn(int iIndex)override;
		BOOL DeleteItem(int iItem)override;
		void Destroy()override;
		[[nodiscard]] auto GetCellData(int iItem, int iSubItem)const->ULONGLONG override;
		[[nodiscard]] auto GetColors()const->LISTEXCOLORS override;
		[[nodiscard]] auto GetColumnSortMode(int iColumn)const->EListExSortMode override;
		[[nodiscard]] int GetSortColumn()const override;
		[[nodiscard]] bool GetSortAscending()const override;
		void HideColumn(int iIndex, bool fHide)override;
		int InsertColumn(int nCol, const LVCOLUMN* pColumn)override;
		int InsertColumn(int nCol, LPCTSTR lpszColumnHeading, int nFormat = LVCFMT_LEFT, int nWidth = -1, int nSubItem = -1)override;
		[[nodiscard]] bool IsCreated()const override;
		[[nodiscard]] bool IsColumnSortable(int iColumn)override;
		void ResetSort()override; //Reset all the sort by any column to its default state.
		void SetCellColor(int iItem, int iSubItem, COLORREF clrBk, COLORREF clrText)override;
		void SetCellData(int iItem, int iSubItem, ULONGLONG ullData)override;
		void SetCellIcon(int iItem, int iSubItem, int iIndex)override; //Icon index in list's image list.
		void SetCellTooltip(int iItem, int iSubItem, std::wstring_view wstrTooltip, std::wstring_view wstrCaption)override;
		void SetColors(const LISTEXCOLORS& lcs)override;
		void SetColumnColor(int iColumn, COLORREF clrBk, COLORREF clrText)override;
		void SetColumnSortMode(int iColumn, bool fSortable, EListExSortMode enSortMode = { })override;
		void SetFont(const LOGFONTW* pLogFontNew)override;
		void SetHdrColumnColor(int iColumn, COLORREF clrBk, COLORREF clrText = -1)override;
		void SetHdrColumnIcon(int iColumn, const LISTEXHDRICON& stIcon)override; //Icon for a given column.
		void SetHdrFont(const LOGFONTW* pLogFontNew)override;
		void SetHdrHeight(DWORD dwHeight)override;
		void SetHdrImageList(CImageList* pList)override;
		void SetRowColor(DWORD dwRow, COLORREF clrBk, COLORREF clrText)override;
		void SetSortable(bool fSortable, PFNLVCOMPARE pfnCompare, EListExSortMode enSortMode)override;
		DECLARE_DYNAMIC(CListEx);
		DECLARE_MESSAGE_MAP();
	private:
		struct SCOLROWCLR;
		struct SITEMDATA;
		[[nodiscard]] long GetFontSize();
		[[nodiscard]] auto GetHeaderCtrl()->CListExHdr & override { return m_stListHeader; }
		void FontSizeIncDec(bool fInc);
		void InitHeader()override;
		auto HasColor(int iItem, int iSubItem)->std::optional<PLISTEXCOLOR>;
		auto HasTooltip(int iItem, int iSubItem)->std::optional<PLISTEXTOOLTIP>;
		int HasIcon(int iItem, int iSubItem); //Does cell have an icon associated.
		auto ParseItemText(int iItem, int iSubitem)->std::vector<SITEMDATA>;
		void RecalcMeasure();
		void SetFontSize(long lSize);
		void TtLinkHide();
		void TtCellHide();
		void TtRowShow(bool fShow, UINT uRow); //Tooltips for HighLatency mode.
		void DrawItem(LPDRAWITEMSTRUCT pDIS)override;
		afx_msg void OnPaint();
		afx_msg BOOL OnEraseBkgnd(CDC* pDC);
		afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
		afx_msg void OnKillFocus(CWnd* pNewWnd);
		afx_msg void OnLButtonDown(UINT nFlags, CPoint pt);
		afx_msg void OnLButtonUp(UINT nFlags, CPoint pt);
		afx_msg void OnRButtonDown(UINT nFlags, CPoint pt);
		afx_msg void OnMouseMove(UINT nFlags, CPoint pt);
		afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
		afx_msg void OnTimer(UINT_PTR nIDEvent);
		afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
		afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
		afx_msg void MeasureItem(LPMEASUREITEMSTRUCT lpMIS);
		BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)override;
		afx_msg void OnLvnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnHdnBegindrag(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnHdnBegintrack(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnDestroy();
	private:
		CListExHdr m_stListHeader;
		LISTEXCOLORS m_stColors { };
		CFont m_fontList;               //Default list font.
		CFont m_fontListUnderline;      //Underlined list font, for links.
		CPen m_penGrid;                 //Pen for list lines between cells.
		CWnd m_stWndTtCell;             //Cells' tool-tip window.
		TTTOOLINFOW m_stTInfoCell { };  //Cells' tool-tip info struct.
		CWnd m_stWndTtLink;             //Link tool-tip window.
		TTTOOLINFOW m_stTInfoLink { };  //Link's tool-tip info struct.
		CWnd m_stWndTtRow { };          //Tooltip window for row in m_fHighLatency mode.
		TTTOOLINFOW m_stToolInfoRow { };//Tooltips struct.
		std::wstring m_wstrTtText { };  //Link's tool-tip current text.
		HCURSOR m_cursorHand { };       //Hand cursor handle.
		HCURSOR m_cursorDefault { };    //Standard (default) cursor handle.
		LVHITTESTINFO m_stCurrCell { }; //Cell's hit struct for tool-tip.
		LVHITTESTINFO m_stCurrLink { }; //Cell's link hit struct for tool-tip.
		DWORD m_dwGridWidth { 1 };		//Grid width.
		int m_iSortColumn { -1 };       //Currently clicked header column.
		PFNLVCOMPARE m_pfnCompare { nullptr };  //Pointer to user provided compare func.
		EListExSortMode m_enDefSortMode { EListExSortMode::SORT_LEX }; //Default sorting mode.
		CRect m_rcLinkCurr { };         //Current link's rect;
		std::unordered_map<int, std::unordered_map<int, LISTEXTOOLTIP>> m_umapCellTt { };  //Cell's tooltips.
		std::unordered_map<int, std::unordered_map<int, ULONGLONG>> m_umapCellData { };    //Cell's custom data.
		std::unordered_map<int, std::unordered_map<int, LISTEXCOLOR>> m_umapCellColor { }; //Cell's colors.
		std::unordered_map<int, SCOLROWCLR> m_umapRowColor { };                   //Row colors.
		std::unordered_map<int, SCOLROWCLR> m_umapColumnColor { };                //Column colors.
		std::unordered_map<int, std::unordered_map<int, int>> m_umapCellIcon { }; //Cell's icon.
		std::unordered_map<int, EListExSortMode> m_umapColumnSortMode { };        //Column sorting mode.
		UINT m_uHLItem { };            //High latency Vscroll item.
		int m_iLOGPIXELSY { };         //GetDeviceCaps(LOGPIXELSY) constant.
		bool m_fCreated { false };     //Is created.
		bool m_fHighLatency { };       //High latency flag.
		bool m_fSortable { false };    //Is list sortable.
		bool m_fSortAscending { };     //Sorting type (ascending, descending).
		bool m_fLinksUnderline { };    //Links are displayed underlined or not.
		bool m_fLinkTooltip { };       //Show links toolips.
		bool m_fVirtual { false };     //Whether list is virtual (LVS_OWNERDATA) or not.
		bool m_fTtCellShown { false }; //Is cell's tool-tip shown atm.
		bool m_fTtLinkShown { false }; //Is link's tool-tip shown atm.
		bool m_fLDownAtLink { false }; //Left mouse down on link.
		bool m_fHLFlag { };            //High latency Vscroll flag.
	};

	//Header column colors.
	struct CListExHdr::SHDRCOLOR {
		COLORREF clrBk { };   //Background color.
		COLORREF clrText { }; //Text color.
	};

	//Header column icons.
	struct CListExHdr::SHDRICON {
		LISTEXHDRICON stIcon { };   //Icon data struct.
		bool  fLMPressed { false }; //Left mouse button pressed atm.
	};

	//Hidden columns.
	struct CListExHdr::SHIDDEN {
		int iPrevPos { };
		int iPrevWidth { };
	};

	//Colors for the row/column.
	struct CListEx::SCOLROWCLR {
		LISTEXCOLOR clr { }; //Colors
		std::chrono::high_resolution_clock::time_point time { }; //Time when added.
	};

	//Text and links in the cell.
	struct CListEx::SITEMDATA {
		SITEMDATA(int iIconIndex, CRect rect) : iIconIndex(iIconIndex), rect(rect) {}; //Ctor for just image index.
		SITEMDATA(std::wstring_view wstrText, std::wstring_view wstrLink, std::wstring_view wstrTitle,
			CRect rect, bool fLink = false, bool fTitle = false) :
			wstrText(wstrText), wstrLink(wstrLink), wstrTitle(wstrTitle), rect(rect), fLink(fLink), fTitle(fTitle) {}
		std::wstring wstrText { };  //Visible text.
		std::wstring wstrLink { };  //Text within link <link="textFromHere"> tag.
		std::wstring wstrTitle { }; //Text within title <...title="textFromHere"> tag.
		CRect rect { };             //Rect text belongs to.
		int iIconIndex { -1 };      //Icon index in the image list, if any.
		bool fLink { false };       //Is it just a text (wstrLink is empty) or text with link?
		bool fTitle { false };      //Is it link with custom title (wstrTitle is not empty)?
	};

	constexpr ULONG_PTR ID_TIMER_TT_CELL_CHECK { 0x01 };    //Cell tool-tip check-timer ID.
	constexpr ULONG_PTR ID_TIMER_TT_LINK_CHECK { 0x02 };    //Link tool-tip check-timer ID.
	constexpr ULONG_PTR ID_TIMER_TT_LINK_ACTIVATE { 0x03 }; //Link tool-tip activate-timer ID.


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

namespace HEXCTRL::LISTEX
{
	IListEx* CreateRawListEx() {
		return new LISTEX::INTERNAL::CListEx();
	}
}

using namespace HEXCTRL::LISTEX::INTERNAL;


/****************************************************
* CListExHdr implementation.                        *
****************************************************/

BEGIN_MESSAGE_MAP(CListExHdr, CMFCHeaderCtrl)
	ON_MESSAGE(HDM_LAYOUT, &CListExHdr::OnLayout)
	ON_WM_HSCROLL()
	ON_WM_DESTROY()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()

CListExHdr::CListExHdr()
{
	m_penGrid.CreatePen(PS_SOLID, 2, RGB(220, 220, 220));
	m_penLight.CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));
	m_penShadow.CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
}

void CListExHdr::DeleteColumn(int iIndex)
{
	if (const auto ID = ColumnIndexToID(iIndex); ID > 0) {
		m_umapColors.erase(ID);
		m_umapIcons.erase(ID);
		m_umapIsSort.erase(ID);
		m_umapHidden.erase(ID);
	}
}

UINT CListExHdr::GetHiddenCount()const
{
	return static_cast<UINT>(m_umapHidden.size());
}

void CListExHdr::HideColumn(int iIndex, bool fHide)
{
	const auto iItemsCount = GetItemCount();
	if (iIndex >= iItemsCount)
		return;

	const auto ID = ColumnIndexToID(iIndex);
	std::vector<int> vecInt(iItemsCount, 0);
	ListView_GetColumnOrderArray(GetParent()->m_hWnd, iItemsCount, vecInt.data());
	HDITEMW hdi { HDI_WIDTH };
	GetItem(iIndex, &hdi);

	if (fHide) { //Hide column.
		m_umapHidden[ID].iPrevWidth = hdi.cxy;
		if (const auto iter = std::find(vecInt.begin(), vecInt.end(), iIndex); iter != vecInt.end()) {
			m_umapHidden[ID].iPrevPos = static_cast<int>(iter - vecInt.begin());
			std::rotate(iter, iter + 1, vecInt.end()); //Moving hiding column to the end of the column array.
		}

		ListView_SetColumnOrderArray(GetParent()->m_hWnd, iItemsCount, vecInt.data());
		hdi.cxy = 0;
		SetItem(iIndex, &hdi);
	}
	else { //Show column.
		const auto opt = IsHidden(ID);
		if (!opt) //No such column is hidden.
			return;

		if (const auto iterRight = std::find(vecInt.rbegin(), vecInt.rend(), iIndex); iterRight != vecInt.rend()) {
			const auto iterMid = iterRight + 1;
			const auto iterEnd = vecInt.rend() - (*opt)->iPrevPos;
			if (iterMid < iterEnd)
				std::rotate(iterRight, iterMid, iterEnd); //Moving hidden column id back to its previous place.
		}

		ListView_SetColumnOrderArray(GetParent()->m_hWnd, iItemsCount, vecInt.data());
		hdi.cxy = (*opt)->iPrevWidth;
		SetItem(iIndex, &hdi);
		m_umapHidden.erase(ID);
	}

	RedrawWindow();
}

bool CListExHdr::IsColumnHidden(int iIndex)
{
	return IsHidden(ColumnIndexToID(iIndex)).has_value();
}

bool CListExHdr::IsColumnSortable(int iIndex)const
{
	return IsSortable(ColumnIndexToID(iIndex));
}

void CListExHdr::SetFont(const LOGFONTW * pLogFont)
{
	if (!pLogFont)
		return;

	m_fontHdr.DeleteObject();
	m_fontHdr.CreateFontIndirectW(pLogFont);

	//If new font's height is higher than current height (m_dwHeaderHeight), we adjust current height as well.
	TEXTMETRICW tm;
	auto pDC = GetDC();
	pDC->SelectObject(m_fontHdr);
	pDC->GetTextMetricsW(&tm);
	ReleaseDC(pDC);
	const DWORD dwHeightFont = tm.tmHeight + tm.tmExternalLeading + 4;
	if (dwHeightFont > m_dwHeaderHeight)
		SetHeight(dwHeightFont);
}

void CListExHdr::SetHeight(DWORD dwHeight)
{
	m_dwHeaderHeight = dwHeight;
}

void CListExHdr::SetColor(const LISTEXCOLORS & lcs)
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
	if (ID == 0)
		return;

	if (clrText == -1)
		clrText = m_clrText;

	m_umapColors[ID] = SHDRCOLOR { clrBk, clrText };
	RedrawWindow();
}

void CListExHdr::SetColumnIcon(int iColumn, const LISTEXHDRICON & stIcon)
{
	const auto ID = ColumnIndexToID(iColumn);
	assert(ID > 0);
	if (ID == 0)
		return;

	if (stIcon.iIndex == -1) //If column already has icon.
		m_umapIcons.erase(ID);
	else {
		SHDRICON stHdrIcon;
		stHdrIcon.stIcon = stIcon;
		m_umapIcons[ID] = stHdrIcon;
	}
	RedrawWindow();
}

void CListExHdr::SetColumnSortable(int iColumn, bool fSortable)
{
	const auto ID = ColumnIndexToID(iColumn);
	assert(ID > 0);
	if (ID == 0)
		return;

	m_umapIsSort[ID] = fSortable;
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
		if (ID == 0)
			return;
	}
	m_uSortColumn = ID;
	m_fSortAscending = fAscending;
	RedrawWindow();
}


//////////////////////////////////////////////////////////////
//CListExHdr private methods:
//////////////////////////////////////////////////////////////

UINT CListExHdr::ColumnIndexToID(int iIndex)const
{
	//Each column has unique internal identifier in HDITEMW::lParam
	UINT uRet { 0 };
	HDITEMW hdi { HDI_LPARAM };
	if (GetItem(iIndex, &hdi) != FALSE)
		uRet = static_cast<UINT>(hdi.lParam);

	return uRet;
}

int CListExHdr::ColumnIDToIndex(UINT uID)const
{
	int iRet { -1 };
	for (int i = 0; i < GetItemCount(); ++i) {
		HDITEMW hdi { HDI_LPARAM };
		GetItem(i, &hdi);
		if (static_cast<UINT>(hdi.lParam) == uID)
			iRet = i;
	}

	return iRet;
}

void CListExHdr::OnDrawItem(CDC * pDC, int iItem, CRect rcOrig, BOOL bIsPressed, BOOL bIsHighlighted)
{
	//Non working area after last column. Or if column resized to zero.
	if (iItem < 0 || rcOrig.IsRectEmpty()) {
		pDC->FillSolidRect(&rcOrig, m_clrBkNWA);
		return;
	}

	CMemDC memDC(*pDC, rcOrig);
	auto& rDC = memDC.GetDC();
	const auto ID = ColumnIndexToID(iItem);

	auto const pClr = HasColor(ID);
	const COLORREF clrText { pClr != nullptr ? pClr->clrText : m_clrText };
	const COLORREF clrBk { bIsHighlighted ? (bIsPressed ? m_clrHglActive : m_clrHglInactive) : (pClr != nullptr ? pClr->clrBk : m_clrBk) };

	rDC.FillSolidRect(&rcOrig, clrBk);
	rDC.SetTextColor(clrText);
	rDC.SelectObject(m_fontHdr);

	//Set item's text buffer first char to zero, then getting item's text and Draw it.
	wchar_t warrHdrText[MAX_PATH];
	warrHdrText[0] = L'\0';
	HDITEMW hdItem { HDI_FORMAT | HDI_TEXT, 0, warrHdrText, nullptr, MAX_PATH };
	GetItem(iItem, &hdItem);

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

	//Draw icon for column, if any.
	long lIndentTextLeft { 4 }; //Left text indent.
	if (const auto pData = HasIcon(ID); pData != nullptr) { //If column has an icon.
		const auto pImgList = GetImageList(LVSIL_NORMAL);
		int iCX { };
		int iCY { };
		ImageList_GetIconSize(pImgList->m_hImageList, &iCX, &iCY); //Icon dimensions.
		pImgList->DrawEx(&rDC, pData->stIcon.iIndex, rcOrig.TopLeft() + pData->stIcon.pt, { }, CLR_NONE, CLR_NONE, ILD_NORMAL);
		lIndentTextLeft += pData->stIcon.pt.x + iCX;
	}

	constexpr long lIndentTextRight = 4;
	CRect rcText { rcOrig.left + lIndentTextLeft, rcOrig.top, rcOrig.right - lIndentTextRight, rcOrig.bottom };
	if (StrStrW(warrHdrText, L"\n")) {	//If it's multiline text, first — calculate rect for the text,
		//with DT_CALCRECT flag (not drawing anything),
		//and then calculate rect for final vertical text alignment.
		CRect rcCalcText;
		rDC.DrawTextW(warrHdrText, &rcCalcText, uFormat | DT_CALCRECT);
		rcText.top = rcText.Height() / 2 - rcCalcText.Height() / 2;
		rDC.DrawTextW(warrHdrText, &rcOrig, uFormat);
	}
	else
		rDC.DrawTextW(warrHdrText, &rcText, uFormat | DT_VCENTER | DT_SINGLELINE);

	//Draw sortable triangle (arrow).
	if (m_fSortable && IsSortable(ID) && ID == m_uSortColumn) {
		rDC.SelectObject(m_penLight);
		const auto iOffset = rcOrig.Height() / 4;

		if (m_fSortAscending) {
			//Draw the UP arrow.
			rDC.MoveTo(rcOrig.right - 2 * iOffset, iOffset);
			rDC.LineTo(rcOrig.right - iOffset, rcOrig.bottom - iOffset - 1);
			rDC.LineTo(rcOrig.right - 3 * iOffset - 2, rcOrig.bottom - iOffset - 1);
			rDC.SelectObject(m_penShadow);
			rDC.MoveTo(rcOrig.right - 3 * iOffset - 1, rcOrig.bottom - iOffset - 1);
			rDC.LineTo(rcOrig.right - 2 * iOffset, iOffset - 1);
		}
		else {
			//Draw the DOWN arrow.
			rDC.MoveTo(rcOrig.right - iOffset - 1, iOffset);
			rDC.LineTo(rcOrig.right - 2 * iOffset - 1, rcOrig.bottom - iOffset);
			rDC.SelectObject(m_penShadow);
			rDC.MoveTo(rcOrig.right - 2 * iOffset - 2, rcOrig.bottom - iOffset);
			rDC.LineTo(rcOrig.right - 3 * iOffset - 1, iOffset);
			rDC.LineTo(rcOrig.right - iOffset - 1, iOffset);
		}
	}

	//rDC.DrawEdge(&rect, EDGE_RAISED, BF_RECT); //3D look edges.
	rDC.SelectObject(m_penGrid);
	rDC.MoveTo(rcOrig.TopLeft());
	rDC.LineTo(rcOrig.left, rcOrig.bottom);
	if (iItem == GetItemCount() - 1) { //Last item.
		rDC.MoveTo(rcOrig.right, rcOrig.top);
		rDC.LineTo(rcOrig.BottomRight());
	}
}

LRESULT CListExHdr::OnLayout(WPARAM /*wParam*/, LPARAM lParam)
{
	CMFCHeaderCtrl::DefWindowProcW(HDM_LAYOUT, 0, lParam);

	const auto pHDL = reinterpret_cast<LPHDLAYOUT>(lParam);
	pHDL->pwpos->cy = m_dwHeaderHeight;	//New header height.
	pHDL->prc->top = m_dwHeaderHeight;  //Decreasing list's height begining by the new header's height.

	return 0;
}

void CListExHdr::OnLButtonDown(UINT nFlags, CPoint point)
{
	HDHITTESTINFO ht { };
	ht.pt = point;
	HitTest(&ht);
	if (ht.iItem >= 0) {
		const auto ID = ColumnIndexToID(ht.iItem);
		if (const auto pData = HasIcon(ID); pData != nullptr && pData->stIcon.fClickable && !IsHidden(ID)) {
			int iCX { };
			int iCY { };
			ImageList_GetIconSize(GetImageList()->m_hImageList, &iCX, &iCY);
			CRect rcColumn;
			GetItemRect(ht.iItem, rcColumn);
			const CRect rcIcon(rcColumn.TopLeft() + pData->stIcon.pt, SIZE { iCX, iCY });

			if (rcIcon.PtInRect(point)) {
				pData->fLMPressed = true;
				return; //Do not invoke default handler.
			}
		}
	}

	CMFCHeaderCtrl::OnLButtonDown(nFlags, point);
}

void CListExHdr::OnLButtonUp(UINT nFlags, CPoint point)
{
	HDHITTESTINFO ht { };
	ht.pt = point;
	HitTest(&ht);
	if (ht.iItem >= 0) {
		for (auto& iter : m_umapIcons) {
			if (!iter.second.fLMPressed)
				continue;

			int iCX { };
			int iCY { };
			ImageList_GetIconSize(GetImageList()->m_hImageList, &iCX, &iCY);
			CRect rcColumn;
			GetItemRect(ht.iItem, rcColumn);
			const CRect rcIcon(rcColumn.TopLeft() + iter.second.stIcon.pt, SIZE { iCX, iCY });

			if (rcIcon.PtInRect(point)) {
				for (auto& iterData : m_umapIcons)
					iterData.second.fLMPressed = false; //Remove fLMPressed flag from all columns.

				if (auto pParent = GetParent(); pParent != nullptr) { //List control pointer.
					const auto uCtrlId = static_cast<UINT>(pParent->GetDlgCtrlID());
					NMHEADERW hdr { { pParent->m_hWnd, uCtrlId, LISTEX_MSG_HDRICONCLICK } };
					hdr.iItem = ColumnIDToIndex(iter.first);
					pParent->GetParent()->SendMessageW(WM_NOTIFY, static_cast<WPARAM>(uCtrlId), reinterpret_cast<LPARAM>(&hdr));
				}
			}

			break;
		}
	}

	CMFCHeaderCtrl::OnLButtonUp(nFlags, point);
}

void CListExHdr::OnRButtonDown(UINT /*nFlags*/, CPoint point)
{
	const auto pParent = GetParent();
	if (pParent == nullptr) //List control pointer.
		return;

	const auto uCtrlId = static_cast<UINT>(pParent->GetDlgCtrlID());
	NMHEADERW hdr { { pParent->m_hWnd, uCtrlId, LISTEX_MSG_HDRRBTNDOWN } };
	HDHITTESTINFO ht { };
	ht.pt = point;
	HitTest(&ht);
	hdr.iItem = ht.iItem;
	pParent->GetParent()->SendMessageW(WM_NOTIFY, static_cast<WPARAM>(uCtrlId), reinterpret_cast<LPARAM>(&hdr));
}

void CListExHdr::OnRButtonUp(UINT /*nFlags*/, CPoint point)
{
	const auto pParent = GetParent();
	if (pParent == nullptr) //List control pointer.
		return;

	const auto uCtrlId = static_cast<UINT>(pParent->GetDlgCtrlID());
	NMHEADERW hdr { { pParent->m_hWnd, uCtrlId, LISTEX_MSG_HDRRBTNUP } };
	HDHITTESTINFO ht { };
	ht.pt = point;
	HitTest(&ht);
	hdr.iItem = ht.iItem;
	pParent->GetParent()->SendMessageW(WM_NOTIFY, static_cast<WPARAM>(uCtrlId), reinterpret_cast<LPARAM>(&hdr));
}

void CListExHdr::OnDestroy()
{
	CMFCHeaderCtrl::OnDestroy();

	m_umapColors.clear();
	m_umapIcons.clear();
	m_umapIsSort.clear();
	m_umapHidden.clear();
}

auto CListExHdr::HasColor(UINT ID)->CListExHdr::SHDRCOLOR*
{
	SHDRCOLOR* pRet { };
	if (const auto it = m_umapColors.find(ID); it != m_umapColors.end())
		pRet = &it->second;

	return pRet;
}

auto CListExHdr::HasIcon(UINT ID)->CListExHdr::SHDRICON*
{
	if (GetImageList() == nullptr)
		return nullptr;

	SHDRICON* pRet { };
	if (const auto it = m_umapIcons.find(ID); it != m_umapIcons.end())
		pRet = &it->second;

	return pRet;
}

auto CListExHdr::IsHidden(UINT ID)->std::optional<SHIDDEN*>
{
	const auto iter = m_umapHidden.find(ID);

	return iter != m_umapHidden.end() ? &iter->second : std::optional<SHIDDEN*> { };
}

bool CListExHdr::IsSortable(UINT ID)const
{
	const auto iter = m_umapIsSort.find(ID); //It's sortable unless found explicitly as false.

	return 	iter == m_umapIsSort.end() || iter->second;
}



/****************************************************
* CListEx implementation.                           *
****************************************************/
IMPLEMENT_DYNAMIC(CListEx, CMFCListCtrl)

BEGIN_MESSAGE_MAP(CListEx, CMFCListCtrl)
	ON_WM_SETCURSOR()
	ON_WM_KILLFOCUS()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_TIMER()
	ON_WM_ERASEBKGND()
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_PAINT()
	ON_WM_MEASUREITEM_REFLECT()
	ON_WM_CONTEXTMENU()
	ON_NOTIFY(HDN_BEGINDRAG, 0, &CListEx::OnHdnBegindrag)
	ON_NOTIFY(HDN_BEGINTRACKA, 0, &CListEx::OnHdnBegintrack)
	ON_NOTIFY(HDN_BEGINTRACKW, 0, &CListEx::OnHdnBegintrack)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, &CListEx::OnLvnColumnClick)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CListEx::ResetSort()
{
	m_iSortColumn = -1;
	GetHeaderCtrl().SetSortArrow(-1, false);
}

bool CListEx::Create(const LISTEXCREATE& lcs)
{
	assert(!IsCreated());
	if (IsCreated())
		return false;

	auto dwStyle = static_cast<LONG_PTR>(lcs.dwStyle);
	if (lcs.fDialogCtrl) {
		SubclassDlgItem(lcs.uID, lcs.pParent);
		dwStyle = GetWindowLongPtrW(m_hWnd, GWL_STYLE);
		SetWindowLongPtrW(m_hWnd, GWL_STYLE, dwStyle | LVS_OWNERDRAWFIXED | LVS_REPORT);
	}
	else if (!CMFCListCtrl::Create(lcs.dwStyle | WS_CHILD | WS_VISIBLE | LVS_OWNERDRAWFIXED | LVS_REPORT,
		lcs.rect, lcs.pParent, lcs.uID))
		return false;

	m_fVirtual = dwStyle & LVS_OWNERDATA;
	m_stColors = lcs.stColor;
	m_fSortable = lcs.fSortable;
	m_fLinksUnderline = lcs.fLinkUnderline;
	m_fLinkTooltip = lcs.fLinkTooltip;
	m_fHighLatency = lcs.fHighLatency;

	if (!m_stWndTtCell.CreateEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr, TTS_BALLOON | TTS_NOANIMATE | TTS_NOFADE | TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr))
		return false;
	SetWindowTheme(m_stWndTtCell, nullptr, L""); //To prevent Windows from changing theme of Balloon window.
	m_stTInfoCell.cbSize = TTTOOLINFOW_V1_SIZE;
	m_stTInfoCell.uFlags = TTF_TRACK;
	m_stTInfoCell.uId = 0x01;
	m_stWndTtCell.SendMessageW(TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&m_stTInfoCell));
	m_stWndTtCell.SendMessageW(TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(400)); //to allow use of newline \n.
	m_stWndTtCell.SendMessageW(TTM_SETTIPTEXTCOLOR, static_cast<WPARAM>(m_stColors.clrTooltipText), 0);
	m_stWndTtCell.SendMessageW(TTM_SETTIPBKCOLOR, static_cast<WPARAM>(m_stColors.clrTooltipBk), 0);

	if (!m_stWndTtLink.CreateEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr, TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr))
		return false;
	m_stTInfoLink.cbSize = TTTOOLINFOW_V1_SIZE;
	m_stTInfoLink.uFlags = TTF_TRACK;
	m_stTInfoLink.uId = 0x02;
	m_stWndTtLink.SendMessageW(TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&m_stTInfoLink));
	m_stWndTtLink.SendMessageW(TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(400)); //to allow use of newline \n.

	if (m_fHighLatency) { //Tooltip for HighLatency mode.
		if (!m_stWndTtRow.CreateEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr, TTS_NOANIMATE | TTS_NOFADE | TTS_NOPREFIX | TTS_ALWAYSTIP,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, nullptr))
			return false;
		m_stToolInfoRow.cbSize = TTTOOLINFOW_V1_SIZE;
		m_stToolInfoRow.uFlags = TTF_TRACK;
		m_stToolInfoRow.uId = 0x03;
		m_stWndTtRow.SendMessageW(TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&m_stToolInfoRow));
	}

	m_dwGridWidth = lcs.dwListGridWidth;

	NONCLIENTMETRICSW ncm { };
	ncm.cbSize = sizeof(NONCLIENTMETRICSW);
	SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);

	auto pDC = GetDC();
	m_iLOGPIXELSY = GetDeviceCaps(pDC->m_hDC, LOGPIXELSY);
	CFont fontDefault;
	fontDefault.CreateFontIndirectW(&ncm.lfMessageFont);
	TEXTMETRICW tm;
	pDC->SelectObject(fontDefault);
	GetTextMetricsW(pDC->m_hDC, &tm);
	const DWORD dwHdrHeight = lcs.dwHdrHeight == 0 ? tm.tmHeight + tm.tmExternalLeading + 4 : lcs.dwHdrHeight; //Header is a bit higher than list rows.
	ReleaseDC(pDC);

	LOGFONT lfList; //Font for a List.
	if (lcs.pListLogFont == nullptr) {
		lfList = ncm.lfMessageFont;
		lfList.lfHeight = -MulDiv(10, m_iLOGPIXELSY, 72); //Font height for list.
	}
	else {
		lfList = *lcs.pListLogFont;
	}

	LOGFONT lfHdr; //Font for a Header.
	if (lcs.pHdrLogFont == nullptr) {
		lfHdr = ncm.lfMessageFont;
		lfHdr.lfHeight = -MulDiv(9, m_iLOGPIXELSY, 72); //Font height for a header.
	}
	else {
		lfHdr = *lcs.pHdrLogFont;
	}

	m_fontList.CreateFontIndirectW(&lfList);
	lfList.lfUnderline = 1;
	m_fontListUnderline.CreateFontIndirectW(&lfList);

	m_penGrid.CreatePen(PS_SOLID, m_dwGridWidth, m_stColors.clrListGrid);
	m_cursorDefault = static_cast<HCURSOR>(LoadImageW(nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
	m_cursorHand = static_cast<HCURSOR>(LoadImageW(nullptr, IDC_HAND, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
	SetClassLongPtrW(m_hWnd, GCLP_HCURSOR, 0); //To prevent cursor from blinking.

	RecalcMeasure();

	m_fCreated = true;

	GetHeaderCtrl().SetColor(lcs.stColor);
	GetHeaderCtrl().SetSortable(lcs.fSortable);
	SetHdrHeight(dwHdrHeight);
	SetHdrFont(&lfHdr);
	Update(0);

	return true;
}

void CListEx::CreateDialogCtrl(UINT uCtrlID, CWnd* pParent)
{
	LISTEXCREATE lcs;
	lcs.pParent = pParent;
	lcs.uID = uCtrlID;
	lcs.fDialogCtrl = true;

	Create(lcs);
}

int CALLBACK CListEx::DefCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	const auto* const pListCtrl = reinterpret_cast<IListEx*>(lParamSort);
	const auto iSortColumn = pListCtrl->GetSortColumn();
	const auto enSortMode = pListCtrl->GetColumnSortMode(iSortColumn);

	const auto wstrItem1 = pListCtrl->GetItemText(static_cast<int>(lParam1), iSortColumn);
	const auto wstrItem2 = pListCtrl->GetItemText(static_cast<int>(lParam2), iSortColumn);

	int iCompare { };
	switch (enSortMode) {
	case EListExSortMode::SORT_LEX:
		iCompare = wstrItem1.Compare(wstrItem2);
		break;
	case EListExSortMode::SORT_NUMERIC:
	{
		LONGLONG llData1 { }, llData2 { };
		StrToInt64ExW(wstrItem1, STIF_SUPPORT_HEX, &llData1);
		StrToInt64ExW(wstrItem2, STIF_SUPPORT_HEX, &llData2);
		iCompare = llData1 != llData2 ? (llData1 - llData2 < 0 ? -1 : 1) : 0;
	}
	break;
	}

	int iResult = 0;
	if (pListCtrl->GetSortAscending()) {
		if (iCompare < 0)
			iResult = -1;
		else if (iCompare > 0)
			iResult = 1;
	}
	else {
		if (iCompare < 0)
			iResult = 1;
		else if (iCompare > 0)
			iResult = -1;
	}

	return iResult;
}

BOOL CListEx::DeleteAllItems()
{
	assert(IsCreated());
	if (!IsCreated())
		return FALSE;

	m_umapCellTt.clear();
	m_umapCellData.clear();
	m_umapCellColor.clear();
	m_umapRowColor.clear();
	m_umapCellIcon.clear();

	return CMFCListCtrl::DeleteAllItems();
}

BOOL CListEx::DeleteColumn(int iIndex)
{
	assert(IsCreated());
	if (!IsCreated())
		return FALSE;

	m_umapColumnColor.erase(iIndex);
	m_umapColumnSortMode.erase(iIndex);
	GetHeaderCtrl().DeleteColumn(iIndex);

	return CMFCListCtrl::DeleteColumn(iIndex);
}

BOOL CListEx::DeleteItem(int iItem)
{
	assert(IsCreated());
	if (!IsCreated())
		return FALSE;

	const auto ID = MapIndexToID(iItem);

	m_umapCellTt.erase(ID);
	m_umapCellData.erase(ID);
	m_umapCellColor.erase(ID);
	m_umapRowColor.erase(ID);
	m_umapCellIcon.erase(ID);

	return CMFCListCtrl::DeleteItem(iItem);
}

void CListEx::Destroy()
{
	delete this;
}

ULONGLONG CListEx::GetCellData(int iItem, int iSubItem)const
{
	assert(IsCreated());
	if (!IsCreated())
		return 0;

	const auto ID = MapIndexToID(iItem);
	const auto it = m_umapCellData.find(ID);

	if (it != m_umapCellData.end()) {
		const auto itInner = it->second.find(iSubItem);

		//If subitem id found.
		if (itInner != it->second.end())
			return itInner->second;
	}

	return 0;
}

auto CListEx::GetColors()const->LISTEXCOLORS {
	return m_stColors;
}

auto CListEx::GetColumnSortMode(int iColumn)const->EListExSortMode
{
	assert(IsCreated());

	EListExSortMode enMode;
	const auto iter = m_umapColumnSortMode.find(iColumn);
	if (iter != m_umapColumnSortMode.end())
		enMode = iter->second;
	else
		enMode = m_enDefSortMode;

	return enMode;
}

long CListEx::GetFontSize()
{
	LOGFONTW lf;
	m_fontList.GetLogFont(&lf);

	return lf.lfHeight;
}

int CListEx::GetSortColumn()const
{
	assert(IsCreated());
	if (!IsCreated())
		return -1;

	return m_iSortColumn;
}

bool CListEx::GetSortAscending()const
{
	assert(IsCreated());
	if (!IsCreated())
		return false;

	return m_fSortAscending;
}

void CListEx::HideColumn(int iIndex, bool fHide)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	GetHeaderCtrl().HideColumn(iIndex, fHide);
	RedrawWindow();
}

int CListEx::InsertColumn(int nCol, const LVCOLUMN* pColumn)
{
	assert(IsCreated());
	if (!IsCreated())
		return -1;

	auto& refHdr = GetHeaderCtrl();
	const auto nHiddenCount = refHdr.GetHiddenCount();

	//Checking that the new column ID (nCol) not greater than the count of 
	//the header items minus count of the already hidden columns.
	if (nHiddenCount > 0 && nCol >= static_cast<int>(refHdr.GetItemCount() - nHiddenCount)) {
		nCol = refHdr.GetItemCount() - nHiddenCount;
	}

	const auto iNewIndex = CMFCListCtrl::InsertColumn(nCol, pColumn);

	//Assigning each column a unique internal random identifier.
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<unsigned int> distrib(1, (std::numeric_limits<unsigned int>::max)());

	HDITEMW hdi { };
	hdi.mask = HDI_LPARAM;
	hdi.lParam = static_cast<LPARAM>(distrib(gen));
	refHdr.SetItem(iNewIndex, &hdi);

	return iNewIndex;
}

int CListEx::InsertColumn(int nCol, LPCTSTR lpszColumnHeading, int nFormat, int nWidth, int nSubItem)
{
	assert(IsCreated());
	if (!IsCreated())
		return -1;

	auto& refHdr = GetHeaderCtrl();
	const auto nHiddenCount = refHdr.GetHiddenCount();

	//Checking that the new column ID (nCol) not greater than the count of 
	//the header items minus count of the already hidden columns.
	if (refHdr.GetHiddenCount() > 0 && nCol >= static_cast<int>(refHdr.GetItemCount() - nHiddenCount)) {
		nCol = refHdr.GetItemCount() - nHiddenCount;
	}

	const auto iNewIndex = CMFCListCtrl::InsertColumn(nCol, lpszColumnHeading, nFormat, nWidth, nSubItem);

	//Assigning each column a unique internal random identifier.
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<unsigned int> distrib(1, (std::numeric_limits<unsigned int>::max)());

	HDITEMW hdi { };
	hdi.mask = HDI_LPARAM;
	hdi.lParam = static_cast<LPARAM>(distrib(gen));
	refHdr.SetItem(iNewIndex, &hdi);

	return iNewIndex;
}

bool CListEx::IsCreated()const
{
	return m_fCreated;
}

bool CListEx::IsColumnSortable(int iColumn)
{
	return GetHeaderCtrl().IsColumnSortable(iColumn);
}

void CListEx::SetCellColor(int iItem, int iSubItem, COLORREF clrBk, COLORREF clrText)
{
	assert(IsCreated());
	assert(!m_fVirtual);
	if (!IsCreated() || m_fVirtual)
		return;

	if (clrText == -1) //-1 for default color.
		clrText = m_stColors.clrListText;

	const auto ID = MapIndexToID(static_cast<UINT>(iItem));

	//If there is no color for such item/subitem we just set it.
	if (const auto it = m_umapCellColor.find(ID); it == m_umapCellColor.end()) {	//Initializing inner map.
		std::unordered_map<int, LISTEXCOLOR> umapInner { { iSubItem, { clrBk, clrText } } };
		m_umapCellColor.insert({ ID, std::move(umapInner) });
	}
	else {
		const auto itInner = it->second.find(iSubItem);

		if (itInner == it->second.end())
			it->second.insert({ iSubItem, { clrBk, clrText } });
		else //If there is already exist this cell's color -> changing.
		{
			itInner->second.clrBk = clrBk;
			itInner->second.clrText = clrText;
		}
	}
}

void CListEx::SetCellData(int iItem, int iSubItem, ULONGLONG ullData)
{
	assert(IsCreated());
	assert(!m_fVirtual);
	if (!IsCreated() || m_fVirtual)
		return;

	const auto ID = static_cast<int>(MapIndexToID(iItem));

	//If there is no data for such item/subitem we just set it.
	if (const auto it = m_umapCellData.find(ID); it == m_umapCellData.end())
		m_umapCellData.insert({ ID, { { iSubItem, ullData } } });
	else {
		if (const auto itInner = it->second.find(iSubItem); itInner == it->second.end())
			it->second.insert({ iSubItem, ullData });
		else //If there is already exist this cell's data -> changing.
			itInner->second = ullData;
	}
}

void CListEx::SetCellIcon(int iItem, int iSubItem, int iIndex)
{
	assert(IsCreated());
	assert(!m_fVirtual);
	if (!IsCreated() || m_fVirtual)
		return;

	const auto ID = static_cast<int>(MapIndexToID(iItem));

	//If there is no icon index for such item we just set it.
	if (const auto it = m_umapCellIcon.find(ID); it == m_umapCellIcon.end())
		m_umapCellIcon.insert({ ID, { { iSubItem, iIndex } } });
	else {
		if (const auto itInner = it->second.find(iSubItem); itInner == it->second.end())
			it->second.insert({ iSubItem, iIndex });
		else
			itInner->second = iIndex;
	}
}

void CListEx::SetCellTooltip(int iItem, int iSubItem, std::wstring_view wstrTooltip, std::wstring_view wstrCaption)
{
	assert(IsCreated());
	assert(!m_fVirtual);
	if (!IsCreated() || m_fVirtual)
		return;

	const auto ID = MapIndexToID(iItem);

	//If there is no tooltip for such item/subitem we just set it.
	if (const auto it = m_umapCellTt.find(ID); it == m_umapCellTt.end()) {
		if (!wstrTooltip.empty() || !wstrCaption.empty()) {	//Initializing inner map.
			std::unordered_map<int, LISTEXTOOLTIP> umapInner {
				{ iSubItem, { std::wstring { wstrTooltip }, std::wstring { wstrCaption } } }
			};
			m_umapCellTt.insert({ ID, std::move(umapInner) });
		}
	}
	else {
		//If there is Item's tooltip but no Subitem's tooltip
		//inserting new Subitem into inner map.
		if (const auto itInner = it->second.find(iSubItem);	itInner == it->second.end()) {
			if (!wstrTooltip.empty() || !wstrCaption.empty())
				it->second.insert({ iSubItem, { std::wstring { wstrTooltip }, std::wstring { wstrCaption } } });
		}
		else {	//If there is already exist this Item-Subitem's tooltip:
			//change or erase it, depending on pwszTooltip emptiness.
			if (!wstrTooltip.empty())
				itInner->second = { std::wstring { wstrTooltip }, std::wstring { wstrCaption } };
			else
				it->second.erase(itInner);
		}
	}
}

void CListEx::SetColors(const LISTEXCOLORS& lcs)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_stColors = lcs;
	GetHeaderCtrl().SetColor(lcs);
	RedrawWindow();
}

void CListEx::SetColumnColor(int iColumn, COLORREF clrBk, COLORREF clrText)
{
	if (clrText == -1) //-1 for default color.
		clrText = m_stColors.clrListText;

	m_umapColumnColor[iColumn] = SCOLROWCLR { clrBk, clrText, std::chrono::high_resolution_clock::now() };
}

void CListEx::SetColumnSortMode(int iColumn, bool fSortable, EListExSortMode enSortMode)
{
	m_umapColumnSortMode[iColumn] = enSortMode;
	GetHeaderCtrl().SetColumnSortable(iColumn, fSortable);
}

void CListEx::SetFont(const LOGFONTW* pLogFontNew)
{
	assert(IsCreated());
	assert(pLogFontNew);
	if (!IsCreated() || !pLogFontNew)
		return;

	m_fontList.DeleteObject();
	m_fontList.CreateFontIndirectW(pLogFontNew);

	RecalcMeasure();
	Update(0);
}

void CListEx::SetHdrHeight(DWORD dwHeight)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	GetHeaderCtrl().SetHeight(dwHeight);
	Update(0);
	GetHeaderCtrl().RedrawWindow();
}

void CListEx::SetHdrImageList(CImageList* pList)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	GetHeaderCtrl().SetImageList(pList);
}

void CListEx::SetHdrFont(const LOGFONTW* pLogFontNew)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	GetHeaderCtrl().SetFont(pLogFontNew);
	Update(0);
	GetHeaderCtrl().RedrawWindow();
}

void CListEx::SetHdrColumnColor(int iColumn, COLORREF clrBk, COLORREF clrText)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	GetHeaderCtrl().SetColumnColor(iColumn, clrBk, clrText);
	Update(0);
	GetHeaderCtrl().RedrawWindow();
}

void CListEx::SetHdrColumnIcon(int iColumn, const LISTEXHDRICON& stIcon)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	GetHeaderCtrl().SetColumnIcon(iColumn, stIcon);
}

void CListEx::SetRowColor(DWORD dwRow, COLORREF clrBk, COLORREF clrText)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	if (clrText == -1) //-1 for default color.
		clrText = m_stColors.clrListText;

	m_umapRowColor[dwRow] = SCOLROWCLR { clrBk, clrText, std::chrono::high_resolution_clock::now() };
}

void CListEx::SetSortable(bool fSortable, PFNLVCOMPARE pfnCompare, EListExSortMode enSortMode)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_fSortable = fSortable;
	m_pfnCompare = pfnCompare;
	m_enDefSortMode = enSortMode;

	GetHeaderCtrl().SetSortable(fSortable);
}


//////////////////////////////////////////////////////////////
//CListEx private methods:
//////////////////////////////////////////////////////////////

void CListEx::DrawItem(LPDRAWITEMSTRUCT pDIS)
{
	if (pDIS->itemID == -1)
		return;

	switch (pDIS->itemAction) {
	case ODA_SELECT:
	case ODA_DRAWENTIRE:
	{
		const auto pDC = CDC::FromHandle(pDIS->hDC);
		const auto clrBkCurrRow = (pDIS->itemID % 2) ? m_stColors.clrListBkRow2 : m_stColors.clrListBkRow1;
		const auto iColumns = GetHeaderCtrl().GetItemCount();

		for (auto iColumn = 0; iColumn < iColumns; ++iColumn) {
			if (GetHeaderCtrl().IsColumnHidden(iColumn))
				continue;

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

				if (const auto opt = HasColor(pDIS->itemID, iColumn); opt) {
					clrText = (*opt)->clrText;
					clrBk = (*opt)->clrBk;
				}
				else {
					if (HasTooltip(pDIS->itemID, iColumn)) {
						clrText = m_stColors.clrListTextCellTt;
						clrBk = m_stColors.clrListBkCellTt;
					}
					else {
						clrText = m_stColors.clrListText;
						clrBk = clrBkCurrRow;
					}
				}
			}
			CRect rcBounds;
			GetSubItemRect(pDIS->itemID, iColumn, LVIR_BOUNDS, rcBounds);
			pDC->FillSolidRect(rcBounds, clrBk);

			CRect rcText;
			GetSubItemRect(pDIS->itemID, iColumn, LVIR_LABEL, rcText);
			if (iColumn != 0) //Not needed for item itself (not subitem).
				rcText.left += 4;

			for (const auto& iter : ParseItemText(pDIS->itemID, iColumn)) {
				if (iter.iIconIndex > -1) {
					GetImageList(LVSIL_NORMAL)->DrawEx(
						pDC, iter.iIconIndex, { iter.rect.left, iter.rect.top }, { }, CLR_NONE, CLR_NONE, ILD_NORMAL);
					continue;
				}
				if (iter.fLink) {
					pDC->SetTextColor(clrTextLink);
					if (m_fLinksUnderline)
						pDC->SelectObject(m_fontListUnderline);
				}
				else {
					pDC->SetTextColor(clrText);
					pDC->SelectObject(m_fontList);
				}
				ExtTextOutW(pDC->m_hDC, iter.rect.left, iter.rect.top, ETO_CLIPPED,
					rcText, iter.wstrText.data(), static_cast<UINT>(iter.wstrText.size()), nullptr);
			}

			//Drawing subitem's rect lines.
			pDC->SelectObject(m_penGrid);
			pDC->MoveTo(rcBounds.TopLeft());
			pDC->LineTo(rcBounds.right, rcBounds.top);
			pDC->MoveTo(rcBounds.TopLeft());
			pDC->LineTo(rcBounds.left, rcBounds.bottom);
			pDC->MoveTo(rcBounds.left, rcBounds.bottom);
			pDC->LineTo(rcBounds.BottomRight());
			pDC->MoveTo(rcBounds.right, rcBounds.top);
			pDC->LineTo(rcBounds.BottomRight());

			//Draw focus rect (marquee).
			if ((pDIS->itemState & ODS_FOCUS) && !(pDIS->itemState & ODS_SELECTED))
				pDC->DrawFocusRect(rcBounds);
		}
	}
	break;
	default:
		break;
	}
}

void CListEx::FontSizeIncDec(bool fInc)
{
	auto lFontSize = MulDiv(-GetFontSize(), 72, m_iLOGPIXELSY);
	if (fInc) {
		++lFontSize;
	}
	else {
		--lFontSize;
	}

	SetFontSize(lFontSize);
}

auto CListEx::HasColor(int iItem, int iSubItem)->std::optional<PLISTEXCOLOR>
{
	if (iItem < 0 || iSubItem < 0)
		return std::nullopt;

	std::optional<PLISTEXCOLOR> opt { };
	if (m_fVirtual) { //In Virtual mode asking parent for color.
		const auto iCtrlID = GetDlgCtrlID();
		NMITEMACTIVATE nmii { { m_hWnd, static_cast<UINT>(iCtrlID), LISTEX_MSG_GETCOLOR } };
		nmii.iItem = iItem;
		nmii.iSubItem = iSubItem;
		GetParent()->SendMessageW(WM_NOTIFY, static_cast<WPARAM>(iCtrlID), reinterpret_cast<LPARAM>(&nmii));
		if (nmii.lParam != 0)
			opt = reinterpret_cast<PLISTEXCOLOR>(nmii.lParam);
	}
	else {
		const auto ID = MapIndexToID(static_cast<UINT>(iItem));
		if (const auto itItem = m_umapCellColor.find(ID); itItem != m_umapCellColor.end()) {
			const auto itSubItem = itItem->second.find(iSubItem);

			//If subitem id found.
			if (itSubItem != itItem->second.end())
				opt = &itSubItem->second;
		}

		if (!opt) {
			const auto itColumn = m_umapColumnColor.find(iSubItem);
			const auto itRow = m_umapRowColor.find(ID);

			if (itColumn != m_umapColumnColor.end() && itRow != m_umapRowColor.end())
				opt = itColumn->second.time > itRow->second.time ? &itColumn->second.clr : &itRow->second.clr;
			else if (itColumn != m_umapColumnColor.end())
				opt = &itColumn->second.clr;
			else if (itRow != m_umapRowColor.end())
				opt = &itRow->second.clr;
		}
	}

	return opt;
}

auto CListEx::HasTooltip(int iItem, int iSubItem)->std::optional<PLISTEXTOOLTIP>
{
	if (iItem < 0 || iSubItem < 0)
		return std::nullopt;

	std::optional<PLISTEXTOOLTIP> opt { };
	if (m_fVirtual) { //In Virtual mode asking parent for tooltip.
		const auto iCtrlID = GetDlgCtrlID();
		NMITEMACTIVATE nmii { { m_hWnd, static_cast<UINT>(iCtrlID), LISTEX_MSG_GETTOOLTIP } };
		nmii.iItem = iItem;
		nmii.iSubItem = iSubItem;
		GetParent()->SendMessageW(WM_NOTIFY, static_cast<WPARAM>(iCtrlID), reinterpret_cast<LPARAM>(&nmii));
		if (nmii.lParam != 0)
			opt = reinterpret_cast<PLISTEXTOOLTIP>(nmii.lParam);
	}
	else {
		const auto ID = MapIndexToID(iItem);
		if (const auto itItem = m_umapCellTt.find(ID); itItem != m_umapCellTt.end())
			if (const auto itSubItem = itItem->second.find(iSubItem); itSubItem != itItem->second.end())
				opt = &itSubItem->second;
	}

	return opt;
}

int CListEx::HasIcon(int iItem, int iSubItem)
{
	if (GetImageList(LVSIL_NORMAL) == nullptr)
		return -1;

	int iRet { -1 }; //-1 is default, when no image for cell set.

	if (m_fVirtual) { //In Virtual List mode asking parent for the icon index in image list.
		const auto uCtrlID = static_cast<UINT>(GetDlgCtrlID());
		NMITEMACTIVATE nmii { { m_hWnd, uCtrlID, LISTEX_MSG_GETICON } };
		nmii.iItem = iItem;
		nmii.iSubItem = iSubItem;
		nmii.lParam = -1; //Default, no icon.
		GetParent()->SendMessageW(WM_NOTIFY, static_cast<WPARAM>(uCtrlID), reinterpret_cast<LPARAM>(&nmii));
		iRet = static_cast<int>(nmii.lParam);
	}
	else {
		const auto ID = MapIndexToID(iItem);
		if (const auto it = m_umapCellIcon.find(ID); it != m_umapCellIcon.end())
			if (const auto itInner = it->second.find(iSubItem); itInner != it->second.end())
				iRet = itInner->second;
	}

	return iRet;
}

void CListEx::InitHeader()
{
	GetHeaderCtrl().SubclassDlgItem(0, this);
}

void CListEx::MeasureItem(LPMEASUREITEMSTRUCT lpMIS)
{
	//Set row height according to current font's height.
	TEXTMETRICW tm;
	auto pDC = GetDC();
	pDC->SelectObject(m_fontList);
	GetTextMetricsW(pDC->m_hDC, &tm);
	ReleaseDC(pDC);
	lpMIS->itemHeight = tm.tmHeight + tm.tmExternalLeading + 1;
}

BOOL CListEx::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (nFlags == MK_CONTROL) {
		FontSizeIncDec(zDelta > 0);
		return TRUE;
	}

	GetHeaderCtrl().RedrawWindow();

	return CMFCListCtrl::OnMouseWheel(nFlags, zDelta, pt);
}

void CListEx::OnMouseMove(UINT /*nFlags*/, CPoint pt)
{
	LVHITTESTINFO hi { pt };
	SubItemHitTest(&hi);

	bool fLink { false }; //Cursor at link's rect area.
	auto vecText = ParseItemText(hi.iItem, hi.iSubItem);
	if (const auto iterFind = std::find_if(vecText.begin(), vecText.end(), [&](SITEMDATA& iter) { return iter.fLink && iter.rect.PtInRect(pt); }); iterFind != vecText.end()) {
		fLink = true;
		if (m_fLinkTooltip && !m_fLDownAtLink && m_rcLinkCurr != iterFind->rect) {
			TtLinkHide();
			m_rcLinkCurr = iterFind->rect;
			m_stCurrLink.iItem = hi.iItem;
			m_stCurrLink.iSubItem = hi.iSubItem;
			m_wstrTtText = iterFind->fTitle ? iterFind->wstrTitle : iterFind->wstrLink;
			m_stTInfoLink.lpszText = m_wstrTtText.data();

			SetTimer(ID_TIMER_TT_LINK_ACTIVATE, 400, nullptr); //Activate (show) tooltip after delay.
		}
	}
	SetCursor(fLink ? m_cursorHand : m_cursorDefault);

	//Link's tooltip area is under cursor.
	if (fLink) {
		//If there is cell's tooltip atm hide it.
		if (m_fTtCellShown)
			TtCellHide();

		return; //Do not process further, cursor is on the link's rect.
	}

	m_fLDownAtLink = false;

	//If there was link's tool-tip shown, hide it.
	if (m_fTtLinkShown)
		TtLinkHide();

	if (auto opt = HasTooltip(hi.iItem, hi.iSubItem); opt) {
		//Check if cursor is still in the same cell's rect. If so - just leave.
		if (m_stCurrCell.iItem != hi.iItem || m_stCurrCell.iSubItem != hi.iSubItem) {
			m_fTtCellShown = true;
			m_stCurrCell.iItem = hi.iItem;
			m_stCurrCell.iSubItem = hi.iSubItem;
			m_stTInfoCell.lpszText = (*opt)->wstrText.data();

			ClientToScreen(&pt);
			m_stWndTtCell.SendMessageW(TTM_TRACKPOSITION, 0, static_cast<LPARAM>MAKELONG(pt.x, pt.y));
			m_stWndTtCell.SendMessageW(TTM_SETTITLE, static_cast<WPARAM>(TTI_NONE), reinterpret_cast<LPARAM>((*opt)->wstrCaption.data()));
			m_stWndTtCell.SendMessageW(TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(&m_stTInfoCell));
			m_stWndTtCell.SendMessageW(TTM_TRACKACTIVATE, static_cast<WPARAM>(TRUE), reinterpret_cast<LPARAM>(&m_stTInfoCell));

			//Timer to check whether mouse left subitem's rect.
			SetTimer(ID_TIMER_TT_CELL_CHECK, 200, nullptr);
		}
	}
	else {
		if (m_fTtCellShown)
			TtCellHide();
		else {
			m_stCurrCell.iItem = -1;
			m_stCurrCell.iSubItem = -1;
		}
	}
}

void CListEx::OnLButtonDown(UINT nFlags, CPoint pt)
{
	bool fLinkDown { false };
	LVHITTESTINFO hi { pt };
	if (SubItemHitTest(&hi) != -1) {
		auto vecText = ParseItemText(hi.iItem, hi.iSubItem);
		if (const auto iterFind = std::find_if(vecText.begin(), vecText.end(), [&](SITEMDATA& iter) { return iter.fLink && iter.rect.PtInRect(pt); }); iterFind != vecText.end()) {
			m_fLDownAtLink = true;
			m_rcLinkCurr = iterFind->rect;
			fLinkDown = true;
		}
	}

	if (!fLinkDown)
		CMFCListCtrl::OnLButtonDown(nFlags, pt);
}

void CListEx::OnLButtonUp(UINT nFlags, CPoint pt)
{
	bool fLinkUp { false };
	if (m_fLDownAtLink) {
		LVHITTESTINFO hi { };
		hi.pt = pt;
		ListView_SubItemHitTest(m_hWnd, &hi);
		if (hi.iSubItem == -1 || hi.iItem == -1) {
			m_fLDownAtLink = false;
			return;
		}

		auto vecText = ParseItemText(hi.iItem, hi.iSubItem);
		if (const auto iterFind = std::find_if(vecText.begin(), vecText.end(), [&](SITEMDATA& iter) { return iter.fLink && iter.rect == m_rcLinkCurr; }); iterFind != vecText.end()) {
			m_rcLinkCurr.SetRectEmpty();
			fLinkUp = true;
			const auto uCtrlId = static_cast<UINT>(GetDlgCtrlID());
			NMITEMACTIVATE nmii { { m_hWnd, uCtrlId, LISTEX_MSG_LINKCLICK } };
			nmii.iItem = hi.iItem;
			nmii.iSubItem = hi.iSubItem;
			nmii.ptAction = pt;
			nmii.lParam = reinterpret_cast<LPARAM>(iterFind->wstrLink.data());
			GetParent()->SendMessageW(WM_NOTIFY, static_cast<WPARAM>(uCtrlId), reinterpret_cast<LPARAM>(&nmii));
		}
	}

	m_fLDownAtLink = false;
	if (!fLinkUp)
		CMFCListCtrl::OnLButtonUp(nFlags, pt);
}

void CListEx::OnRButtonDown(UINT nFlags, CPoint pt)
{
	CMFCListCtrl::OnRButtonDown(nFlags, pt);
}

void CListEx::OnTimer(UINT_PTR nIDEvent)
{
	CPoint ptScreen;
	GetCursorPos(&ptScreen);
	CPoint ptClient = ptScreen;
	ScreenToClient(&ptClient);
	LVHITTESTINFO hitInfo { };
	hitInfo.pt = ptClient;
	ListView_SubItemHitTest(m_hWnd, &hitInfo);

	//Checking if mouse left list's subitem rect,
	//if so — hiding tooltip and killing timer.
	switch (nIDEvent) {
	case ID_TIMER_TT_CELL_CHECK:
		//If cursor is still hovers subitem then do nothing.
		if (m_stCurrCell.iItem != hitInfo.iItem || m_stCurrCell.iSubItem != hitInfo.iSubItem)
			TtCellHide();
		break;
	case ID_TIMER_TT_LINK_CHECK:
		//If cursor has left link subitem's rect.
		if (m_stCurrLink.iItem != hitInfo.iItem || m_stCurrLink.iSubItem != hitInfo.iSubItem)
			TtLinkHide();
		break;
	case ID_TIMER_TT_LINK_ACTIVATE:
		if (m_rcLinkCurr.PtInRect(ptClient)) {
			m_fTtLinkShown = true;

			m_stWndTtLink.SendMessageW(TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&m_stTInfoLink));
			m_stWndTtLink.SendMessageW(TTM_TRACKPOSITION, 0, static_cast<LPARAM>MAKELONG(ptScreen.x + 3, ptScreen.y - 20));
			m_stWndTtLink.SendMessageW(TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(&m_stTInfoLink));
			m_stWndTtLink.SendMessageW(TTM_TRACKACTIVATE, static_cast<WPARAM>(TRUE), reinterpret_cast<LPARAM>(&m_stTInfoLink));

			//Timer to check whether mouse left link subitems's rect.
			SetTimer(ID_TIMER_TT_LINK_CHECK, 200, nullptr);
		}
		else
			m_rcLinkCurr.SetRectEmpty();

		KillTimer(ID_TIMER_TT_LINK_ACTIVATE);
		break;
	default:
		break;
	}
}

BOOL CListEx::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	return CMFCListCtrl::OnSetCursor(pWnd, nHitTest, message);
}

void CListEx::OnKillFocus(CWnd* /*pNewWnd*/)
{
}

BOOL CListEx::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

void CListEx::OnPaint()
{
	CPaintDC dc(this);

	CRect rcClient;
	CRect rcHdr;
	GetClientRect(&rcClient);
	GetHeaderCtrl().GetClientRect(rcHdr);
	rcClient.top += rcHdr.Height();
	if (rcClient.IsRectEmpty())
		return;

	CMemDC memDC(dc, rcClient); //To avoid flickering drawing to CMemDC, excluding list header area (rcHdr).
	CDC& rDC = memDC.GetDC();
	rDC.GetClipBox(&rcClient);
	rDC.FillSolidRect(rcClient, m_stColors.clrNWABk);

	DefWindowProcW(WM_PAINT, reinterpret_cast<WPARAM>(rDC.m_hDC), static_cast<LPARAM>(0));
}

void CListEx::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (m_fVirtual && m_fHighLatency) {
		if (nSBCode != SB_THUMBTRACK) {
			//If there was SB_THUMBTRACK message previously, calculate the scroll amount (up/down)
			//by multiplying item's row height by difference between current (top) and nPos row.
			//Scroll may be negative therefore.
			if (m_fHLFlag) {
				CRect rc;
				GetItemRect(m_uHLItem, rc, LVIR_LABEL);
				const CSize size(0, (m_uHLItem - GetTopIndex()) * rc.Height());
				Scroll(size);
				m_fHLFlag = false;
			}
			TtRowShow(false, 0);
			CMFCListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
		}
		else {
			m_fHLFlag = true;
			SCROLLINFO si { };
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_ALL;
			GetScrollInfo(SB_VERT, &si);
			m_uHLItem = si.nTrackPos; //si.nTrackPos is in fact a row number.
			TtRowShow(true, m_uHLItem);
		}
	}
	else
		CMFCListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CListEx::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	GetHeaderCtrl().RedrawWindow();

	CMFCListCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
}

BOOL CListEx::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	if (!m_fCreated)
		return FALSE;

	//HDN_ITEMCLICK messages should be handled here first, to set m_fSortAscending 
	//and m_iSortColumn. And only then this message goes further, to parent window,
	//in form of HDN_ITEMCLICK and LVN_COLUMNCLICK.
	//If we execute this code in LVN_COLUMNCLICK handler, it will be handled
	//only AFTER the parent window handles LVN_COLUMNCLICK.
	//So briefly, ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, &CListEx::OnLvnColumnClick) fires up
	//only AFTER LVN_COLUMNCLICK sent to the parent.
	const auto* const pNMLV = reinterpret_cast<LPNMHEADERW>(lParam);
	if (m_fSortable && (pNMLV->hdr.code == HDN_ITEMCLICKW || pNMLV->hdr.code == HDN_ITEMCLICKA)) {
		if (IsColumnSortable(pNMLV->iItem)) {
			m_fSortAscending = pNMLV->iItem == m_iSortColumn ? !m_fSortAscending : true;
			GetHeaderCtrl().SetSortArrow(pNMLV->iItem, m_fSortAscending);
			m_iSortColumn = pNMLV->iItem;
		}
		else
			m_iSortColumn = -1;

		if (!m_fVirtual)
			return SortItemsEx(m_pfnCompare ? m_pfnCompare : DefCompareFunc, reinterpret_cast<DWORD_PTR>(this));
	}

	return CMFCListCtrl::OnNotify(wParam, lParam, pResult);
}

void CListEx::OnLvnColumnClick(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	/*******************************************************************************
	* Just an empty handler.
	* Without it all works fine, but assert triggers in Debug mode, when clicking
	* on header, if list is in Virtual mode (LVS_OWNERDATA).
	* ASSERT((GetStyle() & LVS_OWNERDATA)==0)
	*******************************************************************************/
}

void CListEx::OnHdnBegindrag(NMHDR* pNMHDR, LRESULT* pResult)
{
	const auto phdr = reinterpret_cast<LPNMHEADERW>(pNMHDR);
	if (GetHeaderCtrl().IsColumnHidden(phdr->iItem))
		*pResult = TRUE;
}

void CListEx::OnHdnBegintrack(NMHDR* pNMHDR, LRESULT* pResult)
{
	const auto phdr = reinterpret_cast<LPNMHEADERW>(pNMHDR);
	if (GetHeaderCtrl().IsColumnHidden(phdr->iItem))
		*pResult = TRUE;
}

void CListEx::OnDestroy()
{
	CMFCListCtrl::OnDestroy();

	m_stWndTtCell.DestroyWindow();
	m_stWndTtLink.DestroyWindow();
	m_stWndTtRow.DestroyWindow();
	m_fontList.DeleteObject();
	m_fontListUnderline.DeleteObject();
	m_penGrid.DeleteObject();

	m_umapCellTt.clear();
	m_umapCellData.clear();
	m_umapCellColor.clear();
	m_umapCellIcon.clear();
	m_umapRowColor.clear();
	m_umapColumnSortMode.clear();
	m_umapColumnColor.clear();

	m_fCreated = false;
}

auto CListEx::ParseItemText(int iItem, int iSubitem)->std::vector<CListEx::SITEMDATA>
{
	std::vector<SITEMDATA> vecData { };
	auto CStringText = GetItemText(iItem, iSubitem);
	const std::wstring_view wstrText = CStringText.GetString();
	CRect rcTextOrig;  //Original rect of the subitem's text.
	GetSubItemRect(iItem, iSubitem, LVIR_LABEL, rcTextOrig);
	constexpr auto iIndentRc { 4 };
	if (iSubitem != 0) //Not needed for item itself (not subitem).
		rcTextOrig.left += iIndentRc;

	if (const auto iIndex = HasIcon(iItem, iSubitem); iIndex > -1) { //If cell has icon.
		IMAGEINFO stIMG;
		GetImageList(LVSIL_NORMAL)->GetImageInfo(iIndex, &stIMG);
		vecData.emplace_back(iIndex, rcTextOrig);
		rcTextOrig.left += (stIMG.rcImage.right - stIMG.rcImage.left) + iIndentRc; //Indent a rect for image width.
	}

	size_t nPosCurr { 0 }; //Current position in the parsed string.
	CRect rcTextCurr { };  //Current rect.

	while (nPosCurr != std::wstring_view::npos) {
		constexpr std::wstring_view wstrTagLink { L"<link=" };
		constexpr std::wstring_view wstrTagFirstClose { L">" };
		constexpr std::wstring_view wstrTagLast { L"</link>" };
		constexpr std::wstring_view wstrTagTitle { L"title=" };
		constexpr std::wstring_view wstrQuote { L"\"" };

		//Searching the string for a <link=...></link> pattern.
		if (const std::size_t nPosTagLink { wstrText.find(wstrTagLink, nPosCurr) }, //Start position of the opening tag "<link=".
			nPosLinkOpenQuote { wstrText.find(wstrQuote, nPosTagLink) }, //Position of the (link="<-) open quote.
			//Position of the (link=""<-) close quote.
			nPosLinkCloseQuote { wstrText.find(wstrQuote, nPosLinkOpenQuote + wstrQuote.size()) },
			//Start position of the opening tag's closing bracket ">".
			nPosTagFirstClose { wstrText.find(wstrTagFirstClose, nPosLinkCloseQuote + wstrQuote.size()) },
			//Start position of the enclosing tag "</link>".
			nPosTagLast { wstrText.find(wstrTagLast, nPosTagFirstClose + wstrTagFirstClose.size()) };
			nPosTagLink != std::wstring_view::npos && nPosLinkOpenQuote != std::wstring_view::npos
			&& nPosLinkCloseQuote != std::wstring_view::npos && nPosTagFirstClose != std::wstring_view::npos
			&& nPosTagLast != std::wstring_view::npos) {
			const auto pDC = GetDC();
			pDC->SelectObject(m_fontList);
			SIZE size;

			//Any text before found tag.
			if (nPosTagLink > nPosCurr) {
				auto wstrTextBefore = wstrText.substr(nPosCurr, nPosTagLink - nPosCurr);
				GetTextExtentPoint32W(pDC->m_hDC, wstrTextBefore.data(), static_cast<int>(wstrTextBefore.size()), &size);
				if (rcTextCurr.IsRectNull())
					rcTextCurr.SetRect(rcTextOrig.left, rcTextOrig.top, rcTextOrig.left + size.cx, rcTextOrig.bottom);
				else {
					rcTextCurr.left = rcTextCurr.right;
					rcTextCurr.right += size.cx;
				}
				vecData.emplace_back(wstrTextBefore, L"", L"", rcTextCurr);
			}

			//The clickable/linked text, that between <link=...>textFromHere</link> tags.
			auto wstrTextBetweenTags = wstrText.substr(nPosTagFirstClose + wstrTagFirstClose.size(),
				nPosTagLast - (nPosTagFirstClose + wstrTagFirstClose.size()));
			GetTextExtentPoint32W(pDC->m_hDC, wstrTextBetweenTags.data(), static_cast<int>(wstrTextBetweenTags.size()), &size);
			ReleaseDC(pDC);

			if (rcTextCurr.IsRectNull())
				rcTextCurr.SetRect(rcTextOrig.left, rcTextOrig.top, rcTextOrig.left + size.cx, rcTextOrig.bottom);
			else {
				rcTextCurr.left = rcTextCurr.right;
				rcTextCurr.right += size.cx;
			}

			//Link tag text (linkID) between quotes: <link="textFromHere">
			auto wstrTextLink = wstrText.substr(nPosLinkOpenQuote + wstrQuote.size(),
				nPosLinkCloseQuote - nPosLinkOpenQuote - wstrQuote.size());
			nPosCurr = nPosLinkCloseQuote + wstrQuote.size();

			//Searching for title "<link=...title="">" tag.
			bool fTitle { false };
			std::wstring_view wstrTextTitle { };

			if (const std::size_t nPosTagTitle { wstrText.find(wstrTagTitle, nPosCurr) }, //Position of the (title=) tag beginning.
				nPosTitleOpenQuote { wstrText.find(wstrQuote, nPosTagTitle) },  //Position of the (title="<-) opening quote.
				nPosTitleCloseQuote { wstrText.find(wstrQuote, nPosTitleOpenQuote + wstrQuote.size()) }; //Position of the (title=""<-) closing quote.
				nPosTagTitle != std::wstring_view::npos && nPosTitleOpenQuote != std::wstring_view::npos
				&& nPosTitleCloseQuote != std::wstring_view::npos) {
				//Title tag text between quotes: <...title="textFromHere">
				wstrTextTitle = wstrText.substr(nPosTitleOpenQuote + wstrQuote.size(),
					nPosTitleCloseQuote - nPosTitleOpenQuote - wstrQuote.size());
				fTitle = true;
			}

			vecData.emplace_back(wstrTextBetweenTags, wstrTextLink, wstrTextTitle, rcTextCurr, true, fTitle);
			nPosCurr = nPosTagLast + wstrTagLast.size();
		}
		else {
			auto wstrTextAfter = wstrText.substr(nPosCurr, wstrText.size() - nPosCurr);

			if (rcTextCurr.IsRectNull())
				rcTextCurr = rcTextOrig;
			else {
				auto pDC = GetDC();
				SIZE size;
				pDC->SelectObject(m_fontList);
				GetTextExtentPoint32W(pDC->m_hDC, wstrTextAfter.data(), static_cast<int>(wstrTextAfter.size()), &size);
				ReleaseDC(pDC);

				rcTextCurr.left = rcTextCurr.right;
				rcTextCurr.right += size.cx;
			}

			vecData.emplace_back(wstrTextAfter, L"", L"", rcTextCurr);
			nPosCurr = std::wstring_view::npos;
		}
	}

	return vecData;
}

void CListEx::RecalcMeasure()
{
	//To get WM_MEASUREITEM msg after changing the font.
	CRect rc;
	GetWindowRect(&rc);
	WINDOWPOS wp;
	wp.hwnd = m_hWnd;
	wp.cx = rc.Width();
	wp.cy = rc.Height();
	wp.flags = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER;
	SendMessageW(WM_WINDOWPOSCHANGED, 0, reinterpret_cast<LPARAM>(&wp));
}

void CListEx::SetFontSize(long lSize)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	//Prevent font size from being too small or too big.
	if (lSize < 4 || lSize > 64)
		return;

	LOGFONTW lf;
	m_fontList.GetLogFont(&lf);
	lf.lfHeight = -MulDiv(lSize, m_iLOGPIXELSY, 72);;
	m_fontList.DeleteObject();
	m_fontList.CreateFontIndirectW(&lf);
	lf.lfUnderline = 1;
	m_fontListUnderline.DeleteObject();
	m_fontListUnderline.CreateFontIndirectW(&lf);

	RecalcMeasure();
	Update(0);
}

void CListEx::TtLinkHide()
{
	m_fTtLinkShown = false;
	m_stWndTtLink.SendMessageW(TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&m_stTInfoLink));
	KillTimer(ID_TIMER_TT_LINK_CHECK);

	m_stCurrLink.iItem = -1;
	m_stCurrLink.iSubItem = -1;
	m_rcLinkCurr.SetRectEmpty();
}

void CListEx::TtCellHide()
{
	m_fTtCellShown = false;
	m_stCurrCell.iItem = -1;
	m_stCurrCell.iSubItem = -1;

	m_stWndTtCell.SendMessageW(TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&m_stTInfoCell));
	KillTimer(ID_TIMER_TT_CELL_CHECK);
}

void CListEx::TtRowShow(bool fShow, UINT uRow)
{
	if (fShow) {
		CPoint ptScreen;
		GetCursorPos(&ptScreen);

		wchar_t warrOffset[32] { L"Row: " };
		swprintf_s(&warrOffset[5], 24, L"%u", uRow);
		m_stToolInfoRow.lpszText = warrOffset;
		m_stWndTtRow.SendMessageW(TTM_TRACKPOSITION, 0, static_cast<LPARAM>(MAKELONG(ptScreen.x - 5, ptScreen.y - 20)));
		m_stWndTtRow.SendMessageW(TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(&m_stToolInfoRow));
	}
	m_stWndTtRow.SendMessageW(TTM_TRACKACTIVATE, static_cast<WPARAM>(fShow), reinterpret_cast<LPARAM>(&m_stToolInfoRow));
}