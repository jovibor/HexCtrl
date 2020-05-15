/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#include "stdafx.h"
#include "CHexBookmarks.h"
#include "CHexCtrl.h"
#include "CHexSelection.h"
#include "CScrollEx.h"
#include "Dialogs/CHexDlgAbout.h"
#include "Dialogs/CHexDlgBookmarkMgr.h"
#include "Dialogs/CHexDlgCallback.h"
#include "Dialogs/CHexDlgDataInterpret.h"
#include "Dialogs/CHexDlgFillData.h"
#include "Dialogs/CHexDlgOperations.h"
#include "Dialogs/CHexDlgSearch.h"
#include "Helper.h"
#include "strsafe.h"
#include <algorithm>
#include <cassert>
#include <numeric>
#include <thread>
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
		static HEXCTRLINFO stVersion { HEXCTRL_VERSION_WSTR, { HEXCTRL_VERSION_ULL } };
		//	static HEXCTRLINFO stVersion { .pwszVersion = HEXCTRL_VERSION_WSTR, .ullVersion = { HEXCTRL_VERSION_ULL } };

		return &stVersion;
	};

	/********************************************
	* Internal enums and structs.               *
	********************************************/
	namespace INTERNAL {
		enum class EClipboard : DWORD
		{
			COPY_HEX, COPY_HEX_LE, COPY_HEX_FORMATTED, COPY_ASCII, COPY_BASE64,
			COPY_CARRAY, COPY_GREPHEX, COPY_PRINTSCREEN,
			PASTE_HEX, PASTE_ASCII
		};

		//Struct for UNDO command routine.
		struct UNDOSTRUCT
		{
			ULONGLONG              ullOffset { }; //Start byte to apply Undo to.
			std::vector<std::byte> vecData { };   //Data for Undo.
		};

		//Struct for resources auto deletion on destruction.
		struct HBITMAPSTRUCT
		{
			HBITMAPSTRUCT() = default;
			~HBITMAPSTRUCT() { ::DeleteObject(m_hBmp); }
			HBITMAPSTRUCT& operator=(HBITMAP hBmp) { m_hBmp = hBmp; return *this; }
			operator HBITMAP()const { return m_hBmp; }
			HBITMAP m_hBmp { };
		};

		constexpr auto WSTR_HEXCTRL_CLASSNAME = L"HexCtrl";
		constexpr ULONG_PTR ID_TOOLTIP_BKM { 0x01 };    //Tooltip ID for bookmarks.
		constexpr ULONG_PTR ID_TOOLTIP_OFFSET { 0x02 }; //Tooltip ID for offset.
	}
}

/************************************************************************
* CHexCtrl class implementation.                                        *
************************************************************************/
BEGIN_MESSAGE_MAP(CHexCtrl, CWnd)
	ON_WM_ACTIVATE()
	ON_WM_CHAR()
	ON_WM_CONTEXTMENU()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_WM_GETDLGCODE()
	ON_WM_HELPINFO()
	ON_WM_HSCROLL()
	ON_WM_INITMENUPOPUP()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_NCACTIVATE()
	ON_WM_NCCALCSIZE()
	ON_WM_NCPAINT()
	ON_WM_PAINT()
	ON_WM_RBUTTONDOWN()
	ON_WM_SETCURSOR()
	ON_WM_SIZE()
	ON_WM_SYSKEYDOWN()
	ON_WM_TIMER()
	ON_WM_VSCROLL()
END_MESSAGE_MAP()

//CWinApp object is vital for manual MFC, and for in-.dll work,
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
		wc.hCursor = static_cast<HCURSOR>(LoadImageW(nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
		wc.hbrBackground = nullptr;
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = WSTR_HEXCTRL_CLASSNAME;
		if (!RegisterClassExW(&wc))
		{
			MessageBoxW(L"HexControl RegisterClassExW error.", L"Error", MB_ICONERROR);
			return;
		}
	}
}

ULONGLONG CHexCtrl::BkmAdd(const HEXBOOKMARKSTRUCT& hbs, bool fRedraw)
{
	assert(IsCreated());
	if (!IsCreated())
		return 0xFFFFFFFF;

	return m_pBookmarks->Add(hbs, fRedraw);
}

void CHexCtrl::BkmClearAll()
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_pBookmarks->ClearAll();
}

auto CHexCtrl::BkmGetByID(ULONGLONG ullID)->HEXBOOKMARKSTRUCT*
{
	assert(IsCreated());
	if (!IsCreated())
		return nullptr;

	return m_pBookmarks->GetByID(ullID);
}

auto CHexCtrl::BkmGetByIndex(ULONGLONG ullIndex)->HEXBOOKMARKSTRUCT*
{
	assert(IsCreated());
	if (!IsCreated())
		return nullptr;

	return m_pBookmarks->GetByIndex(ullIndex);
}

ULONGLONG CHexCtrl::BkmGetCount() const
{
	assert(IsCreated());
	if (!IsCreated())
		return 0ULL;

	return m_pBookmarks->GetCount();
}

auto CHexCtrl::BkmHitTest(ULONGLONG ullOffset)->HEXBOOKMARKSTRUCT*
{
	assert(IsCreated());
	if (!IsCreated())
		return nullptr;

	return m_pBookmarks->HitTest(ullOffset);
}

void CHexCtrl::BkmRemoveByID(ULONGLONG ullID)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_pBookmarks->RemoveByID(ullID);
}

void CHexCtrl::BkmSetVirtual(bool fEnable, IHexVirtBkm* pVirtual)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_pBookmarks->SetVirtual(fEnable, pVirtual);
}

void CHexCtrl::ClearData()
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_fDataSet = false;
	m_pData = nullptr;
	m_ullDataSize = 0;
	m_fMutable = false;
	m_pHexVirtData = nullptr;
	m_pHexVirtColors = nullptr;
	m_fHighLatency = false;
	m_ullLMouseClick = 0;
	m_optRMouseClick.reset();
	m_ullCaretPos = 0;
	m_ullCurCursor = 0;

	m_deqUndo.clear();
	m_deqRedo.clear();
	m_pScrollV->SetScrollPos(0);
	m_pScrollH->SetScrollPos(0);
	m_pScrollV->SetScrollSizes(0, 0, 0);
	m_pBookmarks->ClearAll();
	m_pSelection->ClearAll();
	UpdateInfoText();
}

bool CHexCtrl::Create(const HEXCREATESTRUCT& hcs)
{
	assert(!IsCreated()); //Already created.
	if (IsCreated())
		return false;

	HINSTANCE hInst = AfxGetInstanceHandle();

	//Menus
	if (!m_menuMain.LoadMenuW(MAKEINTRESOURCE(IDR_HEXCTRL_MENU)))
	{
		MessageBoxW(L"HexControl LoadMenu(IDR_HEXCTRL_MENU) failed.", L"Error", MB_ICONERROR);
		return false;
	}
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
	UINT uID { 0 };
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

	m_wndTtBkm.CreateEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr,
		/*TTS_BALLOON |*/ TTS_NOANIMATE | TTS_NOFADE | TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		m_hWnd, nullptr);
	SetWindowTheme(m_wndTtBkm.m_hWnd, nullptr, L""); //To prevent Windows from changing theme of Balloon window.
	m_stToolInfoBkm.cbSize = TTTOOLINFOW_V1_SIZE;
	m_stToolInfoBkm.uFlags = TTF_TRACK;
	m_stToolInfoBkm.uId = ID_TOOLTIP_BKM;
	m_wndTtBkm.SendMessageW(TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&m_stToolInfoBkm);
	m_wndTtBkm.SendMessageW(TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(400)); //to allow use of newline \n.
	m_wndTtBkm.SendMessageW(TTM_SETTIPTEXTCOLOR, static_cast<WPARAM>(m_stColor.clrTextTooltip), 0);
	m_wndTtBkm.SendMessageW(TTM_SETTIPBKCOLOR, static_cast<WPARAM>(m_stColor.clrBkTooltip), 0);

	m_wndTtOffset.CreateEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr,
		/*TTS_BALLOON |*/ TTS_NOANIMATE | TTS_NOFADE | TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		m_hWnd, nullptr);
	SetWindowTheme(m_wndTtOffset.m_hWnd, nullptr, L""); //To prevent Windows from changing theme of Balloon window.
	m_stToolInfoOffset.cbSize = TTTOOLINFOW_V1_SIZE;
	m_stToolInfoOffset.uFlags = TTF_TRACK;
	m_stToolInfoOffset.uId = ID_TOOLTIP_OFFSET;
	m_wndTtOffset.SendMessageW(TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&m_stToolInfoOffset);
	m_wndTtOffset.SendMessageW(TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(400)); //to allow use of newline \n.
	m_wndTtOffset.SendMessageW(TTM_SETTIPTEXTCOLOR, static_cast<WPARAM>(m_stColor.clrTextTooltip), 0);
	m_wndTtOffset.SendMessageW(TTM_SETTIPBKCOLOR, static_cast<WPARAM>(m_stColor.clrBkTooltip), 0);

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
	m_fontMain.CreateFontIndirectW(&lf);

	lf.lfHeight = 16;
	m_fontInfo.CreateFontIndirectW(&lf);
	//End of font related.///////////////////////////////////////

	m_penLines.CreatePen(PS_SOLID, 1, RGB(200, 200, 200));

	//Removing window's border frame.
	MARGINS marg { 0, 0, 0, 1 };
	DwmExtendFrameIntoClientArea(m_hWnd, &marg);
	SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

	//ScrollBars should be created here, after main Window (to attach to) has already been created, to avoid assertions.
	m_pScrollV->Create(this, SB_VERT, 0, 0, 0); //Actual sizes are set in RecalcAll().
	m_pScrollH->Create(this, SB_HORZ, 0, 0, 0);
	m_pScrollV->AddSibling(m_pScrollH.get());
	m_pScrollH->AddSibling(m_pScrollV.get());

	//All dialogs are created after the main window, to set the parent window correctly.
	m_pDlgSearch->Create(IDD_HEXCTRL_SEARCH, this);
	m_pDlgOpers->Create(IDD_HEXCTRL_OPERATIONS, this);
	m_pDlgFillData->Create(IDD_HEXCTRL_FILLDATA, this);
	m_pDlgBookmarkMgr->Create(IDD_HEXCTRL_BOOKMARKMGR, this, &*m_pBookmarks);
	m_pDlgDataInterpret->Create(IDD_HEXCTRL_DATAINTERPRET, this);
	m_pBookmarks->Attach(this);
	m_pSelection->Attach(this);

	m_fCreated = true;
	SetShowMode(m_enShowMode);

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

void CHexCtrl::ExecuteCmd(EHexCmd enCmd)const
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	WPARAM wParam { };
	switch (enCmd)
	{
	case EHexCmd::CMD_SEARCH:
		wParam = IDM_HEXCTRL_SEARCH;
		break;
	case EHexCmd::CMD_SEARCH_NEXT:
		m_pDlgSearch->Search(true);
		break;
	case EHexCmd::CMD_SEARCH_PREV:
		m_pDlgSearch->Search(false);
		break;
	case EHexCmd::CMD_SHOWDATA_BYTE:
		wParam = IDM_HEXCTRL_SHOWAS_BYTE;
		break;
	case EHexCmd::CMD_SHOWDATA_WORD:
		wParam = IDM_HEXCTRL_SHOWAS_WORD;
		break;
	case EHexCmd::CMD_SHOWDATA_DWORD:
		wParam = IDM_HEXCTRL_SHOWAS_DWORD;
		break;
	case EHexCmd::CMD_SHOWDATA_QWORD:
		wParam = IDM_HEXCTRL_SHOWAS_QWORD;
		break;
	case EHexCmd::CMD_BKM_ADD:
		wParam = IDM_HEXCTRL_BOOKMARKS_ADD;
		break;
	case EHexCmd::CMD_BKM_REMOVE:
		wParam = IDM_HEXCTRL_BOOKMARKS_REMOVE;
		break;
	case EHexCmd::CMD_BKM_NEXT:
		wParam = IDM_HEXCTRL_BOOKMARKS_NEXT;
		break;
	case EHexCmd::CMD_BKM_PREV:
		wParam = IDM_HEXCTRL_BOOKMARKS_PREV;
		break;
	case EHexCmd::CMD_BKM_CLEARALL:
		wParam = IDM_HEXCTRL_BOOKMARKS_CLEARALL;
		break;
	case EHexCmd::CMD_BKM_MANAGER:
		wParam = IDM_HEXCTRL_BOOKMARKS_MANAGER;
		break;
	case EHexCmd::CMD_CLIPBOARD_COPY_HEX:
		wParam = IDM_HEXCTRL_CLIPBOARD_COPYHEX;
		break;
	case EHexCmd::CMD_CLIPBOARD_COPY_HEXLE:
		wParam = IDM_HEXCTRL_CLIPBOARD_COPYHEXLE;
		break;
	case EHexCmd::CMD_CLIPBOARD_COPY_HEXFMT:
		wParam = IDM_HEXCTRL_CLIPBOARD_COPYHEXFORMATTED;
		break;
	case EHexCmd::CMD_CLIPBOARD_COPY_ASCII:
		wParam = IDM_HEXCTRL_CLIPBOARD_COPYASCII;
		break;
	case EHexCmd::CMD_CLIPBOARD_COPY_BASE64:
		wParam = IDM_HEXCTRL_CLIPBOARD_COPYBASE64;
		break;
	case EHexCmd::CMD_CLIPBOARD_COPY_CARR:
		wParam = IDM_HEXCTRL_CLIPBOARD_COPYCARRAY;
		break;
	case EHexCmd::CMD_CLIPBOARD_COPY_GREPHEX:
		wParam = IDM_HEXCTRL_CLIPBOARD_COPYGREPHEX;
		break;
	case EHexCmd::CMD_CLIPBOARD_COPY_PRNTSCRN:
		wParam = IDM_HEXCTRL_CLIPBOARD_COPYPRINTSCREEN;
		break;
	case EHexCmd::CMD_CLIPBOARD_PASTE_HEX:
		wParam = IDM_HEXCTRL_CLIPBOARD_PASTEHEX;
		break;
	case EHexCmd::CMD_CLIPBOARD_PASTE_ASCII:
		wParam = IDM_HEXCTRL_CLIPBOARD_PASTEASCII;
		break;
	case EHexCmd::CMD_MODIFY_OPERS:
		wParam = IDM_HEXCTRL_MODIFY_OPERATIONS;
		break;
	case EHexCmd::CMD_MODIFY_FILLZEROS:
		wParam = IDM_HEXCTRL_MODIFY_FILLZEROS;
		break;
	case EHexCmd::CMD_MODIFY_FILLDATA:
		wParam = IDM_HEXCTRL_MODIFY_FILLDATA;
		break;
	case EHexCmd::CMD_MODIFY_UNDO:
		wParam = IDM_HEXCTRL_MODIFY_UNDO;
		break;
	case EHexCmd::CMD_MODIFY_REDO:
		wParam = IDM_HEXCTRL_MODIFY_REDO;
		break;
	case EHexCmd::CMD_SEL_MARKSTART:
		wParam = IDM_HEXCTRL_SELECTION_MARKSTART;
		break;
	case EHexCmd::CMD_SEL_MARKEND:
		wParam = IDM_HEXCTRL_SELECTION_MARKEND;
		break;
	case EHexCmd::CMD_SEL_SELECTALL:
		wParam = IDM_HEXCTRL_SELECTION_SELECTALL;
		break;
	case EHexCmd::CMD_DATAINTERPRET:
		wParam = IDM_HEXCTRL_DATAINTERPRET;
		break;
	case EHexCmd::CMD_APPEARANCE_FONTINC:
		wParam = IDM_HEXCTRL_APPEARANCE_FONTINCREASE;
		break;
	case EHexCmd::CMD_APPEARANCE_FONTDEC:
		wParam = IDM_HEXCTRL_APPEARANCE_FONTDECREASE;
		break;
	case EHexCmd::CMD_APPEARANCE_CAPACITYINC:
		wParam = IDM_HEXCTRL_APPEARANCE_CAPACITYINCREASE;
		break;
	case EHexCmd::CMD_APPEARANCE_CAPACITYDEC:
		wParam = IDM_HEXCTRL_APPEARANCE_CAPACITYDECREASE;
		break;
	case EHexCmd::CMD_PRINT:
		wParam = IDM_HEXCTRL_PRINT;
		break;
	case EHexCmd::CMD_ABOUT:
		wParam = IDM_HEXCTRL_ABOUT;
		break;
	}

	if (wParam)
		SendMessageW(WM_COMMAND, wParam, 0);
}

DWORD CHexCtrl::GetCapacity()const
{
	assert(IsCreated());
	if (!IsCreated())
		return 0;

	return m_dwCapacity;
}

ULONGLONG CHexCtrl::GetCaretPos()const
{
	return m_ullCaretPos;
}

auto CHexCtrl::GetColors()const->HEXCOLORSSTRUCT
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	return m_stColor;
}

long CHexCtrl::GetFontSize()const
{
	assert(IsCreated());
	if (!IsCreated())
		return 0;

	return m_lFontSize;
}

HMENU CHexCtrl::GetMenuHandle()const
{
	assert(IsCreated());
	if (!IsCreated())
		return nullptr;

	return m_menuMain.GetSubMenu(0)->GetSafeHmenu();
}

DWORD CHexCtrl::GetSectorSize()const
{
	assert(IsCreated());
	if (!IsCreated())
		return 0;

	return m_dwSectorSize;
}

auto CHexCtrl::GetSelection()const->std::vector<HEXSPANSTRUCT>
{
	assert(IsCreated());
	assert(IsDataSet()); //Data is not set yet.

	return m_pSelection->GetData();
}

auto CHexCtrl::GetShowMode()const->EHexShowMode
{
	assert(IsCreated());
	if (!IsCreated())
		return EHexShowMode { };

	return m_enShowMode;
}

HWND CHexCtrl::GetWindowHandle()const
{
	assert(IsCreated());
	if (!IsCreated())
		return nullptr;

	return m_hWnd;
}

void CHexCtrl::GoToOffset(ULONGLONG ullOffset, bool fSelect, ULONGLONG ullSize)
{
	assert(IsCreated());
	assert(IsDataSet()); //Data is not set yet.
	if (!IsCreated() || !IsDataSet())
		return;

	if (fSelect)
		MakeSelection(ullOffset, ullOffset, ullSize, 1, true, true);
	else
		GoToOffset(ullOffset);
}

auto CHexCtrl::HitTest(POINT pt, bool fScreen)const->std::optional<HEXHITTESTSTRUCT>
{
	if (fScreen)
		ScreenToClient(&pt);

	return HitTest(pt);
}

bool CHexCtrl::IsCmdAvail(EHexCmd enCmd)const
{
	assert(IsCreated());
	if (!IsCreated())
		return false;

	bool fDataSet = IsDataSet();
	bool fMutable { false };
	if (fDataSet)
		fMutable = IsMutable();
	bool fStatus = fDataSet && m_pSelection->HasSelection();

	bool fAvail { false };
	switch (enCmd)
	{
	case EHexCmd::CMD_SHOWDATA_BYTE:
	case EHexCmd::CMD_SHOWDATA_WORD:
	case EHexCmd::CMD_SHOWDATA_DWORD:
	case EHexCmd::CMD_SHOWDATA_QWORD:
	case EHexCmd::CMD_APPEARANCE_FONTINC:
	case EHexCmd::CMD_APPEARANCE_FONTDEC:
	case EHexCmd::CMD_APPEARANCE_CAPACITYINC:
	case EHexCmd::CMD_APPEARANCE_CAPACITYDEC:
	case EHexCmd::CMD_PRINT:
	case EHexCmd::CMD_ABOUT:
		fAvail = true;
		break;
	case EHexCmd::CMD_BKM_REMOVE:
		fAvail = m_pBookmarks->HasBookmarks()
			&& (m_pBookmarks->HitTest(GetCaretPos()) != nullptr
				|| m_optRMouseClick ? m_pBookmarks->HitTest(*m_optRMouseClick) != nullptr : false);
		break;
	case EHexCmd::CMD_BKM_NEXT:
	case EHexCmd::CMD_BKM_PREV:
	case EHexCmd::CMD_BKM_CLEARALL:
		fAvail = m_pBookmarks->HasBookmarks();
		break;
	case EHexCmd::CMD_BKM_ADD:
	case EHexCmd::CMD_CLIPBOARD_COPY_HEX:
	case EHexCmd::CMD_CLIPBOARD_COPY_HEXLE:
	case EHexCmd::CMD_CLIPBOARD_COPY_HEXFMT:
	case EHexCmd::CMD_CLIPBOARD_COPY_ASCII:
	case EHexCmd::CMD_CLIPBOARD_COPY_BASE64:
	case EHexCmd::CMD_CLIPBOARD_COPY_CARR:
	case EHexCmd::CMD_CLIPBOARD_COPY_GREPHEX:
	case EHexCmd::CMD_CLIPBOARD_COPY_PRNTSCRN:
		fAvail = fStatus;
		break;
	case EHexCmd::CMD_CLIPBOARD_PASTE_HEX:
	case EHexCmd::CMD_CLIPBOARD_PASTE_ASCII:
		fAvail = fMutable && fStatus && IsClipboardFormatAvailable(CF_TEXT);
		break;
	case EHexCmd::CMD_MODIFY_OPERS:
	case EHexCmd::CMD_MODIFY_FILLZEROS:
	case EHexCmd::CMD_MODIFY_FILLDATA:
		fAvail = fMutable && fStatus;
		break;
	case EHexCmd::CMD_MODIFY_UNDO:
		fAvail = !m_deqUndo.empty();
		break;
	case EHexCmd::CMD_MODIFY_REDO:
		fAvail = !m_deqRedo.empty();
		break;
	case EHexCmd::CMD_SEARCH:
	case EHexCmd::CMD_BKM_MANAGER:
	case EHexCmd::CMD_SEL_MARKSTART:
	case EHexCmd::CMD_SEL_MARKEND:
	case EHexCmd::CMD_SEL_SELECTALL:
	case EHexCmd::CMD_DATAINTERPRET:
		fAvail = fDataSet;
		break;
	case EHexCmd::CMD_SEARCH_NEXT:
	case EHexCmd::CMD_SEARCH_PREV:
		fAvail = m_pDlgSearch->IsSearchAvail();
		break;
	}

	return fAvail;
}

