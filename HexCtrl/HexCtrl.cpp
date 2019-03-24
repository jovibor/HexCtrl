/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/						    *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* This is a Hex control for MFC apps, implemented as CWnd derived class.			    *
* The usage is quite simple:														    *
* 1. Construct CHexCtrl object — HEXCTRL::CHexCtrl myHex;								*
* 2. Call myHex.Create member function to create an instance.   					    *
* 3. Call myHex.SetData method to set the data and its size to display as hex.	        *
****************************************************************************************/
#include "stdafx.h"
#include "strsafe.h"
#include "res/HexCtrlRes.h"
#include "Dialogs/HexCtrlDlgAbout.h"
#include "Dialogs/HexCtrlDlgSearch.h"
#include "HexCtrl.h"
#include "ScrollEx.h"
#include "Helper.h"
#pragma comment(lib, "Dwmapi.lib")

using namespace HEXCTRL;

namespace HEXCTRL {
	/********************************************
	* Internal enums and structs.				*
	********************************************/
	namespace HEXCTRL_INTERNAL {
		enum HEX_SHOWAS {
			ASBYTE = 1, ASWORD = 2, ASDWORD = 4, ASQWORD = 8
		};

		enum HEX_CLIPBOARD {
			COPY_ASHEX, COPY_ASHEXFORMATTED, COPY_ASASCII,
			PASTE_ASHEX, PASTE_ASASCII
		};

		enum HEX_MENU {
			IDM_MAIN_SEARCH,
			IDM_SHOWAS_ASBYTE, IDM_SHOWAS_ASWORD, IDM_SHOWAS_ASDWORD, IDM_SHOWAS_ASQWORD,
			IDM_EDIT_UNDO, IDM_EDIT_REDO, IDM_EDIT_COPY_ASHEX, IDM_EDIT_COPY_ASHEXFORMATTED, IDM_EDIT_COPY_ASASCII,
			IDM_EDIT_PASTE_ASHEX, IDM_EDIT_PASTE_ASASCII,
			IDM_EDIT_FILL_ZERO,
			IDM_MAIN_ABOUT,
		};

		enum HEX_MODIFYAS
		{
			AS_STANDARD, AS_FILL, AS_UNDO
		};

		struct HEXUNDO
		{
			ULONGLONG ullIndex;
			std::string strData;
		};

		struct HEXMODIFYDATA {
			ULONGLONG		ullIndex { };		//Index of the start byte to modify.
			ULONGLONG		ullModifySize { };	//Size in bytes.
			PBYTE			pData { };			//Pointer to data to be set.
			HEX_MODIFYAS	enType { HEX_MODIFYAS::AS_STANDARD }; //Modify type.
			DWORD			dwFillDataSize { }; //Size of pData if enType==AS_FILL.
			bool			fWhole { true };	//Is a whole byte or just a part of it to be modified.
			bool			fHighPart { true };	//Shows whether High or Low part of byte should be modified (If fWhole is false).
			bool			fMoveNext { true };	//Should cursor be moved to the next byte.
		};

		constexpr auto HEXCTRL_WNDCLASS_WSTR = L"HexCtrl";
	}
};

/************************************************************************
* CHexCtrl implementation.												*
************************************************************************/
BEGIN_MESSAGE_MAP(CHexCtrl, CWnd)
	ON_WM_ACTIVATE()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_SETCURSOR()
	ON_WM_MOUSEWHEEL()
	ON_WM_MOUSEMOVE()
	ON_WM_MBUTTONDOWN()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_KEYDOWN()
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	ON_WM_NCACTIVATE()
	ON_WM_NCCALCSIZE()
	ON_WM_NCPAINT()
	ON_WM_CHAR()
	ON_WM_GETDLGCODE()
END_MESSAGE_MAP()

