module;
/********************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/      *
* Official git repository: https://github.com/jovibor/ListEx/       *
* This code is available under the "MIT License".                   *
********************************************************************/
#include <SDKDDKVer.h>
#include <afxcontrolbars.h>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
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
	* LISTEXCOLORS - All ListEx colors.                                               *
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
		CWnd*            pParent { };             //Parent window.
		PCLISTEXCOLORS   pColors { };             //ListEx colors.
		const LOGFONTW*  pListLogFont { };        //ListEx font.
		const LOGFONTW*  pHdrLogFont { };         //Header font.
		CRect            rect;                    //Initial rect.
		UINT             uID { };                 //ListEx control ID.
		DWORD            dwStyle { };             //ListEx window styles.
		DWORD            dwExStyle { };           //Extended window styles.
		DWORD            dwTTStyleCell { };       //Cell's tooltip Window styles.
		DWORD            dwTTStyleLink { };       //Link's tooltip Window styles.
		DWORD            dwTTShowDelay { };       //Tooltip delay before showing, in milliseconds.
		DWORD            dwTTShowTime { 5000 };   //Tooltip show up time, in milliseconds.
		DWORD            dwListGridWidth { 1 };   //Width of the list grid.
		DWORD            dwHdrHeight { };         //Header height.
		POINT            ptTTOffset { };          //Tooltip offset from cursor. Doesn't work for TTS_BALLOON.
		bool             fDialogCtrl { false };   //If it's a list within dialog.
		bool             fSortable { false };     //Is list sortable, by clicking on the header column?
		bool             fLinkUnderline { true }; //Links are displayed underlined or not.
		bool             fLinkTooltip { true };   //Show links' toolips or not.
		bool             fHighLatency { false };  //Do not redraw until scroll thumb is released.
	};

	/********************************************
	* LISTEXHDRICON - Icon for header column.   *
	********************************************/
	struct LISTEXHDRICON {
		POINT pt { };              //Coords of the icon's top-left corner in the header item's rect.
		int   iIndex { };          //Icon index in the header's image list.
		bool  fClickable { true }; //Is icon sending LISTEX_MSG_HDRICONCLICK message when clicked.
	};

	/********************************************
	* IListEx pure virtual base class.          *
	********************************************/
	class IListEx : public CMFCListCtrl {
	public:
		virtual bool Create(const LISTEXCREATE& lcs) = 0;
		virtual void CreateDialogCtrl(UINT uCtrlID, CWnd* pParent) = 0;
		virtual BOOL DeleteAllItems() = 0;
		virtual BOOL DeleteColumn(int nCol) = 0;
		virtual BOOL DeleteItem(int nItem) = 0;
		virtual void Destroy() = 0;
		[[nodiscard]] virtual auto GetCellData(int iItem, int iSubitem)const->ULONGLONG = 0;
		[[nodiscard]] virtual auto GetColors()const->const LISTEXCOLORS & = 0;
		[[nodiscard]] virtual auto GetColumnSortMode(int iColumn)const->EListExSortMode = 0;
		[[nodiscard]] virtual int GetSortColumn()const = 0;
		[[nodiscard]] virtual bool GetSortAscending()const = 0;
		virtual void HideColumn(int iIndex, bool fHide) = 0;
		virtual int InsertColumn(int nCol, const LVCOLUMNW* pColumn, int iDataAlign = LVCFMT_LEFT, bool fEditable = false) = 0;
		virtual int InsertColumn(int nCol, LPCWSTR pwszName, int nFormat = LVCFMT_LEFT, int nWidth = -1,
			int nSubItem = -1, int iDataAlign = LVCFMT_LEFT, bool fEditable = false) = 0;
		[[nodiscard]] virtual bool IsCreated()const = 0;
		[[nodiscard]] virtual bool IsColumnSortable(int iColumn) = 0;
		virtual void ResetSort() = 0; //Reset all the sort by any column to its default state.
		virtual void SetCellColor(int iItem, int iSubitem, COLORREF clrBk, COLORREF clrText = -1) = 0;
		virtual void SetCellData(int iItem, int iSubitem, ULONGLONG ullData) = 0;
		virtual void SetCellIcon(int iItem, int iSubitem, int iIndex) = 0;
		virtual void SetCellTooltip(int iItem, int iSubitem, std::wstring_view wsvTooltip, std::wstring_view wsvCaption = L"") = 0;
		virtual void SetColors(const LISTEXCOLORS& lcs) = 0;
		virtual void SetColumnColor(int iColumn, COLORREF clrBk, COLORREF clrText = -1) = 0;
		virtual void SetColumnEditable(int iColumn, bool fEditable) = 0;
		virtual void SetColumnSortMode(int iColumn, bool fSortable, EListExSortMode eSortMode = { }) = 0;
		virtual void SetFont(const LOGFONTW* pLogFont) = 0;
		virtual void SetHdrColumnColor(int iColumn, COLORREF clrBk, COLORREF clrText = -1) = 0;
		virtual void SetHdrColumnIcon(int iColumn, const LISTEXHDRICON& stIcon) = 0; //Icon for a given column.
		virtual void SetHdrFont(const LOGFONTW* pLogFont) = 0;
		virtual void SetHdrHeight(DWORD dwHeight) = 0;
		virtual void SetHdrImageList(CImageList* pList) = 0;
		virtual void SetRowColor(DWORD dwRow, COLORREF clrBk, COLORREF clrText = -1) = 0;
		virtual void SetSortable(bool fSortable, PFNLVCOMPARE pfnCompare = nullptr,
			EListExSortMode eSortMode = EListExSortMode::SORT_LEX) = 0;
	};

	/***************************************************************************************
	* Factory function CreateListEx. Returns IListExPtr, unique_ptr with custom deleter.   *
	***************************************************************************************/
	struct IIListExDeleter { void operator()(IListEx* p)const { p->Destroy(); } };
	using IListExPtr = std::unique_ptr<IListEx, IIListExDeleter>;
	[[nodiscard]] IListExPtr CreateListEx();

	/****************************************************************************
	* WM_NOTIFY codes (NMHDR.code values)										*
	****************************************************************************/

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

namespace HEXCTRL::LISTEX::INTERNAL {
	class CListExHdr final : public CMFCHeaderCtrl {
	public:
		void DeleteColumn(int iIndex);
		[[nodiscard]] UINT GetHiddenCount()const;
		[[nodiscard]] int GetColumnDataAlign(int iIndex)const;
		void HideColumn(int iIndex, bool fHide);
		[[nodiscard]] bool IsColumnHidden(int iIndex)const; //Column index.
		[[nodiscard]] bool IsColumnSortable(int iIndex)const;
		[[nodiscard]] bool IsColumnEditable(int iIndex)const;
		void SetHeight(DWORD dwHeight);
		void SetFont(const LOGFONTW* pLogFontNew);
		void SetColor(const LISTEXCOLORS& lcs);
		void SetColumnColor(int iColumn, COLORREF clrBk, COLORREF clrText);
		void SetColumnDataAlign(int iColumn, int iAlign);
		void SetColumnIcon(int iColumn, const LISTEXHDRICON& stIcon);
		void SetColumnSortable(int iColumn, bool fSortable);
		void SetColumnEditable(int iColumn, bool fEditable);
		void SetSortable(bool fSortable);
		void SetSortArrow(int iColumn, bool fAscending);
	private:
		struct HDRICON;
		struct HIDDEN;
		[[nodiscard]] UINT ColumnIndexToID(int iIndex)const; //Returns unique column ID. Must be > 0.
		[[nodiscard]] int ColumnIDToIndex(UINT uID)const;
		[[nodiscard]] auto GetHdrColor(UINT ID)const->PLISTEXCOLOR;
		[[nodiscard]] auto GetHdrIcon(UINT ID) -> HDRICON*;
		[[nodiscard]] bool IsEditable(UINT ID)const;
		[[nodiscard]] auto IsHidden(UINT ID)const->const HIDDEN*; //Internal ColumnID.
		[[nodiscard]] bool IsSortable(UINT ID)const;
		afx_msg void OnDestroy();
		afx_msg void OnDrawItem(CDC* pDC, int iItem, CRect rcOrig, BOOL bIsPressed, BOOL bIsHighlighted)override;
		afx_msg LRESULT OnLayout(WPARAM wParam, LPARAM lParam);
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
		afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
		afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
		DECLARE_MESSAGE_MAP();
	private:
		CFont m_fontHdr;
		COLORREF m_clrBkNWA { }; //Bk of non working area.
		COLORREF m_clrText { };
		COLORREF m_clrBk { };
		COLORREF m_clrHglInactive { };
		COLORREF m_clrHglActive { };
		DWORD m_dwHeaderHeight { 19 }; //Standard (default) height.
		std::unordered_map<UINT, LISTEXCOLOR> m_umapColors; //Colors for columns.
		std::unordered_map<UINT, HDRICON> m_umapIcons;   //Icons for columns.
		std::unordered_map<UINT, HIDDEN> m_umapHidden;   //Hidden columns.
		std::unordered_map<UINT, bool> m_umapSortable;   //Is column sortable.
		std::unordered_map<UINT, bool> m_umapEditable;   //Columns that can be in-place edited.
		std::unordered_map<UINT, int> m_umapDataAlign;   //Column's data align mode.
		UINT m_uSortColumn { 0 };   //ColumnID to draw sorting triangle at. 0 is to avoid triangle before first clicking.
		bool m_fSortable { false }; //List-is-sortable global flog. Need to draw sortable triangle or not?
		bool m_fSortAscending { };  //Sorting type.
	};

	//Header column icons.
	struct CListExHdr::HDRICON {
		LISTEXHDRICON stIcon;               //Icon data struct.
		bool          fLMPressed { false }; //Left mouse button pressed atm.
	};

	//Hidden columns.
	struct CListExHdr::HIDDEN {
		int iPrevPos { };
		int iPrevWidth { };
	};
}

using namespace HEXCTRL::LISTEX::INTERNAL;

BEGIN_MESSAGE_MAP(CListExHdr, CMFCHeaderCtrl)
	ON_MESSAGE(HDM_LAYOUT, &CListExHdr::OnLayout)
	ON_WM_HSCROLL()
	ON_WM_DESTROY()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()

