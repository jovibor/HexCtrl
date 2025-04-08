/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
module;
#include <SDKDDKVer.h>
#include "../dep/rapidjson/rapidjson-amalgam.h"
#include "../res/HexCtrlRes.h"
#include "../HexCtrl.h"
#include <algorithm>
#include <bit>
#include <cassert>
#include <chrono>
#include <commctrl.h>
#include <cwctype>
#include <format>
#include <fstream>
#include <intrin.h>
#include <numeric>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#pragma comment(lib, "Comctl32.lib")
export module HEXCTRL;

import :CHexScroll;
import :CHexSelection;
import :CHexDlgBkmMgr;
import :CHexDlgCodepage;
import :CHexDlgDataInterp;
import :CHexDlgGoTo;
import :CHexDlgModify;
import :CHexDlgProgress;
import :CHexDlgSearch;
import :CHexDlgTemplMgr;
import :HexUtility;

using namespace HEXCTRL::INTERNAL;

namespace HEXCTRL::INTERNAL {
	class CHexDlgAbout final {
	public:
		CHexDlgAbout(HINSTANCE hInstRes) { assert(hInstRes != nullptr); m_hInstRes = hInstRes; }
		auto DoModal(HWND hWndParent = nullptr) -> INT_PTR;
		[[nodiscard]] auto ProcessMsg(const MSG& msg) -> INT_PTR;
	private:
		auto OnCommand(const MSG& msg) -> INT_PTR;
		auto OnCtlClrStatic(const MSG& msg) -> INT_PTR;
		auto OnDestroy() -> INT_PTR;
		auto OnInitDialog(const MSG& msg) -> INT_PTR;
		auto OnLButtonDown(const MSG& msg) -> INT_PTR;
		auto OnLButtonUp(const MSG& msg) -> INT_PTR;
		auto OnMouseMove(const MSG& msg) -> INT_PTR;
	private:
		HINSTANCE m_hInstRes { };
		wnd::CWnd m_Wnd;        //Main window.
		wnd::CWnd m_WndLink;    //Static link control
		HBITMAP m_hBmpLogo { }; //Logo bitmap.
		HFONT m_hFontDef { };
		HFONT m_hFontUnderline { };
		bool m_fLinkUnderline { };
		bool m_fLBDownLink { }; //Left button was pressed on the link static control.
	};
}

auto CHexDlgAbout::DoModal(HWND hWndParent)->INT_PTR {
	return ::DialogBoxParamW(m_hInstRes, MAKEINTRESOURCEW(IDD_HEXCTRL_ABOUT),
		hWndParent, wnd::DlgProc<CHexDlgAbout>, reinterpret_cast<LPARAM>(this));
}

auto CHexDlgAbout::ProcessMsg(const MSG& msg)->INT_PTR {
	switch (msg.message) {
	case WM_COMMAND: return OnCommand(msg);
	case WM_CTLCOLORSTATIC: return OnCtlClrStatic(msg);
	case WM_DESTROY: return OnDestroy();
	case WM_INITDIALOG: return OnInitDialog(msg);
	case WM_LBUTTONDOWN: return OnLButtonDown(msg);
	case WM_LBUTTONUP: return OnLButtonUp(msg);
	case WM_MOUSEMOVE: return OnMouseMove(msg);
	default:
		return 0;
	}
}

auto CHexDlgAbout::OnCommand(const MSG& msg)->INT_PTR {
	const auto uCtrlID = LOWORD(msg.wParam);
	switch (uCtrlID) {
	case IDOK:
	case IDCANCEL:
		m_Wnd.EndDialog(IDOK);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

auto CHexDlgAbout::OnCtlClrStatic(const MSG& msg)->INT_PTR
{
	if (const auto hWndFrom = reinterpret_cast<HWND>(msg.lParam); hWndFrom == m_WndLink) {
		const auto hDC = reinterpret_cast<HDC>(msg.wParam);
		::SetTextColor(hDC, RGB(0, 50, 250));
		::SetBkColor(hDC, ::GetSysColor(COLOR_3DFACE));
		::SelectObject(hDC, m_fLinkUnderline ? m_hFontUnderline : m_hFontDef);
		return reinterpret_cast<INT_PTR>(::GetSysColorBrush(COLOR_3DFACE));
	}

	return FALSE; //Default handler.
}

auto CHexDlgAbout::OnDestroy()->INT_PTR {
	::DeleteObject(m_hBmpLogo);
	::DeleteObject(m_hFontDef);
	::DeleteObject(m_hFontUnderline);

	return TRUE;
};

auto CHexDlgAbout::OnInitDialog(const MSG& msg)->INT_PTR
{
	m_Wnd.Attach(msg.hwnd);
	m_Wnd.SetWndClassLong(GCLP_HCURSOR, 0); //To prevent cursor blinking.
	m_WndLink.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_ABOUT_STAT_LINKGH));

	if (const auto hFont = m_WndLink.GetHFont(); hFont != nullptr) {
		m_hFontDef = hFont;
		auto lf = m_WndLink.GetLogFont().value();
		lf.lfUnderline = TRUE;
		m_hFontUnderline = ::CreateFontIndirectW(&lf);
	}
	else {
		LOGFONTW lf { };
		::GetObjectW(static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT)), sizeof(lf), &lf);
		m_hFontDef = ::CreateFontIndirectW(&lf);
		lf.lfUnderline = TRUE;
		m_hFontUnderline = ::CreateFontIndirectW(&lf);
	}

	const auto wstrVersion = std::format(L"Hex Control for Windows apps, v{}.{}.{}\r\nCopyright © 2018-present Jovibor",
		HEXCTRL_VERSION_MAJOR, HEXCTRL_VERSION_MINOR, HEXCTRL_VERSION_PATCH);
	::SetWindowTextW(m_Wnd.GetDlgItem(IDC_HEXCTRL_ABOUT_STAT_VERSION), wstrVersion.data());

	const auto iSizeIcon = static_cast<int>(32 * ut::GetDPIScale(m_Wnd));
	m_hBmpLogo = static_cast<HBITMAP>(::LoadImageW(m_hInstRes, MAKEINTRESOURCEW(IDB_HEXCTRL_LOGO),
		IMAGE_BITMAP, iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	const auto hWndLogo = m_Wnd.GetDlgItem(IDC_HEXCTRL_ABOUT_LOGO);
	::SendMessageW(hWndLogo, STM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(m_hBmpLogo));

	return TRUE;
}

auto CHexDlgAbout::OnLButtonDown(const MSG& msg)->INT_PTR
{
	const POINT pt { .x { wnd::GetXLPARAM(msg.lParam) }, .y { wnd::GetYLPARAM(msg.lParam) } };
	const auto hWnd = m_Wnd.ChildWindowFromPoint(pt);
	if (hWnd != m_WndLink) {
		m_fLBDownLink = false;
		return FALSE;
	}

	m_fLBDownLink = true;

	return TRUE;
}

auto CHexDlgAbout::OnLButtonUp(const MSG& msg) -> INT_PTR
{
	const POINT pt { .x { wnd::GetXLPARAM(msg.lParam) }, .y { wnd::GetYLPARAM(msg.lParam) } };
	const auto hWnd = m_Wnd.ChildWindowFromPoint(pt);
	if (hWnd != m_WndLink) {
		m_fLBDownLink = false;
		return FALSE;
	}

	if (m_fLBDownLink) {
		::ShellExecuteW(nullptr, L"open", m_WndLink.GetWndText().data(), nullptr, nullptr, 0);
	}

	return TRUE;
}

auto CHexDlgAbout::OnMouseMove(const MSG& msg)->INT_PTR
{
	const POINT pt { .x { wnd::GetXLPARAM(msg.lParam) }, .y { wnd::GetYLPARAM(msg.lParam) } };
	const auto hWnd = m_Wnd.ChildWindowFromPoint(pt);
	if (hWnd == nullptr)
		return FALSE;

	const auto curHand = reinterpret_cast<HCURSOR>(::LoadImageW(nullptr, IDC_HAND, IMAGE_CURSOR, 0, 0, LR_SHARED));
	const auto curArrow = reinterpret_cast<HCURSOR>(::LoadImageW(nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED));

	if (m_fLinkUnderline != (m_WndLink == hWnd)) {
		m_fLinkUnderline = m_WndLink == hWnd;
		m_WndLink.Invalidate(false);
		::SetCursor(m_fLinkUnderline ? curHand : curArrow);
	}

	return TRUE;
}


//CHexCtrl.

namespace HEXCTRL::INTERNAL {
	class CHexCtrl final : public IHexCtrl {
	public:
		explicit CHexCtrl();
		CHexCtrl(const CHexCtrl&) = delete;
		CHexCtrl(CHexCtrl&&) = delete;
		CHexCtrl& operator=(const CHexCtrl&) = delete;
		CHexCtrl& operator=(CHexCtrl&&) = delete;
		~CHexCtrl()override;
		void ClearData()override;
		bool Create(const HEXCREATE& hcs)override;
		bool CreateDialogCtrl(UINT uCtrlID, HWND hWndParent)override;
		void Delete()override;
		void DestroyWindow()override;
		void ExecuteCmd(EHexCmd eCmd)override;
		[[nodiscard]] auto GetActualWidth()const->int override;
		[[nodiscard]] auto GetBookmarks()const->IHexBookmarks* override;
		[[nodiscard]] auto GetCacheSize()const->DWORD override;
		[[nodiscard]] auto GetCapacity()const->DWORD override;
		[[nodiscard]] auto GetCaretPos()const->ULONGLONG override;
		[[nodiscard]] auto GetCharsExtraSpace()const->DWORD override;
		[[nodiscard]] auto GetCodepage()const->int override;
		[[nodiscard]] auto GetColors()const->const HEXCOLORS & override;
		[[nodiscard]] auto GetData(HEXSPAN hss)const->SpanByte override;
		[[nodiscard]] auto GetDataSize()const->ULONGLONG override;
		[[nodiscard]] auto GetDateInfo()const->std::tuple<DWORD, wchar_t> override;
		[[nodiscard]] auto GetDlgItemHandle(EHexDlgItem eItem)const->HWND override;
		[[nodiscard]] auto GetFont(bool fMain = true)const->LOGFONTW override;
		[[nodiscard]] auto GetGroupSize()const->DWORD override;
		[[nodiscard]] auto GetMenuHandle()const->HMENU override;
		[[nodiscard]] auto GetOffset(ULONGLONG ullOffset, bool fGetVirt)const->ULONGLONG override;
		[[nodiscard]] auto GetPagesCount()const->ULONGLONG override;
		[[nodiscard]] auto GetPagePos()const->ULONGLONG override;
		[[nodiscard]] auto GetPageSize()const->DWORD override;
		[[nodiscard]] auto GetScrollRatio()const->std::tuple<float, bool> override;
		[[nodiscard]] auto GetSelection()const->VecSpan override;
		[[nodiscard]] auto GetTemplates()const->IHexTemplates* override;
		[[nodiscard]] auto GetUnprintableChar()const->wchar_t override;
		[[nodiscard]] auto GetWndHandle(EHexWnd eWnd, bool fCreate)const->HWND override;
		void GoToOffset(ULONGLONG ullOffset, int iPosAt = 0)override;
		[[nodiscard]] bool HasInfoBar()const override;
		[[nodiscard]] bool HasSelection()const override;
		[[nodiscard]] auto HitTest(POINT pt, bool fScreen)const->std::optional<HEXHITTEST> override;
		[[nodiscard]] bool IsCmdAvail(EHexCmd eCmd)const override;
		[[nodiscard]] bool IsCreated()const override;
		[[nodiscard]] bool IsDataSet()const override;
		[[nodiscard]] bool IsMutable()const override;
		[[nodiscard]] bool IsOffsetAsHex()const override;
		[[nodiscard]] auto IsOffsetVisible(ULONGLONG ullOffset)const->HEXVISION override;
		[[nodiscard]] bool IsVirtual()const override;
		void ModifyData(const HEXMODIFY& hms)override;
		[[nodiscard]] bool PreTranslateMsg(MSG* pMsg)override;
		[[nodiscard]] auto ProcessMsg(const MSG& msg) -> LRESULT;
		void Redraw()override;
		void SetCapacity(DWORD dwCapacity)override;
		void SetCaretPos(ULONGLONG ullOffset, bool fHighLow = true, bool fRedraw = true)override;
		void SetCharsExtraSpace(DWORD dwSpace)override;
		void SetCodepage(int iCodepage)override;
		void SetColors(const HEXCOLORS& hcs)override;
		bool SetConfig(std::wstring_view wsvPath)override;
		void SetData(const HEXDATA& hds, bool fAdjust)override;
		void SetDateInfo(DWORD dwFormat, wchar_t wchSepar)override;
		void SetDlgProperties(EHexWnd eWnd, std::uint64_t u64Flags)override;
		void SetFont(const LOGFONTW& lf, bool fMain = true)override;
		void SetGroupSize(DWORD dwSize)override;
		void SetMutable(bool fMutable)override;
		void SetOffsetMode(bool fHex)override;
		void SetPageSize(DWORD dwSize, std::wstring_view wsvName)override;
		void SetRedraw(bool fRedraw)override;
		void SetScrollRatio(float flRatio, bool fLines)override;
		void SetSelection(const VecSpan& vecSel, bool fRedraw = true, bool fHighlight = false)override;
		void SetUnprintableChar(wchar_t wch)override;
		void SetVirtualBkm(IHexBookmarks* pVirtBkm)override;
		void SetWindowPos(HWND hWndAfter, int iX, int iY, int iWidth, int iHeight, UINT uFlags)override;
		void ShowInfoBar(bool fShow)override;
	private:
		struct KEYBIND; struct UNDO; enum class EClipboard : std::uint8_t;
		[[nodiscard]] auto BuildDataToDraw(ULONGLONG ullStartLine, int iLines)const->std::tuple<std::wstring, std::wstring>;
		void CaretMoveDown();  //Set caret one line down.
		void CaretMoveLeft();  //Set caret one chunk left.
		void CaretMoveRight(); //Set caret one chunk right.
		void CaretMoveUp();    //Set caret one line up.
		void CaretToDataBeg(); //Set caret to a data beginning.
		void CaretToDataEnd(); //Set caret to a data end.
		void CaretToLineBeg(); //Set caret to a current line beginning.
		void CaretToLineEnd(); //Set caret to a current line end.
		void CaretToPageBeg(); //Set caret to a current page beginning.
		void CaretToPageEnd(); //Set caret to a current page end.
		void ChooseFontDlg();  //The "ChooseFont" dialog.
		void ClipboardCopy(EClipboard eType)const;
		void ClipboardPaste(EClipboard eType);
		[[nodiscard]] auto CopyBase64()const->std::wstring;
		[[nodiscard]] auto CopyCArr()const->std::wstring;
		[[nodiscard]] auto CopyGrepHex()const->std::wstring;
		[[nodiscard]] auto CopyHex()const->std::wstring;
		[[nodiscard]] auto CopyHexFmt()const->std::wstring;
		[[nodiscard]] auto CopyHexLE()const->std::wstring;
		[[nodiscard]] auto CopyOffset()const->std::wstring;
		[[nodiscard]] auto CopyPrintScreen()const->std::wstring;
		[[nodiscard]] auto CopyTextCP()const->std::wstring;
		void DrawWindow(HDC hDC)const;
		void DrawInfoBar(HDC hDC)const;
		void DrawOffsets(HDC hDC, ULONGLONG ullStartLine, int iLines)const;
		void DrawHexText(HDC hDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const;
		void DrawTemplates(HDC hDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const;
		void DrawBookmarks(HDC hDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const;
		void DrawSelection(HDC hDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const;
		void DrawSelHighlight(HDC hDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const;
		void DrawCaret(HDC hDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const;
		void DrawDataInterp(HDC hDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const;
		void DrawPageLines(HDC hDC, ULONGLONG ullStartLine, int iLines)const;
		void FillCapacityString(); //Fill m_wstrCapacity according to current m_dwCapacity.
		void FillWithZeros();      //Fill selection with zeros.
		void FontSizeIncDec(bool fInc = true); //Increase os decrease font size by minimum amount.
		[[nodiscard]] auto GetBottomLine()const->ULONGLONG; //Returns current bottom line number in view.
		[[nodiscard]] auto GetCharsWidthArray()const->int*;
		[[nodiscard]] auto GetCharWidthExtras()const->int;  //Width of the one char with extra space, in px.
		[[nodiscard]] auto GetCharWidthNative()const->int;  //Width of the one char, in px.
		[[nodiscard]] auto GetCommandFromKey(UINT uKey, bool fCtrl, bool fShift, bool fAlt)const->std::optional<EHexCmd>; //Get command from keybinding.
		[[nodiscard]] auto GetCommandFromMenu(WORD wMenuID)const->std::optional<EHexCmd>; //Get command from menuID.
		[[nodiscard]] auto GetDigitsOffset()const->DWORD;
		[[nodiscard]] long GetFontSize()const;
		[[nodiscard]] auto GetRectTextCaption()const->wnd::CRect;   //Returns rect of the text caption area.
		[[nodiscard]] auto GetSelectedLines()const->ULONGLONG; //Get amount of selected lines.
		[[nodiscard]] auto GetScrollPageSize()const->ULONGLONG; //Get the "Page" size of the scroll.
		[[nodiscard]] auto GetTopLine()const->ULONGLONG;       //Returns current top line number in view.
		[[nodiscard]] auto GetVirtualOffset(ULONGLONG ullOffset)const->ULONGLONG;
		void HexChunkPoint(ULONGLONG ullOffset, int& iCx, int& iCy)const; //Point of Hex chunk.
		[[nodiscard]] auto HitTest(POINT pt)const->std::optional<HEXHITTEST>; //Is any hex chunk withing given point?
		[[nodiscard]] bool IsCurTextArea()const;               //Whether last focus was set at Text or Hex chunks area.
		[[nodiscard]] bool IsDrawable()const;                  //Should WM_PAINT be handled atm or not.
		[[nodiscard]] bool IsPageVisible()const;               //Returns m_fSectorVisible.
		//Main "Modify" method with different workers.
		void ModifyWorker(const HEXCTRL::HEXMODIFY& hms, const auto& FuncWorker, HEXCTRL::SpanCByte spnOper);
		[[nodiscard]] auto OffsetToWstr(ULONGLONG ullOffset)const->std::wstring; //Format offset as std::wstring.
		void OnCaretPosChange(ULONGLONG ullOffset);            //On changing caret position.
		void OnModifyData();                                   //When data has been modified.
		template<typename T> requires std::is_class_v<T>
		void ParentNotify(const T& t)const;                    //Notify routine used to send messages to Parent window.
		void ParentNotify(UINT uCode)const;                    //Same as above, but only for notification code.
		void Print();                                          //Printing routine.
		void RecalcAll(bool fPrinter = false, HDC hDCPrinter = nullptr, LPCRECT pRCPrinter = nullptr); //Recalculates all sizes for window/printer DC.
		void RecalcClientArea(int iWidth, int iHeight);
		void Redo();
		void ReplaceUnprintable(std::wstring& wstr, bool fASCII, bool fCRLF)const; //Substitute all unprintable wchar symbols with specified wchar.
		void ScrollOffsetH(ULONGLONG ullOffset); //Scroll horizontally to given offset.
		void SelAll();      //Select all.
		void SelAddDown();  //Down Key pressed with the Shift.
		void SelAddLeft();  //Left Key pressed with the Shift.
		void SelAddRight(); //Right Key pressed with the Shift.
		void SelAddUp();    //Up Key pressed with the Shift.
		void SetDataVirtual(SpanByte spnData, const HEXSPAN& hss)const; //Sets data (notifies back) in VirtualData mode.
		void SetFontSize(long lSize); //Set current font size.
		void SnapshotUndo(const VecSpan& vecSpan); //Takes currently modifiable data snapshot.
		void TextChunkPoint(ULONGLONG ullOffset, int& iCx, int& iCy)const; //Point of the text chunk.
		void TTMainShow(bool fShow, bool fTimer = false); //Main tooltip show/hide.
		void TTOffsetShow(bool fShow); //Tooltip Offset show/hide.
		void Undo();
		static void ModifyOper(std::byte* pData, const HEXMODIFY& hms, SpanCByte); //Modify operation classical.
		static void ModifyOperVec128(std::byte* pData, const HEXMODIFY& hms, SpanCByte); //Modify operation x86/x64 vector 128.
		static void ModifyOperVec256(std::byte* pData, const HEXMODIFY& hms, SpanCByte); //Modify operation x86/x64 vector 256.

		//Message handlers.
		auto OnChar(const MSG& msg) -> LRESULT;
		auto OnCommand(const MSG& msg) -> LRESULT;
		auto OnContextMenu(const MSG& msg) -> LRESULT;
		auto OnDestroy() -> LRESULT;
		auto OnEraseBkgnd(const MSG& msg) -> LRESULT;
		auto OnGetDlgCode(const MSG& msg) -> LRESULT;
		auto OnHelp(const MSG& msg) -> LRESULT;
		auto OnHScroll(const MSG& msg) -> LRESULT;
		auto OnInitMenuPopup(const MSG& msg) -> LRESULT;
		auto OnKeyDown(const MSG& msg) -> LRESULT;
		auto OnKeyUp(const MSG& msg) -> LRESULT;
		auto OnLButtonDblClk(const MSG& msg) -> LRESULT;
		auto OnLButtonDown(const MSG& msg) -> LRESULT;
		auto OnLButtonUp(const MSG& msg) -> LRESULT;
		auto OnMouseMove(const MSG& msg) -> LRESULT;
		auto OnMouseWheel(const MSG& msg) -> LRESULT;
		auto OnNCActivate(const MSG& msg) -> LRESULT;
		auto OnNCCalcSize(const MSG& msg) -> LRESULT;
		auto OnNCPaint(const MSG& msg) -> LRESULT;
		auto OnPaint() -> LRESULT;
		auto OnSetCursor(const MSG& msg) -> LRESULT;
		auto OnSize(const MSG& msg) -> LRESULT;
		auto OnSysKeyDown(const MSG& msg) -> LRESULT;
		auto OnTimer(const MSG& msg) -> LRESULT;
		auto OnVScroll(const MSG& msg) -> LRESULT;
		static auto CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
			UINT_PTR uIDSubclass, DWORD_PTR dwRefData)->LRESULT;
	private:
		static constexpr auto m_pwszHexChars { L"0123456789ABCDEF" }; //Hex digits wchars for fast lookup.
		static constexpr auto m_pwszClassName { L"HexCtrl_MainWnd" }; //HexCtrl unique Window Class name.
		static constexpr auto m_uIDTTTMain { 0x01UL };                //Timer ID for default tooltip.
		static constexpr auto m_iFirstHorzLinePx { 0 };               //First horizontal line indent.
		static constexpr auto m_iFirstVertLinePx { 0 };               //First vertical line indent.
		static constexpr auto m_dwVKMouseWheelUp { 0x0100UL };        //Artificial Virtual Key for a Mouse-Wheel Up event.
		static constexpr auto m_dwVKMouseWheelDown { 0x0101UL };      //Artificial Virtual Key for a Mouse-Wheel Down event.
		const std::unique_ptr<CHexDlgBkmMgr> m_pDlgBkmMgr { std::make_unique<CHexDlgBkmMgr>() };             //"Bookmark manager" dialog.
		const std::unique_ptr<CHexDlgCodepage> m_pDlgCodepage { std::make_unique<CHexDlgCodepage>() };       //"Codepage" dialog.
		const std::unique_ptr<CHexDlgDataInterp> m_pDlgDataInterp { std::make_unique<CHexDlgDataInterp>() }; //"Data interpreter" dialog.
		const std::unique_ptr<CHexDlgModify> m_pDlgModify { std::make_unique<CHexDlgModify>() };             //"Modify..." dialog.
		const std::unique_ptr<CHexDlgGoTo> m_pDlgGoTo { std::make_unique<CHexDlgGoTo>() };                   //"GoTo..." dialog.
		const std::unique_ptr<CHexDlgSearch> m_pDlgSearch { std::make_unique<CHexDlgSearch>() };             //"Search..." dialog.
		const std::unique_ptr<CHexDlgTemplMgr> m_pDlgTemplMgr { std::make_unique<CHexDlgTemplMgr>() };       //"Template manager..." dialog.
		const std::unique_ptr<CHexSelection> m_pSelection { std::make_unique<CHexSelection>() };             //Selection class.
		const std::unique_ptr<CHexScroll> m_pScrollV { std::make_unique<CHexScroll>() };                     //Vertical scroll bar.
		const std::unique_ptr<CHexScroll> m_pScrollH { std::make_unique<CHexScroll>() };                     //Horizontal scroll bar.
		HINSTANCE m_hInstRes { };             //Hinstance of the HexCtrl resources.
		wnd::CWnd m_Wnd;                      //Main window.
		wnd::CWnd m_wndTTMain;                //Main tooltip window.
		wnd::CWnd m_wndTTOffset;              //Tooltip window for Offset in m_fHighLatency mode.
		wnd::CMenu m_MenuMain;                //Main popup menu.
		std::wstring m_wstrCapacity;          //Top Capacity string.
		std::wstring m_wstrInfoBar;           //Info bar text.
		std::wstring m_wstrPageName;          //Name of the sector/page.
		std::wstring m_wstrTextTitle;         //Text area title.
		std::vector<std::unique_ptr<std::vector<UNDO>>> m_vecUndo; //Undo data.
		std::vector<std::unique_ptr<std::vector<UNDO>>> m_vecRedo; //Redo data.
		std::vector < std::unique_ptr < std::remove_pointer_t<HBITMAP>,
			decltype([](HBITMAP hBmp) { DeleteObject(hBmp); }) >> m_vecHBITMAP; //Icons for the Menu.
		std::vector<KEYBIND> m_vecKeyBind;    //Vector of key bindings.
		std::vector<int> m_vecCharsWidth;     //Vector of chars widths.
		HFONT m_hFntMain { };                 //Main Hex chunks font.
		HFONT m_hFntInfoBar { };              //Font for bottom Info bar.
		HPEN m_hPenLines { };                 //Pen for lines.
		HPEN m_hPenDataTempl { };             //Pen for templates' fields (vertical lines).
		HEXCOLORS m_stColors;                 //All HexCtrl colors.
		IHexVirtData* m_pHexVirtData { };     //Data handler pointer for Virtual mode.
		IHexVirtColors* m_pHexVirtColors { }; //Pointer for custom colors class.
		SpanByte m_spnData;                   //Main data span.
		TTTOOLINFOW m_ttiMain { };            //Main tooltip info.
		TTTOOLINFOW m_ttiOffset { };          //Tooltip info for Offset.
		std::chrono::steady_clock::time_point m_tmTT; //Start time of the tooltip.
		PHEXBKM m_pBkmTTCurr { };             //Currently shown bookmark's tooltip;
		PCHEXTEMPLFIELD m_pTFieldTTCurr { };  //Currently shown Template field's tooltip;
		ULONGLONG m_ullCaretPos { };          //Current caret position.
		ULONGLONG m_ullCursorNow { };         //The cursor's current clicked pos.
		ULONGLONG m_ullCursorPrev { };        //The cursor's previously clicked pos, used in selection resolutions.
		SIZE m_sizeFontMain { 1, 1 };         //Main font letter's size (width, height).
		SIZE m_sizeFontInfo { 1, 1 };         //Info window font letter's size (width, height).
		POINT m_stMenuClickedPt { };          //RMouse coords when clicked.
		DWORD m_dwGroupSize { };              //Current data grouping size.
		DWORD m_dwCapacity { };               //How many bytes are displayed in one row.
		DWORD m_dwCapacityBlockSize { };      //Size of the block before a space delimiter.
		DWORD m_dwDigitsOffsetDec { };        //Amount of digits for "Offset" in Decimal mode, 10 is max for 32bit number.
		DWORD m_dwDigitsOffsetHex { };        //Amount of digits for "Offset" in Hex mode, 8 is max for 32bit number.
		DWORD m_dwPageSize { };               //Size of a page to print additional lines between.
		DWORD m_dwCacheSize { };              //Data cache size for VirtualData mode.
		DWORD m_dwDateFormat { };             //Current date format. See https://docs.microsoft.com/en-gb/windows/win32/intl/locale-idate
		DWORD m_dwCharsExtraSpace { };        //Extra space between chars.
		int m_iSizeFirstHalfPx { };           //Size in px of the first half of the capacity.
		int m_iSizeHexBytePx { };             //Size in px of two hex letters representing one byte.
		int m_iIndentTextXPx { };             //Indent in px of the text beginning.
		int m_iIndentFirstHexChunkXPx { };    //First hex chunk indent in px.
		int m_iIndentCapTextYPx { };          //Caption text (0 1 2... D E F...) vertical offset.
		int m_iDistanceGroupedHexChunkPx { }; //Distance between begining of the two hex grouped chunks, in px.
		int m_iDistanceBetweenCharsPx { };    //Distance between beginning of the two text chars in px.
		int m_iSpaceBetweenBlocksPx { };      //Additional space between hex chunks after half of capacity, in px.
		int m_iWidthClientAreaPx { };         //Width of the HexCtrl window client area.
		int m_iStartWorkAreaYPx { };          //Start Y of the area where all drawing occurs.
		int m_iEndWorkAreaPx { };             //End of the area where all drawing occurs.
		int m_iHeightClientAreaPx { };        //Height of the HexCtrl window client area.
		int m_iHeightTopRectPx { };           //Height of the header where offsets (0 1 2... D E F...) reside.
		int m_iHeightWorkAreaPx { };          //Height in px of the working area where all drawing occurs.
		int m_iHeightInfoBarPx { };           //Height of the bottom Info rect.
		int m_iHeightBottomOffAreaPx { };     //Height of the not visible rect from window's bottom to m_iThirdHorizLine.
		int m_iSecondHorzLinePx { };          //Second horizontal line indent.
		int m_iThirdHorzLinePx { };           //Third horizontal line indent.
		int m_iFourthHorzLinePx { };          //Fourth horizontal line indent.
		int m_iSecondVertLinePx { };          //Second vert line indent.
		int m_iThirdVertLinePx { };           //Third vert line indent.
		int m_iFourthVertLinePx { };          //Fourth vert line indent.
		int m_iCodePage { };                  //Current code-page for Text area. -1 for default.
		int m_iLOGPIXELSY { };                //GetDeviceCaps(LOGPIXELSY) constant.
		float m_flScrollRatio { };            //Ratio for how much to scroll with mouse-wheel.
		wchar_t m_wchUnprintable { };         //Replacement char for unprintable characters.
		wchar_t m_wchDateSepar { };           //Date separator.
		bool m_fCreated { false };            //Is control created or not yet.
		bool m_fDataSet { false };            //Is data set or not.
		bool m_fMutable { false };            //Does control work in Edit or ReadOnly mode.
		bool m_fInfoBar { true };             //Show bottom Info window or not.
		bool m_fCaretHigh { true };           //Caret's High or Low bits position (first or last digit in hex chunk).
		bool m_fCursorTextArea { false };     //Whether last focus was set at ASCII or Hex chunks area.
		bool m_fLMousePressed { false };      //Is left mouse button pressed.
		bool m_fClickWithAlt { false };       //Mouse click was with Alt pressed.
		bool m_fOffsetHex { false };          //Print offset numbers as Hex or as Decimals.
		bool m_fHighLatency { false };        //Reflects HEXDATA::fHighLatency.
		bool m_fKeyDownAtm { false };         //Whether a key is pressed at the moment.
		bool m_fRedraw { true };              //Should WM_PAINT be handled or not.
		bool m_fScrollLines { false };        //Page scroll in "Screen * m_flScrollRatio" or in lines.
	};
}

export HEXCTRLAPI HEXCTRL::IHexCtrlPtr HEXCTRL::CreateHexCtrl() {
	return IHexCtrlPtr { new HEXCTRL::INTERNAL::CHexCtrl() };
}

struct CHexCtrl::KEYBIND { //Key bindings.
	EHexCmd eCmd { };
	WORD    wMenuID { };
	UINT    uKey { };
	bool    fCtrl { };
	bool    fShift { };
	bool    fAlt { };
};

struct CHexCtrl::UNDO {
	ULONGLONG              ullOffset { }; //Start byte to apply Undo to.
	std::vector<std::byte> vecData;       //Data for Undo.
};

enum class CHexCtrl::EClipboard : std::uint8_t {
	COPY_HEX, COPY_HEXLE, COPY_HEXFMT, COPY_BASE64, COPY_CARR,
	COPY_GREPHEX, COPY_PRNTSCRN, COPY_OFFSET, COPY_TEXT_CP,
	PASTE_HEX, PASTE_TEXT_UTF16, PASTE_TEXT_CP
};

CHexCtrl::CHexCtrl()
{
	//CS_GLOBALCLASS flag creates window class which is global to a whole app.
	//This window class can be accessed from the app itself and from any dll this app uses.
	//If HexCtrl is used as a regular dll, its window class will be registered with
	//the hInstance of the dll and hence will be unavailable from the main app.
	//It's mostly vital for the Custom controls within dialogs, which must have access to custom
	//classes names during dialog creation.
	if (WNDCLASSEXW wc { }; ::GetClassInfoExW(nullptr, m_pwszClassName, &wc) == FALSE) {
		wc.cbSize = sizeof(WNDCLASSEXW);
		wc.style = CS_GLOBALCLASS | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = wnd::WndProc<CHexCtrl>;
		wc.hCursor = static_cast<HCURSOR>(::LoadImageW(nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
		wc.lpszClassName = m_pwszClassName;
		if (::RegisterClassExW(&wc) == 0) {
			ut::DBG_REPORT(L"RegisterClassExW failed.");
			return;
		}
	}
}

CHexCtrl::~CHexCtrl()
{
	::UnregisterClassW(m_pwszClassName, nullptr);
}

void CHexCtrl::ClearData()
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

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
	m_pDlgSearch->ClearData();
	Redraw();
}

bool CHexCtrl::Create(const HEXCREATE& hcs)
{
	if (IsCreated()) { ut::DBG_REPORT(L"Already created."); return false; }

	HWND hWnd;
	if (hcs.fCustom) {
		hWnd = ::GetDlgItem(hcs.hWndParent, hcs.uID);
		if (hWnd == nullptr) {
			ut::DBG_REPORT(L"GetDlgItem failed.");
			return false;
		}

		if (::SetWindowSubclass(hWnd, SubclassProc, reinterpret_cast<UINT_PTR>(this), 0) == FALSE) {
			ut::DBG_REPORT(L"SubclassDlgItem failed, check HEXCREATE parameters.");
			return false;
		}
	}
	else {
		const wnd::CRect rc = hcs.rect;
		if (hWnd = ::CreateWindowExW(hcs.dwExStyle, m_pwszClassName, L"HexCtrl", hcs.dwStyle, rc.left, rc.top, rc.Width(),
			rc.Height(), hcs.hWndParent, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(hcs.uID)), nullptr, this);
			hWnd == nullptr) {
			ut::DBG_REPORT(L"CreateWindowExW failed, check HEXCREATE parameters.");
			return false;
		}
	}

	m_Wnd.Attach(hWnd);

	m_wndTTMain.Attach(::CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr, TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_Wnd, nullptr, nullptr, nullptr));
	m_ttiMain.cbSize = sizeof(TTTOOLINFOW);
	m_ttiMain.uFlags = TTF_TRACK;
	m_wndTTMain.SendMsg(TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&m_ttiMain));
	m_wndTTMain.SendMsg(TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(400)); //To allow the use of a newline \n.

	m_wndTTOffset.Attach(::CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr,
		TTS_NOANIMATE | TTS_NOFADE | TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_Wnd, nullptr, nullptr, nullptr));
	m_ttiOffset.cbSize = sizeof(TTTOOLINFOW);
	m_ttiOffset.uFlags = TTF_TRACK;
	m_wndTTOffset.SendMsg(TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&m_ttiOffset));
	m_wndTTOffset.SendMsg(TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(400)); //To allow the use of a newline \n.

	if (hcs.pColors != nullptr) {
		m_stColors = *hcs.pColors;
	}

	m_hInstRes = hcs.hInstRes != nullptr ? hcs.hInstRes : ut::GetCurrModuleHinst();
	m_dwCapacity = std::clamp(hcs.dwCapacity, 1UL, 100UL);
	m_flScrollRatio = hcs.flScrollRatio;
	m_fScrollLines = hcs.fScrollLines;
	m_fInfoBar = hcs.fInfoBar;
	m_fOffsetHex = hcs.fOffsetHex;
	m_dwDigitsOffsetDec = 10UL;
	m_dwDigitsOffsetHex = 8UL;

	const auto hDC = m_Wnd.GetDC();
	m_iLOGPIXELSY = ::GetDeviceCaps(hDC, LOGPIXELSY);
	m_Wnd.ReleaseDC(hDC);

	//Menu related.
	if (!m_MenuMain.LoadMenuW(m_hInstRes, MAKEINTRESOURCEW(IDR_HEXCTRL_MENU))) {
		ut::DBG_REPORT(L"LoadMenuW failed.");
		return false;
	}

	const auto iSizeIcon = static_cast<int>(16 * ut::GetDPIScale(m_Wnd));
	const auto menuTop = m_MenuMain.GetSubMenu(0); //Context sub-menu handle.

	MENUITEMINFOW mii { .cbSize { sizeof(MENUITEMINFOW) }, .fMask { MIIM_BITMAP } };
	//"Search" menu icon.
	mii.hbmpItem = static_cast<HBITMAP>(::LoadImageW(m_hInstRes, MAKEINTRESOURCEW(IDB_HEXCTRL_SEARCH), IMAGE_BITMAP,
		iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	menuTop.SetMenuItemInfoW(0, &mii, false); //"Search" parent menu icon.
	m_MenuMain.SetMenuItemInfoW(IDM_HEXCTRL_SEARCH_DLGSEARCH, &mii);
	m_vecHBITMAP.emplace_back(mii.hbmpItem);

	//"Group Data" menu icon.
	mii.hbmpItem = static_cast<HBITMAP>(::LoadImageW(m_hInstRes, MAKEINTRESOURCEW(IDB_HEXCTRL_GROUP), IMAGE_BITMAP,
		iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	menuTop.SetMenuItemInfoW(2, &mii, false); //"Group Data" parent menu icon.
	m_vecHBITMAP.emplace_back(mii.hbmpItem);

	//"Bookmarks->Add" menu icon.
	mii.hbmpItem = static_cast<HBITMAP>(::LoadImageW(m_hInstRes, MAKEINTRESOURCEW(IDB_HEXCTRL_BKMS), IMAGE_BITMAP,
		iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	menuTop.SetMenuItemInfoW(4, &mii, false); //"Bookmarks" parent menu icon.
	m_MenuMain.SetMenuItemInfoW(IDM_HEXCTRL_BKM_ADD, &mii);
	m_vecHBITMAP.emplace_back(mii.hbmpItem);

	//"Clipboard->Copy as Hex" menu icon.
	mii.hbmpItem = static_cast<HBITMAP>(::LoadImageW(m_hInstRes, MAKEINTRESOURCEW(IDB_HEXCTRL_CLPBRD_COPYHEX), IMAGE_BITMAP,
		iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	menuTop.SetMenuItemInfoW(5, &mii, false); //"Clipboard" parent menu icon.
	m_MenuMain.SetMenuItemInfoW(IDM_HEXCTRL_CLPBRD_COPYHEX, &mii);
	m_vecHBITMAP.emplace_back(mii.hbmpItem);

	//"Clipboard->Paste as Hex" menu icon.
	mii.hbmpItem = static_cast<HBITMAP>(::LoadImageW(m_hInstRes, MAKEINTRESOURCEW(IDB_HEXCTRL_CLPBRD_PASTEHEX), IMAGE_BITMAP,
		iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	m_MenuMain.SetMenuItemInfoW(IDM_HEXCTRL_CLPBRD_PASTEHEX, &mii);
	m_vecHBITMAP.emplace_back(mii.hbmpItem);

	//"Modify" parent menu icon.
	mii.hbmpItem = static_cast<HBITMAP>(::LoadImageW(m_hInstRes, MAKEINTRESOURCEW(IDB_HEXCTRL_MODIFY), IMAGE_BITMAP,
		iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	menuTop.SetMenuItemInfoW(6, &mii, false);
	m_vecHBITMAP.emplace_back(mii.hbmpItem);

	//"Modify->Fill with Zeros" menu icon.
	mii.hbmpItem = static_cast<HBITMAP>(::LoadImageW(m_hInstRes, MAKEINTRESOURCEW(IDB_HEXCTRL_MODIFY_FILLZEROS), IMAGE_BITMAP,
		iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	m_MenuMain.SetMenuItemInfoW(IDM_HEXCTRL_MODIFY_FILLZEROS, &mii);
	m_vecHBITMAP.emplace_back(mii.hbmpItem);

	//"Appearance->Choose Font" menu icon.
	mii.hbmpItem = static_cast<HBITMAP>(::LoadImageW(m_hInstRes, MAKEINTRESOURCEW(IDB_HEXCTRL_FONTCHOOSE), IMAGE_BITMAP,
		iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	m_MenuMain.SetMenuItemInfoW(IDM_HEXCTRL_APPEAR_DLGFONT, &mii);
	m_vecHBITMAP.emplace_back(mii.hbmpItem);
	//End of menu related.

	//Font related.
	//Default main logfont.
	const LOGFONTW lfMain { .lfHeight { -::MulDiv(11, m_iLOGPIXELSY, 72) }, .lfWeight { FW_NORMAL },
		.lfQuality { CLEARTYPE_QUALITY }, .lfPitchAndFamily { FIXED_PITCH }, .lfFaceName { L"Consolas" } };
	m_hFntMain = ::CreateFontIndirectW(hcs.pLogFont != nullptr ? hcs.pLogFont : &lfMain);

	//Info area font, independent from the main font, its size is a bit smaller than the default main font.
	const LOGFONTW lfInfo { .lfHeight { -::MulDiv(11, m_iLOGPIXELSY, 72) + 1 }, .lfWeight { FW_NORMAL },
		.lfQuality { CLEARTYPE_QUALITY }, .lfPitchAndFamily { FIXED_PITCH }, .lfFaceName { L"Consolas" } };
	m_hFntInfoBar = ::CreateFontIndirectW(&lfInfo);
	//End of font related.

	m_hPenLines = ::CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
	m_hPenDataTempl = ::CreatePen(PS_SOLID, 1, RGB(50, 50, 50));

	//ScrollBars should be created here, after the main window has already been created (to attach to), to avoid assertions.
	m_pScrollV->Create(m_Wnd, true, m_hInstRes, IDB_HEXCTRL_SCROLL_ARROW, 0, 0, 0); //Actual sizes are set in RecalcAll().
	m_pScrollH->Create(m_Wnd, false, m_hInstRes, IDB_HEXCTRL_SCROLL_ARROW, 0, 0, 0);
	m_pScrollV->AddSibling(m_pScrollH.get());
	m_pScrollH->AddSibling(m_pScrollV.get());

	m_fCreated = true; //Main creation flag.

	SetGroupSize(std::clamp(hcs.dwGroupSize, 1UL, 64UL));
	SetCodepage(-1);
	SetConfig(L"");
	SetDateInfo(0xFFFFFFFFUL, L'/');
	SetUnprintableChar(L'.');

	//All dialogs are initialized after the main window, to set the parent handle correctly.
	m_pDlgBkmMgr->Initialize(this, m_hInstRes);
	m_pDlgDataInterp->Initialize(this, m_hInstRes);
	m_pDlgCodepage->Initialize(this, m_hInstRes);
	m_pDlgGoTo->Initialize(this, m_hInstRes);
	m_pDlgSearch->Initialize(this, m_hInstRes);
	m_pDlgTemplMgr->Initialize(this, m_hInstRes);
	m_pDlgModify->Initialize(this, m_hInstRes);

	return true;
}

bool CHexCtrl::CreateDialogCtrl(UINT uCtrlID, HWND hWndParent)
{
	if (IsCreated()) { ut::DBG_REPORT(L"Already created."); return false; }

	return Create({ .hWndParent { hWndParent }, .uID { uCtrlID }, .fCustom { true } });
}

void CHexCtrl::Delete()
{
	//At this point the HexCtrl window should be destroyed anyway.
	//This call is just to make sure it is.
	DestroyWindow();

	delete this;
}

void CHexCtrl::DestroyWindow()
{
	if (m_Wnd.IsWindow()) {
		m_Wnd.DestroyWindow();
	}
}

void CHexCtrl::ExecuteCmd(EHexCmd eCmd)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }
	if (!IsCmdAvail(eCmd)) return;

	using enum EHexCmd;
	switch (eCmd) {
	case CMD_SEARCH_DLG:
		ParentNotify(HEXCTRL_MSG_DLGSEARCH);
		m_pDlgSearch->ShowWindow(SW_SHOW);
		break;
	case CMD_SEARCH_NEXT:
		m_pDlgSearch->SearchNextPrev(true);
		m_Wnd.SetFocus();
		break;
	case CMD_SEARCH_PREV:
		m_pDlgSearch->SearchNextPrev(false);
		m_Wnd.SetFocus();
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
			: VecSpan { { GetCaretPos(), 1 } } },
			.stClr { .clrBk { GetColors().clrBkBkm }, .clrText { GetColors().clrFontBkm } } }, true);
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
	case CMD_SEL_MARKSTARTEND:
		m_pSelection->SetMarkStartEnd(GetCaretPos());
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
		CHexDlgAbout dlgAbout(m_hInstRes);
		dlgAbout.DoModal(m_Wnd);
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
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }

	return m_iFourthVertLinePx + 1; //+1px is the Pen width the line was drawn with.
}

auto CHexCtrl::GetBookmarks()const->IHexBookmarks*
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }

	return &*m_pDlgBkmMgr;
}

auto CHexCtrl::GetCacheSize()const->DWORD
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }
	if (!IsDataSet()) { ut::DBG_REPORT_NO_DATA_SET(); return { }; }

	return m_dwCacheSize;
}

auto CHexCtrl::GetCapacity()const->DWORD
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }

	return m_dwCapacity;
}

auto CHexCtrl::GetCaretPos()const->ULONGLONG
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }
	if (!IsDataSet()) { ut::DBG_REPORT_NO_DATA_SET(); return { }; }

	return m_ullCaretPos;
}

auto CHexCtrl::GetCharsExtraSpace()const->DWORD
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }

	return m_dwCharsExtraSpace;
}

int CHexCtrl::GetCodepage()const
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }

	return m_iCodePage;
}

auto CHexCtrl::GetColors()const->const HEXCOLORS&
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); }

	return m_stColors;
}