CHexCtrl::CHexCtrl()
{
	RegisterWndClass();

	for (unsigned i = 0; i < m_dwCapacityMax; i++)
	{
		WCHAR wstr[3];
		swprintf_s(wstr, 3, L"%X", i);
		m_umapCapacityWstr[i] = wstr;
	}

	//Submenu for data showing options.
	m_menuShowAs.CreatePopupMenu();
	m_menuShowAs.AppendMenuW(MF_STRING | MF_CHECKED, HEXCTRL_INTERNAL::IDM_SHOWAS_ASBYTE, L"BYTE");
	m_menuShowAs.AppendMenuW(MF_STRING, HEXCTRL_INTERNAL::HEX_MENU::IDM_SHOWAS_ASWORD, L"WORD");
	m_menuShowAs.AppendMenuW(MF_STRING, HEXCTRL_INTERNAL::HEX_MENU::IDM_SHOWAS_ASDWORD, L"DWORD");
	m_menuShowAs.AppendMenuW(MF_STRING, HEXCTRL_INTERNAL::HEX_MENU::IDM_SHOWAS_ASQWORD, L"QWORD");

	//Main menu.
	m_menuMain.CreatePopupMenu();
	m_menuMain.AppendMenuW(MF_STRING, HEXCTRL_INTERNAL::HEX_MENU::IDM_MAIN_SEARCH, L"Search...	Ctrl+F");
	m_menuMain.AppendMenuW(MF_SEPARATOR);
	m_menuMain.AppendMenuW(MF_POPUP, (DWORD_PTR)m_menuShowAs.m_hMenu, L"Show data as...");
	m_menuMain.AppendMenuW(MF_SEPARATOR);
	m_menuMain.AppendMenuW(MF_STRING, HEXCTRL_INTERNAL::HEX_MENU::IDM_EDIT_UNDO, L"Undo	Ctrl+Z");
	m_menuMain.AppendMenuW(MF_STRING, HEXCTRL_INTERNAL::HEX_MENU::IDM_EDIT_REDO, L"Redo	Ctrl+Y");
	m_menuMain.AppendMenuW(MF_SEPARATOR);
	m_menuMain.AppendMenuW(MF_STRING, HEXCTRL_INTERNAL::HEX_MENU::IDM_EDIT_COPY_ASHEX, L"Copy as Hex...	Ctrl+C");

	//Menu icons.
	MENUITEMINFOW mii { };
	mii.cbSize = sizeof(MENUITEMINFOW);
	mii.fMask = MIIM_BITMAP;
	mii.hbmpItem = m_umapHBITMAP[HEXCTRL_INTERNAL::HEX_MENU::IDM_EDIT_COPY_ASHEX] =
		(HBITMAP)LoadImageW(GetModuleHandleW(0), MAKEINTRESOURCE(IDB_HEXCTRL_MENU_COPY), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	m_menuMain.SetMenuItemInfoW(HEXCTRL_INTERNAL::HEX_MENU::IDM_EDIT_COPY_ASHEX, &mii);
	m_menuMain.AppendMenuW(MF_STRING, HEXCTRL_INTERNAL::HEX_MENU::IDM_EDIT_COPY_ASHEXFORMATTED, L"Copy as Formatted Hex...");
	m_menuMain.AppendMenuW(MF_STRING, HEXCTRL_INTERNAL::HEX_MENU::IDM_EDIT_COPY_ASASCII, L"Copy as Ascii...");
	m_menuMain.AppendMenuW(MF_SEPARATOR);
	m_menuMain.AppendMenuW(MF_STRING, HEXCTRL_INTERNAL::HEX_MENU::IDM_EDIT_PASTE_ASHEX, L"Paste as Hex	Ctrl+V");
	mii.hbmpItem = m_umapHBITMAP[HEXCTRL_INTERNAL::HEX_MENU::IDM_EDIT_PASTE_ASHEX] =
		(HBITMAP)LoadImageW(GetModuleHandleW(0), MAKEINTRESOURCE(IDB_HEXCTRL_MENU_PASTE), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	m_menuMain.SetMenuItemInfoW(HEXCTRL_INTERNAL::HEX_MENU::IDM_EDIT_PASTE_ASHEX, &mii);
	m_menuMain.AppendMenuW(MF_STRING, HEXCTRL_INTERNAL::HEX_MENU::IDM_EDIT_PASTE_ASASCII, L"Paste as Ascii");
	m_menuMain.AppendMenuW(MF_SEPARATOR);
	m_menuMain.AppendMenuW(MF_STRING, HEXCTRL_INTERNAL::HEX_MENU::IDM_EDIT_FILL_ZERO, L"Fill with zeros");
	m_menuMain.AppendMenuW(MF_SEPARATOR);
	m_menuMain.AppendMenuW(MF_STRING, HEXCTRL_INTERNAL::HEX_MENU::IDM_MAIN_ABOUT, L"About");

	m_pDlgSearch->Create(IDD_HEXCTRL_SEARCH, this);

	m_dwShowAs = HEXCTRL_INTERNAL::HEX_SHOWAS::ASBYTE;
}

CHexCtrl::~CHexCtrl()
{
	//Deleting all loaded bitmaps.
	for (auto const& i : m_umapHBITMAP)
		DeleteObject(i.second);
}

bool CHexCtrl::Create(const HEXCREATESTRUCT& hcs)
{
	if (IsCreated()) //Already created.
		return false;

	m_dwCtrlId = hcs.uId;
	m_fFloat = hcs.fFloat;
	m_pwndParentOwner = hcs.pwndParent;
	if (hcs.pwndMsg)
		m_pwndMsg = hcs.pwndMsg;
	else
		m_pwndMsg = hcs.pwndParent;
	if (hcs.pstColor)
		m_stColor = *hcs.pstColor;

	m_stBrushBkSelected.CreateSolidBrush(m_stColor.clrBkSelected);

	DWORD dwStyle;
	if (m_fFloat)
		dwStyle = WS_VISIBLE | WS_OVERLAPPEDWINDOW;
	else
		dwStyle = WS_VISIBLE | WS_CHILD;

	//Font related.//////////////////////////////////////////////
	LOGFONTW lf { };
	if (hcs.pLogFont)
		lf = *hcs.pLogFont;
	else {
		StringCchCopyW(lf.lfFaceName, 9, L"Consolas");
		lf.lfHeight = 18;
	}

	if (!m_fontHexView.CreateFontIndirectW(&lf))
	{
		NONCLIENTMETRICSW ncm { sizeof(NONCLIENTMETRICSW) };
		SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
		lf = ncm.lfMessageFont;
		lf.lfHeight = 18; //For some reason above func returns lfHeight value as MAX_LONG.

		m_fontHexView.CreateFontIndirectW(&lf);
	}
	lf.lfHeight = 16;
	m_fontBottomRect.CreateFontIndirectW(&lf);
	//End of font related.///////////////////////////////////////

	CRect rc = hcs.rect;
	if (rc.IsRectNull() && m_fFloat)
	{	//If initial rect is null, and it's a float window HexCtrl, then place it at screen center.

		int iPosX = GetSystemMetrics(SM_CXSCREEN) / 4;
		int iPosY = GetSystemMetrics(SM_CYSCREEN) / 4;
		int iPosCX = iPosX * 3;
		int iPosCY = iPosY * 3;
		rc.SetRect(iPosX, iPosY, iPosCX, iPosCY);
	}

	//If it's custom dialog control then there is no need to create window.
	if (!hcs.fCustomCtrl && !CWnd::CreateEx(hcs.dwExStyles, HEXCTRL_INTERNAL::HEXCTRL_WNDCLASS_WSTR, L"HexControl",
		dwStyle, rc, m_pwndParentOwner, m_fFloat ? 0 : m_dwCtrlId))
	{
		CStringW ss;
		ss.Format(L"CHexCtrl (Id:%u) CWnd::CreateEx failed.\r\nCheck HEXCREATESTRUCT filling correctness.", m_dwCtrlId);
		MessageBoxW(ss, L"Error", MB_ICONERROR);
		return false;
	}

	//Removing window's border frame.
	MARGINS marg { 0, 0, 0, 1 };
	DwmExtendFrameIntoClientArea(m_hWnd, &marg);
	SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

	m_pstScrollV->Create(this, SB_VERT, 0, 0, 0); //Actual sizes are set in RecalcAll().
	m_pstScrollH->Create(this, SB_HORZ, 0, 0, 0);
	m_pstScrollV->AddSibling(m_pstScrollH.get());
	m_pstScrollH->AddSibling(m_pstScrollV.get());

	m_fCreated = true;

	RecalcAll();

	return true;
}

bool CHexCtrl::IsCreated()
{
	return m_fCreated;
}

void CHexCtrl::SetData(const HEXDATASTRUCT& hds)
{
	if (!IsCreated())
		return;

	if (hds.pwndMsg)
		m_pwndMsg = hds.pwndMsg;

	//Virtual mode is possible only when there is a msg window,
	//a data requests will be sent to.
	if (hds.fVirtual && !m_pwndMsg)
	{
		MessageBoxW(L"HexCtrl Virtual mode requires HEXCREATESTRUCT::pwndMsg or HEXDATASTRUCT::pwndMsg to be set.", L"Error", MB_ICONWARNING);
		return;
	}

	m_pData = hds.pData;
	m_ullDataSize = hds.ullDataSize;
	m_fVirtual = hds.fVirtual;
	m_fMutable = hds.fMutable;
	m_dwOffsetDigits = hds.ullDataSize <= 0xfffffffful ? 8 :
		(hds.ullDataSize <= 0xfffffffffful ? 10 :
		(hds.ullDataSize <= 0xfffffffffffful ? 12 :
			(hds.ullDataSize <= 0xfffffffffffffful ? 14 : 16)));
	RecalcAll();

	if (hds.ullSelectionSize)
		ShowOffset(hds.ullSelectionStart, hds.ullSelectionSize);
	else
	{
		m_ullSelectionClick = m_ullSelectionStart = m_ullSelectionEnd = m_ullSelectionSize = 0;
		m_pstScrollV->SetScrollPos(0);
		m_pstScrollH->SetScrollPos(0);
		UpdateInfoText();
	}
}

void CHexCtrl::ClearData()
{
	m_ullDataSize = 0;
	m_pData = nullptr;
	m_ullSelectionClick = m_ullSelectionStart = m_ullSelectionEnd = m_ullSelectionSize = 0;

	m_pstScrollV->SetScrollPos(0);
	m_pstScrollH->SetScrollPos(0);
	m_pstScrollV->SetScrollSizes(0, 0, 0);
	UpdateInfoText();
}

void CHexCtrl::ShowOffset(ULONGLONG ullOffset, ULONGLONG ullSize)
{
	SetSelection(ullOffset, ullOffset, ullSize, true);
}

void CHexCtrl::SetFont(const LOGFONT* pLogFontNew)
{
	if (!pLogFontNew)
		return;

	m_fontHexView.DeleteObject();
	m_fontHexView.CreateFontIndirectW(pLogFontNew);

	RecalcAll();
}

void CHexCtrl::SetFontSize(UINT uiSize)
{
	//Prevent font size from being too small or too big.
	if (uiSize < 9 || uiSize > 75)
		return;

	LOGFONT lf;
	m_fontHexView.GetLogFont(&lf);
	lf.lfHeight = uiSize;
	m_fontHexView.DeleteObject();
	m_fontHexView.CreateFontIndirectW(&lf);

	RecalcAll();
}

UINT CHexCtrl::GetFontSize()
{
	LOGFONT lf;
	m_fontHexView.GetLogFont(&lf);

	return lf.lfHeight;
}

void CHexCtrl::SetColor(const HEXCOLORSTRUCT& clr)
{
	m_stColor = clr;
	RedrawWindow();
}

void CHexCtrl::SetCapacity(DWORD dwCapacity)
{
	if (dwCapacity < 1 || dwCapacity > m_dwCapacityMax)
		return;

	if (dwCapacity < m_dwCapacity)
		dwCapacity -= dwCapacity % m_dwShowAs;
	else
		dwCapacity += m_dwShowAs - (dwCapacity % m_dwShowAs);

	if (dwCapacity < (DWORD)m_dwShowAs)
		dwCapacity = m_dwShowAs;
	else if (dwCapacity > m_dwCapacityMax)
		dwCapacity = m_dwCapacityMax;

	m_dwCapacity = dwCapacity;
	m_dwCapacityBlockSize = m_dwCapacity / 2;
	RecalcAll();
}

UINT CHexCtrl::GetDlgCtrlID()const
{
	return m_dwCtrlId;
}

CWnd* CHexCtrl::GetParent()const
{
	return m_pwndParentOwner;
}

bool CHexCtrl::RegisterWndClass()
{
	WNDCLASSEXW wcls;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if (!(::GetClassInfoExW(hInst, HEXCTRL_INTERNAL::HEXCTRL_WNDCLASS_WSTR, &wcls)))
	{
		wcls.cbSize = sizeof(WNDCLASSEXW);
		wcls.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wcls.lpfnWndProc = ::DefWindowProcW;
		wcls.cbClsExtra = wcls.cbWndExtra = 0;
		wcls.hInstance = hInst;
		wcls.hIcon = NULL;
		wcls.hIconSm = NULL;
		wcls.hCursor = (HCURSOR)LoadImageW(0, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
		wcls.hbrBackground = NULL;
		wcls.lpszMenuName = nullptr;
		wcls.lpszClassName = HEXCTRL_INTERNAL::HEXCTRL_WNDCLASS_WSTR;

		if (!RegisterClassExW(&wcls))
		{
			MessageBoxW(L"HexControl RegisterClassExW error.", L"Error");
			return false;
		}
	}

	return true;
}

void CHexCtrl::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	SetFocus();

	CWnd::OnActivate(nState, pWndOther, bMinimized);
}

void CHexCtrl::OnDestroy()
{
	NMHDR nmh { m_hWnd, (UINT)GetDlgCtrlID(), HEXCTRL_MSG_DESTROY };
	if (m_pwndMsg)
		m_pwndMsg->SendMessageW(WM_NOTIFY, nmh.idFrom, (LPARAM)&nmh);
	if (m_pwndParentOwner)
	{
		m_pwndParentOwner->SendMessageW(WM_NOTIFY, nmh.idFrom, (LPARAM)&nmh);
		m_pwndParentOwner->SetForegroundWindow();
	}
	m_fCreated = false;
	ClearData();

	CWnd::OnDestroy();
}

void CHexCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_fLMousePressed)
	{
		//If LMouse is pressed but cursor is outside client area.
		//SetCapture() behaviour.

		CRect rcClient;
		GetClientRect(&rcClient);
		//Checking for scrollbars existence first.
		if (m_pstScrollH->IsVisible())
		{
			if (point.x < rcClient.left) {
				m_pstScrollH->ScrollLineLeft();
				point.x = m_iIndentFirstHexChunk;
			}
			else if (point.x >= rcClient.right) {
				m_pstScrollH->ScrollLineRight();
				point.x = m_iFourthVertLine - 1;
			}
		}
		if (m_pstScrollV->IsVisible())
		{
			if (point.y < m_iHeightTopRect) {
				m_pstScrollV->ScrollLineUp();
				point.y = m_iHeightTopRect;
			}
			else if (point.y >= m_iHeightWorkArea) {
				m_pstScrollV->ScrollLineDown();
				point.y = m_iHeightWorkArea - 1;
			}
		}

		const ULONGLONG ullHit = HitTest(&point);
		if (ullHit != -1)
		{
			ULONGLONG ullClick, ullStart, ullSize;
			if (ullHit <= m_ullSelectionClick) {
				ullClick = m_ullSelectionClick;
				ullStart = ullHit;
				ullSize = ullClick - ullStart + 1;
			}
			else {
				ullClick = m_ullSelectionClick;
				ullStart = m_ullSelectionClick;
				ullSize = ullHit - ullClick + 1;
			}
			SetSelection(ullClick, ullStart, ullSize, false, true);
		}
	}
	else
	{
		m_pstScrollV->OnMouseMove(nFlags, point);
		m_pstScrollH->OnMouseMove(nFlags, point);
	}
}

BOOL CHexCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (nFlags == MK_CONTROL)
	{
		SetFontSize(GetFontSize() + zDelta / WHEEL_DELTA * 2);
		return TRUE;
	}
	else if (nFlags & (MK_CONTROL | MK_SHIFT))
	{
		SetCapacity(m_dwCapacity + zDelta / WHEEL_DELTA);
		return TRUE;
	}

	if (zDelta > 0) //Scrolling Up.
		m_pstScrollV->ScrollPageUp();
	else
		m_pstScrollV->ScrollPageDown();

	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

void CHexCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();

	const ULONGLONG ullHit = HitTest(&point);
	if (ullHit == -1)
		return;

	SetCapture();
	if (m_ullSelectionSize && (nFlags & MK_SHIFT))
	{
		if (ullHit <= m_ullSelectionClick)
		{
			m_ullSelectionStart = ullHit;
			m_ullSelectionEnd = m_ullSelectionClick + 1;
		}
		else
		{
			m_ullSelectionStart = m_ullSelectionClick;
			m_ullSelectionEnd = ullHit + 1;
		}

		m_ullSelectionSize = m_ullSelectionEnd - m_ullSelectionStart;
	}
	else
	{
		m_ullSelectionClick = m_ullSelectionStart = ullHit;
		m_ullSelectionEnd = m_ullSelectionStart + 1;
		m_ullSelectionSize = 1;
	}

	m_ullCursorPos = m_ullSelectionStart;
	m_fCursorHigh = true;
	m_fLMousePressed = true;
	UpdateInfoText();
}

void CHexCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_fLMousePressed = false;
	ReleaseCapture();

	m_pstScrollV->OnLButtonUp(nFlags, point);
	m_pstScrollH->OnLButtonUp(nFlags, point);

	CWnd::OnLButtonUp(nFlags, point);
}

void CHexCtrl::OnMButtonDown(UINT nFlags, CPoint point)
{
}