bool CHexCtrl::IsCreated()const
{
	return m_fCreated;
}

bool CHexCtrl::IsDataSet()const
{
	assert(IsCreated());
	if (!IsCreated())
		return false;

	return m_fDataSet;
}

bool CHexCtrl::IsDlgVisible(EHexDlg enDlg)const
{
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet())
		return false;

	bool fVisible { };
	switch (enDlg)
	{
	case EHexDlg::DLG_BKMMANAGER:
		fVisible = m_pDlgBookmarkMgr->IsWindowVisible();
		break;
	case EHexDlg::DLG_DATAINTERPRET:
		fVisible = m_pDlgDataInterpret->IsWindowVisible();
		break;
	case EHexDlg::DLG_FILLDATA:
		fVisible = m_pDlgFillData->IsWindowVisible();
		break;
	case EHexDlg::DLG_OPERS:
		fVisible = m_pDlgOpers->IsWindowVisible();
		break;
	case EHexDlg::DLG_SEARCH:
		fVisible = m_pDlgSearch->IsWindowVisible();
		break;
	}

	return fVisible;
}

bool CHexCtrl::IsMutable()const
{
	assert(IsCreated());
	assert(IsDataSet()); //Data is not set yet.
	if (!IsCreated() || !IsDataSet())
		return false;

	return m_fMutable;
}

bool CHexCtrl::IsOffsetAsHex()const
{
	assert(IsCreated());
	if (!IsCreated())
		return false;

	return m_fOffsetAsHex;
}

bool CHexCtrl::IsOffsetVisible(ULONGLONG ullOffset)const
{
	auto dwCapacity = GetCapacity();
	auto ullFirst = GetTopLine() * dwCapacity;;
	auto ullLast = GetBottomLine() * dwCapacity + dwCapacity;

	return (ullOffset >= ullFirst) && (ullOffset < ullLast);
}

void CHexCtrl::Redraw()
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	RedrawWindow();
}

void CHexCtrl::SetCapacity(DWORD dwCapacity)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	if (dwCapacity < 1 || dwCapacity > m_dwCapacityMax)
		return;

	//Setting capacity according to current m_enShowMode.
	if (dwCapacity < m_dwCapacity)
		dwCapacity -= dwCapacity % static_cast<DWORD>(m_enShowMode);
	else if (dwCapacity % static_cast<DWORD>(m_enShowMode))
		dwCapacity += static_cast<DWORD>(m_enShowMode) - (dwCapacity % static_cast<DWORD>(m_enShowMode));

	//To prevent under/over flow.
	if (dwCapacity < static_cast<DWORD>(m_enShowMode))
		dwCapacity = static_cast<DWORD>(m_enShowMode);
	else if (dwCapacity > m_dwCapacityMax)
		dwCapacity = m_dwCapacityMax;

	m_dwCapacity = dwCapacity;
	m_dwCapacityBlockSize = m_dwCapacity / 2;

	UpdateSectorVisible();
	WstrCapacityFill();
	RecalcAll();
	MsgWindowNotify(HEXCTRL_MSG_VIEWCHANGE); //Indicates to parent that view has changed.
}

void CHexCtrl::SetColors(const HEXCOLORSSTRUCT& clr)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_stColor = clr;
	RedrawWindow();
}

void CHexCtrl::SetData(const HEXDATASTRUCT& hds)
{
	assert(IsCreated());
	if (!IsCreated() || hds.ullDataSize == 0)
		return;

	//m_hwndMsg was previously set in IHexCtrl::Create.
	if (hds.hwndMsg)
		m_hwndMsg = hds.hwndMsg;

	//Virtual mode is possible only when there is a MSG window, a data requests will be sent to.
	if (hds.enDataMode == EHexDataMode::DATA_MSG && !GetMsgWindow())
	{
		MessageBoxW(L"HexCtrl EHexDataMode::DATA_MSG mode requires HEXDATASTRUCT::hwndMsg to be set.", L"Error", MB_ICONWARNING);
		return;
	}
	if (hds.enDataMode == EHexDataMode::DATA_VIRTUAL && !hds.pHexVirtData)
	{
		MessageBoxW(L"HexCtrl EHexDataMode::DATA_VIRTUAL mode requires HEXDATASTRUCT::pHexVirtData to be set.", L"Error", MB_ICONWARNING);
		return;
	}

	ClearData();

	m_fDataSet = true;
	m_pData = hds.pData;
	m_ullDataSize = hds.ullDataSize;
	m_enDataMode = hds.enDataMode;
	m_fMutable = hds.fMutable;
	m_pHexVirtData = hds.pHexVirtData;
	m_pHexVirtColors = hds.pHexVirtColors;
	m_dwCacheSize = hds.dwCacheSize > 0x10000 ? hds.dwCacheSize : 0x10000; //64Kb is the minimum size
	m_fHighLatency = hds.fHighLatency;

	RecalcAll();
	UpdateSectorVisible();

	if (hds.stSelSpan.ullSize)
		MakeSelection(hds.stSelSpan.ullOffset, hds.stSelSpan.ullOffset, hds.stSelSpan.ullSize, 1, true, true);
}

void CHexCtrl::SetFont(const LOGFONTW* pLogFont)
{
	assert(IsCreated());
	assert(pLogFont); //Null font pointer.
	if (!IsCreated() || !pLogFont)
		return;

	m_fontMain.DeleteObject();
	m_fontMain.CreateFontIndirectW(pLogFont);

	RecalcAll();
}

void CHexCtrl::SetFontSize(UINT uiSize)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	//Prevent font size from being too small or too big.
	if (uiSize < 9 || uiSize > 75)
		return;

	LOGFONTW lf;
	m_fontMain.GetLogFont(&lf);
	m_lFontSize = lf.lfHeight = uiSize;
	m_fontMain.DeleteObject();
	m_fontMain.CreateFontIndirectW(&lf);

	RecalcAll();
	MsgWindowNotify(HEXCTRL_MSG_VIEWCHANGE); //Indicates to parent that view has changed.
}

void CHexCtrl::SetMutable(bool fEnable)
{
	assert(IsCreated());
	assert(IsDataSet()); //Data is not set yet.
	if (!IsCreated() || !IsDataSet())
		return;

	m_fMutable = fEnable;
	UpdateInfoText();
}

void CHexCtrl::SetSectorSize(DWORD dwSize, std::wstring_view wstrName)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_dwSectorSize = dwSize;
	m_wstrSectorName = wstrName;

	if (IsDataSet())
		UpdateSectorVisible();
}

void CHexCtrl::SetSelection(const std::vector<HEXSPANSTRUCT>& vecSel)
{
	assert(IsCreated());
	assert(IsDataSet()); //Data is not set yet.
	if (!IsCreated() || !IsDataSet())
		return;

	m_pSelection->SetSelection(vecSel);
	UpdateInfoText();
	MsgWindowNotify(HEXCTRL_MSG_SELECTION);
}

void CHexCtrl::SetShowMode(EHexShowMode enShowMode)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_enShowMode = enShowMode;

	//Getting the "Show data as..." menu pointer independent of position.
	auto pMenuMain = m_menuMain.GetSubMenu(0);
	CMenu* pMenuShowDataAs { };
	for (int i = 0; i < pMenuMain->GetMenuItemCount(); ++i)
	{
		//Searching through all submenus whose first menuID is IDM_HEXCTRL_SHOWAS_BYTE.
		if (auto pSubMenu = pMenuMain->GetSubMenu(i); pSubMenu != nullptr)
			if (pSubMenu->GetMenuItemID(0) == IDM_HEXCTRL_SHOWAS_BYTE)
			{
				pMenuShowDataAs = pSubMenu;
				break;
			}
	}

	if (pMenuShowDataAs)
	{
		//Unchecking all menus and checking only the currently selected.
		for (int i = 0; i < pMenuShowDataAs->GetMenuItemCount(); ++i)
			pMenuShowDataAs->CheckMenuItem(i, MF_UNCHECKED | MF_BYPOSITION);

		UINT ID { };
		switch (enShowMode)
		{
		case EHexShowMode::ASBYTE:
			ID = IDM_HEXCTRL_SHOWAS_BYTE;
			break;
		case EHexShowMode::ASWORD:
			ID = IDM_HEXCTRL_SHOWAS_WORD;
			break;
		case EHexShowMode::ASDWORD:
			ID = IDM_HEXCTRL_SHOWAS_DWORD;
			break;
		case EHexShowMode::ASQWORD:
			ID = IDM_HEXCTRL_SHOWAS_QWORD;
			break;
		}
		pMenuShowDataAs->CheckMenuItem(ID, MF_CHECKED | MF_BYCOMMAND);
	}
	SetCapacity(m_dwCapacity); //To recalc current representation.
}

void CHexCtrl::SetWheelRatio(double dbRatio)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_dbWheelRatio = dbRatio;
	SendMessageW(WM_SIZE);   //To recalc ScrollPageSize (m_pScrollV->SetScrollPageSize(...);)
}

void CHexCtrl::ShowDlg(EHexDlg enDlg, bool fShow)const
{
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet())
		return;

	int iShow { fShow ? SW_SHOW : SW_HIDE };
	bool fMut = IsMutable();

	switch (enDlg)
	{
	case EHexDlg::DLG_BKMMANAGER:
		m_pDlgBookmarkMgr->ShowWindow(iShow);
		break;
	case EHexDlg::DLG_DATAINTERPRET:
		m_pDlgDataInterpret->ShowWindow(iShow);
		break;
	case EHexDlg::DLG_FILLDATA:
		if (fMut)
			m_pDlgFillData->ShowWindow(iShow);
		break;
	case EHexDlg::DLG_OPERS:
		if (fMut)
			m_pDlgOpers->ShowWindow(iShow);
		break;
	case EHexDlg::DLG_SEARCH:
		m_pDlgSearch->ShowWindow(iShow);
		break;
	}
}


/**************************************************************************
* HexCtrl Private methods.
**************************************************************************/
void CHexCtrl::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	SetFocus();

	CWnd::OnActivate(nState, pWndOther, bMinimized);
}

BOOL CHexCtrl::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
	return FALSE;
}

void CHexCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	const auto optHit = HitTest(point);

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

		//To avoid unnecessary work we're checking if the current cursor pos is the same
		//that is was at previous WM_MOUSEMOVE fire, with m_ullCurCursor.
		if (!optHit || optHit->ullOffset == m_ullCurCursor)
			return;

		m_ullCurCursor = optHit->ullOffset;
		const auto ullHit = optHit->ullOffset;
		ULONGLONG ullClick, ullStart, ullSize, ullLines;
		if (m_fSelectionBlock) //Select block (with Alt)
		{
			ullClick = m_ullLMouseClick;
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
			if (ullHit <= m_ullLMouseClick)
			{
				ullClick = m_ullLMouseClick;
				ullStart = ullHit;
				ullSize = ullClick - ullStart + 1;
			}
			else
			{
				ullClick = m_ullLMouseClick;
				ullStart = m_ullLMouseClick;
				ullSize = ullHit - ullClick + 1;
			}
			ullLines = 1;
		}

		MakeSelection(ullClick, ullStart, ullSize, ullLines, false);
	}
	else
	{
		if (optHit)
		{
			if (const auto pBookmark = m_pBookmarks->HitTest(optHit->ullOffset); pBookmark != nullptr)
			{
				if (m_pBkmCurrTt != pBookmark)
				{
					m_pBkmCurrTt = pBookmark;
					CPoint ptScreen = point;
					ClientToScreen(&ptScreen);

					m_stToolInfoBkm.lpszText = pBookmark->wstrDesc.data();
					TtBkmShow(true, POINT { ptScreen.x, ptScreen.y });
				}
			}
			else
				TtBkmShow(false);
		}
		else if (m_pBkmCurrTt) //If there is already bkm tooltip shown, but cursor is outside data chunks.
			TtBkmShow(false);

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

	if (nFlags == (MK_CONTROL | MK_SHIFT))
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

	const auto optHit = HitTest(point);
	if (!optHit)
		return;

	m_ullCurCursor = optHit->ullOffset;
	m_fCursorTextArea = optHit->fIsAscii;
	SetCapture();
	m_fSelectionBlock = GetAsyncKeyState(VK_MENU) < 0;

	ULONGLONG ullSelSize;
	ULONGLONG ullSelStart;

	if (m_pSelection->HasSelection() && (nFlags & MK_SHIFT))
	{
		ULONGLONG ullSelEnd;
		if (m_ullCurCursor <= m_ullLMouseClick)
		{
			ullSelStart = m_ullCurCursor;
			ullSelEnd = m_ullLMouseClick + 1;
		}
		else
		{
			ullSelStart = m_ullLMouseClick;
			ullSelEnd = m_ullCurCursor + 1;
		}
		ullSelSize = ullSelEnd - ullSelStart;
	}
	else
	{
		m_ullLMouseClick = ullSelStart = m_ullCurCursor;
		ullSelSize = 1;
	}

	m_fCursorHigh = true;
	m_fLMousePressed = true;
	m_optRMouseClick.reset();
	MakeSelection(m_ullLMouseClick, ullSelStart, ullSelSize, 1, false);
}

void CHexCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_fLMousePressed = false;
	ReleaseCapture();

	m_pScrollV->OnLButtonUp(nFlags, point);
	m_pScrollH->OnLButtonUp(nFlags, point);

	CWnd::OnLButtonUp(nFlags, point);
}

void CHexCtrl::OnLButtonDblClk(UINT /*nFlags*/, CPoint point)
{
	if (point.x < m_iSecondVertLine)
	{
		m_fOffsetAsHex = !m_fOffsetAsHex;
		WstrCapacityFill();
		UpdateInfoText();
		RecalcAll();
	}
}

void CHexCtrl::OnRButtonDown(UINT /*nFlags*/, CPoint point)
{
	const auto optHit = HitTest(point);
	m_optRMouseClick = optHit ? optHit->ullOffset : std::optional<ULONGLONG> { };
}

void CHexCtrl::OnMButtonDown(UINT /*nFlags*/, CPoint /*point*/)
{
	if (m_pDlgDataInterpret->IsWindowVisible())
		Beep(1000, 50);
}

BOOL CHexCtrl::OnCommand(WPARAM wParam, LPARAM lParam)
{
	const ULONGLONG uID = LOWORD(wParam);
	switch (uID)
	{
	case IDM_HEXCTRL_SEARCH:
		if (IsDataSet())
			m_pDlgSearch->ShowWindow(SW_SHOW);
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
		m_pBookmarks->Add(HEXBOOKMARKSTRUCT { m_pSelection->GetData() });
		break;
	case IDM_HEXCTRL_BOOKMARKS_REMOVE:
		m_pBookmarks->Remove(m_optRMouseClick.value_or(GetCaretPos()));
		m_optRMouseClick.reset();
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
	case IDM_HEXCTRL_BOOKMARKS_MANAGER:
		m_pDlgBookmarkMgr->ShowWindow(SW_SHOW);
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
	case IDM_HEXCTRL_CLIPBOARD_COPYCARRAY:
		ClipboardCopy(EClipboard::COPY_CARRAY);
		break;
	case IDM_HEXCTRL_CLIPBOARD_COPYGREPHEX:
		ClipboardCopy(EClipboard::COPY_GREPHEX);
		break;
	case IDM_HEXCTRL_CLIPBOARD_COPYPRINTSCREEN:
		ClipboardCopy(EClipboard::COPY_PRINTSCREEN);
		break;
	case IDM_HEXCTRL_CLIPBOARD_PASTEHEX:
		ClipboardPaste(EClipboard::PASTE_HEX);
		break;
	case IDM_HEXCTRL_CLIPBOARD_PASTEASCII:
		ClipboardPaste(EClipboard::PASTE_ASCII);
		break;
	case IDM_HEXCTRL_MODIFY_OPERATIONS:
		m_pDlgOpers->ShowWindow(SW_SHOW);
		break;
	case IDM_HEXCTRL_MODIFY_FILLZEROS:
		FillWithZeros();
		break;
	case IDM_HEXCTRL_MODIFY_FILLDATA:
		m_pDlgFillData->ShowWindow(SW_SHOW);
		break;
	case IDM_HEXCTRL_MODIFY_UNDO:
		Undo();
		break;
	case IDM_HEXCTRL_MODIFY_REDO:
		Redo();
		break;
	case IDM_HEXCTRL_SELECTION_MARKSTART:
		m_pSelection->SetSelectionStart(GetCaretPos());
		break;
	case IDM_HEXCTRL_SELECTION_MARKEND:
		m_pSelection->SetSelectionEnd(GetCaretPos());
		break;
	case IDM_HEXCTRL_SELECTION_SELECTALL:
		SelectAll();
		break;
	case IDM_HEXCTRL_DATAINTERPRET:
		m_pDlgDataInterpret->ShowWindow(SW_SHOW);
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
	case IDM_HEXCTRL_PRINT:
		Print();
		break;
	case IDM_HEXCTRL_ABOUT:
	{
		CHexDlgAbout dlgAbout(this);
		dlgAbout.DoModal();
	}
	break;
	default:
		//For user defined custom menu we notifying parent window.
		HEXNOTIFYSTRUCT hns { { m_hWnd, static_cast<UINT>(GetDlgCtrlID()), HEXCTRL_MSG_MENUCLICK } };
		hns.ullData = uID;
		hns.point = m_stMenuClickedPt;
		MsgWindowNotify(hns);
	}

	return CWnd::OnCommand(wParam, lParam);
}

void CHexCtrl::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	//Notify parent that we are about to display a context menu.
	HEXNOTIFYSTRUCT hns { { m_hWnd, static_cast<UINT>(GetDlgCtrlID()), HEXCTRL_MSG_CONTEXTMENU } };
	hns.point = m_stMenuClickedPt = point;
	MsgWindowNotify(hns);
	m_menuMain.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON, point.x, point.y, this);
}

