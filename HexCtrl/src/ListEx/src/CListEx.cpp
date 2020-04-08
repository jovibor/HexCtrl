/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is very extended and featured version of CMFCListCtrl class.                     *
* Official git repository: https://github.com/jovibor/ListEx/                           *
* This class is available under the "MIT License".                                      *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#include "stdafx.h"
#include "CListEx.h"
#include "strsafe.h"
#include <cassert>

using namespace HEXCTRL::INTERNAL::LISTEX;
using namespace HEXCTRL::INTERNAL::LISTEX::INTERNAL;

namespace HEXCTRL::INTERNAL::LISTEX {
	/********************************************
	* CreateRawListEx function implementation.	*
	********************************************/
	IListEx* CreateRawListEx()
	{
		return new CListEx();
	}

	namespace INTERNAL {
		constexpr ULONG_PTR ID_TIMER_TOOLTIP { 0x01 }; //Timer ID.
	}
}

/****************************************************
* CListEx class implementation.						*
****************************************************/
IMPLEMENT_DYNAMIC(CListEx, CMFCListCtrl)

BEGIN_MESSAGE_MAP(CListEx, CMFCListCtrl)
	ON_WM_SETCURSOR()
	ON_WM_KILLFOCUS()
	ON_WM_HSCROLL()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_TIMER()
	ON_WM_ERASEBKGND()
	ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
	ON_WM_PAINT()
	ON_WM_MEASUREITEM_REFLECT()
	ON_NOTIFY(HDN_DIVIDERDBLCLICKA, 0, &CListEx::OnHdnDividerdblclick)
	ON_NOTIFY(HDN_DIVIDERDBLCLICKW, 0, &CListEx::OnHdnDividerdblclick)
	ON_NOTIFY(HDN_BEGINTRACKA, 0, &CListEx::OnHdnBegintrack)
	ON_NOTIFY(HDN_BEGINTRACKW, 0, &CListEx::OnHdnBegintrack)
	ON_NOTIFY(HDN_TRACKA, 0, &CListEx::OnHdnTrack)
	ON_NOTIFY(HDN_TRACKW, 0, &CListEx::OnHdnTrack)
	ON_WM_CONTEXTMENU()
	ON_WM_DESTROY()
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, &CListEx::OnLvnColumnClick)
END_MESSAGE_MAP()

bool CListEx::Create(const LISTEXCREATESTRUCT& lcs)
{
	assert(!IsCreated());
	if (IsCreated())
		return false;

	auto dwStyle = static_cast<LONG_PTR>(lcs.dwStyle);
	if (lcs.fDialogCtrl)
	{
		SubclassDlgItem(lcs.uID, lcs.pwndParent);
		dwStyle = GetWindowLongPtrW(m_hWnd, GWL_STYLE);
		SetWindowLongPtrW(m_hWnd, GWL_STYLE, dwStyle | LVS_OWNERDRAWFIXED | LVS_REPORT);
	}
	else if (!CMFCListCtrl::Create(lcs.dwStyle | WS_CHILD | WS_VISIBLE | LVS_OWNERDRAWFIXED | LVS_REPORT,
		lcs.rect, lcs.pwndParent, lcs.uID))
		return false;

	m_fVirtual = dwStyle & LVS_OWNERDATA;
	m_stColor = lcs.stColor;
	m_fSortable = lcs.fSortable;

	if (!m_wndTt.CreateEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr, TTS_BALLOON | TTS_NOANIMATE | TTS_NOFADE | TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr))
		return false;

	SetWindowTheme(m_wndTt, nullptr, L""); //To prevent Windows from changing theme of Balloon window.

	m_stToolInfo.cbSize = TTTOOLINFOW_V1_SIZE;
	m_stToolInfo.uFlags = TTF_TRACK;
	m_stToolInfo.uId = 0x1;
	m_wndTt.SendMessageW(TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&m_stToolInfo));
	m_wndTt.SendMessageW(TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(400)); //to allow use of newline \n.
	m_wndTt.SendMessageW(TTM_SETTIPTEXTCOLOR, static_cast<WPARAM>(m_stColor.clrTooltipText), 0);
	m_wndTt.SendMessageW(TTM_SETTIPBKCOLOR, static_cast<WPARAM>(m_stColor.clrTooltipBk), 0);

	m_dwGridWidth = lcs.dwListGridWidth;
	m_stNMII.hdr.idFrom = GetDlgCtrlID();
	m_stNMII.hdr.hwndFrom = m_hWnd;

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
	m_penGrid.CreatePen(PS_SOLID, m_dwGridWidth, m_stColor.clrListGrid);
	m_fCreated = true;

	SetHdrHeight(lcs.dwHdrHeight);
	SetHdrFont(lcs.pHdrLogFont);
	GetHeaderCtrl().SetColor(lcs.stColor);
	GetHeaderCtrl().SetSortable(lcs.fSortable);
	Update(0);

	return true;
}