void CListExHdr::DeleteColumn(int iIndex)
{
	if (const auto ID = ColumnIndexToID(iIndex); ID > 0) {
		m_umapColors.erase(ID);
		m_umapIcons.erase(ID);
		m_umapHidden.erase(ID);
		m_umapSortable.erase(ID);
		m_umapEditable.erase(ID);
	}
}

UINT CListExHdr::GetHiddenCount()const
{
	return static_cast<UINT>(m_umapHidden.size());
}

int CListExHdr::GetColumnDataAlign(int iIndex)const
{
	if (const auto iter = m_umapDataAlign.find(ColumnIndexToID(iIndex));
		iter != m_umapDataAlign.end()) {
		return iter->second;
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
	ListView_GetColumnOrderArray(GetParent()->m_hWnd, iColumnsCount, vecInt.data());
	HDITEMW hdi { HDI_WIDTH };
	GetItem(iIndex, &hdi);

	if (fHide) { //Hide column.
		m_umapHidden[ID].iPrevWidth = hdi.cxy;
		if (const auto iter = std::find(vecInt.begin(), vecInt.end(), iIndex); iter != vecInt.end()) {
			m_umapHidden[ID].iPrevPos = static_cast<int>(iter - vecInt.begin());
			std::rotate(iter, iter + 1, vecInt.end()); //Moving hiding column to the end of the column array.
		}

		ListView_SetColumnOrderArray(GetParent()->m_hWnd, iColumnsCount, vecInt.data());
		hdi.cxy = 0;
		SetItem(iIndex, &hdi);
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

		ListView_SetColumnOrderArray(GetParent()->m_hWnd, iColumnsCount, vecInt.data());
		hdi.cxy = pHidden->iPrevWidth;
		SetItem(iIndex, &hdi);
		m_umapHidden.erase(ID);
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

void CListExHdr::SetFont(const LOGFONTW* pLogFont)
{
	if (!pLogFont) {
		return;
	}

	m_fontHdr.DeleteObject();
	m_fontHdr.CreateFontIndirectW(pLogFont);

	//If new font's height is higher than current height (m_dwHeaderHeight), we adjust current height as well.
	TEXTMETRICW tm;
	auto pDC = GetDC();
	pDC->SelectObject(m_fontHdr);
	pDC->GetTextMetricsW(&tm);
	ReleaseDC(pDC);
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

	m_umapColors[ID] = { .clrBk { clrBk }, .clrText { clrText } };
	RedrawWindow();
}

void CListExHdr::SetColumnDataAlign(int iColumn, int iAlign)
{
	const auto ID = ColumnIndexToID(iColumn);
	assert(ID > 0);
	if (ID == 0) {
		return;
	}

	m_umapDataAlign[ID] = iAlign;
}

void CListExHdr::SetColumnIcon(int iColumn, const LISTEXHDRICON& stIcon)
{
	const auto ID = ColumnIndexToID(iColumn);
	assert(ID > 0);
	if (ID == 0) {
		return;
	}

	if (stIcon.iIndex == -1) { //If column already has icon.
		m_umapIcons.erase(ID);
	}
	else {
		HDRICON stHdrIcon;
		stHdrIcon.stIcon = stIcon;
		m_umapIcons[ID] = stHdrIcon;
	}
	RedrawWindow();
}

void CListExHdr::SetColumnSortable(int iColumn, bool fSortable)
{
	const auto ID = ColumnIndexToID(iColumn);
	assert(ID > 0);
	if (ID == 0) {
		return;
	}

	m_umapSortable[ID] = fSortable;
}

void CListExHdr::SetColumnEditable(int iColumn, bool fEditable)
{
	const auto ID = ColumnIndexToID(iColumn);
	assert(ID > 0);
	if (ID == 0) {
		return;
	}

	m_umapEditable[ID] = fEditable;
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


//CListExHdr private methods:

UINT CListExHdr::ColumnIndexToID(int iIndex)const
{
	//Each column has unique internal identifier in HDITEMW::lParam.
	HDITEMW hdi { HDI_LPARAM };
	const auto ret = GetItem(iIndex, &hdi);
	assert(ret == TRUE); //There is no such column index if FALSE.

	return ret ? static_cast<UINT>(hdi.lParam) : 0;
}

int CListExHdr::ColumnIDToIndex(UINT uID)const
{
	for (int iterColumns = 0; iterColumns < GetItemCount(); ++iterColumns) {
		HDITEMW hdi { HDI_LPARAM };
		GetItem(iterColumns, &hdi);
		if (static_cast<UINT>(hdi.lParam) == uID) {
			return iterColumns;
		}
	}

	return -1;
}

auto CListExHdr::GetHdrColor(UINT ID)const->PLISTEXCOLOR
{
	if (const auto it = m_umapColors.find(ID); it != m_umapColors.end()) {
		return &it->second;
	}

	return nullptr;
}

auto CListExHdr::GetHdrIcon(UINT ID)->CListExHdr::HDRICON*
{
	if (GetImageList() == nullptr) {
		return nullptr;
	}

	if (const auto it = m_umapIcons.find(ID); it != m_umapIcons.end()) {
		return &it->second;
	}

	return nullptr;
}

auto CListExHdr::IsHidden(UINT ID)const->const HIDDEN*
{
	const auto iter = m_umapHidden.find(ID);
	return iter != m_umapHidden.end() ? &iter->second : nullptr;
}

bool CListExHdr::IsSortable(UINT ID)const
{
	const auto iter = m_umapSortable.find(ID); //It's sortable unless found explicitly as false.
	return iter == m_umapSortable.end() || iter->second;
}

bool CListExHdr::IsEditable(UINT ID)const
{
	const auto iter = m_umapEditable.find(ID); //It's editable only if found explicitly as true.
	return iter != m_umapEditable.end() && iter->second;
}

void CListExHdr::OnDestroy()
{
	CMFCHeaderCtrl::OnDestroy();

	m_umapColors.clear();
	m_umapIcons.clear();
	m_umapHidden.clear();
	m_umapSortable.clear();
	m_umapEditable.clear();
	m_umapDataAlign.clear();
}

void CListExHdr::OnDrawItem(CDC* pDC, int iItem, const CRect rcOrig, BOOL bIsPressed, BOOL bIsHighlighted)
{
	//Non working area after last column. Or if column is resized to zero width.
	if (iItem < 0 || rcOrig.IsRectEmpty()) {
		pDC->FillSolidRect(rcOrig, m_clrBkNWA);
		return;
	}

	CMemDC memDC(*pDC, rcOrig);
	auto& rDC = memDC.GetDC();
	const auto ID = ColumnIndexToID(iItem);
	const auto pClr = GetHdrColor(ID);
	const auto clrText { pClr != nullptr ? pClr->clrText : m_clrText };
	const auto clrBk { bIsHighlighted ? (bIsPressed ? m_clrHglActive : m_clrHglInactive)
		: (pClr != nullptr ? pClr->clrBk : m_clrBk) };

	rDC.FillSolidRect(rcOrig, clrBk);
	rDC.SetTextColor(clrText);
	rDC.SelectObject(m_fontHdr);

	//Set item's text buffer first char to zero, then getting item's text and Draw it.
	wchar_t warrHdrText[MAX_PATH];
	warrHdrText[0] = L'\0';
	HDITEMW hdItem { HDI_FORMAT | HDI_TEXT, 0, warrHdrText, nullptr, MAX_PATH };
	GetItem(iItem, &hdItem);

	//Draw icon for column, if any.
	long lIndentTextLeft { 4 }; //Left text indent.
	if (const auto pIcon = GetHdrIcon(ID); pIcon != nullptr) { //If column has an icon.
		const auto pImgList = GetImageList(LVSIL_NORMAL);
		int iCX { };
		int iCY { };
		ImageList_GetIconSize(pImgList->m_hImageList, &iCX, &iCY); //Icon dimensions.
		pImgList->DrawEx(&rDC, pIcon->stIcon.iIndex, rcOrig.TopLeft() + pIcon->stIcon.pt, { }, CLR_NONE, CLR_NONE, ILD_NORMAL);
		lIndentTextLeft += pIcon->stIcon.pt.x + iCX;
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

	constexpr long lIndentTextRight { 4 };
	CRect rcText { rcOrig.left + lIndentTextLeft, rcOrig.top, rcOrig.right - lIndentTextRight, rcOrig.bottom };
	if (StrStrW(warrHdrText, L"\n") != nullptr) {
		//If it's multiline text, first — calculate rect for the text,
		//with DT_CALCRECT flag (not drawing anything),
		//and then calculate rect for final vertical text alignment.
		CRect rcCalcText;
		rDC.DrawTextW(warrHdrText, rcCalcText, uFormat | DT_CALCRECT);
		rcText.top = rcText.Height() / 2 - rcCalcText.Height() / 2;
	}
	else {
		uFormat |= DT_VCENTER | DT_SINGLELINE;
	}

	//Draw Header Text.
	rDC.DrawTextW(warrHdrText, rcText, uFormat);

	//Draw sortable triangle (arrow).
	if (m_fSortable && IsSortable(ID) && ID == m_uSortColumn) {
		static const CPen penArrow(PS_SOLID, 1, RGB(90, 90, 90));
		static const CBrush brFill(GetSysColor(COLOR_3DFACE));
		rDC.SelectObject(penArrow);
		rDC.SelectObject(brFill);
		if (m_fSortAscending) { //Draw the UP arrow.
			const POINT arrPt[] { { rcOrig.right - 10, 3 },
				{ rcOrig.right - 15, 8 }, { rcOrig.right - 5, 8 } };
			rDC.Polygon(arrPt, 3);
		}
		else { //Draw the DOWN arrow.
			const POINT arrPt[] { { rcOrig.right - 10, 8 },
				{ rcOrig.right - 15, 3 }, { rcOrig.right - 5, 3 } };
			rDC.Polygon(arrPt, 3);
		}
	}

	static const CPen penGrid(PS_SOLID, 2, GetSysColor(COLOR_3DFACE));
	rDC.SelectObject(penGrid);
	rDC.MoveTo(rcOrig.TopLeft());
	rDC.LineTo(rcOrig.left, rcOrig.bottom);
	if (iItem == GetItemCount() - 1) { //Last item.
		rDC.MoveTo(rcOrig.right, rcOrig.top);
		rDC.LineTo(rcOrig.BottomRight());
	}
}

auto CListExHdr::OnLayout(WPARAM /*wParam*/, LPARAM lParam)->LRESULT
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
	if (HitTest(&ht); ht.iItem >= 0) {
		const auto ID = ColumnIndexToID(ht.iItem);
		if (const auto pIcon = GetHdrIcon(ID); pIcon != nullptr
			&& pIcon->stIcon.fClickable && IsHidden(ID) == nullptr) {
			int iCX { };
			int iCY { };
			ImageList_GetIconSize(GetImageList()->m_hImageList, &iCX, &iCY);
			CRect rcColumn;
			GetItemRect(ht.iItem, rcColumn);

			if (const CRect rcIcon(rcColumn.TopLeft() + pIcon->stIcon.pt, SIZE { iCX, iCY }); rcIcon.PtInRect(point)) {
				pIcon->fLMPressed = true;
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
	if (HitTest(&ht); ht.iItem >= 0) {
		for (auto& iter : m_umapIcons) {
			if (!iter.second.fLMPressed) {
				continue;
			}

			int iCX { };
			int iCY { };
			ImageList_GetIconSize(GetImageList()->m_hImageList, &iCX, &iCY);
			CRect rcColumn;
			GetItemRect(ht.iItem, rcColumn);
			const CRect rcIcon(rcColumn.TopLeft() + iter.second.stIcon.pt, SIZE { iCX, iCY });

			if (rcIcon.PtInRect(point)) {
				for (auto& iterData : m_umapIcons) {
					iterData.second.fLMPressed = false; //Remove fLMPressed flag from all columns.
				}

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
	if (pParent == nullptr) { //List control pointer.
		return;
	}

	const auto uCtrlId = static_cast<UINT>(pParent->GetDlgCtrlID());
	HDHITTESTINFO ht { };
	ht.pt = point;
	HitTest(&ht);
	NMHEADERW hdr { { pParent->m_hWnd, uCtrlId, LISTEX_MSG_HDRRBTNDOWN } };
	hdr.iItem = ht.iItem;
	pParent->GetParent()->SendMessageW(WM_NOTIFY, static_cast<WPARAM>(uCtrlId), reinterpret_cast<LPARAM>(&hdr));
}

void CListExHdr::OnRButtonUp(UINT /*nFlags*/, CPoint point)
{
	const auto pParent = GetParent();
	if (pParent == nullptr) { //List control pointer.
		return;
	}

	const auto uCtrlId = static_cast<UINT>(pParent->GetDlgCtrlID());
	HDHITTESTINFO ht { };
	ht.pt = point;
	HitTest(&ht);
	NMHEADERW hdr { { pParent->m_hWnd, uCtrlId, LISTEX_MSG_HDRRBTNUP } };
	hdr.iItem = ht.iItem;
	pParent->GetParent()->SendMessageW(WM_NOTIFY, static_cast<WPARAM>(uCtrlId), reinterpret_cast<LPARAM>(&hdr));
}


//CListEx.
namespace HEXCTRL::LISTEX::INTERNAL {
	class CListEx final : public IListEx {
	public:
		bool Create(const LISTEXCREATE& lcs)override;
		void CreateDialogCtrl(UINT uCtrlID, CWnd* pParent)override;
		BOOL DeleteAllItems()override;
		BOOL DeleteColumn(int iIndex)override;
		BOOL DeleteItem(int iItem)override;
		void Destroy()override;
		[[nodiscard]] auto GetCellData(int iItem, int iSubItem)const->ULONGLONG override;
		[[nodiscard]] auto GetColors()const->const LISTEXCOLORS & override;
		[[nodiscard]] auto GetColumnSortMode(int iColumn)const->EListExSortMode override;
		[[nodiscard]] int GetSortColumn()const override;
		[[nodiscard]] bool GetSortAscending()const override;
		void HideColumn(int iIndex, bool fHide)override;
		int InsertColumn(int nCol, const LVCOLUMNW* pColumn, int iDataAlign = LVCFMT_LEFT, bool fEditable = false)override;
		int InsertColumn(int nCol, LPCWSTR pwszName, int nFormat = LVCFMT_LEFT, int nWidth = -1,
			int nSubItem = -1, int iDataAlign = LVCFMT_LEFT, bool fEditable = false)override;
		[[nodiscard]] bool IsCreated()const override;
		[[nodiscard]] bool IsColumnSortable(int iColumn)override;
		void ResetSort()override; //Reset all the sort by any column to its default state.
		void SetCellColor(int iItem, int iSubItem, COLORREF clrBk, COLORREF clrText)override;
		void SetCellData(int iItem, int iSubItem, ULONGLONG ullData)override;
		void SetCellIcon(int iItem, int iSubItem, int iIndex)override; //Icon index in list's image list.
		void SetCellTooltip(int iItem, int iSubItem, std::wstring_view wsvTooltip, std::wstring_view wsvCaption)override;
		void SetColors(const LISTEXCOLORS& lcs)override;
		void SetColumnColor(int iColumn, COLORREF clrBk, COLORREF clrText)override;
		void SetColumnEditable(int iColumn, bool fEditable)override;
		void SetColumnSortMode(int iColumn, bool fSortable, EListExSortMode eSortMode = { })override;
		void SetFont(const LOGFONTW* pLogFont)override;
		void SetHdrColumnColor(int iColumn, COLORREF clrBk, COLORREF clrText = -1)override;
		void SetHdrColumnIcon(int iColumn, const LISTEXHDRICON& stIcon)override; //Icon for a given column.
		void SetHdrFont(const LOGFONTW* pLogFont)override;
		void SetHdrHeight(DWORD dwHeight)override;
		void SetHdrImageList(CImageList* pList)override;
		void SetRowColor(DWORD dwRow, COLORREF clrBk, COLORREF clrText)override;
		void SetSortable(bool fSortable, PFNLVCOMPARE pfnCompare, EListExSortMode eSortMode)override;
		static int CALLBACK DefCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	private:
		struct COLROWCLR;
		struct ITEMDATA;
		struct TOOLTIPS;
		void DrawItem(LPDRAWITEMSTRUCT pDIS)override;
		[[nodiscard]] long GetFontSize();
		[[nodiscard]] auto GetHeaderCtrl() -> CListExHdr & override { return m_stListHeader; }
		void FontSizeIncDec(bool fInc);
		void InitHeader()override;
		[[nodiscard]] auto GetCustomColor(int iItem, int iSubItem)const->std::optional<LISTEXCOLOR>;
		[[nodiscard]] auto GetTooltip(int iItem, int iSubItem)const->std::optional<LISTEXTTDATA>;
		[[nodiscard]] int GetIcon(int iItem, int iSubItem)const; //Does cell have an icon associated.
		afx_msg void MeasureItem(LPMEASUREITEMSTRUCT lpMIS);
		afx_msg void OnDestroy();
		void OnEditInPlaceEnterPressed();
		afx_msg void OnEditInPlaceKillFocus();
		afx_msg BOOL OnEraseBkgnd(CDC* pDC);
		afx_msg void OnHdnBegindrag(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnHdnBegintrack(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
		afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
		afx_msg void OnLButtonDown(UINT nFlags, CPoint pt);
		afx_msg void OnLButtonUp(UINT nFlags, CPoint pt);
		afx_msg void OnLvnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnMouseMove(UINT nFlags, CPoint pt);
		afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
		BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)override;
		afx_msg void OnNotifyEditInPlace(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnPaint();
		afx_msg void OnTimer(UINT_PTR nIDEvent);
		afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
		auto ParseItemData(int iItem, int iSubitem) -> std::vector<ITEMDATA>;
		void RecalcMeasure()const;
		void SetFontSize(long lSize);
		void TTCellShow(bool fShow, bool fTimer = false);
		void TTLinkShow(bool fShow, bool fTimer = false);
		void TTHLShow(bool fShow, UINT uRow); //Tooltips for high latency mode.
		static auto CALLBACK EditSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
			UINT_PTR uIdSubclass, DWORD_PTR dwRefData)->LRESULT;
		DECLARE_DYNAMIC(CListEx);
		DECLARE_MESSAGE_MAP();
	private:
		static constexpr ULONG_PTR m_uIDTCellTTActivate { 0x01 }; //Cell tool-tip activate-timer ID.
		static constexpr ULONG_PTR m_uIDTCellTTCheck { 0x02 };    //Cell tool-tip check-timer ID.
		static constexpr ULONG_PTR m_uIDTLinkTTActivate { 0x03 }; //Link tool-tip activate-timer ID.
		static constexpr ULONG_PTR m_uIDTLinkTTCheck { 0x04 };    //Link tool-tip check-timer ID.
		static constexpr auto m_uIDEditInPlace { 0x01U };         //In place edit-box ID.
		CListExHdr m_stListHeader;
		LISTEXCOLORS m_stColors { };
		CFont m_fontList;               //Default list font.
		CFont m_fontListUnderline;      //Underlined list font, for links.
		CPen m_penGrid;                 //Pen for list lines between cells.
		CWnd m_wndCellTT;               //Cells tool-tip window.
		CWnd m_wndLinkTT;               //Links tool-tip window.
		CWnd m_wndRowTT;                //Tooltip window for row in m_fHighLatency mode.
		TTTOOLINFOW m_ttiCell { };      //Cells tool-tip info struct.
		TTTOOLINFOW m_ttiLink { };      //Links tool-tip info struct.
		TTTOOLINFOW m_ttiHL { };        //High latency tooltip info struct.
		std::chrono::steady_clock::time_point m_tmTT; //Start time of the tooltip.
		std::wstring m_wstrTextTT;      //Tool-tip current text.
		std::wstring m_wstrCaptionTT;   //Tool-tip current caption.
		LVHITTESTINFO m_htiCurrCell { }; //Cells hit struct for tool-tip.
		LVHITTESTINFO m_htiCurrLink { }; //Links hit struct for tool-tip.
		LVHITTESTINFO m_htiEdit { };    //Hit struct for in-place editing.
		CEdit m_editInPlace;            //Edit box for in-place cells editing.
		DWORD m_dwGridWidth { };        //Grid width.
		DWORD m_dwTTShowDelay { };      //Tooltip delay before showing, in milliseconds.
		DWORD m_dwTTShowTime { };       //Tooltip show up time, in milliseconds.
		POINT m_ptTTOffset { };         //Tooltip offset from the cursor point.
		UINT m_uHLItem { };             //High latency Vscroll item.
		int m_iSortColumn { -1 };       //Currently clicked header column.
		int m_iLOGPIXELSY { };          //GetDeviceCaps(LOGPIXELSY) constant.
		PFNLVCOMPARE m_pfnCompare { };  //Pointer to a user provided compare func.
		EListExSortMode m_eDefSortMode { EListExSortMode::SORT_LEX }; //Default sorting mode.
		CRect m_rcLinkCurr;             //Current link's rect;
		std::unordered_map<UINT, std::unordered_map<int, TOOLTIPS>> m_umapCellTt;       //Cell's tooltips.
		std::unordered_map<UINT, std::unordered_map<int, ULONGLONG>> m_umapCellData;    //Cell's custom data.
		std::unordered_map<UINT, std::unordered_map<int, LISTEXCOLOR>> m_umapCellColor; //Cell's colors.
		std::unordered_map<UINT, COLROWCLR> m_umapRowColor;                   //Row colors.
		std::unordered_map<UINT, std::unordered_map<int, int>> m_umapCellIcon; //Cell's icon.
		std::unordered_map<int, COLROWCLR> m_umapColumnColor;                 //Column colors.
		std::unordered_map<int, EListExSortMode> m_umapColumnSortMode;         //Column sorting mode.
		bool m_fCreated { false };     //Is created.
		bool m_fHighLatency { };       //High latency flag.
		bool m_fSortable { false };    //Is list sortable.
		bool m_fSortAscending { };     //Sorting type (ascending, descending).
		bool m_fLinksUnderline { };    //Links are displayed underlined or not.
		bool m_fLinkTooltip { };       //Show links toolips.
		bool m_fVirtual { false };     //Whether list is virtual (LVS_OWNERDATA) or not.
		bool m_fCellTTActive { false }; //Is cell's tool-tip shown atm.
		bool m_fLinkTTActive { false }; //Is link's tool-tip shown atm.
		bool m_fLDownAtLink { false }; //Left mouse down on link.
		bool m_fHLFlag { };            //High latency Vscroll flag.
	};

	//Colors for the row/column.
	struct CListEx::COLROWCLR {
		LISTEXCOLOR clr; //Colors
		std::chrono::high_resolution_clock::time_point time; //Time when added.
	};

	//Text and links in the cell.
	struct CListEx::ITEMDATA {
		ITEMDATA(int iIconIndex, CRect rect) : rect(rect), iIconIndex(iIconIndex) {}; //Ctor for just image index.
		ITEMDATA(std::wstring_view wsvText, std::wstring_view wsvLink, std::wstring_view wsvTitle,
			CRect rect, bool fLink = false, bool fTitle = false) :
			wstrText(wsvText), wstrLink(wsvLink), wstrTitle(wsvTitle), rect(rect), fLink(fLink), fTitle(fTitle) {}
		std::wstring wstrText;  //Visible text.
		std::wstring wstrLink;  //Text within link <link="textFromHere"> tag.
		std::wstring wstrTitle; //Text within title <...title="textFromHere"> tag.
		CRect rect;             //Rect text belongs to.
		int iIconIndex { -1 };  //Icon index in the image list, if any.
		bool fLink { false };   //Is it just a text (wsvLink is empty) or text with link?
		bool fTitle { false };  //Is it link with custom title (wsvTitle is not empty)?
	};

	//List tooltips.
	struct CListEx::TOOLTIPS {
		std::wstring wstrText;
		std::wstring wstrCaption;
	};
}

HEXCTRL::LISTEX::IListExPtr HEXCTRL::LISTEX::CreateListEx() {
	return HEXCTRL::LISTEX::IListExPtr { new HEXCTRL::LISTEX::INTERNAL::CListEx() };
}

IMPLEMENT_DYNAMIC(CListEx, CMFCListCtrl)

BEGIN_MESSAGE_MAP(CListEx, CMFCListCtrl)
	ON_EN_KILLFOCUS(m_uIDEditInPlace, &CListEx::OnEditInPlaceKillFocus)
	ON_NOTIFY(HDN_BEGINDRAG, 0, &CListEx::OnHdnBegindrag)
	ON_NOTIFY(HDN_BEGINTRACKA, 0, &CListEx::OnHdnBegintrack)
	ON_NOTIFY(HDN_BEGINTRACKW, 0, &CListEx::OnHdnBegintrack)
	ON_NOTIFY(VK_RETURN, m_uIDEditInPlace, &CListEx::OnNotifyEditInPlace)
	ON_NOTIFY(VK_ESCAPE, m_uIDEditInPlace, &CListEx::OnNotifyEditInPlace)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, &CListEx::OnLvnColumnClick)
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_WM_HSCROLL()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MEASUREITEM_REFLECT()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_VSCROLL()
END_MESSAGE_MAP()

bool CListEx::Create(const LISTEXCREATE& lcs)
{
	assert(!IsCreated());
	if (IsCreated()) {
		return false;
	}

	const auto dwStyle = lcs.dwStyle | LVS_OWNERDRAWFIXED | LVS_REPORT;
	if (lcs.fDialogCtrl) {
		if (!SubclassDlgItem(lcs.uID, lcs.pParent))
			return false;

		const auto dwStyleCurr = GetWindowLongPtrW(m_hWnd, GWL_STYLE);
		SetWindowLongPtrW(m_hWnd, GWL_STYLE, dwStyleCurr | dwStyle);
		m_fVirtual = dwStyleCurr & LVS_OWNERDATA;
	}
	else {
		if (!CMFCListCtrl::CreateEx(lcs.dwExStyle, dwStyle, lcs.rect, lcs.pParent, lcs.uID))
			return false;

		m_fVirtual = dwStyle & LVS_OWNERDATA;
	}

	if (lcs.pColors != nullptr) {
		m_stColors = *lcs.pColors;
	}

	m_fSortable = lcs.fSortable;
	m_fLinksUnderline = lcs.fLinkUnderline;
	m_fLinkTooltip = lcs.fLinkTooltip;
	m_fHighLatency = lcs.fHighLatency;
	m_dwTTShowDelay = lcs.dwTTShowDelay;
	m_dwTTShowTime = lcs.dwTTShowTime;
	m_ptTTOffset = lcs.ptTTOffset;

	//Cell Tooltip.
	if (!m_wndCellTT.CreateEx(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr, lcs.dwTTStyleCell,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr)) {
		return false;
	}

	m_ttiCell.cbSize = sizeof(TTTOOLINFOW);
	//The TTF_TRACK flag should not be used with the TTS_BALLOON tooltips, because in this case
	//the balloon will always be positioned _below_ provided coordinates.
	m_ttiCell.uFlags = lcs.dwTTStyleCell & TTS_BALLOON ? 0 : TTF_TRACK;
	m_wndCellTT.SendMessageW(TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&m_ttiCell));
	m_wndCellTT.SendMessageW(TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(400)); //to allow use of newline \n.
	if (m_stColors.clrTooltipText != 0xFFFFFFFFUL || m_stColors.clrTooltipBk != 0xFFFFFFFFUL) {
		//To prevent Windows from changing theme of cells tooltip window.
		//Without this call Windows draws Tooltips with current Theme colors, and
		//TTM_SETTIPTEXTCOLOR/TTM_SETTIPBKCOLOR have no effect.
		SetWindowTheme(m_wndCellTT, nullptr, L"");
		m_wndCellTT.SendMessageW(TTM_SETTIPTEXTCOLOR, static_cast<WPARAM>(m_stColors.clrTooltipText), 0);
		m_wndCellTT.SendMessageW(TTM_SETTIPBKCOLOR, static_cast<WPARAM>(m_stColors.clrTooltipBk), 0);
	}

	//Link Tooltip.
	if (!m_wndLinkTT.CreateEx(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr, lcs.dwTTStyleLink,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr)) {
		return false;
	}

	m_ttiLink.cbSize = sizeof(TTTOOLINFOW);
	m_ttiLink.uFlags = lcs.dwTTStyleLink & TTS_BALLOON ? 0 : TTF_TRACK;
	m_wndLinkTT.SendMessageW(TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&m_ttiLink));
	m_wndLinkTT.SendMessageW(TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(400)); //to allow use of newline \n.

	if (m_fHighLatency) { //Tooltip for HighLatency mode.
		if (!m_wndRowTT.CreateEx(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr,
			TTS_NOANIMATE | TTS_NOFADE | TTS_NOPREFIX | TTS_ALWAYSTIP,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, nullptr)) {
			return false;
		}
		m_ttiHL.cbSize = sizeof(TTTOOLINFOW);
		m_ttiHL.uFlags = TTF_TRACK;
		m_wndRowTT.SendMessageW(TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&m_ttiHL));
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
	const DWORD dwHdrHeight = lcs.dwHdrHeight == 0 ?
		tm.tmHeight + tm.tmExternalLeading + 4 : lcs.dwHdrHeight; //Header is a bit higher than list rows.
	ReleaseDC(pDC);

	LOGFONTW lfList; //Font for a List.
	if (lcs.pListLogFont == nullptr) {
		lfList = ncm.lfMessageFont;
		lfList.lfHeight = -MulDiv(10, m_iLOGPIXELSY, 72); //Font height for list.
	}
	else {
		lfList = *lcs.pListLogFont;
	}

	LOGFONTW lfHdr; //Font for a Header.
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
	SetClassLongPtrW(m_hWnd, GCLP_HCURSOR, 0); //To prevent cursor from blinking.
	RecalcMeasure();

	m_fCreated = true;

	GetHeaderCtrl().SetColor(m_stColors);
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

BOOL CListEx::DeleteAllItems()
{
	assert(IsCreated());
	if (!IsCreated()) {
		return FALSE;
	}

	m_umapCellTt.clear();
	m_umapCellData.clear();
	m_umapCellColor.clear();
	m_umapRowColor.clear();
	m_umapCellIcon.clear();
	m_editInPlace.DestroyWindow();

	return CMFCListCtrl::DeleteAllItems();
}

BOOL CListEx::DeleteColumn(int iIndex)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return FALSE;
	}

	m_umapColumnColor.erase(iIndex);
	m_umapColumnSortMode.erase(iIndex);
	GetHeaderCtrl().DeleteColumn(iIndex);

	return CMFCListCtrl::DeleteColumn(iIndex);
}

BOOL CListEx::DeleteItem(int iItem)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return FALSE;
	}

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

auto CListEx::GetCellData(int iItem, int iSubItem)const->ULONGLONG
{
	assert(IsCreated());
	if (!IsCreated()) {
		return { };
	}

	const auto ID = MapIndexToID(iItem);
	const auto it = m_umapCellData.find(ID);

	if (it != m_umapCellData.end()) {
		const auto itInner = it->second.find(iSubItem);

		//If subitem id found.
		if (itInner != it->second.end()) {
			return itInner->second;
		}
	}

	return 0;
}

auto CListEx::GetColors()const->const LISTEXCOLORS& {
	return m_stColors;
}

auto CListEx::GetColumnSortMode(int iColumn)const->EListExSortMode
{
	assert(IsCreated());

	if (const auto iter = m_umapColumnSortMode.find(iColumn); iter != m_umapColumnSortMode.end()) {
		return iter->second;
	}

	return m_eDefSortMode;
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

	return m_fSortAscending;
}

void CListEx::HideColumn(int iIndex, bool fHide)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return;
	}

	GetHeaderCtrl().HideColumn(iIndex, fHide);
	RedrawWindow();
}

int CListEx::InsertColumn(int nCol, const LVCOLUMNW* pColumn, int iDataAlign, bool fEditable)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return -1;
	}

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
	refHdr.SetColumnDataAlign(iNewIndex, iDataAlign);

	//First (zero index) column is always left-aligned by default, no matter what the pColumn->fmt is set to.
	//To change the alignment a user must explicitly call the SetColumn after the InsertColumn.
	//This call here is just to remove that absurd limitation.
	const LVCOLUMNW stCol { LVCF_FMT, pColumn->fmt };
	SetColumn(iNewIndex, &stCol);

	//All new columns are not editable by default.
	if (fEditable) {
		SetColumnEditable(iNewIndex, true);
	}

	return iNewIndex;
}

int CListEx::InsertColumn(int nCol, LPCWSTR pwszName, int nFormat, int nWidth, int nSubItem, int iDataAlign, bool fEditable)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return -1;
	}

	LVCOLUMNW lvcol { };
	lvcol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_TEXT;
	lvcol.fmt = nFormat;
	lvcol.cx = nWidth;
	lvcol.iSubItem = nSubItem;
	lvcol.pszText = const_cast<LPWSTR>(pwszName);

	return InsertColumn(nCol, &lvcol, iDataAlign, fEditable);
}