void CHexCtrl::OnInitMenuPopup(CMenu* /*pPopupMenu*/, UINT /*nIndex*/, BOOL /*bSysMenu*/)
{
	//Main
	m_menuMain.EnableMenuItem(IDM_HEXCTRL_SEARCH, IsCmdAvail(EHexCmd::CMD_SEARCH) ? MF_ENABLED : MF_GRAYED);

	//Bookmarks
	m_menuMain.EnableMenuItem(IDM_HEXCTRL_BOOKMARKS_ADD, IsCmdAvail(EHexCmd::CMD_BKM_ADD) ? MF_ENABLED : MF_GRAYED);
	m_menuMain.EnableMenuItem(IDM_HEXCTRL_BOOKMARKS_REMOVE, IsCmdAvail(EHexCmd::CMD_BKM_REMOVE) ? MF_ENABLED : MF_GRAYED);
	m_menuMain.EnableMenuItem(IDM_HEXCTRL_BOOKMARKS_NEXT, IsCmdAvail(EHexCmd::CMD_BKM_NEXT) ? MF_ENABLED : MF_GRAYED);
	m_menuMain.EnableMenuItem(IDM_HEXCTRL_BOOKMARKS_PREV, IsCmdAvail(EHexCmd::CMD_BKM_PREV) ? MF_ENABLED : MF_GRAYED);
	m_menuMain.EnableMenuItem(IDM_HEXCTRL_BOOKMARKS_CLEARALL, IsCmdAvail(EHexCmd::CMD_BKM_CLEARALL) ? MF_ENABLED : MF_GRAYED);
	m_menuMain.EnableMenuItem(IDM_HEXCTRL_BOOKMARKS_MANAGER, IsCmdAvail(EHexCmd::CMD_BKM_MANAGER) ? MF_ENABLED : MF_GRAYED);

	//Clipboard
	m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLIPBOARD_COPYHEX, IsCmdAvail(EHexCmd::CMD_CLIPBOARD_COPY_HEX) ? MF_ENABLED : MF_GRAYED);
	m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLIPBOARD_COPYHEXLE, IsCmdAvail(EHexCmd::CMD_CLIPBOARD_COPY_HEXLE) ? MF_ENABLED : MF_GRAYED);
	m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLIPBOARD_COPYHEXFORMATTED, IsCmdAvail(EHexCmd::CMD_CLIPBOARD_COPY_HEXFMT) ? MF_ENABLED : MF_GRAYED);
	m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLIPBOARD_COPYASCII, IsCmdAvail(EHexCmd::CMD_CLIPBOARD_COPY_ASCII) ? MF_ENABLED : MF_GRAYED);
	m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLIPBOARD_COPYBASE64, IsCmdAvail(EHexCmd::CMD_CLIPBOARD_COPY_BASE64) ? MF_ENABLED : MF_GRAYED);
	m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLIPBOARD_COPYCARRAY, IsCmdAvail(EHexCmd::CMD_CLIPBOARD_COPY_CARR) ? MF_ENABLED : MF_GRAYED);
	m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLIPBOARD_COPYGREPHEX, IsCmdAvail(EHexCmd::CMD_CLIPBOARD_COPY_GREPHEX) ? MF_ENABLED : MF_GRAYED);
	m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLIPBOARD_COPYPRINTSCREEN, IsCmdAvail(EHexCmd::CMD_CLIPBOARD_COPY_PRNTSCRN) ? MF_ENABLED : MF_GRAYED);
	m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLIPBOARD_PASTEHEX, IsCmdAvail(EHexCmd::CMD_CLIPBOARD_PASTE_HEX) ? MF_ENABLED : MF_GRAYED);
	m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLIPBOARD_PASTEASCII, IsCmdAvail(EHexCmd::CMD_CLIPBOARD_PASTE_ASCII) ? MF_ENABLED : MF_GRAYED);

	//Modify
	m_menuMain.EnableMenuItem(IDM_HEXCTRL_MODIFY_FILLZEROS, IsCmdAvail(EHexCmd::CMD_MODIFY_FILLZEROS) ? MF_ENABLED : MF_GRAYED);
	m_menuMain.EnableMenuItem(IDM_HEXCTRL_MODIFY_FILLDATA, IsCmdAvail(EHexCmd::CMD_MODIFY_FILLDATA) ? MF_ENABLED : MF_GRAYED);
	m_menuMain.EnableMenuItem(IDM_HEXCTRL_MODIFY_OPERATIONS, IsCmdAvail(EHexCmd::CMD_MODIFY_OPERS) ? MF_ENABLED : MF_GRAYED);
	m_menuMain.EnableMenuItem(IDM_HEXCTRL_MODIFY_UNDO, IsCmdAvail(EHexCmd::CMD_MODIFY_UNDO) ? MF_ENABLED : MF_GRAYED);
	m_menuMain.EnableMenuItem(IDM_HEXCTRL_MODIFY_REDO, IsCmdAvail(EHexCmd::CMD_MODIFY_REDO) ? MF_ENABLED : MF_GRAYED);

	//Selection
	m_menuMain.EnableMenuItem(IDM_HEXCTRL_SELECTION_MARKSTART, IsCmdAvail(EHexCmd::CMD_SEL_MARKSTART) ? MF_ENABLED : MF_GRAYED);
	m_menuMain.EnableMenuItem(IDM_HEXCTRL_SELECTION_MARKEND, IsCmdAvail(EHexCmd::CMD_SEL_MARKEND) ? MF_ENABLED : MF_GRAYED);
	m_menuMain.EnableMenuItem(IDM_HEXCTRL_SELECTION_SELECTALL, IsCmdAvail(EHexCmd::CMD_SEL_SELECTALL) ? MF_ENABLED : MF_GRAYED);

	//Data interpreter
	m_menuMain.EnableMenuItem(IDM_HEXCTRL_DATAINTERPRET, IsCmdAvail(EHexCmd::CMD_DATAINTERPRET) ? MF_ENABLED : MF_GRAYED);
}

void CHexCtrl::OnKeyUp(UINT /*nChar*/, UINT /*nRepCnt*/, UINT /*nFlags*/)
{
	//If some key was previously pressed for some time, and now is released.
	//Inspecting current caret position.
	if (m_fKeyDownAtm)
	{
		m_fKeyDownAtm = false;
		m_pDlgDataInterpret->InspectOffset(GetCaretPos());
	}
}

void CHexCtrl::OnKeyDown(UINT nChar, UINT /*nRepCnt*/, UINT nFlags)
{
	//Bit 14 indicates that the key was pressed continuously.
	//0x4000 == 0100 0000 0000 0000.
	if (nFlags & 0x4000)
		m_fKeyDownAtm = true;

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
			CaretMoveRight();
			break;
		case VK_LEFT:
			CaretMoveLeft();
			break;
		case VK_DOWN:
			CaretMoveDown();
			break;
		case VK_UP:
			CaretMoveUp();
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
	m_optRMouseClick.reset(); //Reset right mouse click.
}

void CHexCtrl::OnSysKeyDown(UINT nChar, UINT /*nRepCnt*/, UINT /*nFlags*/)
{
	WPARAM wParam { };
	switch (nChar)
	{
	case '1':
		wParam = IDM_HEXCTRL_SELECTION_MARKSTART;
		break;
	case '2':
		wParam = IDM_HEXCTRL_SELECTION_MARKEND;
		break;
	}
	if (wParam)
		SendMessageW(WM_COMMAND, wParam, 0);
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
		wParam = IDM_HEXCTRL_SEARCH;
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
		wParam = IDM_HEXCTRL_SELECTION_SELECTALL;
		break;
	case 'Z':
		wParam = IDM_HEXCTRL_MODIFY_UNDO;
		break;
	case 'Y':
		wParam = IDM_HEXCTRL_MODIFY_REDO;
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
	ULONGLONG ullSelStart = m_pSelection->GetSelectionStart();
	ULONGLONG ullSelSize = m_pSelection->GetSelectionSize();

	if (m_fMutable)
	{
		if (m_ullCaretPos == m_ullLMouseClick
			|| m_ullCaretPos == ullSelStart
			|| m_ullCaretPos == m_pSelection->GetSelectionEnd())
		{
			if (ullSelStart == m_ullLMouseClick && ullSelSize > 1)
			{
				ullClick = ullStart = m_ullLMouseClick;
				ullSize = ullSelSize - 1;
			}
			else
			{
				ullClick = m_ullLMouseClick;
				ullStart = ullSelStart - 1;
				ullSize = ullSelSize + 1;
			}
		}
		else
		{
			ullClick = ullStart = m_ullCaretPos;
			ullSize = 1;
		}
	}
	else if (m_pSelection->HasSelection())
	{
		if (ullSelStart == m_ullLMouseClick && ullSelSize > 1)
		{
			ullClick = ullStart = m_ullLMouseClick;
			ullSize = ullSelSize - 1;
		}
		else
		{
			ullClick = m_ullLMouseClick;
			ullStart = ullSelStart - 1;
			ullSize = ullSelSize + 1;
		}
	}

	if (ullSize > 0)
		MakeSelection(ullClick, ullStart, ullSize, 1);
}

void CHexCtrl::OnKeyDownShiftRight()
{
	ULONGLONG ullClick { }, ullStart { }, ullSize = 0;
	ULONGLONG ullSelStart = m_pSelection->GetSelectionStart();
	ULONGLONG ullSelSize = m_pSelection->GetSelectionSize();

	if (m_fMutable)
	{
		if (m_ullCaretPos == m_ullLMouseClick || m_ullCaretPos == ullSelStart
			|| m_ullCaretPos == m_pSelection->GetSelectionEnd())
		{
			if (ullSelStart == m_ullLMouseClick)
			{
				ullClick = ullStart = m_ullLMouseClick;
				ullSize = ullSelSize + 1;
			}
			else
			{
				ullClick = m_ullLMouseClick;
				ullStart = ullSelStart + 1;
				ullSize = ullSelSize - 1;
			}
		}
		else
		{
			ullClick = ullStart = m_ullCaretPos;
			ullSize = 1;
		}
	}
	else if (m_pSelection->HasSelection())
	{
		if (ullSelStart == m_ullLMouseClick)
		{
			ullClick = ullStart = m_ullLMouseClick;
			ullSize = ullSelSize + 1;
		}
		else
		{
			ullClick = m_ullLMouseClick;
			ullStart = ullSelStart + 1;
			ullSize = ullSelSize - 1;
		}
	}

	if (ullSize > 0)
		MakeSelection(ullClick, ullStart, ullSize, 1);
}

