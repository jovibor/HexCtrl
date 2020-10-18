/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#include "stdafx.h"
#include "../dep/rapidjson-amalgamation.h"
#include "CHexBookmarks.h"
#include "CHexCtrl.h"
#include "CHexSelection.h"
#include "CScrollEx.h"
#include "Dialogs/CHexDlgAbout.h"
#include "Dialogs/CHexDlgBkmMgr.h"
#include "Dialogs/CHexDlgCallback.h"
#include "Dialogs/CHexDlgDataInterpret.h"
#include "Dialogs/CHexDlgEncoding.h"
#include "Dialogs/CHexDlgFillData.h"
#include "Dialogs/CHexDlgGoTo.h"
#include "Dialogs/CHexDlgOperations.h"
#include "Dialogs/CHexDlgSearch.h"
#include "Helper.h"
#include "strsafe.h"
#include <cassert>
#include <cctype>
#include <fstream>
#include <numeric>
#include <thread>
#pragma comment(lib, "Dwmapi.lib")

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

namespace HEXCTRL
{
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
	namespace INTERNAL
	{
		enum class CHexCtrl::EClipboard : WORD
		{
			COPY_HEX, COPY_HEXLE, COPY_HEXFMT, COPY_TEXT, COPY_BASE64,
			COPY_CARR, COPY_GREPHEX, COPY_PRNTSCRN, PASTE_HEX, PASTE_TEXT
		};

		//Struct for UNDO command routine.
		struct CHexCtrl::SUNDO
		{
			ULONGLONG              ullOffset { }; //Start byte to apply Undo to.
			std::vector<std::byte> vecData { };   //Data for Undo.
		};

		//Struct for resources auto deletion on destruction.
		struct CHexCtrl::SHBITMAP
		{
			SHBITMAP() = default;
			~SHBITMAP() { ::DeleteObject(m_hBmp); }
			SHBITMAP& operator=(HBITMAP hBmp) { m_hBmp = hBmp; return *this; }
			operator HBITMAP()const { return m_hBmp; }
			HBITMAP m_hBmp { };
		};

		//Key bindings.
		struct CHexCtrl::SKEYBIND
		{
			EHexCmd       eCmd { };
			WORD          wMenuID { };
			unsigned char uKey { };
			bool          fCtrl { };
			bool          fShift { };
			bool          fAlt { };
		};

		constexpr auto WSTR_HEXCTRL_CLASSNAME { L"HexCtrl" }; //HexControl Class name.
		constexpr auto ID_TOOLTIP_BKM { 0x01ULL };            //Tooltip ID for bookmarks.
		constexpr auto ID_TOOLTIP_OFFSET { 0x02ULL };         //Tooltip ID for offset.
	}
}

/************************************************************************
* CHexCtrl class implementation.                                        *
************************************************************************/
BEGIN_MESSAGE_MAP(CHexCtrl, CWnd)
	ON_WM_ACTIVATE()
	ON_WM_CHAR()
	ON_WM_CLOSE()
	ON_WM_CONTEXTMENU()
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

	auto hInst = AfxGetInstanceHandle();
	WNDCLASSEXW wc { };
	if (!::GetClassInfoExW(hInst, WSTR_HEXCTRL_CLASSNAME, &wc))
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
		if (!RegisterClassExW(&wc)) {
			MessageBoxW(L"HexControl RegisterClassExW error.", L"Error", MB_ICONERROR);
			return;
		}
	}
}

ULONGLONG CHexCtrl::BkmAdd(const HEXBKMSTRUCT& hbs, bool fRedraw)
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	return m_pBookmarks->Add(hbs, fRedraw);
}

void CHexCtrl::BkmClearAll()
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_pBookmarks->ClearAll();
}

auto CHexCtrl::BkmGetByID(ULONGLONG ullID)->HEXBKMSTRUCT*
{
	assert(IsCreated());
	if (!IsCreated())
		return nullptr;

	return m_pBookmarks->GetByID(ullID);
}

auto CHexCtrl::BkmGetByIndex(ULONGLONG ullIndex)->HEXBKMSTRUCT*
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
		return { };

	return m_pBookmarks->GetCount();
}

auto CHexCtrl::BkmHitTest(ULONGLONG ullOffset)->HEXBKMSTRUCT*
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
	Redraw();
}

bool CHexCtrl::Create(const HEXCREATESTRUCT& hcs)
{
	assert(!IsCreated()); //Already created.
	if (IsCreated())
		return false;

	//Menus
	if (!m_menuMain.LoadMenuW(MAKEINTRESOURCEW(IDR_HEXCTRL_MENU)))
	{
		MessageBoxW(L"HexControl LoadMenu(IDR_HEXCTRL_MENU) failed.", L"Error", MB_ICONERROR);
		return false;
	}
	MENUITEMINFOW mii { };
	mii.cbSize = sizeof(MENUITEMINFOW);
	mii.fMask = MIIM_BITMAP;

	auto hInst = AfxGetInstanceHandle();
	mii.hbmpItem = m_umapHBITMAP[IDM_HEXCTRL_CLIPBOARD_COPYHEX] =
		static_cast<HBITMAP>(LoadImageW(hInst, MAKEINTRESOURCEW(IDB_HEXCTRL_MENU_COPY), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
	m_menuMain.SetMenuItemInfoW(IDM_HEXCTRL_CLIPBOARD_COPYHEX, &mii);
	mii.hbmpItem = m_umapHBITMAP[IDM_HEXCTRL_CLIPBOARD_PASTEHEX] =
		static_cast<HBITMAP>(LoadImageW(hInst, MAKEINTRESOURCEW(IDB_HEXCTRL_MENU_PASTE), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
	m_menuMain.SetMenuItemInfoW(IDM_HEXCTRL_CLIPBOARD_PASTEHEX, &mii);
	mii.hbmpItem = m_umapHBITMAP[IDM_HEXCTRL_MODIFY_FILLZEROS] =
		static_cast<HBITMAP>(LoadImageW(hInst, MAKEINTRESOURCEW(IDB_HEXCTRL_MENU_FILLZEROS), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
	m_menuMain.SetMenuItemInfoW(IDM_HEXCTRL_MODIFY_FILLZEROS, &mii);

	m_hwndMsg = hcs.hwndParent;
	m_stColor = hcs.stColor;
	m_enShowMode = hcs.enShowMode;
	m_dbWheelRatio = hcs.dbWheelRatio;

	auto dwStyle = hcs.dwStyle;
	//1. WS_POPUP style is vital for GetParent to work properly in EHexCreateMode::CREATE_POPUP mode.
	//   Without this style GetParent/GetOwner always return 0, no matter whether pParentWnd is provided to CreateWindowEx or not.
	//2. Created HexCtrl window will always overlap (be on top of) its parent, or owner, window 
	//   if pParentWnd is set (is not nullptr) in CreateWindowEx.
	//3. To force HexCtrl window on taskbar the WS_EX_APPWINDOW extended window style must be set.
	CStringW wstrError;
	CRect rc = hcs.rect;
	UINT uID { 0 };
	switch (hcs.enCreateMode)
	{
	case EHexCreateMode::CREATE_POPUP:
		dwStyle |= WS_VISIBLE | WS_POPUP | WS_OVERLAPPEDWINDOW;
		if (rc.IsRectNull())
		{	//If initial rect is null, and it's a float window, then place it at screen center.
			const auto iPosX = GetSystemMetrics(SM_CXSCREEN) / 4;
			const auto iPosY = GetSystemMetrics(SM_CYSCREEN) / 4;
			rc.SetRect(iPosX, iPosY, iPosX * 3, iPosY * 3);
		}
		break;
	case EHexCreateMode::CREATE_CHILD:
		dwStyle |= WS_VISIBLE | WS_CHILD;
		uID = hcs.uID;
		break;
	case EHexCreateMode::CREATE_CUSTOMCTRL:
		//If it's a Custom Control in dialog, there is no need to create a window, just subclassing.
		if (!SubclassDlgItem(hcs.uID, CWnd::FromHandle(hcs.hwndParent)))
			wstrError.Format(L"HexCtrl (ID:%u) SubclassDlgItem failed.\r\nCheck CreateDialogCtrl parameters.", hcs.uID);
		break;
	}

	//Creating window.
	if (hcs.enCreateMode == EHexCreateMode::CREATE_CHILD || hcs.enCreateMode == EHexCreateMode::CREATE_POPUP)
		if (!CWnd::CreateEx(hcs.dwExStyle, WSTR_HEXCTRL_CLASSNAME, L"HexControl", dwStyle, rc,
			CWnd::FromHandle(hcs.hwndParent), uID))
			wstrError.Format(L"HexCtrl (ID:%u) CreateEx failed.\r\nCheck HEXCREATESTRUCT parameters.", hcs.uID);

	if (wstrError.GetLength()) //If there was some Creation error.
	{
		MessageBoxW(wstrError, L"Error", MB_ICONERROR);
		return false;
	}

	m_wndTtBkm.CreateEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr,
		/*TTS_BALLOON |*/ TTS_NOANIMATE | TTS_NOFADE | TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, nullptr);
	SetWindowTheme(m_wndTtBkm.m_hWnd, nullptr, L""); //To prevent Windows from changing theme of Balloon window.
	m_stToolInfoBkm.cbSize = TTTOOLINFOW_V1_SIZE;
	m_stToolInfoBkm.uFlags = TTF_TRACK;
	m_stToolInfoBkm.uId = ID_TOOLTIP_BKM;
	m_wndTtBkm.SendMessageW(TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&m_stToolInfoBkm));
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
	m_wndTtOffset.SendMessageW(TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&m_stToolInfoOffset));
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

	//ScrollBars should be created here, after the main window has already been created (to attach to), to avoid assertions.
	m_pScrollV->Create(this, SB_VERT, 0, 0, 0); //Actual sizes are set in RecalcAll().
	m_pScrollH->Create(this, SB_HORZ, 0, 0, 0);
	m_pScrollV->AddSibling(m_pScrollH.get());
	m_pScrollH->AddSibling(m_pScrollV.get());

	//All dialogs are created after the main window, to set the parent window correctly.
	m_pDlgBookmarkMgr->Create(IDD_HEXCTRL_BKMMGR, this, &*m_pBookmarks);
	m_pDlgEncoding->Create(IDD_HEXCTRL_ENCODING, this);
	m_pDlgDataInterpret->Create(IDD_HEXCTRL_DATAINTERPRET, this);
	m_pDlgFillData->Create(IDD_HEXCTRL_FILLDATA, this);
	m_pDlgOpers->Create(IDD_HEXCTRL_OPERATIONS, this);
	m_pDlgSearch->Create(IDD_HEXCTRL_SEARCH, this);
	m_pBookmarks->Attach(this);
	m_pSelection->Attach(this);

	m_fCreated = true;
	SetShowMode(m_enShowMode);
	SetEncoding(-1);
	SetConfig(L"");

	return true;
}

bool CHexCtrl::CreateDialogCtrl(UINT uCtrlID, HWND hParent)
{
	assert(!IsCreated()); //Already created.
	if (IsCreated())
		return false;

	HEXCREATESTRUCT hcs;
	hcs.hwndParent = hParent;
	hcs.uID = uCtrlID;
	hcs.enCreateMode = EHexCreateMode::CREATE_CUSTOMCTRL;

	return Create(hcs);
}

void CHexCtrl::Destroy()
{
	delete this;
}

void CHexCtrl::ExecuteCmd(EHexCmd eCmd)
{
	assert(IsCreated());
	if (!IsCreated() || !IsCmdAvail(eCmd))
		return;

	switch (eCmd)
	{
	case EHexCmd::CMD_DLG_SEARCH:
		m_pDlgSearch->ShowWindow(SW_SHOW);
		break;
	case EHexCmd::CMD_SEARCH_NEXT:
		m_pDlgSearch->Search(true);
		break;
	case EHexCmd::CMD_SEARCH_PREV:
		m_pDlgSearch->Search(false);
		break;
	case EHexCmd::CMD_NAV_DLG_GOTO:
	{
		CHexDlgGoTo dlgGoTo(this);
		if (dlgGoTo.DoModal() == IDOK)
		{
			auto ullOffset = dlgGoTo.GetResult();
			SetCaretPos(ullOffset);
			if (!IsOffsetVisible(ullOffset))
				GoToOffset(ullOffset);
		}
	}
	break;
	case EHexCmd::CMD_NAV_DATABEG:
		CaretToDataBeg();
		break;
	case EHexCmd::CMD_NAV_DATAEND:
		CaretToDataEnd();
		break;
	case EHexCmd::CMD_NAV_PAGEBEG:
		CaretToPageBeg();
		break;
	case EHexCmd::CMD_NAV_PAGEEND:
		CaretToPageEnd();
		break;
	case EHexCmd::CMD_NAV_LINEBEG:
		CaretToLineBeg();
		break;
	case EHexCmd::CMD_NAV_LINEEND:
		CaretToLineEnd();
		break;
	case EHexCmd::CMD_SHOWDATA_BYTE:
		SetShowMode(EHexShowMode::ASBYTE);
		break;
	case EHexCmd::CMD_SHOWDATA_WORD:
		SetShowMode(EHexShowMode::ASWORD);
		break;
	case EHexCmd::CMD_SHOWDATA_DWORD:
		SetShowMode(EHexShowMode::ASDWORD);
		break;
	case EHexCmd::CMD_SHOWDATA_QWORD:
		SetShowMode(EHexShowMode::ASQWORD);
		break;
	case EHexCmd::CMD_BKM_ADD:
		m_pBookmarks->Add(HEXBKMSTRUCT { m_pSelection->GetData() });
		break;
	case EHexCmd::CMD_BKM_REMOVE:
		m_pBookmarks->Remove(m_optRMouseClick.value_or(GetCaretPos()));
		m_optRMouseClick.reset();
		break;
	case EHexCmd::CMD_BKM_NEXT:
		m_pBookmarks->GoNext();
		break;
	case EHexCmd::CMD_BKM_PREV:
		m_pBookmarks->GoPrev();
		break;
	case EHexCmd::CMD_BKM_CLEARALL:
		m_pBookmarks->ClearAll();
		break;
	case EHexCmd::CMD_BKM_DLG_MANAGER:
		m_pDlgBookmarkMgr->ShowWindow(SW_SHOW);
		break;
	case EHexCmd::CMD_CLIPBOARD_COPY_HEX:
		ClipboardCopy(EClipboard::COPY_HEX);
		break;
	case EHexCmd::CMD_CLIPBOARD_COPY_HEXLE:
		ClipboardCopy(EClipboard::COPY_HEXLE);
		break;
	case EHexCmd::CMD_CLIPBOARD_COPY_HEXFMT:
		ClipboardCopy(EClipboard::COPY_HEXFMT);
		break;
	case EHexCmd::CMD_CLIPBOARD_COPY_TEXT:
		ClipboardCopy(EClipboard::COPY_TEXT);
		break;
	case EHexCmd::CMD_CLIPBOARD_COPY_BASE64:
		ClipboardCopy(EClipboard::COPY_BASE64);
		break;
	case EHexCmd::CMD_CLIPBOARD_COPY_CARR:
		ClipboardCopy(EClipboard::COPY_CARR);
		break;
	case EHexCmd::CMD_CLIPBOARD_COPY_GREPHEX:
		ClipboardCopy(EClipboard::COPY_GREPHEX);
		break;
	case EHexCmd::CMD_CLIPBOARD_COPY_PRNTSCRN:
		ClipboardCopy(EClipboard::COPY_PRNTSCRN);
		break;
	case EHexCmd::CMD_CLIPBOARD_PASTE_HEX:
		ClipboardPaste(EClipboard::PASTE_HEX);
		break;
	case EHexCmd::CMD_CLIPBOARD_PASTE_TEXT:
		ClipboardPaste(EClipboard::PASTE_TEXT);
		break;
	case EHexCmd::CMD_MODIFY_DLG_OPERS:
		m_pDlgOpers->ShowWindow(SW_SHOW);
		break;
	case EHexCmd::CMD_MODIFY_FILLZEROS:
		FillWithZeros();
		break;
	case EHexCmd::CMD_MODIFY_DLG_FILLDATA:
		m_pDlgFillData->ShowWindow(SW_SHOW);
		break;
	case EHexCmd::CMD_MODIFY_UNDO:
		Undo();
		break;
	case EHexCmd::CMD_MODIFY_REDO:
		Redo();
		break;
	case EHexCmd::CMD_SEL_MARKSTART:
		m_pSelection->SetSelectionStart(GetCaretPos());
		break;
	case EHexCmd::CMD_SEL_MARKEND:
		m_pSelection->SetSelectionEnd(GetCaretPos());
		break;
	case EHexCmd::CMD_SEL_ALL:
		SelAll();
		break;
	case EHexCmd::CMD_SEL_ADDLEFT:
		SelAddLeft();
		break;
	case EHexCmd::CMD_SEL_ADDRIGHT:
		SelAddRight();
		break;
	case EHexCmd::CMD_SEL_ADDUP:
		SelAddUp();
		break;
	case EHexCmd::CMD_SEL_ADDDOWN:
		SelAddDown();
		break;
	case EHexCmd::CMD_DLG_DATAINTERPRET:
		m_pDlgDataInterpret->ShowWindow(SW_SHOW);
		break;
	case EHexCmd::CMD_DLG_ENCODING:
		m_pDlgEncoding->ShowWindow(SW_SHOW);
		break;
	case EHexCmd::CMD_APPEARANCE_FONTINC:
		SetFontSize(GetFontSize() + 2);
		break;
	case EHexCmd::CMD_APPEARANCE_FONTDEC:
		SetFontSize(GetFontSize() - 2);
		break;
	case EHexCmd::CMD_APPEARANCE_CAPACINC:
		SetCapacity(m_dwCapacity + 1);
		break;
	case EHexCmd::CMD_APPEARANCE_CAPACDEC:
		SetCapacity(m_dwCapacity - 1);
		break;
	case EHexCmd::CMD_DLG_PRINT:
		Print();
		break;
	case EHexCmd::CMD_DLG_ABOUT:
	{
		CHexDlgAbout dlgAbout(this);
		dlgAbout.DoModal();
	}
	break;
	case EHexCmd::CMD_CARET_LEFT:
		CaretMoveLeft();
		break;
	case EHexCmd::CMD_CARET_RIGHT:
		CaretMoveRight();
		break;
	case EHexCmd::CMD_CARET_UP:
		CaretMoveUp();
		break;
	case EHexCmd::CMD_CARET_DOWN:
		CaretMoveDown();
		break;
	case EHexCmd::CMD_SCROLL_PAGEUP:
		m_pScrollV->ScrollPageUp();
		break;
	case EHexCmd::CMD_SCROLL_PAGEDOWN:
		m_pScrollV->ScrollPageDown();
		break;
	}
}

DWORD CHexCtrl::GetCapacity()const
{
	assert(IsCreated());
	if (!IsCreated())
		return 1; //Capacity can't be less than one.

	return m_dwCapacity;
}

ULONGLONG CHexCtrl::GetCaretPos()const
{
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet())
		return { };

	return m_ullCaretPos;
}

auto CHexCtrl::GetColors()const->HEXCOLORSSTRUCT
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	return m_stColor;
}

auto CHexCtrl::GetDataSize()const->ULONGLONG
{
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet())
		return { };

	return m_ullDataSize;
}

