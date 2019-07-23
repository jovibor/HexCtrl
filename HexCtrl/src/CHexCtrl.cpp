/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/                         *
* This is a Hex Control for MFC applications.                                           *
* Official git repository of the project: https://github.com/jovibor/HexCtrl/           *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information, or any questions, visit the project's official repository.      *
****************************************************************************************/
#include "stdafx.h"
#include "strsafe.h"
#include "../res/HexCtrlRes.h" //Icons, for menu, etc...
#include "Dialogs/CHexDlgAbout.h"
#include "Dialogs/CHexDlgSearch.h"
#include "CHexCtrl.h"
#include "CScrollEx.h"
#include "Helper.h"
#pragma comment(lib, "Dwmapi.lib")

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

namespace HEXCTRL {
	/********************************************
	* CreateRawHexCtrl function implementation.	*
	********************************************/
	HEXCTRLAPI CreateRawHexCtrl()
	{
		return new CHexCtrl();
	};

	/********************************************
	* Internal enums and structs.				*
	********************************************/
	namespace INTERNAL {
		enum class EClipboard : DWORD
		{
			COPY_HEX, COPY_HEXFORMATTED, COPY_ASCII,
			PASTE_HEX, PASTE_ASCII
		};

		//Internal menu IDs, start from 0x8001.
		enum class EMenu : UINT_PTR
		{
			IDM_MAIN_SEARCH = 0x8001,
			IDM_SHOWAS_BYTE, IDM_SHOWAS_WORD, IDM_SHOWAS_DWORD, IDM_SHOWAS_QWORD,
			IDM_EDIT_UNDO, IDM_EDIT_REDO,
			IDM_EDIT_COPY_HEX, IDM_EDIT_COPY_HEXFORMATTED, IDM_EDIT_COPY_ASCII,
			IDM_EDIT_PASTE_HEX, IDM_EDIT_PASTE_ASCII,
			IDM_EDIT_FILL_ZEROS,
			IDM_MAIN_ABOUT,
		};

		struct UNDOSTRUCT
		{
			ULONGLONG              ullIndex; //Start byte to apply Undo to.
			std::vector<std::byte> vecData;  //Data for Undo.
		};

