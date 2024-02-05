/****************************************************************************************
* Copyright © 2018-2024 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "../dep/StrToNum/StrToNum/StrToNum.h"
#include "../dep/rapidjson/rapidjson-amalgam.h"
#include "../res/HexCtrlRes.h"
#include "CHexCtrl.h"
#include "Dialogs/CHexDlgBkmMgr.h"
#include "Dialogs/CHexDlgCallback.h"
#include "Dialogs/CHexDlgCodepage.h"
#include "Dialogs/CHexDlgDataInterp.h"
#include "Dialogs/CHexDlgGoTo.h"
#include "Dialogs/CHexDlgModify.h"
#include "Dialogs/CHexDlgSearch.h"
#include "Dialogs/CHexDlgTemplMgr.h"
#include <emmintrin.h>
#include <algorithm>
#include <bit>
#include <cassert>
#include <cwctype>
#include <format>
#include <fstream>
#include <numeric>
#include <random>
#include <thread>

import HEXCTRL.CHexScroll;
import HEXCTRL.CHexSelection;
import HEXCTRL.HexUtility;

using namespace HEXCTRL::INTERNAL;

extern "C" HEXCTRLAPI HEXCTRL::IHexCtrl * __cdecl HEXCTRL::CreateRawHexCtrl(HINSTANCE hInstance) {
	return new HEXCTRL::INTERNAL::CHexCtrl(hInstance);
};

namespace HEXCTRL::INTERNAL {
	class CHexDlgAbout final : public CDialogEx {
	public:
		explicit CHexDlgAbout(CWnd* pParent) : CDialogEx(IDD_HEXCTRL_ABOUT, pParent) {}
	private:
		afx_msg void OnDestroy() {
			DeleteObject(m_bmpLogo);
			CDialogEx::OnDestroy();
		};
		BOOL OnInitDialog()override;
		DECLARE_MESSAGE_MAP()
	private:
		HBITMAP m_bmpLogo { }; //Logo bitmap.
	};
}

BEGIN_MESSAGE_MAP(CHexDlgAbout, CDialogEx)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL CHexDlgAbout::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	const auto wstrVersion = std::format(L"Hex Control for MFC/Win32: v{}.{}.{}\r\nCopyright © 2018-2024 Jovibor",
		HEXCTRL_VERSION_MAJOR, HEXCTRL_VERSION_MINOR, HEXCTRL_VERSION_PATCH);
	GetDlgItem(IDC_HEXCTRL_ABOUT_STATIC_VERSION)->SetWindowTextW(wstrVersion.data());

	auto pDC = GetDC();
	const auto iLOGPIXELSY = GetDeviceCaps(pDC->m_hDC, LOGPIXELSY);
	ReleaseDC(pDC);

	const auto fScale = iLOGPIXELSY / 96.0F; //Scale factor for HighDPI displays.
	const auto iSizeIcon = static_cast<int>(32 * fScale);
	m_bmpLogo = static_cast<HBITMAP>(LoadImageW(AfxGetInstanceHandle(),
		MAKEINTRESOURCEW(IDB_HEXCTRL_LOGO), IMAGE_BITMAP, iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	static_cast<CStatic*>(GetDlgItem(IDC_HEXCTRL_ABOUT_LOGO))->SetBitmap(m_bmpLogo);

	return TRUE;
}

enum class CHexCtrl::EClipboard : std::uint8_t {
	COPY_HEX, COPY_HEXLE, COPY_HEXFMT, COPY_BASE64, COPY_CARR,
	COPY_GREPHEX, COPY_PRNTSCRN, COPY_OFFSET, COPY_TEXT_CP,
	PASTE_HEX, PASTE_TEXT_UTF16, PASTE_TEXT_CP
};

//Struct for UNDO command routine.
struct CHexCtrl::UNDO {
	ULONGLONG              ullOffset { }; //Start byte to apply Undo to.
	std::vector<std::byte> vecData { };   //Data for Undo.
};

//Key bindings.
struct CHexCtrl::KEYBIND {
	EHexCmd eCmd { };
	WORD    wMenuID { };
	UINT    uKey { };
	bool    fCtrl { };
	bool    fShift { };
	bool    fAlt { };
};

BEGIN_MESSAGE_MAP(CHexCtrl, CWnd)
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
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_NCACTIVATE()
	ON_WM_NCCALCSIZE()
	ON_WM_NCPAINT()
	ON_WM_PAINT()
	ON_WM_SETCURSOR()
	ON_WM_SIZE()
	ON_WM_SYSKEYDOWN()
	ON_WM_TIMER()
	ON_WM_VSCROLL()
END_MESSAGE_MAP()

#if defined(HEXCTRL_MANUAL_MFC_INIT) || defined(HEXCTRL_SHARED_DLL)
CWinApp theApp; //CWinApp object is vital for manual MFC, and for in-DLL work.

extern "C" HEXCTRLAPI BOOL __cdecl HexCtrlPreTranslateMessage(MSG * pMsg) {
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return theApp.PreTranslateMessage(pMsg);
}
#endif

CHexCtrl::CHexCtrl(HINSTANCE hInstClass)
{
#if defined(HEXCTRL_MANUAL_MFC_INIT)
	//MFC initialization, if HexCtrl is used in non MFC project with the "Shared MFC" linking.
	if (!AfxGetModuleState()->m_lpszCurrentAppName) {
		AfxWinInit(::GetModuleHandleW(nullptr), nullptr, ::GetCommandLineW(), 0);
	}
#endif

	//GetModuleHandleW(nullptr) always returns a handle to the file used to create the calling process (the .exe).
	//AfxGetInstanceHandle returns a handle to a .exe, or DLL if called from a DLL linked with the _USRDLL version of MFC.
	//When using HexCtrl as a Custom control in a dialog, it's vital to register Window Class (wc.hInstance) with
	//the .exe's handle not the DLL's, for proper subclassing in the dialog.
	//Otherwise, a Custom control won't find the "HexCtrl" Window Class, and dialog won't even start.
	//The hInstClass arg here is to provide a handle to the dialog's app, to properly register HexCtrl Window class with it.

	const auto hInst = hInstClass != nullptr ? hInstClass : AfxGetInstanceHandle();
	if (WNDCLASSEXW wc; ::GetClassInfoExW(hInst, m_pwszClassName, &wc) == FALSE) {
		wc.cbSize = sizeof(WNDCLASSEXW);
		wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = ::DefWindowProcW;
		wc.cbClsExtra = wc.cbWndExtra = 0;
		wc.hInstance = hInst;
		wc.hIcon = wc.hIconSm = nullptr;
		wc.hCursor = static_cast<HCURSOR>(LoadImageW(nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
		wc.hbrBackground = nullptr;
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = m_pwszClassName;
		if (!RegisterClassExW(&wc)) {
			DBG_REPORT(L"RegisterClassExW failed.");
			return;
		}
	}
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
	m_ullCaretPos = 0;
	m_ullCursorNow = 0;
	m_vecUndo.clear();
	m_vecRedo.clear();
	m_pScrollV->SetScrollPos(0);
	m_pScrollH->SetScrollPos(0);
	m_pScrollV->SetScrollSizes(0, 0, 0);
	m_pDlgBkmMgr->RemoveAll();
	m_pSelection->ClearAll();
	Redraw();
}

bool CHexCtrl::Create(const HEXCREATE& hcs)
{
	assert(!IsCreated()); //Already created.
	if (IsCreated())
		return false;

	if (hcs.fCustom) {
		//If it's a Custom Control in dialog, there is no need to create a window, just subclassing.
		if (!SubclassDlgItem(hcs.uID, CWnd::FromHandle(hcs.hWndParent))) {
			DBG_REPORT(L"SubclassDlgItem failed, check HEXCREATE parameters.");
			return false;
		}
	}
	else {
		if (!CWnd::CreateEx(hcs.dwExStyle, m_pwszClassName, L"HexCtrl", hcs.dwStyle, hcs.rect,
			CWnd::FromHandle(hcs.hWndParent), hcs.uID)) {
			DBG_REPORT(L"CreateWindowExW failed, check HEXCREATE parameters.");
			return false;
		}
	}

	m_wndTtBkm.CreateEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr, TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, nullptr);
	m_stToolInfoBkm.cbSize = sizeof(TTTOOLINFOW);
	m_stToolInfoBkm.uFlags = TTF_TRACK;
	m_stToolInfoBkm.uId = m_uIDTTBkm;
	m_wndTtBkm.SendMessageW(TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&m_stToolInfoBkm));
	m_wndTtBkm.SendMessageW(TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(400)); //To allow the use of a newline \n.

	m_wndTtTempl.CreateEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr, TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, nullptr);
	m_stToolInfoTempl.cbSize = sizeof(TTTOOLINFOW);
	m_stToolInfoTempl.uFlags = TTF_TRACK;
	m_stToolInfoTempl.uId = m_uIDTTTempl;
	m_wndTtTempl.SendMessageW(TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&m_stToolInfoTempl));
	m_wndTtTempl.SendMessageW(TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(400)); //To allow the use of a newline \n.

	m_wndTtOffset.CreateEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr, TTS_NOANIMATE | TTS_NOFADE | TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, nullptr);
	m_stToolInfoOffset.cbSize = sizeof(TTTOOLINFOW);
	m_stToolInfoOffset.uFlags = TTF_TRACK;
	m_stToolInfoOffset.uId = 0x03UL;
	m_wndTtOffset.SendMessageW(TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&m_stToolInfoOffset));
	m_wndTtOffset.SendMessageW(TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(400)); //To allow the use of a newline \n.

	if (hcs.pColors != nullptr) {
		m_stColors = *hcs.pColors;
	}

	m_dwCapacity = std::clamp(hcs.dwCapacity, 1UL, 100UL);
	m_dwGroupSize = std::clamp(hcs.dwGroupSize, 1UL, 64UL);
	m_flScrollRatio = hcs.flScrollRatio;
	m_fScrollLines = hcs.fScrollLines;
	m_fInfoBar = hcs.fInfoBar;
	m_fOffsetHex = hcs.fOffsetHex;

	const auto pDC = GetDC();
	m_iLOGPIXELSY = GetDeviceCaps(pDC->m_hDC, LOGPIXELSY);
	ReleaseDC(pDC);

	//Menu related.
	if (!m_menuMain.LoadMenuW(MAKEINTRESOURCEW(IDR_HEXCTRL_MENU))) {
		DBG_REPORT(L"LoadMenuW failed.");
		return false;
	}

	const auto hInst = AfxGetInstanceHandle();
	const auto fScale = m_iLOGPIXELSY / 96.0F; //Scale factor for HighDPI displays.
	const auto iSizeIcon = static_cast<int>(16 * fScale);
	const auto pMenuTop = m_menuMain.GetSubMenu(0); //Context sub-menu handle.

	MENUITEMINFOW mii { .cbSize { sizeof(MENUITEMINFOW) }, .fMask { MIIM_BITMAP } };
	//"Search" menu icon.
	mii.hbmpItem = static_cast<HBITMAP>(LoadImageW(hInst, MAKEINTRESOURCEW(IDB_HEXCTRL_SEARCH), IMAGE_BITMAP,
		iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	pMenuTop->SetMenuItemInfoW(0, &mii, TRUE); //"Search" parent menu icon.
	m_menuMain.SetMenuItemInfoW(IDM_HEXCTRL_SEARCH_DLGSEARCH, &mii);
	m_vecHBITMAP.emplace_back(mii.hbmpItem);

	//"Group Data" menu icon.
	mii.hbmpItem = static_cast<HBITMAP>(LoadImageW(hInst, MAKEINTRESOURCEW(IDB_HEXCTRL_GROUP), IMAGE_BITMAP,
		iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	pMenuTop->SetMenuItemInfoW(2, &mii, TRUE); //"Group Data" parent menu icon.
	m_vecHBITMAP.emplace_back(mii.hbmpItem);

	//"Bookmarks->Add" menu icon.
	mii.hbmpItem = static_cast<HBITMAP>(LoadImageW(hInst, MAKEINTRESOURCEW(IDB_HEXCTRL_BKMS), IMAGE_BITMAP,
		iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	pMenuTop->SetMenuItemInfoW(4, &mii, TRUE); //"Bookmarks" parent menu icon.
	m_menuMain.SetMenuItemInfoW(IDM_HEXCTRL_BKM_ADD, &mii);
	m_vecHBITMAP.emplace_back(mii.hbmpItem);

	//"Clipboard->Copy as Hex" menu icon.
	mii.hbmpItem = static_cast<HBITMAP>(LoadImageW(hInst, MAKEINTRESOURCEW(IDB_HEXCTRL_CLPBRD_COPYHEX), IMAGE_BITMAP,
		iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	pMenuTop->SetMenuItemInfoW(5, &mii, TRUE); //"Clipboard" parent menu icon.
	m_menuMain.SetMenuItemInfoW(IDM_HEXCTRL_CLPBRD_COPYHEX, &mii);
	m_vecHBITMAP.emplace_back(mii.hbmpItem);

	//"Clipboard->Paste as Hex" menu icon.
	mii.hbmpItem = static_cast<HBITMAP>(LoadImageW(hInst, MAKEINTRESOURCEW(IDB_HEXCTRL_CLPBRD_PASTEHEX), IMAGE_BITMAP,
		iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	m_menuMain.SetMenuItemInfoW(IDM_HEXCTRL_CLPBRD_PASTEHEX, &mii);
	m_vecHBITMAP.emplace_back(mii.hbmpItem);

	//"Modify" parent menu icon.
	mii.hbmpItem = static_cast<HBITMAP>(LoadImageW(hInst, MAKEINTRESOURCEW(IDB_HEXCTRL_MODIFY), IMAGE_BITMAP,
		iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	pMenuTop->SetMenuItemInfoW(6, &mii, TRUE);
	m_vecHBITMAP.emplace_back(mii.hbmpItem);

	//"Modify->Fill with Zeros" menu icon.
	mii.hbmpItem = static_cast<HBITMAP>(LoadImageW(hInst, MAKEINTRESOURCEW(IDB_HEXCTRL_MODIFY_FILLZEROS), IMAGE_BITMAP,
		iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	m_menuMain.SetMenuItemInfoW(IDM_HEXCTRL_MODIFY_FILLZEROS, &mii);
	m_vecHBITMAP.emplace_back(mii.hbmpItem);

	//"Data View->Data Interpreter" menu icon.
	mii.hbmpItem = static_cast<HBITMAP>(LoadImageW(hInst, MAKEINTRESOURCEW(IDB_HEXCTRL_DATAINTERP), IMAGE_BITMAP,
		iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	m_menuMain.SetMenuItemInfoW(IDM_HEXCTRL_DLGDATAINTERP, &mii);
	m_vecHBITMAP.emplace_back(mii.hbmpItem);

	//"Appearance->Choose Font" menu icon.
	mii.hbmpItem = static_cast<HBITMAP>(LoadImageW(hInst, MAKEINTRESOURCEW(IDB_HEXCTRL_FONTCHOOSE), IMAGE_BITMAP,
		iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	m_menuMain.SetMenuItemInfoW(IDM_HEXCTRL_APPEAR_DLGFONT, &mii);
	m_vecHBITMAP.emplace_back(mii.hbmpItem);
	//End of menu related.

	//Font related.
	//Default main logfont.
	const LOGFONTW lfMain { .lfHeight { -MulDiv(11, m_iLOGPIXELSY, 72) }, .lfPitchAndFamily { FIXED_PITCH },
		.lfFaceName { L"Consolas" } };
	m_fontMain.CreateFontIndirectW(hcs.pLogFont != nullptr ? hcs.pLogFont : &lfMain);

	//Info area font, independent from the main font, its size is a bit smaller than the default main font.
	const LOGFONTW lfInfo { .lfHeight { -MulDiv(11, m_iLOGPIXELSY, 72) + 1 }, .lfPitchAndFamily { FIXED_PITCH },
		.lfFaceName { L"Consolas" } };
	m_fontInfoBar.CreateFontIndirectW(&lfInfo);
	//End of font related.

	m_penLines.CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
	m_penDataTempl.CreatePen(PS_SOLID, 1, RGB(50, 50, 50));

	//ScrollBars should be created here, after the main window has already been created (to attach to), to avoid assertions.
	m_pScrollV->Create(this, true, IDB_HEXCTRL_SCROLL_ARROW, 0, 0, 0); //Actual sizes are set in RecalcAll().
	m_pScrollH->Create(this, false, IDB_HEXCTRL_SCROLL_ARROW, 0, 0, 0);
	m_pScrollV->AddSibling(m_pScrollH.get());
	m_pScrollH->AddSibling(m_pScrollV.get());

	m_fCreated = true; //Main creation flag.

	SetGroupSize(GetGroupSize());
	SetCodepage(-1);
	SetConfig(L"");
	SetDateInfo(0xFFFFFFFFUL, L'/');

	//All dialogs are initialized after the main window, to set the parent window correctly.
	m_pDlgBkmMgr->Initialize(this);
	m_pDlgDataInterp->Initialize(this);
	m_pDlgCodepage->Initialize(this);
	m_pDlgGoTo->Initialize(this);
	m_pDlgSearch->Initialize(this);
	m_pDlgTemplMgr->Initialize(this);
	m_pDlgModify->Initialize(this);

	return true;
}

bool CHexCtrl::CreateDialogCtrl(UINT uCtrlID, HWND hWndParent)
{
	assert(!IsCreated());
	if (IsCreated())
		return false;

	return Create({ .hWndParent { hWndParent }, .uID { uCtrlID }, .fCustom { true } });
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

	using enum EHexCmd;
	switch (eCmd) {
	case CMD_SEARCH_DLG:
		ParentNotify(HEXCTRL_MSG_DLGSEARCH);
		m_pDlgSearch->ShowWindow(SW_SHOW);
		break;
	case CMD_SEARCH_NEXT:
		m_pDlgSearch->SearchNextPrev(true);
		break;
	case CMD_SEARCH_PREV:
		m_pDlgSearch->SearchNextPrev(false);
		break;
	case CMD_NAV_GOTO_DLG:
		ParentNotify(HEXCTRL_MSG_DLGGOTO);
		m_pDlgGoTo->ShowWindow(SW_SHOW);
		break;
	case CMD_NAV_REPFWD:
		m_pDlgGoTo->Repeat();
		break;
	case CMD_NAV_REPBKW:
		m_pDlgGoTo->Repeat(false);
		break;
	case CMD_NAV_DATABEG:
		CaretToDataBeg();
		break;
	case CMD_NAV_DATAEND:
		CaretToDataEnd();
		break;
	case CMD_NAV_PAGEBEG:
		CaretToPageBeg();
		break;
	case CMD_NAV_PAGEEND:
		CaretToPageEnd();
		break;
	case CMD_NAV_LINEBEG:
		CaretToLineBeg();
		break;
	case CMD_NAV_LINEEND:
		CaretToLineEnd();
		break;
	case CMD_GROUPDATA_BYTE:
		SetGroupSize(1);
		break;
	case CMD_GROUPDATA_WORD:
		SetGroupSize(2);
		break;
	case CMD_GROUPDATA_DWORD:
		SetGroupSize(4);
		break;
	case CMD_GROUPDATA_QWORD:
		SetGroupSize(8);
		break;
	case CMD_GROUPDATA_INC:
		SetGroupSize(GetGroupSize() + 1);
		break;
	case CMD_GROUPDATA_DEC:
		SetGroupSize(GetGroupSize() - 1);
		break;
	case CMD_BKM_ADD:
		m_pDlgBkmMgr->AddBkm(HEXBKM { .vecSpan { HasSelection() ? GetSelection()
			: VecSpan { { GetCaretPos(), 1 } } }, .stClr { GetColors().clrBkBkm, GetColors().clrFontBkm } }, true);
		break;
	case CMD_BKM_REMOVE:
		m_pDlgBkmMgr->RemoveByOffset(GetCaretPos());
		break;
	case CMD_BKM_NEXT:
		m_pDlgBkmMgr->GoNext();
		break;
	case CMD_BKM_PREV:
		m_pDlgBkmMgr->GoPrev();
		break;
	case CMD_BKM_REMOVEALL:
		m_pDlgBkmMgr->RemoveAll();
		break;
	case CMD_BKM_DLG_MGR:
		ParentNotify(HEXCTRL_MSG_DLGBKMMGR);
		m_pDlgBkmMgr->ShowWindow(SW_SHOW);
		break;
	case CMD_CLPBRD_COPY_HEX:
		ClipboardCopy(EClipboard::COPY_HEX);
		break;
	case CMD_CLPBRD_COPY_HEXLE:
		ClipboardCopy(EClipboard::COPY_HEXLE);
		break;
	case CMD_CLPBRD_COPY_HEXFMT:
		ClipboardCopy(EClipboard::COPY_HEXFMT);
		break;
	case CMD_CLPBRD_COPY_TEXTCP:
		ClipboardCopy(EClipboard::COPY_TEXT_CP);
		break;
	case CMD_CLPBRD_COPY_BASE64:
		ClipboardCopy(EClipboard::COPY_BASE64);
		break;
	case CMD_CLPBRD_COPY_CARR:
		ClipboardCopy(EClipboard::COPY_CARR);
		break;
	case CMD_CLPBRD_COPY_GREPHEX:
		ClipboardCopy(EClipboard::COPY_GREPHEX);
		break;
	case CMD_CLPBRD_COPY_PRNTSCRN:
		ClipboardCopy(EClipboard::COPY_PRNTSCRN);
		break;
	case CMD_CLPBRD_COPY_OFFSET:
		ClipboardCopy(EClipboard::COPY_OFFSET);
		break;
	case CMD_CLPBRD_PASTE_HEX:
		ClipboardPaste(EClipboard::PASTE_HEX);
		break;
	case CMD_CLPBRD_PASTE_TEXTUTF16:
		ClipboardPaste(EClipboard::PASTE_TEXT_UTF16);
		break;
	case CMD_CLPBRD_PASTE_TEXTCP:
		ClipboardPaste(EClipboard::PASTE_TEXT_CP);
		break;
	case CMD_MODIFY_OPERS_DLG:
		ParentNotify(HEXCTRL_MSG_DLGMODIFY);
		m_pDlgModify->ShowWindow(SW_SHOW, 0);
		break;
	case CMD_MODIFY_FILLZEROS:
		FillWithZeros();
		break;
	case CMD_MODIFY_FILLDATA_DLG:
		ParentNotify(HEXCTRL_MSG_DLGMODIFY);
		m_pDlgModify->ShowWindow(SW_SHOW, 1);
		break;
	case CMD_MODIFY_UNDO:
		Undo();
		break;
	case CMD_MODIFY_REDO:
		Redo();
		break;
	case CMD_SEL_MARKSTART:
		m_pSelection->SetSelStartEnd(GetCaretPos(), true);
		Redraw();
		break;
	case CMD_SEL_MARKEND:
		m_pSelection->SetSelStartEnd(GetCaretPos(), false);
		Redraw();
		break;
	case CMD_SEL_ALL:
		SelAll();
		break;
	case CMD_SEL_ADDLEFT:
		SelAddLeft();
		break;
	case CMD_SEL_ADDRIGHT:
		SelAddRight();
		break;
	case CMD_SEL_ADDUP:
		SelAddUp();
		break;
	case CMD_SEL_ADDDOWN:
		SelAddDown();
		break;
	case CMD_DATAINTERP_DLG:
		ParentNotify(HEXCTRL_MSG_DLGDATAINTERP);
		m_pDlgDataInterp->ShowWindow(SW_SHOW);
		break;
	case CMD_CODEPAGE_DLG:
		ParentNotify(HEXCTRL_MSG_DLGCODEPAGE);
		m_pDlgCodepage->ShowWindow(SW_SHOW);
		break;
	case CMD_APPEAR_FONT_DLG:
		ChooseFontDlg();
		break;
	case CMD_APPEAR_FONTINC:
		FontSizeIncDec(true);
		break;
	case CMD_APPEAR_FONTDEC:
		FontSizeIncDec(false);
		break;
	case CMD_APPEAR_CAPACINC:
		SetCapacity(GetCapacity() + 1);
		break;
	case CMD_APPEAR_CAPACDEC:
		SetCapacity(GetCapacity() - 1);
		break;
	case CMD_PRINT_DLG:
		Print();
		break;
	case CMD_ABOUT_DLG:
	{
		CHexDlgAbout dlgAbout(this);
		dlgAbout.DoModal();
	}
	break;
	case CMD_CARET_LEFT:
		CaretMoveLeft();
		break;
	case CMD_CARET_RIGHT:
		CaretMoveRight();
		break;
	case CMD_CARET_UP:
		CaretMoveUp();
		break;
	case CMD_CARET_DOWN:
		CaretMoveDown();
		break;
	case CMD_SCROLL_PAGEUP:
		m_pScrollV->ScrollPageUp();
		break;
	case CMD_SCROLL_PAGEDOWN:
		m_pScrollV->ScrollPageDown();
		break;
	case CMD_TEMPL_APPLYCURR:
		m_pDlgTemplMgr->ApplyCurr(GetCaretPos());
		break;
	case CMD_TEMPL_DISAPPLY:
		m_pDlgTemplMgr->DisapplyByOffset(GetCaretPos());
		break;
	case CMD_TEMPL_DISAPPALL:
		m_pDlgTemplMgr->DisapplyAll();
		break;
	case CMD_TEMPL_DLG_MGR:
		ParentNotify(HEXCTRL_MSG_DLGTEMPLMGR);
		m_pDlgTemplMgr->ShowWindow(SW_SHOW);
		break;
	}
}

int CHexCtrl::GetActualWidth()const
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	return m_iFourthVertLinePx + 1; //+1px is the Pen width the line was drawn with.
}

auto CHexCtrl::GetBookmarks()const->IHexBookmarks*
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	return &*m_pDlgBkmMgr;
}

auto CHexCtrl::GetCacheSize()const->DWORD
{
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet())
		return { };

	return m_dwCacheSize;
}

auto CHexCtrl::GetCapacity()const->DWORD
{
	assert(IsCreated());
	if (!IsCreated())
		return 0xFFFFFFFFUL; //To disable compiler false warning C4724.

	return m_dwCapacity;
}

auto CHexCtrl::GetCaretPos()const->ULONGLONG
{
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet())
		return { };

	return m_ullCaretPos;
}

auto CHexCtrl::GetCharsExtraSpace()const->DWORD
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	return m_dwCharsExtraSpace;
}

int CHexCtrl::GetCodepage()const
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	return m_iCodePage;
}

auto CHexCtrl::GetColors()const->const HEXCOLORS&
{
	assert(IsCreated());
	return m_stColors;
}

auto CHexCtrl::GetData(HEXSPAN hss)const->SpanByte
{
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet())
		return { };

	SpanByte spnData;
	if (!IsVirtual()) {
		if (hss.ullOffset + hss.ullSize <= GetDataSize()) {
			spnData = { m_spnData.data() + hss.ullOffset, static_cast<std::size_t>(hss.ullSize) };
		}
	}
	else {
		if (hss.ullSize == 0 || hss.ullSize > GetCacheSize()) {
			hss.ullSize = GetCacheSize();
		}

		HEXDATAINFO hdi { .hdr { m_hWnd, static_cast<UINT>(GetDlgCtrlID()) }, .stHexSpan { hss } };
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

auto CHexCtrl::GetDateInfo()const->std::tuple<DWORD, wchar_t>
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	return { m_dwDateFormat, m_wchDateSepar };
}

auto CHexCtrl::GetDlgData(EHexWnd eWnd)const->std::uint64_t
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	switch (eWnd) {
	case EHexWnd::DLG_BKMMGR:
		return m_pDlgBkmMgr->GetDlgData();
	case EHexWnd::DLG_DATAINTERP:
		return m_pDlgDataInterp->GetDlgData();
	case EHexWnd::DLG_MODIFY:
		return m_pDlgModify->GetDlgData();
	case EHexWnd::DLG_SEARCH:
		return m_pDlgSearch->GetDlgData();
	case EHexWnd::DLG_CODEPAGE:
		return m_pDlgCodepage->GetDlgData();
	case EHexWnd::DLG_GOTO:
		return m_pDlgGoTo->GetDlgData();
	case EHexWnd::DLG_TEMPLMGR:
		return m_pDlgTemplMgr->GetDlgData();
	default:
		return { };
	}
}

auto CHexCtrl::GetFont()->LOGFONTW
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	LOGFONTW lf;
	m_fontMain.GetLogFont(&lf);
	return lf;
}

auto CHexCtrl::GetGroupSize()const->DWORD
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	return m_dwGroupSize;
}

auto CHexCtrl::GetMenuHandle()const->HMENU
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	return m_menuMain.GetSubMenu(0)->GetSafeHmenu();
}

auto CHexCtrl::GetPagesCount()const->ULONGLONG
{
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet() || GetPageSize() == 0)
		return { };

	const auto ullSize = GetDataSize();

	return ullSize / m_dwPageSize + ((ullSize % m_dwPageSize) ? 1 : 0);
}

auto CHexCtrl::GetPagePos()const->ULONGLONG
{
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet())
		return { };

	return GetCaretPos() / GetPageSize();
}

auto CHexCtrl::GetPageSize()const->DWORD
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	return m_dwPageSize;
}

auto CHexCtrl::GetScrollRatio()const->std::tuple<float, bool>
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	return { m_flScrollRatio, m_fScrollLines };
}

auto CHexCtrl::GetSelection()const->VecSpan
{
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet())
		return { };

	return m_pSelection->GetData();
}

auto CHexCtrl::GetTemplates()const->IHexTemplates*
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	return &*m_pDlgTemplMgr;
}

auto CHexCtrl::GetUnprintableChar()const->wchar_t
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	return m_wchUnprintable;
}

auto CHexCtrl::GetWndHandle(EHexWnd eWnd, bool fCreate)const->HWND
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	switch (eWnd) {
	case EHexWnd::WND_MAIN:
		return m_hWnd;
	case EHexWnd::DLG_BKMMGR:
		if (!IsWindow(m_pDlgBkmMgr->m_hWnd) && fCreate) {
			m_pDlgBkmMgr->Create(IDD_HEXCTRL_BKMMGR, CWnd::FromHandle(m_hWnd));
		}
		return m_pDlgBkmMgr->m_hWnd;
	case EHexWnd::DLG_DATAINTERP:
		if (!IsWindow(m_pDlgDataInterp->m_hWnd) && fCreate) {
			m_pDlgDataInterp->Create(IDD_HEXCTRL_DATAINTERP, CWnd::FromHandle(m_hWnd));
		}
		return m_pDlgDataInterp->m_hWnd;
	case EHexWnd::DLG_MODIFY:
		if (!IsWindow(m_pDlgModify->m_hWnd) && fCreate) {
			m_pDlgModify->Create(IDD_HEXCTRL_MODIFY, CWnd::FromHandle(m_hWnd));
		}
		return m_pDlgModify->m_hWnd;
	case EHexWnd::DLG_SEARCH:
		if (!IsWindow(m_pDlgSearch->m_hWnd) && fCreate) {
			m_pDlgSearch->Create(IDD_HEXCTRL_SEARCH, CWnd::FromHandle(m_hWnd));
		}
		return m_pDlgSearch->m_hWnd;
	case EHexWnd::DLG_CODEPAGE:
		if (!IsWindow(m_pDlgCodepage->m_hWnd) && fCreate) {
			m_pDlgCodepage->Create(IDD_HEXCTRL_CODEPAGE, CWnd::FromHandle(m_hWnd));
		}
		return m_pDlgCodepage->m_hWnd;
	case EHexWnd::DLG_GOTO:
		if (!IsWindow(m_pDlgGoTo->m_hWnd) && fCreate) {
			m_pDlgGoTo->Create(IDD_HEXCTRL_GOTO, CWnd::FromHandle(m_hWnd));
		}
		return m_pDlgGoTo->m_hWnd;
	case EHexWnd::DLG_TEMPLMGR:
		if (!IsWindow(m_pDlgTemplMgr->m_hWnd) && fCreate) {
			m_pDlgTemplMgr->Create(IDD_HEXCTRL_TEMPLMGR, CWnd::FromHandle(m_hWnd));
		}
		return m_pDlgTemplMgr->m_hWnd;
	default:
		return { };
	}
}

void CHexCtrl::GoToOffset(ULONGLONG ullOffset, int iRelPos)
{
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet() || ullOffset >= GetDataSize())
		return;

	const auto ullNewStartV = ullOffset / GetCapacity() * m_sizeFontMain.cy;
	auto ullNewScrollV { 0ULL };

	switch (iRelPos) {
	case -1: //Offset will be at the top line.
		ullNewScrollV = ullNewStartV;
		break;
	case 0: //Offset at the middle.
		if (ullNewStartV > m_iHeightWorkAreaPx / 2) { //To prevent negative numbers.
			ullNewScrollV = ullNewStartV - m_iHeightWorkAreaPx / 2;
			ullNewScrollV -= ullNewScrollV % m_sizeFontMain.cy;
		}
		break;
	case 1: //Offset at the bottom.
		ullNewScrollV = ullNewStartV - (GetBottomLine() - GetTopLine()) * m_sizeFontMain.cy;
		break;
	default:
		break;
	}
	m_pScrollV->SetScrollPos(ullNewScrollV);

	if (m_pScrollH->IsVisible() && !IsCurTextArea()) {
		ULONGLONG ullNewScrollH = (ullOffset % GetCapacity()) * m_iSizeHexBytePx;
		ullNewScrollH += (ullNewScrollH / m_iDistanceGroupedHexChunkPx) * GetCharWidthExtras();
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

	if (fScreen) {
		ScreenToClient(&pt);
	}

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

	using enum EHexCmd;
	switch (eCmd) {
	case CMD_BKM_REMOVE:
		fAvail = m_pDlgBkmMgr->HasBookmarks() && m_pDlgBkmMgr->HitTest(GetCaretPos()) != nullptr;
		break;
	case CMD_BKM_NEXT:
	case CMD_BKM_PREV:
	case CMD_BKM_REMOVEALL:
		fAvail = m_pDlgBkmMgr->HasBookmarks();
		break;
	case CMD_CLPBRD_COPY_HEX:
	case CMD_CLPBRD_COPY_HEXLE:
	case CMD_CLPBRD_COPY_HEXFMT:
	case CMD_CLPBRD_COPY_TEXTCP:
	case CMD_CLPBRD_COPY_BASE64:
	case CMD_CLPBRD_COPY_CARR:
	case CMD_CLPBRD_COPY_GREPHEX:
	case CMD_CLPBRD_COPY_PRNTSCRN:
		fAvail = fSelection;
		break;
	case CMD_CLPBRD_PASTE_HEX:
	case CMD_CLPBRD_PASTE_TEXTUTF16:
	case CMD_CLPBRD_PASTE_TEXTCP:
		fAvail = fMutable && IsClipboardFormatAvailable(CF_UNICODETEXT);
		break;
	case CMD_MODIFY_OPERS_DLG:
	case CMD_MODIFY_FILLDATA_DLG:
		fAvail = fMutable;
		break;
	case CMD_MODIFY_FILLZEROS:
		fAvail = fMutable && fSelection;
		break;
	case CMD_MODIFY_UNDO:
		fAvail = !m_vecUndo.empty();
		break;
	case CMD_MODIFY_REDO:
		fAvail = !m_vecRedo.empty();
		break;
	case CMD_BKM_ADD:
	case CMD_CARET_RIGHT:
	case CMD_CARET_LEFT:
	case CMD_CARET_DOWN:
	case CMD_CARET_UP:
	case CMD_SEARCH_DLG:
	case CMD_NAV_GOTO_DLG:
	case CMD_NAV_DATABEG:
	case CMD_NAV_DATAEND:
	case CMD_NAV_LINEBEG:
	case CMD_NAV_LINEEND:
	case CMD_BKM_DLG_MGR:
	case CMD_SEL_MARKSTART:
	case CMD_SEL_MARKEND:
	case CMD_SEL_ALL:
	case CMD_DATAINTERP_DLG:
	case CMD_CLPBRD_COPY_OFFSET:
		fAvail = fDataSet;
		break;
	case CMD_SEARCH_NEXT:
	case CMD_SEARCH_PREV:
		fAvail = m_pDlgSearch->IsSearchAvail();
		break;
	case CMD_NAV_PAGEBEG:
	case CMD_NAV_PAGEEND:
		fAvail = fDataSet && GetPagesCount() > 0;
		break;
	case CMD_NAV_REPFWD:
	case CMD_NAV_REPBKW:
		fAvail = fDataSet && m_pDlgGoTo->IsRepeatAvail();
		break;
	case CMD_TEMPL_APPLYCURR:
		fAvail = fDataSet && m_pDlgTemplMgr->HasCurrent();
		break;
	case CMD_TEMPL_DISAPPLY:
		fAvail = fDataSet && m_pDlgTemplMgr->HasApplied() && m_pDlgTemplMgr->HitTest(GetCaretPos()) != nullptr;
		break;
	case CMD_TEMPL_DISAPPALL:
		fAvail = fDataSet && m_pDlgTemplMgr->HasApplied();
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

bool CHexCtrl::IsInfoBar()const
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	return m_fInfoBar;
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

	return m_fOffsetHex;
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
	const auto iMaxClientX = rcClient.Width() - m_iSizeHexBytePx;

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

	m_vecRedo.clear(); //No Redo unless we make Undo.
	SnapshotUndo(hms.vecSpan);

	SetRedraw(false);
	using enum EHexModifyMode;
	switch (hms.eModifyMode) {
	case MODIFY_ONCE:
	{
		const auto stHexSpan = hms.vecSpan.back();
		const auto ullOffsetToModify = stHexSpan.ullOffset;
		const auto ullSizeToModify = (std::min)(stHexSpan.ullSize, static_cast<ULONGLONG>(hms.spnData.size()));

		assert((ullOffsetToModify + ullSizeToModify) <= GetDataSize());
		if ((ullOffsetToModify + ullSizeToModify) > GetDataSize())
			return;

		if (IsVirtual() && ullSizeToModify > GetCacheSize()) {
			const auto ullSizeCache = GetCacheSize();
			const auto ullRem = ullSizeToModify % ullSizeCache;
			auto ullChunks = ullSizeToModify / ullSizeCache + (ullRem > 0 ? 1 : 0);
			auto ullOffsetCurr = ullOffsetToModify;
			auto ullOffsetSpanCurr = 0ULL;
			while (ullChunks-- > 0) {
				const auto ullSizeToModifyCurr = (ullChunks == 1 && ullRem > 0) ? ullRem : ullSizeCache;
				const auto spnData = GetData({ ullOffsetCurr, ullSizeToModifyCurr });
				assert(!spnData.empty());
				std::copy_n(hms.spnData.data() + ullOffsetSpanCurr, ullSizeToModifyCurr, spnData.data());
				SetDataVirtual(spnData, { ullOffsetCurr, ullSizeToModifyCurr });
				ullOffsetCurr += ullSizeToModifyCurr;
				ullOffsetSpanCurr += ullSizeToModifyCurr;
			}
		}
		else {
			const auto spnData = GetData({ ullOffsetToModify, ullSizeToModify });
			assert(!spnData.empty());
			std::copy_n(hms.spnData.data(), static_cast<std::size_t>(ullSizeToModify), spnData.data());
			SetDataVirtual(spnData, { ullOffsetToModify, ullSizeToModify });
		}
	}
	break;
	case MODIFY_RAND_MT19937:
	case MODIFY_RAND_FAST:
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<std::uint64_t> distUInt64(0, (std::numeric_limits<std::uint64_t>::max)());
		const auto lmbRandUInt64 = [&](std::byte* pData, const HEXMODIFY& /**/, SpanCByte /**/) {
			assert(pData != nullptr);
			*reinterpret_cast<std::uint64_t*>(pData) = distUInt64(gen);
			};
		const auto lmbRandByte = [&](std::byte* pData, const HEXMODIFY& /**/, SpanCByte /**/) {
			assert(pData != nullptr);
			*pData = static_cast<std::byte>(distUInt64(gen));
			};
		const auto lmbRandFast = [](std::byte* pData, const HEXMODIFY& /**/, SpanCByte spnDataFrom) {
			assert(pData != nullptr);
			std::copy_n(spnDataFrom.data(), spnDataFrom.size(), pData);
			};

		const auto& refHexSpan = hms.vecSpan.back();
		if (hms.eModifyMode == MODIFY_RAND_MT19937 && hms.vecSpan.size() == 1 && refHexSpan.ullSize >= sizeof(std::uint64_t)) {
			ModifyWorker(hms, lmbRandUInt64, { static_cast<std::byte*>(nullptr), sizeof(std::uint64_t) });

			if (const auto dwRem = refHexSpan.ullSize % sizeof(std::uint64_t); dwRem > 0) { //Remainder.
				const auto ullOffset = refHexSpan.ullOffset + refHexSpan.ullSize - dwRem;
				const auto spnData = GetData({ ullOffset, dwRem });
				for (std::size_t iterRem = 0; iterRem < dwRem; ++iterRem) {
					spnData.data()[iterRem] = static_cast<std::byte>(distUInt64(gen));
				}
				SetDataVirtual(spnData, { ullOffset, dwRem });
			}
		}
		else if (hms.eModifyMode == MODIFY_RAND_FAST && hms.vecSpan.size() == 1 && refHexSpan.ullSize >= GetCacheSize()) {
			//Fill the uptrRandData buffer with true random data of ulSizeRandBuff size.
			//Then clone this buffer to the destination data.
			//Buffer is allocated with alignment for maximum performance.
			constexpr auto ulSizeRandBuff { 1024U * 1024U }; //1MB.
			const std::unique_ptr < std::byte[], decltype([](std::byte* pData) { _aligned_free(pData); }) >
				uptrRandData(static_cast<std::byte*>(_aligned_malloc(ulSizeRandBuff, 32)));
			for (auto iter = 0UL; iter < ulSizeRandBuff; iter += sizeof(std::uint64_t)) {
				*reinterpret_cast<std::uint64_t*>(&uptrRandData[iter]) = distUInt64(gen);
			};

			ModifyWorker(hms, lmbRandFast, { uptrRandData.get(), ulSizeRandBuff });

			//Filling the remainder data.
			if (const auto ullRem = refHexSpan.ullSize % ulSizeRandBuff; ullRem > 0) { //Remainder.
				if (ullRem <= GetCacheSize()) {
					const auto ullOffsetCurr = refHexSpan.ullOffset + refHexSpan.ullSize - ullRem;
					const auto spnData = GetData({ ullOffsetCurr, ullRem });
					assert(!spnData.empty());
					std::copy_n(uptrRandData.get(), ullRem, spnData.data());
					SetDataVirtual(spnData, { ullOffsetCurr, ullRem });
				}
				else {
					const auto ullSizeCache = GetCacheSize();
					const auto dwModCache = ullRem % ullSizeCache;
					auto ullChunks = ullRem / ullSizeCache + (dwModCache > 0 ? 1 : 0);
					auto ullOffsetCurr = refHexSpan.ullOffset + refHexSpan.ullSize - ullRem;
					auto ullOffsetRandCurr = 0ULL;
					while (ullChunks-- > 0) {
						const auto ullSizeToModify = (ullChunks == 1 && dwModCache > 0) ? dwModCache : ullSizeCache;
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
		constexpr auto lmbRepeat = [](std::byte* pData, const HEXMODIFY& /**/, SpanCByte spnDataFrom) {
			assert(pData != nullptr);
			std::copy_n(spnDataFrom.data(), spnDataFrom.size(), pData);
			};

		//In cases where only one affected data region (hms.vecSpan.size()==1) is used,
		//and the size of the repeated data is equal to the extent of 2, we extend that 
		//repeated data to the ulSizeBuffFastFill size, to speed up the whole process of repeating.
		//At the end we simply fill up the remainder (ullSizeToModify % ulSizeBuffFastFill).
		constexpr auto ulSizeBuffFastFill { 256U };
		const auto ullOffsetToModify = hms.vecSpan.back().ullOffset;
		const auto ullSizeToModify = hms.vecSpan.back().ullSize;
		const auto ullSizeToFillWith = hms.spnData.size();

		if (hms.vecSpan.size() == 1 && ullSizeToModify > ulSizeBuffFastFill
			&& ullSizeToFillWith < ulSizeBuffFastFill && (ulSizeBuffFastFill % ullSizeToFillWith) == 0) {
			alignas(32) std::byte buffFillData[ulSizeBuffFastFill]; //Buffer for fast data fill.
			for (auto iter = 0ULL; iter < ulSizeBuffFastFill; iter += ullSizeToFillWith) { //Fill the buffer.
				std::copy_n(hms.spnData.data(), ullSizeToFillWith, buffFillData + iter);
			}
			ModifyWorker(hms, lmbRepeat, { buffFillData, ulSizeBuffFastFill }); //Worker with the big fast buffer.

			if (const auto ullRem = ullSizeToModify % ulSizeBuffFastFill; ullRem >= ullSizeToFillWith) { //Remainder.
				const auto ullOffset = ullOffsetToModify + ullSizeToModify - ullRem;
				const auto spnData = GetData({ ullOffset, ullRem });
				for (std::size_t iterRem = 0; iterRem < (ullRem / ullSizeToFillWith); ++iterRem) { //Works only if ullRem >= ullSizeToFillWith.
					std::copy_n(hms.spnData.data(), ullSizeToFillWith, spnData.data() + (iterRem * ullSizeToFillWith));
				}
				SetDataVirtual(spnData, { ullOffset, ullRem - (ullRem % ullSizeToFillWith) });
			}
		}
		else {
			ModifyWorker(hms, lmbRepeat, hms.spnData);
		}
	}
	break;
	case MODIFY_OPERATION:
	{
		using enum EHexDataType;
		using enum EHexOperMode;
		//Special case for the OPER_ASSIGN operation.
		//It can easily be replaced with the MODIFY_REPEAT mode, which is significantly faster.
		//Additionally, ensuring that the spnData.size() (operand size) is equal eDataType size.
		if (hms.eOperMode == OPER_ASSIGN) {
			HEXMODIFY hmsRepeat = hms;
			hmsRepeat.eModifyMode = MODIFY_REPEAT;
			switch (hms.eDataType) {
			case DATA_INT8:
			case DATA_UINT8:
				hmsRepeat.spnData = { hms.spnData.data(), sizeof(std::int8_t) };
				break;
			case DATA_INT16:
			case DATA_UINT16:
				hmsRepeat.spnData = { hms.spnData.data(), sizeof(std::int16_t) };
				break;
			case DATA_INT32:
			case DATA_UINT32:
			case DATA_FLOAT:
				hmsRepeat.spnData = { hms.spnData.data(), sizeof(std::int32_t) };
				break;
			case DATA_INT64:
			case DATA_UINT64:
			case DATA_DOUBLE:
				hmsRepeat.spnData = { hms.spnData.data(), sizeof(std::int64_t) };
				break;
			default:
				break;
			};

			return ModifyData(hmsRepeat);
		}

		constexpr auto lmbOperDefault = [](std::byte* pData, const HEXMODIFY& hms, [[maybe_unused]] SpanCByte) {
			assert(pData != nullptr);

			constexpr auto lmbOperT = []<typename T>(T * pData, const HEXMODIFY & hms) {
				T tData = hms.fBigEndian ? ByteSwap(*pData) : *pData;
				assert(!hms.spnData.empty());
				const T tOper = *reinterpret_cast<const T*>(hms.spnData.data());

				if constexpr (std::is_integral_v<T>) { //Operations only for integral types.
					switch (hms.eOperMode) {
					case OPER_OR:
						tData |= tOper;
						break;
					case OPER_XOR:
						tData ^= tOper;
						break;
					case OPER_AND:
						tData &= tOper;
						break;
					case OPER_NOT:
						tData = ~tData;
						break;
					case OPER_SHL:
						tData <<= tOper;
						break;
					case OPER_SHR:
						tData >>= tOper;
						break;
					case OPER_ROTL:
						tData = std::rotl(static_cast<std::make_unsigned_t<T>>(tData), static_cast<const int>(tOper));
						break;
					case OPER_ROTR:
						tData = std::rotr(static_cast<std::make_unsigned_t<T>>(tData), static_cast<const int>(tOper));
						break;
					case OPER_BITREV:
						tData = BitReverse(tData);
						break;
					default:
						break;
					}
				}

				switch (hms.eOperMode) { //Operations for integral and floating types.
				case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
					break;
				case OPER_ADD:
					tData += tOper;
					break;
				case OPER_SUB:
					tData -= tOper;
					break;
				case OPER_MUL:
					tData *= tOper;
					break;
				case OPER_DIV:
					assert(tOper > 0);
					tData /= tOper;
					break;
				case OPER_CEIL:
					tData = (std::min)(tData, tOper);
					break;
				case OPER_FLOOR:
					tData = (std::max)(tData, tOper);
					break;
				case OPER_SWAP:
					tData = ByteSwap(tData);
					break;
				default:
					break;
				}

				if (hms.fBigEndian) { //Swap bytes back.
					tData = ByteSwap(tData);
				}

				*pData = tData;
			};

			switch (hms.eDataType) {
			case DATA_INT8:
				lmbOperT(reinterpret_cast<std::int8_t*>(pData), hms);
				break;
			case DATA_UINT8:
				lmbOperT(reinterpret_cast<std::uint8_t*>(pData), hms);
				break;
			case DATA_INT16:
				lmbOperT(reinterpret_cast<std::int16_t*>(pData), hms);
				break;
			case DATA_UINT16:
				lmbOperT(reinterpret_cast<std::uint16_t*>(pData), hms);
				break;
			case DATA_INT32:
				lmbOperT(reinterpret_cast<std::int32_t*>(pData), hms);
				break;
			case DATA_UINT32:
				lmbOperT(reinterpret_cast<std::uint32_t*>(pData), hms);
				break;
			case DATA_INT64:
				lmbOperT(reinterpret_cast<std::int64_t*>(pData), hms);
				break;
			case DATA_UINT64:
				lmbOperT(reinterpret_cast<std::uint64_t*>(pData), hms);
				break;
			case DATA_FLOAT:
				lmbOperT(reinterpret_cast<float*>(pData), hms);
				break;
			case DATA_DOUBLE:
				lmbOperT(reinterpret_cast<double*>(pData), hms);
				break;
			default:
				break;
			}
			};

		constexpr auto lmbOperSIMD = [](std::byte* pData, const HEXMODIFY& hms, [[maybe_unused]] SpanCByte) {
			assert(pData != nullptr);

			constexpr auto lmbOperSIMDInt8 = [](std::int8_t* pi8Data, const HEXMODIFY & hms) {
				const auto m128iData = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pi8Data));
				alignas(16) std::int8_t i8Data[16];
				_mm_store_si128(reinterpret_cast<__m128i*>(i8Data), m128iData);
				assert(!hms.spnData.empty());
				const auto i8Oper = *reinterpret_cast<const std::int8_t*>(hms.spnData.data());
				const auto m128iOper = _mm_set1_epi8(i8Oper);
				__m128i m128iResult { };

				switch (hms.eOperMode) {
				case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
					break;
				case OPER_ADD:
					m128iResult = _mm_add_epi8(m128iData, m128iOper);
					break;
				case OPER_SUB:
					m128iResult = _mm_sub_epi8(m128iData, m128iOper);
					break;
				case OPER_MUL:
				{
					const auto m128iEven = _mm_mullo_epi16(m128iData, m128iOper);
					const auto m128iOdd = _mm_mullo_epi16(_mm_srli_epi16(m128iData, 8), _mm_srli_epi16(m128iOper, 8));
					m128iResult = _mm_or_si128(_mm_slli_epi16(m128iOdd, 8), _mm_srli_epi16(_mm_slli_epi16(m128iEven, 8), 8));
				}
				break;
				case OPER_DIV:
					assert(i8Oper > 0);
					m128iResult = _mm_setr_epi8(i8Data[0] / i8Oper, i8Data[1] / i8Oper, i8Data[2] / i8Oper, i8Data[3] / i8Oper,
						i8Data[4] / i8Oper, i8Data[5] / i8Oper, i8Data[6] / i8Oper, i8Data[7] / i8Oper,
						i8Data[8] / i8Oper, i8Data[9] / i8Oper, i8Data[10] / i8Oper, i8Data[11] / i8Oper,
						i8Data[12] / i8Oper, i8Data[13] / i8Oper, i8Data[14] / i8Oper, i8Data[15] / i8Oper);
					break;
				case OPER_CEIL:
					m128iResult = _mm_min_epi8(m128iData, m128iOper); //SSE4.1
					break;
				case OPER_FLOOR:
					m128iResult = _mm_max_epi8(m128iData, m128iOper); //SSE4.1
					break;
				case OPER_SWAP: //No need for the int8_t.
					break;
				case OPER_OR:
					m128iResult = _mm_or_si128(m128iData, m128iOper);
					break;
				case OPER_XOR:
					m128iResult = _mm_xor_si128(m128iData, m128iOper);
					break;
				case OPER_AND:
					m128iResult = _mm_and_si128(m128iData, m128iOper);
					break;
				case OPER_NOT:
					//_mm_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
					m128iResult = _mm_xor_si128(m128iData, _mm_cmpeq_epi64(m128iData, m128iData)); //SSE4.1
					break;
				case OPER_SHL:
					m128iResult = _mm_setr_epi8(i8Data[0] << i8Oper, i8Data[1] << i8Oper, i8Data[2] << i8Oper, i8Data[3] << i8Oper,
						i8Data[4] << i8Oper, i8Data[5] << i8Oper, i8Data[6] << i8Oper, i8Data[7] << i8Oper,
						i8Data[8] << i8Oper, i8Data[9] << i8Oper, i8Data[10] << i8Oper, i8Data[11] << i8Oper,
						i8Data[12] << i8Oper, i8Data[13] << i8Oper, i8Data[14] << i8Oper, i8Data[15] << i8Oper);
					break;
				case OPER_SHR:
					m128iResult = _mm_setr_epi8(i8Data[0] >> i8Oper, i8Data[1] >> i8Oper, i8Data[2] >> i8Oper, i8Data[3] >> i8Oper,
						i8Data[4] >> i8Oper, i8Data[5] >> i8Oper, i8Data[6] >> i8Oper, i8Data[7] >> i8Oper,
						i8Data[8] >> i8Oper, i8Data[9] >> i8Oper, i8Data[10] >> i8Oper, i8Data[11] >> i8Oper,
						i8Data[12] >> i8Oper, i8Data[13] >> i8Oper, i8Data[14] >> i8Oper, i8Data[15] >> i8Oper);
					break;
				case OPER_ROTL:
					m128iResult = _mm_setr_epi8(std::rotl(static_cast<std::uint8_t>(i8Data[0]), i8Oper),
						std::rotl(static_cast<std::uint8_t>(i8Data[1]), i8Oper),
						std::rotl(static_cast<std::uint8_t>(i8Data[2]), i8Oper),
						std::rotl(static_cast<std::uint8_t>(i8Data[3]), i8Oper),
						std::rotl(static_cast<std::uint8_t>(i8Data[4]), i8Oper),
						std::rotl(static_cast<std::uint8_t>(i8Data[5]), i8Oper),
						std::rotl(static_cast<std::uint8_t>(i8Data[6]), i8Oper),
						std::rotl(static_cast<std::uint8_t>(i8Data[7]), i8Oper),
						std::rotl(static_cast<std::uint8_t>(i8Data[8]), i8Oper),
						std::rotl(static_cast<std::uint8_t>(i8Data[9]), i8Oper),
						std::rotl(static_cast<std::uint8_t>(i8Data[10]), i8Oper),
						std::rotl(static_cast<std::uint8_t>(i8Data[11]), i8Oper),
						std::rotl(static_cast<std::uint8_t>(i8Data[12]), i8Oper),
						std::rotl(static_cast<std::uint8_t>(i8Data[13]), i8Oper),
						std::rotl(static_cast<std::uint8_t>(i8Data[14]), i8Oper),
						std::rotl(static_cast<std::uint8_t>(i8Data[15]), i8Oper));
					break;
				case OPER_ROTR:
					m128iResult = _mm_setr_epi8(std::rotr(static_cast<std::uint8_t>(i8Data[0]), i8Oper),
						std::rotr(static_cast<std::uint8_t>(i8Data[1]), i8Oper),
						std::rotr(static_cast<std::uint8_t>(i8Data[2]), i8Oper),
						std::rotr(static_cast<std::uint8_t>(i8Data[3]), i8Oper),
						std::rotr(static_cast<std::uint8_t>(i8Data[4]), i8Oper),
						std::rotr(static_cast<std::uint8_t>(i8Data[5]), i8Oper),
						std::rotr(static_cast<std::uint8_t>(i8Data[6]), i8Oper),
						std::rotr(static_cast<std::uint8_t>(i8Data[7]), i8Oper),
						std::rotr(static_cast<std::uint8_t>(i8Data[8]), i8Oper),
						std::rotr(static_cast<std::uint8_t>(i8Data[9]), i8Oper),
						std::rotr(static_cast<std::uint8_t>(i8Data[10]), i8Oper),
						std::rotr(static_cast<std::uint8_t>(i8Data[11]), i8Oper),
						std::rotr(static_cast<std::uint8_t>(i8Data[12]), i8Oper),
						std::rotr(static_cast<std::uint8_t>(i8Data[13]), i8Oper),
						std::rotr(static_cast<std::uint8_t>(i8Data[14]), i8Oper),
						std::rotr(static_cast<std::uint8_t>(i8Data[15]), i8Oper));
					break;
				case OPER_BITREV:
					m128iResult = _mm_setr_epi8(BitReverse(i8Data[0]), BitReverse(i8Data[1]), BitReverse(i8Data[2]),
						BitReverse(i8Data[3]), BitReverse(i8Data[4]), BitReverse(i8Data[5]), BitReverse(i8Data[6]),
						BitReverse(i8Data[7]), BitReverse(i8Data[8]), BitReverse(i8Data[9]), BitReverse(i8Data[10]),
						BitReverse(i8Data[11]), BitReverse(i8Data[12]), BitReverse(i8Data[13]), BitReverse(i8Data[14]),
						BitReverse(i8Data[15]));
					break;
				default:
					DBG_REPORT(L"Unsupported int8_t operation.");
					break;
				}

				_mm_storeu_si128(reinterpret_cast<__m128i*>(pi8Data), m128iResult);
				};

			constexpr auto lmbOperSIMDUInt8 = [](std::uint8_t* pui8Data, const HEXMODIFY & hms) {
				const auto m128iData = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pui8Data));
				alignas(16) std::uint8_t ui8Data[16];
				_mm_store_si128(reinterpret_cast<__m128i*>(ui8Data), m128iData);
				assert(!hms.spnData.empty());
				const auto ui8Oper = *reinterpret_cast<const std::uint8_t*>(hms.spnData.data());
				const auto m128iOper = _mm_set1_epi8(ui8Oper);
				__m128i m128iResult { };

				switch (hms.eOperMode) {
				case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
					break;
				case OPER_ADD:
					m128iResult = _mm_add_epi8(m128iData, m128iOper);
					break;
				case OPER_SUB:
					m128iResult = _mm_sub_epi8(m128iData, m128iOper);
					break;
				case OPER_MUL:
					m128iResult = _mm_setr_epi8(ui8Data[0] * ui8Oper, ui8Data[1] * ui8Oper, ui8Data[2] * ui8Oper,
						ui8Data[3] * ui8Oper, ui8Data[4] * ui8Oper, ui8Data[5] * ui8Oper, ui8Data[6] * ui8Oper,
						ui8Data[7] * ui8Oper, ui8Data[8] * ui8Oper, ui8Data[9] * ui8Oper, ui8Data[10] * ui8Oper,
						ui8Data[11] * ui8Oper, ui8Data[12] * ui8Oper, ui8Data[13] * ui8Oper, ui8Data[14] * ui8Oper,
						ui8Data[15] * ui8Oper);
					break;
				case OPER_DIV:
					assert(ui8Oper > 0);
					m128iResult = _mm_setr_epi8(ui8Data[0] / ui8Oper, ui8Data[1] / ui8Oper, ui8Data[2] / ui8Oper,
						ui8Data[3] / ui8Oper, ui8Data[4] / ui8Oper, ui8Data[5] / ui8Oper, ui8Data[6] / ui8Oper,
						ui8Data[7] / ui8Oper, ui8Data[8] / ui8Oper, ui8Data[9] / ui8Oper, ui8Data[10] / ui8Oper,
						ui8Data[11] / ui8Oper, ui8Data[12] / ui8Oper, ui8Data[13] / ui8Oper, ui8Data[14] / ui8Oper,
						ui8Data[15] / ui8Oper);
					break;
				case OPER_CEIL:
					m128iResult = _mm_min_epu8(m128iData, m128iOper); //SSE4.1
					break;
				case OPER_FLOOR:
					m128iResult = _mm_max_epu8(m128iData, m128iOper); //SSE4.1
					break;
				case OPER_SWAP:	//No need for the uint8_t.
					break;
				case OPER_OR:
					m128iResult = _mm_or_si128(m128iData, m128iOper);
					break;
				case OPER_XOR:
					m128iResult = _mm_xor_si128(m128iData, m128iOper);
					break;
				case OPER_AND:
					m128iResult = _mm_and_si128(m128iData, m128iOper);
					break;
				case OPER_NOT:
					//_mm_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
					m128iResult = _mm_xor_si128(m128iData, _mm_cmpeq_epi64(m128iData, m128iData)); //SSE4.1
					break;
				case OPER_SHL:
					m128iResult = _mm_setr_epi8(ui8Data[0] << ui8Oper, ui8Data[1] << ui8Oper, ui8Data[2] << ui8Oper, ui8Data[3] << ui8Oper,
						ui8Data[4] << ui8Oper, ui8Data[5] << ui8Oper, ui8Data[6] << ui8Oper, ui8Data[7] << ui8Oper,
						ui8Data[8] << ui8Oper, ui8Data[9] << ui8Oper, ui8Data[10] << ui8Oper, ui8Data[11] << ui8Oper,
						ui8Data[12] << ui8Oper, ui8Data[13] << ui8Oper, ui8Data[14] << ui8Oper, ui8Data[15] << ui8Oper);
					break;
				case OPER_SHR:
					m128iResult = _mm_setr_epi8(ui8Data[0] >> ui8Oper, ui8Data[1] >> ui8Oper, ui8Data[2] >> ui8Oper, ui8Data[3] >> ui8Oper,
						ui8Data[4] >> ui8Oper, ui8Data[5] >> ui8Oper, ui8Data[6] >> ui8Oper, ui8Data[7] >> ui8Oper,
						ui8Data[8] >> ui8Oper, ui8Data[9] >> ui8Oper, ui8Data[10] >> ui8Oper, ui8Data[11] >> ui8Oper,
						ui8Data[12] >> ui8Oper, ui8Data[13] >> ui8Oper, ui8Data[14] >> ui8Oper, ui8Data[15] >> ui8Oper);
					break;
				case OPER_ROTL:
					m128iResult = _mm_setr_epi8(std::rotl(ui8Data[0], ui8Oper), std::rotl(ui8Data[1], ui8Oper),
						std::rotl(ui8Data[2], ui8Oper), std::rotl(ui8Data[3], ui8Oper), std::rotl(ui8Data[4], ui8Oper),
						std::rotl(ui8Data[5], ui8Oper), std::rotl(ui8Data[6], ui8Oper), std::rotl(ui8Data[7], ui8Oper),
						std::rotl(ui8Data[8], ui8Oper), std::rotl(ui8Data[9], ui8Oper), std::rotl(ui8Data[10], ui8Oper),
						std::rotl(ui8Data[11], ui8Oper), std::rotl(ui8Data[12], ui8Oper), std::rotl(ui8Data[13], ui8Oper),
						std::rotl(ui8Data[14], ui8Oper), std::rotl(ui8Data[15], ui8Oper));
					break;
				case OPER_ROTR:
					m128iResult = _mm_setr_epi8(std::rotr(ui8Data[0], ui8Oper), std::rotr(ui8Data[1], ui8Oper),
						std::rotr(ui8Data[2], ui8Oper), std::rotr(ui8Data[3], ui8Oper), std::rotr(ui8Data[4], ui8Oper),
						std::rotr(ui8Data[5], ui8Oper), std::rotr(ui8Data[6], ui8Oper), std::rotr(ui8Data[7], ui8Oper),
						std::rotr(ui8Data[8], ui8Oper), std::rotr(ui8Data[9], ui8Oper), std::rotr(ui8Data[10], ui8Oper),
						std::rotr(ui8Data[11], ui8Oper), std::rotr(ui8Data[12], ui8Oper), std::rotr(ui8Data[13], ui8Oper),
						std::rotr(ui8Data[14], ui8Oper), std::rotr(ui8Data[15], ui8Oper));
					break;
				case OPER_BITREV:
					m128iResult = _mm_setr_epi8(BitReverse(ui8Data[0]), BitReverse(ui8Data[1]), BitReverse(ui8Data[2]),
						BitReverse(ui8Data[3]), BitReverse(ui8Data[4]), BitReverse(ui8Data[5]), BitReverse(ui8Data[6]),
						BitReverse(ui8Data[7]), BitReverse(ui8Data[8]), BitReverse(ui8Data[9]), BitReverse(ui8Data[10]),
						BitReverse(ui8Data[11]), BitReverse(ui8Data[12]), BitReverse(ui8Data[13]), BitReverse(ui8Data[14]),
						BitReverse(ui8Data[15]));
					break;
				default:
					DBG_REPORT(L"Unsupported uint8_t operation.");
					break;
				}

				_mm_storeu_si128(reinterpret_cast<__m128i*>(pui8Data), m128iResult);
				};

			constexpr auto lmbOperSIMDInt16 = [](std::int16_t* pi16Data, const HEXMODIFY & hms) {
				const auto m128iData = hms.fBigEndian ?
					ByteSwapSIMD<std::int16_t>(_mm_loadu_si128(reinterpret_cast<const __m128i*>(pi16Data)))
					: _mm_loadu_si128(reinterpret_cast<const __m128i*>(pi16Data));
				alignas(16) std::int16_t i16Data[8];
				_mm_store_si128(reinterpret_cast<__m128i*>(i16Data), m128iData);
				assert(!hms.spnData.empty());
				const auto i16Oper = *reinterpret_cast<const std::int16_t*>(hms.spnData.data());
				const auto m128iOper = _mm_set1_epi16(i16Oper);
				__m128i m128iResult { };

				switch (hms.eOperMode) {
				case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
					break;
				case OPER_ADD:
					m128iResult = _mm_add_epi16(m128iData, m128iOper);
					break;
				case OPER_SUB:
					m128iResult = _mm_sub_epi16(m128iData, m128iOper);
					break;
				case OPER_MUL:
					m128iResult = _mm_mullo_epi16(m128iData, m128iOper);
					break;
				case OPER_DIV:
					assert(i16Oper > 0);
					m128iResult = _mm_setr_epi16(i16Data[0] / i16Oper, i16Data[1] / i16Oper, i16Data[2] / i16Oper,
						i16Data[3] / i16Oper, i16Data[4] / i16Oper, i16Data[5] / i16Oper, i16Data[6] / i16Oper, i16Data[7] / i16Oper);
					break;
				case OPER_CEIL:
					m128iResult = _mm_min_epi16(m128iData, m128iOper);
					break;
				case OPER_FLOOR:
					m128iResult = _mm_max_epi16(m128iData, m128iOper);
					break;
				case OPER_SWAP:
					m128iResult = ByteSwapSIMD<std::int16_t>(m128iData);
					break;
				case OPER_OR:
					m128iResult = _mm_or_si128(m128iData, m128iOper);
					break;
				case OPER_XOR:
					m128iResult = _mm_xor_si128(m128iData, m128iOper);
					break;
				case OPER_AND:
					m128iResult = _mm_and_si128(m128iData, m128iOper);
					break;
				case OPER_NOT:
					//_mm_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
					m128iResult = _mm_xor_si128(m128iData, _mm_cmpeq_epi64(m128iData, m128iData)); //SSE4.1
					break;
				case OPER_SHL:
					m128iResult = _mm_setr_epi16(i16Data[0] << i16Oper, i16Data[1] << i16Oper, i16Data[2] << i16Oper,
						i16Data[3] << i16Oper, i16Data[4] << i16Oper, i16Data[5] << i16Oper, i16Data[6] << i16Oper,
						i16Data[7] << i16Oper);
					break;
				case OPER_SHR:
					m128iResult = _mm_setr_epi16(i16Data[0] >> i16Oper, i16Data[1] >> i16Oper, i16Data[2] >> i16Oper,
						i16Data[3] >> i16Oper, i16Data[4] >> i16Oper, i16Data[5] >> i16Oper, i16Data[6] >> i16Oper,
						i16Data[7] >> i16Oper);
					break;
				case OPER_ROTL:
					m128iResult = _mm_setr_epi16(std::rotl(static_cast<std::uint16_t>(i16Data[0]), i16Oper),
						std::rotl(static_cast<std::uint16_t>(i16Data[1]), i16Oper),
						std::rotl(static_cast<std::uint16_t>(i16Data[2]), i16Oper),
						std::rotl(static_cast<std::uint16_t>(i16Data[3]), i16Oper),
						std::rotl(static_cast<std::uint16_t>(i16Data[4]), i16Oper),
						std::rotl(static_cast<std::uint16_t>(i16Data[5]), i16Oper),
						std::rotl(static_cast<std::uint16_t>(i16Data[6]), i16Oper),
						std::rotl(static_cast<std::uint16_t>(i16Data[7]), i16Oper));
					break;
				case OPER_ROTR:
					m128iResult = _mm_setr_epi16(std::rotr(static_cast<std::uint16_t>(i16Data[0]), i16Oper),
						std::rotr(static_cast<std::uint16_t>(i16Data[1]), i16Oper),
						std::rotr(static_cast<std::uint16_t>(i16Data[2]), i16Oper),
						std::rotr(static_cast<std::uint16_t>(i16Data[3]), i16Oper),
						std::rotr(static_cast<std::uint16_t>(i16Data[4]), i16Oper),
						std::rotr(static_cast<std::uint16_t>(i16Data[5]), i16Oper),
						std::rotr(static_cast<std::uint16_t>(i16Data[6]), i16Oper),
						std::rotr(static_cast<std::uint16_t>(i16Data[7]), i16Oper));
					break;
				case OPER_BITREV:
					m128iResult = _mm_setr_epi16(BitReverse(i16Data[0]), BitReverse(i16Data[1]), BitReverse(i16Data[2]),
						BitReverse(i16Data[3]), BitReverse(i16Data[4]), BitReverse(i16Data[5]), BitReverse(i16Data[6]),
						BitReverse(i16Data[7]));
					break;
				default:
					DBG_REPORT(L"Unsupported int16_t operation.");
					break;
				}

				if (hms.fBigEndian) { //Swap bytes back.
					m128iResult = ByteSwapSIMD<std::int16_t>(m128iResult);
				}

				_mm_storeu_si128(reinterpret_cast<__m128i*>(pi16Data), m128iResult);
				};

			constexpr auto lmbOperSIMDUInt16 = [](std::uint16_t* pui16Data, const HEXMODIFY & hms) {
				const auto m128iData = hms.fBigEndian ?
					ByteSwapSIMD<std::uint16_t>(_mm_loadu_si128(reinterpret_cast<const __m128i*>(pui16Data)))
					: _mm_loadu_si128(reinterpret_cast<const __m128i*>(pui16Data));
				alignas(16) std::uint16_t ui16Data[8];
				_mm_store_si128(reinterpret_cast<__m128i*>(ui16Data), m128iData);
				assert(!hms.spnData.empty());
				const auto ui16Oper = *reinterpret_cast<const std::uint16_t*>(hms.spnData.data());
				const auto m128iOper = _mm_set1_epi16(ui16Oper);
				__m128i m128iResult { };

				switch (hms.eOperMode) {
				case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
					break;
				case OPER_ADD:
					m128iResult = _mm_add_epi16(m128iData, m128iOper);
					break;
				case OPER_SUB:
					m128iResult = _mm_sub_epi16(m128iData, m128iOper);
					break;
				case OPER_MUL:
					m128iResult = _mm_setr_epi16(ui16Data[0] * ui16Oper, ui16Data[1] * ui16Oper, ui16Data[2] * ui16Oper,
						ui16Data[3] * ui16Oper, ui16Data[4] * ui16Oper, ui16Data[5] * ui16Oper, ui16Data[6] * ui16Oper,
						ui16Data[7] * ui16Oper);
					break;
				case OPER_DIV:
					assert(ui16Oper > 0);
					m128iResult = _mm_setr_epi16(ui16Data[0] / ui16Oper, ui16Data[1] / ui16Oper, ui16Data[2] / ui16Oper,
						ui16Data[3] / ui16Oper, ui16Data[4] / ui16Oper, ui16Data[5] / ui16Oper, ui16Data[6] / ui16Oper,
						ui16Data[7] / ui16Oper);
					break;
				case OPER_CEIL:
					m128iResult = _mm_min_epu16(m128iData, m128iOper);
					break;
				case OPER_FLOOR:
					m128iResult = _mm_max_epu16(m128iData, m128iOper);
					break;
				case OPER_SWAP:
					m128iResult = ByteSwapSIMD<std::uint16_t>(m128iData);
					break;
				case OPER_OR:
					m128iResult = _mm_or_si128(m128iData, m128iOper);
					break;
				case OPER_XOR:
					m128iResult = _mm_xor_si128(m128iData, m128iOper);
					break;
				case OPER_AND:
					m128iResult = _mm_and_si128(m128iData, m128iOper);
					break;
				case OPER_NOT:
					//_mm_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
					m128iResult = _mm_xor_si128(m128iData, _mm_cmpeq_epi64(m128iData, m128iData)); //SSE4.1
					break;
				case OPER_SHL:
					m128iResult = _mm_setr_epi16(ui16Data[0] << ui16Oper, ui16Data[1] << ui16Oper, ui16Data[2] << ui16Oper,
						ui16Data[3] << ui16Oper, ui16Data[4] << ui16Oper, ui16Data[5] << ui16Oper, ui16Data[6] << ui16Oper,
						ui16Data[7] << ui16Oper);
					break;
				case OPER_SHR:
					m128iResult = _mm_setr_epi16(ui16Data[0] >> ui16Oper, ui16Data[1] >> ui16Oper, ui16Data[2] >> ui16Oper,
						ui16Data[3] >> ui16Oper, ui16Data[4] >> ui16Oper, ui16Data[5] >> ui16Oper, ui16Data[6] >> ui16Oper,
						ui16Data[7] >> ui16Oper);
					break;
				case OPER_ROTL:
					m128iResult = _mm_setr_epi16(std::rotl(ui16Data[0], ui16Oper), std::rotl(ui16Data[1], ui16Oper),
						std::rotl(ui16Data[2], ui16Oper), std::rotl(ui16Data[3], ui16Oper), std::rotl(ui16Data[4], ui16Oper),
						std::rotl(ui16Data[5], ui16Oper), std::rotl(ui16Data[6], ui16Oper), std::rotl(ui16Data[7], ui16Oper));
					break;
				case OPER_ROTR:
					m128iResult = _mm_setr_epi16(std::rotr(ui16Data[0], ui16Oper), std::rotr(ui16Data[1], ui16Oper),
						std::rotr(ui16Data[2], ui16Oper), std::rotr(ui16Data[3], ui16Oper), std::rotr(ui16Data[4], ui16Oper),
						std::rotr(ui16Data[5], ui16Oper), std::rotr(ui16Data[6], ui16Oper), std::rotr(ui16Data[7], ui16Oper));
					break;
				case OPER_BITREV:
					m128iResult = _mm_setr_epi16(BitReverse(ui16Data[0]), BitReverse(ui16Data[1]), BitReverse(ui16Data[2]),
						BitReverse(ui16Data[3]), BitReverse(ui16Data[4]), BitReverse(ui16Data[5]), BitReverse(ui16Data[6]),
						BitReverse(ui16Data[7]));
					break;
				default:
					DBG_REPORT(L"Unsupported uint16_t operation.");
					break;
				}

				if (hms.fBigEndian) { //Swap bytes back.
					m128iResult = ByteSwapSIMD<std::uint16_t>(m128iResult);
				}

				_mm_storeu_si128(reinterpret_cast<__m128i*>(pui16Data), m128iResult);
				};

			constexpr auto lmbOperSIMDInt32 = [](std::int32_t* pi32Data, const HEXMODIFY & hms) {
				const auto m128iData = hms.fBigEndian ?
					ByteSwapSIMD<std::int32_t>(_mm_loadu_si128(reinterpret_cast<const __m128i*>(pi32Data)))
					: _mm_loadu_si128(reinterpret_cast<const __m128i*>(pi32Data));
				alignas(16) std::int32_t i32Data[4];
				_mm_store_si128(reinterpret_cast<__m128i*>(i32Data), m128iData);
				assert(!hms.spnData.empty());
				const auto i32Oper = *reinterpret_cast<const std::int32_t*>(hms.spnData.data());
				const auto m128iOper = _mm_set1_epi32(i32Oper);
				__m128i m128iResult { };

				switch (hms.eOperMode) {
				case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
					break;
				case OPER_ADD:
					m128iResult = _mm_add_epi32(m128iData, m128iOper);
					break;
				case OPER_SUB:
					m128iResult = _mm_sub_epi32(m128iData, m128iOper);
					break;
				case OPER_MUL:
					m128iResult = _mm_mullo_epi32(m128iData, m128iOper);
					break;
				case OPER_DIV:
					assert(i32Oper > 0);
					m128iResult = _mm_setr_epi32(i32Data[0] / i32Oper, i32Data[1] / i32Oper, i32Data[2] / i32Oper,
						i32Data[3] / i32Oper);
					break;
				case OPER_CEIL:
					m128iResult = _mm_min_epi32(m128iData, m128iOper);
					break;
				case OPER_FLOOR:
					m128iResult = _mm_max_epi32(m128iData, m128iOper);
					break;
				case OPER_SWAP:
					m128iResult = ByteSwapSIMD<std::int32_t>(m128iData);
					break;
				case OPER_OR:
					m128iResult = _mm_or_si128(m128iData, m128iOper);
					break;
				case OPER_XOR:
					m128iResult = _mm_xor_si128(m128iData, m128iOper);
					break;
				case OPER_AND:
					m128iResult = _mm_and_si128(m128iData, m128iOper);
					break;
				case OPER_NOT:
					//_mm_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
					m128iResult = _mm_xor_si128(m128iData, _mm_cmpeq_epi64(m128iData, m128iData)); //SSE4.1
					break;
				case OPER_SHL:
					m128iResult = _mm_setr_epi32(i32Data[0] << i32Oper, i32Data[1] << i32Oper, i32Data[2] << i32Oper,
						i32Data[3] << i32Oper);
					break;
				case OPER_SHR:
					m128iResult = _mm_setr_epi32(i32Data[0] >> i32Oper, i32Data[1] >> i32Oper, i32Data[2] >> i32Oper,
						i32Data[3] >> i32Oper);
					break;
				case OPER_ROTL:
					m128iResult = _mm_setr_epi32(std::rotl(static_cast<std::uint32_t>(i32Data[0]), i32Oper),
						std::rotl(static_cast<std::uint32_t>(i32Data[1]), i32Oper),
						std::rotl(static_cast<std::uint32_t>(i32Data[2]), i32Oper),
						std::rotl(static_cast<std::uint32_t>(i32Data[3]), i32Oper));
					break;
				case OPER_ROTR:
					m128iResult = _mm_setr_epi32(std::rotr(static_cast<std::uint32_t>(i32Data[0]), i32Oper),
						std::rotr(static_cast<std::uint32_t>(i32Data[1]), i32Oper),
						std::rotr(static_cast<std::uint32_t>(i32Data[2]), i32Oper),
						std::rotr(static_cast<std::uint32_t>(i32Data[3]), i32Oper));
					break;
				case OPER_BITREV:
					m128iResult = _mm_setr_epi32(BitReverse(i32Data[0]), BitReverse(i32Data[1]), BitReverse(i32Data[2]),
						BitReverse(i32Data[3]));
					break;
				default:
					DBG_REPORT(L"Unsupported int32_t operation.");
					break;
				}

				if (hms.fBigEndian) { //Swap bytes back.
					m128iResult = ByteSwapSIMD<std::int32_t>(m128iResult);
				}

				_mm_storeu_si128(reinterpret_cast<__m128i*>(pi32Data), m128iResult);
				};

			constexpr auto lmbOperSIMDUInt32 = [](std::uint32_t* pui32Data, const HEXMODIFY & hms) {
				const auto m128iData = hms.fBigEndian ?
					ByteSwapSIMD<std::uint32_t>(_mm_loadu_si128(reinterpret_cast<const __m128i*>(pui32Data)))
					: _mm_loadu_si128(reinterpret_cast<const __m128i*>(pui32Data));
				alignas(16) std::uint32_t ui32Data[4];
				_mm_store_si128(reinterpret_cast<__m128i*>(ui32Data), m128iData);
				assert(!hms.spnData.empty());
				const auto ui32Oper = *reinterpret_cast<const std::uint32_t*>(hms.spnData.data());
				const auto m128iOper = _mm_set1_epi32(ui32Oper);
				__m128i m128iResult { };

				switch (hms.eOperMode) {
				case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
					break;
				case OPER_ADD:
					m128iResult = _mm_add_epi32(m128iData, m128iOper);
					break;
				case OPER_SUB:
					m128iResult = _mm_sub_epi32(m128iData, m128iOper);
					break;
				case OPER_MUL:
					m128iResult = _mm_setr_epi32(ui32Data[0] * ui32Oper, ui32Data[1] * ui32Oper, ui32Data[2] * ui32Oper,
						ui32Data[3] * ui32Oper);
					break;
				case OPER_DIV:
					assert(ui32Oper > 0);
					m128iResult = _mm_setr_epi32(ui32Data[0] / ui32Oper, ui32Data[1] / ui32Oper, ui32Data[2] / ui32Oper,
						ui32Data[3] / ui32Oper);
					break;
				case OPER_CEIL:
					m128iResult = _mm_min_epu32(m128iData, m128iOper); //SSE4.1
					break;
				case OPER_FLOOR:
					m128iResult = _mm_max_epu32(m128iData, m128iOper); //SSE4.1
					break;
				case OPER_SWAP:
					m128iResult = ByteSwapSIMD<std::uint32_t>(m128iData);
					break;
				case OPER_OR:
					m128iResult = _mm_or_si128(m128iData, m128iOper);
					break;
				case OPER_XOR:
					m128iResult = _mm_xor_si128(m128iData, m128iOper);
					break;
				case OPER_AND:
					m128iResult = _mm_and_si128(m128iData, m128iOper);
					break;
				case OPER_NOT:
					//_mm_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
					m128iResult = _mm_xor_si128(m128iData, _mm_cmpeq_epi64(m128iData, m128iData)); //SSE4.1
					break;
				case OPER_SHL:
					m128iResult = _mm_setr_epi32(ui32Data[0] << ui32Oper, ui32Data[1] << ui32Oper, ui32Data[2] << ui32Oper,
						ui32Data[3] << ui32Oper);
					break;
				case OPER_SHR:
					m128iResult = _mm_setr_epi32(ui32Data[0] >> ui32Oper, ui32Data[1] >> ui32Oper, ui32Data[2] >> ui32Oper,
						ui32Data[3] >> ui32Oper);
					break;
				case OPER_ROTL:
					m128iResult = _mm_setr_epi32(std::rotl(ui32Data[0], ui32Oper), std::rotl(ui32Data[1], ui32Oper),
						std::rotl(ui32Data[2], ui32Oper), std::rotl(ui32Data[3], ui32Oper));
					break;
				case OPER_ROTR:
					m128iResult = _mm_setr_epi32(std::rotr(ui32Data[0], ui32Oper), std::rotr(ui32Data[1], ui32Oper),
						std::rotr(ui32Data[2], ui32Oper), std::rotr(ui32Data[3], ui32Oper));
					break;
				case OPER_BITREV:
					m128iResult = _mm_setr_epi32(BitReverse(ui32Data[0]), BitReverse(ui32Data[1]), BitReverse(ui32Data[2]),
						BitReverse(ui32Data[3]));
					break;
				default:
					DBG_REPORT(L"Unsupported uint32_t operation.");
					break;
				}

				if (hms.fBigEndian) { //Swap bytes back.
					m128iResult = ByteSwapSIMD<std::uint32_t>(m128iResult);
				}

				_mm_storeu_si128(reinterpret_cast<__m128i*>(pui32Data), m128iResult);
				};

			constexpr auto lmbOperSIMDInt64 = [](std::int64_t* pi64Data, const HEXMODIFY & hms) {
				const auto m128iData = hms.fBigEndian ?
					ByteSwapSIMD<std::int64_t>(_mm_loadu_si128(reinterpret_cast<const __m128i*>(pi64Data)))
					: _mm_loadu_si128(reinterpret_cast<const __m128i*>(pi64Data));
				alignas(16) std::int64_t i64Data[2];
				_mm_store_si128(reinterpret_cast<__m128i*>(i64Data), m128iData);
				assert(!hms.spnData.empty());
				const auto i64Oper = *reinterpret_cast<const std::int64_t*>(hms.spnData.data());
				const auto m128iOper = _mm_set1_epi64x(i64Oper);
				__m128i m128iResult { };

				switch (hms.eOperMode) {
				case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
					break;
				case OPER_ADD:
					m128iResult = _mm_add_epi64(m128iData, m128iOper);
					break;
				case OPER_SUB:
					m128iResult = _mm_sub_epi64(m128iData, m128iOper);
					break;
				case OPER_MUL:
					m128iResult = _mm_set_epi64x(i64Data[1] * i64Oper, i64Data[0] * i64Oper);
					break;
				case OPER_DIV:
					assert(i64Oper > 0);
					m128iResult = _mm_set_epi64x(i64Data[1] / i64Oper, i64Data[0] / i64Oper);
					break;
				case OPER_CEIL:
					m128iResult = _mm_set_epi64x((std::min)(i64Data[1], i64Oper), (std::min)(i64Data[0], i64Oper));
					break;
				case OPER_FLOOR:
					m128iResult = _mm_set_epi64x((std::max)(i64Data[1], i64Oper), (std::max)(i64Data[0], i64Oper));
					break;
				case OPER_SWAP:
					m128iResult = ByteSwapSIMD<std::int64_t>(m128iData);
					break;
				case OPER_OR:
					m128iResult = _mm_or_si128(m128iData, m128iOper);
					break;
				case OPER_XOR:
					m128iResult = _mm_xor_si128(m128iData, m128iOper);
					break;
				case OPER_AND:
					m128iResult = _mm_and_si128(m128iData, m128iOper);
					break;
				case OPER_NOT:
					//_mm_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
					m128iResult = _mm_xor_si128(m128iData, _mm_cmpeq_epi64(m128iData, m128iData)); //SSE4.1
					break;
				case OPER_SHL:
					m128iResult = _mm_set_epi64x(i64Data[1] << i64Oper, i64Data[0] << i64Oper);
					break;
				case OPER_SHR:
					m128iResult = _mm_set_epi64x(i64Data[1] >> i64Oper, i64Data[0] >> i64Oper);
					break;
				case OPER_ROTL:
					m128iResult = _mm_set_epi64x(std::rotl(static_cast<std::uint64_t>(i64Data[1]), static_cast<const int>(i64Oper)),
						std::rotl(static_cast<std::uint64_t>(i64Data[0]), static_cast<const int>(i64Oper)));
					break;
				case OPER_ROTR:
					m128iResult = _mm_set_epi64x(std::rotr(static_cast<std::uint64_t>(i64Data[1]), static_cast<const int>(i64Oper)),
						std::rotr(static_cast<std::uint64_t>(i64Data[0]), static_cast<const int>(i64Oper)));
					break;
				case OPER_BITREV:
					m128iResult = _mm_set_epi64x(BitReverse(i64Data[1]), BitReverse(i64Data[0]));
					break;
				default:
					DBG_REPORT(L"Unsupported int64_t operation.");
					break;
				}

				if (hms.fBigEndian) { //Swap bytes back.
					m128iResult = ByteSwapSIMD<std::int64_t>(m128iResult);
				}

				_mm_storeu_si128(reinterpret_cast<__m128i*>(pi64Data), m128iResult);
				};

			constexpr auto lmbOperSIMDUInt64 = [](std::uint64_t* pui64Data, const HEXMODIFY & hms) {
				const auto m128iData = hms.fBigEndian ?
					ByteSwapSIMD<std::uint64_t>(_mm_loadu_si128(reinterpret_cast<const __m128i*>(pui64Data)))
					: _mm_loadu_si128(reinterpret_cast<const __m128i*>(pui64Data));
				alignas(16) std::uint64_t ui64Data[2];
				_mm_store_si128(reinterpret_cast<__m128i*>(ui64Data), m128iData);
				assert(!hms.spnData.empty());
				const auto ui64Oper = *reinterpret_cast<const std::uint64_t*>(hms.spnData.data());
				const auto m128iOper = _mm_set1_epi64x(ui64Oper);
				__m128i m128iResult { };

				switch (hms.eOperMode) {
				case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
					break;
				case OPER_ADD:
					m128iResult = _mm_add_epi64(m128iData, m128iOper);
					break;
				case OPER_SUB:
					m128iResult = _mm_sub_epi64(m128iData, m128iOper);
					break;
				case OPER_MUL:
					m128iResult = _mm_set_epi64x(ui64Data[1] * ui64Oper, ui64Data[0] * ui64Oper);
					break;
				case OPER_DIV:
					assert(ui64Oper > 0);
					m128iResult = _mm_set_epi64x(ui64Data[1] / ui64Oper, ui64Data[0] / ui64Oper);
					break;
				case OPER_CEIL:
					m128iResult = _mm_set_epi64x((std::min)(ui64Data[1], ui64Oper), (std::min)(ui64Data[0], ui64Oper));
					break;
				case OPER_FLOOR:
					m128iResult = _mm_set_epi64x((std::max)(ui64Data[1], ui64Oper), (std::max)(ui64Data[0], ui64Oper));
					break;
				case OPER_SWAP:
					m128iResult = ByteSwapSIMD<std::uint64_t>(m128iData);
					break;
				case OPER_OR:
					m128iResult = _mm_or_si128(m128iData, m128iOper);
					break;
				case OPER_XOR:
					m128iResult = _mm_xor_si128(m128iData, m128iOper);
					break;
				case OPER_AND:
					m128iResult = _mm_and_si128(m128iData, m128iOper);
					break;
				case OPER_NOT:
					//_mm_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
					m128iResult = _mm_xor_si128(m128iData, _mm_cmpeq_epi64(m128iData, m128iData)); //SSE4.1
					break;
				case OPER_SHL:
					m128iResult = _mm_set_epi64x(ui64Data[1] << ui64Oper, ui64Data[0] << ui64Oper);
					break;
				case OPER_SHR:
					m128iResult = _mm_set_epi64x(ui64Data[1] >> ui64Oper, ui64Data[0] >> ui64Oper);
					break;
				case OPER_ROTL:
					m128iResult = _mm_set_epi64x(std::rotl(ui64Data[1], static_cast<const int>(ui64Oper)),
						std::rotl(ui64Data[0], static_cast<const int>(ui64Oper)));
					break;
				case OPER_ROTR:
					m128iResult = _mm_set_epi64x(std::rotr(ui64Data[1], static_cast<const int>(ui64Oper)),
						std::rotr(ui64Data[0], static_cast<const int>(ui64Oper)));
					break;
				case OPER_BITREV:
					m128iResult = _mm_set_epi64x(BitReverse(ui64Data[1]), BitReverse(ui64Data[0]));
					break;
				default:
					DBG_REPORT(L"Unsupported uint64_t operation.");
					break;
				}

				if (hms.fBigEndian) { //Swap bytes back.
					m128iResult = ByteSwapSIMD<std::uint64_t>(m128iResult);
				}

				_mm_storeu_si128(reinterpret_cast<__m128i*>(pui64Data), m128iResult);
				};

			constexpr auto lmbOperSIMDFloat = [](float* pflData, const HEXMODIFY & hms) {
				const auto m128Data = hms.fBigEndian ? ByteSwapSIMD<float>(_mm_loadu_ps(pflData)) : _mm_loadu_ps(pflData);
				const auto m128Oper = _mm_set1_ps(*reinterpret_cast<const float*>(hms.spnData.data()));
				__m128 m128Result { };

				switch (hms.eOperMode) {
				case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
					break;
				case OPER_ADD:
					m128Result = _mm_add_ps(m128Data, m128Oper);
					break;
				case OPER_SUB:
					m128Result = _mm_sub_ps(m128Data, m128Oper);
					break;
				case OPER_MUL:
					m128Result = _mm_mul_ps(m128Data, m128Oper);
					break;
				case OPER_DIV:
					assert(*reinterpret_cast<const float*>(hms.spnData.data()) > 0.F);
					m128Result = _mm_div_ps(m128Data, m128Oper);
					break;
				case OPER_CEIL:
					m128Result = _mm_min_ps(m128Data, m128Oper);
					break;
				case OPER_FLOOR:
					m128Result = _mm_max_ps(m128Data, m128Oper);
					break;
				case OPER_SWAP:
					m128Result = ByteSwapSIMD<float>(m128Data);
					break;
				default:
					DBG_REPORT(L"Unsupported float operation.");
					return;
				}

				if (hms.fBigEndian) { //Swap bytes back.
					m128Result = ByteSwapSIMD<float>(m128Result);
				}

				_mm_storeu_ps(pflData, m128Result);
				};

			constexpr auto lmbOperSIMDDouble = [](double* pdblData, const HEXMODIFY & hms) {
				const auto m128dData = hms.fBigEndian ? ByteSwapSIMD<double>(_mm_loadu_pd(pdblData)) : _mm_loadu_pd(pdblData);
				const auto m128dOper = _mm_set1_pd(*reinterpret_cast<const double*>(hms.spnData.data()));
				__m128d m128dResult { };

				switch (hms.eOperMode) {
				case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
					break;
				case OPER_ADD:
					m128dResult = _mm_add_pd(m128dData, m128dOper);
					break;
				case OPER_SUB:
					m128dResult = _mm_sub_pd(m128dData, m128dOper);
					break;
				case OPER_MUL:
					m128dResult = _mm_mul_pd(m128dData, m128dOper);
					break;
				case OPER_DIV:
					assert(*reinterpret_cast<const double*>(hms.spnData.data()) > 0.);
					m128dResult = _mm_div_pd(m128dData, m128dOper);
					break;
				case OPER_CEIL:
					m128dResult = _mm_min_pd(m128dData, m128dOper);
					break;
				case OPER_FLOOR:
					m128dResult = _mm_max_pd(m128dData, m128dOper);
					break;
				case OPER_SWAP:
					m128dResult = ByteSwapSIMD<double>(m128dData);
					break;
				default:
					DBG_REPORT(L"Unsupported double operation.");
					return;
				}

				if (hms.fBigEndian) { //Swap bytes back.
					m128dResult = ByteSwapSIMD<double>(m128dResult);
				}

				_mm_storeu_pd(pdblData, m128dResult);
				};

			switch (hms.eDataType) {
			case DATA_INT8:
				lmbOperSIMDInt8(reinterpret_cast<std::int8_t*>(pData), hms);
				break;
			case DATA_UINT8:
				lmbOperSIMDUInt8(reinterpret_cast<std::uint8_t*>(pData), hms);
				break;
			case DATA_INT16:
				lmbOperSIMDInt16(reinterpret_cast<std::int16_t*>(pData), hms);
				break;
			case DATA_UINT16:
				lmbOperSIMDUInt16(reinterpret_cast<std::uint16_t*>(pData), hms);
				break;
			case DATA_INT32:
				lmbOperSIMDInt32(reinterpret_cast<std::int32_t*>(pData), hms);
				break;
			case DATA_UINT32:
				lmbOperSIMDUInt32(reinterpret_cast<std::uint32_t*>(pData), hms);
				break;
			case DATA_INT64:
				lmbOperSIMDInt64(reinterpret_cast<std::int64_t*>(pData), hms);
				break;
			case DATA_UINT64:
				lmbOperSIMDUInt64(reinterpret_cast<std::uint64_t*>(pData), hms);
				break;
			case DATA_FLOAT:
				lmbOperSIMDFloat(reinterpret_cast<float*>(pData), hms);
				break;
			case DATA_DOUBLE:
				lmbOperSIMDDouble(reinterpret_cast<double*>(pData), hms);
				break;
			default:
				break;
			}
			};

		//In cases where only one affected data region (hms.vecSpan.size()==1) is used,
		//and ullSizeToModify > ulSizeSIMD we use SIMD instructions.
		//At the end we simply fill up the remainder (ullSizeToModify % ulSizeSIMD).
		constexpr auto ulSizeSIMD { sizeof(__m128i) }; //Size in bytes of the __m128i data structure.
		const auto ullOffsetToModify = hms.vecSpan.back().ullOffset;
		const auto ullSizeToModify = hms.vecSpan.back().ullSize;
		const auto ullSizeToFillWith = hms.spnData.size();
		const auto ullSIMDCycles = ullSizeToModify / ulSizeSIMD; //How many times to make SIMD opers.

		if (hms.vecSpan.size() == 1 && ullSIMDCycles > 0) {
			ModifyWorker(hms, lmbOperSIMD, { static_cast<std::byte*>(nullptr), ulSizeSIMD }); //Worker with SIMD.

			if (const auto ullRem = ullSizeToModify % ulSizeSIMD; ullRem >= ullSizeToFillWith) { //Remainder of the SIMD.
				const auto ullOffset = ullOffsetToModify + ullSizeToModify - ullRem;
				const auto spnData = GetData({ ullOffset, ullRem });
				for (std::size_t iterRem = 0; iterRem < (ullRem / ullSizeToFillWith); ++iterRem) { //Works only if ullRem >= ullSizeToFillWith.
					lmbOperDefault(spnData.data() + (iterRem * ullSizeToFillWith), hms, { });
				}
				SetDataVirtual(spnData, { ullOffset, ullRem - (ullRem % ullSizeToFillWith) });
			}
		}
		else {
			ModifyWorker(hms, lmbOperDefault, hms.spnData);
		}
	}
	break;
	default:
		break;
	}
	SetRedraw(true);

	OnModifyData();
}