BOOL CHexCtrl::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UINT uiId = LOWORD(wParam);

	switch (uiId)
	{
	case HEXCTRL_INTERNAL::HEX_MENU::IDM_MAIN_SEARCH:
		if (m_fVirtual)
			MessageBoxW(m_wstrErrVirtual.data(), L"Error", MB_ICONEXCLAMATION);
		else
			m_pDlgSearch->ShowWindow(SW_SHOW);
		break;
	case HEXCTRL_INTERNAL::HEX_MENU::IDM_EDIT_UNDO:
		Undo();
		break;
	case HEXCTRL_INTERNAL::HEX_MENU::IDM_EDIT_REDO:
		Redo();
		break;
	case HEXCTRL_INTERNAL::HEX_MENU::IDM_EDIT_COPY_ASHEX:
		ClipboardCopy(HEXCTRL_INTERNAL::HEX_CLIPBOARD::COPY_ASHEX);
		break;
	case HEXCTRL_INTERNAL::HEX_MENU::IDM_EDIT_COPY_ASHEXFORMATTED:
		ClipboardCopy(HEXCTRL_INTERNAL::HEX_CLIPBOARD::COPY_ASHEXFORMATTED);
		break;
	case HEXCTRL_INTERNAL::HEX_MENU::IDM_EDIT_COPY_ASASCII:
		ClipboardCopy(HEXCTRL_INTERNAL::HEX_CLIPBOARD::COPY_ASASCII);
		break;
	case HEXCTRL_INTERNAL::HEX_MENU::IDM_EDIT_PASTE_ASHEX:
		ClipboardPaste(HEXCTRL_INTERNAL::HEX_CLIPBOARD::PASTE_ASHEX);
		break;
	case HEXCTRL_INTERNAL::HEX_MENU::IDM_EDIT_PASTE_ASASCII:
		ClipboardPaste(HEXCTRL_INTERNAL::HEX_CLIPBOARD::PASTE_ASASCII);
		break;
	case HEXCTRL_INTERNAL::HEX_MENU::IDM_EDIT_FILL_ZERO:
	{
		HEXCTRL_INTERNAL::HEXMODIFYDATA hmd;
		hmd.enType = HEXCTRL_INTERNAL::HEX_MODIFYAS::AS_FILL;
		hmd.ullModifySize = m_ullSelectionSize;
		hmd.ullIndex = m_ullSelectionStart;
		hmd.fMoveNext = false;
		unsigned char chZero { 0 };
		hmd.pData = &chZero;
		hmd.dwFillDataSize = 1;
		ModifyData(hmd);
	}
	break;
	case HEXCTRL_INTERNAL::HEX_MENU::IDM_MAIN_ABOUT:
	{
		CHexDlgAbout m_dlgAbout;
		m_dlgAbout.DoModal();
	}
	break;
	case HEXCTRL_INTERNAL::HEX_MENU::IDM_SHOWAS_ASBYTE:
		SetShowAs(HEXCTRL_INTERNAL::HEX_SHOWAS::ASBYTE);
		break;
	case HEXCTRL_INTERNAL::HEX_MENU::IDM_SHOWAS_ASWORD:
		SetShowAs(HEXCTRL_INTERNAL::HEX_SHOWAS::ASWORD);
		break;
	case HEXCTRL_INTERNAL::HEX_MENU::IDM_SHOWAS_ASDWORD:
		SetShowAs(HEXCTRL_INTERNAL::HEX_SHOWAS::ASDWORD);
		break;
	case HEXCTRL_INTERNAL::HEX_MENU::IDM_SHOWAS_ASQWORD:
		SetShowAs(HEXCTRL_INTERNAL::HEX_SHOWAS::ASQWORD);
		break;
	}

	return CWnd::OnCommand(wParam, lParam);
}

void CHexCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
{
	UINT uMenuStatus;
	if (m_ullDataSize == 0 || m_ullSelectionSize == 0)
		uMenuStatus = MF_GRAYED;
	else
		uMenuStatus = MF_ENABLED;

	//To not spread up the efforts through code, all the checks are done here 
	//instead of WM_INITMENUPOPUP.
	m_menuMain.EnableMenuItem(HEXCTRL_INTERNAL::HEX_MENU::IDM_EDIT_COPY_ASHEX, uMenuStatus | MF_BYCOMMAND);
	m_menuMain.EnableMenuItem(HEXCTRL_INTERNAL::HEX_MENU::IDM_EDIT_COPY_ASHEXFORMATTED, uMenuStatus | MF_BYCOMMAND);
	m_menuMain.EnableMenuItem(HEXCTRL_INTERNAL::HEX_MENU::IDM_EDIT_COPY_ASASCII, uMenuStatus | MF_BYCOMMAND);
	m_menuMain.EnableMenuItem(HEXCTRL_INTERNAL::HEX_MENU::IDM_EDIT_UNDO, (m_deqUndo.empty() ? MF_GRAYED : MF_ENABLED) | MF_BYCOMMAND);
	m_menuMain.EnableMenuItem(HEXCTRL_INTERNAL::HEX_MENU::IDM_EDIT_REDO, (m_deqRedo.empty() ? MF_GRAYED : MF_ENABLED) | MF_BYCOMMAND);

	BOOL fFormatAvail = IsClipboardFormatAvailable(CF_TEXT);
	m_menuMain.EnableMenuItem(HEXCTRL_INTERNAL::HEX_MENU::IDM_EDIT_PASTE_ASHEX, ((m_fMutable && fFormatAvail) ?
		uMenuStatus : MF_GRAYED) | MF_BYCOMMAND);
	m_menuMain.EnableMenuItem(HEXCTRL_INTERNAL::HEX_MENU::IDM_EDIT_PASTE_ASASCII, ((m_fMutable && fFormatAvail) ?
		uMenuStatus : MF_GRAYED) | MF_BYCOMMAND);
	m_menuMain.EnableMenuItem(HEXCTRL_INTERNAL::HEX_MENU::IDM_EDIT_FILL_ZERO,
		(m_fMutable ? uMenuStatus : MF_GRAYED) | MF_BYCOMMAND);

	m_menuMain.TrackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON, point.x, point.y, this);
}

void CHexCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	ULONGLONG ullStart, ullSize;
	if (GetKeyState(VK_CONTROL) < 0) //With Ctrl pressed.
	{
		switch (nChar)
		{
		case 'F':
			m_pDlgSearch->ShowWindow(SW_SHOW);
			break;
		case 'C':
			ClipboardCopy(HEXCTRL_INTERNAL::HEX_CLIPBOARD::COPY_ASHEX);
			break;
		case 'V':
			ClipboardPaste(HEXCTRL_INTERNAL::HEX_CLIPBOARD::PASTE_ASHEX);
			break;
		case 'A':
			SelectAll();;
			break;
		case 'Z':
			Undo();
			break;
		case 'Y':
			Redo();
			break;
		}
	}
	else if (GetAsyncKeyState(VK_SHIFT) < 0) //With Shift pressed.
	{
		switch (nChar)
		{
		case VK_RIGHT:
			if (m_fMutable)
			{
				if (m_ullCursorPos == m_ullSelectionClick || m_ullCursorPos == m_ullSelectionStart
					|| m_ullCursorPos == m_ullSelectionEnd)
				{
					if (m_ullSelectionStart == m_ullSelectionClick)
						SetSelection(m_ullSelectionClick, m_ullSelectionClick, m_ullSelectionSize + 1);
					else
						SetSelection(m_ullSelectionClick, m_ullSelectionStart + 1, m_ullSelectionSize - 1);
				}
				else
					SetSelection(m_ullCursorPos, m_ullCursorPos, 1);
			}
			else if (m_ullSelectionSize)
			{
				if (m_ullSelectionStart == m_ullSelectionClick)
					SetSelection(m_ullSelectionClick, m_ullSelectionClick, m_ullSelectionSize + 1);
				else
					SetSelection(m_ullSelectionClick, m_ullSelectionStart + 1, m_ullSelectionSize - 1);
			}
			break;
		case VK_LEFT:
			if (m_fMutable)
			{
				if (m_ullCursorPos == m_ullSelectionClick || m_ullCursorPos == m_ullSelectionStart
					|| m_ullCursorPos == m_ullSelectionEnd)
				{
					if (m_ullSelectionStart == m_ullSelectionClick && m_ullSelectionSize > 1)
						SetSelection(m_ullSelectionClick, m_ullSelectionClick, m_ullSelectionSize - 1);
					else
						SetSelection(m_ullSelectionClick, m_ullSelectionStart - 1, m_ullSelectionSize + 1);
				}
				else
					SetSelection(m_ullCursorPos, m_ullCursorPos, 1);
			}
			else if (m_ullSelectionSize)
			{
				if (m_ullSelectionStart == m_ullSelectionClick && m_ullSelectionSize > 1)
					SetSelection(m_ullSelectionClick, m_ullSelectionClick, m_ullSelectionSize - 1);
				else
					SetSelection(m_ullSelectionClick, m_ullSelectionStart - 1, m_ullSelectionSize + 1);
			}
			break;
		case VK_DOWN:
			if (m_fMutable)
			{
				if (m_ullCursorPos == m_ullSelectionClick || m_ullCursorPos == m_ullSelectionStart
					|| m_ullCursorPos == m_ullSelectionEnd)
				{
					if (m_ullSelectionStart == m_ullSelectionClick)
						SetSelection(m_ullSelectionClick, m_ullSelectionClick, m_ullSelectionSize + m_dwCapacity);
					else if (m_ullSelectionStart < m_ullSelectionClick)
					{
						ullStart = m_ullSelectionSize > m_dwCapacity ? m_ullSelectionStart + m_dwCapacity : m_ullSelectionClick;
						ullSize = m_ullSelectionSize >= m_dwCapacity ? m_ullSelectionSize - m_dwCapacity : m_dwCapacity;
						SetSelection(m_ullSelectionClick, ullStart, ullSize ? ullSize : 1);
					}
				}
				else
					SetSelection(m_ullCursorPos, m_ullCursorPos, 1);
			}
			else if (m_ullSelectionSize)
			{
				if (m_ullSelectionStart == m_ullSelectionClick)
					SetSelection(m_ullSelectionClick, m_ullSelectionClick, m_ullSelectionSize + m_dwCapacity);
				else if (m_ullSelectionStart < m_ullSelectionClick)
				{
					ullStart = m_ullSelectionSize > m_dwCapacity ? m_ullSelectionStart + m_dwCapacity : m_ullSelectionClick;
					ullSize = m_ullSelectionSize >= m_dwCapacity ? m_ullSelectionSize - m_dwCapacity : m_dwCapacity;
					SetSelection(m_ullSelectionClick, ullStart, ullSize ? ullSize : 1);
				}
			}
			break;
		case VK_UP:
			if (m_fMutable)
			{
				if (m_ullCursorPos == m_ullSelectionClick || m_ullCursorPos == m_ullSelectionStart
					|| m_ullCursorPos == m_ullSelectionEnd)
				{
					if (m_ullSelectionStart == 0)
						return;

					if (m_ullSelectionStart < m_ullSelectionClick)
					{
						if (m_ullSelectionStart < m_dwCapacity) {
							ullStart = 0;
							ullSize = m_ullSelectionSize + m_ullSelectionStart;
						}
						else {
							ullStart = m_ullSelectionStart - m_dwCapacity;
							ullSize = m_ullSelectionSize + m_dwCapacity;
						}
						SetSelection(m_ullSelectionClick, ullStart, ullSize);
					}
					else
					{
						ullStart = m_ullSelectionSize >= m_dwCapacity ? m_ullSelectionClick : m_ullSelectionClick - m_dwCapacity + 1;
						ullSize = m_ullSelectionSize >= m_dwCapacity ? m_ullSelectionSize - m_dwCapacity : m_dwCapacity;
						SetSelection(m_ullSelectionClick, ullStart, ullSize ? ullSize : 1);
					}
				}
				else
					SetSelection(m_ullCursorPos, m_ullCursorPos, 1);
			}
			else if (m_ullSelectionSize)
			{
				if (m_ullSelectionStart == 0)
					return;

				if (m_ullSelectionStart < m_ullSelectionClick)
				{
					if (m_ullSelectionStart < m_dwCapacity) {
						ullStart = 0;
						ullSize = m_ullSelectionSize + m_ullSelectionStart;
					}
					else {
						ullStart = m_ullSelectionStart - m_dwCapacity;
						ullSize = m_ullSelectionSize + m_dwCapacity;
					}
					SetSelection(m_ullSelectionClick, ullStart, ullSize);
				}
				else
				{
					ullStart = m_ullSelectionSize >= m_dwCapacity ? m_ullSelectionClick : m_ullSelectionClick - m_dwCapacity + 1;
					ullSize = m_ullSelectionSize >= m_dwCapacity ? m_ullSelectionSize - m_dwCapacity : m_dwCapacity;
					SetSelection(m_ullSelectionClick, ullStart, ullSize ? ullSize : 1);
				}
			}
			break;
		}
	}
	else
	{
		switch (nChar)
		{
		case VK_RIGHT:
			if (m_fMutable)
				CursorMoveRight();
			else
				SetSelection(m_ullSelectionClick + 1, m_ullSelectionClick + 1, 1);
			break;
		case VK_LEFT:
			if (m_fMutable)
				CursorMoveLeft();
			else
				SetSelection(m_ullSelectionClick - 1, m_ullSelectionClick - 1, 1);
			break;
		case VK_DOWN:
			if (m_fMutable)
				CursorMoveDown();
			else
				SetSelection(m_ullSelectionClick + m_dwCapacity, m_ullSelectionClick + m_dwCapacity, 1);
			break;
		case VK_UP:
			if (m_fMutable)
				CursorMoveUp();
			else
				SetSelection(m_ullSelectionClick - m_dwCapacity, m_ullSelectionClick - m_dwCapacity, 1);
			break;
		case VK_PRIOR: //Page-Up
			m_pstScrollV->ScrollPageUp();
			break;
		case VK_NEXT:  //Page-Down
			m_pstScrollV->ScrollPageDown();
			break;
		case VK_HOME:
			m_pstScrollV->ScrollHome();
			break;
		case VK_END:
			m_pstScrollV->ScrollEnd();
			break;
		}
	}
}

