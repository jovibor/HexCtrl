/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/                         *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#include "stdafx.h"
#include "CHexCtrl.h"
#include "Dialogs/CHexDlgAbout.h"
#include "Dialogs/CHexDlgSearch.h"
#include "Dialogs/CHexDlgOperations.h"
#include "Dialogs/CHexDlgFillWith.h"
#include "CScrollEx.h"
#include "CHexSelect.h"
#include "CHexBookmarks.h"
#include "Helper.h"
#include "strsafe.h"
#include <cassert>
#include <algorithm>
#pragma comment(lib, "Dwmapi.lib")

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

namespace HEXCTRL {
	/********************************************
	* CreateRawHexCtrl function implementation. *
	********************************************/
	extern "C" HEXCTRLAPI IHexCtrl * __cdecl CreateRawHexCtrl()
	{
		return new CHexCtrl();
	};

	extern "C" HEXCTRLAPI HEXCTRLINFO * __cdecl GetHexCtrlInfo()
	{
		static HEXCTRLINFO stVersion { HEXCTRL_VERSION_WSTR, HEXCTRL_VERSION_ULL };

		return &stVersion;
	};

	/********************************************
	* Internal enums and structs.               *
	********************************************/
	namespace INTERNAL {
		enum class EClipboard : DWORD
		{
			COPY_HEX, COPY_HEX_LE, COPY_HEX_FORMATTED, COPY_ASCII, COPY_BASE64,
			PASTE_HEX, PASTE_ASCII
		};

		//Structure for UNDO command routine.
		struct UNDOSTRUCT
		{
			ULONGLONG              ullOffset { }; //Start byte to apply Undo to.
			std::vector<std::byte> vecData { };  //Data for Undo.
		};

		constexpr auto WSTR_HEXCTRL_CLASSNAME = L"HexCtrl";
	}
}