void CHexCtrl::Redraw()
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	if (IsDataSet()) {
		//^ - encloses Data name, ` (tilda) - encloses a data itself.
		m_wstrInfoBar = std::vformat(IsOffsetAsHex() ? L"^Caret: ^`0x{:X} `" : L"^Caret: ^`{} `", std::make_wformat_args(GetCaretPos()));

		if (IsPageVisible()) { //Page/Sector.
			m_wstrInfoBar += std::vformat(IsOffsetAsHex() ? L"^{}: ^`0x{:X}/0x{:X} `" : L"^{}: ^`{}/{} `",
				std::make_wformat_args(m_wstrPageName, GetPagePos(), GetPagesCount()));
		}

		if (HasSelection()) {
			const auto ullSelSize = m_pSelection->GetSelSize();
			if (ullSelSize == 1) { //In case of just one byte selected.
				m_wstrInfoBar += std::vformat(IsOffsetAsHex() ? L"^Selected: ^`0x{:X} [0x{:X}] `" : L"^ Selected: ^`{} [{}] `",
					std::make_wformat_args(ullSelSize, m_pSelection->GetSelStart()));
			}
			else {
				m_wstrInfoBar += std::vformat(IsOffsetAsHex() ? L"^Selected: ^`0x{:X} [0x{:X}-0x{:X}] `" : L"^ Selected: ^`{} [{}-{}] `",
					std::make_wformat_args(ullSelSize, m_pSelection->GetSelStart(), m_pSelection->GetSelEnd()));
			}
		}

		m_wstrInfoBar += IsMutable() ? L"^RW^" : L"^RO^"; //RW/RO mode.
	}
	else {
		m_wstrInfoBar.clear();
	}

	RedrawWindow();
}