		constexpr auto WSTR_HEXCTRL_CLASSNAME = L"HexCtrl";
	}
}

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

	//Submenu for data showing options.
	m_menuShowAs.CreatePopupMenu();
	m_menuShowAs.AppendMenuW(MF_STRING | MF_CHECKED, (UINT_PTR)EMenu::IDM_SHOWAS_BYTE, L"BYTE");
	m_menuShowAs.AppendMenuW(MF_STRING, (UINT_PTR)EMenu::IDM_SHOWAS_WORD, L"WORD");
	m_menuShowAs.AppendMenuW(MF_STRING, (UINT_PTR)EMenu::IDM_SHOWAS_DWORD, L"DWORD");
	m_menuShowAs.AppendMenuW(MF_STRING, (UINT_PTR)EMenu::IDM_SHOWAS_QWORD, L"QWORD");

	//Main menu.
	m_menuMain.CreatePopupMenu();
	m_menuMain.AppendMenuW(MF_STRING, (UINT_PTR)EMenu::IDM_MAIN_SEARCH, L"Search and Replace...	Ctrl+F/Ctrl+H");
	m_menuMain.AppendMenuW(MF_SEPARATOR);
	m_menuMain.AppendMenuW(MF_POPUP, (DWORD_PTR)m_menuShowAs.m_hMenu, L"Show data as...");
	m_menuMain.AppendMenuW(MF_SEPARATOR);
	m_menuMain.AppendMenuW(MF_STRING, (UINT_PTR)EMenu::IDM_EDIT_UNDO, L"Undo	Ctrl+Z");
	m_menuMain.AppendMenuW(MF_STRING, (UINT_PTR)EMenu::IDM_EDIT_REDO, L"Redo	Ctrl+Y");
	m_menuMain.AppendMenuW(MF_SEPARATOR);
	m_menuMain.AppendMenuW(MF_STRING, (UINT_PTR)EMenu::IDM_EDIT_COPY_HEX, L"Copy as Hex	Ctrl+C");

	HINSTANCE hInst = AfxGetInstanceHandle();
	//Menu icons.
	MENUITEMINFOW mii { };
	mii.cbSize = sizeof(MENUITEMINFOW);
	mii.fMask = MIIM_BITMAP;
	mii.hbmpItem = m_umapHBITMAP[(UINT_PTR)EMenu::IDM_EDIT_COPY_HEX] =
		(HBITMAP)LoadImageW(hInst, MAKEINTRESOURCE(IDB_HEXCTRL_MENU_COPY), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	m_menuMain.SetMenuItemInfoW((UINT_PTR)EMenu::IDM_EDIT_COPY_HEX, &mii);

	m_menuMain.AppendMenuW(MF_STRING, (UINT_PTR)EMenu::IDM_EDIT_COPY_HEXFORMATTED, L"Copy as Formatted Hex");
	m_menuMain.AppendMenuW(MF_STRING, (UINT_PTR)EMenu::IDM_EDIT_COPY_ASCII, L"Copy as Ascii");
	m_menuMain.AppendMenuW(MF_SEPARATOR);
	m_menuMain.AppendMenuW(MF_STRING, (UINT_PTR)EMenu::IDM_EDIT_PASTE_HEX, L"Paste as Hex	Ctrl+V");
	mii.hbmpItem = m_umapHBITMAP[(UINT_PTR)EMenu::IDM_EDIT_PASTE_HEX] =
		(HBITMAP)LoadImageW(hInst, MAKEINTRESOURCE(IDB_HEXCTRL_MENU_PASTE), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	m_menuMain.SetMenuItemInfoW((UINT_PTR)EMenu::IDM_EDIT_PASTE_HEX, &mii);

	m_menuMain.AppendMenuW(MF_STRING, (UINT_PTR)EMenu::IDM_EDIT_PASTE_ASCII, L"Paste as Ascii");
	m_menuMain.AppendMenuW(MF_SEPARATOR);
	m_menuMain.AppendMenuW(MF_STRING, (UINT_PTR)EMenu::IDM_EDIT_FILL_ZEROS, L"Fill with zeros");

	mii.hbmpItem = m_umapHBITMAP[(UINT_PTR)EMenu::IDM_EDIT_FILL_ZEROS] =
		(HBITMAP)LoadImageW(hInst, MAKEINTRESOURCE(IDB_HEXCTRL_MENU_FILL_ZEROS), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	m_menuMain.SetMenuItemInfoW((UINT_PTR)EMenu::IDM_EDIT_FILL_ZEROS, &mii);

	m_menuMain.AppendMenuW(MF_SEPARATOR);
	m_menuMain.AppendMenuW(MF_STRING, (UINT_PTR)EMenu::IDM_MAIN_ABOUT, L"About");

	m_pDlgSearch->Create(IDD_HEXCTRL_SEARCH, this);

	FillWstrCapacity();
}

CHexCtrl::~CHexCtrl()
{
	//Deleting all loaded bitmaps.
	for (auto const& i : m_umapHBITMAP)
		DeleteObject(i.second);
}

bool CHexCtrl::Create(const HEXCREATESTRUCT & hcs)
{
	if (m_fCreated) //Already created.
		return false;

	m_fFloat = hcs.fFloat;
	m_hwndMsg = hcs.hwndParent;
	m_stColor = hcs.stColor;

	m_stBrushBkSelected.CreateSolidBrush(m_stColor.clrBkSelected);

	DWORD dwStyle = hcs.dwStyle;
	//1. WS_POPUP style is vital for GetParent to work properly in m_fFloat mode.
	//   Without this style GetParent/GetOwner always return 0, no matter whether pParentWnd is provided to CreateWindowEx or not.
	//2. Created HexCtrl window will always overlap (be on top of) its parent, or owner, window 
	//   if pParentWnd is set (not nullptr) in CreateWindowEx.
	//3. To force HexCtrl window on taskbar the WS_EX_APPWINDOW extended window style must be set.
	if (m_fFloat)
		dwStyle |= WS_VISIBLE | WS_POPUP | WS_OVERLAPPEDWINDOW;
	else
		dwStyle |= WS_VISIBLE | WS_CHILD;

	//Font related.//////////////////////////////////////////////
	LOGFONTW lf { };
	if (hcs.pLogFont)
		lf = *hcs.pLogFont;
	else
	{
		StringCchCopyW(lf.lfFaceName, 9, L"Consolas");
		lf.lfHeight = 18;
	}

	m_fontHexView.CreateFontIndirectW(&lf);
	m_lFontSize = lf.lfHeight;

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

	//If it's a custom dialog control, there is no need to create a window.
	if (!hcs.fCustomCtrl)
	{
		if (!CWnd::CreateEx(hcs.dwExStyle, WSTR_HEXCTRL_CLASSNAME, L"HexControl",
			dwStyle, rc, CWnd::FromHandle(hcs.hwndParent), m_fFloat ? 0 : hcs.uId))
		{
			CStringW ss;
			ss.Format(L"HexCtrl (Id:%u) CreateEx failed.\r\nCheck HEXCREATESTRUCT parameters.", hcs.uId);
			MessageBoxW(ss, L"Error", MB_ICONERROR);
			return false;
		}
	}
	else
	{
		if (!SubclassDlgItem(hcs.uId, CWnd::FromHandle(hcs.hwndParent)))
		{
			CStringW ss;
			ss.Format(L"HexCtrl (Id:%u) SubclassDlgItem failed.\r\nCheck CreateDialogCtrl parameters.", hcs.uId);
			MessageBoxW(ss, L"Error", MB_ICONERROR);
			return false;
		}
	}

	//Removing window's border frame.
	MARGINS marg { 0, 0, 0, 1 };
	DwmExtendFrameIntoClientArea(m_hWnd, &marg);
	SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

	m_pScrollV->Create(this, SB_VERT, 0, 0, 0); //Actual sizes are set in RecalcAll().
	m_pScrollH->Create(this, SB_HORZ, 0, 0, 0);
	m_pScrollV->AddSibling(m_pScrollH.get());
	m_pScrollH->AddSibling(m_pScrollV.get());

	m_fCreated = true;

	RecalcAll();

	return true;
}

bool CHexCtrl::CreateDialogCtrl(UINT uCtrlID, HWND hwndDlg)
{
	HEXCREATESTRUCT hcs;
	hcs.hwndParent = hwndDlg;
	hcs.uId = uCtrlID;
	hcs.fCustomCtrl = true;

	return Create(hcs);
}

void CHexCtrl::SetData(const HEXDATASTRUCT & hds)
{
	if (!m_fCreated)
		return;

	//m_hwndMsg was previously set in IHexCtrl::Create.
	if (hds.hwndMsg)
		m_hwndMsg = hds.hwndMsg;

	//Virtual mode is possible only when there is a MSG window, a data requests will be sent to.
	if (hds.enMode == EHexDataMode::DATA_MSG && !GetMsgWindow())
	{
		MessageBoxW(L"HexCtrl EHexDataMode::DATA_MSG mode requires HEXDATASTRUCT::hwndMsg to be not null.", L"Error", MB_ICONWARNING);
		return;
	}
	else if (hds.enMode == EHexDataMode::DATA_VIRTUAL && !hds.pHexVirtual)
	{
		MessageBoxW(L"HexCtrl EHexDataMode::DATA_VIRTUAL mode requires HEXDATASTRUCT::pHexVirtual to be not nullptr.", L"Error", MB_ICONWARNING);
		return;
	}

	ClearData();

	m_fDataSet = true;
	m_pData = hds.pData;
	m_ullDataSize = hds.ullDataSize;
	m_enMode = hds.enMode;
	m_fMutable = hds.fMutable;
	m_dwOffsetDigits = hds.ullDataSize <= 0xffffffffUL ? 8 :
		(hds.ullDataSize <= 0xffffffffffUL ? 10 :
		(hds.ullDataSize <= 0xffffffffffffUL ? 12 :
			(hds.ullDataSize <= 0xffffffffffffffUL ? 14 : 16)));

	RecalcAll();

	if (hds.ullSelectionSize)
		ShowOffset(hds.ullSelectionStart, hds.ullSelectionSize);
}

void CHexCtrl::ClearData()
{
	if (!m_fDataSet)
		return;

	m_fDataSet = false;
	m_ullDataSize = 0;
	m_pData = nullptr;
	m_fMutable = false;
	m_ullSelectionClick = m_ullSelectionStart = m_ullSelectionEnd = m_ullSelectionSize = 0;

	m_deqUndo.clear();
	m_deqRedo.clear();
	m_pScrollV->SetScrollPos(0);
	m_pScrollH->SetScrollPos(0);
	m_pScrollV->SetScrollSizes(0, 0, 0);
	UpdateInfoText();
}

void CHexCtrl::SetEditMode(bool fEnable)
{
	if (!m_fCreated)
		return;

	m_fMutable = fEnable;
	RedrawWindow();
}

void CHexCtrl::ShowOffset(ULONGLONG ullOffset, ULONGLONG ullSize)
{
	SetSelection(ullOffset, ullOffset, ullSize, true);
}

void CHexCtrl::SetFont(const LOGFONTW * pLogFontNew)
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

	LOGFONTW lf;
	m_fontHexView.GetLogFont(&lf);
	m_lFontSize = lf.lfHeight = uiSize;
	m_fontHexView.DeleteObject();
	m_fontHexView.CreateFontIndirectW(&lf);

	RecalcAll();
}

void CHexCtrl::SetColor(const HEXCOLORSTRUCT & clr)
{
	m_stColor = clr;
	RedrawWindow();
}

void CHexCtrl::SetCapacity(DWORD dwCapacity)
{
	if (dwCapacity < 1 || dwCapacity > m_dwCapacityMax)
		return;

	//Setting capacity according to current m_enShowMode.
	if (dwCapacity < m_dwCapacity)
		dwCapacity -= dwCapacity % (DWORD)m_enShowMode;
	else if (dwCapacity % (DWORD)m_enShowMode)
		dwCapacity += (DWORD)m_enShowMode - (dwCapacity % (DWORD)m_enShowMode);

	//To prevent under/over flow.
	if (dwCapacity < (DWORD)m_enShowMode)
		dwCapacity = (DWORD)m_enShowMode;
	else if (dwCapacity > m_dwCapacityMax)
		dwCapacity = m_dwCapacityMax;

	m_dwCapacity = dwCapacity;
	m_dwCapacityBlockSize = m_dwCapacity / 2;

	FillWstrCapacity();
	RecalcAll();
}

bool CHexCtrl::IsCreated()const
{
	return m_fCreated;
}

bool CHexCtrl::IsDataSet()const
{
	return m_fDataSet;
}

bool CHexCtrl::IsMutable()const
{
	return m_fMutable;
}

long CHexCtrl::GetFontSize()const
{
	if (!IsCreated())
		return 0;

	return m_lFontSize;
}

void CHexCtrl::GetSelection(ULONGLONG& ullOffset, ULONGLONG& ullSize)const
{
	ullOffset = m_ullSelectionStart;
	ullSize = m_ullSelectionSize;
}

HWND CHexCtrl::GetWindowHandle() const
{
	return m_hWnd;
}

HMENU CHexCtrl::GetMenuHandle()const
{
	return m_menuMain.m_hMenu;
}

void CHexCtrl::Destroy()
{
	delete this;
}

bool CHexCtrl::RegisterWndClass()
{
	//MFC initialization check, if Control is used outside MFC apps.
	if (!AfxGetAppModuleState()->m_pCurrentWinApp)
		AfxWinInit(::GetModuleHandleW(NULL), NULL, ::GetCommandLineW(), 0);

	HINSTANCE hInst = AfxGetInstanceHandle();
	WNDCLASSEXW wc { };

	if (!(::GetClassInfoExW(hInst, WSTR_HEXCTRL_CLASSNAME, &wc)))
	{
		wc.cbSize = sizeof(WNDCLASSEXW);
		wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = ::DefWindowProcW;
		wc.cbClsExtra = wc.cbWndExtra = 0;
		wc.hInstance = hInst;
		wc.hIcon = wc.hIconSm = nullptr;
		wc.hCursor = (HCURSOR)LoadImageW(0, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
		wc.hbrBackground = nullptr;
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = WSTR_HEXCTRL_CLASSNAME;
		if (!RegisterClassExW(&wc))
		{
			MessageBoxW(L"HexControl RegisterClassExW error.", L"Error");
			return false;
		}
	}

	return true;
}

void CHexCtrl::OnActivate(UINT nState, CWnd * pWndOther, BOOL bMinimized)
{
	SetFocus();

	CWnd::OnActivate(nState, pWndOther, bMinimized);
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
		if (m_pScrollH->IsVisible())
		{
			if (point.x < rcClient.left) {
				m_pScrollH->ScrollLineLeft();
				point.x = m_iIndentFirstHexChunk;
			}
			else if (point.x >= rcClient.right) {
				m_pScrollH->ScrollLineRight();
				point.x = m_iFourthVertLine - 1;
			}
		}
		if (m_pScrollV->IsVisible())
		{
			if (point.y < m_iStartWorkAreaY) {
				m_pScrollV->ScrollLineUp();
				point.y = m_iStartWorkAreaY;
			}
			else if (point.y >= m_iEndWorkArea) {
				m_pScrollV->ScrollLineDown();
				point.y = m_iEndWorkArea - 1;
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
		m_pScrollV->OnMouseMove(nFlags, point);
		m_pScrollH->OnMouseMove(nFlags, point);
	}
}

BOOL CHexCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (nFlags == MK_CONTROL)
	{
		SetFontSize(GetFontSize() + zDelta / WHEEL_DELTA * 2);
		return TRUE;
	}
	else if (nFlags == (MK_CONTROL | MK_SHIFT))
	{
		SetCapacity(m_dwCapacity + zDelta / WHEEL_DELTA);
		return TRUE;
	}

	if (zDelta > 0) //Scrolling Up.
		m_pScrollV->ScrollPageUp();
	else
		m_pScrollV->ScrollPageDown();

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
	MsgWindowNotify(HEXCTRL_MSG_SETSELECTION);
}

void CHexCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_fLMousePressed = false;
	ReleaseCapture();

	m_pScrollV->OnLButtonUp(nFlags, point);
	m_pScrollH->OnLButtonUp(nFlags, point);

	CWnd::OnLButtonUp(nFlags, point);
}

void CHexCtrl::OnMButtonDown(UINT nFlags, CPoint point)
{
}

BOOL CHexCtrl::OnCommand(WPARAM wParam, LPARAM lParam)
{
	static const wchar_t* const pwszErrVirtual { L"This function isn't supported in Virtual mode!" };

	UINT_PTR uId = LOWORD(wParam);

	switch (uId)
	{
	case (UINT_PTR)EMenu::IDM_MAIN_SEARCH:
		if (m_enMode == EHexDataMode::DATA_MEMORY)
			m_pDlgSearch->ShowWindow(SW_SHOW);
		else
			MessageBoxW(pwszErrVirtual, L"Error", MB_ICONEXCLAMATION);
		break;
	case (UINT_PTR)EMenu::IDM_EDIT_UNDO:
		Undo();
		break;
	case (UINT_PTR)EMenu::IDM_EDIT_REDO:
		Redo();
		break;
	case (UINT_PTR)EMenu::IDM_EDIT_COPY_HEX:
		ClipboardCopy(EClipboard::COPY_HEX);
		break;
	case (UINT_PTR)EMenu::IDM_EDIT_COPY_HEXFORMATTED:
		ClipboardCopy(EClipboard::COPY_HEXFORMATTED);
		break;
	case (UINT_PTR)EMenu::IDM_EDIT_COPY_ASCII:
		ClipboardCopy(EClipboard::COPY_ASCII);
		break;
	case (UINT_PTR)EMenu::IDM_EDIT_PASTE_HEX:
		ClipboardPaste(EClipboard::PASTE_HEX);
		break;
	case (UINT_PTR)EMenu::IDM_EDIT_PASTE_ASCII:
		ClipboardPaste(EClipboard::PASTE_ASCII);
		break;
	case (UINT_PTR)EMenu::IDM_EDIT_FILL_ZEROS:
		FillWithZeros();
		break;
	case (UINT_PTR)EMenu::IDM_MAIN_ABOUT:
	{
		CHexDlgAbout m_dlgAbout;
		m_dlgAbout.DoModal();
	}
	break;
	case (UINT_PTR)EMenu::IDM_SHOWAS_BYTE:
		SetShowMode(EShowMode::ASBYTE);
		break;
	case (UINT_PTR)EMenu::IDM_SHOWAS_WORD:
		SetShowMode(EShowMode::ASWORD);
		break;
	case (UINT_PTR)EMenu::IDM_SHOWAS_DWORD:
		SetShowMode(EShowMode::ASDWORD);
		break;
	case (UINT_PTR)EMenu::IDM_SHOWAS_QWORD:
		SetShowMode(EShowMode::ASQWORD);
		break;
	default:
		//For user defined custom menu, we notify parent window.
		HEXNOTIFYSTRUCT hns { { m_hWnd, (UINT)GetDlgCtrlID(), HEXCTRL_MSG_MENUCLICK } };
		hns.uMenuId = uId;
		MsgWindowNotify(hns);
	}

	return CWnd::OnCommand(wParam, lParam);
}

void CHexCtrl::OnContextMenu(CWnd * pWnd, CPoint point)
{
	UINT uMenuStatus;
	if (m_ullDataSize == 0 || m_ullSelectionSize == 0)
		uMenuStatus = MF_GRAYED;
	else
		uMenuStatus = MF_ENABLED;

	//To not spread up the efforts through code, all the checks are done here 
	//instead of WM_INITMENUPOPUP.
	m_menuMain.EnableMenuItem((UINT_PTR)EMenu::IDM_EDIT_COPY_HEX, uMenuStatus | MF_BYCOMMAND);
	m_menuMain.EnableMenuItem((UINT_PTR)EMenu::IDM_EDIT_COPY_HEXFORMATTED, uMenuStatus | MF_BYCOMMAND);
	m_menuMain.EnableMenuItem((UINT_PTR)EMenu::IDM_EDIT_COPY_ASCII, uMenuStatus | MF_BYCOMMAND);
	m_menuMain.EnableMenuItem((UINT_PTR)EMenu::IDM_EDIT_UNDO, (m_deqUndo.empty() ? MF_GRAYED : MF_ENABLED) | MF_BYCOMMAND);
	m_menuMain.EnableMenuItem((UINT_PTR)EMenu::IDM_EDIT_REDO, (m_deqRedo.empty() ? MF_GRAYED : MF_ENABLED) | MF_BYCOMMAND);

	BOOL fFormatAvail = IsClipboardFormatAvailable(CF_TEXT);
	m_menuMain.EnableMenuItem((UINT_PTR)EMenu::IDM_EDIT_PASTE_HEX, ((m_fMutable && fFormatAvail) ?
		uMenuStatus : MF_GRAYED) | MF_BYCOMMAND);
	m_menuMain.EnableMenuItem((UINT_PTR)EMenu::IDM_EDIT_PASTE_ASCII, ((m_fMutable && fFormatAvail) ?
		uMenuStatus : MF_GRAYED) | MF_BYCOMMAND);
	m_menuMain.EnableMenuItem((UINT_PTR)EMenu::IDM_EDIT_FILL_ZEROS,
		(m_fMutable ? uMenuStatus : MF_GRAYED) | MF_BYCOMMAND);

	//Notifying parent that we are about to display context menu.
	MsgWindowNotify(HEXCTRL_MSG_ONCONTEXTMENU);

	m_menuMain.TrackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON, point.x, point.y, this);
}

void CHexCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (GetKeyState(VK_CONTROL) < 0) //With Ctrl pressed.
		OnKeyDownCtrl(nChar);
	else if (GetAsyncKeyState(VK_SHIFT) < 0) //With Shift pressed.
		OnKeyDownShift(nChar);
	else
	{
		switch (nChar)
		{
		case VK_RIGHT:
			CursorMoveRight();
			break;
		case VK_LEFT:
			CursorMoveLeft();
			break;
		case VK_DOWN:
			CursorMoveDown();
			break;
		case VK_UP:
			CursorMoveUp();
			break;
		case VK_PRIOR: //Page-Up
			m_pScrollV->ScrollPageUp();
			break;
		case VK_NEXT:  //Page-Down
			m_pScrollV->ScrollPageDown();
			break;
		case VK_HOME:
			m_pScrollV->ScrollHome();
			break;
		case VK_END:
			m_pScrollV->ScrollEnd();
			break;
		}
	}
}

void CHexCtrl::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (!m_fMutable || (GetKeyState(VK_CONTROL) < 0))
		return;

	HEXMODIFYSTRUCT hmd;
	hmd.ullIndex = m_ullCursorPos;
	hmd.ullSize = hmd.ullDataSize = 1;

	BYTE chByte;
	if (IsCurTextArea()) //If cursor is in text area.
	{
		hmd.fWhole = true;
		chByte = nChar;
	}
	else
	{
		if (nChar >= 0x30 && nChar <= 0x39)      //Digits.
			chByte = nChar - 0x30;
		else if (nChar >= 0x41 && nChar <= 0x46) //Hex letters uppercase.
			chByte = nChar - 0x37;
		else if (nChar >= 0x61 && nChar <= 0x66) //Hex letters lowercase.
			chByte = nChar - 0x57;
		else
			return;

		hmd.fWhole = false;
		hmd.fHighPart = m_fCursorHigh;
	}

	hmd.pData = &chByte;
	ModifyData(hmd);
	CursorMoveRight();
}

UINT CHexCtrl::OnGetDlgCode()
{
	return DLGC_WANTALLKEYS;
}

void CHexCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar * pScrollBar)
{
	if (m_pScrollV->GetScrollPosDelta() != 0)
		RedrawWindow();
}

void CHexCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar * pScrollBar)
{
	if (m_pScrollH->GetScrollPosDelta() != 0)
		RedrawWindow();
}

