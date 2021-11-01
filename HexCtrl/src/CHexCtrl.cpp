/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "../dep/rapidjson/rapidjson-amalgam.h"
#include "../res/HexCtrlRes.h"
#include "CHexBookmarks.h"
#include "CHexCtrl.h"
#include "CHexSelection.h"
#include "CScrollEx.h"
#include "Dialogs/CHexDlgAbout.h"
#include "Dialogs/CHexDlgBkmMgr.h"
#include "Dialogs/CHexDlgCallback.h"
#include "Dialogs/CHexDlgDataInterp.h"
#include "Dialogs/CHexDlgEncoding.h"
#include "Dialogs/CHexDlgFillData.h"
#include "Dialogs/CHexDlgGoTo.h"
#include "Dialogs/CHexDlgOpers.h"
#include "Dialogs/CHexDlgSearch.h"
#include "HexUtility.h"
#include "strsafe.h"
#include <bit>
#include <cassert>
#include <cctype>
#include <format>
#include <fstream>
#include <numeric>
#include <random>
#include <thread>
#pragma comment(lib, "Dwmapi.lib")

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

namespace HEXCTRL
{
	extern "C" HEXCTRLAPI IHexCtrl * __cdecl CreateRawHexCtrl()
	{
		return new CHexCtrl();
	};

	extern "C" HEXCTRLAPI HEXCTRLINFO * __cdecl GetHexCtrlInfo()
	{
		static HEXCTRLINFO stVersion { HEXCTRL_VERSION_WSTR,
		{ static_cast<ULONGLONG>((static_cast<ULONGLONG>(HEXCTRL_VERSION_MAJOR) << 48)
		| (static_cast<ULONGLONG>(HEXCTRL_VERSION_MINOR) << 32)
		| (static_cast<ULONGLONG>(HEXCTRL_VERSION_MAINTENANCE) << 16)) }
		};

		return &stVersion;
	};

	/********************************************
	* Internal enums and structs.               *
	********************************************/
	namespace INTERNAL
	{
		enum class CHexCtrl::EClipboard : std::uint8_t
		{
			COPY_HEX, COPY_HEXLE, COPY_HEXFMT, COPY_TEXT, COPY_BASE64,
			COPY_CARR, COPY_GREPHEX, COPY_PRNTSCRN, COPY_OFFSET, PASTE_HEX, PASTE_TEXT
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

		constexpr auto HEXCTRL_CLASSNAME_WSTR { L"HexCtrl" }; //HexControl Class name.
		constexpr auto ID_TOOLTIP_BKM { 0x01ULL };            //Tooltip ID for bookmarks.
		constexpr auto ID_TOOLTIP_OFFSET { 0x02ULL };         //Tooltip ID for offset.
	}
}

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
	//MFC initialization, if HexCtrl is used in non MFC project with a "Shared MFC" linking.
#if defined HEXCTRL_MANUAL_MFC_INIT
	if (!AfxGetModuleState()->m_lpszCurrentAppName)
		AfxWinInit(::GetModuleHandleW(nullptr), nullptr, ::GetCommandLineW(), 0);
#endif

	const auto hInst = AfxGetInstanceHandle();
	WNDCLASSEXW wc { };
	if (!::GetClassInfoExW(hInst, HEXCTRL_CLASSNAME_WSTR, &wc))
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
		wc.lpszClassName = HEXCTRL_CLASSNAME_WSTR;
		if (!RegisterClassExW(&wc)) {
			MessageBoxW(L"HexControl RegisterClassExW error.", L"Error", MB_ICONERROR);
			return;
		}
	}
}

ULONGLONG CHexCtrl::BkmAdd(const HEXBKM& hbs, bool fRedraw)
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

auto CHexCtrl::BkmGetByID(ULONGLONG ullID)->HEXBKM*
{
	assert(IsCreated());
	if (!IsCreated())
		return nullptr;

	return m_pBookmarks->GetByID(ullID);
}

auto CHexCtrl::BkmGetByIndex(ULONGLONG ullIndex)->HEXBKM*
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

auto CHexCtrl::BkmHitTest(ULONGLONG ullOffset)->HEXBKM*
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

	m_spnData = { };
	m_fDataSet = false;
	m_fMutable = false;
	m_pHexVirtData = nullptr;
	m_pHexVirtColors = nullptr;
	m_fHighLatency = false;
	m_ullCursorPrev = 0;
	m_optRMouseClick.reset();
	m_ullCaretPos = 0;
	m_ullCursorNow = 0;

	m_deqUndo.clear();
	m_deqRedo.clear();
	m_pScrollV->SetScrollPos(0);
	m_pScrollH->SetScrollPos(0);
	m_pScrollV->SetScrollSizes(0, 0, 0);
	m_pBookmarks->ClearAll();
	m_pSelection->ClearAll();
	Redraw();
}

bool CHexCtrl::Create(const HEXCREATE& hcs)
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

	const auto hInst = AfxGetInstanceHandle();
	mii.hbmpItem = m_umapHBITMAP[IDM_HEXCTRL_CLPBRD_COPYHEX] =
		static_cast<HBITMAP>(LoadImageW(hInst, MAKEINTRESOURCEW(IDB_HEXCTRL_MENU_COPY), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
	m_menuMain.SetMenuItemInfoW(IDM_HEXCTRL_CLPBRD_COPYHEX, &mii);
	mii.hbmpItem = m_umapHBITMAP[IDM_HEXCTRL_CLPBRD_PASTEHEX] =
		static_cast<HBITMAP>(LoadImageW(hInst, MAKEINTRESOURCEW(IDB_HEXCTRL_MENU_PASTE), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
	m_menuMain.SetMenuItemInfoW(IDM_HEXCTRL_CLPBRD_PASTEHEX, &mii);
	mii.hbmpItem = m_umapHBITMAP[IDM_HEXCTRL_MODIFY_FILLZEROS] =
		static_cast<HBITMAP>(LoadImageW(hInst, MAKEINTRESOURCEW(IDB_HEXCTRL_MENU_FILLZEROS), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
	m_menuMain.SetMenuItemInfoW(IDM_HEXCTRL_MODIFY_FILLZEROS, &mii);

	m_stColor = hcs.stColor;
	m_dbWheelRatio = hcs.dbWheelRatio;

	//1. WS_POPUP style is vital for GetParent to work properly in EHexCreateMode::CREATE_POPUP mode.
	//   Without this style GetParent/GetOwner always return 0, no matter whether pParentWnd is provided to CreateWindowEx or not.
	//2. Created HexCtrl window will always overlap (be on top of) its parent, or owner, window 
	//   if pParentWnd is set (is not nullptr) in CreateWindowEx.
	//3. To force HexCtrl window on taskbar the WS_EX_APPWINDOW extended window style must be set.
	std::wstring wstrError;
	const auto lmbCreateWnd = [&](UINT uID, DWORD dwStyle)
	{
		if (!CWnd::CreateEx(hcs.dwExStyle, HEXCTRL_CLASSNAME_WSTR, L"HexControl", dwStyle, hcs.rect,
			CWnd::FromHandle(hcs.hwndParent), uID))
			wstrError = std::format(L"HexCtrl (ID: {}) CreateEx failed.\r\nCheck HEXCREATE parameters.", uID);
	};
	switch (hcs.enCreateMode)
	{
	case EHexCreateMode::CREATE_POPUP:
		lmbCreateWnd(0, hcs.dwStyle | WS_VISIBLE | WS_POPUP | WS_OVERLAPPEDWINDOW);
		break;
	case EHexCreateMode::CREATE_CHILD:
		lmbCreateWnd(hcs.uID, hcs.dwStyle | WS_VISIBLE | WS_CHILD);
		break;
	case EHexCreateMode::CREATE_CUSTOMCTRL:
		//If it's a Custom Control in dialog, there is no need to create a window, just subclassing.
		if (!SubclassDlgItem(hcs.uID, CWnd::FromHandle(hcs.hwndParent)))
			wstrError = std::format(L"HexCtrl (ID: {}) SubclassDlgItem failed.\r\nCheck CreateDialogCtrl parameters.", hcs.uID);
		break;
	}
	if (!wstrError.empty()) //If there was any creation error.
	{
		MessageBoxW(wstrError.data(), L"Error", MB_ICONERROR);
		return false;
	}

	m_wndTtBkm.CreateEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr, TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, nullptr);
	m_stToolInfoBkm.cbSize = TTTOOLINFOW_V1_SIZE;
	m_stToolInfoBkm.uFlags = TTF_TRACK;
	m_stToolInfoBkm.uId = ID_TOOLTIP_BKM;
	m_wndTtBkm.SendMessageW(TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&m_stToolInfoBkm));
	m_wndTtBkm.SendMessageW(TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(400)); //to allow use of newline \n.

	m_wndTtOffset.CreateEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr, TTS_NOANIMATE | TTS_NOFADE | TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, nullptr);
	m_stToolInfoOffset.cbSize = TTTOOLINFOW_V1_SIZE;
	m_stToolInfoOffset.uFlags = TTF_TRACK;
	m_stToolInfoOffset.uId = ID_TOOLTIP_OFFSET;
	m_wndTtOffset.SendMessageW(TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&m_stToolInfoOffset));
	m_wndTtOffset.SendMessageW(TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(400)); //to allow use of newline \n.

	//Font related.//////////////////////////////////////////////
	auto pDC = GetDC();
	const auto iLOGPIXELSY = GetDeviceCaps(pDC->m_hDC, LOGPIXELSY);
	ReleaseDC(pDC);

	LOGFONTW lfMain { };
	if (hcs.pLogFont == nullptr) //Creating default main font.
	{
		StringCchCopyW(lfMain.lfFaceName, LF_FACESIZE, L"Consolas");
		lfMain.lfHeight = -MulDiv(11, iLOGPIXELSY, 72);
		lfMain.lfPitchAndFamily = FIXED_PITCH;
	}
	else
		lfMain = *hcs.pLogFont;
	m_fontMain.CreateFontIndirectW(&lfMain);

	LOGFONTW lfInfo { }; //Info area font, independent from the main font.
	StringCchCopyW(lfInfo.lfFaceName, LF_FACESIZE, L"Consolas");
	lfInfo.lfHeight = -MulDiv(11, iLOGPIXELSY, 72) + 1; //Size is less by 1 than the default main font.
	lfInfo.lfPitchAndFamily = FIXED_PITCH;
	m_fontInfo.CreateFontIndirectW(&lfInfo);
	//End of font related.///////////////////////////////////////

	m_penLines.CreatePen(PS_SOLID, 1, RGB(200, 200, 200));

	//Removing window's border frame.
	MARGINS marg { 0, 0, 0, 1 };
	DwmExtendFrameIntoClientArea(m_hWnd, &marg);
	SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

	//ScrollBars should be created here, after the main window has already been created (to attach to), to avoid assertions.
	m_pScrollV->Create(this, true, IDB_HEXCTRL_SCROLL_V, 0, 0, 0); //Actual sizes are set in RecalcAll().
	m_pScrollH->Create(this, false, IDB_HEXCTRL_SCROLL_H, 0, 0, 0);
	m_pScrollV->AddSibling(m_pScrollH.get());
	m_pScrollH->AddSibling(m_pScrollV.get());

	//All dialogs are created after the main window, to set the parent window correctly.
	m_pDlgBkmMgr->Create(IDD_HEXCTRL_BKMMGR, this, &*m_pBookmarks);
	m_pDlgEncoding->Create(IDD_HEXCTRL_ENCODING, this, this);
	m_pDlgDataInterp->Create(IDD_HEXCTRL_DATAINTERP, this, this);
	m_pDlgFillData->Create(IDD_HEXCTRL_FILLDATA, this, this);
	m_pDlgOpers->Create(IDD_HEXCTRL_OPERATIONS, this, this);
	m_pDlgSearch->Create(IDD_HEXCTRL_SEARCH, this, this);
	m_pDlgGoTo->Create(IDD_HEXCTRL_GOTO, this, this);
	m_pBookmarks->Attach(this);
	m_fCreated = true;

	SetGroupMode(m_enGroupMode);
	SetEncoding(-1);
	SetConfig(L"");
	SetDateInfo(0xFFFFFFFF);

	return true;
}

bool CHexCtrl::CreateDialogCtrl(UINT uCtrlID, HWND hParent)
{
	assert(!IsCreated());
	if (IsCreated())
		return false;

	HEXCREATE hcs;
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
		m_pDlgSearch->SearchNextPrev(true);
		break;
	case EHexCmd::CMD_SEARCH_PREV:
		m_pDlgSearch->SearchNextPrev(false);
		break;
	case EHexCmd::CMD_NAV_DLG_GOTO:
		m_pDlgGoTo->ShowWindow(SW_SHOW);
		break;
	case EHexCmd::CMD_NAV_REPFWD:
		m_pDlgGoTo->Repeat();
		break;
	case EHexCmd::CMD_NAV_REPBKW:
		m_pDlgGoTo->Repeat(false);
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
	case EHexCmd::CMD_GROUPBY_BYTE:
		SetGroupMode(EHexDataSize::SIZE_BYTE);
		break;
	case EHexCmd::CMD_GROUPBY_WORD:
		SetGroupMode(EHexDataSize::SIZE_WORD);
		break;
	case EHexCmd::CMD_GROUPBY_DWORD:
		SetGroupMode(EHexDataSize::SIZE_DWORD);
		break;
	case EHexCmd::CMD_GROUPBY_QWORD:
		SetGroupMode(EHexDataSize::SIZE_QWORD);
		break;
	case EHexCmd::CMD_BKM_ADD:
		m_pBookmarks->Add(HEXBKM { HasSelection() ? GetSelection()
			: std::vector<HEXSPAN> { { GetCaretPos(), 1 } } });
		break;
	case EHexCmd::CMD_BKM_REMOVE:
		m_pBookmarks->Remove(m_fMenuCMD ? m_optRMouseClick.value() : GetCaretPos());
		m_optRMouseClick.reset();
		m_fMenuCMD = false;
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
		m_pDlgBkmMgr->ShowWindow(SW_SHOW);
		break;
	case EHexCmd::CMD_CLPBRD_COPYHEX:
		ClipboardCopy(EClipboard::COPY_HEX);
		break;
	case EHexCmd::CMD_CLPBRD_COPYHEXLE:
		ClipboardCopy(EClipboard::COPY_HEXLE);
		break;
	case EHexCmd::CMD_CLPBRD_COPYHEXFMT:
		ClipboardCopy(EClipboard::COPY_HEXFMT);
		break;
	case EHexCmd::CMD_CLPBRD_COPYTEXT:
		ClipboardCopy(EClipboard::COPY_TEXT);
		break;
	case EHexCmd::CMD_CLPBRD_COPYBASE64:
		ClipboardCopy(EClipboard::COPY_BASE64);
		break;
	case EHexCmd::CMD_CLPBRD_COPYCARR:
		ClipboardCopy(EClipboard::COPY_CARR);
		break;
	case EHexCmd::CMD_CLPBRD_COPYGREPHEX:
		ClipboardCopy(EClipboard::COPY_GREPHEX);
		break;
	case EHexCmd::CMD_CLPBRD_COPYPRNTSCRN:
		ClipboardCopy(EClipboard::COPY_PRNTSCRN);
		break;
	case EHexCmd::CMD_CLPBRD_COPYOFFSET:
		ClipboardCopy(EClipboard::COPY_OFFSET);
		break;
	case EHexCmd::CMD_CLPBRD_PASTEHEX:
		ClipboardPaste(EClipboard::PASTE_HEX);
		break;
	case EHexCmd::CMD_CLPBRD_PASTETEXT:
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
		m_pSelection->SetSelStartEnd(GetCaretPos(), true);
		Redraw();
		break;
	case EHexCmd::CMD_SEL_MARKEND:
		m_pSelection->SetSelStartEnd(GetCaretPos(), false);
		Redraw();
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
	case EHexCmd::CMD_DLG_DATAINTERP:
		m_pDlgDataInterp->ShowWindow(SW_SHOW);
		break;
	case EHexCmd::CMD_DLG_ENCODING:
		m_pDlgEncoding->ShowWindow(SW_SHOW);
		break;
	case EHexCmd::CMD_APPEAR_FONTCHOOSE:
		ChooseFontDlg();
		break;
	case EHexCmd::CMD_APPEAR_FONTINC:
		FontSizeIncDec(true);
		break;
	case EHexCmd::CMD_APPEAR_FONTDEC:
		FontSizeIncDec(false);
		break;
	case EHexCmd::CMD_APPEAR_CAPACINC:
		SetCapacity(GetCapacity() + 1);
		break;
	case EHexCmd::CMD_APPEAR_CAPACDEC:
		SetCapacity(GetCapacity() - 1);
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

auto CHexCtrl::GetCacheSize()const->DWORD
{
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet())
		return { };

	return m_dwCacheSize;
}

DWORD CHexCtrl::GetCapacity()const
{
	assert(IsCreated());

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

auto CHexCtrl::GetColors()const->HEXCOLORS
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	return m_stColor;
}

auto CHexCtrl::GetData(HEXSPAN hss)const->std::span<std::byte>
{
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet())
		return { };

	std::span<std::byte> spnData;
	if (!IsVirtual())
	{
		if (hss.ullOffset + hss.ullSize <= GetDataSize())
			spnData = { m_spnData.data() + hss.ullOffset, static_cast<std::size_t>(hss.ullSize) };
	}
	else
	{
		if (hss.ullSize == 0 || hss.ullSize > GetCacheSize())
			hss.ullSize = GetCacheSize();

		HEXDATAINFO hdi { { m_hWnd, static_cast<UINT>(GetDlgCtrlID()) } };
		hdi.stHexSpan = hss;
		m_pHexVirtData->OnHexGetData(hdi);
		spnData = hdi.spnData;
	}

	return spnData;
}

auto CHexCtrl::GetDataSize()const->ULONGLONG
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	return m_spnData.size();
}

DWORD CHexCtrl::GetDateInfo()const
{
	assert(IsCreated());
	if (!IsCreated())
		return 0xFFFFFFFF;

	return m_dwDateFormat;
}

int CHexCtrl::GetEncoding()const
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	return m_iCodePage;
}

void CHexCtrl::GetFont(LOGFONTW& lf)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_fontMain.GetLogFont(&lf);
}

auto CHexCtrl::GetGroupMode()const->EHexDataSize
{
	assert(IsCreated());
	if (!IsCreated())
		return EHexDataSize { };

	return m_enGroupMode;
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

	const auto ullSize = GetDataSize();

	return (ullSize % m_dwPageSize) ? ullSize / m_dwPageSize + 1 : ullSize / m_dwPageSize;
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

auto CHexCtrl::GetSelection()const->std::vector<HEXSPAN>
{
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet())
		return { };

	return m_pSelection->GetData();
}

wchar_t CHexCtrl::GetUnprintableChar()const
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	return m_wchUnprintable;
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
		hWnd = m_pDlgBkmMgr->m_hWnd;
		break;
	case EHexWnd::DLG_DATAINTERP:
		hWnd = m_pDlgDataInterp->m_hWnd;
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
	case EHexWnd::DLG_GOTO:
		hWnd = m_pDlgGoTo->m_hWnd;
		break;
	}

	return hWnd;
}

void CHexCtrl::GoToOffset(ULONGLONG ullOffset, int iRelPos)
{
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet() || ullOffset >= GetDataSize())
		return;

	const auto ullNewStartV = ullOffset / m_dwCapacity * m_sizeLetter.cy;
	auto ullNewScrollV { 0ULL };

	switch (iRelPos)
	{
	case -1: //Offset will be at the top line.
		ullNewScrollV = ullNewStartV;
		break;
	case 0: //Offset at the middle.
		if (ullNewStartV > m_iHeightWorkArea / 2) //To prevent negative numbers.
		{
			ullNewScrollV = ullNewStartV - m_iHeightWorkArea / 2;
			ullNewScrollV -= ullNewScrollV % m_sizeLetter.cy;
		}
		break;
	case 1: //Offset at the bottom.
		ullNewScrollV = ullNewStartV - (GetBottomLine() - GetTopLine()) * m_sizeLetter.cy;
		break;
	default:
		break;
	}
	m_pScrollV->SetScrollPos(ullNewScrollV);

	if (m_pScrollH->IsVisible() && !IsCurTextArea())
	{
		ULONGLONG ullNewScrollH = (ullOffset % m_dwCapacity) * m_iSizeHexByte;
		ullNewScrollH += (ullNewScrollH / m_iDistanceBetweenHexChunks) * m_iSpaceBetweenHexChunks;
		m_pScrollH->SetScrollPos(ullNewScrollH);
	}
}