void CHexCtrl::SetCapacity(DWORD dwCapacity)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	dwCapacity = std::clamp(dwCapacity, 1UL, 100UL);
	const auto dwGroupSize = GetGroupSize();
	const auto dwCurrCapacity = GetCapacity();
	const auto dwMod = dwCapacity % dwGroupSize; //Setting the capacity according to the current data grouping size.

	if (dwCapacity < dwCurrCapacity) {
		dwCapacity -= dwMod;
	}
	else {
		dwCapacity += dwMod > 0 ? dwGroupSize - dwMod : 0; //Remaining part. Actual only if dwMod > 0.
	}

	if (dwCapacity < dwGroupSize) {
		dwCapacity = dwGroupSize;
	}
	else if (dwCapacity > 100UL) { //100UL is the maximum allowed capacity.
		dwCapacity -= dwGroupSize;
	}

	m_dwCapacity = dwCapacity;
	m_dwCapacityBlockSize = dwCapacity / 2;

	FillCapacityString();
	RecalcAll();
	ParentNotify(HEXCTRL_MSG_SETCAPACITY);
}

void CHexCtrl::SetCaretPos(ULONGLONG ullOffset, bool fHighLow, bool fRedraw)
{
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet() || ullOffset >= GetDataSize())
		return;

	m_ullCaretPos = ullOffset;
	m_fCaretHigh = fHighLow;

	if (fRedraw) {
		Redraw();
	}

	OnCaretPosChange(ullOffset);
}