void CHexCtrl::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (!m_fMutable)
		return;

	if (GetKeyState(VK_CONTROL) < 0)
		return;

	HEXCTRL_INTERNAL::HEXMODIFYDATA hmd;
	hmd.enType = HEXCTRL_INTERNAL::HEX_MODIFYAS::AS_STANDARD;
	hmd.ullIndex = m_ullCursorPos;
	hmd.ullModifySize = 1;
	hmd.fMoveNext = true;

	if (m_fCursorAscii) //If cursor is at Ascii area.
	{
		hmd.fWhole = true;
		hmd.pData = (PBYTE)&nChar;
	}
	else
	{
		if (nChar >= 0x30 && nChar <= 0x39)		 //Digits.
			nChar -= 0x30;
		else if (nChar >= 0x41 && nChar <= 0x46) //Hex letters uppercase.
			nChar -= 0x37;
		else if (nChar >= 0x61 && nChar <= 0x66) //Hex letters lowercase.
			nChar -= 0x57;
		else
			return;

		hmd.fWhole = false;
		hmd.fHighPart = m_fCursorHigh;
		hmd.pData = (PBYTE)&nChar;
	}

	ModifyData(hmd);
}

UINT CHexCtrl::OnGetDlgCode()
{
	return DLGC_WANTALLKEYS;
}

void CHexCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (m_pstScrollV->GetScrollPosDelta() != 0)
		RedrawWindow();
}

void CHexCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (m_pstScrollH->GetScrollPosDelta() != 0)
		RedrawWindow();
}

void CHexCtrl::OnPaint()
{
	CPaintDC dc(this);
	if (!IsCreated()) {
		CRect rc;
		dc.GetClipBox(rc);
		dc.FillSolidRect(rc, RGB(250, 250, 250));
		dc.TextOutW(1, 1, L"Call CHexCtrl::Create first.");
		return;
	}

	int iScrollH = (int)m_pstScrollH->GetScrollPos();
	CRect rcClient;
	GetClientRect(rcClient);
	//Drawing through CMemDC to avoid flickering.
	CMemDC memDC(dc, rcClient);
	CDC& rDC = memDC.GetDC();

	RECT rc; //Used for all local rect related drawing.
	rDC.GetClipBox(&rc);
	rDC.FillSolidRect(&rc, m_stColor.clrBk);
	rDC.SelectObject(&m_penLines);
	rDC.SelectObject(&m_fontHexView);

	//To prevent drawing in too small window (can cause hangs).
	if (rcClient.Height() < m_iHeightTopRect + m_iHeightBottomOffArea)
		return;

	//Find the ullLineStart and ullLineEnd position, draw the visible portion.
	const ULONGLONG ullLineStart = GetCurrentLineV();
	ULONGLONG ullLineEndtmp = 0;
	if (m_ullDataSize) {
		ullLineEndtmp = ullLineStart + (rcClient.Height() - m_iHeightTopRect - m_iHeightBottomOffArea) / m_sizeLetter.cy;
		//If m_dwDataCount is really small we adjust dwLineEnd to be not bigger than maximum allowed.
		if (ullLineEndtmp > (m_ullDataSize / m_dwCapacity))
			ullLineEndtmp = (m_ullDataSize % m_dwCapacity) ? m_ullDataSize / m_dwCapacity + 1 : m_ullDataSize / m_dwCapacity;
	}
	const ULONGLONG ullLineEnd = ullLineEndtmp;

	const auto iFirstHorizLine = 0;
	const auto iSecondHorizLine = iFirstHorizLine + m_iHeightTopRect - 1;
	const auto iThirdHorizLine = iFirstHorizLine + rcClient.Height() - m_iHeightBottomOffArea;
	const auto iFourthHorizLine = iThirdHorizLine + m_iHeightBottomRect;

	//First horizontal line.
	rDC.MoveTo(0, iFirstHorizLine);
	rDC.LineTo(m_iFourthVertLine, iFirstHorizLine);

	//Second horizontal line.
	rDC.MoveTo(0, iSecondHorizLine);
	rDC.LineTo(m_iFourthVertLine, iSecondHorizLine);

	//Third horizontal line.
	rDC.MoveTo(0, iThirdHorizLine);
	rDC.LineTo(m_iFourthVertLine, iThirdHorizLine);

	//Fourth horizontal line.
	rDC.MoveTo(0, iFourthHorizLine);
	rDC.LineTo(m_iFourthVertLine, iFourthHorizLine);

	//«Offset» text.
	rc.left = m_iFirstVertLine - iScrollH; rc.top = iFirstHorizLine;
	rc.right = m_iSecondVertLine - iScrollH; rc.bottom = iSecondHorizLine;
	rDC.SetTextColor(m_stColor.clrTextCaption);
	rDC.DrawTextW(L"Offset", 6, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	//«Bytes total:» text.
	rc.left = m_iFirstVertLine + 5;	rc.top = iThirdHorizLine + 1;
	rc.right = rcClient.right > m_iFourthVertLine ? rcClient.right : m_iFourthVertLine;
	rc.bottom = iFourthHorizLine;
	rDC.FillSolidRect(&rc, m_stColor.clrBkInfoRect);
	rDC.SetTextColor(m_stColor.clrTextInfoRect);
	rDC.SelectObject(&m_fontBottomRect);
	rDC.DrawTextW(m_wstrBottomText.data(), (int)m_wstrBottomText.size(), &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

	rDC.SelectObject(&m_fontHexView);
	rDC.SetTextColor(m_stColor.clrTextCaption);
	rDC.SetBkColor(m_stColor.clrBk);

	int iCapacityShowAs = 1;
	int iIndentCapacityX = 0;
	//Loop for printing top Capacity numbers.
	for (unsigned iterCapacity = 0; iterCapacity < m_dwCapacity; iterCapacity++)
	{
		int x = m_iIndentFirstHexChunk + m_sizeLetter.cx + iIndentCapacityX; //Top capacity numbers (0 1 2 3 4 5 6 7...)
		//Top capacity numbers, second block (8 9 A B C D E F...).
		if (iterCapacity >= m_dwCapacityBlockSize && m_dwShowAs == HEXCTRL_INTERNAL::HEX_SHOWAS::ASBYTE)
			x = m_iIndentFirstHexChunk + m_sizeLetter.cx + iIndentCapacityX + m_iSpaceBetweenBlocks;

		//If iterCapacity >= 16 (0xA), then two chars needed (10, 11,... 1F) to be printed.
		int c = 1;
		if (iterCapacity >= 16) {
			c = 2;
			x -= m_sizeLetter.cx;
		}
		ExtTextOutW(rDC.m_hDC, x - iScrollH, iFirstHorizLine + m_iIndentTextCapacityY, NULL, nullptr,
			m_umapCapacityWstr.at(iterCapacity).data(), c, nullptr);

		iIndentCapacityX += m_iSizeHexByte;
		if (iCapacityShowAs == m_dwShowAs) {
			iIndentCapacityX += m_iSpaceBetweenHexChunks;
			iCapacityShowAs = 1;
		}
		else
			iCapacityShowAs++;
	}
	//"Ascii" text.
	rc.left = m_iThirdVertLine - iScrollH; rc.top = iFirstHorizLine;
	rc.right = m_iFourthVertLine - iScrollH; rc.bottom = iSecondHorizLine;
	rDC.DrawTextW(L"Ascii", 5, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	//First Vertical line.
	rDC.MoveTo(m_iFirstVertLine - iScrollH, 0);
	rDC.LineTo(m_iFirstVertLine - iScrollH, iFourthHorizLine);

	//Second Vertical line.
	rDC.MoveTo(m_iSecondVertLine - iScrollH, 0);
	rDC.LineTo(m_iSecondVertLine - iScrollH, iThirdHorizLine);

	//Third Vertical line.
	rDC.MoveTo(m_iThirdVertLine - iScrollH, 0);
	rDC.LineTo(m_iThirdVertLine - iScrollH, iThirdHorizLine);

	//Fourth Vertical line.
	rDC.MoveTo(m_iFourthVertLine - iScrollH, 0);
	rDC.LineTo(m_iFourthVertLine - iScrollH, iFourthHorizLine);

	//Current line to print.
	int iLine = 0;
	//Loop for printing hex and Ascii line by line.
	for (ULONGLONG iterLines = ullLineStart; iterLines < ullLineEnd; iterLines++)
	{
		//Drawing offset with bk color depending on selection range.
		if (m_ullSelectionSize && (iterLines * m_dwCapacity + m_dwCapacity) > m_ullSelectionStart &&
			(iterLines * m_dwCapacity) < m_ullSelectionEnd)
			rDC.SetBkColor(m_stColor.clrBkSelected);
		else
			rDC.SetBkColor(m_stColor.clrBk);

		//Left column offset printing (00000001...0000FFFF...).
		wchar_t pwszOffset[16];
		ToWchars(iterLines * m_dwCapacity, pwszOffset, m_dwOffsetDigits / 2);
		rDC.SetTextColor(m_stColor.clrTextCaption);
		ExtTextOutW(rDC.m_hDC, m_sizeLetter.cx - iScrollH, m_iHeightTopRect + (m_sizeLetter.cy * iLine),
			NULL, nullptr, pwszOffset, m_dwOffsetDigits, nullptr);

		int iIndentHexX = 0;
		int iIndentAsciiX = 0;
		int iShowAs = 1;
		//Main loop for printing Hex chunks and Ascii chars.
		for (unsigned iterChunks = 0; iterChunks < m_dwCapacity; iterChunks++)
		{
			//Additional space between capacity halves. Only with BYTEs representation.
			int iIndentBetweenBlocks = 0;
			if (iterChunks >= m_dwCapacityBlockSize && m_dwShowAs == HEXCTRL_INTERNAL::HEX_SHOWAS::ASBYTE)
				iIndentBetweenBlocks = m_iSpaceBetweenBlocks;

			const UINT iHexPosToPrintX = m_iIndentFirstHexChunk + iIndentHexX + iIndentBetweenBlocks - iScrollH;
			const UINT iHexPosToPrintY = m_iHeightTopRect + m_sizeLetter.cy * iLine;
			const UINT iAsciiPosToPrintX = m_iIndentAscii + iIndentAsciiX - iScrollH;
			const UINT iAsciiPosToPrintY = m_iHeightTopRect + m_sizeLetter.cy * iLine;

			//Index of the next char (in m_pData) to draw.
			const ULONGLONG ullIndexByteToPrint = iterLines * m_dwCapacity + iterChunks;

			if (ullIndexByteToPrint < m_ullDataSize) //Draw until reaching the end of m_dwDataCount.
			{
				//Hex chunk to print.
				unsigned char chByteToPrint = GetByte(ullIndexByteToPrint);
				const wchar_t* const pwszHexMap { L"0123456789ABCDEF" };
				wchar_t pwszHexToPrint[2];
				pwszHexToPrint[0] = pwszHexMap[(chByteToPrint & 0xF0) >> 4];
				pwszHexToPrint[1] = pwszHexMap[(chByteToPrint & 0x0F)];

				//Selection draw with different BK color.
				COLORREF clrBk, clrBkAscii, clrTextHex, clrTextAscii;
				if (m_ullSelectionSize && ullIndexByteToPrint >= m_ullSelectionStart && ullIndexByteToPrint < m_ullSelectionEnd)
				{
					clrBk = clrBkAscii = m_stColor.clrBkSelected;
					clrTextHex = clrTextAscii = m_stColor.clrTextSelected;
					//Space between hex chunks (excluding last hex in a row) filling with bk_selected color.
					if (ullIndexByteToPrint < (m_ullSelectionEnd - 1) && (ullIndexByteToPrint + 1) % m_dwCapacity &&
						((ullIndexByteToPrint + 1) % m_dwShowAs) == 0)
					{	//Rect of the space between Hex chunks. Needed for proper selection drawing.
						rc.left = iHexPosToPrintX + m_iSizeHexByte;
						rc.right = rc.left + m_iSpaceBetweenHexChunks;
						rc.top = iHexPosToPrintY;
						rc.bottom = iHexPosToPrintY + m_sizeLetter.cy;
						if (iterChunks == m_dwCapacityBlockSize - 1) //Space between capacity halves.
							rc.right += m_iSpaceBetweenBlocks;

						rDC.FillRect(&rc, &m_stBrushBkSelected);
					}
				}
				else
				{
					clrBk = clrBkAscii = m_stColor.clrBk;
					clrTextHex = m_stColor.clrTextHex;
					clrTextAscii = m_stColor.clrTextAscii;
				}
				//Hex chunk printing.
				if (m_fMutable && ullIndexByteToPrint == m_ullCursorPos)
				{
					rDC.SetBkColor(m_fCursorHigh ? m_stColor.clrBkCursor : clrBk);
					rDC.SetTextColor(m_fCursorHigh ? m_stColor.clrTextCursor : clrTextHex);
					ExtTextOutW(rDC.m_hDC, iHexPosToPrintX, iHexPosToPrintY, 0, nullptr, &pwszHexToPrint[0], 1, nullptr);
					rDC.SetBkColor(!m_fCursorHigh ? m_stColor.clrBkCursor : clrBk);
					rDC.SetTextColor(!m_fCursorHigh ? m_stColor.clrTextCursor : clrTextHex);
					ExtTextOutW(rDC.m_hDC, iHexPosToPrintX + m_sizeLetter.cx, iHexPosToPrintY, 0, nullptr, &pwszHexToPrint[1], 1, nullptr);
				}
				else
				{
					rDC.SetBkColor(clrBk);
					rDC.SetTextColor(clrTextHex);
					ExtTextOutW(rDC.m_hDC, iHexPosToPrintX, iHexPosToPrintY, 0, nullptr, pwszHexToPrint, 2, nullptr);
				}

				//Ascii to print.
				wchar_t wchAscii = chByteToPrint;
				if (wchAscii < 32 || wchAscii >= 0x7f) //For non printable Ascii just print a dot.
					wchAscii = '.';

				//Ascii printing.
				if (m_fMutable && ullIndexByteToPrint == m_ullCursorPos) {
					clrBkAscii = m_stColor.clrBkCursor;
					clrTextAscii = m_stColor.clrTextCursor;
				}
				rDC.SetBkColor(clrBkAscii);
				rDC.SetTextColor(clrTextAscii);
				ExtTextOutW(rDC.m_hDC, iAsciiPosToPrintX, iAsciiPosToPrintY, 0, nullptr, &wchAscii, 1, nullptr);
			}
			//Increasing indents for next print, for both - Hex and Ascii.
			iIndentHexX += m_iSizeHexByte;
			if (iShowAs == m_dwShowAs) {
				iIndentHexX += m_iSpaceBetweenHexChunks;
				iShowAs = 1;
			}
			else
				iShowAs++;
			iIndentAsciiX += m_iSpaceBetweenAscii;
		}
		iLine++;
	}
}

void CHexCtrl::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	RecalcWorkAreaHeight(cy);
	RecalcScrollPageSize();
}

BOOL CHexCtrl::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}

BOOL CHexCtrl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	m_pstScrollV->OnSetCursor(pWnd, nHitTest, message);
	m_pstScrollH->OnSetCursor(pWnd, nHitTest, message);

	return CWnd::OnSetCursor(pWnd, nHitTest, message);
}