auto CHexCtrl::GetData(HEXSPAN hss)const->SpanByte
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }
	if (!IsDataSet()) { ut::DBG_REPORT_NO_DATA_SET(); return { }; }
	assert(hss.ullSize > 0);

	SpanByte spnData;
	if (!IsVirtual()) {
		if (hss.ullOffset + hss.ullSize <= GetDataSize()) {
			spnData = { m_spnData.data() + hss.ullOffset, static_cast<std::size_t>(hss.ullSize) };
		}
	}
	else {
		assert(hss.ullSize <= GetCacheSize());
		HEXDATAINFO hdi { .hdr { m_Wnd, static_cast<UINT>(m_Wnd.GetDlgCtrlID()) }, .stHexSpan { hss } };
		m_pHexVirtData->OnHexGetData(hdi);
		spnData = hdi.spnData;
	}

	return spnData;
}

auto CHexCtrl::GetDataSize()const->ULONGLONG
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }

	return m_spnData.size();
}

auto CHexCtrl::GetDateInfo()const->std::tuple<DWORD, wchar_t>
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }

	return { m_dwDateFormat, m_wchDateSepar };
}

auto CHexCtrl::GetDlgItemHandle(EHexDlgItem eItem)const->HWND
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }

	using enum EHexDlgItem;
	switch (eItem) {
	case BKMMGR_CHK_HEX:
		return m_pDlgBkmMgr->GetDlgItemHandle(eItem);
	case DATAINTERP_CHK_HEX: case DATAINTERP_CHK_BE:
		return m_pDlgDataInterp->GetDlgItemHandle(eItem);
	case FILLDATA_COMBO_DATA:
		return m_pDlgModify->GetDlgItemHandle(eItem);
	case SEARCH_COMBO_FIND:	case SEARCH_COMBO_REPLACE: case SEARCH_EDIT_START:
	case SEARCH_EDIT_STEP: case SEARCH_EDIT_RNGBEG: case SEARCH_EDIT_RNGEND:
	case SEARCH_EDIT_LIMIT:
		return m_pDlgSearch->GetDlgItemHandle(eItem);
	case TEMPLMGR_CHK_MIN: case TEMPLMGR_CHK_TT: case TEMPLMGR_CHK_HGL:
	case TEMPLMGR_CHK_HEX: case TEMPLMGR_CHK_SWAP:
		return m_pDlgTemplMgr->GetDlgItemHandle(eItem);
	default:
		return { };
	};
}

auto CHexCtrl::GetFont(bool fMain)const->LOGFONTW
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }

	LOGFONTW lf;
	::GetObjectW(fMain ? m_hFntMain : m_hFntInfoBar, sizeof(lf), &lf);

	return lf;
}

auto CHexCtrl::GetGroupSize()const->DWORD
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }

	return m_dwGroupSize;
}

auto CHexCtrl::GetMenuHandle()const->HMENU
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }

	return m_MenuMain.GetSubMenu(0);
}

auto CHexCtrl::GetOffset(ULONGLONG ullOffset, bool fGetVirt)const->ULONGLONG
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }
	if (!IsDataSet()) { ut::DBG_REPORT_NO_DATA_SET(); return { }; }

	if (IsVirtual()) {
		HEXDATAINFO hdi { .hdr { m_Wnd, static_cast<UINT>(m_Wnd.GetDlgCtrlID()) }, .stHexSpan { .ullOffset { ullOffset } } };
		m_pHexVirtData->OnHexGetOffset(hdi, fGetVirt);
		return hdi.stHexSpan.ullOffset;
	}

	return ullOffset;
}

auto CHexCtrl::GetPagesCount()const->ULONGLONG
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }
	if (!IsDataSet()) { ut::DBG_REPORT_NO_DATA_SET(); return { }; }
	if (GetPageSize() == 0) return { };

	const auto ullSize = GetDataSize();
	return (ullSize / m_dwPageSize) + ((ullSize % m_dwPageSize) ? 1 : 0);
}

auto CHexCtrl::GetPagePos()const->ULONGLONG
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }
	if (!IsDataSet()) { ut::DBG_REPORT_NO_DATA_SET(); return { }; }

	return GetCaretPos() / GetPageSize();
}

auto CHexCtrl::GetPageSize()const->DWORD
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }

	return m_dwPageSize;
}

auto CHexCtrl::GetScrollRatio()const->std::tuple<float, bool>
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }

	return { m_flScrollRatio, m_fScrollLines };
}

auto CHexCtrl::GetSelection()const->VecSpan
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }
	if (!IsDataSet()) { ut::DBG_REPORT_NO_DATA_SET(); return { }; }

	return m_pSelection->GetData();
}

auto CHexCtrl::GetTemplates()const->IHexTemplates*
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }

	return &*m_pDlgTemplMgr;
}

auto CHexCtrl::GetUnprintableChar()const->wchar_t
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }

	return m_wchUnprintable;
}

auto CHexCtrl::GetWndHandle(EHexWnd eWnd, bool fCreate)const->HWND
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }

	switch (eWnd) {
	case EHexWnd::WND_MAIN:
		return m_Wnd;
	case EHexWnd::DLG_BKMMGR:
		if (!::IsWindow(m_pDlgBkmMgr->GetHWND()) && fCreate) {
			m_pDlgBkmMgr->CreateDlg();
		}
		return m_pDlgBkmMgr->GetHWND();
	case EHexWnd::DLG_DATAINTERP:
		if (!::IsWindow(m_pDlgDataInterp->GetHWND()) && fCreate) {
			m_pDlgDataInterp->CreateDlg();
		}
		return m_pDlgDataInterp->GetHWND();
	case EHexWnd::DLG_MODIFY:
		if (!::IsWindow(m_pDlgModify->GetHWND()) && fCreate) {
			m_pDlgModify->CreateDlg();
		}
		return m_pDlgModify->GetHWND();
	case EHexWnd::DLG_SEARCH:
		if (!::IsWindow(m_pDlgSearch->GetHWND()) && fCreate) {
			m_pDlgSearch->CreateDlg();
		}
		return m_pDlgSearch->GetHWND();
	case EHexWnd::DLG_CODEPAGE:
		if (!::IsWindow(m_pDlgCodepage->GetHWND()) && fCreate) {
			m_pDlgCodepage->CreateDlg();
		}
		return m_pDlgCodepage->GetHWND();
	case EHexWnd::DLG_GOTO:
		if (!::IsWindow(m_pDlgGoTo->GetHWND()) && fCreate) {
			m_pDlgGoTo->CreateDlg();
		}
		return m_pDlgGoTo->GetHWND();
	case EHexWnd::DLG_TEMPLMGR:
		if (!::IsWindow(m_pDlgTemplMgr->GetHWND()) && fCreate) {
			m_pDlgTemplMgr->CreateDlg();
		}
		return m_pDlgTemplMgr->GetHWND();
	default:
		return { };
	}
}

void CHexCtrl::GoToOffset(ULONGLONG ullOffset, int iPosAt)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }
	if (!IsDataSet()) { ut::DBG_REPORT_NO_DATA_SET(); return; }
	if (ullOffset >= GetDataSize()) return;

	const auto dwCapacity = GetCapacity() > 0 ? GetCapacity() : 0xFFFFFFFFUL; //To suppress warning C4724.
	const auto ullNewStartV = ullOffset / dwCapacity * m_sizeFontMain.cy;
	auto ullNewScrollV { 0ULL };

	switch (iPosAt) {
	case -1: //Position offset at the top line.
		ullNewScrollV = ullNewStartV;
		break;
	case 0: //Position offset in the vcenter.
		if (ullNewStartV > m_iHeightWorkAreaPx / 2) { //To prevent negative numbers.
			ullNewScrollV = ullNewStartV - m_iHeightWorkAreaPx / 2;
			ullNewScrollV -= ullNewScrollV % m_sizeFontMain.cy;
		}
		break;
	case 1: //Position offset at the bottom line.
		ullNewScrollV = ullNewStartV - (GetBottomLine() - GetTopLine()) * m_sizeFontMain.cy;
		break;
	default:
		break;
	}
	m_pScrollV->SetScrollPos(ullNewScrollV);

	if (m_pScrollH->IsVisible() && !IsCurTextArea()) {
		auto ullNewScrollH = (ullOffset % dwCapacity) * m_iSizeHexBytePx;
		ullNewScrollH += (ullNewScrollH / m_iDistanceGroupedHexChunkPx) * GetCharWidthExtras();
		m_pScrollH->SetScrollPos(ullNewScrollH);
	}
}

bool CHexCtrl::HasInfoBar()const
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return false; }

	return m_fInfoBar;
}

bool CHexCtrl::HasSelection()const
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return false; }

	return m_pSelection->HasSelection();
}

auto CHexCtrl::HitTest(POINT pt, bool fScreen)const->std::optional<HEXHITTEST>
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return std::nullopt; }
	if (!IsDataSet()) { ut::DBG_REPORT_NO_DATA_SET(); return std::nullopt; }

	if (fScreen) {
		m_Wnd.ScreenToClient(&pt);
	}

	return HitTest(pt);
}

bool CHexCtrl::IsCmdAvail(EHexCmd eCmd)const
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return false; }

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
		fAvail = fMutable && ::IsClipboardFormatAvailable(CF_UNICODETEXT);
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
	case CMD_SEL_MARKSTARTEND:
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
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return false; }

	return m_fDataSet;
}

bool CHexCtrl::IsMutable()const
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return false; }
	if (!IsDataSet()) { ut::DBG_REPORT_NO_DATA_SET(); return false; }

	return m_fMutable;
}

bool CHexCtrl::IsOffsetAsHex()const
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return false; }

	return m_fOffsetHex;
}

auto CHexCtrl::IsOffsetVisible(ULONGLONG ullOffset)const->HEXVISION
{
	//Returns HEXVISION with two std::int8_t for vertical and horizontal visibility respectively.
	//-1 - ullOffset is higher, or at the left, of the visible area
	// 1 - lower, or at the right
	// 0 - visible.
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { .i8Vert { -1 }, .i8Horz { -1 } }; }
	if (!IsDataSet()) { ut::DBG_REPORT_NO_DATA_SET(); return { .i8Vert { -1 }, .i8Horz { -1 } }; }

	const auto dwCapacity = GetCapacity();
	const auto ullFirst = GetTopLine() * dwCapacity;
	const auto ullLast = (GetBottomLine() * dwCapacity) + dwCapacity;

	const auto rcClient = m_Wnd.GetClientRect();
	int iCx, iCy;
	HexChunkPoint(ullOffset, iCx, iCy);
	const auto iMaxClientX = rcClient.Width() - m_iSizeHexBytePx;

	return { .i8Vert { static_cast<std::int8_t>(ullOffset < ullFirst ? -1 : (ullOffset >= ullLast ? 1 : 0)) },
		.i8Horz { static_cast<std::int8_t>(iCx < 0 ? -1 : (iCx >= iMaxClientX ? 1 : 0)) } };
}

bool CHexCtrl::IsVirtual()const
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return false; }
	if (!IsDataSet()) { ut::DBG_REPORT_NO_DATA_SET(); return false; }

	return m_pHexVirtData != nullptr;
}

void CHexCtrl::ModifyData(const HEXMODIFY& hms)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }
	if (!IsMutable()) return;
	if (hms.vecSpan.empty()) { ut::DBG_REPORT(L"Data to modify is empty."); return; }

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
		std::mt19937 gen(std::random_device { }());
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
				const auto spnData = GetData({ .ullOffset { ullOffset }, .ullSize { dwRem } });
				for (std::size_t itRem = 0; itRem < dwRem; ++itRem) {
					spnData.data()[itRem] = static_cast<std::byte>(distUInt64(gen));
				}
				SetDataVirtual(spnData, { .ullOffset { ullOffset }, .ullSize { dwRem } });
			}
		}
		else if (hms.eModifyMode == MODIFY_RAND_FAST && hms.vecSpan.size() == 1 && refHexSpan.ullSize >= GetCacheSize()) {
			//Fill the uptrRandData buffer with true random data of ulSizeRandBuff size.
			//Then clone this buffer to the destination data.
			//Buffer is allocated with alignment for maximum performance.
			constexpr auto ulSizeRandBuff { 1024U * 1024U }; //1MB.
			const std::unique_ptr < std::byte[], decltype([](std::byte* pData) { ::_aligned_free(pData); }) >
				uptrRandData(static_cast<std::byte*>(::_aligned_malloc(ulSizeRandBuff, 32)));
			for (auto it = 0UL; it < ulSizeRandBuff; it += sizeof(std::uint64_t)) {
				*reinterpret_cast<std::uint64_t*>(&uptrRandData[it]) = distUInt64(gen);
			};

			ModifyWorker(hms, lmbRandFast, { uptrRandData.get(), ulSizeRandBuff });

			//Filling the remainder data.
			if (const auto ullRem = refHexSpan.ullSize % ulSizeRandBuff; ullRem > 0) { //Remainder.
				if (ullRem <= GetCacheSize()) {
					const auto ullOffsetCurr = refHexSpan.ullOffset + refHexSpan.ullSize - ullRem;
					const auto spnData = GetData({ .ullOffset { ullOffsetCurr }, .ullSize { ullRem } });
					assert(!spnData.empty());
					std::copy_n(uptrRandData.get(), ullRem, spnData.data());
					SetDataVirtual(spnData, { .ullOffset { ullOffsetCurr }, .ullSize { ullRem } });
				}
				else {
					const auto ullSizeCache = GetCacheSize();
					const auto dwModCache = ullRem % ullSizeCache;
					auto ullChunks = (ullRem / ullSizeCache) + (dwModCache > 0 ? 1 : 0);
					auto ullOffsetCurr = refHexSpan.ullOffset + refHexSpan.ullSize - ullRem;
					auto ullOffsetRandCurr = 0ULL;
					while (ullChunks-- > 0) {
						const auto ullSizeToModify = (ullChunks == 1 && dwModCache > 0) ? dwModCache : ullSizeCache;
						const auto spnData = GetData({ .ullOffset { ullOffsetCurr }, .ullSize { ullSizeToModify } });
						assert(!spnData.empty());
						std::copy_n(uptrRandData.get() + ullOffsetRandCurr, ullSizeToModify, spnData.data());
						SetDataVirtual(spnData, { .ullOffset { ullOffsetCurr }, .ullSize { ullSizeToModify } });
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
			for (auto it = 0ULL; it < ulSizeBuffFastFill; it += ullSizeToFillWith) { //Fill the buffer.
				std::copy_n(hms.spnData.data(), ullSizeToFillWith, buffFillData + it);
			}
			ModifyWorker(hms, lmbRepeat, { buffFillData, ulSizeBuffFastFill }); //Worker with the big fast buffer.

			if (const auto ullRem = ullSizeToModify % ulSizeBuffFastFill; ullRem >= ullSizeToFillWith) { //Remainder.
				const auto ullOffset = ullOffsetToModify + ullSizeToModify - ullRem;
				const auto spnData = GetData({ .ullOffset { ullOffset }, .ullSize { ullRem } });
				for (std::size_t itRem = 0; itRem < (ullRem / ullSizeToFillWith); ++itRem) { //Works only if ullRem >= ullSizeToFillWith.
					std::copy_n(hms.spnData.data(), ullSizeToFillWith, spnData.data() + (itRem * ullSizeToFillWith));
				}
				SetDataVirtual(spnData, { .ullOffset { ullOffset }, .ullSize { ullRem - (ullRem % ullSizeToFillWith) } });
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

			ModifyData(hmsRepeat);
			return;
		}

	#if defined(_M_IX86) || defined(_M_X64)
		//In cases where the only one affected data region (hms.vecSpan.size()==1) is used,
		//and ullSizeToModify > ulSizeOfVec, we use SIMD.
		//At the end we simply fill up the remainder (ullSizeToModify % ulSizeOfVec).
		const auto ulSizeOfVec { ut::HasAVX2() ? 32UL : 16UL };
		const auto ullOffsetToModify = hms.vecSpan.back().ullOffset;
		const auto ullSizeToModify = hms.vecSpan.back().ullSize;
		const auto ullSizeToFillWith = hms.spnData.size();

		if (hms.vecSpan.size() == 1 && ((ullSizeToModify / ulSizeOfVec) > 0)) {
			ModifyWorker(hms, ut::HasAVX2() ? ModifyOperVec256 : ModifyOperVec128,
				{ static_cast<std::byte*>(nullptr), ulSizeOfVec }); //Worker with vector.

			if (const auto ullRem = ullSizeToModify % ulSizeOfVec; ullRem >= ullSizeToFillWith) { //Remainder of the vector data.
				const auto ullOffset = ullOffsetToModify + ullSizeToModify - ullRem;
				const auto spnData = GetData({ .ullOffset { ullOffset }, .ullSize { ullRem } });
				for (std::size_t itRem = 0; itRem < (ullRem / ullSizeToFillWith); ++itRem) { //Works only if ullRem >= ullSizeToFillWith.
					ModifyOper(spnData.data() + (itRem * ullSizeToFillWith), hms, { });
				}
				SetDataVirtual(spnData, { .ullOffset { ullOffset }, .ullSize { ullRem - (ullRem % ullSizeToFillWith) } });
			}
		}
		else {
			ModifyWorker(hms, ModifyOper, hms.spnData);
		}
	#elif defined(_M_ARM64) //^^^ _M_IX86 || _M_X64 / vvv _M_ARM64
		ModifyWorker(hms, ModifyOper, hms.spnData);
	#endif // ^^^ _M_ARM64
	}
	break;
	default:
		break;
	}
	SetRedraw(true);

	OnModifyData();
}

bool CHexCtrl::PreTranslateMsg(MSG* pMsg)
{
	if (m_pDlgBkmMgr->PreTranslateMsg(pMsg)) { return true; }
	if (m_pDlgDataInterp->PreTranslateMsg(pMsg)) { return true; }
	if (m_pDlgModify->PreTranslateMsg(pMsg)) { return true; }
	if (m_pDlgSearch->PreTranslateMsg(pMsg)) { return true; }
	if (m_pDlgCodepage->PreTranslateMsg(pMsg)) { return true; }
	if (m_pDlgGoTo->PreTranslateMsg(pMsg)) { return true; }
	if (m_pDlgTemplMgr->PreTranslateMsg(pMsg)) { return true; }

	return false;
}

auto CHexCtrl::ProcessMsg(const MSG& msg)->LRESULT
{
	switch (msg.message) {
	case WM_CHAR: return OnChar(msg);
	case WM_COMMAND: return OnCommand(msg);
	case WM_CONTEXTMENU: return OnContextMenu(msg);
	case WM_DESTROY: return OnDestroy();
	case WM_ERASEBKGND: return OnEraseBkgnd(msg);
	case WM_GETDLGCODE: return OnGetDlgCode(msg);
	case WM_HELP: return OnHelp(msg);
	case WM_HSCROLL: return OnHScroll(msg);
	case WM_INITMENUPOPUP: return OnInitMenuPopup(msg);
	case WM_KEYDOWN: return OnKeyDown(msg);
	case WM_KEYUP: return OnKeyUp(msg);
	case WM_LBUTTONDBLCLK: return OnLButtonDblClk(msg);
	case WM_LBUTTONDOWN: return OnLButtonDown(msg);
	case WM_LBUTTONUP: return OnLButtonUp(msg);
	case WM_MOUSEMOVE: return OnMouseMove(msg);
	case WM_MOUSEWHEEL: return OnMouseWheel(msg);
	case WM_NCACTIVATE: return OnNCActivate(msg);
	case WM_NCCALCSIZE: return OnNCCalcSize(msg);
	case WM_NCPAINT: return OnNCPaint(msg);
	case WM_PAINT: return OnPaint();
	case WM_SETCURSOR: return OnSetCursor(msg);
	case WM_SIZE: return OnSize(msg);
	case WM_SYSKEYDOWN: return OnSysKeyDown(msg);
	case WM_TIMER: return OnTimer(msg);
	case WM_VSCROLL: return OnVScroll(msg);
	default: return wnd::DefWndProc(msg);
	}
}

void CHexCtrl::Redraw()
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	if (IsDataSet()) {
		const auto ullCaretPos = GetVirtualOffset(GetCaretPos());
		//^ (caret) - encloses a data name, ` (tilda) - encloses the data itself.
		m_wstrInfoBar = std::vformat(ut::GetLocale(), IsOffsetAsHex() ? L"^Caret: ^`0x{:X}`" : L"^Caret: ^`{:L}`",
			std::make_wformat_args(ullCaretPos));

		if (IsPageVisible()) { //Page/Sector.
			const auto ullPagePos = GetPagePos();
			const auto ullPagesCount = GetPagesCount();
			m_wstrInfoBar += std::vformat(ut::GetLocale(), IsOffsetAsHex() ? L"^{}: ^`0x{:X}/0x{:X}`" : L"^{}: ^`{:L}/{:L}`",
				std::make_wformat_args(m_wstrPageName, ullPagePos, ullPagesCount));
		}

		if (HasSelection()) {
			const auto ullSelStart = GetVirtualOffset(m_pSelection->GetSelStart());
			const auto ullSelSize = m_pSelection->GetSelSize();
			if (ullSelSize == 1) { //In case of just one byte selected.
				m_wstrInfoBar += std::vformat(ut::GetLocale(), IsOffsetAsHex() ? L"^Selected: ^`0x{:X} [0x{:X}]`" :
					L"^Selected: ^`{} [{:L}]`", std::make_wformat_args(ullSelSize, ullSelStart));
			}
			else {
				const auto ullSelEnd = m_pSelection->GetSelEnd();
				m_wstrInfoBar += std::vformat(ut::GetLocale(), IsOffsetAsHex() ? L"^Selected: ^`0x{:X} [0x{:X}-0x{:X}]`" :
					L"^Selected: ^`{:L} [{:L}-{:L}]`", std::make_wformat_args(ullSelSize, ullSelStart, ullSelEnd));
			}
		}

		m_wstrInfoBar += IsMutable() ? L"^RW^" : L"^RO^"; //RW/RO mode.
	}
	else {
		m_wstrInfoBar.clear();
	}

	m_Wnd.RedrawWindow();
}