void CHexCtrl::SetCharsExtraSpace(DWORD dwSpace)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_dwCharsExtraSpace = (std::min)(dwSpace, 10UL);
	RecalcAll();
}

void CHexCtrl::SetCodepage(int iCodepage)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	CPINFOEXW stCPInfo;
	std::wstring_view wsvFmt;
	switch (iCodepage) {
	case -1:
		wsvFmt = L"ASCII";
		break;
	case 0:
		wsvFmt = L"UTF-16";
		break;
	default:
		if (GetCPInfoExW(static_cast<UINT>(iCodepage), 0, &stCPInfo) != FALSE) {
			wsvFmt = L"Codepage {}";
		}
		break;
	}

	if (!wsvFmt.empty()) {
		m_iCodePage = iCodepage;
		m_wstrTextTitle = std::vformat(wsvFmt, std::make_wformat_args(m_iCodePage));
		Redraw();
	}

	ParentNotify(HEXCTRL_MSG_SETCODEPAGE);
}

void CHexCtrl::SetColors(const HEXCOLORS& hcs)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_stColors = hcs;
	RedrawWindow();
}

bool CHexCtrl::SetConfig(std::wstring_view wsvPath)
{
	assert(IsCreated());
	if (!IsCreated())
		return false;

	using enum EHexCmd;
	//Mapping between stringified EHexCmd::* and its value-menuID pairs.
	const std::unordered_map<std::string_view, std::pair<EHexCmd, DWORD>> umapCmdMenu {
		{ "CMD_SEARCH_DLG", { CMD_SEARCH_DLG, IDM_HEXCTRL_SEARCH_DLGSEARCH } },
		{ "CMD_SEARCH_NEXT", { CMD_SEARCH_NEXT, IDM_HEXCTRL_SEARCH_NEXT } },
		{ "CMD_SEARCH_PREV", { CMD_SEARCH_PREV, IDM_HEXCTRL_SEARCH_PREV } },
		{ "CMD_NAV_GOTO_DLG", { CMD_NAV_GOTO_DLG, IDM_HEXCTRL_NAV_DLGGOTO } },
		{ "CMD_NAV_REPFWD", { CMD_NAV_REPFWD, IDM_HEXCTRL_NAV_REPFWD } },
		{ "CMD_NAV_REPBKW", { CMD_NAV_REPBKW, IDM_HEXCTRL_NAV_REPBKW } },
		{ "CMD_NAV_DATABEG", { CMD_NAV_DATABEG, IDM_HEXCTRL_NAV_DATABEG } },
		{ "CMD_NAV_DATAEND", { CMD_NAV_DATAEND, IDM_HEXCTRL_NAV_DATAEND } },
		{ "CMD_NAV_PAGEBEG", { CMD_NAV_PAGEBEG, IDM_HEXCTRL_NAV_PAGEBEG } },
		{ "CMD_NAV_PAGEEND", { CMD_NAV_PAGEEND, IDM_HEXCTRL_NAV_PAGEEND } },
		{ "CMD_NAV_LINEBEG", { CMD_NAV_LINEBEG, IDM_HEXCTRL_NAV_LINEBEG } },
		{ "CMD_NAV_LINEEND", { CMD_NAV_LINEEND, IDM_HEXCTRL_NAV_LINEEND } },
		{ "CMD_GROUPDATA_BYTE", { CMD_GROUPDATA_BYTE, IDM_HEXCTRL_GROUPDATA_BYTE } },
		{ "CMD_GROUPDATA_WORD", { CMD_GROUPDATA_WORD, IDM_HEXCTRL_GROUPDATA_WORD } },
		{ "CMD_GROUPDATA_DWORD", { CMD_GROUPDATA_DWORD, IDM_HEXCTRL_GROUPDATA_DWORD } },
		{ "CMD_GROUPDATA_QWORD", { CMD_GROUPDATA_QWORD, IDM_HEXCTRL_GROUPDATA_QWORD } },
		{ "CMD_GROUPDATA_INC", { CMD_GROUPDATA_INC, IDM_HEXCTRL_GROUPDATA_INC } },
		{ "CMD_GROUPDATA_DEC", { CMD_GROUPDATA_DEC, IDM_HEXCTRL_GROUPDATA_DEC } },
		{ "CMD_BKM_ADD", { CMD_BKM_ADD, IDM_HEXCTRL_BKM_ADD } },
		{ "CMD_BKM_REMOVE", { CMD_BKM_REMOVE, IDM_HEXCTRL_BKM_REMOVE } },
		{ "CMD_BKM_NEXT", { CMD_BKM_NEXT, IDM_HEXCTRL_BKM_NEXT } },
		{ "CMD_BKM_PREV", { CMD_BKM_PREV, IDM_HEXCTRL_BKM_PREV } },
		{ "CMD_BKM_REMOVEALL", { CMD_BKM_REMOVEALL, IDM_HEXCTRL_BKM_REMOVEALL } },
		{ "CMD_BKM_DLG_MGR", { CMD_BKM_DLG_MGR, IDM_HEXCTRL_BKM_DLGMGR } },
		{ "CMD_CLPBRD_COPY_HEX", { CMD_CLPBRD_COPY_HEX, IDM_HEXCTRL_CLPBRD_COPYHEX } },
		{ "CMD_CLPBRD_COPY_HEXLE", { CMD_CLPBRD_COPY_HEXLE, IDM_HEXCTRL_CLPBRD_COPYHEXLE } },
		{ "CMD_CLPBRD_COPY_HEXFMT", { CMD_CLPBRD_COPY_HEXFMT, IDM_HEXCTRL_CLPBRD_COPYHEXFMT } },
		{ "CMD_CLPBRD_COPY_TEXTCP", { CMD_CLPBRD_COPY_TEXTCP, IDM_HEXCTRL_CLPBRD_COPYTEXTCP } },
		{ "CMD_CLPBRD_COPY_BASE64", { CMD_CLPBRD_COPY_BASE64, IDM_HEXCTRL_CLPBRD_COPYBASE64 } },
		{ "CMD_CLPBRD_COPY_CARR", { CMD_CLPBRD_COPY_CARR, IDM_HEXCTRL_CLPBRD_COPYCARR } },
		{ "CMD_CLPBRD_COPY_GREPHEX", { CMD_CLPBRD_COPY_GREPHEX, IDM_HEXCTRL_CLPBRD_COPYGREPHEX } },
		{ "CMD_CLPBRD_COPY_PRNTSCRN", { CMD_CLPBRD_COPY_PRNTSCRN, IDM_HEXCTRL_CLPBRD_COPYPRNTSCRN } },
		{ "CMD_CLPBRD_COPY_OFFSET", { CMD_CLPBRD_COPY_OFFSET, IDM_HEXCTRL_CLPBRD_COPYOFFSET } },
		{ "CMD_CLPBRD_PASTE_HEX", { CMD_CLPBRD_PASTE_HEX, IDM_HEXCTRL_CLPBRD_PASTEHEX } },
		{ "CMD_CLPBRD_PASTE_TEXTUTF16", { CMD_CLPBRD_PASTE_TEXTUTF16, IDM_HEXCTRL_CLPBRD_PASTETEXTUTF16 } },
		{ "CMD_CLPBRD_PASTE_TEXTCP", { CMD_CLPBRD_PASTE_TEXTCP, IDM_HEXCTRL_CLPBRD_PASTETEXTCP } },
		{ "CMD_MODIFY_OPERS_DLG", { CMD_MODIFY_OPERS_DLG, IDM_HEXCTRL_MODIFY_DLGOPERS } },
		{ "CMD_MODIFY_FILLZEROS", { CMD_MODIFY_FILLZEROS, IDM_HEXCTRL_MODIFY_FILLZEROS } },
		{ "CMD_MODIFY_FILLDATA_DLG", { CMD_MODIFY_FILLDATA_DLG, IDM_HEXCTRL_MODIFY_DLGFILLDATA } },
		{ "CMD_MODIFY_UNDO", { CMD_MODIFY_UNDO, IDM_HEXCTRL_MODIFY_UNDO } },
		{ "CMD_MODIFY_REDO", { CMD_MODIFY_REDO, IDM_HEXCTRL_MODIFY_REDO } },
		{ "CMD_SEL_MARKSTART", { CMD_SEL_MARKSTART, IDM_HEXCTRL_SEL_MARKSTART } },
		{ "CMD_SEL_MARKEND", { CMD_SEL_MARKEND, IDM_HEXCTRL_SEL_MARKEND } },
		{ "CMD_SEL_ALL", { CMD_SEL_ALL, IDM_HEXCTRL_SEL_ALL } },
		{ "CMD_SEL_ADDLEFT", { CMD_SEL_ADDLEFT, 0 } },
		{ "CMD_SEL_ADDRIGHT", { CMD_SEL_ADDRIGHT, 0 } },
		{ "CMD_SEL_ADDUP", { CMD_SEL_ADDUP, 0 } },
		{ "CMD_SEL_ADDDOWN", { CMD_SEL_ADDDOWN, 0 } },
		{ "CMD_DATAINTERP_DLG", { CMD_DATAINTERP_DLG, IDM_HEXCTRL_DLGDATAINTERP } },
		{ "CMD_CODEPAGE_DLG", { CMD_CODEPAGE_DLG, IDM_HEXCTRL_DLGCODEPAGE } },
		{ "CMD_APPEAR_FONT_DLG", { CMD_APPEAR_FONT_DLG, IDM_HEXCTRL_APPEAR_DLGFONT } },
		{ "CMD_APPEAR_FONTINC", { CMD_APPEAR_FONTINC, IDM_HEXCTRL_APPEAR_FONTINC } },
		{ "CMD_APPEAR_FONTDEC", { CMD_APPEAR_FONTDEC, IDM_HEXCTRL_APPEAR_FONTDEC } },
		{ "CMD_APPEAR_CAPACINC", { CMD_APPEAR_CAPACINC, IDM_HEXCTRL_APPEAR_CAPACINC } },
		{ "CMD_APPEAR_CAPACDEC", { CMD_APPEAR_CAPACDEC, IDM_HEXCTRL_APPEAR_CAPACDEC } },
		{ "CMD_PRINT_DLG", { CMD_PRINT_DLG, IDM_HEXCTRL_OTHER_DLGPRINT } },
		{ "CMD_ABOUT_DLG", { CMD_ABOUT_DLG, IDM_HEXCTRL_OTHER_DLGABOUT } },
		{ "CMD_CARET_LEFT", { CMD_CARET_LEFT, 0 } },
		{ "CMD_CARET_RIGHT", { CMD_CARET_RIGHT, 0 } },
		{ "CMD_CARET_UP", { CMD_CARET_UP, 0 } },
		{ "CMD_CARET_DOWN", { CMD_CARET_DOWN, 0 } },
		{ "CMD_SCROLL_PAGEUP", { CMD_SCROLL_PAGEUP, 0 } },
		{ "CMD_SCROLL_PAGEDOWN", { CMD_SCROLL_PAGEDOWN, 0 } },
		{ "CMD_TEMPL_APPLYCURR", { CMD_TEMPL_APPLYCURR, IDM_HEXCTRL_TEMPL_APPLYCURR } },
		{ "CMD_TEMPL_DISAPPLY", { CMD_TEMPL_DISAPPLY, IDM_HEXCTRL_TEMPL_DISAPPLY } },
		{ "CMD_TEMPL_DISAPPALL", { CMD_TEMPL_DISAPPALL, IDM_HEXCTRL_TEMPL_DISAPPALL } },
		{ "CMD_TEMPL_DLG_MGR", { CMD_TEMPL_DLG_MGR, IDM_HEXCTRL_TEMPL_DLGMGR } }
	};

	//Mapping between JSON-data commands and actual keyboard codes, with names that appear in the menu.
	const std::unordered_map<std::string_view, std::pair<UINT, std::wstring_view>> umapKeys {
		{ { "ctrl" }, { VK_CONTROL, L"Ctrl" } },
		{ { "shift" }, { VK_SHIFT, L"Shift" } },
		{ { "alt" }, { VK_MENU, L"Alt" } },
		{ { "tab" }, { VK_TAB, L"Tab" } },
		{ { "enter" }, { VK_RETURN, L"Enter" } },
		{ { "esc" }, { VK_ESCAPE, L"Esc" } },
		{ { "space" }, { VK_SPACE, L"Space" } },
		{ { "backspace" }, { VK_BACK, L"Backspace" } },
		{ { "delete" }, { VK_DELETE, L"Delete" } },
		{ { "insert" }, { VK_INSERT, L"Insert" } },
		{ { "f1" }, { VK_F1, L"F1" } },
		{ { "f2" }, { VK_F2, L"F2" } },
		{ { "f3" }, { VK_F3, L"F3" } },
		{ { "f4" }, { VK_F4, L"F4" } },
		{ { "f5" }, { VK_F5, L"F5" } },
		{ { "f6" }, { VK_F6, L"F6" } },
		{ { "f7" }, { VK_F7, L"F7" } },
		{ { "f8" }, { VK_F8, L"F8" } },
		{ { "f9" }, { VK_F9, L"F9" } },
		{ { "f10" }, { VK_F10, L"F10" } },
		{ { "right" }, { VK_RIGHT, L"Right Arrow" } },
		{ { "left" }, { VK_LEFT, L"Left Arrow" } },
		{ { "up" }, { VK_UP, L"Up Arrow" } },
		{ { "down" }, { VK_DOWN, L"Down Arrow" } },
		{ { "pageup" }, { VK_PRIOR, L"PageUp" } },
		{ { "pagedown" }, { VK_NEXT, L"PageDown" } },
		{ { "home" }, { VK_HOME, L"Home" } },
		{ { "end" }, { VK_END, L"End" } },
		{ { "plus" }, { VK_OEM_PLUS, L"Plus" } },
		{ { "minus" }, { VK_OEM_MINUS, L"Minus" } },
		{ { "num_plus" }, { VK_ADD, L"Num Plus" } },
		{ { "num_minus" }, { VK_SUBTRACT, L"Num Minus" } },
		{ { "mouse_wheel_up" }, { m_dwVKMouseWheelUp, L"Mouse-Wheel Up" } },
		{ { "mouse_wheel_down" }, { m_dwVKMouseWheelDown, L"Mouse-Wheel Down" } }
	};

	//Filling m_vecKeyBind with ALL available commands/menus.
	//This is vital for ExecuteCmd to work properly.
	m_vecKeyBind.clear();
	m_vecKeyBind.reserve(umapCmdMenu.size());
	for (const auto& itMap : umapCmdMenu) {
		m_vecKeyBind.emplace_back(KEYBIND { .eCmd { itMap.second.first }, .wMenuID { static_cast<WORD>(itMap.second.second) } });
	}

	rapidjson::Document docJSON;
	bool fJSONParsed { false };
	if (wsvPath.empty()) { //Default IDR_HEXCTRL_JSON_KEYBIND.json, from resources.
		const auto hInst = AfxGetInstanceHandle();
		if (const auto hRes = FindResourceW(hInst, MAKEINTRESOURCEW(IDJ_HEXCTRL_KEYBIND), L"JSON"); hRes != nullptr) {
			if (const auto hData = LoadResource(hInst, hRes); hData != nullptr) {
				const auto nSize = static_cast<std::size_t>(SizeofResource(hInst, hRes));
				const auto* const pData = static_cast<char*>(LockResource(hData));
				if (docJSON.Parse(pData, nSize); !docJSON.IsNull()) { //Parse all default keybindings.
					fJSONParsed = true;
				}
			}
		}
	}
	else if (std::ifstream ifs(std::wstring { wsvPath }); ifs.is_open()) {
		rapidjson::IStreamWrapper isw { ifs };
		if (docJSON.ParseStream(isw); !docJSON.IsNull()) {
			fJSONParsed = true;
		}
	}
	assert(fJSONParsed);

	if (fJSONParsed) {
		const auto lmbParseStr = [&](std::string_view sv)->std::optional<KEYBIND> {
			if (sv.empty())
				return { };

			KEYBIND stRet { };
			const auto nSize = sv.size();
			std::size_t nPosStart { 0 }; //Next position to start search for '+' sign.
			const auto nSubWords = std::count(sv.begin(), sv.end(), '+') + 1; //How many sub-words (divided by '+')?
			for (auto iterSubWords = 0; iterSubWords < nSubWords; ++iterSubWords) {
				const auto nPosNext = sv.find('+', nPosStart);
				const auto nSizeSubWord = nPosNext == std::string_view::npos ? nSize - nPosStart : nPosNext - nPosStart;
				const auto strSubWord = sv.substr(nPosStart, nSizeSubWord);
				nPosStart = nPosNext + 1;

				if (strSubWord.size() == 1) {
					stRet.uKey = static_cast<UCHAR>(std::toupper(strSubWord[0])); //Binding keys are in uppercase.
				}
				else if (const auto iter = umapKeys.find(strSubWord); iter != umapKeys.end()) {
					switch (const auto uChar = iter->second.first; uChar) {
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

		for (auto iterMembers = docJSON.MemberBegin(); iterMembers != docJSON.MemberEnd(); ++iterMembers) { //JSON data iterating.
			if (const auto iterCmd = umapCmdMenu.find(iterMembers->name.GetString()); iterCmd != umapCmdMenu.end()) {
				for (auto iterArrCurr = iterMembers->value.Begin(); iterArrCurr != iterMembers->value.End(); ++iterArrCurr) { //Array iterating.
					if (auto optKey = lmbParseStr(iterArrCurr->GetString()); optKey) {
						optKey->eCmd = iterCmd->second.first;
						optKey->wMenuID = static_cast<WORD>(iterCmd->second.second);
						if (auto it = std::find_if(m_vecKeyBind.begin(), m_vecKeyBind.end(), [&optKey](const KEYBIND& ref) {
							return ref.eCmd == optKey->eCmd; }); it != m_vecKeyBind.end()) {
							if (it->uKey == 0) {
								*it = *optKey; //Adding keybindings from JSON to m_vecKeyBind.
							}
							else {
								//If such command with some key from JSON already exist, we adding another one
								//same command but with a different key, like Ctrl+F/Ctrl+H for Search.
								m_vecKeyBind.emplace_back(*optKey);
							}
						}
					}
				}
			}
		}

		std::size_t i { 0 };
		for (const auto& iterMain : m_vecKeyBind) {
			//Check for previous same menu ID. To assign only one, first, keybinding for menu name.
			//With `"ctrl+f", "ctrl+h"` in JSON, only the "Ctrl+F" will be assigned as the menu name.
			const auto iterEnd = m_vecKeyBind.begin() + i++;
			if (const auto iterTmp = std::find_if(m_vecKeyBind.begin(), iterEnd, [&](const KEYBIND& ref) {
				return ref.wMenuID == iterMain.wMenuID; });
				iterTmp == iterEnd && iterMain.wMenuID != 0 && iterMain.uKey != 0) {
				CStringW wstrMenuName;
				m_menuMain.GetMenuStringW(iterMain.wMenuID, wstrMenuName, MF_BYCOMMAND);
				std::wstring wstr = wstrMenuName.GetString();
				if (const auto nPos = wstr.find('\t'); nPos != std::wstring::npos) {
					wstr.erase(nPos);
				}

				wstr += L'\t';
				if (iterMain.fCtrl) {
					wstr += L"Ctrl+";
				}
				if (iterMain.fShift) {
					wstr += L"Shift+";
				}
				if (iterMain.fAlt) {
					wstr += L"Alt+";
				}

				//Search for any special key names: 'Tab', 'Enter', etc... If not found then it's just a char.
				if (const auto iterUmap = std::find_if(umapKeys.begin(), umapKeys.end(), [&](const auto& ref) {
					return ref.second.first == iterMain.uKey; }); iterUmap != umapKeys.end()) {
					wstr += iterUmap->second.second;
				}
				else {
					wstr += static_cast<unsigned char>(iterMain.uKey);
				}

				//Modify menu with a new name (with shortcut appended) and old bitmap.
				MENUITEMINFOW mii { .cbSize = sizeof(MENUITEMINFOW), .fMask = MIIM_BITMAP | MIIM_STRING };
				m_menuMain.GetMenuItemInfoW(iterMain.wMenuID, &mii);
				mii.dwTypeData = wstr.data();
				m_menuMain.SetMenuItemInfoW(iterMain.wMenuID, &mii);
			}
		}
	}

	return fJSONParsed;
}

void CHexCtrl::SetData(const HEXDATA& hds)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	ClearData();
	if (hds.spnData.empty()) //If the data size is zero we do nothing further.
		return;

	m_spnData = hds.spnData;
	m_pHexVirtColors = hds.pHexVirtColors;
	constexpr auto dwCacheMinSize { 1024U * 64U }; //Minimum data cache size for VirtualData mode.
	m_dwCacheSize = (hds.dwCacheSize < dwCacheMinSize) ? dwCacheMinSize : hds.dwCacheSize;
	m_pHexVirtData = hds.pHexVirtData;
	m_fMutable = hds.fMutable;
	m_fHighLatency = hds.fHighLatency;
	m_fDataSet = true;

	RecalcAll();
}

void CHexCtrl::SetDateInfo(DWORD dwFormat, wchar_t wchSepar)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	//dwFormat: -1 = User default, 0 = MMddYYYY, 1 = ddMMYYYY, 2 = YYYYMMdd
	assert(dwFormat <= 2 || dwFormat == 0xFFFFFFFF);
	if (dwFormat > 2 && dwFormat != 0xFFFFFFFF)
		return;

	if (dwFormat == 0xFFFFFFFF) {
		//Determine current user locale-specific date format.
		if (GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_IDATE | LOCALE_RETURN_NUMBER,
			reinterpret_cast<LPWSTR>(&m_dwDateFormat), sizeof(m_dwDateFormat)) == 0) {
			assert(true); //Something went wrong.			
		}
	}
	else {
		m_dwDateFormat = dwFormat;
	}

	if (wchSepar == L'\0') {
		wchSepar = L'/';
	}
	m_wchDateSepar = wchSepar;
}

auto CHexCtrl::SetDlgData(EHexWnd eWnd, std::uint64_t ullData, bool fCreate)->HWND
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	switch (eWnd) {
	case EHexWnd::WND_MAIN:
		return m_hWnd;
	case EHexWnd::DLG_BKMMGR:
		return m_pDlgBkmMgr->SetDlgData(ullData, fCreate);
	case EHexWnd::DLG_DATAINTERP:
		return m_pDlgDataInterp->SetDlgData(ullData, fCreate);
	case EHexWnd::DLG_MODIFY:
		return m_pDlgModify->SetDlgData(ullData, fCreate);
	case EHexWnd::DLG_SEARCH:
		return m_pDlgSearch->SetDlgData(ullData, fCreate);
	case EHexWnd::DLG_CODEPAGE:
		return m_pDlgCodepage->SetDlgData(ullData, fCreate);
	case EHexWnd::DLG_GOTO:
		return m_pDlgGoTo->SetDlgData(ullData, fCreate);
	case EHexWnd::DLG_TEMPLMGR:
		return m_pDlgTemplMgr->SetDlgData(ullData, fCreate);
	default:
		return { };
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
	ParentNotify(HEXCTRL_MSG_SETFONT);
}

void CHexCtrl::SetGroupSize(DWORD dwSize)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_dwGroupSize = std::clamp(dwSize, 1UL, 64UL);

	//Getting the "Show data as..." menu pointer independent of position.
	const auto* const pMenuMain = m_menuMain.GetSubMenu(0);
	CMenu* pMenuShowDataAs { };
	for (int i = 0; i < pMenuMain->GetMenuItemCount(); ++i) {
		//Searching through all submenus whose first menuID is IDM_HEXCTRL_GROUPDATA_BYTE.
		if (const auto pSubMenu = pMenuMain->GetSubMenu(i); pSubMenu != nullptr) {
			if (pSubMenu->GetMenuItemID(0) == IDM_HEXCTRL_GROUPDATA_BYTE) {
				pMenuShowDataAs = pSubMenu;
				break;
			}
		}
	}

	if (pMenuShowDataAs != nullptr) {
		//Unchecking all menus and checking only the currently selected.
		for (int i = 0; i < pMenuShowDataAs->GetMenuItemCount(); ++i) {
			pMenuShowDataAs->CheckMenuItem(i, MF_UNCHECKED | MF_BYPOSITION);
		}

		UINT uIDToCheck { 0 };
		switch (dwSize) {
		case 1:
			uIDToCheck = IDM_HEXCTRL_GROUPDATA_BYTE;
			break;
		case 2:
			uIDToCheck = IDM_HEXCTRL_GROUPDATA_WORD;
			break;
		case 4:
			uIDToCheck = IDM_HEXCTRL_GROUPDATA_DWORD;
			break;
		case 8:
			uIDToCheck = IDM_HEXCTRL_GROUPDATA_QWORD;
			break;
		default:
			break;
		}

		if (uIDToCheck > 0) {
			pMenuShowDataAs->CheckMenuItem(uIDToCheck, MF_CHECKED | MF_BYCOMMAND);
		}
	}

	SetCapacity(GetCapacity()); //To recalc current representation.
	ParentNotify(HEXCTRL_MSG_SETGROUPSIZE);
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

	m_fOffsetHex = fHex;
	FillCapacityString();
	RecalcAll();
}

void CHexCtrl::SetPageSize(DWORD dwSize, std::wstring_view wsvName)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_dwPageSize = dwSize;
	m_wstrPageName = wsvName;
	if (IsDataSet()) {
		Redraw();
	}
}

void CHexCtrl::SetRedraw(bool fRedraw)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_fRedraw = fRedraw;
}

void CHexCtrl::SetScrollRatio(float flRatio, bool fLines)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_flScrollRatio = flRatio;
	m_fScrollLines = fLines;
	m_pScrollV->SetScrollPageSize(GetScrollPageSize());
}

void CHexCtrl::SetSelection(const VecSpan& vecSel, bool fRedraw, bool fHighlight)
{
	assert(IsCreated());
	assert(IsDataSet());
	if (!IsCreated() || !IsDataSet())
		return;

	m_pSelection->SetSelection(vecSel, fHighlight);

	if (fRedraw) {
		Redraw();
	}

	ParentNotify(HEXCTRL_MSG_SETSELECTION);
}

void CHexCtrl::SetUnprintableChar(wchar_t wch)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_wchUnprintable = wch;
	Redraw();
}

void CHexCtrl::SetVirtualBkm(IHexBookmarks* pVirtBkm)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_pDlgBkmMgr->SetVirtual(pVirtBkm);
}

void CHexCtrl::ShowInfoBar(bool fShow)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_fInfoBar = fShow;
	RecalcAll();
}


//CHexCtrl Private methods.

auto CHexCtrl::BuildDataToDraw(ULONGLONG ullStartLine, int iLines)const->std::tuple<std::wstring, std::wstring>
{
	if (!IsDataSet())
		return { };

	const auto ullOffsetStart = ullStartLine * GetCapacity(); //Offset of the visible data to print.
	const auto ullDataSize = GetDataSize();
	auto sSizeDataToPrint = static_cast<std::size_t>(iLines) * GetCapacity(); //Size of the visible data to print.

	if (ullOffsetStart + sSizeDataToPrint > ullDataSize) {
		sSizeDataToPrint = static_cast<std::size_t>(ullDataSize - ullOffsetStart);
	}

	const auto spnData = GetData({ ullOffsetStart, sSizeDataToPrint }); //Span data to print.
	assert(!spnData.empty());
	assert(spnData.size() >= sSizeDataToPrint);

	const auto pDataBegin = reinterpret_cast<unsigned char*>(spnData.data()); //Pointer to data to print.
	const auto pDataEnd = pDataBegin + sSizeDataToPrint;

	//Hex Bytes to print.
	std::wstring wstrHex { };
	wstrHex.reserve(sSizeDataToPrint * 2);
	for (auto iterData = pDataBegin; iterData < pDataEnd; ++iterData) { //Converting bytes to Hexes.
		wstrHex.push_back(m_pwszHexChars[(*iterData >> 4) & 0x0F]);
		wstrHex.push_back(m_pwszHexChars[*iterData & 0x0F]);
	}

	//Text to print.
	std::wstring wstrText;
	const auto iCodepage = GetCodepage();
	if (iCodepage == -1) { //ASCII codepage: we simply assigning [pDataBegin...pDataEnd) to wstrText w/o any conversion.
		wstrText.assign(pDataBegin, pDataEnd);
	}
	else if (iCodepage == 0) { //UTF-16.
		const auto pDataUTF16Beg = reinterpret_cast<wchar_t*>(pDataBegin);
		const auto pDataUTF16End = reinterpret_cast<wchar_t*>((sSizeDataToPrint % 2) == 0 ? pDataEnd : pDataEnd - 1);
		wstrText.assign(pDataUTF16Beg, pDataUTF16End);
		wstrText.resize(sSizeDataToPrint);
	}
	else {
		wstrText.resize(sSizeDataToPrint);
		MultiByteToWideChar(iCodepage, 0, reinterpret_cast<LPCCH>(pDataBegin),
			static_cast<int>(sSizeDataToPrint), wstrText.data(), static_cast<int>(sSizeDataToPrint));
	}
	ReplaceUnprintable(wstrText, iCodepage == -1, true);

	return { std::move(wstrHex), std::move(wstrText) };
}

void CHexCtrl::CaretMoveDown()
{
	const auto ullOldPos = m_ullCaretPos;
	const auto ullNewPos = ullOldPos + GetCapacity() >= GetDataSize() ? ullOldPos : ullOldPos + GetCapacity();
	SetCaretPos(ullNewPos, m_fCaretHigh, false);

	const auto stOld = IsOffsetVisible(ullOldPos);
	const auto stNew = IsOffsetVisible(ullNewPos);
	if (stOld.i8Vert == 0 && stNew.i8Vert != 0) {
		m_pScrollV->ScrollLineDown();
	}

	Redraw();
}

void CHexCtrl::CaretMoveLeft()
{
	const auto ullOldPos = m_ullCaretPos;
	auto ullNewPos { 0ULL };

	if (IsCurTextArea() || !IsMutable()) {
		if (ullOldPos == 0) //To avoid underflow.
			return;
		ullNewPos = ullOldPos - 1;
	}
	else {
		if (m_fCaretHigh) {
			if (ullOldPos == 0) //To avoid underflow.
				return;
			ullNewPos = ullOldPos - 1;
			m_fCaretHigh = false;
		}
		else {
			ullNewPos = ullOldPos;
			m_fCaretHigh = true;
		}
	}

	SetCaretPos(ullNewPos, m_fCaretHigh, false);

	const auto stOld = IsOffsetVisible(ullOldPos);
	const auto stNew = IsOffsetVisible(ullNewPos);
	if (stOld.i8Vert == 0 && stNew.i8Vert != 0) {
		m_pScrollV->ScrollLineUp();
	}
	else if (stNew.i8Horz != 0 && !IsCurTextArea()) { //Do not horz scroll when in text area.
		ScrollOffsetH(ullNewPos);
	}

	Redraw();
}

void CHexCtrl::CaretMoveRight()
{
	if (!IsDataSet())
		return;

	const auto ullOldPos = m_ullCaretPos;
	auto ullNewPos { 0ULL };

	if (IsCurTextArea() || !IsMutable()) {
		ullNewPos = ullOldPos + 1;
	}
	else {
		if (m_fCaretHigh) {
			ullNewPos = ullOldPos;
			m_fCaretHigh = false;
		}
		else {
			ullNewPos = ullOldPos + 1;
			m_fCaretHigh = ullOldPos != GetDataSize() - 1; //Last byte case.
		}
	}

	if (const auto ullDataSize = GetDataSize(); ullNewPos >= ullDataSize) { //To avoid overflow.
		ullNewPos = ullDataSize - 1;
	}

	SetCaretPos(ullNewPos, m_fCaretHigh, false);

	const auto stOld = IsOffsetVisible(ullOldPos);
	const auto stNew = IsOffsetVisible(ullNewPos);
	if (stOld.i8Vert == 0 && stNew.i8Vert != 0) {
		m_pScrollV->ScrollLineDown();
	}
	else if (stNew.i8Horz != 0 && !IsCurTextArea()) { //Do not horz scroll when in text area.
		ScrollOffsetH(ullNewPos);
	}

	Redraw();
}

void CHexCtrl::CaretMoveUp()
{
	const auto ullOldPos = m_ullCaretPos;
	const auto ullNewPos = ullOldPos >= GetCapacity() ? ullOldPos - GetCapacity() : ullOldPos;
	SetCaretPos(ullNewPos, m_fCaretHigh, false);

	const auto stOld = IsOffsetVisible(ullOldPos);
	const auto stNew = IsOffsetVisible(ullNewPos);
	if (stOld.i8Vert == 0 && stNew.i8Vert != 0) {
		m_pScrollV->ScrollLineUp();
	}

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
	if (!IsOffsetVisible(ullPos)) {
		GoToOffset(ullPos);
	}
}