void CListEx::CreateDialogCtrl(UINT uCtrlID, CWnd* pwndDlg)
{
	LISTEXCREATESTRUCT lcs;
	lcs.pwndParent = pwndDlg;
	lcs.uID = uCtrlID;
	lcs.fDialogCtrl = true;

	Create(lcs);
}

int CALLBACK CListEx::DefCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	auto pListCtrl = reinterpret_cast<IListEx*>(lParamSort);
	auto iSortColumn = pListCtrl->GetSortColumn();
	auto enSortMode = pListCtrl->GetColumnSortMode(iSortColumn);

	std::wstring_view wstrItem1 = pListCtrl->GetItemText(static_cast<int>(lParam1), iSortColumn).GetString();
	std::wstring_view wstrItem2 = pListCtrl->GetItemText(static_cast<int>(lParam2), iSortColumn).GetString();

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

		if ((llData1 - llData2) < 0)
			iCompare = -1;
		else if ((llData1 - llData2) > 0)
			iCompare = 1;
	}
	break;
	}

	int result = 0;
	if (pListCtrl->GetSortAscending())
	{
		if (iCompare < 0)
			result = -1;
		else if (iCompare > 0)
			result = 1;
	}
	else
	{
		if (iCompare < 0)
			result = 1;
		else if (iCompare > 0)
			result = -1;
	}

	return result;
}

BOOL CListEx::DeleteAllItems()
{
	assert(IsCreated());
	if (!IsCreated())
		return FALSE;

	m_umapCellTt.clear();
	m_umapCellMenu.clear();
	m_umapCellData.clear();
	m_umapCellColor.clear();
	m_umapRowColor.clear();

	return CMFCListCtrl::DeleteAllItems();
}

BOOL CListEx::DeleteColumn(int nCol)
{
	assert(IsCreated());
	if (!IsCreated())
		return FALSE;

	if (auto iter = m_umapColumnColor.find(nCol); iter != m_umapColumnColor.end())
		m_umapColumnColor.erase(iter);
	if (auto iter = m_umapColumnSortMode.find(nCol); iter != m_umapColumnSortMode.end())
		m_umapColumnSortMode.erase(iter);

	return CMFCListCtrl::DeleteColumn(nCol);
}

BOOL CListEx::DeleteItem(int iItem)
{
	assert(IsCreated());
	if (!IsCreated())
		return FALSE;

	UINT ID = MapIndexToID(iItem);

	if (auto iter = m_umapCellTt.find(ID); iter != m_umapCellTt.end())
		m_umapCellTt.erase(iter);
	if (auto iter = m_umapCellMenu.find(ID); iter != m_umapCellMenu.end())
		m_umapCellMenu.erase(iter);
	if (auto iter = m_umapCellData.find(ID); iter != m_umapCellData.end())
		m_umapCellData.erase(iter);
	if (auto iter = m_umapCellColor.find(ID); iter != m_umapCellColor.end())
		m_umapCellColor.erase(iter);
	if (auto iter = m_umapRowColor.find(ID); iter != m_umapRowColor.end())
		m_umapRowColor.erase(iter);

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

	UINT ID = MapIndexToID(iItem);
	auto it = m_umapCellData.find(ID);

	if (it != m_umapCellData.end())
	{
		auto itInner = it->second.find(iSubItem);

		//If subitem id found.
		if (itInner != it->second.end())
			return itInner->second;
	}

	return 0;
}