/************************************************************************
* CHexCtrl implementation.                                              *
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
	ON_WM_INITMENUPOPUP()
	ON_WM_NCACTIVATE()
	ON_WM_NCCALCSIZE()
	ON_WM_NCPAINT()
	ON_WM_CHAR()
	ON_WM_GETDLGCODE()
END_MESSAGE_MAP()

//CWinApp object vital for manual MFC, and for in-.dll work,
//to run properly (Resources handling and assertions.)
#if defined HEXCTRL_MANUAL_MFC_INIT || defined HEXCTRL_SHARED_DLL
CWinApp theApp;
#endif

CHexCtrl::CHexCtrl()
{
	//MFC initialization, if HexCtrl is used in non MFC project, with Shared MFC linking.
#if defined HEXCTRL_MANUAL_MFC_INIT
	if (!AfxGetModuleState()->m_lpszCurrentAppName)
		AfxWinInit(::GetModuleHandleW(NULL), NULL, ::GetCommandLineW(), 0);
#endif

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
			return;
		}
	}

	//Menus
	if (m_menuMain.LoadMenuW(MAKEINTRESOURCE(IDR_HEXCTRL_MENU)))
	{
		MENUITEMINFOW mii { };
		mii.cbSize = sizeof(MENUITEMINFOW);
		mii.fMask = MIIM_BITMAP;
		mii.hbmpItem = m_umapHBITMAP[IDM_HEXCTRL_CLIPBOARD_COPYHEX] =
			(HBITMAP)LoadImageW(hInst, MAKEINTRESOURCE(IDB_HEXCTRL_MENU_COPY), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
		m_menuMain.SetMenuItemInfoW(IDM_HEXCTRL_CLIPBOARD_COPYHEX, &mii);
		mii.hbmpItem = m_umapHBITMAP[IDM_HEXCTRL_CLIPBOARD_PASTEHEX] =
			(HBITMAP)LoadImageW(hInst, MAKEINTRESOURCE(IDB_HEXCTRL_MENU_PASTE), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
		m_menuMain.SetMenuItemInfoW(IDM_HEXCTRL_CLIPBOARD_PASTEHEX, &mii);
		mii.hbmpItem = m_umapHBITMAP[IDM_HEXCTRL_MODIFY_FILLZEROS] =
			(HBITMAP)LoadImageW(hInst, MAKEINTRESOURCE(IDB_HEXCTRL_MENU_FILL_ZEROS), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
		m_menuMain.SetMenuItemInfoW(IDM_HEXCTRL_MODIFY_FILLZEROS, &mii);
	}

	m_pDlgSearch->Create(IDD_HEXCTRL_SEARCH, this);
	m_pDlgOpers->Create(IDD_HEXCTRL_OPERATIONS, this);
	m_pDlgFillWith->Create(IDD_HEXCTRL_FILLWITHDATA, this);
	m_pBookmarks->Attach(this);
}

CHexCtrl::~CHexCtrl()
{
	//Deleting all loaded bitmaps.
	for (auto const& i : m_umapHBITMAP)
		DeleteObject(i.second);
}

void CHexCtrl::ClearData()
{
	assert(IsCreated()); //Not created.
	if (!IsCreated())
		return;

	m_fDataSet = false;
	m_ullDataSize = 0;
	m_pData = nullptr;
	m_fMutable = false;
	m_ullSelectionClick = 0;

	m_deqUndo.clear();
	m_deqRedo.clear();
	m_pScrollV->SetScrollPos(0);
	m_pScrollH->SetScrollPos(0);
	m_pScrollV->SetScrollSizes(0, 0, 0);
	m_pBookmarks->ClearAll();
	m_pSelect->ClearAll();
	UpdateInfoText();
}

bool CHexCtrl::Create(const HEXCREATESTRUCT& hcs)
{
	assert(!IsCreated()); //Already created.
	if (IsCreated())
		return false;

	m_hwndMsg = hcs.hwndParent;
	m_stColor = hcs.stColor;
	m_enShowMode = hcs.enShowMode;
	m_dbWheelRatio = hcs.dbWheelRatio;

	DWORD dwStyle = hcs.dwStyle;
	//1. WS_POPUP style is vital for GetParent to work properly in EHexCreateMode::CREATE_POPUP mode.
	//   Without this style GetParent/GetOwner always return 0, no matter whether pParentWnd is provided to CreateWindowEx or not.
	//2. Created HexCtrl window will always overlap (be on top of) its parent, or owner, window 
	//   if pParentWnd is set (is not nullptr) in CreateWindowEx.
	//3. To force HexCtrl window on taskbar the WS_EX_APPWINDOW extended window style must be set.
	CStringW strError;
	CRect rc = hcs.rect;
	UINT uID = 0;
	switch (hcs.enCreateMode)
	{
	case EHexCreateMode::CREATE_POPUP:
	{
		dwStyle |= WS_VISIBLE | WS_POPUP | WS_OVERLAPPEDWINDOW;
		if (rc.IsRectNull())
		{	//If initial rect is null, and it's a float window, then place it at screen center.
			int iPosX = GetSystemMetrics(SM_CXSCREEN) / 4;
			int iPosY = GetSystemMetrics(SM_CYSCREEN) / 4;
			int iPosCX = iPosX * 3;
			int iPosCY = iPosY * 3;
			rc.SetRect(iPosX, iPosY, iPosCX, iPosCY);
		}
	}
	break;
	case EHexCreateMode::CREATE_CHILD:
	{
		dwStyle |= WS_VISIBLE | WS_CHILD;
		uID = hcs.uID;
	}
	break;
	case EHexCreateMode::CREATE_CUSTOMCTRL:
	{   //If it's a Custom Control in dialog, there is no need to create a window, just subclassing.
		if (!SubclassDlgItem(hcs.uID, CWnd::FromHandle(hcs.hwndParent)))
			strError.Format(L"HexCtrl (Id:%u) SubclassDlgItem failed.\r\nCheck CreateDialogCtrl parameters.", hcs.uID);
	}
	break;
	}

	//Creating window.
	if (hcs.enCreateMode != EHexCreateMode::CREATE_CUSTOMCTRL)
	{
		if (!CWnd::CreateEx(hcs.dwExStyle, WSTR_HEXCTRL_CLASSNAME, L"HexControl",
			dwStyle, rc, CWnd::FromHandle(hcs.hwndParent), uID))
			strError.Format(L"HexCtrl (Id:%u) CreateEx failed.\r\nCheck HEXCREATESTRUCT parameters.", hcs.uID);
	}

	if (strError.GetLength()) //If there was some Creation error.
	{
		MessageBoxW(strError, L"Error", MB_ICONERROR);
		return false;
	}

	//Font related.//////////////////////////////////////////////
	LOGFONTW lf { };
	if (hcs.pLogFont)
		lf = *hcs.pLogFont;
	else
	{
		StringCchCopyW(lf.lfFaceName, 9, L"Consolas");
		lf.lfHeight = 18;
	}
	m_lFontSize = lf.lfHeight;
	m_fontHexView.CreateFontIndirectW(&lf);

	lf.lfHeight = 16;
	m_fontBottomRect.CreateFontIndirectW(&lf);
	//End of font related.///////////////////////////////////////

	//Removing window's border frame.
	MARGINS marg { 0, 0, 0, 1 };
	DwmExtendFrameIntoClientArea(m_hWnd, &marg);
	SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

	//ScrollBars should be created here, after main Window (to attach to) has already been created. To avoid assertions.
	m_pScrollV->Create(this, SB_VERT, 0, 0, 0); //Actual sizes are set in RecalcAll().
	m_pScrollH->Create(this, SB_HORZ, 0, 0, 0);
	m_pScrollV->AddSibling(m_pScrollH.get());
	m_pScrollH->AddSibling(m_pScrollV.get());

	m_fCreated = true;
	SetShowMode(m_enShowMode);
	RecalcAll();

	return true;
}

bool CHexCtrl::CreateDialogCtrl(UINT uCtrlID, HWND hwndDlg)
{
	assert(!IsCreated()); //Already created.
	if (IsCreated())
		return false;

	HEXCREATESTRUCT hcs;
	hcs.hwndParent = hwndDlg;
	hcs.uID = uCtrlID;
	hcs.enCreateMode = EHexCreateMode::CREATE_CUSTOMCTRL;

	return Create(hcs);
}

void CHexCtrl::Destroy()
{
	delete this;
}

DWORD CHexCtrl::GetCapacity()const
{
	assert(IsCreated()); //Not created.
	if (!IsCreated())
		return 0;

	return m_dwCapacity;
}

auto CHexCtrl::GetColor()const->HEXCOLORSTRUCT
{
	assert(IsCreated()); //Not created.
	if (!IsCreated())
		return { };

	return m_stColor;
}

long CHexCtrl::GetFontSize()const
{
	assert(IsCreated()); //Not created.
	if (!IsCreated())
		return 0;

	return m_lFontSize;
}

HMENU CHexCtrl::GetMenuHandle()const
{
	assert(IsCreated()); //Not created.
	if (!IsCreated())
		return nullptr;

	return m_menuMain.m_hMenu;
}

auto CHexCtrl::GetSelection()const->std::vector<HEXSPANSTRUCT>&
{
	assert(IsCreated()); //Not created.
	assert(IsDataSet()); //Data is not set yet.

	return m_pSelect->GetVector();
}

auto CHexCtrl::GetShowMode() const -> EHexShowMode
{
	assert(IsCreated()); //Not created.
	if (!IsCreated())
		return EHexShowMode { };

	return m_enShowMode;
}

HWND CHexCtrl::GetWindowHandle() const
{
	assert(IsCreated()); //Not created.
	if (!IsCreated())
		return nullptr;

	return m_hWnd;
}

void CHexCtrl::GoToOffset(ULONGLONG ullOffset, bool fSelect, ULONGLONG ullSize)
{
	assert(IsCreated());  //Not created.
	assert(IsDataSet()); //Data is not set yet.
	if (!IsCreated() || !IsDataSet())
		return;

	if (fSelect)
		SetSelection(ullOffset, ullOffset, ullSize, 1, true, true);
	else
		GoToOffset(ullOffset);
}

bool CHexCtrl::IsCreated()const
{
	return m_fCreated;
}

bool CHexCtrl::IsDataSet()const
{
	assert(IsCreated()); //Not created.
	if (!IsCreated())
		return false;

	return m_fDataSet;
}

bool CHexCtrl::IsMutable()const
{
	assert(IsCreated()); //Not created.
	assert(IsDataSet()); //Data is not set yet.
	if (!IsCreated() || !IsDataSet())
		return false;

	return m_fMutable;
}

void CHexCtrl::SetCapacity(DWORD dwCapacity)
{
	assert(IsCreated()); //Not created.
	if (!IsCreated())
		return;

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

	WstrCapacityFill();
	RecalcAll();
}

void CHexCtrl::SetColor(const HEXCOLORSTRUCT& clr)
{
	assert(IsCreated()); //Not created.
	if (!IsCreated())
		return;

	m_stColor = clr;
	RedrawWindow();
}

void CHexCtrl::SetData(const HEXDATASTRUCT& hds)
{
	assert(IsCreated()); //Not created.
	if (!IsCreated())
		return;

	//m_hwndMsg was previously set in IHexCtrl::Create.
	if (hds.hwndMsg)
		m_hwndMsg = hds.hwndMsg;

	//Virtual mode is possible only when there is a MSG window, a data requests will be sent to.
	if (hds.enCreateMode == EHexDataMode::DATA_MSG && !GetMsgWindow())
	{
		MessageBoxW(L"HexCtrl EHexDataMode::DATA_MSG mode requires HEXDATASTRUCT::hwndMsg to be set.", L"Error", MB_ICONWARNING);
		return;
	}
	else if (hds.enCreateMode == EHexDataMode::DATA_VIRTUAL && !hds.pHexVirtual)
	{
		MessageBoxW(L"HexCtrl EHexDataMode::DATA_VIRTUAL mode requires HEXDATASTRUCT::pHexVirtual to be set.", L"Error", MB_ICONWARNING);
		return;
	}

	ClearData();

	m_fDataSet = true;
	m_pData = hds.pData;
	m_ullDataSize = hds.ullDataSize;
	m_enCreateMode = hds.enCreateMode;
	m_fMutable = hds.fMutable;
	m_dwOffsetDigits = hds.ullDataSize <= 0xffffffffUL ? 8 :
		(hds.ullDataSize <= 0xffffffffffUL ? 10 :
		(hds.ullDataSize <= 0xffffffffffffUL ? 12 :
			(hds.ullDataSize <= 0xffffffffffffffUL ? 14 : 16)));

	RecalcAll();

	if (hds.ullSelectionSize)
		SetSelection(hds.ullSelectionStart, hds.ullSelectionStart, hds.ullSelectionSize, 1, true, true);
}

void CHexCtrl::SetFont(const LOGFONTW* pLogFontNew)
{
	assert(IsCreated()); //Not created.
	assert(pLogFontNew); //Null font pointer.
	if (!IsCreated() || !pLogFontNew)
		return;

	m_fontHexView.DeleteObject();
	m_fontHexView.CreateFontIndirectW(pLogFontNew);

	RecalcAll();
}

void CHexCtrl::SetFontSize(UINT uiSize)
{
	assert(IsCreated()); //Not created.
	if (!IsCreated())
		return;

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

void CHexCtrl::SetMutable(bool fEnable)
{
	assert(IsCreated()); //Not created.
	assert(IsDataSet()); //Data is not set yet.
	if (!IsCreated() || !IsDataSet())
		return;

	m_fMutable = fEnable;
	RedrawWindow();
}

void CHexCtrl::SetSelection(ULONGLONG ullOffset, ULONGLONG ullSize)
{
	assert(IsCreated()); //Not created.
	assert(IsDataSet()); //Data is not set yet.
	if (!IsCreated() || !IsDataSet())
		return;

	SetSelection(ullOffset, ullOffset, ullSize, 1, false);
}

void CHexCtrl::SetShowMode(EHexShowMode enShowMode)
{
	assert(IsCreated()); //Not created.
	if (!IsCreated())
		return;

	m_enShowMode = enShowMode;

	//Getting the "Show data as..." menu.
	auto pMenu = m_menuMain.GetSubMenu(0)->GetSubMenu(2); //Separators are also count as menu!!!
	if (!pMenu)
		return;

	//Unchecking all menus and checking only the current needed.
	for (int i = 0; i < pMenu->GetMenuItemCount(); i++)
		pMenu->CheckMenuItem(i, MF_UNCHECKED | MF_BYPOSITION);

	int iId { };
	switch (enShowMode)
	{
	case EHexShowMode::ASBYTE:
		iId = 0;
		break;
	case EHexShowMode::ASWORD:
		iId = 1;
		break;
	case EHexShowMode::ASDWORD:
		iId = 2;
		break;
	case EHexShowMode::ASQWORD:
		iId = 3;
		break;
	}
	pMenu->CheckMenuItem(iId, MF_CHECKED | MF_BYPOSITION);
	SetCapacity(m_dwCapacity); //To recalc current representation.
}

void CHexCtrl::SetWheelRatio(double dbRatio)
{
	assert(IsCreated()); //Not created.
	if (!IsCreated())
		return;

	m_dbWheelRatio = dbRatio;
	SendMessageW(WM_SIZE);   //To recalc ScrollPageSize (m_pScrollV->SetScrollPageSize(...);)
}


/**************************************************************************
* HexCtrl Private methods.
**************************************************************************/
void CHexCtrl::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
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
		if (ullHit == 0xFFFFFFFFFFFFFFFFull)
			return;

		ULONGLONG ullClick, ullStart, ullSize, ullLines;
		if (m_fSelectionBlock) //Select block (with Alt)
		{
			ullClick = m_ullSelectionClick;
			if (ullHit >= ullClick)
			{
				if (ullHit % m_dwCapacity <= ullClick % m_dwCapacity)
				{
					DWORD dwModStart = ullClick % m_dwCapacity - ullHit % m_dwCapacity;
					ullStart = ullClick - dwModStart;
					ullSize = dwModStart + 1;
				}
				else
				{
					ullStart = ullClick;
					ullSize = ullHit % m_dwCapacity - ullClick % m_dwCapacity + 1;
				}
				ullLines = (ullHit - ullStart) / m_dwCapacity + 1;
			}
			else
			{
				if (ullHit % m_dwCapacity <= ullClick % m_dwCapacity)
				{
					ullStart = ullHit;
					ullSize = ullClick % m_dwCapacity - ullStart % m_dwCapacity + 1;
				}
				else
				{
					DWORD dwModStart = ullHit % m_dwCapacity - ullClick % m_dwCapacity;
					ullStart = ullHit - dwModStart;
					ullSize = dwModStart + 1;
				}
				ullLines = (ullClick - ullStart) / m_dwCapacity + 1;
			}
		}
		else
		{
			if (ullHit <= m_ullSelectionClick)
			{
				ullClick = m_ullSelectionClick;
				ullStart = ullHit;
				ullSize = ullClick - ullStart + 1;
			}
			else
			{
				ullClick = m_ullSelectionClick;
				ullStart = m_ullSelectionClick;
				ullSize = ullHit - ullClick + 1;
			}
			ullLines = 1;
		}

		SetSelection(ullClick, ullStart, ullSize, ullLines, false);
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
	if (ullHit == 0xFFFFFFFFFFFFFFFFull)
		return;

	SetCapture();

	if (GetAsyncKeyState(VK_MENU) < 0)
		m_fSelectionBlock = true;
	else
		m_fSelectionBlock = false;

	ULONGLONG ullSelSize;
	ULONGLONG ullSelStart;

	if (m_pSelect->HasSelection() && (nFlags & MK_SHIFT))
	{
		ULONGLONG ullSelEnd;
		if (ullHit <= m_ullSelectionClick)
		{
			ullSelStart = ullHit;
			ullSelEnd = m_ullSelectionClick + 1;
		}
		else
		{
			ullSelStart = m_ullSelectionClick;
			ullSelEnd = ullHit + 1;
		}
		ullSelSize = ullSelEnd - ullSelStart;
	}
	else
	{
		m_ullSelectionClick = ullSelStart = ullHit;
		ullSelSize = 1;
	}
	std::vector<HEXSPANSTRUCT> vecSel { { ullSelStart, ullSelSize } };
	m_pSelect->SetSelection(vecSel);

	m_ullCursorPos = ullSelStart;
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

void CHexCtrl::OnMButtonDown(UINT /*nFlags*/, CPoint /*point*/)
{
}

