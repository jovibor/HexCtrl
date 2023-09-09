/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "../dep/rapidjson/rapidjson-amalgam.h"
#include "../res/HexCtrlRes.h"
#include "CHexCtrl.h"
#include "CHexSelection.h"
#include "CScrollEx.h"
#include "Dialogs/CHexDlgBkmMgr.h"
#include "Dialogs/CHexDlgCallback.h"
#include "Dialogs/CHexDlgCodepage.h"
#include "Dialogs/CHexDlgDataInterp.h"
#include "Dialogs/CHexDlgGoTo.h"
#include "Dialogs/CHexDlgModify.h"
#include "Dialogs/CHexDlgSearch.h"
#include "Dialogs/CHexDlgTemplMgr.h"
#include "HexUtility.h"
#include <bit>
#include <cassert>
#include <cwctype>
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
	extern "C" HEXCTRLAPI IHexCtrl * __cdecl CreateRawHexCtrl() {
		return new CHexCtrl();
	};

	namespace INTERNAL
	{
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

		BEGIN_MESSAGE_MAP(CHexDlgAbout, CDialogEx)
			ON_WM_DESTROY()
		END_MESSAGE_MAP()

		BOOL CHexDlgAbout::OnInitDialog()
		{
			CDialogEx::OnInitDialog();

			const auto wstrVersion = std::format(L"Hex Control for MFC/Win32: v{}.{}.{}\r\nCopyright: (C)2018 - 2023 Jovibor",
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
		struct CHexCtrl::SUNDO {
			ULONGLONG              ullOffset { }; //Start byte to apply Undo to.
			std::vector<std::byte> vecData { };   //Data for Undo.
		};

		//Key bindings.
		struct CHexCtrl::SKEYBIND {
			EHexCmd eCmd { };
			WORD    wMenuID { };
			UINT    uKey { };
			bool    fCtrl { };
			bool    fShift { };
			bool    fAlt { };
		};
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

//CWinApp object is vital for manual MFC and for in-DLL work,
//to run properly (Resources handling and assertions).
#if defined HEXCTRL_MANUAL_MFC_INIT || defined HEXCTRL_SHARED_DLL
CWinApp theApp;

extern "C" HEXCTRLAPI BOOL __cdecl HexCtrlPreTranslateMessage(MSG * pMsg) {
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return theApp.PreTranslateMessage(pMsg);
}
#endif

CHexCtrl::CHexCtrl()
{
	//MFC initialization, if HexCtrl is used in non MFC project with the "Shared MFC" linking.
#if defined HEXCTRL_MANUAL_MFC_INIT
	if (!AfxGetModuleState()->m_lpszCurrentAppName)
		AfxWinInit(::GetModuleHandleW(nullptr), nullptr, ::GetCommandLineW(), 0);
#endif

	const auto hInst = AfxGetInstanceHandle();
	WNDCLASSEXW wc { };
	if (!::GetClassInfoExW(hInst, m_pwszClassName, &wc)) {
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
			MessageBoxW(L"HexControl RegisterClassExW error.", L"Error", MB_ICONERROR);
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

	//1. WS_POPUP style is vital for GetParent to work properly in EHexCreateMode::CREATE_POPUP mode.
	//   Without this style GetParent/GetOwner always return 0, no matter whether pParentWnd is provided to CreateWindowEx or not.
	//2. Created HexCtrl window will always overlap (be on top of) its parent, or owner, window 
	//   if pParentWnd is set (is not nullptr) in CreateWindowEx.
	//3. To force HexCtrl window on taskbar the WS_EX_APPWINDOW extended window style must be set.
	if (hcs.fCustom) {
		//If it's a Custom Control in dialog, there is no need to create a window, just subclassing.
		if (!SubclassDlgItem(hcs.uID, CWnd::FromHandle(hcs.hWndParent))) {
			MessageBoxW(std::format(L"HexCtrl (ID: {}) SubclassDlgItem failed.\r\nCheck HEXCREATE or CreateDialogCtrl parameters.", hcs.uID).data(),
				L"Error", MB_ICONERROR);
			return false;
		}
	}
	else {
		if (!CWnd::CreateEx(hcs.dwExStyle, m_pwszClassName, L"HexControl", hcs.dwStyle, hcs.rect,
			CWnd::FromHandle(hcs.hWndParent), hcs.uID)) {
			MessageBoxW(std::format(L"HexCtrl (ID: {}) CreateWindowExW failed.\r\nCheck HEXCREATE struct parameters.", hcs.uID).data(),
				L"Error", MB_ICONERROR);
			return false;
		}
	}

	m_wndTtBkm.CreateEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr, TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, nullptr);
	m_stToolInfoBkm.cbSize = sizeof(TTTOOLINFOW);
	m_stToolInfoBkm.uFlags = TTF_TRACK;
	m_stToolInfoBkm.uId = m_uiIDTTBkm;
	m_wndTtBkm.SendMessageW(TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&m_stToolInfoBkm));
	m_wndTtBkm.SendMessageW(TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(400)); //To allow the use of a newline \n.

	m_wndTtTempl.CreateEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr, TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, nullptr);
	m_stToolInfoTempl.cbSize = sizeof(TTTOOLINFOW);
	m_stToolInfoTempl.uFlags = TTF_TRACK;
	m_stToolInfoTempl.uId = m_uiIDTTTempl;
	m_wndTtTempl.SendMessageW(TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&m_stToolInfoTempl));
	m_wndTtTempl.SendMessageW(TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(400)); //To allow the use of a newline \n.

	m_wndTtOffset.CreateEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr, TTS_NOANIMATE | TTS_NOFADE | TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, nullptr);
	m_stToolInfoOffset.cbSize = sizeof(TTTOOLINFOW);
	m_stToolInfoOffset.uFlags = TTF_TRACK;
	m_stToolInfoOffset.uId = 0x02UL; //Tooltip ID for offset.
	m_wndTtOffset.SendMessageW(TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&m_stToolInfoOffset));
	m_wndTtOffset.SendMessageW(TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(400)); //To allow the use of a newline \n.

	m_stColor = hcs.stColor;
	m_dbWheelRatio = hcs.dbWheelRatio;
	m_fPageLines = hcs.fPageLines;
	m_fInfoBar = hcs.fInfoBar;

	const auto pDC = GetDC();
	m_iLOGPIXELSY = GetDeviceCaps(pDC->m_hDC, LOGPIXELSY);
	ReleaseDC(pDC);

	//Menu related.
	if (!m_menuMain.LoadMenuW(MAKEINTRESOURCEW(IDR_HEXCTRL_MENU))) {
		MessageBoxW(L"HexControl LoadMenuW failed.", L"Error", MB_ICONERROR);
		return false;
	}

	MENUITEMINFOW mii { .cbSize { sizeof(MENUITEMINFOW) }, .fMask { MIIM_BITMAP } };
	const auto hInst = AfxGetInstanceHandle();
	const auto fScale = m_iLOGPIXELSY / 96.0F; //Scale factor for HighDPI displays.
	const auto iSizeIcon = static_cast<int>(16 * fScale);
	const auto pMenuTop = m_menuMain.GetSubMenu(0); //Context sub-menu handle.

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
	const LOGFONTW lfMain { .lfHeight { -MulDiv(11, m_iLOGPIXELSY, 72) }, .lfPitchAndFamily { FIXED_PITCH }, .lfFaceName { L"Consolas" } };
	m_fontMain.CreateFontIndirectW(hcs.pLogFont != nullptr ? hcs.pLogFont : &lfMain);

	//Info area font, independent from the main font, its size is a bit smaller than the default main font.
	const LOGFONTW lfInfo { .lfHeight { -MulDiv(11, m_iLOGPIXELSY, 72) + 1 }, .lfPitchAndFamily { FIXED_PITCH }, .lfFaceName { L"Consolas" } };
	m_fontInfoBar.CreateFontIndirectW(&lfInfo);
	//End of font related.

	m_penLines.CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
	m_penDataTempl.CreatePen(PS_SOLID, 1, RGB(50, 50, 50));

	//Removing window's border frame.
	const MARGINS marg { 0, 0, 0, 1 };
	DwmExtendFrameIntoClientArea(m_hWnd, &marg);
	SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

	//ScrollBars should be created here, after the main window has already been created (to attach to), to avoid assertions.
	m_pScrollV->Create(this, true, IDB_HEXCTRL_SCROLL_ARROW, 0, 0, 0); //Actual sizes are set in RecalcAll().
	m_pScrollH->Create(this, false, IDB_HEXCTRL_SCROLL_ARROW, 0, 0, 0);
	m_pScrollV->AddSibling(m_pScrollH.get());
	m_pScrollH->AddSibling(m_pScrollV.get());

	m_fCreated = true; //Main creation flag.

	SetGroupMode(m_enGroupMode);
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

	return Create({ .hWndParent { hWndParent }, .uID { uCtrlID }, .dwStyle { WS_VISIBLE | WS_CHILD }, .fCustom { true } });
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
	case CMD_GROUPBY_BYTE:
		SetGroupMode(EHexDataSize::SIZE_BYTE);
		break;
	case CMD_GROUPBY_WORD:
		SetGroupMode(EHexDataSize::SIZE_WORD);
		break;
	case CMD_GROUPBY_DWORD:
		SetGroupMode(EHexDataSize::SIZE_DWORD);
		break;
	case CMD_GROUPBY_QWORD:
		SetGroupMode(EHexDataSize::SIZE_QWORD);
		break;
	case CMD_BKM_ADD:
		m_pDlgBkmMgr->AddBkm(HEXBKM { HasSelection() ? GetSelection()
			: VecSpan { { GetCaretPos(), 1 } } }, true);
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
		return -1;

	return m_iFourthVertLine + 1; //+1px is the Pen width the line was drawn with.
}

auto CHexCtrl::GetBookmarks()const->IHexBookmarks*
{
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

int CHexCtrl::GetCodepage()const
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	return m_iCodePage;
}

auto CHexCtrl::GetColors()const->HEXCOLORS
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	return m_stColor;
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

	//Returns defaults even if is not IsCreated.
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
		return{ };

	LOGFONTW lf;
	m_fontMain.GetLogFont(&lf);
	return lf;
}

auto CHexCtrl::GetGroupMode()const->EHexDataSize
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	return m_enGroupMode;
}

auto CHexCtrl::GetMenuHandle()const->HMENU
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
	return &*m_pDlgTemplMgr;
}

auto CHexCtrl::GetUnprintableChar()const->wchar_t
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	return m_wchUnprintable;
}