EListExSortMode CListEx::GetColumnSortMode(int iColumn)const
{
	assert(IsCreated());

	EListExSortMode enMode;
	auto iter = m_umapColumnSortMode.find(iColumn);
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

bool CListEx::IsCreated()const
{
	return m_fCreated;
}

UINT CListEx::MapIndexToID(UINT nItem)const
{
	UINT ID;
	//In case of virtual list the client code is responsible for
	//mapping indexes to unique IDs.
	//The unique ID is set in NMITEMACTIVATE::lParam by client.
	if (m_fVirtual)
	{
		UINT uCtrlId = static_cast<UINT>(GetDlgCtrlID());
		NMITEMACTIVATE nmii { { m_hWnd, uCtrlId, LVM_MAPINDEXTOID } };
		nmii.iItem = static_cast<int>(nItem);
		GetParent()->SendMessageW(WM_NOTIFY, static_cast<WPARAM>(uCtrlId), reinterpret_cast<LPARAM>(&nmii));
		ID = static_cast<UINT>(nmii.lParam);
	}
	else
		ID = CMFCListCtrl::MapIndexToID(nItem);

	return ID;
}

void CListEx::SetCellColor(int iItem, int iSubItem, COLORREF clrBk, COLORREF clrText)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	if (clrText == -1) //-1 for default color.
		clrText = m_stColor.clrListText;

	UINT ID = MapIndexToID(static_cast<UINT>(iItem));
	auto it = m_umapCellColor.find(ID);

	//If there is no color for such item/subitem we just set it.
	if (it == m_umapCellColor.end())
	{	//Initializing inner map.
		std::unordered_map<int, LISTEXCELLCOLOR> umapInner { { iSubItem, LISTEXCELLCOLOR { clrBk, clrText } } };
		m_umapCellColor.insert({ ID, std::move(umapInner) });
	}
	else
	{
		auto itInner = it->second.find(iSubItem);

		if (itInner == it->second.end())
			it->second.insert({ iSubItem, LISTEXCELLCOLOR { clrBk, clrText } });
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
	if (!IsCreated())
		return;

	UINT ID = MapIndexToID(iItem);
	auto it = m_umapCellData.find(ID);

	//If there is no data for such item/subitem we just set it.
	if (it == m_umapCellData.end())
	{	//Initializing inner map.
		std::unordered_map<int, ULONGLONG> umapInner { { iSubItem, ullData } };
		m_umapCellData.insert({ ID, std::move(umapInner) });
	}
	else
	{
		auto itInner = it->second.find(iSubItem);

		if (itInner == it->second.end())
			it->second.insert({ iSubItem, ullData });
		else //If there is already exist this cell's data -> changing.
			itInner->second = ullData;
	}
}

void CListEx::SetCellMenu(int iItem, int iSubItem, CMenu* pMenu)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	UINT ID = MapIndexToID(iItem);
	auto it = m_umapCellMenu.find(ID);

	//If there is no menu for such item/subitem we just set it.
	if (it == m_umapCellMenu.end())
	{	//Initializing inner map.
		std::unordered_map<int, CMenu*> umapInner { { iSubItem, pMenu } };
		m_umapCellMenu.insert({ ID, std::move(umapInner) });
	}
	else
	{
		auto itInner = it->second.find(iSubItem);

		//If there is Item's menu but no Subitem's menu
		//inserting new Subitem into inner map.
		if (itInner == it->second.end())
			it->second.insert({ iSubItem, pMenu });
		else //If there is already exist this cell's menu -> changing.
			itInner->second = pMenu;
	}
}