void CHexCtrl::CaretToLineEnd()
{
	auto ullPos = GetCaretPos() + (GetCapacity() - (GetCaretPos() % GetCapacity())) - 1;
	if (ullPos >= GetDataSize()) {
		ullPos = GetDataSize() - 1;
	}
	SetCaretPos(ullPos);
	if (!IsOffsetVisible(ullPos)) {
		GoToOffset(ullPos);
	}
}

void CHexCtrl::CaretToPageBeg()
{
	if (GetPageSize() == 0)
		return;

	const auto ullPos = GetCaretPos() - (GetCaretPos() % GetPageSize());
	SetCaretPos(ullPos);
	if (!IsOffsetVisible(ullPos)) {
		GoToOffset(ullPos);
	}
}

void CHexCtrl::CaretToPageEnd()
{
	if (GetPageSize() == 0)
		return;

	auto ullPos = GetCaretPos() + (GetPageSize() - (GetCaretPos() % GetPageSize())) - 1;
	if (ullPos >= GetDataSize()) {
		ullPos = GetDataSize() - 1;
	}
	SetCaretPos(ullPos);
	if (!IsOffsetVisible(ullPos)) {
		GoToOffset(ullPos);
	}
}

void CHexCtrl::ChooseFontDlg()
{
	auto lf = GetFont();
	auto stClr = GetColors();
	CHOOSEFONTW chf { .lStructSize { sizeof(CHOOSEFONTW) }, .hwndOwner { m_hWnd }, .lpLogFont { &lf },
		.Flags { CF_EFFECTS | CF_FIXEDPITCHONLY | CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_NOSIMULATIONS },
		.rgbColors { stClr.clrFontHex } };

	if (ChooseFontW(&chf) != FALSE) {
		stClr.clrFontHex = chf.rgbColors;
		SetColors(stClr);
		SetFont(lf);
	}
}