void CHexCtrl::SetCapacity(DWORD dwCapacity)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	//SetCapacity can be called with the current capacity size. This needs for the 
	//SetGroupSize to recalc current capacity when group size has changed.
	if (dwCapacity < 1UL || dwCapacity > 100UL) //Restrict capacity size in the [1-100] range.
		return;

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
	else if (dwCapacity > 100UL) { //100 is the maximum allowed capacity.
		dwCapacity -= dwGroupSize;
	}

	m_dwCapacity = dwCapacity;
	m_dwCapacityBlockSize = dwCapacity / 2;

	FillCapacityString();
	RecalcAll();

	//Notify only if capacity has actually changed.
	//SetCapacity is called from the SetGroupSize, and may or may not change the current capacity size.
	if (dwCurrCapacity != dwCapacity) {
		ParentNotify(HEXCTRL_MSG_SETCAPACITY);
	}
}

void CHexCtrl::SetCaretPos(ULONGLONG ullOffset, bool fHighLow, bool fRedraw)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }
	if (!IsDataSet()) { ut::DBG_REPORT_NO_DATA_SET(); return; }
	if (ullOffset >= GetDataSize()) { ut::DBG_REPORT(L"Offset is out of data range."); return; };

	m_ullCaretPos = ullOffset;
	m_fCaretHigh = fHighLow;

	if (fRedraw) {
		Redraw();
	}

	OnCaretPosChange(ullOffset);
}

void CHexCtrl::SetCharsExtraSpace(DWORD dwSpace)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	m_dwCharsExtraSpace = (std::min)(dwSpace, 10UL);
	RecalcAll();
}

void CHexCtrl::SetCodepage(int iCodepage)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	std::wstring_view wsvFmt;
	switch (iCodepage) {
	case -1:
		wsvFmt = L"ASCII";
		break;
	case 0:
		wsvFmt = L"UTF-16";
		break;
	default:
		if (CPINFOEXW stCP; ::GetCPInfoExW(static_cast<UINT>(iCodepage), 0, &stCP) != FALSE) {
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
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	m_stColors = hcs;
	m_Wnd.RedrawWindow();
}

bool CHexCtrl::SetConfig(std::wstring_view wsvPath)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return false; }

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
		{ "CMD_SEL_MARKSTARTEND", { CMD_SEL_MARKSTARTEND, IDM_HEXCTRL_SEL_MARKSTARTEND } },
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
	for (const auto& refMap : umapCmdMenu) {
		m_vecKeyBind.emplace_back(KEYBIND { .eCmd { refMap.second.first }, .wMenuID { static_cast<WORD>(refMap.second.second) } });
	}

	rapidjson::Document docJSON;
	if (wsvPath.empty()) { //Default IDR_HEXCTRL_JSON_KEYBIND.json, from resources.
		const auto hRes = ::FindResourceW(m_hInstRes, MAKEINTRESOURCEW(IDJ_HEXCTRL_KEYBIND), L"JSON");
		if (hRes == nullptr) {
			ut::DBG_REPORT(L"FindResourceW failed.");
			return false;
		}

		const auto hResData = ::LoadResource(m_hInstRes, hRes);
		if (hResData == nullptr) {
			ut::DBG_REPORT(L"LoadResource failed.");
			return false;
		}

		const auto nSize = static_cast<std::size_t>(::SizeofResource(m_hInstRes, hRes));
		const auto pData = static_cast<char*>(::LockResource(hResData));
		docJSON.Parse(pData, nSize);
		if (docJSON.IsNull()) { //Parse all default keybindings.
			ut::DBG_REPORT(L"docJSON.IsNull().");
			return false;
		}

	}
	else if (std::ifstream ifs(std::wstring { wsvPath }); ifs.is_open()) {
		rapidjson::IStreamWrapper isw { ifs };
		if (docJSON.ParseStream(isw); docJSON.IsNull()) {
			ut::DBG_REPORT(L"docJSON.IsNull().");
			return false;
		}
	}

	const auto lmbParseStr = [&](std::string_view sv)->std::optional<KEYBIND> {
		if (sv.empty())
			return { };

		KEYBIND stKB;
		const auto nSize = sv.size();
		std::size_t nPosStart { 0 }; //Next position to start search for '+' sign.
		const auto nSubWords = std::count(sv.begin(), sv.end(), '+') + 1; //How many sub-words (divided by '+')?
		for (auto itSubWords = 0; itSubWords < nSubWords; ++itSubWords) {
			const auto nPosNext = sv.find('+', nPosStart);
			const auto nSizeSubWord = nPosNext == std::string_view::npos ? nSize - nPosStart : nPosNext - nPosStart;
			const auto strSubWord = sv.substr(nPosStart, nSizeSubWord);
			nPosStart = nPosNext + 1;

			if (strSubWord.size() == 1) {
				stKB.uKey = static_cast<UCHAR>(std::toupper(strSubWord[0])); //Binding keys are in uppercase.
			}
			else if (const auto itKey = umapKeys.find(strSubWord); itKey != umapKeys.end()) {
				switch (const auto uChar = itKey->second.first; uChar) {
				case VK_CONTROL:
					stKB.fCtrl = true;
					break;
				case VK_SHIFT:
					stKB.fShift = true;
					break;
				case VK_MENU:
					stKB.fAlt = true;
					break;
				default:
					stKB.uKey = uChar;
				}
			}
		}

		return stKB;
		};

	for (auto itMembers = docJSON.MemberBegin(); itMembers != docJSON.MemberEnd(); ++itMembers) { //JSON data iterating.
		if (const auto itCmd = umapCmdMenu.find(itMembers->name.GetString()); itCmd != umapCmdMenu.end()) {
			for (auto itArrCurr = itMembers->value.Begin(); itArrCurr != itMembers->value.End(); ++itArrCurr) { //Array iterating.
				if (auto optKB = lmbParseStr(itArrCurr->GetString()); optKB) {
					optKB->eCmd = itCmd->second.first;
					optKB->wMenuID = static_cast<WORD>(itCmd->second.second);
					if (const auto itKB = std::find_if(m_vecKeyBind.begin(), m_vecKeyBind.end(),
						[&optKB](const KEYBIND& ref) { return ref.eCmd == optKB->eCmd; }); itKB != m_vecKeyBind.end()) {
						if (itKB->uKey == 0) {
							*itKB = *optKB; //Adding keybindings from JSON to m_vecKeyBind.
						}
						else {
							//If such command with some key from JSON already exist, we adding another one
							//same command but with a different key, like Ctrl+F/Ctrl+H for Search.
							m_vecKeyBind.emplace_back(*optKB);
						}
					}
				}
			}
		}
	}

	std::size_t i { 0 };
	for (const auto& refKB : m_vecKeyBind) {
		//Check for previous same menu ID. To assign only one, first, keybinding for menu name.
		//With `"ctrl+f", "ctrl+h"` in JSON, only the "Ctrl+F" will be assigned as the menu name.
		const auto itEnd = m_vecKeyBind.begin() + i++;
		if (const auto itTmp = std::find_if(m_vecKeyBind.begin(), itEnd, [&](const KEYBIND& ref) {
			return ref.wMenuID == refKB.wMenuID; });
			itTmp == itEnd && refKB.wMenuID != 0 && refKB.uKey != 0) {
			auto wstr = m_MenuMain.GetMenuWstr(refKB.wMenuID);
			if (const auto nPos = wstr.find('\t'); nPos != std::wstring::npos) {
				wstr.erase(nPos);
			}

			wstr += L'\t';
			if (refKB.fCtrl) {
				wstr += L"Ctrl+";
			}
			if (refKB.fShift) {
				wstr += L"Shift+";
			}
			if (refKB.fAlt) {
				wstr += L"Alt+";
			}

			//Search for any special key names: 'Tab', 'Enter', etc... If not found then it's just a char.
			if (const auto itUmap = std::find_if(umapKeys.begin(), umapKeys.end(), [&](const auto& ref) {
				return ref.second.first == refKB.uKey; }); itUmap != umapKeys.end()) {
				wstr += itUmap->second.second;
			}
			else {
				wstr += static_cast<unsigned char>(refKB.uKey);
			}

			//Modify menu with a new name (with shortcut appended) and old bitmap.
			MENUITEMINFOW mii { .cbSize { sizeof(MENUITEMINFOW) }, .fMask { MIIM_BITMAP | MIIM_STRING } };
			m_MenuMain.GetMenuItemInfoW(refKB.wMenuID, &mii);
			mii.dwTypeData = wstr.data();
			m_MenuMain.SetMenuItemInfoW(refKB.wMenuID, &mii);
		}
	}

	return true;
}

void CHexCtrl::SetData(const HEXDATA& hds, bool fAdjust)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }
	if (hds.spnData.empty()) { ut::DBG_REPORT(L"Data size can't be zero."); return; }

	if (fAdjust) {
		if (!IsDataSet()) {
			ut::DBG_REPORT(L"Nothing to adjust, data must be set first.");
			return;
		}

		if (hds.spnData.size() != m_spnData.size()) {
			ut::DBG_REPORT(L"Data size must be equal to the prior data size.");
			return;
		}
	}
	else { //Clear any previously set data before setting the new data.
		ClearData();
	}

	m_spnData = hds.spnData;
	m_pHexVirtData = hds.pHexVirtData;
	m_pHexVirtColors = hds.pHexVirtColors;
	m_dwCacheSize = (std::max)(hds.dwCacheSize, 1024UL * 64UL); //Minimum cache size for VirtualData mode.
	m_fMutable = hds.fMutable;
	m_fHighLatency = hds.fHighLatency;

	const auto ullDataSize = hds.pHexVirtData ? (std::max)(hds.ullMaxVirtOffset,
		static_cast<ULONGLONG>(hds.spnData.size())) : hds.spnData.size();
	if (ullDataSize <= 0xFFFFFFFFUL) {
		m_dwDigitsOffsetDec = 10;
		m_dwDigitsOffsetHex = 8;
	}
	else if (ullDataSize <= 0xFFFFFFFFFFUL) {
		m_dwDigitsOffsetDec = 13;
		m_dwDigitsOffsetHex = 10;
	}
	else if (ullDataSize <= 0xFFFFFFFFFFFFUL) {
		m_dwDigitsOffsetDec = 15;
		m_dwDigitsOffsetHex = 12;
	}
	else if (ullDataSize <= 0xFFFFFFFFFFFFFFUL) {
		m_dwDigitsOffsetDec = 17;
		m_dwDigitsOffsetHex = 14;
	}
	else {
		m_dwDigitsOffsetDec = 19;
		m_dwDigitsOffsetHex = 16;
	}

	m_fDataSet = true;

	RecalcAll();
}

void CHexCtrl::SetDateInfo(DWORD dwFormat, wchar_t wchSepar)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	//dwFormat: 0xFFFFFFFFUL = User default, 0 = MMddYYYY, 1 = ddMMYYYY, 2 = YYYYMMdd
	if (dwFormat > 2 && dwFormat != 0xFFFFFFFFUL) { ut::DBG_REPORT(L"Wrong format."); return; }

	if (dwFormat == 0xFFFFFFFFUL) {
		//Determine current user locale-specific date format.
		if (::GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_IDATE | LOCALE_RETURN_NUMBER,
			reinterpret_cast<LPWSTR>(&m_dwDateFormat), sizeof(m_dwDateFormat)) == 0) {
			ut::DBG_REPORT(L"GetLocaleInfoEx failed.");
		}
	}
	else {
		m_dwDateFormat = dwFormat;
	}

	m_wchDateSepar = wchSepar == L'\0' ? L'/' : wchSepar;
}

void CHexCtrl::SetDlgProperties(EHexWnd eWnd, std::uint64_t u64Flags)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	using enum EHexWnd;
	switch (eWnd) {
	case DLG_BKMMGR:
		m_pDlgBkmMgr->SetDlgProperties(u64Flags);
		break;
	case DLG_DATAINTERP:
		m_pDlgDataInterp->SetDlgProperties(u64Flags);
		break;
	case DLG_MODIFY:
		m_pDlgModify->SetDlgProperties(u64Flags);
		break;
	case DLG_SEARCH:
		m_pDlgSearch->SetDlgProperties(u64Flags);
		break;
	case DLG_CODEPAGE:
		m_pDlgCodepage->SetDlgProperties(u64Flags);
		break;
	case DLG_GOTO:
		m_pDlgGoTo->SetDlgProperties(u64Flags);
		break;
	case DLG_TEMPLMGR:
		m_pDlgTemplMgr->SetDlgProperties(u64Flags);
		break;
	default:
		break;
	}
}

void CHexCtrl::SetFont(const LOGFONTW& lf, bool fMain)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	if (fMain) {
		::DeleteObject(m_hFntMain);
		m_hFntMain = ::CreateFontIndirectW(&lf);
	}
	else {
		::DeleteObject(m_hFntInfoBar);
		m_hFntInfoBar = ::CreateFontIndirectW(&lf);
	}

	RecalcAll();
	ParentNotify(HEXCTRL_MSG_SETFONT);
}

void CHexCtrl::SetGroupSize(DWORD dwSize)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	if (dwSize < 1UL || dwSize > 64UL || dwSize == GetGroupSize()) //Restrict group size in the [1-64] range.
		return;

	m_dwGroupSize = dwSize;

	//Getting the "Group Data" menu pointer independent of position.
	const auto menuMain = m_MenuMain.GetSubMenu(0);
	HMENU hMenuGroupData { };
	for (auto i = 0; i < menuMain.GetMenuItemCount(); ++i) {
		//Searching through all submenus whose first menuID is IDM_HEXCTRL_GROUPDATA_BYTE.
		if (auto menuSub = menuMain.GetSubMenu(i); menuSub.IsMenu()) {
			if (menuSub.GetMenuItemID(0) == IDM_HEXCTRL_GROUPDATA_BYTE) {
				hMenuGroupData = menuSub.GetHMENU();
				break;
			}
		}
	}

	if (hMenuGroupData != nullptr) {
		//Unchecking all menus and checking only the currently selected.
		wnd::CMenu menuGroup(hMenuGroupData);
		for (auto iIDGroupData = 0; iIDGroupData < menuGroup.GetMenuItemCount(); ++iIDGroupData) {
			menuGroup.CheckMenuItem(iIDGroupData, false, false);
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

		if (uIDToCheck != 0) {
			menuGroup.CheckMenuItem(uIDToCheck, true);
		}
	}

	SetCapacity(GetCapacity()); //To recalc current representation.
	ParentNotify(HEXCTRL_MSG_SETGROUPSIZE);
}

void CHexCtrl::SetMutable(bool fMutable)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }
	if (!IsDataSet()) { ut::DBG_REPORT_NO_DATA_SET(); return; }

	m_fMutable = fMutable;
	Redraw();
}

void CHexCtrl::SetOffsetMode(bool fHex)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	m_fOffsetHex = fHex;
	FillCapacityString();
	RecalcAll();
}

void CHexCtrl::SetPageSize(DWORD dwSize, std::wstring_view wsvName)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	m_dwPageSize = dwSize;
	m_wstrPageName = wsvName;
	if (IsDataSet()) {
		Redraw();
	}
}

void CHexCtrl::SetRedraw(bool fRedraw)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	m_fRedraw = fRedraw;
}

void CHexCtrl::SetScrollRatio(float flRatio, bool fLines)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	m_flScrollRatio = flRatio;
	m_fScrollLines = fLines;
	m_pScrollV->SetScrollPageSize(GetScrollPageSize());
}

void CHexCtrl::SetSelection(const VecSpan& vecSel, bool fRedraw, bool fHighlight)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }
	if (!IsDataSet()) { ut::DBG_REPORT_NO_DATA_SET(); return; }

	m_pSelection->SetSelection(vecSel, fHighlight);

	if (fRedraw) {
		Redraw();
	}

	ParentNotify(HEXCTRL_MSG_SETSELECTION);
}

void CHexCtrl::SetUnprintableChar(wchar_t wch)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	m_wchUnprintable = wch;
	Redraw();
}

void CHexCtrl::SetVirtualBkm(IHexBookmarks* pVirtBkm)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	m_pDlgBkmMgr->SetVirtual(pVirtBkm);
}

void CHexCtrl::SetWindowPos(HWND hWndAfter, int iX, int iY, int iWidth, int iHeight, UINT uFlags)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	m_Wnd.SetWindowPos(hWndAfter, iX, iY, iWidth, iHeight, uFlags);
}