void CHexCtrl::OnKeyDownShiftUp()
{
	ULONGLONG ullClick { }, ullStart { 0 }, ullSize { 0 };
	ULONGLONG ullSelStart = m_pSelection->GetSelectionStart();
	ULONGLONG ullSelSize = m_pSelection->GetSelectionSize();

	if (m_fMutable)
	{
		if (m_ullCaretPos == m_ullLMouseClick || m_ullCaretPos == ullSelStart
			|| m_ullCaretPos == m_pSelection->GetSelectionEnd())
		{
			if (ullSelStart == 0)
				return;

			if (ullSelStart < m_ullLMouseClick)
			{
				ullClick = m_ullLMouseClick;
				if (ullSelStart < m_dwCapacity)
				{
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
				ullClick = m_ullLMouseClick;
				ullStart = ullSelSize >= m_dwCapacity ? m_ullLMouseClick : m_ullLMouseClick - m_dwCapacity;
				ullSize = ullSelSize >= m_dwCapacity ? ullSelSize - m_dwCapacity : m_dwCapacity + 1;
				ullSize = ullSize ? ullSize : 1;
			}
		}
		else
		{
			ullClick = ullStart = m_ullCaretPos;
			ullSize = 1;
		}
	}
	else if (m_pSelection->HasSelection())
	{
		if (ullSelStart == 0)
			return;

		if (ullSelStart < m_ullLMouseClick)
		{
			ullClick = m_ullLMouseClick;
			if (ullSelStart < m_dwCapacity)
			{
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
			ullClick = m_ullLMouseClick;
			ullStart = (ullSelSize >= m_dwCapacity) ? m_ullLMouseClick : m_ullLMouseClick - m_dwCapacity;
			ullSize = ullSelSize >= m_dwCapacity ? ullSelSize - m_dwCapacity : static_cast<ULONGLONG>(m_dwCapacity) + 1;
			ullSize = ullSize ? ullSize : 1;
		}
	}

	if (ullSize > 0)
		MakeSelection(ullClick, ullStart, ullSize, 1);
}

void CHexCtrl::OnKeyDownShiftDown()
{
	ULONGLONG ullClick { }, ullStart { }, ullSize = 0;
	ULONGLONG ullSelStart = m_pSelection->GetSelectionStart();
	ULONGLONG ullSelSize = m_pSelection->GetSelectionSize();
	if (m_fMutable)
	{
		if (m_ullCaretPos == m_ullLMouseClick
			|| m_ullCaretPos == ullSelStart
			|| m_ullCaretPos == m_pSelection->GetSelectionEnd())
		{
			if (ullSelStart == m_ullLMouseClick)
			{
				ullClick = ullStart = m_ullLMouseClick;
				ullSize = ullSelSize + m_dwCapacity;
			}
			else if (ullSelStart < m_ullLMouseClick)
			{
				ullClick = m_ullLMouseClick;
				ullStart = ullSelSize > m_dwCapacity ? ullSelStart + m_dwCapacity : m_ullLMouseClick;
				ullSize = ullSelSize >= m_dwCapacity ? ullSelSize - m_dwCapacity : m_dwCapacity;
				ullSize = ullSize ? ullSize : 1;
			}
		}
		else
		{
			ullClick = ullStart = m_ullCaretPos;
			ullSize = 1;
		}
	}
	else if (m_pSelection->HasSelection())
	{
		if (ullSelStart == m_ullLMouseClick)
		{
			ullClick = ullStart = m_ullLMouseClick;
			ullSize = ullSelSize + m_dwCapacity;
		}
		else if (ullSelStart < m_ullLMouseClick)
		{
			ullClick = m_ullLMouseClick;
			ullStart = ullSelSize > m_dwCapacity ? ullSelStart + m_dwCapacity : m_ullLMouseClick;
			ullSize = ullSelSize >= m_dwCapacity ? ullSelSize - m_dwCapacity : m_dwCapacity;
			ullSize = ullSize ? ullSize : 1;
		}
	}

	if (ullSize > 0)
		MakeSelection(ullClick, ullStart, ullSize, 1);
}

void CHexCtrl::OnChar(UINT nChar, UINT /*nRepCnt*/, UINT /*nFlags*/)
{
	if (!m_fMutable || (GetKeyState(VK_CONTROL) < 0))
		return;

	MODIFYSTRUCT hms;
	hms.vecSpan.emplace_back(HEXSPANSTRUCT { m_ullCaretPos, 1 });
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

		BYTE chByteCurr = GetData<BYTE>(m_ullCaretPos);
		if (m_fCursorHigh)
			chByte = (chByte << 4) | (chByteCurr & 0x0F);
		else
			chByte = (chByte & 0x0F) | (chByteCurr & 0xF0);
	}

	hms.pData = reinterpret_cast<std::byte*>(&chByte);
	Modify(hms);
	CaretMoveRight();
}

UINT CHexCtrl::OnGetDlgCode()
{
	return DLGC_WANTALLKEYS;
}

void CHexCtrl::OnVScroll(UINT /*nSBCode*/, UINT /*nPos*/, CScrollBar* /*pScrollBar*/)
{
	bool fRedraw = { false };
	if (m_fHighLatency)
	{
		fRedraw = m_pScrollV->IsThumbReleased();
		TtOffsetShow(!fRedraw);
	}
	else if (m_pScrollV->GetScrollPosDelta() != 0)
		fRedraw = true;

	if (fRedraw)
	{
		RedrawWindow();
		MsgWindowNotify(HEXCTRL_MSG_VIEWCHANGE); //Indicates to parent that view has changed.
	}
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

	CRect rcClient;
	GetClientRect(rcClient);
	//To prevent drawing in too small window (can cause hangs).
	if (rcClient.Height() < m_iHeightTopRect + m_iHeightBottomOffArea)
		return;

	m_iHeightClientArea = rcClient.Height();
	m_iWidthClientArea = rcClient.Width();

	//Drawing through CMemDC to avoid flickering.
	CMemDC memDC(dc, rcClient);
	auto pDC = &memDC.GetDC();
	const auto ullStartLine = GetTopLine();
	const auto ullEndLine = GetBottomLine();

	DrawWindow(pDC, &m_fontMain, &m_fontInfo); //Draw the window with all layouts.
	DrawOffsets(pDC, &m_fontMain, ullStartLine, ullEndLine);
	DrawHexAscii(pDC, &m_fontMain, ullStartLine, ullEndLine);
	DrawBookmarks(pDC, &m_fontMain, ullStartLine, ullEndLine);
	DrawCustomColors(pDC, &m_fontMain, ullStartLine, ullEndLine);
	DrawSelection(pDC, &m_fontMain, ullStartLine, ullEndLine);
	DrawSelHighlight(pDC, &m_fontMain, ullStartLine, ullEndLine);
	DrawCursor(pDC, &m_fontMain, ullStartLine, ullEndLine);
	DrawDataInterpret(pDC, &m_fontMain, ullStartLine, ullEndLine);
	DrawSectorLines(pDC, ullStartLine, ullEndLine);
}

void CHexCtrl::DrawWindow(CDC* pDC, CFont* pFont, CFont* pFontInfo)
{
	const int iScrollH = static_cast<int>(m_pScrollH->GetScrollPos());
	const auto iSecondHorizLine = m_iStartWorkAreaY - 1;
	const auto iThirdHorizLine = m_iFirstHorizLine + m_iHeightClientArea - m_iHeightBottomOffArea;
	const auto iFourthHorizLine = iThirdHorizLine + m_iHeightBottomRect;

	CRect rcWnd(m_iFirstVertLine, m_iFirstHorizLine,
		m_iFirstVertLine + m_iWidthClientArea, m_iFirstHorizLine + m_iHeightClientArea);

	pDC->FillSolidRect(rcWnd, m_stColor.clrBk);
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

	//«Offset» text.
	CRect rcOffset(m_iFirstVertLine - iScrollH, m_iFirstHorizLine, m_iSecondVertLine - iScrollH, iSecondHorizLine);
	pDC->SelectObject(pFont);
	pDC->SetTextColor(m_stColor.clrTextCaption);
	pDC->SetBkColor(m_stColor.clrBk);
	pDC->DrawTextW(L"Offset", 6, rcOffset, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	//Capacity numbers.
	ExtTextOutW(pDC->m_hDC, m_iIndentFirstHexChunk - iScrollH, m_iFirstHorizLine + m_iIndentTextCapacityY, NULL, nullptr,
		m_wstrCapacity.data(), static_cast<UINT>(m_wstrCapacity.size()), nullptr);

	//"ASCII" text.
	CRect rcAscii(m_iThirdVertLine - iScrollH, m_iFirstHorizLine, m_iFourthVertLine - iScrollH, iSecondHorizLine);
	pDC->DrawTextW(L"ASCII", 5, rcAscii, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	//Bottom "Info" rect.
	CRect rcInfo(m_iFirstVertLine + 1 - iScrollH, iThirdHorizLine + 1, m_iFourthVertLine, iFourthHorizLine); //Fill bottom rcClient until iFourthHorizLine.
	pDC->FillSolidRect(rcInfo, m_stColor.clrBkInfoRect);
	rcInfo.left = m_iFirstVertLine + 5; //Draw text beginning with little indent.
	rcInfo.right = m_iFirstVertLine + m_iWidthClientArea; //Draw text to the end of the client area, even if it passes iFourthHorizLine.
	pDC->SelectObject(pFontInfo);
	pDC->SetTextColor(m_stColor.clrTextInfoRect);
	pDC->SetBkColor(m_stColor.clrBkInfoRect);
	pDC->DrawTextW(m_wstrInfo.data(), static_cast<int>(m_wstrInfo.size()), rcInfo, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

void CHexCtrl::DrawOffsets(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, ULONGLONG ullEndLine)
{
	const int iScrollH = static_cast<int>(m_pScrollH->GetScrollPos());
	int iLine = 0; //Current line to print.

	for (auto iterLines = ullStartLine; iterLines < ullEndLine; ++iterLines, ++iLine)
	{
		//Drawing offset with bk color depending on selection range.
		COLORREF clrTextOffset, clrBkOffset;
		if (m_pSelection->HitTestRange({ iterLines * m_dwCapacity, m_dwCapacity }))
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
		wchar_t pwszOffset[32]; //To be enough for max as Hex and as Decimals.
		UllToWchars(iterLines * m_dwCapacity, pwszOffset, static_cast<size_t>(m_dwOffsetBytes), m_fOffsetAsHex);
		pDC->SelectObject(pFont);
		pDC->SetTextColor(clrTextOffset);
		pDC->SetBkColor(clrBkOffset);
		ExtTextOutW(pDC->m_hDC, m_iFirstVertLine + m_sizeLetter.cx - iScrollH, m_iStartWorkAreaY + (m_sizeLetter.cy * iLine),
			NULL, nullptr, pwszOffset, m_dwOffsetDigits, nullptr);
	}
}

void CHexCtrl::DrawHexAscii(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, ULONGLONG ullEndLine)
{
	std::vector<POLYTEXTW> vecPolyHex, vecPolyAscii;
	std::list<std::wstring> listWstrHex, listWstrAscii;
	const int iScrollH = static_cast<int>(m_pScrollH->GetScrollPos());
	int iLine = 0; //Current line to print.

	const auto ullOffset = ullStartLine * m_dwCapacity;      //Offset of the visible data to print.
	auto iSize = (ullEndLine - ullStartLine) * m_dwCapacity; //Size of the visible data to print.
	if ((ullOffset + iSize) > m_ullDataSize)
		iSize = m_ullDataSize - ullOffset;
	auto pData = reinterpret_cast<PBYTE>(GetData({ ullOffset, iSize })); //Pointer to data to print.

	//Loop for printing Hex chunks and Ascii chars line by line.
	for (auto iterLines = ullStartLine; iterLines < ullEndLine; ++iterLines, ++iLine)
	{
		//Hex, Ascii, Bookmarks, Selection, Datainterpret, Cursor wstrings to print.
		std::wstring wstrHexToPrint { }, wstrAsciiToPrint { };

		const auto iHexPosToPrintX = m_iIndentFirstHexChunk - iScrollH;
		const auto iAsciiPosToPrintX = m_iIndentAscii - iScrollH;
		const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeLetter.cy * iLine; //Hex and Ascii the same.
		auto ullByteToPrint = iterLines * m_dwCapacity;

		//Main loop for printing Hex chunks and Ascii chars.
		for (unsigned iterChunks = 0; iterChunks < m_dwCapacity && ullByteToPrint < m_ullDataSize; ++iterChunks, ++pData, ++ullByteToPrint)
		{
			//Hex chunk to print.
			const auto chByteToPrint = *pData; //Get current byte to print.
			const wchar_t pwszHexToPrint[2] { g_pwszHexMap[(chByteToPrint >> 4) & 0x0F], g_pwszHexMap[chByteToPrint & 0x0F] };
			wstrHexToPrint += pwszHexToPrint[0];
			wstrHexToPrint += pwszHexToPrint[1];

			//Additional space between grouped Hex chunks.
			if (((iterChunks + 1) % static_cast<DWORD>(m_enShowMode)) == 0 && iterChunks < (m_dwCapacity - 1))
				wstrHexToPrint += L" ";

			//Additional space between capacity halves, only with ASBYTE representation.
			if (m_enShowMode == EHexShowMode::ASBYTE && iterChunks == (m_dwCapacityBlockSize - 1))
				wstrHexToPrint += L"  ";

			//Ascii to print.
			wchar_t wchAscii = chByteToPrint;
			if (wchAscii < 32 || wchAscii >= 0x7f) //For non printable Ascii just print a dot.
				wchAscii = '.';
			wstrAsciiToPrint += wchAscii;
		}

		//Hex Poly.
		listWstrHex.emplace_back(std::move(wstrHexToPrint));
		vecPolyHex.emplace_back(POLYTEXTW { iHexPosToPrintX, iPosToPrintY,
			static_cast<UINT>(listWstrHex.back().size()), listWstrHex.back().data(), 0, { }, nullptr });

		//Ascii Poly.
		listWstrAscii.emplace_back(std::move(wstrAsciiToPrint));
		vecPolyAscii.emplace_back(POLYTEXTW { iAsciiPosToPrintX, iPosToPrintY,
			static_cast<UINT>(listWstrAscii.back().size()), listWstrAscii.back().data(), 0, { }, nullptr });
	}

	//Hex printing.
	pDC->SelectObject(pFont);
	pDC->SetTextColor(m_stColor.clrTextHex);
	pDC->SetBkColor(m_stColor.clrBk);
	PolyTextOutW(pDC->m_hDC, vecPolyHex.data(), static_cast<UINT>(vecPolyHex.size()));

	//Ascii printing.
	pDC->SetTextColor(m_stColor.clrTextAscii);
	PolyTextOutW(pDC->m_hDC, vecPolyAscii.data(), static_cast<UINT>(vecPolyAscii.size()));
}

void CHexCtrl::DrawBookmarks(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, ULONGLONG ullEndLine)
{
	//Struct for Bookmarks.
	struct BOOKMARKS
	{
		POLYTEXTW stPoly { };
		COLORREF  clrBk { };
		COLORREF  clrText { };
	};

	std::vector<BOOKMARKS> vecBookmarksHex, vecBookmarksAscii;
	std::list<std::wstring> listWstrBookmarkHex, listWstrBookmarkAscii;
	int iLine = 0; //Current line to print.

	const auto ullOffset = ullStartLine * m_dwCapacity;      //Offset of the visible data to print.
	auto iSize = (ullEndLine - ullStartLine) * m_dwCapacity; //Size of the visible data to print.
	if ((ullOffset + iSize) > m_ullDataSize)
		iSize = m_ullDataSize - ullOffset;
	auto pData = reinterpret_cast<PBYTE>(GetData({ ullOffset, iSize })); //Pointer to data to print.

	//Loop for printing Hex chunks and Ascii chars line by line.
	for (auto iterLines = ullStartLine; iterLines < ullEndLine; ++iterLines, ++iLine)
	{
		std::wstring wstrHexBookmarkToPrint { }, wstrAsciiBookmarkToPrint { }; //Bookmarks to print.
		int iBookmarkHexPosToPrintX { 0x7FFFFFFF }, iBookmarkAsciiPosToPrintX { };
		bool fBookmark { false };  //Flag to show current Bookmark in current Hex presence.
		const HEXBOOKMARKSTRUCT* pBookmarkCurr { };
		const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeLetter.cy * iLine; //Hex and Ascii the same.
		auto ullByteToPrint = iterLines * m_dwCapacity;

		//Main loop for printing Hex chunks and Ascii chars.
		for (unsigned iterChunks = 0; iterChunks < m_dwCapacity && ullByteToPrint < m_ullDataSize; ++iterChunks, ++pData, ++ullByteToPrint)
		{
			//Bookmarks.
			if (auto pBookmark = m_pBookmarks->HitTest(ullByteToPrint); pBookmark != nullptr)
			{
				//Hex chunk to print.
				const auto chByteToPrint = *pData; //Get current byte to print.
				const wchar_t pwszHexToPrint[2] { g_pwszHexMap[(chByteToPrint >> 4) & 0x0F], g_pwszHexMap[chByteToPrint & 0x0F] };

				//Ascii to print.
				wchar_t wchAscii = chByteToPrint;
				if (wchAscii < 32 || wchAscii >= 0x7f) //For non printable Ascii just print a dot.
					wchAscii = '.';

				//If it's nested bookmark.
				if (pBookmarkCurr != nullptr && pBookmarkCurr != pBookmark)
				{
					if (!wstrHexBookmarkToPrint.empty()) //Only adding spaces if there are chars beforehead.
					{
						if ((iterChunks % static_cast<DWORD>(m_enShowMode)) == 0)
							wstrHexBookmarkToPrint += L" ";

						//Additional space between capacity halves, only with ASBYTE representation.
						if (m_enShowMode == EHexShowMode::ASBYTE && iterChunks == m_dwCapacityBlockSize)
							wstrHexBookmarkToPrint += L"  ";
					}

					//Hex bookmarks Poly.
					listWstrBookmarkHex.emplace_back(std::move(wstrHexBookmarkToPrint));
					vecBookmarksHex.emplace_back(BOOKMARKS { POLYTEXTW { iBookmarkHexPosToPrintX, iPosToPrintY,
						static_cast<UINT>(listWstrBookmarkHex.back().size()), listWstrBookmarkHex.back().data(), 0, { }, nullptr },
						pBookmarkCurr->clrBk, pBookmarkCurr->clrText });

					//Ascii bookmarks Poly.
					listWstrBookmarkAscii.emplace_back(std::move(wstrAsciiBookmarkToPrint));
					vecBookmarksAscii.emplace_back(BOOKMARKS { POLYTEXTW { iBookmarkAsciiPosToPrintX, iPosToPrintY,
						static_cast<UINT>(listWstrBookmarkAscii.back().size()), listWstrBookmarkAscii.back().data(), 0, { }, nullptr },
						pBookmarkCurr->clrBk, pBookmarkCurr->clrText });

					iBookmarkHexPosToPrintX = 0x7FFFFFFF;
				}
				pBookmarkCurr = pBookmark;

				if (iBookmarkHexPosToPrintX == 0x7FFFFFFF) //For just one time exec.
				{
					int iCy;
					HexChunkPoint(ullByteToPrint, iBookmarkHexPosToPrintX, iCy);
					AsciiChunkPoint(ullByteToPrint, iBookmarkAsciiPosToPrintX, iCy);
				}

				if (!wstrHexBookmarkToPrint.empty()) //Only adding spaces if there are chars beforehead.
				{
					if ((iterChunks % static_cast<DWORD>(m_enShowMode)) == 0)
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
				//So, if there already were bookmarked bytes in the current line, we Poly them.
				//Same Poly mechanism presents at the end of the current (iterLines) loop,
				//to Poly bookmarks that end at the line's end.

				//Hex bookmarks Poly.
				listWstrBookmarkHex.emplace_back(std::move(wstrHexBookmarkToPrint));
				vecBookmarksHex.emplace_back(BOOKMARKS { POLYTEXTW { iBookmarkHexPosToPrintX, iPosToPrintY,
					static_cast<UINT>(listWstrBookmarkHex.back().size()), listWstrBookmarkHex.back().data(), 0, { }, nullptr },
					pBookmarkCurr->clrBk, pBookmarkCurr->clrText });

				//Ascii bookmarks Poly.
				listWstrBookmarkAscii.emplace_back(std::move(wstrAsciiBookmarkToPrint));
				vecBookmarksAscii.emplace_back(BOOKMARKS { POLYTEXTW { iBookmarkAsciiPosToPrintX, iPosToPrintY,
					static_cast<UINT>(listWstrBookmarkAscii.back().size()), listWstrBookmarkAscii.back().data(), 0, { }, nullptr },
					pBookmarkCurr->clrBk, pBookmarkCurr->clrText });

				iBookmarkHexPosToPrintX = 0x7FFFFFFF;
				fBookmark = false;
				pBookmarkCurr = nullptr;
			}
		}

		//Bookmarks Poly.
		if (!wstrHexBookmarkToPrint.empty())
		{
			//Hex bookmarks Poly.
			listWstrBookmarkHex.emplace_back(std::move(wstrHexBookmarkToPrint));
			vecBookmarksHex.emplace_back(BOOKMARKS { POLYTEXTW { iBookmarkHexPosToPrintX, iPosToPrintY,
				static_cast<UINT>(listWstrBookmarkHex.back().size()), listWstrBookmarkHex.back().data(), 0, { }, nullptr },
				pBookmarkCurr->clrBk, pBookmarkCurr->clrText });

			//Ascii bookmarks Poly.
			listWstrBookmarkAscii.emplace_back(std::move(wstrAsciiBookmarkToPrint));
			vecBookmarksAscii.emplace_back(BOOKMARKS { POLYTEXTW { iBookmarkAsciiPosToPrintX, iPosToPrintY,
				static_cast<UINT>(listWstrBookmarkAscii.back().size()), listWstrBookmarkAscii.back().data(), 0, { }, nullptr },
				pBookmarkCurr->clrBk, pBookmarkCurr->clrText });
		}
	}

	//Bookmarks printing.
	if (!vecBookmarksHex.empty())
	{
		pDC->SelectObject(pFont);
		size_t index { 0 }; //Index for vecBookmarksAscii. Its size is always the same as the vecBookmarksHex.
		for (const auto& iter : vecBookmarksHex)
		{
			pDC->SetTextColor(iter.clrText);
			pDC->SetBkColor(iter.clrBk);
			PolyTextOutW(pDC->m_hDC, &iter.stPoly, 1);

			//Ascii bookmark printing.
			PolyTextOutW(pDC->m_hDC, &vecBookmarksAscii[index++].stPoly, 1);
		}
	}
}

void CHexCtrl::DrawCustomColors(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, ULONGLONG ullEndLine)
{
	if (m_pHexVirtColors == nullptr)
		return;

	//Struct for colors.
	struct COLORS
	{
		POLYTEXTW stPoly { };
		HEXCOLOR clr { };
	};

	std::vector<COLORS> vecColorsHex, vecColorsAscii;
	std::list<std::wstring> listWstrColorsHex, listWstrColorsAscii;
	int iLine = 0; //Current line to print.

	const auto ullOffset = ullStartLine * m_dwCapacity;      //Offset of the visible data to print.
	auto iSize = (ullEndLine - ullStartLine) * m_dwCapacity; //Size of the visible data to print.
	if ((ullOffset + iSize) > m_ullDataSize)
		iSize = m_ullDataSize - ullOffset;
	auto pData = reinterpret_cast<PBYTE>(GetData({ ullOffset, iSize })); //Pointer to data to print.

	//Loop for printing Hex chunks and Ascii chars line by line.
	for (auto iterLines = ullStartLine; iterLines < ullEndLine; ++iterLines, ++iLine)
	{
		std::wstring wstrHexColorToPrint { }, wstrAsciiColorToPrint { }; //Colors to print.
		int iColorHexPosToPrintX { 0x7FFFFFFF }, iColorAsciiPosToPrintX { };
		bool fColor { false };  //Flag to show current Color in current Hex presence.
		std::optional<HEXCOLOR> optColorCurr { }; //Current color.
		const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeLetter.cy * iLine; //Hex and Ascii the same.
		auto ullByteToPrint = iterLines * m_dwCapacity;

		//Main loop for printing Hex chunks and Ascii chars.
		for (unsigned iterChunks = 0; iterChunks < m_dwCapacity && ullByteToPrint < m_ullDataSize; ++iterChunks, ++pData, ++ullByteToPrint)
		{
			//Colors.
			if (auto pColor = m_pHexVirtColors->GetColor(ullByteToPrint); pColor != nullptr)
			{
				//Hex chunk to print.
				const auto chByteToPrint = *pData; //Get current byte to print.
				const wchar_t pwszHexToPrint[2] { g_pwszHexMap[(chByteToPrint >> 4) & 0x0F], g_pwszHexMap[chByteToPrint & 0x0F] };

				//Ascii to print.
				wchar_t wchAscii = chByteToPrint;
				if (wchAscii < 32 || wchAscii >= 0x7f) //For non printable Ascii just print a dot.
					wchAscii = '.';

				//If it's different color.
				if (optColorCurr && (optColorCurr->clrBk != pColor->clrBk || optColorCurr->clrText != pColor->clrText))
				{
					if (!wstrHexColorToPrint.empty()) //Only adding spaces if there are chars beforehead.
					{
						if ((iterChunks % static_cast<DWORD>(m_enShowMode)) == 0)
							wstrHexColorToPrint += L" ";

						//Additional space between capacity halves, only with ASBYTE representation.
						if (m_enShowMode == EHexShowMode::ASBYTE && iterChunks == m_dwCapacityBlockSize)
							wstrHexColorToPrint += L"  ";
					}

					//Hex colors Poly.
					listWstrColorsHex.emplace_back(std::move(wstrHexColorToPrint));
					vecColorsHex.emplace_back(COLORS { POLYTEXTW { iColorHexPosToPrintX, iPosToPrintY,
						static_cast<UINT>(listWstrColorsHex.back().size()), listWstrColorsHex.back().data(), 0, { }, nullptr },
						*optColorCurr });

					//Ascii colors Poly.
					listWstrColorsAscii.emplace_back(std::move(wstrAsciiColorToPrint));
					vecColorsAscii.emplace_back(COLORS { POLYTEXTW { iColorAsciiPosToPrintX, iPosToPrintY,
						static_cast<UINT>(listWstrColorsAscii.back().size()), listWstrColorsAscii.back().data(), 0, { }, nullptr },
						*optColorCurr });

					iColorHexPosToPrintX = 0x7FFFFFFF;
				}
				optColorCurr = *pColor;

				if (iColorHexPosToPrintX == 0x7FFFFFFF) //For just one time exec.
				{
					int iCy;
					HexChunkPoint(ullByteToPrint, iColorHexPosToPrintX, iCy);
					AsciiChunkPoint(ullByteToPrint, iColorAsciiPosToPrintX, iCy);
				}

				if (!wstrHexColorToPrint.empty()) //Only adding spaces if there are chars beforehead.
				{
					if ((iterChunks % static_cast<DWORD>(m_enShowMode)) == 0)
						wstrHexColorToPrint += L" ";

					//Additional space between capacity halves, only with ASBYTE representation.
					if (m_enShowMode == EHexShowMode::ASBYTE && iterChunks == m_dwCapacityBlockSize)
						wstrHexColorToPrint += L"  ";
				}
				wstrHexColorToPrint += pwszHexToPrint[0];
				wstrHexColorToPrint += pwszHexToPrint[1];
				wstrAsciiColorToPrint += wchAscii;
				fColor = true;
			}
			else if (fColor)
			{
				//There can be multiple colors in one line. 
				//So, if there already were colored bytes in the current line, we Poly them.
				//Same Poly mechanism presents at the end of the current (iterLines) loop,
				//to Poly colors that end at the line's end.

				//Hex colors Poly.
				listWstrColorsHex.emplace_back(std::move(wstrHexColorToPrint));
				vecColorsHex.emplace_back(COLORS { POLYTEXTW { iColorHexPosToPrintX, iPosToPrintY,
					static_cast<UINT>(listWstrColorsHex.back().size()), listWstrColorsHex.back().data(), 0, { }, nullptr },
					*optColorCurr });

				//Ascii colors Poly.
				listWstrColorsAscii.emplace_back(std::move(wstrAsciiColorToPrint));
				vecColorsAscii.emplace_back(COLORS { POLYTEXTW { iColorAsciiPosToPrintX, iPosToPrintY,
					static_cast<UINT>(listWstrColorsAscii.back().size()), listWstrColorsAscii.back().data(), 0, { }, nullptr },
					*optColorCurr });

				iColorHexPosToPrintX = 0x7FFFFFFF;
				fColor = false;
				optColorCurr.reset();
			}
		}

		//Colors Poly.
		if (!wstrHexColorToPrint.empty())
		{
			//Hex colors Poly.
			listWstrColorsHex.emplace_back(std::move(wstrHexColorToPrint));
			vecColorsHex.emplace_back(COLORS { POLYTEXTW { iColorHexPosToPrintX, iPosToPrintY,
				static_cast<UINT>(listWstrColorsHex.back().size()), listWstrColorsHex.back().data(), 0, { }, nullptr },
				*optColorCurr });

			//Ascii colors Poly.
			listWstrColorsAscii.emplace_back(std::move(wstrAsciiColorToPrint));
			vecColorsAscii.emplace_back(COLORS { POLYTEXTW { iColorAsciiPosToPrintX, iPosToPrintY,
				static_cast<UINT>(listWstrColorsAscii.back().size()), listWstrColorsAscii.back().data(), 0, { }, nullptr },
				*optColorCurr });
		}
	}

	//Colors printing.
	if (!vecColorsHex.empty())
	{
		pDC->SelectObject(pFont);
		size_t index { 0 }; //Index for vecColorsAscii. Its size is always the same as the vecColorsHex.
		for (const auto& iter : vecColorsHex)
		{
			pDC->SetTextColor(iter.clr.clrText);
			pDC->SetBkColor(iter.clr.clrBk);
			PolyTextOutW(pDC->m_hDC, &iter.stPoly, 1);

			//Ascii color printing.
			PolyTextOutW(pDC->m_hDC, &vecColorsAscii[index++].stPoly, 1);
		}
	}
}

void CHexCtrl::DrawSelection(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, ULONGLONG ullEndLine)
{
	std::vector<POLYTEXTW> vecPolySelHex, vecPolySelAscii;
	std::list<std::wstring>	listWstrSelHex, listWstrSelAscii;
	int iLine = 0; //Current line to print.

	const auto ullOffset = ullStartLine * m_dwCapacity;      //Offset of the visible data to print.
	auto iSize = (ullEndLine - ullStartLine) * m_dwCapacity; //Size of the visible data to print.
	if ((ullOffset + iSize) > m_ullDataSize)
		iSize = m_ullDataSize - ullOffset;
	auto pData = reinterpret_cast<PBYTE>(GetData({ ullOffset, iSize })); //Pointer to data to print.

	//Loop for printing Hex chunks and Ascii chars line by line.
	for (auto iterLines = ullStartLine; iterLines < ullEndLine; ++iterLines, ++iLine)
	{
		std::wstring wstrHexSelToPrint { }, wstrAsciiSelToPrint { }; //Selected Hex and Ascii strings to print.
		int iSelHexPosToPrintX { 0x7FFFFFFF }, iSelAsciiPosToPrintX { };
		bool fSelection { false };
		const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeLetter.cy * iLine; //Hex and Ascii the same.
		auto ullByteToPrint = iterLines * m_dwCapacity;

		//Main loop for printing Hex chunks and Ascii chars.
		for (unsigned iterChunks = 0; iterChunks < m_dwCapacity && ullByteToPrint < m_ullDataSize; ++iterChunks, ++pData, ++ullByteToPrint)
		{
			//Selection.
			if (m_pSelection->HitTest(ullByteToPrint))
			{			//Hex chunk to print.
				const auto chByteToPrint = *pData; //Get current byte to print.
				const wchar_t pwszHexToPrint[2] { g_pwszHexMap[(chByteToPrint >> 4) & 0x0F], g_pwszHexMap[chByteToPrint & 0x0F] };

				//Ascii to print.
				wchar_t wchAscii = chByteToPrint;
				if (wchAscii < 32 || wchAscii >= 0x7f) //For non printable Ascii just print a dot.
					wchAscii = '.';

				if (iSelHexPosToPrintX == 0x7FFFFFFF) //For just one time exec.
				{
					int iCy;
					HexChunkPoint(ullByteToPrint, iSelHexPosToPrintX, iCy);
					AsciiChunkPoint(ullByteToPrint, iSelAsciiPosToPrintX, iCy);
				}

				if (!wstrHexSelToPrint.empty()) //Only adding spaces if there are chars beforehead.
				{
					if ((iterChunks % static_cast<DWORD>(m_enShowMode)) == 0)
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
					vecPolySelHex.emplace_back(POLYTEXTW { iSelHexPosToPrintX, iPosToPrintY,
						static_cast<UINT>(listWstrSelHex.back().size()), listWstrSelHex.back().data(), 0, { }, nullptr });

					//Ascii selection Poly.
					listWstrSelAscii.emplace_back(std::move(wstrAsciiSelToPrint));
					vecPolySelAscii.emplace_back(POLYTEXTW { iSelAsciiPosToPrintX, iPosToPrintY,
						static_cast<UINT>(listWstrSelAscii.back().size()), listWstrSelAscii.back().data(), 0, { }, nullptr });
				}
				iSelHexPosToPrintX = 0x7FFFFFFF;
				fSelection = false;
			}
		}

		//Selection Poly.
		if (!wstrHexSelToPrint.empty())
		{
			//Hex selected Poly.
			listWstrSelHex.emplace_back(std::move(wstrHexSelToPrint));
			vecPolySelHex.emplace_back(POLYTEXTW { iSelHexPosToPrintX, iPosToPrintY,
				static_cast<UINT>(listWstrSelHex.back().size()), listWstrSelHex.back().data(), 0, { }, nullptr });

			//Ascii selected Poly.
			listWstrSelAscii.emplace_back(std::move(wstrAsciiSelToPrint));
			vecPolySelAscii.emplace_back(POLYTEXTW { iSelAsciiPosToPrintX, iPosToPrintY,
				static_cast<UINT>(listWstrSelAscii.back().size()), listWstrSelAscii.back().data(), 0, { }, nullptr });
		}
	}

	//Selection printing.
	if (!vecPolySelHex.empty())
	{
		pDC->SelectObject(pFont);
		pDC->SetTextColor(m_stColor.clrTextSelected);
		pDC->SetBkColor(m_stColor.clrBkSelected);
		PolyTextOutW(pDC->m_hDC, vecPolySelHex.data(), static_cast<UINT>(vecPolySelHex.size()));

		//Ascii selected printing.
		PolyTextOutW(pDC->m_hDC, vecPolySelAscii.data(), static_cast<UINT>(vecPolySelAscii.size()));
	}
}

void CHexCtrl::DrawSelHighlight(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, ULONGLONG ullEndLine)
{
	if (!m_pSelection->HasSelHighlight())
		return;

	std::vector<POLYTEXTW> vecPolySelHex, vecPolySelAscii;
	std::list<std::wstring>	listWstrSelHex, listWstrSelAscii;
	int iLine = 0; //Current line to print.

	const auto ullOffset = ullStartLine * m_dwCapacity;      //Offset of the visible data to print.
	auto iSize = (ullEndLine - ullStartLine) * m_dwCapacity; //Size of the visible data to print.
	if ((ullOffset + iSize) > m_ullDataSize)
		iSize = m_ullDataSize - ullOffset;
	auto pData = reinterpret_cast<PBYTE>(GetData({ ullOffset, iSize })); //Pointer to data to print.

	//Loop for printing Hex chunks and Ascii chars line by line.
	for (auto iterLines = ullStartLine; iterLines < ullEndLine; ++iterLines, ++iLine)
	{
		std::wstring wstrHexSelToPrint { }, wstrAsciiSelToPrint { }; //Selected Hex and Ascii strings to print.
		int iSelHexPosToPrintX { 0x7FFFFFFF }, iSelAsciiPosToPrintX { };
		bool fSelection { false };
		const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeLetter.cy * iLine; //Hex and Ascii the same.
		auto ullByteToPrint = iterLines * m_dwCapacity;

		//Main loop for printing Hex chunks and Ascii chars.
		for (unsigned iterChunks = 0; iterChunks < m_dwCapacity && ullByteToPrint < m_ullDataSize; ++iterChunks, ++pData, ++ullByteToPrint)
		{
			//Selection highlights.
			if (m_pSelection->HitTestHighlight(ullByteToPrint))
			{	//Hex chunk to print.
				const auto chByteToPrint = *pData; //Get current byte to print.
				const wchar_t pwszHexToPrint[2] { g_pwszHexMap[(chByteToPrint >> 4) & 0x0F], g_pwszHexMap[chByteToPrint & 0x0F] };

				//Ascii to print.
				wchar_t wchAscii = chByteToPrint;
				if (wchAscii < 32 || wchAscii >= 0x7f) //For non printable Ascii just print a dot.
					wchAscii = '.';

				if (iSelHexPosToPrintX == 0x7FFFFFFF) //For just one time exec.
				{
					int iCy;
					HexChunkPoint(ullByteToPrint, iSelHexPosToPrintX, iCy);
					AsciiChunkPoint(ullByteToPrint, iSelAsciiPosToPrintX, iCy);
				}

				if (!wstrHexSelToPrint.empty()) //Only adding spaces if there are chars beforehead.
				{
					if ((iterChunks % static_cast<DWORD>(m_enShowMode)) == 0)
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
				//There can be multiple selection highlights in one line. 
				//All the same as for bookmarks.

				//Selection highlight Poly.
				if (!wstrHexSelToPrint.empty())
				{
					//Hex selection highlight Poly.
					listWstrSelHex.emplace_back(std::move(wstrHexSelToPrint));
					vecPolySelHex.emplace_back(POLYTEXTW { iSelHexPosToPrintX, iPosToPrintY,
						static_cast<UINT>(listWstrSelHex.back().size()), listWstrSelHex.back().data(), 0, { }, nullptr });

					//Ascii selection highlight Poly.
					listWstrSelAscii.emplace_back(std::move(wstrAsciiSelToPrint));
					vecPolySelAscii.emplace_back(POLYTEXTW { iSelAsciiPosToPrintX, iPosToPrintY,
						static_cast<UINT>(listWstrSelAscii.back().size()), listWstrSelAscii.back().data(), 0, { }, nullptr });
				}
				iSelHexPosToPrintX = 0x7FFFFFFF;
				fSelection = false;
			}
		}

		//Selection highlight Poly.
		if (!wstrHexSelToPrint.empty())
		{
			//Hex selection highlight Poly.
			listWstrSelHex.emplace_back(std::move(wstrHexSelToPrint));
			vecPolySelHex.emplace_back(POLYTEXTW { iSelHexPosToPrintX, iPosToPrintY,
				static_cast<UINT>(listWstrSelHex.back().size()), listWstrSelHex.back().data(), 0, { }, nullptr });

			//Ascii selection highlight Poly.
			listWstrSelAscii.emplace_back(std::move(wstrAsciiSelToPrint));
			vecPolySelAscii.emplace_back(POLYTEXTW { iSelAsciiPosToPrintX, iPosToPrintY,
				static_cast<UINT>(listWstrSelAscii.back().size()), listWstrSelAscii.back().data(), 0, { }, nullptr });
		}
	}

	//Selection highlight printing.
	if (!vecPolySelHex.empty())
	{
		//Colors are inverted colors of the selection.
		pDC->SelectObject(pFont);
		pDC->SetTextColor(m_stColor.clrBkSelected);
		pDC->SetBkColor(m_stColor.clrTextSelected);
		PolyTextOutW(pDC->m_hDC, vecPolySelHex.data(), static_cast<UINT>(vecPolySelHex.size()));

		//Ascii selection highlight printing.
		PolyTextOutW(pDC->m_hDC, vecPolySelAscii.data(), static_cast<UINT>(vecPolySelAscii.size()));
	}
}

void CHexCtrl::DrawCursor(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, ULONGLONG ullEndLine)
{
	if (!IsDataSet() || !IsMutable())
		return;

	std::vector<POLYTEXTW> vecPolyCursor;
	std::list<std::wstring> listWstrCursor;
	COLORREF clrBkCursor { }; //Cursor color.
	int iLine = 0; //Current line to print.

	const auto ullOffset = ullStartLine * m_dwCapacity;      //Offset of the visible data to print.
	auto iSize = (ullEndLine - ullStartLine) * m_dwCapacity; //Size of the visible data to print.
	if ((ullOffset + iSize) > m_ullDataSize)
		iSize = m_ullDataSize - ullOffset;
	auto pData = reinterpret_cast<PBYTE>(GetData({ ullOffset, iSize })); //Pointer to data to print.

	//Loop for printing Hex chunks and Ascii chars line by line.
	for (auto iterLines = ullStartLine; iterLines < ullEndLine; ++iterLines, ++iLine)
	{
		std::wstring wstrHexCursorToPrint { }, wstrAsciiCursorToPrint { };
		int iCursorHexPosToPrintX { }, iCursorAsciiPosToPrintX { }; //Cursor X coords.
		const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeLetter.cy * iLine; //Hex and Ascii the same.
		auto ullByteToPrint = iterLines * m_dwCapacity;

		//Main loop for printing Hex chunks and Ascii chars.
		for (unsigned iterChunks = 0; iterChunks < m_dwCapacity && ullByteToPrint < m_ullDataSize; ++iterChunks, ++pData, ++ullByteToPrint)
		{
			//Cursor position. 
			if (ullByteToPrint == m_ullCaretPos)
			{
				//Hex chunk to print.
				const auto chByteToPrint = *pData; //Get current byte to print.

				//Ascii to print.
				wchar_t wchAscii = chByteToPrint;
				if (wchAscii < 32 || wchAscii >= 0x7f) //For non printable Ascii just print a dot.
					wchAscii = '.';

				int iCy;
				HexChunkPoint(m_ullCaretPos, iCursorHexPosToPrintX, iCy);
				AsciiChunkPoint(m_ullCaretPos, iCursorAsciiPosToPrintX, iCy);
				if (m_fCursorHigh)
					wstrHexCursorToPrint = g_pwszHexMap[(chByteToPrint & 0xF0) >> 4];
				else
				{
					wstrHexCursorToPrint = g_pwszHexMap[(chByteToPrint & 0x0F)];
					iCursorHexPosToPrintX += m_sizeLetter.cx;
				}
				wstrAsciiCursorToPrint = wchAscii;

				if (m_pSelection->HitTest(ullByteToPrint))
					clrBkCursor = m_stColor.clrBkCursorSelected;
				else
					clrBkCursor = m_stColor.clrBkCursor;

				break; //No need to loop further, only one Chunk with cursor.
			}
		}

		//Cursor Poly.
		if (!wstrHexCursorToPrint.empty())
		{
			listWstrCursor.emplace_back(std::move(wstrHexCursorToPrint));
			vecPolyCursor.emplace_back(POLYTEXTW { iCursorHexPosToPrintX, iPosToPrintY,
				static_cast<UINT>(listWstrCursor.back().size()), listWstrCursor.back().data(), 0, { }, nullptr });
			listWstrCursor.emplace_back(std::move(wstrAsciiCursorToPrint));
			vecPolyCursor.emplace_back(POLYTEXTW { iCursorAsciiPosToPrintX, iPosToPrintY,
				static_cast<UINT>(listWstrCursor.back().size()), listWstrCursor.back().data(), 0, { }, nullptr });
		}
	}

	//Cursor printing.
	if (!vecPolyCursor.empty())
	{
		pDC->SelectObject(pFont);
		pDC->SetTextColor(m_stColor.clrTextCursor);
		pDC->SetBkColor(clrBkCursor);
		PolyTextOutW(pDC->m_hDC, vecPolyCursor.data(), static_cast<UINT>(vecPolyCursor.size()));
	}
}

void CHexCtrl::DrawDataInterpret(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, ULONGLONG ullEndLine)
{
	std::vector<POLYTEXTW> vecPolyDataInterpretHex, vecPolyDataInterpretAscii;
	std::list<std::wstring> listWstrDataInterpretHex, listWstrDataInterpretAscii;
	int iLine = 0; //Current line to print.

	const auto ullOffset = ullStartLine * m_dwCapacity;      //Offset of the visible data to print.
	auto iSize = (ullEndLine - ullStartLine) * m_dwCapacity; //Size of the visible data to print.
	if ((ullOffset + iSize) > m_ullDataSize)
		iSize = m_ullDataSize - ullOffset;
	auto pData = reinterpret_cast<PBYTE>(GetData({ ullOffset, iSize })); //Pointer to data to print.

	//Loop for printing Hex chunks and Ascii chars line by line.
	for (auto iterLines = ullStartLine; iterLines < ullEndLine; ++iterLines, ++iLine)
	{
		std::wstring wstrHexDataInterpretToPrint { }, wstrAsciiDataInterpretToPrint { }; //Data Interpreter Hex and Ascii strings to print.
		int iDataInterpretHexPosToPrintX { 0x7FFFFFFF }, iDataInterpretAsciiPosToPrintX { }; //Data Interpreter X coords.
		const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeLetter.cy * iLine; //Hex and Ascii the same.
																			   //Main loop for printing Hex chunks and Ascii chars.
		auto ullByteToPrint = iterLines * m_dwCapacity;

		for (unsigned iterChunks = 0; iterChunks < m_dwCapacity && ullByteToPrint < m_ullDataSize; ++iterChunks, ++pData, ++ullByteToPrint)
		{
			//Data Interpreter.
			if (auto ullSize = m_pDlgDataInterpret->GetSize(); ullSize > 0
				&& ullByteToPrint >= m_ullCaretPos && ullByteToPrint < m_ullCaretPos + ullSize)
			{
				//Hex chunk to print.
				const auto chByteToPrint = *pData; //Get current byte to print.
				const wchar_t pwszHexToPrint[2] { g_pwszHexMap[(chByteToPrint >> 4) & 0x0F], g_pwszHexMap[chByteToPrint & 0x0F] };

				//Ascii to print.
				wchar_t wchAscii = chByteToPrint;
				if (wchAscii < 32 || wchAscii >= 0x7f) //For non printable Ascii just print a dot.
					wchAscii = '.';

				if (iDataInterpretHexPosToPrintX == 0x7FFFFFFF) //For just one time exec.
				{
					int iCy;
					HexChunkPoint(ullByteToPrint, iDataInterpretHexPosToPrintX, iCy);
					AsciiChunkPoint(ullByteToPrint, iDataInterpretAsciiPosToPrintX, iCy);
				}

				if (!wstrHexDataInterpretToPrint.empty()) //Only adding spaces if there are chars beforehead.
				{
					if ((iterChunks % static_cast<DWORD>(m_enShowMode)) == 0)
						wstrHexDataInterpretToPrint += L" ";

					//Additional space between capacity halves, only with ASBYTE representation.
					if (m_enShowMode == EHexShowMode::ASBYTE && iterChunks == m_dwCapacityBlockSize)
						wstrHexDataInterpretToPrint += L"  ";
				}
				wstrHexDataInterpretToPrint += pwszHexToPrint[0];
				wstrHexDataInterpretToPrint += pwszHexToPrint[1];
				wstrAsciiDataInterpretToPrint += wchAscii;
			}
		}

		//Data Interpreter Poly.
		if (!wstrHexDataInterpretToPrint.empty())
		{
			//Hex Data Interpreter Poly.
			listWstrDataInterpretHex.emplace_back(std::move(wstrHexDataInterpretToPrint));
			vecPolyDataInterpretHex.emplace_back(POLYTEXTW { iDataInterpretHexPosToPrintX, iPosToPrintY,
				static_cast<UINT>(listWstrDataInterpretHex.back().size()), listWstrDataInterpretHex.back().data(), 0, { }, nullptr });

			//Ascii Data Interpreter Poly.
			listWstrDataInterpretAscii.emplace_back(std::move(wstrAsciiDataInterpretToPrint));
			vecPolyDataInterpretAscii.emplace_back(POLYTEXTW { iDataInterpretAsciiPosToPrintX, iPosToPrintY,
				static_cast<UINT>(listWstrDataInterpretAscii.back().size()), listWstrDataInterpretAscii.back().data(), 0, { }, nullptr });
		}
	}

	//Data Interpreter printing.
	if (!vecPolyDataInterpretHex.empty())
	{
		pDC->SelectObject(pFont);
		pDC->SetTextColor(m_stColor.clrTextDataInterpret);
		pDC->SetBkColor(m_stColor.clrBkDataInterpret);
		PolyTextOutW(pDC->m_hDC, vecPolyDataInterpretHex.data(), static_cast<UINT>(vecPolyDataInterpretHex.size()));

		//Ascii selected printing.
		PolyTextOutW(pDC->m_hDC, vecPolyDataInterpretAscii.data(), static_cast<UINT>(vecPolyDataInterpretAscii.size()));
	}
}

void CHexCtrl::DrawSectorLines(CDC* pDC, ULONGLONG ullStartLine, ULONGLONG ullEndLine)
{
	//Struct for sector lines.
	struct SECTORLINES
	{
		POINT ptStart;
		POINT ptEnd;
	};
	std::vector<SECTORLINES> vecSecLines;
	int iLine = 0; //Current line to print.

	//Loop for printing Hex chunks and Ascii chars line by line.
	for (auto iterLines = ullStartLine; iterLines < ullEndLine; ++iterLines, ++iLine)
	{
		//Sectors lines vector to print.
		if (IsSectorVisible() && ((iterLines * m_dwCapacity) % m_dwSectorSize == 0) && iLine > 0)
		{
			const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeLetter.cy * iLine; //Hex and Ascii the same.
			vecSecLines.emplace_back(SECTORLINES { { m_iFirstVertLine, iPosToPrintY }, { m_iFourthVertLine, iPosToPrintY } });
		}
	}

	//Sector lines printing.
	if (!vecSecLines.empty())
	{
		for (const auto& iter : vecSecLines)
		{
			pDC->MoveTo(iter.ptStart.x, iter.ptStart.y);
			pDC->LineTo(iter.ptEnd.x, iter.ptEnd.y);
		}
	}
}

void CHexCtrl::OnSize(UINT /*nType*/, int cx, int cy)
{
	RecalcWorkArea(cy, cx);
	auto ullPageSize = static_cast<ULONGLONG>(m_iHeightWorkArea * m_dbWheelRatio);
	if (ullPageSize < m_sizeLetter.cy)
		ullPageSize = m_sizeLetter.cy;
	m_pScrollV->SetScrollPageSize(ullPageSize);
	MsgWindowNotify(HEXCTRL_MSG_VIEWCHANGE);
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

void CHexCtrl::OnTimer(UINT_PTR nIDEvent)
{
	//Checking if cursor left client rect,
	//if so — hiding bookmark tooltip and killing timer.
	if (nIDEvent == ID_TOOLTIP_BKM)
	{
		CRect rcClient;
		GetClientRect(rcClient);
		ClientToScreen(rcClient);
		CPoint ptCursor;
		GetCursorPos(&ptCursor);
		if (!rcClient.PtInRect(ptCursor))
			TtBkmShow(false);
	}
}

void CHexCtrl::OnDestroy()
{
	ClearData();
	m_wndTtBkm.DestroyWindow();
	m_menuMain.DestroyMenu();
	m_fontMain.DeleteObject();
	m_fontInfo.DeleteObject();
	m_penLines.DeleteObject();
	m_umapHBITMAP.clear();
	m_pDlgBookmarkMgr->DestroyWindow();
	m_pDlgDataInterpret->DestroyWindow();
	m_pDlgFillData->DestroyWindow();
	m_pDlgOpers->DestroyWindow();
	m_pDlgSearch->DestroyWindow();
	m_pScrollV->DestroyWindow();
	m_pScrollH->DestroyWindow();

	m_dwCapacity = 0x10;
	m_fCreated = false;

	if (m_enDataMode == EHexDataMode::DATA_MSG && GetMsgWindow() != GetParent()->GetSafeHwnd())
		MsgWindowNotify(HEXCTRL_MSG_DESTROY); //To avoid sending notify message twice to the same window.

	ParentNotify(HEXCTRL_MSG_DESTROY);

	CWnd::OnDestroy();
}

std::byte* CHexCtrl::GetData(HEXSPANSTRUCT hss)const
{
	if (hss.ullOffset >= m_ullDataSize || hss.ullSize > m_ullDataSize)
		return nullptr;

	std::byte* pData { };
	switch (m_enDataMode)
	{
	case EHexDataMode::DATA_MEMORY:
		pData = m_pData + hss.ullOffset;
		break;
	case EHexDataMode::DATA_MSG:
	{
		HEXNOTIFYSTRUCT hns { { m_hWnd, static_cast<UINT>(GetDlgCtrlID()), HEXCTRL_MSG_GETDATA } };
		if (hss.ullSize == 0)
			hss.ullSize = m_dwCacheSize;
		hns.stSpan = hss;
		MsgWindowNotify(hns);
		pData = hns.pData;
	}
	break;
	case EHexDataMode::DATA_VIRTUAL:
		if (hss.ullSize == 0)
			hss.ullSize = m_dwCacheSize;
		pData = m_pHexVirtData->GetData(hss);
		break;
	}

	return pData;
}

void CHexCtrl::SetDataVirtual(std::byte* pData, const HEXSPANSTRUCT& hss)
{
	switch (m_enDataMode)
	{
	case EHexDataMode::DATA_MEMORY:
		//No need to do anything. Data is already set in pData memory.
		break;
	case EHexDataMode::DATA_VIRTUAL:
		m_pHexVirtData->SetData(pData, hss);
		break;
	case EHexDataMode::DATA_MSG:
	{
		HEXNOTIFYSTRUCT hns { { m_hWnd, static_cast<UINT>(GetDlgCtrlID()), HEXCTRL_MSG_SETDATA } };
		hns.stSpan = hss;
		hns.pData = pData;
		MsgWindowNotify(hns);
	}
	break;
	}
}

EHexDataMode CHexCtrl::GetDataMode()const
{
	return m_enDataMode;
}

ULONGLONG CHexCtrl::GetDataSize()const
{
	return m_ullDataSize;
}

void CHexCtrl::Modify(const MODIFYSTRUCT& hms, bool fRedraw)
{
	if (!IsMutable())
		return;

	m_deqRedo.clear(); //No Redo unless we make Undo.
	SnapshotUndo(hms.vecSpan);

	switch (hms.enModifyMode)
	{
	case EModifyMode::MODIFY_DEFAULT:
		ModifyDefault(hms);
		break;
	case EModifyMode::MODIFY_REPEAT:
		ModifyRepeat(hms);
		break;
	case EModifyMode::MODIFY_OPERATION:
		ModifyOperation(hms);
		break;
	}

	ParentNotify(HEXCTRL_MSG_SETDATA);

	if (fRedraw)
		RedrawWindow();
}

void CHexCtrl::ModifyDefault(const MODIFYSTRUCT& hms)
{
	const auto& vecSelRef = hms.vecSpan;
	std::byte* pData = GetData(vecSelRef[0]);
	std::copy_n(hms.pData, static_cast<size_t>(vecSelRef[0].ullSize), pData);
	SetDataVirtual(pData, vecSelRef[0]);
}

void CHexCtrl::ModifyRepeat(const MODIFYSTRUCT& hms)
{
	const auto& vecSelRef = hms.vecSpan;
	constexpr auto sizeQuick { 1024 * 256 }; //256KB.
	const auto ullTotalSize = std::accumulate(vecSelRef.begin(), vecSelRef.end(), 0ULL,
		[](ULONGLONG ullSumm, const HEXSPANSTRUCT& ref) {return ullSumm + ref.ullSize; });
	std::byte* pData { };
	ULONGLONG ullOffset { 0 };

	CHexDlgCallback dlg(L"Modifying...");
	std::thread thrd([&]() {
		for (const auto& iterSel : vecSelRef) //Selections vector's size times.
		{
			if (hms.ullDataSize == sizeof(std::byte) //Special cases for fast std::fill.
				|| hms.ullDataSize == sizeof(WORD)
				|| hms.ullDataSize == sizeof(DWORD)
				|| hms.ullDataSize == sizeof(QWORD))
			{
				ULONGLONG ullSizeChunk { };
				ULONGLONG ullChunks { };
				CalcChunksFromSize(iterSel.ullSize, hms.ullDataSize, ullSizeChunk, ullChunks);

				for (ULONGLONG iterChunk = 0; iterChunk < ullChunks; ++iterChunk)
				{
					ullOffset = iterSel.ullOffset + (iterChunk * ullSizeChunk);
					if (ullOffset + ullSizeChunk > m_ullDataSize) //Overflow check.
						ullSizeChunk = m_ullDataSize - ullOffset;
					if (ullOffset + ullSizeChunk > iterSel.ullOffset + iterSel.ullSize)
						ullSizeChunk = (iterSel.ullOffset + iterSel.ullSize) - ullOffset;

					pData = GetData({ ullOffset, ullSizeChunk });
					switch (hms.ullDataSize)
					{
					case (sizeof(std::byte)):
						std::fill_n(pData, static_cast<size_t>(ullSizeChunk), *hms.pData);
						break;
					case (sizeof(WORD)):
						std::fill_n(reinterpret_cast<PWORD>(pData), static_cast<size_t>(ullSizeChunk) / sizeof(WORD),
							*reinterpret_cast<PWORD>(hms.pData));
						break;
					case (sizeof(DWORD)):
						std::fill_n(reinterpret_cast<PDWORD>(pData), static_cast<size_t>(ullSizeChunk) / sizeof(DWORD),
							*reinterpret_cast<PDWORD>(hms.pData));
						break;
					case (sizeof(QWORD)):
						std::fill_n(reinterpret_cast<PQWORD>(pData), static_cast<size_t>(ullSizeChunk) / sizeof(QWORD),
							*reinterpret_cast<PQWORD>(hms.pData));
						break;
					}
					if (m_enDataMode != EHexDataMode::DATA_MEMORY)
						SetDataVirtual(pData, { ullOffset, ullSizeChunk });
					if (dlg.IsCanceled())
						goto exit;
				}
			}
			else
			{
				//Fill iterSel.ullSize bytes with hms.ullDataSize bytes iterSel.ullSize / hms.ullDataSize times.
				ULONGLONG ullChunks = (iterSel.ullSize >= hms.ullDataSize) ? iterSel.ullSize / hms.ullDataSize : 0;
				for (ULONGLONG iterChunk = 0; iterChunk < ullChunks; ++iterChunk)
				{
					ullOffset = static_cast<size_t>(iterSel.ullOffset) +
						static_cast<size_t>(iterChunk) * static_cast<size_t>(hms.ullDataSize);

					pData = GetData({ ullOffset, hms.ullDataSize });
					std::copy_n(hms.pData, static_cast<size_t>(hms.ullDataSize), pData);
					if (m_enDataMode != EHexDataMode::DATA_MEMORY)
						SetDataVirtual(pData, { ullOffset, hms.ullDataSize });
					if (dlg.IsCanceled())
						goto exit;
				}
			}
		}
	exit:
		dlg.Cancel();
		});
	if (ullTotalSize > sizeQuick) //Showing "Cancel" dialog only when data > sizeQuick
		dlg.DoModal();
	thrd.join();
}

void CHexCtrl::ModifyOperation(const MODIFYSTRUCT& hms)
{
	if (hms.ullDataSize > sizeof(QWORD))
		return;
	assert(hms.ullDataSize > 0 && ((hms.ullDataSize & (hms.ullDataSize - 1)) == 0)); //Power of 2 only! 

	const auto& vecSelRef = hms.vecSpan;
	constexpr auto sizeQuick { 1024 * 256 }; //256KB.
	const auto ullTotalSize = std::accumulate(vecSelRef.begin(), vecSelRef.end(), 0ULL,
		[](ULONGLONG ullSumm, const HEXSPANSTRUCT& ref) {return ullSumm + ref.ullSize; });

	CHexDlgCallback dlg(L"Modifying...");
	std::thread thrd([&]() {
		for (const auto& iterSel : vecSelRef) //Selections vector's size times.
		{
			ULONGLONG ullSizeChunk { };
			ULONGLONG ullChunks { };
			CalcChunksFromSize(iterSel.ullSize, hms.ullDataSize, ullSizeChunk, ullChunks);

			for (ULONGLONG iterChunk = 0; iterChunk < ullChunks; ++iterChunk)
			{
				ULONGLONG ullOffset = iterSel.ullOffset + (iterChunk * ullSizeChunk);
				if (ullOffset + ullSizeChunk > m_ullDataSize) //Overflow check.
					ullSizeChunk = m_ullDataSize - ullOffset;
				if (ullOffset + ullSizeChunk > iterSel.ullOffset + iterSel.ullSize)
					ullSizeChunk = (iterSel.ullOffset + iterSel.ullSize) - ullOffset;

				std::byte* pData = GetData({ ullOffset, ullSizeChunk });
				switch (hms.ullDataSize)
				{
				case (sizeof(BYTE)):
				{
					BYTE bDataOper { };
					if (hms.pData) //pDataOper might be null for, say, EOperMode::OPER_NOT.
						bDataOper = *reinterpret_cast<PBYTE>(hms.pData);
					auto pOper = reinterpret_cast<PBYTE>(pData);
					OperData(pOper, hms.enOperMode, bDataOper, ullSizeChunk);
				}
				break;
				case (sizeof(WORD)):
				{
					WORD wDataOper { };
					if (hms.pData)
						wDataOper = *reinterpret_cast<PWORD>(hms.pData);
					auto pOper = reinterpret_cast<PWORD>(pData);
					OperData(pOper, hms.enOperMode, wDataOper, ullSizeChunk);
				}
				break;
				case (sizeof(DWORD)):
				{
					DWORD dwDataOper { };
					if (hms.pData)
						dwDataOper = *reinterpret_cast<PDWORD>(hms.pData);
					auto pOper = reinterpret_cast<PDWORD>(pData);
					OperData(pOper, hms.enOperMode, dwDataOper, ullSizeChunk);
				}
				break;
				case (sizeof(QWORD)):
				{
					QWORD qwDataOper { };
					if (hms.pData)
						qwDataOper = *reinterpret_cast<PQWORD>(hms.pData);
					auto pOper = reinterpret_cast<PQWORD>(pData);
					OperData(pOper, hms.enOperMode, qwDataOper, ullSizeChunk);
				}
				break;
				}

				if (m_enDataMode != EHexDataMode::DATA_MEMORY)
					SetDataVirtual(pData, { ullOffset, ullSizeChunk });
				if (dlg.IsCanceled())
					goto exit;
			}
		}
	exit:
		dlg.Cancel();
		});
	if (ullTotalSize > sizeQuick) //Showing "Cancel" dialog only when data > sizeQuick
		dlg.DoModal();
	thrd.join();
}

void CHexCtrl::CalcChunksFromSize(ULONGLONG ullSize, ULONGLONG ullAlign, ULONGLONG& ullSizeChunk, ULONGLONG& ullChunks)
{
	switch (m_enDataMode)
	{
	case EHexDataMode::DATA_MEMORY:
		ullSizeChunk = ullSize;
		ullChunks = 1;
		break;
	case EHexDataMode::DATA_MSG:
	case EHexDataMode::DATA_VIRTUAL:
	{
		ullSizeChunk = m_dwCacheSize; //Size of Virtual memory for acquiring, to work with.
		if (ullAlign > 0)
			ullSizeChunk -= (ullSizeChunk & (ullAlign - 1)); //Aligning chunk size to ullAlign.
		if (ullSize < ullSizeChunk)
			ullSizeChunk = ullSize;
		ullChunks = ullSize % ullSizeChunk ? ullSize / ullSizeChunk + 1 : ullSize / ullSizeChunk;
	}
	break;
	}
}

DWORD CHexCtrl::GetCacheSize() const
{
	return m_dwCacheSize;
}

HWND CHexCtrl::GetMsgWindow()const
{
	return m_hwndMsg;
}

void CHexCtrl::RecalcAll()
{
	ULONGLONG ullCurLineV = GetTopLine();

	//Current font size related.
	TEXTMETRICW tm;
	CDC* pDC = GetDC();
	pDC->SelectObject(m_fontMain);
	pDC->GetTextMetricsW(&tm);
	m_sizeLetter.cx = tm.tmAveCharWidth;
	m_sizeLetter.cy = tm.tmHeight + tm.tmExternalLeading;
	pDC->SelectObject(m_fontInfo);
	pDC->GetTextMetricsW(&tm);
	ReleaseDC(pDC);

	//m_iHeightBottomRect depends on m_fontInfo sizes.
	m_iHeightBottomRect = tm.tmHeight + tm.tmExternalLeading;
	m_iHeightBottomRect += m_iHeightBottomRect / 3;
	m_iHeightBottomOffArea = m_iHeightBottomRect + m_iIndentBottomLine;

	RecalcOffsetDigits();
	m_iSecondVertLine = m_iFirstVertLine + m_dwOffsetDigits * m_sizeLetter.cx + m_sizeLetter.cx * 2;
	m_iSizeHexByte = m_sizeLetter.cx * 2;
	m_iSpaceBetweenBlocks = (m_enShowMode == EHexShowMode::ASBYTE && m_dwCapacity > 1) ? m_sizeLetter.cx * 2 : 0;
	m_iSpaceBetweenHexChunks = m_sizeLetter.cx;
	m_iDistanceBetweenHexChunks = m_iSizeHexByte * static_cast<DWORD>(m_enShowMode) + m_iSpaceBetweenHexChunks;
	m_iThirdVertLine = m_iSecondVertLine + m_iDistanceBetweenHexChunks * (m_dwCapacity / static_cast<DWORD>(m_enShowMode))
		+ m_sizeLetter.cx + m_iSpaceBetweenBlocks;
	m_iIndentAscii = m_iThirdVertLine + m_sizeLetter.cx;
	m_iSpaceBetweenAscii = m_sizeLetter.cx;
	m_iFourthVertLine = m_iIndentAscii + (m_iSpaceBetweenAscii * m_dwCapacity) + m_sizeLetter.cx;
	m_iIndentFirstHexChunk = m_iSecondVertLine + m_sizeLetter.cx;
	m_iSizeFirstHalf = m_iIndentFirstHexChunk + m_dwCapacityBlockSize * (m_sizeLetter.cx * 2) +
		(m_dwCapacityBlockSize / static_cast<DWORD>(m_enShowMode) - 1) * m_iSpaceBetweenHexChunks;
	m_iHeightTopRect = std::lround(m_sizeLetter.cy * 1.5);
	m_iStartWorkAreaY = m_iFirstHorizLine + m_iHeightTopRect;
	m_iIndentTextCapacityY = m_iHeightTopRect / 2 - (m_sizeLetter.cy / 2);

	CRect rc;
	GetClientRect(&rc);
	RecalcWorkArea(rc.Height(), rc.Width());

	//Scroll sizes according to current font size.
	m_pScrollV->SetScrollSizes(m_sizeLetter.cy, static_cast<ULONGLONG>(m_iHeightWorkArea * m_dbWheelRatio),
		static_cast<ULONGLONG>(m_iStartWorkAreaY) + m_iHeightBottomOffArea + m_sizeLetter.cy * (m_ullDataSize / m_dwCapacity + 2));
	m_pScrollH->SetScrollSizes(m_sizeLetter.cx, rc.Width(), static_cast<ULONGLONG>(m_iFourthVertLine) + 1);
	m_pScrollV->SetScrollPos(ullCurLineV * m_sizeLetter.cy);

	RedrawWindow();
	MsgWindowNotify(HEXCTRL_MSG_VIEWCHANGE);
}

void CHexCtrl::RecalcPrint(CDC * pDC, CFont * pFontMain, CFont * pFontInfo, const CRect & rc)
{
	pDC->SelectObject(pFontMain);
	TEXTMETRICW tm;
	pDC->GetTextMetricsW(&tm);
	m_sizeLetter.cx = tm.tmAveCharWidth;
	m_sizeLetter.cy = tm.tmHeight + tm.tmExternalLeading;
	pDC->SelectObject(pFontInfo);
	pDC->GetTextMetricsW(&tm);

	//m_iHeightBottomRect depends on pFontInfo sizes.
	m_iHeightBottomRect = tm.tmHeight + tm.tmExternalLeading;
	m_iHeightBottomRect += m_iHeightBottomRect / 3;
	m_iHeightBottomOffArea = m_iHeightBottomRect + m_iIndentBottomLine;

	RecalcOffsetDigits();
	m_iSecondVertLine = m_iFirstVertLine + m_dwOffsetDigits * m_sizeLetter.cx + m_sizeLetter.cx * 2;
	m_iSizeHexByte = m_sizeLetter.cx * 2;
	m_iSpaceBetweenBlocks = (m_enShowMode == EHexShowMode::ASBYTE && m_dwCapacity > 1) ? m_sizeLetter.cx * 2 : 0;
	m_iSpaceBetweenHexChunks = m_sizeLetter.cx;
	m_iDistanceBetweenHexChunks = m_iSizeHexByte * static_cast<DWORD>(m_enShowMode) + m_iSpaceBetweenHexChunks;
	m_iThirdVertLine = m_iSecondVertLine + m_iDistanceBetweenHexChunks * (m_dwCapacity / static_cast<DWORD>(m_enShowMode))
		+ m_sizeLetter.cx + m_iSpaceBetweenBlocks;
	m_iIndentAscii = m_iThirdVertLine + m_sizeLetter.cx;
	m_iSpaceBetweenAscii = m_sizeLetter.cx;
	m_iFourthVertLine = m_iIndentAscii + (m_iSpaceBetweenAscii * m_dwCapacity) + m_sizeLetter.cx;
	m_iIndentFirstHexChunk = m_iSecondVertLine + m_sizeLetter.cx;
	m_iSizeFirstHalf = m_iIndentFirstHexChunk + m_dwCapacityBlockSize * (m_sizeLetter.cx * 2) +
		(m_dwCapacityBlockSize / static_cast<DWORD>(m_enShowMode) - 1) * m_iSpaceBetweenHexChunks;
	m_iHeightTopRect = std::lround(m_sizeLetter.cy * 1.5);
	m_iStartWorkAreaY = m_iFirstHorizLine + m_iHeightTopRect;
	m_iIndentTextCapacityY = m_iHeightTopRect / 2 - (m_sizeLetter.cy / 2);

	RecalcWorkArea(rc.Height(), rc.Width());
}

void CHexCtrl::RecalcWorkArea(int iHeight, int iWidth)
{
	m_iHeightClientArea = iHeight;
	m_iWidthClientArea = iWidth;
	m_iEndWorkArea = m_iHeightClientArea - m_iHeightBottomOffArea -
		((m_iHeightClientArea - m_iStartWorkAreaY - m_iHeightBottomOffArea) % m_sizeLetter.cy);
	m_iHeightWorkArea = m_iEndWorkArea - m_iStartWorkAreaY;
}

void CHexCtrl::RecalcOffsetDigits()
{
	if (m_ullDataSize <= 0xffffffffUL)
	{
		m_dwOffsetBytes = 4;
		if (m_fOffsetAsHex)
			m_dwOffsetDigits = 8;
		else
			m_dwOffsetDigits = 10;
	}
	else if (m_ullDataSize <= 0xffffffffffUL)
	{
		m_dwOffsetBytes = 5;
		if (m_fOffsetAsHex)
			m_dwOffsetDigits = 10;
		else
			m_dwOffsetDigits = 13;
	}
	else if (m_ullDataSize <= 0xffffffffffffUL)
	{
		m_dwOffsetBytes = 6;
		if (m_fOffsetAsHex)
			m_dwOffsetDigits = 12;
		else
			m_dwOffsetDigits = 15;
	}
	else if (m_ullDataSize <= 0xffffffffffffffUL)
	{
		m_dwOffsetBytes = 7;
		if (m_fOffsetAsHex)
			m_dwOffsetDigits = 14;
		else
			m_dwOffsetDigits = 17;
	}
	else
	{
		m_dwOffsetBytes = 8;
		if (m_fOffsetAsHex)
			m_dwOffsetDigits = 16;
		else
			m_dwOffsetDigits = 19;
	}
}

ULONGLONG CHexCtrl::GetTopLine()const
{
	return m_pScrollV->GetScrollPos() / m_sizeLetter.cy;
}

ULONGLONG CHexCtrl::GetBottomLine() const
{
	ULONGLONG ullEndLine { 0 };
	if (m_ullDataSize)
	{
		ullEndLine = GetTopLine() + m_iHeightWorkArea / m_sizeLetter.cy;
		//If m_dwDataCount is really small we adjust ullEndLine to be not bigger than maximum allowed.
		if (ullEndLine > (m_ullDataSize / m_dwCapacity))
			ullEndLine = (m_ullDataSize % m_dwCapacity) ? m_ullDataSize / m_dwCapacity + 1 : m_ullDataSize / m_dwCapacity;
	}
	return ullEndLine;
}

auto CHexCtrl::HitTest(POINT pt)const->std::optional<HEXHITTESTSTRUCT>
{
	HEXHITTESTSTRUCT stHit;
	bool fHit { false };
	int iY = pt.y;
	int iX = pt.x + static_cast<int>(m_pScrollH->GetScrollPos()); //To compensate horizontal scroll.
	ULONGLONG ullCurLine = GetTopLine();

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
		for (unsigned i = 1; i <= m_dwCapacity; ++i)
		{
			if ((i % static_cast<DWORD>(m_enShowMode)) == 0)
				iSpaceBetweenHexChunks += m_iSpaceBetweenHexChunks;
			if ((m_iSizeHexByte * i + iSpaceBetweenHexChunks) > dwX)
			{
				dwChunkX = i - 1;
				break;
			}
		}
		stHit.ullOffset = static_cast<ULONGLONG>(dwChunkX) + ((iY - m_iStartWorkAreaY) / m_sizeLetter.cy) *
			m_dwCapacity + (ullCurLine * m_dwCapacity);
		fHit = true;
	}
	//Or within Ascii area.
	else if ((iX >= m_iIndentAscii) && (iX < (m_iIndentAscii + m_iSpaceBetweenAscii * static_cast<int>(m_dwCapacity)))
		&& (iY >= m_iStartWorkAreaY) && iY <= m_iEndWorkArea)
	{
		//Calculate ullHit Ascii symbol.
		stHit.ullOffset = ((iX - static_cast<ULONGLONG>(m_iIndentAscii)) / m_iSpaceBetweenAscii) +
			((iY - m_iStartWorkAreaY) / m_sizeLetter.cy) * m_dwCapacity + (ullCurLine * m_dwCapacity);
		stHit.fIsAscii = true;
		fHit = true;
	}

	//If cursor is out of end-bound of hex chunks or Ascii chars.
	if (stHit.ullOffset >= m_ullDataSize)
		fHit = false;

	return fHit ? std::optional<HEXHITTESTSTRUCT> { stHit } : std::nullopt;
}

void CHexCtrl::HexChunkPoint(ULONGLONG ullOffset, int& iCx, int& iCy)const
{	//This func computes x and y pos of given Hex chunk.
	DWORD dwMod = ullOffset % m_dwCapacity;
	int iBetweenBlocks { 0 };
	if (dwMod >= m_dwCapacityBlockSize)
		iBetweenBlocks = m_iSpaceBetweenBlocks;

	iCx = static_cast<int>(((m_iIndentFirstHexChunk + iBetweenBlocks + dwMod * m_iSizeHexByte) +
		(dwMod / static_cast<DWORD>(m_enShowMode)) * m_iSpaceBetweenHexChunks) - m_pScrollH->GetScrollPos());

	auto ullScrollV = m_pScrollV->GetScrollPos();
	iCy = static_cast<int>((m_iStartWorkAreaY + (ullOffset / m_dwCapacity) * m_sizeLetter.cy) -
		(ullScrollV - (ullScrollV % m_sizeLetter.cy)));
}

void CHexCtrl::AsciiChunkPoint(ULONGLONG ullOffset, int& iCx, int& iCy) const
{	//This func computes x and y pos of given Ascii chunk.
	DWORD dwMod = ullOffset % m_dwCapacity;
	iCx = static_cast<int>((m_iIndentAscii + dwMod * m_sizeLetter.cx) - m_pScrollH->GetScrollPos());

	auto ullScrollV = m_pScrollV->GetScrollPos();
	iCy = static_cast<int>((m_iStartWorkAreaY + (ullOffset / m_dwCapacity) * m_sizeLetter.cy) -
		(ullScrollV - (ullScrollV % m_sizeLetter.cy)));
}

void CHexCtrl::ClipboardCopy(EClipboard enType)
{
	if (!m_pSelection->HasSelection() || m_pSelection->GetSelectionSize() > 0x6400000) //>~100MB
	{
		MessageBoxW(L"Selection size is too big to copy.\r\nTry to select less.", L"Error", MB_ICONERROR);
		return;
	}

	std::string strData;
	switch (enType)
	{
	case EClipboard::COPY_HEX:
		strData = CopyHex();
		break;
	case EClipboard::COPY_HEX_LE:
		strData = CopyHexLE();
		break;
	case EClipboard::COPY_HEX_FORMATTED:
		strData = CopyHexFormatted();
		break;
	case EClipboard::COPY_ASCII:
		strData = CopyAscii();
		break;
	case EClipboard::COPY_BASE64:
		strData = CopyBase64();
		break;
	case EClipboard::COPY_CARRAY:
		strData = CopyCArr();
		break;
	case EClipboard::COPY_GREPHEX:
		strData = CopyGrepHex();
		break;
	case EClipboard::COPY_PRINTSCREEN:
		strData = CopyPrintScreen();
		break;
	default:
		break;
	}

	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, strData.size() + 1);
	if (!hMem) {
		MessageBoxW(L"GlobalAlloc error.", L"Error", MB_ICONERROR);
		return;
	}
	LPVOID lpMemLock = GlobalLock(hMem);
	if (!lpMemLock) {
		MessageBoxW(L"GlobalLock error.", L"Error", MB_ICONERROR);
		return;
	}
	memcpy(lpMemLock, strData.data(), strData.size() + 1);
	GlobalUnlock(hMem);
	OpenClipboard();
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hMem);
	CloseClipboard();
}

std::string CHexCtrl::CopyHex()
{
	std::string strData;
	auto ullSelSize = m_pSelection->GetSelectionSize();

	strData.reserve(static_cast<size_t>(ullSelSize) * 2);
	for (unsigned i = 0; i < ullSelSize; ++i)
	{
		auto chByte = GetData<BYTE>(m_pSelection->GetOffsetByIndex(i));
		strData += g_pszHexMap[(chByte & 0xF0) >> 4];
		strData += g_pszHexMap[(chByte & 0x0F)];
	}

	return strData;
}

std::string CHexCtrl::CopyHexLE()
{
	std::string strData;
	auto ullSelSize = m_pSelection->GetSelectionSize();

	strData.reserve(static_cast<size_t>(ullSelSize) * 2);
	for (int i = static_cast<int>(ullSelSize); i > 0; --i)
	{
		auto chByte = GetData<BYTE>(m_pSelection->GetOffsetByIndex(i - 1));
		strData += g_pszHexMap[(chByte & 0xF0) >> 4];
		strData += g_pszHexMap[(chByte & 0x0F)];
	}

	return strData;
}

std::string CHexCtrl::CopyHexFormatted()
{
	std::string strData;
	auto ullSelStart = m_pSelection->GetSelectionStart();
	auto ullSelSize = m_pSelection->GetSelectionSize();

	strData.reserve(static_cast<size_t>(ullSelSize) * 3);
	if (m_fSelectionBlock)
	{
		DWORD dwTail = m_pSelection->GetLineLength();
		for (unsigned i = 0; i < ullSelSize; ++i)
		{
			auto chByte = GetData<BYTE>(m_pSelection->GetOffsetByIndex(i));
			strData += g_pszHexMap[(chByte & 0xF0) >> 4];
			strData += g_pszHexMap[(chByte & 0x0F)];

			if (i < (ullSelSize - 1) && (dwTail - 1) != 0)
				if (((m_pSelection->GetLineLength() - dwTail + 1) % static_cast<DWORD>(m_enShowMode)) == 0) //Add space after hex full chunk, ShowAs_size depending.
					strData += " ";
			if (--dwTail == 0 && i < (ullSelSize - 1)) //Next row.
			{
				strData += "\r\n";
				dwTail = m_pSelection->GetLineLength();
			}
		}
	}
	else
	{
		//How many spaces are needed to be inserted at the beginning.
		DWORD dwModStart = ullSelStart % m_dwCapacity;

		//When to insert first "\r\n".
		DWORD dwTail = m_dwCapacity - dwModStart;
		DWORD dwNextBlock = (m_dwCapacity % 2) ? m_dwCapacityBlockSize + 2 : m_dwCapacityBlockSize + 1;

		//If at least two rows are selected.
		if (dwModStart + ullSelSize > m_dwCapacity)
		{
			DWORD dwCount = dwModStart * 2 + dwModStart / static_cast<DWORD>(m_enShowMode);
			//Additional spaces between halves. Only in ASBYTE's view mode.
			dwCount += (m_enShowMode == EHexShowMode::ASBYTE) ? (dwTail <= m_dwCapacityBlockSize ? 2 : 0) : 0;
			strData.insert(0, static_cast<size_t>(dwCount), ' ');
		}

		for (unsigned i = 0; i < ullSelSize; ++i)
		{
			auto chByte = GetData<BYTE>(m_pSelection->GetOffsetByIndex(i));
			strData += g_pszHexMap[(chByte & 0xF0) >> 4];
			strData += g_pszHexMap[(chByte & 0x0F)];

			if (i < (ullSelSize - 1) && (dwTail - 1) != 0)
			{
				if (m_enShowMode == EHexShowMode::ASBYTE && dwTail == dwNextBlock)
					strData += "   "; //Additional spaces between halves. Only in ASBYTE view mode.
				else if (((m_dwCapacity - dwTail + 1) % static_cast<DWORD>(m_enShowMode)) == 0) //Add space after hex full chunk, ShowAs_size depending.
					strData += " ";
			}
			if (--dwTail == 0 && i < (ullSelSize - 1)) //Next row.
			{
				strData += "\r\n";
				dwTail = m_dwCapacity;
			}
		}
	}

	return strData;
}

std::string CHexCtrl::CopyAscii()
{
	std::string strData;
	auto ullSelSize = m_pSelection->GetSelectionSize();

	strData.reserve(static_cast<size_t>(ullSelSize));
	for (unsigned i = 0; i < ullSelSize; ++i)
	{
		auto ch = GetData<BYTE>(m_pSelection->GetOffsetByIndex(i));
		//If next byte is zero —> substitute it with space.
		if (ch == 0)
			ch = ' ';
		strData += ch;
	}

	return strData;
}

std::string CHexCtrl::CopyBase64()
{
	std::string strData;
	auto ullSelSize = m_pSelection->GetSelectionSize();

	strData.reserve(static_cast<size_t>(ullSelSize) * 2);
	const char* const pszBase64Map { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" };
	unsigned int uValA = 0;
	int iValB = -6;
	for (unsigned i = 0; i < ullSelSize; ++i)
	{
		uValA = (uValA << 8) + GetData<BYTE>(m_pSelection->GetOffsetByIndex(i));
		iValB += 8;
		while (iValB >= 0)
		{
			strData += pszBase64Map[(uValA >> iValB) & 0x3F];
			iValB -= 6;
		}
	}

	if (iValB > -6)
		strData += pszBase64Map[((uValA << 8) >> (iValB + 8)) & 0x3F];
	while (strData.size() % 4)
		strData += '=';

	return strData;
}

std::string CHexCtrl::CopyCArr()
{
	std::string strData;
	auto ullSelSize = m_pSelection->GetSelectionSize();

	strData.reserve(static_cast<size_t>(ullSelSize) * 3 + 64);
	strData = "unsigned char data[";
	char arrSelectionNum[8] { };
	sprintf_s(arrSelectionNum, 8, "%llu", ullSelSize);
	strData += arrSelectionNum;
	strData += "] = {\r\n";

	for (unsigned i = 0; i < ullSelSize; ++i)
	{
		strData += "0x";
		auto chByte = GetData<BYTE>(m_pSelection->GetOffsetByIndex(i));
		strData += g_pszHexMap[(chByte & 0xF0) >> 4];
		strData += g_pszHexMap[(chByte & 0x0F)];
		if (i < ullSelSize - 1)
			strData += ",";
		if ((i + 1) % 16 == 0)
			strData += "\r\n";
		else
			strData += " ";
	}
	if (strData.back() != '\n') //To prevent double new line if ullSelSize % 16 == 0
		strData += "\r\n";
	strData += "};";

	return strData;
}

std::string CHexCtrl::CopyGrepHex()
{
	std::string strData;
	auto ullSelSize = m_pSelection->GetSelectionSize();

	strData.reserve(static_cast<size_t>(ullSelSize) * 2);
	for (unsigned i = 0; i < ullSelSize; ++i)
	{
		strData += "\\x";
		auto chByte = GetData<BYTE>(m_pSelection->GetOffsetByIndex(i));
		strData += g_pszHexMap[(chByte & 0xF0) >> 4];
		strData += g_pszHexMap[(chByte & 0x0F)];
	}

	return strData;
}

std::string CHexCtrl::CopyPrintScreen()
{
	if (m_fSelectionBlock) //Only works with classical selection.
		return { };

	std::string strData;
	auto ullSelStart = m_pSelection->GetSelectionStart();
	auto ullSelSize = m_pSelection->GetSelectionSize();

	strData.reserve(static_cast<size_t>(ullSelSize) * 4);
	strData = "Offset";
	strData.insert(0, (static_cast<size_t>(m_dwOffsetDigits) - strData.size()) / 2, ' ');
	strData.insert(strData.size(), static_cast<size_t>(m_dwOffsetDigits) - strData.size(), ' ');
	strData += "   "; //Spaces to Capacity.
	strData += WstrToStr(m_wstrCapacity);
	strData += "   "; //Spaces to ASCII.
	if (int iSize = static_cast<int>(m_dwCapacity) - 5; iSize > 0) //5 is strlen of "ASCII".
		strData.insert(strData.size(), static_cast<size_t>(iSize / 2), ' ');
	strData += "ASCII";
	strData += "\r\n";

	//How many spaces are needed to be inserted at the beginning.
	DWORD dwModStart = ullSelStart % m_dwCapacity;

	DWORD dwLines = static_cast<DWORD>(ullSelSize) / m_dwCapacity;
	if ((dwModStart + static_cast<DWORD>(ullSelSize)) > m_dwCapacity)
		++dwLines;
	if ((ullSelSize % m_dwCapacity) + dwModStart > m_dwCapacity)
		++dwLines;
	if (!dwLines)
		dwLines = 1;

	auto ullStartLine = ullSelStart / m_dwCapacity;
	std::string strAscii;
	ULONGLONG ullIndexByteToPrint { };

	for (DWORD iterLines = 0; iterLines < dwLines; ++iterLines)
	{
		wchar_t pwszOffset[32] { }; //To be enough for max as Hex and as Decimals.
		UllToWchars(ullStartLine * m_dwCapacity + m_dwCapacity * iterLines,
			pwszOffset, static_cast<size_t>(m_dwOffsetBytes), m_fOffsetAsHex);
		strData += WstrToStr(pwszOffset);
		strData.insert(strData.size(), 3, ' ');

		for (unsigned iterChunks = 0; iterChunks < m_dwCapacity; ++iterChunks)
		{
			if (dwModStart == 0 && ullIndexByteToPrint < ullSelSize)
			{
				auto chByte = GetData<BYTE>(m_pSelection->GetOffsetByIndex(ullIndexByteToPrint++));
				strData += g_pszHexMap[(chByte & 0xF0) >> 4];
				strData += g_pszHexMap[(chByte & 0x0F)];

				if (chByte < 32 || chByte >= 0x7f) //For non printable Ascii just print a dot.
					chByte = '.';

				strAscii += chByte;
			}
			else
			{
				strData += "  ";
				strAscii += " ";
				--dwModStart;
			}

			//Additional space between grouped Hex chunks.
			if (((iterChunks + 1) % static_cast<DWORD>(m_enShowMode)) == 0 && iterChunks < (m_dwCapacity - 1))
				strData += " ";

			//Additional space between capacity halves, only with ASBYTE representation.
			if (m_enShowMode == EHexShowMode::ASBYTE && iterChunks == (m_dwCapacityBlockSize - 1))
				strData += "  ";
		}
		strData += "   "; //Ascii beginning.
		strData += strAscii;
		if (iterLines < dwLines - 1)
			strData += "\r\n";
		strAscii.clear();
	}

	return strData;
}

void CHexCtrl::ClipboardPaste(EClipboard enType)
{
	if (!m_fMutable || !m_pSelection->HasSelection())
		return;

	if (!OpenClipboard())
		return;

	HANDLE hClipboard = GetClipboardData(CF_TEXT);
	if (!hClipboard)
		return;

	auto pszClipboardData = static_cast<LPSTR>(GlobalLock(hClipboard));
	if (!pszClipboardData)
	{
		CloseClipboard();
		return;
	}

	ULONGLONG ullSize = strlen(pszClipboardData);
	ULONGLONG ullSizeToModify { };
	MODIFYSTRUCT hmd;

	std::string strData;
	switch (enType)
	{
	case EClipboard::PASTE_ASCII:
		if (m_ullCaretPos + ullSize > m_ullDataSize)
			ullSize = m_ullDataSize - m_ullCaretPos;

		hmd.pData = reinterpret_cast<std::byte*>(pszClipboardData);
		ullSizeToModify = hmd.ullDataSize = ullSize;
		break;
	case EClipboard::PASTE_HEX:
	{
		ULONGLONG ullRealSize = ullSize / 2 + ullSize % 2;
		if (m_ullCaretPos + ullRealSize > m_ullDataSize)
			ullSize = (m_ullDataSize - m_ullCaretPos) * 2;

		auto dwIterations = static_cast<size_t>(ullSize / 2 + ullSize % 2);
		char chToUL[3] { }; //Array for actual Ascii chars to convert from.
		for (size_t i = 0; i < dwIterations; ++i)
		{
			if (i + 2 <= ullSize)
			{
				chToUL[0] = pszClipboardData[i * 2];
				chToUL[1] = pszClipboardData[i * 2 + 1];
			}
			else
			{
				chToUL[0] = pszClipboardData[i * 2];
				chToUL[1] = '\0';
			}

			unsigned char chNumber;
			if (!str2num(chToUL, chNumber, 16))
			{
				GlobalUnlock(hClipboard);
				CloseClipboard();
				return;
			}
			strData += chNumber;
		}
		hmd.pData = reinterpret_cast<std::byte*>(strData.data());
		ullSizeToModify = hmd.ullDataSize = strData.size();
	}
	break;
	default:
		break;
	}

	hmd.vecSpan.emplace_back(HEXSPANSTRUCT { m_ullCaretPos, ullSizeToModify });
	Modify(hmd);

	GlobalUnlock(hClipboard);
	CloseClipboard();

	RedrawWindow();
}

void CHexCtrl::UpdateInfoText()
{
	if (IsDataSet())
	{
		WCHAR wBuff[260];
		m_wstrInfo.clear();
		m_wstrInfo.reserve(_countof(wBuff));

		if (m_fOffsetAsHex)
			swprintf_s(wBuff, _countof(wBuff), L"Cursor: 0x%llX; ", GetCaretPos());
		else
			swprintf_s(wBuff, _countof(wBuff), L"Cursor: %llu; ", GetCaretPos());
		m_wstrInfo = wBuff;

		//Page/Sector
		if (IsSectorVisible())
		{
			auto ullSectorCurr = GetCaretPos() / m_dwSectorSize;

			if (m_fOffsetAsHex)
				swprintf_s(wBuff, _countof(wBuff), L"%s: 0x%llX/0x%llX; ",
					m_wstrSectorName.data(), ullSectorCurr, m_ullTotalSectors);
			else
				swprintf_s(wBuff, _countof(wBuff), L"%s: %llu/%llu; ",
					m_wstrSectorName.data(), ullSectorCurr, m_ullTotalSectors);
			m_wstrInfo += wBuff;
		}

		if (m_pSelection->HasSelection())
		{
			if (m_fOffsetAsHex)
				swprintf_s(wBuff, _countof(wBuff), L"Selected: 0x%llX [0x%llX-0x%llX]; ",
					m_pSelection->GetSelectionSize(), m_pSelection->GetSelectionStart(), m_pSelection->GetSelectionEnd() - 1);
			else
				swprintf_s(wBuff, _countof(wBuff), L"Selected: %llu [%llu-%llu]; ",
					m_pSelection->GetSelectionSize(), m_pSelection->GetSelectionStart(), m_pSelection->GetSelectionEnd() - 1);
			m_wstrInfo += wBuff;
		}

		//Mutable state
		if (!IsMutable())
			m_wstrInfo += L"RO;";
		else
			m_wstrInfo += L"RW;";
	}
	else
		m_wstrInfo.clear();

	RedrawWindow();
}

void CHexCtrl::ParentNotify(const HEXNOTIFYSTRUCT & hns)const
{
	HWND hwndSendTo = GetParent()->GetSafeHwnd();
	if (hwndSendTo)
		::SendMessageW(hwndSendTo, WM_NOTIFY, GetDlgCtrlID(), reinterpret_cast<LPARAM>(&hns));
}

void CHexCtrl::ParentNotify(UINT uCode)const
{
	HEXNOTIFYSTRUCT hns { { m_hWnd, static_cast<UINT>(GetDlgCtrlID()), uCode } };
	ParentNotify(hns);
}

void CHexCtrl::MsgWindowNotify(const HEXNOTIFYSTRUCT& hns)const
{
	HWND hwndSendTo = GetMsgWindow();
	if (hwndSendTo)
		::SendMessageW(hwndSendTo, WM_NOTIFY, GetDlgCtrlID(), reinterpret_cast<LPARAM>(&hns));
}

void CHexCtrl::MsgWindowNotify(UINT uCode)const
{
	HEXNOTIFYSTRUCT hns { { m_hWnd, static_cast<UINT>(GetDlgCtrlID()), uCode } };
	std::vector<HEXSPANSTRUCT> vecData { };

	switch (uCode)
	{
	case HEXCTRL_MSG_CARETCHANGE:
		hns.ullData = GetCaretPos();
		break;
	case HEXCTRL_MSG_SELECTION:
		vecData = m_pSelection->GetData();
		hns.pData = reinterpret_cast<std::byte*>(&vecData);
		break;
	case HEXCTRL_MSG_VIEWCHANGE:
	{
		const auto ullStartLine = GetTopLine();
		const auto ullEndLine = GetBottomLine();
		hns.stSpan.ullOffset = ullStartLine * m_dwCapacity;
		hns.stSpan.ullSize = (ullEndLine - ullStartLine) * m_dwCapacity;
		if (hns.stSpan.ullOffset + hns.stSpan.ullSize > m_ullDataSize)
			hns.stSpan.ullSize = m_ullDataSize - hns.stSpan.ullOffset;
	}
	break;
	}
	MsgWindowNotify(hns);
}

void CHexCtrl::MoveCaret(ULONGLONG ullPos, bool fHighPart)
{
	m_ullCaretPos = ullPos;
	if (m_ullCaretPos >= m_ullDataSize)
	{
		m_ullCaretPos = m_ullDataSize - 1;
		m_fCursorHigh = false;
	}
	else
		m_fCursorHigh = fHighPart;

	auto ullCurrScrollV = m_pScrollV->GetScrollPos();
	if (auto iMod = ullCurrScrollV % m_sizeLetter.cy; iMod > 0)
		ullCurrScrollV -= iMod;
	auto ullCurrScrollH = m_pScrollH->GetScrollPos();

	int iCx, iCy;
	HexChunkPoint(m_ullCaretPos, iCx, iCy);
	CRect rcClient;
	GetClientRect(&rcClient);

	ULONGLONG ullMaxV = ullCurrScrollV + rcClient.Height() - m_iHeightBottomOffArea - m_iStartWorkAreaY -
		((rcClient.Height() - m_iStartWorkAreaY - m_iHeightBottomOffArea) % m_sizeLetter.cy);
	ULONGLONG ullNewStartV = m_ullCaretPos / m_dwCapacity * m_sizeLetter.cy;

	auto ullNewScrollV = ullCurrScrollV;
	if (ullNewStartV >= ullMaxV)
		ullNewScrollV = ullCurrScrollV + m_sizeLetter.cy;
	else if (ullNewStartV < ullCurrScrollV)
		ullNewScrollV = ullCurrScrollV - m_sizeLetter.cy;

	ULONGLONG ullNewScrollH { };
	int iMaxClientX = rcClient.Width() - m_iSizeHexByte;
	if (iCx >= iMaxClientX)
		ullNewScrollH = ullCurrScrollH + (iCx - iMaxClientX);
	else if (iCx < 0)
		ullNewScrollH = ullCurrScrollH + iCx;
	else
		ullNewScrollH = ullCurrScrollH;

	if (ullNewScrollV != ullCurrScrollV)
		m_pScrollV->SetScrollPos(ullNewScrollV);
	if (m_pScrollH->IsVisible() && !IsCurTextArea()) //Do not horz scroll when modifying text area (not Hex).
		m_pScrollH->SetScrollPos(ullNewScrollH);

	UpdateInfoText();
	OnCaretPosChange(m_ullCaretPos);
}

void CHexCtrl::OnCaretPosChange(ULONGLONG ullOffset)
{
	//To prevent inspecting while key is pressed continuously.
	//Only when one time pressing.
	if (!m_fKeyDownAtm)
		m_pDlgDataInterpret->InspectOffset(ullOffset);

	if (auto pBkm = m_pBookmarks->HitTest(ullOffset); pBkm != nullptr) //If clicked on bookmark.
	{
		HEXNOTIFYSTRUCT hns { { m_hWnd, static_cast<UINT>(GetDlgCtrlID()), HEXCTRL_MSG_BKMCLICK } };
		hns.pData = reinterpret_cast<std::byte*>(pBkm);
		MsgWindowNotify(hns);
	}

	MsgWindowNotify(HEXCTRL_MSG_CARETCHANGE);
}

void CHexCtrl::CaretMoveRight()
{
	if (m_fMutable)
	{
		ULONGLONG ullPos;
		bool fHighPart;
		if (IsCurTextArea())
		{
			ullPos = m_ullCaretPos + 1;
			fHighPart = m_fCursorHigh;
		}
		else if (m_fCursorHigh)
		{
			ullPos = m_ullCaretPos;
			fHighPart = false;
		}
		else
		{
			ullPos = m_ullCaretPos + 1;
			fHighPart = true;
		}
		MoveCaret(ullPos, fHighPart);
	}
	else
		MakeSelection(m_ullLMouseClick + 1, m_ullLMouseClick + 1, 1, 1);
}

void CHexCtrl::CaretMoveLeft()
{
	if (m_fMutable)
	{
		ULONGLONG ull = m_ullCaretPos;
		bool fHighPart;
		if (IsCurTextArea())
		{
			if (ull > 0) //To avoid overflow.
				--ull;
			else
				return; //Zero index byte.
			fHighPart = m_fCursorHigh;
		}
		else if (m_fCursorHigh)
		{
			if (ull > 0) //To avoid overflow.
				--ull;
			else
				return; //Zero index byte.
			fHighPart = false;
		}
		else
			fHighPart = true;

		MoveCaret(ull, fHighPart);
	}
	else
		MakeSelection(m_ullLMouseClick - 1, m_ullLMouseClick - 1, 1, 1);
}

void CHexCtrl::CaretMoveUp()
{
	ULONGLONG ullNewPos = m_fMutable ? m_ullCaretPos : m_ullLMouseClick;
	if (ullNewPos >= m_dwCapacity)
		ullNewPos -= m_dwCapacity;

	if (m_fMutable)
		MoveCaret(ullNewPos, m_fCursorHigh);
	else
		MakeSelection(ullNewPos, ullNewPos, 1, 1);
}

void CHexCtrl::CaretMoveDown()
{
	ULONGLONG ullNewPos = m_fMutable ? m_ullCaretPos : m_ullLMouseClick;
	ullNewPos += m_dwCapacity;

	if (m_fMutable)
		MoveCaret(ullNewPos, m_fCursorHigh);
	else
		MakeSelection(ullNewPos, ullNewPos, 1, 1);
}

void CHexCtrl::Undo()
{
	if (m_deqUndo.empty())
		return;

	const auto& refUndo = m_deqUndo.back();

	//Making new Redo data snapshot.
	const auto& refRedo = m_deqRedo.emplace_back(std::make_unique<std::vector<UNDOSTRUCT>>());

	//Bad alloc may happen here!!!
	//If there is no more free memory, just clear the vec and return.
	try
	{
		for (auto& i : *refUndo)
		{
			refRedo->emplace_back(UNDOSTRUCT { i.ullOffset, { } });
			refRedo->back().vecData.resize(i.vecData.size());
		}
	}
	catch (const std::bad_alloc&)
	{
		m_deqRedo.clear();
		return;
	}

	ULONGLONG ullIndex { 0 };
	for (const auto& iter : *refUndo)
	{
		const auto& refUndoData = iter.vecData;
		const auto pData = GetData({ iter.ullOffset, refUndoData.size() });
		//Fill Redo with the data.
		std::copy_n(pData, refUndoData.size(), refRedo->at(static_cast<size_t>(ullIndex)).vecData.begin());
		//Undo the data.
		std::copy_n(refUndoData.begin(), refUndoData.size(), pData);
		SetDataVirtual(pData, { iter.ullOffset, refUndoData.size() });
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

	for (const auto& iter : *refRedo)
	{
		const auto& refData = iter.vecData;
		const auto pData = GetData({ iter.ullOffset, refData.size() });
		std::copy_n(refData.begin(), refData.size(), pData);
		SetDataVirtual(pData, { iter.ullOffset, refData.size() });
	}

	m_deqRedo.pop_back();
	RedrawWindow();
}

void CHexCtrl::SnapshotUndo(const std::vector<HEXSPANSTRUCT>& vecSpan)
{
	auto ullTotalSize = std::accumulate(vecSpan.begin(), vecSpan.end(), 0ULL,
		[](ULONGLONG ullSumm, const HEXSPANSTRUCT& ref) {return ullSumm + ref.ullSize; });

	//Check for very big undo size.
	if (ullTotalSize > 1024 * 1024 * 10)
		return;

	//If Undo deque size is exceeding max limit,
	//remove first snapshot from the beginning (the oldest one).
	if (m_deqUndo.size() > static_cast<size_t>(m_dwUndoMax))
		m_deqUndo.pop_front();

	//Making new Undo data snapshot.
	const auto& refUndo = m_deqUndo.emplace_back(std::make_unique<std::vector<UNDOSTRUCT>>());

	//Bad alloc may happen here!!!
	try
	{
		for (const auto& iterSel : vecSpan)
		{
			refUndo->emplace_back(UNDOSTRUCT { iterSel.ullOffset, { } });
			refUndo->back().vecData.resize(static_cast<size_t>(iterSel.ullSize));
		}
	}
	catch (const std::bad_alloc&)
	{
		m_deqUndo.clear();
		m_deqRedo.clear();
		return;
	}

	ULONGLONG ullIndex { 0 };
	for (const auto& iterSel : vecSpan)
	{
		std::byte* pData = GetData(iterSel);
		std::copy_n(pData, iterSel.ullSize, refUndo->at(static_cast<size_t>(ullIndex)).vecData.begin());
		++ullIndex;
	}
}

bool CHexCtrl::IsCurTextArea()const
{
	return m_fCursorTextArea;
}

void CHexCtrl::MakeSelection(ULONGLONG ullClick, ULONGLONG ullStart, ULONGLONG ullSize, ULONGLONG ullLines, bool fScroll, bool fGoToStart)
{
	if (ullClick >= m_ullDataSize || ullStart >= m_ullDataSize || ullSize == 0)
		return;

	if ((ullStart + ullSize) > m_ullDataSize)
		ullSize = m_ullDataSize - ullStart;

	ULONGLONG ullEnd = ullStart + ullSize; //ullEnd is exclusive - ).
	m_ullLMouseClick = ullClick;
	m_ullCaretPos = ullStart;

	std::vector<HEXSPANSTRUCT> vecSelection;
	for (ULONGLONG i = 0; i < ullLines; ++i)
		vecSelection.emplace_back(HEXSPANSTRUCT { ullStart + m_dwCapacity * i, ullSize });
	m_pSelection->SetSelection(vecSelection);

	UpdateInfoText();

	//Don't need to scroll if it's mouse selection, or just selection assignment.
	//If fScroll is true then do scrolling according to the made selection.
	if (fScroll)
	{
		//fGoToStart means centralize scroll position on the screen (used in Search()).
		if (fGoToStart)
			GoToOffset(ullStart);
		else
		{
			auto ullCurrScrollV = m_pScrollV->GetScrollPos();
			if (auto iMod = ullCurrScrollV % m_sizeLetter.cy; iMod > 0)
				ullCurrScrollV -= iMod;
			auto ullCurrScrollH = m_pScrollH->GetScrollPos();

			int iCx, iCy;
			HexChunkPoint(ullStart, iCx, iCy);
			CRect rcClient;
			GetClientRect(&rcClient);

			//New scroll depending on selection direction: top <-> bottom.
			ULONGLONG ullMaxV = ullCurrScrollV + rcClient.Height() - m_iHeightBottomOffArea - m_iStartWorkAreaY -
				((rcClient.Height() - m_iStartWorkAreaY - m_iHeightBottomOffArea) % m_sizeLetter.cy);
			ULONGLONG ullNewStartV = ullStart / m_dwCapacity * m_sizeLetter.cy;
			ULONGLONG ullNewEndV = (ullEnd - 1) / m_dwCapacity * m_sizeLetter.cy;
			auto ullNewScrollV = ullCurrScrollV;

			if (ullStart == ullClick)
			{
				if (ullNewEndV >= ullMaxV)
					ullNewScrollV = ullCurrScrollV + m_sizeLetter.cy;
				else if (ullNewStartV < ullCurrScrollV)
					ullNewScrollV = ullCurrScrollV - m_sizeLetter.cy;
			}
			else
			{
				if (ullNewStartV < ullCurrScrollV)
					ullNewScrollV = ullCurrScrollV - m_sizeLetter.cy;
				else
					ullNewScrollV = ullCurrScrollV + m_sizeLetter.cy;
			}

			ULONGLONG ullNewScrollH { };
			int iMaxClientX = rcClient.Width() - m_iSizeHexByte;
			if (iCx >= iMaxClientX)
				ullNewScrollH = ullCurrScrollH + (iCx - iMaxClientX);
			else if (iCx < 0)
				ullNewScrollH = ullCurrScrollH + iCx;
			else
				ullNewScrollH = ullCurrScrollH;

			if (ullNewScrollV != ullCurrScrollV)
				m_pScrollV->SetScrollPos(ullNewScrollV);
			if (m_pScrollH->IsVisible() && !IsCurTextArea())
				m_pScrollH->SetScrollPos(ullNewScrollH);
		}
	}

	MsgWindowNotify(HEXCTRL_MSG_SELECTION);
	OnCaretPosChange(ullStart);
}

void CHexCtrl::SetSelHighlight(const std::vector<HEXSPANSTRUCT>& vecSelHighlight)
{
	m_pSelection->SetSelHighlight(vecSelHighlight);
}

void CHexCtrl::ClearSelHighlight()
{
	m_pSelection->ClearSelHighlight();
}

void CHexCtrl::GoToOffset(ULONGLONG ullOffset)
{
	if (!IsDataSet())
		return;

	ULONGLONG ullNewStartV = ullOffset / m_dwCapacity * m_sizeLetter.cy;
	ULONGLONG ullNewScrollV { 0 }, ullNewScrollH { };

	//To prevent negative numbers.
	if (ullNewStartV > m_iHeightWorkArea / 2)
	{
		ullNewScrollV = ullNewStartV - m_iHeightWorkArea / 2;
		ullNewScrollV -= ullNewScrollV % m_sizeLetter.cy;
	}

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

	MakeSelection(0, 0, m_ullDataSize, 1, false);
}

void CHexCtrl::FillWithZeros()
{
	if (!IsDataSet())
		return;

	MODIFYSTRUCT hms;
	hms.vecSpan = m_pSelection->GetData();
	hms.ullDataSize = 1;
	hms.enModifyMode = EModifyMode::MODIFY_REPEAT;
	std::byte byteZero { 0 };
	hms.pData = &byteZero;
	Modify(hms);
}

void CHexCtrl::WstrCapacityFill()
{
	m_wstrCapacity.clear();
	m_wstrCapacity.reserve(static_cast<size_t>(m_dwCapacity) * 3);
	for (unsigned iter = 0; iter < m_dwCapacity; ++iter)
	{
		if (m_fOffsetAsHex)
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
		}
		else
		{
			if (iter < 10)
			{
				m_wstrCapacity += L" ";
				m_wstrCapacity += g_pwszHexMap[iter];
			}
			else
			{
				wchar_t buff[4];
				swprintf_s(buff, 4, L"%u", iter);
				m_wstrCapacity += buff;
			}
		}

		//Additional space between hex chunk blocks.
		if ((((iter + 1) % static_cast<DWORD>(m_enShowMode)) == 0) && (iter < (m_dwCapacity - 1)))
			m_wstrCapacity += L" ";

		//Additional space between hex halves.
		if (m_enShowMode == EHexShowMode::ASBYTE && iter == (m_dwCapacityBlockSize - 1))
			m_wstrCapacity += L"  ";
	}
}

bool CHexCtrl::IsSectorVisible()const
{
	return m_fSectorVisible;
}

void CHexCtrl::UpdateSectorVisible()
{
	if (m_dwSectorSize && (m_dwSectorSize % m_dwCapacity == 0) && m_dwSectorSize >= m_dwCapacity)
	{
		m_fSectorVisible = true;
		m_ullTotalSectors = m_ullDataSize % m_dwSectorSize ? m_ullDataSize / m_dwSectorSize + 1 : m_ullDataSize / m_dwSectorSize;
	}
	else
		m_fSectorVisible = false;

	UpdateInfoText();
}

void CHexCtrl::Print()
{
	DWORD dwFlags;
	if (m_pSelection->HasSelection())
		dwFlags = PD_ALLPAGES | PD_CURRENTPAGE | PD_PAGENUMS | PD_SELECTION;
	else
		dwFlags = PD_ALLPAGES | PD_CURRENTPAGE | PD_PAGENUMS | PD_NOSELECTION;

	CPrintDialog dlg(FALSE, dwFlags, this);
	dlg.m_pd.nMaxPage = 0xffff;
	dlg.m_pd.nFromPage = 1;
	dlg.m_pd.nToPage = 1;

	if (dlg.DoModal() == IDCANCEL) {
		DeleteDC(dlg.m_pd.hDC);
		return;
	}

	HDC hdcPrinter = dlg.GetPrinterDC();
	if (hdcPrinter == nullptr) {
		MessageBoxW(L"No printer found!");
		return;
	}

	CDC dcPr;
	dcPr.Attach(hdcPrinter);
	CDC* pDC = &dcPr;

	DOCINFOW di { };
	di.cbSize = sizeof(DOCINFOW);
	di.lpszDocName = L"HexCtrl";

	if (dcPr.StartDocW(&di) < 0) {
		dcPr.AbortDoc();
		dcPr.DeleteDC();
		return;
	}

	const CSize sizePrintDpi = { GetDeviceCaps(dcPr, LOGPIXELSX), GetDeviceCaps(dcPr, LOGPIXELSY) };
	//const CSize sizePaper = { GetDeviceCaps(dcPr, PHYSICALWIDTH), GetDeviceCaps(dcPr, PHYSICALHEIGHT) };
	CRect rcPrint(CPoint(GetDeviceCaps(dcPr, PHYSICALOFFSETX), GetDeviceCaps(dcPr, PHYSICALOFFSETY)),
		CSize(GetDeviceCaps(dcPr, HORZRES), GetDeviceCaps(dcPr, VERTRES)));
	CDC* pCurrDC = GetDC();
	auto iScreenDpiY = GetDeviceCaps(pCurrDC->m_hDC, LOGPIXELSY);
	ReleaseDC(pCurrDC);
	int iRatio = sizePrintDpi.cy / iScreenDpiY;

	CFont fontMain; //Main font for printing
	LOGFONTW lf { };
	m_fontMain.GetLogFont(&lf);
	lf.lfHeight *= iRatio;
	fontMain.CreateFontIndirectW(&lf);
	CFont fontInfo; //Info font for printing
	LOGFONTW lfInfo { };
	m_fontInfo.GetLogFont(&lfInfo);
	lfInfo.lfHeight *= iRatio;
	fontInfo.CreateFontIndirectW(&lfInfo);

	ULONGLONG ullStartLine { };
	ULONGLONG ullEndLine { };
	ullStartLine = GetTopLine();
	ullEndLine = GetBottomLine();
	auto stOldLetter = m_sizeLetter;
	auto ullTotalLines = m_ullDataSize / m_dwCapacity;

	rcPrint.bottom -= 200; //Ajust bottom indent of the printing rect.
	RecalcPrint(pDC, &fontMain, &fontInfo, rcPrint);
	auto iLinesInPage = m_iHeightWorkArea / m_sizeLetter.cy;
	ULONGLONG ullTotalPages = ullTotalLines / iLinesInPage + 1;
	int iPagesToPrint { };

	if (dlg.PrintAll())
	{
		iPagesToPrint = static_cast<int>(ullTotalPages);
		ullStartLine = 0;
	}
	else if (dlg.PrintRange())
	{
		auto iFromPage = dlg.GetFromPage() - 1;
		auto iToPage = dlg.GetToPage();
		if (iFromPage <= ullTotalPages) //Checks for out-of-range pages user input.
		{
			iPagesToPrint = iToPage - iFromPage;
			if (iPagesToPrint + iFromPage > ullTotalPages)
				iPagesToPrint = static_cast<int>(ullTotalPages - iFromPage);
		}
		ullStartLine = iFromPage * iLinesInPage;
	}

	const auto iIndentX = 100;
	const auto iIndentY = 100;
	pDC->SetMapMode(MM_TEXT);
	if (dlg.PrintSelection())
	{
		pDC->StartPage();
		pDC->OffsetViewportOrg(iIndentX, iIndentY);

		auto ullSelStart = m_pSelection->GetSelectionStart();
		auto ullSelSize = m_pSelection->GetSelectionSize();
		ullStartLine = ullSelStart / m_dwCapacity;

		DWORD dwModStart = ullSelStart % m_dwCapacity;
		DWORD dwLines = static_cast<DWORD>(ullSelSize) / m_dwCapacity;
		if ((dwModStart + static_cast<DWORD>(ullSelSize)) > m_dwCapacity)
			++dwLines;
		if ((ullSelSize % m_dwCapacity) + dwModStart > m_dwCapacity)
			++dwLines;
		if (!dwLines)
			dwLines = 1;
		ullEndLine = ullStartLine + dwLines;

		DrawWindow(pDC, &fontMain, &fontInfo);
		auto clBkrOld = m_stColor.clrBkSelected;
		auto clTextOld = m_stColor.clrTextSelected;
		m_stColor.clrBkSelected = m_stColor.clrBk;
		m_stColor.clrTextSelected = m_stColor.clrTextHex;
		DrawOffsets(pDC, &fontMain, ullStartLine, ullEndLine);
		DrawSelection(pDC, &fontMain, ullStartLine, ullEndLine);
		m_stColor.clrBkSelected = clBkrOld;
		m_stColor.clrTextSelected = clTextOld;

		pDC->EndPage();
	}
	else
	{
		for (int i = 0; i < iPagesToPrint; ++i)
		{
			pDC->StartPage();
			pDC->OffsetViewportOrg(iIndentX, iIndentY);
			ullEndLine = ullStartLine + iLinesInPage;
			//If m_dwDataCount is really small we adjust ullEndLine to be not bigger than maximum allowed.
			if (ullEndLine > ullTotalLines)
				ullEndLine = (m_ullDataSize % m_dwCapacity) ? ullTotalLines + 1 : ullTotalLines;

			DrawWindow(pDC, &fontMain, &fontInfo);
			DrawOffsets(pDC, &fontMain, ullStartLine, ullEndLine);
			DrawHexAscii(pDC, &fontMain, ullStartLine, ullEndLine);
			DrawBookmarks(pDC, &fontMain, ullStartLine, ullEndLine);
			DrawSelection(pDC, &fontMain, ullStartLine, ullEndLine);
			DrawCursor(pDC, &fontMain, ullStartLine, ullEndLine);
			DrawDataInterpret(pDC, &fontMain, ullStartLine, ullEndLine);
			DrawSectorLines(pDC, ullStartLine, ullEndLine);

			ullStartLine += iLinesInPage;
			pDC->OffsetViewportOrg(-iIndentX, -iIndentY);
			pDC->EndPage();
		}
	}

	pDC->EndDoc();
	pDC->DeleteDC();

	m_sizeLetter = stOldLetter;
	RecalcAll();
}

void CHexCtrl::TtOffsetShow(bool fShow)
{
	if (fShow)
	{
		CPoint ptScreen;
		GetCursorPos(&ptScreen);

		static wchar_t warrOffset[40] { L"Offset: " };
		UllToWchars(GetTopLine() * m_dwCapacity, &warrOffset[8], static_cast<size_t>(m_dwOffsetBytes), m_fOffsetAsHex);
		m_stToolInfoOffset.lpszText = warrOffset;
		m_wndTtOffset.SendMessageW(TTM_TRACKPOSITION, 0, static_cast<LPARAM>(MAKELONG(ptScreen.x - 5, ptScreen.y - 20)));
		m_wndTtOffset.SendMessageW(TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(&m_stToolInfoOffset));
	}
	m_wndTtOffset.SendMessageW(TTM_TRACKACTIVATE, static_cast<WPARAM>(fShow), reinterpret_cast<LPARAM>(&m_stToolInfoOffset));
}

void CHexCtrl::TtBkmShow(bool fShow, POINT pt)
{
	if (fShow)
	{
		m_wndTtBkm.SendMessageW(TTM_TRACKPOSITION, 0, static_cast<LPARAM>(MAKELONG(pt.x, pt.y)));
		m_wndTtBkm.SendMessageW(TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(&m_stToolInfoBkm));
		m_wndTtBkm.SendMessageW(TTM_TRACKACTIVATE, static_cast<WPARAM>(TRUE), reinterpret_cast<LPARAM>(&m_stToolInfoBkm));
		SetTimer(ID_TOOLTIP_BKM, 300, nullptr);
	}
	else
	{
		m_pBkmCurrTt = nullptr;
		m_wndTtBkm.SendMessageW(TTM_TRACKACTIVATE, (WPARAM)FALSE, reinterpret_cast<LPARAM>(&m_stToolInfoBkm));
		KillTimer(ID_TOOLTIP_BKM);
	}
}