void CListEx::SetCellTooltip(int iItem, int iSubItem, std::wstring_view wstrTooltip, std::wstring_view wstrCaption)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	UINT ID = MapIndexToID(iItem);
	auto it = m_umapCellTt.find(ID);

	//If there is no tooltip for such item/subitem we just set it.
	if (it == m_umapCellTt.end())
	{
		if (!wstrTooltip.empty() || !wstrCaption.empty())
		{	//Initializing inner map.
			std::unordered_map<int, CELLTOOLTIP> umapInner {
				{ iSubItem, { CELLTOOLTIP { std::move(std::wstring { wstrTooltip }),
				std::move(std::wstring { wstrCaption }) } } } };
			m_umapCellTt.insert({ ID, std::move(umapInner) });
		}
	}
	else
	{
		auto itInner = it->second.find(iSubItem);

		//If there is Item's tooltip but no Subitem's tooltip
		//inserting new Subitem into inner map.
		if (itInner == it->second.end())
		{
			if (!wstrTooltip.empty() || !wstrCaption.empty())
				it->second.insert({ iSubItem, { std::move(std::wstring { wstrTooltip }),
					std::move(std::wstring { wstrCaption }) } });
		}
		else
		{	//If there is already exist this Item-Subitem's tooltip:
			//change or erase it, depending on pwszTooltip emptiness.
			if (!wstrTooltip.empty())
				itInner->second = { std::move(std::wstring { wstrTooltip }),
				std::move(std::wstring { wstrCaption }) };
			else
				it->second.erase(itInner);
		}
	}
}

void CListEx::SetColor(const LISTEXCOLORS& lcs)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_stColor = lcs;
	GetHeaderCtrl().SetColor(lcs);
	RedrawWindow();
}

void CListEx::SetColumnColor(int iColumn, COLORREF clrBk, COLORREF clrText)
{
	if (clrText == -1) //-1 for default color.
		clrText = m_stColor.clrListText;

	m_umapColumnColor[iColumn] = COLUMNCOLOR { clrBk, clrText, std::chrono::high_resolution_clock::now() };
}

