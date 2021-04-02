/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
* This is very extended and featured version of CMFCListCtrl class.                     *
* Official git repository: https://github.com/jovibor/ListEx/                           *
* This class is available under the "MIT License".                                      *
****************************************************************************************/
#include "stdafx.h"
#include "../ListEx.h"
#include "CListExHdr.h"
#include <cassert>

using namespace HEXCTRL::LISTEX;
using namespace HEXCTRL::LISTEX::INTERNAL;

namespace HEXCTRL::LISTEX::INTERNAL
{
	/********************************************
	* SHDRCOLOR - header column colors.         *
	********************************************/
	struct CListExHdr::SHDRCOLOR
	{
		COLORREF clrBk { };   //Background color.
		COLORREF clrText { }; //Text color.
	};

	/********************************************
	* SHDRICON - header column icons.           *
	********************************************/
	struct CListExHdr::SHDRICON
	{
		LISTEXHDRICON stIcon { };   //Icon data struct.
		bool  fLMPressed { false }; //Left mouse button pressed atm.
	};

	/********************************************
	* SHIDDEN - hidden columns.                 *
	********************************************/
	struct CListExHdr::SHIDDEN
	{
		int iPrevPos { };
		int iPrevWidth { };
	};
};

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
	NONCLIENTMETRICSW ncm { };
	ncm.cbSize = sizeof(NONCLIENTMETRICSW);
	SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
	ncm.lfMessageFont.lfHeight = 16; //For some weird reason above func returns this value as MAX_LONG.

	m_fontHdr.CreateFontIndirectW(&ncm.lfMessageFont);
	m_penGrid.CreatePen(PS_SOLID, 2, RGB(220, 220, 220));
	m_penLight.CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));
	m_penShadow.CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
}

CListExHdr::~CListExHdr() = default;