bool CHexCtrl::HasSelection()const
{
	assert(IsCreated());
	if (!IsCreated())
		return false;

	return m_pSelection->HasSelection();
}

auto CHexCtrl::HitTest(POINT pt, bool fScreen)const->std::optional<HEXHITTEST>
{
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet())
		return std::nullopt;

	if (fScreen)
		ScreenToClient(&pt);

	return HitTest(pt);
}

bool CHexCtrl::IsCmdAvail(EHexCmd eCmd)const
{
	assert(IsCreated());
	if (!IsCreated())
		return false;

	const auto fDataSet = IsDataSet();
	const auto fMutable = fDataSet ? IsMutable() : false;
	const auto fSelection = fDataSet && HasSelection();
	bool fAvail;

	switch (eCmd)
	{
	case EHexCmd::CMD_BKM_REMOVE:
		fAvail = m_pBookmarks->HitTest(m_fMenuCMD ? *m_optRMouseClick : GetCaretPos()) != nullptr;
		break;
	case EHexCmd::CMD_BKM_NEXT:
	case EHexCmd::CMD_BKM_PREV:
	case EHexCmd::CMD_BKM_CLEARALL:
		fAvail = m_pBookmarks->HasBookmarks();
		break;
	case EHexCmd::CMD_CLPBRD_COPYHEX:
	case EHexCmd::CMD_CLPBRD_COPYHEXLE:
	case EHexCmd::CMD_CLPBRD_COPYHEXFMT:
	case EHexCmd::CMD_CLPBRD_COPYTEXT:
	case EHexCmd::CMD_CLPBRD_COPYBASE64:
	case EHexCmd::CMD_CLPBRD_COPYCARR:
	case EHexCmd::CMD_CLPBRD_COPYGREPHEX:
	case EHexCmd::CMD_CLPBRD_COPYPRNTSCRN:
		fAvail = fSelection;
		break;
	case EHexCmd::CMD_CLPBRD_PASTEHEX:
	case EHexCmd::CMD_CLPBRD_PASTETEXT:
		fAvail = fMutable && IsClipboardFormatAvailable(CF_TEXT);
		break;
	case EHexCmd::CMD_MODIFY_DLG_OPERS:
	case EHexCmd::CMD_MODIFY_DLG_FILLDATA:
		fAvail = fMutable;
		break;
	case EHexCmd::CMD_MODIFY_FILLZEROS:
		fAvail = fMutable && fSelection;
		break;
	case EHexCmd::CMD_MODIFY_UNDO:
		fAvail = !m_deqUndo.empty();
		break;
	case EHexCmd::CMD_MODIFY_REDO:
		fAvail = !m_deqRedo.empty();
		break;
	case EHexCmd::CMD_BKM_ADD:
	case EHexCmd::CMD_CARET_RIGHT:
	case EHexCmd::CMD_CARET_LEFT:
	case EHexCmd::CMD_CARET_DOWN:
	case EHexCmd::CMD_CARET_UP:
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
	case EHexCmd::CMD_DLG_DATAINTERP:
	case EHexCmd::CMD_CLPBRD_COPYOFFSET:
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
	case EHexCmd::CMD_NAV_REPFWD:
	case EHexCmd::CMD_NAV_REPBKW:
		fAvail = fDataSet && m_pDlgGoTo->IsRepeatAvail();
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

auto CHexCtrl::IsOffsetVisible(ULONGLONG ullOffset)const->HEXVISION
{
	//Returns HEXVISION with two std::int8_t for vertical and horizontal visibility respectively.
	//-1 - ullOffset is higher, or at the left, of the visible area
	// 1 - lower, or at the right
	// 0 - visible.
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet())
		return { -1, -1 }; //To ensure false return.

	const auto dwCapacity = GetCapacity();
	const auto ullFirst = GetTopLine() * dwCapacity;
	const auto ullLast = GetBottomLine() * dwCapacity + dwCapacity;

	CRect rcClient;
	GetClientRect(&rcClient);
	int iCx, iCy;
	HexChunkPoint(ullOffset, iCx, iCy);
	const auto iMaxClientX = rcClient.Width() - m_iSizeHexByte;

	return { static_cast<std::int8_t>(ullOffset < ullFirst ? -1 : (ullOffset >= ullLast ? 1 : 0)),
		static_cast<std::int8_t>(iCx < 0 ? -1 : (iCx >= iMaxClientX ? 1 : 0)) };
}

bool CHexCtrl::IsVirtual()const
{
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet())
		return false;

	return m_pHexVirtData != nullptr;
}

void CHexCtrl::ModifyData(const HEXMODIFY& hms)
{
	assert(!hms.vecSpan.empty());
	if (!IsMutable() || hms.vecSpan.empty())
		return;

	m_deqRedo.clear(); //No Redo unless we make Undo.
	SnapshotUndo(hms.vecSpan);

	SetRedraw(false);
	using enum EHexModifyMode;
	switch (hms.enModifyMode)
	{
	case MODIFY_ONCE:
	{
		const auto stHexSpan = hms.vecSpan.back();
		const auto ullOffsetToModify = stHexSpan.ullOffset;
		const auto ullSizeToModify = (std::min)(stHexSpan.ullSize, static_cast<ULONGLONG>(hms.spnData.size()));

		assert((ullOffsetToModify + ullSizeToModify) <= GetDataSize());
		if ((ullOffsetToModify + ullSizeToModify) > GetDataSize())
			return;

		if (IsVirtual() && ullSizeToModify > GetCacheSize())
		{
			const auto ullSizeCache = GetCacheSize();
			const auto iMod = ullSizeToModify % ullSizeCache;
			auto ullChunks = ullSizeToModify / ullSizeCache + (iMod > 0 ? 1 : 0);
			auto ullOffsetCurr = ullOffsetToModify;
			auto ullOffsetSpanCurr = 0ULL;
			while (ullChunks-- > 0)
			{
				const auto ullSizeToModifyCurr = (ullChunks == 1 && iMod > 0) ? iMod : ullSizeCache;
				const auto spnData = GetData({ ullOffsetCurr, ullSizeToModifyCurr });
				assert(!spnData.empty());
				std::copy_n(hms.spnData.data() + ullOffsetSpanCurr, ullSizeToModifyCurr, spnData.data());
				SetDataVirtual(spnData, { ullOffsetCurr, ullSizeToModifyCurr });
				ullOffsetCurr += ullSizeToModifyCurr;
				ullOffsetSpanCurr += ullSizeToModifyCurr;
			}
		}
		else
		{
			const auto spnData = GetData({ ullOffsetToModify, ullSizeToModify });
			assert(!spnData.empty());
			std::copy_n(hms.spnData.data(), static_cast<size_t>(ullSizeToModify), spnData.data());
			SetDataVirtual(spnData, { ullOffsetToModify, ullSizeToModify });
		}
	}
	break;
	case MODIFY_RAND_MT19937:
	case MODIFY_RAND_FAST:
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		const std::uniform_int_distribution<std::uint64_t> distUInt64(0, (std::numeric_limits<std::uint64_t>::max)());
		const auto lmbRandUInt64 = [&](std::byte* pData, const HEXMODIFY& /**/, std::span<std::byte> /**/)
		{
			assert(pData != nullptr);
			*reinterpret_cast<std::uint64_t*>(pData) = distUInt64(gen);
		};
		const auto lmbRandByte = [&](std::byte* pData, const HEXMODIFY& /**/, std::span<std::byte> /**/)
		{
			assert(pData != nullptr);
			*pData = static_cast<std::byte>(distUInt64(gen));
		};
		const auto lmbRandFast = [](std::byte* pData, const HEXMODIFY& /**/, std::span<std::byte> spnDataFrom)
		{
			assert(pData != nullptr);
			std::copy_n(spnDataFrom.data(), spnDataFrom.size(), pData);
		};

		const auto& refHexSpan = hms.vecSpan.back();
		if (hms.enModifyMode == MODIFY_RAND_MT19937 && hms.vecSpan.size() == 1 && refHexSpan.ullSize >= sizeof(std::uint64_t))
		{
			ModifyWorker(hms, lmbRandUInt64, { static_cast<std::byte*>(nullptr), sizeof(std::uint64_t) });

			if (const auto iRem = refHexSpan.ullSize % sizeof(std::uint64_t); iRem > 0) //Remainder.
			{
				const auto ullOffset = refHexSpan.ullOffset + refHexSpan.ullSize - iRem;
				const auto spnData = GetData({ ullOffset, iRem });
				for (std::size_t iterRem = 0; iterRem < iRem; ++iterRem)
				{
					spnData.data()[iterRem] = static_cast<std::byte>(distUInt64(gen));
				}
				SetDataVirtual(spnData, { ullOffset, iRem });
			}
		}
		else if (hms.enModifyMode == MODIFY_RAND_FAST && hms.vecSpan.size() == 1 && refHexSpan.ullSize >= GetCacheSize())
		{
			//Fill the uptrRandData buffer with true random data of ulSizeRandBuff size.
			//Then clone this buffer to the destination data.
			//Buffer is allocated with alignment for maximum performance.
			constexpr auto ulSizeRandBuff = 1024U * 1024U; //1MB.
			std::unique_ptr < std::byte [], decltype([](std::byte* pData) { _aligned_free(pData); }) >
				uptrRandData(static_cast<std::byte*>(_aligned_malloc(ulSizeRandBuff, 32)));

			for (auto iter = 0UL; iter < ulSizeRandBuff; iter += sizeof(std::uint64_t))
			{
				*reinterpret_cast<std::uint64_t*>(&uptrRandData[iter]) = distUInt64(gen);
			};

			ModifyWorker(hms, lmbRandFast, { uptrRandData.get(), ulSizeRandBuff });

			//Filling the remainder data.
			if (const auto iRem = refHexSpan.ullSize % ulSizeRandBuff; iRem > 0) //Remainder.
			{
				if (iRem <= GetCacheSize())
				{
					const auto ullOffsetCurr = refHexSpan.ullOffset + refHexSpan.ullSize - iRem;
					const auto spnData = GetData({ ullOffsetCurr, iRem });
					assert(!spnData.empty());
					std::copy_n(uptrRandData.get(), iRem, spnData.data());
					SetDataVirtual(spnData, { ullOffsetCurr, iRem });
				}
				else
				{
					const auto ullSizeCache = GetCacheSize();
					const auto iMod = iRem % ullSizeCache;
					auto ullChunks = iRem / ullSizeCache + (iMod > 0 ? 1 : 0);
					auto ullOffsetCurr = refHexSpan.ullOffset + refHexSpan.ullSize - iRem;
					auto ullOffsetRandCurr = 0ULL;
					while (ullChunks-- > 0)
					{
						const auto ullSizeToModify = (ullChunks == 1 && iMod > 0) ? iMod : ullSizeCache;
						const auto spnData = GetData({ ullOffsetCurr, ullSizeToModify });
						assert(!spnData.empty());
						std::copy_n(uptrRandData.get() + ullOffsetRandCurr, ullSizeToModify, spnData.data());
						SetDataVirtual(spnData, { ullOffsetCurr, ullSizeToModify });
						ullOffsetCurr += ullSizeToModify;
						ullOffsetRandCurr += ullSizeToModify;
					}
				}
			}
		}
		else {
			ModifyWorker(hms, lmbRandByte, { static_cast<std::byte*>(nullptr), sizeof(std::byte) });
		}
	}
	break;
	case MODIFY_REPEAT:
	{
		constexpr auto lmbRepeat = [](std::byte* pData, const HEXMODIFY& /**/, std::span<std::byte> spnDataFrom)
		{
			assert(pData != nullptr);
			std::copy_n(spnDataFrom.data(), spnDataFrom.size(), pData);
		};

		//In cases where only one affected data region (hms.vecSpan.size()==1) is used,
		//and the size of the repeated data is equal to the extent of 2,
		//we extend that repeated data to iBuffSizeForFastFill size to speed up 
		//the whole process of repeating.
		//At the end we simply fill up the remainder (ullSizeToModify % ulSizeBuffFastFill ).
		const auto ullOffsetToModify = hms.vecSpan.back().ullOffset;
		const auto ullSizeToModify = hms.vecSpan.back().ullSize;
		const auto ullSizeToFillWith = hms.spnData.size();
		constexpr auto ulSizeBuffFastFill { 256U };

		if (hms.vecSpan.size() == 1 && ullSizeToModify > ulSizeBuffFastFill
			&& ullSizeToFillWith < ulSizeBuffFastFill && (ulSizeBuffFastFill % ullSizeToFillWith) == 0)
		{
			alignas(32) std::byte buffFillData[ulSizeBuffFastFill]; //Buffer for fast data fill.
			for (auto iter = 0ULL; iter < ulSizeBuffFastFill; iter += ullSizeToFillWith)
			{
				std::copy_n(hms.spnData.data(), ullSizeToFillWith, buffFillData + iter);
			}

			ModifyWorker(hms, lmbRepeat, { buffFillData, ulSizeBuffFastFill });

			if (const auto iRem = ullSizeToModify % ulSizeBuffFastFill; iRem >= ullSizeToFillWith) //Remainder.
			{
				const auto ullOffset = ullOffsetToModify + ullSizeToModify - iRem;
				const auto spnData = GetData({ ullOffset, iRem });
				for (std::size_t iterRem = 0; iterRem < (iRem / ullSizeToFillWith); ++iterRem) //Works only if iRem >= ullSizeToFillWith.
				{
					std::copy_n(hms.spnData.data(), ullSizeToFillWith, spnData.data() + (iterRem * ullSizeToFillWith));
				}
				SetDataVirtual(spnData, { ullOffset, iRem - (iRem % ullSizeToFillWith) });
			}
		}
		else {
			ModifyWorker(hms, lmbRepeat, hms.spnData);
		}
	}
	break;
	case MODIFY_OPERATION:
	{
		constexpr auto lmbOperData = [](std::byte* pData, const HEXMODIFY& hms, std::span<std::byte>/**/)
		{
			assert(pData != nullptr);

			constexpr auto lmbSwap = []<typename T>(T * pData)
			{
				if constexpr (sizeof(T) == sizeof(WORD))
					*pData = _byteswap_ushort(*pData);
				else if constexpr (sizeof(T) == sizeof(DWORD))
					*pData = _byteswap_ulong(*pData);
				else if constexpr (sizeof(T) == sizeof(QWORD))
					*pData = _byteswap_uint64(*pData);
			};

			constexpr auto lmbOper = []<typename T>(T * pData, const EHexOperMode eMode, const T tDataOper, const auto lmbSwap)
			{
				switch (eMode)
				{
				case EHexOperMode::OPER_ASSIGN:
					*pData = tDataOper;
					break;
				case EHexOperMode::OPER_OR:
					*pData |= tDataOper;
					break;
				case EHexOperMode::OPER_XOR:
					*pData ^= tDataOper;
					break;
				case EHexOperMode::OPER_AND:
					*pData &= tDataOper;
					break;
				case EHexOperMode::OPER_NOT:
					*pData = ~*pData;
					break;
				case EHexOperMode::OPER_SHL:
					*pData <<= tDataOper;
					break;
				case EHexOperMode::OPER_SHR:
					*pData >>= tDataOper;
					break;
				case EHexOperMode::OPER_ROTL:
					*pData = std::rotl(*pData, static_cast<int>(tDataOper));
					break;
				case EHexOperMode::OPER_ROTR:
					*pData = std::rotr(*pData, static_cast<int>(tDataOper));
					break;
				case EHexOperMode::OPER_SWAP:
					lmbSwap(pData);
					break;
				case EHexOperMode::OPER_ADD:
					*pData += tDataOper;
					break;
				case EHexOperMode::OPER_SUBTRACT:
					*pData -= tDataOper;
					break;
				case EHexOperMode::OPER_MULTIPLY:
					*pData *= tDataOper;
					break;
				case EHexOperMode::OPER_DIVIDE:
					if (tDataOper > 0) //Division by zero check.
						*pData /= tDataOper;
					break;
				case EHexOperMode::OPER_CEILING:
					if (*pData > tDataOper)
						*pData = tDataOper;
					break;
				case EHexOperMode::OPER_FLOOR:
					if (*pData < tDataOper)
						*pData = tDataOper;
					break;
				}
			};

			if (hms.fBigEndian)
			{
				switch (hms.enOperSize)
				{
				case EHexDataSize::SIZE_BYTE:
					lmbOper(reinterpret_cast<PBYTE>(pData), hms.enOperMode, *reinterpret_cast<PBYTE>(hms.spnData.data()), lmbSwap);
					break;
				case EHexDataSize::SIZE_WORD:
					lmbSwap(reinterpret_cast<PWORD>(pData));
					lmbOper(reinterpret_cast<PWORD>(pData), hms.enOperMode, *reinterpret_cast<PWORD>(hms.spnData.data()), lmbSwap);
					lmbSwap(reinterpret_cast<PWORD>(pData));
					break;
				case EHexDataSize::SIZE_DWORD:
					lmbSwap(reinterpret_cast<PDWORD>(pData));
					lmbOper(reinterpret_cast<PDWORD>(pData), hms.enOperMode, *reinterpret_cast<PDWORD>(hms.spnData.data()), lmbSwap);
					lmbSwap(reinterpret_cast<PDWORD>(pData));
					break;
				case EHexDataSize::SIZE_QWORD:
					lmbSwap(reinterpret_cast<PQWORD>(pData));
					lmbOper(reinterpret_cast<PQWORD>(pData), hms.enOperMode, *reinterpret_cast<PQWORD>(hms.spnData.data()), lmbSwap);
					lmbSwap(reinterpret_cast<PQWORD>(pData));
					break;
				}
			}
			else
			{
				switch (hms.enOperSize)
				{
				case EHexDataSize::SIZE_BYTE:
					lmbOper(reinterpret_cast<PBYTE>(pData), hms.enOperMode, *reinterpret_cast<PBYTE>(hms.spnData.data()), lmbSwap);
					break;
				case EHexDataSize::SIZE_WORD:
					lmbOper(reinterpret_cast<PWORD>(pData), hms.enOperMode, *reinterpret_cast<PWORD>(hms.spnData.data()), lmbSwap);
					break;
				case EHexDataSize::SIZE_DWORD:
					lmbOper(reinterpret_cast<PDWORD>(pData), hms.enOperMode, *reinterpret_cast<PDWORD>(hms.spnData.data()), lmbSwap);
					break;
				case EHexDataSize::SIZE_QWORD:
					lmbOper(reinterpret_cast<PQWORD>(pData), hms.enOperMode, *reinterpret_cast<PQWORD>(hms.spnData.data()), lmbSwap);
					break;
				}
			}
		};
		ModifyWorker(hms, lmbOperData, { static_cast<std::byte*>(nullptr), static_cast<std::size_t>(hms.enOperSize) });
	}
	break;
	default:
		break;
	}
	SetRedraw(true);

	ParentNotify(HEXCTRL_MSG_SETDATA);
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

		if (HasSelection())
		{
			swprintf_s(wBuff, std::size(wBuff),
				IsOffsetAsHex() ? L"Selected: 0x%llX [0x%llX-0x%llX]; " : L"Selected: %llu [%llu-%llu]; ",
				m_pSelection->GetSelSize(), m_pSelection->GetSelStart(), m_pSelection->GetSelEnd());
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

	constexpr DWORD dwCapacityMax { 99 }; //Maximum capacity allowed.

	if (dwCapacity < 1 || dwCapacity > dwCapacityMax)
		return;

	//Setting capacity according to current m_enGroupMode.
	if (dwCapacity < m_dwCapacity)
		dwCapacity -= dwCapacity % static_cast<DWORD>(m_enGroupMode);
	else if (dwCapacity % static_cast<DWORD>(m_enGroupMode))
		dwCapacity += static_cast<DWORD>(m_enGroupMode) - (dwCapacity % static_cast<DWORD>(m_enGroupMode));

	//To prevent under/over flow.
	if (dwCapacity < static_cast<DWORD>(m_enGroupMode))
		dwCapacity = static_cast<DWORD>(m_enGroupMode);
	else if (dwCapacity > dwCapacityMax)
		dwCapacity = dwCapacityMax;

	m_dwCapacity = dwCapacity;
	m_dwCapacityBlockSize = m_dwCapacity / 2;

	WstrCapacityFill();
	RecalcAll();
	ParentNotify(HEXCTRL_MSG_VIEWCHANGE);
}

