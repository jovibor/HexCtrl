/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
* This is very extended and featured version of CMFCListCtrl class.                     *
* Official git repository: https://github.com/jovibor/ListEx/                           *
* This class is available under the "MIT License".                                      *
****************************************************************************************/
#include "stdafx.h"
#include "CListEx.h"
#include "strsafe.h"
#include <algorithm>
#include <cassert>
#include <limits>
#include <random>

using namespace HEXCTRL::LISTEX;
using namespace HEXCTRL::LISTEX::INTERNAL;

namespace HEXCTRL::LISTEX
{
	/********************************************
	* CreateRawListEx function implementation.	*
	********************************************/
	IListEx* CreateRawListEx()
	{
		return new CListEx();
	}

	namespace INTERNAL
	{
		/********************************************
		* SCOLROWCLR - colors for the row/column.   *
		********************************************/
		struct CListEx::SCOLROWCLR
		{
			LISTEXCOLOR clr { }; //Colors
			std::chrono::high_resolution_clock::time_point time { }; //Time when added.
		};

		/********************************************
		* SITEMTEXT - text and links in the cell.   *
		********************************************/
		struct CListEx::SITEMDATA
		{
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
	}
}

/****************************************************
* CListEx class implementation.						*
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

bool CListEx::Create(const LISTEXCREATESTRUCT& lcs)
{
	assert(!IsCreated());
	if (IsCreated())
		return false;

	auto dwStyle = static_cast<LONG_PTR>(lcs.dwStyle);
	if (lcs.fDialogCtrl)
	{
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

	if (m_fHighLatency) //Tooltip for HighLatency mode.
	{
		if (!m_stWndTtRow.CreateEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr, TTS_NOANIMATE | TTS_NOFADE | TTS_NOPREFIX | TTS_ALWAYSTIP,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, nullptr))
			return false;
		m_stToolInfoRow.cbSize = TTTOOLINFOW_V1_SIZE;
		m_stToolInfoRow.uFlags = TTF_TRACK;
		m_stToolInfoRow.uId = 0x03;
		m_stWndTtRow.SendMessageW(TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&m_stToolInfoRow));
	}

	m_dwGridWidth = lcs.dwListGridWidth;

	LOGFONTW lf;
	if (lcs.pListLogFont)
		lf = *lcs.pListLogFont;
	else
	{
		NONCLIENTMETRICSW ncm { };
		ncm.cbSize = sizeof(NONCLIENTMETRICSW);
		SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
		ncm.lfMessageFont.lfHeight = 18; //For some weird reason above func returns this value as MAX_LONG.
		lf = ncm.lfMessageFont;
	}

	m_lSizeFont = lf.lfHeight;
	m_fontList.CreateFontIndirectW(&lf);
	lf.lfUnderline = 1;
	m_fontListUnderline.CreateFontIndirectW(&lf);
	m_penGrid.CreatePen(PS_SOLID, m_dwGridWidth, m_stColors.clrListGrid);
	m_cursorDefault = static_cast<HCURSOR>(LoadImageW(nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
	m_cursorHand = static_cast<HCURSOR>(LoadImageW(nullptr, IDC_HAND, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
	SetClassLongPtrW(m_hWnd, GCLP_HCURSOR, 0); //To prevent cursor from blinking.

	m_fCreated = true;

	SetHdrHeight(lcs.dwHdrHeight);
	SetHdrFont(lcs.pHdrLogFont);
	GetHeaderCtrl().SetColor(lcs.stColor);
	GetHeaderCtrl().SetSortable(lcs.fSortable);
	Update(0);

	return true;
}

void CListEx::CreateDialogCtrl(UINT uCtrlID, CWnd* pParent)
{
	LISTEXCREATESTRUCT lcs;
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

	const std::wstring_view wstrItem1 = pListCtrl->GetItemText(static_cast<int>(lParam1), iSortColumn).GetString();
	const std::wstring_view wstrItem2 = pListCtrl->GetItemText(static_cast<int>(lParam2), iSortColumn).GetString();

	int iCompare { };
	switch (enSortMode)
	{
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
	if (pListCtrl->GetSortAscending())
	{
		if (iCompare < 0)
			iResult = -1;
		else if (iCompare > 0)
			iResult = 1;
	}
	else
	{
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

	if (it != m_umapCellData.end())
	{
		const auto itInner = it->second.find(iSubItem);

		//If subitem id found.
		if (itInner != it->second.end())
			return itInner->second;
	}

	return 0;
}

LISTEXCOLORS CListEx::GetColors() const
{
	return m_stColors;
}

EListExSortMode CListEx::GetColumnSortMode(int iColumn)const
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

UINT CListEx::GetFontSize()const
{
	assert(IsCreated());
	if (!IsCreated())
		return 0;

	return m_lSizeFont;
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
	if (refHdr.GetHiddenCount() > 0 && nCol >= static_cast<int>(refHdr.GetItemCount() - nHiddenCount))
	{
		nCol = refHdr.GetItemCount() - nHiddenCount;
	}

	const auto iNewIndex = CMFCListCtrl::InsertColumn(nCol, pColumn);

	//Assigning each column a unique internal random identifier.
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<unsigned int> distrib(1, (std::numeric_limits<unsigned int>::max)());

	HDITEMW hdi { };
	hdi.mask = HDI_LPARAM;
	hdi.lParam = distrib(gen);
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
	if (refHdr.GetHiddenCount() > 0 && nCol >= static_cast<int>(refHdr.GetItemCount() - nHiddenCount))
	{
		nCol = refHdr.GetItemCount() - nHiddenCount;
	}

	const auto iNewIndex = CMFCListCtrl::InsertColumn(nCol, lpszColumnHeading, nFormat, nWidth, nSubItem);

	//Assigning each column a unique internal random identifier.
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<unsigned int> distrib(1, (std::numeric_limits<unsigned int>::max)());

	HDITEMW hdi { };
	hdi.mask = HDI_LPARAM;
	hdi.lParam = distrib(gen);
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
	if (const auto it = m_umapCellColor.find(ID); it == m_umapCellColor.end())
	{	//Initializing inner map.
		std::unordered_map<int, LISTEXCOLOR> umapInner { { iSubItem, { clrBk, clrText } } };
		m_umapCellColor.insert({ ID, std::move(umapInner) });
	}
	else
	{
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
	else
	{
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
	else
	{
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
	if (const auto it = m_umapCellTt.find(ID); it == m_umapCellTt.end())
	{
		if (!wstrTooltip.empty() || !wstrCaption.empty())
		{	//Initializing inner map.
			std::unordered_map<int, LISTEXTOOLTIP> umapInner {
				{ iSubItem, { std::wstring { wstrTooltip }, std::wstring { wstrCaption } } }
			};
			m_umapCellTt.insert({ ID, std::move(umapInner) });
		}
	}
	else
	{
		//If there is Item's tooltip but no Subitem's tooltip
		//inserting new Subitem into inner map.
		if (const auto itInner = it->second.find(iSubItem);	itInner == it->second.end())
		{
			if (!wstrTooltip.empty() || !wstrCaption.empty())
				it->second.insert({ iSubItem, { std::wstring { wstrTooltip }, std::wstring { wstrCaption } } });
		}
		else
		{	//If there is already exist this Item-Subitem's tooltip:
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

	//To get WM_MEASUREITEM msg after changing font.
	CRect rc;
	GetWindowRect(&rc);
	WINDOWPOS wp;
	wp.hwnd = m_hWnd;
	wp.cx = rc.Width();
	wp.cy = rc.Height();
	wp.flags = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER;
	SendMessageW(WM_WINDOWPOSCHANGED, 0, reinterpret_cast<LPARAM>(&wp));

	Update(0);
}

void CListEx::SetFontSize(UINT uiSize)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	//Prevent size from being too small or too big.
	if (uiSize < 9 || uiSize > 75)
		return;

	LOGFONTW lf;
	m_fontList.GetLogFont(&lf);
	lf.lfHeight = m_lSizeFont = uiSize;
	m_fontList.DeleteObject();
	m_fontList.CreateFontIndirectW(&lf);
	lf.lfUnderline = 1;
	m_fontListUnderline.DeleteObject();
	m_fontListUnderline.CreateFontIndirectW(&lf);

	//To get WM_MEASUREITEM msg after changing font.
	CRect rc;
	GetWindowRect(&rc);
	WINDOWPOS wp;
	wp.hwnd = m_hWnd;
	wp.cx = rc.Width();
	wp.cy = rc.Height();
	wp.flags = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER;
	SendMessageW(WM_WINDOWPOSCHANGED, 0, reinterpret_cast<LPARAM>(&wp));

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
//Protected methods:
//////////////////////////////////////////////////////////////
void CListEx::InitHeader()
{
	GetHeaderCtrl().SubclassDlgItem(0, this);
}

auto CListEx::HasColor(int iItem, int iSubItem)->std::optional<PLISTEXCOLOR>
{
	if (iItem < 0 || iSubItem < 0)
		return std::nullopt;

	std::optional<PLISTEXCOLOR> opt { };
	if (m_fVirtual) //In Virtual mode asking parent for color.
	{
		const auto iCtrlID = GetDlgCtrlID();
		NMITEMACTIVATE nmii { { m_hWnd, static_cast<UINT>(iCtrlID), LISTEX_MSG_GETCOLOR } };
		nmii.iItem = iItem;
		nmii.iSubItem = iSubItem;
		GetParent()->SendMessageW(WM_NOTIFY, static_cast<WPARAM>(iCtrlID), reinterpret_cast<LPARAM>(&nmii));
		if (nmii.lParam != 0)
			opt = reinterpret_cast<PLISTEXCOLOR>(nmii.lParam);
	}
	else
	{
		const auto ID = MapIndexToID(static_cast<UINT>(iItem));
		if (const auto itItem = m_umapCellColor.find(ID); itItem != m_umapCellColor.end())
		{
			const auto itSubItem = itItem->second.find(iSubItem);

			//If subitem id found.
			if (itSubItem != itItem->second.end())
				opt = &itSubItem->second;
		}

		if (!opt)
		{
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
	if (m_fVirtual) //In Virtual mode asking parent for tooltip.
	{
		const auto iCtrlID = GetDlgCtrlID();
		NMITEMACTIVATE nmii { { m_hWnd, static_cast<UINT>(iCtrlID), LISTEX_MSG_GETTOOLTIP } };
		nmii.iItem = iItem;
		nmii.iSubItem = iSubItem;
		GetParent()->SendMessageW(WM_NOTIFY, static_cast<WPARAM>(iCtrlID), reinterpret_cast<LPARAM>(&nmii));
		if (nmii.lParam != 0)
			opt = reinterpret_cast<PLISTEXTOOLTIP>(nmii.lParam);
	}
	else
	{
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

	if (m_fVirtual) //In Virtual List mode asking parent for the icon index in image list.
	{
		const auto uCtrlID = static_cast<UINT>(GetDlgCtrlID());
		NMITEMACTIVATE nmii { { m_hWnd, uCtrlID, LISTEX_MSG_GETICON } };
		nmii.iItem = iItem;
		nmii.iSubItem = iSubItem;
		nmii.lParam = -1; //Default, no icon.
		GetParent()->SendMessageW(WM_NOTIFY, static_cast<WPARAM>(uCtrlID), reinterpret_cast<LPARAM>(&nmii));
		iRet = static_cast<int>(nmii.lParam);
	}
	else
	{
		const auto ID = MapIndexToID(iItem);
		if (const auto it = m_umapCellIcon.find(ID); it != m_umapCellIcon.end())
			if (const auto itInner = it->second.find(iSubItem); itInner != it->second.end())
				iRet = itInner->second;
	}

	return iRet;
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

	if (const auto iIndex = HasIcon(iItem, iSubitem); iIndex > -1) //If cell has icon.
	{
		IMAGEINFO stIMG;
		GetImageList(LVSIL_NORMAL)->GetImageInfo(iIndex, &stIMG);
		vecData.emplace_back(iIndex, rcTextOrig);
		rcTextOrig.left += (stIMG.rcImage.right - stIMG.rcImage.left) + iIndentRc; //Indent a rect for image width.
	}

	size_t nPosCurr { 0 }; //Current position in the parsed string.
	CRect rcTextCurr { };  //Current rect.

	while (nPosCurr != std::wstring_view::npos)
	{
		constexpr std::wstring_view wstrTagLink { L"<link=" };
		constexpr std::wstring_view wstrTagFirstClose { L">" };
		constexpr std::wstring_view wstrTagLast { L"</link>" };
		constexpr std::wstring_view wstrTagTitle { L"title=" };
		constexpr std::wstring_view wstrQuote { L"\"" };

		//Searching the string for a <link=...></link> pattern.
		if (size_t nPosTagLink { }, //Start position of the opening tag "<link=".
			nPosLinkOpenQuote { },  //Position of the (link="<-) open quote.
			nPosLinkCloseQuote { }, //Position of the (link=""<-) close quote.
			nPosTagFirstClose { },  //Start position of the opening tag's closing bracket ">".
			nPosTagLast { };        //Start position of the enclosing tag "</link>".
			(nPosTagLink = wstrText.find(wstrTagLink, nPosCurr)) != std::wstring_view::npos
			&& (nPosLinkOpenQuote = wstrText.find(wstrQuote, nPosTagLink)) != std::wstring_view::npos
			&& (nPosLinkCloseQuote = wstrText.find(wstrQuote, nPosLinkOpenQuote + wstrQuote.size())) != std::wstring_view::npos
			&& (nPosTagFirstClose = wstrText.find(wstrTagFirstClose, nPosLinkCloseQuote + wstrQuote.size())) != std::wstring_view::npos
			&& (nPosTagLast = wstrText.find(wstrTagLast, nPosTagFirstClose + wstrTagFirstClose.size())) != std::wstring_view::npos)
		{
			auto pDC = GetDC();
			pDC->SelectObject(m_fontList);
			SIZE size;

			//Any text before found tag.
			if (nPosTagLink > nPosCurr)
			{
				auto wstrTextBefore = wstrText.substr(nPosCurr, nPosTagLink - nPosCurr);
				GetTextExtentPoint32W(pDC->m_hDC, wstrTextBefore.data(), static_cast<int>(wstrTextBefore.size()), &size);
				if (rcTextCurr.IsRectNull())
					rcTextCurr.SetRect(rcTextOrig.left, rcTextOrig.top, rcTextOrig.left + size.cx, rcTextOrig.bottom);
				else
				{
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
			else
			{
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

			if (size_t nPosTagTitle { }, //Position of the (title=) tag beginning.
				nPosTitleOpenQuote { },  //Position of the (title="<-) opening quote.
				nPosTitleCloseQuote { }; //Position of the (title=""<-) closing equote.
				(nPosTagTitle = wstrText.find(wstrTagTitle, nPosCurr)) != std::wstring_view::npos
				&& (nPosTitleOpenQuote = wstrText.find(wstrQuote, nPosTagTitle)) != std::wstring_view::npos
				&& (nPosTitleCloseQuote = wstrText.find(wstrQuote, nPosTitleOpenQuote + wstrQuote.size())) != std::wstring_view::npos)
			{
				//Title tag text between quotes: <...title="textFromHere">
				wstrTextTitle = wstrText.substr(nPosTitleOpenQuote + wstrQuote.size(),
					nPosTitleCloseQuote - nPosTitleOpenQuote - wstrQuote.size());
				fTitle = true;
			}

			vecData.emplace_back(wstrTextBetweenTags, wstrTextLink, wstrTextTitle, rcTextCurr, true, fTitle);
			nPosCurr = nPosTagLast + wstrTagLast.size();
		}
		else
		{
			auto wstrTextAfter = wstrText.substr(nPosCurr, wstrText.size() - nPosCurr);

			if (rcTextCurr.IsRectNull())
				rcTextCurr = rcTextOrig;
			else
			{
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
	if (fShow)
	{
		CPoint ptScreen;
		GetCursorPos(&ptScreen);

		static wchar_t warrOffset[32] { L"Row: " };
		swprintf_s(&warrOffset[5], 24, L"%u", uRow);
		m_stToolInfoRow.lpszText = warrOffset;
		m_stWndTtRow.SendMessageW(TTM_TRACKPOSITION, 0, static_cast<LPARAM>(MAKELONG(ptScreen.x - 5, ptScreen.y - 20)));
		m_stWndTtRow.SendMessageW(TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(&m_stToolInfoRow));
	}
	m_stWndTtRow.SendMessageW(TTM_TRACKACTIVATE, static_cast<WPARAM>(fShow), reinterpret_cast<LPARAM>(&m_stToolInfoRow));
}

void CListEx::MeasureItem(LPMEASUREITEMSTRUCT lpMIS)
{
	//Set row height according to current font's height.
	TEXTMETRICW tm;
	CDC* pDC = GetDC();
	pDC->SelectObject(&m_fontList);
	GetTextMetricsW(pDC->m_hDC, &tm);
	lpMIS->itemHeight = tm.tmHeight + tm.tmExternalLeading + 1;
	ReleaseDC(pDC);
}

void CListEx::DrawItem(LPDRAWITEMSTRUCT pDIS)
{
	if (pDIS->itemID == -1)
		return;

	switch (pDIS->itemAction)
	{
	case ODA_SELECT:
	case ODA_DRAWENTIRE:
	{
		const auto pDC = CDC::FromHandle(pDIS->hDC);
		const auto clrBkCurrRow = (pDIS->itemID % 2) ? m_stColors.clrListBkRow2 : m_stColors.clrListBkRow1;
		const auto iColumns = GetHeaderCtrl().GetItemCount();

		for (auto iColumn = 0; iColumn < iColumns; ++iColumn)
		{
			if (GetHeaderCtrl().IsColumnHidden(iColumn))
				continue;

			COLORREF clrText;
			COLORREF clrBk;
			COLORREF clrTextLink;

			//Subitems' draw routine. 
			//Colors depending on whether subitem selected or not, and has tooltip or not.
			if (pDIS->itemState & ODS_SELECTED)
			{
				clrText = m_stColors.clrListTextSel;
				clrBk = m_stColors.clrListBkSel;
				clrTextLink = m_stColors.clrListTextLinkSel;
			}
			else
			{
				clrTextLink = m_stColors.clrListTextLink;

				if (const auto opt = HasColor(pDIS->itemID, iColumn); opt)
				{
					clrText = (*opt)->clrText;
					clrBk = (*opt)->clrBk;
				}
				else
				{
					if (HasTooltip(pDIS->itemID, iColumn))
					{
						clrText = m_stColors.clrListTextCellTt;
						clrBk = m_stColors.clrListBkCellTt;
					}
					else
					{
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

			for (const auto& iter : ParseItemText(pDIS->itemID, iColumn))
			{
				if (iter.iIconIndex > -1)
				{
					GetImageList(LVSIL_NORMAL)->DrawEx(
						pDC, iter.iIconIndex, { iter.rect.left, iter.rect.top }, { }, CLR_NONE, CLR_NONE, ILD_NORMAL);
					continue;
				}
				if (iter.fLink)
				{
					pDC->SetTextColor(clrTextLink);
					if (m_fLinksUnderline)
						pDC->SelectObject(m_fontListUnderline);
				}
				else
				{
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

BOOL CListEx::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (nFlags == MK_CONTROL)
	{
		SetFontSize(GetFontSize() + zDelta / WHEEL_DELTA * 2);
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
	if (const auto iterFind = std::find_if(vecText.begin(), vecText.end(), [&](SITEMDATA& iter)
		{ return iter.fLink && iter.rect.PtInRect(pt); }); iterFind != vecText.end())
	{
		fLink = true;
		if (m_fLinkTooltip && !m_fLDownAtLink && m_rcLinkCurr != iterFind->rect)
		{
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
	if (fLink)
	{
		//If there is cell's tooltip atm hide it.
		if (m_fTtCellShown)
			TtCellHide();

		return; //Do not process further, cursor is on the link's rect.
	}

	m_fLDownAtLink = false;

	//If there was link's tool-tip shown, hide it.
	if (m_fTtLinkShown)
		TtLinkHide();

	if (auto opt = HasTooltip(hi.iItem, hi.iSubItem); opt)
	{
		//Check if cursor is still in the same cell's rect. If so - just leave.
		if (m_stCurrCell.iItem != hi.iItem || m_stCurrCell.iSubItem != hi.iSubItem)
		{
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
	else
	{
		if (m_fTtCellShown)
			TtCellHide();
		else
		{
			m_stCurrCell.iItem = -1;
			m_stCurrCell.iSubItem = -1;
		}
	}
}

void CListEx::OnLButtonDown(UINT nFlags, CPoint pt)
{
	bool fLinkDown { false };
	LVHITTESTINFO hi { pt };
	if (SubItemHitTest(&hi) != -1)
	{
		auto vecText = ParseItemText(hi.iItem, hi.iSubItem);
		if (const auto iterFind = std::find_if(vecText.begin(), vecText.end(), [&](SITEMDATA& iter)
			{ return iter.fLink && iter.rect.PtInRect(pt); }); iterFind != vecText.end())
		{
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
	if (m_fLDownAtLink)
	{
		LVHITTESTINFO hi { };
		hi.pt = pt;
		ListView_SubItemHitTest(m_hWnd, &hi);
		if (hi.iSubItem == -1 || hi.iItem == -1)
		{
			m_fLDownAtLink = false;
			return;
		}

		auto vecText = ParseItemText(hi.iItem, hi.iSubItem);
		if (const auto iterFind = std::find_if(vecText.begin(), vecText.end(), [&](SITEMDATA& iter)
			{ return iter.fLink && iter.rect == m_rcLinkCurr; }); iterFind != vecText.end())
		{
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
	switch (nIDEvent)
	{
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
		if (m_rcLinkCurr.PtInRect(ptClient))
		{
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
	//To avoid flickering.
	//Drawing to CMemDC, excluding list header area (rcHdr).
	CRect rcClient, rcHdr;
	GetClientRect(&rcClient);
	GetHeaderCtrl().GetClientRect(rcHdr);
	rcClient.top += rcHdr.Height();

	CPaintDC dc(this);
	CMemDC memDC(dc, rcClient);
	CDC& rDC = memDC.GetDC();
	rDC.GetClipBox(&rcClient);
	rDC.FillSolidRect(rcClient, m_stColors.clrNWABk);

	DefWindowProcW(WM_PAINT, reinterpret_cast<WPARAM>(rDC.m_hDC), static_cast<LPARAM>(0));
}

void CListEx::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (m_fVirtual && m_fHighLatency)
	{
		static bool flag { };
		static UINT uItem;
		if (nSBCode != SB_THUMBTRACK)
		{
			//If there was SB_THUMBTRACK message previously, calculate the scroll amount (up/down)
			//by multiplying item's row height by difference between current (top) and nPos row.
			//Scroll may be negative therefore.
			if (flag)
			{
				CRect rc;
				GetItemRect(uItem, rc, LVIR_LABEL);
				CSize size(0, (uItem - GetTopIndex()) * rc.Height());
				Scroll(size);
				flag = false;
			}
			TtRowShow(false, 0);
			CMFCListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
		}
		else
		{
			flag = true;
			SCROLLINFO si { };
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_ALL;
			GetScrollInfo(SB_VERT, &si);
			uItem = si.nTrackPos; //si.nTrackPos is in fact a row number.
			TtRowShow(true, uItem);
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
	if (m_fSortable && (pNMLV->hdr.code == HDN_ITEMCLICKW || pNMLV->hdr.code == HDN_ITEMCLICKA))
	{
		if (IsColumnSortable(pNMLV->iItem))
		{
			m_iSortColumn = pNMLV->iItem;
			m_fSortAscending = pNMLV->iItem == m_iSortColumn ? !m_fSortAscending : true;
			GetHeaderCtrl().SetSortArrow(m_iSortColumn, m_fSortAscending);
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