int CHexCtrl::GetEncoding()const
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	return m_iCodePage;
}

long CHexCtrl::GetFontSize()const
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	return m_lFontSize;
}

HMENU CHexCtrl::GetMenuHandle()const
{
	assert(IsCreated());
	if (!IsCreated())
		return nullptr;

	return m_menuMain.GetSubMenu(0)->GetSafeHmenu();
}

auto CHexCtrl::GetPagesCount()const->ULONGLONG
{
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet() || GetPageSize() == 0)
		return { };

	return (m_ullDataSize % m_dwPageSize) ? m_ullDataSize / m_dwPageSize + 1 : m_ullDataSize / m_dwPageSize;
}

auto CHexCtrl::GetPagePos()const->ULONGLONG
{
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet())
		return { };

	return GetCaretPos() / GetPageSize();
}

DWORD CHexCtrl::GetPageSize()const
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	return m_dwPageSize;
}

auto CHexCtrl::GetSelection()const->std::vector<HEXSPANSTRUCT>
{
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet())
		return { };

	return m_pSelection->GetData();
}

auto CHexCtrl::GetShowMode()const->EHexShowMode
{
	assert(IsCreated());
	if (!IsCreated())
		return EHexShowMode { };

	return m_enShowMode;
}

HWND CHexCtrl::GetWindowHandle(EHexWnd enWnd)const
{
	assert(IsCreated());
	if (!IsCreated())
		return nullptr;

	HWND hWnd { };
	switch (enWnd)
	{
	case EHexWnd::WND_MAIN:
		hWnd = m_hWnd;
		break;
	case EHexWnd::DLG_BKMMANAGER:
		hWnd = m_pDlgBookmarkMgr->m_hWnd;
		break;
	case EHexWnd::DLG_DATAINTERPRET:
		hWnd = m_pDlgDataInterpret->m_hWnd;
		break;
	case EHexWnd::DLG_FILLDATA:
		hWnd = m_pDlgFillData->m_hWnd;
		break;
	case EHexWnd::DLG_OPERS:
		hWnd = m_pDlgOpers->m_hWnd;
		break;
	case EHexWnd::DLG_SEARCH:
		hWnd = m_pDlgSearch->m_hWnd;
		break;
	case EHexWnd::DLG_ENCODING:
		hWnd = m_pDlgEncoding->m_hWnd;
		break;
	}

	return hWnd;
}

void CHexCtrl::GoToOffset(ULONGLONG ullOffset)
{
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet())
		return;

	ULONGLONG ullNewStartV = ullOffset / m_dwCapacity * m_sizeLetter.cy;
	ULONGLONG ullNewScrollV { 0ULL }, ullNewScrollH { };

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

auto CHexCtrl::HitTest(POINT pt, bool fScreen)const->std::optional<HEXHITTESTSTRUCT>
{
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet())
		return { };

	if (fScreen)
		ScreenToClient(&pt);

	return HitTest(pt);
}

bool CHexCtrl::IsCmdAvail(EHexCmd eCmd)const
{
	assert(IsCreated());
	if (!IsCreated())
		return false;

	bool fDataSet = IsDataSet();
	bool fMutable = fDataSet ? IsMutable() : false;
	bool fSelection = fDataSet && m_pSelection->HasSelection();
	bool fAvail;

	switch (eCmd)
	{
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
	case EHexCmd::CMD_CLIPBOARD_COPY_TEXT:
	case EHexCmd::CMD_CLIPBOARD_COPY_BASE64:
	case EHexCmd::CMD_CLIPBOARD_COPY_CARR:
	case EHexCmd::CMD_CLIPBOARD_COPY_GREPHEX:
	case EHexCmd::CMD_CLIPBOARD_COPY_PRNTSCRN:
		fAvail = fSelection;
		break;
	case EHexCmd::CMD_CLIPBOARD_PASTE_HEX:
	case EHexCmd::CMD_CLIPBOARD_PASTE_TEXT:
		fAvail = fMutable && fSelection && IsClipboardFormatAvailable(CF_TEXT);
		break;
	case EHexCmd::CMD_MODIFY_DLG_OPERS:
	case EHexCmd::CMD_MODIFY_FILLZEROS:
	case EHexCmd::CMD_MODIFY_DLG_FILLDATA:
		fAvail = fMutable && fSelection;
		break;
	case EHexCmd::CMD_MODIFY_UNDO:
		fAvail = !m_deqUndo.empty();
		break;
	case EHexCmd::CMD_MODIFY_REDO:
		fAvail = !m_deqRedo.empty();
		break;
	case EHexCmd::CMD_DLG_SEARCH:
	case EHexCmd::CMD_NAV_DLG_GOTO:
	case EHexCmd::CMD_NAV_DATABEG:
	case EHexCmd::CMD_NAV_DATAEND:
	case EHexCmd::CMD_NAV_LINEBEG:
	case EHexCmd::CMD_NAV_LINEEND:
	case EHexCmd::CMD_BKM_DLG_MANAGER:
	case EHexCmd::CMD_SEL_MARKSTART:
	case EHexCmd::CMD_SEL_MARKEND:
	case EHexCmd::CMD_SEL_ALL:
	case EHexCmd::CMD_DLG_DATAINTERPRET:
		fAvail = fDataSet;
		break;
	case EHexCmd::CMD_SEARCH_NEXT:
	case EHexCmd::CMD_SEARCH_PREV:
		fAvail = m_pDlgSearch->IsSearchAvail();
		break;
	case EHexCmd::CMD_NAV_PAGEBEG:
	case EHexCmd::CMD_NAV_PAGEEND:
		fAvail = fDataSet && GetPagesCount() > 0;
		break;
	default:
		fAvail = true;
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
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet())
		return false;

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

	if (IsDataSet())
	{
		WCHAR wBuff[256];
		m_wstrInfo.clear();
		m_wstrInfo.reserve(std::size(wBuff));

		swprintf_s(wBuff, std::size(wBuff), IsOffsetAsHex() ? L"Caret: 0x%llX; " : L"Caret: %llu; ", GetCaretPos());
		m_wstrInfo = wBuff;

		if (IsPageVisible()) //Page/Sector
		{
			swprintf_s(wBuff, std::size(wBuff), IsOffsetAsHex() ? L"%s: 0x%llX/0x%llX; " : L"%s: %llu/%llu; ",
				m_wstrPageName.data(), GetPagePos(), GetPagesCount());
			m_wstrInfo += wBuff;
		}

		if (m_pSelection->HasSelection())
		{
			swprintf_s(wBuff, std::size(wBuff),
				IsOffsetAsHex() ? L"Selected: 0x%llX [0x%llX-0x%llX]; " : L"Selected: %llu [%llu-%llu]; ",
				m_pSelection->GetSelectionSize(), m_pSelection->GetSelectionStart(), m_pSelection->GetSelectionEnd() - 1);
			m_wstrInfo += wBuff;
		}

		m_wstrInfo += IsMutable() ? L"RW;" : L"RO;"; //Mutable state
	}
	else
		m_wstrInfo.clear();

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

	WstrCapacityFill();
	RecalcAll();
	MsgWindowNotify(HEXCTRL_MSG_VIEWCHANGE); //Indicates to parent that view has changed.
}

void CHexCtrl::SetCaretPos(ULONGLONG ullOffset, bool fHighLow, bool fRedraw)
{
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet() || ullOffset >= GetDataSize())
		return;

	m_ullCaretPos = ullOffset;
	if (m_fMutable)
	{
		m_fCursorHigh = fHighLow;
		if (fRedraw)
			Redraw();
	}
	else
	{
		m_ullLMouseClick = ullOffset;
		SetSelection({ { ullOffset, 1 } }, fRedraw);
	}
	OnCaretPosChange(ullOffset);
}

void CHexCtrl::SetColors(const HEXCOLORSSTRUCT& clr)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_stColor = clr;
	RedrawWindow();
}