auto CHexCtrl::GetWindowHandle(EHexWnd eWnd)const->HWND
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	switch (eWnd) {
	case EHexWnd::WND_MAIN:
		return m_hWnd;
	case EHexWnd::DLG_BKMMGR:
		if (!IsWindow(m_pDlgBkmMgr->m_hWnd)) {
			m_pDlgBkmMgr->Create(IDD_HEXCTRL_BKMMGR, CWnd::FromHandle(m_hWnd));
		}
		return m_pDlgBkmMgr->m_hWnd;
	case EHexWnd::DLG_DATAINTERP:
		if (!IsWindow(m_pDlgDataInterp->m_hWnd)) {
			m_pDlgDataInterp->Create(IDD_HEXCTRL_DATAINTERP, CWnd::FromHandle(m_hWnd));
		}
		return m_pDlgDataInterp->m_hWnd;
	case EHexWnd::DLG_MODIFY:
		if (!IsWindow(m_pDlgModify->m_hWnd)) {
			m_pDlgModify->Create(IDD_HEXCTRL_MODIFY, CWnd::FromHandle(m_hWnd));
		}
		return m_pDlgModify->m_hWnd;
	case EHexWnd::DLG_SEARCH:
		if (!IsWindow(m_pDlgSearch->m_hWnd)) {
			m_pDlgSearch->Create(IDD_HEXCTRL_SEARCH, CWnd::FromHandle(m_hWnd));
		}
		return m_pDlgSearch->m_hWnd;
	case EHexWnd::DLG_CODEPAGE:
		if (!IsWindow(m_pDlgCodepage->m_hWnd)) {
			m_pDlgCodepage->Create(IDD_HEXCTRL_CODEPAGE, CWnd::FromHandle(m_hWnd));
		}
		return m_pDlgCodepage->m_hWnd;
	case EHexWnd::DLG_GOTO:
		if (!IsWindow(m_pDlgGoTo->m_hWnd)) {
			m_pDlgGoTo->Create(IDD_HEXCTRL_GOTO, CWnd::FromHandle(m_hWnd));
		}
		return m_pDlgGoTo->m_hWnd;
	case EHexWnd::DLG_TEMPLMGR:
		if (!IsWindow(m_pDlgTemplMgr->m_hWnd)) {
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

	const auto ullNewStartV = ullOffset / m_dwCapacity * m_sizeFontMain.cy;
	auto ullNewScrollV { 0ULL };

	switch (iRelPos) {
	case -1: //Offset will be at the top line.
		ullNewScrollV = ullNewStartV;
		break;
	case 0: //Offset at the middle.
		if (ullNewStartV > m_iHeightWorkArea / 2) { //To prevent negative numbers.
			ullNewScrollV = ullNewStartV - m_iHeightWorkArea / 2;
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

	m_vecRedo.clear(); //No Redo unless we make Undo.
	SnapshotUndo(hms.vecSpan);

	SetRedraw(false);
	using enum EHexModifyMode;
	switch (hms.enModifyMode) {
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
			const auto iMod = ullSizeToModify % ullSizeCache;
			auto ullChunks = ullSizeToModify / ullSizeCache + (iMod > 0 ? 1 : 0);
			auto ullOffsetCurr = ullOffsetToModify;
			auto ullOffsetSpanCurr = 0ULL;
			while (ullChunks-- > 0) {
				const auto ullSizeToModifyCurr = (ullChunks == 1 && iMod > 0) ? iMod : ullSizeCache;
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
		if (hms.enModifyMode == MODIFY_RAND_MT19937 && hms.vecSpan.size() == 1 && refHexSpan.ullSize >= sizeof(std::uint64_t)) {
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
		else if (hms.enModifyMode == MODIFY_RAND_FAST && hms.vecSpan.size() == 1 && refHexSpan.ullSize >= GetCacheSize()) {
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
			if (const auto dwRem = refHexSpan.ullSize % ulSizeRandBuff; dwRem > 0) { //Remainder.
				if (dwRem <= GetCacheSize()) {
					const auto ullOffsetCurr = refHexSpan.ullOffset + refHexSpan.ullSize - dwRem;
					const auto spnData = GetData({ ullOffsetCurr, dwRem });
					assert(!spnData.empty());
					std::copy_n(uptrRandData.get(), dwRem, spnData.data());
					SetDataVirtual(spnData, { ullOffsetCurr, dwRem });
				}
				else {
					const auto ullSizeCache = GetCacheSize();
					const auto dwModCache = dwRem % ullSizeCache;
					auto ullChunks = dwRem / ullSizeCache + (dwModCache > 0 ? 1 : 0);
					auto ullOffsetCurr = refHexSpan.ullOffset + refHexSpan.ullSize - dwRem;
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
		//and the size of the repeated data is equal to the extent of 2,
		//we extend that repeated data to iBuffSizeForFastFill size to speed up 
		//the whole process of repeating.
		//At the end we simply fill up the remainder (ullSizeToModify % ulSizeBuffFastFill ).
		const auto ullOffsetToModify = hms.vecSpan.back().ullOffset;
		const auto ullSizeToModify = hms.vecSpan.back().ullSize;
		const auto ullSizeToFillWith = hms.spnData.size();
		constexpr auto ulSizeBuffFastFill { 256U };

		if (hms.vecSpan.size() == 1 && ullSizeToModify > ulSizeBuffFastFill
			&& ullSizeToFillWith < ulSizeBuffFastFill && (ulSizeBuffFastFill % ullSizeToFillWith) == 0) {
			alignas(32) std::byte buffFillData[ulSizeBuffFastFill]; //Buffer for fast data fill.
			for (auto iter = 0ULL; iter < ulSizeBuffFastFill; iter += ullSizeToFillWith) {
				std::copy_n(hms.spnData.data(), ullSizeToFillWith, buffFillData + iter);
			}

			ModifyWorker(hms, lmbRepeat, { buffFillData, ulSizeBuffFastFill });

			if (const auto dwRem = ullSizeToModify % ulSizeBuffFastFill; dwRem >= ullSizeToFillWith) { //Remainder.
				const auto ullOffset = ullOffsetToModify + ullSizeToModify - dwRem;
				const auto spnData = GetData({ ullOffset, dwRem });
				for (std::size_t iterRem = 0; iterRem < (dwRem / ullSizeToFillWith); ++iterRem) { //Works only if dwRem >= ullSizeToFillWith.
					std::copy_n(hms.spnData.data(), ullSizeToFillWith, spnData.data() + (iterRem * ullSizeToFillWith));
				}
				SetDataVirtual(spnData, { ullOffset, dwRem - (dwRem % ullSizeToFillWith) });
			}
		}
		else {
			ModifyWorker(hms, lmbRepeat, hms.spnData);
		}
	}
	break;
	case MODIFY_OPERATION:
	{
		constexpr auto lmbOperData = [](std::byte* pData, const HEXMODIFY& hms, SpanCByte/**/) {
			assert(pData != nullptr);

			constexpr auto lmbOper = []<typename T>(T * pData, const HEXMODIFY & hms) {
				T tData = hms.fBigEndian ? ByteSwap(*pData) : *pData;
				const T tDataOper = *reinterpret_cast<const T*>(hms.spnData.data());

				using enum EHexOperMode;
				switch (hms.enOperMode) {
				case OPER_ASSIGN:
					tData = tDataOper;
					break;
				case OPER_ADD:
					tData += tDataOper;
					break;
				case OPER_SUB:
					tData -= tDataOper;
					break;
				case OPER_MUL:
					tData *= tDataOper;
					break;
				case OPER_DIV:
					if (tDataOper > 0) { //Division by zero check.
						tData /= tDataOper;
					}
					break;
				case OPER_CEIL:
					if (tData > tDataOper) {
						tData = tDataOper;
					}
					break;
				case OPER_FLOOR:
					if (tData < tDataOper) {
						tData = tDataOper;
					}
					break;
				case OPER_OR:
					tData |= tDataOper;
					break;
				case OPER_XOR:
					tData ^= tDataOper;
					break;
				case OPER_AND:
					tData &= tDataOper;
					break;
				case OPER_NOT:
					tData = ~tData;
					break;
				case OPER_SHL:
					tData <<= tDataOper;
					break;
				case OPER_SHR:
					tData >>= tDataOper;
					break;
				case OPER_ROTL:
					tData = std::rotl(tData, static_cast<int>(tDataOper));
					break;
				case OPER_ROTR:
					tData = std::rotr(tData, static_cast<int>(tDataOper));
					break;
				case OPER_SWAP:
					tData = ByteSwap(tData);
					break;
				case OPER_BITREV:
					tData = BitReverse(tData);
					break;
				default:
					break;
				}

				if (hms.fBigEndian) { //Swap bytes back.
					tData = ByteSwap(tData);
				}

				*pData = tData;
			};

			using enum EHexDataSize;
			switch (hms.enDataSize) {
			case SIZE_BYTE:
				lmbOper(reinterpret_cast<PBYTE>(pData), hms);
				break;
			case SIZE_WORD:
				lmbOper(reinterpret_cast<PWORD>(pData), hms);
				break;
			case SIZE_DWORD:
				lmbOper(reinterpret_cast<PDWORD>(pData), hms);
				break;
			case SIZE_QWORD:
				lmbOper(reinterpret_cast<PQWORD>(pData), hms);
				break;
			}
		};
		ModifyWorker(hms, lmbOperData, { static_cast<std::byte*>(nullptr), static_cast<std::size_t>(hms.enDataSize) });
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

	m_wstrInfoBar.clear();

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

	RedrawWindow();
}

void CHexCtrl::SetCapacity(DWORD dwCapacity)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	constexpr auto dwCapacityMax { 0x64U }; //Maximum capacity allowed.

	if (dwCapacity < 1 || dwCapacity > dwCapacityMax)
		return;

	//Setting capacity according to current m_enGroupMode.
	const auto dwRem = dwCapacity % static_cast<DWORD>(m_enGroupMode);
	if (dwCapacity < m_dwCapacity) {
		dwCapacity -= dwRem;
	}
	else if (dwRem > 0) {
		dwCapacity += static_cast<DWORD>(m_enGroupMode) - dwRem;
	}

	//To prevent under/over flow.
	if (dwCapacity < static_cast<DWORD>(m_enGroupMode)) {
		dwCapacity = static_cast<DWORD>(m_enGroupMode);
	}
	else if (dwCapacity > dwCapacityMax) {
		dwCapacity = dwCapacityMax;
	}

	m_dwCapacity = dwCapacity;
	m_dwCapacityBlockSize = m_dwCapacity / 2;

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

void CHexCtrl::SetColors(const HEXCOLORS& clr)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_stColor = clr;
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
		{ "CMD_GROUPBY_BYTE", { CMD_GROUPBY_BYTE, IDM_HEXCTRL_GROUPBY_BYTE } },
		{ "CMD_GROUPBY_WORD", { CMD_GROUPBY_WORD, IDM_HEXCTRL_GROUPBY_WORD } },
		{ "CMD_GROUPBY_DWORD", { CMD_GROUPBY_DWORD, IDM_HEXCTRL_GROUPBY_DWORD } },
		{ "CMD_GROUPBY_QWORD", { CMD_GROUPBY_QWORD, IDM_HEXCTRL_GROUPBY_QWORD } },
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
	//Mapping between JSON-data Command Names and actual keyboard codes with the names that appear in the menu.
	const std::unordered_map<std::string_view, std::pair<UINT, std::wstring_view>> umapKeys {
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

	//Filling m_vecKeyBind with ALL available commands/menus.
	//This is vital for ExecuteCmd to work properly.
	m_vecKeyBind.clear();
	m_vecKeyBind.reserve(umapCmdMenu.size());
	for (const auto& itMap : umapCmdMenu) {
		m_vecKeyBind.emplace_back(SKEYBIND { .eCmd { itMap.second.first }, .wMenuID { static_cast<WORD>(itMap.second.second) } });
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
		const auto lmbParseStr = [&](std::string_view sv)->std::optional<SKEYBIND> {
			if (sv.empty())
				return { };

			SKEYBIND stRet { };
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
						if (auto it = std::find_if(m_vecKeyBind.begin(), m_vecKeyBind.end(), [&optKey](const SKEYBIND& ref) {
							return ref.eCmd == optKey->eCmd; }); it != m_vecKeyBind.end()) {
							if (it->uKey == 0) {
								*it = *optKey; //Adding keybindings from JSON to m_vecKeyBind.
							}
							else {
								//If such command with some key from JSON already exist, we adding another one
								//same command but with different key, like Ctrl+F/Ctrl+H for Search.
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
			if (const auto iterTmp = std::find_if(m_vecKeyBind.begin(), iterEnd, [&](const SKEYBIND& ref) {
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

	m_fDataSet = true;
	m_spnData = hds.spnData;
	m_pHexVirtData = hds.pHexVirtData;
	m_pHexVirtColors = hds.pHexVirtColors;
	constexpr auto dwCacheMinSize { 1024U * 64U };      //64Kb is the minimum default cache size.
	m_dwCacheSize = ((hds.dwCacheSize < dwCacheMinSize)	//Cache size must be at least sizeof(std::uint64_t) aligned.
		|| (hds.dwCacheSize % sizeof(std::uint64_t) != 0)) ? dwCacheMinSize : hds.dwCacheSize;
	m_fMutable = hds.fMutable;
	m_fHighLatency = hds.fHighLatency;

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

auto CHexCtrl::SetDlgData(EHexWnd eWnd, std::uint64_t ullData)->HWND
{
	assert(IsCreated());
	if (!IsCreated())
		return { };

	switch (eWnd) {
	case EHexWnd::WND_MAIN:
		return m_hWnd;
	case EHexWnd::DLG_BKMMGR:
		return m_pDlgBkmMgr->SetDlgData(ullData);
	case EHexWnd::DLG_DATAINTERP:
		return m_pDlgDataInterp->SetDlgData(ullData);
	case EHexWnd::DLG_MODIFY:
		return m_pDlgModify->SetDlgData(ullData);
	case EHexWnd::DLG_SEARCH:
		return m_pDlgSearch->SetDlgData(ullData);
	case EHexWnd::DLG_CODEPAGE:
		return m_pDlgCodepage->SetDlgData(ullData);
	case EHexWnd::DLG_GOTO:
		return m_pDlgGoTo->SetDlgData(ullData);
	case EHexWnd::DLG_TEMPLMGR:
		return m_pDlgTemplMgr->SetDlgData(ullData);
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

void CHexCtrl::SetGroupMode(EHexDataSize eGroupMode)
{
	assert(IsCreated());
	assert(eGroupMode <= EHexDataSize::SIZE_QWORD);
	if (!IsCreated() || eGroupMode > EHexDataSize::SIZE_QWORD)
		return;

	m_enGroupMode = eGroupMode;

	//Getting the "Show data as..." menu pointer independent of position.
	const auto* const pMenuMain = m_menuMain.GetSubMenu(0);
	CMenu* pMenuShowDataAs { };
	for (int i = 0; i < pMenuMain->GetMenuItemCount(); ++i) {
		//Searching through all submenus whose first menuID is IDM_HEXCTRL_GROUPBY_BYTE.
		if (const auto pSubMenu = pMenuMain->GetSubMenu(i); pSubMenu != nullptr) {
			if (pSubMenu->GetMenuItemID(0) == IDM_HEXCTRL_GROUPBY_BYTE) {
				pMenuShowDataAs = pSubMenu;
				break;
			}
		}
	}

	if (pMenuShowDataAs) {
		//Unchecking all menus and checking only the currently selected.
		for (int i = 0; i < pMenuShowDataAs->GetMenuItemCount(); ++i) {
			pMenuShowDataAs->CheckMenuItem(i, MF_UNCHECKED | MF_BYPOSITION);
		}

		UINT ID { };
		switch (eGroupMode) {
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
	ParentNotify(HEXCTRL_MSG_SETGROUPMODE);
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

void CHexCtrl::SetVirtualBkm(IHexBookmarks* pVirtBkm)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_pDlgBkmMgr->SetVirtual(pVirtBkm);
}

void CHexCtrl::SetRedraw(bool fRedraw)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_fRedraw = fRedraw;
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
}

void CHexCtrl::SetWheelRatio(double dbRatio, bool fLines)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_dbWheelRatio = dbRatio;
	m_fPageLines = fLines;
	ULONGLONG ullScrollPageSize;
	if (m_fPageLines) { //How many text lines to scroll.
		ullScrollPageSize = m_sizeFontMain.cy * static_cast<LONG>(dbRatio); //How many text lines to scroll.
		if (ullScrollPageSize < m_sizeFontMain.cy) {
			ullScrollPageSize = m_sizeFontMain.cy;
		}
	}
	else { //How mane screens to scroll.
		const auto ullSizeInScreens = static_cast<ULONGLONG>(m_iHeightWorkArea * m_dbWheelRatio);
		ullScrollPageSize = ullSizeInScreens < m_sizeFontMain.cy ? m_sizeFontMain.cy : ullSizeInScreens;
	}
	m_pScrollV->SetScrollPageSize(ullScrollPageSize);
}

void CHexCtrl::ShowInfoBar(bool fShow)
{
	assert(IsCreated());
	if (!IsCreated())
		return;

	m_fInfoBar = fShow;
	RecalcAll();
}


/********************************************************************
* HexCtrl Private methods.
********************************************************************/

auto CHexCtrl::BuildDataToDraw(ULONGLONG ullStartLine, int iLines)const->std::tuple<std::wstring, std::wstring>
{
	if (!IsDataSet())
		return { };

	const auto ullOffsetStart = ullStartLine * m_dwCapacity; //Offset of the visible data to print.
	const auto ullDataSize = GetDataSize();
	auto sSizeDataToPrint = static_cast<std::size_t>(iLines) * m_dwCapacity; //Size of the visible data to print.

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
	const auto ullNewPos = ullOldPos + m_dwCapacity >= GetDataSize() ? ullOldPos : ullOldPos + m_dwCapacity;
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
	const auto ullNewPos = ullOldPos >= m_dwCapacity ? ullOldPos - m_dwCapacity : ullOldPos;
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
	CHOOSEFONTW chf { .lStructSize = sizeof(CHOOSEFONTW), .hwndOwner = m_hWnd, .lpLogFont = &lf,
		.Flags = CF_EFFECTS | CF_FIXEDPITCHONLY | CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_NOSIMULATIONS,
		.rgbColors = stClr.clrFontHex };

	if (ChooseFontW(&chf) != FALSE) {
		SetFont(lf);
		stClr.clrFontHex = stClr.clrFontText = chf.rgbColors;
		SetColors(stClr);
	}
}

void CHexCtrl::ClipboardCopy(EClipboard eType)const
{
	if (m_pSelection->GetSelSize() > 1024 * 1024 * 8) { //8MB
		::MessageBoxW(nullptr, L"Selection size is too big to copy.\r\nTry to select less.", L"Error", MB_ICONERROR);
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
		::MessageBoxW(nullptr, L"GlobalAlloc error.", L"Error", MB_ICONERROR);
		return;
	}

	const auto lpMemLock = GlobalLock(hMem);
	if (!lpMemLock) {
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

	wstrData.reserve(static_cast<std::size_t>(ullSelSize) * 3);
	if (m_fSelectionBlock) {
		auto dwTail = m_pSelection->GetLineLength();
		for (auto i { 0U }; i < ullSelSize; ++i) {
			const auto chByte = GetIHexTData<BYTE>(*this, m_pSelection->GetOffsetByIndex(i));
			wstrData += m_pwszHexChars[(chByte & 0xF0) >> 4];
			wstrData += m_pwszHexChars[(chByte & 0x0F)];

			if (i < (ullSelSize - 1) && (dwTail - 1) != 0) {
				if (((m_pSelection->GetLineLength() - dwTail + 1) % static_cast<DWORD>(m_enGroupMode)) == 0) { //Add space after hex full chunk, ShowAs_size depending.
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
		const DWORD dwModStart = ullSelStart % m_dwCapacity;

		//When to insert first "\r\n".
		DWORD dwTail = m_dwCapacity - dwModStart;
		const DWORD dwNextBlock = (m_dwCapacity % 2) ? m_dwCapacityBlockSize + 2 : m_dwCapacityBlockSize + 1;

		//If at least two rows are selected.
		if (dwModStart + ullSelSize > m_dwCapacity) {
			DWORD dwCount = dwModStart * 2 + dwModStart / static_cast<DWORD>(m_enGroupMode);
			//Additional spaces between halves. Only in SIZE_BYTE's view mode.
			dwCount += (m_enGroupMode == EHexDataSize::SIZE_BYTE) ? (dwTail <= m_dwCapacityBlockSize ? 2 : 0) : 0;
			wstrData.insert(0, static_cast<std::size_t>(dwCount), ' ');
		}

		for (auto i { 0U }; i < ullSelSize; ++i) {
			const auto chByte = GetIHexTData<BYTE>(*this, m_pSelection->GetOffsetByIndex(i));
			wstrData += m_pwszHexChars[(chByte & 0xF0) >> 4];
			wstrData += m_pwszHexChars[(chByte & 0x0F)];

			if (i < (ullSelSize - 1) && (dwTail - 1) != 0) {
				if (m_enGroupMode == EHexDataSize::SIZE_BYTE && dwTail == dwNextBlock) {
					wstrData += L"   "; //Additional spaces between halves. Only in SIZE_BYTE view mode.
				}
				else if (((m_dwCapacity - dwTail + 1) % static_cast<DWORD>(m_enGroupMode)) == 0) { //Add space after hex full chunk, ShowAs_size depending.
					wstrData += L" ";
				}
			}
			if (--dwTail == 0 && i < (ullSelSize - 1)) { //Next row.
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
	wchar_t pwszBuff[32];
	OffsetToString(GetCaretPos(), pwszBuff);
	std::wstring wstrData { IsOffsetAsHex() ? L"0x" : L"" };
	wstrData += pwszBuff;

	return wstrData;
}

auto CHexCtrl::CopyPrintScreen()const->std::wstring
{
	if (m_fSelectionBlock) //Only works with classical selection.
		return { };

	const auto ullSelStart = m_pSelection->GetSelStart();
	const auto ullSelSize = m_pSelection->GetSelSize();

	std::wstring wstrRet { };
	wstrRet.reserve(static_cast<std::size_t>(ullSelSize) * 4);
	wstrRet = L"Offset";
	wstrRet.insert(0, (static_cast<std::size_t>(m_dwOffsetDigits) - wstrRet.size()) / 2, ' ');
	wstrRet.insert(wstrRet.size(), static_cast<std::size_t>(m_dwOffsetDigits) - wstrRet.size(), ' ');
	wstrRet += L"   "; //Spaces to Capacity.
	wstrRet += m_wstrCapacity;
	wstrRet += L"   "; //Spaces to Text.
	if (const auto iSize = static_cast<int>(m_dwCapacity) - static_cast<int>(m_wstrTextTitle.size()); iSize > 0) {
		wstrRet.insert(wstrRet.size(), static_cast<std::size_t>(iSize / 2), ' ');
	}
	wstrRet += m_wstrTextTitle;
	wstrRet += L"\r\n";

	//How many spaces are needed to be inserted at the beginning.
	DWORD dwModStart = ullSelStart % m_dwCapacity;

	auto dwLines = static_cast<DWORD>(ullSelSize) / m_dwCapacity;
	if ((dwModStart + static_cast<DWORD>(ullSelSize)) > m_dwCapacity) {
		++dwLines;
	}
	if ((ullSelSize % m_dwCapacity) + dwModStart > m_dwCapacity) {
		++dwLines;
	}
	if (!dwLines) {
		dwLines = 1;
	}

	const auto ullStartLine = ullSelStart / m_dwCapacity;
	const auto dwStartOffset = dwModStart; //Offset from the line start in the wstrHex.
	const auto& [wstrHex, wstrText] = BuildDataToDraw(ullStartLine, static_cast<int>(dwLines));
	std::wstring wstrDataText;
	std::size_t sIndexToPrint { 0 };

	for (auto iterLines { 0U }; iterLines < dwLines; ++iterLines) {
		wchar_t pwszBuff[32]; //To be enough for max as Hex and as Decimals.
		OffsetToString(ullStartLine * m_dwCapacity + m_dwCapacity * iterLines, pwszBuff);
		wstrRet += pwszBuff;
		wstrRet.insert(wstrRet.size(), 3, ' ');

		for (auto iterChunks { 0U }; iterChunks < m_dwCapacity; ++iterChunks) {
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
			if (((iterChunks + 1) % static_cast<DWORD>(m_enGroupMode)) == 0 && iterChunks < (m_dwCapacity - 1)) {
				wstrRet += L" ";
			}

			//Additional space between capacity halves, only with SIZE_BYTE representation.
			if (m_enGroupMode == EHexDataSize::SIZE_BYTE && iterChunks == (m_dwCapacityBlockSize - 1)) {
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
	pDC->MoveTo(m_iFirstVertLine - iScrollH, m_iThirdHorzLine);
	pDC->LineTo(m_iFourthVertLine, m_iThirdHorzLine);

	//Fourth horizontal line.
	pDC->MoveTo(m_iFirstVertLine - iScrollH, m_iFourthHorzLine);
	pDC->LineTo(m_iFourthVertLine, m_iFourthHorzLine);

	//First Vertical line.
	pDC->MoveTo(m_iFirstVertLine - iScrollH, m_iFirstHorzLine);
	pDC->LineTo(m_iFirstVertLine - iScrollH, m_iFourthHorzLine);

	//Second Vertical line.
	pDC->MoveTo(m_iSecondVertLine - iScrollH, m_iFirstHorzLine);
	pDC->LineTo(m_iSecondVertLine - iScrollH, m_iThirdHorzLine);

	//Third Vertical line.
	pDC->MoveTo(m_iThirdVertLine - iScrollH, m_iFirstHorzLine);
	pDC->LineTo(m_iThirdVertLine - iScrollH, m_iThirdHorzLine);

	//Fourth Vertical line.
	pDC->MoveTo(m_iFourthVertLine - iScrollH, m_iFirstHorzLine);
	pDC->LineTo(m_iFourthVertLine - iScrollH, m_iFourthHorzLine);

	//«Offset» text.
	CRect rcOffset(m_iFirstVertLine - iScrollH, m_iFirstHorzLine, m_iSecondVertLine - iScrollH, m_iSecondHorzLine);
	pDC->SelectObject(m_fontMain);
	pDC->SetTextColor(m_stColor.clrFontCaption);
	pDC->SetBkColor(m_stColor.clrBk);
	pDC->DrawTextW(L"Offset", 6, rcOffset, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	//Capacity numbers.
	ExtTextOutW(pDC->m_hDC, m_iIndentFirstHexChunkX - iScrollH, m_iFirstHorzLine + m_iIndentCapTextY, 0, nullptr,
		m_wstrCapacity.data(), static_cast<UINT>(m_wstrCapacity.size()), nullptr);

	//Text area caption.
	auto rcText = GetRectTextCaption();
	pDC->DrawTextW(m_wstrTextTitle.data(), static_cast<int>(m_wstrTextTitle.size()), rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void CHexCtrl::DrawInfoBar(CDC* pDC)const
{
	if (!IsInfoBar())
		return;

	const auto iScrollH = static_cast<int>(m_pScrollH->GetScrollPos());
	CRect rcInfoBar(m_iFirstVertLine + 1 - iScrollH, m_iThirdHorzLine + 1, m_iFourthVertLine, m_iFourthHorzLine); //Info bar rc until m_iFourthHorizLine.
	pDC->FillSolidRect(rcInfoBar, m_stColor.clrBkInfoBar);
	pDC->DrawEdge(rcInfoBar, BDR_RAISEDINNER, BF_TOP);

	CRect rcText = rcInfoBar;
	rcText.left = m_iFirstVertLine + 5; //Draw the text beginning with little indent.
	rcText.right = m_iFirstVertLine + m_iWidthClientArea; //Draw text to the end of the client area, even if it passes iFourthHorizLine.
	pDC->SelectObject(m_fontInfoBar);
	pDC->SetBkColor(m_stColor.clrBkInfoBar);

	std::size_t sCurrPosBegin { };
	std::size_t sCurrPosEnd { };
	while (sCurrPosBegin != std::wstring::npos) {
		if (sCurrPosBegin = m_wstrInfoBar.find_first_of('^', sCurrPosBegin); sCurrPosBegin != std::wstring::npos) {
			if (sCurrPosEnd = m_wstrInfoBar.find_first_of('^', sCurrPosBegin + 1); sCurrPosEnd != std::wstring::npos) {
				const auto iSize = static_cast<int>(sCurrPosEnd - sCurrPosBegin - 1);
				pDC->SetTextColor(m_stColor.clrFontInfoParam);
				pDC->DrawTextW(m_wstrInfoBar.data() + sCurrPosBegin + 1, iSize, rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
				rcText.left += iSize * m_sizeFontInfo.cx;
			}
			sCurrPosBegin = sCurrPosEnd + 1;
		}

		if (sCurrPosBegin = m_wstrInfoBar.find_first_of('`', sCurrPosBegin); sCurrPosBegin != std::wstring::npos) {
			if (sCurrPosEnd = m_wstrInfoBar.find_first_of('`', sCurrPosBegin + 1); sCurrPosEnd != std::wstring::npos) {
				const auto size = sCurrPosEnd - sCurrPosBegin - 1;
				pDC->SetTextColor(m_stColor.clrFontInfoData);
				pDC->DrawTextW(m_wstrInfoBar.data() + sCurrPosBegin + 1, static_cast<int>(size), rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
				rcText.left += static_cast<long>(size * m_sizeFontInfo.cx);
			}
			sCurrPosBegin = sCurrPosEnd + 1;
		}
	}
}

void CHexCtrl::DrawOffsets(CDC* pDC, ULONGLONG ullStartLine, int iLines)const
{
	const auto ullStartOffset = ullStartLine * m_dwCapacity;
	const auto iScrollH = static_cast<int>(m_pScrollH->GetScrollPos());

	for (auto iterLines = 0; iterLines < iLines; ++iterLines) {
		//Drawing offset with bk color depending on the selection range.
		COLORREF clrTextOffset;
		COLORREF clrBkOffset;
		if (m_pSelection->HitTestRange({ ullStartOffset + iterLines * m_dwCapacity, m_dwCapacity })) {
			clrTextOffset = m_stColor.clrFontSel;
			clrBkOffset = m_stColor.clrBkSel;
		}
		else {
			clrTextOffset = m_stColor.clrFontCaption;
			clrBkOffset = m_stColor.clrBk;
		}

		//Left column offset printing (00000001...0000FFFF).
		wchar_t pwszBuff[32]; //To be enough for max as Hex and as Decimals.
		OffsetToString((ullStartLine + iterLines) * m_dwCapacity, pwszBuff);
		pDC->SelectObject(m_fontMain);
		pDC->SetTextColor(clrTextOffset);
		pDC->SetBkColor(clrBkOffset);
		ExtTextOutW(pDC->m_hDC, m_iFirstVertLine + m_sizeFontMain.cx - iScrollH, m_iStartWorkAreaY + (m_sizeFontMain.cy * iterLines),
			0, nullptr, pwszBuff, m_dwOffsetDigits, nullptr);
	}
}

void CHexCtrl::DrawHexText(CDC* pDC, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const
{
	std::vector<POLYTEXTW> vecPolyHex;
	std::vector<POLYTEXTW> vecPolyText;
	vecPolyHex.reserve(static_cast<std::size_t>(iLines));
	vecPolyText.reserve(static_cast<std::size_t>(iLines));

	std::vector<std::wstring> vecWstrHex;
	std::vector<std::wstring> vecWstrText;
	vecWstrHex.reserve(static_cast<std::size_t>(iLines));  //Amount of strings is exactly the same as iLines.
	vecWstrText.reserve(static_cast<std::size_t>(iLines));

	const auto iScrollH = static_cast<int>(m_pScrollH->GetScrollPos());
	std::size_t sIndexToPrint { };

	for (auto iterLines = 0; iterLines < iLines; ++iterLines) {
		std::wstring wstrHexToPrint;  //Hex/Text wstrings to print.
		std::wstring wstrTextToPrint;
		const auto iHexPosToPrintX = m_iIndentFirstHexChunkX - iScrollH;
		const auto iTextPosToPrintX = m_iIndentTextX - iScrollH;
		const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeFontMain.cy * iterLines; //Hex and Text are the same.

		//Main loop for printing Hex chunks and Text chars.
		for (auto iterChunks { 0U }; iterChunks < m_dwCapacity && sIndexToPrint < wsvText.size(); ++iterChunks, ++sIndexToPrint) {
			//Hex chunk to print.
			wstrHexToPrint += wsvHex[sIndexToPrint * 2];
			wstrHexToPrint += wsvHex[sIndexToPrint * 2 + 1];

			//Additional space between grouped Hex chunks.
			if (((iterChunks + 1) % static_cast<DWORD>(m_enGroupMode)) == 0 && iterChunks < (m_dwCapacity - 1)) {
				wstrHexToPrint += L' ';
			}

			//Additional space between capacity halves, only with SIZE_BYTE representation.
			if (m_enGroupMode == EHexDataSize::SIZE_BYTE && iterChunks == (m_dwCapacityBlockSize - 1)) {
				wstrHexToPrint += L"  ";
			}

			//Text to print.
			wstrTextToPrint += wsvText[sIndexToPrint];
		}

		//Hex Poly.
		vecWstrHex.emplace_back(std::move(wstrHexToPrint));
		vecPolyHex.emplace_back(iHexPosToPrintX, iPosToPrintY,
			static_cast<UINT>(vecWstrHex.back().size()), vecWstrHex.back().data(), 0, RECT { }, nullptr);

		//Text Poly.
		vecWstrText.emplace_back(std::move(wstrTextToPrint));
		vecPolyText.emplace_back(iTextPosToPrintX, iPosToPrintY,
			static_cast<UINT>(vecWstrText.back().size()), vecWstrText.back().data(), 0, RECT { }, nullptr);
	}

	//Hex printing.
	pDC->SelectObject(m_fontMain);
	pDC->SetTextColor(m_stColor.clrFontHex);
	pDC->SetBkColor(m_stColor.clrBk);
	PolyTextOutW(pDC->m_hDC, vecPolyHex.data(), static_cast<UINT>(vecPolyHex.size()));

	//Text printing.
	pDC->SetTextColor(m_stColor.clrFontText);
	for (const auto& iter : vecPolyText) {
		ExtTextOutW(pDC->m_hDC, iter.x, iter.y, iter.uiFlags, &iter.rcl, iter.lpstr, iter.n, iter.pdx);
	}
}

void CHexCtrl::DrawTemplates(CDC* pDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const
{
	if (!m_pDlgTemplMgr->HasApplied())
		return;

	struct SFIELDS { //Struct for fields.
		POLYTEXTW stPoly { };
		COLORREF  clrBk { };
		COLORREF  clrText { };
		//Flag to avoid print vert line at the beginnig of the line if this line is just a continuation
		//of the previous line above.
		bool      fPrintVertLine { true };
	};
	std::vector<SFIELDS> vecFieldsHex;
	std::vector<SFIELDS> vecFieldsText;
	std::vector<std::unique_ptr<std::wstring>> vecWstrFieldsHex; //unique_ptr to avoid wstring ptr invalidation.
	std::vector<std::unique_ptr<std::wstring>> vecWstrFieldsText;
	const auto ullStartOffset = ullStartLine * m_dwCapacity;
	std::size_t sIndexToPrint { };
	const HEXTEMPLATEFIELD* pFieldCurr { };

	for (auto iterLines = 0; iterLines < iLines; ++iterLines) {
		std::wstring wstrHexFieldToPrint;
		std::wstring wstrTextFieldToPrint;
		int iFieldHexPosToPrintX { -1 };
		int iFieldTextPosToPrintX { };
		bool fField { false };  //Flag to show current Field in current Hex presence.
		bool fPrintVertLine { true };
		const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeFontMain.cy * iterLines; //Hex and Text are the same.
		const auto lmbPoly = [&]() {
			//Hex Fields Poly.
			vecWstrFieldsHex.emplace_back(std::make_unique<std::wstring>(std::move(wstrHexFieldToPrint)));
			vecFieldsHex.emplace_back(POLYTEXTW { iFieldHexPosToPrintX, iPosToPrintY,
				static_cast<UINT>(vecWstrFieldsHex.back()->size()), vecWstrFieldsHex.back()->data(), 0, { }, nullptr },
				pFieldCurr->clrBk, pFieldCurr->clrText, fPrintVertLine);

			//Text Fields Poly.
			vecWstrFieldsText.emplace_back(std::make_unique<std::wstring>(std::move(wstrTextFieldToPrint)));
			vecFieldsText.emplace_back(POLYTEXTW { iFieldTextPosToPrintX, iPosToPrintY,
				static_cast<UINT>(vecWstrFieldsText.back()->size()), vecWstrFieldsText.back()->data(), 0, { }, nullptr },
				pFieldCurr->clrBk, pFieldCurr->clrText, fPrintVertLine);

			fPrintVertLine = true;
		};
		const auto lmbHexSpaces = [&](const unsigned iterChunks) {
			if (!wstrHexFieldToPrint.empty()) { //Only adding spaces if there are chars beforehead.
				if ((iterChunks % static_cast<DWORD>(m_enGroupMode)) == 0) {
					wstrHexFieldToPrint += L' ';
				}

				//Additional space between capacity halves, only with SIZE_BYTE representation.
				if (m_enGroupMode == EHexDataSize::SIZE_BYTE && iterChunks == m_dwCapacityBlockSize) {
					wstrHexFieldToPrint += L"  ";
				}
			}
		};

		//Main loop for printing Hex chunks and Text chars.
		for (auto iterChunks { 0U }; iterChunks < m_dwCapacity && sIndexToPrint < wsvText.size(); ++iterChunks, ++sIndexToPrint) {
			//Fields.
			if (auto pField = m_pDlgTemplMgr->HitTest(ullStartOffset + sIndexToPrint); pField != nullptr) {
				if (iterChunks == 0 && pField == pFieldCurr) {
					fPrintVertLine = false;
				}

				//If it's nested Field.
				if (pFieldCurr != nullptr && pFieldCurr != pField) {
					lmbHexSpaces(iterChunks);
					lmbPoly();
					iFieldHexPosToPrintX = -1;
				}

				pFieldCurr = pField;

				if (iFieldHexPosToPrintX == -1) { //For just one time exec.
					int iCy;
					HexChunkPoint(sIndexToPrint, iFieldHexPosToPrintX, iCy);
					TextChunkPoint(sIndexToPrint, iFieldTextPosToPrintX, iCy);
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
				iFieldHexPosToPrintX = -1;
				fField = false;
				pFieldCurr = nullptr;
			}
		}

		//Fields Poly.
		if (!wstrHexFieldToPrint.empty()) {
			lmbPoly();
		}
		pFieldCurr = nullptr;
	}

	//Fieds printing.
	if (!vecFieldsHex.empty()) {
		pDC->SelectObject(m_fontMain);
		std::size_t index { 0 }; //Index for vecFieldsText, its size is always equal to vecFieldsHex.
		const auto penOld = SelectObject(pDC->m_hDC, m_penDataTempl);
		for (const auto& iter : vecFieldsHex) { //Loop is needed because different Fields can have different colors.
			pDC->SetTextColor(iter.clrText);
			pDC->SetBkColor(iter.clrBk);

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
				const auto iFieldLastLineHexX = refH.x + refH.n * m_sizeFontMain.cx + 1; //Little indent after vert line.
				MoveToEx(pDC->m_hDC, iFieldLastLineHexX, refH.y, nullptr);
				LineTo(pDC->m_hDC, iFieldLastLineHexX, refH.y + m_sizeFontMain.cy);

				const auto iFieldLastLineTextX = refT.x + refT.n * m_sizeFontMain.cx;
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

	struct SBOOKMARKS { //Struct for Bookmarks.
		POLYTEXTW stPoly { };
		COLORREF  clrBk { };
		COLORREF  clrText { };
	};

	std::vector<SBOOKMARKS> vecBkmHex;
	std::vector<SBOOKMARKS> vecBkmText;
	std::vector<std::unique_ptr<std::wstring>> vecWstrBkmHex; //unique_ptr to avoid wstring ptr invalidation.
	std::vector<std::unique_ptr<std::wstring>> vecWstrBkmText;
	const auto ullStartOffset = ullStartLine * m_dwCapacity;
	std::size_t sIndexToPrint { };

	for (auto iterLines = 0; iterLines < iLines; ++iterLines) {
		std::wstring wstrHexBkmToPrint;
		std::wstring wstrTextBkmToPrint;
		int iBkmHexPosToPrintX { -1 };
		int iBkmTextPosToPrintX { };
		bool fBookmark { false };  //Flag to show current Bookmark in current Hex presence.
		PHEXBKM pBkmCurr { };
		const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeFontMain.cy * iterLines; //Hex and Text are the same.
		const auto lmbPoly = [&]() {
			//Hex bookmarks Poly.
			vecWstrBkmHex.emplace_back(std::make_unique<std::wstring>(std::move(wstrHexBkmToPrint)));
			vecBkmHex.emplace_back(POLYTEXTW { iBkmHexPosToPrintX, iPosToPrintY,
				static_cast<UINT>(vecWstrBkmHex.back()->size()), vecWstrBkmHex.back()->data(), 0, { }, nullptr },
				pBkmCurr->clrBk, pBkmCurr->clrText);

			//Text bookmarks Poly.
			vecWstrBkmText.emplace_back(std::make_unique<std::wstring>(std::move(wstrTextBkmToPrint)));
			vecBkmText.emplace_back(POLYTEXTW { iBkmTextPosToPrintX, iPosToPrintY,
				static_cast<UINT>(vecWstrBkmText.back()->size()), vecWstrBkmText.back()->data(), 0, { }, nullptr },
				pBkmCurr->clrBk, pBkmCurr->clrText);
		};
		const auto lmbHexSpaces = [&](const unsigned iterChunks) {
			if (!wstrHexBkmToPrint.empty()) { //Only adding spaces if there are chars beforehead.
				if ((iterChunks % static_cast<DWORD>(m_enGroupMode)) == 0) {
					wstrHexBkmToPrint += L' ';
				}

				//Additional space between capacity halves, only with SIZE_BYTE representation.
				if (m_enGroupMode == EHexDataSize::SIZE_BYTE && iterChunks == m_dwCapacityBlockSize) {
					wstrHexBkmToPrint += L"  ";
				}
			}
		};

		//Main loop for printing Hex chunks and Text chars.
		for (auto iterChunks { 0U }; iterChunks < m_dwCapacity && sIndexToPrint < wsvText.size(); ++iterChunks, ++sIndexToPrint) {
			//Bookmarks.
			if (auto* pBkm = m_pDlgBkmMgr->HitTest(ullStartOffset + sIndexToPrint); pBkm != nullptr) {
				//If it's nested bookmark.
				if (pBkmCurr != nullptr && pBkmCurr != pBkm) {
					lmbHexSpaces(iterChunks);
					lmbPoly();
					iBkmHexPosToPrintX = -1;
				}

				pBkmCurr = pBkm;

				if (iBkmHexPosToPrintX == -1) { //For just one time exec.
					int iCy;
					HexChunkPoint(sIndexToPrint, iBkmHexPosToPrintX, iCy);
					TextChunkPoint(sIndexToPrint, iBkmTextPosToPrintX, iCy);
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
				iBkmHexPosToPrintX = -1;
				fBookmark = false;
				pBkmCurr = nullptr;
			}
		}

		//Bookmarks Poly.
		if (!wstrHexBkmToPrint.empty()) {
			lmbPoly();
		}
		pBkmCurr = nullptr;
	}

	//Bookmarks printing.
	if (!vecBkmHex.empty()) {
		pDC->SelectObject(m_fontMain);
		std::size_t index { 0 }; //Index for vecBkmText, its size is always equal to vecBkmHex.
		for (const auto& iter : vecBkmHex) { //Loop is needed because bkms have different colors.
			pDC->SetTextColor(iter.clrText);
			pDC->SetBkColor(iter.clrBk);
			const auto& refH = iter.stPoly;
			ExtTextOutW(pDC->m_hDC, refH.x, refH.y, refH.uiFlags, &refH.rcl, refH.lpstr, refH.n, refH.pdx); //Hex bookmarks printing.
			const auto& refT = vecBkmText[index++].stPoly;
			ExtTextOutW(pDC->m_hDC, refT.x, refT.y, refT.uiFlags, &refT.rcl, refT.lpstr, refT.n, refT.pdx); //Text bookmarks printing.
		}
	}
}

void CHexCtrl::DrawCustomColors(CDC* pDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const
{
	if (m_pHexVirtColors == nullptr)
		return;

	struct SCOLORS { //Struct for colors.
		POLYTEXTW stPoly { };
		HEXCOLOR clr { };
	};

	std::vector<SCOLORS> vecColorsHex;
	std::vector<SCOLORS> vecColorsText;
	std::vector<std::unique_ptr<std::wstring>> vecWstrColorsHex; //unique_ptr to avoid wstring ptr invalidation.
	std::vector<std::unique_ptr<std::wstring>> vecWstrColorsText;
	const auto ullStartOffset = ullStartLine * m_dwCapacity;
	std::size_t sIndexToPrint { };

	for (auto iterLines = 0; iterLines < iLines; ++iterLines) {
		std::wstring wstrHexColorToPrint;
		std::wstring wstrTextColorToPrint;
		int iColorHexPosToPrintX { -1 };
		int iColorTextPosToPrintX { };
		bool fColor { false };  //Flag to show current Color in current Hex presence.
		std::optional<HEXCOLOR> optColorCurr { }; //Current color.
		const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeFontMain.cy * iterLines; //Hex and Text are the same.
		HEXCOLORINFO hci { .hdr { m_hWnd, static_cast<UINT>(GetDlgCtrlID()) } };
		const auto lmbPoly = [&]() {
			//Hex colors Poly.
			vecWstrColorsHex.emplace_back(std::make_unique<std::wstring>(std::move(wstrHexColorToPrint)));
			vecColorsHex.emplace_back(POLYTEXTW { iColorHexPosToPrintX, iPosToPrintY,
				static_cast<UINT>(vecWstrColorsHex.back()->size()), vecWstrColorsHex.back()->data(), 0, { }, nullptr },
				*optColorCurr);

			//Text colors Poly.
			vecWstrColorsText.emplace_back(std::make_unique<std::wstring>(std::move(wstrTextColorToPrint)));
			vecColorsText.emplace_back(POLYTEXTW { iColorTextPosToPrintX, iPosToPrintY,
				static_cast<UINT>(vecWstrColorsText.back()->size()), vecWstrColorsText.back()->data(), 0, { }, nullptr },
				*optColorCurr);
		};
		const auto lmbHexSpaces = [&](const unsigned iterChunks) {
			if (!wstrHexColorToPrint.empty()) { //Only adding spaces if there are chars beforehead.
				if ((iterChunks % static_cast<DWORD>(m_enGroupMode)) == 0) {
					wstrHexColorToPrint += L' ';
				}

				//Additional space between capacity halves, only with SIZE_BYTE representation.
				if (m_enGroupMode == EHexDataSize::SIZE_BYTE && iterChunks == m_dwCapacityBlockSize) {
					wstrHexColorToPrint += L"  ";
				}
			}
		};

		//Main loop for printing Hex chunks and Text chars.
		for (auto iterChunks { 0U }; iterChunks < m_dwCapacity && sIndexToPrint < wsvText.size(); ++iterChunks, ++sIndexToPrint) {
			hci.ullOffset = ullStartOffset + sIndexToPrint;
			if (m_pHexVirtColors->OnHexGetColor(hci)) {
				//If it's different color.
				if (optColorCurr && (optColorCurr->clrBk != hci.stClr.clrBk || optColorCurr->clrText != hci.stClr.clrText)) {
					lmbHexSpaces(iterChunks);
					lmbPoly();
					iColorHexPosToPrintX = -1;
				}
				optColorCurr = hci.stClr;

				if (iColorHexPosToPrintX == -1) { //For just one time exec.
					int iCy;
					HexChunkPoint(sIndexToPrint, iColorHexPosToPrintX, iCy);
					TextChunkPoint(sIndexToPrint, iColorTextPosToPrintX, iCy);
				}

				lmbHexSpaces(iterChunks);
				wstrHexColorToPrint += wsvHex[sIndexToPrint * 2];
				wstrHexColorToPrint += wsvHex[sIndexToPrint * 2 + 1];
				wstrTextColorToPrint += wsvText[sIndexToPrint];
				fColor = true;
			}
			else if (fColor) {
				//There can be multiple colors in one line. 
				//So, if there already were colored bytes in the current line, we Poly them.
				//Same Poly mechanism presents at the end of the current (iterLines) loop,
				//to Poly colors that end at the line's end.

				lmbPoly();
				iColorHexPosToPrintX = -1;
				fColor = false;
				optColorCurr.reset();
			}
		}

		//Colors Poly.
		if (!wstrHexColorToPrint.empty()) {
			lmbPoly();
		}
	}

	//Colors printing.
	if (!vecColorsHex.empty()) {
		pDC->SelectObject(m_fontMain);
		std::size_t index { 0 }; //Index for vecColorsText, its size is always equal to vecColorsHex.
		for (const auto& iter : vecColorsHex) { //Loop is needed because of different colors.
			pDC->SetTextColor(iter.clr.clrText);
			pDC->SetBkColor(iter.clr.clrBk);
			const auto& refH = iter.stPoly;
			ExtTextOutW(pDC->m_hDC, refH.x, refH.y, refH.uiFlags, &refH.rcl, refH.lpstr, refH.n, refH.pdx); //Hex colors printing.
			const auto& refT = vecColorsText[index++].stPoly;
			ExtTextOutW(pDC->m_hDC, refT.x, refT.y, refT.uiFlags, &refT.rcl, refT.lpstr, refT.n, refT.pdx); //Text colors printing.
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
	const auto ullStartOffset = ullStartLine * m_dwCapacity;
	std::size_t sIndexToPrint { };

	for (auto iterLines = 0; iterLines < iLines; ++iterLines) {
		std::wstring wstrHexSelToPrint; //Selected Hex and Text strings to print.
		std::wstring wstrTextSelToPrint;
		int iSelHexPosToPrintX { -1 };
		int iSelTextPosToPrintX { };
		bool fSelection { false };
		const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeFontMain.cy * iterLines; //Hex and Text are the same.
		const auto lmbPoly = [&]() {
			//Hex selection Poly.
			vecWstrSel.emplace_back(std::make_unique<std::wstring>(std::move(wstrHexSelToPrint)));
			vecPolySelHex.emplace_back(iSelHexPosToPrintX, iPosToPrintY,
				static_cast<UINT>(vecWstrSel.back()->size()), vecWstrSel.back()->data(), 0, RECT { }, nullptr);

			//Text selection Poly.
			vecWstrSel.emplace_back(std::make_unique<std::wstring>(std::move(wstrTextSelToPrint)));
			vecPolySelText.emplace_back(iSelTextPosToPrintX, iPosToPrintY,
				static_cast<UINT>(vecWstrSel.back()->size()), vecWstrSel.back()->data(), 0, RECT { }, nullptr);
		};

		//Main loop for printing Hex chunks and Text chars.
		for (auto iterChunks { 0U }; iterChunks < m_dwCapacity && sIndexToPrint < wsvText.size(); ++iterChunks, ++sIndexToPrint) {
			//Selection.
			if (m_pSelection->HitTest(ullStartOffset + sIndexToPrint)) {
				if (iSelHexPosToPrintX == -1) { //For just one time exec.
					int iCy;
					HexChunkPoint(sIndexToPrint, iSelHexPosToPrintX, iCy);
					TextChunkPoint(sIndexToPrint, iSelTextPosToPrintX, iCy);
				}

				if (!wstrHexSelToPrint.empty()) { //Only adding spaces if there are chars beforehead.
					if ((iterChunks % static_cast<DWORD>(m_enGroupMode)) == 0) {
						wstrHexSelToPrint += L' ';
					}

					//Additional space between capacity halves, only with SIZE_BYTE representation.
					if (m_enGroupMode == EHexDataSize::SIZE_BYTE && iterChunks == m_dwCapacityBlockSize) {
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

				//Selection Poly.
				if (!wstrHexSelToPrint.empty()) {
					lmbPoly();
				}
				iSelHexPosToPrintX = -1;
				fSelection = false;
			}
		}

		//Selection Poly.
		if (!wstrHexSelToPrint.empty()) {
			lmbPoly();
		}
	}

	//Selection printing.
	if (!vecPolySelHex.empty()) {
		pDC->SelectObject(m_fontMain);
		pDC->SetTextColor(m_stColor.clrFontSel);
		pDC->SetBkColor(m_stColor.clrBkSel);
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
	const auto ullStartOffset = ullStartLine * m_dwCapacity;
	std::size_t sIndexToPrint { };

	for (auto iterLines = 0; iterLines < iLines; ++iterLines) {
		std::wstring wstrHexSelToPrint; //Selected Hex and Text strings to print.
		std::wstring wstrTextSelToPrint;
		int iSelHexPosToPrintX { -1 };
		int iSelTextPosToPrintX { };
		bool fSelection { false };
		const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeFontMain.cy * iterLines; //Hex and Text are the same.
		const auto lmbPoly = [&]() {
			//Hex selection highlight Poly.
			vecWstrSelHgl.emplace_back(std::make_unique<std::wstring>(std::move(wstrHexSelToPrint)));
			vecPolySelHexHgl.emplace_back(iSelHexPosToPrintX, iPosToPrintY,
				static_cast<UINT>(vecWstrSelHgl.back()->size()), vecWstrSelHgl.back()->data(), 0, RECT { }, nullptr);

			//Text selection highlight Poly.
			vecWstrSelHgl.emplace_back(std::make_unique<std::wstring>(std::move(wstrTextSelToPrint)));
			vecPolySelTextHgl.emplace_back(iSelTextPosToPrintX, iPosToPrintY,
				static_cast<UINT>(vecWstrSelHgl.back()->size()), vecWstrSelHgl.back()->data(), 0, RECT { }, nullptr);
		};

		//Main loop for printing Hex chunks and Text chars.
		for (auto iterChunks { 0U }; iterChunks < m_dwCapacity && sIndexToPrint < wsvText.size(); ++iterChunks, ++sIndexToPrint) {
			//Selection highlights.
			if (m_pSelection->HitTestHighlight(ullStartOffset + sIndexToPrint)) {
				if (iSelHexPosToPrintX == -1) { //For just one time exec.
					int iCy;
					HexChunkPoint(sIndexToPrint, iSelHexPosToPrintX, iCy);
					TextChunkPoint(sIndexToPrint, iSelTextPosToPrintX, iCy);
				}

				if (!wstrHexSelToPrint.empty()) { //Only adding spaces if there are chars beforehead.
					if ((iterChunks % static_cast<DWORD>(m_enGroupMode)) == 0) {
						wstrHexSelToPrint += L' ';
					}

					//Additional space between capacity halves, only with SIZE_BYTE representation.
					if (m_enGroupMode == EHexDataSize::SIZE_BYTE && iterChunks == m_dwCapacityBlockSize) {
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

				//Selection highlight Poly.
				if (!wstrHexSelToPrint.empty()) {
					lmbPoly();
				}
				iSelHexPosToPrintX = -1;
				fSelection = false;
			}
		}

		//Selection highlight Poly.
		if (!wstrHexSelToPrint.empty()) {
			lmbPoly();
		}
	}

	//Selection highlight printing.
	if (!vecPolySelHexHgl.empty()) {
		//Colors are the inverted selection colors.
		pDC->SelectObject(m_fontMain);
		pDC->SetTextColor(m_stColor.clrBkSel);
		pDC->SetBkColor(m_stColor.clrFontSel);
		PolyTextOutW(pDC->m_hDC, vecPolySelHexHgl.data(), static_cast<UINT>(vecPolySelHexHgl.size())); //Hex selection highlight printing.
		for (const auto& iter : vecPolySelTextHgl) {
			ExtTextOutW(pDC->m_hDC, iter.x, iter.y, iter.uiFlags, &iter.rcl, iter.lpstr, iter.n, iter.pdx); //Text selection highlight printing.
		}
	}
}

void CHexCtrl::DrawCaret(CDC* pDC, ULONGLONG ullStartLine, std::wstring_view wsvHex, std::wstring_view wsvText)const
{
	const auto ullCaretPos = GetCaretPos();
	if (!IsOffsetVisible(ullCaretPos))
		return;

	int iCaretHexPosToPrintX;
	int iCaretTextPosToPrintX;
	int iCaretHexPosToPrintY;
	int iCaretTextPosToPrintY;
	HexChunkPoint(ullCaretPos, iCaretHexPosToPrintX, iCaretHexPosToPrintY);
	TextChunkPoint(ullCaretPos, iCaretTextPosToPrintX, iCaretTextPosToPrintY);

	const auto sIndexToPrint = static_cast<std::size_t>(ullCaretPos - (ullStartLine * m_dwCapacity));
	std::wstring wstrHexCaretToPrint;
	std::wstring wstrTextCaretToPrint;
	if (m_fCaretHigh) {
		wstrHexCaretToPrint = wsvHex[sIndexToPrint * 2];
	}
	else {
		wstrHexCaretToPrint = wsvHex[sIndexToPrint * 2 + 1];
		iCaretHexPosToPrintX += m_sizeFontMain.cx;
	}
	wstrTextCaretToPrint = wsvText[sIndexToPrint];

	POLYTEXTW arrPolyCaret[2]; //Caret Poly array.

	//Hex Caret Poly.
	arrPolyCaret[0] = { iCaretHexPosToPrintX, iCaretHexPosToPrintY,
		static_cast<UINT>(wstrHexCaretToPrint.size()), wstrHexCaretToPrint.data(), 0, RECT { }, nullptr };

	//Text Caret Poly.
	arrPolyCaret[1] = { iCaretTextPosToPrintX, iCaretTextPosToPrintY,
		static_cast<UINT>(wstrTextCaretToPrint.size()), wstrTextCaretToPrint.data(), 0, RECT { }, nullptr };

	//Caret color.
	const auto clrBkCaret = m_pSelection->HitTest(ullCaretPos) ? m_stColor.clrBkCaretSel : m_stColor.clrBkCaret;

	//Caret printing.
	pDC->SelectObject(m_fontMain);
	pDC->SetTextColor(m_stColor.clrFontCaret);
	pDC->SetBkColor(clrBkCaret);
	for (const auto iter : arrPolyCaret) {
		ExtTextOutW(pDC->m_hDC, iter.x, iter.y, iter.uiFlags, &iter.rcl, iter.lpstr, iter.n, iter.pdx); //Hex/Text caret printing.
	}
}

void CHexCtrl::DrawDataInterp(CDC* pDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const
{
	const auto ullDISize = m_pDlgDataInterp->GetDataSize();
	if (ullDISize == 0)
		return;

	const auto ullCaretPos = GetCaretPos();
	if ((ullCaretPos + ullDISize) > GetDataSize())
		return;

	std::vector<POLYTEXTW> vecPolyDataInterp;
	std::vector<std::unique_ptr<std::wstring>> vecWstrDataInterp;

	const auto ullStartOffset = ullStartLine * m_dwCapacity;
	std::size_t sIndexToPrint { };

	for (auto iterLines = 0; iterLines < iLines; ++iterLines) {
		std::wstring wstrHexDataInterpToPrint; //Data Interpreter Hex and Text strings to print.
		std::wstring wstrTextDataInterpToPrint;
		int iDataInterpHexPosToPrintX { -1 }; //Data Interpreter X coords.
		int iDataInterpTextPosToPrintX { };
		const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeFontMain.cy * iterLines; //Hex and Text are the same.

		//Main loop for printing Hex chunks and Text chars.
		for (auto iterChunks { 0U }; iterChunks < m_dwCapacity && sIndexToPrint < wsvText.size(); ++iterChunks, ++sIndexToPrint) {
			const auto ullOffsetCurr = ullStartOffset + sIndexToPrint;
			if (ullOffsetCurr >= ullCaretPos && ullOffsetCurr < (ullCaretPos + ullDISize)) {
				if (iDataInterpHexPosToPrintX == -1) { //For just one time exec.
					int iCy;
					HexChunkPoint(sIndexToPrint, iDataInterpHexPosToPrintX, iCy);
					TextChunkPoint(sIndexToPrint, iDataInterpTextPosToPrintX, iCy);
				}

				if (!wstrHexDataInterpToPrint.empty()) { //Only adding spaces if there are chars beforehead.
					if ((iterChunks % static_cast<DWORD>(m_enGroupMode)) == 0) {
						wstrHexDataInterpToPrint += L' ';
					}

					//Additional space between capacity halves, only with SIZE_BYTE representation.
					if (m_enGroupMode == EHexDataSize::SIZE_BYTE && iterChunks == m_dwCapacityBlockSize) {
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
				static_cast<UINT>(vecWstrDataInterp.back()->size()), vecWstrDataInterp.back()->data(), 0, RECT { }, nullptr);

			//Text Data Interpreter Poly.
			vecWstrDataInterp.emplace_back(std::make_unique<std::wstring>(std::move(wstrTextDataInterpToPrint)));
			vecPolyDataInterp.emplace_back(iDataInterpTextPosToPrintX, iPosToPrintY,
				static_cast<UINT>(vecWstrDataInterp.back()->size()), vecWstrDataInterp.back()->data(), 0, RECT { }, nullptr);
		}
	}

	//Data Interpreter printing.
	if (!vecPolyDataInterp.empty()) {
		pDC->SelectObject(m_fontMain);
		pDC->SetTextColor(m_stColor.clrFontDataInterp);
		pDC->SetBkColor(m_stColor.clrBkDataInterp);
		for (const auto& iter : vecPolyDataInterp) {
			ExtTextOutW(pDC->m_hDC, iter.x, iter.y, iter.uiFlags, &iter.rcl, iter.lpstr, iter.n, iter.pdx); //Hex/Text Data Interpreter printing.
		}
	}
}

void CHexCtrl::DrawPageLines(CDC* pDC, ULONGLONG ullStartLine, int iLines)
{
	if (!IsPageVisible())
		return;

	struct SPAGELINES { //Struct for sector lines.
		POINT ptStart;
		POINT ptEnd;
	};
	std::vector<SPAGELINES> vecPageLines;

	//Loop for printing Hex chunks and Text chars line by line.
	for (auto iterLines = 0; iterLines < iLines; ++iterLines) {
		//Page's lines vector to print.
		if ((((ullStartLine + iterLines) * m_dwCapacity) % m_dwPageSize == 0) && iterLines > 0) {
			const auto iPosToPrintY = m_iStartWorkAreaY + m_sizeFontMain.cy * iterLines;
			vecPageLines.emplace_back(POINT { m_iFirstVertLine, iPosToPrintY }, POINT { m_iFourthVertLine, iPosToPrintY });
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
	m_wstrCapacity.clear();
	m_wstrCapacity.reserve(static_cast<std::size_t>(m_dwCapacity) * 3);
	for (auto iter { 0U }; iter < m_dwCapacity; ++iter) {
		m_wstrCapacity += std::vformat(IsOffsetAsHex() ? L"{: >2X}" : L"{: >2d}", std::make_wformat_args(iter));

		//Additional space between hex chunk blocks.
		if ((((iter + 1) % static_cast<DWORD>(m_enGroupMode)) == 0) && (iter < (m_dwCapacity - 1))) {
			m_wstrCapacity += L" ";
		}

		//Additional space between hex halves.
		if (m_enGroupMode == EHexDataSize::SIZE_BYTE && iter == (m_dwCapacityBlockSize - 1)) {
			m_wstrCapacity += L"  ";
		}
	}
}

void CHexCtrl::FillWithZeros()
{
	if (!IsDataSet())
		return;

	std::byte byteZero { 0 };
	ModifyData({ .enModifyMode { EHexModifyMode::MODIFY_REPEAT }, .spnData { &byteZero, sizeof(byteZero) },
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
	if (const auto iLines = m_iHeightWorkArea / m_sizeFontMain.cy; iLines > 0) { //How many visible lines.
		ullEndLine += iLines - 1;
	}

	const auto ullDataSize = GetDataSize();
	const auto ullTotalLines = ullDataSize / m_dwCapacity;

	//If ullDataSize is really small, or we at the scroll end, adjust ullEndLine to be not bigger than maximum possible.
	if (ullEndLine >= ullTotalLines) {
		ullEndLine = ullTotalLines - ((ullDataSize % m_dwCapacity) == 0 ? 1 : 0);
	}

	return ullEndLine;
}

auto CHexCtrl::GetCommand(UINT uKey, bool fCtrl, bool fShift, bool fAlt)const->std::optional<EHexCmd>
{
	std::optional<EHexCmd> optRet { std::nullopt };
	if (const auto iter = std::find_if(m_vecKeyBind.begin(), m_vecKeyBind.end(), [=](const SKEYBIND& ref) {
		return ref.fCtrl == fCtrl && ref.fShift == fShift && ref.fAlt == fAlt && ref.uKey == uKey; });
		iter != m_vecKeyBind.end()) {
		optRet = iter->eCmd;
	}

	return optRet;
}

long CHexCtrl::GetFontSize()
{
	return GetFont().lfHeight;
}

CRect CHexCtrl::GetRectTextCaption()const
{
	const auto iScrollH { static_cast<int>(m_pScrollH->GetScrollPos()) };
	return { m_iThirdVertLine - iScrollH, m_iFirstHorzLine, m_iFourthVertLine - iScrollH, m_iSecondHorzLine };
}

auto CHexCtrl::GetTopLine()const->ULONGLONG
{
	return m_pScrollV->GetScrollPos() / m_sizeFontMain.cy;
}

void CHexCtrl::HexChunkPoint(ULONGLONG ullOffset, int& iCx, int& iCy)const
{	//This func computes x and y pos of given Hex chunk.
	const DWORD dwMod = ullOffset % m_dwCapacity;
	int iBetweenBlocks { 0 };
	if (dwMod >= m_dwCapacityBlockSize) {
		iBetweenBlocks = m_iSpaceBetweenBlocks;
	}

	iCx = static_cast<int>(((m_iIndentFirstHexChunkX + iBetweenBlocks + dwMod * m_iSizeHexByte) +
		(dwMod / static_cast<DWORD>(m_enGroupMode)) * m_iSpaceBetweenHexChunks) - m_pScrollH->GetScrollPos());

	auto ullScrollV = m_pScrollV->GetScrollPos();
	iCy = static_cast<int>((m_iStartWorkAreaY + (ullOffset / m_dwCapacity) * m_sizeFontMain.cy) -
		(ullScrollV - (ullScrollV % m_sizeFontMain.cy)));
}

auto CHexCtrl::HitTest(POINT pt)const->std::optional<HEXHITTEST>
{
	HEXHITTEST stHit;
	const auto iY = pt.y;
	const auto iX = pt.x + static_cast<int>(m_pScrollH->GetScrollPos()); //To compensate horizontal scroll.
	const auto ullCurLine = GetTopLine();

	bool fHit { false };
	//Checking if iX is within Hex chunks area.
	if ((iX >= m_iIndentFirstHexChunkX) && (iX < m_iThirdVertLine) && (iY >= m_iStartWorkAreaY) && (iY <= m_iEndWorkArea)) {
		int iTotalSpaceBetweenChunks { 0 };
		for (auto iterCapacity = 0U; iterCapacity < m_dwCapacity; ++iterCapacity) {
			if (iterCapacity > 0 && (iterCapacity % static_cast<DWORD>(m_enGroupMode)) == 0) {
				iTotalSpaceBetweenChunks += m_iSpaceBetweenHexChunks;
				if (m_enGroupMode == EHexDataSize::SIZE_BYTE && iterCapacity == m_dwCapacityBlockSize) {
					iTotalSpaceBetweenChunks += m_iSpaceBetweenBlocks;
				}
			}

			const auto iCurrChunkBegin = m_iIndentFirstHexChunkX + (m_iSizeHexByte * iterCapacity) + iTotalSpaceBetweenChunks;
			const auto iCurrChunkEnd = iCurrChunkBegin + m_iSizeHexByte +
				(((iterCapacity + 1) % static_cast<DWORD>(m_enGroupMode)) == 0 ? m_iSpaceBetweenHexChunks : 0)
				+ ((m_enGroupMode == EHexDataSize::SIZE_BYTE && (iterCapacity + 1) == m_dwCapacityBlockSize) ? m_iSpaceBetweenBlocks : 0);

			if (static_cast<unsigned int>(iX) < iCurrChunkEnd) { //If iX lays in-between [iCurrChunkBegin...iCurrChunkEnd).
				stHit.ullOffset = static_cast<ULONGLONG>(iterCapacity) + ((iY - m_iStartWorkAreaY) / m_sizeFontMain.cy) *
					m_dwCapacity + (ullCurLine * m_dwCapacity);

				if ((iX - iCurrChunkBegin) < static_cast<DWORD>(m_sizeFontMain.cx)) { //Check byte's High or Low half was hit.
					stHit.fIsHigh = true;
				}

				fHit = true;
				break;
			}
		}
	}
	//Or within Text area.
	else if ((iX >= m_iIndentTextX) && (iX < (m_iIndentTextX + m_iDistanceBetweenChars * static_cast<int>(m_dwCapacity)))
		&& (iY >= m_iStartWorkAreaY) && iY <= m_iEndWorkArea) {
		//Calculate ullOffset Text symbol.
		stHit.ullOffset = ((iX - static_cast<ULONGLONG>(m_iIndentTextX)) / m_iDistanceBetweenChars) +
			((iY - m_iStartWorkAreaY) / m_sizeFontMain.cy) * m_dwCapacity + (ullCurLine * m_dwCapacity);
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

bool CHexCtrl::IsInfoBar()const
{
	return m_fInfoBar;
}

bool CHexCtrl::IsPageVisible()const
{
	return m_dwPageSize > 0 && (m_dwPageSize % m_dwCapacity == 0) && m_dwPageSize >= m_dwCapacity;
}

template<typename T>
void CHexCtrl::ModifyWorker(const HEXMODIFY& hms, const T& lmbWorker, const SpanCByte spnDataToOperWith)
{
	assert(!spnDataToOperWith.empty());
	if (spnDataToOperWith.empty())
		return;

	const auto& vecSpanRef = hms.vecSpan;
	const auto ullTotalSize = std::accumulate(vecSpanRef.begin(), vecSpanRef.end(), 0ULL,
		[](ULONGLONG ullSumm, const HEXSPAN& ref) { return ullSumm + ref.ullSize; });
	assert(ullTotalSize <= GetDataSize());

	CHexDlgCallback dlgClbk(L"Modifying...", vecSpanRef.back().ullOffset,
		vecSpanRef.back().ullOffset + ullTotalSize, this);
	const auto lmbThread = [&]() {
		for (const auto& iterSpan : vecSpanRef) { //Span-vector's size times.
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

			if (!IsVirtual()) {
				ullSizeCache = ullSizeToModify;
				ullChunks = 1;
			}
			else {
				ullSizeCache = GetCacheSize(); //Size of Virtual memory for acquiring, to work with.
				if (ullSizeCache >= ullSizeToOperWith) {
					ullSizeCache -= ullSizeCache % ullSizeToOperWith; //Aligning chunk size to ullSizeToOperWith.

					if (ullSizeToModify < ullSizeCache) {
						ullSizeCache = ullSizeToModify;
					}
					ullChunks = ullSizeToModify / ullSizeCache + ((ullSizeToModify % ullSizeCache) ? 1 : 0);
				}
				else {
					fCacheIsLargeEnough = false;
					const auto iSmallMod = ullSizeToOperWith % ullSizeCache;
					const auto ullSmallChunks = ullSizeToOperWith / ullSizeCache + (iSmallMod > 0 ? 1 : 0);
					ullChunks = (ullSizeToModify / ullSizeToOperWith) * ullSmallChunks;
				}
			}

			if (fCacheIsLargeEnough) {
				for (auto iterChunk { 0ULL }; iterChunk < ullChunks; ++iterChunk) {
					const auto ullOffsetCurr = ullOffsetToModify + (iterChunk * ullSizeCache);
					if (ullOffsetCurr + ullSizeCache > GetDataSize()) { //Overflow check.
						ullSizeCache = GetDataSize() - ullOffsetCurr;
						if (ullSizeCache < ullSizeToOperWith) //ullSizeChunk is too small for ullSizeToOperWith.
							break;
					}

					if ((ullOffsetCurr + ullSizeCache) > (ullOffsetToModify + ullSizeToModify)) {
						ullSizeCache = (ullOffsetToModify + ullSizeToModify) - ullOffsetCurr;
						if (ullSizeCache < ullSizeToOperWith) //ullSizeChunk is too small for ullSizeToOperWith.
							break;
					}

					const auto spnData = GetData({ ullOffsetCurr, ullSizeCache });
					assert(!spnData.empty());
					for (auto ullIndex { 0ULL }; ullIndex <= (ullSizeCache - ullSizeToOperWith); ullIndex += ullSizeToOperWith) {
						lmbWorker(spnData.data() + ullIndex, hms, spnDataToOperWith);
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
				//It's a special case for when the ullSizeToOperWith is larger than
				//the current cache size (only in Virtual mode).

				const auto iSmallMod = ullSizeToOperWith % ullSizeCache;
				const auto ullSmallChunks = ullSizeToOperWith / ullSizeCache + (iSmallMod > 0 ? 1 : 0);
				auto ullSmallChunkCur = 0ULL; //Current small chunk index.
				auto ullOffsetCurr = 0ULL;
				auto ullOffsetSubSpan = 0ULL; //Current offset for spnDataToOperWith.subspan().
				auto ullSizeCacheCurr = 0ULL; //Current cache size.
				for (auto iterChunk { 0ULL }; iterChunk < ullChunks; ++iterChunk) {
					if (ullSmallChunkCur == (ullSmallChunks - 1) && iSmallMod > 0) {
						ullOffsetCurr += iSmallMod;
						ullSizeCacheCurr = iSmallMod;
						ullOffsetSubSpan += iSmallMod;
					}
					else {
						ullOffsetCurr = ullOffsetToModify + (iterChunk * ullSizeCache);
						ullSizeCacheCurr = ullSizeCache;
						ullOffsetSubSpan = ullSmallChunkCur * ullSizeCache;
					}

					const auto spnData = GetData({ ullOffsetCurr, ullSizeCacheCurr });
					assert(!spnData.empty());

					lmbWorker(spnData.data(), hms, spnDataToOperWith.subspan(static_cast<std::size_t>(ullOffsetSubSpan),
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

	constexpr auto iSizeToRunThread { 1024 * 1024 }; //1MB.
	if (ullTotalSize > iSizeToRunThread) { //Spawning new thread only if data size is big enough.
		std::thread thrdWorker(lmbThread);
		dlgClbk.DoModal();
		thrdWorker.join();
	}
	else {
		lmbThread();
	}
}

void CHexCtrl::OffsetToString(ULONGLONG ullOffset, wchar_t* buffOut)const
{
	*std::vformat_to(buffOut, IsOffsetAsHex() ? L"{:0>{}X}" : L"{:0>{}}", std::make_wformat_args(ullOffset, m_dwOffsetDigits)) = L'\0';
}

void CHexCtrl::OnCaretPosChange(ULONGLONG ullOffset)
{
	//To prevent inspecting while key is pressed continuously.
	//Only when one time pressing.
	if (!m_fKeyDownAtm && ::IsWindowVisible(m_pDlgDataInterp->m_hWnd)) {
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
	::SendMessageW(GetParent()->m_hWnd, WM_NOTIFY, GetDlgCtrlID(), reinterpret_cast<LPARAM>(&t));
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
		MessageBoxW(L"Internal printer initialization error!", L"Error", MB_ICONERROR);
		return;
	}

	//User pressed "Cancel", or "Apply" and then "Cancel".
	if (dlg.m_pdex.dwResultAction == PD_RESULT_CANCEL || dlg.m_pdex.dwResultAction == PD_RESULT_APPLY) {
		DeleteDC(dlg.m_pdex.hDC);
		return;
	}

	const auto hdcPrinter = dlg.GetPrinterDC();
	if (hdcPrinter == nullptr) {
		MessageBoxW(L"No printer found!");
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
	const auto ullTotalLines = GetDataSize() / m_dwCapacity;
	RecalcAll(pDC, &rcPrint);
	const auto iLinesInPage = m_iHeightWorkArea / m_sizeFontMain.cy;
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
		ullStartLine = ullSelStart / m_dwCapacity;

		const DWORD dwModStart = ullSelStart % m_dwCapacity;
		auto dwLines = static_cast<DWORD>(ullSelSize) / m_dwCapacity;
		if ((dwModStart + static_cast<DWORD>(ullSelSize)) > m_dwCapacity) {
			++dwLines;
		}
		if ((ullSelSize % m_dwCapacity) + dwModStart > m_dwCapacity) {
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
		const auto clBkOld = m_stColor.clrBkSel;
		const auto clTextOld = m_stColor.clrFontSel;
		m_stColor.clrBkSel = m_stColor.clrBk;
		m_stColor.clrFontSel = m_stColor.clrFontHex;
		DrawOffsets(pDC, ullStartLine, iLines);
		DrawSelection(pDC, ullStartLine, iLines, wstrHex, wstrText);
		m_stColor.clrBkSel = clBkOld;
		m_stColor.clrFontSel = clTextOld;
		pDC->EndPage();
	}
	else {
		for (auto iterPage = 0; iterPage < iPagesToPrint; ++iterPage) {
			pDC->StartPage();
			auto ullEndLine = ullStartLine + iLinesInPage;

			//If m_dwDataCount is really small we adjust ullEndLine to be not bigger than maximum allowed.
			if (ullEndLine > ullTotalLines) {
				ullEndLine = (GetDataSize() % m_dwCapacity) ? ullTotalLines + 1 : ullTotalLines;
			}

			const auto iLines = static_cast<int>(ullEndLine - ullStartLine);
			const auto& [wstrHex, wstrText] = BuildDataToDraw(ullStartLine, iLines);

			DrawWindow(pDC); //Draw the window with all layouts.
			DrawInfoBar(pDC);

			if (IsDataSet()) {
				DrawOffsets(pDC, ullStartLine, iLines);
				DrawHexText(pDC, iLines, wstrHex, wstrText);
				DrawTemplates(pDC, ullStartLine, iLines, wstrHex, wstrText);
				DrawBookmarks(pDC, ullStartLine, iLines, wstrHex, wstrText);
				DrawCustomColors(pDC, ullStartLine, iLines, wstrHex, wstrText);
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
	auto* const pDCCurr = pDC == nullptr ? GetDC() : pDC;
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
		m_iHeightInfoBar = m_sizeFontInfo.cy;
		m_iHeightInfoBar += m_iHeightInfoBar / 3;
	}
	else { //If Info window is disabled, we set its height to zero.
		m_iHeightInfoBar = 0;
	}
	m_iHeightBottomOffArea = m_iHeightInfoBar + m_iIndentBottomLine;

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

	m_iSecondVertLine = m_iFirstVertLine + m_dwOffsetDigits * m_sizeFontMain.cx + m_sizeFontMain.cx * 2;
	m_iSizeHexByte = m_sizeFontMain.cx * 2;
	m_iSpaceBetweenBlocks = (m_enGroupMode == EHexDataSize::SIZE_BYTE && m_dwCapacity > 1) ? m_sizeFontMain.cx * 2 : 0;
	m_iSpaceBetweenHexChunks = m_sizeFontMain.cx;
	m_iDistanceBetweenHexChunks = m_iSizeHexByte * static_cast<DWORD>(m_enGroupMode) + m_iSpaceBetweenHexChunks;
	m_iThirdVertLine = m_iSecondVertLine + m_iDistanceBetweenHexChunks * (m_dwCapacity / static_cast<DWORD>(m_enGroupMode))
		+ m_sizeFontMain.cx + m_iSpaceBetweenBlocks;
	m_iIndentTextX = m_iThirdVertLine + m_sizeFontMain.cx;
	m_iDistanceBetweenChars = m_sizeFontMain.cx;
	m_iFourthVertLine = m_iIndentTextX + (m_iDistanceBetweenChars * m_dwCapacity) + m_sizeFontMain.cx;
	m_iIndentFirstHexChunkX = m_iSecondVertLine + m_sizeFontMain.cx;
	m_iSizeFirstHalf = m_iIndentFirstHexChunkX + m_dwCapacityBlockSize * (m_sizeFontMain.cx * 2) +
		(m_dwCapacityBlockSize / static_cast<DWORD>(m_enGroupMode) - 1) * m_iSpaceBetweenHexChunks;
	m_iHeightTopRect = std::lround(m_sizeFontMain.cy * 1.5);
	m_iStartWorkAreaY = m_iFirstHorzLine + m_iHeightTopRect;
	m_iSecondHorzLine = m_iStartWorkAreaY - 1;
	m_iIndentCapTextY = m_iHeightTopRect / 2 - (m_sizeFontMain.cy / 2);

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
		ULONGLONG ullScrollPageSize;
		if (m_fPageLines) { //How many text lines to scroll.
			ullScrollPageSize = m_sizeFontMain.cy * static_cast<LONG>(m_dbWheelRatio);
			if (ullScrollPageSize < m_sizeFontMain.cy) {
				ullScrollPageSize = m_sizeFontMain.cy;
			}
		}
		else { //How mane screens to scroll.
			const auto ullSizeInScreens = static_cast<ULONGLONG>(m_iHeightWorkArea * m_dbWheelRatio);
			ullScrollPageSize = ullSizeInScreens < m_sizeFontMain.cy ? m_sizeFontMain.cy : ullSizeInScreens;
		}

		m_pScrollV->SetScrollSizes(m_sizeFontMain.cy, ullScrollPageSize,
			static_cast<ULONGLONG>(m_iStartWorkAreaY) + m_iHeightBottomOffArea + m_sizeFontMain.cy *
			(ullDataSize / m_dwCapacity + (ullDataSize % m_dwCapacity == 0 ? 1 : 2)));
		m_pScrollH->SetScrollSizes(m_sizeFontMain.cx, rc.Width(), static_cast<ULONGLONG>(m_iFourthVertLine) + 1);
		m_pScrollV->SetScrollPos(ullCurLineV * m_sizeFontMain.cy);

		ReleaseDC(pDCCurr);
		Redraw();
	}
}

void CHexCtrl::RecalcClientArea(int iHeight, int iWidth)
{
	m_iHeightClientArea = iHeight;
	m_iWidthClientArea = iWidth;
	m_iEndWorkArea = m_iHeightClientArea - m_iHeightBottomOffArea -
		((m_iHeightClientArea - m_iStartWorkAreaY - m_iHeightBottomOffArea) % m_sizeFontMain.cy);
	m_iHeightWorkArea = m_iEndWorkArea - m_iStartWorkAreaY;
	m_iThirdHorzLine = m_iFirstHorzLine + m_iHeightClientArea - m_iHeightBottomOffArea;
	m_iFourthHorzLine = m_iThirdHorzLine + m_iHeightInfoBar;
}

void CHexCtrl::Redo()
{
	if (m_vecRedo.empty())
		return;

	const auto& refRedo = m_vecRedo.back();

	VecSpan vecSpan;
	std::transform(refRedo->begin(), refRedo->end(), std::back_inserter(vecSpan),
		[](SUNDO& ref) { return HEXSPAN { ref.ullOffset, ref.vecData.size() }; });

	//Making new Undo data snapshot.
	SnapshotUndo(vecSpan);

	for (const auto& iter : *refRedo) {
		const auto& refRedoData = iter.vecData;

		//In Virtual mode processing data chunk by chunk.
		if (IsVirtual() && refRedoData.size() > GetCacheSize()) {
			const auto ullSizeChunk = GetCacheSize();
			const auto iMod = refRedoData.size() % ullSizeChunk;
			auto ullChunks = refRedoData.size() / ullSizeChunk + (iMod > 0 ? 1 : 0);
			std::size_t ullOffset = 0;
			while (ullChunks-- > 0) {
				const auto ullSize = (ullChunks == 1 && iMod > 0) ? iMod : ullSizeChunk;
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
	const auto iMaxClientX = rcClient.Width() - m_iSizeHexByte;
	const auto iAdditionalSpace { m_iSizeHexByte / 2 };
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
	ULONGLONG ullClick { }, ullStart { }, ullSize { 0ULL };
	ULONGLONG ullNewPos { }; //Future pos of selection start.
	ULONGLONG ullOldPos { }; //Current pos of selection start.

	const auto lmbSelection = [&]() {
		if (ullSelStart == m_ullCursorPrev) {
			ullClick = ullStart = m_ullCursorPrev;
			ullSize = ullSelSize + m_dwCapacity;
			ullNewPos = ullClick + ullSize - 1;
			ullOldPos = ullNewPos - m_dwCapacity;
		}
		else if (ullSelStart < m_ullCursorPrev) {
			ullClick = m_ullCursorPrev;
			if (ullSelSize > m_dwCapacity) {
				ullStart = ullSelStart + m_dwCapacity;
				ullSize = ullSelSize - m_dwCapacity;
			}
			else {
				ullStart = ullClick;
				ullSize = 1;
			}
			ullOldPos = ullSelStart;
			ullNewPos = ullOldPos + m_dwCapacity;
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
	ULONGLONG ullClick { }, ullStart { }, ullSize { 0 };
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
	ULONGLONG ullClick { }, ullStart { 0 }, ullSize { 0 };
	ULONGLONG ullNewPos { }; //Future pos of selection start.
	ULONGLONG ullOldPos { }; //Current pos of selection start.

	const auto lmbSelection = [&]() {
		if (ullSelStart < m_ullCursorPrev) {
			ullClick = m_ullCursorPrev;
			ullOldPos = ullSelStart;

			if (ullSelStart < m_dwCapacity) {
				ullSize = ullSelSize + ullSelStart;
				ullNewPos = ullOldPos;
			}
			else {
				ullStart = ullSelStart - m_dwCapacity;
				ullSize = ullSelSize + m_dwCapacity;
				ullNewPos = ullStart;
			}
		}
		else {
			ullClick = m_ullCursorPrev;
			if (ullSelSize > m_dwCapacity) {
				ullStart = ullClick;
				ullSize = ullSelSize - m_dwCapacity;
			}
			else if (ullSelSize > 1) {
				ullStart = ullClick;
				ullSize = 1;
			}
			else {
				ullStart = ullClick >= m_dwCapacity ? ullClick - m_dwCapacity : 0;
				ullSize = ullClick >= m_dwCapacity ? ullSelSize + m_dwCapacity : ullSelSize + ullSelStart;
			}

			ullOldPos = ullSelStart + ullSelSize - 1;
			ullNewPos = ullOldPos - m_dwCapacity;
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
	const auto& refUndo = m_vecUndo.emplace_back(std::make_unique<std::vector<SUNDO>>());

	//Bad alloc may happen here!!!
	try {
		for (const auto& iterSel : vecSpan) { //vecSpan.size() amount of continuous areas to preserve.
			auto& refUNDO = refUndo->emplace_back(SUNDO { iterSel.ullOffset, { } });
			refUNDO.vecData.resize(static_cast<std::size_t>(iterSel.ullSize));

			//In Virtual mode processing data chunk by chunk.
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
	const DWORD dwMod = ullOffset % m_dwCapacity;
	iCx = static_cast<int>((m_iIndentTextX + dwMod * m_sizeFontMain.cx) - m_pScrollH->GetScrollPos());

	const auto ullScrollV = m_pScrollV->GetScrollPos();
	iCy = static_cast<int>((m_iStartWorkAreaY + (ullOffset / m_dwCapacity) * m_sizeFontMain.cy) -
		(ullScrollV - (ullScrollV % m_sizeFontMain.cy)));
}

void CHexCtrl::ToolTipBkmShow(bool fShow, POINT pt, bool fTimerCancel)
{
	if (fShow) {
		m_tmTtBkm = std::time(nullptr);
		m_wndTtBkm.SendMessageW(TTM_TRACKPOSITION, 0, static_cast<LPARAM>(MAKELONG(pt.x, pt.y)));
		m_wndTtBkm.SendMessageW(TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(&m_stToolInfoBkm));
		m_wndTtBkm.SendMessageW(TTM_TRACKACTIVATE, static_cast<WPARAM>(TRUE), reinterpret_cast<LPARAM>(&m_stToolInfoBkm));
		SetTimer(m_uiIDTTBkm, 300, nullptr);
	}
	else if (fTimerCancel) { //Tooltip was canceled by the timer, not mouse move.
		m_wndTtBkm.SendMessageW(TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&m_stToolInfoBkm));
	}
	else {
		m_pBkmTtCurr = nullptr;
		m_wndTtBkm.SendMessageW(TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&m_stToolInfoBkm));
		KillTimer(m_uiIDTTBkm);
	}
}

void CHexCtrl::ToolTipTemplShow(bool fShow, POINT pt, bool fTimerCancel)
{
	if (fShow) {
		m_tmTtTempl = std::time(nullptr);
		m_wndTtTempl.SendMessageW(TTM_TRACKPOSITION, 0, static_cast<LPARAM>(MAKELONG(pt.x, pt.y)));
		m_wndTtTempl.SendMessageW(TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(&m_stToolInfoTempl));
		m_wndTtTempl.SendMessageW(TTM_TRACKACTIVATE, static_cast<WPARAM>(TRUE), reinterpret_cast<LPARAM>(&m_stToolInfoTempl));
		SetTimer(m_uiIDTTTempl, 300, nullptr);
	}
	else if (fTimerCancel) { //Tooltip was canceled by the timer, not mouse move.
		m_wndTtTempl.SendMessageW(TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&m_stToolInfoTempl));
	}
	else {
		m_pTFieldTtCurr = nullptr;
		m_wndTtTempl.SendMessageW(TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&m_stToolInfoTempl));
		KillTimer(m_uiIDTTTempl);
	}
}

void CHexCtrl::ToolTipOffsetShow(bool fShow)
{
	if (fShow) {
		CPoint ptScreen;
		GetCursorPos(&ptScreen);

		wchar_t warrOffset[64] { L"Offset: " };
		OffsetToString(GetTopLine() * m_dwCapacity, &warrOffset[8]);
		m_stToolInfoOffset.lpszText = warrOffset;
		m_wndTtOffset.SendMessageW(TTM_TRACKPOSITION, 0, static_cast<LPARAM>(MAKELONG(ptScreen.x - 5, ptScreen.y - 20)));
		m_wndTtOffset.SendMessageW(TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(&m_stToolInfoOffset));
	}
	m_wndTtOffset.SendMessageW(TTM_TRACKACTIVATE, static_cast<WPARAM>(fShow), reinterpret_cast<LPARAM>(&m_stToolInfoOffset));
}

void CHexCtrl::Undo()
{
	if (m_vecUndo.empty())
		return;

	//Bad alloc may happen here!!!
	//If there is no more free memory, just clear the vec and return.
	try {
		//Making new Redo data snapshot.
		const auto& refRedo = m_vecRedo.emplace_back(std::make_unique<std::vector<SUNDO>>());

		for (const auto& iter : *m_vecUndo.back()) {
			auto& refRedoBack = refRedo->emplace_back(SUNDO { iter.ullOffset, { } });
			refRedoBack.vecData.resize(iter.vecData.size());
			const auto& refUndoData = iter.vecData;

			//In Virtual mode processing data chunk by chunk.
			if (IsVirtual() && refUndoData.size() > GetCacheSize()) {
				const auto ullSizeChunk = GetCacheSize();
				const auto iMod = refUndoData.size() % ullSizeChunk;
				auto ullChunks = refUndoData.size() / ullSizeChunk + (iMod > 0 ? 1 : 0);
				std::size_t ullOffset = 0;
				while (ullChunks-- > 0) {
					const auto ullSize = (ullChunks == 1 && iMod > 0) ? iMod : ullSizeChunk;
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

	unsigned char chByte = nChar & 0xFF;
	wchar_t warrCurrLocaleID[KL_NAMELENGTH];
	GetKeyboardLayoutNameW(warrCurrLocaleID); //Current langID as wstring.
	if (const auto optLocID = stn::StrToUInt(warrCurrLocaleID, 16); optLocID) { //Convert langID from wstr to number.
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
	if (const auto iter = std::find_if(m_vecKeyBind.begin(), m_vecKeyBind.end(),
		[=](const SKEYBIND& ref) { return ref.wMenuID == wMenuID; }); iter != m_vecKeyBind.end()) {
		ExecuteCmd(iter->eCmd);
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
	m_menuMain.DestroyMenu();
	m_fontMain.DeleteObject();
	m_fontInfoBar.DeleteObject();
	m_penLines.DeleteObject();
	m_penDataTempl.DeleteObject();
	m_vecHBITMAP.clear();
	m_vecKeyBind.clear();
	m_pDlgTemplMgr->UnloadAll(); //Explicitly unloading all loaded Templates.
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
	//Bit 14 indicates that the key was pressed continuously.
	//0x4000 == 0100 0000 0000 0000.
	if (nFlags & 0x4000) {
		m_fKeyDownAtm = true;
	}

	if (const auto optCmd = GetCommand(nChar, GetAsyncKeyState(VK_CONTROL) < 0,
		GetAsyncKeyState(VK_SHIFT) < 0, GetAsyncKeyState(VK_MENU) < 0); optCmd) {
		ExecuteCmd(*optCmd);
	}
	else if (IsDataSet() && IsMutable() && !IsCurTextArea()) { //If caret is in Hex area, just one part (High/Low) of the byte must be changed.
		unsigned char chByte = nChar & 0xFF;
		//Normalizing all input in Hex area, reducing it to 0-15 (0x0-F) digit range.
		//Allowing only [0-9][A-F][NUM0-NUM9].
		if (chByte >= 0x30 && chByte <= 0x39) {      //Digits [0-9].
			chByte -= 0x30;
		}
		else if (chByte >= 0x41 && chByte <= 0x46) { //Hex letters [A-F].
			chByte -= 0x37;
		}
		else if (chByte >= 0x60 && chByte <= 0x69) { //VK_NUMPAD0 - VK_NUMPAD9 [NUM0-NUM9].
			chByte -= 0x60;
		}
		else
			return;

		auto chByteCurr = GetIHexTData<unsigned char>(*this, GetCaretPos());
		if (m_fCaretHigh) {
			chByte = (chByte << 4) | (chByteCurr & 0x0F);
		}
		else {
			chByte = (chByte & 0x0F) | (chByteCurr & 0xF0);
		}
		ModifyData({ .spnData { reinterpret_cast<std::byte*>(&chByte), sizeof(chByte) }, .vecSpan { { GetCaretPos(), 1 } } });
		CaretMoveRight();
	}
}

void CHexCtrl::OnKeyUp(UINT /*nChar*/, UINT /*nRepCnt*/, UINT /*nFlags*/)
{
	//If some key was previously pressed for some time, and now is released.
	//Inspecting current caret position.
	if (m_fKeyDownAtm && IsDataSet()) {
		m_fKeyDownAtm = false;
		m_pDlgDataInterp->UpdateData();
	}
}

void CHexCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if ((point.x + static_cast<long>(m_pScrollH->GetScrollPos())) < m_iSecondVertLine) { //DblClick on "Offset" area.
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
	SetFocus();

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
				point.x = m_iIndentFirstHexChunkX;
			}
			else if (point.x >= rcClient.right) {
				m_pScrollH->ScrollLineRight();
				point.x = m_iFourthVertLine - 1;
			}
		}
		if (m_pScrollV->IsVisible()) {
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
		//Doesn't apply when moving in Text area.
		if (!optHit || (optHit->ullOffset == m_ullCursorNow && m_fCaretHigh == optHit->fIsHigh && !optHit->fIsText))
			return;

		m_ullCursorNow = optHit->ullOffset;
		const auto ullOffsetHit = optHit->ullOffset;
		ULONGLONG ullClick, ullStart, ullSize, ullLines;
		if (m_fSelectionBlock) { //Select block (with Alt).
			ullClick = m_ullCursorPrev;
			const DWORD dwModOffset = ullOffsetHit % m_dwCapacity;
			const DWORD dwModClick = ullClick % m_dwCapacity;
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
				ullLines = (ullOffsetHit - ullStart) / m_dwCapacity + 1;
			}
			else {
				if (dwModOffset <= dwModClick) {
					ullStart = ullOffsetHit;
					ullSize = dwModClick - ullStart % m_dwCapacity + 1;
				}
				else {
					const auto dwModStart = dwModOffset - dwModClick;
					ullStart = ullOffsetHit - dwModStart;
					ullSize = dwModStart + 1;
				}
				ullLines = (ullClick - ullStart) / m_dwCapacity + 1;
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
			vecSel.emplace_back(ullStart + m_dwCapacity * iterLines, ullSize);
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
					m_stToolInfoTempl.lpszText = pField->wstrName.data();
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
	if (nFlags == MK_CONTROL) {
		FontSizeIncDec(zDelta > 0);
		return TRUE;
	}

	if (nFlags == (MK_CONTROL | MK_SHIFT)) {
		SetCapacity(m_dwCapacity + zDelta / WHEEL_DELTA);
		return TRUE;
	}

	if (zDelta > 0) { //Scrolling Up.
		m_pScrollV->ScrollPageUp();
	}
	else {
		m_pScrollV->ScrollPageDown();
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
	if (rcClient.IsRectEmpty() || rcClient.Height() <= m_iHeightTopRect + m_iHeightBottomOffArea)
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
	DrawHexText(pDC, iLines, wstrHex, wstrText);
	DrawTemplates(pDC, ullStartLine, iLines, wstrHex, wstrText);
	DrawBookmarks(pDC, ullStartLine, iLines, wstrHex, wstrText);
	DrawCustomColors(pDC, ullStartLine, iLines, wstrHex, wstrText);
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
	if (!m_fPageLines) {
		const auto ullPageSize = static_cast<ULONGLONG>(m_iHeightWorkArea * m_dbWheelRatio);
		m_pScrollV->SetScrollPageSize(ullPageSize < m_sizeFontMain.cy ? m_sizeFontMain.cy : ullPageSize);
	}
}

void CHexCtrl::OnSysKeyDown(UINT nChar, UINT /*nRepCnt*/, UINT /*nFlags*/)
{
	if (auto optCmd = GetCommand(nChar, GetAsyncKeyState(VK_CONTROL) < 0, GetAsyncKeyState(VK_SHIFT) < 0, true); optCmd) {
		ExecuteCmd(*optCmd);
	}
}

void CHexCtrl::OnTimer(UINT_PTR nIDEvent)
{
	constexpr auto tmSecToShow { 5.0 }; //How many seconds to show the Tooltip.

	CRect rcClient;
	GetClientRect(rcClient);
	ClientToScreen(rcClient);
	CPoint ptCursor;
	GetCursorPos(&ptCursor);

	switch (nIDEvent) {
	case m_uiIDTTBkm:
		if (!rcClient.PtInRect(ptCursor)) { //Checking if cursor has left client rect.
			ToolTipBkmShow(false);
		}
		//Or more than tmSecToShow seconds have passed since bkm toolip is shown.
		else if (std::difftime(std::time(nullptr), m_tmTtBkm) >= tmSecToShow) {
			ToolTipBkmShow(false, { }, true);
		}
		break;
	case m_uiIDTTTempl:
		if (!rcClient.PtInRect(ptCursor)) { //Checking if cursor has left client rect.
			ToolTipTemplShow(false);
		}
		//Or more than tmSecToShow seconds have passed since template toolip is shown.
		else if (std::difftime(std::time(nullptr), m_tmTtTempl) >= tmSecToShow) {
			ToolTipTemplShow(false, { }, true);
		}
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