void CHexCtrl::ShowInfoBar(bool fShow)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

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

	const auto spnData = GetData({ .ullOffset { ullOffsetStart }, .ullSize { sSizeDataToPrint } });
	assert(!spnData.empty());
	assert(spnData.size() >= sSizeDataToPrint);
	const auto pDataBegin = reinterpret_cast<unsigned char*>(spnData.data()); //Pointer to data to print.
	const auto pDataEnd = pDataBegin + sSizeDataToPrint;

	//Hex Bytes to print.
	std::wstring wstrHex;
	wstrHex.reserve(sSizeDataToPrint * 2);
	for (auto itData = pDataBegin; itData < pDataEnd; ++itData) { //Converting bytes to Hexes.
		wstrHex.push_back(m_pwszHexChars[(*itData >> 4) & 0x0F]);
		wstrHex.push_back(m_pwszHexChars[*itData & 0x0F]);
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
		::MultiByteToWideChar(iCodepage, 0, reinterpret_cast<LPCCH>(pDataBegin),
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
	const auto dwCapacity = GetCapacity() > 0 ? GetCapacity() : 0xFFFFFFFFUL; //To suppress warning C4724.
	const auto ullPos = GetCaretPos() - (GetCaretPos() % dwCapacity);
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
	CHOOSEFONTW chf { .lStructSize { sizeof(CHOOSEFONTW) }, .hwndOwner { m_Wnd }, .lpLogFont { &lf },
		.Flags { CF_EFFECTS | CF_FIXEDPITCHONLY | CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_NOSIMULATIONS },
		.rgbColors { stClr.clrFontHex } };

	if (::ChooseFontW(&chf) != FALSE) {
		stClr.clrFontHex = chf.rgbColors;
		SetColors(stClr);
		SetFont(lf);
	}
}

void CHexCtrl::ClipboardCopy(EClipboard eType)const
{
	if (m_pSelection->GetSelSize() > 1024 * 1024 * 8) { //8MB
		::MessageBoxW(m_Wnd, L"Selection size is too big to copy.\r\nTry selecting less.", L"Error", MB_ICONERROR);
		return;
	}

	std::wstring wstrData;
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
	const std::size_t sMemSize = (wstrData.size() * sCharSize) + sCharSize;
	const auto hMem = ::GlobalAlloc(GMEM_MOVEABLE, sMemSize);
	if (!hMem) {
		ut::DBG_REPORT(L"GlobalAlloc error.");
		return;
	}

	const auto lpMemLock = ::GlobalLock(hMem);
	if (!lpMemLock) {
		ut::DBG_REPORT(L"GlobalLock error.");
		return;
	}

	std::memcpy(lpMemLock, wstrData.data(), sMemSize);
	::GlobalUnlock(hMem);
	::OpenClipboard(m_Wnd);
	::EmptyClipboard();
	::SetClipboardData(CF_UNICODETEXT, hMem);
	::CloseClipboard();
}

void CHexCtrl::ClipboardPaste(EClipboard eType)
{
	if (!IsMutable() || !::OpenClipboard(m_Wnd))
		return;

	const auto hClpbrd = ::GetClipboardData(CF_UNICODETEXT);
	if (!hClpbrd)
		return;

	const auto pDataClpbrd = static_cast<wchar_t*>(::GlobalLock(hClpbrd));
	if (pDataClpbrd == nullptr) {
		::CloseClipboard();
		return;
	}

	const auto sSizeClpbrd = wcslen(pDataClpbrd) * sizeof(wchar_t);
	const auto ullDataSize = GetDataSize();
	const auto ullCaretPos = GetCaretPos();
	HEXMODIFY hmd;
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
		strDataModify = ut::WstrToStr(pDataClpbrd, iCodepage);
		ullSizeModify = strDataModify.size();
		if (ullCaretPos + ullSizeModify > ullDataSize) {
			ullSizeModify = ullDataSize - ullCaretPos;
		}
		hmd.spnData = { reinterpret_cast<const std::byte*>(strDataModify.data()), static_cast<std::size_t>(ullSizeModify) };
	}
	break;
	case EClipboard::PASTE_HEX:
	{
		auto optData = ut::NumStrToHex(pDataClpbrd);
		if (!optData) {
			::GlobalUnlock(hClpbrd);
			::CloseClipboard();
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

	::GlobalUnlock(hClpbrd);
	::CloseClipboard();
	m_Wnd.RedrawWindow();
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
		uValA = (uValA << 8) + ut::GetIHexTData<BYTE>(*this, m_pSelection->GetOffsetByIndex(i));
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
	wstrData.reserve((static_cast<std::size_t>(ullSelSize) * 3) + 64);
	wstrData = std::format(L"unsigned char data[{}] = {{\r\n", ullSelSize);

	for (auto i { 0U }; i < ullSelSize; ++i) {
		wstrData += L"0x";
		const auto chByte = ut::GetIHexTData<BYTE>(*this, m_pSelection->GetOffsetByIndex(i));
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
		const auto chByte = ut::GetIHexTData<BYTE>(*this, m_pSelection->GetOffsetByIndex(i));
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
		const auto chByte = ut::GetIHexTData<BYTE>(*this, m_pSelection->GetOffsetByIndex(i));
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
	if (m_pSelection->HasContiguousSel()) {
		auto dwTail = m_pSelection->GetLineLength();
		for (auto i { 0U }; i < ullSelSize; ++i) {
			const auto chByte = ut::GetIHexTData<BYTE>(*this, m_pSelection->GetOffsetByIndex(i));
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
			DWORD dwCount = (dwModStart * 2) + (dwModStart / dwGroupSize);
			//Additional spaces between halves. Only in 1 byte grouping size.
			dwCount += (dwGroupSize == 1) ? (dwTail <= m_dwCapacityBlockSize ? 2 : 0) : 0;
			wstrData.insert(0, static_cast<std::size_t>(dwCount), ' ');
		}

		for (auto i { 0U }; i < ullSelSize; ++i) {
			const auto chByte = ut::GetIHexTData<BYTE>(*this, m_pSelection->GetOffsetByIndex(i));
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
		const auto chByte = ut::GetIHexTData<BYTE>(*this, m_pSelection->GetOffsetByIndex(i - 1));
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
	if (!m_pSelection->HasContiguousSel()) //Only works with contiguous selection.
		return { };

	const auto ullSelStart = m_pSelection->GetSelStart();
	const auto ullSelSize = m_pSelection->GetSelSize();
	const auto dwCapacity = GetCapacity();

	std::wstring wstrRet;
	wstrRet.reserve(static_cast<std::size_t>(ullSelSize) * 4);
	wstrRet = L"Offset";
	wstrRet.insert(0, (static_cast<std::size_t>(GetDigitsOffset()) - wstrRet.size()) / 2, ' ');
	wstrRet.insert(wstrRet.size(), static_cast<std::size_t>(GetDigitsOffset()) - wstrRet.size(), ' ');
	wstrRet += L"   "; //Spaces to Capacity.
	wstrRet += m_wstrCapacity;
	wstrRet += L"   "; //Spaces to Text.
	if (const auto iSize = static_cast<int>(dwCapacity) - static_cast<int>(m_wstrTextTitle.size()); iSize > 0) {
		wstrRet.insert(wstrRet.size(), static_cast<std::size_t>(iSize / 2), ' ');
	}
	wstrRet += m_wstrTextTitle;
	wstrRet += L"\r\n";

	//How many spaces to insert at the beginning.
	DWORD dwModStart = ullSelStart % dwCapacity;
	const auto ullLines = GetSelectedLines();
	const auto ullStartLine = ullSelStart / dwCapacity;
	const auto dwStartOffset = dwModStart; //Offset from the line start in the wstrHex.
	const auto& [wstrHex, wstrText] = BuildDataToDraw(ullStartLine, static_cast<int>(ullLines));
	std::wstring wstrDataText;
	std::size_t sIndexToPrint { 0 };

	for (auto itLine { 0U }; itLine < ullLines; ++itLine) {
		wstrRet += OffsetToWstr(ullStartLine * dwCapacity + dwCapacity * itLine);
		wstrRet.insert(wstrRet.size(), 3, ' ');

		for (auto itChunk { 0U }; itChunk < dwCapacity; ++itChunk) {
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
			if (((itChunk + 1) % GetGroupSize()) == 0 && itChunk < (dwCapacity - 1)) {
				wstrRet += L" ";
			}

			//Additional space between capacity halves, only in 1 byte grouping size.
			if (GetGroupSize() == 1 && itChunk == (m_dwCapacityBlockSize - 1)) {
				wstrRet += L"  ";
			}
		}
		wstrRet += L"   "; //Text beginning.
		wstrRet += wstrDataText;
		if (itLine < ullLines - 1) {
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
		strData.push_back(ut::GetIHexTData<BYTE>(*this, m_pSelection->GetOffsetByIndex(i)));
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
		wstrText = ut::StrToWstr(strData, iCodepage);
	}
	ReplaceUnprintable(wstrText, iCodepage == -1, false);

	return wstrText;
}

void CHexCtrl::DrawWindow(HDC hDC)const
{
	const auto iScrollH = static_cast<int>(m_pScrollH->GetScrollPos());
	wnd::CRect rcWnd(m_iFirstVertLinePx, m_iFirstHorzLinePx,
		m_iFirstVertLinePx + m_iWidthClientAreaPx, m_iFirstHorzLinePx + m_iHeightClientAreaPx);

	wnd::CDC dc(hDC);
	dc.FillSolidRect(rcWnd, m_stColors.clrBk);
	dc.SelectObject(m_hPenLines);

	//First horizontal line.
	dc.MoveTo(m_iFirstVertLinePx - iScrollH, m_iFirstHorzLinePx);
	dc.LineTo(m_iFourthVertLinePx, m_iFirstHorzLinePx);

	//Second horizontal line.
	dc.MoveTo(m_iFirstVertLinePx - iScrollH, m_iSecondHorzLinePx);
	dc.LineTo(m_iFourthVertLinePx, m_iSecondHorzLinePx);

	//Third horizontal line.
	dc.MoveTo(m_iFirstVertLinePx - iScrollH, m_iThirdHorzLinePx);
	dc.LineTo(m_iFourthVertLinePx, m_iThirdHorzLinePx);

	//Fourth horizontal line.
	dc.MoveTo(m_iFirstVertLinePx - iScrollH, m_iFourthHorzLinePx);
	dc.LineTo(m_iFourthVertLinePx, m_iFourthHorzLinePx);

	//First Vertical line.
	dc.MoveTo(m_iFirstVertLinePx - iScrollH, m_iFirstHorzLinePx);
	dc.LineTo(m_iFirstVertLinePx - iScrollH, m_iFourthHorzLinePx);

	//Second Vertical line.
	dc.MoveTo(m_iSecondVertLinePx - iScrollH, m_iFirstHorzLinePx);
	dc.LineTo(m_iSecondVertLinePx - iScrollH, m_iThirdHorzLinePx);

	//Third Vertical line.
	dc.MoveTo(m_iThirdVertLinePx - iScrollH, m_iFirstHorzLinePx);
	dc.LineTo(m_iThirdVertLinePx - iScrollH, m_iThirdHorzLinePx);

	//Fourth Vertical line.
	dc.MoveTo(m_iFourthVertLinePx - iScrollH, m_iFirstHorzLinePx);
	dc.LineTo(m_iFourthVertLinePx - iScrollH, m_iFourthHorzLinePx);

	//«Offset» text.
	wnd::CRect rcOffset(m_iFirstVertLinePx - iScrollH, m_iFirstHorzLinePx, m_iSecondVertLinePx - iScrollH, m_iSecondHorzLinePx);
	dc.SelectObject(m_hFntMain);
	dc.SetTextColor(m_stColors.clrFontCaption);
	dc.SetBkColor(m_stColors.clrBk);
	dc.DrawTextW(L"Offset", rcOffset, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	//Capacity numbers.
	::ExtTextOutW(dc, m_iIndentFirstHexChunkXPx - iScrollH, m_iFirstHorzLinePx + m_iIndentCapTextYPx, 0, nullptr,
		m_wstrCapacity.data(), static_cast<UINT>(m_wstrCapacity.size()), GetCharsWidthArray());

	//Text area caption.
	auto rcText = GetRectTextCaption();
	dc.DrawTextW(m_wstrTextTitle, rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void CHexCtrl::DrawInfoBar(HDC hDC)const
{
	if (!HasInfoBar())
		return;

	struct POLYINFODATA { //InfoBar text, colors, and vertical lines.
		POLYTEXTW stPoly { };
		COLORREF  clrText { };
		int       iVertLineX { };
	};
	std::vector<POLYINFODATA> vecInfoData;
	vecInfoData.reserve(4);

	const auto iScrollH = static_cast<int>(m_pScrollH->GetScrollPos());
	wnd::CRect rcInfoBar(m_iFirstVertLinePx + 1 - iScrollH, m_iThirdHorzLinePx + 1,
		m_iFourthVertLinePx, m_iFourthHorzLinePx); //Info bar rc until m_iFourthHorizLine.
	wnd::CRect rcInfoBarText = rcInfoBar;
	rcInfoBarText.left = m_iFirstVertLinePx + 5; //Draw the text beginning with little indent.
	rcInfoBarText.right = m_iFirstVertLinePx + m_iWidthClientAreaPx; //Draw text to the end of the client area, even if it passes iFourthHorizLine.

	std::size_t sCurrPosBegin { };
	while (true) {
		const auto sParamPosBegin = m_wstrInfoBar.find_first_of('^', sCurrPosBegin);
		if (sParamPosBegin == std::wstring::npos)
			break;

		const auto sParamPosEnd = m_wstrInfoBar.find_first_of('^', sParamPosBegin + 1);
		if (sParamPosEnd == std::wstring::npos)
			break;

		const auto iParamSize = static_cast<UINT>(sParamPosEnd - sParamPosBegin - 1);
		vecInfoData.emplace_back(POLYTEXTW { .n { iParamSize }, .lpstr { m_wstrInfoBar.data() + sParamPosBegin + 1 },
			.rcl { rcInfoBarText } }, m_stColors.clrFontInfoParam);
		rcInfoBarText.left += iParamSize * m_sizeFontInfo.cx; //Increase rect left offset by string size.
		sCurrPosBegin = sParamPosEnd + 1;

		if (const auto sDataPosBegin = m_wstrInfoBar.find_first_of('`', sCurrPosBegin);
			sDataPosBegin != std::wstring::npos) {
			if (const auto sDataPosEnd = m_wstrInfoBar.find_first_of('`', sDataPosBegin + 1);
				sDataPosEnd != std::wstring::npos) {
				const auto iDataSize = static_cast<UINT>(sDataPosEnd - sDataPosBegin - 1);
				vecInfoData.emplace_back(POLYTEXTW { .n { iDataSize }, .lpstr { m_wstrInfoBar.data() + sDataPosBegin + 1 },
					.rcl { rcInfoBarText } }, m_stColors.clrFontInfoData);
				rcInfoBarText.left += iDataSize * m_sizeFontInfo.cx;
				sCurrPosBegin = sDataPosEnd + 1;
			}
		}

		rcInfoBarText.left += m_sizeFontInfo.cx; //Additional space to the next rect's left side.
		vecInfoData.back().iVertLineX = rcInfoBarText.left - (m_sizeFontInfo.cx / 2); //Vertical line after current rect.
	}

	wnd::CDC dc(hDC);
	dc.FillSolidRect(rcInfoBar, m_stColors.clrBkInfoBar); //Info bar rect.
	dc.DrawEdge(rcInfoBar, BDR_RAISEDINNER, BF_TOP);
	dc.SelectObject(m_hFntInfoBar);
	dc.SetBkColor(m_stColors.clrBkInfoBar);

	for (auto& ref : vecInfoData) {
		dc.SetTextColor(ref.clrText);
		auto& refPoly = ref.stPoly;
		dc.DrawTextW(refPoly.lpstr, refPoly.n, &refPoly.rcl, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

		if (ref.iVertLineX > 0) {
			dc.MoveTo(ref.iVertLineX, refPoly.rcl.top);
			dc.LineTo(ref.iVertLineX, refPoly.rcl.bottom);
		}
	}
}

void CHexCtrl::DrawOffsets(HDC hDC, ULONGLONG ullStartLine, int iLines)const
{
	const auto dwCapacity = GetCapacity();
	const auto ullStartOffset = ullStartLine * dwCapacity;
	const auto iScrollH = static_cast<int>(m_pScrollH->GetScrollPos());

	for (auto itLine = 0; itLine < iLines; ++itLine) {
		//Drawing offset with bk color depending on the selection range.
		HEXCOLOR stClrOffset;
		if (m_pSelection->HitTestRange({ ullStartOffset + (itLine * dwCapacity), dwCapacity })) {
			stClrOffset.clrBk = m_stColors.clrBkSel;
			stClrOffset.clrText = m_stColors.clrFontSel;
		}
		else {
			stClrOffset.clrBk = m_stColors.clrBk;
			stClrOffset.clrText = m_stColors.clrFontCaption;
		}

		//Left column offset printing (00000000...0000FFFF).
		wnd::CDC dc(hDC);
		dc.SelectObject(m_hFntMain);
		dc.SetTextColor(stClrOffset.clrText);
		dc.SetBkColor(stClrOffset.clrBk);
		::ExtTextOutW(dc, m_iFirstVertLinePx + GetCharWidthNative() - iScrollH,
			m_iStartWorkAreaYPx + (m_sizeFontMain.cy * itLine), 0, nullptr,
			OffsetToWstr((ullStartLine + itLine) * dwCapacity).data(), GetDigitsOffset(), nullptr);
	}
}

void CHexCtrl::DrawHexText(HDC hDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const
{
	struct POLYTEXTCLR {
		POLYTEXTW stPoly { };
		HEXCOLOR  stClr;
	};

	std::vector<POLYTEXTCLR> vecPolyHex;
	std::vector<POLYTEXTCLR> vecPolyText;
	std::vector<std::unique_ptr<std::wstring>> vecWstrHex; //unique_ptr to avoid wstring.data() invalidation.
	std::vector<std::unique_ptr<std::wstring>> vecWstrText;
	vecWstrHex.reserve(static_cast<std::size_t>(iLines));
	vecWstrText.reserve(static_cast<std::size_t>(iLines));
	const auto ullStartOffset = ullStartLine * GetCapacity();
	std::size_t sIndexToPrint { 0 };
	HEXCOLORINFO hci { .hdr { m_Wnd, static_cast<UINT>(m_Wnd.GetDlgCtrlID()) },
		.stClr { .clrBk { m_stColors.clrBk }, .clrText { m_stColors.clrFontHex } } };

	for (auto itLine = 0; itLine < iLines; ++itLine) {
		std::wstring wstrHexToPrint;
		std::wstring wstrTextToPrint;
		int iHexPosToPrintX { };
		int iTextPosToPrintX { };
		bool fNeedChunkPoint { true }; //For just one time exec.
		const auto iPosToPrintY = m_iStartWorkAreaYPx + (m_sizeFontMain.cy * itLine); //Hex and Text are the same.
		HEXCOLOR stHexClr { }; //Current Hex area color.
		HEXCOLOR stTextClr { .clrText { m_stColors.clrFontText } }; //Current Text area color.
		const auto lmbPoly = [&]() {
			if (wstrHexToPrint.empty())
				return;

			//Hex colors Poly.
			vecWstrHex.emplace_back(std::make_unique<std::wstring>(std::move(wstrHexToPrint)));
			vecPolyHex.emplace_back(POLYTEXTW { .x { iHexPosToPrintX }, .y { iPosToPrintY },
				.n { static_cast<UINT>(vecWstrHex.back()->size()) }, .lpstr { vecWstrHex.back()->data() },
				.pdx { GetCharsWidthArray() } }, stHexClr);

			//Text colors Poly.
			vecWstrText.emplace_back(std::make_unique<std::wstring>(std::move(wstrTextToPrint)));
			vecPolyText.emplace_back(POLYTEXTW { .x { iTextPosToPrintX }, .y { iPosToPrintY },
				.n { static_cast<UINT>(vecWstrText.back()->size()) }, .lpstr { vecWstrText.back()->data() },
				.pdx { GetCharsWidthArray() } }, stTextClr);
			};
		const auto lmbHexSpaces = [&](const unsigned itChunk) {
			if (wstrHexToPrint.empty()) //Only adding spaces if there are chars beforehead.
				return;

			if ((itChunk % GetGroupSize()) == 0) {
				wstrHexToPrint += L' ';
			}

			//Additional space between capacity halves, only in 1 byte grouping size.
			if (GetGroupSize() == 1 && itChunk == m_dwCapacityBlockSize) {
				wstrHexToPrint += L"  ";
			}
			};

		//Main loop for printing Hex chunks and Text chars.
		for (auto itChunk { 0U }; itChunk < GetCapacity() && sIndexToPrint < wsvText.size(); ++itChunk, ++sIndexToPrint) {
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
				lmbHexSpaces(itChunk);
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

			lmbHexSpaces(itChunk);
			wstrHexToPrint += wsvHex[sIndexToPrint * 2];
			wstrHexToPrint += wsvHex[(sIndexToPrint * 2) + 1];
			wstrTextToPrint += wsvText[sIndexToPrint];
		}

		lmbPoly();
	}

	wnd::CDC dc(hDC);
	dc.SelectObject(m_hFntMain);
	std::size_t index { 0 }; //Index for vecPolyText, its size is always equal to vecPolyHex.
	for (const auto& ref : vecPolyHex) { //Loop is needed because of different colors.
		dc.SetTextColor(ref.stClr.clrText);
		dc.SetBkColor(ref.stClr.clrBk);
		const auto& refH = ref.stPoly;
		::ExtTextOutW(dc, refH.x, refH.y, refH.uiFlags, &refH.rcl, refH.lpstr, refH.n, refH.pdx); //Hex printing.
		const auto& refVecText = vecPolyText[index++];
		dc.SetTextColor(refVecText.stClr.clrText); //Text color for the Text area.
		const auto& refT = refVecText.stPoly;
		::ExtTextOutW(dc, refT.x, refT.y, refT.uiFlags, &refT.rcl, refT.lpstr, refT.n, refT.pdx); //Text printing.
	}
}

void CHexCtrl::DrawTemplates(HDC hDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const
{
	if (!m_pDlgTemplMgr->HasApplied())
		return;

	struct POLYFIELDSCLR { //Struct for fields.
		POLYTEXTW stPoly { };
		HEXCOLOR  stClr;
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

	for (auto itLine = 0; itLine < iLines; ++itLine) {
		std::wstring wstrHexFieldToPrint;
		std::wstring wstrTextFieldToPrint;
		int iFieldHexPosToPrintX { };
		int iFieldTextPosToPrintX { };
		bool fNeedChunkPoint { true };
		bool fField { false };  //Flag to show current Field in current Hex presence.
		bool fPrintVertLine { true };
		const auto iPosToPrintY = m_iStartWorkAreaYPx + (m_sizeFontMain.cy * itLine); //Hex and Text are the same.
		const auto lmbPoly = [&]() {
			if (wstrHexFieldToPrint.empty())
				return;

			//Hex Fields Poly.
			vecWstrFieldsHex.emplace_back(std::make_unique<std::wstring>(std::move(wstrHexFieldToPrint)));
			vecFieldsHex.emplace_back(POLYTEXTW { .x { iFieldHexPosToPrintX }, .y { iPosToPrintY },
				.n { static_cast<UINT>(vecWstrFieldsHex.back()->size()) }, .lpstr { vecWstrFieldsHex.back()->data() },
				.pdx { GetCharsWidthArray() } }, pFieldCurr->stClr, fPrintVertLine);

			//Text Fields Poly.
			vecWstrFieldsText.emplace_back(std::make_unique<std::wstring>(std::move(wstrTextFieldToPrint)));
			vecFieldsText.emplace_back(POLYTEXTW { .x { iFieldTextPosToPrintX }, .y { iPosToPrintY },
				.n { static_cast<UINT>(vecWstrFieldsText.back()->size()) }, .lpstr { vecWstrFieldsText.back()->data() },
				.pdx { GetCharsWidthArray() } }, pFieldCurr->stClr, fPrintVertLine);

			fPrintVertLine = true;
			};
		const auto lmbHexSpaces = [&](const unsigned itChunk) {
			if (wstrHexFieldToPrint.empty()) //Only adding spaces if there are chars beforehead.
				return;

			if ((itChunk % GetGroupSize()) == 0) {
				wstrHexFieldToPrint += L' ';
			}

			//Additional space between capacity halves, only in 1 byte grouping size.
			if (GetGroupSize() == 1 && itChunk == m_dwCapacityBlockSize) {
				wstrHexFieldToPrint += L"  ";
			}
			};

		//Main loop for printing Hex chunks and Text chars.
		for (auto itChunk { 0U }; itChunk < GetCapacity() && sIndexToPrint < wsvText.size(); ++itChunk, ++sIndexToPrint) {
			//Fields.
			if (auto pField = m_pDlgTemplMgr->HitTest(ullStartOffset + sIndexToPrint); pField != nullptr) {
				if (itChunk == 0 && pField == pFieldCurr) {
					fPrintVertLine = false;
				}

				//If it's nested Field.
				if (pFieldCurr != nullptr && pFieldCurr != pField) {
					lmbHexSpaces(itChunk);
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

				lmbHexSpaces(itChunk);
				wstrHexFieldToPrint += wsvHex[sIndexToPrint * 2];
				wstrHexFieldToPrint += wsvHex[(sIndexToPrint * 2) + 1];
				wstrTextFieldToPrint += wsvText[sIndexToPrint];
				fField = true;
			}
			else if (fField) {
				//There can be multiple Fields in one line. 
				//So, if there already were Field bytes in the current line, we Poly them.
				//Same Poly mechanism presents at the end of the current (itLine) loop,
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
		wnd::CDC dc(hDC);
		dc.SelectObject(m_hFntMain);
		std::size_t index { 0 }; //Index for vecFieldsText, its size is always equal to vecFieldsHex.
		const auto penOld = dc.SelectObject(m_hPenDataTempl);
		for (const auto& ref : vecFieldsHex) { //Loop is needed because different Fields can have different colors.
			dc.SetTextColor(ref.stClr.clrText);
			dc.SetBkColor(ref.stClr.clrBk);

			const auto& refH = ref.stPoly;
			::ExtTextOutW(dc, refH.x, refH.y, refH.uiFlags, &refH.rcl, refH.lpstr, refH.n, refH.pdx); //Hex Field printing.
			const auto& refT = vecFieldsText[index++].stPoly;
			::ExtTextOutW(dc, refT.x, refT.y, refT.uiFlags, &refT.rcl, refT.lpstr, refT.n, refT.pdx); //Text Field printing.

			if (ref.fPrintVertLine) {
				const auto iFieldLineHexX = refH.x - 2; //Little indent before vert line.
				dc.MoveTo(iFieldLineHexX, refH.y);
				dc.LineTo(iFieldLineHexX, refH.y + m_sizeFontMain.cy);

				const auto iFieldLineTextX = refT.x;
				dc.MoveTo(iFieldLineTextX, refT.y);
				dc.LineTo(iFieldLineTextX, refT.y + m_sizeFontMain.cy);
			}

			if (index == vecFieldsHex.size()) { //Last vertical Field line.
				const auto iFieldLastLineHexX = refH.x + refH.n * GetCharWidthExtras() + 1; //Little indent after vert line.
				dc.MoveTo(iFieldLastLineHexX, refH.y);
				dc.LineTo(iFieldLastLineHexX, refH.y + m_sizeFontMain.cy);

				const auto iFieldLastLineTextX = refT.x + refT.n * GetCharWidthExtras();
				dc.MoveTo(iFieldLastLineTextX, refT.y);
				dc.LineTo(iFieldLastLineTextX, refT.y + m_sizeFontMain.cy);
			}
		}
		dc.SelectObject(penOld);
	}
}

void CHexCtrl::DrawBookmarks(HDC hDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const
{
	if (!m_pDlgBkmMgr->HasBookmarks())
		return;

	struct POLYTEXTCLR { //Struct for Bookmarks.
		POLYTEXTW stPoly { };
		HEXCOLOR  stClr;
	};

	std::vector<POLYTEXTCLR> vecBkmHex;
	std::vector<POLYTEXTCLR> vecBkmText;
	std::vector<std::unique_ptr<std::wstring>> vecWstrBkmHex; //unique_ptr to avoid wstring ptr invalidation.
	std::vector<std::unique_ptr<std::wstring>> vecWstrBkmText;
	const auto ullStartOffset = ullStartLine * GetCapacity();
	std::size_t sIndexToPrint { };

	for (auto itLine = 0; itLine < iLines; ++itLine) {
		std::wstring wstrHexBkmToPrint;
		std::wstring wstrTextBkmToPrint;
		int iBkmHexPosToPrintX { };
		int iBkmTextPosToPrintX { };
		bool fNeedChunkPoint { true };
		bool fBookmark { false };  //Flag to show current Bookmark in current Hex presence.
		PHEXBKM pBkmCurr { };
		const auto iPosToPrintY = m_iStartWorkAreaYPx + (m_sizeFontMain.cy * itLine); //Hex and Text are the same.
		const auto lmbPoly = [&]() {
			if (wstrHexBkmToPrint.empty())
				return;

			//Hex bookmarks Poly.
			vecWstrBkmHex.emplace_back(std::make_unique<std::wstring>(std::move(wstrHexBkmToPrint)));
			vecBkmHex.emplace_back(POLYTEXTW { .x { iBkmHexPosToPrintX }, .y { iPosToPrintY },
				.n { static_cast<UINT>(vecWstrBkmHex.back()->size()) }, .lpstr { vecWstrBkmHex.back()->data() },
				.pdx { GetCharsWidthArray() } }, pBkmCurr->stClr);

			//Text bookmarks Poly.
			vecWstrBkmText.emplace_back(std::make_unique<std::wstring>(std::move(wstrTextBkmToPrint)));
			vecBkmText.emplace_back(POLYTEXTW { .x { iBkmTextPosToPrintX }, .y { iPosToPrintY },
				.n { static_cast<UINT>(vecWstrBkmText.back()->size()) }, .lpstr { vecWstrBkmText.back()->data() },
				.pdx { GetCharsWidthArray() } }, pBkmCurr->stClr);
			};
		const auto lmbHexSpaces = [&](const unsigned itChunk) {
			if (wstrHexBkmToPrint.empty()) //Only adding spaces if there are chars beforehead.
				return;

			if ((itChunk % GetGroupSize()) == 0) {
				wstrHexBkmToPrint += L' ';
			}

			//Additional space between capacity halves, only in 1 byte grouping size.
			if (GetGroupSize() == 1 && itChunk == m_dwCapacityBlockSize) {
				wstrHexBkmToPrint += L"  ";
			}
			};

		//Main loop for printing Hex chunks and Text chars.
		for (auto itChunk { 0U }; itChunk < GetCapacity() && sIndexToPrint < wsvText.size(); ++itChunk, ++sIndexToPrint) {
			//Bookmarks.
			if (const auto pBkm = m_pDlgBkmMgr->HitTest(ullStartOffset + sIndexToPrint); pBkm != nullptr) {
				//If it's nested bookmark.
				if (pBkmCurr != nullptr && pBkmCurr != pBkm) {
					lmbHexSpaces(itChunk);
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

				lmbHexSpaces(itChunk);
				wstrHexBkmToPrint += wsvHex[sIndexToPrint * 2];
				wstrHexBkmToPrint += wsvHex[(sIndexToPrint * 2) + 1];
				wstrTextBkmToPrint += wsvText[sIndexToPrint];
				fBookmark = true;
			}
			else if (fBookmark) {
				//There can be multiple bookmarks in one line. 
				//So, if there already were bookmarked bytes in the current line, we Poly them.
				//Same Poly mechanism presents at the end of the current (itLine) loop,
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
		wnd::CDC dc(hDC);
		dc.SelectObject(m_hFntMain);
		std::size_t index { 0 }; //Index for vecBkmText, its size is always equal to vecBkmHex.
		for (const auto& ref : vecBkmHex) { //Loop is needed because bkms have different colors.
			dc.SetTextColor(ref.stClr.clrText);
			dc.SetBkColor(ref.stClr.clrBk);
			const auto& refH = ref.stPoly;
			::ExtTextOutW(dc, refH.x, refH.y, refH.uiFlags, &refH.rcl, refH.lpstr, refH.n, refH.pdx); //Hex bookmarks printing.
			const auto& refT = vecBkmText[index++].stPoly;
			::ExtTextOutW(dc, refT.x, refT.y, refT.uiFlags, &refT.rcl, refT.lpstr, refT.n, refT.pdx); //Text bookmarks printing.
		}
	}
}

void CHexCtrl::DrawSelection(HDC hDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const
{
	if (!HasSelection())
		return;

	std::vector<POLYTEXTW> vecPolySelHex;
	std::vector<POLYTEXTW> vecPolySelText;
	std::vector<std::unique_ptr<std::wstring>> vecWstrSel; //unique_ptr to avoid wstring ptr invalidation.
	const auto ullStartOffset = ullStartLine * GetCapacity();
	std::size_t sIndexToPrint { };

	for (auto itLine = 0; itLine < iLines; ++itLine) {
		std::wstring wstrHexSelToPrint; //Selected Hex and Text strings to print.
		std::wstring wstrTextSelToPrint;
		int iSelHexPosToPrintX { };
		int iSelTextPosToPrintX { };
		bool fNeedChunkPoint { true };
		bool fSelection { false };
		const auto iPosToPrintY = m_iStartWorkAreaYPx + (m_sizeFontMain.cy * itLine); //Hex and Text are the same.
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
		for (auto itChunk { 0U }; itChunk < GetCapacity() && sIndexToPrint < wsvText.size(); ++itChunk, ++sIndexToPrint) {
			//Selection.
			if (m_pSelection->HitTest(ullStartOffset + sIndexToPrint)) {
				if (fNeedChunkPoint) {
					int iCy;
					HexChunkPoint(sIndexToPrint, iSelHexPosToPrintX, iCy);
					TextChunkPoint(sIndexToPrint, iSelTextPosToPrintX, iCy);
					fNeedChunkPoint = false;
				}

				if (!wstrHexSelToPrint.empty()) { //Only adding spaces if there are chars beforehead.
					if ((itChunk % GetGroupSize()) == 0) {
						wstrHexSelToPrint += L' ';
					}

					//Additional space between capacity halves, only in 1 byte grouping size.
					if (GetGroupSize() == 1 && itChunk == m_dwCapacityBlockSize) {
						wstrHexSelToPrint += L"  ";
					}
				}
				wstrHexSelToPrint += wsvHex[sIndexToPrint * 2];
				wstrHexSelToPrint += wsvHex[(sIndexToPrint * 2) + 1];
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
		wnd::CDC dc(hDC);
		dc.SelectObject(m_hFntMain);
		dc.SetTextColor(m_stColors.clrFontSel);
		dc.SetBkColor(m_stColors.clrBkSel);
		::PolyTextOutW(dc, vecPolySelHex.data(), static_cast<UINT>(vecPolySelHex.size())); //Hex selection printing.
		for (const auto& ref : vecPolySelText) {
			::ExtTextOutW(dc, ref.x, ref.y, ref.uiFlags, &ref.rcl, ref.lpstr, ref.n, ref.pdx); //Text selection printing.
		}
	}
}

void CHexCtrl::DrawSelHighlight(HDC hDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const
{
	if (!m_pSelection->HasSelHighlight())
		return;

	std::vector<POLYTEXTW> vecPolySelHexHgl;
	std::vector<POLYTEXTW> vecPolySelTextHgl;
	std::vector<std::unique_ptr<std::wstring>> vecWstrSelHgl; //unique_ptr to avoid wstring ptr invalidation.
	const auto ullStartOffset = ullStartLine * GetCapacity();
	std::size_t sIndexToPrint { };

	for (auto itLine = 0; itLine < iLines; ++itLine) {
		std::wstring wstrHexSelToPrint; //Selected Hex and Text strings to print.
		std::wstring wstrTextSelToPrint;
		int iSelHexPosToPrintX { };
		int iSelTextPosToPrintX { };
		bool fNeedChunkPoint { true };
		bool fSelection { false };
		const auto iPosToPrintY = m_iStartWorkAreaYPx + (m_sizeFontMain.cy * itLine); //Hex and Text are the same.
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
		for (auto itChunk { 0U }; itChunk < GetCapacity() && sIndexToPrint < wsvText.size(); ++itChunk, ++sIndexToPrint) {
			//Selection highlights.
			if (m_pSelection->HitTestHighlight(ullStartOffset + sIndexToPrint)) {
				if (fNeedChunkPoint) {
					int iCy;
					HexChunkPoint(sIndexToPrint, iSelHexPosToPrintX, iCy);
					TextChunkPoint(sIndexToPrint, iSelTextPosToPrintX, iCy);
					fNeedChunkPoint = false;
				}

				if (!wstrHexSelToPrint.empty()) { //Only adding spaces if there are chars beforehead.
					if ((itChunk % GetGroupSize()) == 0) {
						wstrHexSelToPrint += L' ';
					}

					//Additional space between capacity halves, only in 1 byte grouping size.
					if (GetGroupSize() == 1 && itChunk == m_dwCapacityBlockSize) {
						wstrHexSelToPrint += L"  ";
					}
				}
				wstrHexSelToPrint += wsvHex[sIndexToPrint * 2];
				wstrHexSelToPrint += wsvHex[(sIndexToPrint * 2) + 1];
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
		wnd::CDC dc(hDC);
		dc.SelectObject(m_hFntMain);
		dc.SetTextColor(m_stColors.clrBkSel);
		dc.SetBkColor(m_stColors.clrFontSel);
		::PolyTextOutW(dc, vecPolySelHexHgl.data(), static_cast<UINT>(vecPolySelHexHgl.size())); //Hex selection highlight printing.
		for (const auto& ref : vecPolySelTextHgl) {
			::ExtTextOutW(dc, ref.x, ref.y, ref.uiFlags, &ref.rcl, ref.lpstr, ref.n, ref.pdx); //Text selection highlight printing.
		}
	}
}

void CHexCtrl::DrawCaret(HDC hDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const
{
	const auto ullCaretPos = GetCaretPos();
	const auto dwCapacity = GetCapacity();
	const auto ullFirstOffset = ullStartLine * dwCapacity;
	const auto ullEndOffset = ullFirstOffset + (iLines * dwCapacity);

	//Check if caret is within the drawing range.
	if (ullCaretPos < ullFirstOffset || ullCaretPos >= ullEndOffset)
		return;

	int iCaretHexPosToPrintX;
	int iCaretTextPosToPrintX;
	int iCaretHexPosToPrintY;
	int iCaretTextPosToPrintY;
	HexChunkPoint(ullCaretPos, iCaretHexPosToPrintX, iCaretHexPosToPrintY);
	TextChunkPoint(ullCaretPos, iCaretTextPosToPrintX, iCaretTextPosToPrintY);

	const auto sIndexToPrint = static_cast<std::size_t>(ullCaretPos - ullFirstOffset);
	std::wstring wstrHexCaretToPrint;
	std::wstring wstrTextCaretToPrint;
	if (m_fCaretHigh) {
		wstrHexCaretToPrint = wsvHex[sIndexToPrint * 2];
	}
	else {
		wstrHexCaretToPrint = wsvHex[(sIndexToPrint * 2) + 1];
		iCaretHexPosToPrintX += GetCharWidthExtras();
	}
	wstrTextCaretToPrint = wsvText[sIndexToPrint];

	POLYTEXTW arrPolyCaret[2]; //Caret Poly array.

	//Hex Caret Poly.
	arrPolyCaret[0] = { .x { iCaretHexPosToPrintX }, .y { iCaretHexPosToPrintY },
		.n { static_cast<UINT>(wstrHexCaretToPrint.size()) }, .lpstr { wstrHexCaretToPrint.data() },
		.pdx { GetCharsWidthArray() } };

	//Text Caret Poly.
	arrPolyCaret[1] = { .x { iCaretTextPosToPrintX }, .y { iCaretTextPosToPrintY },
		.n { static_cast<UINT>(wstrTextCaretToPrint.size()) }, .lpstr { wstrTextCaretToPrint.data() },
		.pdx { GetCharsWidthArray() } };

	//Caret color.
	const auto clrBkCaret = m_pSelection->HitTest(ullCaretPos) ? m_stColors.clrBkCaretSel : m_stColors.clrBkCaret;

	//Caret printing.
	wnd::CDC dc(hDC);
	dc.SelectObject(m_hFntMain);
	dc.SetTextColor(m_stColors.clrFontCaret);
	dc.SetBkColor(clrBkCaret);
	for (const auto ref : arrPolyCaret) {
		::ExtTextOutW(dc, ref.x, ref.y, ref.uiFlags, &ref.rcl, ref.lpstr, ref.n, ref.pdx); //Hex/Text caret printing.
	}
}

void CHexCtrl::DrawDataInterp(HDC hDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const
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

	for (auto itLine = 0; itLine < iLines; ++itLine) {
		std::wstring wstrHexDataInterpToPrint; //Data Interpreter Hex and Text strings to print.
		std::wstring wstrTextDataInterpToPrint;
		int iDataInterpHexPosToPrintX { }; //Data Interpreter X coords.
		int iDataInterpTextPosToPrintX { };
		bool fNeedChunkPoint { true };
		const auto iPosToPrintY = m_iStartWorkAreaYPx + (m_sizeFontMain.cy * itLine); //Hex and Text are the same.

		//Main loop for printing Hex chunks and Text chars.
		for (auto itChunk { 0U }; itChunk < GetCapacity() && sIndexToPrint < wsvText.size(); ++itChunk, ++sIndexToPrint) {
			const auto ullOffsetCurr = ullStartOffset + sIndexToPrint;
			if (ullOffsetCurr >= ullCaretPos && ullOffsetCurr < (ullCaretPos + dwHglSize)) {
				if (fNeedChunkPoint) {
					int iCy;
					HexChunkPoint(sIndexToPrint, iDataInterpHexPosToPrintX, iCy);
					TextChunkPoint(sIndexToPrint, iDataInterpTextPosToPrintX, iCy);
					fNeedChunkPoint = false;
				}

				if (!wstrHexDataInterpToPrint.empty()) { //Only adding spaces if there are chars beforehead.
					if ((itChunk % GetGroupSize()) == 0) {
						wstrHexDataInterpToPrint += L' ';
					}

					//Additional space between capacity halves, only in 1 byte grouping size.
					if (GetGroupSize() == 1 && itChunk == m_dwCapacityBlockSize) {
						wstrHexDataInterpToPrint += L"  ";
					}
				}
				wstrHexDataInterpToPrint += wsvHex[sIndexToPrint * 2];
				wstrHexDataInterpToPrint += wsvHex[(sIndexToPrint * 2) + 1];
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
		wnd::CDC dc(hDC);
		dc.SelectObject(m_hFntMain);
		dc.SetTextColor(m_stColors.clrFontDataInterp);
		dc.SetBkColor(m_stColors.clrBkDataInterp);
		for (const auto& ref : vecPolyDataInterp) {
			::ExtTextOutW(dc, ref.x, ref.y, ref.uiFlags, &ref.rcl, ref.lpstr, ref.n, ref.pdx); //Hex/Text Data Interpreter printing.
		}
	}
}

void CHexCtrl::DrawPageLines(HDC hDC, ULONGLONG ullStartLine, int iLines)const
{
	if (!IsPageVisible())
		return;

	struct PAGELINES { //Struct for pages lines.
		POINT ptStart;
		POINT ptEnd;
	};
	std::vector<PAGELINES> vecPageLines;

	//Loop for printing Hex chunks and Text chars line by line.
	for (auto itLine = 0; itLine < iLines; ++itLine) {
		//Page's lines vector to print.
		if ((((ullStartLine + itLine) * GetCapacity()) % m_dwPageSize == 0) && itLine > 0) {
			const auto iPosToPrintY = m_iStartWorkAreaYPx + (m_sizeFontMain.cy * itLine);
			vecPageLines.emplace_back(POINT { .x { m_iFirstVertLinePx }, .y { iPosToPrintY } },
				POINT { .x { m_iFourthVertLinePx }, .y { iPosToPrintY } });
		}
	}

	//Page lines printing.
	if (!vecPageLines.empty()) {
		for (const auto& ref : vecPageLines) {
			wnd::CDC dc(hDC);
			dc.MoveTo(ref.ptStart.x, ref.ptStart.y);
			dc.LineTo(ref.ptEnd.x, ref.ptEnd.y);
		}
	}
}

void CHexCtrl::FillCapacityString()
{
	const auto dwCapacity = GetCapacity();
	m_wstrCapacity.clear();
	m_wstrCapacity.reserve(static_cast<std::size_t>(dwCapacity) * 3);
	for (auto it { 0U }; it < dwCapacity; ++it) {
		m_wstrCapacity += std::vformat(IsOffsetAsHex() ? L"{: >2X}" : L"{: >2d}", std::make_wformat_args(it));

		//Additional space between hex chunk blocks.
		if ((((it + 1) % GetGroupSize()) == 0) && (it < (dwCapacity - 1))) {
			m_wstrCapacity += L" ";
		}

		//Additional space between hex halves.
		if (GetGroupSize() == 1 && it == (m_dwCapacityBlockSize - 1)) {
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
	const auto lFontSize = ::MulDiv(-GetFontSize(), 72, m_iLOGPIXELSY) + (fInc ? 1 : -1); //Convert font Height to point size.
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
	if (const auto it = std::find_if(m_vecKeyBind.begin(), m_vecKeyBind.end(), [=](const KEYBIND& ref) {
		return ref.fCtrl == fCtrl && ref.fShift == fShift && ref.fAlt == fAlt && ref.uKey == uKey; });
		it != m_vecKeyBind.end()) {
		return it->eCmd;
	}

	return std::nullopt;
}

auto CHexCtrl::GetCommandFromMenu(WORD wMenuID)const->std::optional<EHexCmd>
{
	if (const auto it = std::find_if(m_vecKeyBind.begin(), m_vecKeyBind.end(), [=](const KEYBIND& ref) {
		return ref.wMenuID == wMenuID; }); it != m_vecKeyBind.end()) {
		return it->eCmd;
	}

	return std::nullopt;
}

auto CHexCtrl::GetDigitsOffset()const->DWORD
{
	return IsOffsetAsHex() ? m_dwDigitsOffsetHex : m_dwDigitsOffsetDec;
}

long CHexCtrl::GetFontSize()const
{
	return GetFont().lfHeight;
}

auto CHexCtrl::GetRectTextCaption()const->wnd::CRect
{
	const auto iScrollH { static_cast<int>(m_pScrollH->GetScrollPos()) };
	return { m_iThirdVertLinePx - iScrollH, m_iFirstHorzLinePx, m_iFourthVertLinePx - iScrollH, m_iSecondHorzLinePx };
}

auto CHexCtrl::GetSelectedLines()const->ULONGLONG
{
	if (!m_pSelection->HasContiguousSel())
		return 0ULL;

	const auto dwCapacity = GetCapacity();
	const auto ullSelStart = m_pSelection->GetSelStart();
	const auto ullSelEnd = m_pSelection->GetSelEnd();
	const auto ullSelSize = m_pSelection->GetSelSize();
	const auto ullModStart = ullSelStart % dwCapacity;
	const auto ullModEnd = ullSelEnd % dwCapacity;
	const auto ullLines = ullSelSize / dwCapacity + ((ullModStart > (ullModEnd + 1)) ? 2
		: ((ullSelSize % dwCapacity) ? 1 : (ullModStart > 0) ? 1 : 0));

	return ullLines;
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

auto CHexCtrl::GetVirtualOffset(ULONGLONG ullOffset)const->ULONGLONG
{
	return GetOffset(ullOffset, true);
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
		for (auto itCapacity = 0U; itCapacity < dwCapacity; ++itCapacity) {
			if (itCapacity > 0 && (itCapacity % dwGroupSize) == 0) {
				iTotalSpaceBetweenChunks += GetCharWidthExtras();
				if (dwGroupSize == 1 && itCapacity == m_dwCapacityBlockSize) {
					iTotalSpaceBetweenChunks += m_iSpaceBetweenBlocksPx;
				}
			}

			const auto iCurrChunkBegin = m_iIndentFirstHexChunkXPx + (m_iSizeHexBytePx * itCapacity) + iTotalSpaceBetweenChunks;
			const auto iCurrChunkEnd = iCurrChunkBegin + m_iSizeHexBytePx +
				(((itCapacity + 1) % dwGroupSize) == 0 ? GetCharWidthExtras() : 0)
				+ ((dwGroupSize == 1 && (itCapacity + 1) == m_dwCapacityBlockSize) ? m_iSpaceBetweenBlocksPx : 0);

			if (static_cast<unsigned int>(iX) < iCurrChunkEnd) { //If iX lays in-between [iCurrChunkBegin...iCurrChunkEnd).
				stHit.ullOffset = static_cast<ULONGLONG>(itCapacity) + ((iY - m_iStartWorkAreaYPx) / m_sizeFontMain.cy) *
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
	if (spnOper.empty()) { ut::DBG_REPORT(L"Operation span is empty."); return; }

	const auto& vecSpan = hms.vecSpan;
	const auto ullTotalSize = std::reduce(vecSpan.begin(), vecSpan.end(), 0ULL,
		[](ULONGLONG ullSumm, const HEXSPAN& ref) { return ullSumm + ref.ullSize; });
	assert(ullTotalSize <= GetDataSize());

	CHexDlgProgress dlgProg(L"Modifying...", L"", vecSpan.back().ullOffset, vecSpan.back().ullOffset + ullTotalSize);
	const auto lmbModify = [&]() {
		for (const auto& ref : vecSpan) { //Span-vector's size times.
			const auto ullOffsetToModify { ref.ullOffset };
			const auto ullSizeToModify { ref.ullSize };
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
				for (auto itChunk { 0ULL }; itChunk < ullChunks; ++itChunk) {
					const auto ullOffsetCurr = ullOffsetToModify + (itChunk * ullSizeCache);
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
						if (dlgProg.IsCanceled()) {
							SetDataVirtual(spnData, { ullOffsetCurr, ullSizeCache });
							goto exit;
						}
						dlgProg.SetCurrent(ullOffsetCurr + ullIndex);
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
				for (auto itChunk { 0ULL }; itChunk < ullChunks; ++itChunk) {
					if (ullSmallChunkCur == (ullSmallChunks - 1) && ullSmallMod > 0) {
						ullOffsetCurr += ullSmallMod;
						ullSizeCacheCurr = ullSmallMod;
						ullOffsetSubSpan += ullSmallMod;
					}
					else {
						ullOffsetCurr = ullOffsetToModify + (itChunk * ullSizeCache);
						ullSizeCacheCurr = ullSizeCache;
						ullOffsetSubSpan = ullSmallChunkCur * ullSizeCache;
					}

					const auto spnData = GetData({ ullOffsetCurr, ullSizeCacheCurr });
					assert(!spnData.empty());
					FuncWorker(spnData.data(), hms, spnOper.subspan(static_cast<std::size_t>(ullOffsetSubSpan),
						static_cast<std::size_t>(ullSizeCacheCurr)));

					if (dlgProg.IsCanceled()) {
						SetDataVirtual(spnData, { ullOffsetCurr, ullSizeCacheCurr });
						goto exit;
					}

					dlgProg.SetCurrent(ullOffsetCurr + ullSizeCacheCurr);
					SetDataVirtual(spnData, { ullOffsetCurr, ullSizeCacheCurr });

					if (++ullSmallChunkCur == ullSmallChunks) {
						ullSmallChunkCur = 0ULL;
						ullOffsetSubSpan = 0ULL;
					}
				}
			}
		}
	exit:
		dlgProg.OnCancel();
		};

	static constexpr auto uSizeToRunThread { 1024U * 1024U * 50U }; //50MB.
	if (ullTotalSize > uSizeToRunThread) { //Spawning new thread only if data size is big enough.
		std::thread thrd(lmbModify);
		dlgProg.DoModal(m_Wnd, m_hInstRes);
		thrd.join();
	}
	else {
		lmbModify();
	}
}

auto CHexCtrl::OffsetToWstr(ULONGLONG ullOffset)const->std::wstring
{
	const auto dwDigitsOffset = GetDigitsOffset();
	ullOffset = GetVirtualOffset(ullOffset);
	return std::vformat(IsOffsetAsHex() ? L"{:0>{}X}" : L"{:0>{}}", std::make_wformat_args(ullOffset, dwDigitsOffset));
}

void CHexCtrl::OnCaretPosChange(ULONGLONG ullOffset)
{
	//To prevent UpdateData() while key is pressed continuously, only when one time pressed.
	if (!m_fKeyDownAtm) {
		m_pDlgDataInterp->UpdateData();
	}

	if (auto pBkm = m_pDlgBkmMgr->HitTest(ullOffset); pBkm != nullptr) { //If clicked on bookmark.
		const HEXBKMINFO hbi { .hdr { m_Wnd, static_cast<UINT>(m_Wnd.GetDlgCtrlID()), HEXCTRL_MSG_BKMCLICK }, .pBkm { pBkm } };
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
	if (const auto hWndParent = m_Wnd.GetParent(); hWndParent != nullptr) {
		::SendMessageW(hWndParent, WM_NOTIFY, m_Wnd.GetDlgCtrlID(), reinterpret_cast<LPARAM>(&t));
	}
}

void CHexCtrl::ParentNotify(UINT uCode)const
{
	ParentNotify(NMHDR { .hwndFrom { m_Wnd }, .idFrom { static_cast<UINT>(m_Wnd.GetDlgCtrlID()) }, .code { uCode } });
}

void CHexCtrl::Print()
{
	PRINTPAGERANGE ppr { .nFromPage { 1 }, .nToPage { 1 } };
	PRINTDLGEXW m_pdex { .lStructSize { sizeof(PRINTDLGEXW) }, .hwndOwner { m_Wnd },
		.Flags { static_cast<DWORD>(PD_RETURNDC | PD_NOCURRENTPAGE | (m_pSelection->HasContiguousSel() ?
			PD_SELECTION : (PD_NOSELECTION | PD_PAGENUMS))) }, .nPageRanges { 1 }, .nMaxPageRanges { 1 },
		.lpPageRanges { &ppr }, .nMinPage { 1 }, .nMaxPage { 0xFFFFUL }, .nStartPage { START_PAGE_GENERAL } };

	if (::PrintDlgExW(&m_pdex) != S_OK) {
		ut::DBG_REPORT(L"PrintDlgExW error.");
		return;
	}

	if (m_pdex.hDC == nullptr) {
		ut::DBG_REPORT(L"No printer found (m_pdex.hDC == nullptr).");
		return;
	}

	//User pressed "Cancel", or "Apply" then "Cancel".
	if (m_pdex.dwResultAction == PD_RESULT_CANCEL || m_pdex.dwResultAction == PD_RESULT_APPLY) {
		::DeleteDC(m_pdex.hDC);
		return;
	}

	const auto fPrintSelection = m_pdex.Flags & PD_SELECTION;
	const auto fPrintRange = m_pdex.Flags & PD_PAGENUMS;
	const auto fPrintAll = !fPrintSelection && !fPrintRange;

	if (!fPrintAll && !fPrintRange && !fPrintSelection) {
		::DeleteDC(m_pdex.hDC);
		return;
	}

	wnd::CDC dcPrint(m_pdex.hDC);
	if (const DOCINFOW di { .cbSize { sizeof(DOCINFOW) }, .lpszDocName { L"HexCtrl" } }; dcPrint.StartDocW(&di) < 0) {
		dcPrint.AbortDoc();
		dcPrint.DeleteDC();
		return;
	}

	constexpr auto iMarginX = 150;
	constexpr auto iMarginY = 150;
	const wnd::CRect rcPrint(POINT(0, 0), SIZE(::GetDeviceCaps(dcPrint, HORZRES) - (iMarginX * 2),
		::GetDeviceCaps(dcPrint, VERTRES) - (iMarginY * 2)));
	const SIZE sizePrintDpi { ::GetDeviceCaps(dcPrint, LOGPIXELSX), ::GetDeviceCaps(dcPrint, LOGPIXELSY) };
	const auto iFontSizeRatio { sizePrintDpi.cy / m_iLOGPIXELSY };
	const auto dwCapacity = GetCapacity();

	//Setting scaled fonts for printing, and temporarily disabling redraw.
	SetRedraw(false);
	const auto lfOrigMain = GetFont(true);
	const auto lfOrigInfo = GetFont(false);
	LOGFONTW lfPrintMain { lfOrigMain };
	LOGFONTW lfPrintInfo { lfOrigInfo };
	lfPrintMain.lfHeight *= iFontSizeRatio;
	lfPrintInfo.lfHeight *= iFontSizeRatio;
	SetFont(lfPrintMain, true);
	SetFont(lfPrintInfo, false);
	RecalcAll(true, dcPrint, &rcPrint); //Recalc for printer dc, to get correct printing sizes.
	const auto iLinesInPage = m_iHeightWorkAreaPx / m_sizeFontMain.cy;
	dcPrint.SetMapMode(MM_TEXT);

	if (fPrintSelection) {
		const auto ullSelStart = m_pSelection->GetSelStart();
		const auto ullLinesToPrint = GetSelectedLines();
		const auto dwPagesToPrint = static_cast<DWORD>(ullLinesToPrint / iLinesInPage
			+ ((ullLinesToPrint % iLinesInPage) > 0 ? 1 : 0));
		auto ullStartLine = ullSelStart / dwCapacity;
		const auto ullLastLine = ullStartLine + ullLinesToPrint;
		const auto hcsOrig = GetColors();
		auto hcsPrint { hcsOrig }; //To print with normal text/bk colors, not white text on black bk.
		hcsPrint.clrBkSel = hcsOrig.clrBk;
		hcsPrint.clrFontSel = hcsOrig.clrFontHex;
		SetColors(hcsPrint);

		for (auto itPage = 0U; itPage < dwPagesToPrint; ++itPage) {
			auto ullEndLine = ullStartLine + iLinesInPage;
			if (ullEndLine > ullLastLine) {
				ullEndLine = ullLastLine;
			}

			const auto iLines = static_cast<int>(ullEndLine - ullStartLine);
			const auto& [wstrHex, wstrText] = BuildDataToDraw(ullStartLine, iLines);
			//Set viewport to have some indent from the edges.
			//Viewport must be set on every page, otherwise only first page is printed with the proper offset.
			dcPrint.SetViewportOrg(iMarginX, iMarginY);
			dcPrint.StartPage();
			DrawWindow(dcPrint);
			DrawInfoBar(dcPrint);
			DrawOffsets(dcPrint, ullStartLine, iLines);
			DrawSelection(dcPrint, ullStartLine, iLines, wstrHex, wstrText);
			DrawSelHighlight(dcPrint, ullStartLine, iLines, wstrHex, wstrText);
			DrawPageLines(dcPrint, ullStartLine, iLines);
			ullStartLine += iLinesInPage;
			dcPrint.EndPage();
		}

		SetColors(hcsOrig); //Restore original colors.
	}
	else {
		const auto ullTotalLines = GetDataSize() / dwCapacity + ((GetDataSize() % dwCapacity) ? 1 : 0);
		const auto ullTotalPages = ullTotalLines / iLinesInPage + ((ullTotalLines % iLinesInPage) ? 1 : 0);
		ULONGLONG ullStartLine { 0 };
		DWORD dwPagesToPrint { 0 };

		if (fPrintAll) {
			dwPagesToPrint = static_cast<DWORD>(ullTotalPages);
		}
		else if (fPrintRange) {
			const auto dwFromPage = ppr.nFromPage - 1;
			const auto dwToPage = ppr.nToPage;
			if (dwFromPage <= ullTotalPages) { //Checks for out-of-range pages user input.
				dwPagesToPrint = dwToPage - dwFromPage;
				if (dwPagesToPrint + dwFromPage > ullTotalPages) {
					dwPagesToPrint = static_cast<DWORD>(ullTotalPages - dwFromPage);
				}
			}

			ullStartLine = dwFromPage * iLinesInPage;
		}

		for (auto itPage = 0U; itPage < dwPagesToPrint; ++itPage) {
			auto ullEndLine = ullStartLine + iLinesInPage;
			if (ullEndLine > ullTotalLines) {
				ullEndLine = ullTotalLines;
			}

			const auto iLines = static_cast<int>(ullEndLine - ullStartLine);
			const auto& [wstrHex, wstrText] = BuildDataToDraw(ullStartLine, iLines);
			dcPrint.SetViewportOrg(iMarginX, iMarginY);
			dcPrint.StartPage();
			DrawWindow(dcPrint);
			DrawInfoBar(dcPrint);

			if (IsDataSet()) {
				DrawOffsets(dcPrint, ullStartLine, iLines);
				DrawHexText(dcPrint, ullStartLine, iLines, wstrHex, wstrText);
				DrawTemplates(dcPrint, ullStartLine, iLines, wstrHex, wstrText);
				DrawBookmarks(dcPrint, ullStartLine, iLines, wstrHex, wstrText);
				DrawSelection(dcPrint, ullStartLine, iLines, wstrHex, wstrText);
				DrawSelHighlight(dcPrint, ullStartLine, iLines, wstrHex, wstrText);
				DrawCaret(dcPrint, ullStartLine, iLines, wstrHex, wstrText);
				DrawDataInterp(dcPrint, ullStartLine, iLines, wstrHex, wstrText);
				DrawPageLines(dcPrint, ullStartLine, iLines);
			}

			ullStartLine += iLinesInPage;
			dcPrint.EndPage();
		}
	}

	dcPrint.EndDoc();
	dcPrint.DeleteDC();

	//Restore original fonts.
	SetFont(lfOrigMain, true);
	SetFont(lfOrigInfo, false);
	SetRedraw(true);
	RecalcAll();
}

void CHexCtrl::RecalcAll(bool fPrinter, HDC hDCPrinter, LPCRECT pRCPrinter)
{
	const wnd::CDC dcCurr = fPrinter ? hDCPrinter : m_Wnd.GetDC();
	const auto ullCurLineV = GetTopLine();
	TEXTMETRICW tm;
	dcCurr.SelectObject(m_hFntMain);
	dcCurr.GetTextMetricsW(&tm);
	m_sizeFontMain.cx = tm.tmAveCharWidth;
	m_sizeFontMain.cy = tm.tmHeight + tm.tmExternalLeading;
	dcCurr.SelectObject(m_hFntInfoBar);
	dcCurr.GetTextMetricsW(&tm);
	m_sizeFontInfo.cx = tm.tmAveCharWidth;
	m_sizeFontInfo.cy = tm.tmHeight + tm.tmExternalLeading;
	m_iHeightInfoBarPx = HasInfoBar() ? m_sizeFontInfo.cy + (m_sizeFontInfo.cy / 3) : 0;
	constexpr auto iIndentBottomLine { 1 }; //Bottom line indent from window's bottom.
	m_iHeightBottomOffAreaPx = m_iHeightInfoBarPx + iIndentBottomLine;

	const auto dwGroupSize = GetGroupSize();
	const auto dwCapacity = GetCapacity();
	const auto iCharWidth = GetCharWidthNative();
	const auto iCharWidthExt = GetCharWidthExtras();

	//Approximately "dwCapacity * 3 + 1" size array of char's width, to be enough for the Hex area chars.
	m_vecCharsWidth.assign((dwCapacity * 3) + 1, iCharWidthExt);
	m_iSecondVertLinePx = m_iFirstVertLinePx + GetDigitsOffset() * iCharWidth + iCharWidth * 2;
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

	const wnd::CRect rc { fPrinter ? *pRCPrinter : m_Wnd.GetClientRect() };
	RecalcClientArea(rc.Width(), rc.Height());

	//Scrolls, ReleaseDC and Redraw only for window DC, not for printer DC.
	if (!fPrinter) {
		const auto ullDataSize = GetDataSize();
		m_pScrollV->SetScrollSizes(m_sizeFontMain.cy, GetScrollPageSize(),
			static_cast<ULONGLONG>(m_iStartWorkAreaYPx) + m_iHeightBottomOffAreaPx
			+ (m_sizeFontMain.cy * (ullDataSize / dwCapacity + (ullDataSize % dwCapacity == 0 ? 1 : 2))));
		m_pScrollH->SetScrollSizes(iCharWidthExt, rc.Width(), static_cast<ULONGLONG>(m_iFourthVertLinePx) + 1);
		m_pScrollV->SetScrollPos(ullCurLineV * m_sizeFontMain.cy);
		m_Wnd.ReleaseDC(dcCurr);
		Redraw();
	}
}

void CHexCtrl::RecalcClientArea(int iWidth, int iHeight)
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

	const auto& uptrRedo = m_vecRedo.back();
	VecSpan vecSpan;
	vecSpan.reserve(uptrRedo->size());
	std::transform(uptrRedo->begin(), uptrRedo->end(), std::back_inserter(vecSpan),
		[](UNDO& ref) { return HEXSPAN { ref.ullOffset, ref.vecData.size() }; });
	SnapshotUndo(vecSpan); //Creating new Undo data snapshot.

	for (const auto& ref : *uptrRedo) {
		const auto& vecRedoData = ref.vecData;

		if (IsVirtual() && vecRedoData.size() > GetCacheSize()) { //In VirtualData mode processing data chunk by chunk.
			const auto dwSizeChunk = GetCacheSize();
			const auto sMod = vecRedoData.size() % dwSizeChunk;
			auto ullChunks = vecRedoData.size() / dwSizeChunk + (sMod > 0 ? 1 : 0);
			std::size_t ullOffset = 0;
			while (ullChunks-- > 0) {
				const auto ullSize = (ullChunks == 1 && sMod > 0) ? sMod : dwSizeChunk;
				if (const auto spnData = GetData({ ullOffset, ullSize }); !spnData.empty()) {
					std::copy_n(vecRedoData.begin() + ullOffset, ullSize, spnData.data());
					SetDataVirtual(spnData, { ullOffset, ullSize });
				}
				ullOffset += ullSize;
			}
		}
		else {
			if (const auto spnData = GetData({ ref.ullOffset, vecRedoData.size() }); !spnData.empty()) {
				std::copy_n(vecRedoData.begin(), vecRedoData.size(), spnData.data());
				SetDataVirtual(spnData, { ref.ullOffset, vecRedoData.size() });
			}
		}
	}

	m_vecRedo.pop_back();
	OnModifyData();
	m_Wnd.RedrawWindow();
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
	const auto rcClient = m_Wnd.GetClientRect();

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

	m_pHexVirtData->OnHexSetData({ .hdr { m_Wnd, static_cast<UINT>(m_Wnd.GetDlgCtrlID()) },
		.stHexSpan { hss }, .spnData { spnData } });
}

void CHexCtrl::SetFontSize(long lSize)
{
	if (lSize < 4 || lSize > 64) //Prevent font size from being too small or too big.
		return;

	auto lf = GetFont();
	lf.lfHeight = -::MulDiv(lSize, m_iLOGPIXELSY, 72); //Convert point size to font Height.
	SetFont(lf);
}

void CHexCtrl::SnapshotUndo(const VecSpan& vecSpan)
{
	constexpr auto dwUndoMax { 512U }; //Undo's max limit.
	const auto ullTotalSize = std::reduce(vecSpan.begin(), vecSpan.end(), 0ULL,
		[](ULONGLONG ullSumm, const HEXSPAN& ref) { return ullSumm + ref.ullSize; });

	//Check for very big undo size.
	if (ullTotalSize > 1024 * 1024 * 10)
		return;

	//If Undo vec's size is exceeding Undo's max limit, remove first 64 snapshots (the oldest ones).
	if (m_vecUndo.size() >= static_cast<std::size_t>(dwUndoMax)) {
		const auto itFirst = m_vecUndo.begin();
		const auto itLast = itFirst + 64U;
		m_vecUndo.erase(itFirst, itLast);
	}

	//Making new Undo data snapshot.
	const auto& refUndo = m_vecUndo.emplace_back(std::make_unique<std::vector<UNDO>>());

	//Bad alloc may happen here!!!
	try {
		for (const auto& ref : vecSpan) { //vecSpan.size() amount of continuous areas to preserve.
			auto& refUNDO = refUndo->emplace_back(UNDO { ref.ullOffset, { } });
			refUNDO.vecData.resize(static_cast<std::size_t>(ref.ullSize));

			//In VirtualData mode processing data chunk by chunk.
			if (IsVirtual() && ref.ullSize > GetCacheSize()) {
				const auto dwSizeChunk = GetCacheSize();
				const auto ullMod = ref.ullSize % dwSizeChunk;
				auto ullChunks = ref.ullSize / dwSizeChunk + (ullMod > 0 ? 1 : 0);
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
				if (const auto spnData = GetData(ref); !spnData.empty()) {
					std::copy_n(spnData.data(), ref.ullSize, refUNDO.vecData.data());
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
	const auto dwCapacity = GetCapacity() > 0 ? GetCapacity() : 0xFFFFFFFFUL; //To suppress warning C4724.
	const DWORD dwMod = ullOffset % dwCapacity;
	iCx = static_cast<int>((m_iIndentTextXPx + dwMod * GetCharWidthExtras()) - m_pScrollH->GetScrollPos());

	const auto ullScrollV = m_pScrollV->GetScrollPos();
	iCy = static_cast<int>((m_iStartWorkAreaYPx + (ullOffset / dwCapacity) * m_sizeFontMain.cy) -
		(ullScrollV - (ullScrollV % m_sizeFontMain.cy)));
}

void CHexCtrl::TTMainShow(bool fShow, bool fTimer)
{
	if (fShow) {
		m_tmTT = std::chrono::high_resolution_clock::now();
		POINT ptCur;
		::GetCursorPos(&ptCur);
		m_wndTTMain.SendMsg(TTM_TRACKPOSITION, 0, static_cast<LPARAM>(MAKELONG(ptCur.x + 3, ptCur.y - 20)));
		m_wndTTMain.SendMsg(TTM_UPDATETIPTEXTW, 0, reinterpret_cast<LPARAM>(&m_ttiMain));
		m_wndTTMain.SendMsg(TTM_TRACKACTIVATE, static_cast<WPARAM>(TRUE), reinterpret_cast<LPARAM>(&m_ttiMain));
		m_Wnd.SetTimer(m_uIDTTTMain, 300, nullptr);
	}
	else {
		m_Wnd.KillTimer(m_uIDTTTMain);

		//When hiding tooltip by timer we not nullify the pointer.
		//Otherwise tooltip will be shown again after mouse movement,
		//even if cursor didn't leave current bkm area.
		if (!fTimer) {
			m_pBkmTTCurr = nullptr;
			m_pTFieldTTCurr = nullptr;
		}

		m_wndTTMain.SendMsg(TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&m_ttiMain));
	}
}

void CHexCtrl::TTOffsetShow(bool fShow)
{
	if (fShow) {
		POINT ptCur;
		::GetCursorPos(&ptCur);
		auto wstrOffset = (IsOffsetAsHex() ? L"Offset: 0x" : L"Offset: ") + OffsetToWstr(GetTopLine() * GetCapacity());
		m_ttiOffset.lpszText = wstrOffset.data();
		m_wndTTOffset.SendMsg(TTM_TRACKPOSITION, 0, static_cast<LPARAM>(MAKELONG(ptCur.x - 5, ptCur.y - 20)));
		m_wndTTOffset.SendMsg(TTM_UPDATETIPTEXTW, 0, reinterpret_cast<LPARAM>(&m_ttiOffset));
		m_ttiOffset.lpszText = nullptr;
	}

	m_wndTTOffset.SendMsg(TTM_TRACKACTIVATE, static_cast<WPARAM>(fShow), reinterpret_cast<LPARAM>(&m_ttiOffset));
}

void CHexCtrl::Undo()
{
	if (m_vecUndo.empty())
		return;

	//Bad alloc may happen here! If there is no more free memory, just clear the vec and return.
	try {
		//Creating new Redo data snapshot.
		const auto& refRedo = m_vecRedo.emplace_back(std::make_unique<std::vector<UNDO>>());
		for (const auto& ref : *m_vecUndo.back()) {
			auto& refRedoBack = refRedo->emplace_back(UNDO { ref.ullOffset, { } });
			refRedoBack.vecData.resize(ref.vecData.size());
			const auto& vecUndoData = ref.vecData;

			if (IsVirtual() && vecUndoData.size() > GetCacheSize()) { //In VirtualData mode processing data chunk by chunk.
				const auto dwSizeChunk = GetCacheSize();
				const auto sMod = vecUndoData.size() % dwSizeChunk;
				auto ullChunks = vecUndoData.size() / dwSizeChunk + (sMod > 0 ? 1 : 0);
				std::size_t ullOffset = 0;
				while (ullChunks-- > 0) {
					const auto ullSize = (ullChunks == 1 && sMod > 0) ? sMod : dwSizeChunk;
					if (const auto spnData = GetData({ ullOffset, ullSize }); !spnData.empty()) {
						std::copy_n(spnData.data(), ullSize, refRedoBack.vecData.begin() + ullOffset); //Fill Redo with the data.
						std::copy_n(vecUndoData.begin() + ullOffset, ullSize, spnData.data()); //Undo the data.
						SetDataVirtual(spnData, { ullOffset, ullSize });
					}
					ullOffset += ullSize;
				}
			}
			else {
				if (const auto spnData = GetData({ ref.ullOffset, vecUndoData.size() }); !spnData.empty()) {
					std::copy_n(spnData.data(), vecUndoData.size(), refRedoBack.vecData.begin()); //Fill Redo with the data.
					std::copy_n(vecUndoData.begin(), vecUndoData.size(), spnData.data()); //Undo the data.
					SetDataVirtual(spnData, { ref.ullOffset, vecUndoData.size() });
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
	m_Wnd.RedrawWindow();
}

void CHexCtrl::ModifyOper(std::byte* pData, const HEXMODIFY& hms, [[maybe_unused]] SpanCByte)
{
	assert(pData != nullptr);
	using enum EHexDataType;
	using enum EHexOperMode;

	constexpr auto lmbOperT = []<typename T>(T * pData, const HEXMODIFY & hms) {
		T tData = hms.fBigEndian ? ut::ByteSwap(*pData) : *pData;
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
				tData = ut::BitReverse(tData);
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
		case OPER_MIN:
			tData = (std::max)(tData, tOper);
			break;
		case OPER_MAX:
			tData = (std::min)(tData, tOper);
			break;
		case OPER_SWAP:
			tData = ut::ByteSwap(tData);
			break;
		default:
			break;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			tData = ut::ByteSwap(tData);
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
}

#if defined(_M_IX86) || defined(_M_X64)
void CHexCtrl::ModifyOperVec128(std::byte* pData, const HEXMODIFY& hms, [[maybe_unused]] SpanCByte)
{
	assert(pData != nullptr);
	using enum EHexDataType; using enum EHexOperMode;

	constexpr auto lmbOperVec128Int8 = [](std::int8_t* pi8Data, const HEXMODIFY& hms) {
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
		case OPER_MIN:
			m128iResult = _mm_max_epi8(m128iData, m128iOper); //SSE4.1
			break;
		case OPER_MAX:
			m128iResult = _mm_min_epi8(m128iData, m128iOper); //SSE4.1
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
			m128iResult = _mm_setr_epi8(ut::BitReverse(i8Data[0]), ut::BitReverse(i8Data[1]), ut::BitReverse(i8Data[2]),
				ut::BitReverse(i8Data[3]), ut::BitReverse(i8Data[4]), ut::BitReverse(i8Data[5]), ut::BitReverse(i8Data[6]),
				ut::BitReverse(i8Data[7]), ut::BitReverse(i8Data[8]), ut::BitReverse(i8Data[9]), ut::BitReverse(i8Data[10]),
				ut::BitReverse(i8Data[11]), ut::BitReverse(i8Data[12]), ut::BitReverse(i8Data[13]), ut::BitReverse(i8Data[14]),
				ut::BitReverse(i8Data[15]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported int8_t operation.");
			break;
		}

		_mm_storeu_si128(reinterpret_cast<__m128i*>(pi8Data), m128iResult);
		};

	constexpr auto lmbOperVec128UInt8 = [](std::uint8_t* pui8Data, const HEXMODIFY& hms) {
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
		case OPER_MIN:
			m128iResult = _mm_max_epu8(m128iData, m128iOper); //SSE4.1
			break;
		case OPER_MAX:
			m128iResult = _mm_min_epu8(m128iData, m128iOper); //SSE4.1
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
			m128iResult = _mm_setr_epi8(ut::BitReverse(ui8Data[0]), ut::BitReverse(ui8Data[1]), ut::BitReverse(ui8Data[2]),
				ut::BitReverse(ui8Data[3]), ut::BitReverse(ui8Data[4]), ut::BitReverse(ui8Data[5]), ut::BitReverse(ui8Data[6]),
				ut::BitReverse(ui8Data[7]), ut::BitReverse(ui8Data[8]), ut::BitReverse(ui8Data[9]), ut::BitReverse(ui8Data[10]),
				ut::BitReverse(ui8Data[11]), ut::BitReverse(ui8Data[12]), ut::BitReverse(ui8Data[13]), ut::BitReverse(ui8Data[14]),
				ut::BitReverse(ui8Data[15]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported uint8_t operation.");
			break;
		}

		_mm_storeu_si128(reinterpret_cast<__m128i*>(pui8Data), m128iResult);
		};

	constexpr auto lmbOperVec128Int16 = [](std::int16_t* pi16Data, const HEXMODIFY& hms) {
		const auto m128iData = hms.fBigEndian ?
			ut::ByteSwapVec<std::int16_t>(_mm_loadu_si128(reinterpret_cast<const __m128i*>(pi16Data)))
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
		case OPER_MIN:
			m128iResult = _mm_max_epi16(m128iData, m128iOper);
			break;
		case OPER_MAX:
			m128iResult = _mm_min_epi16(m128iData, m128iOper);
			break;
		case OPER_SWAP:
			m128iResult = ut::ByteSwapVec<std::int16_t>(m128iData);
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
			m128iResult = _mm_slli_epi16(m128iData, i16Oper);
			break;
		case OPER_SHR:
			m128iResult = _mm_srai_epi16(m128iData, i16Oper); //Arithmetic shift.
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
			m128iResult = _mm_setr_epi16(ut::BitReverse(i16Data[0]), ut::BitReverse(i16Data[1]), ut::BitReverse(i16Data[2]),
				ut::BitReverse(i16Data[3]), ut::BitReverse(i16Data[4]), ut::BitReverse(i16Data[5]), ut::BitReverse(i16Data[6]),
				ut::BitReverse(i16Data[7]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported int16_t operation.");
			break;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m128iResult = ut::ByteSwapVec<std::int16_t>(m128iResult);
		}

		_mm_storeu_si128(reinterpret_cast<__m128i*>(pi16Data), m128iResult);
		};

	constexpr auto lmbOperVec128UInt16 = [](std::uint16_t* pui16Data, const HEXMODIFY& hms) {
		const auto m128iData = hms.fBigEndian ?
			ut::ByteSwapVec<std::uint16_t>(_mm_loadu_si128(reinterpret_cast<const __m128i*>(pui16Data)))
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
		case OPER_MIN:
			m128iResult = _mm_max_epu16(m128iData, m128iOper);
			break;
		case OPER_MAX:
			m128iResult = _mm_min_epu16(m128iData, m128iOper);
			break;
		case OPER_SWAP:
			m128iResult = ut::ByteSwapVec<std::uint16_t>(m128iData);
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
			m128iResult = _mm_slli_epi16(m128iData, ui16Oper);
			break;
		case OPER_SHR:
			m128iResult = _mm_srli_epi16(m128iData, ui16Oper); //Logical shift.
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
			m128iResult = _mm_setr_epi16(ut::BitReverse(ui16Data[0]), ut::BitReverse(ui16Data[1]), ut::BitReverse(ui16Data[2]),
				ut::BitReverse(ui16Data[3]), ut::BitReverse(ui16Data[4]), ut::BitReverse(ui16Data[5]), ut::BitReverse(ui16Data[6]),
				ut::BitReverse(ui16Data[7]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported uint16_t operation.");
			break;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m128iResult = ut::ByteSwapVec<std::uint16_t>(m128iResult);
		}

		_mm_storeu_si128(reinterpret_cast<__m128i*>(pui16Data), m128iResult);
		};

	constexpr auto lmbOperVec128Int32 = [](std::int32_t* pi32Data, const HEXMODIFY& hms) {
		const auto m128iData = hms.fBigEndian ?
			ut::ByteSwapVec<std::int32_t>(_mm_loadu_si128(reinterpret_cast<const __m128i*>(pi32Data)))
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
		case OPER_MIN:
			m128iResult = _mm_max_epi32(m128iData, m128iOper);
			break;
		case OPER_MAX:
			m128iResult = _mm_min_epi32(m128iData, m128iOper);
			break;
		case OPER_SWAP:
			m128iResult = ut::ByteSwapVec<std::int32_t>(m128iData);
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
			m128iResult = _mm_slli_epi32(m128iData, i32Oper);
			break;
		case OPER_SHR:
			m128iResult = _mm_srai_epi32(m128iData, i32Oper); //Arithmetic shift.
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
			m128iResult = _mm_setr_epi32(ut::BitReverse(i32Data[0]), ut::BitReverse(i32Data[1]), ut::BitReverse(i32Data[2]),
				ut::BitReverse(i32Data[3]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported int32_t operation.");
			break;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m128iResult = ut::ByteSwapVec<std::int32_t>(m128iResult);
		}

		_mm_storeu_si128(reinterpret_cast<__m128i*>(pi32Data), m128iResult);
		};

	constexpr auto lmbOperVec128UInt32 = [](std::uint32_t* pui32Data, const HEXMODIFY& hms) {
		const auto m128iData = hms.fBigEndian ?
			ut::ByteSwapVec<std::uint32_t>(_mm_loadu_si128(reinterpret_cast<const __m128i*>(pui32Data)))
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
		case OPER_MIN:
			m128iResult = _mm_max_epu32(m128iData, m128iOper); //SSE4.1
			break;
		case OPER_MAX:
			m128iResult = _mm_min_epu32(m128iData, m128iOper); //SSE4.1
			break;
		case OPER_SWAP:
			m128iResult = ut::ByteSwapVec<std::uint32_t>(m128iData);
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
			m128iResult = _mm_slli_epi32(m128iData, ui32Oper);
			break;
		case OPER_SHR:
			m128iResult = _mm_srli_epi32(m128iData, ui32Oper); //Logical shift.
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
			m128iResult = _mm_setr_epi32(ut::BitReverse(ui32Data[0]), ut::BitReverse(ui32Data[1]), ut::BitReverse(ui32Data[2]),
				ut::BitReverse(ui32Data[3]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported uint32_t operation.");
			break;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m128iResult = ut::ByteSwapVec<std::uint32_t>(m128iResult);
		}

		_mm_storeu_si128(reinterpret_cast<__m128i*>(pui32Data), m128iResult);
		};

	constexpr auto lmbOperVec128Int64 = [](std::int64_t* pi64Data, const HEXMODIFY& hms) {
		const auto m128iData = hms.fBigEndian ?
			ut::ByteSwapVec<std::int64_t>(_mm_loadu_si128(reinterpret_cast<const __m128i*>(pi64Data)))
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
			m128iResult = _mm_setr_epi64x(i64Data[0] * i64Oper, i64Data[1] * i64Oper);
			break;
		case OPER_DIV:
			assert(i64Oper > 0);
			m128iResult = _mm_setr_epi64x(i64Data[0] / i64Oper, i64Data[1] / i64Oper);
			break;
		case OPER_MIN:
			m128iResult = _mm_setr_epi64x((std::max)(i64Data[0], i64Oper), (std::max)(i64Data[1], i64Oper));
			break;
		case OPER_MAX:
			m128iResult = _mm_setr_epi64x((std::min)(i64Data[0], i64Oper), (std::min)(i64Data[1], i64Oper));
			break;
		case OPER_SWAP:
			m128iResult = ut::ByteSwapVec<std::int64_t>(m128iData);
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
			m128iResult = _mm_slli_epi64(m128iData, static_cast<int>(i64Oper));
			break;
		case OPER_SHR:
			m128iResult = _mm_setr_epi64x(i64Data[0] >> i64Oper, i64Data[1] >> i64Oper);
			break;
		case OPER_ROTL:
			m128iResult = _mm_setr_epi64x(std::rotl(static_cast<std::uint64_t>(i64Data[0]), static_cast<const int>(i64Oper)),
				std::rotl(static_cast<std::uint64_t>(i64Data[1]), static_cast<const int>(i64Oper)));
			break;
		case OPER_ROTR:
			m128iResult = _mm_setr_epi64x(std::rotr(static_cast<std::uint64_t>(i64Data[0]), static_cast<const int>(i64Oper)),
				std::rotr(static_cast<std::uint64_t>(i64Data[1]), static_cast<const int>(i64Oper)));
			break;
		case OPER_BITREV:
			m128iResult = _mm_setr_epi64x(ut::BitReverse(i64Data[0]), ut::BitReverse(i64Data[1]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported int64_t operation.");
			break;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m128iResult = ut::ByteSwapVec<std::int64_t>(m128iResult);
		}

		_mm_storeu_si128(reinterpret_cast<__m128i*>(pi64Data), m128iResult);
		};

	constexpr auto lmbOperVec128UInt64 = [](std::uint64_t* pui64Data, const HEXMODIFY& hms) {
		const auto m128iData = hms.fBigEndian ?
			ut::ByteSwapVec<std::uint64_t>(_mm_loadu_si128(reinterpret_cast<const __m128i*>(pui64Data)))
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
			m128iResult = _mm_setr_epi64x(ui64Data[0] * ui64Oper, ui64Data[1] * ui64Oper);
			break;
		case OPER_DIV:
			assert(ui64Oper > 0);
			m128iResult = _mm_setr_epi64x(ui64Data[0] / ui64Oper, ui64Data[1] / ui64Oper);
			break;
		case OPER_MIN:
			m128iResult = _mm_setr_epi64x((std::max)(ui64Data[0], ui64Oper), (std::max)(ui64Data[1], ui64Oper));
			break;
		case OPER_MAX:
			m128iResult = _mm_setr_epi64x((std::min)(ui64Data[0], ui64Oper), (std::min)(ui64Data[1], ui64Oper));
			break;
		case OPER_SWAP:
			m128iResult = ut::ByteSwapVec<std::uint64_t>(m128iData);
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
			m128iResult = _mm_slli_epi64(m128iData, static_cast<int>(ui64Oper));
			break;
		case OPER_SHR:
			m128iResult = _mm_srli_epi64(m128iData, static_cast<int>(ui64Oper)); //Logical shift.
			break;
		case OPER_ROTL:
			m128iResult = _mm_setr_epi64x(std::rotl(ui64Data[0], static_cast<const int>(ui64Oper)),
				std::rotl(ui64Data[1], static_cast<const int>(ui64Oper)));
			break;
		case OPER_ROTR:
			m128iResult = _mm_setr_epi64x(std::rotr(ui64Data[0], static_cast<const int>(ui64Oper)),
				std::rotr(ui64Data[1], static_cast<const int>(ui64Oper)));
			break;
		case OPER_BITREV:
			m128iResult = _mm_setr_epi64x(ut::BitReverse(ui64Data[0]), ut::BitReverse(ui64Data[1]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported uint64_t operation.");
			break;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m128iResult = ut::ByteSwapVec<std::uint64_t>(m128iResult);
		}

		_mm_storeu_si128(reinterpret_cast<__m128i*>(pui64Data), m128iResult);
		};

	constexpr auto lmbOperVec128Float = [](float* pflData, const HEXMODIFY& hms) {
		const auto m128Data = hms.fBigEndian ? ut::ByteSwapVec<float>(_mm_loadu_ps(pflData)) : _mm_loadu_ps(pflData);
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
		case OPER_MIN:
			m128Result = _mm_max_ps(m128Data, m128Oper);
			break;
		case OPER_MAX:
			m128Result = _mm_min_ps(m128Data, m128Oper);
			break;
		case OPER_SWAP:
			m128Result = ut::ByteSwapVec<float>(m128Data);
			break;
		default:
			ut::DBG_REPORT(L"Unsupported float operation.");
			return;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m128Result = ut::ByteSwapVec<float>(m128Result);
		}

		_mm_storeu_ps(pflData, m128Result);
		};

	constexpr auto lmbOperVec128Double = [](double* pdblData, const HEXMODIFY& hms) {
		const auto m128dData = hms.fBigEndian ? ut::ByteSwapVec<double>(_mm_loadu_pd(pdblData)) : _mm_loadu_pd(pdblData);
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
		case OPER_MIN:
			m128dResult = _mm_max_pd(m128dData, m128dOper);
			break;
		case OPER_MAX:
			m128dResult = _mm_min_pd(m128dData, m128dOper);
			break;
		case OPER_SWAP:
			m128dResult = ut::ByteSwapVec<double>(m128dData);
			break;
		default:
			ut::DBG_REPORT(L"Unsupported double operation.");
			return;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m128dResult = ut::ByteSwapVec<double>(m128dResult);
		}

		_mm_storeu_pd(pdblData, m128dResult);
		};

	switch (hms.eDataType) {
	case DATA_INT8:
		lmbOperVec128Int8(reinterpret_cast<std::int8_t*>(pData), hms);
		break;
	case DATA_UINT8:
		lmbOperVec128UInt8(reinterpret_cast<std::uint8_t*>(pData), hms);
		break;
	case DATA_INT16:
		lmbOperVec128Int16(reinterpret_cast<std::int16_t*>(pData), hms);
		break;
	case DATA_UINT16:
		lmbOperVec128UInt16(reinterpret_cast<std::uint16_t*>(pData), hms);
		break;
	case DATA_INT32:
		lmbOperVec128Int32(reinterpret_cast<std::int32_t*>(pData), hms);
		break;
	case DATA_UINT32:
		lmbOperVec128UInt32(reinterpret_cast<std::uint32_t*>(pData), hms);
		break;
	case DATA_INT64:
		lmbOperVec128Int64(reinterpret_cast<std::int64_t*>(pData), hms);
		break;
	case DATA_UINT64:
		lmbOperVec128UInt64(reinterpret_cast<std::uint64_t*>(pData), hms);
		break;
	case DATA_FLOAT:
		lmbOperVec128Float(reinterpret_cast<float*>(pData), hms);
		break;
	case DATA_DOUBLE:
		lmbOperVec128Double(reinterpret_cast<double*>(pData), hms);
		break;
	default:
		break;
	}
}

void CHexCtrl::ModifyOperVec256(std::byte* pData, const HEXMODIFY& hms, [[maybe_unused]] SpanCByte)
{
	assert(pData != nullptr);
	using enum EHexDataType; using enum EHexOperMode;

	constexpr auto lmbOperVec256Int8 = [](std::int8_t* pi8Data, const HEXMODIFY& hms) {
		const auto m256iData = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pi8Data));
		alignas(32) std::int8_t i8Data[32];
		_mm256_store_si256(reinterpret_cast<__m256i*>(i8Data), m256iData);
		assert(!hms.spnData.empty());
		const auto i8Oper = *reinterpret_cast<const std::int8_t*>(hms.spnData.data());
		const auto m256iOper = _mm256_set1_epi8(i8Oper);
		__m256i m256iResult { };

		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m256iResult = _mm256_add_epi8(m256iData, m256iOper);
			break;
		case OPER_SUB:
			m256iResult = _mm256_sub_epi8(m256iData, m256iOper);
			break;
		case OPER_MUL:
		{
			const auto m256iEven = _mm256_mullo_epi16(m256iData, m256iOper);
			const auto m256iOdd = _mm256_mullo_epi16(_mm256_srli_epi16(m256iData, 8), _mm256_srli_epi16(m256iOper, 8));
			m256iResult = _mm256_or_si256(_mm256_slli_epi16(m256iOdd, 8), _mm256_srli_epi16(_mm256_slli_epi16(m256iEven, 8), 8));
		}
		break;
		case OPER_DIV:
			assert(i8Oper > 0);
			m256iResult = _mm256_setr_epi8(i8Data[0] / i8Oper, i8Data[1] / i8Oper, i8Data[2] / i8Oper, i8Data[3] / i8Oper,
				i8Data[4] / i8Oper, i8Data[5] / i8Oper, i8Data[6] / i8Oper, i8Data[7] / i8Oper, i8Data[8] / i8Oper,
				i8Data[9] / i8Oper, i8Data[10] / i8Oper, i8Data[11] / i8Oper, i8Data[12] / i8Oper, i8Data[13] / i8Oper,
				i8Data[14] / i8Oper, i8Data[15] / i8Oper, i8Data[16] / i8Oper, i8Data[17] / i8Oper, i8Data[18] / i8Oper,
				i8Data[19] / i8Oper, i8Data[20] / i8Oper, i8Data[21] / i8Oper, i8Data[22] / i8Oper, i8Data[23] / i8Oper,
				i8Data[24] / i8Oper, i8Data[25] / i8Oper, i8Data[26] / i8Oper, i8Data[27] / i8Oper, i8Data[28] / i8Oper,
				i8Data[29] / i8Oper, i8Data[30] / i8Oper, i8Data[31] / i8Oper);
			break;
		case OPER_MIN:
			m256iResult = _mm256_max_epi8(m256iData, m256iOper);
			break;
		case OPER_MAX:
			m256iResult = _mm256_min_epi8(m256iData, m256iOper);
			break;
		case OPER_SWAP: //No need for the int8_t.
			break;
		case OPER_OR:
			m256iResult = _mm256_or_si256(m256iData, m256iOper);
			break;
		case OPER_XOR:
			m256iResult = _mm256_xor_si256(m256iData, m256iOper);
			break;
		case OPER_AND:
			m256iResult = _mm256_and_si256(m256iData, m256iOper);
			break;
		case OPER_NOT:
			//_mm256_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
			m256iResult = _mm256_xor_si256(m256iData, _mm256_cmpeq_epi64(m256iData, m256iData));
			break;
		case OPER_SHL:
			m256iResult = _mm256_setr_epi8(i8Data[0] << i8Oper, i8Data[1] << i8Oper, i8Data[2] << i8Oper,
				i8Data[3] << i8Oper, i8Data[4] << i8Oper, i8Data[5] << i8Oper, i8Data[6] << i8Oper,
				i8Data[7] << i8Oper, i8Data[8] << i8Oper, i8Data[9] << i8Oper, i8Data[10] << i8Oper,
				i8Data[11] << i8Oper, i8Data[12] << i8Oper, i8Data[13] << i8Oper, i8Data[14] << i8Oper,
				i8Data[15] << i8Oper, i8Data[16] << i8Oper, i8Data[17] << i8Oper, i8Data[18] << i8Oper,
				i8Data[19] << i8Oper, i8Data[20] << i8Oper, i8Data[21] << i8Oper, i8Data[22] << i8Oper,
				i8Data[23] << i8Oper, i8Data[24] << i8Oper, i8Data[25] << i8Oper, i8Data[26] << i8Oper,
				i8Data[27] << i8Oper, i8Data[28] << i8Oper, i8Data[29] << i8Oper, i8Data[30] << i8Oper,
				i8Data[31] << i8Oper);
			break;
		case OPER_SHR:
			m256iResult = _mm256_setr_epi8(i8Data[0] >> i8Oper, i8Data[1] >> i8Oper, i8Data[2] >> i8Oper,
				i8Data[3] >> i8Oper, i8Data[4] >> i8Oper, i8Data[5] >> i8Oper, i8Data[6] >> i8Oper,
				i8Data[7] >> i8Oper, i8Data[8] >> i8Oper, i8Data[9] >> i8Oper, i8Data[10] >> i8Oper,
				i8Data[11] >> i8Oper, i8Data[12] >> i8Oper, i8Data[13] >> i8Oper, i8Data[14] >> i8Oper,
				i8Data[15] >> i8Oper, i8Data[16] >> i8Oper, i8Data[17] >> i8Oper, i8Data[18] >> i8Oper,
				i8Data[19] >> i8Oper, i8Data[20] >> i8Oper, i8Data[21] >> i8Oper, i8Data[22] >> i8Oper,
				i8Data[23] >> i8Oper, i8Data[24] >> i8Oper, i8Data[25] >> i8Oper, i8Data[26] >> i8Oper,
				i8Data[27] >> i8Oper, i8Data[28] >> i8Oper, i8Data[29] >> i8Oper, i8Data[30] >> i8Oper,
				i8Data[31] >> i8Oper);
			break;
		case OPER_ROTL:
			m256iResult = _mm256_setr_epi8(std::rotl(static_cast<std::uint8_t>(i8Data[0]), i8Oper),
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
				std::rotl(static_cast<std::uint8_t>(i8Data[15]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[16]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[17]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[18]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[19]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[20]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[21]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[22]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[23]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[24]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[25]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[26]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[27]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[28]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[29]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[30]), i8Oper),
				std::rotl(static_cast<std::uint8_t>(i8Data[31]), i8Oper));
			break;
		case OPER_ROTR:
			m256iResult = _mm256_setr_epi8(std::rotr(static_cast<std::uint8_t>(i8Data[0]), i8Oper),
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
				std::rotr(static_cast<std::uint8_t>(i8Data[15]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[16]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[17]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[18]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[19]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[20]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[21]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[22]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[23]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[24]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[25]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[26]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[27]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[28]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[29]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[30]), i8Oper),
				std::rotr(static_cast<std::uint8_t>(i8Data[31]), i8Oper));
			break;
		case OPER_BITREV:
			m256iResult = _mm256_setr_epi8(ut::BitReverse(i8Data[0]), ut::BitReverse(i8Data[1]), ut::BitReverse(i8Data[2]),
				ut::BitReverse(i8Data[3]), ut::BitReverse(i8Data[4]), ut::BitReverse(i8Data[5]), ut::BitReverse(i8Data[6]),
				ut::BitReverse(i8Data[7]), ut::BitReverse(i8Data[8]), ut::BitReverse(i8Data[9]), ut::BitReverse(i8Data[10]),
				ut::BitReverse(i8Data[11]), ut::BitReverse(i8Data[12]), ut::BitReverse(i8Data[13]), ut::BitReverse(i8Data[14]),
				ut::BitReverse(i8Data[15]), ut::BitReverse(i8Data[16]), ut::BitReverse(i8Data[17]), ut::BitReverse(i8Data[18]),
				ut::BitReverse(i8Data[19]), ut::BitReverse(i8Data[20]), ut::BitReverse(i8Data[21]), ut::BitReverse(i8Data[22]),
				ut::BitReverse(i8Data[23]), ut::BitReverse(i8Data[24]), ut::BitReverse(i8Data[25]), ut::BitReverse(i8Data[26]),
				ut::BitReverse(i8Data[27]), ut::BitReverse(i8Data[28]), ut::BitReverse(i8Data[29]), ut::BitReverse(i8Data[30]),
				ut::BitReverse(i8Data[31]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported int8_t operation.");
			break;
		}

		_mm256_storeu_si256(reinterpret_cast<__m256i*>(pi8Data), m256iResult);
		};

	constexpr auto lmbOperVec256UInt8 = [](std::uint8_t* pui8Data, const HEXMODIFY& hms) {
		const auto m256iData = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pui8Data));
		alignas(32) std::uint8_t ui8Data[32];
		_mm256_store_si256(reinterpret_cast<__m256i*>(ui8Data), m256iData);
		assert(!hms.spnData.empty());
		const auto ui8Oper = *reinterpret_cast<const std::uint8_t*>(hms.spnData.data());
		const auto m256iOper = _mm256_set1_epi8(ui8Oper);
		__m256i m256iResult { };

		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m256iResult = _mm256_add_epi8(m256iData, m256iOper);
			break;
		case OPER_SUB:
			m256iResult = _mm256_sub_epi8(m256iData, m256iOper);
			break;
		case OPER_MUL:
			m256iResult = _mm256_setr_epi8(ui8Data[0] * ui8Oper, ui8Data[1] * ui8Oper, ui8Data[2] * ui8Oper,
				ui8Data[3] * ui8Oper, ui8Data[4] * ui8Oper, ui8Data[5] * ui8Oper, ui8Data[6] * ui8Oper,
				ui8Data[7] * ui8Oper, ui8Data[8] * ui8Oper, ui8Data[9] * ui8Oper, ui8Data[10] * ui8Oper,
				ui8Data[11] * ui8Oper, ui8Data[12] * ui8Oper, ui8Data[13] * ui8Oper, ui8Data[14] * ui8Oper,
				ui8Data[15] * ui8Oper, ui8Data[16] * ui8Oper, ui8Data[17] * ui8Oper, ui8Data[18] * ui8Oper,
				ui8Data[19] * ui8Oper, ui8Data[20] * ui8Oper, ui8Data[21] * ui8Oper, ui8Data[22] * ui8Oper,
				ui8Data[23] * ui8Oper, ui8Data[24] * ui8Oper, ui8Data[25] * ui8Oper, ui8Data[26] * ui8Oper,
				ui8Data[27] * ui8Oper, ui8Data[28] * ui8Oper, ui8Data[29] * ui8Oper, ui8Data[30] * ui8Oper,
				ui8Data[31] * ui8Oper);
			break;
		case OPER_DIV:
			assert(ui8Oper > 0);
			m256iResult = _mm256_setr_epi8(ui8Data[0] / ui8Oper, ui8Data[1] / ui8Oper, ui8Data[2] / ui8Oper,
				ui8Data[3] / ui8Oper, ui8Data[4] / ui8Oper, ui8Data[5] / ui8Oper, ui8Data[6] / ui8Oper,
				ui8Data[7] / ui8Oper, ui8Data[8] / ui8Oper, ui8Data[9] / ui8Oper, ui8Data[10] / ui8Oper,
				ui8Data[11] / ui8Oper, ui8Data[12] / ui8Oper, ui8Data[13] / ui8Oper, ui8Data[14] / ui8Oper,
				ui8Data[15] / ui8Oper, ui8Data[16] / ui8Oper, ui8Data[17] / ui8Oper, ui8Data[18] / ui8Oper,
				ui8Data[19] / ui8Oper, ui8Data[20] / ui8Oper, ui8Data[21] / ui8Oper, ui8Data[22] / ui8Oper,
				ui8Data[23] / ui8Oper, ui8Data[24] / ui8Oper, ui8Data[25] / ui8Oper, ui8Data[26] / ui8Oper,
				ui8Data[27] / ui8Oper, ui8Data[28] / ui8Oper, ui8Data[29] / ui8Oper, ui8Data[30] / ui8Oper,
				ui8Data[31] / ui8Oper);
			break;
		case OPER_MIN:
			m256iResult = _mm256_max_epu8(m256iData, m256iOper);
			break;
		case OPER_MAX:
			m256iResult = _mm256_min_epu8(m256iData, m256iOper);
			break;
		case OPER_SWAP:	//No need for the uint8_t.
			break;
		case OPER_OR:
			m256iResult = _mm256_or_si256(m256iData, m256iOper);
			break;
		case OPER_XOR:
			m256iResult = _mm256_xor_si256(m256iData, m256iOper);
			break;
		case OPER_AND:
			m256iResult = _mm256_and_si256(m256iData, m256iOper);
			break;
		case OPER_NOT:
			//_mm256_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
			m256iResult = _mm256_xor_si256(m256iData, _mm256_cmpeq_epi64(m256iData, m256iData));
			break;
		case OPER_SHL:
			m256iResult = _mm256_setr_epi8(ui8Data[0] << ui8Oper, ui8Data[1] << ui8Oper, ui8Data[2] << ui8Oper,
				ui8Data[3] << ui8Oper, ui8Data[4] << ui8Oper, ui8Data[5] << ui8Oper, ui8Data[6] << ui8Oper,
				ui8Data[7] << ui8Oper, ui8Data[8] << ui8Oper, ui8Data[9] << ui8Oper, ui8Data[10] << ui8Oper,
				ui8Data[11] << ui8Oper, ui8Data[12] << ui8Oper, ui8Data[13] << ui8Oper, ui8Data[14] << ui8Oper,
				ui8Data[15] << ui8Oper, ui8Data[16] << ui8Oper, ui8Data[17] << ui8Oper, ui8Data[18] << ui8Oper,
				ui8Data[19] << ui8Oper, ui8Data[20] << ui8Oper, ui8Data[21] << ui8Oper, ui8Data[22] << ui8Oper,
				ui8Data[23] << ui8Oper, ui8Data[24] << ui8Oper, ui8Data[25] << ui8Oper, ui8Data[26] << ui8Oper,
				ui8Data[27] << ui8Oper, ui8Data[28] << ui8Oper, ui8Data[29] << ui8Oper, ui8Data[30] << ui8Oper,
				ui8Data[31] << ui8Oper);
			break;
		case OPER_SHR:
			m256iResult = _mm256_setr_epi8(ui8Data[0] >> ui8Oper, ui8Data[1] >> ui8Oper, ui8Data[2] >> ui8Oper,
				ui8Data[3] >> ui8Oper, ui8Data[4] >> ui8Oper, ui8Data[5] >> ui8Oper, ui8Data[6] >> ui8Oper,
				ui8Data[7] >> ui8Oper, ui8Data[8] >> ui8Oper, ui8Data[9] >> ui8Oper, ui8Data[10] >> ui8Oper,
				ui8Data[11] >> ui8Oper, ui8Data[12] >> ui8Oper, ui8Data[13] >> ui8Oper, ui8Data[14] >> ui8Oper,
				ui8Data[15] >> ui8Oper, ui8Data[16] >> ui8Oper, ui8Data[17] >> ui8Oper, ui8Data[18] >> ui8Oper,
				ui8Data[19] >> ui8Oper, ui8Data[20] >> ui8Oper, ui8Data[21] >> ui8Oper, ui8Data[22] >> ui8Oper,
				ui8Data[23] >> ui8Oper, ui8Data[24] >> ui8Oper, ui8Data[25] >> ui8Oper, ui8Data[26] >> ui8Oper,
				ui8Data[27] >> ui8Oper, ui8Data[28] >> ui8Oper, ui8Data[29] >> ui8Oper, ui8Data[30] >> ui8Oper,
				ui8Data[31] >> ui8Oper);
			break;
		case OPER_ROTL:
			m256iResult = _mm256_setr_epi8(std::rotl(ui8Data[0], ui8Oper), std::rotl(ui8Data[1], ui8Oper),
				std::rotl(ui8Data[2], ui8Oper), std::rotl(ui8Data[3], ui8Oper), std::rotl(ui8Data[4], ui8Oper),
				std::rotl(ui8Data[5], ui8Oper), std::rotl(ui8Data[6], ui8Oper), std::rotl(ui8Data[7], ui8Oper),
				std::rotl(ui8Data[8], ui8Oper), std::rotl(ui8Data[9], ui8Oper), std::rotl(ui8Data[10], ui8Oper),
				std::rotl(ui8Data[11], ui8Oper), std::rotl(ui8Data[12], ui8Oper), std::rotl(ui8Data[13], ui8Oper),
				std::rotl(ui8Data[14], ui8Oper), std::rotl(ui8Data[15], ui8Oper), std::rotl(ui8Data[16], ui8Oper),
				std::rotl(ui8Data[17], ui8Oper), std::rotl(ui8Data[18], ui8Oper), std::rotl(ui8Data[19], ui8Oper),
				std::rotl(ui8Data[20], ui8Oper), std::rotl(ui8Data[21], ui8Oper), std::rotl(ui8Data[22], ui8Oper),
				std::rotl(ui8Data[23], ui8Oper), std::rotl(ui8Data[24], ui8Oper), std::rotl(ui8Data[25], ui8Oper),
				std::rotl(ui8Data[26], ui8Oper), std::rotl(ui8Data[27], ui8Oper), std::rotl(ui8Data[28], ui8Oper),
				std::rotl(ui8Data[29], ui8Oper), std::rotl(ui8Data[30], ui8Oper), std::rotl(ui8Data[31], ui8Oper));
			break;
		case OPER_ROTR:
			m256iResult = _mm256_setr_epi8(std::rotr(ui8Data[0], ui8Oper), std::rotr(ui8Data[1], ui8Oper),
				std::rotr(ui8Data[2], ui8Oper), std::rotr(ui8Data[3], ui8Oper), std::rotr(ui8Data[4], ui8Oper),
				std::rotr(ui8Data[5], ui8Oper), std::rotr(ui8Data[6], ui8Oper), std::rotr(ui8Data[7], ui8Oper),
				std::rotr(ui8Data[8], ui8Oper), std::rotr(ui8Data[9], ui8Oper), std::rotr(ui8Data[10], ui8Oper),
				std::rotr(ui8Data[11], ui8Oper), std::rotr(ui8Data[12], ui8Oper), std::rotr(ui8Data[13], ui8Oper),
				std::rotr(ui8Data[14], ui8Oper), std::rotr(ui8Data[15], ui8Oper), std::rotr(ui8Data[16], ui8Oper),
				std::rotr(ui8Data[17], ui8Oper), std::rotr(ui8Data[18], ui8Oper), std::rotr(ui8Data[19], ui8Oper),
				std::rotr(ui8Data[20], ui8Oper), std::rotr(ui8Data[21], ui8Oper), std::rotr(ui8Data[22], ui8Oper),
				std::rotr(ui8Data[23], ui8Oper), std::rotr(ui8Data[24], ui8Oper), std::rotr(ui8Data[25], ui8Oper),
				std::rotr(ui8Data[26], ui8Oper), std::rotr(ui8Data[27], ui8Oper), std::rotr(ui8Data[28], ui8Oper),
				std::rotr(ui8Data[29], ui8Oper), std::rotr(ui8Data[30], ui8Oper), std::rotr(ui8Data[31], ui8Oper));
			break;
		case OPER_BITREV:
			m256iResult = _mm256_setr_epi8(ut::BitReverse(ui8Data[0]), ut::BitReverse(ui8Data[1]), ut::BitReverse(ui8Data[2]),
				ut::BitReverse(ui8Data[3]), ut::BitReverse(ui8Data[4]), ut::BitReverse(ui8Data[5]), ut::BitReverse(ui8Data[6]),
				ut::BitReverse(ui8Data[7]), ut::BitReverse(ui8Data[8]), ut::BitReverse(ui8Data[9]), ut::BitReverse(ui8Data[10]),
				ut::BitReverse(ui8Data[11]), ut::BitReverse(ui8Data[12]), ut::BitReverse(ui8Data[13]), ut::BitReverse(ui8Data[14]),
				ut::BitReverse(ui8Data[15]), ut::BitReverse(ui8Data[16]), ut::BitReverse(ui8Data[17]), ut::BitReverse(ui8Data[18]),
				ut::BitReverse(ui8Data[19]), ut::BitReverse(ui8Data[20]), ut::BitReverse(ui8Data[21]), ut::BitReverse(ui8Data[22]),
				ut::BitReverse(ui8Data[23]), ut::BitReverse(ui8Data[24]), ut::BitReverse(ui8Data[25]), ut::BitReverse(ui8Data[26]),
				ut::BitReverse(ui8Data[27]), ut::BitReverse(ui8Data[28]), ut::BitReverse(ui8Data[29]), ut::BitReverse(ui8Data[30]),
				ut::BitReverse(ui8Data[31]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported uint8_t operation.");
			break;
		}

		_mm256_storeu_si256(reinterpret_cast<__m256i*>(pui8Data), m256iResult);
		};

	constexpr auto lmbOperVec256Int16 = [](std::int16_t* pi16Data, const HEXMODIFY& hms) {
		const auto m256iData = hms.fBigEndian ?
			ut::ByteSwapVec<std::int16_t>(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(pi16Data)))
			: _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pi16Data));
		alignas(32) std::int16_t i16Data[16];
		_mm256_store_si256(reinterpret_cast<__m256i*>(i16Data), m256iData);
		assert(!hms.spnData.empty());
		const auto i16Oper = *reinterpret_cast<const std::int16_t*>(hms.spnData.data());
		const auto m256iOper = _mm256_set1_epi16(i16Oper);
		__m256i m256iResult { };

		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m256iResult = _mm256_add_epi16(m256iData, m256iOper);
			break;
		case OPER_SUB:
			m256iResult = _mm256_sub_epi16(m256iData, m256iOper);
			break;
		case OPER_MUL:
			m256iResult = _mm256_mullo_epi16(m256iData, m256iOper);
			break;
		case OPER_DIV:
			assert(i16Oper > 0);
			m256iResult = _mm256_setr_epi16(i16Data[0] / i16Oper, i16Data[1] / i16Oper, i16Data[2] / i16Oper,
				i16Data[3] / i16Oper, i16Data[4] / i16Oper, i16Data[5] / i16Oper, i16Data[6] / i16Oper,
				i16Data[7] / i16Oper, i16Data[8] / i16Oper, i16Data[9] / i16Oper, i16Data[10] / i16Oper,
				i16Data[11] / i16Oper, i16Data[12] / i16Oper, i16Data[13] / i16Oper, i16Data[14] / i16Oper,
				i16Data[15] / i16Oper);
			break;
		case OPER_MIN:
			m256iResult = _mm256_max_epi16(m256iData, m256iOper);
			break;
		case OPER_MAX:
			m256iResult = _mm256_min_epi16(m256iData, m256iOper);
			break;
		case OPER_SWAP:
			m256iResult = ut::ByteSwapVec<std::int16_t>(m256iData);
			break;
		case OPER_OR:
			m256iResult = _mm256_or_si256(m256iData, m256iOper);
			break;
		case OPER_XOR:
			m256iResult = _mm256_xor_si256(m256iData, m256iOper);
			break;
		case OPER_AND:
			m256iResult = _mm256_and_si256(m256iData, m256iOper);
			break;
		case OPER_NOT:
			//_mm256_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
			m256iResult = _mm256_xor_si256(m256iData, _mm256_cmpeq_epi64(m256iData, m256iData));
			break;
		case OPER_SHL:
			m256iResult = _mm256_slli_epi16(m256iData, i16Oper);
			break;
		case OPER_SHR:
			m256iResult = _mm256_srai_epi16(m256iData, i16Oper); //Arithmetic shift.
			break;
		case OPER_ROTL:
			m256iResult = _mm256_setr_epi16(std::rotl(static_cast<std::uint16_t>(i16Data[0]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[1]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[2]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[3]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[4]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[5]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[6]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[7]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[8]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[9]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[10]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[11]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[12]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[13]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[14]), i16Oper),
				std::rotl(static_cast<std::uint16_t>(i16Data[15]), i16Oper));
			break;
		case OPER_ROTR:
			m256iResult = _mm256_setr_epi16(std::rotr(static_cast<std::uint16_t>(i16Data[0]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[1]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[2]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[3]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[4]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[5]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[6]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[7]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[8]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[9]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[10]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[11]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[12]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[13]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[14]), i16Oper),
				std::rotr(static_cast<std::uint16_t>(i16Data[15]), i16Oper));
			break;
		case OPER_BITREV:
			m256iResult = _mm256_setr_epi16(ut::BitReverse(i16Data[0]), ut::BitReverse(i16Data[1]), ut::BitReverse(i16Data[2]),
				ut::BitReverse(i16Data[3]), ut::BitReverse(i16Data[4]), ut::BitReverse(i16Data[5]), ut::BitReverse(i16Data[6]),
				ut::BitReverse(i16Data[7]), ut::BitReverse(i16Data[8]), ut::BitReverse(i16Data[9]), ut::BitReverse(i16Data[10]),
				ut::BitReverse(i16Data[11]), ut::BitReverse(i16Data[12]), ut::BitReverse(i16Data[13]), ut::BitReverse(i16Data[14]),
				ut::BitReverse(i16Data[15]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported int16_t operation.");
			break;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m256iResult = ut::ByteSwapVec<std::int16_t>(m256iResult);
		}

		_mm256_storeu_si256(reinterpret_cast<__m256i*>(pi16Data), m256iResult);
		};

	constexpr auto lmbOperVec256UInt16 = [](std::uint16_t* pui16Data, const HEXMODIFY& hms) {
		const auto m256iData = hms.fBigEndian ?
			ut::ByteSwapVec<std::uint16_t>(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(pui16Data)))
			: _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pui16Data));
		alignas(32) std::uint16_t ui16Data[16];
		_mm256_store_si256(reinterpret_cast<__m256i*>(ui16Data), m256iData);
		assert(!hms.spnData.empty());
		const auto ui16Oper = *reinterpret_cast<const std::uint16_t*>(hms.spnData.data());
		const auto m256iOper = _mm256_set1_epi16(ui16Oper);
		__m256i m256iResult { };

		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m256iResult = _mm256_add_epi16(m256iData, m256iOper);
			break;
		case OPER_SUB:
			m256iResult = _mm256_sub_epi16(m256iData, m256iOper);
			break;
		case OPER_MUL:
			m256iResult = _mm256_setr_epi16(ui16Data[0] * ui16Oper, ui16Data[1] * ui16Oper, ui16Data[2] * ui16Oper,
				ui16Data[3] * ui16Oper, ui16Data[4] * ui16Oper, ui16Data[5] * ui16Oper, ui16Data[6] * ui16Oper,
				ui16Data[7] * ui16Oper, ui16Data[8] * ui16Oper, ui16Data[9] * ui16Oper, ui16Data[10] * ui16Oper,
				ui16Data[11] * ui16Oper, ui16Data[12] * ui16Oper, ui16Data[13] * ui16Oper, ui16Data[14] * ui16Oper,
				ui16Data[15] * ui16Oper);
			break;
		case OPER_DIV:
			assert(ui16Oper > 0);
			m256iResult = _mm256_setr_epi16(ui16Data[0] / ui16Oper, ui16Data[1] / ui16Oper, ui16Data[2] / ui16Oper,
				ui16Data[3] / ui16Oper, ui16Data[4] / ui16Oper, ui16Data[5] / ui16Oper, ui16Data[6] / ui16Oper,
				ui16Data[7] / ui16Oper, ui16Data[8] / ui16Oper, ui16Data[9] / ui16Oper, ui16Data[10] / ui16Oper,
				ui16Data[11] / ui16Oper, ui16Data[12] / ui16Oper, ui16Data[13] / ui16Oper, ui16Data[14] / ui16Oper,
				ui16Data[15] / ui16Oper);
			break;
		case OPER_MIN:
			m256iResult = _mm256_max_epu16(m256iData, m256iOper);
			break;
		case OPER_MAX:
			m256iResult = _mm256_min_epu16(m256iData, m256iOper);
			break;
		case OPER_SWAP:
			m256iResult = ut::ByteSwapVec<std::uint16_t>(m256iData);
			break;
		case OPER_OR:
			m256iResult = _mm256_or_si256(m256iData, m256iOper);
			break;
		case OPER_XOR:
			m256iResult = _mm256_xor_si256(m256iData, m256iOper);
			break;
		case OPER_AND:
			m256iResult = _mm256_and_si256(m256iData, m256iOper);
			break;
		case OPER_NOT:
			//_mm256_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
			m256iResult = _mm256_xor_si256(m256iData, _mm256_cmpeq_epi64(m256iData, m256iData));
			break;
		case OPER_SHL:
			m256iResult = _mm256_slli_epi16(m256iData, ui16Oper);
			break;
		case OPER_SHR:
			m256iResult = _mm256_srli_epi16(m256iData, ui16Oper); //Logical shift.
			break;
		case OPER_ROTL:
			m256iResult = _mm256_setr_epi16(std::rotl(ui16Data[0], ui16Oper), std::rotl(ui16Data[1], ui16Oper),
				std::rotl(ui16Data[2], ui16Oper), std::rotl(ui16Data[3], ui16Oper), std::rotl(ui16Data[4], ui16Oper),
				std::rotl(ui16Data[5], ui16Oper), std::rotl(ui16Data[6], ui16Oper), std::rotl(ui16Data[7], ui16Oper),
				std::rotl(ui16Data[8], ui16Oper), std::rotl(ui16Data[9], ui16Oper), std::rotl(ui16Data[10], ui16Oper),
				std::rotl(ui16Data[11], ui16Oper), std::rotl(ui16Data[12], ui16Oper), std::rotl(ui16Data[13], ui16Oper),
				std::rotl(ui16Data[14], ui16Oper), std::rotl(ui16Data[15], ui16Oper));
			break;
		case OPER_ROTR:
			m256iResult = _mm256_setr_epi16(std::rotr(ui16Data[0], ui16Oper), std::rotr(ui16Data[1], ui16Oper),
				std::rotr(ui16Data[2], ui16Oper), std::rotr(ui16Data[3], ui16Oper), std::rotr(ui16Data[4], ui16Oper),
				std::rotr(ui16Data[5], ui16Oper), std::rotr(ui16Data[6], ui16Oper), std::rotr(ui16Data[7], ui16Oper),
				std::rotr(ui16Data[8], ui16Oper), std::rotr(ui16Data[9], ui16Oper), std::rotr(ui16Data[10], ui16Oper),
				std::rotr(ui16Data[11], ui16Oper), std::rotr(ui16Data[12], ui16Oper), std::rotr(ui16Data[13], ui16Oper),
				std::rotr(ui16Data[14], ui16Oper), std::rotr(ui16Data[15], ui16Oper));
			break;
		case OPER_BITREV:
			m256iResult = _mm256_setr_epi16(ut::BitReverse(ui16Data[0]), ut::BitReverse(ui16Data[1]), ut::BitReverse(ui16Data[2]),
				ut::BitReverse(ui16Data[3]), ut::BitReverse(ui16Data[4]), ut::BitReverse(ui16Data[5]), ut::BitReverse(ui16Data[6]),
				ut::BitReverse(ui16Data[7]), ut::BitReverse(ui16Data[8]), ut::BitReverse(ui16Data[9]), ut::BitReverse(ui16Data[10]),
				ut::BitReverse(ui16Data[11]), ut::BitReverse(ui16Data[12]), ut::BitReverse(ui16Data[13]), ut::BitReverse(ui16Data[14]),
				ut::BitReverse(ui16Data[15]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported uint16_t operation.");
			break;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m256iResult = ut::ByteSwapVec<std::uint16_t>(m256iResult);
		}

		_mm256_storeu_si256(reinterpret_cast<__m256i*>(pui16Data), m256iResult);
		};

	constexpr auto lmbOperVec256Int32 = [](std::int32_t* pi32Data, const HEXMODIFY& hms) {
		const auto m256iData = hms.fBigEndian ?
			ut::ByteSwapVec<std::int32_t>(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(pi32Data)))
			: _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pi32Data));
		alignas(32) std::int32_t i32Data[8];
		_mm256_store_si256(reinterpret_cast<__m256i*>(i32Data), m256iData);
		assert(!hms.spnData.empty());
		const auto i32Oper = *reinterpret_cast<const std::int32_t*>(hms.spnData.data());
		const auto m256iOper = _mm256_set1_epi32(i32Oper);
		__m256i m256iResult { };

		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m256iResult = _mm256_add_epi32(m256iData, m256iOper);
			break;
		case OPER_SUB:
			m256iResult = _mm256_sub_epi32(m256iData, m256iOper);
			break;
		case OPER_MUL:
			m256iResult = _mm256_mullo_epi32(m256iData, m256iOper);
			break;
		case OPER_DIV:
			assert(i32Oper > 0);
			m256iResult = _mm256_setr_epi32(i32Data[0] / i32Oper, i32Data[1] / i32Oper, i32Data[2] / i32Oper,
				i32Data[3] / i32Oper, i32Data[4] / i32Oper, i32Data[5] / i32Oper, i32Data[6] / i32Oper,
				i32Data[7] / i32Oper);
			break;
		case OPER_MIN:
			m256iResult = _mm256_max_epi32(m256iData, m256iOper);
			break;
		case OPER_MAX:
			m256iResult = _mm256_min_epi32(m256iData, m256iOper);
			break;
		case OPER_SWAP:
			m256iResult = ut::ByteSwapVec<std::int32_t>(m256iData);
			break;
		case OPER_OR:
			m256iResult = _mm256_or_si256(m256iData, m256iOper);
			break;
		case OPER_XOR:
			m256iResult = _mm256_xor_si256(m256iData, m256iOper);
			break;
		case OPER_AND:
			m256iResult = _mm256_and_si256(m256iData, m256iOper);
			break;
		case OPER_NOT:
			//_mm256_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
			m256iResult = _mm256_xor_si256(m256iData, _mm256_cmpeq_epi64(m256iData, m256iData));
			break;
		case OPER_SHL:
			m256iResult = _mm256_slli_epi32(m256iData, i32Oper);
			break;
		case OPER_SHR:
			m256iResult = _mm256_srai_epi32(m256iData, i32Oper); //Arithmetic shift.
			break;
		case OPER_ROTL:
			m256iResult = _mm256_setr_epi32(std::rotl(static_cast<std::uint32_t>(i32Data[0]), i32Oper),
				std::rotl(static_cast<std::uint32_t>(i32Data[1]), i32Oper),
				std::rotl(static_cast<std::uint32_t>(i32Data[2]), i32Oper),
				std::rotl(static_cast<std::uint32_t>(i32Data[3]), i32Oper),
				std::rotl(static_cast<std::uint32_t>(i32Data[4]), i32Oper),
				std::rotl(static_cast<std::uint32_t>(i32Data[5]), i32Oper),
				std::rotl(static_cast<std::uint32_t>(i32Data[6]), i32Oper),
				std::rotl(static_cast<std::uint32_t>(i32Data[7]), i32Oper));
			break;
		case OPER_ROTR:
			m256iResult = _mm256_setr_epi32(std::rotr(static_cast<std::uint32_t>(i32Data[0]), i32Oper),
				std::rotr(static_cast<std::uint32_t>(i32Data[1]), i32Oper),
				std::rotr(static_cast<std::uint32_t>(i32Data[2]), i32Oper),
				std::rotr(static_cast<std::uint32_t>(i32Data[3]), i32Oper),
				std::rotr(static_cast<std::uint32_t>(i32Data[4]), i32Oper),
				std::rotr(static_cast<std::uint32_t>(i32Data[5]), i32Oper),
				std::rotr(static_cast<std::uint32_t>(i32Data[6]), i32Oper),
				std::rotr(static_cast<std::uint32_t>(i32Data[7]), i32Oper));
			break;
		case OPER_BITREV:
			m256iResult = _mm256_setr_epi32(ut::BitReverse(i32Data[0]), ut::BitReverse(i32Data[1]), ut::BitReverse(i32Data[2]),
				ut::BitReverse(i32Data[3]), ut::BitReverse(i32Data[4]), ut::BitReverse(i32Data[5]), ut::BitReverse(i32Data[6]),
				ut::BitReverse(i32Data[7]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported int32_t operation.");
			break;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m256iResult = ut::ByteSwapVec<std::int32_t>(m256iResult);
		}

		_mm256_storeu_si256(reinterpret_cast<__m256i*>(pi32Data), m256iResult);
		};

	constexpr auto lmbOperVec256UInt32 = [](std::uint32_t* pui32Data, const HEXMODIFY& hms) {
		const auto m256iData = hms.fBigEndian ?
			ut::ByteSwapVec<std::uint32_t>(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(pui32Data)))
			: _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pui32Data));
		alignas(32) std::uint32_t ui32Data[8];
		_mm256_store_si256(reinterpret_cast<__m256i*>(ui32Data), m256iData);
		assert(!hms.spnData.empty());
		const auto ui32Oper = *reinterpret_cast<const std::uint32_t*>(hms.spnData.data());
		const auto m256iOper = _mm256_set1_epi32(ui32Oper);
		__m256i m256iResult { };

		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m256iResult = _mm256_add_epi32(m256iData, m256iOper);
			break;
		case OPER_SUB:
			m256iResult = _mm256_sub_epi32(m256iData, m256iOper);
			break;
		case OPER_MUL:
			m256iResult = _mm256_setr_epi32(ui32Data[0] * ui32Oper, ui32Data[1] * ui32Oper, ui32Data[2] * ui32Oper,
				ui32Data[3] * ui32Oper, ui32Data[4] * ui32Oper, ui32Data[5] * ui32Oper, ui32Data[6] * ui32Oper,
				ui32Data[7] * ui32Oper);
			break;
		case OPER_DIV:
			assert(ui32Oper > 0);
			m256iResult = _mm256_setr_epi32(ui32Data[0] / ui32Oper, ui32Data[1] / ui32Oper, ui32Data[2] / ui32Oper,
				ui32Data[3] / ui32Oper, ui32Data[4] / ui32Oper, ui32Data[5] / ui32Oper, ui32Data[6] / ui32Oper,
				ui32Data[7] / ui32Oper);
			break;
		case OPER_MIN:
			m256iResult = _mm256_max_epu32(m256iData, m256iOper);
			break;
		case OPER_MAX:
			m256iResult = _mm256_min_epu32(m256iData, m256iOper);
			break;
		case OPER_SWAP:
			m256iResult = ut::ByteSwapVec<std::uint32_t>(m256iData);
			break;
		case OPER_OR:
			m256iResult = _mm256_or_si256(m256iData, m256iOper);
			break;
		case OPER_XOR:
			m256iResult = _mm256_xor_si256(m256iData, m256iOper);
			break;
		case OPER_AND:
			m256iResult = _mm256_and_si256(m256iData, m256iOper);
			break;
		case OPER_NOT:
			//_mm256_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
			m256iResult = _mm256_xor_si256(m256iData, _mm256_cmpeq_epi64(m256iData, m256iData));
			break;
		case OPER_SHL:
			m256iResult = _mm256_slli_epi32(m256iData, ui32Oper);
			break;
		case OPER_SHR:
			m256iResult = _mm256_srli_epi32(m256iData, ui32Oper); //Logical shift.
			break;
		case OPER_ROTL:
			m256iResult = _mm256_setr_epi32(std::rotl(ui32Data[0], ui32Oper), std::rotl(ui32Data[1], ui32Oper),
				std::rotl(ui32Data[2], ui32Oper), std::rotl(ui32Data[3], ui32Oper), std::rotl(ui32Data[4], ui32Oper),
				std::rotl(ui32Data[5], ui32Oper), std::rotl(ui32Data[6], ui32Oper), std::rotl(ui32Data[7], ui32Oper));
			break;
		case OPER_ROTR:
			m256iResult = _mm256_setr_epi32(std::rotr(ui32Data[0], ui32Oper), std::rotr(ui32Data[1], ui32Oper),
				std::rotr(ui32Data[2], ui32Oper), std::rotr(ui32Data[3], ui32Oper), std::rotr(ui32Data[4], ui32Oper),
				std::rotr(ui32Data[5], ui32Oper), std::rotr(ui32Data[6], ui32Oper), std::rotr(ui32Data[7], ui32Oper));
			break;
		case OPER_BITREV:
			m256iResult = _mm256_setr_epi32(ut::BitReverse(ui32Data[0]), ut::BitReverse(ui32Data[1]), ut::BitReverse(ui32Data[2]),
				ut::BitReverse(ui32Data[3]), ut::BitReverse(ui32Data[4]), ut::BitReverse(ui32Data[5]), ut::BitReverse(ui32Data[6]),
				ut::BitReverse(ui32Data[7]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported uint32_t operation.");
			break;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m256iResult = ut::ByteSwapVec<std::uint32_t>(m256iResult);
		}

		_mm256_storeu_si256(reinterpret_cast<__m256i*>(pui32Data), m256iResult);
		};

	constexpr auto lmbOperVec256Int64 = [](std::int64_t* pi64Data, const HEXMODIFY& hms) {
		const auto m256iData = hms.fBigEndian ?
			ut::ByteSwapVec<std::int64_t>(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(pi64Data)))
			: _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pi64Data));
		alignas(32) std::int64_t i64Data[4];
		_mm256_store_si256(reinterpret_cast<__m256i*>(i64Data), m256iData);
		assert(!hms.spnData.empty());
		const auto i64Oper = *reinterpret_cast<const std::int64_t*>(hms.spnData.data());
		const auto m256iOper = _mm256_set1_epi64x(i64Oper);
		__m256i m256iResult { };

		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m256iResult = _mm256_add_epi64(m256iData, m256iOper);
			break;
		case OPER_SUB:
			m256iResult = _mm256_sub_epi64(m256iData, m256iOper);
			break;
		case OPER_MUL:
			m256iResult = _mm256_setr_epi64x(i64Data[0] * i64Oper, i64Data[1] * i64Oper, i64Data[2] * i64Oper,
				i64Data[3] * i64Oper);
			break;
		case OPER_DIV:
			assert(i64Oper > 0);
			m256iResult = _mm256_setr_epi64x(i64Data[0] / i64Oper, i64Data[1] / i64Oper, i64Data[2] / i64Oper,
				i64Data[3] / i64Oper);
			break;
		case OPER_MIN:
			m256iResult = _mm256_setr_epi64x((std::max)(i64Data[0], i64Oper), (std::max)(i64Data[1], i64Oper),
				(std::max)(i64Data[2], i64Oper), (std::max)(i64Data[3], i64Oper));
			break;
		case OPER_MAX:
			m256iResult = _mm256_setr_epi64x((std::min)(i64Data[0], i64Oper), (std::min)(i64Data[1], i64Oper),
				(std::min)(i64Data[2], i64Oper), (std::min)(i64Data[3], i64Oper));
			break;
		case OPER_SWAP:
			m256iResult = ut::ByteSwapVec<std::int64_t>(m256iData);
			break;
		case OPER_OR:
			m256iResult = _mm256_or_si256(m256iData, m256iOper);
			break;
		case OPER_XOR:
			m256iResult = _mm256_xor_si256(m256iData, m256iOper);
			break;
		case OPER_AND:
			m256iResult = _mm256_and_si256(m256iData, m256iOper);
			break;
		case OPER_NOT:
			//_mm256_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
			m256iResult = _mm256_xor_si256(m256iData, _mm256_cmpeq_epi64(m256iData, m256iData));
			break;
		case OPER_SHL:
			m256iResult = _mm256_slli_epi64(m256iData, static_cast<int>(i64Oper));
			break;
		case OPER_SHR:
			m256iResult = _mm256_setr_epi64x(i64Data[0] >> i64Oper, i64Data[1] >> i64Oper, i64Data[2] >> i64Oper,
				i64Data[3] >> i64Oper);
			break;
		case OPER_ROTL:
			m256iResult = _mm256_setr_epi64x(std::rotl(static_cast<std::uint64_t>(i64Data[0]), static_cast<const int>(i64Oper)),
					std::rotl(static_cast<std::uint64_t>(i64Data[1]), static_cast<const int>(i64Oper)),
					std::rotl(static_cast<std::uint64_t>(i64Data[2]), static_cast<const int>(i64Oper)),
					std::rotl(static_cast<std::uint64_t>(i64Data[3]), static_cast<const int>(i64Oper)));
			break;
		case OPER_ROTR:
			m256iResult = _mm256_setr_epi64x(std::rotr(static_cast<std::uint64_t>(i64Data[0]), static_cast<const int>(i64Oper)),
				std::rotr(static_cast<std::uint64_t>(i64Data[1]), static_cast<const int>(i64Oper)),
				std::rotr(static_cast<std::uint64_t>(i64Data[2]), static_cast<const int>(i64Oper)),
				std::rotr(static_cast<std::uint64_t>(i64Data[3]), static_cast<const int>(i64Oper)));
			break;
		case OPER_BITREV:
			m256iResult = _mm256_setr_epi64x(ut::BitReverse(i64Data[0]), ut::BitReverse(i64Data[1]), ut::BitReverse(i64Data[2]),
				ut::BitReverse(i64Data[3]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported int64_t operation.");
			break;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m256iResult = ut::ByteSwapVec<std::int64_t>(m256iResult);
		}

		_mm256_storeu_si256(reinterpret_cast<__m256i*>(pi64Data), m256iResult);
		};

	constexpr auto lmbOperVec256UInt64 = [](std::uint64_t* pui64Data, const HEXMODIFY& hms) {
		const auto m256iData = hms.fBigEndian ?
			ut::ByteSwapVec<std::uint64_t>(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(pui64Data)))
			: _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pui64Data));
		alignas(32) std::uint64_t ui64Data[4];
		_mm256_store_si256(reinterpret_cast<__m256i*>(ui64Data), m256iData);
		assert(!hms.spnData.empty());
		const auto ui64Oper = *reinterpret_cast<const std::uint64_t*>(hms.spnData.data());
		const auto m256iOper = _mm256_set1_epi64x(ui64Oper);
		__m256i m256iResult { };

		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m256iResult = _mm256_add_epi64(m256iData, m256iOper);
			break;
		case OPER_SUB:
			m256iResult = _mm256_sub_epi64(m256iData, m256iOper);
			break;
		case OPER_MUL:
			m256iResult = _mm256_setr_epi64x(ui64Data[0] * ui64Oper, ui64Data[1] * ui64Oper, ui64Data[2] * ui64Oper,
				ui64Data[3] * ui64Oper);
			break;
		case OPER_DIV:
			assert(ui64Oper > 0);
			m256iResult = _mm256_setr_epi64x(ui64Data[0] / ui64Oper, ui64Data[1] / ui64Oper, ui64Data[2] / ui64Oper,
				ui64Data[3] / ui64Oper);
			break;
		case OPER_MIN:
			m256iResult = _mm256_setr_epi64x((std::max)(ui64Data[0], ui64Oper), (std::max)(ui64Data[1], ui64Oper),
				(std::max)(ui64Data[2], ui64Oper), (std::max)(ui64Data[3], ui64Oper));
			break;
		case OPER_MAX:
			m256iResult = _mm256_setr_epi64x((std::min)(ui64Data[0], ui64Oper), (std::min)(ui64Data[1], ui64Oper),
				 (std::min)(ui64Data[2], ui64Oper), (std::min)(ui64Data[3], ui64Oper));
			break;
		case OPER_SWAP:
			m256iResult = ut::ByteSwapVec<std::uint64_t>(m256iData);
			break;
		case OPER_OR:
			m256iResult = _mm256_or_si256(m256iData, m256iOper);
			break;
		case OPER_XOR:
			m256iResult = _mm256_xor_si256(m256iData, m256iOper);
			break;
		case OPER_AND:
			m256iResult = _mm256_and_si256(m256iData, m256iOper);
			break;
		case OPER_NOT:
			//_mm256_cmpeq_epi64(a,a) will result in all 1s, then XOR to reverse bits.
			m256iResult = _mm256_xor_si256(m256iData, _mm256_cmpeq_epi64(m256iData, m256iData));
			break;
		case OPER_SHL:
			m256iResult = _mm256_slli_epi64(m256iData, static_cast<int>(ui64Oper));
			break;
		case OPER_SHR:
			m256iResult = _mm256_srli_epi64(m256iData, static_cast<int>(ui64Oper)); //Logical shift.
			break;
		case OPER_ROTL:
			m256iResult = _mm256_setr_epi64x(std::rotl(ui64Data[0], static_cast<const int>(ui64Oper)),
				std::rotl(ui64Data[1], static_cast<const int>(ui64Oper)),
				std::rotl(ui64Data[2], static_cast<const int>(ui64Oper)),
				std::rotl(ui64Data[3], static_cast<const int>(ui64Oper)));
			break;
		case OPER_ROTR:
			m256iResult = _mm256_setr_epi64x(std::rotr(ui64Data[0], static_cast<const int>(ui64Oper)),
				std::rotr(ui64Data[1], static_cast<const int>(ui64Oper)),
				std::rotr(ui64Data[2], static_cast<const int>(ui64Oper)),
				std::rotr(ui64Data[3], static_cast<const int>(ui64Oper)));
			break;
		case OPER_BITREV:
			m256iResult = _mm256_setr_epi64x(ut::BitReverse(ui64Data[0]), ut::BitReverse(ui64Data[1]), ut::BitReverse(ui64Data[2]),
				ut::BitReverse(ui64Data[3]));
			break;
		default:
			ut::DBG_REPORT(L"Unsupported uint64_t operation.");
			break;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m256iResult = ut::ByteSwapVec<std::uint64_t>(m256iResult);
		}

		_mm256_storeu_si256(reinterpret_cast<__m256i*>(pui64Data), m256iResult);
		};

	constexpr auto lmbOperVec256Float = [](float* pflData, const HEXMODIFY& hms) {
		const auto m256Data = hms.fBigEndian ? ut::ByteSwapVec<float>(_mm256_loadu_ps(pflData)) : _mm256_loadu_ps(pflData);
		const auto m256Oper = _mm256_set1_ps(*reinterpret_cast<const float*>(hms.spnData.data()));
		__m256 m256Result { };

		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m256Result = _mm256_add_ps(m256Data, m256Oper);
			break;
		case OPER_SUB:
			m256Result = _mm256_sub_ps(m256Data, m256Oper);
			break;
		case OPER_MUL:
			m256Result = _mm256_mul_ps(m256Data, m256Oper);
			break;
		case OPER_DIV:
			assert(*reinterpret_cast<const float*>(hms.spnData.data()) > 0.F);
			m256Result = _mm256_div_ps(m256Data, m256Oper);
			break;
		case OPER_MIN:
			m256Result = _mm256_max_ps(m256Data, m256Oper);
			break;
		case OPER_MAX:
			m256Result = _mm256_min_ps(m256Data, m256Oper);
			break;
		case OPER_SWAP:
			m256Result = ut::ByteSwapVec<float>(m256Data);
			break;
		default:
			ut::DBG_REPORT(L"Unsupported float operation.");
			return;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m256Result = ut::ByteSwapVec<float>(m256Result);
		}

		_mm256_storeu_ps(pflData, m256Result);
		};

	constexpr auto lmbOperVec256Double = [](double* pdblData, const HEXMODIFY& hms) {
		const auto m256dData = hms.fBigEndian ? ut::ByteSwapVec<double>(_mm256_loadu_pd(pdblData)) : _mm256_loadu_pd(pdblData);
		const auto m256dOper = _mm256_set1_pd(*reinterpret_cast<const double*>(hms.spnData.data()));
		__m256d m256dResult { };

		switch (hms.eOperMode) {
		case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
			break;
		case OPER_ADD:
			m256dResult = _mm256_add_pd(m256dData, m256dOper);
			break;
		case OPER_SUB:
			m256dResult = _mm256_sub_pd(m256dData, m256dOper);
			break;
		case OPER_MUL:
			m256dResult = _mm256_mul_pd(m256dData, m256dOper);
			break;
		case OPER_DIV:
			assert(*reinterpret_cast<const double*>(hms.spnData.data()) > 0.);
			m256dResult = _mm256_div_pd(m256dData, m256dOper);
			break;
		case OPER_MIN:
			m256dResult = _mm256_max_pd(m256dData, m256dOper);
			break;
		case OPER_MAX:
			m256dResult = _mm256_min_pd(m256dData, m256dOper);
			break;
		case OPER_SWAP:
			m256dResult = ut::ByteSwapVec<double>(m256dData);
			break;
		default:
			ut::DBG_REPORT(L"Unsupported double operation.");
			return;
		}

		if (hms.fBigEndian) { //Swap bytes back.
			m256dResult = ut::ByteSwapVec<double>(m256dResult);
		}

		_mm256_storeu_pd(pdblData, m256dResult);
		};

	switch (hms.eDataType) {
	case DATA_INT8:
		lmbOperVec256Int8(reinterpret_cast<std::int8_t*>(pData), hms);
		break;
	case DATA_UINT8:
		lmbOperVec256UInt8(reinterpret_cast<std::uint8_t*>(pData), hms);
		break;
	case DATA_INT16:
		lmbOperVec256Int16(reinterpret_cast<std::int16_t*>(pData), hms);
		break;
	case DATA_UINT16:
		lmbOperVec256UInt16(reinterpret_cast<std::uint16_t*>(pData), hms);
		break;
	case DATA_INT32:
		lmbOperVec256Int32(reinterpret_cast<std::int32_t*>(pData), hms);
		break;
	case DATA_UINT32:
		lmbOperVec256UInt32(reinterpret_cast<std::uint32_t*>(pData), hms);
		break;
	case DATA_INT64:
		lmbOperVec256Int64(reinterpret_cast<std::int64_t*>(pData), hms);
		break;
	case DATA_UINT64:
		lmbOperVec256UInt64(reinterpret_cast<std::uint64_t*>(pData), hms);
		break;
	case DATA_FLOAT:
		lmbOperVec256Float(reinterpret_cast<float*>(pData), hms);
		break;
	case DATA_DOUBLE:
		lmbOperVec256Double(reinterpret_cast<double*>(pData), hms);
		break;
	default:
		break;
	}
}
#endif //^^^ _M_IX86 || _M_X64

//CHexCtrl message handlers.

auto CHexCtrl::OnChar(const MSG& msg)->LRESULT
{
	const auto nChar = static_cast<UINT>(msg.wParam);
	if (!IsDataSet() || !IsMutable() || !IsCurTextArea() || (GetKeyState(VK_CONTROL) < 0)
		|| !std::iswprint(static_cast<wint_t>(nChar)))
		return 0;

	unsigned char chByte = nChar & 0xFF;

	wchar_t warrCurrLocaleID[KL_NAMELENGTH];
	::GetKeyboardLayoutNameW(warrCurrLocaleID); //Current langID as wstring.
	if (const auto optLocID = stn::StrToUInt32(warrCurrLocaleID, 16); optLocID) { //Convert langID from wstr to number.
		UINT uCurrCodePage { };
		constexpr int iSize = sizeof(uCurrCodePage) / sizeof(wchar_t);
		if (::GetLocaleInfoW(*optLocID, LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
			reinterpret_cast<LPWSTR>(&uCurrCodePage), iSize) == iSize) { //ANSI code page for the current langID (if any).
			const wchar_t wch { static_cast<wchar_t>(nChar) };
			//Convert input symbol (wchar) to char according to current Windows' code page.
			if (auto str = ut::WstrToStr(&wch, uCurrCodePage); !str.empty()) {
				chByte = str[0];
			}
		}
	}

	ModifyData({ .spnData { reinterpret_cast<std::byte*>(&chByte), sizeof(chByte) }, .vecSpan { { GetCaretPos(), 1 } } });
	CaretMoveRight();

	return 0;
}

auto CHexCtrl::OnCommand(const MSG& msg)->LRESULT
{
	const auto wMenuID = LOWORD(msg.wParam);
	if (const auto opt = GetCommandFromMenu(wMenuID); opt) {
		ExecuteCmd(*opt);
		return 0;
	}

	//For a user defined custom menu we notify the parent window.
	ParentNotify(HEXMENUINFO { .hdr { m_Wnd, static_cast<UINT>(m_Wnd.GetDlgCtrlID()), HEXCTRL_MSG_MENUCLICK },
		.pt { m_stMenuClickedPt }, .wMenuID { wMenuID } });

	return 0;
}

auto CHexCtrl::OnContextMenu(const MSG& msg)->LRESULT
{
	const POINT pt { .x { wnd::GetXLPARAM(msg.lParam) }, .y { wnd::GetYLPARAM(msg.lParam) } };

	//Notify parent that we are about to display a context menu.
	const HEXMENUINFO hmi { .hdr { m_Wnd, static_cast<UINT>(m_Wnd.GetDlgCtrlID()), HEXCTRL_MSG_CONTEXTMENU },
		.pt { m_stMenuClickedPt = pt }, .fShow { true } };
	ParentNotify(hmi);
	if (hmi.fShow) { //Parent window can disable context menu showing up.
		m_MenuMain.GetSubMenu(0).TrackPopupMenu(pt.x, pt.y, m_Wnd);
	}

	return 0;
}

auto CHexCtrl::OnDestroy()->LRESULT
{
	//All these cleanups below are important when HexCtrl window is destroyed but IHexCtrl object
	//itself is still alive. The IHexCtrl object is alive until the IHexCtrl::Delete() method is called.
	//Child windows of the IHexCtrl (e.g. tooltips) will be destroyed automatically by Windows.

	//Note: The MSDN for the DestroyWindow clearly states that:
	//"If the specified window is a parent or owner window, DestroyWindow automatically destroys the associated 
	//child or owned windows when it destroys the parent or owner window. The function first destroys child or
	//owned windows, and then it destroys the parent or owner window."
	//But this doesn't seem to always be the case for owned dialog windows in some environments (mainly when 
	//IHexCtrl is a child of MFC's CView class).
	//These DestroyDlg calls to make sure the dialogs are always properly destroyed.

	ClearData();
	m_pDlgBkmMgr->DestroyDlg();
	m_pDlgCodepage->DestroyDlg();
	m_pDlgDataInterp->DestroyDlg();
	m_pDlgModify->DestroyDlg();
	m_pDlgGoTo->DestroyDlg();
	m_pDlgSearch->DestroyDlg();
	m_pDlgTemplMgr->DestroyDlg();
	m_pDlgTemplMgr->UnloadAll(); //Templates could be loaded without creating the dialog itself.
	m_vecHBITMAP.clear();
	m_vecKeyBind.clear();
	m_vecUndo.clear();
	m_vecRedo.clear();
	m_vecCharsWidth.clear();
	m_MenuMain.DestroyMenu();
	::DeleteObject(m_hFntMain);
	::DeleteObject(m_hFntInfoBar);
	::DeleteObject(m_hPenLines);
	::DeleteObject(m_hPenDataTempl);
	m_pScrollV->DestroyWindow(); //Not a child of the IHexCtrl.
	m_pScrollH->DestroyWindow(); //Not a child of the IHexCtrl.
	ParentNotify(HEXCTRL_MSG_DESTROY);
	m_fCreated = false;

	return 0;
}

auto CHexCtrl::OnEraseBkgnd(const MSG& /*msg*/)->LRESULT
{
	return TRUE;
}

auto CHexCtrl::OnGetDlgCode(const MSG& /*msg*/)->LRESULT
{
	return DLGC_WANTALLKEYS;
}

auto CHexCtrl::OnHelp(const MSG& /*msg*/)->LRESULT
{
	return FALSE;
}

auto CHexCtrl::OnHScroll(const MSG& /*msg*/)->LRESULT
{
	m_Wnd.RedrawWindow();

	return 0;
}

auto CHexCtrl::OnInitMenuPopup(const MSG& msg)->LRESULT
{
	const auto nIndex = static_cast<UINT>(LOWORD(msg.lParam));
	using enum EHexCmd;
	//The nIndex specifies the zero-based relative position of the menu item that opens the drop-down menu or submenu.
	switch (nIndex) {
	case 0:	//Search.
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_SEARCH_DLGSEARCH, IsCmdAvail(CMD_SEARCH_DLG));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_SEARCH_NEXT, IsCmdAvail(CMD_SEARCH_NEXT));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_SEARCH_PREV, IsCmdAvail(CMD_SEARCH_PREV));
		break;
	case 3:	//Navigation.
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_NAV_DLGGOTO, IsCmdAvail(CMD_NAV_GOTO_DLG));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_NAV_REPFWD, IsCmdAvail(CMD_NAV_REPFWD));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_NAV_REPBKW, IsCmdAvail(CMD_NAV_REPBKW));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_NAV_DATABEG, IsCmdAvail(CMD_NAV_DATABEG));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_NAV_DATAEND, IsCmdAvail(CMD_NAV_DATAEND));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_NAV_PAGEBEG, IsCmdAvail(CMD_NAV_PAGEBEG));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_NAV_PAGEEND, IsCmdAvail(CMD_NAV_PAGEEND));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_NAV_LINEBEG, IsCmdAvail(CMD_NAV_LINEBEG));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_NAV_LINEEND, IsCmdAvail(CMD_NAV_LINEEND));
		break;
	case 4:	//Bookmarks.
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_BKM_ADD, IsCmdAvail(CMD_BKM_ADD));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_BKM_REMOVE, IsCmdAvail(CMD_BKM_REMOVE));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_BKM_NEXT, IsCmdAvail(CMD_BKM_NEXT));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_BKM_PREV, IsCmdAvail(CMD_BKM_PREV));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_BKM_REMOVEALL, IsCmdAvail(CMD_BKM_REMOVEALL));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_BKM_DLGMGR, IsCmdAvail(CMD_BKM_DLG_MGR));
		break;
	case 5:	//Clipboard.
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_COPYHEX, IsCmdAvail(CMD_CLPBRD_COPY_HEX));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_COPYHEXLE, IsCmdAvail(CMD_CLPBRD_COPY_HEXLE));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_COPYHEXFMT, IsCmdAvail(CMD_CLPBRD_COPY_HEXFMT));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_COPYTEXTCP, IsCmdAvail(CMD_CLPBRD_COPY_TEXTCP));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_COPYBASE64, IsCmdAvail(CMD_CLPBRD_COPY_BASE64));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_COPYCARR, IsCmdAvail(CMD_CLPBRD_COPY_CARR));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_COPYGREPHEX, IsCmdAvail(CMD_CLPBRD_COPY_GREPHEX));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_COPYPRNTSCRN, IsCmdAvail(CMD_CLPBRD_COPY_PRNTSCRN));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_COPYOFFSET, IsCmdAvail(CMD_CLPBRD_COPY_OFFSET));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_PASTEHEX, IsCmdAvail(CMD_CLPBRD_PASTE_HEX));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_PASTETEXTUTF16, IsCmdAvail(CMD_CLPBRD_PASTE_TEXTUTF16));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_CLPBRD_PASTETEXTCP, IsCmdAvail(CMD_CLPBRD_PASTE_TEXTCP));
		break;
	case 6: //Modify.
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_MODIFY_FILLZEROS, IsCmdAvail(CMD_MODIFY_FILLZEROS));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_MODIFY_DLGFILLDATA, IsCmdAvail(CMD_MODIFY_FILLDATA_DLG));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_MODIFY_DLGOPERS, IsCmdAvail(CMD_MODIFY_OPERS_DLG));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_MODIFY_UNDO, IsCmdAvail(CMD_MODIFY_UNDO));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_MODIFY_REDO, IsCmdAvail(CMD_MODIFY_REDO));
		break;
	case 7: //Selection.
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_SEL_MARKSTARTEND, IsCmdAvail(CMD_SEL_MARKSTARTEND));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_SEL_ALL, IsCmdAvail(CMD_SEL_ALL));
		break;
	case 8: //Templates.
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_TEMPL_APPLYCURR, IsCmdAvail(CMD_TEMPL_APPLYCURR));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_TEMPL_DISAPPLY, IsCmdAvail(CMD_TEMPL_DISAPPLY));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_TEMPL_DISAPPALL, IsCmdAvail(CMD_TEMPL_DISAPPALL));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_TEMPL_DLGMGR, IsCmdAvail(CMD_TEMPL_DLG_MGR));
		break;
	case 9: //Data Presentation.
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_DLGDATAINTERP, IsCmdAvail(CMD_DATAINTERP_DLG));
		m_MenuMain.EnableMenuItem(IDM_HEXCTRL_DLGCODEPAGE, IsCmdAvail(CMD_CODEPAGE_DLG));
		break;
	default:
		break;
	}

	return 0;
}