void CHexCtrl::ClipboardCopy(EClipboard eType)const
{
	if (m_pSelection->GetSelSize() > 1024 * 1024 * 8) { //8MB
		::MessageBoxW(nullptr, L"Selection size is too big to copy.\r\nTry selecting less.", L"Error", MB_ICONERROR);
		return;
	}

	std::wstring wstrData { };
	switch (eType) {
	case EClipboard::COPY_HEX:
		wstrData = CopyHex();
		break;
	case EClipboard::COPY_HEXLE:
		wstrData = CopyHexLE();
		break;
	case EClipboard::COPY_HEXFMT:
		wstrData = CopyHexFmt();
		break;
	case EClipboard::COPY_TEXT_CP:
		wstrData = CopyTextCP();
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

	constexpr auto sCharSize { sizeof(wchar_t) };
	const std::size_t sMemSize = wstrData.size() * sCharSize + sCharSize;
	const auto hMem = GlobalAlloc(GMEM_MOVEABLE, sMemSize);
	if (!hMem) {
		DBG_REPORT(L"GlobalAlloc error.");
		return;
	}

	const auto lpMemLock = GlobalLock(hMem);
	if (!lpMemLock) {
		DBG_REPORT(L"GlobalLock error.");
		return;
	}

	std::memcpy(lpMemLock, wstrData.data(), sMemSize);
	GlobalUnlock(hMem);
	::OpenClipboard(m_hWnd);
	EmptyClipboard();
	SetClipboardData(CF_UNICODETEXT, hMem);
	CloseClipboard();
}

void CHexCtrl::ClipboardPaste(EClipboard eType)
{
	if (!IsMutable() || !::OpenClipboard(m_hWnd))
		return;

	const auto hClpbrd = GetClipboardData(CF_UNICODETEXT);
	if (!hClpbrd)
		return;

	const auto pDataClpbrd = static_cast<wchar_t*>(GlobalLock(hClpbrd));
	if (pDataClpbrd == nullptr) {
		CloseClipboard();
		return;
	}

	const auto sSizeClpbrd = wcslen(pDataClpbrd) * sizeof(wchar_t);
	const auto ullDataSize = GetDataSize();
	const auto ullCaretPos = GetCaretPos();
	HEXMODIFY hmd { };
	ULONGLONG ullSizeModify;
	std::string strDataModify; //Actual data to paste, must outlive hmd.
	const auto lmbPasteUTF16 = [&]() {
		ullSizeModify = sSizeClpbrd;
		if (ullCaretPos + sSizeClpbrd > ullDataSize) {
			ullSizeModify = ullDataSize - ullCaretPos;
		}
		hmd.spnData = { reinterpret_cast<std::byte*>(pDataClpbrd), static_cast<std::size_t>(ullSizeModify) };
		};

	switch (eType) {
	case EClipboard::PASTE_TEXT_UTF16:
		lmbPasteUTF16();
		break;
	case EClipboard::PASTE_TEXT_CP:
	{
		auto iCodepage = GetCodepage();
		if (iCodepage == 0) { //UTF-16.
			lmbPasteUTF16();
			break;
		}

		if (iCodepage == -1) { //ASCII.
			iCodepage = 1252;  //ANSI-Latin codepage for default ASCII.
		}
		strDataModify = WstrToStr(pDataClpbrd, iCodepage);
		ullSizeModify = strDataModify.size();
		if (ullCaretPos + ullSizeModify > ullDataSize) {
			ullSizeModify = ullDataSize - ullCaretPos;
		}
		hmd.spnData = { reinterpret_cast<const std::byte*>(strDataModify.data()), static_cast<std::size_t>(ullSizeModify) };
	}
	break;
	case EClipboard::PASTE_HEX:
	{
		auto optData = NumStrToHex(pDataClpbrd);
		if (!optData) {
			GlobalUnlock(hClpbrd);
			CloseClipboard();
			return;
		}
		strDataModify = std::move(*optData);
		ullSizeModify = strDataModify.size();
		if (ullCaretPos + ullSizeModify > ullDataSize) {
			ullSizeModify = ullDataSize - ullCaretPos;
		}
		hmd.spnData = { reinterpret_cast<const std::byte*>(strDataModify.data()), static_cast<std::size_t>(ullSizeModify) };
	}
	break;
	default:
		break;
	}

	hmd.vecSpan.emplace_back(ullCaretPos, ullSizeModify);
	ModifyData(hmd);

	GlobalUnlock(hClpbrd);
	CloseClipboard();
	RedrawWindow();
}

auto CHexCtrl::CopyBase64()const->std::wstring
{
	static constexpr auto pwszBase64Map { L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" };
	const auto ullSelSize = m_pSelection->GetSelSize();
	std::wstring wstrData;
	wstrData.reserve(static_cast<std::size_t>(ullSelSize) * 2);
	auto uValA = 0U;
	auto iValB = -6;
	for (auto i { 0U }; i < ullSelSize; ++i) {
		uValA = (uValA << 8) + GetIHexTData<BYTE>(*this, m_pSelection->GetOffsetByIndex(i));
		iValB += 8;
		while (iValB >= 0) {
			wstrData += pwszBase64Map[(uValA >> iValB) & 0x3F];
			iValB -= 6;
		}
	}

	if (iValB > -6) {
		wstrData += pwszBase64Map[((uValA << 8) >> (iValB + 8)) & 0x3F];
	}
	while (wstrData.size() % 4) {
		wstrData += '=';
	}

	return wstrData;
}

auto CHexCtrl::CopyCArr()const->std::wstring
{
	std::wstring wstrData;
	const auto ullSelSize = m_pSelection->GetSelSize();
	wstrData.reserve(static_cast<std::size_t>(ullSelSize) * 3 + 64);
	wstrData = std::format(L"unsigned char data[{}] = {{\r\n", ullSelSize);

	for (auto i { 0U }; i < ullSelSize; ++i) {
		wstrData += L"0x";
		const auto chByte = GetIHexTData<BYTE>(*this, m_pSelection->GetOffsetByIndex(i));
		wstrData += m_pwszHexChars[(chByte & 0xF0) >> 4];
		wstrData += m_pwszHexChars[(chByte & 0x0F)];
		if (i < ullSelSize - 1) {
			wstrData += L",";
		}

		if ((i + 1) % 16 == 0) {
			wstrData += L"\r\n";
		}
		else {
			wstrData += L" ";
		}
	}
	if (wstrData.back() != '\n') { //To prevent double new line if ullSelSize % 16 == 0
		wstrData += L"\r\n";
	}
	wstrData += L"};";

	return wstrData;
}

auto CHexCtrl::CopyGrepHex()const->std::wstring
{
	std::wstring wstrData;
	const auto ullSelSize = m_pSelection->GetSelSize();

	wstrData.reserve(static_cast<std::size_t>(ullSelSize) * 2);
	for (auto i { 0U }; i < ullSelSize; ++i) {
		wstrData += L"\\x";
		const auto chByte = GetIHexTData<BYTE>(*this, m_pSelection->GetOffsetByIndex(i));
		wstrData += m_pwszHexChars[(chByte & 0xF0) >> 4];
		wstrData += m_pwszHexChars[(chByte & 0x0F)];
	}

	return wstrData;
}

auto CHexCtrl::CopyHex()const->std::wstring
{
	std::wstring wstrData;
	const auto ullSelSize = m_pSelection->GetSelSize();

	wstrData.reserve(static_cast<std::size_t>(ullSelSize) * 2);
	for (auto i { 0U }; i < ullSelSize; ++i) {
		const auto chByte = GetIHexTData<BYTE>(*this, m_pSelection->GetOffsetByIndex(i));
		wstrData += m_pwszHexChars[(chByte & 0xF0) >> 4];
		wstrData += m_pwszHexChars[(chByte & 0x0F)];
	}

	return wstrData;
}

auto CHexCtrl::CopyHexFmt()const->std::wstring
{
	std::wstring wstrData;
	const auto ullSelStart = m_pSelection->GetSelStart();
	const auto ullSelSize = m_pSelection->GetSelSize();
	const auto dwGroupSize = GetGroupSize();
	const auto dwCapacity = GetCapacity();

	wstrData.reserve(static_cast<std::size_t>(ullSelSize) * 3);
	if (m_fSelectionBlock) {
		auto dwTail = m_pSelection->GetLineLength();
		for (auto i { 0U }; i < ullSelSize; ++i) {
			const auto chByte = GetIHexTData<BYTE>(*this, m_pSelection->GetOffsetByIndex(i));
			wstrData += m_pwszHexChars[(chByte & 0xF0) >> 4];
			wstrData += m_pwszHexChars[(chByte & 0x0F)];

			if (i < (ullSelSize - 1) && (dwTail - 1) != 0) {
				if (((m_pSelection->GetLineLength() - dwTail + 1) % dwGroupSize) == 0) { //Add space after hex full chunk, grouping size depending.
					wstrData += L" ";
				}
			}
			if (--dwTail == 0 && i < (ullSelSize - 1)) { //Next row.
				wstrData += L"\r\n";
				dwTail = m_pSelection->GetLineLength();
			}
		}
	}
	else {
		//How many spaces are needed to be inserted at the beginning.
		const DWORD dwModStart = ullSelStart % dwCapacity;

		//When to insert first "\r\n".
		auto dwTail = dwCapacity - dwModStart;
		const DWORD dwNextBlock = (dwCapacity % 2) ? m_dwCapacityBlockSize + 2 : m_dwCapacityBlockSize + 1;

		//If at least two rows are selected.
		if (dwModStart + ullSelSize > dwCapacity) {
			DWORD dwCount = dwModStart * 2 + dwModStart / dwGroupSize;
			//Additional spaces between halves. Only in 1 byte grouping size.
			dwCount += (dwGroupSize == 1) ? (dwTail <= m_dwCapacityBlockSize ? 2 : 0) : 0;
			wstrData.insert(0, static_cast<std::size_t>(dwCount), ' ');
		}

		for (auto i { 0U }; i < ullSelSize; ++i) {
			const auto chByte = GetIHexTData<BYTE>(*this, m_pSelection->GetOffsetByIndex(i));
			wstrData += m_pwszHexChars[(chByte & 0xF0) >> 4];
			wstrData += m_pwszHexChars[(chByte & 0x0F)];

			if (i < (ullSelSize - 1) && (dwTail - 1) != 0) {
				if (dwGroupSize == 1 && dwTail == dwNextBlock) {
					wstrData += L"   "; //Additional spaces between halves. Only in 1 byte grouping size.
				}
				else if (((dwCapacity - dwTail + 1) % dwGroupSize) == 0) { //Add space after hex full chunk, ShowAs_size depending.
					wstrData += L" ";
				}
			}
			if (--dwTail == 0 && i < (ullSelSize - 1)) { //Next row.
				wstrData += L"\r\n";
				dwTail = dwCapacity;
			}
		}
	}

	return wstrData;
}

auto CHexCtrl::CopyHexLE()const->std::wstring
{
	std::wstring wstrData;
	const auto ullSelSize = m_pSelection->GetSelSize();

	wstrData.reserve(static_cast<std::size_t>(ullSelSize) * 2);
	for (auto i = ullSelSize; i > 0; --i) {
		const auto chByte = GetIHexTData<BYTE>(*this, m_pSelection->GetOffsetByIndex(i - 1));
		wstrData += m_pwszHexChars[(chByte & 0xF0) >> 4];
		wstrData += m_pwszHexChars[(chByte & 0x0F)];
	}

	return wstrData;
}

auto CHexCtrl::CopyOffset()const->std::wstring
{
	return (IsOffsetAsHex() ? L"0x" : L"") + OffsetToWstr(GetCaretPos());
}

auto CHexCtrl::CopyPrintScreen()const->std::wstring
{
	if (m_fSelectionBlock) //Only works with classical selection.
		return { };

	const auto ullSelStart = m_pSelection->GetSelStart();
	const auto ullSelSize = m_pSelection->GetSelSize();
	const auto dwCapacity = GetCapacity();

	std::wstring wstrRet { };
	wstrRet.reserve(static_cast<std::size_t>(ullSelSize) * 4);
	wstrRet = L"Offset";
	wstrRet.insert(0, (static_cast<std::size_t>(m_dwOffsetDigits) - wstrRet.size()) / 2, ' ');
	wstrRet.insert(wstrRet.size(), static_cast<std::size_t>(m_dwOffsetDigits) - wstrRet.size(), ' ');
	wstrRet += L"   "; //Spaces to Capacity.
	wstrRet += m_wstrCapacity;
	wstrRet += L"   "; //Spaces to Text.
	if (const auto iSize = static_cast<int>(dwCapacity) - static_cast<int>(m_wstrTextTitle.size()); iSize > 0) {
		wstrRet.insert(wstrRet.size(), static_cast<std::size_t>(iSize / 2), ' ');
	}
	wstrRet += m_wstrTextTitle;
	wstrRet += L"\r\n";

	//How many spaces are needed to be inserted at the beginning.
	DWORD dwModStart = ullSelStart % dwCapacity;

	auto dwLines = static_cast<DWORD>(ullSelSize) / dwCapacity;
	if ((dwModStart + static_cast<DWORD>(ullSelSize)) > dwCapacity) {
		++dwLines;
	}
	if ((ullSelSize % dwCapacity) + dwModStart > dwCapacity) {
		++dwLines;
	}
	if (!dwLines) {
		dwLines = 1;
	}

	const auto ullStartLine = ullSelStart / dwCapacity;
	const auto dwStartOffset = dwModStart; //Offset from the line start in the wstrHex.
	const auto& [wstrHex, wstrText] = BuildDataToDraw(ullStartLine, static_cast<int>(dwLines));
	std::wstring wstrDataText;
	std::size_t sIndexToPrint { 0 };

	for (auto iterLines { 0U }; iterLines < dwLines; ++iterLines) {
		wstrRet += OffsetToWstr(ullStartLine * dwCapacity + dwCapacity * iterLines);
		wstrRet.insert(wstrRet.size(), 3, ' ');

		for (auto iterChunks { 0U }; iterChunks < dwCapacity; ++iterChunks) {
			if (dwModStart == 0 && sIndexToPrint < ullSelSize) {
				wstrRet += wstrHex[(sIndexToPrint + dwStartOffset) * 2];
				wstrRet += wstrHex[(sIndexToPrint + dwStartOffset) * 2 + 1];
				wstrDataText += wstrText[sIndexToPrint + dwStartOffset];
				++sIndexToPrint;
			}
			else {
				wstrRet += L"  ";
				wstrDataText += L" ";
				--dwModStart;
			}

			//Additional space between grouped Hex chunks.
			if (((iterChunks + 1) % GetGroupSize()) == 0 && iterChunks < (dwCapacity - 1)) {
				wstrRet += L" ";
			}

			//Additional space between capacity halves, only in 1 byte grouping size.
			if (GetGroupSize() == 1 && iterChunks == (m_dwCapacityBlockSize - 1)) {
				wstrRet += L"  ";
			}
		}
		wstrRet += L"   "; //Text beginning.
		wstrRet += wstrDataText;
		if (iterLines < dwLines - 1) {
			wstrRet += L"\r\n";
		}
		wstrDataText.clear();
	}

	return wstrRet;
}

auto CHexCtrl::CopyTextCP()const->std::wstring
{
	const auto ullSelSize = m_pSelection->GetSelSize();
	std::string strData;
	strData.reserve(static_cast<std::size_t>(ullSelSize));

	for (auto i = 0; i < ullSelSize; ++i) {
		strData.push_back(GetIHexTData<BYTE>(*this, m_pSelection->GetOffsetByIndex(i)));
	}

	std::wstring wstrText;
	const auto iCodepage = GetCodepage();
	if (iCodepage == -1) { //ASCII codepage: we simply assigning [strData.begin()...strData.end()) to wstrText w/o any conversion.
		wstrText.assign(strData.begin(), strData.end());
	}
	else if (iCodepage == 0) { //UTF-16.
		const auto sSizeWstr = (strData.size() - (strData.size() % 2)) / sizeof(wchar_t);
		const auto pDataUTF16Beg = reinterpret_cast<const wchar_t*>(strData.data());
		const auto pDataUTF16End = pDataUTF16Beg + sSizeWstr;
		wstrText.assign(pDataUTF16Beg, pDataUTF16End);
	}
	else {
		wstrText = StrToWstr(strData, iCodepage);
	}
	ReplaceUnprintable(wstrText, iCodepage == -1, false);

	return wstrText;
}

void CHexCtrl::DrawWindow(CDC* pDC)const
{
	const auto iScrollH = static_cast<int>(m_pScrollH->GetScrollPos());
	CRect rcWnd(m_iFirstVertLinePx, m_iFirstHorzLinePx,
		m_iFirstVertLinePx + m_iWidthClientAreaPx, m_iFirstHorzLinePx + m_iHeightClientAreaPx);

	pDC->FillSolidRect(rcWnd, m_stColors.clrBk);
	pDC->SelectObject(m_penLines);

	//First horizontal line.
	pDC->MoveTo(m_iFirstVertLinePx - iScrollH, m_iFirstHorzLinePx);
	pDC->LineTo(m_iFourthVertLinePx, m_iFirstHorzLinePx);

	//Second horizontal line.
	pDC->MoveTo(m_iFirstVertLinePx - iScrollH, m_iSecondHorzLinePx);
	pDC->LineTo(m_iFourthVertLinePx, m_iSecondHorzLinePx);

	//Third horizontal line.
	pDC->MoveTo(m_iFirstVertLinePx - iScrollH, m_iThirdHorzLinePx);
	pDC->LineTo(m_iFourthVertLinePx, m_iThirdHorzLinePx);

	//Fourth horizontal line.
	pDC->MoveTo(m_iFirstVertLinePx - iScrollH, m_iFourthHorzLinePx);
	pDC->LineTo(m_iFourthVertLinePx, m_iFourthHorzLinePx);

	//First Vertical line.
	pDC->MoveTo(m_iFirstVertLinePx - iScrollH, m_iFirstHorzLinePx);
	pDC->LineTo(m_iFirstVertLinePx - iScrollH, m_iFourthHorzLinePx);

	//Second Vertical line.
	pDC->MoveTo(m_iSecondVertLinePx - iScrollH, m_iFirstHorzLinePx);
	pDC->LineTo(m_iSecondVertLinePx - iScrollH, m_iThirdHorzLinePx);

	//Third Vertical line.
	pDC->MoveTo(m_iThirdVertLinePx - iScrollH, m_iFirstHorzLinePx);
	pDC->LineTo(m_iThirdVertLinePx - iScrollH, m_iThirdHorzLinePx);

	//Fourth Vertical line.
	pDC->MoveTo(m_iFourthVertLinePx - iScrollH, m_iFirstHorzLinePx);
	pDC->LineTo(m_iFourthVertLinePx - iScrollH, m_iFourthHorzLinePx);

	//«Offset» text.
	CRect rcOffset(m_iFirstVertLinePx - iScrollH, m_iFirstHorzLinePx, m_iSecondVertLinePx - iScrollH, m_iSecondHorzLinePx);
	pDC->SelectObject(m_fontMain);
	pDC->SetTextColor(m_stColors.clrFontCaption);
	pDC->SetBkColor(m_stColors.clrBk);
	pDC->DrawTextW(L"Offset", 6, rcOffset, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	//Capacity numbers.
	ExtTextOutW(pDC->m_hDC, m_iIndentFirstHexChunkXPx - iScrollH, m_iFirstHorzLinePx + m_iIndentCapTextYPx, 0, nullptr,
		m_wstrCapacity.data(), static_cast<UINT>(m_wstrCapacity.size()), GetCharsWidthArray());

	//Text area caption.
	auto rcText = GetRectTextCaption();
	pDC->DrawTextW(m_wstrTextTitle.data(), static_cast<int>(m_wstrTextTitle.size()), rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void CHexCtrl::DrawInfoBar(CDC* pDC)const
{
	if (!IsInfoBar())
		return;

	const auto iScrollH = static_cast<int>(m_pScrollH->GetScrollPos());
	CRect rcInfoBar(m_iFirstVertLinePx + 1 - iScrollH, m_iThirdHorzLinePx + 1, m_iFourthVertLinePx, m_iFourthHorzLinePx); //Info bar rc until m_iFourthHorizLine.
	pDC->FillSolidRect(rcInfoBar, m_stColors.clrBkInfoBar);
	pDC->DrawEdge(rcInfoBar, BDR_RAISEDINNER, BF_TOP);

	CRect rcText = rcInfoBar;
	rcText.left = m_iFirstVertLinePx + 5; //Draw the text beginning with little indent.
	rcText.right = m_iFirstVertLinePx + m_iWidthClientAreaPx; //Draw text to the end of the client area, even if it passes iFourthHorizLine.
	pDC->SelectObject(m_fontInfoBar);
	pDC->SetBkColor(m_stColors.clrBkInfoBar);

	std::size_t sCurrPosBegin { };
	std::size_t sCurrPosEnd { };
	while (sCurrPosBegin != std::wstring::npos) {
		if (sCurrPosBegin = m_wstrInfoBar.find_first_of('^', sCurrPosBegin); sCurrPosBegin != std::wstring::npos) {
			if (sCurrPosEnd = m_wstrInfoBar.find_first_of('^', sCurrPosBegin + 1); sCurrPosEnd != std::wstring::npos) {
				const auto iSize = static_cast<int>(sCurrPosEnd - sCurrPosBegin - 1);
				pDC->SetTextColor(m_stColors.clrFontInfoParam);
				pDC->DrawTextW(m_wstrInfoBar.data() + sCurrPosBegin + 1, iSize, rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
				rcText.left += iSize * m_sizeFontInfo.cx;
			}
			sCurrPosBegin = sCurrPosEnd + 1;
		}

		if (sCurrPosBegin = m_wstrInfoBar.find_first_of('`', sCurrPosBegin); sCurrPosBegin != std::wstring::npos) {
			if (sCurrPosEnd = m_wstrInfoBar.find_first_of('`', sCurrPosBegin + 1); sCurrPosEnd != std::wstring::npos) {
				const auto size = sCurrPosEnd - sCurrPosBegin - 1;
				pDC->SetTextColor(m_stColors.clrFontInfoData);
				pDC->DrawTextW(m_wstrInfoBar.data() + sCurrPosBegin + 1, static_cast<int>(size), rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
				rcText.left += static_cast<long>(size * m_sizeFontInfo.cx);
			}
			sCurrPosBegin = sCurrPosEnd + 1;
		}
	}
}

void CHexCtrl::DrawOffsets(CDC* pDC, ULONGLONG ullStartLine, int iLines)const
{
	const auto dwCapacity = GetCapacity();
	const auto ullStartOffset = ullStartLine * dwCapacity;
	const auto iScrollH = static_cast<int>(m_pScrollH->GetScrollPos());

	for (auto iterLines = 0; iterLines < iLines; ++iterLines) {
		//Drawing offset with bk color depending on the selection range.
		HEXCOLOR stClrOffset;
		if (m_pSelection->HitTestRange({ ullStartOffset + iterLines * dwCapacity, dwCapacity })) {
			stClrOffset.clrBk = m_stColors.clrBkSel;
			stClrOffset.clrText = m_stColors.clrFontSel;
		}
		else {
			stClrOffset.clrBk = m_stColors.clrBk;
			stClrOffset.clrText = m_stColors.clrFontCaption;
		}

		//Left column offset printing (00000000...0000FFFF).
		pDC->SelectObject(m_fontMain);
		pDC->SetTextColor(stClrOffset.clrText);
		pDC->SetBkColor(stClrOffset.clrBk);
		ExtTextOutW(pDC->m_hDC, m_iFirstVertLinePx + GetCharWidthNative() - iScrollH, m_iStartWorkAreaYPx + (m_sizeFontMain.cy * iterLines),
			0, nullptr, OffsetToWstr((ullStartLine + iterLines) * dwCapacity).data(), m_dwOffsetDigits, nullptr);
	}
}

void CHexCtrl::DrawHexText(CDC* pDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const
{
	struct POLYTEXTCLR {
		POLYTEXTW stPoly { };
		HEXCOLOR stClr;
	};

	std::vector<POLYTEXTCLR> vecPolyHex;
	std::vector<POLYTEXTCLR> vecPolyText;
	std::vector<std::unique_ptr<std::wstring>> vecWstrHex; //unique_ptr to avoid wstring.data() invalidation.
	std::vector<std::unique_ptr<std::wstring>> vecWstrText;
	vecWstrHex.reserve(static_cast<std::size_t>(iLines));
	vecWstrText.reserve(static_cast<std::size_t>(iLines));
	const auto ullStartOffset = ullStartLine * GetCapacity();
	std::size_t sIndexToPrint { 0 };
	HEXCOLORINFO hci { .hdr { m_hWnd, static_cast<UINT>(GetDlgCtrlID()) },
		.stClr { .clrBk { m_stColors.clrBk }, .clrText { m_stColors.clrFontHex } } };

	for (auto iterLines = 0; iterLines < iLines; ++iterLines) {
		std::wstring wstrHexToPrint;
		std::wstring wstrTextToPrint;
		int iHexPosToPrintX { };
		int iTextPosToPrintX { };
		bool fNeedChunkPoint { true }; //For just one time exec.
		const auto iPosToPrintY = m_iStartWorkAreaYPx + m_sizeFontMain.cy * iterLines; //Hex and Text are the same.
		HEXCOLOR stHexClr { }; //Current Hex area color.
		HEXCOLOR stTextClr { .clrText { m_stColors.clrFontText } }; //Current Text area color.
		const auto lmbPoly = [&]() {
			if (wstrHexToPrint.empty())
				return;

			//Hex colors Poly.
			vecWstrHex.emplace_back(std::make_unique<std::wstring>(std::move(wstrHexToPrint)));
			vecPolyHex.emplace_back(POLYTEXTW { iHexPosToPrintX, iPosToPrintY,
				static_cast<UINT>(vecWstrHex.back()->size()), vecWstrHex.back()->data(), 0, { }, GetCharsWidthArray() },
				stHexClr);

			//Text colors Poly.
			vecWstrText.emplace_back(std::make_unique<std::wstring>(std::move(wstrTextToPrint)));
			vecPolyText.emplace_back(POLYTEXTW { iTextPosToPrintX, iPosToPrintY,
				static_cast<UINT>(vecWstrText.back()->size()), vecWstrText.back()->data(), 0, { }, GetCharsWidthArray() },
				stTextClr);
			};
		const auto lmbHexSpaces = [&](const unsigned iterChunks) {
			if (wstrHexToPrint.empty()) //Only adding spaces if there are chars beforehead.
				return;

			if ((iterChunks % GetGroupSize()) == 0) {
				wstrHexToPrint += L' ';
			}

			//Additional space between capacity halves, only in 1 byte grouping size.
			if (GetGroupSize() == 1 && iterChunks == m_dwCapacityBlockSize) {
				wstrHexToPrint += L"  ";
			}
			};

		//Main loop for printing Hex chunks and Text chars.
		for (auto iterChunks { 0U }; iterChunks < GetCapacity() && sIndexToPrint < wsvText.size(); ++iterChunks, ++sIndexToPrint) {
			if (m_pHexVirtColors != nullptr) {
				hci.ullOffset = ullStartOffset + sIndexToPrint;
				if (m_pHexVirtColors->OnHexGetColor(hci)) {
					stTextClr = hci.stClr; //Text area color will be equal to the Hex area color.
				}
				else {
					hci.stClr.clrBk = m_stColors.clrBk;
					hci.stClr.clrText = m_stColors.clrFontHex;
					stTextClr.clrText = m_stColors.clrFontText;
				}
			}

			if (stHexClr != hci.stClr) { //If it's a different color.
				lmbHexSpaces(iterChunks);
				lmbPoly();
				fNeedChunkPoint = true;
				stHexClr = hci.stClr;
			}

			if (fNeedChunkPoint) {
				int iCy;
				HexChunkPoint(sIndexToPrint, iHexPosToPrintX, iCy);
				TextChunkPoint(sIndexToPrint, iTextPosToPrintX, iCy);
				fNeedChunkPoint = false;
			}

			lmbHexSpaces(iterChunks);
			wstrHexToPrint += wsvHex[sIndexToPrint * 2];
			wstrHexToPrint += wsvHex[sIndexToPrint * 2 + 1];
			wstrTextToPrint += wsvText[sIndexToPrint];
		}

		lmbPoly();
	}

	pDC->SelectObject(m_fontMain);
	std::size_t index { 0 }; //Index for vecPolyText, its size is always equal to vecPolyHex.
	for (const auto& iter : vecPolyHex) { //Loop is needed because of different colors.
		pDC->SetTextColor(iter.stClr.clrText);
		pDC->SetBkColor(iter.stClr.clrBk);
		const auto& refH = iter.stPoly;
		ExtTextOutW(pDC->m_hDC, refH.x, refH.y, refH.uiFlags, &refH.rcl, refH.lpstr, refH.n, refH.pdx); //Hex printing.
		const auto& refVecText = vecPolyText[index++];
		pDC->SetTextColor(refVecText.stClr.clrText); //Text color for the Text area.
		const auto& refT = refVecText.stPoly;
		ExtTextOutW(pDC->m_hDC, refT.x, refT.y, refT.uiFlags, &refT.rcl, refT.lpstr, refT.n, refT.pdx); //Text printing.
	}
}

void CHexCtrl::DrawTemplates(CDC* pDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const
{
	if (!m_pDlgTemplMgr->HasApplied())
		return;

	struct POLYFIELDSCLR { //Struct for fields.
		POLYTEXTW stPoly { };
		HEXCOLOR stClr;
		//Flag to avoid print vert line at the beginnig of the line if this line is just a continuation
		//of the previous line above.
		bool      fPrintVertLine { true };
	};
	std::vector<POLYFIELDSCLR> vecFieldsHex;
	std::vector<POLYFIELDSCLR> vecFieldsText;
	std::vector<std::unique_ptr<std::wstring>> vecWstrFieldsHex; //unique_ptr to avoid wstring ptr invalidation.
	std::vector<std::unique_ptr<std::wstring>> vecWstrFieldsText;
	const auto ullStartOffset = ullStartLine * GetCapacity();
	std::size_t sIndexToPrint { };
	PCHEXTEMPLFIELD pFieldCurr { };

	for (auto iterLines = 0; iterLines < iLines; ++iterLines) {
		std::wstring wstrHexFieldToPrint;
		std::wstring wstrTextFieldToPrint;
		int iFieldHexPosToPrintX { };
		int iFieldTextPosToPrintX { };
		bool fNeedChunkPoint { true };
		bool fField { false };  //Flag to show current Field in current Hex presence.
		bool fPrintVertLine { true };
		const auto iPosToPrintY = m_iStartWorkAreaYPx + m_sizeFontMain.cy * iterLines; //Hex and Text are the same.
		const auto lmbPoly = [&]() {
			if (wstrHexFieldToPrint.empty())
				return;

			//Hex Fields Poly.
			vecWstrFieldsHex.emplace_back(std::make_unique<std::wstring>(std::move(wstrHexFieldToPrint)));
			vecFieldsHex.emplace_back(POLYTEXTW { iFieldHexPosToPrintX, iPosToPrintY,
				static_cast<UINT>(vecWstrFieldsHex.back()->size()), vecWstrFieldsHex.back()->data(), 0, { }, GetCharsWidthArray() },
				pFieldCurr->stClr, fPrintVertLine);

			//Text Fields Poly.
			vecWstrFieldsText.emplace_back(std::make_unique<std::wstring>(std::move(wstrTextFieldToPrint)));
			vecFieldsText.emplace_back(POLYTEXTW { iFieldTextPosToPrintX, iPosToPrintY,
				static_cast<UINT>(vecWstrFieldsText.back()->size()), vecWstrFieldsText.back()->data(), 0, { }, GetCharsWidthArray() },
				pFieldCurr->stClr, fPrintVertLine);

			fPrintVertLine = true;
			};
		const auto lmbHexSpaces = [&](const unsigned iterChunks) {
			if (wstrHexFieldToPrint.empty()) //Only adding spaces if there are chars beforehead.
				return;

			if ((iterChunks % GetGroupSize()) == 0) {
				wstrHexFieldToPrint += L' ';
			}

			//Additional space between capacity halves, only in 1 byte grouping size.
			if (GetGroupSize() == 1 && iterChunks == m_dwCapacityBlockSize) {
				wstrHexFieldToPrint += L"  ";
			}
			};

		//Main loop for printing Hex chunks and Text chars.
		for (auto iterChunks { 0U }; iterChunks < GetCapacity() && sIndexToPrint < wsvText.size(); ++iterChunks, ++sIndexToPrint) {
			//Fields.
			if (auto pField = m_pDlgTemplMgr->HitTest(ullStartOffset + sIndexToPrint); pField != nullptr) {
				if (iterChunks == 0 && pField == pFieldCurr) {
					fPrintVertLine = false;
				}

				//If it's nested Field.
				if (pFieldCurr != nullptr && pFieldCurr != pField) {
					lmbHexSpaces(iterChunks);
					lmbPoly();
					fNeedChunkPoint = true;
				}

				pFieldCurr = pField;

				if (fNeedChunkPoint) {
					int iCy;
					HexChunkPoint(sIndexToPrint, iFieldHexPosToPrintX, iCy);
					TextChunkPoint(sIndexToPrint, iFieldTextPosToPrintX, iCy);
					fNeedChunkPoint = false;
				}

				lmbHexSpaces(iterChunks);
				wstrHexFieldToPrint += wsvHex[sIndexToPrint * 2];
				wstrHexFieldToPrint += wsvHex[sIndexToPrint * 2 + 1];
				wstrTextFieldToPrint += wsvText[sIndexToPrint];
				fField = true;
			}
			else if (fField) {
				//There can be multiple Fields in one line. 
				//So, if there already were Field bytes in the current line, we Poly them.
				//Same Poly mechanism presents at the end of the current (iterLines) loop,
				//to Poly Fields that end at the line's end.

				lmbPoly();
				fNeedChunkPoint = true;
				fField = false;
				pFieldCurr = nullptr;
			}
		}

		lmbPoly(); //Fields Poly.
		pFieldCurr = nullptr;
	}

	//Fieds printing.
	if (!vecFieldsHex.empty()) {
		pDC->SelectObject(m_fontMain);
		std::size_t index { 0 }; //Index for vecFieldsText, its size is always equal to vecFieldsHex.
		const auto penOld = SelectObject(pDC->m_hDC, m_penDataTempl);
		for (const auto& iter : vecFieldsHex) { //Loop is needed because different Fields can have different colors.
			pDC->SetTextColor(iter.stClr.clrText);
			pDC->SetBkColor(iter.stClr.clrBk);

			const auto& refH = iter.stPoly;
			ExtTextOutW(pDC->m_hDC, refH.x, refH.y, refH.uiFlags, &refH.rcl, refH.lpstr, refH.n, refH.pdx); //Hex Field printing.
			const auto& refT = vecFieldsText[index++].stPoly;
			ExtTextOutW(pDC->m_hDC, refT.x, refT.y, refT.uiFlags, &refT.rcl, refT.lpstr, refT.n, refT.pdx); //Text Field printing.

			if (iter.fPrintVertLine) {
				const auto iFieldLineHexX = refH.x - 2; //Little indent before vert line.
				MoveToEx(pDC->m_hDC, iFieldLineHexX, refH.y, nullptr);
				LineTo(pDC->m_hDC, iFieldLineHexX, refH.y + m_sizeFontMain.cy);

				const auto iFieldLineTextX = refT.x;
				MoveToEx(pDC->m_hDC, iFieldLineTextX, refT.y, nullptr);
				LineTo(pDC->m_hDC, iFieldLineTextX, refT.y + m_sizeFontMain.cy);
			}

			if (index == vecFieldsHex.size()) { //Last vertical Field line.
				const auto iFieldLastLineHexX = refH.x + refH.n * GetCharWidthExtras() + 1; //Little indent after vert line.
				MoveToEx(pDC->m_hDC, iFieldLastLineHexX, refH.y, nullptr);
				LineTo(pDC->m_hDC, iFieldLastLineHexX, refH.y + m_sizeFontMain.cy);

				const auto iFieldLastLineTextX = refT.x + refT.n * GetCharWidthExtras();
				MoveToEx(pDC->m_hDC, iFieldLastLineTextX, refT.y, nullptr);
				LineTo(pDC->m_hDC, iFieldLastLineTextX, refT.y + m_sizeFontMain.cy);
			}
		}
		SelectObject(pDC->m_hDC, penOld);
	}
}

void CHexCtrl::DrawBookmarks(CDC* pDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const
{
	if (!m_pDlgBkmMgr->HasBookmarks())
		return;

	struct POLYTEXTCLR { //Struct for Bookmarks.
		POLYTEXTW stPoly { };
		HEXCOLOR stClr;
	};

	std::vector<POLYTEXTCLR> vecBkmHex;
	std::vector<POLYTEXTCLR> vecBkmText;
	std::vector<std::unique_ptr<std::wstring>> vecWstrBkmHex; //unique_ptr to avoid wstring ptr invalidation.
	std::vector<std::unique_ptr<std::wstring>> vecWstrBkmText;
	const auto ullStartOffset = ullStartLine * GetCapacity();
	std::size_t sIndexToPrint { };

	for (auto iterLines = 0; iterLines < iLines; ++iterLines) {
		std::wstring wstrHexBkmToPrint;
		std::wstring wstrTextBkmToPrint;
		int iBkmHexPosToPrintX { };
		int iBkmTextPosToPrintX { };
		bool fNeedChunkPoint { true };
		bool fBookmark { false };  //Flag to show current Bookmark in current Hex presence.
		PHEXBKM pBkmCurr { };
		const auto iPosToPrintY = m_iStartWorkAreaYPx + m_sizeFontMain.cy * iterLines; //Hex and Text are the same.
		const auto lmbPoly = [&]() {
			if (wstrHexBkmToPrint.empty())
				return;

			//Hex bookmarks Poly.
			vecWstrBkmHex.emplace_back(std::make_unique<std::wstring>(std::move(wstrHexBkmToPrint)));
			vecBkmHex.emplace_back(POLYTEXTW { iBkmHexPosToPrintX, iPosToPrintY,
				static_cast<UINT>(vecWstrBkmHex.back()->size()), vecWstrBkmHex.back()->data(), 0, { }, GetCharsWidthArray() },
				pBkmCurr->stClr);

			//Text bookmarks Poly.
			vecWstrBkmText.emplace_back(std::make_unique<std::wstring>(std::move(wstrTextBkmToPrint)));
			vecBkmText.emplace_back(POLYTEXTW { iBkmTextPosToPrintX, iPosToPrintY,
				static_cast<UINT>(vecWstrBkmText.back()->size()), vecWstrBkmText.back()->data(), 0, { }, GetCharsWidthArray() },
				pBkmCurr->stClr);
			};
		const auto lmbHexSpaces = [&](const unsigned iterChunks) {
			if (wstrHexBkmToPrint.empty()) //Only adding spaces if there are chars beforehead.
				return;

			if ((iterChunks % GetGroupSize()) == 0) {
				wstrHexBkmToPrint += L' ';
			}

			//Additional space between capacity halves, only in 1 byte grouping size.
			if (GetGroupSize() == 1 && iterChunks == m_dwCapacityBlockSize) {
				wstrHexBkmToPrint += L"  ";
			}
			};

		//Main loop for printing Hex chunks and Text chars.
		for (auto iterChunks { 0U }; iterChunks < GetCapacity() && sIndexToPrint < wsvText.size(); ++iterChunks, ++sIndexToPrint) {
			//Bookmarks.
			if (const auto pBkm = m_pDlgBkmMgr->HitTest(ullStartOffset + sIndexToPrint); pBkm != nullptr) {
				//If it's nested bookmark.
				if (pBkmCurr != nullptr && pBkmCurr != pBkm) {
					lmbHexSpaces(iterChunks);
					lmbPoly();
					fNeedChunkPoint = true;
				}

				pBkmCurr = pBkm;

				if (fNeedChunkPoint) {
					int iCy;
					HexChunkPoint(sIndexToPrint, iBkmHexPosToPrintX, iCy);
					TextChunkPoint(sIndexToPrint, iBkmTextPosToPrintX, iCy);
					fNeedChunkPoint = false;
				}

				lmbHexSpaces(iterChunks);
				wstrHexBkmToPrint += wsvHex[sIndexToPrint * 2];
				wstrHexBkmToPrint += wsvHex[sIndexToPrint * 2 + 1];
				wstrTextBkmToPrint += wsvText[sIndexToPrint];
				fBookmark = true;
			}
			else if (fBookmark) {
				//There can be multiple bookmarks in one line. 
				//So, if there already were bookmarked bytes in the current line, we Poly them.
				//Same Poly mechanism presents at the end of the current (iterLines) loop,
				//to Poly bookmarks that end at the line's end.

				lmbPoly();
				fNeedChunkPoint = true;
				fBookmark = false;
				pBkmCurr = nullptr;
			}
		}

		lmbPoly(); //Bookmarks Poly.
		pBkmCurr = nullptr;
	}

	//Bookmarks printing.
	if (!vecBkmHex.empty()) {
		pDC->SelectObject(m_fontMain);
		std::size_t index { 0 }; //Index for vecBkmText, its size is always equal to vecBkmHex.
		for (const auto& iter : vecBkmHex) { //Loop is needed because bkms have different colors.
			pDC->SetTextColor(iter.stClr.clrText);
			pDC->SetBkColor(iter.stClr.clrBk);
			const auto& refH = iter.stPoly;
			ExtTextOutW(pDC->m_hDC, refH.x, refH.y, refH.uiFlags, &refH.rcl, refH.lpstr, refH.n, refH.pdx); //Hex bookmarks printing.
			const auto& refT = vecBkmText[index++].stPoly;
			ExtTextOutW(pDC->m_hDC, refT.x, refT.y, refT.uiFlags, &refT.rcl, refT.lpstr, refT.n, refT.pdx); //Text bookmarks printing.
		}
	}
}

void CHexCtrl::DrawSelection(CDC* pDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const
{
	if (!HasSelection())
		return;

	std::vector<POLYTEXTW> vecPolySelHex;
	std::vector<POLYTEXTW> vecPolySelText;
	std::vector<std::unique_ptr<std::wstring>> vecWstrSel; //unique_ptr to avoid wstring ptr invalidation.
	const auto ullStartOffset = ullStartLine * GetCapacity();
	std::size_t sIndexToPrint { };

	for (auto iterLines = 0; iterLines < iLines; ++iterLines) {
		std::wstring wstrHexSelToPrint; //Selected Hex and Text strings to print.
		std::wstring wstrTextSelToPrint;
		int iSelHexPosToPrintX { };
		int iSelTextPosToPrintX { };
		bool fNeedChunkPoint { true };
		bool fSelection { false };
		const auto iPosToPrintY = m_iStartWorkAreaYPx + m_sizeFontMain.cy * iterLines; //Hex and Text are the same.
		const auto lmbPoly = [&]() {
			if (wstrHexSelToPrint.empty())
				return;

			//Hex selection Poly.
			vecWstrSel.emplace_back(std::make_unique<std::wstring>(std::move(wstrHexSelToPrint)));
			vecPolySelHex.emplace_back(iSelHexPosToPrintX, iPosToPrintY,
				static_cast<UINT>(vecWstrSel.back()->size()), vecWstrSel.back()->data(), 0, RECT { }, GetCharsWidthArray());

			//Text selection Poly.
			vecWstrSel.emplace_back(std::make_unique<std::wstring>(std::move(wstrTextSelToPrint)));
			vecPolySelText.emplace_back(iSelTextPosToPrintX, iPosToPrintY,
				static_cast<UINT>(vecWstrSel.back()->size()), vecWstrSel.back()->data(), 0, RECT { }, GetCharsWidthArray());
			};

		//Main loop for printing Hex chunks and Text chars.
		for (auto iterChunks { 0U }; iterChunks < GetCapacity() && sIndexToPrint < wsvText.size(); ++iterChunks, ++sIndexToPrint) {
			//Selection.
			if (m_pSelection->HitTest(ullStartOffset + sIndexToPrint)) {
				if (fNeedChunkPoint) {
					int iCy;
					HexChunkPoint(sIndexToPrint, iSelHexPosToPrintX, iCy);
					TextChunkPoint(sIndexToPrint, iSelTextPosToPrintX, iCy);
					fNeedChunkPoint = false;
				}

				if (!wstrHexSelToPrint.empty()) { //Only adding spaces if there are chars beforehead.
					if ((iterChunks % GetGroupSize()) == 0) {
						wstrHexSelToPrint += L' ';
					}

					//Additional space between capacity halves, only in 1 byte grouping size.
					if (GetGroupSize() == 1 && iterChunks == m_dwCapacityBlockSize) {
						wstrHexSelToPrint += L"  ";
					}
				}
				wstrHexSelToPrint += wsvHex[sIndexToPrint * 2];
				wstrHexSelToPrint += wsvHex[sIndexToPrint * 2 + 1];
				wstrTextSelToPrint += wsvText[sIndexToPrint];
				fSelection = true;
			}
			else if (fSelection) {
				//There can be multiple selections in one line. 
				//All the same as for bookmarks.

				lmbPoly();
				fNeedChunkPoint = true;
				fSelection = false;
			}
		}

		lmbPoly(); //Selection Poly.
	}

	//Selection printing.
	if (!vecPolySelHex.empty()) {
		pDC->SelectObject(m_fontMain);
		pDC->SetTextColor(m_stColors.clrFontSel);
		pDC->SetBkColor(m_stColors.clrBkSel);
		PolyTextOutW(pDC->m_hDC, vecPolySelHex.data(), static_cast<UINT>(vecPolySelHex.size())); //Hex selection printing.
		for (const auto& iter : vecPolySelText) {
			ExtTextOutW(pDC->m_hDC, iter.x, iter.y, iter.uiFlags, &iter.rcl, iter.lpstr, iter.n, iter.pdx); //Text selection printing.
		}
	}
}

void CHexCtrl::DrawSelHighlight(CDC* pDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const
{
	if (!m_pSelection->HasSelHighlight())
		return;

	std::vector<POLYTEXTW> vecPolySelHexHgl;
	std::vector<POLYTEXTW> vecPolySelTextHgl;
	std::vector<std::unique_ptr<std::wstring>> vecWstrSelHgl; //unique_ptr to avoid wstring ptr invalidation.
	const auto ullStartOffset = ullStartLine * GetCapacity();
	std::size_t sIndexToPrint { };

	for (auto iterLines = 0; iterLines < iLines; ++iterLines) {
		std::wstring wstrHexSelToPrint; //Selected Hex and Text strings to print.
		std::wstring wstrTextSelToPrint;
		int iSelHexPosToPrintX { };
		int iSelTextPosToPrintX { };
		bool fNeedChunkPoint { true };
		bool fSelection { false };
		const auto iPosToPrintY = m_iStartWorkAreaYPx + m_sizeFontMain.cy * iterLines; //Hex and Text are the same.
		const auto lmbPoly = [&]() {
			if (wstrHexSelToPrint.empty())
				return;

			//Hex selection highlight Poly.
			vecWstrSelHgl.emplace_back(std::make_unique<std::wstring>(std::move(wstrHexSelToPrint)));
			vecPolySelHexHgl.emplace_back(iSelHexPosToPrintX, iPosToPrintY,
				static_cast<UINT>(vecWstrSelHgl.back()->size()), vecWstrSelHgl.back()->data(), 0, RECT { }, GetCharsWidthArray());

			//Text selection highlight Poly.
			vecWstrSelHgl.emplace_back(std::make_unique<std::wstring>(std::move(wstrTextSelToPrint)));
			vecPolySelTextHgl.emplace_back(iSelTextPosToPrintX, iPosToPrintY,
				static_cast<UINT>(vecWstrSelHgl.back()->size()), vecWstrSelHgl.back()->data(), 0, RECT { }, GetCharsWidthArray());
			};

		//Main loop for printing Hex chunks and Text chars.
		for (auto iterChunks { 0U }; iterChunks < GetCapacity() && sIndexToPrint < wsvText.size(); ++iterChunks, ++sIndexToPrint) {
			//Selection highlights.
			if (m_pSelection->HitTestHighlight(ullStartOffset + sIndexToPrint)) {
				if (fNeedChunkPoint) {
					int iCy;
					HexChunkPoint(sIndexToPrint, iSelHexPosToPrintX, iCy);
					TextChunkPoint(sIndexToPrint, iSelTextPosToPrintX, iCy);
					fNeedChunkPoint = false;
				}

				if (!wstrHexSelToPrint.empty()) { //Only adding spaces if there are chars beforehead.
					if ((iterChunks % GetGroupSize()) == 0) {
						wstrHexSelToPrint += L' ';
					}

					//Additional space between capacity halves, only in 1 byte grouping size.
					if (GetGroupSize() == 1 && iterChunks == m_dwCapacityBlockSize) {
						wstrHexSelToPrint += L"  ";
					}
				}
				wstrHexSelToPrint += wsvHex[sIndexToPrint * 2];
				wstrHexSelToPrint += wsvHex[sIndexToPrint * 2 + 1];
				wstrTextSelToPrint += wsvText[sIndexToPrint];
				fSelection = true;
			}
			else if (fSelection) {
				//There can be multiple selection highlights in one line. 
				//All the same as for bookmarks.

				lmbPoly();
				fNeedChunkPoint = true;
				fSelection = false;
			}
		}

		lmbPoly(); //Selection highlight Poly.
	}

	//Selection highlight printing.
	if (!vecPolySelHexHgl.empty()) {
		//Colors are the inverted selection colors.
		pDC->SelectObject(m_fontMain);
		pDC->SetTextColor(m_stColors.clrBkSel);
		pDC->SetBkColor(m_stColors.clrFontSel);
		PolyTextOutW(pDC->m_hDC, vecPolySelHexHgl.data(), static_cast<UINT>(vecPolySelHexHgl.size())); //Hex selection highlight printing.
		for (const auto& iter : vecPolySelTextHgl) {
			ExtTextOutW(pDC->m_hDC, iter.x, iter.y, iter.uiFlags, &iter.rcl, iter.lpstr, iter.n, iter.pdx); //Text selection highlight printing.
		}
	}
}

void CHexCtrl::DrawCaret(CDC* pDC, ULONGLONG ullStartLine, std::wstring_view wsvHex, std::wstring_view wsvText)const
{
	const auto ullCaretPos = GetCaretPos();

	//If caret is higher or lower of the visible area we don't draw it.
	//Otherwise, caret of the Hex area can be hidden but caret of the Text area can be visible,
	//or vice-versa, so we proceed drawing it.
	if (IsOffsetVisible(ullCaretPos).i8Vert != 0)
		return;

	int iCaretHexPosToPrintX;
	int iCaretTextPosToPrintX;
	int iCaretHexPosToPrintY;
	int iCaretTextPosToPrintY;
	HexChunkPoint(ullCaretPos, iCaretHexPosToPrintX, iCaretHexPosToPrintY);
	TextChunkPoint(ullCaretPos, iCaretTextPosToPrintX, iCaretTextPosToPrintY);

	const auto sIndexToPrint = static_cast<std::size_t>(ullCaretPos - (ullStartLine * GetCapacity()));
	std::wstring wstrHexCaretToPrint;
	std::wstring wstrTextCaretToPrint;
	if (m_fCaretHigh) {
		wstrHexCaretToPrint = wsvHex[sIndexToPrint * 2];
	}
	else {
		wstrHexCaretToPrint = wsvHex[sIndexToPrint * 2 + 1];
		iCaretHexPosToPrintX += GetCharWidthExtras();
	}
	wstrTextCaretToPrint = wsvText[sIndexToPrint];

	POLYTEXTW arrPolyCaret[2]; //Caret Poly array.

	//Hex Caret Poly.
	arrPolyCaret[0] = { iCaretHexPosToPrintX, iCaretHexPosToPrintY,
		static_cast<UINT>(wstrHexCaretToPrint.size()), wstrHexCaretToPrint.data(), 0, RECT { }, GetCharsWidthArray() };

	//Text Caret Poly.
	arrPolyCaret[1] = { iCaretTextPosToPrintX, iCaretTextPosToPrintY,
		static_cast<UINT>(wstrTextCaretToPrint.size()), wstrTextCaretToPrint.data(), 0, RECT { }, GetCharsWidthArray() };

	//Caret color.
	const auto clrBkCaret = m_pSelection->HitTest(ullCaretPos) ? m_stColors.clrBkCaretSel : m_stColors.clrBkCaret;

	//Caret printing.
	pDC->SelectObject(m_fontMain);
	pDC->SetTextColor(m_stColors.clrFontCaret);
	pDC->SetBkColor(clrBkCaret);
	for (const auto iter : arrPolyCaret) {
		ExtTextOutW(pDC->m_hDC, iter.x, iter.y, iter.uiFlags, &iter.rcl, iter.lpstr, iter.n, iter.pdx); //Hex/Text caret printing.
	}
}

void CHexCtrl::DrawDataInterp(CDC* pDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const
{
	if (!m_pDlgDataInterp->HasHighlight())
		return;

	const auto dwHglSize = m_pDlgDataInterp->GetHglDataSize();
	const auto ullCaretPos = GetCaretPos();
	if ((ullCaretPos + dwHglSize) > GetDataSize())
		return;

	std::vector<POLYTEXTW> vecPolyDataInterp;
	std::vector<std::unique_ptr<std::wstring>> vecWstrDataInterp;
	const auto ullStartOffset = ullStartLine * GetCapacity();
	std::size_t sIndexToPrint { };

	for (auto iterLines = 0; iterLines < iLines; ++iterLines) {
		std::wstring wstrHexDataInterpToPrint; //Data Interpreter Hex and Text strings to print.
		std::wstring wstrTextDataInterpToPrint;
		int iDataInterpHexPosToPrintX { }; //Data Interpreter X coords.
		int iDataInterpTextPosToPrintX { };
		bool fNeedChunkPoint { true };
		const auto iPosToPrintY = m_iStartWorkAreaYPx + m_sizeFontMain.cy * iterLines; //Hex and Text are the same.

		//Main loop for printing Hex chunks and Text chars.
		for (auto iterChunks { 0U }; iterChunks < GetCapacity() && sIndexToPrint < wsvText.size(); ++iterChunks, ++sIndexToPrint) {
			const auto ullOffsetCurr = ullStartOffset + sIndexToPrint;
			if (ullOffsetCurr >= ullCaretPos && ullOffsetCurr < (ullCaretPos + dwHglSize)) {
				if (fNeedChunkPoint) {
					int iCy;
					HexChunkPoint(sIndexToPrint, iDataInterpHexPosToPrintX, iCy);
					TextChunkPoint(sIndexToPrint, iDataInterpTextPosToPrintX, iCy);
					fNeedChunkPoint = false;
				}

				if (!wstrHexDataInterpToPrint.empty()) { //Only adding spaces if there are chars beforehead.
					if ((iterChunks % GetGroupSize()) == 0) {
						wstrHexDataInterpToPrint += L' ';
					}

					//Additional space between capacity halves, only in 1 byte grouping size.
					if (GetGroupSize() == 1 && iterChunks == m_dwCapacityBlockSize) {
						wstrHexDataInterpToPrint += L"  ";
					}
				}
				wstrHexDataInterpToPrint += wsvHex[sIndexToPrint * 2];
				wstrHexDataInterpToPrint += wsvHex[sIndexToPrint * 2 + 1];
				wstrTextDataInterpToPrint += wsvText[sIndexToPrint];
			}
		}

		//Data Interpreter Poly.
		if (!wstrHexDataInterpToPrint.empty()) {
			//Hex Data Interpreter Poly.
			vecWstrDataInterp.emplace_back(std::make_unique<std::wstring>(std::move(wstrHexDataInterpToPrint)));
			vecPolyDataInterp.emplace_back(iDataInterpHexPosToPrintX, iPosToPrintY,
				static_cast<UINT>(vecWstrDataInterp.back()->size()), vecWstrDataInterp.back()->data(), 0, RECT { }, GetCharsWidthArray());

			//Text Data Interpreter Poly.
			vecWstrDataInterp.emplace_back(std::make_unique<std::wstring>(std::move(wstrTextDataInterpToPrint)));
			vecPolyDataInterp.emplace_back(iDataInterpTextPosToPrintX, iPosToPrintY,
				static_cast<UINT>(vecWstrDataInterp.back()->size()), vecWstrDataInterp.back()->data(), 0, RECT { }, GetCharsWidthArray());
		}
	}

	//Data Interpreter printing.
	if (!vecPolyDataInterp.empty()) {
		pDC->SelectObject(m_fontMain);
		pDC->SetTextColor(m_stColors.clrFontDataInterp);
		pDC->SetBkColor(m_stColors.clrBkDataInterp);
		for (const auto& iter : vecPolyDataInterp) {
			ExtTextOutW(pDC->m_hDC, iter.x, iter.y, iter.uiFlags, &iter.rcl, iter.lpstr, iter.n, iter.pdx); //Hex/Text Data Interpreter printing.
		}
	}
}

void CHexCtrl::DrawPageLines(CDC* pDC, ULONGLONG ullStartLine, int iLines)
{
	if (!IsPageVisible())
		return;

	struct PAGELINES { //Struct for pages lines.
		POINT ptStart;
		POINT ptEnd;
	};
	std::vector<PAGELINES> vecPageLines;

	//Loop for printing Hex chunks and Text chars line by line.
	for (auto iterLines = 0; iterLines < iLines; ++iterLines) {
		//Page's lines vector to print.
		if ((((ullStartLine + iterLines) * GetCapacity()) % m_dwPageSize == 0) && iterLines > 0) {
			const auto iPosToPrintY = m_iStartWorkAreaYPx + m_sizeFontMain.cy * iterLines;
			vecPageLines.emplace_back(POINT { m_iFirstVertLinePx, iPosToPrintY }, POINT { m_iFourthVertLinePx, iPosToPrintY });
		}
	}

	//Page lines printing.
	if (!vecPageLines.empty()) {
		for (const auto& iter : vecPageLines) {
			pDC->MoveTo(iter.ptStart.x, iter.ptStart.y);
			pDC->LineTo(iter.ptEnd.x, iter.ptEnd.y);
		}
	}
}

void CHexCtrl::FillCapacityString()
{
	const auto dwCapacity = GetCapacity();
	m_wstrCapacity.clear();
	m_wstrCapacity.reserve(static_cast<std::size_t>(dwCapacity) * 3);
	for (auto iter { 0U }; iter < dwCapacity; ++iter) {
		m_wstrCapacity += std::vformat(IsOffsetAsHex() ? L"{: >2X}" : L"{: >2d}", std::make_wformat_args(iter));

		//Additional space between hex chunk blocks.
		if ((((iter + 1) % GetGroupSize()) == 0) && (iter < (dwCapacity - 1))) {
			m_wstrCapacity += L" ";
		}

		//Additional space between hex halves.
		if (GetGroupSize() == 1 && iter == (m_dwCapacityBlockSize - 1)) {
			m_wstrCapacity += L"  ";
		}
	}
}

void CHexCtrl::FillWithZeros()
{
	if (!IsDataSet())
		return;

	std::byte byteZero { 0 };
	ModifyData({ .eModifyMode { EHexModifyMode::MODIFY_REPEAT }, .spnData { &byteZero, sizeof(byteZero) },
		.vecSpan { GetSelection() } });
	Redraw();
}

void CHexCtrl::FontSizeIncDec(bool fInc)
{
	auto lFontSize = MulDiv(-GetFontSize(), 72, m_iLOGPIXELSY);
	if (fInc) {
		++lFontSize;
	}
	else {
		--lFontSize;
	}

	SetFontSize(lFontSize);
}

auto CHexCtrl::GetBottomLine()const->ULONGLONG
{
	if (!IsDataSet())
		return { };

	auto ullEndLine = GetTopLine();
	if (const auto iLines = m_iHeightWorkAreaPx / m_sizeFontMain.cy; iLines > 0) { //How many visible lines.
		ullEndLine += iLines - 1;
	}

	const auto ullDataSize = GetDataSize();
	const auto ullTotalLines = ullDataSize / GetCapacity();

	//If ullDataSize is really small, or we at the scroll end, adjust ullEndLine to be not bigger than maximum possible.
	if (ullEndLine >= ullTotalLines) {
		ullEndLine = ullTotalLines - ((ullDataSize % GetCapacity()) == 0 ? 1 : 0);
	}

	return ullEndLine;
}

auto CHexCtrl::GetCharsWidthArray()const->int*
{
	return const_cast<int*>(m_vecCharsWidth.data());
}

auto CHexCtrl::GetCharWidthExtras()const->int
{
	return GetCharWidthNative() + GetCharsExtraSpace();
}

auto CHexCtrl::GetCharWidthNative()const->int
{
	return m_sizeFontMain.cx;
}

auto CHexCtrl::GetCommandFromKey(UINT uKey, bool fCtrl, bool fShift, bool fAlt)const->std::optional<EHexCmd>
{
	if (const auto iter = std::find_if(m_vecKeyBind.begin(), m_vecKeyBind.end(), [=](const KEYBIND& ref) {
		return ref.fCtrl == fCtrl && ref.fShift == fShift && ref.fAlt == fAlt && ref.uKey == uKey; });
		iter != m_vecKeyBind.end()) {
		return iter->eCmd;
	}

	return std::nullopt;
}

auto CHexCtrl::GetCommandFromMenu(WORD wMenuID)const->std::optional<EHexCmd>
{
	if (const auto iter = std::find_if(m_vecKeyBind.begin(), m_vecKeyBind.end(), [=](const KEYBIND& ref) {
		return ref.wMenuID == wMenuID; }); iter != m_vecKeyBind.end()) {
		return iter->eCmd;
	}

	return std::nullopt;
}

long CHexCtrl::GetFontSize()
{
	return GetFont().lfHeight;
}

auto CHexCtrl::GetRectTextCaption()const->CRect
{
	const auto iScrollH { static_cast<int>(m_pScrollH->GetScrollPos()) };
	return { m_iThirdVertLinePx - iScrollH, m_iFirstHorzLinePx, m_iFourthVertLinePx - iScrollH, m_iSecondHorzLinePx };
}

auto CHexCtrl::GetScrollPageSize()const->ULONGLONG
{
	const auto ullPageSize = static_cast<ULONGLONG>(m_flScrollRatio * (m_fScrollLines ? m_sizeFontMain.cy : m_iHeightWorkAreaPx));
	return ullPageSize < m_sizeFontMain.cy ? m_sizeFontMain.cy : ullPageSize;
}

auto CHexCtrl::GetTopLine()const->ULONGLONG
{
	return m_pScrollV->GetScrollPos() / m_sizeFontMain.cy;
}

void CHexCtrl::HexChunkPoint(ULONGLONG ullOffset, int& iCx, int& iCy)const
{
	//This func computes x and y pos of the given Hex chunk.
	const auto dwCapacity = GetCapacity();
	const DWORD dwMod = ullOffset % dwCapacity;
	const auto iBetweenBlocks { dwMod >= m_dwCapacityBlockSize ? m_iSpaceBetweenBlocksPx : 0 };
	iCx = static_cast<int>(((m_iIndentFirstHexChunkXPx + iBetweenBlocks + dwMod * m_iSizeHexBytePx)
		+ (dwMod / GetGroupSize()) * GetCharWidthExtras()) - m_pScrollH->GetScrollPos());

	const auto ullScrollV = m_pScrollV->GetScrollPos();
	iCy = static_cast<int>((m_iStartWorkAreaYPx + (ullOffset / dwCapacity) * m_sizeFontMain.cy) -
		(ullScrollV - (ullScrollV % m_sizeFontMain.cy)));
}

auto CHexCtrl::HitTest(POINT pt)const->std::optional<HEXHITTEST>
{
	HEXHITTEST stHit;
	const auto iY = pt.y;
	const auto iX = pt.x + static_cast<int>(m_pScrollH->GetScrollPos()); //To compensate horizontal scroll.
	const auto ullCurLine = GetTopLine();
	const auto dwGroupSize = GetGroupSize();
	const auto dwCapacity = GetCapacity();

	bool fHit { false };
	//Checking if iX is within Hex chunks area.
	if ((iX >= m_iIndentFirstHexChunkXPx) && (iX < m_iThirdVertLinePx) && (iY >= m_iStartWorkAreaYPx) && (iY <= m_iEndWorkAreaPx)) {
		int iTotalSpaceBetweenChunks { 0 };
		for (auto iterCapacity = 0U; iterCapacity < dwCapacity; ++iterCapacity) {
			if (iterCapacity > 0 && (iterCapacity % dwGroupSize) == 0) {
				iTotalSpaceBetweenChunks += GetCharWidthExtras();
				if (dwGroupSize == 1 && iterCapacity == m_dwCapacityBlockSize) {
					iTotalSpaceBetweenChunks += m_iSpaceBetweenBlocksPx;
				}
			}

			const auto iCurrChunkBegin = m_iIndentFirstHexChunkXPx + (m_iSizeHexBytePx * iterCapacity) + iTotalSpaceBetweenChunks;
			const auto iCurrChunkEnd = iCurrChunkBegin + m_iSizeHexBytePx +
				(((iterCapacity + 1) % dwGroupSize) == 0 ? GetCharWidthExtras() : 0)
				+ ((dwGroupSize == 1 && (iterCapacity + 1) == m_dwCapacityBlockSize) ? m_iSpaceBetweenBlocksPx : 0);

			if (static_cast<unsigned int>(iX) < iCurrChunkEnd) { //If iX lays in-between [iCurrChunkBegin...iCurrChunkEnd).
				stHit.ullOffset = static_cast<ULONGLONG>(iterCapacity) + ((iY - m_iStartWorkAreaYPx) / m_sizeFontMain.cy) *
					dwCapacity + (ullCurLine * dwCapacity);

				if ((iX - iCurrChunkBegin) < static_cast<DWORD>(GetCharWidthExtras())) { //Check byte's High or Low half was hit.
					stHit.fIsHigh = true;
				}

				fHit = true;
				break;
			}
		}
	}
	//Or within Text area.
	else if ((iX >= m_iIndentTextXPx) && (iX < (m_iIndentTextXPx + m_iDistanceBetweenCharsPx * static_cast<int>(dwCapacity)))
		&& (iY >= m_iStartWorkAreaYPx) && iY <= m_iEndWorkAreaPx) {
		//Calculate ullOffset Text symbol.
		stHit.ullOffset = ((iX - static_cast<ULONGLONG>(m_iIndentTextXPx)) / m_iDistanceBetweenCharsPx) +
			((iY - m_iStartWorkAreaYPx) / m_sizeFontMain.cy) * dwCapacity + (ullCurLine * dwCapacity);
		stHit.fIsText = true;
		fHit = true;
	}

	//If iX is out of end-bound of Hex chunks or Text chars.
	if (stHit.ullOffset >= GetDataSize()) {
		fHit = false;
	}

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
	return GetPageSize() > 0 && (GetPageSize() % GetCapacity() == 0) && GetPageSize() >= GetCapacity();
}

void CHexCtrl::ModifyWorker(const HEXCTRL::HEXMODIFY& hms, const auto& FuncWorker, const HEXCTRL::SpanCByte spnOper)
{
	assert(!spnOper.empty());
	if (spnOper.empty())
		return;

	const auto& vecSpanRef = hms.vecSpan;
	const auto ullTotalSize = std::accumulate(vecSpanRef.begin(), vecSpanRef.end(), 0ULL,
		[](ULONGLONG ullSumm, const HEXSPAN& ref) { return ullSumm + ref.ullSize; });
	assert(ullTotalSize <= GetDataSize());

	CHexDlgCallback dlgClbk(L"Modifying...", vecSpanRef.back().ullOffset, vecSpanRef.back().ullOffset + ullTotalSize, this);
	const auto lmbModify = [&]() {
		for (const auto& iterSpan : vecSpanRef) { //Span-vector's size times.
			const auto ullOffsetToModify { iterSpan.ullOffset };
			const auto ullSizeToModify { iterSpan.ullSize };
			const auto ullSizeDataOper { spnOper.size() };

			//If the size of the data to_modify_from is bigger than
			//the data to modify, we do nothing.
			if (ullSizeDataOper > ullSizeToModify)
				break;

			ULONGLONG ullSizeCache { };
			ULONGLONG ullChunks { };
			bool fCacheIsLargeEnough { true }; //Cache is larger than ullSizeDataOper.

			if (IsVirtual()) {
				ullSizeCache = GetCacheSize(); //Size of Virtual memory for acquiring, to work with.
				if (ullSizeCache >= ullSizeDataOper) {
					ullSizeCache -= ullSizeCache % ullSizeDataOper; //Aligning chunk size to ullSizeDataOper.

					if (ullSizeToModify < ullSizeCache) {
						ullSizeCache = ullSizeToModify;
					}
					ullChunks = ullSizeToModify / ullSizeCache + ((ullSizeToModify % ullSizeCache) ? 1 : 0);
				}
				else {
					fCacheIsLargeEnough = false;
					const auto iSmallMod = ullSizeDataOper % ullSizeCache;
					const auto ullSmallChunks = ullSizeDataOper / ullSizeCache + (iSmallMod > 0 ? 1 : 0);
					ullChunks = (ullSizeToModify / ullSizeDataOper) * ullSmallChunks;
				}
			}
			else {
				ullSizeCache = ullSizeToModify;
				ullChunks = 1;
			}

			if (fCacheIsLargeEnough) {
				for (auto iterChunk { 0ULL }; iterChunk < ullChunks; ++iterChunk) {
					const auto ullOffsetCurr = ullOffsetToModify + (iterChunk * ullSizeCache);
					if (ullOffsetCurr + ullSizeCache > GetDataSize()) { //Overflow check.
						ullSizeCache = GetDataSize() - ullOffsetCurr;
						if (ullSizeCache < ullSizeDataOper) //ullSizeChunk is too small for ullSizeDataOper.
							break;
					}

					if ((ullOffsetCurr + ullSizeCache) > (ullOffsetToModify + ullSizeToModify)) {
						ullSizeCache = (ullOffsetToModify + ullSizeToModify) - ullOffsetCurr;
						if (ullSizeCache < ullSizeDataOper) //ullSizeChunk is too small for ullSizeDataOper.
							break;
					}

					const auto spnData = GetData({ ullOffsetCurr, ullSizeCache });
					assert(!spnData.empty());
					for (auto ullIndex { 0ULL }; ullIndex <= (ullSizeCache - ullSizeDataOper); ullIndex += ullSizeDataOper) {
						FuncWorker(spnData.data() + ullIndex, hms, spnOper);
						if (dlgClbk.IsCanceled()) {
							SetDataVirtual(spnData, { ullOffsetCurr, ullSizeCache });
							goto exit;
						}
						dlgClbk.SetProgress(ullOffsetCurr + ullIndex);
					}
					SetDataVirtual(spnData, { ullOffsetCurr, ullSizeCache });
				}
			}
			else {
				//It's a special case for when the ullSizeDataOper is larger than
				//the current cache size (only in VirtualData mode).

				const auto ullSmallMod = ullSizeDataOper % ullSizeCache;
				const auto ullSmallChunks = ullSizeDataOper / ullSizeCache + (ullSmallMod > 0 ? 1 : 0);
				auto ullSmallChunkCur = 0ULL; //Current small chunk index.
				auto ullOffsetCurr = 0ULL;
				auto ullOffsetSubSpan = 0ULL; //Current offset for spnOper.subspan().
				auto ullSizeCacheCurr = 0ULL; //Current cache size.
				for (auto iterChunk { 0ULL }; iterChunk < ullChunks; ++iterChunk) {
					if (ullSmallChunkCur == (ullSmallChunks - 1) && ullSmallMod > 0) {
						ullOffsetCurr += ullSmallMod;
						ullSizeCacheCurr = ullSmallMod;
						ullOffsetSubSpan += ullSmallMod;
					}
					else {
						ullOffsetCurr = ullOffsetToModify + (iterChunk * ullSizeCache);
						ullSizeCacheCurr = ullSizeCache;
						ullOffsetSubSpan = ullSmallChunkCur * ullSizeCache;
					}

					const auto spnData = GetData({ ullOffsetCurr, ullSizeCacheCurr });
					assert(!spnData.empty());
					FuncWorker(spnData.data(), hms, spnOper.subspan(static_cast<std::size_t>(ullOffsetSubSpan),
						static_cast<std::size_t>(ullSizeCacheCurr)));

					if (dlgClbk.IsCanceled()) {
						SetDataVirtual(spnData, { ullOffsetCurr, ullSizeCacheCurr });
						goto exit;
					}

					dlgClbk.SetProgress(ullOffsetCurr + ullSizeCacheCurr);
					SetDataVirtual(spnData, { ullOffsetCurr, ullSizeCacheCurr });

					if (++ullSmallChunkCur == ullSmallChunks) {
						ullSmallChunkCur = 0ULL;
						ullOffsetSubSpan = 0ULL;
					}
				}
			}
		}
	exit:
		dlgClbk.ExitDlg();
		};

	static constexpr auto uSizeToRunThread { 1024U * 1024U * 50U }; //50MB.
	if (ullTotalSize > uSizeToRunThread) { //Spawning new thread only if data size is big enough.
		std::thread thrd(lmbModify);
		dlgClbk.DoModal();
		thrd.join();
	}
	else {
		lmbModify();
	}
}

auto CHexCtrl::OffsetToWstr(ULONGLONG ullOffset)const->std::wstring
{
	return std::vformat(IsOffsetAsHex() ? L"{:0>{}X}" : L"{:0>{}}", std::make_wformat_args(ullOffset, m_dwOffsetDigits));
}

void CHexCtrl::OnCaretPosChange(ULONGLONG ullOffset)
{
	//To prevent UpdateData() while key is pressed continuously, only when one time pressed.
	if (!m_fKeyDownAtm) {
		m_pDlgDataInterp->UpdateData();
	}

	if (auto pBkm = m_pDlgBkmMgr->HitTest(ullOffset); pBkm != nullptr) { //If clicked on bookmark.
		const HEXBKMINFO hbi { .hdr { m_hWnd, static_cast<UINT>(GetDlgCtrlID()), HEXCTRL_MSG_BKMCLICK }, .pBkm { pBkm } };
		ParentNotify(hbi);
	}

	ParentNotify(HEXCTRL_MSG_SETCARET);
}

void CHexCtrl::OnModifyData()
{
	ParentNotify(HEXCTRL_MSG_SETDATA);
	m_pDlgTemplMgr->UpdateData();
	m_pDlgDataInterp->UpdateData();
}

template<typename T> requires std::is_class_v<T>
void CHexCtrl::ParentNotify(const T& t)const
{
	if (const auto hwnd = ::GetParent(m_hWnd); hwnd != nullptr) {
		::SendMessageW(m_hWnd, WM_NOTIFY, GetDlgCtrlID(), reinterpret_cast<LPARAM>(&t));
	}
}

void CHexCtrl::ParentNotify(UINT uCode)const
{
	ParentNotify(NMHDR { .hwndFrom { m_hWnd }, .idFrom { static_cast<UINT>(GetDlgCtrlID()) }, .code { uCode } });
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

	if (dlg.DoModal() != S_OK) {
		MessageBoxW(L"Internal printer initialization error.", L"Error", MB_ICONERROR);
		return;
	}

	//User pressed "Cancel", or "Apply" and then "Cancel".
	if (dlg.m_pdex.dwResultAction == PD_RESULT_CANCEL || dlg.m_pdex.dwResultAction == PD_RESULT_APPLY) {
		DeleteDC(dlg.m_pdex.hDC);
		return;
	}

	const auto hdcPrinter = dlg.GetPrinterDC();
	if (hdcPrinter == nullptr) {
		MessageBoxW(L"No printer found.");
		return;
	}

	CDC dcPrint;
	dcPrint.Attach(hdcPrinter);
	const auto pDC = &dcPrint;
	DOCINFOW di { };
	di.cbSize = sizeof(DOCINFOW);
	di.lpszDocName = L"HexCtrl";

	if (dcPrint.StartDocW(&di) < 0) {
		dcPrint.AbortDoc();
		dcPrint.DeleteDC();
		return;
	}

	constexpr auto iMarginX = 150;
	constexpr auto iMarginY = 150;
	const CRect rcPrint(CPoint(0, 0), CSize(GetDeviceCaps(dcPrint, HORZRES) - iMarginX * 2, GetDeviceCaps(dcPrint, VERTRES) - iMarginY * 2));
	const CSize sizePrintDpi = { GetDeviceCaps(dcPrint, LOGPIXELSX), GetDeviceCaps(dcPrint, LOGPIXELSY) };
	const auto iRatio = sizePrintDpi.cy / m_iLOGPIXELSY;

	/***Changing Main and Info Bar fonts***/
	LOGFONTW lfMainOrig;
	m_fontMain.GetLogFont(&lfMainOrig);
	LOGFONTW lfMain { lfMainOrig };
	lfMain.lfHeight *= iRatio;
	m_fontMain.DeleteObject();
	m_fontMain.CreateFontIndirectW(&lfMain);
	LOGFONTW lfInfoBarOrig;
	m_fontInfoBar.GetLogFont(&lfInfoBarOrig);
	LOGFONTW lfInfoBar { lfInfoBarOrig };
	lfInfoBar.lfHeight *= iRatio;
	m_fontInfoBar.DeleteObject();
	m_fontInfoBar.CreateFontIndirectW(&lfInfoBar);
	/***Changing Main and Info Bar fonts END***/

	auto ullStartLine = GetTopLine();
	const auto dwCapacity = GetCapacity();
	const auto ullTotalLines = GetDataSize() / dwCapacity;
	RecalcAll(pDC, &rcPrint);
	const auto iLinesInPage = m_iHeightWorkAreaPx / m_sizeFontMain.cy;
	const auto ullTotalPages = ullTotalLines / iLinesInPage + 1;
	int iPagesToPrint { };

	if (dlg.PrintAll()) {
		iPagesToPrint = static_cast<int>(ullTotalPages);
		ullStartLine = 0;
	}
	else if (dlg.PrintRange()) {
		const auto iFromPage = ppr.nFromPage - 1;
		const auto iToPage = ppr.nToPage;
		if (iFromPage <= ullTotalPages) { //Checks for out-of-range pages user input.
			iPagesToPrint = iToPage - iFromPage;
			if (iPagesToPrint + iFromPage > ullTotalPages) {
				iPagesToPrint = static_cast<int>(ullTotalPages - iFromPage);
			}
		}
		ullStartLine = iFromPage * iLinesInPage;
	}

	pDC->SetMapMode(MM_TEXT);
	pDC->SetViewportOrg(iMarginX, iMarginY); //Move a viewport to have some indent from the edge.
	if (dlg.PrintSelection()) {
		pDC->StartPage();
		const auto ullSelStart = m_pSelection->GetSelStart();
		const auto ullSelSize = m_pSelection->GetSelSize();
		ullStartLine = ullSelStart / dwCapacity;

		const DWORD dwModStart = ullSelStart % dwCapacity;
		auto dwLines = static_cast<DWORD>(ullSelSize) / dwCapacity;
		if ((dwModStart + static_cast<DWORD>(ullSelSize)) > dwCapacity) {
			++dwLines;
		}
		if ((ullSelSize % dwCapacity) + dwModStart > dwCapacity) {
			++dwLines;
		}
		if (!dwLines) {
			dwLines = 1;
		}

		const auto ullEndLine = ullStartLine + dwLines;
		const auto iLines = static_cast<int>(ullEndLine - ullStartLine);
		const auto& [wstrHex, wstrText] = BuildDataToDraw(ullStartLine, iLines);

		DrawWindow(pDC);
		DrawInfoBar(pDC);
		const auto clBkOld = m_stColors.clrBkSel;
		const auto clTextOld = m_stColors.clrFontSel;
		m_stColors.clrBkSel = m_stColors.clrBk; //To print as normal text on normal bk, not white text on black bk.
		m_stColors.clrFontSel = m_stColors.clrFontHex;
		DrawOffsets(pDC, ullStartLine, iLines);
		DrawSelection(pDC, ullStartLine, iLines, wstrHex, wstrText);
		m_stColors.clrBkSel = clBkOld;
		m_stColors.clrFontSel = clTextOld;
		pDC->EndPage();
	}
	else {
		for (auto iterPage = 0; iterPage < iPagesToPrint; ++iterPage) {
			pDC->StartPage();
			auto ullEndLine = ullStartLine + iLinesInPage;

			//If m_dwDataCount is really small we adjust ullEndLine to be not bigger than maximum allowed.
			if (ullEndLine > ullTotalLines) {
				ullEndLine = (GetDataSize() % dwCapacity) ? ullTotalLines + 1 : ullTotalLines;
			}

			const auto iLines = static_cast<int>(ullEndLine - ullStartLine);
			const auto& [wstrHex, wstrText] = BuildDataToDraw(ullStartLine, iLines);

			DrawWindow(pDC); //Draw the window with all layouts.
			DrawInfoBar(pDC);

			if (IsDataSet()) {
				DrawOffsets(pDC, ullStartLine, iLines);
				DrawHexText(pDC, ullStartLine, iLines, wstrHex, wstrText);
				DrawTemplates(pDC, ullStartLine, iLines, wstrHex, wstrText);
				DrawBookmarks(pDC, ullStartLine, iLines, wstrHex, wstrText);
				DrawSelection(pDC, ullStartLine, iLines, wstrHex, wstrText);
				DrawSelHighlight(pDC, ullStartLine, iLines, wstrHex, wstrText);
				DrawCaret(pDC, ullStartLine, wstrHex, wstrText);
				DrawDataInterp(pDC, ullStartLine, iLines, wstrHex, wstrText);
				DrawPageLines(pDC, ullStartLine, iLines);
			}
			ullStartLine += iLinesInPage;
			pDC->EndPage();
		}
	}
	pDC->EndDoc();
	pDC->DeleteDC();

	/***Changing Main and Info Bar fonts back to originals.***/
	m_fontMain.DeleteObject();
	m_fontMain.CreateFontIndirectW(&lfMainOrig);
	m_fontInfoBar.DeleteObject();
	m_fontInfoBar.CreateFontIndirectW(&lfInfoBarOrig);

	RecalcAll();
}

void CHexCtrl::RecalcAll(CDC* pDC, const CRect* pRect)
{
	const auto pDCCurr = pDC == nullptr ? GetDC() : pDC;
	const auto ullCurLineV = GetTopLine();
	TEXTMETRICW tm;
	pDCCurr->SelectObject(m_fontMain);
	pDCCurr->GetTextMetricsW(&tm);
	m_sizeFontMain.cx = tm.tmAveCharWidth;
	m_sizeFontMain.cy = tm.tmHeight + tm.tmExternalLeading;
	pDCCurr->SelectObject(m_fontInfoBar);
	pDCCurr->GetTextMetricsW(&tm);
	m_sizeFontInfo.cx = tm.tmAveCharWidth;
	m_sizeFontInfo.cy = tm.tmHeight + tm.tmExternalLeading;

	if (IsInfoBar()) {
		m_iHeightInfoBarPx = m_sizeFontInfo.cy;
		m_iHeightInfoBarPx += m_iHeightInfoBarPx / 3;
	}
	else { //If Info window is disabled, we set its height to zero.
		m_iHeightInfoBarPx = 0;
	}
	m_iHeightBottomOffAreaPx = m_iHeightInfoBarPx + m_iIndentBottomLine;

	const auto ullDataSize = GetDataSize();
	const auto fAsHex = IsOffsetAsHex();
	if (ullDataSize <= 0xffffffffUL) {
		m_dwOffsetDigits = fAsHex ? 8 : 10;
	}
	else if (ullDataSize <= 0xffffffffffUL) {
		m_dwOffsetDigits = fAsHex ? 10 : 13;
	}
	else if (ullDataSize <= 0xffffffffffffUL) {
		m_dwOffsetDigits = fAsHex ? 12 : 15;
	}
	else if (ullDataSize <= 0xffffffffffffffUL) {
		m_dwOffsetDigits = fAsHex ? 14 : 17;
	}
	else {
		m_dwOffsetDigits = fAsHex ? 16 : 19;
	}

	const auto dwGroupSize = GetGroupSize();
	const auto dwCapacity = GetCapacity();
	const auto iCharWidth = GetCharWidthNative();
	const auto iCharWidthExt = GetCharWidthExtras();

	//Approximately "dwCapacity * 3 + 1" size array of char's width, to be enough for the Hex area chars.
	m_vecCharsWidth.assign(dwCapacity * 3 + 1, iCharWidthExt);
	m_iSecondVertLinePx = m_iFirstVertLinePx + m_dwOffsetDigits * iCharWidth + iCharWidth * 2;
	m_iSizeHexBytePx = iCharWidthExt * 2;
	m_iSpaceBetweenBlocksPx = (dwGroupSize == 1 && dwCapacity > 1) ? iCharWidthExt * 2 : 0;
	m_iDistanceGroupedHexChunkPx = m_iSizeHexBytePx * dwGroupSize + iCharWidthExt;
	m_iThirdVertLinePx = m_iSecondVertLinePx + m_iDistanceGroupedHexChunkPx * (dwCapacity / dwGroupSize)
		+ iCharWidth + m_iSpaceBetweenBlocksPx;
	m_iIndentTextXPx = m_iThirdVertLinePx + iCharWidth;
	m_iDistanceBetweenCharsPx = iCharWidthExt;
	m_iFourthVertLinePx = m_iIndentTextXPx + (m_iDistanceBetweenCharsPx * dwCapacity) + iCharWidth;
	m_iIndentFirstHexChunkXPx = m_iSecondVertLinePx + iCharWidth;
	m_iSizeFirstHalfPx = m_iIndentFirstHexChunkXPx + m_dwCapacityBlockSize * (iCharWidthExt * 2) +
		(m_dwCapacityBlockSize / dwGroupSize - 1) * iCharWidthExt;
	m_iHeightTopRectPx = std::lround(m_sizeFontMain.cy * 1.5);
	m_iStartWorkAreaYPx = m_iFirstHorzLinePx + m_iHeightTopRectPx;
	m_iSecondHorzLinePx = m_iStartWorkAreaYPx - 1;
	m_iIndentCapTextYPx = m_iHeightTopRectPx / 2 - (m_sizeFontMain.cy / 2);

	CRect rc;
	if (pRect == nullptr) {
		GetClientRect(rc);
	}
	else {
		rc = *pRect;
	}
	RecalcClientArea(rc.Height(), rc.Width());

	//Scrolls, ReleaseDC and Redraw only for current DC, not for PrintingDC.
	if (pDC == nullptr) {
		m_pScrollV->SetScrollSizes(m_sizeFontMain.cy, GetScrollPageSize(),
			static_cast<ULONGLONG>(m_iStartWorkAreaYPx) + m_iHeightBottomOffAreaPx + m_sizeFontMain.cy *
			(ullDataSize / dwCapacity + (ullDataSize % dwCapacity == 0 ? 1 : 2)));
		m_pScrollH->SetScrollSizes(iCharWidthExt, rc.Width(), static_cast<ULONGLONG>(m_iFourthVertLinePx) + 1);
		m_pScrollV->SetScrollPos(ullCurLineV * m_sizeFontMain.cy);

		ReleaseDC(pDCCurr);
		Redraw();
	}
}

void CHexCtrl::RecalcClientArea(int iHeight, int iWidth)
{
	m_iHeightClientAreaPx = iHeight;
	m_iWidthClientAreaPx = iWidth;
	m_iEndWorkAreaPx = m_iHeightClientAreaPx - m_iHeightBottomOffAreaPx -
		((m_iHeightClientAreaPx - m_iStartWorkAreaYPx - m_iHeightBottomOffAreaPx) % m_sizeFontMain.cy);
	m_iHeightWorkAreaPx = m_iEndWorkAreaPx - m_iStartWorkAreaYPx;
	m_iThirdHorzLinePx = m_iFirstHorzLinePx + m_iHeightClientAreaPx - m_iHeightBottomOffAreaPx;
	m_iFourthHorzLinePx = m_iThirdHorzLinePx + m_iHeightInfoBarPx;
}

void CHexCtrl::Redo()
{
	if (m_vecRedo.empty())
		return;

	const auto& refRedo = m_vecRedo.back();
	VecSpan vecSpan;
	vecSpan.reserve(refRedo->size());
	std::transform(refRedo->begin(), refRedo->end(), std::back_inserter(vecSpan),
		[](UNDO& ref) { return HEXSPAN { ref.ullOffset, ref.vecData.size() }; });
	SnapshotUndo(vecSpan); //Creating new Undo data snapshot.

	for (const auto& iter : *refRedo) {
		const auto& refRedoData = iter.vecData;

		if (IsVirtual() && refRedoData.size() > GetCacheSize()) { //In VirtualData mode processing data chunk by chunk.
			const auto dwSizeChunk = GetCacheSize();
			const auto sMod = refRedoData.size() % dwSizeChunk;
			auto ullChunks = refRedoData.size() / dwSizeChunk + (sMod > 0 ? 1 : 0);
			std::size_t ullOffset = 0;
			while (ullChunks-- > 0) {
				const auto ullSize = (ullChunks == 1 && sMod > 0) ? sMod : dwSizeChunk;
				if (const auto spnData = GetData({ ullOffset, ullSize }); !spnData.empty()) {
					std::copy_n(refRedoData.begin() + ullOffset, ullSize, spnData.data());
					SetDataVirtual(spnData, { ullOffset, ullSize });
				}
				ullOffset += ullSize;
			}
		}
		else {
			if (const auto spnData = GetData({ iter.ullOffset, refRedoData.size() }); !spnData.empty()) {
				std::copy_n(refRedoData.begin(), refRedoData.size(), spnData.data());
				SetDataVirtual(spnData, { iter.ullOffset, refRedoData.size() });
			}
		}
	}

	m_vecRedo.pop_back();
	OnModifyData();
	RedrawWindow();
}

void CHexCtrl::ReplaceUnprintable(std::wstring& wstr, bool fASCII, bool fCRLF)const
{
	//If fASCII is true, then only wchars in 0x1F<...<0x7F range are considered printable.
	//If fCRLF is false, then CR(0x0D) and LF(0x0A) wchars remain untouched.
	if (fASCII) {
		std::replace_if(wstr.begin(), wstr.end(), [=](wchar_t wch) //All non ASCII.
			{ return (wch <= 0x1F || wch >= 0x7F) && (fCRLF || (wch != 0x0D && wch != 0x0A)); }, m_wchUnprintable);
	}
	else {
		std::replace_if(wstr.begin(), wstr.end(), [=](wchar_t wch) //All non printable wchars.
			{ return !std::iswprint(wch) && (fCRLF || (wch != 0x0D && wch != 0x0A)); }, m_wchUnprintable);
	}
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
	const auto iMaxClientX = rcClient.Width() - m_iSizeHexBytePx;
	const auto iAdditionalSpace { m_iSizeHexBytePx / 2 };
	if (iCx >= iMaxClientX) {
		ullNewScrollH += iCx - iMaxClientX + iAdditionalSpace;
	}
	else if (iCx < 0) {
		ullNewScrollH += iCx - iAdditionalSpace;
	}

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
	const auto dwCapacity = GetCapacity();
	ULONGLONG ullClick { };
	ULONGLONG ullStart { };
	ULONGLONG ullSize { 0ULL };
	ULONGLONG ullNewPos { }; //Future pos of selection start.
	ULONGLONG ullOldPos { }; //Current pos of selection start.

	const auto lmbSelection = [&]() {
		if (ullSelStart == m_ullCursorPrev) {
			ullClick = ullStart = m_ullCursorPrev;
			ullSize = ullSelSize + dwCapacity;
			ullNewPos = ullClick + ullSize - 1;
			ullOldPos = ullNewPos - dwCapacity;
		}
		else if (ullSelStart < m_ullCursorPrev) {
			ullClick = m_ullCursorPrev;
			if (ullSelSize > dwCapacity) {
				ullStart = ullSelStart + dwCapacity;
				ullSize = ullSelSize - dwCapacity;
			}
			else {
				ullStart = ullClick;
				ullSize = 1;
			}
			ullOldPos = ullSelStart;
			ullNewPos = ullOldPos + dwCapacity;
		}
		};

	if (m_ullCaretPos == m_ullCursorPrev || m_ullCaretPos == ullSelStart || m_ullCaretPos == m_pSelection->GetSelEnd()) {
		lmbSelection();
	}
	else {
		ullClick = ullStart = m_ullCaretPos;
		ullSize = 1;
	}

	if (ullStart + ullSize > GetDataSize()) { //To avoid overflow.
		ullSize = GetDataSize() - ullStart;
	}

	if (ullSize > 0) {
		m_ullCaretPos = ullStart;
		m_ullCursorPrev = ullClick;
		SetSelection({ { ullStart, ullSize } }, false);

		const auto stOld = IsOffsetVisible(ullOldPos);
		const auto stNew = IsOffsetVisible(ullNewPos);
		if (stNew.i8Vert != 0 && stOld.i8Vert == 0) {
			m_pScrollV->ScrollLineDown();
		}
		Redraw();
		OnCaretPosChange(GetCaretPos());
	}
}

void CHexCtrl::SelAddLeft()
{
	const auto fHasSel = HasSelection();
	const auto ullSelStart = fHasSel ? m_pSelection->GetSelStart() : m_ullCaretPos;
	const auto ullSelSize = fHasSel ? m_pSelection->GetSelSize() : 1;
	ULONGLONG ullClick { };
	ULONGLONG ullStart { };
	ULONGLONG ullSize { 0ULL };
	ULONGLONG ullNewPos { }; //Future pos of selection start.
	ULONGLONG ullOldPos { }; //Current pos of selection start.

	const auto lmbSelection = [&]() {
		if (ullSelStart == m_ullCursorPrev && ullSelSize > 1) {
			ullClick = ullStart = m_ullCursorPrev;
			ullSize = ullSelSize - 1;
			ullOldPos = ullStart + ullSize;
			ullNewPos = ullOldPos - 1;
		}
		else {
			ullClick = m_ullCursorPrev;
			ullStart = ullSelStart > 0 ? ullSelStart - 1 : 0;
			ullSize = ullSelStart > 0 ? ullSelSize + 1 : ullSelSize;
			ullNewPos = ullStart;
			ullOldPos = ullNewPos + 1;
		}
		};

	if (m_ullCaretPos == m_ullCursorPrev || m_ullCaretPos == ullSelStart || m_ullCaretPos == m_pSelection->GetSelEnd()) {
		lmbSelection();
	}
	else {
		ullClick = ullStart = m_ullCaretPos;
		ullSize = 1;
	}

	if (ullSize > 0) {
		m_ullCaretPos = ullStart;
		m_ullCursorPrev = ullClick;
		SetSelection({ { ullStart, ullSize } }, false);

		const auto stOld = IsOffsetVisible(ullOldPos);
		const auto stNew = IsOffsetVisible(ullNewPos);
		if (stNew.i8Vert != 0 && stOld.i8Vert == 0) {
			m_pScrollV->ScrollLineUp();
		}
		else if (stNew.i8Horz != 0 && !IsCurTextArea()) { //Do not horz scroll when in text area.
			ScrollOffsetH(ullNewPos);
		}
		else {
			Redraw();
		}
		OnCaretPosChange(GetCaretPos());
	}
}

void CHexCtrl::SelAddRight()
{
	const auto fHasSel = HasSelection();
	const auto ullSelStart = fHasSel ? m_pSelection->GetSelStart() : m_ullCaretPos;
	const auto ullSelSize = fHasSel ? m_pSelection->GetSelSize() : 1;
	ULONGLONG ullClick { };
	ULONGLONG ullStart { };
	ULONGLONG ullSize { 0ULL };
	ULONGLONG ullNewPos { }; //Future pos of selection start.
	ULONGLONG ullOldPos { }; //Current pos of selection start.

	const auto lmbSelection = [&]() {
		if (ullSelStart == m_ullCursorPrev) {
			ullClick = ullStart = m_ullCursorPrev;
			ullSize = ullSelSize + 1;

			ullNewPos = ullClick + ullSize - 1;
			ullOldPos = ullNewPos - 1;
		}
		else {
			ullClick = m_ullCursorPrev;
			ullStart = ullSelStart + 1;
			ullSize = ullSelSize - 1;

			ullOldPos = ullSelStart;
			ullNewPos = ullOldPos + 1;
		}
		};

	if (m_ullCaretPos == m_ullCursorPrev || m_ullCaretPos == ullSelStart || m_ullCaretPos == m_pSelection->GetSelEnd()) {
		lmbSelection();
	}
	else {
		ullClick = ullStart = m_ullCaretPos;
		ullSize = 1;
	}

	if (ullStart + ullSize > GetDataSize()) { //To avoid overflow.
		ullSize = GetDataSize() - ullStart;
	}

	if (ullSize > 0) {
		m_ullCaretPos = ullStart;
		m_ullCursorPrev = ullClick;
		SetSelection({ { ullStart, ullSize } }, false);

		const auto stOld = IsOffsetVisible(ullOldPos);
		const auto stNew = IsOffsetVisible(ullNewPos);
		if (stNew.i8Vert != 0 && stOld.i8Vert == 0) {
			m_pScrollV->ScrollLineDown();
		}
		else if (stNew.i8Horz != 0 && !IsCurTextArea()) { //Do not horz scroll when in text area.
			ScrollOffsetH(ullNewPos);
		}
		else {
			Redraw();
		}
		OnCaretPosChange(GetCaretPos());
	}
}

void CHexCtrl::SelAddUp()
{
	const auto fHasSel = HasSelection();
	const auto ullSelStart = fHasSel ? m_pSelection->GetSelStart() : m_ullCaretPos;
	const auto ullSelSize = fHasSel ? m_pSelection->GetSelSize() : 1;
	const auto dwCapacity = GetCapacity();
	ULONGLONG ullClick { };
	ULONGLONG ullStart { };
	ULONGLONG ullSize { 0ULL };
	ULONGLONG ullNewPos { }; //Future pos of selection start.
	ULONGLONG ullOldPos { }; //Current pos of selection start.

	const auto lmbSelection = [&]() {
		if (ullSelStart < m_ullCursorPrev) {
			ullClick = m_ullCursorPrev;
			ullOldPos = ullSelStart;

			if (ullSelStart < dwCapacity) {
				ullSize = ullSelSize + ullSelStart;
				ullNewPos = ullOldPos;
			}
			else {
				ullStart = ullSelStart - dwCapacity;
				ullSize = ullSelSize + dwCapacity;
				ullNewPos = ullStart;
			}
		}
		else {
			ullClick = m_ullCursorPrev;
			if (ullSelSize > dwCapacity) {
				ullStart = ullClick;
				ullSize = ullSelSize - dwCapacity;
			}
			else if (ullSelSize > 1) {
				ullStart = ullClick;
				ullSize = 1;
			}
			else {
				ullStart = ullClick >= dwCapacity ? ullClick - dwCapacity : 0;
				ullSize = ullClick >= dwCapacity ? ullSelSize + dwCapacity : ullSelSize + ullSelStart;
			}

			ullOldPos = ullSelStart + ullSelSize - 1;
			ullNewPos = ullOldPos - dwCapacity;
		}
		};

	if (m_ullCaretPos == m_ullCursorPrev || m_ullCaretPos == ullSelStart || m_ullCaretPos == m_pSelection->GetSelEnd()) {
		lmbSelection();
	}
	else {
		ullClick = ullStart = m_ullCaretPos;
		ullSize = 1;
	}

	if (ullSize > 0) {
		m_ullCaretPos = ullStart;
		m_ullCursorPrev = ullClick;
		SetSelection({ { ullStart, ullSize } }, false);

		const auto stOld = IsOffsetVisible(ullOldPos);
		const auto stNew = IsOffsetVisible(ullNewPos);
		if (stNew.i8Vert != 0 && stOld.i8Vert == 0) {
			m_pScrollV->ScrollLineUp();
		}
		Redraw();
		OnCaretPosChange(GetCaretPos());
	}
}

void CHexCtrl::SetDataVirtual(SpanByte spnData, const HEXSPAN& hss)const
{
	//Note: Since this method can be executed asynchronously (in search/replace, etc...),
	//the SendMesage(parent, ...) is impossible here because receiver window
	//must be run in the same thread as a sender.

	if (!IsVirtual())
		return;

	m_pHexVirtData->OnHexSetData({ .hdr { m_hWnd, static_cast<UINT>(GetDlgCtrlID()) }, .stHexSpan { hss }, .spnData { spnData } });
}

void CHexCtrl::SetFontSize(long lSize)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	//Prevent font size from being too small or too big.
	if (lSize < 4 || lSize > 64)
		return;

	auto lf = GetFont();
	lf.lfHeight = -MulDiv(lSize, m_iLOGPIXELSY, 72);
	SetFont(lf);
}

void CHexCtrl::SnapshotUndo(const VecSpan& vecSpan)
{
	constexpr auto dwUndoMax { 512U }; //Undo's max limit.
	const auto ullTotalSize = std::accumulate(vecSpan.begin(), vecSpan.end(), 0ULL,
		[](ULONGLONG ullSumm, const HEXSPAN& ref) { return ullSumm + ref.ullSize; });

	//Check for very big undo size.
	if (ullTotalSize > 1024 * 1024 * 10)
		return;

	//If Undo vec's size is exceeding Undo's max limit, remove first 64 snapshots (the oldest ones).
	if (m_vecUndo.size() >= static_cast<std::size_t>(dwUndoMax)) {
		const auto iterFirst = m_vecUndo.begin();
		const auto iterLast = iterFirst + 64U;
		m_vecUndo.erase(iterFirst, iterLast);
	}

	//Making new Undo data snapshot.
	const auto& refUndo = m_vecUndo.emplace_back(std::make_unique<std::vector<UNDO>>());

	//Bad alloc may happen here!!!
	try {
		for (const auto& iterSel : vecSpan) { //vecSpan.size() amount of continuous areas to preserve.
			auto& refUNDO = refUndo->emplace_back(UNDO { iterSel.ullOffset, { } });
			refUNDO.vecData.resize(static_cast<std::size_t>(iterSel.ullSize));

			//In VirtualData mode processing data chunk by chunk.
			if (IsVirtual() && iterSel.ullSize > GetCacheSize()) {
				const auto dwSizeChunk = GetCacheSize();
				const auto ullMod = iterSel.ullSize % dwSizeChunk;
				auto ullChunks = iterSel.ullSize / dwSizeChunk + (ullMod > 0 ? 1 : 0);
				ULONGLONG ullOffset { 0 };
				while (ullChunks-- > 0) {
					const auto ullSize = (ullChunks == 1 && ullMod > 0) ? ullMod : dwSizeChunk;
					if (const auto spnData = GetData({ ullOffset, ullSize }); !spnData.empty()) {
						std::copy_n(spnData.data(), ullSize, refUNDO.vecData.data() + static_cast<std::size_t>(ullOffset));
					}
					ullOffset += ullSize;
				}
			}
			else {
				if (const auto spnData = GetData(iterSel); !spnData.empty()) {
					std::copy_n(spnData.data(), iterSel.ullSize, refUNDO.vecData.data());
				}
			}
		}
	}
	catch (const std::bad_alloc&) {
		m_vecUndo.clear();
		m_vecRedo.clear();
		return;
	}
}

void CHexCtrl::TextChunkPoint(ULONGLONG ullOffset, int& iCx, int& iCy)const
{	//This func computes x and y pos of given Text chunk.
	const DWORD dwMod = ullOffset % GetCapacity();
	iCx = static_cast<int>((m_iIndentTextXPx + dwMod * GetCharWidthExtras()) - m_pScrollH->GetScrollPos());

	const auto ullScrollV = m_pScrollV->GetScrollPos();
	iCy = static_cast<int>((m_iStartWorkAreaYPx + (ullOffset / GetCapacity()) * m_sizeFontMain.cy) -
		(ullScrollV - (ullScrollV % m_sizeFontMain.cy)));
}

void CHexCtrl::ToolTipBkmShow(bool fShow, POINT pt, bool fTimerCancel)
{
	if (fShow) {
		m_tmTtBkm = std::time(nullptr);
		m_wndTtBkm.SendMessageW(TTM_TRACKPOSITION, 0, static_cast<LPARAM>(MAKELONG(pt.x, pt.y)));
		m_wndTtBkm.SendMessageW(TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(&m_stToolInfoBkm));
		m_wndTtBkm.SendMessageW(TTM_TRACKACTIVATE, static_cast<WPARAM>(TRUE), reinterpret_cast<LPARAM>(&m_stToolInfoBkm));
		SetTimer(m_uIDTTBkm, 300, nullptr);
	}
	else if (fTimerCancel) { //Tooltip was canceled by the timer, not mouse move.
		m_wndTtBkm.SendMessageW(TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&m_stToolInfoBkm));
	}
	else {
		m_pBkmTtCurr = nullptr;
		m_wndTtBkm.SendMessageW(TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&m_stToolInfoBkm));
		KillTimer(m_uIDTTBkm);
	}
}

void CHexCtrl::ToolTipTemplShow(bool fShow, POINT pt, bool fTimerCancel)
{
	if (fShow) {
		m_tmTtTempl = std::time(nullptr);
		m_wndTtTempl.SendMessageW(TTM_TRACKPOSITION, 0, static_cast<LPARAM>(MAKELONG(pt.x, pt.y)));
		m_wndTtTempl.SendMessageW(TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(&m_stToolInfoTempl));
		m_wndTtTempl.SendMessageW(TTM_TRACKACTIVATE, static_cast<WPARAM>(TRUE), reinterpret_cast<LPARAM>(&m_stToolInfoTempl));
		SetTimer(m_uIDTTTempl, 300, nullptr);
	}
	else if (fTimerCancel) { //Tooltip was canceled by the timer, not mouse move.
		m_wndTtTempl.SendMessageW(TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&m_stToolInfoTempl));
	}
	else {
		m_pTFieldTtCurr = nullptr;
		m_wndTtTempl.SendMessageW(TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&m_stToolInfoTempl));
		KillTimer(m_uIDTTTempl);
	}
}

void CHexCtrl::ToolTipOffsetShow(bool fShow)
{
	if (fShow) {
		CPoint ptScreen;
		GetCursorPos(&ptScreen);
		auto wstrOffset = (IsOffsetAsHex() ? L"Offset: 0x" : L"Offset: ") + OffsetToWstr(GetTopLine() * GetCapacity());
		m_stToolInfoOffset.lpszText = wstrOffset.data();
		m_wndTtOffset.SendMessageW(TTM_TRACKPOSITION, 0, static_cast<LPARAM>(MAKELONG(ptScreen.x - 5, ptScreen.y - 20)));
		m_wndTtOffset.SendMessageW(TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(&m_stToolInfoOffset));
		m_stToolInfoOffset.lpszText = nullptr;
	}

	m_wndTtOffset.SendMessageW(TTM_TRACKACTIVATE, static_cast<WPARAM>(fShow), reinterpret_cast<LPARAM>(&m_stToolInfoOffset));
}

void CHexCtrl::Undo()
{
	if (m_vecUndo.empty())
		return;

	//Bad alloc may happen here! If there is no more free memory, just clear the vec and return.
	try {
		//Creating new Redo data snapshot.
		const auto& refRedo = m_vecRedo.emplace_back(std::make_unique<std::vector<UNDO>>());
		for (const auto& iter : *m_vecUndo.back()) {
			auto& refRedoBack = refRedo->emplace_back(UNDO { iter.ullOffset, { } });
			refRedoBack.vecData.resize(iter.vecData.size());
			const auto& refUndoData = iter.vecData;

			if (IsVirtual() && refUndoData.size() > GetCacheSize()) { //In VirtualData mode processing data chunk by chunk.
				const auto dwSizeChunk = GetCacheSize();
				const auto sMod = refUndoData.size() % dwSizeChunk;
				auto ullChunks = refUndoData.size() / dwSizeChunk + (sMod > 0 ? 1 : 0);
				std::size_t ullOffset = 0;
				while (ullChunks-- > 0) {
					const auto ullSize = (ullChunks == 1 && sMod > 0) ? sMod : dwSizeChunk;
					if (const auto spnData = GetData({ ullOffset, ullSize }); !spnData.empty()) {
						std::copy_n(spnData.data(), ullSize, refRedoBack.vecData.begin() + ullOffset); //Fill Redo with the data.
						std::copy_n(refUndoData.begin() + ullOffset, ullSize, spnData.data()); //Undo the data.
						SetDataVirtual(spnData, { ullOffset, ullSize });
					}
					ullOffset += ullSize;
				}
			}
			else {
				if (const auto spnData = GetData({ iter.ullOffset, refUndoData.size() }); !spnData.empty()) {
					std::copy_n(spnData.data(), refUndoData.size(), refRedoBack.vecData.begin()); //Fill Redo with the data.
					std::copy_n(refUndoData.begin(), refUndoData.size(), spnData.data()); //Undo the data.
					SetDataVirtual(spnData, { iter.ullOffset, refUndoData.size() });
				}
			}
		}
	}
	catch (const std::bad_alloc&) {
		m_vecRedo.clear();
		return;
	}

	m_vecUndo.pop_back();
	OnModifyData();
	RedrawWindow();
}


//CHexCtrl MFC message handlers.

void CHexCtrl::OnChar(UINT nChar, UINT /*nRepCnt*/, UINT /*nFlags*/)
{
	if (!IsDataSet() || !IsMutable() || !IsCurTextArea() || (GetKeyState(VK_CONTROL) < 0))
		return;

	unsigned char chByte = nChar & 0xFF;
	wchar_t warrCurrLocaleID[KL_NAMELENGTH];
	GetKeyboardLayoutNameW(warrCurrLocaleID); //Current langID as wstring.
	if (const auto optLocID = stn::StrToUInt32(warrCurrLocaleID, 16); optLocID) { //Convert langID from wstr to number.
		UINT uCurrCodePage { };
		constexpr int iSize = sizeof(uCurrCodePage) / sizeof(wchar_t);
		if (GetLocaleInfoW(*optLocID, LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
			reinterpret_cast<LPWSTR>(&uCurrCodePage), iSize) == iSize) { //ANSI code page for the current langID (if any).
			const wchar_t wch { static_cast<wchar_t>(nChar) };
			//Convert input symbol (wchar) to char according to current Windows' code page.
			if (auto str = WstrToStr(&wch, uCurrCodePage); !str.empty()) {
				chByte = str[0];
			}
		}
	}

	ModifyData({ .spnData { reinterpret_cast<std::byte*>(&chByte), sizeof(chByte) }, .vecSpan { { GetCaretPos(), 1 } } });
	CaretMoveRight();
}

BOOL CHexCtrl::OnCommand(WPARAM wParam, LPARAM /*lParam*/)
{
	const auto wMenuID = LOWORD(wParam);
	if (const auto opt = GetCommandFromMenu(wMenuID); opt) {
		ExecuteCmd(*opt);
		return TRUE;
	}

	//For a user defined custom menu we notify the parent window.
	ParentNotify(HEXMENUINFO { .hdr { m_hWnd, static_cast<UINT>(GetDlgCtrlID()), HEXCTRL_MSG_MENUCLICK },
		.pt { m_stMenuClickedPt }, .wMenuID { wMenuID } });

	return TRUE; //An application returns nonzero if it processes this message; (c) Microsoft.
}

void CHexCtrl::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	//Notify parent that we are about to display a context menu.
	const HEXMENUINFO hmi { .hdr { m_hWnd, static_cast<UINT>(GetDlgCtrlID()), HEXCTRL_MSG_CONTEXTMENU },
		.pt { m_stMenuClickedPt = point }, .fShow { true } };
	ParentNotify(hmi);
	if (hmi.fShow) { //Parent window can disable context menu showing up.
		m_menuMain.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON, point.x, point.y, this);
	}
}

void CHexCtrl::OnDestroy()
{
	//All these cleanups below are important in case when HexCtrl window is destroyed 
	//but IHexCtrl object itself is still alive. When a window is destroyed, the IHexCtrl object 
	//is alive unless the IHexCtrl::Destroy() method is called.

	ClearData();
	m_vecHBITMAP.clear();
	m_vecKeyBind.clear();
	m_vecUndo.clear();
	m_vecRedo.clear();
	m_vecCharsWidth.clear();
	m_pDlgTemplMgr->UnloadAll(); //Explicitly unloading all loaded Templates.
	m_menuMain.DestroyMenu();
	m_fontMain.DeleteObject();
	m_fontInfoBar.DeleteObject();
	m_penLines.DeleteObject();
	m_penDataTempl.DeleteObject();
	m_pScrollV->DestroyWindow();
	m_pScrollH->DestroyWindow();
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
	using enum EHexCmd;
	//The nIndex specifies the zero-based relative position of the menu item that opens the drop-down menu or submenu.
	switch (nIndex) {
	case 0:	//Search.
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_SEARCH_DLGSEARCH, IsCmdAvail(CMD_SEARCH_DLG) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_SEARCH_NEXT, IsCmdAvail(CMD_SEARCH_NEXT) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_SEARCH_PREV, IsCmdAvail(CMD_SEARCH_PREV) ? MF_ENABLED : MF_GRAYED);
		break;
	case 3:	//Navigation.
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_NAV_DLGGOTO, IsCmdAvail(CMD_NAV_GOTO_DLG) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_NAV_REPFWD, IsCmdAvail(CMD_NAV_REPFWD) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_NAV_REPBKW, IsCmdAvail(CMD_NAV_REPBKW) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_NAV_DATABEG, IsCmdAvail(CMD_NAV_DATABEG) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_NAV_DATAEND, IsCmdAvail(CMD_NAV_DATAEND) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_NAV_PAGEBEG, IsCmdAvail(CMD_NAV_PAGEBEG) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_NAV_PAGEEND, IsCmdAvail(CMD_NAV_PAGEEND) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_NAV_LINEBEG, IsCmdAvail(CMD_NAV_LINEBEG) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_NAV_LINEEND, IsCmdAvail(CMD_NAV_LINEEND) ? MF_ENABLED : MF_GRAYED);
		break;
	case 4:	//Bookmarks.
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_BKM_ADD, IsCmdAvail(CMD_BKM_ADD) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_BKM_REMOVE, IsCmdAvail(CMD_BKM_REMOVE) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_BKM_NEXT, IsCmdAvail(CMD_BKM_NEXT) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_BKM_PREV, IsCmdAvail(CMD_BKM_PREV) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_BKM_REMOVEALL, IsCmdAvail(CMD_BKM_REMOVEALL) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_BKM_DLGMGR, IsCmdAvail(CMD_BKM_DLG_MGR) ? MF_ENABLED : MF_GRAYED);
		break;
	case 5:	//Clipboard.
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_COPYHEX, IsCmdAvail(CMD_CLPBRD_COPY_HEX) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_COPYHEXLE, IsCmdAvail(CMD_CLPBRD_COPY_HEXLE) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_COPYHEXFMT, IsCmdAvail(CMD_CLPBRD_COPY_HEXFMT) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_COPYTEXTCP, IsCmdAvail(CMD_CLPBRD_COPY_TEXTCP) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_COPYBASE64, IsCmdAvail(CMD_CLPBRD_COPY_BASE64) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_COPYCARR, IsCmdAvail(CMD_CLPBRD_COPY_CARR) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_COPYGREPHEX, IsCmdAvail(CMD_CLPBRD_COPY_GREPHEX) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_COPYPRNTSCRN, IsCmdAvail(CMD_CLPBRD_COPY_PRNTSCRN) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_COPYOFFSET, IsCmdAvail(CMD_CLPBRD_COPY_OFFSET) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_PASTEHEX, IsCmdAvail(CMD_CLPBRD_PASTE_HEX) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_PASTETEXTUTF16, IsCmdAvail(CMD_CLPBRD_PASTE_TEXTUTF16) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_PASTETEXTCP, IsCmdAvail(CMD_CLPBRD_PASTE_TEXTCP) ? MF_ENABLED : MF_GRAYED);
		break;
	case 6: //Modify.
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_MODIFY_FILLZEROS, IsCmdAvail(CMD_MODIFY_FILLZEROS) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_MODIFY_DLGFILLDATA, IsCmdAvail(CMD_MODIFY_FILLDATA_DLG) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_MODIFY_DLGOPERS, IsCmdAvail(CMD_MODIFY_OPERS_DLG) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_MODIFY_UNDO, IsCmdAvail(CMD_MODIFY_UNDO) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_MODIFY_REDO, IsCmdAvail(CMD_MODIFY_REDO) ? MF_ENABLED : MF_GRAYED);
		break;
	case 7: //Selection.
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_SEL_MARKSTART, IsCmdAvail(CMD_SEL_MARKSTART) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_SEL_MARKEND, IsCmdAvail(CMD_SEL_MARKEND) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_SEL_ALL, IsCmdAvail(CMD_SEL_ALL) ? MF_ENABLED : MF_GRAYED);
		break;
	case 8: //Templates.
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_TEMPL_APPLYCURR, IsCmdAvail(CMD_TEMPL_APPLYCURR) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_TEMPL_DISAPPLY, IsCmdAvail(CMD_TEMPL_DISAPPLY) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_TEMPL_DISAPPALL, IsCmdAvail(CMD_TEMPL_DISAPPALL) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_TEMPL_DLGMGR, IsCmdAvail(CMD_TEMPL_DLG_MGR) ? MF_ENABLED : MF_GRAYED);
		break;
	case 9: //Data Presentation.
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_DLGDATAINTERP, IsCmdAvail(CMD_DATAINTERP_DLG) ? MF_ENABLED : MF_GRAYED);
		m_menuMain.EnableMenuItem(IDM_HEXCTRL_DLGCODEPAGE, IsCmdAvail(CMD_CODEPAGE_DLG) ? MF_ENABLED : MF_GRAYED);
		break;
	default:
		break;
	}
}