void CHexCtrl::SetCaretPos(ULONGLONG ullOffset, bool fHighLow, bool fRedraw)
{
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet() || ullOffset >= GetDataSize())
		return;

	m_ullCaretPos = ullOffset;
	m_fCaretHigh = fHighLow;

	if (fRedraw)
		Redraw();

	OnCaretPosChange(ullOffset);
}

void CHexCtrl::SetColors(const HEXCOLORS& clr)
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
		{ "CMD_DLG_SEARCH", { EHexCmd::CMD_DLG_SEARCH, static_cast<WORD>(IDM_HEXCTRL_DLG_SEARCH) } },
		{ "CMD_SEARCH_NEXT", { EHexCmd::CMD_SEARCH_NEXT, static_cast<WORD>(IDM_HEXCTRL_SEARCH_NEXT) } },
		{ "CMD_SEARCH_PREV", { EHexCmd::CMD_SEARCH_PREV, static_cast<WORD>(IDM_HEXCTRL_SEARCH_PREV) } },
		{ "CMD_NAV_DLG_GOTO", { EHexCmd::CMD_NAV_DLG_GOTO, static_cast<WORD>(IDM_HEXCTRL_NAV_DLG_GOTO) } },
		{ "CMD_NAV_REPFWD", { EHexCmd::CMD_NAV_REPFWD, static_cast<WORD>(IDM_HEXCTRL_NAV_REPFWD) } },
		{ "CMD_NAV_REPBKW", { EHexCmd::CMD_NAV_REPBKW, static_cast<WORD>(IDM_HEXCTRL_NAV_REPBKW) } },
		{ "CMD_NAV_DATABEG", { EHexCmd::CMD_NAV_DATABEG, static_cast<WORD>(IDM_HEXCTRL_NAV_DATABEG) } },
		{ "CMD_NAV_DATAEND", { EHexCmd::CMD_NAV_DATAEND, static_cast<WORD>(IDM_HEXCTRL_NAV_DATAEND) } },
		{ "CMD_NAV_PAGEBEG", { EHexCmd::CMD_NAV_PAGEBEG, static_cast<WORD>(IDM_HEXCTRL_NAV_PAGEBEG) } },
		{ "CMD_NAV_PAGEEND", { EHexCmd::CMD_NAV_PAGEEND, static_cast<WORD>(IDM_HEXCTRL_NAV_PAGEEND) } },
		{ "CMD_NAV_LINEBEG", { EHexCmd::CMD_NAV_LINEBEG, static_cast<WORD>(IDM_HEXCTRL_NAV_LINEBEG) } },
		{ "CMD_NAV_LINEEND", { EHexCmd::CMD_NAV_LINEEND, static_cast<WORD>(IDM_HEXCTRL_NAV_LINEEND) } },
		{ "CMD_GROUPBY_BYTE", { EHexCmd::CMD_GROUPBY_BYTE, static_cast<WORD>(IDM_HEXCTRL_GROUPBY_BYTE) } },
		{ "CMD_GROUPBY_WORD", { EHexCmd::CMD_GROUPBY_WORD, static_cast<WORD>(IDM_HEXCTRL_GROUPBY_WORD) } },
		{ "CMD_GROUPBY_DWORD", { EHexCmd::CMD_GROUPBY_DWORD, static_cast<WORD>(IDM_HEXCTRL_GROUPBY_DWORD) } },
		{ "CMD_GROUPBY_QWORD", { EHexCmd::CMD_GROUPBY_QWORD, static_cast<WORD>(IDM_HEXCTRL_GROUPBY_QWORD) } },
		{ "CMD_BKM_ADD", { EHexCmd::CMD_BKM_ADD, static_cast<WORD>(IDM_HEXCTRL_BKM_ADD) } },
		{ "CMD_BKM_REMOVE", { EHexCmd::CMD_BKM_REMOVE, static_cast<WORD>(IDM_HEXCTRL_BKM_REMOVE) } },
		{ "CMD_BKM_NEXT", { EHexCmd::CMD_BKM_NEXT, static_cast<WORD>(IDM_HEXCTRL_BKM_NEXT) } },
		{ "CMD_BKM_PREV", { EHexCmd::CMD_BKM_PREV, static_cast<WORD>(IDM_HEXCTRL_BKM_PREV) } },
		{ "CMD_BKM_CLEARALL", { EHexCmd::CMD_BKM_CLEARALL, static_cast<WORD>(IDM_HEXCTRL_BKM_CLEARALL) } },
		{ "CMD_BKM_DLG_MANAGER", { EHexCmd::CMD_BKM_DLG_MANAGER, static_cast<WORD>(IDM_HEXCTRL_BKM_DLG_MANAGER) } },
		{ "CMD_CLPBRD_COPYHEX", { EHexCmd::CMD_CLPBRD_COPYHEX, static_cast<WORD>(IDM_HEXCTRL_CLPBRD_COPYHEX) } },
		{ "CMD_CLPBRD_COPYHEXLE", { EHexCmd::CMD_CLPBRD_COPYHEXLE, static_cast<WORD>(IDM_HEXCTRL_CLPBRD_COPYHEXLE) } },
		{ "CMD_CLPBRD_COPYHEXFMT", { EHexCmd::CMD_CLPBRD_COPYHEXFMT, static_cast<WORD>(IDM_HEXCTRL_CLPBRD_COPYHEXFMT) } },
		{ "CMD_CLPBRD_COPYTEXT", { EHexCmd::CMD_CLPBRD_COPYTEXT, static_cast<WORD>(IDM_HEXCTRL_CLPBRD_COPYTEXT) } },
		{ "CMD_CLPBRD_COPYBASE64", { EHexCmd::CMD_CLPBRD_COPYBASE64, static_cast<WORD>(IDM_HEXCTRL_CLPBRD_COPYBASE64) } },
		{ "CMD_CLPBRD_COPYCARR", { EHexCmd::CMD_CLPBRD_COPYCARR, static_cast<WORD>(IDM_HEXCTRL_CLPBRD_COPYCARR) } },
		{ "CMD_CLPBRD_COPYGREPHEX", { EHexCmd::CMD_CLPBRD_COPYGREPHEX, static_cast<WORD>(IDM_HEXCTRL_CLPBRD_COPYGREPHEX) } },
		{ "CMD_CLPBRD_COPYPRNTSCRN", { EHexCmd::CMD_CLPBRD_COPYPRNTSCRN, static_cast<WORD>(IDM_HEXCTRL_CLPBRD_COPYPRNTSCRN) } },
		{ "CMD_CLPBRD_COPYOFFSET", { EHexCmd::CMD_CLPBRD_COPYOFFSET, static_cast<WORD>(IDM_HEXCTRL_CLPBRD_COPYOFFSET) } },
		{ "CMD_CLPBRD_PASTEHEX", { EHexCmd::CMD_CLPBRD_PASTEHEX, static_cast<WORD>(IDM_HEXCTRL_CLPBRD_PASTEHEX) } },
		{ "CMD_CLPBRD_PASTETEXT", { EHexCmd::CMD_CLPBRD_PASTETEXT, static_cast<WORD>(IDM_HEXCTRL_CLPBRD_PASTETEXT) } },
		{ "CMD_MODIFY_DLG_OPERS", { EHexCmd::CMD_MODIFY_DLG_OPERS, static_cast<WORD>(IDM_HEXCTRL_MODIFY_DLG_OPERS) } },
		{ "CMD_MODIFY_FILLZEROS", { EHexCmd::CMD_MODIFY_FILLZEROS, static_cast<WORD>(IDM_HEXCTRL_MODIFY_FILLZEROS) } },
		{ "CMD_MODIFY_DLG_FILLDATA", { EHexCmd::CMD_MODIFY_DLG_FILLDATA, static_cast<WORD>(IDM_HEXCTRL_MODIFY_DLG_FILLDATA) } },
		{ "CMD_MODIFY_UNDO", { EHexCmd::CMD_MODIFY_UNDO, static_cast<WORD>(IDM_HEXCTRL_MODIFY_UNDO) } },
		{ "CMD_MODIFY_REDO", { EHexCmd::CMD_MODIFY_REDO, static_cast<WORD>(IDM_HEXCTRL_MODIFY_REDO) } },
		{ "CMD_SEL_MARKSTART", { EHexCmd::CMD_SEL_MARKSTART, static_cast<WORD>(IDM_HEXCTRL_SEL_MARKSTART) } },
		{ "CMD_SEL_MARKEND", { EHexCmd::CMD_SEL_MARKEND, static_cast<WORD>(IDM_HEXCTRL_SEL_MARKEND) } },
		{ "CMD_SEL_ALL", { EHexCmd::CMD_SEL_ALL, static_cast<WORD>(IDM_HEXCTRL_SEL_ALL) } },
		{ "CMD_SEL_ADDLEFT", { EHexCmd::CMD_SEL_ADDLEFT, static_cast<WORD>(0) } },
		{ "CMD_SEL_ADDRIGHT", { EHexCmd::CMD_SEL_ADDRIGHT, static_cast<WORD>(0) } },
		{ "CMD_SEL_ADDUP", { EHexCmd::CMD_SEL_ADDUP, static_cast<WORD>(0) } },
		{ "CMD_SEL_ADDDOWN", { EHexCmd::CMD_SEL_ADDDOWN, static_cast<WORD>(0) } },
		{ "CMD_DLG_DATAINTERPRET", { EHexCmd::CMD_DLG_DATAINTERP, static_cast<WORD>(IDM_HEXCTRL_DLG_DATAINTERP) } },
		{ "CMD_DLG_ENCODING", { EHexCmd::CMD_DLG_ENCODING, static_cast<WORD>(IDM_HEXCTRL_DLG_ENCODING) } },
		{ "CMD_APPEAR_FONTCHOOSE", { EHexCmd::CMD_APPEAR_FONTCHOOSE, static_cast<WORD>(IDM_HEXCTRL_APPEAR_FONTCHOOSE) } },
		{ "CMD_APPEAR_FONTINC", { EHexCmd::CMD_APPEAR_FONTINC, static_cast<WORD>(IDM_HEXCTRL_APPEAR_FONTINC) } },
		{ "CMD_APPEAR_FONTDEC", { EHexCmd::CMD_APPEAR_FONTDEC, static_cast<WORD>(IDM_HEXCTRL_APPEAR_FONTDEC) } },
		{ "CMD_APPEAR_CAPACINC", { EHexCmd::CMD_APPEAR_CAPACINC, static_cast<WORD>(IDM_HEXCTRL_APPEAR_CAPACINC) } },
		{ "CMD_APPEAR_CAPACDEC", { EHexCmd::CMD_APPEAR_CAPACDEC, static_cast<WORD>(IDM_HEXCTRL_APPEAR_CAPACDEC) } },
		{ "CMD_DLG_PRINT", { EHexCmd::CMD_DLG_PRINT, static_cast<WORD>(IDM_HEXCTRL_DLG_PRINT) } },
		{ "CMD_DLG_ABOUT", { EHexCmd::CMD_DLG_ABOUT, static_cast<WORD>(IDM_HEXCTRL_DLG_ABOUT) } },
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

	const auto lmbJson = [&](auto&& refJson, auto&& lmbParse)
	{
		std::vector<SKEYBIND> vecRet { };
		for (auto iterJ = refJson.MemberBegin(); iterJ != refJson.MemberEnd(); ++iterJ) //JSON data iterating.
		{
			if (const auto iterCmd = umapCmdMenu.find(iterJ->name.GetString()); iterCmd != umapCmdMenu.end())
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
	const auto lmbParse = [&](std::string_view str)->std::optional<SKEYBIND>
	{
		if (str.empty())
			return { };

		SKEYBIND stRet { };
		const auto iSize = str.size();
		size_t nPosStart { 0 }; //Next position to start search for '+' sign.
		const auto nSubWords = std::count(str.begin(), str.end(), '+') + 1; //How many sub-words (divided by '+')?
		for (auto i = 0; i < nSubWords; ++i)
		{
			size_t nSizeSubWord;
			const auto nPosNext = str.find('+', nPosStart);
			if (nPosNext == std::string_view::npos)
				nSizeSubWord = iSize - nPosStart;
			else
				nSizeSubWord = nPosNext - nPosStart;

			const auto strSubWord = str.substr(nPosStart, nSizeSubWord);
			nPosStart = nPosNext + 1;

			if (strSubWord.size() == 1)
				stRet.uKey = static_cast<UCHAR>(std::toupper(strSubWord[0])); //Binding keys are in uppercase.
			else if (const auto iter = umapKeys.find(strSubWord); iter != umapKeys.end())
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
	const auto lmbVecEqualize = [](std::vector<SKEYBIND>& vecMain, const std::vector<SKEYBIND>& vecFrom)
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
		if (const auto hRes = FindResourceW(hInst, MAKEINTRESOURCEW(IDR_HEXCTRL_JSON_KEYBIND), L"JSON"); hRes != nullptr)
		{
			if (const auto hData = LoadResource(hInst, hRes); hData != nullptr)
			{
				const auto nSize = static_cast<std::size_t>(SizeofResource(hInst, hRes));
				const auto* const pData = static_cast<char*>(LockResource(hData));

				if (doc.Parse(pData, nSize); !doc.IsNull()) //Parse all default keybindings.
				{
					m_vecKeyBind.clear();
					for (const auto& iter : umapCmdMenu) //Fill m_vecKeyBind with all possible commands from umapCmdMenu.
					{
						m_vecKeyBind.emplace_back(iter.second.first, iter.second.second);
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
			const auto iterEnd = m_vecKeyBind.begin() + i++;
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

void CHexCtrl::SetData(const HEXDATA& hds)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	ClearData();
	if (hds.spnData.empty()) //If data size is zero we do nothing further, just return after ClearData.
		return;

	m_fDataSet = true;
	m_spnData = hds.spnData;
	m_fMutable = hds.fMutable;
	m_pHexVirtData = hds.pHexVirtData;
	m_pHexVirtColors = hds.pHexVirtColors;
	m_dwCacheSize = hds.dwCacheSize; //Cache size must be at least sizeof(std::uint64_t) aligned.
	if (m_dwCacheSize < 0x10000 || (m_dwCacheSize % sizeof(std::uint64_t)) != 0)
	{
		m_dwCacheSize = 0x10000; //64Kb is the minimum default size.
	}
	m_fHighLatency = hds.fHighLatency;

	RecalcAll();
}

void CHexCtrl::SetDateInfo(DWORD dwDateFormat)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	assert(dwDateFormat == 0xFFFFFFFF || dwDateFormat <= 2);
	if (dwDateFormat != 0xFFFFFFFF || dwDateFormat > 2)
		return;

	if (dwDateFormat == 0xFFFFFFFF)
	{
		//Determine current user locale specific date format.
		if (GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_IDATE | LOCALE_RETURN_NUMBER,
			reinterpret_cast<LPWSTR>(&m_dwDateFormat), sizeof(m_dwDateFormat)) == 0)
		{
			assert(true); //Something went wrong.			
		}
	}
	else
		m_dwDateFormat = dwDateFormat;
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
	else if (GetCPInfoExW(static_cast<UINT>(iCodePage), 0, &stCPInfo) != FALSE && stCPInfo.MaxCharSize == 1)
		wstrFormat = L"Codepage %i";

	if (!wstrFormat.empty())
	{
		m_iCodePage = iCodePage;
		WCHAR buff[32];
		swprintf_s(buff, std::size(buff), wstrFormat.data(), m_iCodePage);
		m_wstrTextTitle = buff;
		Redraw();
	}
}

void CHexCtrl::SetFont(const LOGFONTW& lf)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_fontMain.DeleteObject();
	m_fontMain.CreateFontIndirectW(&lf);

	RecalcAll();
	ParentNotify(HEXCTRL_MSG_VIEWCHANGE);
}

void CHexCtrl::SetGroupMode(EHexDataSize enGroupMode)
{
	assert(IsCreated());
	assert(enGroupMode <= EHexDataSize::SIZE_QWORD);
	if (!IsCreated() || enGroupMode > EHexDataSize::SIZE_QWORD)
		return;

	m_enGroupMode = enGroupMode;

	//Getting the "Show data as..." menu pointer independent of position.
	const auto* const pMenuMain = m_menuMain.GetSubMenu(0);
	CMenu* pMenuShowDataAs { };
	for (int i = 0; i < pMenuMain->GetMenuItemCount(); ++i)
	{
		//Searching through all submenus whose first menuID is IDM_HEXCTRL_SHOWAS_BYTE.
		if (const auto pSubMenu = pMenuMain->GetSubMenu(i); pSubMenu != nullptr)
			if (pSubMenu->GetMenuItemID(0) == IDM_HEXCTRL_GROUPBY_BYTE)
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
		switch (enGroupMode)
		{
		case EHexDataSize::SIZE_BYTE:
			ID = IDM_HEXCTRL_GROUPBY_BYTE;
			break;
		case EHexDataSize::SIZE_WORD:
			ID = IDM_HEXCTRL_GROUPBY_WORD;
			break;
		case EHexDataSize::SIZE_DWORD:
			ID = IDM_HEXCTRL_GROUPBY_DWORD;
			break;
		case EHexDataSize::SIZE_QWORD:
			ID = IDM_HEXCTRL_GROUPBY_QWORD;
			break;
		}
		pMenuShowDataAs->CheckMenuItem(ID, MF_CHECKED | MF_BYCOMMAND);
	}
	SetCapacity(m_dwCapacity); //To recalc current representation.
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

void CHexCtrl::SetOffsetMode(bool fHex)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_fOffsetAsHex = fHex;
	WstrCapacityFill();
	RecalcAll();
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

void CHexCtrl::SetRedraw(bool fRedraw)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_fRedraw = fRedraw;
}

void CHexCtrl::SetSelection(const std::vector<HEXSPAN>& vecSel, bool fRedraw, bool fHighlight)
{
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet())
		return;

	m_pSelection->SetSelection(vecSel, fHighlight);

	if (fRedraw)
		Redraw();

	ParentNotify(HEXCTRL_MSG_SELECTION);
}

void CHexCtrl::SetUnprintableChar(wchar_t wcUnprintable)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_wchUnprintable = wcUnprintable;
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

auto CHexCtrl::BuildDataToDraw(ULONGLONG ullStartLine, int iLines)const->std::tuple<std::wstring, std::wstring>
{
	if (!IsDataSet())
		return { };

	const auto ullOffsetStart = ullStartLine * m_dwCapacity; //Offset of the visible data to print.
	std::size_t sSizeDataToPrint = static_cast<std::size_t>(iLines) * static_cast<std::size_t>(m_dwCapacity); //Size of the visible data to print.
	if (ullOffsetStart + sSizeDataToPrint > GetDataSize())
		sSizeDataToPrint = static_cast<std::size_t>(GetDataSize() - ullOffsetStart);

	const auto spnData = GetData({ ullOffsetStart, sSizeDataToPrint }); //Span data to print.
	assert(!spnData.empty());
	assert(spnData.size() == sSizeDataToPrint);

	const auto pData = reinterpret_cast<PBYTE>(spnData.data()); //Pointer to data to print.
	std::wstring wstrHex { };
	wstrHex.reserve(sSizeDataToPrint * 2);
	for (auto iterpData = pData; iterpData < pData + sSizeDataToPrint; ++iterpData) //Converting bytes to Hexes.
	{
		wstrHex.push_back(g_pwszHexMap[(*iterpData >> 4) & 0x0F]);
		wstrHex.push_back(g_pwszHexMap[*iterpData & 0x0F]);
	}

	//Converting bytes to Text.
	std::wstring wstrText { };
	const auto iEncoding = GetEncoding();
	if (iEncoding == -1) //If default ASCII codepage, simply assigning pData to wstrText w/o any conversion.
	{
		wstrText.assign(pData, pData + sSizeDataToPrint);
	}
	else
	{
		wstrText.resize(sSizeDataToPrint);
		MultiByteToWideChar(iEncoding, 0, reinterpret_cast<LPCCH>(pData),
			static_cast<int>(sSizeDataToPrint), wstrText.data(), static_cast<int>(sSizeDataToPrint));
	}
	ReplaceUnprintable(wstrText, iEncoding == -1, true, m_wchUnprintable);

	return { std::move(wstrHex), std::move(wstrText) };
}

void CHexCtrl::CaretMoveDown()
{
	const auto ullOldPos = m_ullCaretPos;
	const auto ullNewPos = ullOldPos + m_dwCapacity >= GetDataSize() ? ullOldPos : ullOldPos + m_dwCapacity;
	SetCaretPos(ullNewPos, m_fCaretHigh, false);

	const auto stOld = IsOffsetVisible(ullOldPos);
	const auto stNew = IsOffsetVisible(ullNewPos);
	if (stOld.i8Vert == 0 && stNew.i8Vert != 0)
		m_pScrollV->ScrollLineDown();

	Redraw();
}

void CHexCtrl::CaretMoveLeft()
{
	const auto ullOldPos = m_ullCaretPos;
	auto ullNewPos { 0ULL };

	if (IsCurTextArea() || !IsMutable())
	{
		if (ullOldPos == 0) //To avoid underflow.
			return;
		ullNewPos = ullOldPos - 1;
	}
	else
	{
		if (m_fCaretHigh)
		{
			if (ullOldPos == 0) //To avoid underflow.
				return;
			ullNewPos = ullOldPos - 1;
			m_fCaretHigh = false;
		}
		else
		{
			ullNewPos = ullOldPos;
			m_fCaretHigh = true;
		}
	}

	SetCaretPos(ullNewPos, m_fCaretHigh, false);

	const auto stOld = IsOffsetVisible(ullOldPos);
	const auto stNew = IsOffsetVisible(ullNewPos);
	if (stOld.i8Vert == 0 && stNew.i8Vert != 0)
		m_pScrollV->ScrollLineUp();
	else if (stNew.i8Horz != 0 && !IsCurTextArea()) //Do not horz scroll when in text area.
		ScrollOffsetH(ullNewPos);

	Redraw();
}

void CHexCtrl::CaretMoveRight()
{
	if (!IsDataSet())
		return;

	const auto ullOldPos = m_ullCaretPos;
	auto ullNewPos { 0ULL };

	if (IsCurTextArea() || !IsMutable())
		ullNewPos = ullOldPos + 1;
	else
	{
		if (m_fCaretHigh)
		{
			ullNewPos = ullOldPos;
			m_fCaretHigh = false;
		}
		else
		{
			ullNewPos = ullOldPos + 1;
			m_fCaretHigh = ullOldPos != GetDataSize() - 1; //Last byte case.
		}
	}

	if (const auto ullDataSize = GetDataSize(); ullNewPos >= ullDataSize) //To avoid overflow.
		ullNewPos = ullDataSize - 1;

	SetCaretPos(ullNewPos, m_fCaretHigh, false);

	const auto stOld = IsOffsetVisible(ullOldPos);
	const auto stNew = IsOffsetVisible(ullNewPos);
	if (stOld.i8Vert == 0 && stNew.i8Vert != 0)
		m_pScrollV->ScrollLineDown();
	else if (stNew.i8Horz != 0 && !IsCurTextArea()) //Do not horz scroll when in text area.
		ScrollOffsetH(ullNewPos);

	Redraw();
}

void CHexCtrl::CaretMoveUp()
{
	const auto ullOldPos = m_ullCaretPos;
	const auto ullNewPos = ullOldPos >= m_dwCapacity ? ullOldPos - m_dwCapacity : ullOldPos;
	SetCaretPos(ullNewPos, m_fCaretHigh, false);

	const auto stOld = IsOffsetVisible(ullOldPos);
	const auto stNew = IsOffsetVisible(ullNewPos);
	if (stOld.i8Vert == 0 && stNew.i8Vert != 0)
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
	const auto ullPos = GetDataSize() - 1;
	SetCaretPos(ullPos);
	GoToOffset(ullPos);
}

void CHexCtrl::CaretToLineBeg()
{
	const auto ullPos = GetCaretPos() - (GetCaretPos() % GetCapacity());
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

	const auto ullPos = GetCaretPos() - (GetCaretPos() % GetPageSize());
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

void CHexCtrl::ChooseFontDlg()
{
	CHOOSEFONTW chf { };
	LOGFONTW lf { };
	GetFont(lf);
	auto stClr = GetColors();
	chf.lStructSize = sizeof(CHOOSEFONTW);
	chf.hwndOwner = m_hWnd;
	chf.lpLogFont = &lf;
	chf.rgbColors = stClr.clrFontHex;
	chf.Flags = CF_FIXEDPITCHONLY | CF_NOSCRIPTSEL | CF_NOSIMULATIONS | CF_EFFECTS
		| CF_INITTOLOGFONTSTRUCT | CF_FORCEFONTEXIST;

	if (ChooseFontW(&chf) != FALSE)
	{
		SetFont(lf);
		stClr.clrFontHex = stClr.clrFontText = chf.rgbColors;
		SetColors(stClr);
	}
}

void CHexCtrl::ClipboardCopy(EClipboard enType)const
{
	if (m_pSelection->GetSelSize() > 1024 * 1024 * 8) //8MB
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
	case EClipboard::COPY_OFFSET:
		wstrData = CopyOffset();
		break;
	default:
		break;
	}

	constexpr auto sCharSize = sizeof(wchar_t);
	const size_t sMemSize = wstrData.size() * sCharSize + sCharSize;
	const auto hMem = GlobalAlloc(GMEM_MOVEABLE, sMemSize);
	if (!hMem)
	{
		::MessageBoxW(nullptr, L"GlobalAlloc error.", L"Error", MB_ICONERROR);
		return;
	}

	const auto lpMemLock = GlobalLock(hMem);
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
	if (!IsMutable() || !::OpenClipboard(m_hWnd))
		return;

	const auto hClipboard = GetClipboardData(CF_TEXT);
	if (!hClipboard)
		return;

	const auto pszClipboardData = static_cast<LPSTR>(GlobalLock(hClipboard));
	if (pszClipboardData == nullptr)
	{
		CloseClipboard();
		return;
	}

	ULONGLONG ullSize = strlen(pszClipboardData);
	ULONGLONG ullSizeToModify { };
	HEXMODIFY hmd;
	std::string strData; //Actual data to paste, must outlive hmd.

	const auto ullDataSize = GetDataSize();
	switch (enType)
	{
	case EClipboard::PASTE_TEXT:
		if (m_ullCaretPos + ullSize > ullDataSize)
			ullSize = ullDataSize - m_ullCaretPos;

		hmd.spnData = { reinterpret_cast<std::byte*>(pszClipboardData), static_cast<std::size_t>(ullSize) };
		ullSizeToModify = ullSize;
		break;
	case EClipboard::PASTE_HEX:
	{
		const ULONGLONG ullRealSize = ullSize / 2 + ullSize % 2;
		if (m_ullCaretPos + ullRealSize > ullDataSize)
			ullSize = (ullDataSize - m_ullCaretPos) * 2;

		const auto nIterations = static_cast<size_t>(ullSize / 2 + ullSize % 2);
		char chToUL[3] { }; //Array for actual Ascii chars to convert from.
		for (size_t i = 0; i < nIterations; ++i)
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

		hmd.spnData = { reinterpret_cast<std::byte*>(strData.data()), strData.size() };
		ullSizeToModify = strData.size();
	}
	break;
	default:
		break;
	}

	hmd.vecSpan.emplace_back(GetCaretPos(), ullSizeToModify);
	ModifyData(hmd);

	GlobalUnlock(hClipboard);
	CloseClipboard();
	RedrawWindow();
}

auto CHexCtrl::CopyBase64()const->std::wstring
{
	std::wstring wstrData;
	const auto ullSelSize = m_pSelection->GetSelSize();

	wstrData.reserve(static_cast<size_t>(ullSelSize) * 2);
	constexpr const wchar_t* const pwszBase64Map { L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" };
	unsigned int uValA = 0;
	int iValB = -6;
	for (unsigned i = 0; i < ullSelSize; ++i)
	{
		uValA = (uValA << 8) + GetIHexTData<BYTE>(*this, m_pSelection->GetOffsetByIndex(i));
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
	const auto ullSelSize = m_pSelection->GetSelSize();

	wstrData.reserve(static_cast<size_t>(ullSelSize) * 3 + 64);
	wstrData = L"unsigned char data[";
	wchar_t arrSelectionNum[8] { };
	swprintf_s(arrSelectionNum, std::size(arrSelectionNum), L"%llu", ullSelSize);
	wstrData += arrSelectionNum;
	wstrData += L"] = {\r\n";

	for (unsigned i = 0; i < ullSelSize; ++i)
	{
		wstrData += L"0x";
		const auto chByte = GetIHexTData<BYTE>(*this, m_pSelection->GetOffsetByIndex(i));
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
	const auto ullSelSize = m_pSelection->GetSelSize();

	wstrData.reserve(static_cast<size_t>(ullSelSize) * 2);
	for (unsigned i = 0; i < ullSelSize; ++i)
	{
		wstrData += L"\\x";
		const auto chByte = GetIHexTData<BYTE>(*this, m_pSelection->GetOffsetByIndex(i));
		wstrData += g_pwszHexMap[(chByte & 0xF0) >> 4];
		wstrData += g_pwszHexMap[(chByte & 0x0F)];
	}

	return wstrData;
}

auto CHexCtrl::CopyHex()const->std::wstring
{
	std::wstring wstrData;
	const auto ullSelSize = m_pSelection->GetSelSize();

	wstrData.reserve(static_cast<size_t>(ullSelSize) * 2);
	for (unsigned i = 0; i < ullSelSize; ++i)
	{
		const auto chByte = GetIHexTData<BYTE>(*this, m_pSelection->GetOffsetByIndex(i));
		wstrData += g_pwszHexMap[(chByte & 0xF0) >> 4];
		wstrData += g_pwszHexMap[(chByte & 0x0F)];
	}

	return wstrData;
}

auto CHexCtrl::CopyHexFormatted()const->std::wstring
{
	std::wstring wstrData;
	const auto ullSelStart = m_pSelection->GetSelStart();
	const auto ullSelSize = m_pSelection->GetSelSize();

	wstrData.reserve(static_cast<size_t>(ullSelSize) * 3);
	if (m_fSelectionBlock)
	{
		auto dwTail = m_pSelection->GetLineLength();
		for (unsigned i = 0; i < ullSelSize; ++i)
		{
			const auto chByte = GetIHexTData<BYTE>(*this, m_pSelection->GetOffsetByIndex(i));
			wstrData += g_pwszHexMap[(chByte & 0xF0) >> 4];
			wstrData += g_pwszHexMap[(chByte & 0x0F)];

			if (i < (ullSelSize - 1) && (dwTail - 1) != 0)
				if (((m_pSelection->GetLineLength() - dwTail + 1) % static_cast<DWORD>(m_enGroupMode)) == 0) //Add space after hex full chunk, ShowAs_size depending.
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
		const DWORD dwModStart = ullSelStart % m_dwCapacity;

		//When to insert first "\r\n".
		DWORD dwTail = m_dwCapacity - dwModStart;
		const DWORD dwNextBlock = (m_dwCapacity % 2) ? m_dwCapacityBlockSize + 2 : m_dwCapacityBlockSize + 1;

		//If at least two rows are selected.
		if (dwModStart + ullSelSize > m_dwCapacity)
		{
			DWORD dwCount = dwModStart * 2 + dwModStart / static_cast<DWORD>(m_enGroupMode);
			//Additional spaces between halves. Only in SIZE_BYTE's view mode.
			dwCount += (m_enGroupMode == EHexDataSize::SIZE_BYTE) ? (dwTail <= m_dwCapacityBlockSize ? 2 : 0) : 0;
			wstrData.insert(0, static_cast<size_t>(dwCount), ' ');
		}

		for (unsigned i = 0; i < ullSelSize; ++i)
		{
			const auto chByte = GetIHexTData<BYTE>(*this, m_pSelection->GetOffsetByIndex(i));
			wstrData += g_pwszHexMap[(chByte & 0xF0) >> 4];
			wstrData += g_pwszHexMap[(chByte & 0x0F)];

			if (i < (ullSelSize - 1) && (dwTail - 1) != 0)
			{
				if (m_enGroupMode == EHexDataSize::SIZE_BYTE && dwTail == dwNextBlock)
					wstrData += L"   "; //Additional spaces between halves. Only in SIZE_BYTE view mode.
				else if (((m_dwCapacity - dwTail + 1) % static_cast<DWORD>(m_enGroupMode)) == 0) //Add space after hex full chunk, ShowAs_size depending.
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
	const auto ullSelSize = m_pSelection->GetSelSize();

	wstrData.reserve(static_cast<size_t>(ullSelSize) * 2);
	for (auto i = ullSelSize; i > 0; --i)
	{
		const auto chByte = GetIHexTData<BYTE>(*this, m_pSelection->GetOffsetByIndex(i - 1));
		wstrData += g_pwszHexMap[(chByte & 0xF0) >> 4];
		wstrData += g_pwszHexMap[(chByte & 0x0F)];
	}

	return wstrData;
}

auto CHexCtrl::CopyOffset()const->std::wstring
{
	wchar_t pwszOffset[32] { };
	UllToWchars(GetCaretPos(), pwszOffset, static_cast<size_t>(m_dwOffsetBytes), m_fOffsetAsHex);
	std::wstring wstrData { m_fOffsetAsHex ? L"0x" : L"" };
	wstrData += pwszOffset;

	return wstrData;
}

auto CHexCtrl::CopyPrintScreen()const->std::wstring
{
	if (m_fSelectionBlock) //Only works with classical selection.
		return { };

	const auto ullSelStart = m_pSelection->GetSelStart();
	const auto ullSelSize = m_pSelection->GetSelSize();

	std::wstring wstrRet { };
	wstrRet.reserve(static_cast<size_t>(ullSelSize) * 4);
	wstrRet = L"Offset";
	wstrRet.insert(0, (static_cast<size_t>(m_dwOffsetDigits) - wstrRet.size()) / 2, ' ');
	wstrRet.insert(wstrRet.size(), static_cast<size_t>(m_dwOffsetDigits) - wstrRet.size(), ' ');
	wstrRet += L"   "; //Spaces to Capacity.
	wstrRet += m_wstrCapacity;
	wstrRet += L"   "; //Spaces to ASCII.
	if (const auto iSize = static_cast<int>(m_dwCapacity) - static_cast<int>(m_wstrTextTitle.size()); iSize > 0)
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

	const auto ullStartLine = ullSelStart / m_dwCapacity;
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
			if (((iterChunks + 1) % static_cast<DWORD>(m_enGroupMode)) == 0 && iterChunks < (m_dwCapacity - 1))
				wstrRet += L" ";

			//Additional space between capacity halves, only with SIZE_BYTE representation.
			if (m_enGroupMode == EHexDataSize::SIZE_BYTE && iterChunks == (m_dwCapacityBlockSize - 1))
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
	const auto ullSelSize = m_pSelection->GetSelSize();
	std::string strData;
	strData.reserve(static_cast<size_t>(ullSelSize));

	for (auto i = 0; i < ullSelSize; ++i)
		strData.push_back(GetIHexTData<BYTE>(*this, m_pSelection->GetOffsetByIndex(i)));

	std::wstring wstrText { };
	const auto iEncoding = GetEncoding();
	if (iEncoding == -1) //If default ASCII codepage, simply assigning strData to wstrText w/o any conversion.
	{
		wstrText.assign(strData.begin(), strData.end());
	}
	else
	{
		wstrText = str2wstr(strData, iEncoding);
	}
	ReplaceUnprintable(wstrText, iEncoding == -1, false, m_wchUnprintable);

	return wstrText;
}

void CHexCtrl::DrawWindow(CDC* pDC, CFont* pFont, CFont* pFontInfo)const
{
	const auto iScrollH = static_cast<int>(m_pScrollH->GetScrollPos());
	const auto iThirdHorzLine = m_iFirstHorzLine + m_iHeightClientArea - m_iHeightBottomOffArea;
	const auto iFourthHorzLine = iThirdHorzLine + m_iHeightBottomRect;

	CRect rcWnd(m_iFirstVertLine, m_iFirstHorzLine,
		m_iFirstVertLine + m_iWidthClientArea, m_iFirstHorzLine + m_iHeightClientArea);

	pDC->FillSolidRect(rcWnd, m_stColor.clrBk);
	pDC->SelectObject(m_penLines);

	//First horizontal line.
	pDC->MoveTo(m_iFirstVertLine - iScrollH, m_iFirstHorzLine);
	pDC->LineTo(m_iFourthVertLine, m_iFirstHorzLine);

	//Second horizontal line.
	pDC->MoveTo(m_iFirstVertLine - iScrollH, m_iSecondHorzLine);
	pDC->LineTo(m_iFourthVertLine, m_iSecondHorzLine);

	//Third horizontal line.
	pDC->MoveTo(m_iFirstVertLine - iScrollH, iThirdHorzLine);
	pDC->LineTo(m_iFourthVertLine, iThirdHorzLine);

	//Fourth horizontal line.
	pDC->MoveTo(m_iFirstVertLine - iScrollH, iFourthHorzLine);
	pDC->LineTo(m_iFourthVertLine, iFourthHorzLine);

	//First Vertical line.
	pDC->MoveTo(m_iFirstVertLine - iScrollH, m_iFirstHorzLine);
	pDC->LineTo(m_iFirstVertLine - iScrollH, iFourthHorzLine);

	//Second Vertical line.
	pDC->MoveTo(m_iSecondVertLine - iScrollH, m_iFirstHorzLine);
	pDC->LineTo(m_iSecondVertLine - iScrollH, iThirdHorzLine);

	//Third Vertical line.
	pDC->MoveTo(m_iThirdVertLine - iScrollH, m_iFirstHorzLine);
	pDC->LineTo(m_iThirdVertLine - iScrollH, iThirdHorzLine);

	//Fourth Vertical line.
	pDC->MoveTo(m_iFourthVertLine - iScrollH, m_iFirstHorzLine);
	pDC->LineTo(m_iFourthVertLine - iScrollH, iFourthHorzLine);

	//«Offset» text.
	CRect rcOffset(m_iFirstVertLine - iScrollH, m_iFirstHorzLine, m_iSecondVertLine - iScrollH, m_iSecondHorzLine);
	pDC->SelectObject(pFont);
	pDC->SetTextColor(m_stColor.clrFontCaption);
	pDC->SetBkColor(m_stColor.clrBk);
	pDC->DrawTextW(L"Offset", 6, rcOffset, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	//Capacity numbers.
	ExtTextOutW(pDC->m_hDC, m_iIndentFirstHexChunkX - iScrollH, m_iFirstHorzLine + m_iIndentCapTextY, 0, nullptr,
		m_wstrCapacity.data(), static_cast<UINT>(m_wstrCapacity.size()), nullptr);

	//Text area caption.
	auto rcText = GetRectTextCaption();
	pDC->DrawTextW(m_wstrTextTitle.data(), static_cast<int>(m_wstrTextTitle.size()), rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	//Bottom "Info" rect.
	CRect rcInfo(m_iFirstVertLine + 1 - iScrollH, iThirdHorzLine + 1, m_iFourthVertLine, iFourthHorzLine); //Fill bottom rcClient until iFourthHorizLine.
	pDC->FillSolidRect(rcInfo, m_stColor.clrBkInfoRect);
	rcInfo.left = m_iFirstVertLine + 5; //Draw text beginning with little indent.
	rcInfo.right = m_iFirstVertLine + m_iWidthClientArea; //Draw text to the end of the client area, even if it passes iFourthHorizLine.
	pDC->SelectObject(pFontInfo);
	pDC->SetTextColor(m_stColor.clrFontInfoRect);
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
			clrTextOffset = m_stColor.clrFontSelect;
			clrBkOffset = m_stColor.clrBkSelect;
		}
		else
		{
			clrTextOffset = m_stColor.clrFontCaption;
			clrBkOffset = m_stColor.clrBk;
		}

		//Left column offset printing (00000001...0000FFFF).
		wchar_t pwszOffset[32]; //To be enough for max as Hex and as Decimals.
		UllToWchars((ullStartLine + iterLines) * m_dwCapacity, pwszOffset, static_cast<size_t>(m_dwOffsetBytes), m_fOffsetAsHex);
		pDC->SelectObject(pFont);
		pDC->SetTextColor(clrTextOffset);
		pDC->SetBkColor(clrBkOffset);
		ExtTextOutW(pDC->m_hDC, m_iFirstVertLine + m_sizeLetter.cx - iScrollH, m_iStartWorkAreaY + (m_sizeLetter.cy * iterLines),
			0, nullptr, pwszOffset, m_dwOffsetDigits, nullptr);
	}
}

void CHexCtrl::DrawHexText(CDC* pDC, CFont* pFont, int iLines, std::wstring_view wstrHex, std::wstring_view wstrText)const
{
	std::vector<POLYTEXTW> vecPolyHex, vecPolyAscii;
	std::list<std::wstring> listWstrHex, listWstrAscii;
	const auto iScrollH = static_cast<int>(m_pScrollH->GetScrollPos());
	size_t sIndexToPrint { };

	for (auto iterLines = 0; iterLines < iLines; ++iterLines)
	{
		//Hex, Ascii, Bookmarks, Selection, Datainterpret, Cursor wstrings to print.
		std::wstring wstrHexToPrint { }, wstrAsciiToPrint { };
		const auto iHexPosToPrintX = m_iIndentFirstHexChunkX - iScrollH;
		const auto iAsciiPosToPrintX = m_iIndentTextX - iScrollH;
		const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeLetter.cy * iterLines; //Hex and Ascii the same.

		//Main loop for printing Hex chunks and Ascii chars.
		for (unsigned iterChunks = 0; iterChunks < m_dwCapacity && sIndexToPrint < wstrText.size(); ++iterChunks, ++sIndexToPrint)
		{
			//Hex chunk to print.
			wstrHexToPrint += wstrHex[sIndexToPrint * 2];
			wstrHexToPrint += wstrHex[sIndexToPrint * 2 + 1];

			//Additional space between grouped Hex chunks.
			if (((iterChunks + 1) % static_cast<DWORD>(m_enGroupMode)) == 0 && iterChunks < (m_dwCapacity - 1))
				wstrHexToPrint += L" ";

			//Additional space between capacity halves, only with SIZE_BYTE representation.
			if (m_enGroupMode == EHexDataSize::SIZE_BYTE && iterChunks == (m_dwCapacityBlockSize - 1))
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
	pDC->SetTextColor(m_stColor.clrFontHex);
	pDC->SetBkColor(m_stColor.clrBk);
	PolyTextOutW(pDC->m_hDC, vecPolyHex.data(), static_cast<UINT>(vecPolyHex.size()));

	//Ascii printing.
	pDC->SetTextColor(m_stColor.clrFontText);
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
		std::wstring wstrHexBkmToPrint { }, wstrAsciiBkmToPrint { }; //Bookmarks to print.
		int iBkmHexPosToPrintX { -1 }, iBkmAsciiPosToPrintX { };
		bool fBookmark { false };  //Flag to show current Bookmark in current Hex presence.
		const HEXBKM* pBkmCurr { };
		const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeLetter.cy * iterLines; //Hex and Ascii the same.

		//Main loop for printing Hex chunks and Ascii chars.
		for (unsigned iterChunks = 0; iterChunks < m_dwCapacity && sIndexToPrint < wstrText.size(); ++iterChunks, ++sIndexToPrint)
		{
			//Bookmarks.
			if (const auto* const pBookmark = m_pBookmarks->HitTest(ullStartOffset + sIndexToPrint); pBookmark != nullptr)
			{
				//If it's nested bookmark.
				if (pBkmCurr != nullptr && pBkmCurr != pBookmark)
				{
					if (!wstrHexBkmToPrint.empty()) //Only adding spaces if there are chars beforehead.
					{
						if ((iterChunks % static_cast<DWORD>(m_enGroupMode)) == 0)
							wstrHexBkmToPrint += L" ";

						//Additional space between capacity halves, only with SIZE_BYTE representation.
						if (m_enGroupMode == EHexDataSize::SIZE_BYTE && iterChunks == m_dwCapacityBlockSize)
							wstrHexBkmToPrint += L"  ";
					}

					//Hex bookmarks Poly.
					listWstrBookmarkHex.emplace_back(std::move(wstrHexBkmToPrint));
					vecBookmarksHex.emplace_back(BOOKMARKS { POLYTEXTW { iBkmHexPosToPrintX, iPosToPrintY,
						static_cast<UINT>(listWstrBookmarkHex.back().size()), listWstrBookmarkHex.back().data(), 0, { }, nullptr },
						pBkmCurr->clrBk, pBkmCurr->clrText });

					//Ascii bookmarks Poly.
					listWstrBookmarkAscii.emplace_back(std::move(wstrAsciiBkmToPrint));
					vecBookmarksAscii.emplace_back(BOOKMARKS { POLYTEXTW { iBkmAsciiPosToPrintX, iPosToPrintY,
						static_cast<UINT>(listWstrBookmarkAscii.back().size()), listWstrBookmarkAscii.back().data(), 0, { }, nullptr },
						pBkmCurr->clrBk, pBkmCurr->clrText });

					iBkmHexPosToPrintX = -1;
				}
				pBkmCurr = pBookmark;

				if (iBkmHexPosToPrintX == -1) //For just one time exec.
				{
					int iCy;
					HexChunkPoint(sIndexToPrint, iBkmHexPosToPrintX, iCy);
					TextChunkPoint(sIndexToPrint, iBkmAsciiPosToPrintX, iCy);
				}

				if (!wstrHexBkmToPrint.empty()) //Only adding spaces if there are chars beforehead.
				{
					if ((iterChunks % static_cast<DWORD>(m_enGroupMode)) == 0)
						wstrHexBkmToPrint += L" ";

					//Additional space between capacity halves, only with SIZE_BYTE representation.
					if (m_enGroupMode == EHexDataSize::SIZE_BYTE && iterChunks == m_dwCapacityBlockSize)
						wstrHexBkmToPrint += L"  ";
				}
				wstrHexBkmToPrint += wstrHex[sIndexToPrint * 2];
				wstrHexBkmToPrint += wstrHex[sIndexToPrint * 2 + 1];
				wstrAsciiBkmToPrint += wstrText[sIndexToPrint];
				fBookmark = true;
			}
			else if (fBookmark)
			{
				//There can be multiple bookmarks in one line. 
				//So, if there already were bookmarked bytes in the current line, we Poly them.
				//Same Poly mechanism presents at the end of the current (iterLines) loop,
				//to Poly bookmarks that end at the line's end.

				//Hex bookmarks Poly.
				listWstrBookmarkHex.emplace_back(std::move(wstrHexBkmToPrint));
				vecBookmarksHex.emplace_back(BOOKMARKS { POLYTEXTW { iBkmHexPosToPrintX, iPosToPrintY,
					static_cast<UINT>(listWstrBookmarkHex.back().size()), listWstrBookmarkHex.back().data(), 0, { }, nullptr },
					pBkmCurr->clrBk, pBkmCurr->clrText });

				//Ascii bookmarks Poly.
				listWstrBookmarkAscii.emplace_back(std::move(wstrAsciiBkmToPrint));
				vecBookmarksAscii.emplace_back(BOOKMARKS { POLYTEXTW { iBkmAsciiPosToPrintX, iPosToPrintY,
					static_cast<UINT>(listWstrBookmarkAscii.back().size()), listWstrBookmarkAscii.back().data(), 0, { }, nullptr },
					pBkmCurr->clrBk, pBkmCurr->clrText });

				iBkmHexPosToPrintX = -1;
				fBookmark = false;
				pBkmCurr = nullptr;
			}
		}

		//Bookmarks Poly.
		if (!wstrHexBkmToPrint.empty())
		{
			//Hex bookmarks Poly.
			listWstrBookmarkHex.emplace_back(std::move(wstrHexBkmToPrint));
			vecBookmarksHex.emplace_back(BOOKMARKS { POLYTEXTW { iBkmHexPosToPrintX, iPosToPrintY,
				static_cast<UINT>(listWstrBookmarkHex.back().size()), listWstrBookmarkHex.back().data(), 0, { }, nullptr },
				pBkmCurr->clrBk, pBkmCurr->clrText });

			//Ascii bookmarks Poly.
			listWstrBookmarkAscii.emplace_back(std::move(wstrAsciiBkmToPrint));
			vecBookmarksAscii.emplace_back(BOOKMARKS { POLYTEXTW { iBkmAsciiPosToPrintX, iPosToPrintY,
				static_cast<UINT>(listWstrBookmarkAscii.back().size()), listWstrBookmarkAscii.back().data(), 0, { }, nullptr },
				pBkmCurr->clrBk, pBkmCurr->clrText });
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
		HEXCOLORINFO hci { { m_hWnd, static_cast<UINT>(GetDlgCtrlID()) } };

		//Main loop for printing Hex chunks and Ascii chars.
		for (unsigned iterChunks = 0; iterChunks < m_dwCapacity && sIndexToPrint < wstrText.size(); ++iterChunks, ++sIndexToPrint)
		{
			//Colors.
			hci.ullOffset = ullStartOffset + sIndexToPrint;
			hci.pClr = nullptr; //Nullify for the next iteration.
			if (m_pHexVirtColors->OnHexGetColor(hci); hci.pClr != nullptr)
			{
				//If it's different color.
				if (optColorCurr && (optColorCurr->clrBk != hci.pClr->clrBk || optColorCurr->clrText != hci.pClr->clrText))
				{
					if (!wstrHexColorToPrint.empty()) //Only adding spaces if there are chars beforehead.
					{
						if ((iterChunks % static_cast<DWORD>(m_enGroupMode)) == 0)
							wstrHexColorToPrint += L" ";

						//Additional space between capacity halves, only with SIZE_BYTE representation.
						if (m_enGroupMode == EHexDataSize::SIZE_BYTE && iterChunks == m_dwCapacityBlockSize)
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
				optColorCurr = *hci.pClr;

				if (iColorHexPosToPrintX == -1) //For just one time exec.
				{
					int iCy;
					HexChunkPoint(sIndexToPrint, iColorHexPosToPrintX, iCy);
					TextChunkPoint(sIndexToPrint, iColorAsciiPosToPrintX, iCy);
				}

				if (!wstrHexColorToPrint.empty()) //Only adding spaces if there are chars beforehead.
				{
					if ((iterChunks % static_cast<DWORD>(m_enGroupMode)) == 0)
						wstrHexColorToPrint += L" ";

					//Additional space between capacity halves, only with SIZE_BYTE representation.
					if (m_enGroupMode == EHexDataSize::SIZE_BYTE && iterChunks == m_dwCapacityBlockSize)
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
					TextChunkPoint(sIndexToPrint, iSelAsciiPosToPrintX, iCy);
				}

				if (!wstrHexSelToPrint.empty()) //Only adding spaces if there are chars beforehead.
				{
					if ((iterChunks % static_cast<DWORD>(m_enGroupMode)) == 0)
						wstrHexSelToPrint += L" ";

					//Additional space between capacity halves, only with SIZE_BYTE representation.
					if (m_enGroupMode == EHexDataSize::SIZE_BYTE && iterChunks == m_dwCapacityBlockSize)
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
		pDC->SetTextColor(m_stColor.clrFontSelect);
		pDC->SetBkColor(m_stColor.clrBkSelect);
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
					TextChunkPoint(sIndexToPrint, iSelAsciiPosToPrintX, iCy);
				}

				if (!wstrHexSelToPrint.empty()) //Only adding spaces if there are chars beforehead.
				{
					if ((iterChunks % static_cast<DWORD>(m_enGroupMode)) == 0)
						wstrHexSelToPrint += L" ";

					//Additional space between capacity halves, only with SIZE_BYTE representation.
					if (m_enGroupMode == EHexDataSize::SIZE_BYTE && iterChunks == m_dwCapacityBlockSize)
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
		pDC->SetTextColor(m_stColor.clrBkSelect);
		pDC->SetBkColor(m_stColor.clrFontSelect);
		PolyTextOutW(pDC->m_hDC, vecPolySelHex.data(), static_cast<UINT>(vecPolySelHex.size()));

		//Ascii selection highlight printing.
		PolyTextOutW(pDC->m_hDC, vecPolySelAscii.data(), static_cast<UINT>(vecPolySelAscii.size()));
	}
}

void CHexCtrl::DrawCaret(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, int iLines, std::wstring_view wstrHex, std::wstring_view wstrText)const
{
	if (!IsDataSet())// || !IsMutable())
		return;

	std::vector<POLYTEXTW> vecPolyCaret;
	std::list<std::wstring> listWstrCaret;
	COLORREF clrBkCaret { }; //Caret color.
	const auto ullStartOffset = ullStartLine * m_dwCapacity;
	size_t sIndexToPrint { };
	bool fCurFound { false };

	for (auto iterLines = 0; iterLines < iLines && !fCurFound; ++iterLines)
	{
		std::wstring wstrHexCaretToPrint { }, wstrAsciiCaretToPrint { };
		int iCaretHexPosToPrintX { }, iCaretAsciiPosToPrintX { }; //Caret X coords.
		const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeLetter.cy * iterLines; //Hex and Ascii the same.

		//Main loop for printing Hex chunks and Ascii chars.
		for (unsigned iterChunks = 0; iterChunks < m_dwCapacity && sIndexToPrint < wstrText.size(); ++iterChunks, ++sIndexToPrint)
		{
			//Caret position. 
			if (ullStartOffset + sIndexToPrint == m_ullCaretPos)
			{
				int iCy;
				HexChunkPoint(m_ullCaretPos, iCaretHexPosToPrintX, iCy);
				TextChunkPoint(m_ullCaretPos, iCaretAsciiPosToPrintX, iCy);
				if (m_fCaretHigh)
					wstrHexCaretToPrint = wstrHex[sIndexToPrint * 2];
				else
				{
					wstrHexCaretToPrint = wstrHex[sIndexToPrint * 2 + 1];
					iCaretHexPosToPrintX += m_sizeLetter.cx;
				}
				wstrAsciiCaretToPrint = wstrText[sIndexToPrint];

				if (m_pSelection->HitTest(ullStartOffset + sIndexToPrint))
					clrBkCaret = m_stColor.clrBkCaretSelect;
				else
					clrBkCaret = m_stColor.clrBkCaret;

				fCurFound = true;
				break; //No need to loop further, only one Chunk with caret.
			}
		}

		//Caret Poly.
		if (!wstrHexCaretToPrint.empty())
		{
			listWstrCaret.emplace_back(std::move(wstrHexCaretToPrint));
			vecPolyCaret.emplace_back(POLYTEXTW { iCaretHexPosToPrintX, iPosToPrintY,
				static_cast<UINT>(listWstrCaret.back().size()), listWstrCaret.back().data(), 0, { }, nullptr });
			listWstrCaret.emplace_back(std::move(wstrAsciiCaretToPrint));
			vecPolyCaret.emplace_back(POLYTEXTW { iCaretAsciiPosToPrintX, iPosToPrintY,
				static_cast<UINT>(listWstrCaret.back().size()), listWstrCaret.back().data(), 0, { }, nullptr });
		}
	}

	//Caret printing.
	if (!vecPolyCaret.empty())
	{
		pDC->SelectObject(pFont);
		pDC->SetTextColor(m_stColor.clrFontCaret);
		pDC->SetBkColor(clrBkCaret);
		PolyTextOutW(pDC->m_hDC, vecPolyCaret.data(), static_cast<UINT>(vecPolyCaret.size()));
	}
}

void CHexCtrl::DrawDataInterp(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, int iLines, std::wstring_view wstrHex, std::wstring_view wstrText)const
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
			if (const auto ullSize = m_pDlgDataInterp->GetSize(); ullSize > 0
				&& ullStartOffset + sIndexToPrint >= m_ullCaretPos
				&& ullStartOffset + sIndexToPrint < m_ullCaretPos + ullSize)
			{
				if (iDataInterpretHexPosToPrintX == -1) //For just one time exec.
				{
					int iCy;
					HexChunkPoint(sIndexToPrint, iDataInterpretHexPosToPrintX, iCy);
					TextChunkPoint(sIndexToPrint, iDataInterpretAsciiPosToPrintX, iCy);
				}

				if (!wstrHexDataInterpretToPrint.empty()) //Only adding spaces if there are chars beforehead.
				{
					if ((iterChunks % static_cast<DWORD>(m_enGroupMode)) == 0)
						wstrHexDataInterpretToPrint += L" ";

					//Additional space between capacity halves, only with SIZE_BYTE representation.
					if (m_enGroupMode == EHexDataSize::SIZE_BYTE && iterChunks == m_dwCapacityBlockSize)
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
		pDC->SetTextColor(m_stColor.clrFontDataInterp);
		pDC->SetBkColor(m_stColor.clrBkDataInterp);
		PolyTextOutW(pDC->m_hDC, vecPolyDataInterpretHex.data(), static_cast<UINT>(vecPolyDataInterpretHex.size()));

		//Ascii selected printing.
		PolyTextOutW(pDC->m_hDC, vecPolyDataInterpretAscii.data(), static_cast<UINT>(vecPolyDataInterpretAscii.size()));
	}
}

void CHexCtrl::DrawPageLines(CDC* pDC, ULONGLONG ullStartLine, int iLines)
{
	if (!IsPageVisible())
		return;

	//Struct for sector lines.
	struct PAGELINES
	{
		POINT ptStart;
		POINT ptEnd;
	};
	std::vector<PAGELINES> vecPageLines;

	//Loop for printing Hex chunks and ASCII chars line by line.
	for (auto iterLines = 0; iterLines < iLines; ++iterLines)
	{
		//Page's lines vector to print.
		if ((((ullStartLine + iterLines) * m_dwCapacity) % m_dwPageSize == 0) && iterLines > 0)
		{
			const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeLetter.cy * iterLines; //Hex and Ascii the same.
			vecPageLines.emplace_back(PAGELINES { { m_iFirstVertLine, iPosToPrintY }, { m_iFourthVertLine, iPosToPrintY } });
		}
	}

	//Page lines printing.
	if (!vecPageLines.empty())
	{
		for (const auto& iter : vecPageLines)
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

	HEXMODIFY hms;
	hms.vecSpan = GetSelection();
	hms.enModifyMode = EHexModifyMode::MODIFY_REPEAT;
	std::byte byteZero { 0 };
	hms.spnData = { &byteZero, sizeof(byteZero) };
	ModifyData(hms);
	Redraw();
}

void CHexCtrl::FontSizeIncDec(bool fInc)
{
	//https://docs.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-logfontw#members
	auto lFontSize = GetFontSize();
	if (fInc)
		lFontSize = lFontSize < 0 ? lFontSize - 1 : lFontSize + 2;
	else
		lFontSize = lFontSize < 0 ? lFontSize + 1 : lFontSize - 2;
	SetFontSize(lFontSize);
}

auto CHexCtrl::GetBottomLine()const->ULONGLONG
{
	ULONGLONG ullEndLine { };
	if (IsDataSet())
	{
		ullEndLine = GetTopLine() + m_iHeightWorkArea / m_sizeLetter.cy;
		if (ullEndLine > 0) //To avoid underflow.
			--ullEndLine;
		//If m_ullDataSize is really small, or we at the scroll end,
		//we adjust ullEndLine to be not bigger than maximum allowed.
		if (const auto ullSize = GetDataSize(); ullEndLine >= (ullSize / m_dwCapacity))
			ullEndLine = (ullSize % m_dwCapacity) ? ullSize / m_dwCapacity : ullSize / m_dwCapacity - 1;
	}

	return ullEndLine;
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

long CHexCtrl::GetFontSize()
{
	LOGFONTW lf;
	GetFont(lf);

	return lf.lfHeight;
}

CRect CHexCtrl::GetRectTextCaption()const
{
	const auto iScrollH { static_cast<int>(m_pScrollH->GetScrollPos()) };
	return { m_iThirdVertLine - iScrollH, m_iFirstHorzLine, m_iFourthVertLine - iScrollH, m_iSecondHorzLine };
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

	iCx = static_cast<int>(((m_iIndentFirstHexChunkX + iBetweenBlocks + dwMod * m_iSizeHexByte) +
		(dwMod / static_cast<DWORD>(m_enGroupMode)) * m_iSpaceBetweenHexChunks) - m_pScrollH->GetScrollPos());

	auto ullScrollV = m_pScrollV->GetScrollPos();
	iCy = static_cast<int>((m_iStartWorkAreaY + (ullOffset / m_dwCapacity) * m_sizeLetter.cy) -
		(ullScrollV - (ullScrollV % m_sizeLetter.cy)));
}

auto CHexCtrl::HitTest(POINT pt)const->std::optional<HEXHITTEST>
{
	HEXHITTEST stHit;
	const auto iY = pt.y;
	const auto iX = pt.x + static_cast<int>(m_pScrollH->GetScrollPos()); //To compensate horizontal scroll.
	const auto ullCurLine = GetTopLine();

	bool fHit { false };
	//Checking if iX is within Hex chunks area.
	if ((iX >= m_iIndentFirstHexChunkX) && (iX < m_iThirdVertLine) && (iY >= m_iStartWorkAreaY) && (iY <= m_iEndWorkArea))
	{
		int iTotalSpaceBetweenChunks { 0 };
		for (auto i = 0U; i < m_dwCapacity; ++i)
		{
			if (i > 0 && (i % static_cast<DWORD>(m_enGroupMode)) == 0)
			{
				iTotalSpaceBetweenChunks += m_iSpaceBetweenHexChunks;
				if (m_enGroupMode == EHexDataSize::SIZE_BYTE && i == m_dwCapacityBlockSize)
					iTotalSpaceBetweenChunks += m_iSpaceBetweenBlocks;
			}

			const auto iCurrChunkBegin = m_iIndentFirstHexChunkX + (m_iSizeHexByte * i) + iTotalSpaceBetweenChunks;
			const auto iCurrChunkEnd = iCurrChunkBegin + m_iSizeHexByte +
				(((i + 1) % static_cast<DWORD>(m_enGroupMode)) == 0 ? m_iSpaceBetweenHexChunks : 0)
				+ ((m_enGroupMode == EHexDataSize::SIZE_BYTE && (i + 1) == m_dwCapacityBlockSize) ? m_iSpaceBetweenBlocks : 0);

			if (static_cast<unsigned int>(iX) < iCurrChunkEnd) //If iX lays in-between [iCurrChunkBegin...iCurrChunkEnd).
			{
				stHit.ullOffset = static_cast<ULONGLONG>(i) + ((iY - m_iStartWorkAreaY) / m_sizeLetter.cy) *
					m_dwCapacity + (ullCurLine * m_dwCapacity);

				if ((iX - iCurrChunkBegin) < static_cast<DWORD>(m_sizeLetter.cx)) //Check byte's High or Low half was hit.
					stHit.fIsHigh = true;

				fHit = true;
				break;
			}
		}
	}
	//Or within ASCII area.
	else if ((iX >= m_iIndentTextX) && (iX < (m_iIndentTextX + m_iDistanceBetweenChars * static_cast<int>(m_dwCapacity)))
		&& (iY >= m_iStartWorkAreaY) && iY <= m_iEndWorkArea)
	{
		//Calculate ullOffset ASCII symbol.
		stHit.ullOffset = ((iX - static_cast<ULONGLONG>(m_iIndentTextX)) / m_iDistanceBetweenChars) +
			((iY - m_iStartWorkAreaY) / m_sizeLetter.cy) * m_dwCapacity + (ullCurLine * m_dwCapacity);
		stHit.fIsAscii = true;
		fHit = true;
	}

	//If iX is out of end-bound of Hex chunks or ASCII chars.
	if (stHit.ullOffset >= GetDataSize())
		fHit = false;

	return fHit ? std::optional<HEXHITTEST> { stHit } : std::nullopt;
}

bool CHexCtrl::IsCurTextArea()const
{
	return m_fCursorTextArea;
}

bool CHexCtrl::IsDrawable()const
{
	return m_fRedraw;
}

bool CHexCtrl::IsPageVisible()const
{
	return m_dwPageSize > 0 && (m_dwPageSize % m_dwCapacity == 0) && m_dwPageSize >= m_dwCapacity;
}

template<typename T>
void CHexCtrl::ModifyWorker(const HEXMODIFY& hms, const T& lmbWorker, const std::span<std::byte> spnDataToOperWith)
{
	assert(!spnDataToOperWith.empty());
	if (spnDataToOperWith.empty())
		return;

	const auto& vecSpanRef = hms.vecSpan;
	const auto ullTotalSize = std::accumulate(vecSpanRef.begin(), vecSpanRef.end(), 0ULL,
		[](ULONGLONG ullSumm, const HEXSPAN& ref) {return ullSumm + ref.ullSize; });
	assert(ullTotalSize <= GetDataSize());

	CHexDlgCallback dlgClbk(L"Modifying...", vecSpanRef.back().ullOffset,
		vecSpanRef.back().ullOffset + ullTotalSize, this);
	const auto lmbThread = [&]()
	{
		for (const auto& iterSpan : vecSpanRef) //Span-vector's size times.
		{
			const auto ullOffsetToModify { iterSpan.ullOffset };
			const auto ullSizeToModify { iterSpan.ullSize };
			const auto ullSizeToOperWith { spnDataToOperWith.size() };

			//If the size of the data to_modify_from is bigger than
			//the data to modify, we do nothing.
			if (ullSizeToOperWith > ullSizeToModify)
				break;

			ULONGLONG ullSizeCache { };
			ULONGLONG ullChunks { };
			bool fCacheIsLargeEnough { true }; //Cache is larger than ullSizeToOperWith.

			if (!IsVirtual())
			{
				ullSizeCache = ullSizeToModify;
				ullChunks = 1;
			}
			else
			{
				ullSizeCache = GetCacheSize(); //Size of Virtual memory for acquiring, to work with.
				if (ullSizeCache >= ullSizeToOperWith)
				{
					ullSizeCache -= ullSizeCache % ullSizeToOperWith; //Aligning chunk size to ullSizeToOperWith.

					if (ullSizeToModify < ullSizeCache)
						ullSizeCache = ullSizeToModify;
					ullChunks = ullSizeToModify / ullSizeCache + ((ullSizeToModify % ullSizeCache) ? 1 : 0);
				}
				else
				{
					fCacheIsLargeEnough = false;
					const auto iSmallMod = ullSizeToOperWith % ullSizeCache;
					const auto ullSmallChunks = ullSizeToOperWith / ullSizeCache + (iSmallMod > 0 ? 1 : 0);
					ullChunks = (ullSizeToModify / ullSizeToOperWith) * ullSmallChunks;
				}
			}

			if (fCacheIsLargeEnough)
			{
				for (auto iterChunk { 0ULL }; iterChunk < ullChunks; ++iterChunk)
				{
					const auto ullOffsetCurr = ullOffsetToModify + (iterChunk * ullSizeCache);
					if (ullOffsetCurr + ullSizeCache > GetDataSize()) //Overflow check.
					{
						ullSizeCache = GetDataSize() - ullOffsetCurr;
						if (ullSizeCache < ullSizeToOperWith) //ullSizeChunk is too small for ullSizeToOperWith.
							break;
					}

					if ((ullOffsetCurr + ullSizeCache) > (ullOffsetToModify + ullSizeToModify))
					{
						ullSizeCache = (ullOffsetToModify + ullSizeToModify) - ullOffsetCurr;
						if (ullSizeCache < ullSizeToOperWith) //ullSizeChunk is too small for ullSizeToOperWith.
							break;
					}

					const auto spnData = GetData({ ullOffsetCurr, ullSizeCache });
					assert(!spnData.empty());
					for (auto ullIndex { 0ULL }; ullIndex <= (ullSizeCache - ullSizeToOperWith); ullIndex += ullSizeToOperWith)
					{
						lmbWorker(spnData.data() + ullIndex, hms, spnDataToOperWith);
						if (dlgClbk.IsCanceled())
						{
							SetDataVirtual(spnData, { ullOffsetCurr, ullSizeCache });
							goto exit;
						}
						dlgClbk.SetProgress(ullOffsetCurr + ullIndex);
					}
					SetDataVirtual(spnData, { ullOffsetCurr, ullSizeCache });
				}
			}
			else
			{
				//It's a special case for when the ullSizeToOperWith is larger than
				//the current cache size (only in Virtual mode).

				const auto iSmallMod = ullSizeToOperWith % ullSizeCache;
				const auto ullSmallChunks = ullSizeToOperWith / ullSizeCache + (iSmallMod > 0 ? 1 : 0);
				auto ullSmallChunkCur = 0ULL; //Current small chunk index.
				auto ullOffsetCurr = 0ULL;
				auto ullOffsetSubSpan = 0ULL; //Current offset for spnDataToOperWith.subspan().
				auto ullSizeCacheCurr = 0ULL; //Current cache size.
				for (auto iterChunk { 0ULL }; iterChunk < ullChunks; ++iterChunk)
				{
					if (ullSmallChunkCur == (ullSmallChunks - 1) && iSmallMod > 0)
					{
						ullOffsetCurr += iSmallMod;
						ullSizeCacheCurr = iSmallMod;
						ullOffsetSubSpan += iSmallMod;
					}
					else
					{
						ullOffsetCurr = ullOffsetToModify + (iterChunk * ullSizeCache);
						ullSizeCacheCurr = ullSizeCache;
						ullOffsetSubSpan = ullSmallChunkCur * ullSizeCache;
					}

					const auto spnData = GetData({ ullOffsetCurr, ullSizeCacheCurr });
					assert(!spnData.empty());

					lmbWorker(spnData.data(), hms, spnDataToOperWith.subspan(static_cast<std::size_t>(ullOffsetSubSpan),
						static_cast<std::size_t>(ullSizeCacheCurr)));
					if (dlgClbk.IsCanceled())
					{
						SetDataVirtual(spnData, { ullOffsetCurr, ullSizeCacheCurr });
						goto exit;
					}
					dlgClbk.SetProgress(ullOffsetCurr + ullSizeCacheCurr);
					SetDataVirtual(spnData, { ullOffsetCurr, ullSizeCacheCurr });

					if (++ullSmallChunkCur == ullSmallChunks)
					{
						ullSmallChunkCur = 0ULL;
						ullOffsetSubSpan = 0ULL;
					}
				}
			}
		}
	exit:
		dlgClbk.ExitDlg();
	};

	constexpr auto iSizeToRunThread { 1024 * 1024 }; //1MB.
	if (ullTotalSize > iSizeToRunThread) //Spawning new thread only if data size is big enough.
	{
		std::thread thrdWorker(lmbThread);
		dlgClbk.DoModal();
		thrdWorker.join();
	}
	else
	{
		lmbThread();
	}
}

void CHexCtrl::OnCaretPosChange(ULONGLONG ullOffset)
{
	//To prevent inspecting while key is pressed continuously.
	//Only when one time pressing.
	if (!m_fKeyDownAtm && m_pDlgDataInterp->IsWindowVisible())
		m_pDlgDataInterp->InspectOffset(ullOffset);

	if (auto pBkm = m_pBookmarks->HitTest(ullOffset); pBkm != nullptr) //If clicked on bookmark.
	{
		HEXBKMINFO hbi { { m_hWnd, static_cast<UINT>(GetDlgCtrlID()), HEXCTRL_MSG_BKMCLICK } };
		hbi.pBkm = pBkm;
		ParentNotify(hbi);
	}

	ParentNotify(HEXCTRL_MSG_CARETCHANGE);
}

template<typename T>
void CHexCtrl::ParentNotify(const T& t)const
{
	::SendMessageW(GetParent()->GetSafeHwnd(), WM_NOTIFY, GetDlgCtrlID(), reinterpret_cast<LPARAM>(&t));
}

void CHexCtrl::ParentNotify(UINT uCode)const
{
	NMHDR nmhdr { m_hWnd, static_cast<UINT>(GetDlgCtrlID()), uCode };
	ParentNotify(nmhdr);
}

void CHexCtrl::Print()
{
	CPrintDialogEx dlg(PD_RETURNDC | (HasSelection() ? PD_SELECTION : PD_NOSELECTION | PD_PAGENUMS), this);
	dlg.m_pdex.lStructSize = sizeof(PRINTDLGEX);
	dlg.m_pdex.nStartPage = START_PAGE_GENERAL;
	dlg.m_pdex.nMinPage = 1;
	dlg.m_pdex.nMaxPage = 0xffff;
	dlg.m_pdex.nPageRanges = 1;
	dlg.m_pdex.nMaxPageRanges = 1;
	PRINTPAGERANGE ppr { .nFromPage = 1, .nToPage = 1 };
	dlg.m_pdex.lpPageRanges = &ppr;

	if (dlg.DoModal() != S_OK)
	{
		MessageBoxW(L"Internal printer initialization error", L"Error", MB_ICONERROR);
		return;
	}

	//User pressed "Cancel", or "Apply" and then "Cancel".
	if (dlg.m_pdex.dwResultAction == PD_RESULT_CANCEL || dlg.m_pdex.dwResultAction == PD_RESULT_APPLY)
	{
		DeleteDC(dlg.m_pdex.hDC);
		return;
	}

	auto hdcPrinter = dlg.GetPrinterDC();
	if (hdcPrinter == nullptr)
	{
		MessageBoxW(L"No printer found!");
		return;
	}

	CDC dcPr;
	dcPr.Attach(hdcPrinter);
	auto pDC = &dcPr;

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
	const auto iScreenDpiY = GetDeviceCaps(pCurrDC->m_hDC, LOGPIXELSY);
	ReleaseDC(pCurrDC);
	const int iRatio = sizePrintDpi.cy / iScreenDpiY;

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
	const auto stOldLetter = m_sizeLetter;
	const auto ullTotalLines = GetDataSize() / m_dwCapacity;

	rcPrint.bottom -= 200; //Ajust bottom indent of the printing rect.
	RecalcPrint(pDC, &fontMain, &fontInfo, rcPrint);
	const auto iLinesInPage = m_iHeightWorkArea / m_sizeLetter.cy;
	const ULONGLONG ullTotalPages = ullTotalLines / iLinesInPage + 1;
	int iPagesToPrint { };

	if (dlg.PrintAll())
	{
		iPagesToPrint = static_cast<int>(ullTotalPages);
		ullStartLine = 0;
	}
	else if (dlg.PrintRange())
	{
		const auto iFromPage = ppr.nFromPage - 1;
		const auto iToPage = ppr.nToPage;
		if (iFromPage <= ullTotalPages) //Checks for out-of-range pages user input.
		{
			iPagesToPrint = iToPage - iFromPage;
			if (iPagesToPrint + iFromPage > ullTotalPages)
				iPagesToPrint = static_cast<int>(ullTotalPages - iFromPage);
		}
		ullStartLine = iFromPage * iLinesInPage;
	}

	constexpr auto iIndentX = 100;
	constexpr auto iIndentY = 100;
	pDC->SetMapMode(MM_TEXT);
	if (dlg.PrintSelection())
	{
		pDC->StartPage();
		pDC->OffsetViewportOrg(iIndentX, iIndentY);

		const auto ullSelStart = m_pSelection->GetSelStart();
		const auto ullSelSize = m_pSelection->GetSelSize();
		ullStartLine = ullSelStart / m_dwCapacity;

		const DWORD dwModStart = ullSelStart % m_dwCapacity;
		DWORD dwLines = static_cast<DWORD>(ullSelSize) / m_dwCapacity;
		if ((dwModStart + static_cast<DWORD>(ullSelSize)) > m_dwCapacity)
			++dwLines;
		if ((ullSelSize % m_dwCapacity) + dwModStart > m_dwCapacity)
			++dwLines;
		if (!dwLines)
			dwLines = 1;

		ullEndLine = ullStartLine + dwLines;
		const auto iLines = static_cast<int>(ullEndLine - ullStartLine);
		const auto& [wstrHex, wstrText] = BuildDataToDraw(ullStartLine, iLines);

		DrawWindow(pDC, &fontMain, &fontInfo);
		const auto clBkOld = m_stColor.clrBkSelect;
		const auto clTextOld = m_stColor.clrFontSelect;
		m_stColor.clrBkSelect = m_stColor.clrBk;
		m_stColor.clrFontSelect = m_stColor.clrFontHex;
		DrawOffsets(pDC, &fontMain, ullStartLine, iLines);
		DrawSelection(pDC, &fontMain, ullStartLine, iLines, wstrHex, wstrText);
		m_stColor.clrBkSelect = clBkOld;
		m_stColor.clrFontSelect = clTextOld;

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
				ullEndLine = (GetDataSize() % m_dwCapacity) ? ullTotalLines + 1 : ullTotalLines;

			const auto iLines = static_cast<int>(ullEndLine - ullStartLine);
			const auto& [wstrHex, wstrText] = BuildDataToDraw(ullStartLine, iLines);

			DrawWindow(pDC, &fontMain, &fontInfo); //Draw the window with all layouts.
			DrawOffsets(pDC, &fontMain, ullStartLine, iLines);
			DrawHexText(pDC, &fontMain, iLines, wstrHex, wstrText);
			DrawBookmarks(pDC, &fontMain, ullStartLine, iLines, wstrHex, wstrText);
			DrawCustomColors(pDC, &fontMain, ullStartLine, iLines, wstrHex, wstrText);
			DrawSelection(pDC, &fontMain, ullStartLine, iLines, wstrHex, wstrText);
			DrawSelHighlight(pDC, &fontMain, ullStartLine, iLines, wstrHex, wstrText);
			DrawCaret(pDC, &fontMain, ullStartLine, iLines, wstrHex, wstrText);
			DrawDataInterp(pDC, &fontMain, ullStartLine, iLines, wstrHex, wstrText);
			DrawPageLines(pDC, ullStartLine, iLines);

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
	const auto ullCurLineV = GetTopLine();

	//Current font size related.
	auto* pDC = GetDC();
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
	m_iSpaceBetweenBlocks = (m_enGroupMode == EHexDataSize::SIZE_BYTE && m_dwCapacity > 1) ? m_sizeLetter.cx * 2 : 0;
	m_iSpaceBetweenHexChunks = m_sizeLetter.cx;
	m_iDistanceBetweenHexChunks = m_iSizeHexByte * static_cast<DWORD>(m_enGroupMode) + m_iSpaceBetweenHexChunks;
	m_iThirdVertLine = m_iSecondVertLine + m_iDistanceBetweenHexChunks * (m_dwCapacity / static_cast<DWORD>(m_enGroupMode))
		+ m_sizeLetter.cx + m_iSpaceBetweenBlocks;
	m_iIndentTextX = m_iThirdVertLine + m_sizeLetter.cx;
	m_iDistanceBetweenChars = m_sizeLetter.cx;
	m_iFourthVertLine = m_iIndentTextX + (m_iDistanceBetweenChars * m_dwCapacity) + m_sizeLetter.cx;
	m_iIndentFirstHexChunkX = m_iSecondVertLine + m_sizeLetter.cx;
	m_iSizeFirstHalf = m_iIndentFirstHexChunkX + m_dwCapacityBlockSize * (m_sizeLetter.cx * 2) +
		(m_dwCapacityBlockSize / static_cast<DWORD>(m_enGroupMode) - 1) * m_iSpaceBetweenHexChunks;
	m_iHeightTopRect = std::lround(m_sizeLetter.cy * 1.5);
	m_iStartWorkAreaY = m_iFirstHorzLine + m_iHeightTopRect;
	m_iSecondHorzLine = m_iStartWorkAreaY - 1;
	m_iIndentCapTextY = m_iHeightTopRect / 2 - (m_sizeLetter.cy / 2);

	CRect rc;
	GetClientRect(&rc);
	RecalcWorkArea(rc.Height(), rc.Width());

	//Scroll sizes according to current font size.
	m_pScrollV->SetScrollSizes(m_sizeLetter.cy, static_cast<ULONGLONG>(m_iHeightWorkArea * m_dbWheelRatio),
		static_cast<ULONGLONG>(m_iStartWorkAreaY) + m_iHeightBottomOffArea + m_sizeLetter.cy *
		(GetDataSize() / m_dwCapacity + (GetDataSize() % m_dwCapacity == 0 ? 1 : 2)));
	m_pScrollH->SetScrollSizes(m_sizeLetter.cx, rc.Width(), static_cast<ULONGLONG>(m_iFourthVertLine) + 1);
	m_pScrollV->SetScrollPos(ullCurLineV * m_sizeLetter.cy);

	Redraw();
	ParentNotify(HEXCTRL_MSG_VIEWCHANGE);
}

void CHexCtrl::RecalcOffsetDigits()
{
	const auto ullDataSize = GetDataSize();
	if (ullDataSize <= 0xffffffffUL)
	{
		m_dwOffsetBytes = 4;
		m_dwOffsetDigits = IsOffsetAsHex() ? 8 : 10;
	}
	else if (ullDataSize <= 0xffffffffffUL)
	{
		m_dwOffsetBytes = 5;
		m_dwOffsetDigits = IsOffsetAsHex() ? 10 : 13;
	}
	else if (ullDataSize <= 0xffffffffffffUL)
	{
		m_dwOffsetBytes = 6;
		m_dwOffsetDigits = IsOffsetAsHex() ? 12 : 15;
	}
	else if (ullDataSize <= 0xffffffffffffffUL)
	{
		m_dwOffsetBytes = 7;
		m_dwOffsetDigits = IsOffsetAsHex() ? 14 : 17;
	}
	else
	{
		m_dwOffsetBytes = 8;
		m_dwOffsetDigits = IsOffsetAsHex() ? 16 : 19;
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
	m_iSpaceBetweenBlocks = (m_enGroupMode == EHexDataSize::SIZE_BYTE && m_dwCapacity > 1) ? m_sizeLetter.cx * 2 : 0;
	m_iSpaceBetweenHexChunks = m_sizeLetter.cx;
	m_iDistanceBetweenHexChunks = m_iSizeHexByte * static_cast<DWORD>(m_enGroupMode) + m_iSpaceBetweenHexChunks;
	m_iThirdVertLine = m_iSecondVertLine + m_iDistanceBetweenHexChunks * (m_dwCapacity / static_cast<DWORD>(m_enGroupMode))
		+ m_sizeLetter.cx + m_iSpaceBetweenBlocks;
	m_iIndentTextX = m_iThirdVertLine + m_sizeLetter.cx;
	m_iDistanceBetweenChars = m_sizeLetter.cx;
	m_iFourthVertLine = m_iIndentTextX + (m_iDistanceBetweenChars * m_dwCapacity) + m_sizeLetter.cx;
	m_iIndentFirstHexChunkX = m_iSecondVertLine + m_sizeLetter.cx;
	m_iSizeFirstHalf = m_iIndentFirstHexChunkX + m_dwCapacityBlockSize * (m_sizeLetter.cx * 2) +
		(m_dwCapacityBlockSize / static_cast<DWORD>(m_enGroupMode) - 1) * m_iSpaceBetweenHexChunks;
	m_iHeightTopRect = std::lround(m_sizeLetter.cy * 1.5);
	m_iStartWorkAreaY = m_iFirstHorzLine + m_iHeightTopRect;
	m_iSecondHorzLine = m_iStartWorkAreaY - 1;
	m_iIndentCapTextY = m_iHeightTopRect / 2 - (m_sizeLetter.cy / 2);

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

	std::vector<HEXSPAN> vecSpan { };
	std::transform(refRedo->begin(), refRedo->end(), std::back_inserter(vecSpan),
		[](SUNDO& ref) { return HEXSPAN { ref.ullOffset, ref.vecData.size() }; });

	//Making new Undo data snapshot.
	SnapshotUndo(vecSpan);

	for (const auto& iter : *refRedo)
	{
		const auto& refRedoData = iter.vecData;

		//In Virtual mode processing data chunk by chunk.
		if (IsVirtual() && refRedoData.size() > GetCacheSize())
		{
			const auto ullSizeChunk = GetCacheSize();
			const auto iMod = refRedoData.size() % ullSizeChunk;
			auto ullChunks = refRedoData.size() / ullSizeChunk + (iMod > 0 ? 1 : 0);
			std::size_t ullOffset = 0;
			while (ullChunks-- > 0)
			{
				const auto ullSize = (ullChunks == 1 && iMod > 0) ? iMod : ullSizeChunk;
				if (const auto spnData = GetData({ ullOffset, ullSize }); !spnData.empty())
				{
					std::copy_n(refRedoData.begin() + ullOffset, ullSize, spnData.data());
					SetDataVirtual(spnData, { ullOffset, ullSize });
				}
				ullOffset += ullSize;
			}
		}
		else {
			if (const auto spnData = GetData({ iter.ullOffset, refRedoData.size() }); !spnData.empty())
			{
				std::copy_n(refRedoData.begin(), refRedoData.size(), spnData.data());
				SetDataVirtual(spnData, { iter.ullOffset, refRedoData.size() });
			}
		}
	}

	m_deqRedo.pop_back();
	ParentNotify(HEXCTRL_MSG_SETDATA);
	RedrawWindow();
}

void CHexCtrl::ScrollOffsetH(ULONGLONG ullOffset)
{
	//Horizontally-only scrolls to a given offset.
	if (!m_pScrollH->IsVisible())
		return;

	int iCx, iCy;
	HexChunkPoint(ullOffset, iCx, iCy);
	CRect rcClient;
	GetClientRect(&rcClient);

	const auto ullCurrScrollH = m_pScrollH->GetScrollPos();
	auto ullNewScrollH { ullCurrScrollH };
	const auto iMaxClientX = rcClient.Width() - m_iSizeHexByte;
	const auto iAdditionalSpace { m_iSizeHexByte / 2 };
	if (iCx >= iMaxClientX)
		ullNewScrollH += iCx - iMaxClientX + iAdditionalSpace;
	else if (iCx < 0)
		ullNewScrollH += iCx - iAdditionalSpace;

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
	const auto fHasSel = HasSelection();
	const auto ullSelStart = fHasSel ? m_pSelection->GetSelStart() : m_ullCaretPos;
	const auto ullSelSize = fHasSel ? m_pSelection->GetSelSize() : 1;
	ULONGLONG ullClick { }, ullStart { }, ullSize { 0ULL };
	ULONGLONG ullNewPos { }; //Future pos of selection start.
	ULONGLONG ullOldPos { }; //Current pos of selection start.

	const auto lmbSelection = [&]()
	{
		if (ullSelStart == m_ullCursorPrev)
		{
			ullClick = ullStart = m_ullCursorPrev;
			ullSize = ullSelSize + m_dwCapacity;
			ullNewPos = ullClick + ullSize - 1;
			ullOldPos = ullNewPos - m_dwCapacity;
		}
		else if (ullSelStart < m_ullCursorPrev)
		{
			ullClick = m_ullCursorPrev;
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

	if (m_ullCaretPos == m_ullCursorPrev || m_ullCaretPos == ullSelStart
		|| m_ullCaretPos == m_pSelection->GetSelEnd())
		lmbSelection();
	else
	{
		ullClick = ullStart = m_ullCaretPos;
		ullSize = 1;
	}

	if (ullStart + ullSize > GetDataSize()) //To avoid overflow.
		ullSize = GetDataSize() - ullStart;

	if (ullSize > 0)
	{
		m_ullCaretPos = ullStart;
		m_ullCursorPrev = ullClick;
		SetSelection({ { ullStart, ullSize } }, false);

		const auto stOld = IsOffsetVisible(ullOldPos);
		const auto stNew = IsOffsetVisible(ullNewPos);
		if (stNew.i8Vert != 0 && stOld.i8Vert == 0)
			m_pScrollV->ScrollLineDown();
		Redraw();
		OnCaretPosChange(GetCaretPos());
	}
}

void CHexCtrl::SelAddLeft()
{
	const auto fHasSel = HasSelection();
	const auto ullSelStart = fHasSel ? m_pSelection->GetSelStart() : m_ullCaretPos;
	const auto ullSelSize = fHasSel ? m_pSelection->GetSelSize() : 1;
	ULONGLONG ullClick { }, ullStart { }, ullSize { 0 };
	ULONGLONG ullNewPos { }; //Future pos of selection start.
	ULONGLONG ullOldPos { }; //Current pos of selection start.

	const auto lmbSelection = [&]()
	{
		if (ullSelStart == m_ullCursorPrev && ullSelSize > 1)
		{
			ullClick = ullStart = m_ullCursorPrev;
			ullSize = ullSelSize - 1;
			ullOldPos = ullStart + ullSize;
			ullNewPos = ullOldPos - 1;
		}
		else
		{
			ullClick = m_ullCursorPrev;
			ullStart = ullSelStart > 0 ? ullSelStart - 1 : 0;
			ullSize = ullSelStart > 0 ? ullSelSize + 1 : ullSelSize;
			ullNewPos = ullStart;
			ullOldPos = ullNewPos + 1;
		}
	};

	if (m_ullCaretPos == m_ullCursorPrev
		|| m_ullCaretPos == ullSelStart
		|| m_ullCaretPos == m_pSelection->GetSelEnd())
		lmbSelection();
	else
	{
		ullClick = ullStart = m_ullCaretPos;
		ullSize = 1;
	}

	if (ullSize > 0)
	{
		m_ullCaretPos = ullStart;
		m_ullCursorPrev = ullClick;
		SetSelection({ { ullStart, ullSize } }, false);

		const auto stOld = IsOffsetVisible(ullOldPos);
		const auto stNew = IsOffsetVisible(ullNewPos);
		if (stNew.i8Vert != 0 && stOld.i8Vert == 0)
			m_pScrollV->ScrollLineUp();
		else if (stNew.i8Horz != 0 && !IsCurTextArea()) //Do not horz scroll when in text area.
			ScrollOffsetH(ullNewPos);
		else
			Redraw();
		OnCaretPosChange(GetCaretPos());
	}
}

void CHexCtrl::SelAddRight()
{
	const auto fHasSel = HasSelection();
	const auto ullSelStart = fHasSel ? m_pSelection->GetSelStart() : m_ullCaretPos;
	const auto ullSelSize = fHasSel ? m_pSelection->GetSelSize() : 1;
	ULONGLONG ullClick { }, ullStart { }, ullSize { 0UL };
	ULONGLONG ullNewPos { }; //Future pos of selection start.
	ULONGLONG ullOldPos { }; //Current pos of selection start.

	const auto lmbSelection = [&]()
	{
		if (ullSelStart == m_ullCursorPrev)
		{
			ullClick = ullStart = m_ullCursorPrev;
			ullSize = ullSelSize + 1;

			ullNewPos = ullClick + ullSize - 1;
			ullOldPos = ullNewPos - 1;
		}
		else
		{
			ullClick = m_ullCursorPrev;
			ullStart = ullSelStart + 1;
			ullSize = ullSelSize - 1;

			ullOldPos = ullSelStart;
			ullNewPos = ullOldPos + 1;
		}
	};

	if (m_ullCaretPos == m_ullCursorPrev
		|| m_ullCaretPos == ullSelStart
		|| m_ullCaretPos == m_pSelection->GetSelEnd())
		lmbSelection();
	else
	{
		ullClick = ullStart = m_ullCaretPos;
		ullSize = 1;
	}

	if (ullStart + ullSize > GetDataSize()) //To avoid overflow.
		ullSize = GetDataSize() - ullStart;

	if (ullSize > 0)
	{
		m_ullCaretPos = ullStart;
		m_ullCursorPrev = ullClick;
		SetSelection({ { ullStart, ullSize } }, false);

		const auto stOld = IsOffsetVisible(ullOldPos);
		const auto stNew = IsOffsetVisible(ullNewPos);
		if (stNew.i8Vert != 0 && stOld.i8Vert == 0)
			m_pScrollV->ScrollLineDown();
		else if (stNew.i8Horz != 0 && !IsCurTextArea()) //Do not horz scroll when in text area.
			ScrollOffsetH(ullNewPos);
		else
			Redraw();
		OnCaretPosChange(GetCaretPos());
	}
}

void CHexCtrl::SelAddUp()
{
	const auto fHasSel = HasSelection();
	const auto ullSelStart = fHasSel ? m_pSelection->GetSelStart() : m_ullCaretPos;
	const auto ullSelSize = fHasSel ? m_pSelection->GetSelSize() : 1;
	ULONGLONG ullClick { }, ullStart { 0 }, ullSize { 0 };
	ULONGLONG ullNewPos { }; //Future pos of selection start.
	ULONGLONG ullOldPos { }; //Current pos of selection start.

	const auto lmbSelection = [&]()
	{
		if (ullSelStart < m_ullCursorPrev)
		{
			ullClick = m_ullCursorPrev;
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
			ullClick = m_ullCursorPrev;
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

	if (m_ullCaretPos == m_ullCursorPrev
		|| m_ullCaretPos == ullSelStart
		|| m_ullCaretPos == m_pSelection->GetSelEnd())
		lmbSelection();
	else
	{
		ullClick = ullStart = m_ullCaretPos;
		ullSize = 1;
	}

	if (ullSize > 0)
	{
		m_ullCaretPos = ullStart;
		m_ullCursorPrev = ullClick;
		SetSelection({ { ullStart, ullSize } }, false);

		const auto stOld = IsOffsetVisible(ullOldPos);
		const auto stNew = IsOffsetVisible(ullNewPos);
		if (stNew.i8Vert != 0 && stOld.i8Vert == 0)
			m_pScrollV->ScrollLineUp();
		Redraw();
		OnCaretPosChange(GetCaretPos());
	}
}

void CHexCtrl::SetDataVirtual(std::span<std::byte> spnData, const HEXSPAN& hss)
{
	//Note: Since this method can be executed asynchronously (in search/replace, etc...),
	//the SendMesage(parent, ...) is impossible here because receiver window
	//must be run in the same thread as a sender.

	if (!IsVirtual())
		return;

	HEXDATAINFO hdi { { m_hWnd, static_cast<UINT>(GetDlgCtrlID()) } };
	hdi.stHexSpan = hss;
	hdi.spnData = spnData;
	m_pHexVirtData->OnHexSetData(hdi);
}

void CHexCtrl::SetFontSize(long lSize)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	//Prevent font size from being too small or too big.
	if (lSize < 0)
	{
		if (lSize < -100/*Max size*/ || lSize > -7/*Min size*/)
			return;
	}
	else if (lSize > 0)
	{
		if (lSize < 9/*Min size*/ || lSize > 75/*Max size*/)
			return;
	}

	LOGFONTW lf;
	GetFont(lf);
	lf.lfHeight = lSize;
	SetFont(lf);
}

void CHexCtrl::SnapshotUndo(const std::vector<HEXSPAN>& vecSpan)
{
	constexpr DWORD dwUndoMax { 500 }; //How many Undo states to preserve.
	const auto ullTotalSize = std::accumulate(vecSpan.begin(), vecSpan.end(), 0ULL,
		[](ULONGLONG ullSumm, const HEXSPAN& ref) {return ullSumm + ref.ullSize; });

	//Check for very big undo size.
	if (ullTotalSize > 1024 * 1024 * 10)
		return;

	//If Undo deque size is exceeding max limit,
	//remove first snapshot from the beginning (the oldest one).
	if (m_deqUndo.size() > static_cast<size_t>(dwUndoMax))
		m_deqUndo.pop_front();

	//Making new Undo data snapshot.
	const auto& refUndo = m_deqUndo.emplace_back(std::make_unique<std::vector<SUNDO>>());

	//Bad alloc may happen here!!!
	try
	{
		for (const auto& iterSel : vecSpan) //vecSpan.size() amount of continuous areas to preserve.
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

	std::size_t ullIndex { 0 };
	for (const auto& iterSel : vecSpan)
	{
		//In Virtual mode processing data chunk by chunk.
		if (IsVirtual() && iterSel.ullSize > GetCacheSize())
		{
			const auto dwSizeChunk = GetCacheSize();
			const auto ullMod = iterSel.ullSize % dwSizeChunk;
			auto ullChunks = iterSel.ullSize / dwSizeChunk + (ullMod > 0 ? 1 : 0);
			ULONGLONG ullOffset { 0 };
			while (ullChunks-- > 0)
			{
				const auto ullSize = (ullChunks == 1 && ullMod > 0) ? ullMod : dwSizeChunk;
				if (const auto spnData = GetData({ ullOffset, ullSize }); !spnData.empty())
					std::copy_n(spnData.data(), ullSize, refUndo->at(ullIndex).vecData.begin() + static_cast<std::size_t>(ullOffset));
				ullOffset += ullSize;
			}
		}
		else {
			if (const auto spnData = GetData(iterSel); !spnData.empty())
				std::copy_n(spnData.data(), iterSel.ullSize, refUndo->at(ullIndex).vecData.begin());
		}
		++ullIndex;
	}
}

void CHexCtrl::TextChunkPoint(ULONGLONG ullOffset, int& iCx, int& iCy)const
{	//This func computes x and y pos of given Ascii chunk.
	DWORD dwMod = ullOffset % m_dwCapacity;
	iCx = static_cast<int>((m_iIndentTextX + dwMod * m_sizeLetter.cx) - m_pScrollH->GetScrollPos());

	auto ullScrollV = m_pScrollV->GetScrollPos();
	iCy = static_cast<int>((m_iStartWorkAreaY + (ullOffset / m_dwCapacity) * m_sizeLetter.cy) -
		(ullScrollV - (ullScrollV % m_sizeLetter.cy)));
}

void CHexCtrl::TtBkmShow(bool fShow, POINT pt, bool fTimerCancel)
{
	if (fShow)
	{
		m_tmBkmTt = std::time(nullptr);
		m_wndTtBkm.SendMessageW(TTM_TRACKPOSITION, 0, static_cast<LPARAM>(MAKELONG(pt.x, pt.y)));
		m_wndTtBkm.SendMessageW(TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(&m_stToolInfoBkm));
		m_wndTtBkm.SendMessageW(TTM_TRACKACTIVATE, static_cast<WPARAM>(TRUE), reinterpret_cast<LPARAM>(&m_stToolInfoBkm));
		SetTimer(ID_TOOLTIP_BKM, 300, nullptr);
	}
	else if (fTimerCancel) //Tooltip was canceled by the timer, not mouse move.
	{
		m_wndTtBkm.SendMessageW(TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&m_stToolInfoBkm));
	}
	else
	{
		m_pBkmTtCurr = nullptr;
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

		wchar_t warrOffset[64] { L"Offset: " };
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

	std::size_t ullIndex { 0 };
	for (const auto& iter : *refUndo)
	{
		const auto& refUndoData = iter.vecData;

		//In Virtual mode processing data chunk by chunk.
		if (IsVirtual() && refUndoData.size() > GetCacheSize())
		{
			const auto ullSizeChunk = GetCacheSize();
			const auto iMod = refUndoData.size() % ullSizeChunk;
			auto ullChunks = refUndoData.size() / ullSizeChunk + (iMod > 0 ? 1 : 0);
			std::size_t ullOffset = 0;
			while (ullChunks-- > 0)
			{
				const auto ullSize = (ullChunks == 1 && iMod > 0) ? iMod : ullSizeChunk;
				if (const auto spnData = GetData({ ullOffset, ullSize }); !spnData.empty())
				{
					std::copy_n(spnData.data(), ullSize, refRedo->at(ullIndex).vecData.begin() + ullOffset); //Fill Redo with the data.
					std::copy_n(refUndoData.begin() + ullOffset, ullSize, spnData.data()); //Undo the data.
					SetDataVirtual(spnData, { ullOffset, ullSize });
				}
				ullOffset += ullSize;
			}
		}
		else {
			if (const auto spnData = GetData({ iter.ullOffset, refUndoData.size() }); !spnData.empty())
			{
				std::copy_n(spnData.data(), refUndoData.size(), refRedo->at(ullIndex).vecData.begin()); //Fill Redo with the data.
				std::copy_n(refUndoData.begin(), refUndoData.size(), spnData.data()); //Undo the data.
				SetDataVirtual(spnData, { iter.ullOffset, refUndoData.size() });
			}
		}
		++ullIndex;
	}
	m_deqUndo.pop_back();
	ParentNotify(HEXCTRL_MSG_SETDATA);
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
		if ((((iter + 1) % static_cast<DWORD>(m_enGroupMode)) == 0) && (iter < (m_dwCapacity - 1)))
			m_wstrCapacity += L" ";

		//Additional space between hex halves.
		if (m_enGroupMode == EHexDataSize::SIZE_BYTE && iter == (m_dwCapacityBlockSize - 1))
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
	if (!IsDataSet() || !IsMutable() || !IsCurTextArea() || (GetKeyState(VK_CONTROL) < 0))
		return;

	BYTE chByte = nChar & 0xFF;
	WCHAR warrCurrLocaleID[KL_NAMELENGTH];
	GetKeyboardLayoutNameW(warrCurrLocaleID); //Current langID as wstring.
	LCID dwCurrLocaleID { };
	if (wstr2num(warrCurrLocaleID, dwCurrLocaleID, 16)) //Convert langID from wstr to number.
	{
		UINT uCurrCodePage { };
		constexpr int iSize = sizeof(uCurrCodePage) / sizeof(WCHAR);
		if (GetLocaleInfoW(dwCurrLocaleID, LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
			reinterpret_cast<LPWSTR>(&uCurrCodePage), iSize) == iSize) //ANSI code page for the current langID (if any).
		{
			const wchar_t wch { static_cast<wchar_t>(nChar) };
			//Convert input symbol (wchar) to char according to current Windows' code page.
			if (auto str = wstr2str(&wch, uCurrCodePage); !str.empty())
				chByte = str[0];
		}
	}

	HEXMODIFY hms;
	hms.vecSpan.emplace_back(m_ullCaretPos, 1);
	hms.spnData = { reinterpret_cast<std::byte*>(&chByte), sizeof(chByte) };
	ModifyData(hms);
	CaretMoveRight();
}

BOOL CHexCtrl::OnCommand(WPARAM wParam, LPARAM lParam)
{
	const auto wMenuID = LOWORD(wParam);
	if (auto iter = std::find_if(m_vecKeyBind.begin(), m_vecKeyBind.end(), [=](const SKEYBIND& ref)
		{return ref.wMenuID == wMenuID;	}); iter != m_vecKeyBind.end())
	{
		m_fMenuCMD = true;
		ExecuteCmd(iter->eCmd);
	}
	else
	{	//For user defined custom menu we notifying parent window.
		HEXMENUINFO hmi { { m_hWnd, static_cast<UINT>(GetDlgCtrlID()), HEXCTRL_MSG_MENUCLICK } };
		hmi.wMenuID = wMenuID;
		hmi.pt = m_stMenuClickedPt;
		ParentNotify(hmi);
	}

	return CWnd::OnCommand(wParam, lParam);
}

void CHexCtrl::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	//Notify parent that we are about to display a context menu.
	HEXMENUINFO hmi { { m_hWnd, static_cast<UINT>(GetDlgCtrlID()), HEXCTRL_MSG_CONTEXTMENU } };
	hmi.pt = m_stMenuClickedPt = point;
	ParentNotify(hmi);
	m_menuMain.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON, point.x, point.y, this);
}

void CHexCtrl::OnDestroy()
{
	//All these cleanups below are important in case of HexCtrl 
	//being destroyed as a window but not as an object itself.
	//When a window is destroyed, the HexCtrl object itself is still alive if the IHexCtrl::Destroy() 
	//method was not called.

	ClearData();
	m_wndTtBkm.DestroyWindow();
	m_wndTtOffset.DestroyWindow();
	m_menuMain.DestroyMenu();
	m_fontMain.DeleteObject();
	m_fontInfo.DeleteObject();
	m_penLines.DeleteObject();
	m_umapHBITMAP.clear();
	m_vecKeyBind.clear();
	m_pDlgBkmMgr->DestroyWindow();
	m_pDlgDataInterp->DestroyWindow();
	m_pDlgFillData->DestroyWindow();
	m_pDlgEncoding->DestroyWindow();
	m_pDlgOpers->DestroyWindow();
	m_pDlgSearch->DestroyWindow();
	m_pDlgGoTo->DestroyWindow();
	m_pScrollV->DestroyWindow();
	m_pScrollH->DestroyWindow();

	m_dwCapacity = 0x10;
	m_fCreated = false;

	ParentNotify(HEXCTRL_MSG_DESTROY);

	CWnd::OnDestroy();
}

BOOL CHexCtrl::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
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
	RedrawWindow();
}

void CHexCtrl::OnInitMenuPopup(CMenu* /*pPopupMenu*/, UINT nIndex, BOOL /*bSysMenu*/)
{
	m_fMenuCMD = true;
	//The nIndex specifies the zero-based relative position of the menu item that opens the drop-down menu or submenu.
	switch (nIndex)
	{
	case 0:	//Search.
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_DLG_SEARCH, IsCmdAvail(EHexCmd::CMD_DLG_SEARCH) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_SEARCH_NEXT, IsCmdAvail(EHexCmd::CMD_SEARCH_NEXT) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_SEARCH_PREV, IsCmdAvail(EHexCmd::CMD_SEARCH_PREV) ? MF_ENABLED : MF_GRAYED);
		break;
	case 3:	//Navigation.
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_NAV_DLG_GOTO, IsCmdAvail(EHexCmd::CMD_NAV_DLG_GOTO) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_NAV_REPFWD, IsCmdAvail(EHexCmd::CMD_NAV_REPFWD) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_NAV_REPBKW, IsCmdAvail(EHexCmd::CMD_NAV_REPBKW) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_NAV_DATABEG, IsCmdAvail(EHexCmd::CMD_NAV_DATABEG) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_NAV_DATAEND, IsCmdAvail(EHexCmd::CMD_NAV_DATAEND) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_NAV_PAGEBEG, IsCmdAvail(EHexCmd::CMD_NAV_PAGEBEG) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_NAV_PAGEEND, IsCmdAvail(EHexCmd::CMD_NAV_PAGEEND) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_NAV_LINEBEG, IsCmdAvail(EHexCmd::CMD_NAV_LINEBEG) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_NAV_LINEEND, IsCmdAvail(EHexCmd::CMD_NAV_LINEEND) ? MF_ENABLED : MF_GRAYED);
		break;
	case 4:	//Bookmarks
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_BKM_ADD, IsCmdAvail(EHexCmd::CMD_BKM_ADD) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_BKM_REMOVE, IsCmdAvail(EHexCmd::CMD_BKM_REMOVE) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_BKM_NEXT, IsCmdAvail(EHexCmd::CMD_BKM_NEXT) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_BKM_PREV, IsCmdAvail(EHexCmd::CMD_BKM_PREV) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_BKM_CLEARALL, IsCmdAvail(EHexCmd::CMD_BKM_CLEARALL) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_BKM_DLG_MANAGER, IsCmdAvail(EHexCmd::CMD_BKM_DLG_MANAGER) ? MF_ENABLED : MF_GRAYED);
		break;
	case 5:	//Clipboard
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_COPYHEX, IsCmdAvail(EHexCmd::CMD_CLPBRD_COPYHEX) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_COPYHEXLE, IsCmdAvail(EHexCmd::CMD_CLPBRD_COPYHEXLE) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_COPYHEXFMT, IsCmdAvail(EHexCmd::CMD_CLPBRD_COPYHEXFMT) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_COPYTEXT, IsCmdAvail(EHexCmd::CMD_CLPBRD_COPYTEXT) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_COPYBASE64, IsCmdAvail(EHexCmd::CMD_CLPBRD_COPYBASE64) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_COPYCARR, IsCmdAvail(EHexCmd::CMD_CLPBRD_COPYCARR) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_COPYGREPHEX, IsCmdAvail(EHexCmd::CMD_CLPBRD_COPYGREPHEX) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_COPYPRNTSCRN, IsCmdAvail(EHexCmd::CMD_CLPBRD_COPYPRNTSCRN) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_COPYOFFSET, IsCmdAvail(EHexCmd::CMD_CLPBRD_COPYOFFSET) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_PASTEHEX, IsCmdAvail(EHexCmd::CMD_CLPBRD_PASTEHEX) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_PASTETEXT, IsCmdAvail(EHexCmd::CMD_CLPBRD_PASTETEXT) ? MF_ENABLED : MF_GRAYED);
		break;
	case 6: //Modify
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_MODIFY_FILLZEROS, IsCmdAvail(EHexCmd::CMD_MODIFY_FILLZEROS) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_MODIFY_DLG_FILLDATA, IsCmdAvail(EHexCmd::CMD_MODIFY_DLG_FILLDATA) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_MODIFY_DLG_OPERS, IsCmdAvail(EHexCmd::CMD_MODIFY_DLG_OPERS) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_MODIFY_UNDO, IsCmdAvail(EHexCmd::CMD_MODIFY_UNDO) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_MODIFY_REDO, IsCmdAvail(EHexCmd::CMD_MODIFY_REDO) ? MF_ENABLED : MF_GRAYED);
		break;
	case 7: //Selection
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_SEL_MARKSTART, IsCmdAvail(EHexCmd::CMD_SEL_MARKSTART) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_SEL_MARKEND, IsCmdAvail(EHexCmd::CMD_SEL_MARKEND) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_SEL_ALL, IsCmdAvail(EHexCmd::CMD_SEL_ALL) ? MF_ENABLED : MF_GRAYED);
		break;
	case 8: //Data Presentation.
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_DLG_DATAINTERP, IsCmdAvail(EHexCmd::CMD_DLG_DATAINTERP) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_DLG_ENCODING, IsCmdAvail(EHexCmd::CMD_DLG_ENCODING) ? MF_ENABLED : MF_GRAYED);
		break;
	default:
		break;
	}
	m_fMenuCMD = false;
}

void CHexCtrl::OnKeyDown(UINT nChar, UINT /*nRepCnt*/, UINT nFlags)
{
	//Bit 14 indicates that the key was pressed continuously.
	//0x4000 == 0100 0000 0000 0000.
	if (nFlags & 0x4000)
		m_fKeyDownAtm = true;

	if (auto optCmd = GetCommand(static_cast<BYTE>(nChar),
		GetAsyncKeyState(VK_CONTROL) < 0, GetAsyncKeyState(VK_SHIFT) < 0, GetAsyncKeyState(VK_MENU) < 0); optCmd)
		ExecuteCmd(*optCmd);
	else if (IsDataSet() && IsMutable() && !IsCurTextArea()) //If caret is in Hex area, just one part (High/Low) of byte must be changed.
	{
		BYTE chByte = nChar & 0xFF;
		//Normalizing all input in Hex area, reducing it to 0-15 (0x0-F) digit range.
		//Allowing only [0-9][A-F][NUM0-NUM9].
		if (chByte >= 0x30 && chByte <= 0x39)      //Digits [0-9].
			chByte -= 0x30;
		else if (chByte >= 0x41 && chByte <= 0x46) //Hex letters [A-F].
			chByte -= 0x37;
		else if (chByte >= 0x60 && chByte <= 0x69) //VK_NUMPAD0 - VK_NUMPAD9 [NUM0-NUM9].
			chByte -= 0x60;
		else
			return;

		auto chByteCurr = GetIHexTData<BYTE>(*this, m_ullCaretPos);
		if (m_fCaretHigh)
			chByte = (chByte << 4) | (chByteCurr & 0x0F);
		else
			chByte = (chByte & 0x0F) | (chByteCurr & 0xF0);

		HEXMODIFY hms;
		hms.vecSpan.emplace_back(m_ullCaretPos, 1);
		hms.spnData = { reinterpret_cast<std::byte*>(&chByte), sizeof(chByte) };
		ModifyData(hms);
		CaretMoveRight();
	}

	m_optRMouseClick.reset(); //Reset right mouse click.
}

void CHexCtrl::OnKeyUp(UINT /*nChar*/, UINT /*nRepCnt*/, UINT /*nFlags*/)
{
	//If some key was previously pressed for some time, and now is released.
	//Inspecting current caret position.
	if (m_fKeyDownAtm && IsDataSet())
	{
		m_fKeyDownAtm = false;
		m_pDlgDataInterp->InspectOffset(GetCaretPos());
	}
}

void CHexCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if ((point.x + static_cast<long>(m_pScrollH->GetScrollPos())) < m_iSecondVertLine) //DblClick on "Offset" area.
	{
		SetOffsetMode(!IsOffsetAsHex());
	}
	else if (GetRectTextCaption().PtInRect(point) != FALSE) //DblClick on codepage caption area.
	{
		ExecuteCmd(EHexCmd::CMD_DLG_ENCODING);
	}
	else if (const auto optHit = HitTest(point); optHit) //DblClick on hex/text area.
	{
		m_fLMousePressed = true;
		m_ullCaretPos = optHit->ullOffset;
		m_fCursorTextArea = optHit->fIsAscii;
		m_optRMouseClick.reset();
		if (!optHit->fIsAscii)
			m_fCaretHigh = optHit->fIsHigh;

		HEXSPAN hs;
		if (nFlags & MK_SHIFT)
		{
			const auto dwCapacity = GetCapacity();
			hs.ullOffset = m_ullCursorPrev = m_ullCursorNow = m_ullCaretPos - (m_ullCaretPos % dwCapacity);
			hs.ullSize = dwCapacity;
		}
		else
		{
			m_fSelectionBlock = GetAsyncKeyState(VK_MENU) < 0;
			m_ullCursorPrev = m_ullCursorNow = m_ullCaretPos;
			hs.ullOffset = m_ullCaretPos;
			hs.ullSize = 1ULL;
		}
		SetSelection({ hs });
		SetCapture();
	}
}

void CHexCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();

	const auto optHit = HitTest(point);
	if (!optHit)
		return;

	m_fLMousePressed = true;
	m_ullCursorNow = m_ullCaretPos = optHit->ullOffset;
	m_fCursorTextArea = optHit->fIsAscii;
	m_optRMouseClick.reset();
	if (!optHit->fIsAscii)
		m_fCaretHigh = optHit->fIsHigh;
	m_fSelectionBlock = GetAsyncKeyState(VK_MENU) < 0;
	SetCapture();

	std::vector<HEXSPAN> vecSel { };
	if (nFlags & MK_SHIFT)
	{
		ULONGLONG ullSelStart;
		ULONGLONG ullSelEnd;
		if (m_ullCursorNow <= m_ullCursorPrev)
		{
			ullSelStart = m_ullCursorNow;
			ullSelEnd = m_ullCursorPrev + 1;
		}
		else
		{
			ullSelStart = m_ullCursorPrev;
			ullSelEnd = m_ullCursorNow + 1;
		}
		vecSel.emplace_back(ullSelStart, ullSelEnd - ullSelStart);
	}
	else
		m_ullCursorPrev = m_ullCursorNow;

	if (HasSelection() || !vecSel.empty()) //To avoid set empty selection when it's already empty.
		SetSelection(vecSel);
	else
		Redraw();

	OnCaretPosChange(GetCaretPos());
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
		//If LMouse is pressed but cursor is outside of client area.
		//SetCapture() behaviour.

		CRect rcClient;
		GetClientRect(&rcClient);
		//Checking for scrollbars existence first.
		if (m_pScrollH->IsVisible())
		{
			if (point.x < rcClient.left) {
				m_pScrollH->ScrollLineLeft();
				point.x = m_iIndentFirstHexChunkX;
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

		//Checking if the current cursor pos is at the same byte's half
		//that it was at the previous WM_MOUSEMOVE fire.
		//Making selection of the byte only if the cursor has crossed byte's halves.
		//Doesn't apply when moving in ASCII area.
		if (!optHit || (optHit->ullOffset == m_ullCursorNow && m_fCaretHigh == optHit->fIsHigh && !optHit->fIsAscii))
			return;

		m_ullCursorNow = optHit->ullOffset;
		const auto ullHit = optHit->ullOffset;
		ULONGLONG ullClick, ullStart, ullSize, ullLines;
		if (m_fSelectionBlock) //Select block (with Alt)
		{
			ullClick = m_ullCursorPrev;
			if (ullHit >= ullClick)
			{
				if (ullHit % m_dwCapacity <= ullClick % m_dwCapacity)
				{
					const DWORD dwModStart = ullClick % m_dwCapacity - ullHit % m_dwCapacity;
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
					const DWORD dwModStart = ullHit % m_dwCapacity - ullClick % m_dwCapacity;
					ullStart = ullHit - dwModStart;
					ullSize = dwModStart + 1;
				}
				ullLines = (ullClick - ullStart) / m_dwCapacity + 1;
			}
		}
		else
		{
			if (ullHit <= m_ullCursorPrev)
			{
				ullClick = m_ullCursorPrev;
				ullStart = ullHit;
				ullSize = ullClick - ullStart + 1;
			}
			else
			{
				ullClick = m_ullCursorPrev;
				ullStart = m_ullCursorPrev;
				ullSize = ullHit - ullClick + 1;
			}
			ullLines = 1;
		}

		m_ullCursorPrev = ullClick;
		m_ullCaretPos = ullStart;
		std::vector<HEXSPAN> vecSel;
		vecSel.reserve(static_cast<size_t>(ullLines));
		for (auto iterLines = 0ULL; iterLines < ullLines; ++iterLines)
			vecSel.emplace_back(ullStart + m_dwCapacity * iterLines, ullSize);
		SetSelection(vecSel);
	}
	else
	{
		if (optHit)
		{
			if (const auto pBkm = m_pBookmarks->HitTest(optHit->ullOffset); pBkm != nullptr)
			{
				if (m_pBkmTtCurr != pBkm)
				{
					m_pBkmTtCurr = pBkm;
					CPoint ptScreen = point;
					ClientToScreen(&ptScreen);

					m_stToolInfoBkm.lpszText = pBkm->wstrDesc.data();
					TtBkmShow(true, POINT { ptScreen.x, ptScreen.y });
				}
			}
			else if (m_pBkmTtCurr != nullptr)
				TtBkmShow(false);
		}
		else if (m_pBkmTtCurr != nullptr) //If there is already bkm tooltip shown, but cursor is outside of data chunks.
			TtBkmShow(false);

		m_pScrollV->OnMouseMove(nFlags, point);
		m_pScrollH->OnMouseMove(nFlags, point);
	}
}

BOOL CHexCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (nFlags == MK_CONTROL)
	{
		FontSizeIncDec(zDelta > 0);
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

	if (!IsDrawable()) //Control should not be rendered atm.
		return;

	CRect rcClient;
	GetClientRect(rcClient);

	if (!IsCreated())
	{
		dc.FillSolidRect(rcClient, RGB(250, 250, 250));
		dc.TextOutW(1, 1, L"Call IHexCtrl::Create first.");
		return;
	}

	//To prevent drawing in too small window (can cause hangs).
	if (rcClient.IsRectEmpty() || rcClient.Height() < m_iHeightTopRect + m_iHeightBottomOffArea)
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
	DrawHexText(pDC, &m_fontMain, iLines, wstrHex, wstrText);
	DrawBookmarks(pDC, &m_fontMain, ullStartLine, iLines, wstrHex, wstrText);
	DrawCustomColors(pDC, &m_fontMain, ullStartLine, iLines, wstrHex, wstrText);
	DrawSelection(pDC, &m_fontMain, ullStartLine, iLines, wstrHex, wstrText);
	DrawSelHighlight(pDC, &m_fontMain, ullStartLine, iLines, wstrHex, wstrText);
	DrawCaret(pDC, &m_fontMain, ullStartLine, iLines, wstrHex, wstrText);
	DrawDataInterp(pDC, &m_fontMain, ullStartLine, iLines, wstrHex, wstrText);
	DrawPageLines(pDC, ullStartLine, iLines);
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
	ParentNotify(HEXCTRL_MSG_VIEWCHANGE);
}

void CHexCtrl::OnSysKeyDown(UINT nChar, UINT /*nRepCnt*/, UINT /*nFlags*/)
{
	if (auto optCmd = GetCommand(static_cast<BYTE>(nChar),
		GetAsyncKeyState(VK_CONTROL) < 0, GetAsyncKeyState(VK_SHIFT) < 0, true); optCmd)
		ExecuteCmd(*optCmd);
}

void CHexCtrl::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == ID_TOOLTIP_BKM)
	{
		constexpr auto tmSecToShow { 10.0 }; //How many seconds to show bkm tooltip.
		CRect rcClient;
		GetClientRect(rcClient);
		ClientToScreen(rcClient);
		CPoint ptCursor;
		GetCursorPos(&ptCursor);

		//Checking if cursor has left client rect.
		if (!rcClient.PtInRect(ptCursor))
			TtBkmShow(false);
		//Or more than tmSecToShow seconds have passed since bkm toolip is shown.
		else if (std::difftime(std::time(nullptr), m_tmBkmTt) >= tmSecToShow)
			TtBkmShow(false, { }, true);
	}
}

void CHexCtrl::OnVScroll(UINT /*nSBCode*/, UINT /*nPos*/, CScrollBar* /*pScrollBar*/)
{
	bool fRedraw { true };
	if (m_fHighLatency)
	{
		fRedraw = m_pScrollV->IsThumbReleased();
		TtOffsetShow(!fRedraw);
	}

	if (fRedraw)
	{
		RedrawWindow();
		ParentNotify(HEXCTRL_MSG_VIEWCHANGE); //Indicates to parent that view has changed.
	}
}