void CListEx::SetColumnSortMode(int iColumn, EListExSortMode enSortMode)
{
	m_umapColumnSortMode[iColumn] = enSortMode;
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

void CListEx::SetListMenu(CMenu* pMenu)
{
	assert(IsCreated());
	assert(pMenu);
	if (!IsCreated() || !pMenu)
		return;

	m_pListMenu = pMenu;
}

void CListEx::SetRowColor(DWORD dwRow, COLORREF clrBk, COLORREF clrText)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	if (clrText == -1) //-1 for default color.
		clrText = m_stColor.clrListText;

	m_umapRowColor[dwRow] = ROWCOLOR { clrBk, clrText, std::chrono::high_resolution_clock::now() };
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

bool CListEx::HasCellColor(int iItem, int iSubItem, COLORREF& clrBk, COLORREF& clrText)
{
	if (iItem < 0 || iSubItem < 0)
		return false;

	bool fHasColor { false };

	//If parent responds for LISTEX_MSG_CELLCOLOR message, we use lParam
	//as a pointer to LISTEXCELLCOLOR. Otherwise we doing inner lookup.
	auto iCtrlID = GetDlgCtrlID();
	NMITEMACTIVATE nmii { { m_hWnd, static_cast<UINT>(iCtrlID), LISTEX_MSG_CELLCOLOR } };
	nmii.iItem = iItem;
	nmii.iSubItem = iSubItem;
	GetParent()->SendMessageW(WM_NOTIFY, static_cast<WPARAM>(iCtrlID), reinterpret_cast<LPARAM>(&nmii));
	if (nmii.lParam != 0)
	{
		auto pClr = reinterpret_cast<PLISTEXCELLCOLOR>(nmii.lParam);
		clrBk = pClr->clrBk;
		clrText = pClr->clrText;

		fHasColor = true;
	}

	if (!fHasColor)
	{
		UINT ID = MapIndexToID(static_cast<UINT>(iItem));
		auto it = m_umapCellColor.find(ID);

		if (it != m_umapCellColor.end())
		{
			auto itInner = it->second.find(iSubItem);

			//If subitem id found.
			if (itInner != it->second.end())
			{
				clrBk = itInner->second.clrBk;
				clrText = itInner->second.clrText;
				fHasColor = true;
			}
		}

		if (!fHasColor)
		{
			auto itColumn = m_umapColumnColor.find(iSubItem);
			auto itRow = m_umapRowColor.find(ID);

			if (itColumn != m_umapColumnColor.end() && itRow != m_umapRowColor.end())
			{
				clrBk = itColumn->second.time > itRow->second.time ? itColumn->second.clrBk : itRow->second.clrBk;
				clrText = itColumn->second.time > itRow->second.time ? itColumn->second.clrText : itRow->second.clrText;
				fHasColor = true;
			}
			else if (itColumn != m_umapColumnColor.end())
			{
				clrBk = itColumn->second.clrBk;
				clrText = itColumn->second.clrText;
				fHasColor = true;
			}
			else if (itRow != m_umapRowColor.end())
			{
				clrBk = itRow->second.clrBk;
				clrText = itRow->second.clrText;
				fHasColor = true;
			}
		}
	}

	return fHasColor;
}

bool CListEx::HasTooltip(int iItem, int iSubItem, std::wstring** ppwstrText, std::wstring** ppwstrCaption)
{
	if (iItem < 0 || iSubItem < 0)
		return false;

	//Can return true/false indicating if subitem has tooltip,
	//or can return pointers to tooltip text as well, if poiters are not nullptr.
	UINT ID = MapIndexToID(iItem);
	auto it = m_umapCellTt.find(ID);

	if (it != m_umapCellTt.end())
	{
		auto itInner = it->second.find(iSubItem);

		//If subitem id found and its text is not empty.
		if (itInner != it->second.end() && !itInner->second.wstrText.empty())
		{
			//If pointer for text is nullptr we just return true.
			if (ppwstrText)
			{
				*ppwstrText = &itInner->second.wstrText;
				if (ppwstrCaption)
					*ppwstrCaption = &itInner->second.wstrCaption;
			}
			return true;
		}
	}

	return false;
}

bool CListEx::HasMenu(int iItem, int iSubItem, CMenu** ppMenu)
{
	bool fHasMenu { false };

	if (iItem < 0 || iSubItem < 0)
	{
		if (m_pListMenu)
		{
			if (ppMenu)
				*ppMenu = m_pListMenu;
			fHasMenu = true;
		}
	}
	else
	{
		UINT ID = MapIndexToID(iItem);
		auto it = m_umapCellMenu.find(ID);

		if (it != m_umapCellMenu.end())
		{
			auto itInner = it->second.find(iSubItem);

			//If subitem id found.
			if (itInner != it->second.end())
			{
				if (ppMenu)
					*ppMenu = itInner->second;
				fHasMenu = true;
			}
		}
		else if (m_pListMenu) //If there is no menu for cell, then checking global menu for the list.
		{
			if (ppMenu)
				*ppMenu = m_pListMenu;
			fHasMenu = true;
		}
	}

	return fHasMenu;
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

	CDC* pDC = CDC::FromHandle(pDIS->hDC);
	pDC->SelectObject(m_penGrid);
	pDC->SelectObject(m_fontList);
	COLORREF clrBkCurrRow = (pDIS->itemID % 2) ? m_stColor.clrListBkRow2 : m_stColor.clrListBkRow1;

	switch (pDIS->itemAction)
	{
	case ODA_SELECT:
	case ODA_DRAWENTIRE:
	{
		for (int i = 0; i < GetHeaderCtrl().GetItemCount(); i++)
		{
			COLORREF clrText, clrBk;
			//Subitems' draw routine. Colors depending on whether subitem selected or not,
			//and has tooltip or not.
			if (pDIS->itemState & ODS_SELECTED)
			{
				clrText = m_stColor.clrListTextSelected;
				clrBk = m_stColor.clrListBkSelected;
			}
			else
			{
				if (!HasCellColor(pDIS->itemID, i, clrBk, clrText))
				{
					if (HasTooltip(pDIS->itemID, i))
					{
						clrText = m_stColor.clrListTextCellTt;
						clrBk = m_stColor.clrListBkCellTt;
					}
					else
					{
						clrText = m_stColor.clrListText;
						clrBk = clrBkCurrRow;
					}
				}
			}
			CRect rcBounds;
			GetSubItemRect(pDIS->itemID, i, LVIR_BOUNDS, rcBounds);
			pDC->FillSolidRect(&rcBounds, clrBk);

			CRect rcText;
			GetSubItemRect(pDIS->itemID, i, LVIR_LABEL, rcText);
			if (i != 0) //Not needed for item itself (not subitem).
				rcText.left += 4;
			CStringW wstrSubitem = GetItemText(pDIS->itemID, i);
			pDC->SetTextColor(clrText);
			ExtTextOutW(pDC->m_hDC, rcText.left, rcText.top, ETO_CLIPPED, rcText, wstrSubitem, wstrSubitem.GetLength(), nullptr);

			//Drawing Subitem's rect lines. 
			pDC->MoveTo(rcBounds.left, rcBounds.top);
			pDC->LineTo(rcBounds.right, rcBounds.top);
			pDC->MoveTo(rcBounds.left, rcBounds.top);
			pDC->LineTo(rcBounds.left, rcBounds.bottom);
			pDC->MoveTo(rcBounds.left, rcBounds.bottom);
			pDC->LineTo(rcBounds.right, rcBounds.bottom);
			pDC->MoveTo(rcBounds.right, rcBounds.top);
			pDC->LineTo(rcBounds.right, rcBounds.bottom);
		}
	}
	break;
	case ODA_FOCUS:
		break;
	}
}

void CListEx::OnMouseMove(UINT /*nFlags*/, CPoint pt)
{
	LVHITTESTINFO hi { };
	hi.pt = pt;
	ListView_SubItemHitTest(m_hWnd, &hi);
	std::wstring  *pwstrTt { }, *pwstrCaption { };

	if (HasTooltip(hi.iItem, hi.iSubItem, &pwstrTt, &pwstrCaption))
	{
		//Check if cursor is still in the same cell's rect. If so - just leave.
		if (m_stCurrCell.iItem == hi.iItem && m_stCurrCell.iSubItem == hi.iSubItem)
			return;

		m_fTtShown = true;
		m_stCurrCell.iItem = hi.iItem;
		m_stCurrCell.iSubItem = hi.iSubItem;
		m_stToolInfo.lpszText = const_cast<LPWSTR>(pwstrTt->data());

		ClientToScreen(&pt);
		m_wndTt.SendMessageW(TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(pt.x, pt.y));
		m_wndTt.SendMessageW(TTM_SETTITLE, static_cast<WPARAM>(TTI_NONE), reinterpret_cast<LPARAM>(pwstrCaption->data()));
		m_wndTt.SendMessageW(TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(&m_stToolInfo));
		m_wndTt.SendMessageW(TTM_TRACKACTIVATE, static_cast<WPARAM>(TRUE), reinterpret_cast<LPARAM>(&m_stToolInfo));

		//Timer to check whether mouse left subitem's rect.
		SetTimer(ID_TIMER_TOOLTIP, 200, nullptr);
	}
	else
	{
		m_stCurrCell.iItem = hi.iItem;
		m_stCurrCell.iSubItem = hi.iSubItem;

		//If there is shown tooltip window.
		if (m_fTtShown)
		{
			m_fTtShown = false;
			m_wndTt.SendMessageW(TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)(LPTOOLINFO)&m_stToolInfo);
		}
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

void CListEx::OnLButtonDown(UINT nFlags, CPoint pt)
{
	LVHITTESTINFO hi { };
	hi.pt = pt;
	ListView_SubItemHitTest(m_hWnd, &hi);
	if (hi.iSubItem == -1 || hi.iItem == -1)
		return;

	CMFCListCtrl::OnLButtonDown(nFlags, pt);
}

void CListEx::OnRButtonDown(UINT nFlags, CPoint pt)
{
	CMFCListCtrl::OnRButtonDown(nFlags, pt);
}

void CListEx::OnContextMenu(CWnd* /*pWnd*/, CPoint pt)
{
	CPoint ptClient = pt;
	ScreenToClient(&ptClient);
	LVHITTESTINFO hi;
	hi.pt = ptClient;
	ListView_SubItemHitTest(m_hWnd, &hi);

	CMenu* pMenu;
	if (HasMenu(hi.iItem, hi.iSubItem, &pMenu))
	{
		m_stNMII.iItem = hi.iItem;
		m_stNMII.iSubItem = hi.iSubItem;
		pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON, pt.x, pt.y, this);
	}
}

BOOL CListEx::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (HIWORD(wParam) == 0) //Message is from menu.
	{
		m_stNMII.hdr.code = LISTEX_MSG_MENUSELECTED;
		m_stNMII.lParam = LOWORD(wParam); //LOWORD(wParam) holds uiMenuItemId.
		GetParent()->SendMessageW(WM_NOTIFY, GetDlgCtrlID(), reinterpret_cast<LPARAM>(&m_stNMII));
	}

	return CMFCListCtrl::OnCommand(wParam, lParam);
}