BOOL CHexCtrl::OnCommand(WPARAM wParam, LPARAM lParam)
{
	static const wchar_t* const pwszErrVirtual { L"This function isn't supported in Virtual mode!" };
	UINT_PTR uId = LOWORD(wParam);

	switch (uId)
	{
	case IDM_HEXCTRL_MAIN_SEARCH:
		if (m_enCreateMode == EHexDataMode::DATA_MEMORY)
			m_pDlgSearch->ShowWindow(SW_SHOW);
		else
			MessageBoxW(pwszErrVirtual, L"Error", MB_ICONEXCLAMATION);
		break;
	case IDM_HEXCTRL_SHOWAS_BYTE:
		SetShowMode(EHexShowMode::ASBYTE);
		break;
	case IDM_HEXCTRL_SHOWAS_WORD:
		SetShowMode(EHexShowMode::ASWORD);
		break;
	case IDM_HEXCTRL_SHOWAS_DWORD:
		SetShowMode(EHexShowMode::ASDWORD);
		break;
	case IDM_HEXCTRL_SHOWAS_QWORD:
		SetShowMode(EHexShowMode::ASQWORD);
		break;
	case IDM_HEXCTRL_BOOKMARKS_ADD:
		m_pBookmarks->Add(m_pSelect->GetVector());
		break;
	case IDM_HEXCTRL_BOOKMARKS_REMOVE:
		m_pBookmarks->Remove(m_ullRMouseChunk != 0xFFFFFFFFFFFFFFFFull ? m_ullRMouseChunk : GetCursorPos());
		m_ullRMouseChunk = 0xFFFFFFFFFFFFFFFFull;
		break;
	case IDM_HEXCTRL_BOOKMARKS_NEXT:
		m_pBookmarks->GoNext();
		break;
	case IDM_HEXCTRL_BOOKMARKS_PREV:
		m_pBookmarks->GoPrev();
		break;
	case IDM_HEXCTRL_BOOKMARKS_CLEARALL:
		m_pBookmarks->ClearAll();
		break;
	case IDM_HEXCTRL_CLIPBOARD_COPYHEX:
		ClipboardCopy(EClipboard::COPY_HEX);
		break;
	case IDM_HEXCTRL_CLIPBOARD_COPYHEXLE:
		ClipboardCopy(EClipboard::COPY_HEX_LE);
		break;
	case IDM_HEXCTRL_CLIPBOARD_COPYHEXFORMATTED:
		ClipboardCopy(EClipboard::COPY_HEX_FORMATTED);
		break;
	case IDM_HEXCTRL_CLIPBOARD_COPYASCII:
		ClipboardCopy(EClipboard::COPY_ASCII);
		break;
	case IDM_HEXCTRL_CLIPBOARD_COPYBASE64:
		ClipboardCopy(EClipboard::COPY_BASE64);
		break;
	case IDM_HEXCTRL_CLIPBOARD_PASTEHEX:
		ClipboardPaste(EClipboard::PASTE_HEX);
		break;
	case IDM_HEXCTRL_CLIPBOARD_PASTEASCII:
		ClipboardPaste(EClipboard::PASTE_ASCII);
		break;
	case IDM_HEXCTRL_MAIN_UNDO:
		Undo();
		break;
	case IDM_HEXCTRL_MAIN_REDO:
		Redo();
		break;
	case IDM_HEXCTRL_MODIFY_OPERATIONS:
		m_pDlgOpers->ShowWindow(SW_SHOW);
		break;
	case IDM_HEXCTRL_MODIFY_FILLZEROS:
		FillWithZeros();
		break;
	case IDM_HEXCTRL_MODIFY_FILLWITHDATA:
		m_pDlgFillWith->ShowWindow(SW_SHOW);
		break;
	case IDM_HEXCTRL_MAIN_SELECTALL:
		SelectAll();
		break;
	case IDM_HEXCTRL_APPEARANCE_FONTINCREASE:
		SetFontSize(GetFontSize() + 2);
		break;
	case IDM_HEXCTRL_APPEARANCE_FONTDECREASE:
		SetFontSize(GetFontSize() - 2);
		break;
	case IDM_HEXCTRL_APPEARANCE_CAPACITYINCREASE:
		SetCapacity(m_dwCapacity + 1);
		break;
	case IDM_HEXCTRL_APPEARANCE_CAPACITYDECREASE:
		SetCapacity(m_dwCapacity - 1);
		break;
	case IDM_HEXCTRL_MAIN_ABOUT:
	{
		CHexDlgAbout dlgAbout(this);
		dlgAbout.DoModal();
	}
	break;
	default:
		//For user defined custom menu, we notify parent window.
		HEXNOTIFYSTRUCT hns { { m_hWnd, (UINT)GetDlgCtrlID(), HEXCTRL_MSG_MENUCLICK } };
		hns.uMenuId = uId;
		MsgWindowNotify(hns);
	}

	return CWnd::OnCommand(wParam, lParam);
}

void CHexCtrl::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	POINT pp = point;
	ScreenToClient(&pp);
	m_ullRMouseChunk = HitTest(&pp);

	//Notifying parent that we are about to display context menu.
	MsgWindowNotify(HEXCTRL_MSG_ONCONTEXTMENU);
	m_menuMain.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON, point.x, point.y, this);
}

void CHexCtrl::OnInitMenuPopup(CMenu* pPopupMenu, UINT /*nIndex*/, BOOL /*bSysMenu*/)
{
	UINT uStatus;
	bool fDataSet = IsDataSet() && m_ullDataSize; //To prevent menu on zero sized data.

	if (!fDataSet || !m_pSelect->HasSelection())
		uStatus = MF_GRAYED;
	else
		uStatus = MF_ENABLED;

	if (pPopupMenu == m_menuMain.GetSubMenu(0)) //Main menu
	{
		//Main
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_MAIN_SEARCH, (fDataSet ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_MAIN_UNDO, (m_deqUndo.empty() ? MF_GRAYED : MF_ENABLED) | MF_BYCOMMAND);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_MAIN_REDO, (m_deqRedo.empty() ? MF_GRAYED : MF_ENABLED) | MF_BYCOMMAND);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_MODIFY_FILLZEROS, (m_fMutable ? uStatus : MF_GRAYED) | MF_BYCOMMAND);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_MODIFY_FILLWITHDATA, (m_fMutable ? uStatus : MF_GRAYED) | MF_BYCOMMAND);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_MODIFY_OPERATIONS, (m_fMutable ? uStatus : MF_GRAYED) | MF_BYCOMMAND);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_MAIN_SELECTALL, (fDataSet ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);

		//Bookmarks
		bool fBookmarks = m_pBookmarks->HasBookmarks();
		bool fHasRMBM = m_pBookmarks->HitTest(m_ullRMouseChunk); //Is there a bookmark under R-mouse cursor?
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_BOOKMARKS_ADD, uStatus | MF_BYCOMMAND);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_BOOKMARKS_REMOVE, (fBookmarks && fHasRMBM ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_BOOKMARKS_NEXT, (fBookmarks ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_BOOKMARKS_PREV, (fBookmarks ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_BOOKMARKS_CLEARALL, (fBookmarks ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);

		//Clipboard
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLIPBOARD_COPYHEX, uStatus | MF_BYCOMMAND);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLIPBOARD_COPYHEXLE, uStatus | MF_BYCOMMAND);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLIPBOARD_COPYHEXFORMATTED, uStatus | MF_BYCOMMAND);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLIPBOARD_COPYASCII, uStatus | MF_BYCOMMAND);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLIPBOARD_COPYBASE64, uStatus | MF_BYCOMMAND);
		BOOL fClipboard = IsClipboardFormatAvailable(CF_TEXT);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLIPBOARD_PASTEHEX, ((m_fMutable && fClipboard) ?
			uStatus : MF_GRAYED) | MF_BYCOMMAND);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLIPBOARD_PASTEASCII, ((m_fMutable && fClipboard) ?
			uStatus : MF_GRAYED) | MF_BYCOMMAND);
	}
}