void CListExHdr::DeleteColumn(int iIndex)
{
	if (const auto ID = ColumnIndexToID(iIndex); ID > 0)
	{
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

	if (fHide) //Hide column.
	{
		m_umapHidden[ID].iPrevWidth = hdi.cxy;
		if (const auto iter = std::find(vecInt.begin(), vecInt.end(), iIndex); iter != vecInt.end())
		{
			m_umapHidden[ID].iPrevPos = static_cast<int>(iter - vecInt.begin());
			std::rotate(iter, iter + 1, vecInt.end()); //Moving hiding column to the end of the column array.
		}

		ListView_SetColumnOrderArray(GetParent()->m_hWnd, iItemsCount, vecInt.data());
		hdi.cxy = 0;
		SetItem(iIndex, &hdi);
	}
	else //Show column.
	{
		const auto opt = IsHidden(ID);
		if (!opt) //No such column is hidden.
			return;

		if (const auto iterRight = std::find(vecInt.rbegin(), vecInt.rend(), iIndex); iterRight != vecInt.rend())
		{
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

void CListExHdr::OnDrawItem(CDC * pDC, int iItem, CRect rcOrig, BOOL bIsPressed, BOOL bIsHighlighted)
{
	//Non working area after last column. Or if column resized to zero.
	if (iItem < 0 || rcOrig.IsRectEmpty())
	{
		pDC->FillSolidRect(&rcOrig, m_clrBkNWA);
		return;
	}

	CMemDC memDC(*pDC, rcOrig);
	CDC& rDC = memDC.GetDC();
	const auto ID = ColumnIndexToID(iItem);

	auto const pClr = HasColor(ID);
	const COLORREF clrText { pClr != nullptr ? pClr->clrText : m_clrText };
	const COLORREF clrBk { bIsHighlighted ? (bIsPressed ? m_clrHglActive : m_clrHglInactive) : (pClr != nullptr ? pClr->clrBk : m_clrBk) };

	rDC.FillSolidRect(&rcOrig, clrBk);
	rDC.SetTextColor(clrText);
	rDC.SelectObject(m_fontHdr);

	//Set item's text buffer first char to zero, then getting item's text and Draw it.
	static WCHAR warrHdrText[MAX_PATH] { };
	warrHdrText[0] = L'\0';
	static HDITEMW hdItem { HDI_FORMAT | HDI_TEXT };
	hdItem.cchTextMax = MAX_PATH;
	hdItem.pszText = warrHdrText;

	GetItem(iItem, &hdItem);
	UINT uFormat { };
	switch (hdItem.fmt)
	{
	case (HDF_STRING | HDF_LEFT):
		uFormat = DT_LEFT;
		break;
	case (HDF_STRING | HDF_CENTER):
		uFormat = DT_CENTER;
		break;
	case (HDF_STRING | HDF_RIGHT):
		uFormat = DT_RIGHT;
		break;
	default:
		break;
	}

	//Draw icon for column, if any.
	long lIndentTextLeft { 4 }; //Left text indent.
	if (const auto pData = HasIcon(ID); pData != nullptr) //If column has an icon.
	{
		const auto pImgList = GetImageList(LVSIL_NORMAL);
		int iCX { };
		int iCY { };
		ImageList_GetIconSize(pImgList->m_hImageList, &iCX, &iCY); //Icon dimensions.
		pImgList->DrawEx(&rDC, pData->stIcon.iIndex, rcOrig.TopLeft() + pData->stIcon.pt, { }, CLR_NONE, CLR_NONE, ILD_NORMAL);
		lIndentTextLeft += pData->stIcon.pt.x + iCX;
	}

	constexpr long lIndentTextRight = 4;
	CRect rcText { rcOrig.left + lIndentTextLeft, rcOrig.top, rcOrig.right - lIndentTextRight, rcOrig.bottom };
	if (StrStrW(warrHdrText, L"\n"))
	{	//If it's multiline text, first — calculate rect for the text,
		//with DT_CALCRECT flag (not drawing anything),
		//and then calculate rect for final vertical text alignment.
		CRect rcCalcText;
		rDC.DrawTextW(warrHdrText, &rcCalcText, DT_CENTER | DT_CALCRECT);
		rcText.top = rcText.Height() / 2 - rcCalcText.Height() / 2;
		rDC.DrawTextW(warrHdrText, &rcOrig, uFormat);
	}
	else
		rDC.DrawTextW(warrHdrText, &rcText, uFormat | DT_VCENTER | DT_SINGLELINE);

	//Draw sortable triangle (arrow).
	if (m_fSortable && IsSortable(ID) && ID == m_uSortColumn)
	{
		rDC.SelectObject(m_penLight);
		const auto iOffset = rcOrig.Height() / 4;

		if (m_fSortAscending)
		{
			//Draw the UP arrow.
			rDC.MoveTo(rcOrig.right - 2 * iOffset, iOffset);
			rDC.LineTo(rcOrig.right - iOffset, rcOrig.bottom - iOffset - 1);
			rDC.LineTo(rcOrig.right - 3 * iOffset - 2, rcOrig.bottom - iOffset - 1);
			rDC.SelectObject(m_penShadow);
			rDC.MoveTo(rcOrig.right - 3 * iOffset - 1, rcOrig.bottom - iOffset - 1);
			rDC.LineTo(rcOrig.right - 2 * iOffset, iOffset - 1);
		}
		else
		{
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
	if (iItem == GetItemCount() - 1) //Last item.
	{
		rDC.MoveTo(rcOrig.right, rcOrig.top);
		rDC.LineTo(rcOrig.BottomRight());
	}
}

LRESULT CListExHdr::OnLayout(WPARAM /*wParam*/, LPARAM lParam)
{
	CMFCHeaderCtrl::DefWindowProcW(HDM_LAYOUT, 0, lParam);

	auto pHDL = reinterpret_cast<LPHDLAYOUT>(lParam);
	pHDL->pwpos->cy = m_dwHeaderHeight;	//New header height.
	pHDL->prc->top = m_dwHeaderHeight;  //Decreasing list's height begining by the new header's height.

	return 0;
}

void CListExHdr::OnLButtonDown(UINT nFlags, CPoint point)
{
	HDHITTESTINFO ht { };
	ht.pt = point;
	HitTest(&ht);
	if (ht.iItem >= 0)
	{
		const auto ID = ColumnIndexToID(ht.iItem);
		if (const auto pData = HasIcon(ID); pData != nullptr && pData->stIcon.fClickable && !IsHidden(ID))
		{
			int iCX { };
			int iCY { };
			ImageList_GetIconSize(GetImageList()->m_hImageList, &iCX, &iCY);
			CRect rcColumn;
			GetItemRect(ht.iItem, rcColumn);
			CRect rcIcon(rcColumn.TopLeft() + pData->stIcon.pt, SIZE { iCX, iCY });

			if (rcIcon.PtInRect(point))
			{
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
	if (ht.iItem >= 0)
	{
		for (auto& iter : m_umapIcons)
		{
			if (!iter.second.fLMPressed)
				continue;

			int iCX { };
			int iCY { };
			ImageList_GetIconSize(GetImageList()->m_hImageList, &iCX, &iCY);
			CRect rcColumn;
			GetItemRect(ht.iItem, rcColumn);
			CRect rcIcon(rcColumn.TopLeft() + iter.second.stIcon.pt, SIZE { iCX, iCY });

			if (rcIcon.PtInRect(point))
			{
				for (auto& iterData : m_umapIcons)
					iterData.second.fLMPressed = false; //Remove fLMPressed flag from all columns.

				if (auto pParent = GetParent(); pParent != nullptr) //List control pointer.
				{
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
	for (int i = 0; i < GetItemCount(); ++i)
	{
		HDITEMW hdi { HDI_LPARAM };
		GetItem(i, &hdi);
		if (static_cast<UINT>(hdi.lParam) == uID)
			iRet = i;
	}

	return iRet;
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
	auto iter = m_umapHidden.find(ID);

	return iter != m_umapHidden.end() ? &iter->second : std::optional<SHIDDEN*> { };
}

bool CListExHdr::IsSortable(UINT ID)const
{
	const auto iter = m_umapIsSort.find(ID); //It's sortable unless found explicitly as false.

	return 	iter == m_umapIsSort.end() || iter->second;
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
	else
	{
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
	if (iColumn >= 0)
	{
		ID = ColumnIndexToID(iColumn);
		assert(ID > 0);
		if (ID == 0)
			return;
	}
	m_uSortColumn = ID;
	m_fSortAscending = fAscending;
	RedrawWindow();
}

void CListExHdr::SetFont(const LOGFONTW * pLogFontNew)
{
	if (!pLogFontNew)
		return;

	m_fontHdr.DeleteObject();
	m_fontHdr.CreateFontIndirectW(pLogFontNew);

	//If new font's height is higher than current height (m_dwHeaderHeight)
	//we adjust current height as well.
	TEXTMETRICW tm;
	CDC* pDC = GetDC();
	pDC->SelectObject(m_fontHdr);
	pDC->GetTextMetricsW(&tm);
	ReleaseDC(pDC);
	const DWORD dwHeightFont = tm.tmHeight + tm.tmExternalLeading + 1;
	if (dwHeightFont > m_dwHeaderHeight)
		SetHeight(dwHeightFont);
}