bool CListEx::IsCreated()const
{
	return m_fCreated;
}

bool CListEx::IsColumnSortable(int iColumn)
{
	return GetHeaderCtrl().IsColumnSortable(iColumn);
}

void CListEx::ResetSort()
{
	m_iSortColumn = -1;
	GetHeaderCtrl().SetSortArrow(-1, false);
}

void CListEx::SetCellColor(int iItem, int iSubItem, COLORREF clrBk, COLORREF clrText)
{
	assert(IsCreated());
	assert(!m_fVirtual);
	if (!IsCreated() || m_fVirtual) {
		return;
	}

	if (clrText == -1) { //-1 for default color.
		clrText = m_stColors.clrListText;
	}

	const auto ID = MapIndexToID(static_cast<UINT>(iItem));

	//If there is no color for such item/subitem, we just set it.
	if (const auto it = m_umapCellColor.find(ID); it == m_umapCellColor.end()) {
		m_umapCellColor.try_emplace(ID, std::unordered_map<int, LISTEXCOLOR> {
			{ iSubItem, { .clrBk { clrBk }, .clrText { clrText } } } });
	}
	else {
		const auto itInner = it->second.find(iSubItem);

		if (itInner == it->second.end()) {
			it->second.insert({ iSubItem, { .clrBk { clrBk }, .clrText { clrText } } });
		}
		else { //If there is already this cell's color, we change it.
			itInner->second.clrBk = clrBk;
			itInner->second.clrText = clrText;
		}
	}
}