void CHexCtrl::OnPaint()
{
	CPaintDC dc(this);
	if (!m_fCreated)
	{
		CRect rc;
		dc.GetClipBox(rc);
		dc.FillSolidRect(rc, RGB(250, 250, 250));
		dc.TextOutW(1, 1, L"Call IHexCtrl::Create first.");
		return;
	}

	const int iScrollH = (int)m_pScrollH->GetScrollPos();
	CRect rcClient;
	GetClientRect(rcClient);

	//To prevent drawing in too small window (can cause hangs).
	if (rcClient.Height() < m_iHeightTopRect + m_iHeightBottomOffArea)
		return;

	//Drawing through CMemDC to avoid flickering.
	CMemDC memDC(dc, rcClient);
	CDC* pDC = &memDC.GetDC();

	RECT rc; //Used for all local rect related drawings.
	pDC->GetClipBox(&rc);
	pDC->FillSolidRect(&rc, m_stColor.clrBk);
	pDC->SelectObject(m_penLines);
	pDC->SelectObject(m_fontHexView);

	//Find the ullLineStart and ullLineEnd position, draw the visible part.
	const auto ullLineStart = GetTopLine();
	ULONGLONG ullLineEnd { 0 };
	if (m_ullDataSize)
	{
		ullLineEnd = ullLineStart + (rcClient.Height() - m_iStartWorkAreaY - m_iHeightBottomOffArea) / m_sizeLetter.cy;
		//If m_dwDataCount is really small we adjust dwLineEnd to be not bigger than maximum allowed.
		if (ullLineEnd > (m_ullDataSize / m_dwCapacity))
			ullLineEnd = (m_ullDataSize % m_dwCapacity) ? m_ullDataSize / m_dwCapacity + 1 : m_ullDataSize / m_dwCapacity;
	}

	const auto iThirdHorizLine = rcClient.Height() - m_iHeightBottomOffArea;
	const auto iFourthHorizLine = iThirdHorizLine + m_iHeightBottomRect;

	//First horizontal line.
	pDC->MoveTo(m_iFirstVertLine - iScrollH, m_iFirstHorizLine);
	pDC->LineTo(m_iFourthVertLine, m_iFirstHorizLine);

	//Second horizontal line.
	pDC->MoveTo(m_iFirstVertLine - iScrollH, m_iSecondHorizLine);
	pDC->LineTo(m_iFourthVertLine, m_iSecondHorizLine);

	//Third horizontal line.
	pDC->MoveTo(m_iFirstVertLine - iScrollH, iThirdHorizLine);
	pDC->LineTo(m_iFourthVertLine, iThirdHorizLine);

	//Fourth horizontal line.
	pDC->MoveTo(m_iFirstVertLine - iScrollH, iFourthHorizLine);
	pDC->LineTo(m_iFourthVertLine, iFourthHorizLine);

	//«Offset» text.
	rc.left = m_iFirstVertLine - iScrollH;
	rc.top = m_iFirstHorizLine;
	rc.right = m_iSecondVertLine - iScrollH;
	rc.bottom = m_iSecondHorizLine;
	pDC->SetTextColor(m_stColor.clrTextCaption);
	pDC->DrawTextW(L"Offset", 6, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	//Info rect's text.
	rc.left = m_iFirstVertLine - iScrollH;
	rc.top = iThirdHorizLine + 1;
	rc.right = m_iFourthVertLine;
	rc.bottom = iFourthHorizLine;	//Fill bottom rect until iFourthHorizLine.
	pDC->FillSolidRect(&rc, m_stColor.clrBkInfoRect);
	rc.left = m_iFirstVertLine + 5; //Draw text beginning with little indent.
	rc.right = rcClient.right;		//Draw text to the end of the client area, even if it pass iFourthHorizLine.
	pDC->SetTextColor(m_stColor.clrTextInfoRect);
	pDC->SelectObject(m_fontBottomRect);
	pDC->DrawTextW(m_wstrBottomText.data(), (int)m_wstrBottomText.size(), &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

	//Capacity numbers.
	pDC->SelectObject(m_fontHexView);
	pDC->SetTextColor(m_stColor.clrTextCaption);
	pDC->SetBkColor(m_stColor.clrBk);
	ExtTextOutW(pDC->m_hDC, m_iIndentFirstHexChunk - iScrollH, m_iFirstHorizLine + m_iIndentTextCapacityY, NULL, nullptr,
		m_wstrCapacity.data(), (UINT)m_wstrCapacity.size(), nullptr);

	//"Ascii" text.
	rc.left = m_iThirdVertLine - iScrollH; rc.top = m_iFirstHorizLine;
	rc.right = m_iFourthVertLine - iScrollH; rc.bottom = m_iSecondHorizLine;
	pDC->DrawTextW(L"Ascii", 5, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	//First Vertical line.
	pDC->MoveTo(m_iFirstVertLine - iScrollH, m_iFirstHorizLine);
	pDC->LineTo(m_iFirstVertLine - iScrollH, iFourthHorizLine);

	//Second Vertical line.
	pDC->MoveTo(m_iSecondVertLine - iScrollH, m_iFirstHorizLine);
	pDC->LineTo(m_iSecondVertLine - iScrollH, iThirdHorizLine);

	//Third Vertical line.
	pDC->MoveTo(m_iThirdVertLine - iScrollH, m_iFirstHorizLine);
	pDC->LineTo(m_iThirdVertLine - iScrollH, iThirdHorizLine);

	//Fourth Vertical line.
	pDC->MoveTo(m_iFourthVertLine - iScrollH, m_iFirstHorizLine);
	pDC->LineTo(m_iFourthVertLine - iScrollH, iFourthHorizLine);

	int iLine = 0;        //Current line to print.
	std::vector<POLYTEXTW> vecPolyHex, vecPolyAscii, vecPolySelHex, vecPolySelAscii, vecPolyCursor;
	std::list<std::wstring> listWstrHex, listWstrAscii, listWstrSelHex, listWstrSelAscii, listWstrCursor;
	COLORREF clrBkCursor; //Cursor color.

	//Loop for printing Hex chunks and Ascii chars line by line.
	for (ULONGLONG iterLines = ullLineStart; iterLines < ullLineEnd; iterLines++, iLine++)
	{
		//Drawing offset with bk color depending on selection range.
		if (m_ullSelectionSize && (iterLines * m_dwCapacity + m_dwCapacity) > m_ullSelectionStart &&
			(iterLines * m_dwCapacity) < m_ullSelectionEnd)
		{
			pDC->SetTextColor(m_stColor.clrTextSelected);
			pDC->SetBkColor(m_stColor.clrBkSelected);
		}
		else
		{
			pDC->SetTextColor(m_stColor.clrTextCaption);
			pDC->SetBkColor(m_stColor.clrBk);
		}

		//Left column offset printing (00000001...0000FFFF).
		wchar_t pwszOffset[16];
		UllToWchars(iterLines * m_dwCapacity, pwszOffset, (size_t)m_dwOffsetDigits / 2);
		ExtTextOutW(pDC->m_hDC, m_iFirstVertLine + m_sizeLetter.cx - iScrollH, m_iStartWorkAreaY + (m_sizeLetter.cy * iLine),
			NULL, nullptr, pwszOffset, m_dwOffsetDigits, nullptr);

		//Hex, Ascii and Cursor strings to print.
		std::wstring wstrHexToPrint { }, wstrAsciiToPrint { }, wstrHexCursorToPrint { }, wstrAsciiCursorToPrint { };
		std::wstring wstrHexSelToPrint { }, wstrAsciiSelToPrint { }; //Selected Hex and Ascii strings to print.

		//Selection Hex and Ascii X coords. 0x7FFFFFFF is important.
		int iSelHexPosToPrintX = 0x7FFFFFFF, iSelAsciiPosToPrintX;
		int iCursorHexPosToPrintX, iCursorAsciiPosToPrintX; //Cursor X coords.

		//Main loop for printing Hex chunks and Ascii chars.
		for (unsigned iterChunks = 0; iterChunks < m_dwCapacity; iterChunks++)
		{
			//Index of the next Byte to draw.
			const ULONGLONG ullIndexByteToPrint = iterLines * m_dwCapacity + iterChunks;
			if (ullIndexByteToPrint >= m_ullDataSize) //Draw until reaching the end of m_ullDataSize.
				break;

			//Hex chunk to print.
			const auto chByteToPrint = GetByte(ullIndexByteToPrint);
			const wchar_t pwszHexToPrint[2] { g_pwszHexMap[(chByteToPrint >> 4) & 0x0F], g_pwszHexMap[chByteToPrint & 0x0F] };
			wstrHexToPrint += pwszHexToPrint[0];
			wstrHexToPrint += pwszHexToPrint[1];

			//Additional space between grouped Hex chunks.
			if (((iterChunks + 1) % (DWORD)m_enShowMode) == 0 && iterChunks < (m_dwCapacity - 1))
				wstrHexToPrint += L" ";

			//Additional space between capacity halves, only with ASBYTE representation.
			if (m_enShowMode == EShowMode::ASBYTE && iterChunks == (m_dwCapacityBlockSize - 1))
				wstrHexToPrint += L"  ";

			//Ascii to print.
			wchar_t wchAscii = chByteToPrint;
			if (wchAscii < 32 || wchAscii >= 0x7f) //For non printable Ascii just print a dot.
				wchAscii = '.';
			wstrAsciiToPrint += wchAscii;

			//If there is a selection.
			if (m_ullSelectionSize && (ullIndexByteToPrint >= m_ullSelectionStart) && (ullIndexByteToPrint < m_ullSelectionEnd))
			{
				if (iSelHexPosToPrintX == 0x7FFFFFFF)
				{
					int iCy;
					HexChunkPoint(ullIndexByteToPrint, iSelHexPosToPrintX, iCy);
					AsciiChunkPoint(ullIndexByteToPrint, iSelAsciiPosToPrintX, iCy);
				}

				if (!wstrHexSelToPrint.empty()) //Only adding spaces if there are chars beforehead.
				{
					if ((iterChunks % (DWORD)m_enShowMode) == 0)
						wstrHexSelToPrint += L" ";

					//Additional space between capacity halves, only with ASBYTE representation.
					if (m_enShowMode == EShowMode::ASBYTE && iterChunks == m_dwCapacityBlockSize)
						wstrHexSelToPrint += L"  ";
				}
				wstrHexSelToPrint += pwszHexToPrint[0];
				wstrHexSelToPrint += pwszHexToPrint[1];
				wstrAsciiSelToPrint += wchAscii;
			}

			//Cursor position. 
			if (m_fMutable && (ullIndexByteToPrint == m_ullCursorPos))
			{
				int iCy;
				HexChunkPoint(m_ullCursorPos, iCursorHexPosToPrintX, iCy);
				if (m_fCursorHigh)
					wstrHexCursorToPrint = g_pwszHexMap[(chByteToPrint & 0xF0) >> 4];
				else
				{
					wstrHexCursorToPrint = g_pwszHexMap[(chByteToPrint & 0x0F)];
					iCursorHexPosToPrintX += m_sizeLetter.cx;
				}
				AsciiChunkPoint(m_ullCursorPos, iCursorAsciiPosToPrintX, iCy);
				wstrAsciiCursorToPrint = wchAscii;

				if (m_ullSelectionSize && ullIndexByteToPrint >= m_ullSelectionStart && ullIndexByteToPrint < m_ullSelectionEnd)
					clrBkCursor = m_stColor.clrBkCursorSelected;
				else
					clrBkCursor = m_stColor.clrBkCursor;
			}
		}

		const auto iHexPosToPrintX = m_iIndentFirstHexChunk - iScrollH;
		const auto iAsciiPosToPrintX = m_iIndentAscii - iScrollH;
		const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeLetter.cy * iLine; //Hex and Ascii the same.

		//Hex Poly.
		listWstrHex.emplace_back(std::move(wstrHexToPrint));
		vecPolyHex.emplace_back(POLYTEXTW { iHexPosToPrintX, iPosToPrintY,
			(UINT)listWstrHex.back().size(), listWstrHex.back().data(), 0, { }, nullptr });

		//Ascii Poly.
		listWstrAscii.emplace_back(std::move(wstrAsciiToPrint));
		vecPolyAscii.emplace_back(POLYTEXTW { iAsciiPosToPrintX, iPosToPrintY,
			(UINT)listWstrAscii.back().size(), listWstrAscii.back().data(), 0, { }, nullptr });

		//Selection.
		if (!wstrHexSelToPrint.empty())
		{
			//Hex selected Poly.
			listWstrSelHex.emplace_back(std::move(wstrHexSelToPrint));
			vecPolySelHex.emplace_back(POLYTEXTW { iSelHexPosToPrintX, iPosToPrintY,
				(UINT)listWstrSelHex.back().size(), listWstrSelHex.back().data(), 0, { }, nullptr });

			//Ascii selected Poly.
			listWstrSelAscii.emplace_back(std::move(wstrAsciiSelToPrint));
			vecPolySelAscii.emplace_back(POLYTEXTW { iSelAsciiPosToPrintX, iPosToPrintY,
				(UINT)listWstrSelAscii.back().size(), listWstrSelAscii.back().data(), 0, { }, nullptr });
		}

		//Cursor Poly.
		if (!wstrHexCursorToPrint.empty())
		{
			listWstrCursor.emplace_back(std::move(wstrHexCursorToPrint));
			vecPolyCursor.emplace_back(POLYTEXTW { iCursorHexPosToPrintX, iPosToPrintY,
				(UINT)listWstrCursor.back().size(), listWstrCursor.back().data(), 0, { }, nullptr });
			listWstrCursor.emplace_back(std::move(wstrAsciiCursorToPrint));
			vecPolyCursor.emplace_back(POLYTEXTW { iCursorAsciiPosToPrintX, iPosToPrintY,
				(UINT)listWstrCursor.back().size(), listWstrCursor.back().data(), 0, { }, nullptr });
		}
	}

	//Hex printing.
	pDC->SetBkColor(m_stColor.clrBk);
	pDC->SetTextColor(m_stColor.clrTextHex);
	PolyTextOutW(pDC->m_hDC, vecPolyHex.data(), (UINT)vecPolyHex.size());

	//Ascii printing.
	pDC->SetTextColor(m_stColor.clrTextAscii);
	PolyTextOutW(pDC->m_hDC, vecPolyAscii.data(), (UINT)vecPolyAscii.size());

	//Hex selected printing.
	pDC->SetBkColor(m_stColor.clrBkSelected);
	pDC->SetTextColor(m_stColor.clrTextSelected);
	PolyTextOutW(pDC->m_hDC, vecPolySelHex.data(), (UINT)vecPolySelHex.size());

	//Ascii selected printing.
	PolyTextOutW(pDC->m_hDC, vecPolySelAscii.data(), (UINT)vecPolySelAscii.size());

	//Cursor printing.
	if (!vecPolyCursor.empty())
	{
		pDC->SetBkColor(clrBkCursor);
		pDC->SetTextColor(m_stColor.clrTextCursor);
		PolyTextOutW(pDC->m_hDC, vecPolyCursor.data(), (UINT)vecPolyCursor.size());
	}
}

void CHexCtrl::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	RecalcWorkAreaHeight(cy);
	m_pScrollV->SetScrollPageSize(m_iHeightWorkArea);
}

BOOL CHexCtrl::OnEraseBkgnd(CDC * pDC)
{
	return FALSE;
}

BOOL CHexCtrl::OnSetCursor(CWnd * pWnd, UINT nHitTest, UINT message)
{
	m_pScrollV->OnSetCursor(pWnd, nHitTest, message);
	m_pScrollH->OnSetCursor(pWnd, nHitTest, message);

	return CWnd::OnSetCursor(pWnd, nHitTest, message);
}

BOOL CHexCtrl::OnNcActivate(BOOL bActive)
{
	m_pScrollV->OnNcActivate(bActive);
	m_pScrollH->OnNcActivate(bActive);

	return CWnd::OnNcActivate(bActive);
}

void CHexCtrl::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS * lpncsp)
{
	CWnd::OnNcCalcSize(bCalcValidRects, lpncsp);

	//Sequence is important — H->V.
	m_pScrollH->OnNcCalcSize(bCalcValidRects, lpncsp);
	m_pScrollV->OnNcCalcSize(bCalcValidRects, lpncsp);
}

void CHexCtrl::OnNcPaint()
{
	Default();

	m_pScrollV->OnNcPaint();
	m_pScrollH->OnNcPaint();
}

void CHexCtrl::OnDestroy()
{
	//Send messages to both, m_hwndMsg and pwndParent.
	NMHDR nmh { m_hWnd, (UINT)GetDlgCtrlID(), HEXCTRL_MSG_DESTROY };
	HWND hwndMsg = GetMsgWindow();
	if (hwndMsg)
		::SendMessageW(hwndMsg, WM_NOTIFY, nmh.idFrom, (LPARAM)& nmh);

	CWnd* pwndParent = GetParent();
	if (pwndParent)
	{
		pwndParent->SendMessageW(WM_NOTIFY, nmh.idFrom, (LPARAM)& nmh);
		pwndParent->SetForegroundWindow();
	}

	ClearData();
	m_fCreated = false;

	CWnd::OnDestroy();
}

BYTE CHexCtrl::GetByte(ULONGLONG ullIndex)const
{
	//If it's virtual data control we aquire next byte_to_print from m_hwndMsg window.
	switch (m_enMode)
	{
	case EHexDataMode::DATA_MEMORY:
	{
		return m_pData[ullIndex];
	}
	break;
	case EHexDataMode::DATA_MSG:
	{
		HEXNOTIFYSTRUCT hns { { m_hWnd, (UINT)GetDlgCtrlID(), HEXCTRL_MSG_GETDATA } };
		hns.ullIndex = ullIndex;
		hns.ullSize = 1;
		MsgWindowNotify(hns);
		return *hns.pData;
	}
	break;
	case EHexDataMode::DATA_VIRTUAL:
	{
		if (m_pHexVirtual)
			return m_pHexVirtual->GetByte(ullIndex);
	}
	break;
	}
	return 0;
}

void CHexCtrl::ModifyData(const HEXMODIFYSTRUCT & hms, bool fRedraw)
{	//Changes byte(s) in memory. High or Low part, depending on hms.fHighPart.
	if (!m_fMutable || hms.ullIndex >= m_ullDataSize || (hms.ullIndex + hms.ullSize) > m_ullDataSize
		|| (hms.ullIndex + hms.ullDataSize) > m_ullDataSize)
		return;

	switch (m_enMode)
	{
	case EHexDataMode::DATA_MEMORY: //Modify only in non Virtual mode.
	{
		if (hms.ullSize == hms.ullDataSize)
		{
			m_deqRedo.clear(); //No Redo unless we make Undo.
			SnapshotUndo(hms.ullIndex, hms.ullSize);

			if (hms.fWhole)
				for (ULONGLONG i = 0; i < hms.ullSize; i++)
					m_pData[hms.ullIndex + i] = hms.pData[i];
			else
			{	//If just one part (High/Low) of byte must be changed.
				unsigned char chByte = GetByte(hms.ullIndex);
				if (hms.fHighPart)
					chByte = (*hms.pData << 4) | (chByte & 0x0F);
				else
					chByte = (*hms.pData & 0x0F) | (chByte & 0xF0);

				m_pData[hms.ullIndex] = chByte;
			}
		}
		else //Fill ullSize bytes with ullDataSize bytes of hms.pData (fill with zeroes for instance).
		{
			m_deqRedo.clear(); //No Redo unless we make Undo.
			SnapshotUndo(hms.ullIndex, hms.ullSize);

			ULONGLONG ullChunks = hms.ullSize > hms.ullDataSize ? (hms.fRepeat ? hms.ullSize / hms.ullDataSize : 1) : 1;
			for (ULONGLONG iterChunk = 0; iterChunk < ullChunks; iterChunk++)
				for (ULONGLONG iterData = 0; iterData < hms.ullDataSize; iterData++)
					m_pData[hms.ullIndex + hms.ullDataSize * iterChunk + iterData] = hms.pData[iterData];
		}
	}
	break;
	case EHexDataMode::DATA_MSG:
	{
		//In EHexDataMode::DATA_MSG mode we send pointer to hms.
		HEXNOTIFYSTRUCT hns { { m_hWnd, (UINT)GetDlgCtrlID(), HEXCTRL_MSG_MODIFYDATA } };
		hns.pData = (PBYTE)& hms;
		MsgWindowNotify(hns);
	}
	break;
	case EHexDataMode::DATA_VIRTUAL:
	{
		if (m_pHexVirtual)
			m_pHexVirtual->ModifyData(hms);
	}
	break;
	}

	if (fRedraw)
		RedrawWindow();
}

HWND CHexCtrl::GetMsgWindow()const
{
	return m_hwndMsg;
}

void CHexCtrl::RecalcAll()
{
	ULONGLONG ullCurLineV = GetTopLine();

	//Current font size related.
	HDC hDC = ::GetDC(m_hWnd);
	SelectObject(hDC, m_fontHexView);
	TEXTMETRICW tm { };
	GetTextMetricsW(hDC, &tm);
	m_sizeLetter.cx = tm.tmAveCharWidth;
	m_sizeLetter.cy = tm.tmHeight + tm.tmExternalLeading;
	::ReleaseDC(m_hWnd, hDC);

	m_iSecondVertLine = m_iFirstVertLine + m_dwOffsetDigits * m_sizeLetter.cx + m_sizeLetter.cx * 2;
	m_iSizeHexByte = m_sizeLetter.cx * 2;
	m_iSpaceBetweenBlocks = m_enShowMode == EShowMode::ASBYTE ? m_sizeLetter.cx * 2 : 0;
	m_iSpaceBetweenHexChunks = m_sizeLetter.cx;
	m_iDistanceBetweenHexChunks = m_iSizeHexByte * (DWORD)m_enShowMode + m_iSpaceBetweenHexChunks;
	m_iThirdVertLine = m_iSecondVertLine + m_iDistanceBetweenHexChunks * (m_dwCapacity / (DWORD)m_enShowMode)
		+ m_sizeLetter.cx + m_iSpaceBetweenBlocks;
	m_iIndentAscii = m_iThirdVertLine + m_sizeLetter.cx;
	m_iSpaceBetweenAscii = m_sizeLetter.cx;
	m_iFourthVertLine = m_iIndentAscii + (m_iSpaceBetweenAscii * m_dwCapacity) + m_sizeLetter.cx;
	m_iIndentFirstHexChunk = m_iSecondVertLine + m_sizeLetter.cx;
	m_iSizeFirstHalf = m_iIndentFirstHexChunk + m_dwCapacityBlockSize * (m_sizeLetter.cx * 2) +
		(m_dwCapacityBlockSize / (DWORD)m_enShowMode - 1) * m_iSpaceBetweenHexChunks;
	m_iHeightTopRect = std::lround(m_sizeLetter.cy * 1.5);
	m_iStartWorkAreaY = m_iFirstHorizLine + m_iHeightTopRect;
	m_iSecondHorizLine = m_iStartWorkAreaY - 1;
	m_iIndentTextCapacityY = m_iHeightTopRect / 2 - (m_sizeLetter.cy / 2);

	RecalcScrollSizes();
	m_pScrollV->SetScrollPos(ullCurLineV * m_sizeLetter.cy);

	RedrawWindow();
}

void CHexCtrl::RecalcWorkAreaHeight(int iClientHeight)
{
	m_iEndWorkArea = iClientHeight - m_iHeightBottomOffArea -
		((iClientHeight - m_iStartWorkAreaY - m_iHeightBottomOffArea) % m_sizeLetter.cy);
	m_iHeightWorkArea = m_iEndWorkArea - m_iStartWorkAreaY;
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
	m_pScrollV->SetScrollSizes(m_sizeLetter.cy, m_iHeightWorkArea,
		m_iStartWorkAreaY + m_iHeightBottomOffArea + m_sizeLetter.cy * (m_ullDataSize / m_dwCapacity + 2));
	m_pScrollH->SetScrollSizes(m_sizeLetter.cx, iClientWidth, m_iFourthVertLine + 1);
}

ULONGLONG CHexCtrl::GetTopLine()const
{
	return m_pScrollV->GetScrollPos() / m_sizeLetter.cy;
}

ULONGLONG CHexCtrl::HitTest(const POINT * pPoint)
{
	int iY = pPoint->y;
	int iX = pPoint->x + (int)m_pScrollH->GetScrollPos(); //To compensate horizontal scroll.
	ULONGLONG ullCurLine = GetTopLine();
	ULONGLONG ullHexChunk;
	m_fCursorTextArea = false;

	//Checking if cursor is within hex chunks area.
	if ((iX >= m_iIndentFirstHexChunk) && (iX < m_iThirdVertLine) && (iY >= m_iStartWorkAreaY) && (iY <= m_iEndWorkArea))
	{
		//Additional space between halves. Only in BYTE's view mode.
		int iBetweenBlocks;
		if (m_enShowMode == EShowMode::ASBYTE && iX > m_iSizeFirstHalf)
			iBetweenBlocks = m_iSpaceBetweenBlocks;
		else
			iBetweenBlocks = 0;

		//Calculate hex chunk.
		DWORD dwX = iX - m_iIndentFirstHexChunk - iBetweenBlocks;
		DWORD dwChunkX { };
		int iSpaceBetweenHexChunks = 0;
		for (unsigned i = 1; i <= m_dwCapacity; i++)
		{
			if ((i % (DWORD)m_enShowMode) == 0)
				iSpaceBetweenHexChunks += m_iSpaceBetweenHexChunks;
			if ((m_iSizeHexByte * i + iSpaceBetweenHexChunks) > dwX)
			{
				dwChunkX = i - 1;
				break;
			}
		}
		ullHexChunk = dwChunkX + ((iY - m_iStartWorkAreaY) / m_sizeLetter.cy) * m_dwCapacity + (ullCurLine * m_dwCapacity);
	}
	else if ((iX >= m_iIndentAscii) && (iX < (m_iIndentAscii + m_iSpaceBetweenAscii * (int)m_dwCapacity))
		&& (iY >= m_iStartWorkAreaY) && iY <= m_iEndWorkArea)
	{
		//Calculate ullHit Ascii symbol.
		ullHexChunk = ((iX - m_iIndentAscii) / m_iSpaceBetweenAscii) +
			((iY - m_iStartWorkAreaY) / m_sizeLetter.cy) * m_dwCapacity + (ullCurLine * m_dwCapacity);
		m_fCursorTextArea = true;
	}
	else
		ullHexChunk = -1;

	//If cursor is out of end-bound of hex chunks or Ascii chars.
	if (ullHexChunk >= m_ullDataSize)
		ullHexChunk = -1;

	return ullHexChunk;
}

void CHexCtrl::HexChunkPoint(ULONGLONG ullChunk, int& iCx, int& iCy)const
{	//This func computes x and y pos of given Hex chunk.
	int iBetweenBlocks;
	if (m_enShowMode == EShowMode::ASBYTE && (ullChunk % m_dwCapacity) >= m_dwCapacityBlockSize)
		iBetweenBlocks = m_iSpaceBetweenBlocks;
	else
		iBetweenBlocks = 0;

	iCx = int(((m_iIndentFirstHexChunk + iBetweenBlocks + (ullChunk % m_dwCapacity) * m_iSizeHexByte) +
		((ullChunk % m_dwCapacity) / (DWORD)m_enShowMode) * m_iSpaceBetweenHexChunks) - m_pScrollH->GetScrollPos());

	iCy = int((m_iStartWorkAreaY + (ullChunk / m_dwCapacity) * m_sizeLetter.cy) - m_pScrollV->GetScrollPos());
	if (ullChunk % m_dwCapacity)
		iCy += m_sizeLetter.cy;
}

void CHexCtrl::AsciiChunkPoint(ULONGLONG ullChunk, int & iCx, int& iCy) const
{	//This func computes x and y pos of given Ascii chunk.
	iCx = int((m_iIndentAscii + (ullChunk % m_dwCapacity) * m_sizeLetter.cx) - m_pScrollH->GetScrollPos());

	iCy = int(((ullChunk / m_dwCapacity) * m_sizeLetter.cy) - m_pScrollV->GetScrollPos());
	if (ullChunk % m_dwCapacity)
		iCy += m_sizeLetter.cy;
}

void CHexCtrl::ClipboardCopy(EClipboard enType)
{
	if (!m_ullSelectionSize || m_ullSelectionSize > 0x6400000) //>~100MB
	{
		MessageBoxW(L"Selection size is too big to copy.\r\nTry to select less.", L"Error", MB_ICONERROR);
		return;
	}
	const char* const pszHexMap { "0123456789ABCDEF" };
	std::string strToClipboard;

	switch (enType)
	{
	case EClipboard::COPY_HEX:
	{
		strToClipboard.reserve((size_t)m_ullSelectionSize * 2);
		for (unsigned i = 0; i < m_ullSelectionSize; i++)
		{
			BYTE chByte = GetByte(m_ullSelectionStart + i);
			strToClipboard += pszHexMap[(chByte & 0xF0) >> 4];
			strToClipboard += pszHexMap[(chByte & 0x0F)];
		}
	}
	break;
	case EClipboard::COPY_HEXFORMATTED:
	{
		strToClipboard.reserve((size_t)m_ullSelectionSize * 3);

		//How many spaces are needed to be inserted at the beginnig.
		DWORD dwModStart = m_ullSelectionStart % m_dwCapacity;

		//When to insert first "\r\n".
		DWORD dwTail = m_dwCapacity - dwModStart;
		DWORD dwNextBlock = (m_dwCapacity % 2) ? m_dwCapacityBlockSize + 2 : m_dwCapacityBlockSize + 1;

		//If at least two rows are selected.
		if (dwModStart + m_ullSelectionSize > m_dwCapacity)
		{
			DWORD dwCount = dwModStart * 2 + dwModStart / (DWORD)m_enShowMode;
			//Additional spaces between halves. Only in ASBYTE's view mode.
			dwCount += m_enShowMode == EShowMode::ASBYTE ? (dwTail <= m_dwCapacityBlockSize ? 2 : 0) : 0;
			strToClipboard.insert(0, (size_t)dwCount, ' ');
		}

		for (unsigned i = 0; i < m_ullSelectionSize; i++)
		{
			BYTE chByte = GetByte(m_ullSelectionStart + i);
			strToClipboard += pszHexMap[(chByte & 0xF0) >> 4];
			strToClipboard += pszHexMap[(chByte & 0x0F)];

			if (i < (m_ullSelectionSize - 1) && (dwTail - 1) != 0)
				if (m_enShowMode == EShowMode::ASBYTE && dwTail == dwNextBlock)
					strToClipboard += "   "; //Additional spaces between halves. Only in BYTE's view mode.
				else if (((m_dwCapacity - dwTail + 1) % (DWORD)m_enShowMode) == 0) //Add space after hex full chunk, ShowAs_size depending.
					strToClipboard += " ";
			if (--dwTail == 0 && i < (m_ullSelectionSize - 1)) //Next row.
			{
				strToClipboard += "\r\n";
				dwTail = m_dwCapacity;
			}
		}
	}
	break;
	case EClipboard::COPY_ASCII:
	{
		strToClipboard.reserve((size_t)m_ullSelectionSize);
		for (unsigned i = 0; i < m_ullSelectionSize; i++)
		{
			char ch = GetByte(m_ullSelectionStart + i);
			//If next byte is zero —> substitute it with space.
			if (ch == 0)
				ch = ' ';
			strToClipboard += ch;
		}
	}
	break;
	default: //default.
		break;
	}

	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, strToClipboard.size() + 1);
	if (!hMem) {
		MessageBoxW(L"GlobalAlloc error.", L"Error", MB_ICONERROR);
		return;
	}
	LPVOID hMemLock = GlobalLock(hMem);
	if (!hMemLock) {
		MessageBoxW(L"GlobalLock error.", L"Error", MB_ICONERROR);
		return;
	}
	memcpy(hMemLock, strToClipboard.data(), strToClipboard.size() + 1);
	GlobalUnlock(hMem);
	OpenClipboard();
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hMem);
	CloseClipboard();
}