void CHexCtrl::OnKeyDown(UINT nChar, UINT /*nRepCnt*/, UINT /*nFlags*/)
{
	if (GetAsyncKeyState(VK_CONTROL) < 0)    //With Ctrl pressed.
		OnKeyDownCtrl(nChar);
	else if (GetAsyncKeyState(VK_SHIFT) < 0) //With Shift pressed.
		OnKeyDownShift(nChar);
	else
	{
		WPARAM wParam { };

		switch (nChar)
		{
		case VK_F2:
			wParam = IDM_HEXCTRL_BOOKMARKS_NEXT;
			break;
		case VK_F3:
			m_pDlgSearch->Search(true);
			break;
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
		if (wParam)
			SendMessageW(WM_COMMAND, wParam, 0);
	}
}

void CHexCtrl::OnKeyDownCtrl(UINT nChar)
{
	//TranslateAccelerator doesn't work within .dll without client's code modification.
	//So, this is the manual, kind of, implementation (translation).
	WPARAM wParam { };
	switch (nChar)
	{
	case 'F':
	case 'H':
		wParam = IDM_HEXCTRL_MAIN_SEARCH;
		break;
	case 'B':
		wParam = IDM_HEXCTRL_BOOKMARKS_ADD;
		break;
	case 'N':
		wParam = IDM_HEXCTRL_BOOKMARKS_REMOVE;
		break;
	case 'C':
		wParam = IDM_HEXCTRL_CLIPBOARD_COPYHEX;
		break;
	case 'V':
		wParam = IDM_HEXCTRL_CLIPBOARD_PASTEHEX;
		break;
	case 'O':
		wParam = IDM_HEXCTRL_MODIFY_OPERATIONS;
		break;
	case 'A':
		wParam = IDM_HEXCTRL_MAIN_SELECTALL;
		break;
	case 'Z':
		wParam = IDM_HEXCTRL_MAIN_UNDO;
		break;
	case 'Y':
		wParam = IDM_HEXCTRL_MAIN_REDO;
		break;
	}
	if (wParam)
		SendMessageW(WM_COMMAND, wParam, 0);
}

void CHexCtrl::OnKeyDownShift(UINT nChar)
{
	WPARAM wParam { };

	switch (nChar)
	{
	case VK_LEFT:
		OnKeyDownShiftLeft();
		break;
	case VK_RIGHT:
		OnKeyDownShiftRight();
		break;
	case VK_UP:
		OnKeyDownShiftUp();
		break;
	case VK_DOWN:
		OnKeyDownShiftDown();
		break;
	case VK_F2:
		wParam = IDM_HEXCTRL_BOOKMARKS_PREV;
		break;
	case VK_F3:
		m_pDlgSearch->Search(false);
		break;
	}
	if (wParam)
		SendMessageW(WM_COMMAND, wParam, 0);
}

void CHexCtrl::OnKeyDownShiftLeft()
{
	ULONGLONG ullClick { }, ullStart { }, ullSize = 0;
	ULONGLONG ullSelStart = m_pSelect->GetSelectionStart();
	ULONGLONG ullSelSize = m_pSelect->GetSelectionSize();

	if (m_fMutable)
	{
		if (m_ullCursorPos == m_ullSelectionClick
			|| m_ullCursorPos == ullSelStart
			|| m_ullCursorPos == m_pSelect->GetSelectionEnd())
		{
			if (ullSelStart == m_ullSelectionClick && ullSelSize > 1)
			{
				ullClick = ullStart = m_ullSelectionClick;
				ullSize = ullSelSize - 1;
			}
			else
			{
				ullClick = m_ullSelectionClick;
				ullStart = ullSelStart - 1;
				ullSize = ullSelSize + 1;
			}
		}
		else
		{
			ullClick = ullStart = m_ullCursorPos;
			ullSize = 1;
		}
	}
	else if (m_pSelect->HasSelection())
	{
		if (ullSelStart == m_ullSelectionClick && ullSelSize > 1)
		{
			ullClick = ullStart = m_ullSelectionClick;
			ullSize = ullSelSize - 1;
		}
		else
		{
			ullClick = m_ullSelectionClick;
			ullStart = ullSelStart - 1;
			ullSize = ullSelSize + 1;
		}
	}

	if (ullSize > 0)
		SetSelection(ullClick, ullStart, ullSize, 1);
}

void CHexCtrl::OnKeyDownShiftRight()
{
	ULONGLONG ullClick { }, ullStart { }, ullSize = 0;
	ULONGLONG ullSelStart = m_pSelect->GetSelectionStart();
	ULONGLONG ullSelSize = m_pSelect->GetSelectionSize();

	if (m_fMutable)
	{
		if (m_ullCursorPos == m_ullSelectionClick || m_ullCursorPos == ullSelStart
			|| m_ullCursorPos == m_pSelect->GetSelectionEnd())
		{
			if (ullSelStart == m_ullSelectionClick)
			{
				ullClick = ullStart = m_ullSelectionClick;
				ullSize = ullSelSize + 1;
			}
			else
			{
				ullClick = m_ullSelectionClick;
				ullStart = ullSelStart + 1;
				ullSize = ullSelSize - 1;
			}
		}
		else
		{
			ullClick = ullStart = m_ullCursorPos;
			ullSize = 1;
		}
	}
	else if (m_pSelect->HasSelection())
	{
		if (ullSelStart == m_ullSelectionClick)
		{
			ullClick = ullStart = m_ullSelectionClick;
			ullSize = ullSelSize + 1;
		}
		else
		{
			ullClick = m_ullSelectionClick;
			ullStart = ullSelStart + 1;
			ullSize = ullSelSize - 1;
		}
	}

	if (ullSize > 0)
		SetSelection(ullClick, ullStart, ullSize, 1);
}

void CHexCtrl::OnKeyDownShiftUp()
{
	ULONGLONG ullClick { }, ullStart { }, ullSize = 0;
	ULONGLONG ullSelStart = m_pSelect->GetSelectionStart();
	ULONGLONG ullSelSize = m_pSelect->GetSelectionSize();

	if (m_fMutable)
	{
		if (m_ullCursorPos == m_ullSelectionClick || m_ullCursorPos == ullSelStart
			|| m_ullCursorPos == m_pSelect->GetSelectionEnd())
		{
			if (ullSelStart == 0)
				return;

			if (ullSelStart < m_ullSelectionClick)
			{
				ullClick = m_ullSelectionClick;
				if (ullSelStart < m_dwCapacity)
				{
					ullStart = 0;
					ullSize = ullSelSize + ullSelStart;
				}
				else
				{
					ullStart = ullSelStart - m_dwCapacity;
					ullSize = ullSelSize + m_dwCapacity;
				}
			}
			else
			{
				ullClick = m_ullSelectionClick;
				ullStart = ullSelSize >= m_dwCapacity ? m_ullSelectionClick : m_ullSelectionClick - m_dwCapacity;
				ullSize = ullSelSize >= m_dwCapacity ? ullSelSize - m_dwCapacity : m_dwCapacity + 1;
				ullSize = ullSize ? ullSize : 1;
			}
		}
		else
		{
			ullClick = ullStart = m_ullCursorPos;
			ullSize = 1;
		}
	}
	else if (m_pSelect->HasSelection())
	{
		if (ullSelStart == 0)
			return;

		if (ullSelStart < m_ullSelectionClick)
		{
			ullClick = m_ullSelectionClick;
			if (ullSelStart < m_dwCapacity)
			{
				ullStart = 0;
				ullSize = ullSelSize + ullSelStart;
			}
			else
			{
				ullStart = ullSelStart - m_dwCapacity;
				ullSize = ullSelSize + m_dwCapacity;
			}
		}
		else
		{
			ullClick = m_ullSelectionClick;
			ullStart = (ullSelSize >= m_dwCapacity) ? m_ullSelectionClick : m_ullSelectionClick - m_dwCapacity;
			ullSize = ullSelSize >= m_dwCapacity ? ullSelSize - m_dwCapacity : (ULONGLONG)m_dwCapacity + 1;
			ullSize = ullSize ? ullSize : 1;
		}
	}

	if (ullSize > 0)
		SetSelection(ullClick, ullStart, ullSize, 1);
}

void CHexCtrl::OnKeyDownShiftDown()
{
	ULONGLONG ullClick { }, ullStart { }, ullSize = 0;
	ULONGLONG ullSelStart = m_pSelect->GetSelectionStart();
	ULONGLONG ullSelSize = m_pSelect->GetSelectionSize();
	if (m_fMutable)
	{
		if (m_ullCursorPos == m_ullSelectionClick
			|| m_ullCursorPos == ullSelStart
			|| m_ullCursorPos == m_pSelect->GetSelectionEnd())
		{
			if (ullSelStart == m_ullSelectionClick)
			{
				ullClick = ullStart = m_ullSelectionClick;
				ullSize = ullSelSize + m_dwCapacity;
			}
			else if (ullSelStart < m_ullSelectionClick)
			{
				ullClick = m_ullSelectionClick;
				ullStart = ullSelSize > m_dwCapacity ? ullSelStart + m_dwCapacity : m_ullSelectionClick;
				ullSize = ullSelSize >= m_dwCapacity ? ullSelSize - m_dwCapacity : m_dwCapacity;
				ullSize = ullSize ? ullSize : 1;
			}
		}
		else
		{
			ullClick = ullStart = m_ullCursorPos;
			ullSize = 1;
		}
	}
	else if (m_pSelect->HasSelection())
	{
		if (ullSelStart == m_ullSelectionClick)
		{
			ullClick = ullStart = m_ullSelectionClick;
			ullSize = ullSelSize + m_dwCapacity;
		}
		else if (ullSelStart < m_ullSelectionClick)
		{
			ullClick = m_ullSelectionClick;
			ullStart = ullSelSize > m_dwCapacity ? ullSelStart + m_dwCapacity : m_ullSelectionClick;
			ullSize = ullSelSize >= m_dwCapacity ? ullSelSize - m_dwCapacity : m_dwCapacity;
			ullSize = ullSize ? ullSize : 1;
		}
	}

	if (ullSize > 0)
		SetSelection(ullClick, ullStart, ullSize, 1);
}

void CHexCtrl::OnChar(UINT nChar, UINT /*nRepCnt*/, UINT /*nFlags*/)
{
	if (!m_fMutable || (GetKeyState(VK_CONTROL) < 0))
		return;

	HEXMODIFYSTRUCT hms;
	hms.vecSpan.emplace_back(HEXSPANSTRUCT { m_ullCursorPos, 1 });
	hms.ullDataSize = 1;

	BYTE chByte = nChar & 0xFF;
	if (!IsCurTextArea()) //If cursor is not in Ascii area - just one part (High/Low) of byte must be changed.
	{
		if (chByte >= 0x30 && chByte <= 0x39)      //Digits.
			chByte -= 0x30;
		else if (chByte >= 0x41 && chByte <= 0x46) //Hex letters uppercase.
			chByte -= 0x37;
		else if (chByte >= 0x61 && chByte <= 0x66) //Hex letters lowercase.
			chByte -= 0x57;
		else
			return;

		BYTE chByteCurr = GetByte(m_ullCursorPos);
		if (m_fCursorHigh)
			chByte = (chByte << 4) | (chByteCurr & 0x0F);
		else
			chByte = (chByte & 0x0F) | (chByteCurr & 0xF0);
	}

	hms.pData = &chByte;
	ModifyData(hms);
	CursorMoveRight();
}

UINT CHexCtrl::OnGetDlgCode()
{
	return DLGC_WANTALLKEYS;
}

void CHexCtrl::OnVScroll(UINT /*nSBCode*/, UINT /*nPos*/, CScrollBar* /*pScrollBar*/)
{
	if (m_pScrollV->GetScrollPosDelta() != 0)
		RedrawWindow();
}

void CHexCtrl::OnHScroll(UINT /*nSBCode*/, UINT /*nPos*/, CScrollBar* /*pScrollBar*/)
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

	const auto iSecondHorizLine = m_iStartWorkAreaY - 1;
	const auto iThirdHorizLine = rcClient.Height() - m_iHeightBottomOffArea;
	const auto iFourthHorizLine = iThirdHorizLine + m_iHeightBottomRect;

	pDC->SelectObject(m_penLines);

	//First horizontal line.
	pDC->MoveTo(m_iFirstVertLine - iScrollH, m_iFirstHorizLine);
	pDC->LineTo(m_iFourthVertLine, m_iFirstHorizLine);

	//Second horizontal line.
	pDC->MoveTo(m_iFirstVertLine - iScrollH, iSecondHorizLine);
	pDC->LineTo(m_iFourthVertLine, iSecondHorizLine);

	//Third horizontal line.
	pDC->MoveTo(m_iFirstVertLine - iScrollH, iThirdHorizLine);
	pDC->LineTo(m_iFourthVertLine, iThirdHorizLine);

	//Fourth horizontal line.
	pDC->MoveTo(m_iFirstVertLine - iScrollH, iFourthHorizLine);
	pDC->LineTo(m_iFourthVertLine, iFourthHorizLine);

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

	//Bottom "Info" rect.
	rc.left = m_iFirstVertLine + 1 - iScrollH;
	rc.top = iThirdHorizLine + 1;
	rc.right = m_iFourthVertLine;
	rc.bottom = iFourthHorizLine;	//Fill bottom rect until iFourthHorizLine.
	pDC->FillSolidRect(&rc, m_stColor.clrBkInfoRect);
	rc.left = m_iFirstVertLine + 5; //Draw text beginning with little indent.
	rc.right = rcClient.right;		//Draw text to the end of the client area, even if it pass iFourthHorizLine.
	pDC->SelectObject(m_fontBottomRect);
	pDC->SetTextColor(m_stColor.clrTextInfoRect);
	pDC->SetBkColor(m_stColor.clrBkInfoRect);
	pDC->DrawTextW(m_wstrBottomText.data(), (int)m_wstrBottomText.size(), &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

	//«Offset» text.
	rc.left = m_iFirstVertLine - iScrollH;
	rc.top = m_iFirstHorizLine;
	rc.right = m_iSecondVertLine - iScrollH;
	rc.bottom = iSecondHorizLine;
	pDC->SelectObject(m_fontHexView);
	pDC->SetTextColor(m_stColor.clrTextCaption);
	pDC->SetBkColor(m_stColor.clrBk);
	pDC->DrawTextW(L"Offset", 6, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	//Capacity numbers.
	ExtTextOutW(pDC->m_hDC, m_iIndentFirstHexChunk - iScrollH, m_iFirstHorizLine + m_iIndentTextCapacityY, NULL, nullptr,
		m_wstrCapacity.data(), (UINT)m_wstrCapacity.size(), nullptr);

	//"Ascii" text.
	rc.left = m_iThirdVertLine - iScrollH;
	rc.top = m_iFirstHorizLine;
	rc.right = m_iFourthVertLine - iScrollH;
	rc.bottom = iSecondHorizLine;
	pDC->DrawTextW(L"Ascii", 5, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	int iLine = 0; //Current line to print.
	std::vector<POLYTEXTW> vecPolyHex, vecPolyAscii, vecPolyBookmarkHex, vecPolyBookmarkAscii,
		vecPolySelHex, vecPolySelAscii, vecPolyCursor;
	std::list<std::wstring> listWstrHex, listWstrAscii, listWstrBookmarkHex, listWstrBookmarkAscii,
		listWstrSelHex, listWstrSelAscii, listWstrCursor;
	COLORREF clrBkCursor { }; //Cursor color.

	//Loop for printing Hex chunks and Ascii chars line by line.
	for (ULONGLONG iterLines = ullLineStart; iterLines < ullLineEnd; iterLines++, iLine++)
	{
		//Drawing offset with bk color depending on selection range.
		COLORREF clrTextOffset, clrBkOffset;
		if (m_pSelect->HasSelection()
			&& (iterLines * m_dwCapacity + m_dwCapacity) > m_pSelect->GetSelectionStart()
			&& (iterLines * m_dwCapacity) < m_pSelect->GetSelectionEnd())
		{
			clrTextOffset = m_stColor.clrTextSelected;
			clrBkOffset = m_stColor.clrBkSelected;
		}
		else
		{
			clrTextOffset = m_stColor.clrTextCaption;
			clrBkOffset = m_stColor.clrBk;
		}

		//Left column offset printing (00000001...0000FFFF).
		wchar_t pwszOffset[16];
		UllToWchars(iterLines * m_dwCapacity, pwszOffset, (size_t)m_dwOffsetDigits / 2);
		pDC->SetTextColor(clrTextOffset);
		pDC->SetBkColor(clrBkOffset);
		ExtTextOutW(pDC->m_hDC, m_iFirstVertLine + m_sizeLetter.cx - iScrollH, m_iStartWorkAreaY + (m_sizeLetter.cy * iLine),
			NULL, nullptr, pwszOffset, m_dwOffsetDigits, nullptr);

		//Hex, Ascii and Cursor strings to print.
		std::wstring wstrHexToPrint { }, wstrAsciiToPrint { }, wstrHexCursorToPrint { }, wstrAsciiCursorToPrint { };
		std::wstring wstrHexBookmarkToPrint { }, wstrAsciiBookmarkToPrint { }; //Bookmarks to print.
		std::wstring wstrHexSelToPrint { }, wstrAsciiSelToPrint { };           //Selected Hex and Ascii strings to print.

		//Selection Hex and Ascii X coords. 0x7FFFFFFF is important.
		int iSelHexPosToPrintX = 0x7FFFFFFF, iSelAsciiPosToPrintX { };
		int iBookmarkHexPosToPrintX = 0x7FFFFFFF, iBookmarkAsciiPosToPrintX { };
		int iCursorHexPosToPrintX { }, iCursorAsciiPosToPrintX { }; //Cursor X coords.
		bool fBookmark { false };  //Flag to show current Bookmark in current Hex presence.
		bool fSelection { false }; //Same as above but for selection.

		const auto iHexPosToPrintX = m_iIndentFirstHexChunk - iScrollH;
		const auto iAsciiPosToPrintX = m_iIndentAscii - iScrollH;
		const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeLetter.cy * iLine; //Hex and Ascii the same.

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
			if (m_enShowMode == EHexShowMode::ASBYTE && iterChunks == (m_dwCapacityBlockSize - 1))
				wstrHexToPrint += L"  ";

			//Ascii to print.
			wchar_t wchAscii = chByteToPrint;
			if (wchAscii < 32 || wchAscii >= 0x7f) //For non printable Ascii just print a dot.
				wchAscii = '.';
			wstrAsciiToPrint += wchAscii;

			//If there is a Bookmark.
			if (m_pBookmarks->HitTest(ullIndexByteToPrint))
			{
				if (iBookmarkHexPosToPrintX == 0x7FFFFFFF) //For just one time exec.
				{
					int iCy;
					HexChunkPoint(ullIndexByteToPrint, iBookmarkHexPosToPrintX, iCy);
					AsciiChunkPoint(ullIndexByteToPrint, iBookmarkAsciiPosToPrintX, iCy);
				}

				if (!wstrHexBookmarkToPrint.empty()) //Only adding spaces if there are chars beforehead.
				{
					if ((iterChunks % (DWORD)m_enShowMode) == 0)
						wstrHexBookmarkToPrint += L" ";

					//Additional space between capacity halves, only with ASBYTE representation.
					if (m_enShowMode == EHexShowMode::ASBYTE && iterChunks == m_dwCapacityBlockSize)
						wstrHexBookmarkToPrint += L"  ";
				}
				wstrHexBookmarkToPrint += pwszHexToPrint[0];
				wstrHexBookmarkToPrint += pwszHexToPrint[1];
				wstrAsciiBookmarkToPrint += wchAscii;
				fBookmark = true;
			}
			else if (fBookmark)
			{
				//There can be multiple bookmarks in one line. 
				//So, if there already were bookmarked Hexes in the current line we Poly them.
				//Same Poly mechanism presents at the end of the current (line) loop,
				//to Poly bookmarks that end at the line's end.

				//Bookmarks Poly.
				if (!wstrHexBookmarkToPrint.empty())
				{
					//Hex bookmarks Poly.
					listWstrBookmarkHex.emplace_back(std::move(wstrHexBookmarkToPrint));
					wstrHexBookmarkToPrint.clear();
					vecPolyBookmarkHex.emplace_back(POLYTEXTW { iBookmarkHexPosToPrintX, iPosToPrintY,
						(UINT)listWstrBookmarkHex.back().size(), listWstrBookmarkHex.back().data(), 0, { }, nullptr });

					//Ascii bookmarks Poly.
					listWstrBookmarkAscii.emplace_back(std::move(wstrAsciiBookmarkToPrint));
					wstrAsciiBookmarkToPrint.clear();
					vecPolyBookmarkAscii.emplace_back(POLYTEXTW { iBookmarkAsciiPosToPrintX, iPosToPrintY,
						(UINT)listWstrBookmarkAscii.back().size(), listWstrBookmarkAscii.back().data(), 0, { }, nullptr });
				}
				iBookmarkHexPosToPrintX = 0x7FFFFFFF;
				fBookmark = false;
			}

			//If there is a selection.
			if (m_pSelect->HitTest(ullIndexByteToPrint))
			{
				if (iSelHexPosToPrintX == 0x7FFFFFFF) //For just one time exec.
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
					if (m_enShowMode == EHexShowMode::ASBYTE && iterChunks == m_dwCapacityBlockSize)
						wstrHexSelToPrint += L"  ";
				}
				wstrHexSelToPrint += pwszHexToPrint[0];
				wstrHexSelToPrint += pwszHexToPrint[1];
				wstrAsciiSelToPrint += wchAscii;
				fSelection = true;
			}
			else if (fSelection)
			{
				//There can be multiple selections in one line. 
				//All the same as for bookmarks.

				//Selection Poly.
				if (!wstrHexSelToPrint.empty())
				{
					//Hex selection Poly.
					listWstrSelHex.emplace_back(std::move(wstrHexSelToPrint));
					wstrHexSelToPrint.clear();
					vecPolySelHex.emplace_back(POLYTEXTW { iSelHexPosToPrintX, iPosToPrintY,
						(UINT)listWstrSelHex.back().size(), listWstrSelHex.back().data(), 0, { }, nullptr });

					//Ascii selection Poly.
					listWstrSelAscii.emplace_back(std::move(wstrAsciiSelToPrint));
					wstrAsciiSelToPrint.clear();
					vecPolySelAscii.emplace_back(POLYTEXTW { iSelAsciiPosToPrintX, iPosToPrintY,
						(UINT)listWstrSelAscii.back().size(), listWstrSelAscii.back().data(), 0, { }, nullptr });
				}
				iSelHexPosToPrintX = 0x7FFFFFFF;
				fSelection = false;
			}

			//Cursor position. 
			if (m_fMutable && (ullIndexByteToPrint == m_ullCursorPos))
			{
				int iCy;
				HexChunkPoint(m_ullCursorPos, iCursorHexPosToPrintX, iCy);
				AsciiChunkPoint(m_ullCursorPos, iCursorAsciiPosToPrintX, iCy);
				if (m_fCursorHigh)
					wstrHexCursorToPrint = g_pwszHexMap[(chByteToPrint & 0xF0) >> 4];
				else
				{
					wstrHexCursorToPrint = g_pwszHexMap[(chByteToPrint & 0x0F)];
					iCursorHexPosToPrintX += m_sizeLetter.cx;
				}
				wstrAsciiCursorToPrint = wchAscii;

				if (m_pSelect->HitTest(ullIndexByteToPrint))
					clrBkCursor = m_stColor.clrBkCursorSelected;
				else
					clrBkCursor = m_stColor.clrBkCursor;
			}
		}

		//Hex Poly.
		listWstrHex.emplace_back(std::move(wstrHexToPrint));
		vecPolyHex.emplace_back(POLYTEXTW { iHexPosToPrintX, iPosToPrintY,
			(UINT)listWstrHex.back().size(), listWstrHex.back().data(), 0, { }, nullptr });

		//Ascii Poly.
		listWstrAscii.emplace_back(std::move(wstrAsciiToPrint));
		vecPolyAscii.emplace_back(POLYTEXTW { iAsciiPosToPrintX, iPosToPrintY,
			(UINT)listWstrAscii.back().size(), listWstrAscii.back().data(), 0, { }, nullptr });

		//Bookmarks Poly.
		if (!wstrHexBookmarkToPrint.empty())
		{
			//Hex bookmarks Poly.
			listWstrBookmarkHex.emplace_back(std::move(wstrHexBookmarkToPrint));
			vecPolyBookmarkHex.emplace_back(POLYTEXTW { iBookmarkHexPosToPrintX, iPosToPrintY,
				(UINT)listWstrBookmarkHex.back().size(), listWstrBookmarkHex.back().data(), 0, { }, nullptr });

			//Ascii bookmarks Poly.
			listWstrBookmarkAscii.emplace_back(std::move(wstrAsciiBookmarkToPrint));
			vecPolyBookmarkAscii.emplace_back(POLYTEXTW { iBookmarkAsciiPosToPrintX, iPosToPrintY,
				(UINT)listWstrBookmarkAscii.back().size(), listWstrBookmarkAscii.back().data(), 0, { }, nullptr });
		}

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
	pDC->SetTextColor(m_stColor.clrTextHex);
	pDC->SetBkColor(m_stColor.clrBk);
	PolyTextOutW(pDC->m_hDC, vecPolyHex.data(), (UINT)vecPolyHex.size());

	//Ascii printing.
	pDC->SetTextColor(m_stColor.clrTextAscii);
	PolyTextOutW(pDC->m_hDC, vecPolyAscii.data(), (UINT)vecPolyAscii.size());

	//Hex bookmark printing.
	if (!vecPolyBookmarkHex.empty())
	{
		pDC->SetTextColor(m_stColor.clrTextBookmark);
		pDC->SetBkColor(m_stColor.clrBkBookmark);
		PolyTextOutW(pDC->m_hDC, vecPolyBookmarkHex.data(), (UINT)vecPolyBookmarkHex.size());

		//Ascii bookmark printing.
		PolyTextOutW(pDC->m_hDC, vecPolyBookmarkAscii.data(), (UINT)vecPolyBookmarkAscii.size());
	}

	//Hex selected printing.
	if (!vecPolySelHex.empty())
	{
		pDC->SetTextColor(m_stColor.clrTextSelected);
		pDC->SetBkColor(m_stColor.clrBkSelected);
		PolyTextOutW(pDC->m_hDC, vecPolySelHex.data(), (UINT)vecPolySelHex.size());

		//Ascii selected printing.
		PolyTextOutW(pDC->m_hDC, vecPolySelAscii.data(), (UINT)vecPolySelAscii.size());
	}

	//Cursor printing.
	if (!vecPolyCursor.empty())
	{
		pDC->SetTextColor(m_stColor.clrTextCursor);
		pDC->SetBkColor(clrBkCursor);
		PolyTextOutW(pDC->m_hDC, vecPolyCursor.data(), (UINT)vecPolyCursor.size());
	}
}

void CHexCtrl::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	RecalcWorkAreaHeight(cy);
	ULONGLONG ullPageSize = ULONGLONG(m_iHeightWorkArea * m_dbWheelRatio);
	if (ullPageSize < m_sizeLetter.cy)
		ullPageSize = m_sizeLetter.cy;
	m_pScrollV->SetScrollPageSize(ullPageSize);
}

BOOL CHexCtrl::OnEraseBkgnd(CDC* /*pDC*/)
{
	return FALSE;
}

BOOL CHexCtrl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
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

void CHexCtrl::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
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
	if (m_enCreateMode == EHexDataMode::DATA_MSG)
		if (GetMsgWindow() != GetParent()->GetSafeHwnd()) //To avoid sending notify message twice to the same window.
			MsgWindowNotify(HEXCTRL_MSG_DESTROY);

	ParentNotify(HEXCTRL_MSG_DESTROY);
	CWnd* pwndParent = GetParent();
	if (pwndParent)
		pwndParent->SetForegroundWindow();

	ClearData();
	m_fCreated = false;

	CWnd::OnDestroy();
}