void CListEx::OnTimer(UINT_PTR nIDEvent)
{
	//Checking if mouse left list's subitem rect,
	//if so — hiding tooltip and killing timer.
	if (nIDEvent == ID_TIMER_TOOLTIP)
	{
		CPoint pt;
		GetCursorPos(&pt);
		ScreenToClient(&pt);
		LVHITTESTINFO hitInfo { };
		hitInfo.pt = pt;
		ListView_SubItemHitTest(m_hWnd, &hitInfo);

		//If cursor is still hovers subitem then do nothing.
		if (m_stCurrCell.iItem == hitInfo.iItem && m_stCurrCell.iSubItem == hitInfo.iSubItem)
			return;

		//If it left.
		m_fTtShown = false;
		m_wndTt.SendMessageW(TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)(LPTOOLINFO)&m_stToolInfo);
		KillTimer(ID_TIMER_TOOLTIP);
		m_stCurrCell.iItem = hitInfo.iItem;
		m_stCurrCell.iSubItem = hitInfo.iSubItem;
	}

	CMFCListCtrl::OnTimer(nIDEvent);
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
	return FALSE;
}

void CListEx::OnPaint()
{
	//To avoid flickering.
	//Drawing to CMemDC, excluding list header area (rect).
	CRect rc, rcHdr;
	GetClientRect(&rc);
	GetHeaderCtrl().GetClientRect(rcHdr);
	rc.top += rcHdr.Height();

	CPaintDC dc(this);
	CMemDC memDC(dc, rc);
	CDC& rDC = memDC.GetDC();
	rDC.GetClipBox(&rc);
	rDC.FillSolidRect(rc, m_stColor.clrBkNWA);

	DefWindowProcW(WM_PAINT, reinterpret_cast<WPARAM>(rDC.m_hDC), static_cast<LPARAM>(0));
}