bool CHexCtrl::SetConfig(std::wstring_view wstrPath)
{
	assert(IsCreated());
	if (!IsCreated())
		return false;

	//Mapping between stringified EHexCmd::* and its value-menuID pairs.
	const std::unordered_map<std::string, std::pair<EHexCmd, WORD>> umapCmdMenu {
		{ "CMD_DLG_SEARCH", { EHexCmd::CMD_DLG_SEARCH, static_cast<WORD>(IDM_HEXCTRL_SEARCH) } },
		{ "CMD_SEARCH_NEXT", { EHexCmd::CMD_SEARCH_NEXT, static_cast<WORD>(0) } },
		{ "CMD_SEARCH_PREV", { EHexCmd::CMD_SEARCH_PREV, static_cast<WORD>(0) } },
		{ "CMD_NAV_DLG_GOTO", { EHexCmd::CMD_NAV_DLG_GOTO, static_cast<WORD>(IDM_HEXCTRL_NAV_GOTO) } },
		{ "CMD_NAV_DATABEG", { EHexCmd::CMD_NAV_DATABEG, static_cast<WORD>(IDM_HEXCTRL_NAV_DATABEG) } },
		{ "CMD_NAV_DATAEND", { EHexCmd::CMD_NAV_DATAEND, static_cast<WORD>(IDM_HEXCTRL_NAV_DATAEND) } },
		{ "CMD_NAV_PAGEBEG", { EHexCmd::CMD_NAV_PAGEBEG, static_cast<WORD>(IDM_HEXCTRL_NAV_PAGEBEG) } },
		{ "CMD_NAV_PAGEEND", { EHexCmd::CMD_NAV_PAGEEND, static_cast<WORD>(IDM_HEXCTRL_NAV_PAGEEND) } },
		{ "CMD_NAV_LINEBEG", { EHexCmd::CMD_NAV_LINEBEG, static_cast<WORD>(IDM_HEXCTRL_NAV_LINEBEG) } },
		{ "CMD_NAV_LINEEND", { EHexCmd::CMD_NAV_LINEEND, static_cast<WORD>(IDM_HEXCTRL_NAV_LINEEND) } },
		{ "CMD_SHOWDATA_BYTE", { EHexCmd::CMD_SHOWDATA_BYTE, static_cast<WORD>(IDM_HEXCTRL_SHOWAS_BYTE) } },
		{ "CMD_SHOWDATA_WORD", { EHexCmd::CMD_SHOWDATA_WORD, static_cast<WORD>(IDM_HEXCTRL_SHOWAS_WORD) } },
		{ "CMD_SHOWDATA_DWORD", { EHexCmd::CMD_SHOWDATA_DWORD, static_cast<WORD>(IDM_HEXCTRL_SHOWAS_DWORD) } },
		{ "CMD_SHOWDATA_QWORD", { EHexCmd::CMD_SHOWDATA_QWORD, static_cast<WORD>(IDM_HEXCTRL_SHOWAS_QWORD) } },
		{ "CMD_BKM_ADD", { EHexCmd::CMD_BKM_ADD, static_cast<WORD>(IDM_HEXCTRL_BOOKMARKS_ADD) } },
		{ "CMD_BKM_REMOVE", { EHexCmd::CMD_BKM_REMOVE, static_cast<WORD>(IDM_HEXCTRL_BOOKMARKS_REMOVE) } },
		{ "CMD_BKM_NEXT", { EHexCmd::CMD_BKM_NEXT, static_cast<WORD>(IDM_HEXCTRL_BOOKMARKS_NEXT) } },
		{ "CMD_BKM_PREV", { EHexCmd::CMD_BKM_PREV, static_cast<WORD>(IDM_HEXCTRL_BOOKMARKS_PREV) } },
		{ "CMD_BKM_CLEARALL", { EHexCmd::CMD_BKM_CLEARALL, static_cast<WORD>(IDM_HEXCTRL_BOOKMARKS_CLEARALL) } },
		{ "CMD_BKM_DLG_MANAGER", { EHexCmd::CMD_BKM_DLG_MANAGER, static_cast<WORD>(IDM_HEXCTRL_BOOKMARKS_MANAGER) } },
		{ "CMD_CLIPBOARD_COPY_HEX", { EHexCmd::CMD_CLIPBOARD_COPY_HEX, static_cast<WORD>(IDM_HEXCTRL_CLIPBOARD_COPYHEX) } },
		{ "CMD_CLIPBOARD_COPY_HEXLE", { EHexCmd::CMD_CLIPBOARD_COPY_HEXLE, static_cast<WORD>(IDM_HEXCTRL_CLIPBOARD_COPYHEXLE) } },
		{ "CMD_CLIPBOARD_COPY_HEXFMT", { EHexCmd::CMD_CLIPBOARD_COPY_HEXFMT, static_cast<WORD>(IDM_HEXCTRL_CLIPBOARD_COPYHEXFMT) } },
		{ "CMD_CLIPBOARD_COPY_TEXT", { EHexCmd::CMD_CLIPBOARD_COPY_TEXT, static_cast<WORD>(IDM_HEXCTRL_CLIPBOARD_COPYTEXT) } },
		{ "CMD_CLIPBOARD_COPY_BASE64", { EHexCmd::CMD_CLIPBOARD_COPY_BASE64, static_cast<WORD>(IDM_HEXCTRL_CLIPBOARD_COPYBASE64) } },
		{ "CMD_CLIPBOARD_COPY_CARR", { EHexCmd::CMD_CLIPBOARD_COPY_CARR, static_cast<WORD>(IDM_HEXCTRL_CLIPBOARD_COPYCARRAY) } },
		{ "CMD_CLIPBOARD_COPY_GREPHEX", { EHexCmd::CMD_CLIPBOARD_COPY_GREPHEX, static_cast<WORD>(IDM_HEXCTRL_CLIPBOARD_COPYGREPHEX) } },
		{ "CMD_CLIPBOARD_COPY_PRNTSCRN", { EHexCmd::CMD_CLIPBOARD_COPY_PRNTSCRN, static_cast<WORD>(IDM_HEXCTRL_CLIPBOARD_COPYPRNTSCRN) } },
		{ "CMD_CLIPBOARD_PASTE_HEX", { EHexCmd::CMD_CLIPBOARD_PASTE_HEX, static_cast<WORD>(IDM_HEXCTRL_CLIPBOARD_PASTEHEX) } },
		{ "CMD_CLIPBOARD_PASTE_TEXT", { EHexCmd::CMD_CLIPBOARD_PASTE_TEXT, static_cast<WORD>(IDM_HEXCTRL_CLIPBOARD_PASTETEXT) } },
		{ "CMD_MODIFY_DLG_OPERS", { EHexCmd::CMD_MODIFY_DLG_OPERS, static_cast<WORD>(IDM_HEXCTRL_MODIFY_OPERS) } },
		{ "CMD_MODIFY_FILLZEROS", { EHexCmd::CMD_MODIFY_FILLZEROS, static_cast<WORD>(IDM_HEXCTRL_MODIFY_FILLZEROS) } },
		{ "CMD_MODIFY_DLG_FILLDATA", { EHexCmd::CMD_MODIFY_DLG_FILLDATA, static_cast<WORD>(IDM_HEXCTRL_MODIFY_FILLDATA) } },
		{ "CMD_MODIFY_UNDO", { EHexCmd::CMD_MODIFY_UNDO, static_cast<WORD>(IDM_HEXCTRL_MODIFY_UNDO) } },
		{ "CMD_MODIFY_REDO", { EHexCmd::CMD_MODIFY_REDO, static_cast<WORD>(IDM_HEXCTRL_MODIFY_REDO) } },
		{ "CMD_SEL_MARKSTART", { EHexCmd::CMD_SEL_MARKSTART, static_cast<WORD>(IDM_HEXCTRL_SEL_MARKSTART) } },
		{ "CMD_SEL_MARKEND", { EHexCmd::CMD_SEL_MARKEND, static_cast<WORD>(IDM_HEXCTRL_SEL_MARKEND) } },
		{ "CMD_SEL_ALL", { EHexCmd::CMD_SEL_ALL, static_cast<WORD>(IDM_HEXCTRL_SEL_ALL) } },
		{ "CMD_SEL_ADDLEFT", { EHexCmd::CMD_SEL_ADDLEFT, static_cast<WORD>(0) } },
		{ "CMD_SEL_ADDRIGHT", { EHexCmd::CMD_SEL_ADDRIGHT, static_cast<WORD>(0) } },
		{ "CMD_SEL_ADDUP", { EHexCmd::CMD_SEL_ADDUP, static_cast<WORD>(0) } },
		{ "CMD_SEL_ADDDOWN", { EHexCmd::CMD_SEL_ADDDOWN, static_cast<WORD>(0) } },
		{ "CMD_DLG_DATAINTERPRET", { EHexCmd::CMD_DLG_DATAINTERPRET, static_cast<WORD>(IDM_HEXCTRL_DATAINTERPRET) } },
		{ "CMD_DLG_ENCODING", { EHexCmd::CMD_DLG_ENCODING, static_cast<WORD>(IDM_HEXCTRL_ENCODING) } },
		{ "CMD_APPEARANCE_FONTINC", { EHexCmd::CMD_APPEARANCE_FONTINC, static_cast<WORD>(IDM_HEXCTRL_APPEARANCE_FONTINC) } },
		{ "CMD_APPEARANCE_FONTDEC", { EHexCmd::CMD_APPEARANCE_FONTDEC, static_cast<WORD>(IDM_HEXCTRL_APPEARANCE_FONTDEC) } },
		{ "CMD_APPEARANCE_CAPACINC", { EHexCmd::CMD_APPEARANCE_CAPACINC, static_cast<WORD>(IDM_HEXCTRL_APPEARANCE_CAPACINC) } },
		{ "CMD_APPEARANCE_CAPACDEC", { EHexCmd::CMD_APPEARANCE_CAPACDEC, static_cast<WORD>(IDM_HEXCTRL_APPEARANCE_CAPACDEC) } },
		{ "CMD_DLG_PRINT", { EHexCmd::CMD_DLG_PRINT, static_cast<WORD>(IDM_HEXCTRL_PRINT) } },
		{ "CMD_DLG_ABOUT", { EHexCmd::CMD_DLG_ABOUT, static_cast<WORD>(IDM_HEXCTRL_ABOUT) } },
		{ "CMD_CARET_LEFT", { EHexCmd::CMD_CARET_LEFT, static_cast<WORD>(0) } },
		{ "CMD_CARET_RIGHT", { EHexCmd::CMD_CARET_RIGHT, static_cast<WORD>(0) } },
		{ "CMD_CARET_UP", { EHexCmd::CMD_CARET_UP, static_cast<WORD>(0) } },
		{ "CMD_CARET_DOWN", { EHexCmd::CMD_CARET_DOWN, static_cast<WORD>(0) } },
		{ "CMD_SCROLL_PAGEUP", { EHexCmd::CMD_SCROLL_PAGEUP, static_cast<WORD>(0) } },
		{ "CMD_SCROLL_PAGEDOWN", { EHexCmd::CMD_SCROLL_PAGEDOWN, static_cast<WORD>(0) } }
	};

	const std::unordered_map<std::string_view, std::pair<UCHAR, std::wstring_view>> umapKeys
	{
		{ { "ctrl" }, { VK_CONTROL, { L"Ctrl" } } },
		{ { "shift" }, { VK_SHIFT, { L"Shift" } } },
		{ { "alt" }, { VK_MENU, { L"Alt" } } },
		{ { "tab" }, { VK_TAB, { L"Tab" } } },
		{ { "enter" }, { VK_RETURN, { L"Enter" } } },
		{ { "esc" }, { VK_ESCAPE, { L"Esc" } } },
		{ { "space" }, { VK_SPACE, { L"Space" } } },
		{ { "backspace" }, { VK_BACK, { L"Backspace" } } },
		{ { "delete" }, { VK_DELETE, { L"Delete" } } },
		{ { "insert" }, { VK_INSERT, { L"Insert" } } },
		{ { "f1" }, { VK_F1, { L"F1" } } },
		{ { "f2" }, { VK_F2, { L"F2" } } },
		{ { "f3" }, { VK_F3, { L"F3" } } },
		{ { "f4" }, { VK_F4, { L"F4" } } },
		{ { "f5" }, { VK_F5, { L"F5" } } },
		{ { "f6" }, { VK_F6, { L"F6" } } },
		{ { "f7" }, { VK_F7, { L"F7" } } },
		{ { "f8" }, { VK_F8, { L"F8" } } },
		{ { "f9" }, { VK_F9, { L"F9" } } },
		{ { "f10" }, { VK_F10, { L"F10" } } },
		{ { "right" }, { VK_RIGHT, { L"Right Arrow" } } },
		{ { "left" }, { VK_LEFT, { L"Left Arrow" } } },
		{ { "up" }, { VK_UP, { L"Up Arrow" } } },
		{ { "down" }, { VK_DOWN, { L"Down Arrow" } } },
		{ { "pageup" }, { VK_PRIOR, { L"PageUp" } } },
		{ { "pagedown" }, { VK_NEXT, { L"PageDown" } } },
		{ { "home" }, { VK_HOME, { L"Home" } } },
		{ { "end" }, { VK_END, { L"End" } } },
		{ { "plus" }, { VK_OEM_PLUS, { L"Plus" } } },
		{ { "minus" }, { VK_OEM_MINUS, { L"Minus" } } },
		{ { "num_plus" }, { VK_ADD, { L"Num Plus" } } },
		{ { "num_minus" }, { VK_SUBTRACT, { L"Num Minus" } } }
	};

	auto lmbJson = [&](auto&& refJson, auto&& lmbParse)
	{
		std::vector<SKEYBIND> vecRet { };
		for (auto iterJ = refJson.MemberBegin(); iterJ != refJson.MemberEnd(); ++iterJ) //JSON data iterating.
		{
			if (auto iterCmd = umapCmdMenu.find(iterJ->name.GetString()); iterCmd != umapCmdMenu.end())
				for (auto iterValue = iterJ->value.Begin(); iterValue != iterJ->value.End(); ++iterValue) //Array iterating.
					if (auto opt = lmbParse(iterValue->GetString()); opt)
					{
						auto stKeyBind = opt.value();
						stKeyBind.eCmd = iterCmd->second.first;
						stKeyBind.wMenuID = iterCmd->second.second;
						vecRet.emplace_back(stKeyBind);
					}
		}

		return vecRet;
	};
	auto lmbParse = [&](std::string_view str)->std::optional<SKEYBIND>
	{
		if (str.empty())
			return { };

		SKEYBIND stRet { };
		auto iSize = str.size();
		size_t nPosStart { 0 }; //Next position to start search for '+' sign.
		auto nSubWords = std::count(str.begin(), str.end(), '+') + 1; //How many sub-words (divided by '+')?
		for (auto i = 0; i < nSubWords; ++i)
		{
			size_t nSizeSubWord;
			auto nPosNext = str.find('+', nPosStart);
			if (nPosNext == std::string_view::npos)
				nSizeSubWord = iSize - nPosStart;
			else
				nSizeSubWord = nPosNext - nPosStart;

			auto strSubWord = str.substr(nPosStart, nSizeSubWord);
			nPosStart = nPosNext + 1;

			if (strSubWord.size() == 1)
				stRet.uKey = static_cast<UCHAR>(std::toupper(strSubWord[0])); //Binding keys are in uppercase.
			else if (auto iter = umapKeys.find(strSubWord); iter != umapKeys.end())
			{
				const auto uChar = iter->second.first;
				switch (uChar)
				{
				case VK_CONTROL:
					stRet.fCtrl = true;
					break;
				case VK_SHIFT:
					stRet.fShift = true;
					break;
				case VK_MENU:
					stRet.fAlt = true;
					break;
				default:
					stRet.uKey = uChar;
				}
			}
		}

		return stRet;
	};
	auto lmbVecEqualize = [](std::vector<SKEYBIND>& vecMain, const std::vector<SKEYBIND>& vecFrom)
	{
		//Remove everything from vecMain that exists in vecFrom.
		for (const auto& iterFrom : vecFrom)
			vecMain.erase(std::remove_if(vecMain.begin(), vecMain.end(),
				[&](const SKEYBIND& ref) {return ref.eCmd == iterFrom.eCmd; }),
				vecMain.end());

		//Add everything from vecFrom to vecMain.
		vecMain.insert(vecMain.end(), vecFrom.begin(), vecFrom.end());
	};

	rapidjson::Document doc;
	bool fRet { false };
	if (wstrPath.empty()) //Default keybind.json from resources.
	{
		const auto hInst = AfxGetInstanceHandle();
		if (const auto hRes = FindResourceW(hInst, MAKEINTRESOURCEW(IDR_JSON_KEYBIND), L"JSON"); hRes != nullptr)
		{
			if (const auto hData = LoadResource(hInst, hRes); hData != nullptr)
			{
				const auto nSize = static_cast<std::size_t>(SizeofResource(hInst, hRes));
				const auto pData = static_cast<char*>(LockResource(hData));

				if (doc.Parse(pData, nSize); !doc.IsNull()) //Parse all default keybindings.
				{
					m_vecKeyBind.clear();
					for (const auto& iter : umapCmdMenu) //Fill m_vecKeyBind with all possible commands from umapCmdMenu.
					{
						SKEYBIND st;
						st.eCmd = iter.second.first;
						st.wMenuID = iter.second.second;
						m_vecKeyBind.emplace_back(st);
					}
					fRet = true;
				}
			}
		}
	}
	else if (std::ifstream ifs(std::wstring { wstrPath }); ifs.is_open())
	{
		rapidjson::IStreamWrapper isw { ifs };
		if (doc.ParseStream(isw); !doc.IsNull())
			fRet = true;
	}

	if (fRet)
	{
		lmbVecEqualize(m_vecKeyBind, lmbJson(doc, lmbParse));

		std::size_t i { 0 };
		for (const auto& iterMain : m_vecKeyBind)
		{
			//Check for previous same menu ID, to assign only one (first) keybinding for menu name.
			auto iterEnd = m_vecKeyBind.begin() + i++;
			if (auto iterTmp = std::find_if(m_vecKeyBind.begin(), iterEnd,
				[&](const SKEYBIND& ref) { return ref.wMenuID == iterMain.wMenuID; });
				iterTmp == iterEnd && iterMain.wMenuID != 0 && iterMain.uKey != 0)
			{
				CStringW wstrMenuName;
				m_menuMain.GetMenuStringW(iterMain.wMenuID, wstrMenuName, MF_BYCOMMAND);
				std::wstring wstr = wstrMenuName.GetString();
				if (const auto nPos = wstr.find('\t'); nPos != std::wstring::npos)
					wstr.erase(nPos);

				wstr += L'\t';
				if (iterMain.fCtrl)
					wstr += L"Ctrl+";
				if (iterMain.fShift)
					wstr += L"Shift+";
				if (iterMain.fAlt)
					wstr += L"Alt+";

				//Search for any special key names: 'Tab', 'Enter', etc...
				//If not found then it's just a char.
				if (auto iterUmap = std::find_if(umapKeys.begin(), umapKeys.end(), [&](const auto& ref)
					{ return ref.second.first == iterMain.uKey; }); iterUmap != umapKeys.end())
					wstr += iterUmap->second.second;
				else
					wstr += iterMain.uKey;

				//Modify menu with a new name (with shortcut appended) and old bitmap.
				MENUITEMINFOW mii { };
				mii.cbSize = sizeof(MENUITEMINFOW);
				mii.fMask = MIIM_BITMAP | MIIM_STRING;
				m_menuMain.GetMenuItemInfoW(iterMain.wMenuID, &mii);
				mii.dwTypeData = wstr.data();
				m_menuMain.SetMenuItemInfoW(iterMain.wMenuID, &mii);
			}
		}
	}

	return fRet;
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

	//Make selection during SetData if hds.stSelSpan.ullSize is provided.
	if (hds.stSelSpan.ullSize > 0)
	{
		SetSelection({ hds.stSelSpan });
		GoToOffset(hds.stSelSpan.ullOffset);
	}
}