PBYTE CHexCtrl::GetData(ULONGLONG* pUllSize)
{
	if (pUllSize) //Optionally returns data size.
		*pUllSize = m_ullDataSize;

	return m_pData;
}

BYTE CHexCtrl::GetByte(ULONGLONG ullIndex)const
{
	if (ullIndex >= m_ullDataSize)
		return 0x00;

	//If it's virtual data control we aquire next byte_to_print from m_hwndMsg window.
	switch (m_enCreateMode)
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
		BYTE byte;
		if (hns.pData)
			byte = *hns.pData;
		else
			byte = 0x00;
		return byte;
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

WORD CHexCtrl::GetWord(ULONGLONG ullIndex) const
{
	//Data overflow check.
	if ((ullIndex + sizeof(WORD)) > m_ullDataSize)
		return 0;

	WORD wData;
	if (m_enCreateMode == EHexDataMode::DATA_MEMORY)
		wData = *(PWORD)((DWORD_PTR)m_pData + ullIndex);
	else
		wData = ((WORD)GetByte(ullIndex)) | ((WORD)GetByte(ullIndex + 1) << 8);

	return wData;
}

DWORD CHexCtrl::GetDword(ULONGLONG ullIndex) const
{
	//Data overflow check.
	if ((ullIndex + sizeof(DWORD)) > m_ullDataSize)
		return 0;

	DWORD dwData;
	if (m_enCreateMode == EHexDataMode::DATA_MEMORY)
		dwData = *(PDWORD)((DWORD_PTR)m_pData + ullIndex);
	else
		dwData = ((DWORD)GetByte(ullIndex)) | ((DWORD)GetByte(ullIndex + 1) << 8)
		| ((DWORD)GetByte(ullIndex + 2) << 16) | ((DWORD)GetByte(ullIndex + 3) << 24);

	return dwData;
}

QWORD CHexCtrl::GetQword(ULONGLONG ullIndex) const
{
	//Data overflow check.
	if ((ullIndex + sizeof(QWORD)) > m_ullDataSize)
		return 0;

	QWORD ullData;
	if (m_enCreateMode == EHexDataMode::DATA_MEMORY)
		ullData = *(PQWORD)((DWORD_PTR)m_pData + ullIndex);
	else
		ullData = ((QWORD)GetByte(ullIndex)) | ((QWORD)GetByte(ullIndex + 1) << 8)
		| ((QWORD)GetByte(ullIndex + 2) << 16) | ((QWORD)GetByte(ullIndex + 3) << 24)
		| ((QWORD)GetByte(ullIndex + 4) << 32) | ((QWORD)GetByte(ullIndex + 5) << 40)
		| ((QWORD)GetByte(ullIndex + 6) << 48) | ((QWORD)GetByte(ullIndex + 7) << 56);

	return ullData;
}

bool CHexCtrl::SetByte(ULONGLONG ullIndex, BYTE bData)
{
	//Data overflow check.
	if (ullIndex >= m_ullDataSize || m_enCreateMode != EHexDataMode::DATA_MEMORY)
		return false;

	m_pData[ullIndex] = bData;

	return true;
}

bool CHexCtrl::SetWord(ULONGLONG ullIndex, WORD wData)
{
	//Data overflow check.
	if ((ullIndex + sizeof(WORD)) > m_ullDataSize || m_enCreateMode != EHexDataMode::DATA_MEMORY)
		return false;

	*(PWORD)((DWORD_PTR)m_pData + ullIndex) = wData;

	return true;
}

bool CHexCtrl::SetDword(ULONGLONG ullIndex, DWORD dwData)
{
	//Data overflow check.
	if ((ullIndex + sizeof(DWORD)) > m_ullDataSize || m_enCreateMode != EHexDataMode::DATA_MEMORY)
		return false;

	*(PDWORD)((DWORD_PTR)m_pData + ullIndex) = dwData;

	return true;
}

bool CHexCtrl::SetQword(ULONGLONG ullIndex, QWORD qwData)
{
	//Data overflow check.
	if ((ullIndex + sizeof(QWORD)) > m_ullDataSize || m_enCreateMode != EHexDataMode::DATA_MEMORY)
		return false;

	*(PQWORD)((DWORD_PTR)m_pData + ullIndex) = qwData;

	return true;
}

void CHexCtrl::ModifyData(const HEXMODIFYSTRUCT& hms, bool fRedraw)
{
	if (!m_fMutable)
		return;

	const auto& vecRef = hms.vecSpan;
	const auto& vecRefB = vecRef.back();
	if (vecRefB.ullOffset >= m_ullDataSize || (vecRefB.ullOffset + vecRefB.ullSize) > m_ullDataSize
		|| (vecRefB.ullOffset + hms.ullDataSize) > m_ullDataSize)
		return;

	m_deqRedo.clear(); //No Redo unless we make Undo.
	SnapshotUndo(vecRef);

	ULONGLONG ullTotalSize { };
	for (auto& i : vecRef)
		ullTotalSize += i.ullSize;

	switch (m_enCreateMode)
	{
	case EHexDataMode::DATA_MEMORY: //Modify only in non Virtual mode.
	{
		PBYTE pData = GetData();
		switch (hms.enCreateMode)
		{
		case EHexModifyMode::MODIFY_DEFAULT:
		{
			for (ULONGLONG i = 0; i < hms.ullDataSize; i++)
				pData[vecRefB.ullOffset + i] = hms.pData[i];
		}
		break;
		case EHexModifyMode::MODIFY_REPEAT:
		{
			//Fill hms.ullSize bytes with hms.ullDataSize bytes hms.ullSize/hms.ullDataSize times.
			ULONGLONG ullChunks = (ullTotalSize >= hms.ullDataSize) ? ullTotalSize / hms.ullDataSize : 0;
			ULONGLONG ullVecIndex { 0 };
			ULONGLONG ullTotalIndex { };

			for (ULONGLONG iterChunk = 0; iterChunk < ullChunks; iterChunk++)
				for (ULONGLONG iterData = 0; iterData < hms.ullDataSize; iterData++)
				{
					if (ullTotalIndex >= vecRef.at((size_t)ullVecIndex).ullSize)
					{
						++ullVecIndex;
						ullTotalIndex = 0;
					}
					pData[vecRef.at((size_t)ullVecIndex).ullOffset + ullTotalIndex] = hms.pData[iterData];
					++ullTotalIndex;
				}
		}
		break;
		case EHexModifyMode::MODIFY_OPERATION:
		{
			if (hms.ullDataSize > sizeof(QWORD))
				return;

			ULONGLONG ullChunks = vecRef.at(0).ullSize / hms.ullDataSize;
			ULONGLONG ullDataOper { };

			if (hms.pData) //hms.pData might be null for, say, EHexOperMode::OPER_NOT.
			{
				switch (hms.ullDataSize)
				{
				case (sizeof(BYTE)):
					ullDataOper = *(PBYTE)hms.pData;
					break;
				case (sizeof(WORD)):
					ullDataOper = *(PWORD)hms.pData;
					break;
				case (sizeof(DWORD)):
					ullDataOper = *(PDWORD)hms.pData;
					break;
				case (sizeof(QWORD)):
					ullDataOper = *(PQWORD)hms.pData;
					break;
				}
			}

			//hms.vecSpan.size() times do following operations.
			for (auto& ref : vecRef)
			{
				for (ULONGLONG i = 0; i < ullChunks; i++)
				{
					QWORD ullData { };
					switch (hms.ullDataSize)
					{
					case (sizeof(BYTE)):
						ullData = GetByte(ref.ullOffset + i);
						break;
					case (sizeof(WORD)):
						ullData = GetWord(ref.ullOffset + (i * hms.ullDataSize));
						break;
					case (sizeof(DWORD)):
						ullData = GetDword(ref.ullOffset + (i * hms.ullDataSize));
						break;
					case (sizeof(QWORD)):
						ullData = GetQword(ref.ullOffset + (i * hms.ullDataSize));
						break;
					};

					switch (hms.enOperMode)
					{
					case EHexOperMode::OPER_OR:
						ullData |= ullDataOper;
						break;
					case EHexOperMode::OPER_XOR:
						ullData ^= ullDataOper;
						break;
					case EHexOperMode::OPER_AND:
						ullData &= ullDataOper;
						break;
					case EHexOperMode::OPER_NOT:
						ullData = ~ullData;
						break;
					case EHexOperMode::OPER_SHL:
						ullData <<= ullDataOper;
						break;
					case EHexOperMode::OPER_SHR:
						ullData >>= ullDataOper;
						break;
					case EHexOperMode::OPER_ADD:
						ullData += ullDataOper;
						break;
					case EHexOperMode::OPER_SUBTRACT:
						ullData -= ullDataOper;
						break;
					case EHexOperMode::OPER_MULTIPLY:
						ullData *= ullDataOper;
						break;
					case EHexOperMode::OPER_DIVIDE:
						if (ullDataOper) //Division by Zero check.
							ullData /= ullDataOper;
						break;
					}

					switch (hms.ullDataSize)
					{
					case (sizeof(BYTE)):
						SetByte(ref.ullOffset + i, BYTE(ullData & 0xFF));
						break;
					case (sizeof(WORD)):
						SetWord(ref.ullOffset + (i * sizeof(WORD)), WORD(ullData & 0xFFFF));
						break;
					case (sizeof(DWORD)):
						SetDword(ref.ullOffset + (i * sizeof(DWORD)), DWORD(ullData & 0xFFFFFFFF));
						break;
					case (sizeof(QWORD)):
						SetQword(ref.ullOffset + (i * sizeof(QWORD)), ullData);
						break;
					};
				}
			}
		}
		break;
		}

		HEXNOTIFYSTRUCT hns { { m_hWnd, (UINT)GetDlgCtrlID(), HEXCTRL_MSG_MODIFYDATA } };
		hns.pData = (PBYTE)&hms;
		ParentNotify(hns);
	}
	break;
	case EHexDataMode::DATA_MSG:
	{
		//In EHexDataMode::DATA_MSG mode we send pointer to HEXMODIFYSTRUCT to Message window.
		HEXNOTIFYSTRUCT hns { { m_hWnd, (UINT)GetDlgCtrlID(), HEXCTRL_MSG_MODIFYDATA } };
		hns.pData = (PBYTE)&hms;
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
	m_iSpaceBetweenBlocks = (m_enShowMode == EHexShowMode::ASBYTE && m_dwCapacity > 1) ? m_sizeLetter.cx * 2 : 0;
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
	m_pScrollV->SetScrollSizes(m_sizeLetter.cy, ULONGLONG(m_iHeightWorkArea * m_dbWheelRatio),
		(ULONGLONG)m_iStartWorkAreaY + m_iHeightBottomOffArea + m_sizeLetter.cy * (m_ullDataSize / m_dwCapacity + 2));
	m_pScrollH->SetScrollSizes(m_sizeLetter.cx, iClientWidth, (ULONGLONG)m_iFourthVertLine + 1);
}

ULONGLONG CHexCtrl::GetTopLine()const
{
	return m_pScrollV->GetScrollPos() / m_sizeLetter.cy;
}

ULONGLONG CHexCtrl::HitTest(const POINT* pPoint)
{
	int iY = pPoint->y;
	int iX = pPoint->x + (int)m_pScrollH->GetScrollPos(); //To compensate horizontal scroll.
	ULONGLONG ullCurLine = GetTopLine();
	ULONGLONG ullHexChunk;

	//Checking if cursor is within hex chunks area.
	if ((iX >= m_iIndentFirstHexChunk) && (iX < m_iThirdVertLine) && (iY >= m_iStartWorkAreaY) && (iY <= m_iEndWorkArea))
	{
		//Additional space between halves. Only in BYTE's view mode.
		int iBetweenBlocks;
		if (m_enShowMode == EHexShowMode::ASBYTE && iX > m_iSizeFirstHalf)
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
		ullHexChunk = (ULONGLONG)dwChunkX + ((iY - m_iStartWorkAreaY) / m_sizeLetter.cy) *
			m_dwCapacity + (ullCurLine * m_dwCapacity);
		m_fCursorTextArea = false;
	}
	//Or within Ascii area.
	else if ((iX >= m_iIndentAscii) && (iX < (m_iIndentAscii + m_iSpaceBetweenAscii * (int)m_dwCapacity))
		&& (iY >= m_iStartWorkAreaY) && iY <= m_iEndWorkArea)
	{
		//Calculate ullHit Ascii symbol.
		ullHexChunk = ((iX - (ULONGLONG)m_iIndentAscii) / m_iSpaceBetweenAscii) +
			((iY - m_iStartWorkAreaY) / m_sizeLetter.cy) * m_dwCapacity + (ullCurLine * m_dwCapacity);
		m_fCursorTextArea = true;
	}
	else
		ullHexChunk = 0xFFFFFFFFFFFFFFFFull;

	//If cursor is out of end-bound of hex chunks or Ascii chars.
	if (ullHexChunk >= m_ullDataSize)
		ullHexChunk = 0xFFFFFFFFFFFFFFFFull;

	return ullHexChunk;
}

void CHexCtrl::HexChunkPoint(ULONGLONG ullChunk, int& iCx, int& iCy)const
{	//This func computes x and y pos of given Hex chunk.
	DWORD dwMod = ullChunk % m_dwCapacity;
	int iBetweenBlocks { 0 };
	if (dwMod >= m_dwCapacityBlockSize)
		iBetweenBlocks = m_iSpaceBetweenBlocks;

	iCx = int(((m_iIndentFirstHexChunk + iBetweenBlocks + dwMod * m_iSizeHexByte) +
		(dwMod / (DWORD)m_enShowMode) * m_iSpaceBetweenHexChunks) - m_pScrollH->GetScrollPos());

	iCy = int((m_iStartWorkAreaY + (ullChunk / m_dwCapacity) * m_sizeLetter.cy) - m_pScrollV->GetScrollPos());
	if (dwMod)
		iCy += m_sizeLetter.cy;
}

void CHexCtrl::AsciiChunkPoint(ULONGLONG ullChunk, int& iCx, int& iCy) const
{	//This func computes x and y pos of given Ascii chunk.
	DWORD dwMod = ullChunk % m_dwCapacity;
	iCx = int((m_iIndentAscii + dwMod * m_sizeLetter.cx) - m_pScrollH->GetScrollPos());

	iCy = int(((ullChunk / m_dwCapacity) * m_sizeLetter.cy) - m_pScrollV->GetScrollPos());
	if (dwMod)
		iCy += m_sizeLetter.cy;
}

void CHexCtrl::ClipboardCopy(EClipboard enType)
{
	if (!m_pSelect->HasSelection() || m_pSelect->GetSelectionSize() > 0x6400000) //>~100MB
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
		strToClipboard.reserve((size_t)m_pSelect->GetSelectionSize() * 2);
		for (unsigned i = 0; i < m_pSelect->GetSelectionSize(); ++i)
		{
			BYTE chByte = GetByte(m_pSelect->GetNextOffset(i));
			strToClipboard += pszHexMap[(chByte & 0xF0) >> 4];
			strToClipboard += pszHexMap[(chByte & 0x0F)];
		}
	}
	break;
	case EClipboard::COPY_HEX_LE:
	{
		strToClipboard.reserve((size_t)m_pSelect->GetSelectionSize() * 2);
		for (int i = (int)m_pSelect->GetSelectionSize(); i > 0; --i)
		{
			BYTE chByte = GetByte(m_pSelect->GetNextOffset(i - 1));
			strToClipboard += pszHexMap[(chByte & 0xF0) >> 4];
			strToClipboard += pszHexMap[(chByte & 0x0F)];
		}
	}
	break;
	case EClipboard::COPY_HEX_FORMATTED:
	{
		strToClipboard.reserve((size_t)m_pSelect->GetSelectionSize() * 3);
		if (m_fSelectionBlock)
		{
			DWORD dwTail = m_pSelect->GetLineLength();
			for (unsigned i = 0; i < m_pSelect->GetSelectionSize(); i++)
			{
				BYTE chByte = GetByte(m_pSelect->GetNextOffset(i));
				strToClipboard += pszHexMap[(chByte & 0xF0) >> 4];
				strToClipboard += pszHexMap[(chByte & 0x0F)];

				if (i < (m_pSelect->GetSelectionSize() - 1) && (dwTail - 1) != 0)
					if (((m_pSelect->GetLineLength() - dwTail + 1) % (DWORD)m_enShowMode) == 0) //Add space after hex full chunk, ShowAs_size depending.
						strToClipboard += " ";
				if (--dwTail == 0 && i < (m_pSelect->GetSelectionSize() - 1)) //Next row.
				{
					strToClipboard += "\r\n";
					dwTail = m_pSelect->GetLineLength();
				}
			}
		}
		else
		{
			//How many spaces are needed to be inserted at the beginning.
			DWORD dwModStart = m_pSelect->GetSelectionStart() % m_dwCapacity;

			//When to insert first "\r\n".
			DWORD dwTail = m_dwCapacity - dwModStart;
			DWORD dwNextBlock = (m_dwCapacity % 2) ? m_dwCapacityBlockSize + 2 : m_dwCapacityBlockSize + 1;

			//If at least two rows are selected.
			if (dwModStart + m_pSelect->GetSelectionSize() > m_dwCapacity)
			{
				DWORD dwCount = dwModStart * 2 + dwModStart / (DWORD)m_enShowMode;
				//Additional spaces between halves. Only in ASBYTE's view mode.
				dwCount += (m_enShowMode == EHexShowMode::ASBYTE) ? (dwTail <= m_dwCapacityBlockSize ? 2 : 0) : 0;
				strToClipboard.insert(0, (size_t)dwCount, ' ');
			}

			for (unsigned i = 0; i < m_pSelect->GetSelectionSize(); i++)
			{
				BYTE chByte = GetByte(m_pSelect->GetNextOffset(i));
				strToClipboard += pszHexMap[(chByte & 0xF0) >> 4];
				strToClipboard += pszHexMap[(chByte & 0x0F)];

				if (i < (m_pSelect->GetSelectionSize() - 1) && (dwTail - 1) != 0)
					if (m_enShowMode == EHexShowMode::ASBYTE && dwTail == dwNextBlock)
						strToClipboard += "   "; //Additional spaces between halves. Only in ASBYTE view mode.
					else if (((m_dwCapacity - dwTail + 1) % (DWORD)m_enShowMode) == 0) //Add space after hex full chunk, ShowAs_size depending.
						strToClipboard += " ";
				if (--dwTail == 0 && i < (m_pSelect->GetSelectionSize() - 1)) //Next row.
				{
					strToClipboard += "\r\n";
					dwTail = m_dwCapacity;
				}
			}
		}
	}
	break;
	case EClipboard::COPY_ASCII:
	{
		strToClipboard.reserve((size_t)m_pSelect->GetSelectionSize());
		for (unsigned i = 0; i < m_pSelect->GetSelectionSize(); i++)
		{
			char ch = GetByte(m_pSelect->GetNextOffset(i));
			//If next byte is zero —> substitute it with space.
			if (ch == 0)
				ch = ' ';
			strToClipboard += ch;
		}
	}
	break;
	case EClipboard::COPY_BASE64:
	{
		strToClipboard.reserve((size_t)m_pSelect->GetSelectionSize() * 2);
		const char* const pszBase64Map { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" };
		unsigned int uValA = 0;
		int iValB = -6;
		for (unsigned i = 0; i < m_pSelect->GetSelectionSize(); i++)
		{
			uValA = (uValA << 8) + GetByte(m_pSelect->GetNextOffset(i));
			iValB += 8;
			while (iValB >= 0)
			{
				strToClipboard += pszBase64Map[(uValA >> iValB) & 0x3F];
				iValB -= 6;
			}
		}

		if (iValB > -6)
			strToClipboard += pszBase64Map[((uValA << 8) >> (iValB + 8)) & 0x3F];
		while (strToClipboard.size() % 4)
			strToClipboard += '=';
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
	if (!m_fMutable || !m_pSelect->HasSelection())
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
	ULONGLONG ullSizeToModify { };

	std::string strData;
	switch (enType)
	{
	case EClipboard::PASTE_ASCII:
		hmd.pData = (PBYTE)pClipboardData;
		ullSizeToModify = hmd.ullDataSize = ullSize;
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
		ullSizeToModify = hmd.ullDataSize = strData.size();
	}
	break;
	}

	hmd.vecSpan.emplace_back(HEXSPANSTRUCT { m_ullCursorPos, ullSizeToModify });

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
		if (m_pSelect->HasSelection())
			m_wstrBottomText.resize(swprintf_s(&m_wstrBottomText[0], 128,
				L"Selected: 0x%llX(%llu); Block: 0x%llX(%llu) - 0x%llX(%llu)",
				m_pSelect->GetSelectionSize(), m_pSelect->GetSelectionSize(),
				m_pSelect->GetSelectionStart(), m_pSelect->GetSelectionStart(),
				m_pSelect->GetSelectionEnd() - 1, m_pSelect->GetSelectionEnd() - 1));
		else
			m_wstrBottomText.resize(swprintf_s(&m_wstrBottomText[0], 128, L"Bytes total: 0x%llX(%llu)",
				m_ullDataSize, m_ullDataSize));
	}
	RedrawWindow();
}

void CHexCtrl::ParentNotify(const HEXNOTIFYSTRUCT& hns) const
{
	HWND hwndSendTo = GetParent()->GetSafeHwnd();
	if (hwndSendTo)
		::SendMessageW(hwndSendTo, WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&hns);
}

void CHexCtrl::ParentNotify(UINT uCode) const
{
	HEXNOTIFYSTRUCT hns { { m_hWnd, (UINT)GetDlgCtrlID(), uCode } };
}

void CHexCtrl::MsgWindowNotify(const HEXNOTIFYSTRUCT& hns)const
{
	//Send notification to the Message window if it was set.
	//Otherwise send to parent window.
	HWND hwndSendTo = GetMsgWindow();
	if (hwndSendTo)
		::SendMessageW(hwndSendTo, WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&hns);
}

void CHexCtrl::MsgWindowNotify(UINT uCode)const
{
	HEXNOTIFYSTRUCT hns { { m_hWnd, (UINT)GetDlgCtrlID(), uCode } };
	MsgWindowNotify(hns);
}

ULONGLONG CHexCtrl::GetCursorPos()
{
	return IsMutable() ? m_ullCursorPos : m_ullSelectionClick;
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
		SetSelection(m_ullSelectionClick + 1, m_ullSelectionClick + 1, 1, 1);
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
		SetSelection(m_ullSelectionClick - 1, m_ullSelectionClick - 1, 1, 1);
}

void CHexCtrl::CursorMoveUp()
{
	ULONGLONG ullNewPos = m_fMutable ? m_ullCursorPos : m_ullSelectionClick;
	if (ullNewPos >= m_dwCapacity)
		ullNewPos -= m_dwCapacity;

	if (m_fMutable)
		SetCursorPos(ullNewPos, m_fCursorHigh);
	else
		SetSelection(ullNewPos, ullNewPos, 1, 1);
}

void CHexCtrl::CursorMoveDown()
{
	ULONGLONG ullNewPos = m_fMutable ? m_ullCursorPos : m_ullSelectionClick;
	ullNewPos += m_dwCapacity;

	if (m_fMutable)
		SetCursorPos(ullNewPos, m_fCursorHigh);
	else
		SetSelection(ullNewPos, ullNewPos, 1, 1);
}

void CHexCtrl::Undo()
{
	if (m_deqUndo.empty())
		return;

	auto& refUndo = m_deqUndo.back();

	//Making new Redo data snapshot.
	auto& refRedo = m_deqRedo.emplace_back(std::make_unique<std::vector<UNDOSTRUCT>>());

	//Bad alloc may happen here!!!
	//If there is no more free memory, just clear the vec and return.
	try
	{
		for (auto& i : *refUndo)
		{
			refRedo->emplace_back(UNDOSTRUCT { i.ullOffset, { } });
			refRedo->back().vecData.reserve(i.vecData.size());
		}
	}
	catch (const std::bad_alloc&)
	{
		m_deqRedo.clear();
		return;
	}

	ULONGLONG ullIndex { 0 };
	for (auto& iter : *refUndo)
	{
		auto& refUndoData = iter.vecData;
		for (size_t i = 0; i < refUndoData.size(); i++)
		{
			refRedo->at((size_t)ullIndex).vecData.push_back((std::byte)GetByte(iter.ullOffset + i)); //Fill Redo data with the next byte.
			SetByte(iter.ullOffset + i, (BYTE)refUndoData[i]);                  //Undo the next byte.
		}
		++ullIndex;
	}
	m_deqUndo.pop_back();

	RedrawWindow();
}

void CHexCtrl::Redo()
{
	if (m_deqRedo.empty())
		return;

	const auto& refRedo = m_deqRedo.back();

	std::vector<HEXSPANSTRUCT> vecSpan { };
	std::transform(refRedo->begin(), refRedo->end(), std::back_inserter(vecSpan),
		[](UNDOSTRUCT& ref) { return HEXSPANSTRUCT { ref.ullOffset, ref.vecData.size() }; });

	//Making new Undo data snapshot.
	SnapshotUndo(vecSpan);

	for (auto& iter : *refRedo)
	{
		auto& refData = iter.vecData;
		for (ULONGLONG i = 0; i < refData.size(); i++)
			SetByte(iter.ullOffset + i, (BYTE)refData[(size_t)i]);
	}

	m_deqRedo.pop_back();

	RedrawWindow();
}

void CHexCtrl::SnapshotUndo(const std::vector<HEXSPANSTRUCT>& vecSpan)
{
	//If Undo deque size is exceeding max limit,
	//remove first snapshot from the beginning (the oldest one).
	if (m_deqUndo.size() > m_dwsUndoMax)
		m_deqUndo.pop_front();

	//Making new Undo data snapshot.
	auto& refUndo = m_deqUndo.emplace_back(std::make_unique<std::vector<UNDOSTRUCT>>());

	//Bad alloc may happen here!!!
	try
	{
		for (auto& i : vecSpan)
		{
			refUndo->emplace_back(UNDOSTRUCT { i.ullOffset, { } });
			refUndo->back().vecData.reserve((size_t)i.ullSize);
		}
	}

	catch (const std::bad_alloc&)
	{
		m_deqUndo.clear();
		m_deqRedo.clear();
		return;
	}

	ULONGLONG ullIndex { 0 };
	for (auto& iter : vecSpan)
	{
		for (size_t i = 0; i < iter.ullSize; i++)
			refUndo->at((size_t)ullIndex).vecData.push_back((std::byte)GetByte(iter.ullOffset + i));
		++ullIndex;
	}
}

bool CHexCtrl::IsCurTextArea()const
{
	return m_fCursorTextArea;
}

void CHexCtrl::SetSelection(ULONGLONG ullClick, ULONGLONG ullStart, ULONGLONG ullSize, ULONGLONG ullLines, bool fScroll, bool fGoToStart)
{
	if (ullClick >= m_ullDataSize || ullStart >= m_ullDataSize || !ullSize)
		return;

	if ((ullStart + ullSize) > m_ullDataSize)
		ullSize = m_ullDataSize - ullStart;

	ULONGLONG ullEnd = ullStart + ullSize; //ullEnd is exclusive - ).
	m_ullSelectionClick = ullClick;
	m_ullCursorPos = ullStart;

	std::vector<HEXSPANSTRUCT> vecSelection;
	for (ULONGLONG i = 0; i < ullLines; i++)
		vecSelection.emplace_back(HEXSPANSTRUCT { ullStart + m_dwCapacity * i, ullSize });
	m_pSelect->SetSelection(vecSelection);

	UpdateInfoText();
	MsgWindowNotify(HEXCTRL_MSG_SETSELECTION);

	//Don't need to scroll if it's mouse selection, or just selection assignment.
	//If fScroll is true then do scrolling according to the selection made.
	if (fScroll)
	{
		if (fGoToStart)
			GoToOffset(ullStart);
		else
		{
			ULONGLONG ullCurrScrollV = m_pScrollV->GetScrollPos();
			ULONGLONG ullCurrScrollH = m_pScrollH->GetScrollPos();
			int iCx, iCy;
			HexChunkPoint(ullStart, iCx, iCy);
			CRect rcClient;
			GetClientRect(&rcClient);

			//New scroll depending on selection direction: top <-> bottom.
			//fGoToStart means centralize scroll position on the screen (used in Search()).
			ULONGLONG ullMaxV = ullCurrScrollV + rcClient.Height() - m_iHeightBottomOffArea - m_iStartWorkAreaY -
				((rcClient.Height() - m_iStartWorkAreaY - m_iHeightBottomOffArea) % m_sizeLetter.cy);
			ULONGLONG ullNewStartV = ullStart / m_dwCapacity * m_sizeLetter.cy;
			ULONGLONG ullNewEndV = (ullEnd - 1) / m_dwCapacity * m_sizeLetter.cy;
			ULONGLONG ullNewScrollV { }, ullNewScrollH { };

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

			ullNewScrollV -= ullNewScrollV % m_sizeLetter.cy;
			m_pScrollV->SetScrollPos(ullNewScrollV);
			if (m_pScrollH->IsVisible() && !IsCurTextArea())
				m_pScrollH->SetScrollPos(ullNewScrollH);
		}
	}
}

void CHexCtrl::GoToOffset(ULONGLONG ullOffset)
{
	if (!IsDataSet())
		return;

	ULONGLONG ullNewStartV = ullOffset / m_dwCapacity * m_sizeLetter.cy;
	ULONGLONG ullNewScrollV { }, ullNewScrollH { };

	//To prevent negative numbers.
	if (ullNewStartV > m_iHeightWorkArea / 2)
	{
		ullNewScrollV = ullNewStartV - m_iHeightWorkArea / 2;
		ullNewScrollV -= ullNewScrollV % m_sizeLetter.cy;
	}
	else
		ullNewScrollV = 0;

	ullNewScrollH = (ullOffset % m_dwCapacity) * m_iSizeHexByte;
	ullNewScrollH += (ullNewScrollH / m_iDistanceBetweenHexChunks) * m_iSpaceBetweenHexChunks;

	m_pScrollV->SetScrollPos(ullNewScrollV);
	if (m_pScrollH->IsVisible() && !IsCurTextArea())
		m_pScrollH->SetScrollPos(ullNewScrollH);
}

void CHexCtrl::SelectAll()
{
	if (!IsDataSet())
		return;

	std::vector<HEXSPANSTRUCT> vecSpan { { 0, m_ullDataSize } };
	m_pSelect->SetSelection(vecSpan);
	UpdateInfoText();
}

void CHexCtrl::FillWithZeros()
{
	if (!IsDataSet())
		return;

	HEXMODIFYSTRUCT hms;
	hms.vecSpan = m_pSelect->GetVector();
	hms.ullDataSize = 1;
	hms.enCreateMode = EHexModifyMode::MODIFY_REPEAT;
	unsigned char chZero { 0 };
	hms.pData = &chZero;
	ModifyData(hms);
}

void CHexCtrl::WstrCapacityFill()
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
		if ((((iter + 1) % (DWORD)m_enShowMode) == 0) && (iter < (m_dwCapacity - 1)))
			m_wstrCapacity += L" ";

		//Additional space between hex halves.
		if (m_enShowMode == EHexShowMode::ASBYTE && iter == (m_dwCapacityBlockSize - 1))
			m_wstrCapacity += L"  ";
	}
}