BOOL CHexCtrl::OnNcActivate(BOOL bActive)
{
	m_pstScrollV->OnNcActivate(bActive);
	m_pstScrollH->OnNcActivate(bActive);

	return CWnd::OnNcActivate(bActive);
}

void CHexCtrl::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
	CWnd::OnNcCalcSize(bCalcValidRects, lpncsp);

	//Sequence is important — H->V.
	m_pstScrollH->OnNcCalcSize(bCalcValidRects, lpncsp);
	m_pstScrollV->OnNcCalcSize(bCalcValidRects, lpncsp);
}

void CHexCtrl::OnNcPaint()
{
	Default();

	m_pstScrollV->OnNcPaint();
	m_pstScrollH->OnNcPaint();
}

void CHexCtrl::RecalcAll()
{
	ULONGLONG ullCurLineV = GetCurrentLineV();

	//Current font size related.
	HDC hDC = ::GetDC(m_hWnd);
	SelectObject(hDC, m_fontHexView.m_hObject);
	TEXTMETRICW tm { };
	GetTextMetricsW(hDC, &tm);
	m_sizeLetter.cx = tm.tmAveCharWidth;
	m_sizeLetter.cy = tm.tmHeight + tm.tmExternalLeading;
	::ReleaseDC(m_hWnd, hDC);

	m_iFirstVertLine = 0;
	m_iSecondVertLine = m_dwOffsetDigits * m_sizeLetter.cx + m_sizeLetter.cx * 2;
	m_iSizeHexByte = m_sizeLetter.cx * 2;
	m_iSpaceBetweenBlocks = m_sizeLetter.cx * 2;
	m_iSpaceBetweenHexChunks = m_sizeLetter.cx;
	m_iDistanceBetweenHexChunks = m_iSizeHexByte * m_dwShowAs + m_iSpaceBetweenHexChunks;
	m_iThirdVertLine = m_iSecondVertLine + m_iDistanceBetweenHexChunks * (m_dwCapacity / m_dwShowAs)
		+ m_iSpaceBetweenBlocks + m_sizeLetter.cx;
	m_iIndentAscii = m_iThirdVertLine + m_sizeLetter.cx;
	m_iSpaceBetweenAscii = m_sizeLetter.cx + 1;
	m_iFourthVertLine = m_iIndentAscii + (m_iSpaceBetweenAscii * m_dwCapacity) + m_sizeLetter.cx;
	m_iIndentFirstHexChunk = m_iSecondVertLine + m_sizeLetter.cx;
	m_iSizeFirstHalf = m_iIndentFirstHexChunk + m_dwCapacityBlockSize * (m_sizeLetter.cx * 2) +
		(m_dwCapacityBlockSize / m_dwShowAs - 1) * m_iSpaceBetweenHexChunks;
	m_iHeightTopRect = int(m_sizeLetter.cy * 1.5);
	m_iIndentTextCapacityY = m_iHeightTopRect / 2 - (m_sizeLetter.cy / 2);

	RecalcScrollSizes();
	m_pstScrollV->SetScrollPos(ullCurLineV * m_sizeLetter.cy);

	RedrawWindow();
}

void CHexCtrl::RecalcWorkAreaHeight(int iClientHeight)
{
	m_iHeightWorkArea = iClientHeight - m_iHeightBottomOffArea -
		((iClientHeight - m_iHeightTopRect - m_iHeightBottomOffArea) % m_sizeLetter.cy);
}

void CHexCtrl::RecalcScrollSizes(int iClientHeight, int iClientWidth)
{
	if (iClientHeight == 0 && iClientWidth == 0)
	{
		CRect rc;
		GetClientRect(&rc);
		iClientHeight = rc.Height();
		iClientWidth = rc.Width();
	}

	RecalcWorkAreaHeight(iClientHeight);
	//Scroll sizes according to current font size.
	m_pstScrollV->SetScrollSizes(m_sizeLetter.cy, m_iHeightWorkArea - m_iHeightTopRect,
		m_iHeightTopRect + m_iHeightBottomOffArea + m_sizeLetter.cy * (m_ullDataSize / m_dwCapacity + 2));
	m_pstScrollH->SetScrollSizes(m_sizeLetter.cx, iClientWidth, m_iFourthVertLine + 1);
}

void CHexCtrl::RecalcScrollPageSize()
{
	UINT uiPage = m_iHeightWorkArea - m_iHeightTopRect;
	m_pstScrollV->SetScrollPageSize(uiPage);
}