void CHexCtrl::SetEncoding(int iCodePage)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	CPINFOEXW stCPInfo { };
	std::wstring_view wstrFormat { };
	if (iCodePage == -1)
		wstrFormat = L"ASCII";
	else if (GetCPInfoExW(static_cast<UINT>(iCodePage), 0, &stCPInfo) != 0 && stCPInfo.MaxCharSize == 1)
		wstrFormat = L"ASCII (%i)";

	if (!wstrFormat.empty())
	{
		m_iCodePage = iCodePage;
		WCHAR buff[32];
		swprintf_s(buff, std::size(buff), wstrFormat.data(), m_iCodePage);
		m_wstrTextTitle = buff;
		Redraw();
	}
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
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet())
		return;

	m_fMutable = fEnable;
	Redraw();
}

void CHexCtrl::SetPageSize(DWORD dwSize, std::wstring_view wstrName)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_dwPageSize = dwSize;
	m_wstrPageName = wstrName;
	if (IsDataSet())
		Redraw();
}

void CHexCtrl::SetSelection(const std::vector<HEXSPANSTRUCT>& vecSel, bool fRedraw)
{
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet())
		return;

	m_pSelection->SetSelection(vecSel, fRedraw);
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


/********************************************************************
* HexCtrl Private methods.
********************************************************************/

void CHexCtrl::AsciiChunkPoint(ULONGLONG ullOffset, int& iCx, int& iCy)const
{	//This func computes x and y pos of given Ascii chunk.
	DWORD dwMod = ullOffset % m_dwCapacity;
	iCx = static_cast<int>((m_iIndentAscii + dwMod * m_sizeLetter.cx) - m_pScrollH->GetScrollPos());

	auto ullScrollV = m_pScrollV->GetScrollPos();
	iCy = static_cast<int>((m_iStartWorkAreaY + (ullOffset / m_dwCapacity) * m_sizeLetter.cy) -
		(ullScrollV - (ullScrollV % m_sizeLetter.cy)));
}

auto CHexCtrl::BuildDataToDraw(ULONGLONG ullStartLine, int iLines)const->std::tuple<std::wstring, std::wstring>
{
	const auto ullOffsetStart = ullStartLine * m_dwCapacity; //Offset of the visible data to print.
	size_t sSizeData = static_cast<size_t>(iLines) * static_cast<size_t>(m_dwCapacity); //Size of the visible data to print.
	if (ullOffsetStart + sSizeData > m_ullDataSize)
		sSizeData = static_cast<size_t>(m_ullDataSize - ullOffsetStart);
	const auto pData = reinterpret_cast<PBYTE>(GetData({ ullOffsetStart, sSizeData })); //Pointer to data to print.

	std::wstring wstrHex { };
	wstrHex.reserve(sSizeData * 2);
	for (auto iterpData = pData; iterpData < pData + sSizeData; ++iterpData) //Converting bytes to Hexes.
	{
		wstrHex.push_back(g_pwszHexMap[(*iterpData >> 4) & 0x0F]);
		wstrHex.push_back(g_pwszHexMap[*iterpData & 0x0F]);
	}

	//Converting bytes to Text.
	UINT uCodePage = m_iCodePage == -1 ? CODEPAGE_DEFAULT : static_cast<UINT>(m_iCodePage);
	std::wstring wstrText(sSizeData, 0);
	MultiByteToWideChar(uCodePage, 0, reinterpret_cast<LPCCH>(pData),
		static_cast<int>(sSizeData), wstrText.data(), static_cast<int>(sSizeData));
	ReplaceUnprintable(wstrText, m_iCodePage == -1);

	return { std::move(wstrHex), std::move(wstrText) };
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
		ullChunks = (ullSize % ullSizeChunk) ? ullSize / ullSizeChunk + 1 : ullSize / ullSizeChunk;
	}
	break;
	}
}

void CHexCtrl::CaretMoveDown()
{
	auto ullOldPos = m_fMutable ? m_ullCaretPos : m_ullLMouseClick;
	auto ullNewPos = ullOldPos + m_dwCapacity >= GetDataSize() ? ullOldPos : ullOldPos + m_dwCapacity;
	SetCaretPos(ullNewPos, m_fCursorHigh, false);

	auto [i8VOld, i8HOld] = TestOffsetVis(ullOldPos);
	auto [i8VNew, i8HNew] = TestOffsetVis(ullNewPos);
	if (i8VOld == 0 && i8VNew != 0)
		m_pScrollV->ScrollLineDown();

	Redraw();
}

void CHexCtrl::CaretMoveLeft()
{
	auto ullOldPos = m_fMutable ? m_ullCaretPos : m_ullLMouseClick;
	auto ullNewPos { 0ULL };
	if (m_fMutable)
	{
		if (IsCurTextArea())
		{
			if (ullOldPos == 0) //To avoid underflow.
				return;
			ullNewPos = ullOldPos - 1;
		}
		else if (m_fCursorHigh)
		{
			if (ullOldPos == 0) //To avoid underflow.
				return;
			ullNewPos = ullOldPos - 1;
			m_fCursorHigh = false;
		}
		else
		{
			ullNewPos = ullOldPos;
			m_fCursorHigh = true;
		}
	}
	else if (ullOldPos > 0)
		ullNewPos = ullOldPos - 1;

	SetCaretPos(ullNewPos, m_fCursorHigh, false);

	auto [i8VOld, i8HOld] = TestOffsetVis(ullOldPos);
	auto [i8VNew, i8HNew] = TestOffsetVis(ullNewPos);
	if (i8VOld == 0 && i8VNew != 0)
		m_pScrollV->ScrollLineUp();
	else if (i8HNew != 0 && !IsCurTextArea()) //Do not horz scroll when in text area.
		ScrollOffsetH(ullNewPos);
	else
		Redraw();
}

void CHexCtrl::CaretMoveRight()
{
	auto ullOldPos = m_fMutable ? m_ullCaretPos : m_ullLMouseClick;
	auto ullNewPos { 0ULL };
	if (m_fMutable)
	{
		if (IsCurTextArea())
			ullNewPos = ullOldPos + 1;
		else if (m_fCursorHigh)
		{
			ullNewPos = ullOldPos;
			m_fCursorHigh = false;
		}
		else
		{
			ullNewPos = ullOldPos + 1;
			m_fCursorHigh = ullOldPos != GetDataSize() - 1; //Last byte case.
		}
	}
	else
		ullNewPos = ullOldPos + 1;

	if (auto ullDataSize = GetDataSize(); ullNewPos >= ullDataSize) //To avoid overflow.
		ullNewPos = ullDataSize - 1;

	SetCaretPos(ullNewPos, m_fCursorHigh, false);

	auto [i8VOld, i8HOld] = TestOffsetVis(ullOldPos);
	auto [i8VNew, i8HNew] = TestOffsetVis(ullNewPos);
	if (i8VOld == 0 && i8VNew != 0)
		m_pScrollV->ScrollLineDown();
	else if (i8HNew != 0 && !IsCurTextArea()) //Do not horz scroll when in text area.
		ScrollOffsetH(ullNewPos);
	else
		Redraw();
}

void CHexCtrl::CaretMoveUp()
{
	auto ullOldPos = m_fMutable ? m_ullCaretPos : m_ullLMouseClick;
	auto ullNewPos = ullOldPos >= m_dwCapacity ? ullOldPos - m_dwCapacity : ullOldPos;
	SetCaretPos(ullNewPos, m_fCursorHigh, false);

	auto [i8VOld, i8HOld] = TestOffsetVis(ullOldPos);
	auto [i8VNew, i8HNew] = TestOffsetVis(ullNewPos);
	if (i8VOld == 0 && i8VNew != 0)
		m_pScrollV->ScrollLineUp();

	Redraw();
}

void CHexCtrl::CaretToDataBeg()
{
	SetCaretPos(0, true);
	GoToOffset(0);
}

void CHexCtrl::CaretToDataEnd()
{
	auto ullPos = GetDataSize() - 1;
	SetCaretPos(ullPos);
	GoToOffset(ullPos);
}

void CHexCtrl::CaretToLineBeg()
{
	auto ullPos = GetCaretPos() - (GetCaretPos() % GetCapacity());
	SetCaretPos(ullPos);
	if (!IsOffsetVisible(ullPos))
		GoToOffset(ullPos);
}

void CHexCtrl::CaretToLineEnd()
{
	auto ullPos = GetCaretPos() + (GetCapacity() - (GetCaretPos() % GetCapacity())) - 1;
	if (ullPos >= GetDataSize())
		ullPos = GetDataSize() - 1;
	SetCaretPos(ullPos);
	if (!IsOffsetVisible(ullPos))
		GoToOffset(ullPos);
}

void CHexCtrl::CaretToPageBeg()
{
	if (GetPageSize() == 0)
		return;

	auto ullPos = GetCaretPos() - (GetCaretPos() % GetPageSize());
	SetCaretPos(ullPos);
	if (!IsOffsetVisible(ullPos))
		GoToOffset(ullPos);
}

void CHexCtrl::CaretToPageEnd()
{
	if (GetPageSize() == 0)
		return;

	auto ullPos = GetCaretPos() + (GetPageSize() - (GetCaretPos() % GetPageSize())) - 1;
	if (ullPos >= GetDataSize())
		ullPos = GetDataSize() - 1;
	SetCaretPos(ullPos);
	if (!IsOffsetVisible(ullPos))
		GoToOffset(ullPos);
}

void CHexCtrl::ClearSelHighlight()
{
	m_pSelection->ClearSelHighlight();
}

void CHexCtrl::ClipboardCopy(EClipboard enType)const
{
	if (!m_pSelection->HasSelection() || m_pSelection->GetSelectionSize() > 1024 * 1024 * 8) //8MB
	{
		::MessageBoxW(nullptr, L"Selection size is too big to copy.\r\nTry to select less.", L"Error", MB_ICONERROR);
		return;
	}

	std::wstring wstrData { };
	switch (enType)
	{
	case EClipboard::COPY_HEX:
		wstrData = CopyHex();
		break;
	case EClipboard::COPY_HEXLE:
		wstrData = CopyHexLE();
		break;
	case EClipboard::COPY_HEXFMT:
		wstrData = CopyHexFormatted();
		break;
	case EClipboard::COPY_TEXT:
		wstrData = CopyText();
		break;
	case EClipboard::COPY_BASE64:
		wstrData = CopyBase64();
		break;
	case EClipboard::COPY_CARR:
		wstrData = CopyCArr();
		break;
	case EClipboard::COPY_GREPHEX:
		wstrData = CopyGrepHex();
		break;
	case EClipboard::COPY_PRNTSCRN:
		wstrData = CopyPrintScreen();
		break;
	default:
		break;
	}

	const size_t sCharSize = sizeof(wchar_t);
	const size_t sMemSize = wstrData.size() * sCharSize + sCharSize;
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, sMemSize);
	if (!hMem)
	{
		::MessageBoxW(nullptr, L"GlobalAlloc error.", L"Error", MB_ICONERROR);
		return;
	}
	LPVOID lpMemLock = GlobalLock(hMem);
	if (!lpMemLock)
	{
		::MessageBoxW(nullptr, L"GlobalLock error.", L"Error", MB_ICONERROR);
		return;
	}
	std::memcpy(lpMemLock, wstrData.data(), sMemSize);
	GlobalUnlock(hMem);
	::OpenClipboard(m_hWnd);
	EmptyClipboard();
	SetClipboardData(CF_UNICODETEXT, hMem);
	CloseClipboard();
}