auto CHexCtrl::OnKeyDown(const MSG& msg)->LRESULT
{
	const auto uChar = static_cast<UINT>(msg.wParam);
	const auto uFlags = static_cast<UINT>(msg.lParam);

	//LORE: If some key combinations do not work for seemingly no reason, it might be due to
	//global hotkeys hooks from some running app.

	//KF_REPEAT indicates that the key was pressed continuously.
	if (uFlags & KF_REPEAT) {
		m_fKeyDownAtm = true;
	}

	if (const auto optCmd = GetCommandFromKey(uChar, ::GetAsyncKeyState(VK_CONTROL) < 0,
		::GetAsyncKeyState(VK_SHIFT) < 0, ::GetAsyncKeyState(VK_MENU) < 0); optCmd) {
		ExecuteCmd(*optCmd);
		return 0;
	}

	if (!IsDataSet() || !IsMutable() || IsCurTextArea())
		return 0;

	//If caret is in the Hex area then just one part (High/Low) of the byte must be changed.
	//Normalizing all input in the Hex area to only [0x0-0xF] range, allowing only [0-9], [A-F], [NUM0-NUM9].
	unsigned char chByte = uChar & 0xFFU;
	if (chByte >= '0' && chByte <= '9') {
		chByte -= '0'; //'1' - '0' = 0x01.
	}
	else if (chByte >= 'A' && chByte <= 'F') {
		chByte -= 0x37; //'A' - 0x37 = 0x0A.
	}
	else if (chByte >= VK_NUMPAD0 && chByte <= VK_NUMPAD9) {
		chByte -= VK_NUMPAD0;
	}
	else
		return 0;

	const auto chByteCurr = ut::GetIHexTData<unsigned char>(*this, GetCaretPos());
	const auto bByteToSet = static_cast<std::byte>(m_fCaretHigh ? ((chByte << 4U) | (chByteCurr & 0x0FU))
		: ((chByte & 0x0FU) | (chByteCurr & 0xF0U)));
	ModifyData({ .eModifyMode { EHexModifyMode::MODIFY_ONCE }, .spnData { &bByteToSet, sizeof(bByteToSet) },
		.vecSpan { { GetCaretPos(), 1 } } });
	CaretMoveRight();

	return 0;
}