ULONGLONG CHexCtrl::GetCurrentLineV()
{
	return m_pstScrollV->GetScrollPos() / m_sizeLetter.cy;
}

ULONGLONG CHexCtrl::HitTest(LPPOINT pPoint)
{
	int iY = pPoint->y;
	int iX = pPoint->x + (int)m_pstScrollH->GetScrollPos(); //To compensate horizontal scroll.
	ULONGLONG ullCurLine = GetCurrentLineV();
	ULONGLONG ullHexChunk;
	m_fCursorAscii = false;

	//Checking if cursor is within hex chunks area.
	if ((iX >= m_iIndentFirstHexChunk) && (iX < m_iThirdVertLine) && (iY >= m_iHeightTopRect) && (iY <= m_iHeightWorkArea))
	{
		//Additional space between halves. Only in BYTE's view mode.
		int iBetweenBlocks;
		if (m_dwShowAs == HEXCTRL_INTERNAL::HEX_SHOWAS::ASBYTE && iX > m_iSizeFirstHalf)
			iBetweenBlocks = m_iSpaceBetweenBlocks;
		else
			iBetweenBlocks = 0;

		//Calculate hex chunk.
		DWORD dwX = iX - m_iIndentFirstHexChunk - iBetweenBlocks;
		DWORD dwChunkX { };
		int iSpaceBetweenHexChunks = 0;
		for (unsigned i = 1; i <= m_dwCapacity; i++)
		{
			if ((i % m_dwShowAs) == 0)
				iSpaceBetweenHexChunks += m_iSpaceBetweenHexChunks;
			if ((m_iSizeHexByte * i + iSpaceBetweenHexChunks) > dwX)
			{
				dwChunkX = i - 1;
				break;
			}
		}
		ullHexChunk = dwChunkX + ((iY - m_iHeightTopRect) / m_sizeLetter.cy) * m_dwCapacity + (ullCurLine  * m_dwCapacity);
	}
	else if ((iX >= m_iIndentAscii) && (iX < (m_iIndentAscii + m_iSpaceBetweenAscii * (int)m_dwCapacity))
		&& (iY >= m_iHeightTopRect) && iY <= m_iHeightWorkArea)
	{
		//Calculate ullHit Ascii symbol.
		ullHexChunk = ((iX - m_iIndentAscii) / m_iSpaceBetweenAscii) +
			((iY - m_iHeightTopRect) / m_sizeLetter.cy) * m_dwCapacity + (ullCurLine * m_dwCapacity);
		m_fCursorAscii = true;
	}
	else
		ullHexChunk = -1;

	//If cursor is out of end-bound of hex chunks or Ascii chars.
	if (ullHexChunk >= m_ullDataSize)
		ullHexChunk = -1;

	return ullHexChunk;
}

void CHexCtrl::HexPoint(ULONGLONG ullChunk, ULONGLONG& ullCx, ULONGLONG& ullCy)
{
	int iBetweenBlocks;
	if (m_dwShowAs == HEXCTRL_INTERNAL::HEX_SHOWAS::ASBYTE && (ullChunk % m_dwCapacity) > m_dwCapacityBlockSize)
		iBetweenBlocks = m_iSpaceBetweenBlocks;
	else
		iBetweenBlocks = 0;

	ullCx = m_iIndentFirstHexChunk + iBetweenBlocks + (ullChunk % m_dwCapacity) * m_iSizeHexByte;
	ullCx += ((ullChunk % m_dwCapacity) / m_dwShowAs) * m_iSpaceBetweenHexChunks;

	if (ullChunk % m_dwCapacity)
		ullCy = (ullChunk / m_dwCapacity) * m_sizeLetter.cy + m_sizeLetter.cy;
	else
		ullCy = (ullChunk / m_dwCapacity) * m_sizeLetter.cy;
}

void CHexCtrl::ClipboardCopy(DWORD dwType)
{
	if (!m_ullSelectionSize)
		return;

	const char* const pszHexMap { "0123456789ABCDEF" };
	std::string strToClipboard;

	switch (dwType)
	{
	case HEXCTRL_INTERNAL::HEX_CLIPBOARD::COPY_ASHEX:
	{
		for (unsigned i = 0; i < m_ullSelectionSize; i++)
		{
			BYTE chByte = GetByte(m_ullSelectionStart + i);
			strToClipboard += pszHexMap[(chByte & 0xF0) >> 4];
			strToClipboard += pszHexMap[(chByte & 0x0F)];
		}
		break;
	}
	case HEXCTRL_INTERNAL::HEX_CLIPBOARD::COPY_ASHEXFORMATTED:
	{
		//How many spaces are needed to be inserted at the beginnig.
		DWORD dwModStart = m_ullSelectionStart % m_dwCapacity;

		//When to insert first "\r\n".
		DWORD dwTail = m_dwCapacity - dwModStart;
		DWORD dwNextBlock = (m_dwCapacity % 2) ? m_dwCapacityBlockSize + 2 : m_dwCapacityBlockSize + 1;

		//If at least two rows are selected.
		if (dwModStart + m_ullSelectionSize > m_dwCapacity)
		{
			size_t sCount = (dwModStart * 2) + (dwModStart / m_dwShowAs);
			//Additional spaces between halves. Only in ASBYTE's view mode.
			sCount += m_dwShowAs == HEXCTRL_INTERNAL::HEX_SHOWAS::ASBYTE ? (dwTail <= m_dwCapacityBlockSize ? 2 : 0) : 0;
			strToClipboard.insert(0, sCount, ' ');
		}

		for (unsigned i = 0; i < m_ullSelectionSize; i++)
		{
			BYTE chByte = GetByte(m_ullSelectionStart + i);
			strToClipboard += pszHexMap[(chByte & 0xF0) >> 4];
			strToClipboard += pszHexMap[(chByte & 0x0F)];

			if (i < (m_ullSelectionSize - 1) && (dwTail - 1) != 0)
				if (m_dwShowAs == HEXCTRL_INTERNAL::HEX_SHOWAS::ASBYTE && dwTail == dwNextBlock)
					strToClipboard += "   "; //Additional spaces between halves. Only in BYTE's view mode.
				else if (((m_dwCapacity - dwTail + 1) % m_dwShowAs) == 0) //Add space after hex full chunk, ShowAs_size depending.
					strToClipboard += " ";
			if (--dwTail == 0 && i < (m_ullSelectionSize - 1)) //Next row.
			{
				strToClipboard += "\r\n";
				dwTail = m_dwCapacity;
			}
		}
		break;
	}
	case HEXCTRL_INTERNAL::HEX_CLIPBOARD::COPY_ASASCII:
	{
		for (unsigned i = 0; i < m_ullSelectionSize; i++)
		{
			char ch = GetByte(m_ullSelectionStart + i);
			//If next byte is zero —> substitute it with space.
			if (ch == 0)
				ch = ' ';
			strToClipboard += ch;
		}
		break;
	}
	}

	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, strToClipboard.length() + 1);
	if (!hMem) {
		MessageBox(L"GlobalAlloc error.", L"Error", MB_ICONERROR);
		return;
	}
	LPVOID hMemLock = GlobalLock(hMem);
	if (!hMemLock) {
		MessageBox(L"GlobalLock error.", L"Error", MB_ICONERROR);
		return;
	}
	memcpy(hMemLock, strToClipboard.data(), strToClipboard.length() + 1);
	GlobalUnlock(hMem);
	OpenClipboard();
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hMem);
	CloseClipboard();
}

void CHexCtrl::ClipboardPaste(DWORD dwType)
{
	if (!m_fMutable || m_ullSelectionSize == 0)
		return;

	if (!OpenClipboard())
		return;

	HANDLE hClipboard = GetClipboardData(CF_TEXT);
	if (!hClipboard)
		return;

	LPSTR pClipboardData = (LPSTR)GlobalLock(hClipboard);
	if (!pClipboardData)
		return;

	ULONGLONG ullSize = strlen(pClipboardData);
	if (m_ullCursorPos + ullSize > m_ullDataSize)
		ullSize = m_ullDataSize - m_ullCursorPos;

	HEXCTRL_INTERNAL::HEXMODIFYDATA hmd;
	hmd.enType = HEXCTRL_INTERNAL::HEX_MODIFYAS::AS_STANDARD;
	hmd.ullIndex = m_ullCursorPos;
	hmd.fWhole = true;
	hmd.fMoveNext = false;

	std::string strData;
	switch (dwType)
	{
	case HEXCTRL_INTERNAL::HEX_CLIPBOARD::PASTE_ASASCII:
		hmd.pData = (PBYTE)pClipboardData;
		hmd.ullModifySize = ullSize;
		break;
	case HEXCTRL_INTERNAL::HEX_CLIPBOARD::PASTE_ASHEX:
	{
		DWORD dwIterations = DWORD(ullSize / 2 + ullSize % 2);
		char chToUL[3] { }; //Array for actual Ascii chars to convert from.
		for (size_t i = 0; i < dwIterations; i++)
		{
			if (i + 2 <= ullSize)
			{
				chToUL[0] = pClipboardData[i * 2];
				chToUL[1] = pClipboardData[i * 2 + 1];
			}
			else
			{
				chToUL[0] = pClipboardData[i * 2];
				chToUL[1] = '\0';
			}

			unsigned long ulNumber;
			if (!ToUl(chToUL, ulNumber))
				return;

			strData += (unsigned char)ulNumber;
		}
		hmd.pData = (PBYTE)strData.data();
		hmd.ullModifySize = strData.size();
	}
	break;
	}

	ModifyData(hmd);

	GlobalUnlock(hClipboard);
	CloseClipboard();

	RedrawWindow();
}

void CHexCtrl::UpdateInfoText()
{
	if (!m_ullDataSize)
		m_wstrBottomText.clear();
	else
	{
		m_wstrBottomText.resize(128);
		if (m_ullSelectionSize)
			m_wstrBottomText.resize(swprintf_s(&m_wstrBottomText[0], 128, L"Selected: 0x%llX(%llu); Block: 0x%llX(%llu) - 0x%llX(%llu)",
				m_ullSelectionSize, m_ullSelectionSize, m_ullSelectionStart, m_ullSelectionStart, m_ullSelectionEnd - 1, m_ullSelectionEnd - 1));
		else
			m_wstrBottomText.resize(swprintf_s(&m_wstrBottomText[0], 128, L"Bytes total: 0x%llX(%llu)", m_ullDataSize, m_ullDataSize));
	}
	RedrawWindow();
}

BYTE CHexCtrl::GetByte(ULONGLONG ullIndex)
{
	//If it's virtual data control we aquire next byte_to_print from m_pwndMsg window.
	if (m_fVirtual)
	{
		if (!m_pwndMsg)
			return 0;

		HEXNOTIFYSTRUCT hexntfy { { m_hWnd, (UINT)GetDlgCtrlID(), HEXCTRL_MSG_GETDATA } };
		hexntfy.ullIndex = ullIndex;
		hexntfy.ullSize = 1;
		m_pwndMsg->SendMessageW(WM_NOTIFY, hexntfy.hdr.idFrom, (LPARAM)&hexntfy);
		return hexntfy.chByte;
	}
	else
		return m_pData[ullIndex];
}