void CListEx::OnHdnDividerdblclick(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	//LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	//*pResult = 0;
}

void CListEx::OnHdnBegintrack(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	//LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	//*pResult = 0;
}

void CListEx::OnHdnTrack(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	//LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	//*pResult = 0;
}

void CListEx::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
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
	auto pNMLV = reinterpret_cast<LPNMHEADERW>(lParam);
	if (m_fSortable && (pNMLV->hdr.code == HDN_ITEMCLICKW || pNMLV->hdr.code == HDN_ITEMCLICKA))
	{
		m_fSortAscending = pNMLV->iItem == m_iSortColumn ? !m_fSortAscending : true;
		m_iSortColumn = pNMLV->iItem;

		GetHeaderCtrl().SetSortArrow(m_iSortColumn, m_fSortAscending);
		if (!m_fVirtual)
			SortItemsEx(m_pfnCompare ? m_pfnCompare : DefCompareFunc, reinterpret_cast<DWORD_PTR>(this));
	}

	return CMFCListCtrl::OnNotify(wParam, lParam, pResult);
}

void CListEx::OnLvnColumnClick(NMHDR* /*pNMHDR*/, LRESULT *pResult)
{
	//Just an empty handler. Without it all works fine, but assert 
	//triggers in Debug mode when clicking on header.
	*pResult = 0;
}

void CListEx::OnDestroy()
{
	CMFCListCtrl::OnDestroy();

	m_wndTt.DestroyWindow();
	m_fontList.DeleteObject();
	m_penGrid.DeleteObject();

	m_umapCellTt.clear();
	m_umapCellMenu.clear();
	m_umapCellData.clear();
	m_umapCellColor.clear();
	m_umapRowColor.clear();
	m_umapColumnSortMode.clear();
	m_umapColumnColor.clear();

	m_fCreated = false;
}