void CHexCtrl::ClipboardPaste(EClipboard enType)
{
	if (!m_fMutable || !m_pSelection->HasSelection())
		return;

	if (!::OpenClipboard(m_hWnd))
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
	SMODIFY hmd;

	std::string strData;
	switch (enType)
	{
	case EClipboard::PASTE_TEXT:
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

auto CHexCtrl::CopyBase64()const->std::wstring
{
	std::wstring wstrData;
	auto ullSelSize = m_pSelection->GetSelectionSize();

	wstrData.reserve(static_cast<size_t>(ullSelSize) * 2);
	const wchar_t* const pwszBase64Map { L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" };
	unsigned int uValA = 0;
	int iValB = -6;
	for (unsigned i = 0; i < ullSelSize; ++i)
	{
		uValA = (uValA << 8) + GetData<BYTE>(m_pSelection->GetOffsetByIndex(i));
		iValB += 8;
		while (iValB >= 0)
		{
			wstrData += pwszBase64Map[(uValA >> iValB) & 0x3F];
			iValB -= 6;
		}
	}

	if (iValB > -6)
		wstrData += pwszBase64Map[((uValA << 8) >> (iValB + 8)) & 0x3F];
	while (wstrData.size() % 4)
		wstrData += '=';

	return wstrData;
}

auto CHexCtrl::CopyCArr()const->std::wstring
{
	std::wstring wstrData;
	auto ullSelSize = m_pSelection->GetSelectionSize();

	wstrData.reserve(static_cast<size_t>(ullSelSize) * 3 + 64);
	wstrData = L"unsigned char data[";
	wchar_t arrSelectionNum[8] { };
	swprintf_s(arrSelectionNum, std::size(arrSelectionNum), L"%llu", ullSelSize);
	wstrData += arrSelectionNum;
	wstrData += L"] = {\r\n";

	for (unsigned i = 0; i < ullSelSize; ++i)
	{
		wstrData += L"0x";
		auto chByte = GetData<BYTE>(m_pSelection->GetOffsetByIndex(i));
		wstrData += g_pwszHexMap[(chByte & 0xF0) >> 4];
		wstrData += g_pwszHexMap[(chByte & 0x0F)];
		if (i < ullSelSize - 1)
			wstrData += L",";
		if ((i + 1) % 16 == 0)
			wstrData += L"\r\n";
		else
			wstrData += L" ";
	}
	if (wstrData.back() != '\n') //To prevent double new line if ullSelSize % 16 == 0
		wstrData += L"\r\n";
	wstrData += L"};";

	return wstrData;
}

auto CHexCtrl::CopyGrepHex()const->std::wstring
{
	std::wstring wstrData;
	auto ullSelSize = m_pSelection->GetSelectionSize();

	wstrData.reserve(static_cast<size_t>(ullSelSize) * 2);
	for (unsigned i = 0; i < ullSelSize; ++i)
	{
		wstrData += L"\\x";
		auto chByte = GetData<BYTE>(m_pSelection->GetOffsetByIndex(i));
		wstrData += g_pwszHexMap[(chByte & 0xF0) >> 4];
		wstrData += g_pwszHexMap[(chByte & 0x0F)];
	}

	return wstrData;
}

auto CHexCtrl::CopyHex()const->std::wstring
{
	std::wstring wstrData;
	auto ullSelSize = m_pSelection->GetSelectionSize();

	wstrData.reserve(static_cast<size_t>(ullSelSize) * 2);
	for (unsigned i = 0; i < ullSelSize; ++i)
	{
		auto chByte = GetData<BYTE>(m_pSelection->GetOffsetByIndex(i));
		wstrData += g_pwszHexMap[(chByte & 0xF0) >> 4];
		wstrData += g_pwszHexMap[(chByte & 0x0F)];
	}

	return wstrData;
}

auto CHexCtrl::CopyHexFormatted()const->std::wstring
{
	std::wstring wstrData;
	auto ullSelStart = m_pSelection->GetSelectionStart();
	auto ullSelSize = m_pSelection->GetSelectionSize();

	wstrData.reserve(static_cast<size_t>(ullSelSize) * 3);
	if (m_fSelectionBlock)
	{
		DWORD dwTail = m_pSelection->GetLineLength();
		for (unsigned i = 0; i < ullSelSize; ++i)
		{
			auto chByte = GetData<BYTE>(m_pSelection->GetOffsetByIndex(i));
			wstrData += g_pwszHexMap[(chByte & 0xF0) >> 4];
			wstrData += g_pwszHexMap[(chByte & 0x0F)];

			if (i < (ullSelSize - 1) && (dwTail - 1) != 0)
				if (((m_pSelection->GetLineLength() - dwTail + 1) % static_cast<DWORD>(m_enShowMode)) == 0) //Add space after hex full chunk, ShowAs_size depending.
					wstrData += L" ";
			if (--dwTail == 0 && i < (ullSelSize - 1)) //Next row.
			{
				wstrData += L"\r\n";
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
			wstrData.insert(0, static_cast<size_t>(dwCount), ' ');
		}

		for (unsigned i = 0; i < ullSelSize; ++i)
		{
			auto chByte = GetData<BYTE>(m_pSelection->GetOffsetByIndex(i));
			wstrData += g_pwszHexMap[(chByte & 0xF0) >> 4];
			wstrData += g_pwszHexMap[(chByte & 0x0F)];

			if (i < (ullSelSize - 1) && (dwTail - 1) != 0)
			{
				if (m_enShowMode == EHexShowMode::ASBYTE && dwTail == dwNextBlock)
					wstrData += L"   "; //Additional spaces between halves. Only in ASBYTE view mode.
				else if (((m_dwCapacity - dwTail + 1) % static_cast<DWORD>(m_enShowMode)) == 0) //Add space after hex full chunk, ShowAs_size depending.
					wstrData += L" ";
			}
			if (--dwTail == 0 && i < (ullSelSize - 1)) //Next row.
			{
				wstrData += L"\r\n";
				dwTail = m_dwCapacity;
			}
		}
	}

	return wstrData;
}

auto CHexCtrl::CopyHexLE()const->std::wstring
{
	std::wstring wstrData;
	auto ullSelSize = m_pSelection->GetSelectionSize();

	wstrData.reserve(static_cast<size_t>(ullSelSize) * 2);
	for (int i = static_cast<int>(ullSelSize); i > 0; --i)
	{
		auto chByte = GetData<BYTE>(m_pSelection->GetOffsetByIndex(i - 1));
		wstrData += g_pwszHexMap[(chByte & 0xF0) >> 4];
		wstrData += g_pwszHexMap[(chByte & 0x0F)];
	}

	return wstrData;
}

auto CHexCtrl::CopyPrintScreen()const->std::wstring
{
	if (m_fSelectionBlock) //Only works with classical selection.
		return { };

	auto ullSelStart = m_pSelection->GetSelectionStart();
	auto ullSelSize = m_pSelection->GetSelectionSize();

	std::wstring wstrRet { };
	wstrRet.reserve(static_cast<size_t>(ullSelSize) * 4);
	wstrRet = L"Offset";
	wstrRet.insert(0, (static_cast<size_t>(m_dwOffsetDigits) - wstrRet.size()) / 2, ' ');
	wstrRet.insert(wstrRet.size(), static_cast<size_t>(m_dwOffsetDigits) - wstrRet.size(), ' ');
	wstrRet += L"   "; //Spaces to Capacity.
	wstrRet += m_wstrCapacity;
	wstrRet += L"   "; //Spaces to ASCII.
	if (auto iSize = static_cast<int>(m_dwCapacity) - static_cast<int>(m_wstrTextTitle.size()); iSize > 0)
		wstrRet.insert(wstrRet.size(), static_cast<size_t>(iSize / 2), ' ');
	wstrRet += m_wstrTextTitle;
	wstrRet += L"\r\n";

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
	std::wstring wstrDataText;
	size_t sIndexToPrint { };
	const auto& [wstrHex, wstrText] = BuildDataToDraw(ullStartLine, static_cast<int>(dwLines));

	for (DWORD iterLines = 0; iterLines < dwLines; ++iterLines)
	{
		wchar_t pwszOffset[32] { }; //To be enough for max as Hex and as Decimals.
		UllToWchars(ullStartLine * m_dwCapacity + m_dwCapacity * iterLines,
			pwszOffset, static_cast<size_t>(m_dwOffsetBytes), m_fOffsetAsHex);
		wstrRet += pwszOffset;
		wstrRet.insert(wstrRet.size(), 3, ' ');

		for (unsigned iterChunks = 0; iterChunks < m_dwCapacity; ++iterChunks)
		{
			if (dwModStart == 0 && sIndexToPrint < ullSelSize)
			{
				wstrRet += wstrHex[sIndexToPrint * 2];
				wstrRet += wstrHex[sIndexToPrint * 2 + 1];
				wstrDataText += wstrText[sIndexToPrint];
				++sIndexToPrint;
			}
			else
			{
				wstrRet += L"  ";
				wstrDataText += L" ";
				--dwModStart;
			}

			//Additional space between grouped Hex chunks.
			if (((iterChunks + 1) % static_cast<DWORD>(m_enShowMode)) == 0 && iterChunks < (m_dwCapacity - 1))
				wstrRet += L" ";

			//Additional space between capacity halves, only with ASBYTE representation.
			if (m_enShowMode == EHexShowMode::ASBYTE && iterChunks == (m_dwCapacityBlockSize - 1))
				wstrRet += L"  ";
		}
		wstrRet += L"   "; //Ascii beginning.
		wstrRet += wstrDataText;
		if (iterLines < dwLines - 1)
			wstrRet += L"\r\n";
		wstrDataText.clear();
	}

	return wstrRet;
}

auto CHexCtrl::CopyText()const->std::wstring
{
	std::string strData;
	auto ullSelSize = m_pSelection->GetSelectionSize();
	strData.reserve(static_cast<size_t>(ullSelSize));

	for (auto i = 0; i < ullSelSize; ++i)
		strData.push_back(GetData<BYTE>(m_pSelection->GetOffsetByIndex(i)));

	auto wstrData = str2wstr(strData, m_iCodePage == -1 ? CODEPAGE_DEFAULT : m_iCodePage);
	ReplaceUnprintable(wstrData, m_iCodePage == -1, false);

	return wstrData;
}

void CHexCtrl::DrawWindow(CDC* pDC, CFont* pFont, CFont* pFontInfo)const
{
	const auto iScrollH = static_cast<int>(m_pScrollH->GetScrollPos());
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

	//Text area title.
	CRect rcAscii(m_iThirdVertLine - iScrollH, m_iFirstHorizLine, m_iFourthVertLine - iScrollH, iSecondHorizLine);
	pDC->DrawTextW(m_wstrTextTitle.data(), static_cast<int>(m_wstrTextTitle.size()), rcAscii, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

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

void CHexCtrl::DrawOffsets(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, int iLines)const
{
	const auto ullStartOffset = ullStartLine * m_dwCapacity;
	const auto iScrollH = static_cast<int>(m_pScrollH->GetScrollPos());

	for (auto iterLines = 0; iterLines < iLines; ++iterLines)
	{
		//Drawing offset with bk color depending on selection range.
		COLORREF clrTextOffset, clrBkOffset;
		if (m_pSelection->HitTestRange({ ullStartOffset + iterLines * m_dwCapacity, m_dwCapacity }))
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
		UllToWchars((ullStartLine + iterLines) * m_dwCapacity, pwszOffset, static_cast<size_t>(m_dwOffsetBytes), m_fOffsetAsHex);
		pDC->SelectObject(pFont);
		pDC->SetTextColor(clrTextOffset);
		pDC->SetBkColor(clrBkOffset);
		ExtTextOutW(pDC->m_hDC, m_iFirstVertLine + m_sizeLetter.cx - iScrollH, m_iStartWorkAreaY + (m_sizeLetter.cy * iterLines),
			NULL, nullptr, pwszOffset, m_dwOffsetDigits, nullptr);
	}
}

void CHexCtrl::DrawHexAscii(CDC* pDC, CFont* pFont, int iLines, std::wstring_view wstrHex, std::wstring_view wstrText)const
{
	std::vector<POLYTEXTW> vecPolyHex, vecPolyAscii;
	std::list<std::wstring> listWstrHex, listWstrAscii;
	const int iScrollH = static_cast<int>(m_pScrollH->GetScrollPos());
	size_t sIndexToPrint { };

	for (auto iterLines = 0; iterLines < iLines; ++iterLines)
	{
		//Hex, Ascii, Bookmarks, Selection, Datainterpret, Cursor wstrings to print.
		std::wstring wstrHexToPrint { }, wstrAsciiToPrint { };
		const auto iHexPosToPrintX = m_iIndentFirstHexChunk - iScrollH;
		const auto iAsciiPosToPrintX = m_iIndentAscii - iScrollH;
		const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeLetter.cy * iterLines; //Hex and Ascii the same.

		//Main loop for printing Hex chunks and Ascii chars.
		for (unsigned iterChunks = 0; iterChunks < m_dwCapacity && sIndexToPrint < wstrText.size(); ++iterChunks, ++sIndexToPrint)
		{
			//Hex chunk to print.
			wstrHexToPrint += wstrHex[sIndexToPrint * 2];
			wstrHexToPrint += wstrHex[sIndexToPrint * 2 + 1];

			//Additional space between grouped Hex chunks.
			if (((iterChunks + 1) % static_cast<DWORD>(m_enShowMode)) == 0 && iterChunks < (m_dwCapacity - 1))
				wstrHexToPrint += L" ";

			//Additional space between capacity halves, only with ASBYTE representation.
			if (m_enShowMode == EHexShowMode::ASBYTE && iterChunks == (m_dwCapacityBlockSize - 1))
				wstrHexToPrint += L"  ";

			//Ascii to print.
			wstrAsciiToPrint += wstrText[sIndexToPrint];
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

void CHexCtrl::DrawBookmarks(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, int iLines, std::wstring_view wstrHex, std::wstring_view wstrText)const
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
	const auto ullStartOffset = ullStartLine * m_dwCapacity;
	size_t sIndexToPrint { };

	for (auto iterLines = 0; iterLines < iLines; ++iterLines)
	{
		std::wstring wstrHexBookmarkToPrint { }, wstrAsciiBookmarkToPrint { }; //Bookmarks to print.
		int iBookmarkHexPosToPrintX { -1 }, iBookmarkAsciiPosToPrintX { };
		bool fBookmark { false };  //Flag to show current Bookmark in current Hex presence.
		const HEXBKMSTRUCT* pBookmarkCurr { };
		const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeLetter.cy * iterLines; //Hex and Ascii the same.

		//Main loop for printing Hex chunks and Ascii chars.
		for (unsigned iterChunks = 0; iterChunks < m_dwCapacity && sIndexToPrint < wstrText.size(); ++iterChunks, ++sIndexToPrint)
		{
			//Bookmarks.
			if (auto pBookmark = m_pBookmarks->HitTest(ullStartOffset + sIndexToPrint); pBookmark != nullptr)
			{
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

					iBookmarkHexPosToPrintX = -1;
				}
				pBookmarkCurr = pBookmark;

				if (iBookmarkHexPosToPrintX == -1) //For just one time exec.
				{
					int iCy;
					HexChunkPoint(sIndexToPrint, iBookmarkHexPosToPrintX, iCy);
					AsciiChunkPoint(sIndexToPrint, iBookmarkAsciiPosToPrintX, iCy);
				}

				if (!wstrHexBookmarkToPrint.empty()) //Only adding spaces if there are chars beforehead.
				{
					if ((iterChunks % static_cast<DWORD>(m_enShowMode)) == 0)
						wstrHexBookmarkToPrint += L" ";

					//Additional space between capacity halves, only with ASBYTE representation.
					if (m_enShowMode == EHexShowMode::ASBYTE && iterChunks == m_dwCapacityBlockSize)
						wstrHexBookmarkToPrint += L"  ";
				}
				wstrHexBookmarkToPrint += wstrHex[sIndexToPrint * 2];
				wstrHexBookmarkToPrint += wstrHex[sIndexToPrint * 2 + 1];
				wstrAsciiBookmarkToPrint += wstrText[sIndexToPrint];
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

				iBookmarkHexPosToPrintX = -1;
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

void CHexCtrl::DrawCustomColors(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, int iLines, std::wstring_view wstrHex, std::wstring_view wstrText)const
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
	const auto ullStartOffset = ullStartLine * m_dwCapacity;
	size_t sIndexToPrint { };

	for (auto iterLines = 0; iterLines < iLines; ++iterLines)
	{
		std::wstring wstrHexColorToPrint { }, wstrAsciiColorToPrint { }; //Colors to print.
		int iColorHexPosToPrintX { -1 }, iColorAsciiPosToPrintX { };
		bool fColor { false };  //Flag to show current Color in current Hex presence.
		std::optional<HEXCOLOR> optColorCurr { }; //Current color.
		const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeLetter.cy * iterLines; //Hex and Ascii the same.

		//Main loop for printing Hex chunks and Ascii chars.
		for (unsigned iterChunks = 0; iterChunks < m_dwCapacity && sIndexToPrint < wstrText.size(); ++iterChunks, ++sIndexToPrint)
		{
			//Colors.
			if (auto pColor = m_pHexVirtColors->GetColor(ullStartOffset + sIndexToPrint); pColor != nullptr)
			{
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

					iColorHexPosToPrintX = -1;
				}
				optColorCurr = *pColor;

				if (iColorHexPosToPrintX == -1) //For just one time exec.
				{
					int iCy;
					HexChunkPoint(sIndexToPrint, iColorHexPosToPrintX, iCy);
					AsciiChunkPoint(sIndexToPrint, iColorAsciiPosToPrintX, iCy);
				}

				if (!wstrHexColorToPrint.empty()) //Only adding spaces if there are chars beforehead.
				{
					if ((iterChunks % static_cast<DWORD>(m_enShowMode)) == 0)
						wstrHexColorToPrint += L" ";

					//Additional space between capacity halves, only with ASBYTE representation.
					if (m_enShowMode == EHexShowMode::ASBYTE && iterChunks == m_dwCapacityBlockSize)
						wstrHexColorToPrint += L"  ";
				}
				wstrHexColorToPrint += wstrHex[sIndexToPrint * 2];
				wstrHexColorToPrint += wstrHex[sIndexToPrint * 2 + 1];
				wstrAsciiColorToPrint += wstrText[sIndexToPrint];
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

				iColorHexPosToPrintX = -1;
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

void CHexCtrl::DrawSelection(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, int iLines, std::wstring_view wstrHex, std::wstring_view wstrText)const
{
	std::vector<POLYTEXTW> vecPolySelHex, vecPolySelAscii;
	std::list<std::wstring>	listWstrSelHex, listWstrSelAscii;
	const auto ullStartOffset = ullStartLine * m_dwCapacity;
	size_t sIndexToPrint { };

	for (auto iterLines = 0; iterLines < iLines; ++iterLines)
	{
		std::wstring wstrHexSelToPrint { }, wstrAsciiSelToPrint { }; //Selected Hex and Ascii strings to print.
		int iSelHexPosToPrintX { -1 }, iSelAsciiPosToPrintX { };
		bool fSelection { false };
		const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeLetter.cy * iterLines; //Hex and Ascii the same.

		//Main loop for printing Hex chunks and Ascii chars.
		for (unsigned iterChunks = 0; iterChunks < m_dwCapacity && sIndexToPrint < wstrText.size(); ++iterChunks, ++sIndexToPrint)
		{
			//Selection.
			if (m_pSelection->HitTest(ullStartOffset + sIndexToPrint))
			{
				if (iSelHexPosToPrintX == -1) //For just one time exec.
				{
					int iCy;
					HexChunkPoint(sIndexToPrint, iSelHexPosToPrintX, iCy);
					AsciiChunkPoint(sIndexToPrint, iSelAsciiPosToPrintX, iCy);
				}

				if (!wstrHexSelToPrint.empty()) //Only adding spaces if there are chars beforehead.
				{
					if ((iterChunks % static_cast<DWORD>(m_enShowMode)) == 0)
						wstrHexSelToPrint += L" ";

					//Additional space between capacity halves, only with ASBYTE representation.
					if (m_enShowMode == EHexShowMode::ASBYTE && iterChunks == m_dwCapacityBlockSize)
						wstrHexSelToPrint += L"  ";
				}
				wstrHexSelToPrint += wstrHex[sIndexToPrint * 2];
				wstrHexSelToPrint += wstrHex[sIndexToPrint * 2 + 1];
				wstrAsciiSelToPrint += wstrText[sIndexToPrint];
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
				iSelHexPosToPrintX = -1;
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

void CHexCtrl::DrawSelHighlight(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, int iLines, std::wstring_view wstrHex, std::wstring_view wstrText)const
{
	if (!m_pSelection->HasSelHighlight())
		return;

	std::vector<POLYTEXTW> vecPolySelHex, vecPolySelAscii;
	std::list<std::wstring>	listWstrSelHex, listWstrSelAscii;
	const auto ullStartOffset = ullStartLine * m_dwCapacity;
	size_t sIndexToPrint { };

	for (auto iterLines = 0; iterLines < iLines; ++iterLines)
	{
		std::wstring wstrHexSelToPrint { }, wstrAsciiSelToPrint { }; //Selected Hex and Ascii strings to print.
		int iSelHexPosToPrintX { -1 }, iSelAsciiPosToPrintX { };
		bool fSelection { false };
		const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeLetter.cy * iterLines; //Hex and Ascii the same.

		//Main loop for printing Hex chunks and Ascii chars.
		for (unsigned iterChunks = 0; iterChunks < m_dwCapacity && sIndexToPrint < wstrText.size(); ++iterChunks, ++sIndexToPrint)
		{
			//Selection highlights.
			if (m_pSelection->HitTestHighlight(ullStartOffset + sIndexToPrint))
			{
				if (iSelHexPosToPrintX == -1) //For just one time exec.
				{
					int iCy;
					HexChunkPoint(sIndexToPrint, iSelHexPosToPrintX, iCy);
					AsciiChunkPoint(sIndexToPrint, iSelAsciiPosToPrintX, iCy);
				}

				if (!wstrHexSelToPrint.empty()) //Only adding spaces if there are chars beforehead.
				{
					if ((iterChunks % static_cast<DWORD>(m_enShowMode)) == 0)
						wstrHexSelToPrint += L" ";

					//Additional space between capacity halves, only with ASBYTE representation.
					if (m_enShowMode == EHexShowMode::ASBYTE && iterChunks == m_dwCapacityBlockSize)
						wstrHexSelToPrint += L"  ";
				}
				wstrHexSelToPrint += wstrHex[sIndexToPrint * 2];
				wstrHexSelToPrint += wstrHex[sIndexToPrint * 2 + 1];
				wstrAsciiSelToPrint += wstrText[sIndexToPrint];
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
				iSelHexPosToPrintX = -1;
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

void CHexCtrl::DrawCursor(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, int iLines, std::wstring_view wstrHex, std::wstring_view wstrText)const
{
	if (!IsDataSet() || !IsMutable())
		return;

	std::vector<POLYTEXTW> vecPolyCursor;
	std::list<std::wstring> listWstrCursor;
	COLORREF clrBkCursor { }; //Cursor color.
	const auto ullStartOffset = ullStartLine * m_dwCapacity;
	size_t sIndexToPrint { };
	bool fCurFound { false };

	for (auto iterLines = 0; iterLines < iLines && !fCurFound; ++iterLines)
	{
		std::wstring wstrHexCursorToPrint { }, wstrAsciiCursorToPrint { };
		int iCursorHexPosToPrintX { }, iCursorAsciiPosToPrintX { }; //Cursor X coords.
		const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeLetter.cy * iterLines; //Hex and Ascii the same.

		//Main loop for printing Hex chunks and Ascii chars.
		for (unsigned iterChunks = 0; iterChunks < m_dwCapacity && sIndexToPrint < wstrText.size(); ++iterChunks, ++sIndexToPrint)
		{
			//Cursor position. 
			if (ullStartOffset + sIndexToPrint == m_ullCaretPos)
			{
				int iCy;
				HexChunkPoint(m_ullCaretPos, iCursorHexPosToPrintX, iCy);
				AsciiChunkPoint(m_ullCaretPos, iCursorAsciiPosToPrintX, iCy);
				if (m_fCursorHigh)
					wstrHexCursorToPrint = wstrHex[sIndexToPrint * 2];
				else
				{
					wstrHexCursorToPrint = wstrHex[sIndexToPrint * 2 + 1];
					iCursorHexPosToPrintX += m_sizeLetter.cx;
				}
				wstrAsciiCursorToPrint = wstrText[sIndexToPrint];

				if (m_pSelection->HitTest(ullStartOffset + sIndexToPrint))
					clrBkCursor = m_stColor.clrBkCursorSelected;
				else
					clrBkCursor = m_stColor.clrBkCursor;

				fCurFound = true;
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

void CHexCtrl::DrawDataInterpret(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, int iLines, std::wstring_view wstrHex, std::wstring_view wstrText)const
{
	std::vector<POLYTEXTW> vecPolyDataInterpretHex, vecPolyDataInterpretAscii;
	std::list<std::wstring> listWstrDataInterpretHex, listWstrDataInterpretAscii;
	const auto ullStartOffset = ullStartLine * m_dwCapacity;
	size_t sIndexToPrint { };

	for (auto iterLines = 0; iterLines < iLines; ++iterLines)
	{
		std::wstring wstrHexDataInterpretToPrint { }, wstrAsciiDataInterpretToPrint { }; //Data Interpreter Hex and Ascii strings to print.
		int iDataInterpretHexPosToPrintX { -1 }, iDataInterpretAsciiPosToPrintX { }; //Data Interpreter X coords.
		const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeLetter.cy * iterLines; //Hex and Ascii the same.

		//Main loop for printing Hex chunks and Ascii chars.
		for (unsigned iterChunks = 0; iterChunks < m_dwCapacity && sIndexToPrint < wstrText.size(); ++iterChunks, ++sIndexToPrint)
		{
			//Data Interpreter.
			if (auto ullSize = m_pDlgDataInterpret->GetSize(); ullSize > 0
				&& ullStartOffset + sIndexToPrint >= m_ullCaretPos
				&& ullStartOffset + sIndexToPrint < m_ullCaretPos + ullSize)
			{
				if (iDataInterpretHexPosToPrintX == -1) //For just one time exec.
				{
					int iCy;
					HexChunkPoint(sIndexToPrint, iDataInterpretHexPosToPrintX, iCy);
					AsciiChunkPoint(sIndexToPrint, iDataInterpretAsciiPosToPrintX, iCy);
				}

				if (!wstrHexDataInterpretToPrint.empty()) //Only adding spaces if there are chars beforehead.
				{
					if ((iterChunks % static_cast<DWORD>(m_enShowMode)) == 0)
						wstrHexDataInterpretToPrint += L" ";

					//Additional space between capacity halves, only with ASBYTE representation.
					if (m_enShowMode == EHexShowMode::ASBYTE && iterChunks == m_dwCapacityBlockSize)
						wstrHexDataInterpretToPrint += L"  ";
				}
				wstrHexDataInterpretToPrint += wstrHex[sIndexToPrint * 2];
				wstrHexDataInterpretToPrint += wstrHex[sIndexToPrint * 2 + 1];
				wstrAsciiDataInterpretToPrint += wstrText[sIndexToPrint];
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

void CHexCtrl::DrawSectorLines(CDC* pDC, ULONGLONG ullStartLine, int iLines)
{
	if (!IsPageVisible())
		return;

	//Struct for sector lines.
	struct SECTORLINES
	{
		POINT ptStart;
		POINT ptEnd;
	};
	std::vector<SECTORLINES> vecSecLines;

	//Loop for printing Hex chunks and Ascii chars line by line.
	for (auto iterLines = 0; iterLines < iLines; ++iterLines)
	{
		//Sectors lines vector to print.
		if ((((ullStartLine + iterLines) * m_dwCapacity) % m_dwPageSize == 0) && iterLines > 0)
		{
			const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeLetter.cy * iterLines; //Hex and Ascii the same.
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

void CHexCtrl::FillWithZeros()
{
	if (!IsDataSet())
		return;

	SMODIFY hms;
	hms.vecSpan = m_pSelection->GetData();
	hms.ullDataSize = 1;
	hms.enModifyMode = EModifyMode::MODIFY_REPEAT;
	std::byte byteZero { 0 };
	hms.pData = &byteZero;
	Modify(hms);
}

auto CHexCtrl::GetBottomLine()const->ULONGLONG
{
	ULONGLONG ullEndLine { };
	if (IsDataSet())
	{
		ullEndLine = (GetTopLine() + m_iHeightWorkArea / m_sizeLetter.cy) - 1;
		//If m_ullDataSize is really small, or we at the scroll end,
		//we adjust ullEndLine to be not bigger than maximum allowed.
		if (ullEndLine >= (m_ullDataSize / m_dwCapacity))
			ullEndLine = (m_ullDataSize % m_dwCapacity) ? m_ullDataSize / m_dwCapacity : m_ullDataSize / m_dwCapacity - 1;
	}

	return ullEndLine;
}

auto CHexCtrl::GetCacheSize()const->DWORD
{
	return m_dwCacheSize;
}

auto CHexCtrl::GetCommand(UCHAR uChar, bool fCtrl, bool fShift, bool fAlt)const->std::optional<EHexCmd>
{
	std::optional<EHexCmd> optRet { };
	if (auto iter = std::find_if(m_vecKeyBind.begin(), m_vecKeyBind.end(), [=](const SKEYBIND& ref)
		{return ref.fCtrl == fCtrl && ref.fShift == fShift && ref.fAlt == fAlt && ref.uKey == uChar; });
		iter != m_vecKeyBind.end())
		optRet = iter->eCmd;

	return optRet;
}

auto CHexCtrl::GetData(HEXSPANSTRUCT hss)const->std::byte*
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

auto CHexCtrl::GetDataMode()const->EHexDataMode
{
	return m_enDataMode;
}

auto CHexCtrl::GetMsgWindow()const->HWND
{
	return m_hwndMsg;
}

auto CHexCtrl::GetTopLine()const->ULONGLONG
{
	return m_pScrollV->GetScrollPos() / m_sizeLetter.cy;
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

bool CHexCtrl::IsCurTextArea()const
{
	return m_fCursorTextArea;
}

bool CHexCtrl::IsPageVisible()const
{
	return m_dwPageSize > 0 && (m_dwPageSize % m_dwCapacity == 0) && m_dwPageSize >= m_dwCapacity;
}

void CHexCtrl::Modify(const SMODIFY& hms)
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

	if (hms.fParentNtfy)
		ParentNotify(HEXCTRL_MSG_SETDATA);

	if (hms.fRedraw)
		RedrawWindow();
}

void CHexCtrl::ModifyDefault(const SMODIFY& hms)
{
	const auto& vecSelRef = hms.vecSpan;
	std::byte* pData = GetData(vecSelRef[0]);
	std::copy_n(hms.pData, static_cast<size_t>(vecSelRef[0].ullSize), pData);
	SetDataVirtual(pData, vecSelRef[0]);
}

void CHexCtrl::ModifyOperation(const SMODIFY& hms)
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

void CHexCtrl::ModifyRepeat(const SMODIFY& hms)
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

void CHexCtrl::MsgWindowNotify(const HEXNOTIFYSTRUCT& hns)const
{
	::SendMessageW(GetMsgWindow(), WM_NOTIFY, GetDlgCtrlID(), reinterpret_cast<LPARAM>(&hns));
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

void CHexCtrl::ParentNotify(const HEXNOTIFYSTRUCT& hns)const
{
	::SendMessageW(GetParent()->GetSafeHwnd(), WM_NOTIFY, GetDlgCtrlID(), reinterpret_cast<LPARAM>(&hns));
}

void CHexCtrl::ParentNotify(UINT uCode)const
{
	HEXNOTIFYSTRUCT hns { { m_hWnd, static_cast<UINT>(GetDlgCtrlID()), uCode } };
	ParentNotify(hns);
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
	auto pCurrDC = GetDC();
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

	auto ullStartLine = GetTopLine();
	ULONGLONG ullEndLine { };
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
		auto iLines = static_cast<int>(ullEndLine - ullStartLine);
		const auto& [wstrHex, wstrText] = BuildDataToDraw(ullStartLine, iLines);

		DrawWindow(pDC, &fontMain, &fontInfo);
		auto clBkOld = m_stColor.clrBkSelected;
		auto clTextOld = m_stColor.clrTextSelected;
		m_stColor.clrBkSelected = m_stColor.clrBk;
		m_stColor.clrTextSelected = m_stColor.clrTextHex;
		DrawOffsets(pDC, &fontMain, ullStartLine, iLines);
		DrawSelection(pDC, &fontMain, ullStartLine, iLines, wstrHex, wstrText);
		m_stColor.clrBkSelected = clBkOld;
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

			auto iLines = static_cast<int>(ullEndLine - ullStartLine);
			const auto& [wstrHex, wstrText] = BuildDataToDraw(ullStartLine, iLines);

			DrawWindow(pDC, &fontMain, &fontInfo); //Draw the window with all layouts.
			DrawOffsets(pDC, &fontMain, ullStartLine, iLines);
			DrawHexAscii(pDC, &fontMain, iLines, wstrHex, wstrText);
			DrawBookmarks(pDC, &fontMain, ullStartLine, iLines, wstrHex, wstrText);
			DrawCustomColors(pDC, &fontMain, ullStartLine, iLines, wstrHex, wstrText);
			DrawSelection(pDC, &fontMain, ullStartLine, iLines, wstrHex, wstrText);
			DrawSelHighlight(pDC, &fontMain, ullStartLine, iLines, wstrHex, wstrText);
			DrawCursor(pDC, &fontMain, ullStartLine, iLines, wstrHex, wstrText);
			DrawDataInterpret(pDC, &fontMain, ullStartLine, iLines, wstrHex, wstrText);
			DrawSectorLines(pDC, ullStartLine, iLines);

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

void CHexCtrl::RecalcAll()
{
	ULONGLONG ullCurLineV = GetTopLine();

	//Current font size related.
	CDC* pDC = GetDC();
	pDC->SelectObject(m_fontMain);
	TEXTMETRICW tm;
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
		static_cast<ULONGLONG>(m_iStartWorkAreaY) + m_iHeightBottomOffArea + m_sizeLetter.cy *
		(m_ullDataSize / m_dwCapacity + (m_ullDataSize % m_dwCapacity == 0 ? 1 : 2)));
	m_pScrollH->SetScrollSizes(m_sizeLetter.cx, rc.Width(), static_cast<ULONGLONG>(m_iFourthVertLine) + 1);
	m_pScrollV->SetScrollPos(ullCurLineV * m_sizeLetter.cy);

	Redraw();
	MsgWindowNotify(HEXCTRL_MSG_VIEWCHANGE);
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

void CHexCtrl::RecalcPrint(CDC* pDC, CFont* pFontMain, CFont* pFontInfo, const CRect& rc)
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

void CHexCtrl::Redo()
{
	if (m_deqRedo.empty())
		return;

	const auto& refRedo = m_deqRedo.back();

	std::vector<HEXSPANSTRUCT> vecSpan { };
	std::transform(refRedo->begin(), refRedo->end(), std::back_inserter(vecSpan),
		[](SUNDO& ref) { return HEXSPANSTRUCT { ref.ullOffset, ref.vecData.size() }; });

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

void CHexCtrl::ScrollOffsetH(ULONGLONG ullOffset)
{
	//Horisontally scroll to given offset.
	if (!m_pScrollH->IsVisible())
		return;

	int iCx, iCy;
	HexChunkPoint(ullOffset, iCx, iCy);
	CRect rcClient;
	GetClientRect(&rcClient);

	ULONGLONG ullNewScrollH { };
	auto ullCurrScrollH = m_pScrollH->GetScrollPos();
	auto iMaxClientX = rcClient.Width() - m_iSizeHexByte;
	if (iCx >= iMaxClientX)
		ullNewScrollH = ullCurrScrollH + (iCx - iMaxClientX);
	else if (iCx < 0)
		ullNewScrollH = ullCurrScrollH + iCx;
	else
		ullNewScrollH = ullCurrScrollH;

	m_pScrollH->SetScrollPos(ullNewScrollH);
}

void CHexCtrl::SelAll()
{
	if (!IsDataSet())
		return;

	SetSelection({ { 0, GetDataSize() } }); //Select all.
}

void CHexCtrl::SelAddDown()
{
	ULONGLONG ullClick { }, ullStart { }, ullSize { 0ULL };
	ULONGLONG ullSelStart = m_pSelection->GetSelectionStart();
	ULONGLONG ullSelSize = m_pSelection->GetSelectionSize();
	ULONGLONG ullNewPos { }; //Future pos of selection start.
	ULONGLONG ullOldPos { }; //Current pos of selection start.

	auto lmbSelection = [&]()
	{
		if (ullSelStart == m_ullLMouseClick)
		{
			ullClick = ullStart = m_ullLMouseClick;
			ullSize = ullSelSize + m_dwCapacity;
			ullNewPos = ullClick + ullSize - 1;
			ullOldPos = ullNewPos - m_dwCapacity;
		}
		else if (ullSelStart < m_ullLMouseClick)
		{
			ullClick = m_ullLMouseClick;
			if (ullSelSize > m_dwCapacity)
			{
				ullStart = ullSelStart + m_dwCapacity;
				ullSize = ullSelSize - m_dwCapacity;
			}
			else
			{
				ullStart = ullClick;
				ullSize = 1;
			}
			ullOldPos = ullSelStart;
			ullNewPos = ullOldPos + m_dwCapacity;
		}
	};

	if (m_fMutable)
	{
		if (m_ullCaretPos == m_ullLMouseClick || m_ullCaretPos == ullSelStart
			|| m_ullCaretPos == m_pSelection->GetSelectionEnd())
			lmbSelection();
		else
		{
			ullClick = ullStart = m_ullCaretPos;
			ullSize = 1;
		}
	}
	else if (m_pSelection->HasSelection())
		lmbSelection();

	if (ullStart + ullSize > m_ullDataSize) //To avoid overflow.
		ullSize = m_ullDataSize - ullStart;

	if (ullSize > 0)
	{
		m_ullCaretPos = ullStart;
		m_ullLMouseClick = ullClick;
		SetSelection({ { ullStart, ullSize } }, false);

		auto [i8i8VOld, i8i8HOld] = TestOffsetVis(ullOldPos);
		auto [i8i8VNew, i8i8HNew] = TestOffsetVis(ullNewPos);
		if (i8i8VNew != 0 && i8i8VOld == 0)
			m_pScrollV->ScrollLineDown();
		Redraw();
		OnCaretPosChange(GetCaretPos());
	}
}

void CHexCtrl::SelAddLeft()
{
	ULONGLONG ullClick { }, ullStart { }, ullSize { 0 };
	ULONGLONG ullSelStart = m_pSelection->GetSelectionStart();
	ULONGLONG ullSelSize = m_pSelection->GetSelectionSize();
	ULONGLONG ullNewPos { }; //Future pos of selection start.
	ULONGLONG ullOldPos { }; //Current pos of selection start.

	auto lmbSelection = [&]()
	{
		if (ullSelStart == m_ullLMouseClick && ullSelSize > 1)
		{
			ullClick = ullStart = m_ullLMouseClick;
			ullSize = ullSelSize - 1;
			ullOldPos = ullStart + ullSize;
			ullNewPos = ullOldPos - 1;
		}
		else
		{
			ullClick = m_ullLMouseClick;
			ullStart = ullSelStart > 0 ? ullSelStart - 1 : 0;
			ullSize = ullSelStart > 0 ? ullSelSize + 1 : ullSelSize;
			ullNewPos = ullStart;
			ullOldPos = ullNewPos + 1;
		}
	};

	if (m_fMutable)
	{
		if (m_ullCaretPos == m_ullLMouseClick || m_ullCaretPos == ullSelStart
			|| m_ullCaretPos == m_pSelection->GetSelectionEnd())
			lmbSelection();
		else
		{
			ullClick = ullStart = m_ullCaretPos;
			ullSize = 1;
		}
	}
	else if (m_pSelection->HasSelection())
		lmbSelection();

	if (ullSize > 0)
	{
		m_ullCaretPos = ullStart;
		m_ullLMouseClick = ullClick;
		SetSelection({ { ullStart, ullSize } }, false);

		auto [i8VOld, i8HOld] = TestOffsetVis(ullOldPos);
		auto [i8VNew, i8HNew] = TestOffsetVis(ullNewPos);
		if (i8VNew != 0 && i8VOld == 0)
			m_pScrollV->ScrollLineUp();
		else if (i8HNew != 0 && !IsCurTextArea()) //Do not horz scroll when in text area.
			ScrollOffsetH(ullNewPos);
		else
			Redraw();
		OnCaretPosChange(GetCaretPos());
	}
}

void CHexCtrl::SelAddRight()
{
	ULONGLONG ullClick { }, ullStart { }, ullSize { 0UL };
	ULONGLONG ullSelStart = m_pSelection->GetSelectionStart();
	ULONGLONG ullSelSize = m_pSelection->GetSelectionSize();
	ULONGLONG ullNewPos { }; //Future pos of selection start.
	ULONGLONG ullOldPos { }; //Current pos of selection start.

	auto lmbSelection = [&]()
	{
		if (ullSelStart == m_ullLMouseClick)
		{
			ullClick = ullStart = m_ullLMouseClick;
			ullSize = ullSelSize + 1;

			ullNewPos = ullClick + ullSize - 1;
			ullOldPos = ullNewPos - 1;
		}
		else
		{
			ullClick = m_ullLMouseClick;
			ullStart = ullSelStart + 1;
			ullSize = ullSelSize - 1;

			ullOldPos = ullSelStart;
			ullNewPos = ullOldPos + 1;
		}
	};

	if (m_fMutable)
	{
		if (m_ullCaretPos == m_ullLMouseClick || m_ullCaretPos == ullSelStart
			|| m_ullCaretPos == m_pSelection->GetSelectionEnd())
			lmbSelection();
		else
		{
			ullClick = ullStart = m_ullCaretPos;
			ullSize = 1;
		}
	}
	else if (m_pSelection->HasSelection())
		lmbSelection();

	if (ullStart + ullSize > m_ullDataSize) //To avoid overflow.
		ullSize = m_ullDataSize - ullStart;

	if (ullSize > 0)
	{
		m_ullCaretPos = ullStart;
		m_ullLMouseClick = ullClick;
		SetSelection({ { ullStart, ullSize } }, false);

		auto [i8VOld, i8HOld] = TestOffsetVis(ullOldPos);
		auto [i8VNew, i8HNew] = TestOffsetVis(ullNewPos);
		if (i8VNew != 0 && i8VOld == 0)
			m_pScrollV->ScrollLineDown();
		else if (i8HNew != 0 && !IsCurTextArea()) //Do not horz scroll when in text area.
			ScrollOffsetH(ullNewPos);
		else
			Redraw();
		OnCaretPosChange(GetCaretPos());
	}
}

void CHexCtrl::SelAddUp()
{
	ULONGLONG ullClick { }, ullStart { 0 }, ullSize { 0 };
	ULONGLONG ullSelStart = m_pSelection->GetSelectionStart();
	ULONGLONG ullSelSize = m_pSelection->GetSelectionSize();
	ULONGLONG ullNewPos { }; //Future pos of selection start.
	ULONGLONG ullOldPos { }; //Current pos of selection start.

	auto lmbSelection = [&]()
	{
		if (ullSelStart < m_ullLMouseClick)
		{
			ullClick = m_ullLMouseClick;
			ullOldPos = ullSelStart;

			if (ullSelStart < m_dwCapacity)
			{
				ullSize = ullSelSize + ullSelStart;
				ullNewPos = ullOldPos;
			}
			else
			{
				ullStart = ullSelStart - m_dwCapacity;
				ullSize = ullSelSize + m_dwCapacity;
				ullNewPos = ullStart;
			}
		}
		else
		{
			ullClick = m_ullLMouseClick;
			if (ullSelSize > m_dwCapacity)
			{
				ullStart = ullClick;
				ullSize = ullSelSize - m_dwCapacity;
			}
			else if (ullSelSize > 1)
			{
				ullStart = ullClick;
				ullSize = 1;
			}
			else
			{
				ullStart = ullClick >= m_dwCapacity ? ullClick - m_dwCapacity : 0;
				ullSize = ullClick >= m_dwCapacity ? ullSelSize + m_dwCapacity : ullSelSize + ullSelStart;
			}

			ullOldPos = ullSelStart + ullSelSize - 1;
			ullNewPos = ullOldPos - m_dwCapacity;
		}
	};

	if (m_fMutable)
	{
		if (m_ullCaretPos == m_ullLMouseClick || m_ullCaretPos == ullSelStart
			|| m_ullCaretPos == m_pSelection->GetSelectionEnd())
			lmbSelection();
		else
		{
			ullClick = ullStart = m_ullCaretPos;
			ullSize = 1;
		}
	}
	else if (m_pSelection->HasSelection())
		lmbSelection();

	if (ullSize > 0)
	{
		m_ullCaretPos = ullStart;
		m_ullLMouseClick = ullClick;
		SetSelection({ { ullStart, ullSize } }, false);

		auto [i8VOld, i8HOld] = TestOffsetVis(ullOldPos);
		auto [i8VNew, i8HNew] = TestOffsetVis(ullNewPos);
		if (i8VNew != 0 && i8VOld == 0)
			m_pScrollV->ScrollLineUp();
		Redraw();
		OnCaretPosChange(GetCaretPos());
	}
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

void CHexCtrl::SetSelHighlight(const std::vector<HEXSPANSTRUCT>& vecSelHighlight)
{
	m_pSelection->SetSelHighlight(vecSelHighlight);
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
	const auto& refUndo = m_deqUndo.emplace_back(std::make_unique<std::vector<SUNDO>>());

	//Bad alloc may happen here!!!
	try
	{
		for (const auto& iterSel : vecSpan)
		{
			refUndo->emplace_back(SUNDO { iterSel.ullOffset, { } });
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

auto CHexCtrl::TestOffsetVis(ULONGLONG ullOffset)const->std::tuple<std::int8_t, std::int8_t>
{
	//Returns two std::int8_t for vertical and horizontal visibility for given offset.
	//-1 - ullOffset is higher or at the left of visible area
	// 1 - lower or at the right
	// 0 - visible.
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet())
		return { };

	auto dwCapacity = GetCapacity();
	auto ullFirst = GetTopLine() * dwCapacity;;
	auto ullLast = GetBottomLine() * dwCapacity + dwCapacity;

	CRect rcClient;
	GetClientRect(&rcClient);
	int iCx, iCy;
	HexChunkPoint(ullOffset, iCx, iCy);
	int iMaxClientX = rcClient.Width() - m_iSizeHexByte;

	return { static_cast<std::int8_t>(ullOffset < ullFirst ? -1 : (ullOffset >= ullLast ? 1 : 0)),
		static_cast<std::int8_t>(iCx < 0 ? -1 : (iCx >= iMaxClientX ? 1 : 0)) };
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
		m_wndTtBkm.SendMessageW(TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&m_stToolInfoBkm));
		KillTimer(ID_TOOLTIP_BKM);
	}
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

void CHexCtrl::Undo()
{
	if (m_deqUndo.empty())
		return;

	const auto& refUndo = m_deqUndo.back();

	//Making new Redo data snapshot.
	const auto& refRedo = m_deqRedo.emplace_back(std::make_unique<std::vector<SUNDO>>());

	//Bad alloc may happen here!!!
	//If there is no more free memory, just clear the vec and return.
	try
	{
		for (auto& i : *refUndo)
		{
			refRedo->emplace_back(SUNDO { i.ullOffset, { } });
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


/********************************************************************
* MFC message handlers.
********************************************************************/
void CHexCtrl::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	SetFocus();

	CWnd::OnActivate(nState, pWndOther, bMinimized);
}

void CHexCtrl::OnChar(UINT nChar, UINT /*nRepCnt*/, UINT /*nFlags*/)
{
	if (!m_fMutable || !IsCurTextArea() || (GetKeyState(VK_CONTROL) < 0))
		return;

	SMODIFY hms;
	hms.vecSpan.emplace_back(HEXSPANSTRUCT { m_ullCaretPos, 1 });
	hms.ullDataSize = 1;

	BYTE chByte = nChar & 0xFF;
	WCHAR warrCurrLocaleID[KL_NAMELENGTH];
	GetKeyboardLayoutNameW(warrCurrLocaleID); //Current langID as wstring.
	LCID dwCurrLocaleID { };
	if (wstr2num(warrCurrLocaleID, dwCurrLocaleID, 16)) //Convert langID from wstr to number.
	{
		UINT uCurrCodePage { };
		int iSize = sizeof(uCurrCodePage) / sizeof(WCHAR);
		if (GetLocaleInfoW(dwCurrLocaleID, LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
			reinterpret_cast<LPWSTR>(&uCurrCodePage), iSize) == iSize) //ANSI code page for the current langID (if any).
		{
			wchar_t wch { static_cast<wchar_t>(nChar) };
			//Convert input symbol (wchar) to char according to current Windows' code page.
			if (auto str = wstr2str(&wch, uCurrCodePage); !str.empty())
				chByte = str[0];
		}
	}

	hms.pData = reinterpret_cast<std::byte*>(&chByte);
	Modify(hms);
	CaretMoveRight();
}

BOOL CHexCtrl::OnCommand(WPARAM wParam, LPARAM lParam)
{
	const auto wMenuID = LOWORD(wParam);
	if (auto iter = std::find_if(m_vecKeyBind.begin(), m_vecKeyBind.end(), [=](const SKEYBIND& ref)
		{return ref.wMenuID == wMenuID;	}); iter != m_vecKeyBind.end())
		ExecuteCmd(iter->eCmd);
	else
	{	//For user defined custom menu we notifying parent window.
		HEXNOTIFYSTRUCT hns { { m_hWnd, static_cast<UINT>(GetDlgCtrlID()), HEXCTRL_MSG_MENUCLICK } };
		hns.ullData = wMenuID;
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

void CHexCtrl::OnDestroy()
{
	//All these cleanups are important in case of HexCtrl being destroyed as a child window.
	//When a child window is destroyed, the HexCtrl object itself is stil alive,
	//if IHexCtrl::Destroy() method was not called.

	ClearData();
	m_wndTtBkm.DestroyWindow();
	m_menuMain.DestroyMenu();
	m_fontMain.DeleteObject();
	m_fontInfo.DeleteObject();
	m_penLines.DeleteObject();
	m_umapHBITMAP.clear();
	m_vecKeyBind.clear();
	m_pDlgBookmarkMgr->DestroyWindow();
	m_pDlgDataInterpret->DestroyWindow();
	m_pDlgFillData->DestroyWindow();
	m_pDlgEncoding->DestroyWindow();
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

BOOL CHexCtrl::OnEraseBkgnd(CDC* /*pDC*/)
{
	return FALSE;
}

UINT CHexCtrl::OnGetDlgCode()
{
	return DLGC_WANTALLKEYS;
}

BOOL CHexCtrl::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
	return FALSE;
}

void CHexCtrl::OnHScroll(UINT /*nSBCode*/, UINT /*nPos*/, CScrollBar* /*pScrollBar*/)
{
	if (m_pScrollH->GetScrollPosDelta() != 0)
		RedrawWindow();
}

void CHexCtrl::OnInitMenuPopup(CMenu* /*pPopupMenu*/, UINT nIndex, BOOL /*bSysMenu*/)
{
	switch (nIndex) //Zero based index of the menu. Zero means main menu itself, 1 - first (sub)menu, and so on.
	{
	case 0:	//Main menu.
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_SEARCH, IsCmdAvail(EHexCmd::CMD_DLG_SEARCH) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_DATAINTERPRET, IsCmdAvail(EHexCmd::CMD_DLG_DATAINTERPRET) ? MF_ENABLED : MF_GRAYED);
		break;
	case 3:	//Navigation.
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_NAV_GOTO, IsCmdAvail(EHexCmd::CMD_NAV_DLG_GOTO) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_NAV_DATABEG, IsCmdAvail(EHexCmd::CMD_NAV_DATABEG) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_NAV_DATAEND, IsCmdAvail(EHexCmd::CMD_NAV_DATAEND) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_NAV_PAGEBEG, IsCmdAvail(EHexCmd::CMD_NAV_PAGEBEG) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_NAV_PAGEEND, IsCmdAvail(EHexCmd::CMD_NAV_PAGEEND) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_NAV_LINEBEG, IsCmdAvail(EHexCmd::CMD_NAV_LINEBEG) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_NAV_LINEEND, IsCmdAvail(EHexCmd::CMD_NAV_LINEEND) ? MF_ENABLED : MF_GRAYED);
		break;
	case 4:	//Bookmarks
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_BOOKMARKS_ADD, IsCmdAvail(EHexCmd::CMD_BKM_ADD) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_BOOKMARKS_REMOVE, IsCmdAvail(EHexCmd::CMD_BKM_REMOVE) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_BOOKMARKS_NEXT, IsCmdAvail(EHexCmd::CMD_BKM_NEXT) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_BOOKMARKS_PREV, IsCmdAvail(EHexCmd::CMD_BKM_PREV) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_BOOKMARKS_CLEARALL, IsCmdAvail(EHexCmd::CMD_BKM_CLEARALL) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_BOOKMARKS_MANAGER, IsCmdAvail(EHexCmd::CMD_BKM_DLG_MANAGER) ? MF_ENABLED : MF_GRAYED);
		break;
	case 5:	//Clipboard
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLIPBOARD_COPYHEX, IsCmdAvail(EHexCmd::CMD_CLIPBOARD_COPY_HEX) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLIPBOARD_COPYHEXLE, IsCmdAvail(EHexCmd::CMD_CLIPBOARD_COPY_HEXLE) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLIPBOARD_COPYHEXFMT, IsCmdAvail(EHexCmd::CMD_CLIPBOARD_COPY_HEXFMT) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLIPBOARD_COPYTEXT, IsCmdAvail(EHexCmd::CMD_CLIPBOARD_COPY_TEXT) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLIPBOARD_COPYBASE64, IsCmdAvail(EHexCmd::CMD_CLIPBOARD_COPY_BASE64) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLIPBOARD_COPYCARRAY, IsCmdAvail(EHexCmd::CMD_CLIPBOARD_COPY_CARR) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLIPBOARD_COPYGREPHEX, IsCmdAvail(EHexCmd::CMD_CLIPBOARD_COPY_GREPHEX) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLIPBOARD_COPYPRNTSCRN, IsCmdAvail(EHexCmd::CMD_CLIPBOARD_COPY_PRNTSCRN) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLIPBOARD_PASTEHEX, IsCmdAvail(EHexCmd::CMD_CLIPBOARD_PASTE_HEX) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLIPBOARD_PASTETEXT, IsCmdAvail(EHexCmd::CMD_CLIPBOARD_PASTE_TEXT) ? MF_ENABLED : MF_GRAYED);
		break;
	case 6: //Modify
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_MODIFY_FILLZEROS, IsCmdAvail(EHexCmd::CMD_MODIFY_FILLZEROS) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_MODIFY_FILLDATA, IsCmdAvail(EHexCmd::CMD_MODIFY_DLG_FILLDATA) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_MODIFY_OPERS, IsCmdAvail(EHexCmd::CMD_MODIFY_DLG_OPERS) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_MODIFY_UNDO, IsCmdAvail(EHexCmd::CMD_MODIFY_UNDO) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_MODIFY_REDO, IsCmdAvail(EHexCmd::CMD_MODIFY_REDO) ? MF_ENABLED : MF_GRAYED);
		break;
	case 7: //Selection
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_SEL_MARKSTART, IsCmdAvail(EHexCmd::CMD_SEL_MARKSTART) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_SEL_MARKEND, IsCmdAvail(EHexCmd::CMD_SEL_MARKEND) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_SEL_ALL, IsCmdAvail(EHexCmd::CMD_SEL_ALL) ? MF_ENABLED : MF_GRAYED);
		break;
	}
}

void CHexCtrl::OnKeyDown(UINT nChar, UINT /*nRepCnt*/, UINT nFlags)
{
	//Bit 14 indicates that the key was pressed continuously.
	//0x4000 == 0100 0000 0000 0000.
	if (nFlags & 0x4000)
		m_fKeyDownAtm = true;

	if (auto opt = GetCommand(static_cast<BYTE>(nChar),
		GetAsyncKeyState(VK_CONTROL) < 0, GetAsyncKeyState(VK_SHIFT) < 0, GetAsyncKeyState(VK_MENU) < 0); opt)
		ExecuteCmd(opt.value());
	else if (m_fMutable)
	{
		SMODIFY hms;
		hms.vecSpan.emplace_back(HEXSPANSTRUCT { m_ullCaretPos, 1 });
		hms.ullDataSize = 1;

		BYTE chByte = nChar & 0xFF;
		if (!IsCurTextArea()) //If cursor is not in Ascii area then just one part (High/Low) of byte must be changed.
		{
			if (chByte >= 0x30 && chByte <= 0x39)      //Digits.
				chByte -= 0x30;
			else if (chByte >= 0x41 && chByte <= 0x46) //Hex letters uppercase.
				chByte -= 0x37;
			else if (chByte >= 0x61 && chByte <= 0x66) //Hex letters lowercase.
				chByte -= 0x57;
			else
				return;

			auto chByteCurr = GetData<BYTE>(m_ullCaretPos);
			if (m_fCursorHigh)
				chByte = (chByte << 4) | (chByteCurr & 0x0F);
			else
				chByte = (chByte & 0x0F) | (chByteCurr & 0xF0);

			hms.pData = reinterpret_cast<std::byte*>(&chByte);
			Modify(hms);
			CaretMoveRight();
		}
	}

	m_optRMouseClick.reset(); //Reset right mouse click.
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

void CHexCtrl::OnLButtonDblClk(UINT /*nFlags*/, CPoint point)
{
	if (point.x < m_iSecondVertLine)
	{
		m_fOffsetAsHex = !m_fOffsetAsHex;
		WstrCapacityFill();
		RecalcAll();
	}
}

void CHexCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();

	const auto optHit = HitTest(point);
	if (!optHit)
		return;

	m_ullCurCursor = optHit->ullOffset;
	m_fCursorTextArea = optHit->fIsAscii;
	m_fSelectionBlock = GetAsyncKeyState(VK_MENU) < 0;
	SetCapture();

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
	m_ullCaretPos = ullSelStart;
	std::vector<HEXSPANSTRUCT> vecSel { { ullSelStart, ullSelSize } };
	SetSelection(vecSel);
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

		m_ullLMouseClick = ullClick;
		m_ullCaretPos = ullStart;
		std::vector<HEXSPANSTRUCT> vecSel;
		vecSel.reserve(ullLines);
		for (auto iterLines = 0ULL; iterLines < ullLines; ++iterLines)
			vecSel.emplace_back(HEXSPANSTRUCT { ullStart + m_dwCapacity * iterLines, ullSize });
		SetSelection(vecSel);
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
		else if (m_pBkmCurrTt) //If there is already bkm tooltip shown, but cursor is outside of data chunks.
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

	const auto ullStartLine = GetTopLine();
	const auto ullEndLine = GetBottomLine();
	auto iLines = static_cast<int>(ullEndLine - ullStartLine);
	assert(iLines >= 0);
	if (iLines < 0)
		return;

	//Actual amount of lines, "ullEndLine - ullStartLine" always shows one line less.
	if (IsDataSet())
		++iLines;

	//Drawing through CMemDC to avoid flickering.
	CMemDC memDC(dc, rcClient);
	auto pDC = &memDC.GetDC();
	DrawWindow(pDC, &m_fontMain, &m_fontInfo); //Draw the window with all layouts.
	DrawOffsets(pDC, &m_fontMain, ullStartLine, iLines);
	const auto& [wstrHex, wstrText] = BuildDataToDraw(ullStartLine, iLines);
	DrawHexAscii(pDC, &m_fontMain, iLines, wstrHex, wstrText);
	DrawBookmarks(pDC, &m_fontMain, ullStartLine, iLines, wstrHex, wstrText);
	DrawCustomColors(pDC, &m_fontMain, ullStartLine, iLines, wstrHex, wstrText);
	DrawSelection(pDC, &m_fontMain, ullStartLine, iLines, wstrHex, wstrText);
	DrawSelHighlight(pDC, &m_fontMain, ullStartLine, iLines, wstrHex, wstrText);
	DrawCursor(pDC, &m_fontMain, ullStartLine, iLines, wstrHex, wstrText);
	DrawDataInterpret(pDC, &m_fontMain, ullStartLine, iLines, wstrHex, wstrText);
	DrawSectorLines(pDC, ullStartLine, iLines);
}

void CHexCtrl::OnRButtonDown(UINT /*nFlags*/, CPoint point)
{
	const auto optHit = HitTest(point);
	m_optRMouseClick = optHit ? optHit->ullOffset : std::optional<ULONGLONG> { };
}

BOOL CHexCtrl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	m_pScrollV->OnSetCursor(pWnd, nHitTest, message);
	m_pScrollH->OnSetCursor(pWnd, nHitTest, message);

	return CWnd::OnSetCursor(pWnd, nHitTest, message);
}

void CHexCtrl::OnSize(UINT /*nType*/, int cx, int cy)
{
	if (!IsCreated())
		return;

	RecalcWorkArea(cy, cx);
	auto ullPageSize = static_cast<ULONGLONG>(m_iHeightWorkArea * m_dbWheelRatio);
	if (ullPageSize < m_sizeLetter.cy)
		ullPageSize = m_sizeLetter.cy;
	m_pScrollV->SetScrollPageSize(ullPageSize);
	MsgWindowNotify(HEXCTRL_MSG_VIEWCHANGE);
}

void CHexCtrl::OnSysKeyDown(UINT nChar, UINT /*nRepCnt*/, UINT /*nFlags*/)
{
	if (auto opt = GetCommand(static_cast<BYTE>(nChar),
		GetAsyncKeyState(VK_CONTROL) < 0, GetAsyncKeyState(VK_SHIFT) < 0, true); opt)
		ExecuteCmd(opt.value());
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