void CHexCtrl::SetShowAs(DWORD dwShowAs)
{
	//Unchecking all menus and checking only the current needed.
	m_dwShowAs = dwShowAs;
	for (int i = 0; i < m_menuShowAs.GetMenuItemCount(); i++)
		m_menuShowAs.CheckMenuItem(i, MF_UNCHECKED | MF_BYPOSITION);

	int id { };
	switch (dwShowAs)
	{
	case HEXCTRL_INTERNAL::HEX_SHOWAS::ASBYTE:
		id = 0;
		break;
	case HEXCTRL_INTERNAL::HEX_SHOWAS::ASWORD:
		id = 1;
		break;
	case HEXCTRL_INTERNAL::HEX_SHOWAS::ASDWORD:
		id = 2;
		break;
	case HEXCTRL_INTERNAL::HEX_SHOWAS::ASQWORD:
		id = 3;
		break;
	}
	m_menuShowAs.CheckMenuItem(id, MF_CHECKED | MF_BYPOSITION);
	SetCapacity(m_dwCapacity);
	RecalcAll();
}

void CHexCtrl::ModifyData(const HEXCTRL_INTERNAL::HEXMODIFYDATA& hmd)
{
	//Changes byte(s) in memory, High or Low part, depending on hmd.fHighPart.
	if (!m_fMutable || hmd.ullIndex >= m_ullDataSize)
		return;

	if (hmd.enType != HEXCTRL_INTERNAL::HEX_MODIFYAS::AS_UNDO)
	{
		m_deqRedo.clear(); //No Redo unless we make Undo.

		//If Undo size is exceeding max limit,
		//remove first snapshot from the beginning (the ildest one).
		if (m_deqUndo.size() > m_dwUndoMax)
			m_deqUndo.pop_front();

		//Making new Undo data snapshot.
		auto& refUndo = m_deqUndo.emplace_back(std::make_unique<HEXCTRL_INTERNAL::HEXUNDO>());
		refUndo->ullIndex = hmd.ullIndex;
		for (unsigned i = 0; i < hmd.ullModifySize; i++)
			refUndo->strData += GetByte(hmd.ullIndex + i);
	}

	if (!m_fVirtual) //Modify only in non Virtual mode.
	{
		if (hmd.enType == HEXCTRL_INTERNAL::HEX_MODIFYAS::AS_FILL)
		{
			ULONGLONG ullChunks = hmd.ullModifySize / hmd.dwFillDataSize;
			for (ULONGLONG iterChunk = 0; iterChunk < ullChunks; iterChunk++)
				for (ULONGLONG iterData = 0; iterData < hmd.dwFillDataSize; iterData++)
					m_pData[hmd.ullIndex + hmd.dwFillDataSize * iterChunk + iterData] = hmd.pData[iterData];
		}
		else //All the other data modifications: paste, undo, redo, keyboard input...
		{
			if (hmd.fWhole)
				for (ULONGLONG i = 0; i < hmd.ullModifySize; i++)
					m_pData[hmd.ullIndex + i] = hmd.pData[i];
			else
			{	//If just one part (High/Low) of byte must be changed.
				unsigned char chByte = GetByte(hmd.ullIndex);
				if (hmd.fHighPart)
					chByte = (*hmd.pData << 4) | (chByte & 0x0F);
				else
					chByte = (*hmd.pData & 0x0F) | (chByte & 0xF0);

				m_pData[hmd.ullIndex] = chByte;
			}
		}
	}

	if (hmd.fMoveNext)
		CursorMoveRight();

	HEXNOTIFYSTRUCT hns { { m_hWnd, (UINT)GetDlgCtrlID(), HEXCTRL_MSG_MODIFYDATA } };
	hns.ullIndex = hmd.ullIndex;
	hns.ullSize = hmd.ullModifySize;
	hns.pData = hmd.pData;
	ParentNotify(hns);

	RedrawWindow();
}

void CHexCtrl::ParentNotify(const HEXNOTIFYSTRUCT& hns)
{
	if (m_pwndMsg)
		m_pwndMsg->SendMessageW(WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&hns);
}

void CHexCtrl::SetCursorPos(ULONGLONG ullPos, bool fHighPart)
{
	m_ullCursorPos = ullPos;
	if (m_ullCursorPos >= m_ullDataSize)
	{
		m_ullCursorPos = m_ullDataSize - 1;
		m_fCursorHigh = false;
	}
	else
		m_fCursorHigh = fHighPart;

	CursorScroll();
	RedrawWindow();
}

void CHexCtrl::CursorMoveRight()
{
	if (m_fCursorAscii)
		SetCursorPos(m_ullCursorPos + 1, m_fCursorHigh);
	else if (m_fCursorHigh)
		SetCursorPos(m_ullCursorPos, false);
	else
		SetCursorPos(m_ullCursorPos + 1, true);
}

void CHexCtrl::CursorMoveLeft()
{
	if (m_fCursorAscii)
	{
		ULONGLONG ull = m_ullCursorPos;
		if (ull > 0) //To avoid overflow.
			ull--;
		else
			return; //Zero index byte.

		SetCursorPos(ull, m_fCursorHigh);
	}
	else if (m_fCursorHigh)
	{
		ULONGLONG ull = m_ullCursorPos;
		if (ull > 0) //To avoid overflow.
			ull--;
		else
			return; //Zero index byte.

		SetCursorPos(ull, false);
	}
	else
		SetCursorPos(m_ullCursorPos, true);
}

void CHexCtrl::CursorMoveUp()
{
	ULONGLONG ull = m_ullCursorPos;
	if (ull > m_dwCapacity)
		ull -= m_dwCapacity;
	SetCursorPos(ull, m_fCursorHigh);
}

void CHexCtrl::CursorMoveDown()
{
	SetCursorPos(m_ullCursorPos + m_dwCapacity, m_fCursorHigh);
}

void CHexCtrl::CursorScroll()
{
	ULONGLONG ullCurrScrollV = m_pstScrollV->GetScrollPos();
	ULONGLONG ullCurrScrollH = m_pstScrollH->GetScrollPos();
	ULONGLONG ullCx, ullCy;
	HexPoint(m_ullCursorPos, ullCx, ullCy);
	CRect rcClient;
	GetClientRect(&rcClient);

	//New scroll depending on selection direction: top <-> bottom.
	ULONGLONG ullEnd = m_ullCursorPos; //ullEnd is inclusive here.
	ULONGLONG ullMaxV = ullCurrScrollV + rcClient.Height() - m_iHeightBottomOffArea - m_iHeightTopRect -
		((rcClient.Height() - m_iHeightTopRect - m_iHeightBottomOffArea) % m_sizeLetter.cy);
	ULONGLONG ullNewStartV = m_ullCursorPos / m_dwCapacity * m_sizeLetter.cy;
	ULONGLONG ullNewEndV = ullEnd / m_dwCapacity * m_sizeLetter.cy;

	ULONGLONG ullNewScrollV { }, ullNewScrollH { };
	if (ullNewEndV >= ullMaxV)
		ullNewScrollV = ullCurrScrollV + m_sizeLetter.cy;
	else
	{
		if (ullNewEndV >= ullCurrScrollV)
			ullNewScrollV = ullCurrScrollV;
		else if (ullNewStartV <= ullCurrScrollV)
			ullNewScrollV = ullCurrScrollV - m_sizeLetter.cy;
	}

	ULONGLONG ullMaxClient = ullCurrScrollH + rcClient.Width() - m_iSizeHexByte;
	if (ullCx >= ullMaxClient)
		ullNewScrollH = ullCurrScrollH + (ullCx - ullMaxClient);
	else if (ullCx < ullCurrScrollH)
		ullNewScrollH = ullCx;
	else
		ullNewScrollH = ullCurrScrollH;

	ullNewScrollV -= ullNewScrollV % m_sizeLetter.cy;

	m_pstScrollV->SetScrollPos(ullNewScrollV);
	if (m_pstScrollH->IsVisible())
		m_pstScrollH->SetScrollPos(ullNewScrollH);
}

void CHexCtrl::Undo()
{
	if (m_deqUndo.empty())
		return;

	auto& refUndo = m_deqUndo.back();
	auto& refStr = refUndo->strData;

	//Making new Redo data snapshot.
	auto& refRedo = m_deqRedo.emplace_back(std::make_unique<HEXCTRL_INTERNAL::HEXUNDO>());
	refRedo->ullIndex = refUndo->ullIndex;
	for (unsigned i = 0; i < refStr.size(); i++)
		refRedo->strData += GetByte(refUndo->ullIndex + i);

	//Extracting Undo data and forward it to ModifyData.
	HEXCTRL_INTERNAL::HEXMODIFYDATA hmd;
	hmd.enType = HEXCTRL_INTERNAL::HEX_MODIFYAS::AS_UNDO;
	hmd.ullIndex = refUndo->ullIndex;
	hmd.ullModifySize = refStr.size();
	hmd.pData = (PBYTE)refStr.data();
	hmd.fWhole = true;
	hmd.fMoveNext = false;

	ModifyData(hmd);
	m_deqUndo.pop_back();
}

void CHexCtrl::Redo()
{
	if (m_deqRedo.empty())
		return;

	auto& refRedo = m_deqRedo.back();
	auto& refStr = refRedo->strData;

	//Making new Undo data snapshot.
	auto& refUndo = m_deqUndo.emplace_back(std::make_unique<HEXCTRL_INTERNAL::HEXUNDO>());
	refUndo->ullIndex = refRedo->ullIndex;
	for (unsigned i = 0; i < refStr.size(); i++)
		refUndo->strData += GetByte(refRedo->ullIndex + i);

	//Extracting Redo data and forward it to ModifyData.
	HEXCTRL_INTERNAL::HEXMODIFYDATA hmd;
	hmd.enType = HEXCTRL_INTERNAL::HEX_MODIFYAS::AS_UNDO;
	hmd.ullIndex = refRedo->ullIndex;
	hmd.ullModifySize = refStr.size();
	hmd.pData = (PBYTE)refStr.data();
	hmd.fWhole = true;
	hmd.fMoveNext = false;

	ModifyData(hmd);
	m_deqRedo.pop_back();
}