auto CHexCtrl::OnKeyUp(const MSG& /*msg*/)->LRESULT
{
	if (!IsDataSet())
		return 0;

	if (m_fKeyDownAtm) { //If a key was previously pressed continuously and is now released.
		m_pDlgDataInterp->UpdateData();
		m_fKeyDownAtm = false;
	}

	return 0;
}

auto CHexCtrl::OnLButtonDblClk(const MSG& msg)->LRESULT
{
	const POINT pt { .x { wnd::GetXLPARAM(msg.lParam) }, .y { wnd::GetYLPARAM(msg.lParam) } };
	const auto nFlags = static_cast<UINT>(msg.wParam);

	if ((pt.x + static_cast<long>(m_pScrollH->GetScrollPos())) < m_iSecondVertLinePx) { //DblClick on "Offset" area.
		SetOffsetMode(!IsOffsetAsHex());
	}
	else if (GetRectTextCaption().PtInRect(pt) != FALSE) { //DblClick on codepage caption area.
		ExecuteCmd(EHexCmd::CMD_CODEPAGE_DLG);
	}
	else if (const auto optHit = HitTest(pt); optHit) { //DblClick on hex/text area.
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
			m_fClickWithAlt = ::GetAsyncKeyState(VK_MENU) < 0;
			m_ullCursorPrev = m_ullCursorNow = m_ullCaretPos;
			hs.ullOffset = m_ullCaretPos;
			hs.ullSize = 1ULL;
		}
		SetSelection({ hs });
		m_Wnd.SetCapture();
	}

	return 0;
}