void CHexCtrl::OnKeyDown(UINT nChar, UINT /*nRepCnt*/, UINT nFlags)
{
	//KF_REPEAT indicates that the key was pressed continuously.
	if (nFlags & KF_REPEAT) {
		m_fKeyDownAtm = true;
	}

	if (const auto optCmd = GetCommandFromKey(nChar, GetAsyncKeyState(VK_CONTROL) < 0,
		GetAsyncKeyState(VK_SHIFT) < 0, GetAsyncKeyState(VK_MENU) < 0); optCmd) {
		ExecuteCmd(*optCmd);
	}
	else if (IsDataSet() && IsMutable() && !IsCurTextArea()) {
		//If caret is in the Hex area then just one part (High/Low) of the byte must be changed.
		//Normalizing all input in the Hex area to only [0x0-0xF] range, allowing only [0-9], [A-F], [NUM0-NUM9].
		unsigned char chByte = nChar & 0xFF;
		if (chByte >= '0' && chByte <= '9') {
			chByte -= '0';
		}
		else if (chByte >= 'A' && chByte <= 'F') {
			chByte -= 0x37; //'A' - 0x37 = 0xA.
		}
		else if (chByte >= VK_NUMPAD0 && chByte <= VK_NUMPAD9) {
			chByte -= VK_NUMPAD0;
		}
		else
			return;

		auto chByteCurr = GetIHexTData<unsigned char>(*this, GetCaretPos());
		if (m_fCaretHigh) {
			chByte = (chByte << 4U) | (chByteCurr & 0x0FU);
		}
		else {
			chByte = (chByte & 0x0FU) | (chByteCurr & 0xF0U);
		}
		ModifyData({ .eModifyMode { EHexModifyMode::MODIFY_ONCE },
			.spnData { reinterpret_cast<std::byte*>(&chByte), sizeof(chByte) }, .vecSpan { { GetCaretPos(), 1 } } });
		CaretMoveRight();
	}
}