void CListEx::SetCellData(int iItem, int iSubItem, ULONGLONG ullData)
{
	assert(IsCreated());
	assert(!m_fVirtual);
	if (!IsCreated() || m_fVirtual) {
		return;
	}

	const auto ID = MapIndexToID(iItem);

	//If there is no data for such item/subitem, we just set it.
	if (const auto it = m_umapCellData.find(ID); it == m_umapCellData.end()) {
		m_umapCellData.try_emplace(ID, std::unordered_map<int, ULONGLONG>{ { iSubItem, ullData } });
	}
	else {
		if (const auto itInner = it->second.find(iSubItem); itInner == it->second.end()) {
			it->second.insert({ iSubItem, ullData });
		}
		else { //If there is already this cell's data, we change it.
			itInner->second = ullData;
		}
	}
}

void CListEx::SetCellIcon(int iItem, int iSubItem, int iIndex)
{
	assert(IsCreated());
	assert(!m_fVirtual);
	if (!IsCreated() || m_fVirtual) {
		return;
	}

	const auto ID = MapIndexToID(iItem);

	//If there is no icon index for such item, we just set it.
	if (const auto it = m_umapCellIcon.find(ID); it == m_umapCellIcon.end()) {
		m_umapCellIcon.try_emplace(ID, std::unordered_map<int, int>{ { iSubItem, iIndex } });
	}
	else {
		if (const auto itInner = it->second.find(iSubItem); itInner == it->second.end()) {
			it->second.insert({ iSubItem, iIndex });
		}
		else {
			itInner->second = iIndex;
		}
	}
}