void CHexCtrl::Search(HEXCTRL_INTERNAL::HEXSEARCH& rSearch)
{
	rSearch.fFound = false;
	ULONGLONG ullStartAt = rSearch.ullStartAt;
	ULONGLONG ullSizeBytes;
	ULONGLONG ullUntil;
	std::string strSearch { };

	if (rSearch.wstrSearch.empty() || m_ullDataSize == 0 || rSearch.ullStartAt > (m_ullDataSize - 1))
		return m_pDlgSearch->SearchCallback();

	int iSizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &rSearch.wstrSearch[0], (int)rSearch.wstrSearch.size(), nullptr, 0, nullptr, nullptr);
	std::string strSearchAscii(iSizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, &rSearch.wstrSearch[0], (int)rSearch.wstrSearch.size(), &strSearchAscii[0], iSizeNeeded, nullptr, nullptr);

	switch (rSearch.dwSearchType)
	{
	case HEXCTRL_INTERNAL::HEX_SEARCH::SEARCH_HEX:
	{
		DWORD dwIterations = DWORD(strSearchAscii.size() / 2 + strSearchAscii.size() % 2);
		std::string strToUL; //String to hold currently extracted two letters.

		for (size_t i = 0; i < dwIterations; i++)
		{
			if (i + 2 <= strSearchAscii.size())
				strToUL = strSearchAscii.substr(i * 2, 2);
			else
				strToUL = strSearchAscii.substr(i * 2, 1);

			unsigned long ulNumber;
			if (!ToUl(strToUL.data(), ulNumber))
			{
				rSearch.fFound = false;
				m_pDlgSearch->SearchCallback();
				return;
			}

			strSearch += (unsigned char)ulNumber;
		}

		ullSizeBytes = strSearch.size();
		if (ullSizeBytes > m_ullDataSize)
			goto End;

		break;
	}
	case HEXCTRL_INTERNAL::HEX_SEARCH::SEARCH_ASCII:
	{
		ullSizeBytes = strSearchAscii.size();
		if (ullSizeBytes > m_ullDataSize)
			goto End;

		strSearch = std::move(strSearchAscii);
		break;
	}
	case HEXCTRL_INTERNAL::HEX_SEARCH::SEARCH_UNICODE:
	{
		ullSizeBytes = rSearch.wstrSearch.length() * sizeof(wchar_t);
		if (ullSizeBytes > m_ullDataSize)
			goto End;

		break;
	}
	}

	///////////////Actual Search:////////////////////////////////////////////
	switch (rSearch.dwSearchType) {
	case HEXCTRL_INTERNAL::HEX_SEARCH::SEARCH_HEX:
	case HEXCTRL_INTERNAL::HEX_SEARCH::SEARCH_ASCII:
	{
		if (rSearch.iDirection == HEXCTRL_INTERNAL::HEX_SEARCH::SEARCH_FORWARD)
		{
			ullUntil = m_ullDataSize - strSearch.size();
			ullStartAt = rSearch.fSecondMatch ? rSearch.ullStartAt + 1 : 0;

			for (ULONGLONG i = ullStartAt; i <= ullUntil; i++)
			{
				if (memcmp(m_pData + i, strSearch.data(), strSearch.size()) == 0)
				{
					rSearch.fFound = true;
					rSearch.ullStartAt = i;
					rSearch.fWrap = false;
					goto End;
				}
			}

			ullStartAt = 0;
			for (ULONGLONG i = ullStartAt; i <= ullUntil; i++)
			{
				if (memcmp(m_pData + i, strSearch.data(), strSearch.size()) == 0)
				{
					rSearch.fFound = true;
					rSearch.ullStartAt = i;
					rSearch.fWrap = true;
					rSearch.iWrap = HEXCTRL_INTERNAL::HEX_SEARCH::SEARCH_END;
					rSearch.fCount = true;
					goto End;
				}
			}
		}
		if (rSearch.iDirection == HEXCTRL_INTERNAL::HEX_SEARCH::SEARCH_BACKWARD)
		{
			if (rSearch.fSecondMatch && ullStartAt > 0)
			{
				ullStartAt--;
				for (int i = (int)ullStartAt; i >= 0; i--)
				{
					if (memcmp(m_pData + i, strSearch.data(), strSearch.size()) == 0)
					{
						rSearch.fFound = true;
						rSearch.ullStartAt = i;
						rSearch.fWrap = false;
						goto End;
					}
				}
			}

			ullStartAt = m_ullDataSize - strSearch.size();
			for (int i = (int)ullStartAt; i >= 0; i--)
			{
				if (memcmp(m_pData + i, strSearch.data(), strSearch.size()) == 0)
				{
					rSearch.fFound = true;
					rSearch.ullStartAt = i;
					rSearch.fWrap = true;
					rSearch.iWrap = HEXCTRL_INTERNAL::HEX_SEARCH::SEARCH_BEGINNING;
					rSearch.fCount = false;
					goto End;
				}
			}
		}
		break;
	}
	case HEXCTRL_INTERNAL::HEX_SEARCH::SEARCH_UNICODE:
	{
		if (rSearch.iDirection == HEXCTRL_INTERNAL::HEX_SEARCH::SEARCH_FORWARD)
		{
			ullUntil = m_ullDataSize - ullSizeBytes;
			ullStartAt = rSearch.fSecondMatch ? rSearch.ullStartAt + 1 : 0;

			for (ULONGLONG i = ullStartAt; i <= ullUntil; i++)
			{
				if (wmemcmp((const wchar_t*)(m_pData + i), rSearch.wstrSearch.data(), rSearch.wstrSearch.length()) == 0)
				{
					rSearch.fFound = true;
					rSearch.ullStartAt = i;
					rSearch.fWrap = false;
					goto End;
				}
			}

			ullStartAt = 0;
			for (ULONGLONG i = ullStartAt; i <= ullUntil; i++)
			{
				if (wmemcmp((const wchar_t*)(m_pData + i), rSearch.wstrSearch.data(), rSearch.wstrSearch.length()) == 0)
				{
					rSearch.fFound = true;
					rSearch.ullStartAt = i;
					rSearch.iWrap = HEXCTRL_INTERNAL::HEX_SEARCH::SEARCH_END;
					rSearch.fWrap = true;
					rSearch.fCount = true;
					goto End;
				}
			}
		}
		else if (rSearch.iDirection == HEXCTRL_INTERNAL::HEX_SEARCH::SEARCH_BACKWARD)
		{
			if (rSearch.fSecondMatch && ullStartAt > 0)
			{
				ullStartAt--;
				for (int i = (int)ullStartAt; i >= 0; i--)
				{
					if (wmemcmp((const wchar_t*)(m_pData + i), rSearch.wstrSearch.data(), rSearch.wstrSearch.length()) == 0)
					{
						rSearch.fFound = true;
						rSearch.ullStartAt = i;
						rSearch.fWrap = false;
						goto End;
					}
				}
			}

			ullStartAt = m_ullDataSize - ullSizeBytes;
			for (int i = (int)ullStartAt; i >= 0; i--)
			{
				if (wmemcmp((const wchar_t*)(m_pData + i), rSearch.wstrSearch.data(), rSearch.wstrSearch.length()) == 0)
				{
					rSearch.fFound = true;
					rSearch.ullStartAt = i;
					rSearch.fWrap = true;
					rSearch.iWrap = HEXCTRL_INTERNAL::HEX_SEARCH::SEARCH_BEGINNING;
					rSearch.fCount = false;
					goto End;
				}
			}
		}
		break;
	}
	}

End:
	{
		m_pDlgSearch->SearchCallback();
		if (rSearch.fFound)
			SetSelection(rSearch.ullStartAt, rSearch.ullStartAt, ullSizeBytes, true);
	}
}

void CHexCtrl::SetSelection(ULONGLONG ullClick, ULONGLONG ullStart, ULONGLONG ullSize, bool fHighlight, bool fMouse)
{
	if (ullClick >= m_ullDataSize || ullStart >= m_ullDataSize || !ullSize)
		return;
	if ((ullStart + ullSize) > m_ullDataSize)
		ullSize = m_ullDataSize - ullStart;

	ULONGLONG ullCurrScrollV = m_pstScrollV->GetScrollPos();
	ULONGLONG ullCurrScrollH = m_pstScrollH->GetScrollPos();
	ULONGLONG ullCx, ullCy;
	HexPoint(ullStart, ullCx, ullCy);
	CRect rcClient;
	GetClientRect(&rcClient);

	//New scroll depending on selection direction: top <-> bottom.
	//fHighlight means centralize scroll position on the screen (used in Search()).
	ULONGLONG ullEnd = ullStart + ullSize; //ullEnd is exclusive.
	ULONGLONG ullMaxV = ullCurrScrollV + rcClient.Height() - m_iHeightBottomOffArea - m_iHeightTopRect -
		((rcClient.Height() - m_iHeightTopRect - m_iHeightBottomOffArea) % m_sizeLetter.cy);
	ULONGLONG ullNewStartV = ullStart / m_dwCapacity * m_sizeLetter.cy;
	ULONGLONG ullNewEndV = (ullEnd - 1) / m_dwCapacity * m_sizeLetter.cy;

	ULONGLONG ullNewScrollV { }, ullNewScrollH { };
	if (fHighlight)
	{
		//To prevent negative numbers.
		if (ullNewStartV > m_iHeightWorkArea / 2)
			ullNewScrollV = ullNewStartV - m_iHeightWorkArea / 2;
		else
			ullNewScrollV = 0;

		ullNewScrollH = (ullStart % m_dwCapacity) * m_iSizeHexByte;
		ullNewScrollH += (ullNewScrollH / m_iDistanceBetweenHexChunks) * m_iSpaceBetweenHexChunks;
	}
	else
	{
		if (ullStart == ullClick)
		{
			if (ullNewEndV >= ullMaxV)
				ullNewScrollV = ullCurrScrollV + m_sizeLetter.cy;
			else
			{
				if (ullNewEndV >= ullCurrScrollV)
					ullNewScrollV = ullCurrScrollV;
				else if (ullNewStartV <= ullCurrScrollV)
					ullNewScrollV = ullCurrScrollV - m_sizeLetter.cy;
			}
		}
		else
		{
			if (ullNewStartV < ullCurrScrollV)
				ullNewScrollV = ullCurrScrollV - m_sizeLetter.cy;
			else
			{
				if (ullNewStartV < ullMaxV)
					ullNewScrollV = ullCurrScrollV;
				else
					ullNewScrollV = ullCurrScrollV + m_sizeLetter.cy;
			}
		}

		ULONGLONG ullMaxClient = ullCurrScrollH + rcClient.Width() - m_iSizeHexByte;
		if (ullCx >= ullMaxClient)
			ullNewScrollH = ullCurrScrollH + (ullCx - ullMaxClient);
		else if (ullCx < ullCurrScrollH)
			ullNewScrollH = ullCx;
		else
			ullNewScrollH = ullCurrScrollH;
	}
	ullNewScrollV -= ullNewScrollV % m_sizeLetter.cy;

	m_ullSelectionClick = ullClick;
	m_ullSelectionStart = m_ullCursorPos = ullStart;
	m_ullSelectionEnd = ullEnd;
	m_ullSelectionSize = ullEnd - ullStart;

	if (!fMouse) //Don't need to scroll if it's Mouse selection.
	{
		m_pstScrollV->SetScrollPos(ullNewScrollV);
		if (m_pstScrollH->IsVisible())
			m_pstScrollH->SetScrollPos(ullNewScrollH);
	}

	UpdateInfoText();

	HEXNOTIFYSTRUCT hns { m_hWnd, (UINT)GetDlgCtrlID(), HEXCTRL_MSG_SETSELECTION };
	hns.ullIndex = m_ullSelectionStart;
	hns.ullSize = m_ullSelectionSize;
	ParentNotify(hns);
}

void CHexCtrl::SelectAll()
{
	if (!m_ullDataSize)
		return;

	m_ullSelectionClick = m_ullSelectionStart = 0;
	m_ullSelectionEnd = m_ullSelectionSize = m_ullDataSize;
	UpdateInfoText();
}