void CHexCtrl::OnKeyUp(UINT /*nChar*/, UINT /*nRepCnt*/, UINT /*nFlags*/)
{
	if (!IsDataSet())
		return;

	if (m_fKeyDownAtm) { //If a key was previously pressed continuously and is now released.
		m_pDlgDataInterp->UpdateData();
		m_fKeyDownAtm = false;
	}
}

void CHexCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if ((point.x + static_cast<long>(m_pScrollH->GetScrollPos())) < m_iSecondVertLinePx) { //DblClick on "Offset" area.
		SetOffsetMode(!IsOffsetAsHex());
	}
	else if (GetRectTextCaption().PtInRect(point) != FALSE) { //DblClick on codepage caption area.
		ExecuteCmd(EHexCmd::CMD_CODEPAGE_DLG);
	}
	else if (const auto optHit = HitTest(point); optHit) { //DblClick on hex/text area.
		m_fLMousePressed = true;
		m_ullCaretPos = optHit->ullOffset;
		m_fCursorTextArea = optHit->fIsText;
		if (!optHit->fIsText) {
			m_fCaretHigh = optHit->fIsHigh;
		}

		HEXSPAN hs;
		if (nFlags & MK_SHIFT) {
			const auto dwCapacity = GetCapacity();
			hs.ullOffset = m_ullCursorPrev = m_ullCursorNow = m_ullCaretPos - (m_ullCaretPos % dwCapacity);
			hs.ullSize = dwCapacity;
		}
		else {
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
	SetFocus(); //SetFocus is vital to give the proper keyboard input to the window.
	const auto optHit = HitTest(point);
	if (!optHit)
		return;

	m_fLMousePressed = true;
	m_ullCursorNow = m_ullCaretPos = optHit->ullOffset;
	m_fCursorTextArea = optHit->fIsText;
	if (!optHit->fIsText) {
		m_fCaretHigh = optHit->fIsHigh;
	}

	m_fSelectionBlock = GetAsyncKeyState(VK_MENU) < 0;
	SetCapture();

	VecSpan vecSel;
	if (nFlags & MK_SHIFT) {
		ULONGLONG ullSelStart;
		ULONGLONG ullSelEnd;
		if (m_ullCursorNow <= m_ullCursorPrev) {
			ullSelStart = m_ullCursorNow;
			ullSelEnd = m_ullCursorPrev + 1;
		}
		else {
			ullSelStart = m_ullCursorPrev;
			ullSelEnd = m_ullCursorNow + 1;
		}
		vecSel.emplace_back(ullSelStart, ullSelEnd - ullSelStart);
	}
	else {
		m_ullCursorPrev = m_ullCursorNow;
	}

	if (HasSelection() || !vecSel.empty()) { //To avoid set empty selection when it's already empty.
		SetSelection(vecSel);
	}
	else {
		Redraw();
	}

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

void CHexCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	const auto optHit = HitTest(point);

	if (m_fLMousePressed) {
		//If LMouse is pressed but cursor is outside of client area.
		//SetCapture() behaviour.

		CRect rcClient;
		GetClientRect(&rcClient);
		//Checking for scrollbars existence first.
		if (m_pScrollH->IsVisible()) {
			if (point.x < rcClient.left) {
				m_pScrollH->ScrollLineLeft();
				point.x = m_iIndentFirstHexChunkXPx;
			}
			else if (point.x >= rcClient.right) {
				m_pScrollH->ScrollLineRight();
				point.x = m_iFourthVertLinePx - 1;
			}
		}
		if (m_pScrollV->IsVisible()) {
			if (point.y < m_iStartWorkAreaYPx) {
				m_pScrollV->ScrollLineUp();
				point.y = m_iStartWorkAreaYPx;
			}
			else if (point.y >= m_iEndWorkAreaPx) {
				m_pScrollV->ScrollLineDown();
				point.y = m_iEndWorkAreaPx - 1;
			}
		}

		//Checking if the current cursor pos is at the same byte's half
		//that it was at the previous WM_MOUSEMOVE fire.
		//Making selection of the byte only if the cursor has crossed byte's halves.
		//Doesn't apply when moving in Text area.
		if (!optHit || (optHit->ullOffset == m_ullCursorNow && m_fCaretHigh == optHit->fIsHigh && !optHit->fIsText))
			return;

		m_ullCursorNow = optHit->ullOffset;
		const auto ullOffsetHit = optHit->ullOffset;
		const auto dwCapacity = GetCapacity();
		ULONGLONG ullClick;
		ULONGLONG  ullStart;
		ULONGLONG ullSize;
		ULONGLONG ullLines;
		if (m_fSelectionBlock) { //Select block (with Alt).
			ullClick = m_ullCursorPrev;
			const DWORD dwModOffset = ullOffsetHit % dwCapacity;
			const DWORD dwModClick = ullClick % dwCapacity;
			if (ullOffsetHit >= ullClick) {
				if (dwModOffset <= dwModClick) {
					const auto dwModStart = dwModClick - dwModOffset;
					ullStart = ullClick - dwModStart;
					ullSize = dwModStart + 1;
				}
				else {
					ullStart = ullClick;
					ullSize = dwModOffset - dwModClick + 1;
				}
				ullLines = (ullOffsetHit - ullStart) / dwCapacity + 1;
			}
			else {
				if (dwModOffset <= dwModClick) {
					ullStart = ullOffsetHit;
					ullSize = dwModClick - ullStart % dwCapacity + 1;
				}
				else {
					const auto dwModStart = dwModOffset - dwModClick;
					ullStart = ullOffsetHit - dwModStart;
					ullSize = dwModStart + 1;
				}
				ullLines = (ullClick - ullStart) / dwCapacity + 1;
			}
		}
		else {
			if (ullOffsetHit <= m_ullCursorPrev) {
				ullClick = m_ullCursorPrev;
				ullStart = ullOffsetHit;
				ullSize = ullClick - ullStart + 1;
			}
			else {
				ullClick = m_ullCursorPrev;
				ullStart = m_ullCursorPrev;
				ullSize = ullOffsetHit - ullClick + 1;
			}
			ullLines = 1;
		}

		m_ullCursorPrev = ullClick;
		m_ullCaretPos = ullStart;
		VecSpan vecSel;
		vecSel.reserve(static_cast<std::size_t>(ullLines));
		for (auto iterLines = 0ULL; iterLines < ullLines; ++iterLines) {
			vecSel.emplace_back(ullStart + dwCapacity * iterLines, ullSize);
		}
		SetSelection(vecSel);
	}
	else {
		if (optHit) {
			if (const auto pBkm = m_pDlgBkmMgr->HitTest(optHit->ullOffset); pBkm != nullptr) {
				if (m_pBkmTtCurr != pBkm) {
					m_pBkmTtCurr = pBkm;
					CPoint ptScreen = point;
					ClientToScreen(&ptScreen);
					ptScreen.Offset(3, 3);
					m_stToolInfoBkm.lpszText = pBkm->wstrDesc.data();
					ToolTipBkmShow(true, ptScreen);
				}
			}
			else if (m_pBkmTtCurr != nullptr) {
				ToolTipBkmShow(false);
			}
			else if (const auto pField = m_pDlgTemplMgr->HitTest(optHit->ullOffset);
				m_pDlgTemplMgr->IsTooltips() && pField != nullptr) {
				if (m_pTFieldTtCurr != pField) {
					m_pTFieldTtCurr = pField;
					CPoint ptScreen = point;
					ClientToScreen(&ptScreen);
					ptScreen.Offset(3, 3);
					m_stToolInfoTempl.lpszText = const_cast<LPWSTR>(pField->wstrName.data());
					ToolTipTemplShow(true, ptScreen);
				}
			}
			else if (m_pTFieldTtCurr != nullptr) {
				ToolTipTemplShow(false);
			}
		}
		else {
			if (m_pBkmTtCurr != nullptr) { //If there is already bkm tooltip shown, but cursor is outside of data chunks.
				ToolTipBkmShow(false);
			}

			if (m_pTFieldTtCurr != nullptr) { //If there is already Template's field tooltip shown.
				ToolTipTemplShow(false);
			}
		}

		m_pScrollV->OnMouseMove(nFlags, point);
		m_pScrollH->OnMouseMove(nFlags, point);
	}
}

BOOL CHexCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (const auto opt = GetCommandFromKey(zDelta > 0 ? m_dwVKMouseWheelUp : m_dwVKMouseWheelDown,
		nFlags & MK_CONTROL, nFlags & MK_SHIFT, false); opt) {
		ExecuteCmd(*opt);
	}

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

	if (!IsCreated()) {
		dc.FillSolidRect(rcClient, RGB(250, 250, 250));
		dc.TextOutW(1, 1, L"Call IHexCtrl::Create first.");
		return;
	}

	//To prevent drawing in too small window (can cause hangs).
	if (rcClient.IsRectEmpty() || rcClient.Height() <= m_iHeightTopRectPx + m_iHeightBottomOffAreaPx)
		return;

	const auto ullStartLine = GetTopLine();
	const auto ullEndLine = GetBottomLine();
	auto iLines = static_cast<int>(ullEndLine - ullStartLine);
	assert(iLines >= 0);
	if (iLines < 0)
		return;

	//Actual amount of lines, "ullEndLine - ullStartLine" always shows one line less.
	if (IsDataSet()) {
		++iLines;
	}

	//Drawing through CMemDC to avoid flickering.
	CMemDC memDC(dc, rcClient);
	auto pDC = &memDC.GetDC();
	DrawWindow(pDC);
	DrawInfoBar(pDC);

	if (!IsDataSet())
		return;

	DrawOffsets(pDC, ullStartLine, iLines);
	const auto& [wstrHex, wstrText] = BuildDataToDraw(ullStartLine, iLines);
	DrawHexText(pDC, ullStartLine, iLines, wstrHex, wstrText);
	DrawTemplates(pDC, ullStartLine, iLines, wstrHex, wstrText);
	DrawBookmarks(pDC, ullStartLine, iLines, wstrHex, wstrText);
	DrawSelection(pDC, ullStartLine, iLines, wstrHex, wstrText);
	DrawSelHighlight(pDC, ullStartLine, iLines, wstrHex, wstrText);
	DrawCaret(pDC, ullStartLine, wstrHex, wstrText);
	DrawDataInterp(pDC, ullStartLine, iLines, wstrHex, wstrText);
	DrawPageLines(pDC, ullStartLine, iLines);
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

	RecalcClientArea(cy, cx);
	m_pScrollV->SetScrollPageSize(GetScrollPageSize());
}

void CHexCtrl::OnSysKeyDown(UINT nChar, UINT /*nRepCnt*/, UINT /*nFlags*/)
{
	if (auto optCmd = GetCommandFromKey(nChar, GetAsyncKeyState(VK_CONTROL) < 0, GetAsyncKeyState(VK_SHIFT) < 0, true); optCmd) {
		ExecuteCmd(*optCmd);
	}
}

void CHexCtrl::OnTimer(UINT_PTR nIDEvent)
{
	static constexpr auto dbSecToShow { 5.0 }; //How many seconds to show the Tooltip.

	CRect rcClient;
	GetClientRect(rcClient);
	ClientToScreen(rcClient);
	CPoint ptCursor;
	GetCursorPos(&ptCursor);

	switch (nIDEvent) {
	case m_uIDTTBkm:
		if (!rcClient.PtInRect(ptCursor)) { //Checking if cursor has left client rect.
			ToolTipBkmShow(false);
		}
		//Or more than dbSecToShow seconds have passed since bkm toolip is shown.
		else if (std::difftime(std::time(nullptr), m_tmTtBkm) >= dbSecToShow) {
			ToolTipBkmShow(false, { }, true);
		}
		break;
	case m_uIDTTTempl:
		if (!rcClient.PtInRect(ptCursor)) { //Checking if cursor has left client rect.
			ToolTipTemplShow(false);
		}
		//Or more than dbSecToShow seconds have passed since template toolip is shown.
		else if (std::difftime(std::time(nullptr), m_tmTtTempl) >= dbSecToShow) {
			ToolTipTemplShow(false, { }, true);
		}
		break;
	default:
		break;
	}
}

void CHexCtrl::OnVScroll(UINT /*nSBCode*/, UINT /*nPos*/, CScrollBar* /*pScrollBar*/)
{
	bool fRedraw { true };
	if (m_fHighLatency) {
		fRedraw = m_pScrollV->IsThumbReleased();
		ToolTipOffsetShow(!fRedraw);
	}

	if (fRedraw) {
		RedrawWindow();
	}
}