void CListEx::SetCellTooltip(int iItem, int iSubItem, std::wstring_view wsvTooltip, std::wstring_view wsvCaption)
{
	assert(IsCreated());
	assert(!m_fVirtual);
	if (!IsCreated() || m_fVirtual) {
		return;
	}

	const auto ID = MapIndexToID(iItem);

	//If there is no tooltip for such item/subitem, we just set it.
	if (const auto it = m_umapCellTt.find(ID); it == m_umapCellTt.end()) {
		if (!wsvTooltip.empty() || !wsvCaption.empty()) {
			m_umapCellTt.try_emplace(ID, std::unordered_map<int, TOOLTIPS> {
				{ iSubItem, { .wstrText { std::wstring { wsvTooltip } }, .wstrCaption { std::wstring { wsvCaption } } } } });
		}
	}
	else {
		//If there is Item's tooltip but no Subitem's tooltip, inserting new Subitem into inner map.
		if (const auto itInner = it->second.find(iSubItem);	itInner == it->second.end()) {
			if (!wsvTooltip.empty() || !wsvCaption.empty()) {
				it->second.insert({ iSubItem,
					{ .wstrText { std::wstring { wsvTooltip } }, .wstrCaption { std::wstring { wsvCaption } } } });
			}
		}
		else { //If there is already this Item-Subitem's tooltip, change or erase it, depending on pwszTooltip emptiness.
			if (!wsvTooltip.empty()) {
				itInner->second = { .wstrText { std::wstring { wsvTooltip } }, .wstrCaption { std::wstring { wsvCaption } } };
			}
			else {
				it->second.erase(itInner);
			}
		}
	}
}

void CListEx::SetColors(const LISTEXCOLORS& lcs)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return;
	}

	m_stColors = lcs;
	GetHeaderCtrl().SetColor(lcs);
	RedrawWindow();
}

void CListEx::SetColumnColor(int iColumn, COLORREF clrBk, COLORREF clrText)
{
	if (clrText == -1) { //-1 for default color.
		clrText = m_stColors.clrListText;
	}

	m_umapColumnColor[iColumn] = { .clr { .clrBk { clrBk }, .clrText { clrText } },
		.time { std::chrono::high_resolution_clock::now() } };
}

void CListEx::SetColumnEditable(int iColumn, bool fEditable)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return;
	}

	GetHeaderCtrl().SetColumnEditable(iColumn, fEditable);
}

void CListEx::SetColumnSortMode(int iColumn, bool fSortable, EListExSortMode eSortMode)
{
	m_umapColumnSortMode[iColumn] = eSortMode;
	GetHeaderCtrl().SetColumnSortable(iColumn, fSortable);
}

void CListEx::SetFont(const LOGFONTW* pLogFont)
{
	assert(IsCreated());
	assert(pLogFont);
	if (!IsCreated() || !pLogFont) {
		return;
	}

	m_fontList.DeleteObject();
	m_fontList.CreateFontIndirectW(pLogFont);

	RecalcMeasure();
	Update(0);
}

void CListEx::SetHdrHeight(DWORD dwHeight)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return;
	}

	GetHeaderCtrl().SetHeight(dwHeight);
	Update(0);
	GetHeaderCtrl().RedrawWindow();
}

void CListEx::SetHdrImageList(CImageList* pList)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return;
	}

	GetHeaderCtrl().SetImageList(pList);
}

void CListEx::SetHdrFont(const LOGFONTW* pLogFont)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return;
	}

	GetHeaderCtrl().SetFont(pLogFont);
	Update(0);
	GetHeaderCtrl().RedrawWindow();
}

void CListEx::SetHdrColumnColor(int iColumn, COLORREF clrBk, COLORREF clrText)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return;
	}

	GetHeaderCtrl().SetColumnColor(iColumn, clrBk, clrText);
	Update(0);
	GetHeaderCtrl().RedrawWindow();
}

void CListEx::SetHdrColumnIcon(int iColumn, const LISTEXHDRICON& stIcon)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return;
	}

	GetHeaderCtrl().SetColumnIcon(iColumn, stIcon);
}

void CListEx::SetRowColor(DWORD dwRow, COLORREF clrBk, COLORREF clrText)
{
	assert(IsCreated());
	if (!IsCreated()) {
		return;
	}

	if (clrText == -1) { //-1 for default color.
		clrText = m_stColors.clrListText;
	}

	m_umapRowColor[dwRow] = { .clr { .clrBk { clrBk }, .clrText { clrText } }, .time { std::chrono::high_resolution_clock::now() } };
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

	GetHeaderCtrl().SetSortable(fSortable);
}