void CHexCtrl::ClipboardPaste(EClipboard enType)
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

	HEXMODIFYSTRUCT hmd;
	hmd.ullIndex = m_ullCursorPos;
	hmd.fWhole = true;

	std::string strData;
	switch (enType)
	{
	case EClipboard::PASTE_ASCII:
		hmd.pData = (PBYTE)pClipboardData;
		hmd.ullSize = hmd.ullDataSize = ullSize;
		break;
	case EClipboard::PASTE_HEX:
	{
		size_t dwIterations = size_t(ullSize / 2 + ullSize % 2);
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
			if (!CharsToUl(chToUL, ulNumber))
				return;

			strData += (unsigned char)ulNumber;
		}
		hmd.pData = (PBYTE)strData.data();
		hmd.ullSize = hmd.ullDataSize = strData.size();
	}
	break;
	}

	ModifyData(hmd);

	GlobalUnlock(hClipboard);
	CloseClipboard();

	RedrawWindow();
}

void CHexCtrl::OnKeyDownShift(UINT nChar)
{
	ULONGLONG ullStart, ullSize;
	if (m_fMutable)
	{
		switch (nChar)
		{
		case VK_RIGHT:
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
			break;
		case VK_LEFT:
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
			break;
		case VK_DOWN:
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
			break;
		case VK_UP:
			if (m_ullCursorPos == m_ullSelectionClick || m_ullCursorPos == m_ullSelectionStart
				|| m_ullCursorPos == m_ullSelectionEnd)
			{
				if (m_ullSelectionStart == 0)
					return;

				if (m_ullSelectionStart < m_ullSelectionClick)
				{
					if (m_ullSelectionStart < m_dwCapacity)
					{
						ullStart = 0;
						ullSize = m_ullSelectionSize + m_ullSelectionStart;
					}
					else
					{
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
			break;
		}
	}
	else if (m_ullSelectionSize)
	{
		switch (nChar)
		{
		case VK_RIGHT:
			if (m_ullSelectionStart == m_ullSelectionClick)
				SetSelection(m_ullSelectionClick, m_ullSelectionClick, m_ullSelectionSize + 1);
			else
				SetSelection(m_ullSelectionClick, m_ullSelectionStart + 1, m_ullSelectionSize - 1);
			break;
		case VK_LEFT:
			if (m_ullSelectionStart == m_ullSelectionClick && m_ullSelectionSize > 1)
				SetSelection(m_ullSelectionClick, m_ullSelectionClick, m_ullSelectionSize - 1);
			else
				SetSelection(m_ullSelectionClick, m_ullSelectionStart - 1, m_ullSelectionSize + 1);
			break;
		case VK_DOWN:
			if (m_ullSelectionStart == m_ullSelectionClick)
				SetSelection(m_ullSelectionClick, m_ullSelectionClick, m_ullSelectionSize + m_dwCapacity);
			else if (m_ullSelectionStart < m_ullSelectionClick)
			{
				ullStart = m_ullSelectionSize > m_dwCapacity ? m_ullSelectionStart + m_dwCapacity : m_ullSelectionClick;
				ullSize = m_ullSelectionSize >= m_dwCapacity ? m_ullSelectionSize - m_dwCapacity : m_dwCapacity;
				SetSelection(m_ullSelectionClick, ullStart, ullSize ? ullSize : 1);
			}
			break;
		case VK_UP:
			if (m_ullSelectionStart == 0)
				return;

			if (m_ullSelectionStart < m_ullSelectionClick)
			{
				if (m_ullSelectionStart < m_dwCapacity)
				{
					ullStart = 0;
					ullSize = m_ullSelectionSize + m_ullSelectionStart;
				}
				else
				{
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
			break;
		}
	}
}

void CHexCtrl::OnKeyDownCtrl(UINT nChar)
{
	switch (nChar)
	{
	case 'F':
	case 'H':
		m_pDlgSearch->ShowWindow(SW_SHOW);
		break;
	case 'C':
		ClipboardCopy(EClipboard::COPY_HEX);
		break;
	case 'V':
		ClipboardPaste(EClipboard::PASTE_HEX);
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

void CHexCtrl::SetShowMode(EShowMode enShowMode)
{
	//Unchecking all menus and checking only the current needed.
	m_enShowMode = enShowMode;
	for (int i = 0; i < m_menuShowAs.GetMenuItemCount(); i++)
		m_menuShowAs.CheckMenuItem(i, MF_UNCHECKED | MF_BYPOSITION);

	int id { };
	switch (enShowMode)
	{
	case EShowMode::ASBYTE:
		id = 0;
		break;
	case EShowMode::ASWORD:
		id = 1;
		break;
	case EShowMode::ASDWORD:
		id = 2;
		break;
	case EShowMode::ASQWORD:
		id = 3;
		break;
	}
	m_menuShowAs.CheckMenuItem(id, MF_CHECKED | MF_BYPOSITION);
	SetCapacity(m_dwCapacity); //To recalc current representation.
}

void CHexCtrl::MsgWindowNotify(const HEXNOTIFYSTRUCT & hns)const
{
	//Send notification to the Message window if it was set.
	//Otherwise send to parent window.
	HWND hwndSend = GetMsgWindow();
	if (!hwndSend)
		hwndSend = GetParent()->GetSafeHwnd();
	if (hwndSend)
		::SendMessageW(hwndSend, WM_NOTIFY, GetDlgCtrlID(), (LPARAM)& hns);
}

void CHexCtrl::MsgWindowNotify(UINT uCode)const
{
	HEXNOTIFYSTRUCT hns { { m_hWnd, (UINT)GetDlgCtrlID(), uCode } };
	MsgWindowNotify(hns);
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

	ULONGLONG ullCurrScrollV = m_pScrollV->GetScrollPos();
	ULONGLONG ullCurrScrollH = m_pScrollH->GetScrollPos();
	int iCx, iCy;
	HexChunkPoint(m_ullCursorPos, iCx, iCy);
	CRect rcClient;
	GetClientRect(&rcClient);

	//New scroll depending on selection direction: top <-> bottom.
	ULONGLONG ullMaxV = ullCurrScrollV + rcClient.Height() - m_iHeightBottomOffArea - m_iStartWorkAreaY -
		((rcClient.Height() - m_iStartWorkAreaY - m_iHeightBottomOffArea) % m_sizeLetter.cy);
	ULONGLONG ullNewStartV = m_ullCursorPos / m_dwCapacity * m_sizeLetter.cy;
	ULONGLONG ullNewEndV = ullNewStartV;

	ULONGLONG ullNewScrollV { }, ullNewScrollH { };
	if (ullNewEndV >= ullMaxV)
		ullNewScrollV = ullCurrScrollV + m_sizeLetter.cy;
	else
	{
		if (ullNewEndV >= ullCurrScrollV)
			ullNewScrollV = ullCurrScrollV;
		else
			ullNewScrollV = ullCurrScrollV - m_sizeLetter.cy;
	}

	int iMaxClientX = rcClient.Width() - m_iSizeHexByte;
	if (iCx >= iMaxClientX)
		ullNewScrollH = ullCurrScrollH + (iCx - iMaxClientX);
	else if (iCx < 0)
		ullNewScrollH = ullCurrScrollH + iCx;
	else
		ullNewScrollH = ullCurrScrollH;

	ullNewScrollV -= ullNewScrollV % m_sizeLetter.cy;

	m_pScrollV->SetScrollPos(ullNewScrollV);
	if (m_pScrollH->IsVisible() && !IsCurTextArea()) //Do not horz scroll when modifying text area (not Hex).
		m_pScrollH->SetScrollPos(ullNewScrollH);

	RedrawWindow();
}

void CHexCtrl::CursorMoveRight()
{
	if (m_fMutable)
	{
		if (IsCurTextArea())
			SetCursorPos(m_ullCursorPos + 1, m_fCursorHigh);
		else if (m_fCursorHigh)
			SetCursorPos(m_ullCursorPos, false);
		else
			SetCursorPos(m_ullCursorPos + 1, true);
	}
	else
		SetSelection(m_ullSelectionClick + 1, m_ullSelectionClick + 1, 1);
}

void CHexCtrl::CursorMoveLeft()
{
	if (m_fMutable)
	{
		if (IsCurTextArea())
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
	else
		SetSelection(m_ullSelectionClick - 1, m_ullSelectionClick - 1, 1);
}

void CHexCtrl::CursorMoveUp()
{
	ULONGLONG ullNewPos = m_fMutable ? m_ullCursorPos : m_ullSelectionClick;
	if (ullNewPos >= m_dwCapacity)
		ullNewPos -= m_dwCapacity;

	if (m_fMutable)
		SetCursorPos(ullNewPos, m_fCursorHigh);
	else
		SetSelection(ullNewPos, ullNewPos, 1);
}

void CHexCtrl::CursorMoveDown()
{
	ULONGLONG ullNewPos = m_fMutable ? m_ullCursorPos : m_ullSelectionClick;
	ullNewPos += m_dwCapacity;

	if (m_fMutable)
		SetCursorPos(ullNewPos, m_fCursorHigh);
	else
		SetSelection(ullNewPos, ullNewPos, 1);
}

void CHexCtrl::Undo()
{
	switch (m_enMode)
	{
	case EHexDataMode::DATA_MEMORY:
	{
		if (m_deqUndo.empty())
			return;

		const auto& refUndo = m_deqUndo.back();
		const auto& refData = refUndo->vecData;

		//Making new Redo data snapshot.
		const auto& refRedo = m_deqRedo.emplace_back(std::make_unique<UNDOSTRUCT>());
		refRedo->ullIndex = refUndo->ullIndex;
		for (size_t i = 0; i < refData.size(); i++)
		{
			refRedo->vecData.push_back((std::byte)m_pData[refUndo->ullIndex + i]); //Fill Redo data with the next byte.
			m_pData[refUndo->ullIndex + i] = (BYTE)refData[i];                     //Undo the next byte.
		}
		m_deqUndo.pop_back();
	}
	break;
	case EHexDataMode::DATA_MSG:
	{
		//In EHexDataMode::DATA_MSG mode we send pointer to hms.
		HEXNOTIFYSTRUCT hns { { m_hWnd, (UINT)GetDlgCtrlID(), HEXCTRL_MSG_UNDO } };
		MsgWindowNotify(hns);
	}
	break;
	case EHexDataMode::DATA_VIRTUAL:
	{
		if (m_pHexVirtual)
			m_pHexVirtual->Undo();
	}
	break;
	}

	RedrawWindow();
}

void CHexCtrl::Redo()
{
	switch (m_enMode)
	{
	case EHexDataMode::DATA_MEMORY:
	{
		if (m_deqRedo.empty())
			return;

		const auto& refRedo = m_deqRedo.back();
		const auto& refData = refRedo->vecData;

		//Making new Undo data snapshot.
		SnapshotUndo(refRedo->ullIndex, refData.size());

		for (size_t i = 0; i < refData.size(); i++)
			m_pData[refRedo->ullIndex + i] = (BYTE)refData[i];

		m_deqRedo.pop_back();
	}
	break;
	case EHexDataMode::DATA_MSG:
	{
		//In EHexDataMode::DATA_MSG mode we send pointer to hms.
		HEXNOTIFYSTRUCT hns { { m_hWnd, (UINT)GetDlgCtrlID(), HEXCTRL_MSG_REDO } };
		MsgWindowNotify(hns);
	}
	break;
	case EHexDataMode::DATA_VIRTUAL:
	{
		if (m_pHexVirtual)
			m_pHexVirtual->Redo();
	}
	break;
	}

	RedrawWindow();
}

void CHexCtrl::SnapshotUndo(ULONGLONG ullIndex, ULONGLONG ullSize)
{
	//If Undo size is exceeding max limit,
	//remove first snapshot from the beginning (the oldest one).
	if (m_deqUndo.size() > (size_t)m_dwUndoMax)
		m_deqUndo.pop_front();

	//Making new Undo data snapshot.
	const auto& refUndo = m_deqUndo.emplace_back(std::make_unique<UNDOSTRUCT>());
	refUndo->ullIndex = ullIndex;
	for (size_t i = 0; i < ullSize; i++)
		refUndo->vecData.push_back((std::byte)GetByte(ullIndex + i));
}

bool CHexCtrl::IsCurTextArea()const
{
	return m_fCursorTextArea;
}

void CHexCtrl::SearchCallback(SEARCHSTRUCT & rSearch)
{
	if (rSearch.wstrSearch.empty() || m_ullDataSize == 0 || rSearch.ullIndex >= m_ullDataSize)
		return;

	rSearch.fFound = false;
	ULONGLONG ullIndex = rSearch.ullIndex;
	ULONGLONG ullSizeBytes;
	ULONGLONG ullUntil;
	ULONGLONG ullSizeBytesReplaced;
	DWORD dwSizeNext; //Bytes to add to the next search beginning.
	std::string strSearch;
	std::string strReplace;
	static const wchar_t* const wstrReplaceWarning { L"Replacing string is longer than Find string.\r\n"
		"Do you want to overwrite the bytes following search occurrence?\r\n"
		"Choosing \"No\" will cancel search." };

	//Perform convertation only for non Unicode search.
	if (rSearch.enSearchType == ESearchType::SEARCH_HEX ||
		rSearch.enSearchType == ESearchType::SEARCH_ASCII)
	{
		strSearch = WstrToStr(rSearch.wstrSearch);
		if (rSearch.fReplace)
		{
			strReplace = WstrToStr(rSearch.wstrReplace);

			//Check replace string size out of bounds.
			if ((strReplace.size() / 2 + rSearch.ullIndex) > m_ullDataSize)
			{
				rSearch.ullIndex = 0;
				rSearch.fSecondMatch = false;
				return;
			}
			if (strReplace.size() > strSearch.size() && m_fReplaceWarning)
				if (IDNO == MessageBoxW(wstrReplaceWarning, L"Warning", MB_YESNO | MB_ICONQUESTION | MB_TOPMOST))
					return;
				else
					m_fReplaceWarning = false;
		}
	}

	switch (rSearch.enSearchType)
	{
	case ESearchType::SEARCH_HEX:
	{
		if (!StrToHex(strSearch, strSearch))
		{
			rSearch.iWrap = 1;
			return;
		}
		if ((rSearch.fReplace && !StrToHex(strReplace, strReplace)) || (ullSizeBytes = strSearch.size()) > m_ullDataSize)
			return;

		ullSizeBytesReplaced = strReplace.size();
		dwSizeNext = (DWORD)strReplace.size() / 2;
	}
	break;
	case ESearchType::SEARCH_ASCII:
	{
		ullSizeBytes = strSearch.size();
		ullSizeBytesReplaced = dwSizeNext = (DWORD)strReplace.size();
		if (ullSizeBytes > m_ullDataSize)
			return;
	}
	break;
	case ESearchType::SEARCH_UTF16:
	{
		ullSizeBytes = rSearch.wstrSearch.size() * sizeof(wchar_t);
		ullSizeBytesReplaced = dwSizeNext = DWORD(rSearch.wstrReplace.size() * sizeof(wchar_t));
		if (ullSizeBytes > m_ullDataSize)
			return;
	}
	break;
	}

	///////////////Actual Search:////////////////////////////
	if (rSearch.fReplace && rSearch.fAll) //SearchReplace All
	{
		switch (rSearch.enSearchType)
		{
		case ESearchType::SEARCH_HEX:
		case ESearchType::SEARCH_ASCII:
			ullUntil = m_ullDataSize - strSearch.size();
			for (ULONGLONG i = 0; i <= ullUntil; ++i)
			{
				if (memcmp(m_pData + i, strSearch.data(), strSearch.size()) == 0)
				{
					SearchReplace(i, (PBYTE)strReplace.data(), strSearch.size(), strReplace.size(), false);
					i += dwSizeNext - 1;
					rSearch.fFound = true;
					rSearch.dwReplaced++;
				}
			}
			break;
		case ESearchType::SEARCH_UTF16:
			ullUntil = m_ullDataSize - ullSizeBytes;
			for (ULONGLONG i = ullIndex; i <= ullUntil; i++)
			{
				if (wmemcmp((const wchar_t*)(m_pData + i), rSearch.wstrSearch.data(), rSearch.wstrSearch.size()) == 0)
				{
					SearchReplace(i, (PBYTE)rSearch.wstrSearch.data(), rSearch.wstrSearch.size() * 2,
						rSearch.wstrReplace.size() * 2, false);
					i += dwSizeNext - 1; //To compensate i++ in loop.
					rSearch.fFound = true;
					rSearch.dwReplaced++;
				}
			}
			break;
		}
	}
	else //Search or SearchReplace
	{
		switch (rSearch.enSearchType)
		{
		case ESearchType::SEARCH_HEX:
		case ESearchType::SEARCH_ASCII:
			if (rSearch.iDirection == 1) //Forward direction
			{
				if (rSearch.fReplace && rSearch.fSecondMatch)
				{
					SearchReplace(rSearch.ullIndex, (PBYTE)strReplace.data(), strSearch.size(), strReplace.size());
					rSearch.ullIndex += dwSizeNext;
					rSearch.dwReplaced++;
				}

				ullUntil = m_ullDataSize - strSearch.size();
				ullIndex = rSearch.fSecondMatch ? rSearch.ullIndex + 1 : 0;
				for (ULONGLONG i = ullIndex; i <= ullUntil; i++)
				{
					if (memcmp(m_pData + i, strSearch.data(), strSearch.size()) == 0)
					{
						rSearch.fFound = true;
						rSearch.fSecondMatch = true;
						rSearch.ullIndex = i;
						rSearch.fWrap = false;
						rSearch.dwCount++;
						break;
					}
				}
				if (!rSearch.fFound && rSearch.fSecondMatch)
				{
					ullIndex = 0; //Starting from the beginning.
					for (ULONGLONG i = ullIndex; i <= ullUntil; i++)
					{
						if (memcmp(m_pData + i, strSearch.data(), strSearch.size()) == 0)
						{
							rSearch.fFound = true;
							rSearch.fSecondMatch = true;
							rSearch.fWrap = true;
							rSearch.fDoCount = true;
							rSearch.dwCount = 1;

							if (rSearch.fReplace)
							{
								SearchReplace(i, (PBYTE)strReplace.data(), strSearch.size(), strReplace.size());
								rSearch.ullIndex = i + strReplace.size();
							}
							else
								rSearch.ullIndex = i;

							break;
						}
					}
					rSearch.iWrap = 1;
				}
			}
			else if (rSearch.iDirection == -1) //Backward direction
			{
				if (rSearch.fSecondMatch && ullIndex > 0)
				{
					ullIndex--;
					for (INT_PTR i = (INT_PTR)ullIndex; i >= 0; i--)
					{
						if (memcmp(m_pData + i, strSearch.data(), strSearch.size()) == 0)
						{
							rSearch.fFound = true;
							rSearch.fSecondMatch = true;
							rSearch.ullIndex = i;
							rSearch.fWrap = false;
							rSearch.dwCount--;
							break;
						}
					}
				}
				if (!rSearch.fFound)
				{
					ullIndex = m_ullDataSize - strSearch.size();
					for (INT_PTR i = (INT_PTR)ullIndex; i >= 0; i--)
					{
						if (memcmp(m_pData + i, strSearch.data(), strSearch.size()) == 0)
						{
							rSearch.fFound = true;
							rSearch.fSecondMatch = true;
							rSearch.ullIndex = i;
							rSearch.fWrap = true;
							rSearch.iWrap = -1;
							rSearch.fDoCount = false;
							rSearch.dwCount = 1;
							break;
						}
					}
				}
			}
			break;
		case ESearchType::SEARCH_UTF16:
		{
			if (rSearch.iDirection == 1)
			{
				if (rSearch.fReplace && rSearch.fSecondMatch)
				{
					SearchReplace(rSearch.ullIndex, (PBYTE)rSearch.wstrReplace.data(), rSearch.wstrSearch.size() * 2,
						rSearch.wstrReplace.size() * 2);
					rSearch.ullIndex += dwSizeNext;
					rSearch.dwReplaced++;
				}

				ullUntil = m_ullDataSize - ullSizeBytes;
				ullIndex = rSearch.fSecondMatch ? rSearch.ullIndex + 1 : 0;
				for (ULONGLONG i = ullIndex; i <= ullUntil; i++)
				{
					if (wmemcmp((const wchar_t*)(m_pData + i), rSearch.wstrSearch.data(), rSearch.wstrSearch.size()) == 0)
					{
						rSearch.fFound = true;
						rSearch.fSecondMatch = true;
						rSearch.ullIndex = i;
						rSearch.fWrap = false;
						rSearch.dwCount++;
						break;
					}
				}
				if (!rSearch.fFound && rSearch.fSecondMatch)
				{
					ullIndex = 0;
					for (ULONGLONG i = ullIndex; i <= ullUntil; i++)
					{
						if (wmemcmp((const wchar_t*)(m_pData + i), rSearch.wstrSearch.data(), rSearch.wstrSearch.size()) == 0)
						{
							rSearch.fFound = true;
							rSearch.fSecondMatch = true;
							rSearch.ullIndex = i;
							rSearch.iWrap = 1;
							rSearch.fWrap = true;
							rSearch.fDoCount = true;
							rSearch.dwCount = 1;
							break;
						}
					}
				}
			}
			else if (rSearch.iDirection == -1)
			{
				if (rSearch.fSecondMatch && ullIndex > 0)
				{
					ullIndex--;
					for (INT_PTR i = (INT_PTR)ullIndex; i >= 0; i--)
					{
						if (wmemcmp((const wchar_t*)(m_pData + i), rSearch.wstrSearch.data(), rSearch.wstrSearch.size()) == 0)
						{
							rSearch.fFound = true;
							rSearch.fSecondMatch = true;
							rSearch.ullIndex = i;
							rSearch.fWrap = false;
							rSearch.dwCount--;
							break;
						}
					}
				}
				if (!rSearch.fFound)
				{
					ullIndex = m_ullDataSize - ullSizeBytes;
					for (INT_PTR i = (INT_PTR)ullIndex; i >= 0; i--)
					{
						if (wmemcmp((const wchar_t*)(m_pData + i), rSearch.wstrSearch.data(), rSearch.wstrSearch.size()) == 0)
						{
							rSearch.fFound = true;
							rSearch.fSecondMatch = true;
							rSearch.ullIndex = i;
							rSearch.fWrap = true;
							rSearch.iWrap = -1;
							rSearch.fDoCount = false;
							rSearch.dwCount = 1;
							break;
						}
					}
				}
			}
			break;
		}
		}
	}

	if (rSearch.fFound)
	{
		if (rSearch.fReplace && rSearch.fAll)
			RedrawWindow();
		else
		{
			ULONGLONG ullSelIndex = rSearch.ullIndex;
			ULONGLONG ullSelSize = rSearch.fReplace ? ullSizeBytesReplaced : ullSizeBytes;
			SetSelection(ullSelIndex, ullSelIndex, ullSelSize, true);
		}
	}
}

void CHexCtrl::SearchReplace(ULONGLONG ullIndex, PBYTE pData, size_t nSizeData, size_t nSizeReplace, bool fRedraw)
{
	HEXMODIFYSTRUCT hms;
	hms.ullSize = nSizeData;
	hms.ullDataSize = nSizeReplace;
	hms.ullIndex = ullIndex;
	hms.pData = pData;
	ModifyData(hms, fRedraw);
}

void CHexCtrl::SetSelection(ULONGLONG ullClick, ULONGLONG ullStart, ULONGLONG ullSize, bool fHighlight, bool fMouse)
{
	if (ullClick >= m_ullDataSize || ullStart >= m_ullDataSize || !ullSize)
		return;
	if ((ullStart + ullSize) > m_ullDataSize)
		ullSize = m_ullDataSize - ullStart;

	ULONGLONG ullCurrScrollV = m_pScrollV->GetScrollPos();
	ULONGLONG ullCurrScrollH = m_pScrollH->GetScrollPos();
	int iCx, iCy;
	HexChunkPoint(ullStart, iCx, iCy);
	CRect rcClient;
	GetClientRect(&rcClient);

	//New scroll depending on selection direction: top <-> bottom.
	//fHighlight means centralize scroll position on the screen (used in SearchCallback()).
	ULONGLONG ullEnd = ullStart + ullSize; //ullEnd is exclusive.
	ULONGLONG ullMaxV = ullCurrScrollV + rcClient.Height() - m_iHeightBottomOffArea - m_iStartWorkAreaY -
		((rcClient.Height() - m_iStartWorkAreaY - m_iHeightBottomOffArea) % m_sizeLetter.cy);
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

		int iMaxClientX = rcClient.Width() - m_iSizeHexByte;
		if (iCx >= iMaxClientX)
			ullNewScrollH = ullCurrScrollH + (iCx - iMaxClientX);
		else if (iCx < 0)
			ullNewScrollH = ullCurrScrollH + iCx;
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
		m_pScrollV->SetScrollPos(ullNewScrollV);
		if (m_pScrollH->IsVisible() && !IsCurTextArea())
			m_pScrollH->SetScrollPos(ullNewScrollH);
	}

	UpdateInfoText();

	MsgWindowNotify(HEXCTRL_MSG_SETSELECTION);
}

void CHexCtrl::SelectAll()
{
	if (!m_ullDataSize)
		return;

	m_ullSelectionClick = m_ullSelectionStart = 0;
	m_ullSelectionEnd = m_ullSelectionSize = m_ullDataSize;
	UpdateInfoText();
}

void CHexCtrl::FillWithZeros()
{
	HEXMODIFYSTRUCT hms;
	hms.ullSize = m_ullSelectionSize;
	hms.ullIndex = m_ullSelectionStart;
	hms.ullDataSize = 1;
	hms.fRepeat = true;
	unsigned char chZero { 0 };
	hms.pData = &chZero;
	ModifyData(hms);
}

void CHexCtrl::FillWstrCapacity()
{
	m_wstrCapacity.clear();
	m_wstrCapacity.reserve((size_t)m_dwCapacity * 3);
	for (unsigned iter = 0; iter < m_dwCapacity; iter++)
	{
		if (iter < 0x10)
		{
			m_wstrCapacity += L" ";
			m_wstrCapacity += g_pwszHexMap[iter];
		}
		else
		{
			m_wstrCapacity += g_pwszHexMap[(iter & 0xF0) >> 4];
			m_wstrCapacity += g_pwszHexMap[iter & 0x0F];
		}

		//Additional space between hex chunk blocks.
		if (((iter + 1) % (DWORD)m_enShowMode) == 0)
			m_wstrCapacity += L" ";

		//Additional space between hex halves.
		if (m_enShowMode == EShowMode::ASBYTE && iter == (m_dwCapacityBlockSize - 1))
			m_wstrCapacity += L"  ";
	}
}