auto CHexCtrl::OnLButtonDown(const MSG& msg)->LRESULT
{
	const POINT pt { .x { wnd::GetXLPARAM(msg.lParam) }, .y { wnd::GetYLPARAM(msg.lParam) } };
	const auto nFlags = static_cast<UINT>(msg.wParam);

	m_Wnd.SetFocus(); //SetFocus is vital to give proper keyboard input to the main HexCtrl window.
	const auto optHit = HitTest(pt);
	if (!optHit)
		return 0;

	m_fLMousePressed = true;
	m_ullCursorNow = m_ullCaretPos = optHit->ullOffset;
	m_fCursorTextArea = optHit->fIsText;
	if (!optHit->fIsText) {
		m_fCaretHigh = optHit->fIsHigh;
	}

	m_fClickWithAlt = ::GetAsyncKeyState(VK_MENU) < 0;
	m_Wnd.SetCapture();

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

	return 0;
}

auto CHexCtrl::OnLButtonUp(const MSG& /*msg*/)->LRESULT
{
	m_fLMousePressed = false;
	::ReleaseCapture();
	m_pScrollV->OnLButtonUp();
	m_pScrollH->OnLButtonUp();

	return 0;
}

auto CHexCtrl::OnMouseMove(const MSG& msg)->LRESULT
{
	POINT pt { .x { wnd::GetXLPARAM(msg.lParam) }, .y { wnd::GetYLPARAM(msg.lParam) } };
	const auto optHit = HitTest(pt);

	if (m_fLMousePressed) {
		//If LMouse is pressed but cursor is outside of client area.
		//SetCapture() behaviour.

		const auto rcClient = m_Wnd.GetClientRect();
		//Checking for scrollbars existence first.
		if (m_pScrollH->IsVisible()) {
			if (pt.x < rcClient.left) {
				m_pScrollH->ScrollLineLeft();
				pt.x = m_iIndentFirstHexChunkXPx;
			}
			else if (pt.x >= rcClient.right) {
				m_pScrollH->ScrollLineRight();
				pt.x = m_iFourthVertLinePx - 1;
			}
		}
		if (m_pScrollV->IsVisible()) {
			if (pt.y < m_iStartWorkAreaYPx) {
				m_pScrollV->ScrollLineUp();
				pt.y = m_iStartWorkAreaYPx;
			}
			else if (pt.y >= m_iEndWorkAreaPx) {
				m_pScrollV->ScrollLineDown();
				pt.y = m_iEndWorkAreaPx - 1;
			}
		}

		//Checking if the current cursor pos is at the same byte's half
		//that it was at the previous WM_MOUSEMOVE fire.
		//Making selection of the byte only if the cursor has crossed byte's halves.
		//Doesn't apply when moving in Text area.
		if (!optHit || (optHit->ullOffset == m_ullCursorNow && m_fCaretHigh == optHit->fIsHigh && !optHit->fIsText))
			return 0;

		m_ullCursorNow = optHit->ullOffset;
		const auto ullOffsetHit = optHit->ullOffset;
		const auto dwCapacity = GetCapacity();
		ULONGLONG ullClick;
		ULONGLONG  ullStart;
		ULONGLONG ullSize;
		ULONGLONG ullLines;
		if (m_fClickWithAlt) { //Select block (with Alt).
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
		for (auto itLine = 0ULL; itLine < ullLines; ++itLine) {
			vecSel.emplace_back(ullStart + (dwCapacity * itLine), ullSize);
		}
		SetSelection(vecSel);
	}
	else {
		if (optHit) {
			if (const auto pBkm = m_pDlgBkmMgr->HitTest(optHit->ullOffset); pBkm != nullptr) {
				if (m_pBkmTTCurr != pBkm) {
					m_pBkmTTCurr = pBkm;
					wnd::CPoint ptScreen = pt;
					m_Wnd.ClientToScreen(ptScreen);
					ptScreen.Offset(3, 3);
					m_ttiMain.lpszText = pBkm->wstrDesc.data();
					TTMainShow(true);
				}
			}
			else if (m_pBkmTTCurr != nullptr) {
				TTMainShow(false);
			}
			else if (const auto pField = m_pDlgTemplMgr->HitTest(optHit->ullOffset);
				m_pDlgTemplMgr->IsTooltips() && pField != nullptr) {
				if (m_pTFieldTTCurr != pField) {
					m_pTFieldTTCurr = pField;
					wnd::CPoint ptScreen = pt;
					m_Wnd.ClientToScreen(ptScreen);
					ptScreen.Offset(3, 3);
					m_ttiMain.lpszText = const_cast<LPWSTR>(pField->wstrName.data());
					TTMainShow(true);
				}
			}
			else if (m_pTFieldTTCurr != nullptr) {
				TTMainShow(false);
			}
		}
		else {
			//If there is already tooltip shown, but cursor is outside of data chunks.
			if (m_pBkmTTCurr != nullptr || m_pTFieldTTCurr != nullptr) {
				TTMainShow(false);
			}
		}

		m_pScrollV->OnMouseMove(pt);
		m_pScrollH->OnMouseMove(pt);
	}

	return 0;
}

auto CHexCtrl::OnMouseWheel(const MSG& msg)->LRESULT
{
	const auto zDelta = GET_WHEEL_DELTA_WPARAM(msg.wParam);
	const auto nFlags = GET_KEYSTATE_WPARAM(msg.wParam);

	if (const auto opt = GetCommandFromKey(zDelta > 0 ? m_dwVKMouseWheelUp : m_dwVKMouseWheelDown,
		nFlags & MK_CONTROL, nFlags & MK_SHIFT, false); opt) {
		ExecuteCmd(*opt);
	}

	return 0;
}

auto CHexCtrl::OnNCActivate(const MSG& /*msg*/)->LRESULT
{
	m_pScrollV->OnNCActivate();
	m_pScrollH->OnNCActivate();

	return TRUE;
}

auto CHexCtrl::OnNCCalcSize(const MSG& msg)->LRESULT
{
	wnd::DefWndProc(msg);
	const auto pNCSP = reinterpret_cast<LPNCCALCSIZE_PARAMS>(msg.lParam);

	//Sequence is important — H->V.
	m_pScrollH->OnNCCalcSize(pNCSP);
	m_pScrollV->OnNCCalcSize(pNCSP);

	return 0;
}

auto CHexCtrl::OnNCPaint(const MSG& msg)->LRESULT
{
	wnd::DefWndProc(msg);
	m_pScrollV->OnNCPaint();
	m_pScrollH->OnNCPaint();

	return 0;
}

auto CHexCtrl::OnPaint()->LRESULT
{
	wnd::CPaintDC dcPaint(m_Wnd);

	if (!IsDrawable()) //Control should not be rendered atm.
		return 0;

	const auto rcClient = m_Wnd.GetClientRect();

	if (!IsCreated()) {
		dcPaint.FillSolidRect(rcClient, RGB(250, 250, 250));
		dcPaint.TextOutW(1, 1, L"Call IHexCtrl::Create first.");
		return 0;
	}

	//To prevent drawing in too small window (can cause hangs).
	if (rcClient.IsRectEmpty() || rcClient.Height() <= m_iHeightTopRectPx + m_iHeightBottomOffAreaPx)
		return 0;

	const auto ullStartLine = GetTopLine();
	const auto ullEndLine = GetBottomLine();
	auto iLines = static_cast<int>(ullEndLine - ullStartLine);
	if (iLines < 0) {
		ut::DBG_REPORT(L"iLines < 0");
		return 0;
	}

	//Actual amount of lines, "ullEndLine - ullStartLine" always shows one line less.
	if (IsDataSet()) {
		++iLines;
	}

	//Drawing through CMemDC to avoid flickering.
	wnd::CMemDC dcMem(dcPaint, rcClient);
	DrawWindow(dcMem);
	DrawInfoBar(dcMem);

	if (!IsDataSet())
		return 0;

	DrawOffsets(dcMem, ullStartLine, iLines);
	const auto& [wstrHex, wstrText] = BuildDataToDraw(ullStartLine, iLines);
	DrawHexText(dcMem, ullStartLine, iLines, wstrHex, wstrText);
	DrawTemplates(dcMem, ullStartLine, iLines, wstrHex, wstrText);
	DrawBookmarks(dcMem, ullStartLine, iLines, wstrHex, wstrText);
	DrawSelection(dcMem, ullStartLine, iLines, wstrHex, wstrText);
	DrawSelHighlight(dcMem, ullStartLine, iLines, wstrHex, wstrText);
	DrawCaret(dcMem, ullStartLine, iLines, wstrHex, wstrText);
	DrawDataInterp(dcMem, ullStartLine, iLines, wstrHex, wstrText);
	DrawPageLines(dcMem, ullStartLine, iLines);

	return 0;
}

auto CHexCtrl::OnSetCursor(const MSG& msg)->LRESULT
{
	const auto nHitTest = static_cast<UINT>(LOWORD(msg.lParam));
	const auto message = static_cast<UINT>(HIWORD(msg.lParam));

	m_pScrollV->OnSetCursor(nHitTest, message);
	m_pScrollH->OnSetCursor(nHitTest, message);

	return wnd::DefWndProc(msg); //To set appropriate cursor.
}

auto CHexCtrl::OnSize(const MSG& msg)->LRESULT
{
	const auto iWidth = LOWORD(msg.lParam);
	const auto iHeight = HIWORD(msg.lParam);

	if (!IsCreated())
		return 0;

	RecalcClientArea(iWidth, iHeight);
	m_pScrollV->SetScrollPageSize(GetScrollPageSize());

	return 0;
}

auto CHexCtrl::OnSysKeyDown(const MSG& msg)->LRESULT
{
	const auto nChar = static_cast<UINT>(msg.wParam);

	if (const auto optCmd = GetCommandFromKey(nChar, ::GetAsyncKeyState(VK_CONTROL) < 0,
		::GetAsyncKeyState(VK_SHIFT) < 0, true); optCmd) {
		ExecuteCmd(*optCmd);
	}

	return 0;
}

auto CHexCtrl::OnTimer(const MSG& msg)->LRESULT
{
	static constexpr auto dbSecToShow { 5000.0 }; //How many ms to show tooltips.
	const auto nIDEvent = static_cast<UINT_PTR>(msg.wParam);
	if (nIDEvent != m_uIDTTTMain)
		return 	wnd::DefWndProc(msg);

	auto rcClient = m_Wnd.GetClientRect();
	m_Wnd.ClientToScreen(rcClient);
	wnd::CPoint ptCur;
	::GetCursorPos(ptCur);

	if (!rcClient.PtInRect(ptCur)) { //Check if cursor has left client rect,
		TTMainShow(false);
	}
	else if (const auto msElapsed = std::chrono::duration<double, std::milli>
		(std::chrono::high_resolution_clock::now() - m_tmTT).count();
		msElapsed >= dbSecToShow) { //or more than dbSecToShow ms have passed since toolip was shown.
		TTMainShow(false, true);
	}

	return 0;
}

auto CHexCtrl::OnVScroll(const MSG& /*msg*/)->LRESULT
{
	bool fRedraw { true };
	if (m_fHighLatency) {
		fRedraw = m_pScrollV->IsThumbReleased();
		TTOffsetShow(!fRedraw);
	}

	if (fRedraw) {
		m_Wnd.RedrawWindow();
	}

	return 0;
}

auto CHexCtrl::SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIDSubclass, DWORD_PTR /*dwRefData*/) -> LRESULT {
	if (uMsg == WM_NCDESTROY) {
		::RemoveWindowSubclass(hWnd, SubclassProc, uIDSubclass);
	}

	const auto pHexCtrl = reinterpret_cast<CHexCtrl*>(uIDSubclass);
	return pHexCtrl->ProcessMsg({ .hwnd { hWnd }, .message { uMsg }, .wParam { wParam }, .lParam { lParam } });
}