int CALLBACK CListEx::DefCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	const auto* const pListCtrl = reinterpret_cast<IListEx*>(lParamSort);
	const auto iSortColumn = pListCtrl->GetSortColumn();
	const auto eSortMode = pListCtrl->GetColumnSortMode(iSortColumn);
	const auto wstrItem1 = pListCtrl->GetItemText(static_cast<int>(lParam1), iSortColumn);
	const auto wstrItem2 = pListCtrl->GetItemText(static_cast<int>(lParam2), iSortColumn);

	int iCompare { };
	switch (eSortMode) {
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
	const auto iItem = pDIS->itemID;

	switch (pDIS->itemAction) {
	case ODA_SELECT:
	case ODA_DRAWENTIRE:
	{
		const auto pDC = CDC::FromHandle(pDIS->hDC);
		const auto clrBkCurrRow = (iItem % 2) ? m_stColors.clrListBkEven : m_stColors.clrListBkOdd;
		const auto& refHdr = GetHeaderCtrl();
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

			CRect rcBounds;
			GetSubItemRect(iItem, iSubitem, LVIR_BOUNDS, rcBounds);
			pDC->FillSolidRect(rcBounds, clrBk);

			for (const auto& itItemData : ParseItemData(iItem, iSubitem)) {
				if (itItemData.iIconIndex > -1) {
					GetImageList(LVSIL_NORMAL)->DrawEx(pDC, itItemData.iIconIndex,
						{ itItemData.rect.left, itItemData.rect.top }, { }, CLR_NONE, CLR_NONE, ILD_NORMAL);
					continue;
				}

				if (itItemData.fLink) {
					pDC->SetTextColor(clrTextLink);
					if (m_fLinksUnderline) {
						pDC->SelectObject(m_fontListUnderline);
					}
				}
				else {
					pDC->SetTextColor(clrText);
					pDC->SelectObject(m_fontList);
				}

				ExtTextOutW(pDC->m_hDC, itItemData.rect.left, itItemData.rect.top, ETO_CLIPPED,
					rcBounds, itItemData.wstrText.data(), static_cast<UINT>(itItemData.wstrText.size()), nullptr);
			}

			//Drawing subitem's rect lines.
			if (m_dwGridWidth > 0) {
				pDC->SelectObject(m_penGrid);
				pDC->MoveTo(rcBounds.TopLeft());
				pDC->LineTo(rcBounds.right, rcBounds.top);   //Top line.
				pDC->MoveTo(rcBounds.TopLeft());
				pDC->LineTo(rcBounds.left, rcBounds.bottom); //Left line.
				pDC->MoveTo(rcBounds.left, rcBounds.bottom);
				pDC->LineTo(rcBounds.BottomRight());         //Bottom line.
				if (iSubitem == iColumns - 1) { //Drawing a right line only for the last column.
					rcBounds.right -= 1; //To overcome a glitch with a last line disappearing if resizing a header.
					pDC->MoveTo(rcBounds.right, rcBounds.top);
					pDC->LineTo(rcBounds.BottomRight());     //Right line.
				}
			}

			//Draw focus rect (marquee).
			if ((pDIS->itemState & ODS_FOCUS) && !(pDIS->itemState & ODS_SELECTED)) {
				pDC->DrawFocusRect(rcBounds);
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

	if (m_fVirtual) { //In Virtual mode asking parent for color.
		const auto iCtrlID = GetDlgCtrlID();
		const LISTEXCOLORINFO lci { .hdr { .hwndFrom { m_hWnd }, .idFrom { static_cast<UINT>(iCtrlID) }, .code { LISTEX_MSG_GETCOLOR } },
			.iItem { iItem }, .iSubItem { iSubItem }, .stClr { .clrBk { 0xFFFFFFFFUL }, .clrText { 0xFFFFFFFFUL } } };
		GetParent()->SendMessageW(WM_NOTIFY, static_cast<WPARAM>(iCtrlID), reinterpret_cast<LPARAM>(&lci));
		if (lci.stClr.clrBk != -1 || lci.stClr.clrText != -1) {
			return lci.stClr;
		}
	}
	else {
		const auto ID = MapIndexToID(static_cast<UINT>(iItem));
		if (const auto itItem = m_umapCellColor.find(ID); itItem != m_umapCellColor.end()) {
			if (const auto itSubItem = itItem->second.find(iSubItem); itSubItem != itItem->second.end()) { //If subitem id found.
				return itSubItem->second;
			}
		}

		const auto itColumn = m_umapColumnColor.find(iSubItem);
		const auto itRow = m_umapRowColor.find(ID);

		if (itColumn != m_umapColumnColor.end() && itRow != m_umapRowColor.end()) {
			return itColumn->second.time > itRow->second.time ? itColumn->second.clr : itRow->second.clr;
		}

		if (itColumn != m_umapColumnColor.end()) {
			return itColumn->second.clr;
		}

		if (itRow != m_umapRowColor.end()) {
			return itRow->second.clr;
		}
	}

	return std::nullopt;
}

auto CListEx::GetTooltip(int iItem, int iSubItem)const->std::optional<LISTEXTTDATA>
{
	if (iItem < 0 || iSubItem < 0) {
		return std::nullopt;
	}

	if (m_fVirtual) { //In Virtual mode asking parent for tooltip.
		const auto iCtrlID = GetDlgCtrlID();
		const LISTEXTTINFO ltti { .hdr { .hwndFrom { m_hWnd }, .idFrom { static_cast<UINT>(iCtrlID) },
			.code { LISTEX_MSG_GETTOOLTIP } }, .iItem { iItem }, .iSubItem { iSubItem } };
		if (GetParent()->SendMessageW(WM_NOTIFY, static_cast<WPARAM>(iCtrlID), reinterpret_cast<LPARAM>(&ltti));
			ltti.stData.pwszText != nullptr) { //If tooltip text was set by the Parent.
			return ltti.stData;
		}
	}
	else {
		const auto ID = MapIndexToID(iItem);
		if (const auto itItem = m_umapCellTt.find(ID); itItem != m_umapCellTt.end()) {
			if (const auto itSubItem = itItem->second.find(iSubItem); itSubItem != itItem->second.end()) {
				return { { .pwszText { itSubItem->second.wstrText.data() }, .pwszCaption { itSubItem->second.wstrCaption.data() } } };
			}
		}
	}

	return std::nullopt;
}

int CListEx::GetIcon(int iItem, int iSubItem)const
{
	if (GetImageList(LVSIL_NORMAL) == nullptr) {
		return -1; //-1 is the default, when no image for cell is set.
	}

	if (m_fVirtual) { //In Virtual mode asking parent for the icon index in image list.
		const auto uCtrlID = static_cast<UINT>(GetDlgCtrlID());
		const LISTEXICONINFO lii { .hdr { .hwndFrom { m_hWnd }, .idFrom { uCtrlID }, .code { LISTEX_MSG_GETICON } },
			.iItem { iItem }, .iSubItem { iSubItem } };
		GetParent()->SendMessageW(WM_NOTIFY, static_cast<WPARAM>(uCtrlID), reinterpret_cast<LPARAM>(&lii));
		return lii.iIconIndex; //By default it's -1, meaning no icon.
	}

	const auto ID = MapIndexToID(iItem);
	if (const auto it = m_umapCellIcon.find(ID); it != m_umapCellIcon.end()) {
		if (const auto itInner = it->second.find(iSubItem); itInner != it->second.end()) {
			return itInner->second;
		}
	}

	return -1;
}

void CListEx::InitHeader()
{
	GetHeaderCtrl().SubclassDlgItem(0, this);
}

void CListEx::MeasureItem(LPMEASUREITEMSTRUCT lpMIS)
{
	//Set row height according to current font's height.
	auto pDC = GetDC();
	pDC->SelectObject(m_fontList);
	TEXTMETRICW tm;
	GetTextMetricsW(pDC->m_hDC, &tm);
	ReleaseDC(pDC);
	lpMIS->itemHeight = tm.tmHeight + tm.tmExternalLeading + 1;
}

void CListEx::OnDestroy()
{
	CMFCListCtrl::OnDestroy();

	m_wndCellTT.DestroyWindow();
	m_wndLinkTT.DestroyWindow();
	m_wndRowTT.DestroyWindow();
	m_fontList.DeleteObject();
	m_fontListUnderline.DeleteObject();
	m_penGrid.DeleteObject();
	m_editInPlace.DestroyWindow();
	m_umapCellTt.clear();
	m_umapCellData.clear();
	m_umapCellColor.clear();
	m_umapCellIcon.clear();
	m_umapRowColor.clear();
	m_umapColumnSortMode.clear();
	m_umapColumnColor.clear();
	m_fCreated = false;
}

void CListEx::OnEditInPlaceEnterPressed()
{
	CStringW wstr;
	m_editInPlace.GetWindowTextW(wstr);
	if (m_fVirtual) {
		const auto uCtrlId = static_cast<UINT>(GetDlgCtrlID());
		const LISTEXDATAINFO ldi { .hdr { .hwndFrom { m_hWnd }, .idFrom { uCtrlId }, .code { LISTEX_MSG_SETDATA } },
			.iItem { m_htiEdit.iItem }, .iSubItem { m_htiEdit.iSubItem }, .pwszData { wstr.GetString() } };
		GetParent()->SendMessageW(WM_NOTIFY, static_cast<WPARAM>(uCtrlId), reinterpret_cast<LPARAM>(&ldi));
	}
	else {
		SetItemText(m_htiEdit.iItem, m_htiEdit.iSubItem, wstr);
	}

	OnEditInPlaceKillFocus();
}

void CListEx::OnEditInPlaceKillFocus()
{
	m_editInPlace.DestroyWindow();
	RedrawWindow();
}

BOOL CListEx::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

void CListEx::OnHdnBegindrag(NMHDR* pNMHDR, LRESULT* pResult)
{
	const auto phdr = reinterpret_cast<LPNMHEADERW>(pNMHDR);
	if (GetHeaderCtrl().IsColumnHidden(phdr->iItem)) {
		*pResult = TRUE;
	}
}

void CListEx::OnHdnBegintrack(NMHDR* pNMHDR, LRESULT* pResult)
{
	const auto phdr = reinterpret_cast<LPNMHEADERW>(pNMHDR);
	if (GetHeaderCtrl().IsColumnHidden(phdr->iItem)) {
		*pResult = TRUE;
	}
}

void CListEx::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	GetHeaderCtrl().RedrawWindow();
	CMFCListCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CListEx::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	m_htiEdit.pt = point;
	SubItemHitTest(&m_htiEdit);
	if (m_htiEdit.iItem < 0 || m_htiEdit.iSubItem < 0 || !GetHeaderCtrl().IsColumnEditable(m_htiEdit.iSubItem)) {
		CMFCListCtrl::OnLButtonDblClk(nFlags, point);
		return;
	}

	const auto uCtrlId = static_cast<UINT>(GetDlgCtrlID());
	const LISTEXDATAINFO ldi { .hdr { .hwndFrom { m_hWnd }, .idFrom { uCtrlId }, .code { LISTEX_MSG_EDITBEGIN } },
		.iItem { m_htiEdit.iItem }, .iSubItem { m_htiEdit.iSubItem } };
	GetParent()->SendMessageW(WM_NOTIFY, static_cast<WPARAM>(uCtrlId), reinterpret_cast<LPARAM>(&ldi));
	if (!ldi.fAllowEdit) { //User explicitly declined to display edit-box.
		CMFCListCtrl::OnLButtonDblClk(nFlags, point);
		return;
	}

	//Get Column data alignment.
	const auto iAlignment = GetHeaderCtrl().GetColumnDataAlign(m_htiEdit.iSubItem);
	const DWORD dwStyle = iAlignment == LVCFMT_LEFT ? ES_LEFT : (iAlignment == LVCFMT_RIGHT ? ES_RIGHT : ES_CENTER);
	CRect rcCell;
	GetSubItemRect(m_htiEdit.iItem, m_htiEdit.iSubItem, LVIR_BOUNDS, rcCell);
	if (m_htiEdit.iSubItem == 0) { //Clicked on item (first column).
		CRect rcLabel;
		GetItemRect(m_htiEdit.iItem, rcLabel, LVIR_LABEL);
		rcCell.right = rcLabel.right;
	}

	const auto str = GetItemText(m_htiEdit.iItem, m_htiEdit.iSubItem);
	m_editInPlace.DestroyWindow();
	m_editInPlace.Create(dwStyle | WS_BORDER | WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, rcCell, this, m_uIDEditInPlace);
	::SetWindowSubclass(m_editInPlace, EditSubclassProc, 0, m_uIDEditInPlace);
	m_editInPlace.SetFont(&m_fontList, FALSE);
	m_editInPlace.SetWindowTextW(str);
	m_editInPlace.SetFocus();

	CMFCListCtrl::OnLButtonDblClk(nFlags, point);
}

void CListEx::OnLButtonDown(UINT nFlags, CPoint pt)
{
	bool fLinkDown { false };
	LVHITTESTINFO hti { pt };
	SubItemHitTest(&hti);
	if (hti.iItem >= 0 && hti.iSubItem >= 0) {
		const auto vecText = ParseItemData(hti.iItem, hti.iSubItem);
		if (const auto iterFind = std::find_if(vecText.begin(), vecText.end(), [&](const ITEMDATA& item) {
			return item.fLink && item.rect.PtInRect(pt); }); iterFind != vecText.end()) {
			m_fLDownAtLink = true;
			m_rcLinkCurr = iterFind->rect;
			fLinkDown = true;
		}
	}

	if (!fLinkDown) {
		CMFCListCtrl::OnLButtonDown(nFlags, pt);
	}
}

void CListEx::OnLButtonUp(UINT nFlags, CPoint pt)
{
	bool fLinkUp { false };
	if (m_fLDownAtLink) {
		LVHITTESTINFO hti { pt };
		SubItemHitTest(&hti);
		if (hti.iItem < 0 || hti.iSubItem < 0) {
			m_fLDownAtLink = false;
			return;
		}

		const auto vecText = ParseItemData(hti.iItem, hti.iSubItem);
		if (const auto iterFind = std::find_if(vecText.begin(), vecText.end(), [&](const ITEMDATA& item) {
			return item.fLink && item.rect == m_rcLinkCurr; }); iterFind != vecText.end()) {
			m_rcLinkCurr.SetRectEmpty();
			fLinkUp = true;
			const auto uCtrlId = static_cast<UINT>(GetDlgCtrlID());
			const LISTEXLINKINFO lli { .hdr { .hwndFrom { m_hWnd }, .idFrom { uCtrlId }, .code { LISTEX_MSG_LINKCLICK } },
				.iItem { hti.iItem }, .iSubItem { hti.iSubItem }, .ptClick { pt }, .pwszText { iterFind->wstrLink.data() } };
			GetParent()->SendMessageW(WM_NOTIFY, static_cast<WPARAM>(uCtrlId), reinterpret_cast<LPARAM>(&lli));
		}
	}

	m_fLDownAtLink = false;
	if (!fLinkUp) {
		CMFCListCtrl::OnLButtonUp(nFlags, pt);
	}
}

void CListEx::OnLvnColumnClick(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	/*******************************************************************************
	* Just an empty handler.
	* Without it all works fine, but assert triggers in a Debug mode when clicking
	* on the header if list is in the Virtual Mode (LVS_OWNERDATA).
	* ASSERT((GetStyle() & LVS_OWNERDATA)==0)
	*
	* Handler must be present for the LVN_COLUMNCLICK notification message in the
	* list's parent window, and default OnNotify handler should not be called.
	* Then there will be no assertion.
	*******************************************************************************/
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
	static const auto hCurArrow = static_cast<HCURSOR>(LoadImageW(nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
	static const auto hCurHand = static_cast<HCURSOR>(LoadImageW(nullptr, IDC_HAND, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED));

	LVHITTESTINFO hti { pt };
	SubItemHitTest(&hti);
	bool fLinkRect { false }; //Cursor at link's rect area?
	if (hti.iItem >= 0 && hti.iSubItem >= 0) {
		auto vecText = ParseItemData(hti.iItem, hti.iSubItem);     //Non const to allow std::move(itLink->wstrLink).
		auto itLink = std::find_if(vecText.begin(), vecText.end(), //Non const to allow std::move(itLink->wstrLink).
			[&](const ITEMDATA& item) {	return item.fLink && item.rect.PtInRect(pt); });
		if (itLink != vecText.end()) {
			fLinkRect = true;
			if (m_fLinkTooltip && !m_fLDownAtLink && m_rcLinkCurr != itLink->rect) {
				TTLinkShow(false);
				m_fLinkTTActive = true;
				m_rcLinkCurr = itLink->rect;
				m_htiCurrLink.iItem = hti.iItem;
				m_htiCurrLink.iSubItem = hti.iSubItem;
				m_wstrTextTT = itLink->fTitle ? std::move(itLink->wstrTitle) : std::move(itLink->wstrLink);
				m_ttiLink.lpszText = m_wstrTextTT.data();
				if (m_dwTTShowDelay > 0) {
					SetTimer(m_uIDTLinkTTActivate, m_dwTTShowDelay, nullptr); //Activate link's tooltip after delay.
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
		return; //Do not process further, cursor is on the link's rect.
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
			m_wstrTextTT = optTT->pwszText; //Tooltip text.
			m_wstrCaptionTT = optTT->pwszCaption ? optTT->pwszCaption : L""; //Tooltip caption.
			m_ttiCell.lpszText = m_wstrTextTT.data();
			if (m_dwTTShowDelay > 0) {
				SetTimer(m_uIDTCellTTActivate, m_dwTTShowDelay, nullptr); //Activate cell's tooltip after delay.
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
}

BOOL CListEx::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	if (!m_fCreated) {
		return FALSE;
	}

	//HDN_ITEMCLICK messages should be handled here first, to set m_fSortAscending 
	//and m_iSortColumn. And only then this message goes further, to parent window,
	//in form of HDN_ITEMCLICK and LVN_COLUMNCLICK.
	//If we execute this code in LVN_COLUMNCLICK handler, it will be handled
	//only AFTER the parent window handles LVN_COLUMNCLICK.
	//So briefly, ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, &CListEx::OnLvnColumnClick) fires up
	//only AFTER LVN_COLUMNCLICK sent to the parent.
	if (const auto* const pNMLV = reinterpret_cast<LPNMHEADERW>(lParam); m_fSortable
		&& (pNMLV->hdr.code == HDN_ITEMCLICKW || pNMLV->hdr.code == HDN_ITEMCLICKA)) {
		if (IsColumnSortable(pNMLV->iItem)) {
			m_fSortAscending = pNMLV->iItem == m_iSortColumn ? !m_fSortAscending : true;
			GetHeaderCtrl().SetSortArrow(pNMLV->iItem, m_fSortAscending);
			m_iSortColumn = pNMLV->iItem;
		}
		else {
			m_iSortColumn = -1;
		}

		if (!m_fVirtual) {
			return SortItemsEx(m_pfnCompare ? m_pfnCompare : DefCompareFunc, reinterpret_cast<DWORD_PTR>(this));
		}
	}

	return CMFCListCtrl::OnNotify(wParam, lParam, pResult);
}

void CListEx::OnNotifyEditInPlace(NMHDR* pNMHDR, LRESULT* /*pResult*/)
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

void CListEx::OnPaint()
{
	CPaintDC dc(this);

	CRect rcClient;
	GetClientRect(&rcClient);
	CRect rcHdr;
	GetHeaderCtrl().GetClientRect(rcHdr);
	rcClient.top += rcHdr.Height();
	if (rcClient.IsRectEmpty()) {
		return;
	}

	CMemDC memDC(dc, rcClient); //To avoid flickering drawing to CMemDC, excluding list header area (rcHdr).
	auto& rDC = memDC.GetDC();
	rDC.GetClipBox(&rcClient);
	rDC.FillSolidRect(rcClient, m_stColors.clrNWABk);

	DefWindowProcW(WM_PAINT, reinterpret_cast<WPARAM>(rDC.m_hDC), 0);
}

void CListEx::OnTimer(UINT_PTR nIDEvent)
{
	CPoint ptScreen;
	GetCursorPos(&ptScreen);
	CPoint ptClient = ptScreen;
	ScreenToClient(&ptClient);
	LVHITTESTINFO hitInfo { ptClient };
	SubItemHitTest(&hitInfo);

	switch (nIDEvent) {
	case m_uIDTCellTTActivate:
		KillTimer(m_uIDTCellTTActivate);
		if (m_htiCurrCell.iItem == hitInfo.iItem && m_htiCurrCell.iSubItem == hitInfo.iSubItem) {
			TTCellShow(true);
		}
		break;
	case m_uIDTLinkTTActivate:
		KillTimer(m_uIDTLinkTTActivate);
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
		CMFCListCtrl::OnTimer(nIDEvent);
		break;
	}
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
			TTHLShow(false, 0);
			CMFCListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
		}
		else {
			m_fHLFlag = true;
			SCROLLINFO si { };
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_ALL;
			GetScrollInfo(SB_VERT, &si);
			m_uHLItem = si.nTrackPos; //si.nTrackPos is in fact a row number.
			TTHLShow(true, m_uHLItem);
		}
	}
	else {
		CMFCListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
	}
}

auto CListEx::ParseItemData(int iItem, int iSubitem)->std::vector<CListEx::ITEMDATA>
{
	constexpr auto iIndentRc { 4 };
	const auto cstrText = GetItemText(iItem, iSubitem);
	const std::wstring_view wsvText = cstrText.GetString();
	CRect rcTextOrig;  //Original rect of the subitem's text.
	GetSubItemRect(iItem, iSubitem, LVIR_LABEL, rcTextOrig);
	if (iSubitem > 0) { //Not needed for item itself (not subitem).
		rcTextOrig.left += iIndentRc;
	}

	std::vector<ITEMDATA> vecData;
	int iImageWidth { 0 };

	if (const auto iIndex = GetIcon(iItem, iSubitem); iIndex > -1) { //If cell has icon.
		IMAGEINFO stII;
		GetImageList(LVSIL_NORMAL)->GetImageInfo(iIndex, &stII);
		iImageWidth = stII.rcImage.right - stII.rcImage.left;
		const auto iImgHeight = stII.rcImage.bottom - stII.rcImage.top;
		const auto iImgTop = iImgHeight < rcTextOrig.Height() ?
			rcTextOrig.top + (rcTextOrig.Height() / 2 - iImgHeight / 2) : rcTextOrig.top;
		const CRect rcImage(rcTextOrig.left, iImgTop, rcTextOrig.left + iImageWidth, iImgTop + iImgHeight);
		vecData.emplace_back(iIndex, rcImage);
		rcTextOrig.left += iImageWidth + iIndentRc; //Offset rect for image width.
	}

	std::size_t nPosCurr { 0 }; //Current position in the parsed string.
	CRect rcTextCurr { }; //Current rect.
	const auto pDC = GetDC();

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
			nPosTagLink != std::wstring_view::npos && nPosLinkOpenQuote != std::wstring_view::npos
			&& nPosLinkCloseQuote != std::wstring_view::npos && nPosTagFirstClose != std::wstring_view::npos
			&& nPosTagLast != std::wstring_view::npos) {
			pDC->SelectObject(m_fontList);
			SIZE size;

			//Any text before found tag.
			if (nPosTagLink > nPosCurr) {
				const auto wsvTextBefore = wsvText.substr(nPosCurr, nPosTagLink - nPosCurr);
				GetTextExtentPoint32W(pDC->m_hDC, wsvTextBefore.data(), static_cast<int>(wsvTextBefore.size()), &size);
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
			GetTextExtentPoint32W(pDC->m_hDC, wsvTextBetweenTags.data(), static_cast<int>(wsvTextBetweenTags.size()), &size);

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
			pDC->SelectObject(m_fontList);
			SIZE size;
			GetTextExtentPoint32W(pDC->m_hDC, wsvTextAfter.data(), static_cast<int>(wsvTextAfter.size()), &size);

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
	ReleaseDC(pDC);

	//Depending on column's data alignment we adjust rects coords to render properly.
	switch (GetHeaderCtrl().GetColumnDataAlign(iSubitem)) {
	case LVCFMT_LEFT: //Do nothing, by default it's already left aligned.
		break;
	case LVCFMT_CENTER:
	{
		int iWidthTotal { };
		for (const auto& iter : vecData) {
			iWidthTotal += iter.rect.Width();
		}

		if (iWidthTotal < rcTextOrig.Width()) {
			const auto iRcOffset = (rcTextOrig.Width() - iWidthTotal) / 2;
			for (auto& iter : vecData) {
				if (iter.iIconIndex == -1) { //Offset only rects with text, not icons rects.
					iter.rect.OffsetRect(iRcOffset, 0);
				}
			}
		}
	}
	break;
	case LVCFMT_RIGHT:
	{
		int iWidthTotal { };
		for (const auto& iter : vecData) {
			iWidthTotal += iter.rect.Width();
		}

		const auto iWidthRect = rcTextOrig.Width() + iImageWidth - iIndentRc;
		if (iWidthTotal < iWidthRect) {
			const auto iRcOffset = iWidthRect - iWidthTotal;
			for (auto& iter : vecData) {
				if (iter.iIconIndex == -1) { //Offset only rects with text, not icons rects.
					iter.rect.OffsetRect(iRcOffset, 0);
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

void CListEx::RecalcMeasure()const
{
	//To get WM_MEASUREITEM msg after changing the font.
	CRect rc;
	GetWindowRect(rc);
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
	if (!IsCreated()) {
		return;
	}

	//Prevent font size from being too small or too big.
	if (lSize < 4 || lSize > 64) {
		return;
	}

	LOGFONTW lf;
	m_fontList.GetLogFont(&lf);
	m_fontList.DeleteObject();
	lf.lfHeight = -MulDiv(lSize, m_iLOGPIXELSY, 72);
	m_fontList.CreateFontIndirectW(&lf);
	lf.lfUnderline = 1;
	m_fontListUnderline.DeleteObject();
	m_fontListUnderline.CreateFontIndirectW(&lf);

	RecalcMeasure();
	Update(0);

	if (IsWindow(m_editInPlace)) { //If m_editInPlace is active, ammend its rect.
		CRect rcCell;
		GetSubItemRect(m_htiEdit.iItem, m_htiEdit.iSubItem, LVIR_BOUNDS, rcCell);
		if (m_htiEdit.iSubItem == 0) { //Clicked on item (first column).
			CRect rcLabel;
			GetItemRect(m_htiEdit.iItem, rcLabel, LVIR_LABEL);
			rcCell.right = rcLabel.right;
		}
		m_editInPlace.SetWindowPos(nullptr, rcCell.left, rcCell.top, rcCell.Width(), rcCell.Height(), SWP_NOZORDER);
		m_editInPlace.SetFont(&m_fontList, FALSE);
	}
}

void CListEx::TTCellShow(bool fShow, bool fTimer)
{
	if (fShow) {
		CPoint ptScreen;
		GetCursorPos(&ptScreen);
		ptScreen.Offset(m_ptTTOffset);
		m_wndCellTT.SendMessageW(TTM_TRACKPOSITION, 0, static_cast<LPARAM>MAKELONG(ptScreen.x, ptScreen.y));
		m_wndCellTT.SendMessageW(TTM_SETTITLEW, static_cast<WPARAM>(TTI_NONE), reinterpret_cast<LPARAM>(m_wstrCaptionTT.data()));
		m_wndCellTT.SendMessageW(TTM_UPDATETIPTEXTW, 0, reinterpret_cast<LPARAM>(&m_ttiCell));
		m_wndCellTT.SendMessageW(TTM_TRACKACTIVATE, static_cast<WPARAM>(TRUE), reinterpret_cast<LPARAM>(&m_ttiCell));
		m_tmTT = std::chrono::high_resolution_clock::now();
		SetTimer(m_uIDTCellTTCheck, 300, nullptr); //Timer to check whether mouse has left subitem's rect.
	}
	else {
		KillTimer(m_uIDTCellTTCheck);

		//When hiding tooltip by timer we not nullify the m_htiCurrCell.
		//Otherwise tooltip will be shown again after mouse movement,
		//even if cursor didn't leave current cell area.
		if (!fTimer) {
			m_htiCurrCell.iItem = -1;
			m_htiCurrCell.iSubItem = -1;
		}

		m_fCellTTActive = false;
		m_wndCellTT.SendMessageW(TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&m_ttiCell));
	}
}

void CListEx::TTLinkShow(bool fShow, bool fTimer)
{
	if (fShow) {
		CPoint ptScreen;
		GetCursorPos(&ptScreen);
		ptScreen.Offset(m_ptTTOffset);
		m_wndLinkTT.SendMessageW(TTM_TRACKPOSITION, 0, static_cast<LPARAM>MAKELONG(ptScreen.x, ptScreen.y));
		m_wndLinkTT.SendMessageW(TTM_UPDATETIPTEXTW, 0, reinterpret_cast<LPARAM>(&m_ttiLink));
		m_wndLinkTT.SendMessageW(TTM_TRACKACTIVATE, static_cast<WPARAM>(TRUE), reinterpret_cast<LPARAM>(&m_ttiLink));
		m_tmTT = std::chrono::high_resolution_clock::now();
		SetTimer(m_uIDTLinkTTCheck, 300, nullptr); //Timer to check whether mouse has left link's rect.
	}
	else {
		KillTimer(m_uIDTLinkTTCheck);
		if (!fTimer) {
			m_htiCurrLink.iItem = -1;
			m_htiCurrLink.iSubItem = -1;
			m_rcLinkCurr.SetRectEmpty();
		}

		m_fLinkTTActive = false;
		m_wndLinkTT.SendMessageW(TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&m_ttiLink));
	}
}

void CListEx::TTHLShow(bool fShow, UINT uRow)
{
	if (fShow) {
		CPoint ptScreen;
		GetCursorPos(&ptScreen);
		wchar_t warrOffset[32] { L"Row: " };
		swprintf_s(&warrOffset[5], 24, L"%u", uRow);
		m_ttiHL.lpszText = warrOffset;
		m_wndRowTT.SendMessageW(TTM_TRACKPOSITION, 0, static_cast<LPARAM>(MAKELONG(ptScreen.x - 5, ptScreen.y - 20)));
		m_wndRowTT.SendMessageW(TTM_UPDATETIPTEXTW, 0, reinterpret_cast<LPARAM>(&m_ttiHL));
	}
	m_wndRowTT.SendMessageW(TTM_TRACKACTIVATE, static_cast<WPARAM>(fShow), reinterpret_cast<LPARAM>(&m_ttiHL));
}

auto CListEx::EditSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	UINT_PTR /*uIdSubclass*/, DWORD_PTR dwRefData)->LRESULT
{
	switch (uMsg) {
	case WM_GETDLGCODE:
		return DLGC_WANTALLKEYS;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE || wParam == VK_RETURN) {
			NMHDR hdr;
			hdr.hwndFrom = hWnd;
			hdr.idFrom = dwRefData;
			hdr.code = static_cast<UINT>(wParam);
			::SendMessageW(::GetParent(hWnd), WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&hdr));
		}
		break;
	case WM_NCDESTROY:
		RemoveWindowSubclass(hWnd, EditSubclassProc, 0);
		break;
	default:
		break;
	}

	return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
}