/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
module;
#include <SDKDDKVer.h>
#include "../HexCtrl.h"
#include "dep/rapidjson-amalgam.h"
#include "res/HexCtrlRes.h"
#include <Windows.h>
#include <commctrl.h>
#include <intrin.h>
#include <algorithm>
#include <bit>
#include <cassert>
#include <chrono>
#include <cwctype>
#include <format>
#include <fstream>
#include <numeric>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
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
		void CreateRes();
		auto OnCommand(const MSG& msg) -> INT_PTR;
		auto OnCtlClrStatic(const MSG& msg) -> INT_PTR;
		auto OnDestroy() -> INT_PTR;
		auto OnDPIChanged(const MSG& msg) -> INT_PTR;
		auto OnInitDialog(const MSG& msg) -> INT_PTR;
		auto OnLButtonDown(const MSG& msg) -> INT_PTR;
		auto OnLButtonUp(const MSG& msg) -> INT_PTR;
		auto OnMouseMove(const MSG& msg) -> INT_PTR;
		auto OnSetCursor(const MSG& msg) -> INT_PTR;
	private:
		HINSTANCE m_hInstRes { };
		GDIUT::CWnd m_Wnd;      //Main window.
		GDIUT::CWnd m_WndLink;  //Static link control
		HBITMAP m_hBmpLogo { }; //Logo bitmap.
		HFONT m_hFontDef { };
		HFONT m_hFontUnderline { };
		bool m_fHandCursor { }; //Is cursor a hand cursor atm?
		bool m_fLBDownLink { }; //Left button was pressed on the link static control.
	};
}

auto CHexDlgAbout::DoModal(HWND hWndParent)->INT_PTR {
	return ::DialogBoxParamW(m_hInstRes, MAKEINTRESOURCEW(IDD_HEXCTRL_ABOUT),
		hWndParent, GDIUT::DlgProc<CHexDlgAbout>, reinterpret_cast<LPARAM>(this));
}

auto CHexDlgAbout::ProcessMsg(const MSG& msg)->INT_PTR {
	switch (msg.message) {
	case WM_COMMAND: return OnCommand(msg);
	case WM_CTLCOLORSTATIC: return OnCtlClrStatic(msg);
	case WM_DESTROY: return OnDestroy();
	case WM_DPICHANGED: return OnDPIChanged(msg);
	case WM_INITDIALOG: return OnInitDialog(msg);
	case WM_LBUTTONDOWN: return OnLButtonDown(msg);
	case WM_LBUTTONUP: return OnLButtonUp(msg);
	case WM_MOUSEMOVE: return OnMouseMove(msg);
	case WM_SETCURSOR: return OnSetCursor(msg);
	default:
		return 0;
	}
}

void CHexDlgAbout::CreateRes()
{
	::DeleteObject(m_hFontDef);
	::DeleteObject(m_hFontUnderline);

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

	::DeleteObject(m_hBmpLogo);
	const auto iSizeIcon = static_cast<int>(32 * GDIUT::GetDPIScaleForHWND(m_Wnd));
	m_hBmpLogo = static_cast<HBITMAP>(::LoadImageW(m_hInstRes, MAKEINTRESOURCEW(IDB_HEXCTRL_LOGO),
		IMAGE_BITMAP, iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	m_Wnd.GetDlgItem(IDC_HEXCTRL_ABOUT_LOGO).SendMsg(STM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(m_hBmpLogo));
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
		::SelectObject(hDC, m_fHandCursor ? m_hFontUnderline : m_hFontDef);
		return reinterpret_cast<INT_PTR>(::GetSysColorBrush(COLOR_3DFACE));
	}

	return FALSE; //Default handler.
}

auto CHexDlgAbout::OnDestroy()->INT_PTR {
	::DeleteObject(m_hBmpLogo);
	::DeleteObject(m_hFontDef);
	::DeleteObject(m_hFontUnderline);

	return TRUE;
}

auto CHexDlgAbout::OnDPIChanged([[maybe_unused]] const MSG& msg)->INT_PTR
{
	CreateRes();
	return 0;
}

auto CHexDlgAbout::OnInitDialog(const MSG& msg)->INT_PTR
{
	m_Wnd.Attach(msg.hwnd);
	m_WndLink.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_ABOUT_STAT_LINKGH));
	CreateRes();

	const auto wstrVersion = std::format(L"Hex Control for Windows apps, v{}.{}.{}\r\nCopyright © 2018-present Jovibor",
		HEXCTRL_VERSION_MAJOR, HEXCTRL_VERSION_MINOR, HEXCTRL_VERSION_PATCH);
	::SetWindowTextW(m_Wnd.GetDlgItem(IDC_HEXCTRL_ABOUT_STAT_VERSION), wstrVersion.data());

	return TRUE;
}

auto CHexDlgAbout::OnLButtonDown(const MSG& msg)->INT_PTR
{
	const POINT pt { .x { ut::GetXLPARAM(msg.lParam) }, .y { ut::GetYLPARAM(msg.lParam) } };
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
	const POINT pt { .x { ut::GetXLPARAM(msg.lParam) }, .y { ut::GetYLPARAM(msg.lParam) } };
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
	const POINT pt { .x { ut::GetXLPARAM(msg.lParam) }, .y { ut::GetYLPARAM(msg.lParam) } };
	const auto hWnd = m_Wnd.ChildWindowFromPoint(pt);
	if (hWnd == nullptr)
		return FALSE;

	if (m_fHandCursor != (m_WndLink == hWnd)) {
		m_fHandCursor = m_WndLink == hWnd;
		m_WndLink.Invalidate(false);
	}

	return TRUE;
}

auto CHexDlgAbout::OnSetCursor([[maybe_unused]] const MSG& msg)->INT_PTR
{
	if (m_fHandCursor) {
		const auto hCurHand = static_cast<HCURSOR>(::LoadImageW(nullptr, IDC_HAND, IMAGE_CURSOR, 0, 0,
			LR_DEFAULTSIZE | LR_SHARED));
		::SetCursor(hCurHand);
		return TRUE;
	}

	return 0; //Default cursor.
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
		[[nodiscard]] auto GetActualWidth()const -> int override;
		[[nodiscard]] auto GetBookmarks() -> IHexBookmarks* override;
		[[nodiscard]] auto GetCacheSize()const -> DWORD override;
		[[nodiscard]] auto GetCapacity()const -> DWORD override;
		[[nodiscard]] auto GetCaretPos()const -> ULONGLONG override;
		[[nodiscard]] auto GetCharsExtraSpace()const -> DWORD override;
		[[nodiscard]] auto GetCodepage()const -> int override;
		[[nodiscard]] auto GetColors()const -> const HEXCOLORS & override;
		[[nodiscard]] auto GetData(HEXSPAN hss)const -> SpanByte override;
		[[nodiscard]] auto GetDataSize()const -> ULONGLONG override;
		[[nodiscard]] auto GetDateInfo()const -> std::tuple<DWORD, wchar_t> override;
		[[nodiscard]] auto GetDlgItemHandle(EHexDlgItem eItem)const -> HWND override;
		[[nodiscard]] auto GetFont(bool fMain = true)const -> LOGFONTW override;
		[[nodiscard]] auto GetGroupSize()const -> DWORD override;
		[[nodiscard]] auto GetMenuHandle()const -> HMENU override;
		[[nodiscard]] auto GetOffset(ULONGLONG ullOffset, bool fGetVirt)const -> ULONGLONG override;
		[[nodiscard]] auto GetPagesCount()const -> ULONGLONG override;
		[[nodiscard]] auto GetPagePos()const -> ULONGLONG override;
		[[nodiscard]] auto GetPageSize()const -> DWORD override;
		[[nodiscard]] auto GetScrollRatio()const -> std::tuple<float, bool> override;
		[[nodiscard]] auto GetSelection()const -> VecSpan override;
		[[nodiscard]] auto GetTemplates() -> IHexTemplates* override;
		[[nodiscard]] auto GetUnprintableChar()const -> wchar_t override;
		[[nodiscard]] auto GetWndHandle(EHexWnd eWnd, bool fCreate)const -> HWND override;
		void GoToOffset(ULONGLONG ullOffset, int iPosAt = 0)override;
		[[nodiscard]] bool HasInfoBar()const override;
		[[nodiscard]] bool HasSelection()const override;
		[[nodiscard]] auto HitTest(POINT pt, bool fScreen)const -> std::optional<HEXHITTEST> override;
		[[nodiscard]] bool IsCmdAvail(EHexCmd eCmd)const override;
		[[nodiscard]] bool IsCreated()const override;
		[[nodiscard]] bool IsDataSet()const override;
		[[nodiscard]] bool IsHexCharsUpper()const override;
		[[nodiscard]] bool IsMutable()const override;
		[[nodiscard]] bool IsOffsetAsHex()const override;
		[[nodiscard]] auto IsOffsetVisible(ULONGLONG ullOffset)const -> HEXVISION override;
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
		void SetHexCharsCase(bool fUpper)override;
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
		[[nodiscard]] auto BuildDataToDraw(ULONGLONG ullStartLine, int iLines)const -> std::tuple<std::wstring, std::wstring>;
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
		[[nodiscard]] auto CopyBase64()const -> std::wstring;
		[[nodiscard]] auto CopyCArr()const -> std::wstring;
		[[nodiscard]] auto CopyGrepHex()const -> std::wstring;
		[[nodiscard]] auto CopyHex()const -> std::wstring;
		[[nodiscard]] auto CopyHexFmt()const -> std::wstring;
		[[nodiscard]] auto CopyHexLE()const -> std::wstring;
		[[nodiscard]] auto CopyOffset()const -> std::wstring;
		[[nodiscard]] auto CopyPrintScreen()const -> std::wstring;
		[[nodiscard]] auto CopyTextCP()const -> std::wstring;
		[[nodiscard]] auto CreateCapacityString()const -> std::wstring;
		void CreateMenu();
		void CreatePens();
		[[nodiscard]] auto CreateTextAreaString()const -> std::wstring;
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
		void FillWithZeros(); //Fill selection with zeros.
		[[nodiscard]] auto FontPointsFromScaledPixels(long iSizePixels)const -> float;  //Get font size in points from size in scaled pixels.
		[[nodiscard]] auto FontScaledPixelsFromPoints(float flSizePoints)const -> long; //Get font size in scaled pixels from size in points.
		void FontSizeIncDec(bool fInc = true); //Increase os decrease font size by minimum amount.
		[[nodiscard]] auto GetBottomLine()const -> ULONGLONG; //Returns current bottom line number in view.
		[[nodiscard]] auto GetCapacityImpl()const -> DWORD;
		[[nodiscard]] auto GetCaretPosImpl()const -> std::uint64_t;
		[[nodiscard]] auto GetCharsWidthArray()const -> int*;
		[[nodiscard]] auto GetCharWidthExtras()const -> int;  //Width of the one char with extra space, in px.
		[[nodiscard]] auto GetCharWidthNative()const -> int;  //Width of the one char, in px.
		[[nodiscard]] auto GetCommandFromKey(UINT uKey, bool fCtrl, bool fShift, bool fAlt)const -> std::optional<EHexCmd>; //Get command from keybinding.
		[[nodiscard]] auto GetCommandFromMenu(WORD wMenuID)const -> std::optional<EHexCmd>; //Get command from menuID.
		[[nodiscard]] auto GetDataSizeImpl()const -> std::uint64_t;
		[[nodiscard]] auto GetDigitsOffset()const -> DWORD;
		[[nodiscard]] auto GetDPIScale()const -> float;
		[[nodiscard]] long GetFontSizeInPixels(bool fMain)const;
		[[nodiscard]] auto GetHexChars()const -> const wchar_t*;
		[[nodiscard]] auto GetOffsetImpl(std::uint64_t u64Offset, bool fGetVirt)const -> std::uint64_t;
		[[nodiscard]] auto GetPagePosImpl()const -> std::uint64_t;
		[[nodiscard]] auto GetPageSizeImpl()const -> std::uint32_t;
		[[nodiscard]] auto GetPagesCountImpl()const -> std::uint64_t;
		[[nodiscard]] auto GetRectTextCaption()const -> GDIUT::CRect; //Returns rect of the text caption area.
		[[nodiscard]] auto GetSelectedLines()const -> ULONGLONG;  //Get amount of selected lines.
		[[nodiscard]] auto GetScrollPageSize()const -> ULONGLONG; //Get the "Page" size of the scroll.
		[[nodiscard]] auto GetTopLine()const -> ULONGLONG;        //Returns current top line number in view.
		[[nodiscard]] auto GetVirtualOffset(ULONGLONG ullOffset)const -> ULONGLONG;
		void HexChunkPoint(ULONGLONG ullOffset, int& iCx, int& iCy)const; //Point of Hex chunk.
		[[nodiscard]] auto HitTest(POINT pt)const -> std::optional<HEXHITTEST>; //Is any hex chunk withing given point?
		[[nodiscard]] bool IsCurTextArea()const;               //Whether last focus was set at Text or Hex chunks area.
		[[nodiscard]] bool IsDataSetImpl()const;               //Internal implementation of the interface IsDataSet method.
		[[nodiscard]] bool IsDrawable()const;                  //Should WM_PAINT be handled atm or not.
		[[nodiscard]] bool IsMutableImpl()const;
		[[nodiscard]] bool IsOffsetAsHexImpl()const;
		[[nodiscard]] bool IsPageVisible()const;               //Returns m_fSectorVisible.
		[[nodiscard]] bool IsVirtualImpl()const;
		void ModifyWorker(const HEXCTRL::HEXMODIFY& hms, const auto& FuncWorker, HEXCTRL::SpanCByte spnOper); //Main "Modify" method with different workers.
		[[nodiscard]] auto OffsetToWstr(ULONGLONG ullOffset)const -> std::wstring; //Format offset as std::wstring.
		void OnCaretPosChange(ULONGLONG ullOffset);            //On changing caret position.
		auto OnChar(const MSG& msg) -> LRESULT;
		auto OnCommand(const MSG& msg) -> LRESULT;
		auto OnContextMenu(const MSG& msg) -> LRESULT;
		auto OnDestroy() -> LRESULT;
		auto OnDPIChangedAfterParent() -> LRESULT;
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
		auto OnMButtonDown(const MSG& msg) -> LRESULT;
		void OnModifyData();                                   //When data has been modified.
		auto OnMouseMove(const MSG& msg) -> LRESULT;
		auto OnMouseWheel(const MSG& msg) -> LRESULT;
		auto OnNCActivate(const MSG& msg) -> LRESULT;
		auto OnNCCalcSize(const MSG& msg) -> LRESULT;
		auto OnNCPaint(const MSG& msg) -> LRESULT;
		auto OnPaint() -> LRESULT;
		auto OnRButtonDown(const MSG& msg) -> LRESULT;
		auto OnSetCursor(const MSG& msg) -> LRESULT;
		auto OnSetFocus(const MSG& msg) -> LRESULT;
		auto OnSize(const MSG& msg) -> LRESULT;
		auto OnTimer(const MSG& msg) -> LRESULT;
		auto OnVScroll(const MSG& msg) -> LRESULT;
		template<typename T> requires std::is_class_v<T>
		void ParentNotify(const T& t)const;                    //Notify routine used to send messages to Parent window.
		void ParentNotify(UINT uCode)const;                    //Same as above, but only for notification code.
		void Print();                                          //Printing routine.
		void RecalcAll(bool fPrinter = false, HDC hDCPrinter = nullptr, LPCRECT pRCPrinter = nullptr); //Recalculates all sizes for window/printer DC.
		void RecalcClientArea(int iWidth, int iHeight);
		void Redo();
		void RedrawImpl();  //Internal implementation of the interface Redraw method.
		void ReplaceUnprintable(std::wstring& wstr, bool fASCII, bool fCRLF)const; //Substitute all unprintable wchar symbols with specified wchar.
		void ScrollOffsetH(ULONGLONG ullOffset); //Scroll horizontally to given offset.
		void SelAll();      //Select all.
		void SelAddDown();  //Down Key pressed with the Shift.
		void SelAddLeft();  //Left Key pressed with the Shift.
		void SelAddRight(); //Right Key pressed with the Shift.
		void SelAddUp();    //Up Key pressed with the Shift.
		void SetCapacityImpl(std::uint32_t dwCapacity, bool fRedraw = true, bool fNotify = true);
		void SetCodepageImpl(int iCodepage, bool fRedraw = true, bool fNotify = true);
		bool SetConfigImpl(std::wstring_view wsvPath);
		void SetDateInfoImpl(std::uint32_t dwFormat, wchar_t wchSepar);
		void SetDataVirtual(SpanByte spnData, const HEXSPAN& hss)const; //Sets data (notifies back) in VirtualData mode.
		void SetFontImpl(const LOGFONTW& lf, bool fMain, bool fRedraw = true, bool fNotify = true);
		void SetFontSizeInPoints(float flSizePoints, bool fMain); //Set font size in points.
		void SetGroupSizeImpl(DWORD dwSize, bool fRedraw = true, bool fNotify = true);
		void SetScrollCursor();
		void SetUnprintableCharImpl(wchar_t wch, bool fRedraw = true);
		void SnapshotUndo(const VecSpan& vecSpan); //Takes currently modifiable data snapshot.
		void TextChunkPoint(ULONGLONG ullOffset, int& iCx, int& iCy)const; //Point of the text chunk.
		void TTMainShow(bool fShow, bool fTimer = false); //Main tooltip show/hide.
		void TTOffsetShow(bool fShow); //Tooltip Offset show/hide.
		void Undo();
		void UpdateDPIScale(); //Set new DPI scale factor according to current DPI.
		static void ModifyOperScalar(std::byte* pData, const HEXMODIFY& hms, SpanCByte); //Modify operation scalar.
		static auto CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
			UINT_PTR uIDSubclass, DWORD_PTR dwRefData)->LRESULT;
	private:
		static constexpr auto m_pwszClassName { L"HexCtrl_MainWnd" }; //HexCtrl unique Window Class name.
		static constexpr auto m_uIDTTTMain { 0x01UL };                //Timer ID for default tooltip.
		static constexpr auto m_uIDTScrolCursor { 0x02UL };           //Timer ID for the scroll cursor.
		static constexpr auto m_iFirstHorzLinePx { 0 };               //First horizontal line indent.
		static constexpr auto m_iFirstVertLinePx { 0 };               //First vertical line indent.
		static constexpr auto m_dwVKMouseWheelUp { 0x0100UL };        //Artificial Virtual Key for a Mouse-Wheel Up event.
		static constexpr auto m_dwVKMouseWheelDown { 0x0101UL };      //Artificial Virtual Key for a Mouse-Wheel Down event.
		static constexpr auto m_dwVKMiddleButtonDown { 0x0102UL };    //Artificial Virtual Key for a Middle Button Down event.
		CHexDlgBkmMgr m_DlgBkmMgr;            //"Bookmark manager" dialog.
		CHexDlgCodepage m_DlgCodepage;        //"Codepage" dialog.
		CHexDlgDataInterp m_DlgDataInterp;    //"Data interpreter" dialog.
		CHexDlgGoTo m_DlgGoTo;                //"GoTo..." dialog.
		CHexDlgModify m_DlgModify;            //"Modify..." dialog.
		CHexDlgSearch m_DlgSearch;            //"Search..." dialog.
		CHexDlgTemplMgr m_DlgTemplMgr;        //"Template manager..." dialog.
		CHexSelection m_Selection;            //Selection class.
		CHexScroll m_ScrollV;                 //Vertical scroll bar.
		CHexScroll m_ScrollH;                 //Horizontal scroll bar.
		HINSTANCE m_hInstRes { };             //Hinstance of the HexCtrl resources.
		GDIUT::CWnd m_Wnd;                    //Main window.
		GDIUT::CWnd m_wndTTMain;              //Main tooltip window. It differs in creation flags from m_wndTTOffset.
		GDIUT::CWnd m_wndTTOffset;            //Tooltip window for Offset in m_fHighLatency mode.
		GDIUT::CMenu m_MenuMain;              //Main popup menu.
		GDIUT::CPoint m_ptScrollCursorClick;  //Scroll cursor click coordinates.
		std::wstring m_wstrPageName;          //Name of the sector/page.
		std::vector<std::unique_ptr<std::vector<UNDO>>> m_vecUndo; //Undo data.
		std::vector<std::unique_ptr<std::vector<UNDO>>> m_vecRedo; //Redo data.
		std::vector < std::unique_ptr < std::remove_pointer_t<HBITMAP>,
			decltype([](HBITMAP hBmp) { ::DeleteObject(hBmp); }) >> m_vecIconsMenu; //Icons for the Menu.
		std::vector<KEYBIND> m_vecKeyBind;    //Vector of key bindings.
		std::vector<int> m_vecCharsWidth;     //Vector of chars widths.
		HFONT m_hFntMain { };                 //Main Hex chunks font.
		HFONT m_hFntInfoBar { };              //Font for bottom Info bar.
		HPEN m_hPenLinesMain { };             //Pen for main lines.
		HPEN m_hPenLinesTempl { };            //Pen for templates' fields (vertical lines).
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
		DWORD m_dwDigitsOffsetDec { 10UL };   //Amount of digits for "Offset" in Decimal mode, 10 is max for 32bit number.
		DWORD m_dwDigitsOffsetHex { 8UL };    //Amount of digits for "Offset" in Hex mode, 8 is max for 32bit number.
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
		float m_flScrollRatio { };            //Ratio for how much to scroll with mouse-wheel.
		float m_flDPIScale { 1.F };           //DPI scale factor for window.
		wchar_t m_wchUnprintable { };         //Replacement char for unprintable characters.
		wchar_t m_wchDateSepar { };           //Date separator.
		bool m_fCreated { false };            //Is control created or not.
		bool m_fDataSet { false };            //Is data set or not.
		bool m_fMutable { false };            //Does control work in Edit or ReadOnly mode.
		bool m_fInfoBar { true };             //Show bottom Info window or not.
		bool m_fCaretHigh { true };           //Caret's High or Low bits position (first or last digit in hex chunk).
		bool m_fCursorTextArea { false };     //Whether last focus was set at ASCII or Hex chunks area.
		bool m_fLMousePressed { false };      //Is left mouse button pressed.
		bool m_fClickWithAlt { false };       //Mouse click was with Alt pressed.
		bool m_fOffsetHex { false };          //Print offset numbers as Hex or as Decimals.
		bool m_fHighLatency { false };        //Reflects HEXDATA::fHighLatency.
		bool m_fRedraw { true };              //Should WM_PAINT be handled or not.
		bool m_fScrollLines { false };        //Page scroll in "Screen * m_flScrollRatio" or in lines.
		bool m_fHexCharsUpper { true };       //Hex chars printed in UPPER or lower case.
		bool m_fScrollCursor { false };       //Is scroll cursor active atm?
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
		wc.lpfnWndProc = GDIUT::WndProc<CHexCtrl>;
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
	m_ScrollV.SetScrollPos(0);
	m_ScrollH.SetScrollPos(0);
	m_ScrollV.SetScrollSizes(0, 0, 0);
	m_DlgBkmMgr.RemoveAll();
	m_DlgDataInterp.ClearData();
	m_DlgSearch.ClearData();
	m_Selection.ClearAll();
	RedrawImpl();
}

bool CHexCtrl::Create(const HEXCREATE& hcs)
{
	if (IsCreated()) { ut::DBG_REPORT(L"Already created."); return false; }

	if (hcs.fCustom) {
		m_Wnd = ::GetDlgItem(hcs.hWndParent, hcs.uID);
		if (m_Wnd.IsNull()) {
			ut::DBG_REPORT(L"GetDlgItem failed.");
			return false;
		}

		if (::SetWindowSubclass(m_Wnd, SubclassProc, reinterpret_cast<UINT_PTR>(this), 0) == FALSE) {
			ut::DBG_REPORT(L"SubclassDlgItem failed, check HEXCREATE parameters.");
			return false;
		}
	}
	else {
		const GDIUT::CRect rc = hcs.rect;
		if (m_Wnd = ::CreateWindowExW(hcs.dwExStyle, m_pwszClassName, L"HexCtrl", hcs.dwStyle, rc.left, rc.top, rc.Width(),
			rc.Height(), hcs.hWndParent, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(hcs.uID)), nullptr, this); m_Wnd.IsNull()) {
			ut::DBG_REPORT(L"CreateWindowExW failed, check HEXCREATE parameters.");
			return false;
		}
	}

	//Actual scroll sizes are set in the RecalcAll.
	m_ScrollV.Create(m_Wnd, true, m_stColors.clrScrollBar, m_stColors.clrScrollThumb, m_stColors.clrScrollArrow, 0, 0, 0);
	m_ScrollH.Create(m_Wnd, false, m_stColors.clrScrollBar, m_stColors.clrScrollThumb, m_stColors.clrScrollArrow, 0, 0, 0);
	m_ScrollV.AddSibling(&m_ScrollH);
	m_ScrollH.AddSibling(&m_ScrollV);

	m_wndTTMain.Attach(::CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr, TTS_ALWAYSTIP | TTS_NOPREFIX,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_Wnd, nullptr, nullptr, nullptr));
	m_wndTTMain.SendMsg(TTM_SETMAXTIPWIDTH, 0, 400); //To allow the use of a newline \n.
	m_ttiMain.cbSize = sizeof(TTTOOLINFOW);
	m_ttiMain.uFlags = TTF_TRACK;
	m_wndTTMain.SendMsg(TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&m_ttiMain));

	m_wndTTOffset.Attach(::CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr,
		TTS_ALWAYSTIP | TTS_NOPREFIX | TTS_NOANIMATE | TTS_NOFADE,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_Wnd, nullptr, nullptr, nullptr));
	m_wndTTOffset.SendMsg(TTM_SETMAXTIPWIDTH, 0, 400); //To allow the use of a newline \n.
	m_ttiOffset.cbSize = sizeof(TTTOOLINFOW);
	m_ttiOffset.uFlags = TTF_TRACK;
	m_wndTTOffset.SendMsg(TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&m_ttiOffset));

	if (hcs.pColors != nullptr) {
		m_stColors = *hcs.pColors;
	}

	m_hInstRes = hcs.hInstRes != nullptr ? hcs.hInstRes : ut::GetCurrModuleHinst();
	m_dwCapacity = std::clamp(hcs.dwCapacity, 1UL, 100UL);
	m_flScrollRatio = hcs.flScrollRatio;
	m_fScrollLines = hcs.fScrollLines;
	m_fInfoBar = hcs.fInfoBar;
	m_fOffsetHex = hcs.fOffsetHex;

	UpdateDPIScale();
	CreateMenu();
	CreatePens();

	//Default main font.
	const LOGFONTW lfMain { .lfHeight { -FontScaledPixelsFromPoints(11.F) }, .lfWeight { FW_NORMAL },
		.lfQuality { CLEARTYPE_QUALITY }, .lfPitchAndFamily { FIXED_PITCH }, .lfFaceName { L"Consolas" } };
	SetFontImpl(hcs.pLogFont != nullptr ? *hcs.pLogFont : lfMain, true, false, false);

	//Info area font, independent from the main font, its size is a bit smaller than the default main font.
	const LOGFONTW lfInfo { .lfHeight { -FontScaledPixelsFromPoints(11.F) + 1 }, .lfWeight { FW_NORMAL },
		.lfQuality { CLEARTYPE_QUALITY }, .lfPitchAndFamily { FIXED_PITCH }, .lfFaceName { L"Consolas" } };
	SetFontImpl(lfInfo, false, false, false);

	SetCodepageImpl(-1, false, false);
	SetConfigImpl(L"");
	SetDateInfoImpl(0xFFFFFFFFUL, L'/');
	SetGroupSizeImpl(std::clamp(hcs.dwGroupSize, 1UL, 64UL), false, false);
	SetUnprintableCharImpl(L'.', false);

	//All dialogs are initialized after the main window, to set the parent handle correctly.
	m_DlgBkmMgr.Initialize(*this, m_hInstRes);
	m_DlgDataInterp.Initialize(*this, m_hInstRes);
	m_DlgCodepage.Initialize(*this, m_hInstRes);
	m_DlgGoTo.Initialize(*this, m_hInstRes);
	m_DlgSearch.Initialize(*this, m_hInstRes);
	m_DlgTemplMgr.Initialize(*this, m_hInstRes);
	m_DlgModify.Initialize(*this, m_hInstRes);

	RedrawImpl();

	return m_fCreated = true;
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
		m_DlgSearch.ShowWindow(SW_SHOW);
		break;
	case CMD_SEARCH_NEXT:
		m_DlgSearch.SearchNextPrev(true);
		m_Wnd.SetFocus();
		break;
	case CMD_SEARCH_PREV:
		m_DlgSearch.SearchNextPrev(false);
		m_Wnd.SetFocus();
		break;
	case CMD_NAV_GOTO_DLG:
		ParentNotify(HEXCTRL_MSG_DLGGOTO);
		m_DlgGoTo.ShowWindow(SW_SHOW);
		break;
	case CMD_NAV_REPFWD:
		m_DlgGoTo.Repeat();
		break;
	case CMD_NAV_REPBKW:
		m_DlgGoTo.Repeat(false);
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
		m_DlgBkmMgr.AddBkm(HEXBKM { .vecSpan { HasSelection() ? GetSelection()
			: VecSpan { { GetCaretPosImpl(), 1 } } },
			.stClr { .clrBk { GetColors().clrBkBkm }, .clrText { GetColors().clrFontBkm } } }, true);
		break;
	case CMD_BKM_REMOVE:
		m_DlgBkmMgr.RemoveByOffset(GetCaretPosImpl());
		break;
	case CMD_BKM_NEXT:
		m_DlgBkmMgr.GoNext();
		break;
	case CMD_BKM_PREV:
		m_DlgBkmMgr.GoPrev();
		break;
	case CMD_BKM_REMOVEALL:
		m_DlgBkmMgr.RemoveAll();
		break;
	case CMD_BKM_DLG_MGR:
		ParentNotify(HEXCTRL_MSG_DLGBKMMGR);
		m_DlgBkmMgr.ShowWindow(SW_SHOW);
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
		m_DlgModify.ShowWindow(SW_SHOW, 0);
		break;
	case CMD_MODIFY_FILLZEROS:
		FillWithZeros();
		break;
	case CMD_MODIFY_FILLDATA_DLG:
		ParentNotify(HEXCTRL_MSG_DLGMODIFY);
		m_DlgModify.ShowWindow(SW_SHOW, 1);
		break;
	case CMD_MODIFY_UNDO:
		Undo();
		break;
	case CMD_MODIFY_REDO:
		Redo();
		break;
	case CMD_SEL_MARKSTARTEND:
		m_Selection.SetMarkStartEnd(GetCaretPosImpl());
		RedrawImpl();
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
		m_DlgDataInterp.ShowWindow(SW_SHOW);
		break;
	case CMD_CODEPAGE_DLG:
		ParentNotify(HEXCTRL_MSG_DLGCODEPAGE);
		m_DlgCodepage.ShowWindow(SW_SHOW);
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
		SetCapacityImpl(GetCapacity() + 1);
		break;
	case CMD_APPEAR_CAPACDEC:
		SetCapacityImpl(GetCapacity() - 1);
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
	case CMD_SCROLL_CURSOR:
		SetScrollCursor();
		break;
	case CMD_SCROLL_PAGEUP:
		m_ScrollV.ScrollPageUp();
		break;
	case CMD_SCROLL_PAGEDOWN:
		m_ScrollV.ScrollPageDown();
		break;
	case CMD_TEMPL_APPLYCURR:
		m_DlgTemplMgr.ApplyCurr(GetCaretPosImpl());
		break;
	case CMD_TEMPL_DISAPPLY:
		m_DlgTemplMgr.DisapplyByOffset(GetCaretPosImpl());
		break;
	case CMD_TEMPL_DISAPPALL:
		m_DlgTemplMgr.DisapplyAll();
		break;
	case CMD_TEMPL_DLG_MGR:
		ParentNotify(HEXCTRL_MSG_DLGTEMPLMGR);
		m_DlgTemplMgr.ShowWindow(SW_SHOW);
		break;
	}
}

int CHexCtrl::GetActualWidth()const
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }

	return m_iFourthVertLinePx + 1; //+1px is the Pen width the line was drawn with.
}

auto CHexCtrl::GetBookmarks()->IHexBookmarks*
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }

	return &m_DlgBkmMgr;
}

auto CHexCtrl::GetCacheSize()const->DWORD
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }
	if (!IsDataSetImpl()) { ut::DBG_REPORT_NO_DATA_SET(); return { }; }

	return m_dwCacheSize;
}

auto CHexCtrl::GetCapacity()const->DWORD
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }

	return GetCapacityImpl();
}

auto CHexCtrl::GetCaretPos()const->ULONGLONG
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }
	if (!IsDataSetImpl()) { ut::DBG_REPORT_NO_DATA_SET(); return { }; }

	return GetCaretPosImpl();
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
	if (!IsDataSetImpl()) { ut::DBG_REPORT_NO_DATA_SET(); return { }; }
	assert(hss.ullSize > 0);

	SpanByte spnData;
	if (!IsVirtualImpl()) {
		if (hss.ullOffset + hss.ullSize <= GetDataSizeImpl()) {
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

	return GetDataSizeImpl();
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
		return m_DlgBkmMgr.GetDlgItemHandle(eItem);
	case DATAINTERP_CHK_HEX: case DATAINTERP_CHK_BE:
		return m_DlgDataInterp.GetDlgItemHandle(eItem);
	case FILLDATA_COMBO_DATA:
		return m_DlgModify.GetDlgItemHandle(eItem);
	case SEARCH_COMBO_FIND:	case SEARCH_COMBO_REPLACE: case SEARCH_EDIT_START:
	case SEARCH_EDIT_STEP: case SEARCH_EDIT_RNGBEG: case SEARCH_EDIT_RNGEND:
	case SEARCH_EDIT_LIMIT:
		return m_DlgSearch.GetDlgItemHandle(eItem);
	case TEMPLMGR_CHK_MIN: case TEMPLMGR_CHK_TT: case TEMPLMGR_CHK_HGL:
	case TEMPLMGR_CHK_HEX: case TEMPLMGR_CHK_SWAP:
		return m_DlgTemplMgr.GetDlgItemHandle(eItem);
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
	if (!IsDataSetImpl()) { ut::DBG_REPORT_NO_DATA_SET(); return { }; }

	return GetOffsetImpl(ullOffset, fGetVirt);
}

auto CHexCtrl::GetPagesCount()const->ULONGLONG
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }
	if (!IsDataSetImpl()) { ut::DBG_REPORT_NO_DATA_SET(); return { }; }

	return GetPagesCountImpl();
}

auto CHexCtrl::GetPagePos()const->ULONGLONG
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }
	if (!IsDataSetImpl()) { ut::DBG_REPORT_NO_DATA_SET(); return { }; }

	return GetPagePosImpl();
}

auto CHexCtrl::GetPageSize()const->DWORD
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }

	return GetPageSizeImpl();
}

auto CHexCtrl::GetScrollRatio()const->std::tuple<float, bool>
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }

	return { m_flScrollRatio, m_fScrollLines };
}

auto CHexCtrl::GetSelection()const->VecSpan
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }
	if (!IsDataSetImpl()) { ut::DBG_REPORT_NO_DATA_SET(); return { }; }

	return m_Selection.GetData();
}

auto CHexCtrl::GetTemplates()->IHexTemplates*
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { }; }

	return &m_DlgTemplMgr;
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
		if (!::IsWindow(m_DlgBkmMgr.GetHWND()) && fCreate) {
			m_DlgBkmMgr.CreateDlg();
		}
		return m_DlgBkmMgr.GetHWND();
	case EHexWnd::DLG_DATAINTERP:
		if (!::IsWindow(m_DlgDataInterp.GetHWND()) && fCreate) {
			m_DlgDataInterp.CreateDlg();
		}
		return m_DlgDataInterp.GetHWND();
	case EHexWnd::DLG_MODIFY:
		if (!::IsWindow(m_DlgModify.GetHWND()) && fCreate) {
			m_DlgModify.CreateDlg();
		}
		return m_DlgModify.GetHWND();
	case EHexWnd::DLG_SEARCH:
		if (!::IsWindow(m_DlgSearch.GetHWND()) && fCreate) {
			m_DlgSearch.CreateDlg();
		}
		return m_DlgSearch.GetHWND();
	case EHexWnd::DLG_CODEPAGE:
		if (!::IsWindow(m_DlgCodepage.GetHWND()) && fCreate) {
			m_DlgCodepage.CreateDlg();
		}
		return m_DlgCodepage.GetHWND();
	case EHexWnd::DLG_GOTO:
		if (!::IsWindow(m_DlgGoTo.GetHWND()) && fCreate) {
			m_DlgGoTo.CreateDlg();
		}
		return m_DlgGoTo.GetHWND();
	case EHexWnd::DLG_TEMPLMGR:
		if (!::IsWindow(m_DlgTemplMgr.GetHWND()) && fCreate) {
			m_DlgTemplMgr.CreateDlg();
		}
		return m_DlgTemplMgr.GetHWND();
	default:
		return { };
	}
}

void CHexCtrl::GoToOffset(ULONGLONG ullOffset, int iPosAt)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }
	if (!IsDataSetImpl()) { ut::DBG_REPORT_NO_DATA_SET(); return; }
	if (ullOffset >= GetDataSizeImpl()) return;

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
	m_ScrollV.SetScrollPos(ullNewScrollV);

	if (m_ScrollH.IsVisible() && !IsCurTextArea()) {
		auto ullNewScrollH = (ullOffset % dwCapacity) * m_iSizeHexBytePx;
		ullNewScrollH += (ullNewScrollH / m_iDistanceGroupedHexChunkPx) * GetCharWidthExtras();
		m_ScrollH.SetScrollPos(ullNewScrollH);
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

	return m_Selection.HasSelection();
}

auto CHexCtrl::HitTest(POINT pt, bool fScreen)const->std::optional<HEXHITTEST>
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return std::nullopt; }
	if (!IsDataSetImpl()) { ut::DBG_REPORT_NO_DATA_SET(); return std::nullopt; }

	if (fScreen) {
		m_Wnd.ScreenToClient(&pt);
	}

	return HitTest(pt);
}

bool CHexCtrl::IsCmdAvail(EHexCmd eCmd)const
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return false; }

	const auto fDataSet = IsDataSetImpl();
	const auto fMutable = fDataSet ? IsMutableImpl() : false;
	const auto fSelection = fDataSet && HasSelection();
	bool fAvail;

	using enum EHexCmd;
	switch (eCmd) {
	case CMD_BKM_REMOVE:
		fAvail = fDataSet && m_DlgBkmMgr.HasBookmark(GetCaretPosImpl());
		break;
	case CMD_BKM_NEXT:
	case CMD_BKM_PREV:
	case CMD_BKM_REMOVEALL:
		fAvail = fDataSet && m_DlgBkmMgr.HasBookmarks();
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
	case CMD_BKM_DLG_MGR:
	case CMD_CARET_RIGHT:
	case CMD_CARET_LEFT:
	case CMD_CARET_DOWN:
	case CMD_CARET_UP:
	case CMD_CLPBRD_COPY_OFFSET:
	case CMD_NAV_GOTO_DLG:
	case CMD_NAV_DATABEG:
	case CMD_NAV_DATAEND:
	case CMD_NAV_LINEBEG:
	case CMD_NAV_LINEEND:
	case CMD_SEARCH_DLG:
	case CMD_SEL_MARKSTARTEND:
	case CMD_SEL_ALL:
		fAvail = fDataSet;
		break;
	case CMD_SEARCH_NEXT:
	case CMD_SEARCH_PREV:
		fAvail = m_DlgSearch.IsSearchAvail();
		break;
	case CMD_NAV_PAGEBEG:
	case CMD_NAV_PAGEEND:
		fAvail = fDataSet && GetPagesCountImpl() > 0;
		break;
	case CMD_NAV_REPFWD:
	case CMD_NAV_REPBKW:
		fAvail = fDataSet && m_DlgGoTo.IsRepeatAvail();
		break;
	case CMD_TEMPL_APPLYCURR:
		fAvail = fDataSet && m_DlgTemplMgr.HasCurrent();
		break;
	case CMD_TEMPL_DISAPPLY:
		fAvail = fDataSet && m_DlgTemplMgr.HasApplied() && m_DlgTemplMgr.HitTest(GetCaretPosImpl()) != nullptr;
		break;
	case CMD_TEMPL_DISAPPALL:
		fAvail = fDataSet && m_DlgTemplMgr.HasApplied();
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

	return IsDataSetImpl();
}

bool CHexCtrl::IsHexCharsUpper()const
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return false; }

	return m_fHexCharsUpper;
}

bool CHexCtrl::IsMutable()const
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return false; }
	if (!IsDataSetImpl()) { ut::DBG_REPORT_NO_DATA_SET(); return false; }

	return IsMutableImpl();
}

bool CHexCtrl::IsOffsetAsHex()const
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return false; }

	return IsOffsetAsHexImpl();
}

auto CHexCtrl::IsOffsetVisible(ULONGLONG ullOffset)const->HEXVISION
{
	//Returns HEXVISION with two std::int8_t for vertical and horizontal visibility respectively.
	//-1 - ullOffset is higher, or at the left, of the visible area
	// 1 - lower, or at the right
	// 0 - visible.
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return { .i8Vert { -1 }, .i8Horz { -1 } }; }
	if (!IsDataSetImpl()) { ut::DBG_REPORT_NO_DATA_SET(); return { .i8Vert { -1 }, .i8Horz { -1 } }; }

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
	if (!IsDataSetImpl()) { ut::DBG_REPORT_NO_DATA_SET(); return false; }

	return IsVirtualImpl();
}

void CHexCtrl::ModifyData(const HEXMODIFY& hms)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }
	if (!IsMutableImpl()) return;
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

		assert((ullOffsetToModify + ullSizeToModify) <= GetDataSizeImpl());
		if ((ullOffsetToModify + ullSizeToModify) > GetDataSizeImpl())
			return;

		if (IsVirtualImpl() && ullSizeToModify > GetCacheSize()) {
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
		const auto lmbRandUInt64 = [&](std::byte* pData, const HEXMODIFY&, SpanCByte) {
			assert(pData != nullptr);
			*reinterpret_cast<std::uint64_t*>(pData) = distUInt64(gen);
			};
		const auto lmbRandByte = [&](std::byte* pData, const HEXMODIFY&, SpanCByte) {
			assert(pData != nullptr);
			*pData = static_cast<std::byte>(distUInt64(gen));
			};
		const auto lmbRandFast = [](std::byte* pData, const HEXMODIFY&, SpanCByte spnDataFrom) {
			assert(pData != nullptr);
			std::copy_n(spnDataFrom.data(), spnDataFrom.size(), pData);
			};

		const auto& hs = hms.vecSpan.back();
		if (hms.eModifyMode == MODIFY_RAND_MT19937 && hms.vecSpan.size() == 1 && hs.ullSize >= sizeof(std::uint64_t)) {
			ModifyWorker(hms, lmbRandUInt64, { static_cast<std::byte*>(nullptr), sizeof(std::uint64_t) });

			if (const auto dwRem = hs.ullSize % sizeof(std::uint64_t); dwRem > 0) { //Remainder.
				const auto ullOffset = hs.ullOffset + hs.ullSize - dwRem;
				const auto spnData = GetData({ .ullOffset { ullOffset }, .ullSize { dwRem } });
				for (std::size_t itRem = 0; itRem < dwRem; ++itRem) {
					spnData.data()[itRem] = static_cast<std::byte>(distUInt64(gen));
				}
				SetDataVirtual(spnData, { .ullOffset { ullOffset }, .ullSize { dwRem } });
			}
		}
		else if (hms.eModifyMode == MODIFY_RAND_FAST && hms.vecSpan.size() == 1 && hs.ullSize >= GetCacheSize()) {
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
			if (const auto ullRem = hs.ullSize % ulSizeRandBuff; ullRem > 0) { //Remainder.
				if (ullRem <= GetCacheSize()) {
					const auto ullOffsetCurr = hs.ullOffset + hs.ullSize - ullRem;
					const auto spnData = GetData({ .ullOffset { ullOffsetCurr }, .ullSize { ullRem } });
					assert(!spnData.empty());
					std::copy_n(uptrRandData.get(), ullRem, spnData.data());
					SetDataVirtual(spnData, { .ullOffset { ullOffsetCurr }, .ullSize { ullRem } });
				}
				else {
					const auto ullSizeCache = GetCacheSize();
					const auto dwModCache = ullRem % ullSizeCache;
					auto ullChunks = (ullRem / ullSizeCache) + (dwModCache > 0 ? 1 : 0);
					auto ullOffsetCurr = hs.ullOffset + hs.ullSize - ullRem;
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
		//Special case for the OPER_ASSIGN operation. This operation can easily be replaced 
		//with the MODIFY_REPEAT mode, which is significantly faster.

		if (hms.eOperMode == OPER_ASSIGN) {
			HEXMODIFY hmsRepeat = hms;
			hmsRepeat.eModifyMode = MODIFY_REPEAT;
			std::uint64_t u64Data { };

			switch (hms.eDataType) {
			case DATA_INT16:
			case DATA_UINT16:
			{
				auto u16 = *reinterpret_cast<const std::uint16_t*>(hms.spnData.data());
				if (hms.fBigEndian) { u16 = ut::ByteSwap(u16); }
				u64Data = u16;
				hmsRepeat.spnData = { reinterpret_cast<const std::byte*>(&u64Data), sizeof(std::uint16_t) };
			}
			break;
			case DATA_INT32:
			case DATA_UINT32:
			case DATA_FLOAT:
			{
				auto u32 = *reinterpret_cast<const std::uint32_t*>(hms.spnData.data());
				if (hms.fBigEndian) { u32 = ut::ByteSwap(u32); }
				u64Data = u32;
				hmsRepeat.spnData = { reinterpret_cast<const std::byte*>(&u64Data), sizeof(std::uint32_t) };
			}
			break;
			case DATA_INT64:
			case DATA_UINT64:
			case DATA_DOUBLE:
			{
				auto u64 = *reinterpret_cast<const std::uint64_t*>(hms.spnData.data());
				if (hms.fBigEndian) { u64 = ut::ByteSwap(u64); }
				u64Data = u64;
				hmsRepeat.spnData = { reinterpret_cast<const std::byte*>(&u64Data), sizeof(std::uint64_t) };
			}
			break;
			default:
				break;
			};

			ModifyData(hmsRepeat);
			return;
		}

		//In cases where the only one affected data region (hms.vecSpan.size()==1) is used,
		//and ullSizeToModify > ulSizeOfVec, we use SIMD.
		//At the end we simply fill up the remainder (ullSizeToModify % ulSizeOfVec).
		const auto ulSizeOfVec { simd::VecTypeToSize(simd::GetVectorType()) };
		const auto ullOffsetToModify = hms.vecSpan.back().ullOffset;
		const auto ullSizeToModify = hms.vecSpan.back().ullSize;
		const auto ullSizeToFillWith = hms.spnData.size();

		if (hms.vecSpan.size() == 1 && ((ullSizeToModify / ulSizeOfVec) > 0)) {
			using PFuncWorker = void(*)(std::byte* pData, const HEXCTRL::HEXMODIFY& hms, HEXCTRL::SpanCByte);
			PFuncWorker pFuncWorker;
			switch (simd::GetVectorType()) {
			case simd::EVecType::VECTOR_128:
				pFuncWorker = simd::ModifyOperVec<simd::EVecType::VECTOR_128>;
				break;
			case simd::EVecType::VECTOR_256:
				pFuncWorker = simd::ModifyOperVec<simd::EVecType::VECTOR_256>;
				break;
			default: return;
			}

			ModifyWorker(hms, pFuncWorker, { static_cast<std::byte*>(nullptr), ulSizeOfVec }); //Vector worker.

			if (const auto ullRem = ullSizeToModify % ulSizeOfVec; ullRem >= ullSizeToFillWith) { //Remainder of the vector data.
				const auto ullOffset = ullOffsetToModify + ullSizeToModify - ullRem;
				const auto spnData = GetData({ .ullOffset { ullOffset }, .ullSize { ullRem } });
				for (std::size_t itRem = 0; itRem < (ullRem / ullSizeToFillWith); ++itRem) { //Works only if ullRem >= ullSizeToFillWith.
					ModifyOperScalar(spnData.data() + (itRem * ullSizeToFillWith), hms, { });
				}
				SetDataVirtual(spnData, { .ullOffset { ullOffset }, .ullSize { ullRem - (ullRem % ullSizeToFillWith) } });
			}
		}
		else {
			ModifyWorker(hms, ModifyOperScalar, hms.spnData);
		}
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
	if (m_DlgBkmMgr.PreTranslateMsg(pMsg)) { return true; }
	if (m_DlgDataInterp.PreTranslateMsg(pMsg)) { return true; }
	if (m_DlgModify.PreTranslateMsg(pMsg)) { return true; }
	if (m_DlgSearch.PreTranslateMsg(pMsg)) { return true; }
	if (m_DlgCodepage.PreTranslateMsg(pMsg)) { return true; }
	if (m_DlgGoTo.PreTranslateMsg(pMsg)) { return true; }
	if (m_DlgTemplMgr.PreTranslateMsg(pMsg)) { return true; }

	return false;
}

auto CHexCtrl::ProcessMsg(const MSG& msg)->LRESULT
{
	switch (msg.message) {
	case WM_CHAR: return OnChar(msg);
	case WM_COMMAND: return OnCommand(msg);
	case WM_CONTEXTMENU: return OnContextMenu(msg);
	case WM_DESTROY: return OnDestroy();
	case WM_DPICHANGED_AFTERPARENT: return OnDPIChangedAfterParent();
	case WM_ERASEBKGND: return OnEraseBkgnd(msg);
	case WM_GETDLGCODE: return OnGetDlgCode(msg);
	case WM_HELP: return OnHelp(msg);
	case WM_HSCROLL: return OnHScroll(msg);
	case WM_INITMENUPOPUP: return OnInitMenuPopup(msg);
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN: return OnKeyDown(msg);
	case WM_KEYUP: return OnKeyUp(msg);
	case WM_LBUTTONDBLCLK: return OnLButtonDblClk(msg);
	case WM_LBUTTONDOWN: return OnLButtonDown(msg);
	case WM_LBUTTONUP: return OnLButtonUp(msg);
	case WM_MBUTTONDOWN: return OnMButtonDown(msg);
	case WM_MOUSEMOVE: return OnMouseMove(msg);
	case WM_MOUSEWHEEL: return OnMouseWheel(msg);
	case WM_NCACTIVATE: return OnNCActivate(msg);
	case WM_NCCALCSIZE: return OnNCCalcSize(msg);
	case WM_NCPAINT: return OnNCPaint(msg);
	case WM_PAINT: return OnPaint();
	case WM_RBUTTONDOWN: return OnRButtonDown(msg);
	case WM_SETCURSOR: return OnSetCursor(msg);
	case WM_SETFOCUS: return OnSetFocus(msg);
	case WM_SIZE: return OnSize(msg);
	case WM_TIMER: return OnTimer(msg);
	case WM_VSCROLL: return OnVScroll(msg);
	default: return GDIUT::DefWndProc(msg);
	}
}

void CHexCtrl::Redraw()
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	RedrawImpl();
}

void CHexCtrl::SetCapacity(DWORD dwCapacity)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	SetCapacityImpl(dwCapacity);
}

void CHexCtrl::SetCaretPos(ULONGLONG ullOffset, bool fHighLow, bool fRedraw)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }
	if (!IsDataSetImpl()) { ut::DBG_REPORT_NO_DATA_SET(); return; }
	if (ullOffset >= GetDataSizeImpl()) { ut::DBG_REPORT(L"Offset is out of data range."); return; };

	m_ullCaretPos = ullOffset;
	m_fCaretHigh = fHighLow;

	if (fRedraw) {
		RedrawImpl();
	}

	OnCaretPosChange(ullOffset);
}

void CHexCtrl::SetCharsExtraSpace(DWORD dwSpace)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	m_dwCharsExtraSpace = (std::min)(dwSpace, 10UL);
	RecalcAll();
	RedrawImpl();
}

void CHexCtrl::SetCodepage(int iCodepage)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	SetCodepageImpl(iCodepage);
}

void CHexCtrl::SetColors(const HEXCOLORS& hcs)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	m_stColors = hcs;
	CreatePens();
	m_ScrollV.SetColors(hcs.clrScrollBar, hcs.clrScrollThumb, hcs.clrScrollArrow);
	m_ScrollH.SetColors(hcs.clrScrollBar, hcs.clrScrollThumb, hcs.clrScrollArrow);
	RedrawImpl();
}

bool CHexCtrl::SetConfig(std::wstring_view wsvPath)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return false; }

	return SetConfigImpl(wsvPath);
}

void CHexCtrl::SetData(const HEXDATA& hds, bool fAdjust)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }
	if (hds.spnData.empty()) { ut::DBG_REPORT(L"Data size can't be zero."); return; }

	if (fAdjust) {
		if (!IsDataSetImpl()) {
			ut::DBG_REPORT(L"Nothing to adjust, data must be set first.");
			return;
		}

		if (hds.spnData.size() != GetDataSizeImpl()) {
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
		m_dwDigitsOffsetDec = 10UL;
		m_dwDigitsOffsetHex = 8UL;
	}
	else if (ullDataSize <= 0xFFFFFFFFFFUL) {
		m_dwDigitsOffsetDec = 13UL;
		m_dwDigitsOffsetHex = 10UL;
	}
	else if (ullDataSize <= 0xFFFFFFFFFFFFUL) {
		m_dwDigitsOffsetDec = 15UL;
		m_dwDigitsOffsetHex = 12UL;
	}
	else if (ullDataSize <= 0xFFFFFFFFFFFFFFUL) {
		m_dwDigitsOffsetDec = 17UL;
		m_dwDigitsOffsetHex = 14UL;
	}
	else {
		m_dwDigitsOffsetDec = 19UL;
		m_dwDigitsOffsetHex = 16UL;
	}

	m_fDataSet = true;
	RecalcAll();
	RedrawImpl();
	m_DlgDataInterp.UpdateData(); //Update data if DI dialog is opened.
}

void CHexCtrl::SetDateInfo(DWORD dwFormat, wchar_t wchSepar)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	SetDateInfoImpl(dwFormat, wchSepar);
}

void CHexCtrl::SetDlgProperties(EHexWnd eWnd, std::uint64_t u64Flags)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	using enum EHexWnd;
	switch (eWnd) {
	case DLG_BKMMGR:
		m_DlgBkmMgr.SetDlgProperties(u64Flags);
		break;
	case DLG_DATAINTERP:
		m_DlgDataInterp.SetDlgProperties(u64Flags);
		break;
	case DLG_MODIFY:
		m_DlgModify.SetDlgProperties(u64Flags);
		break;
	case DLG_SEARCH:
		m_DlgSearch.SetDlgProperties(u64Flags);
		break;
	case DLG_CODEPAGE:
		m_DlgCodepage.SetDlgProperties(u64Flags);
		break;
	case DLG_GOTO:
		m_DlgGoTo.SetDlgProperties(u64Flags);
		break;
	case DLG_TEMPLMGR:
		m_DlgTemplMgr.SetDlgProperties(u64Flags);
		break;
	default:
		break;
	}
}

void CHexCtrl::SetFont(const LOGFONTW& lf, bool fMain)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	SetFontImpl(lf, fMain);
}

void CHexCtrl::SetGroupSize(DWORD dwSize)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	SetGroupSizeImpl(dwSize);
}

void CHexCtrl::SetHexCharsCase(bool fUpper)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	m_fHexCharsUpper = fUpper;
	RedrawImpl();
}

void CHexCtrl::SetMutable(bool fMutable)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }
	if (!IsDataSetImpl()) { ut::DBG_REPORT_NO_DATA_SET(); return; }

	m_fMutable = fMutable;
	RedrawImpl();
}

void CHexCtrl::SetOffsetMode(bool fHex)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	m_fOffsetHex = fHex;
	RecalcAll();
	RedrawImpl();
}

void CHexCtrl::SetPageSize(DWORD dwSize, std::wstring_view wsvName)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	m_dwPageSize = dwSize;
	m_wstrPageName = wsvName;
	if (IsDataSetImpl()) {
		RedrawImpl();
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
	m_ScrollV.SetScrollPageSize(GetScrollPageSize());
}

void CHexCtrl::SetSelection(const VecSpan& vecSel, bool fRedraw, bool fHighlight)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }
	if (!IsDataSetImpl()) { ut::DBG_REPORT_NO_DATA_SET(); return; }

	m_Selection.SetSelection(vecSel, fHighlight);

	if (fRedraw) {
		RedrawImpl();
	}

	ParentNotify(HEXCTRL_MSG_SETSELECTION);
}

void CHexCtrl::SetUnprintableChar(wchar_t wch)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	SetUnprintableCharImpl(wch);
}

void CHexCtrl::SetVirtualBkm(IHexBookmarks* pVirtBkm)
{
	if (!IsCreated()) { ut::DBG_REPORT_NOT_CREATED(); return; }

	m_DlgBkmMgr.SetVirtual(pVirtBkm);
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
	RedrawImpl();
}


//CHexCtrl Private methods.

auto CHexCtrl::BuildDataToDraw(ULONGLONG ullStartLine, int iLines)const->std::tuple<std::wstring, std::wstring>
{
	if (!IsDataSetImpl())
		return { };

	const auto ullOffsetStart = ullStartLine * GetCapacity(); //Offset of the visible data to print.
	const auto ullDataSize = GetDataSizeImpl();
	auto uzSizeDataToPrint = static_cast<std::size_t>(iLines) * GetCapacity(); //Size of the visible data to print.
	if (ullOffsetStart + uzSizeDataToPrint > ullDataSize) {
		uzSizeDataToPrint = static_cast<std::size_t>(ullDataSize - ullOffsetStart);
	}

	const auto spnData = GetData({ .ullOffset { ullOffsetStart }, .ullSize { uzSizeDataToPrint } });
	assert(!spnData.empty());
	assert(spnData.size() >= uzSizeDataToPrint);
	const auto pDataBegin = reinterpret_cast<unsigned char*>(spnData.data()); //Pointer to data to print.
	const auto pDataEnd = pDataBegin + uzSizeDataToPrint;

	//Hex Bytes to print.
	std::wstring wstrHex;
	wstrHex.reserve(uzSizeDataToPrint * 2);
	const auto pwszHexChars = GetHexChars();
	for (auto itData = pDataBegin; itData < pDataEnd; ++itData) { //Converting bytes to Hexes.
		wstrHex.push_back(pwszHexChars[(*itData >> 4) & 0x0F]);
		wstrHex.push_back(pwszHexChars[*itData & 0x0F]);
	}

	//Text to print.
	std::wstring wstrText;
	const auto iCodepage = GetCodepage();
	if (iCodepage == -1) { //ASCII codepage: we simply assigning [pDataBegin...pDataEnd) to wstrText w/o any conversion.
		wstrText.assign(pDataBegin, pDataEnd);
	}
	else if (iCodepage == 0) { //UTF-16.
		const auto pDataUTF16Beg = reinterpret_cast<wchar_t*>(pDataBegin);
		const auto pDataUTF16End = reinterpret_cast<wchar_t*>((uzSizeDataToPrint % 2) == 0 ? pDataEnd : pDataEnd - 1);
		wstrText.assign(pDataUTF16Beg, pDataUTF16End);
		wstrText.resize(uzSizeDataToPrint);
	}
	else {
		wstrText.resize(uzSizeDataToPrint);
		::MultiByteToWideChar(iCodepage, 0, reinterpret_cast<LPCCH>(pDataBegin),
			static_cast<int>(uzSizeDataToPrint), wstrText.data(), static_cast<int>(uzSizeDataToPrint));
	}

	ReplaceUnprintable(wstrText, iCodepage == -1, true);

	return { std::move(wstrHex), std::move(wstrText) };
}

void CHexCtrl::CaretMoveDown()
{
	const auto ullOldPos = m_ullCaretPos;
	const auto ullNewPos = ullOldPos + GetCapacity() >= GetDataSizeImpl() ? ullOldPos : ullOldPos + GetCapacity();
	SetCaretPos(ullNewPos, m_fCaretHigh, false);

	const auto stOld = IsOffsetVisible(ullOldPos);
	const auto stNew = IsOffsetVisible(ullNewPos);
	if (stOld.i8Vert == 0 && stNew.i8Vert != 0) {
		m_ScrollV.ScrollLineDown();
	}

	RedrawImpl();
}

void CHexCtrl::CaretMoveLeft()
{
	const auto ullOldPos = m_ullCaretPos;
	auto ullNewPos { 0ULL };

	if (IsCurTextArea() || !IsMutableImpl()) {
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
		m_ScrollV.ScrollLineUp();
	}
	else if (stNew.i8Horz != 0 && !IsCurTextArea()) { //Do not horz scroll when in text area.
		ScrollOffsetH(ullNewPos);
	}

	RedrawImpl();
}

void CHexCtrl::CaretMoveRight()
{
	if (!IsDataSetImpl())
		return;

	const auto ullOldPos = m_ullCaretPos;
	auto ullNewPos { 0ULL };

	if (IsCurTextArea() || !IsMutableImpl()) {
		ullNewPos = ullOldPos + 1;
	}
	else {
		if (m_fCaretHigh) {
			ullNewPos = ullOldPos;
			m_fCaretHigh = false;
		}
		else {
			ullNewPos = ullOldPos + 1;
			m_fCaretHigh = ullOldPos != GetDataSizeImpl() - 1; //Last byte case.
		}
	}

	if (const auto ullDataSize = GetDataSizeImpl(); ullNewPos >= ullDataSize) { //To avoid overflow.
		ullNewPos = ullDataSize - 1;
	}

	SetCaretPos(ullNewPos, m_fCaretHigh, false);

	const auto stOld = IsOffsetVisible(ullOldPos);
	const auto stNew = IsOffsetVisible(ullNewPos);
	if (stOld.i8Vert == 0 && stNew.i8Vert != 0) {
		m_ScrollV.ScrollLineDown();
	}
	else if (stNew.i8Horz != 0 && !IsCurTextArea()) { //Do not horz scroll when in text area.
		ScrollOffsetH(ullNewPos);
	}

	RedrawImpl();
}

void CHexCtrl::CaretMoveUp()
{
	const auto ullOldPos = m_ullCaretPos;
	const auto ullNewPos = ullOldPos >= GetCapacity() ? ullOldPos - GetCapacity() : ullOldPos;
	SetCaretPos(ullNewPos, m_fCaretHigh, false);

	const auto stOld = IsOffsetVisible(ullOldPos);
	const auto stNew = IsOffsetVisible(ullNewPos);
	if (stOld.i8Vert == 0 && stNew.i8Vert != 0) {
		m_ScrollV.ScrollLineUp();
	}

	RedrawImpl();
}

void CHexCtrl::CaretToDataBeg()
{
	SetCaretPos(0ULL, true);
	GoToOffset(0ULL);
}

void CHexCtrl::CaretToDataEnd()
{
	const auto ullPos = GetDataSizeImpl() - 1;
	SetCaretPos(ullPos);
	GoToOffset(ullPos);
}

void CHexCtrl::CaretToLineBeg()
{
	const auto dwCapacity = GetCapacity() > 0 ? GetCapacity() : 0xFFFFFFFFUL; //To suppress warning C4724.
	const auto ullPos = GetCaretPosImpl() - (GetCaretPosImpl() % dwCapacity);
	SetCaretPos(ullPos);
	if (!IsOffsetVisible(ullPos)) {
		GoToOffset(ullPos);
	}
}

void CHexCtrl::CaretToLineEnd()
{
	auto ullPos = GetCaretPosImpl() + (GetCapacity() - (GetCaretPosImpl() % GetCapacity())) - 1;
	if (ullPos >= GetDataSizeImpl()) {
		ullPos = GetDataSizeImpl() - 1;
	}
	SetCaretPos(ullPos);
	if (!IsOffsetVisible(ullPos)) {
		GoToOffset(ullPos);
	}
}

void CHexCtrl::CaretToPageBeg()
{
	if (GetPageSizeImpl() == 0)
		return;

	const auto ullPos = GetCaretPosImpl() - (GetCaretPosImpl() % GetPageSizeImpl());
	SetCaretPos(ullPos);
	if (!IsOffsetVisible(ullPos)) {
		GoToOffset(ullPos);
	}
}

void CHexCtrl::CaretToPageEnd()
{
	if (GetPageSizeImpl() == 0)
		return;

	auto ullPos = GetCaretPosImpl() + (GetPageSizeImpl() - (GetCaretPosImpl() % GetPageSizeImpl())) - 1;
	if (ullPos >= GetDataSizeImpl()) {
		ullPos = GetDataSizeImpl() - 1;
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
	if (m_Selection.GetSelSize() > 1024 * 1024 * 8) { //8MB
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

	constexpr auto uzCharSize { sizeof(wchar_t) };
	const std::size_t uzMemSize = (wstrData.size() * uzCharSize) + uzCharSize;
	const auto hMem = ::GlobalAlloc(GMEM_MOVEABLE, uzMemSize);
	if (!hMem) {
		ut::DBG_REPORT(L"GlobalAlloc error.");
		return;
	}

	const auto lpMemLock = ::GlobalLock(hMem);
	if (!lpMemLock) {
		ut::DBG_REPORT(L"GlobalLock error.");
		return;
	}

	std::memcpy(lpMemLock, wstrData.data(), uzMemSize);
	::GlobalUnlock(hMem);
	::OpenClipboard(m_Wnd);
	::EmptyClipboard();
	::SetClipboardData(CF_UNICODETEXT, hMem);
	::CloseClipboard();
}

void CHexCtrl::ClipboardPaste(EClipboard eType)
{
	if (!IsMutableImpl() || !::OpenClipboard(m_Wnd))
		return;

	const auto hClpbrd = ::GetClipboardData(CF_UNICODETEXT);
	if (!hClpbrd)
		return;

	const auto uzSizeClpbrd = ::GlobalSize(hClpbrd);
	if (uzSizeClpbrd == 0) {
		::CloseClipboard();
		return;
	}

	const auto pDataClpbrd = static_cast<wchar_t*>(::GlobalLock(hClpbrd));
	if (pDataClpbrd == nullptr) {
		::CloseClipboard();
		return;
	}

	const auto ullDataSize = GetDataSizeImpl();
	const auto ullCaretPos = GetCaretPosImpl();
	HEXMODIFY hmd;
	ULONGLONG ullSizeModify;
	std::string strDataModify; //Actual data to paste, must outlive hmd.
	const auto lmbPasteUTF16 = [&]() {
		ullSizeModify = uzSizeClpbrd;
		if (ullCaretPos + uzSizeClpbrd > ullDataSize) {
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
	RedrawImpl();
}

auto CHexCtrl::CopyBase64()const->std::wstring
{
	static constexpr auto pwszBase64Map { L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" };
	const auto ullSelSize = m_Selection.GetSelSize();
	std::wstring wstrData;
	wstrData.reserve(static_cast<std::size_t>(ullSelSize) * 2);
	auto uValA = 0U;
	auto iValB = -6;
	for (auto i { 0U }; i < ullSelSize; ++i) {
		uValA = (uValA << 8) + ut::GetIHexTData<BYTE>(*this, m_Selection.GetOffsetByIndex(i));
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
	const auto ullSelSize = m_Selection.GetSelSize();
	const auto pwszHexChars = GetHexChars();
	wstrData.reserve((static_cast<std::size_t>(ullSelSize) * 3) + 64);
	wstrData = std::format(L"unsigned char data[{}] = {{\r\n", ullSelSize);

	for (auto i { 0U }; i < ullSelSize; ++i) {
		wstrData += L"0x";
		const auto chByte = ut::GetIHexTData<BYTE>(*this, m_Selection.GetOffsetByIndex(i));
		wstrData += pwszHexChars[(chByte & 0xF0) >> 4];
		wstrData += pwszHexChars[(chByte & 0x0F)];
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
	const auto ullSelSize = m_Selection.GetSelSize();
	const auto pwszHexChars = GetHexChars();

	wstrData.reserve(static_cast<std::size_t>(ullSelSize) * 2);
	for (auto i { 0U }; i < ullSelSize; ++i) {
		wstrData += L"\\x";
		const auto chByte = ut::GetIHexTData<BYTE>(*this, m_Selection.GetOffsetByIndex(i));
		wstrData += pwszHexChars[(chByte & 0xF0) >> 4];
		wstrData += pwszHexChars[(chByte & 0x0F)];
	}

	return wstrData;
}

auto CHexCtrl::CopyHex()const->std::wstring
{
	std::wstring wstrData;
	const auto ullSelSize = m_Selection.GetSelSize();
	const auto pwszHexChars = GetHexChars();

	wstrData.reserve(static_cast<std::size_t>(ullSelSize) * 2);
	for (auto i { 0U }; i < ullSelSize; ++i) {
		const auto chByte = ut::GetIHexTData<BYTE>(*this, m_Selection.GetOffsetByIndex(i));
		wstrData += pwszHexChars[(chByte & 0xF0) >> 4];
		wstrData += pwszHexChars[(chByte & 0x0F)];
	}

	return wstrData;
}

auto CHexCtrl::CopyHexFmt()const->std::wstring
{
	std::wstring wstrData;
	const auto ullSelStart = m_Selection.GetSelStart();
	const auto ullSelSize = m_Selection.GetSelSize();
	const auto dwGroupSize = GetGroupSize();
	const auto dwCapacity = GetCapacity();
	const auto pwszHexChars = GetHexChars();

	wstrData.reserve(static_cast<std::size_t>(ullSelSize) * 3);
	if (m_Selection.HasContiguousSel()) {
		auto dwTail = m_Selection.GetLineLength();
		for (auto i { 0U }; i < ullSelSize; ++i) {
			const auto chByte = ut::GetIHexTData<BYTE>(*this, m_Selection.GetOffsetByIndex(i));
			wstrData += pwszHexChars[(chByte & 0xF0) >> 4];
			wstrData += pwszHexChars[(chByte & 0x0F)];

			if (i < (ullSelSize - 1) && (dwTail - 1) != 0) {
				if (((m_Selection.GetLineLength() - dwTail + 1) % dwGroupSize) == 0) { //Add space after hex full chunk, grouping size depending.
					wstrData += L" ";
				}
			}
			if (--dwTail == 0 && i < (ullSelSize - 1)) { //Next row.
				wstrData += L"\r\n";
				dwTail = m_Selection.GetLineLength();
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
			const auto chByte = ut::GetIHexTData<BYTE>(*this, m_Selection.GetOffsetByIndex(i));
			wstrData += pwszHexChars[(chByte & 0xF0) >> 4];
			wstrData += pwszHexChars[(chByte & 0x0F)];

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
	const auto ullSelSize = m_Selection.GetSelSize();
	const auto pwszHexChars = GetHexChars();

	wstrData.reserve(static_cast<std::size_t>(ullSelSize) * 2);
	for (auto i = ullSelSize; i > 0; --i) {
		const auto chByte = ut::GetIHexTData<BYTE>(*this, m_Selection.GetOffsetByIndex(i - 1));
		wstrData += pwszHexChars[(chByte & 0xF0) >> 4];
		wstrData += pwszHexChars[(chByte & 0x0F)];
	}

	return wstrData;
}

auto CHexCtrl::CopyOffset()const->std::wstring
{
	return (IsOffsetAsHexImpl() ? L"0x" : L"") + OffsetToWstr(GetCaretPosImpl());
}

auto CHexCtrl::CopyPrintScreen()const->std::wstring
{
	if (!m_Selection.HasContiguousSel()) //Only works with contiguous selection.
		return { };

	const auto ullSelStart = m_Selection.GetSelStart();
	const auto ullSelSize = m_Selection.GetSelSize();
	const auto dwCapacity = GetCapacity();

	std::wstring wstrRet;
	wstrRet.reserve(static_cast<std::size_t>(ullSelSize) * 4);
	wstrRet = L"Offset";
	wstrRet.insert(0, (static_cast<std::size_t>(GetDigitsOffset()) - wstrRet.size()) / 2, ' ');
	wstrRet.insert(wstrRet.size(), static_cast<std::size_t>(GetDigitsOffset()) - wstrRet.size(), ' ');
	wstrRet += L"   "; //Spaces to Capacity.
	wstrRet += CreateCapacityString();
	wstrRet += L"   "; //Spaces to Text.
	const auto wstrTextTitle = CreateTextAreaString();
	if (const auto iSize = static_cast<int>(dwCapacity) - static_cast<int>(wstrTextTitle.size()); iSize > 0) {
		wstrRet.insert(wstrRet.size(), static_cast<std::size_t>(iSize / 2), ' ');
	}
	wstrRet += wstrTextTitle;
	wstrRet += L"\r\n";

	//How many spaces to insert at the beginning.
	DWORD dwModStart = ullSelStart % dwCapacity;
	const auto ullLines = GetSelectedLines();
	const auto ullStartLine = ullSelStart / dwCapacity;
	const auto dwStartOffset = dwModStart; //Offset from the line start in the wstrHex.
	const auto& [wstrHex, wstrText] = BuildDataToDraw(ullStartLine, static_cast<int>(ullLines));
	std::wstring wstrDataText;
	std::size_t uzIndexToPrint { 0 };

	for (auto itLine { 0U }; itLine < ullLines; ++itLine) {
		wstrRet += OffsetToWstr(ullStartLine * dwCapacity + dwCapacity * itLine);
		wstrRet.insert(wstrRet.size(), 3, ' ');

		for (auto itChunk { 0U }; itChunk < dwCapacity; ++itChunk) {
			if (dwModStart == 0 && uzIndexToPrint < ullSelSize) {
				wstrRet += wstrHex[(uzIndexToPrint + dwStartOffset) * 2];
				wstrRet += wstrHex[(uzIndexToPrint + dwStartOffset) * 2 + 1];
				wstrDataText += wstrText[uzIndexToPrint + dwStartOffset];
				++uzIndexToPrint;
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
	const auto ullSelSize = m_Selection.GetSelSize();
	std::string strData;
	strData.reserve(static_cast<std::size_t>(ullSelSize));

	for (auto i = 0; i < ullSelSize; ++i) {
		strData.push_back(ut::GetIHexTData<BYTE>(*this, m_Selection.GetOffsetByIndex(i)));
	}

	std::wstring wstrText;
	const auto iCodepage = GetCodepage();
	if (iCodepage == -1) { //ASCII codepage: we simply assigning [strData.begin()...strData.end()) to wstrText w/o any conversion.
		wstrText.assign(strData.begin(), strData.end());
	}
	else if (iCodepage == 0) { //UTF-16.
		const auto uzSizeWstr = (strData.size() - (strData.size() % 2)) / sizeof(wchar_t);
		const auto pDataUTF16Beg = reinterpret_cast<const wchar_t*>(strData.data());
		const auto pDataUTF16End = pDataUTF16Beg + uzSizeWstr;
		wstrText.assign(pDataUTF16Beg, pDataUTF16End);
	}
	else {
		wstrText = ut::StrToWstr(strData, iCodepage);
	}
	ReplaceUnprintable(wstrText, iCodepage == -1, false);

	return wstrText;
}

auto CHexCtrl::CreateCapacityString()const->std::wstring
{
	const auto dwCapacity = GetCapacityImpl();
	std::wstring wstrCapacity;
	wstrCapacity.reserve(static_cast<std::size_t>(dwCapacity) * 3);
	for (auto i { 0U }; i < dwCapacity; ++i) {
		wstrCapacity += std::vformat(m_fOffsetHex ? L"{: >2X}" : L"{: >2d}", std::make_wformat_args(i));

		//Additional space between hex chunk's blocks.
		if ((((i + 1) % m_dwGroupSize) == 0) && (i < (dwCapacity - 1))) {
			wstrCapacity += L" ";
		}

		//Additional space between hex halves.
		if (m_dwGroupSize == 1 && i == (m_dwCapacityBlockSize - 1)) {
			wstrCapacity += L"  ";
		}
	}

	return wstrCapacity;
}

void CHexCtrl::CreateMenu()
{
	if (!m_MenuMain.IsMenu()) {
		if (!m_MenuMain.LoadMenuW(m_hInstRes, IDR_HEXCTRL_MENU)) {
			ut::DBG_REPORT(L"LoadMenuW failed.");
			return;
		}
	}

	m_vecIconsMenu.clear();
	const auto iSizeIcon = static_cast<int>(16 * GetDPIScale());
	const auto menuTop = m_MenuMain.GetSubMenu(0); //Context sub-menu handle.

	//"Search" menu icon.
	auto hBmp = static_cast<HBITMAP>(::LoadImageW(m_hInstRes, MAKEINTRESOURCEW(IDB_HEXCTRL_SEARCH), IMAGE_BITMAP,
		iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));

	menuTop.SetItemBitmap(0, hBmp, false); //"Search" parent menu icon.
	m_MenuMain.SetItemBitmap(IDM_HEXCTRL_SEARCH_DLGSEARCH, hBmp);
	m_vecIconsMenu.emplace_back(hBmp);

	//"Group Data" menu icon.
	hBmp = static_cast<HBITMAP>(::LoadImageW(m_hInstRes, MAKEINTRESOURCEW(IDB_HEXCTRL_GROUP), IMAGE_BITMAP,
		iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	menuTop.SetItemBitmap(2, hBmp, false); //"Group Data" parent menu icon.
	m_vecIconsMenu.emplace_back(hBmp);

	//"Bookmarks->Add" menu icon.
	hBmp = static_cast<HBITMAP>(::LoadImageW(m_hInstRes, MAKEINTRESOURCEW(IDB_HEXCTRL_BKMS), IMAGE_BITMAP,
		iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	menuTop.SetItemBitmap(4, hBmp, false); //"Bookmarks" parent menu icon.
	m_MenuMain.SetItemBitmap(IDM_HEXCTRL_BKM_ADD, hBmp);
	m_vecIconsMenu.emplace_back(hBmp);

	//"Clipboard->Copy as Hex" menu icon.
	hBmp = static_cast<HBITMAP>(::LoadImageW(m_hInstRes, MAKEINTRESOURCEW(IDB_HEXCTRL_CLPBRD_COPYHEX), IMAGE_BITMAP,
		iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	menuTop.SetItemBitmap(5, hBmp, false); //"Clipboard" parent menu icon.
	m_MenuMain.SetItemBitmap(IDM_HEXCTRL_CLPBRD_COPYHEX, hBmp);
	m_vecIconsMenu.emplace_back(hBmp);

	//"Clipboard->Paste as Hex" menu icon.
	hBmp = static_cast<HBITMAP>(::LoadImageW(m_hInstRes, MAKEINTRESOURCEW(IDB_HEXCTRL_CLPBRD_PASTEHEX), IMAGE_BITMAP,
		iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	m_MenuMain.SetItemBitmap(IDM_HEXCTRL_CLPBRD_PASTEHEX, hBmp);
	m_vecIconsMenu.emplace_back(hBmp);

	//"Modify" parent menu icon.
	hBmp = static_cast<HBITMAP>(::LoadImageW(m_hInstRes, MAKEINTRESOURCEW(IDB_HEXCTRL_MODIFY), IMAGE_BITMAP,
		iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	menuTop.SetItemBitmap(6, hBmp, false);
	m_vecIconsMenu.emplace_back(hBmp);

	//"Modify->Fill with Zeros" menu icon.
	hBmp = static_cast<HBITMAP>(::LoadImageW(m_hInstRes, MAKEINTRESOURCEW(IDB_HEXCTRL_MODIFY_FILLZEROS), IMAGE_BITMAP,
		iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	m_MenuMain.SetItemBitmap(IDM_HEXCTRL_MODIFY_FILLZEROS, hBmp);
	m_vecIconsMenu.emplace_back(hBmp);

	//"Appearance->Choose Font" menu icon.
	hBmp = static_cast<HBITMAP>(::LoadImageW(m_hInstRes, MAKEINTRESOURCEW(IDB_HEXCTRL_FONTCHOOSE), IMAGE_BITMAP,
		iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	m_MenuMain.SetItemBitmap(IDM_HEXCTRL_APPEAR_DLGFONT, hBmp);
	m_vecIconsMenu.emplace_back(hBmp);
}

void CHexCtrl::CreatePens()
{
	::DeleteObject(m_hPenLinesMain);
	::DeleteObject(m_hPenLinesTempl);
	m_hPenLinesMain = ::CreatePen(PS_SOLID, 1, m_stColors.clrLinesMain);
	m_hPenLinesTempl = ::CreatePen(PS_SOLID, 1, m_stColors.clrLinesTempl);
}

auto CHexCtrl::CreateTextAreaString()const->std::wstring
{
	const auto iCP = GetCodepage();
	std::wstring_view wsvFmt;
	switch (iCP) {
	case -1:
		wsvFmt = L"ASCII";
		break;
	case 0:
		wsvFmt = L"UTF-16";
		break;
	default:
		wsvFmt = L"Codepage {}";
		break;
	}

	return std::vformat(wsvFmt, std::make_wformat_args(iCP));
}

void CHexCtrl::DrawWindow(HDC hDC)const
{
	const auto iScrollH = static_cast<int>(m_ScrollH.GetScrollPos());
	GDIUT::CRect rcClient(m_iFirstVertLinePx, m_iFirstHorzLinePx,
		m_iFirstVertLinePx + m_iWidthClientAreaPx, m_iFirstHorzLinePx + m_iHeightClientAreaPx);
	GDIUT::CRect rcOffset(m_iFirstVertLinePx - iScrollH, m_iFirstHorzLinePx,
		m_iSecondVertLinePx - iScrollH, m_iThirdHorzLinePx);
	GDIUT::CRect rcHex(m_iSecondVertLinePx - iScrollH, m_iFirstHorzLinePx,
			m_iThirdVertLinePx - iScrollH, m_iThirdHorzLinePx);
	GDIUT::CRect rcText(m_iThirdVertLinePx - iScrollH, m_iFirstHorzLinePx,
		m_iFourthVertLinePx - iScrollH, m_iThirdHorzLinePx);

	GDIUT::CDC dc(hDC);
	dc.FillSolidRect(rcClient, m_stColors.clrBk);       //Client area bk.
	dc.FillSolidRect(rcOffset, m_stColors.clrBkOffset); //Offset area bk.
	dc.FillSolidRect(rcHex, m_stColors.clrBkHex);       //Hex area bk.
	dc.FillSolidRect(rcText, m_stColors.clrBkText);     //Text area bk.

	dc.SelectObject(m_hPenLinesMain);

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
	GDIUT::CRect rcCaptionOffset(m_iFirstVertLinePx - iScrollH, m_iFirstHorzLinePx, m_iSecondVertLinePx - iScrollH,
		m_iSecondHorzLinePx);
	dc.SelectObject(m_hFntMain);
	dc.SetTextColor(m_stColors.clrFontCaption);
	dc.SetBkColor(m_stColors.clrBkOffset);
	dc.DrawTextW(L"Offset", rcCaptionOffset, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	//Capacity numbers.
	const auto wstrCapacity = CreateCapacityString();
	dc.SetBkColor(m_stColors.clrBkHex);
	::ExtTextOutW(dc, m_iIndentFirstHexChunkXPx - iScrollH, m_iFirstHorzLinePx + m_iIndentCapTextYPx, 0, nullptr,
		wstrCapacity.data(), static_cast<UINT>(wstrCapacity.size()), GetCharsWidthArray());

	//Text area caption.
	dc.SetBkColor(m_stColors.clrBkText);
	auto rcCaptionText = GetRectTextCaption();
	dc.DrawTextW(CreateTextAreaString(), rcCaptionText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void CHexCtrl::DrawInfoBar(HDC hDC)const
{
	if (!HasInfoBar() || !IsDataSetImpl())
		return;

	const auto ullCaretPos = GetVirtualOffset(GetCaretPosImpl());

	//^ (caret) - encloses a data name, ` (tilda) - encloses the data itself.
	auto wstrInfoBar = std::vformat(ut::GetLocale(), IsOffsetAsHexImpl() ? L"^Caret: ^`0x{:X}`" : L"^Caret: ^`{:L}`",
		std::make_wformat_args(ullCaretPos));

	if (IsPageVisible()) { //Page/Sector.
		const auto ullPagePos = GetPagePosImpl();
		const auto ullPagesCount = GetPagesCountImpl();
		wstrInfoBar += std::vformat(ut::GetLocale(), IsOffsetAsHexImpl() ? L"^{}: ^`0x{:X}/0x{:X}`" : L"^{}: ^`{:L}/{:L}`",
			std::make_wformat_args(m_wstrPageName, ullPagePos, ullPagesCount));
	}

	if (HasSelection()) {
		const auto ullSelStart = GetVirtualOffset(m_Selection.GetSelStart());
		const auto ullSelSize = m_Selection.GetSelSize();
		if (ullSelSize == 1) { //In case of just one byte selected.
			wstrInfoBar += std::vformat(ut::GetLocale(), IsOffsetAsHexImpl() ? L"^Selected: ^`0x{:X} [0x{:X}]`" :
				L"^Selected: ^`{} [{:L}]`", std::make_wformat_args(ullSelSize, ullSelStart));
		}
		else {
			const auto ullSelEnd = m_Selection.GetSelEnd();
			wstrInfoBar += std::vformat(ut::GetLocale(), IsOffsetAsHexImpl() ? L"^Selected: ^`0x{:X} [0x{:X}-0x{:X}]`" :
				L"^Selected: ^`{:L} [{:L}-{:L}]`", std::make_wformat_args(ullSelSize, ullSelStart, ullSelEnd));
		}
	}

	wstrInfoBar += IsMutableImpl() ? L"^RW^" : L"^RO^"; //RW/RO mode.

	struct POLYINFODATA { //InfoBar text, colors, and vertical lines.
		POLYTEXTW stPoly { };
		COLORREF  clrText { };
		int       iVertLineX { };
	};
	std::vector<POLYINFODATA> vecInfoData;
	vecInfoData.reserve(4);

	const auto iScrollH = static_cast<int>(m_ScrollH.GetScrollPos());
	GDIUT::CRect rcInfoBar(m_iFirstVertLinePx + 1 - iScrollH, m_iThirdHorzLinePx + 1,
		m_iFourthVertLinePx, m_iFourthHorzLinePx); //Info bar rc until m_iFourthHorizLine.
	GDIUT::CRect rcInfoBarText = rcInfoBar;
	rcInfoBarText.left = m_iFirstVertLinePx + 5; //Draw the text beginning with little indent.
	rcInfoBarText.right = m_iFirstVertLinePx + m_iWidthClientAreaPx; //Draw text to the end of the client area, even if it passes iFourthHorizLine.

	std::size_t uzCurrPosBegin { };
	while (true) {
		const auto uzParamPosBegin = wstrInfoBar.find_first_of('^', uzCurrPosBegin);
		if (uzParamPosBegin == std::wstring::npos)
			break;

		const auto uzParamPosEnd = wstrInfoBar.find_first_of('^', uzParamPosBegin + 1);
		if (uzParamPosEnd == std::wstring::npos)
			break;

		const auto uParamSize = static_cast<UINT>(uzParamPosEnd - uzParamPosBegin - 1);
		vecInfoData.emplace_back(POLYTEXTW { .n { uParamSize }, .lpstr { wstrInfoBar.data() + uzParamPosBegin + 1 },
			.rcl { rcInfoBarText } }, m_stColors.clrFontInfoParam);
		rcInfoBarText.left += uParamSize * m_sizeFontInfo.cx; //Increase rect left offset by string size.
		uzCurrPosBegin = uzParamPosEnd + 1;

		if (const auto uzDataPosBegin = wstrInfoBar.find_first_of('`', uzCurrPosBegin);
			uzDataPosBegin != std::wstring::npos) {
			if (const auto uzDataPosEnd = wstrInfoBar.find_first_of('`', uzDataPosBegin + 1);
				uzDataPosEnd != std::wstring::npos) {
				const auto iDataSize = static_cast<UINT>(uzDataPosEnd - uzDataPosBegin - 1);
				vecInfoData.emplace_back(POLYTEXTW { .n { iDataSize }, .lpstr { wstrInfoBar.data() + uzDataPosBegin + 1 },
					.rcl { rcInfoBarText } }, m_stColors.clrFontInfoData);
				rcInfoBarText.left += iDataSize * m_sizeFontInfo.cx;
				uzCurrPosBegin = uzDataPosEnd + 1;
			}
		}

		rcInfoBarText.left += m_sizeFontInfo.cx; //Additional space to the next rect's left side.
		vecInfoData.back().iVertLineX = rcInfoBarText.left - (m_sizeFontInfo.cx / 2); //Vertical line after current rect.
	}

	GDIUT::CDC dc(hDC);
	dc.FillSolidRect(rcInfoBar, m_stColors.clrBkInfoBar); //Info bar rect.
	dc.DrawEdge(rcInfoBar, BDR_RAISEDINNER, BF_TOP);
	dc.SelectObject(m_hFntInfoBar);
	dc.SetBkColor(m_stColors.clrBkInfoBar);

	for (const auto& pid : vecInfoData) {
		dc.SetTextColor(pid.clrText);
		const auto& poly = pid.stPoly;
		auto rc = poly.rcl;
		dc.DrawTextW(poly.lpstr, poly.n, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

		if (pid.iVertLineX > 0) {
			dc.MoveTo(pid.iVertLineX, poly.rcl.top);
			dc.LineTo(pid.iVertLineX, poly.rcl.bottom);
		}
	}
}

void CHexCtrl::DrawOffsets(HDC hDC, ULONGLONG ullStartLine, int iLines)const
{
	const auto dwCapacity = GetCapacity();
	const auto ullStartOffset = ullStartLine * dwCapacity;
	const auto iScrollH = static_cast<int>(m_ScrollH.GetScrollPos());

	for (auto itLine = 0; itLine < iLines; ++itLine) {
		//Drawing offset with bk color depending on the selection range.
		HEXCOLOR stClrOffset;
		if (m_Selection.HitTestRange({ ullStartOffset + (itLine * dwCapacity), dwCapacity })) {
			stClrOffset.clrBk = m_stColors.clrBkSel;
			stClrOffset.clrText = m_stColors.clrFontSel;
		}
		else {
			stClrOffset.clrBk = m_stColors.clrBkOffset;
			stClrOffset.clrText = m_stColors.clrFontOffset;
		}

		//Left column offset printing (00000000...0000FFFF).
		GDIUT::CDC dc(hDC);
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
	std::size_t uzIndexToPrint { 0 };
	HEXCOLORINFO hci { .hdr { m_Wnd, static_cast<UINT>(m_Wnd.GetDlgCtrlID()) },
		.stClr { .clrBk { m_stColors.clrBkHex }, .clrText { m_stColors.clrFontHex } } };

	for (auto itLine = 0; itLine < iLines; ++itLine) {
		std::wstring wstrHexToPrint;
		std::wstring wstrTextToPrint;
		int iHexPosToPrintX { };
		int iTextPosToPrintX { };
		bool fNeedChunkPoint { true }; //For just one time exec.
		const auto iPosToPrintY = m_iStartWorkAreaYPx + (m_sizeFontMain.cy * itLine); //Hex and Text are the same.
		HEXCOLOR stClrHexArea; //Current Hex area color.
		HEXCOLOR stClrTextArea { .clrBk { m_stColors.clrBkText }, .clrText { m_stColors.clrFontText } }; //Current Text area color.
		const auto lmbPoly = [&]() {
			if (wstrHexToPrint.empty())
				return;

			//Hex colors Poly.
			vecWstrHex.emplace_back(std::make_unique<std::wstring>(std::move(wstrHexToPrint)));
			vecPolyHex.emplace_back(POLYTEXTW { .x { iHexPosToPrintX }, .y { iPosToPrintY },
				.n { static_cast<UINT>(vecWstrHex.back()->size()) }, .lpstr { vecWstrHex.back()->data() },
				.pdx { GetCharsWidthArray() } }, stClrHexArea);

			//Text colors Poly.
			vecWstrText.emplace_back(std::make_unique<std::wstring>(std::move(wstrTextToPrint)));
			vecPolyText.emplace_back(POLYTEXTW { .x { iTextPosToPrintX }, .y { iPosToPrintY },
				.n { static_cast<UINT>(vecWstrText.back()->size()) }, .lpstr { vecWstrText.back()->data() },
				.pdx { GetCharsWidthArray() } }, stClrTextArea);
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
		for (auto itChunk { 0U }; itChunk < GetCapacity() && uzIndexToPrint < wsvText.size(); ++itChunk, ++uzIndexToPrint) {
			if (m_pHexVirtColors != nullptr) {
				hci.ullOffset = ullStartOffset + uzIndexToPrint;
				if (m_pHexVirtColors->OnHexGetColor(hci)) {
					stClrTextArea = hci.stClr; //Text area color is now equal to the Hex area color.
				}
				else {
					hci.stClr.clrBk = m_stColors.clrBkHex;
					hci.stClr.clrText = m_stColors.clrFontHex;
					stClrTextArea.clrBk = m_stColors.clrBkText;
					stClrTextArea.clrText = m_stColors.clrFontText;
				}
			}

			if (stClrHexArea != hci.stClr) { //If it's a different color.
				lmbHexSpaces(itChunk);
				lmbPoly();
				fNeedChunkPoint = true;
				stClrHexArea = hci.stClr;
			}

			if (fNeedChunkPoint) {
				int iCy;
				HexChunkPoint(uzIndexToPrint, iHexPosToPrintX, iCy);
				TextChunkPoint(uzIndexToPrint, iTextPosToPrintX, iCy);
				fNeedChunkPoint = false;
			}

			lmbHexSpaces(itChunk);
			wstrHexToPrint += wsvHex[uzIndexToPrint * 2];
			wstrHexToPrint += wsvHex[(uzIndexToPrint * 2) + 1];
			wstrTextToPrint += wsvText[uzIndexToPrint];
		}

		lmbPoly();
	}

	GDIUT::CDC dc(hDC);
	dc.SelectObject(m_hFntMain);
	std::size_t index { 0 }; //Index for the vecPolyText, which size is always equal to the vecPolyHex.
	for (const auto& ptc : vecPolyHex) { //Loop is needed because of different colors.
		dc.SetTextColor(ptc.stClr.clrText); //Text color for the Hex area.
		dc.SetBkColor(ptc.stClr.clrBk);     //Bk color for the Hex area.
		const auto& refH = ptc.stPoly;
		::ExtTextOutW(dc, refH.x, refH.y, refH.uiFlags, &refH.rcl, refH.lpstr, refH.n, refH.pdx); //Hex printing.
		const auto& refVecText = vecPolyText[index++];
		dc.SetTextColor(refVecText.stClr.clrText); //Text color for the Text area.
		dc.SetBkColor(refVecText.stClr.clrBk);     //Bk color for the Text area.
		const auto& refT = refVecText.stPoly;
		::ExtTextOutW(dc, refT.x, refT.y, refT.uiFlags, &refT.rcl, refT.lpstr, refT.n, refT.pdx); //Text printing.
	}
}

void CHexCtrl::DrawTemplates(HDC hDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const
{
	if (!m_DlgTemplMgr.HasApplied())
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
	std::size_t uzIndexToPrint { };
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
		for (auto itChunk { 0U }; itChunk < GetCapacity() && uzIndexToPrint < wsvText.size(); ++itChunk, ++uzIndexToPrint) {
			//Fields.
			if (auto pField = m_DlgTemplMgr.HitTest(ullStartOffset + uzIndexToPrint); pField != nullptr) {
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
					HexChunkPoint(uzIndexToPrint, iFieldHexPosToPrintX, iCy);
					TextChunkPoint(uzIndexToPrint, iFieldTextPosToPrintX, iCy);
					fNeedChunkPoint = false;
				}

				lmbHexSpaces(itChunk);
				wstrHexFieldToPrint += wsvHex[uzIndexToPrint * 2];
				wstrHexFieldToPrint += wsvHex[(uzIndexToPrint * 2) + 1];
				wstrTextFieldToPrint += wsvText[uzIndexToPrint];
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
		GDIUT::CDC dc(hDC);
		dc.SelectObject(m_hFntMain);
		std::size_t index { 0 }; //Index for vecFieldsText, its size is always equal to vecFieldsHex.
		const auto penOld = dc.SelectObject(m_hPenLinesTempl);
		for (const auto& pfc : vecFieldsHex) { //Loop is needed because different Fields can have different colors.
			dc.SetTextColor(pfc.stClr.clrText);
			dc.SetBkColor(pfc.stClr.clrBk);

			const auto& refH = pfc.stPoly;
			::ExtTextOutW(dc, refH.x, refH.y, refH.uiFlags, &refH.rcl, refH.lpstr, refH.n, refH.pdx); //Hex Field printing.
			const auto& refT = vecFieldsText[index++].stPoly;
			::ExtTextOutW(dc, refT.x, refT.y, refT.uiFlags, &refT.rcl, refT.lpstr, refT.n, refT.pdx); //Text Field printing.

			if (pfc.fPrintVertLine) {
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
	if (!m_DlgBkmMgr.HasBookmarks())
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
	std::size_t uzIndexToPrint { };

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
		for (auto itChunk { 0U }; itChunk < GetCapacity() && uzIndexToPrint < wsvText.size(); ++itChunk, ++uzIndexToPrint) {
			//Bookmarks.
			if (const auto pBkm = m_DlgBkmMgr.HitTest(ullStartOffset + uzIndexToPrint); pBkm != nullptr) {
				//If it's nested bookmark.
				if (pBkmCurr != nullptr && pBkmCurr != pBkm) {
					lmbHexSpaces(itChunk);
					lmbPoly();
					fNeedChunkPoint = true;
				}

				pBkmCurr = pBkm;

				if (fNeedChunkPoint) {
					int iCy;
					HexChunkPoint(uzIndexToPrint, iBkmHexPosToPrintX, iCy);
					TextChunkPoint(uzIndexToPrint, iBkmTextPosToPrintX, iCy);
					fNeedChunkPoint = false;
				}

				lmbHexSpaces(itChunk);
				wstrHexBkmToPrint += wsvHex[uzIndexToPrint * 2];
				wstrHexBkmToPrint += wsvHex[(uzIndexToPrint * 2) + 1];
				wstrTextBkmToPrint += wsvText[uzIndexToPrint];
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
		GDIUT::CDC dc(hDC);
		dc.SelectObject(m_hFntMain);
		std::size_t index { 0 }; //Index for vecBkmText, its size is always equal to vecBkmHex.
		for (const auto& ptc : vecBkmHex) { //Loop is needed because bkms have different colors.
			dc.SetTextColor(ptc.stClr.clrText);
			dc.SetBkColor(ptc.stClr.clrBk);
			const auto& refH = ptc.stPoly;
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
	std::size_t uzIndexToPrint { };

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
		for (auto itChunk { 0U }; itChunk < GetCapacity() && uzIndexToPrint < wsvText.size(); ++itChunk, ++uzIndexToPrint) {
			//Selection.
			if (m_Selection.HitTest(ullStartOffset + uzIndexToPrint)) {
				if (fNeedChunkPoint) {
					int iCy;
					HexChunkPoint(uzIndexToPrint, iSelHexPosToPrintX, iCy);
					TextChunkPoint(uzIndexToPrint, iSelTextPosToPrintX, iCy);
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
				wstrHexSelToPrint += wsvHex[uzIndexToPrint * 2];
				wstrHexSelToPrint += wsvHex[(uzIndexToPrint * 2) + 1];
				wstrTextSelToPrint += wsvText[uzIndexToPrint];
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
		GDIUT::CDC dc(hDC);
		dc.SelectObject(m_hFntMain);
		dc.SetTextColor(m_stColors.clrFontSel);
		dc.SetBkColor(m_stColors.clrBkSel);
		::PolyTextOutW(dc, vecPolySelHex.data(), static_cast<UINT>(vecPolySelHex.size())); //Hex selection printing.
		for (const auto& pol : vecPolySelText) {
			::ExtTextOutW(dc, pol.x, pol.y, pol.uiFlags, &pol.rcl, pol.lpstr, pol.n, pol.pdx); //Text selection printing.
		}
	}
}

void CHexCtrl::DrawSelHighlight(HDC hDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const
{
	if (!m_Selection.HasSelHighlight())
		return;

	std::vector<POLYTEXTW> vecPolySelHexHgl;
	std::vector<POLYTEXTW> vecPolySelTextHgl;
	std::vector<std::unique_ptr<std::wstring>> vecWstrSelHgl; //unique_ptr to avoid wstring ptr invalidation.
	const auto ullStartOffset = ullStartLine * GetCapacity();
	std::size_t uzIndexToPrint { };

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
		for (auto itChunk { 0U }; itChunk < GetCapacity() && uzIndexToPrint < wsvText.size(); ++itChunk, ++uzIndexToPrint) {
			//Selection highlights.
			if (m_Selection.HitTestHighlight(ullStartOffset + uzIndexToPrint)) {
				if (fNeedChunkPoint) {
					int iCy;
					HexChunkPoint(uzIndexToPrint, iSelHexPosToPrintX, iCy);
					TextChunkPoint(uzIndexToPrint, iSelTextPosToPrintX, iCy);
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
				wstrHexSelToPrint += wsvHex[uzIndexToPrint * 2];
				wstrHexSelToPrint += wsvHex[(uzIndexToPrint * 2) + 1];
				wstrTextSelToPrint += wsvText[uzIndexToPrint];
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
		GDIUT::CDC dc(hDC);
		dc.SelectObject(m_hFntMain);
		dc.SetTextColor(m_stColors.clrBkSel);
		dc.SetBkColor(m_stColors.clrFontSel);
		::PolyTextOutW(dc, vecPolySelHexHgl.data(), static_cast<UINT>(vecPolySelHexHgl.size())); //Hex selection highlight printing.
		for (const auto& pol : vecPolySelTextHgl) {
			::ExtTextOutW(dc, pol.x, pol.y, pol.uiFlags, &pol.rcl, pol.lpstr, pol.n, pol.pdx); //Text selection highlight printing.
		}
	}
}

void CHexCtrl::DrawCaret(HDC hDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const
{
	const auto ullCaretPos = GetCaretPosImpl();
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

	const auto uzIndexToPrint = static_cast<std::size_t>(ullCaretPos - ullFirstOffset);
	std::wstring wstrHexCaretToPrint;
	std::wstring wstrTextCaretToPrint;
	if (m_fCaretHigh) {
		wstrHexCaretToPrint = wsvHex[uzIndexToPrint * 2];
	}
	else {
		wstrHexCaretToPrint = wsvHex[(uzIndexToPrint * 2) + 1];
		iCaretHexPosToPrintX += GetCharWidthExtras();
	}
	wstrTextCaretToPrint = wsvText[uzIndexToPrint];

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
	const auto clrBkCaret = m_Selection.HitTest(ullCaretPos) ? m_stColors.clrBkCaretSel : m_stColors.clrBkCaret;

	//Caret printing.
	GDIUT::CDC dc(hDC);
	dc.SelectObject(m_hFntMain);
	dc.SetTextColor(m_stColors.clrFontCaret);
	dc.SetBkColor(clrBkCaret);
	for (const auto& pol : arrPolyCaret) {
		::ExtTextOutW(dc, pol.x, pol.y, pol.uiFlags, &pol.rcl, pol.lpstr, pol.n, pol.pdx); //Hex/Text caret printing.
	}
}

void CHexCtrl::DrawDataInterp(HDC hDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const
{
	if (!m_DlgDataInterp.HasHighlight())
		return;

	const auto dwHglSize = m_DlgDataInterp.GetHighlightSize();
	const auto ullCaretPos = GetCaretPosImpl();
	if ((ullCaretPos + dwHglSize) > GetDataSizeImpl())
		return;

	std::vector<POLYTEXTW> vecPolyDataInterp;
	std::vector<std::unique_ptr<std::wstring>> vecWstrDataInterp;
	const auto ullStartOffset = ullStartLine * GetCapacity();
	std::size_t uzIndexToPrint { };

	for (auto itLine = 0; itLine < iLines; ++itLine) {
		std::wstring wstrHexDataInterpToPrint; //Data Interpreter Hex and Text strings to print.
		std::wstring wstrTextDataInterpToPrint;
		int iDataInterpHexPosToPrintX { }; //Data Interpreter X coords.
		int iDataInterpTextPosToPrintX { };
		bool fNeedChunkPoint { true };
		const auto iPosToPrintY = m_iStartWorkAreaYPx + (m_sizeFontMain.cy * itLine); //Hex and Text are the same.

		//Main loop for printing Hex chunks and Text chars.
		for (auto itChunk { 0U }; itChunk < GetCapacity() && uzIndexToPrint < wsvText.size(); ++itChunk, ++uzIndexToPrint) {
			const auto ullOffsetCurr = ullStartOffset + uzIndexToPrint;
			if (ullOffsetCurr >= ullCaretPos && ullOffsetCurr < (ullCaretPos + dwHglSize)) {
				if (fNeedChunkPoint) {
					int iCy;
					HexChunkPoint(uzIndexToPrint, iDataInterpHexPosToPrintX, iCy);
					TextChunkPoint(uzIndexToPrint, iDataInterpTextPosToPrintX, iCy);
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
				wstrHexDataInterpToPrint += wsvHex[uzIndexToPrint * 2];
				wstrHexDataInterpToPrint += wsvHex[(uzIndexToPrint * 2) + 1];
				wstrTextDataInterpToPrint += wsvText[uzIndexToPrint];
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
		GDIUT::CDC dc(hDC);
		dc.SelectObject(m_hFntMain);
		dc.SetTextColor(m_stColors.clrFontDataInterp);
		dc.SetBkColor(m_stColors.clrBkDataInterp);
		for (const auto& pol : vecPolyDataInterp) {
			::ExtTextOutW(dc, pol.x, pol.y, pol.uiFlags, &pol.rcl, pol.lpstr, pol.n, pol.pdx); //Hex/Text Data Interpreter printing.
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
		GDIUT::CDC dc(hDC);
		const auto penOld = dc.SelectObject(m_hPenLinesMain);
		for (const auto& pl : vecPageLines) {
			dc.MoveTo(pl.ptStart.x, pl.ptStart.y);
			dc.LineTo(pl.ptEnd.x, pl.ptEnd.y);
		}
		dc.SelectObject(penOld);
	}
}

void CHexCtrl::FillWithZeros()
{
	if (!IsDataSetImpl())
		return;

	std::byte byteZero { 0 };
	ModifyData({ .eModifyMode { EHexModifyMode::MODIFY_REPEAT }, .spnData { &byteZero, sizeof(byteZero) },
		.vecSpan { GetSelection() } });
	RedrawImpl();
}

auto CHexCtrl::FontPointsFromScaledPixels(long iSizePixels)const->float {
	return GDIUT::FontPointsFromPixels(iSizePixels) / GetDPIScale();
}

auto CHexCtrl::FontScaledPixelsFromPoints(float flSizePoints)const->long {
	return std::lround(GDIUT::FontPixelsFromPoints(flSizePoints) * GetDPIScale());
}

void CHexCtrl::FontSizeIncDec(bool fInc)
{
	const auto flFontSizePoints = FontPointsFromScaledPixels(GetFontSizeInPixels(true)) + (fInc ? 1 : -1);
	SetFontSizeInPoints(flFontSizePoints, true);
}

auto CHexCtrl::GetBottomLine()const->ULONGLONG
{
	if (!IsDataSetImpl())
		return { };

	auto ullEndLine = GetTopLine();
	if (const auto iLines = m_iHeightWorkAreaPx / m_sizeFontMain.cy; iLines > 0) { //How many visible lines.
		ullEndLine += iLines - 1;
	}

	const auto ullDataSize = GetDataSizeImpl();
	const auto ullTotalLines = ullDataSize / GetCapacity();

	//If ullDataSize is really small, or we at the scroll end, adjust ullEndLine to be not bigger than maximum possible.
	if (ullEndLine >= ullTotalLines) {
		ullEndLine = ullTotalLines - ((ullDataSize % GetCapacity()) == 0 ? 1 : 0);
	}

	return ullEndLine;
}

auto CHexCtrl::GetCapacityImpl()const->DWORD {
	return m_dwCapacity;
}

auto CHexCtrl::GetCaretPosImpl()const->std::uint64_t
{
	return m_ullCaretPos;
}

auto CHexCtrl::GetCharsWidthArray()const->int*
{
	return const_cast<int*>(m_vecCharsWidth.data());
}

auto CHexCtrl::GetCharWidthExtras()const->int
{
	return GetCharWidthNative() + m_dwCharsExtraSpace;
}

auto CHexCtrl::GetCharWidthNative()const->int
{
	return m_sizeFontMain.cx;
}

auto CHexCtrl::GetCommandFromKey(UINT uKey, bool fCtrl, bool fShift, bool fAlt)const->std::optional<EHexCmd>
{
	if (const auto it = std::find_if(m_vecKeyBind.begin(), m_vecKeyBind.end(), [=](const KEYBIND& kb) {
		return kb.fCtrl == fCtrl && kb.fShift == fShift && kb.fAlt == fAlt && kb.uKey == uKey; });
		it != m_vecKeyBind.end()) {
		return it->eCmd;
	}

	return std::nullopt;
}

auto CHexCtrl::GetCommandFromMenu(WORD wMenuID)const->std::optional<EHexCmd>
{
	if (const auto it = std::find_if(m_vecKeyBind.begin(), m_vecKeyBind.end(), [=](const KEYBIND& kb) {
		return kb.wMenuID == wMenuID; }); it != m_vecKeyBind.end()) {
		return it->eCmd;
	}

	return std::nullopt;
}

auto CHexCtrl::GetDataSizeImpl()const->std::uint64_t
{
	return m_spnData.size();
}

auto CHexCtrl::GetDigitsOffset()const->DWORD
{
	return m_fOffsetHex ? m_dwDigitsOffsetHex : m_dwDigitsOffsetDec;
}

auto CHexCtrl::GetDPIScale()const->float
{
	return m_flDPIScale;
}

long CHexCtrl::GetFontSizeInPixels(bool fMain)const
{
	return GetFont(fMain).lfHeight;
}

auto CHexCtrl::GetHexChars()const->const wchar_t*
{
	return IsHexCharsUpper() ? L"0123456789ABCDEF" : L"0123456789abcdef";
}

auto CHexCtrl::GetOffsetImpl(std::uint64_t u64Offset, bool fGetVirt) const -> std::uint64_t
{
	if (IsVirtualImpl()) {
		HEXDATAINFO hdi { .hdr { m_Wnd, static_cast<UINT>(m_Wnd.GetDlgCtrlID()) }, .stHexSpan { .ullOffset { u64Offset } } };
		m_pHexVirtData->OnHexGetOffset(hdi, fGetVirt);
		return hdi.stHexSpan.ullOffset;
	}

	return u64Offset;
}

auto CHexCtrl::GetPagePosImpl()const->std::uint64_t
{
	return GetCaretPosImpl() / GetPageSizeImpl();
}

auto CHexCtrl::GetPageSizeImpl()const->std::uint32_t
{
	return m_dwPageSize;
}

auto CHexCtrl::GetPagesCountImpl()const->std::uint64_t
{
	const auto u64PageSize = GetPageSizeImpl();
	if (u64PageSize == 0) {
		return { };
	}

	const auto ullDataSize = GetDataSizeImpl();
	return (ullDataSize / u64PageSize) + ((ullDataSize % u64PageSize) ? 1 : 0);
}

auto CHexCtrl::GetRectTextCaption()const->GDIUT::CRect
{
	const auto iScrollH { static_cast<int>(m_ScrollH.GetScrollPos()) };
	return { m_iThirdVertLinePx - iScrollH, m_iFirstHorzLinePx, m_iFourthVertLinePx - iScrollH, m_iSecondHorzLinePx };
}

auto CHexCtrl::GetSelectedLines()const->ULONGLONG
{
	if (!m_Selection.HasContiguousSel())
		return 0ULL;

	const auto dwCapacity = GetCapacity();
	const auto ullSelStart = m_Selection.GetSelStart();
	const auto ullSelEnd = m_Selection.GetSelEnd();
	const auto ullSelSize = m_Selection.GetSelSize();
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
	return m_ScrollV.GetScrollPos() / m_sizeFontMain.cy;
}

auto CHexCtrl::GetVirtualOffset(ULONGLONG ullOffset)const->ULONGLONG
{
	return GetOffsetImpl(ullOffset, true);
}

void CHexCtrl::HexChunkPoint(ULONGLONG ullOffset, int& iCx, int& iCy)const
{
	//This func computes x and y pos of the given Hex chunk.
	const auto dwCapacity = GetCapacity();
	const DWORD dwMod = ullOffset % dwCapacity;
	const auto iBetweenBlocks { dwMod >= m_dwCapacityBlockSize ? m_iSpaceBetweenBlocksPx : 0 };
	iCx = static_cast<int>(((m_iIndentFirstHexChunkXPx + iBetweenBlocks + dwMod * m_iSizeHexBytePx)
		+ (dwMod / GetGroupSize()) * GetCharWidthExtras()) - m_ScrollH.GetScrollPos());

	const auto ullScrollV = m_ScrollV.GetScrollPos();
	iCy = static_cast<int>((m_iStartWorkAreaYPx + (ullOffset / dwCapacity) * m_sizeFontMain.cy) -
		(ullScrollV - (ullScrollV % m_sizeFontMain.cy)));
}

auto CHexCtrl::HitTest(POINT pt)const->std::optional<HEXHITTEST>
{
	HEXHITTEST stHit;
	const auto iY = pt.y;
	const auto iX = pt.x + static_cast<int>(m_ScrollH.GetScrollPos()); //To compensate horizontal scroll.
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
	if (stHit.ullOffset >= GetDataSizeImpl()) {
		fHit = false;
	}

	return fHit ? std::optional<HEXHITTEST> { stHit } : std::nullopt;
}

bool CHexCtrl::IsCurTextArea()const
{
	return m_fCursorTextArea;
}

bool CHexCtrl::IsDataSetImpl()const
{
	return m_fDataSet;
}

bool CHexCtrl::IsDrawable()const
{
	return m_fRedraw;
}

bool CHexCtrl::IsMutableImpl()const
{
	return m_fMutable;
}

bool CHexCtrl::IsOffsetAsHexImpl()const
{
	return m_fOffsetHex;
}

bool CHexCtrl::IsPageVisible()const
{
	return GetPageSizeImpl() > 0 && (GetPageSizeImpl() % GetCapacity() == 0) && GetPageSizeImpl() >= GetCapacity();
}

bool CHexCtrl::IsVirtualImpl()const
{
	return m_pHexVirtData != nullptr;
}

void CHexCtrl::ModifyWorker(const HEXCTRL::HEXMODIFY& hms, const auto& FuncWorker, const HEXCTRL::SpanCByte spnOper)
{
	if (spnOper.empty()) { ut::DBG_REPORT(L"Operation span is empty."); return; }

	const auto& vecSpan = hms.vecSpan;
	const auto ullTotalSize = std::reduce(vecSpan.begin(), vecSpan.end(), 0ULL,
		[](ULONGLONG ullSumm, const HEXSPAN& hs) { return ullSumm + hs.ullSize; });
	assert(ullTotalSize <= GetDataSizeImpl());

	CHexDlgProgress dlgProg(L"Modifying...", L"", vecSpan.back().ullOffset, vecSpan.back().ullOffset + ullTotalSize);
	const auto lmbModify = [&]() {
		for (const auto& hs : vecSpan) { //Span-vector's size times.
			const auto ullOffsetToModify { hs.ullOffset };
			const auto ullSizeToModify { hs.ullSize };
			const auto ullSizeDataOper { spnOper.size() };

			//If the size of the data to_modify_from is bigger than
			//the data to modify, we do nothing.
			if (ullSizeDataOper > ullSizeToModify)
				break;

			ULONGLONG ullSizeCache { };
			ULONGLONG ullChunks { };
			bool fCacheIsLargeEnough { true }; //Cache is larger than ullSizeDataOper.

			if (IsVirtualImpl()) {
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
					if (ullOffsetCurr + ullSizeCache > GetDataSizeImpl()) { //Overflow check.
						ullSizeCache = GetDataSizeImpl() - ullOffsetCurr;
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

	constexpr auto uSizeToRunThread { 1024U * 1024U * 50U }; //50MB.
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
	return std::vformat(IsOffsetAsHexImpl() ? L"{:0{}X}" : L"{:0{}}", std::make_wformat_args(ullOffset, dwDigitsOffset));
}

void CHexCtrl::OnCaretPosChange(ULONGLONG ullOffset)
{
	m_DlgDataInterp.UpdateData();

	if (auto pBkm = m_DlgBkmMgr.HitTest(ullOffset); pBkm != nullptr) { //If clicked on bookmark.
		const HEXBKMINFO hbi { .hdr { m_Wnd, static_cast<UINT>(m_Wnd.GetDlgCtrlID()), HEXCTRL_MSG_BKMCLICK }, .pBkm { pBkm } };
		ParentNotify(hbi);
	}

	ParentNotify(HEXCTRL_MSG_SETCARET);
}

auto CHexCtrl::OnChar(const MSG& msg)->LRESULT
{
	const auto wChar = LOWORD(msg.wParam); //LOWORD holds wchar_t symbol.
	if (!IsDataSetImpl() || !IsMutableImpl() || !IsCurTextArea() || (::GetKeyState(VK_CONTROL) < 0)
		|| !std::iswprint(wChar))
		return 0;

	std::string strASCII; //ASCII char to set.
	//Keyboard Layout ID (locale identifier) for the current thread.
	const auto lcid = static_cast<LCID>(reinterpret_cast<DWORD_PTR>(::GetKeyboardLayout(0UL)) & 0xFFFFUL);
	UINT uANSICP { }; //ANSI CodePage ID.
	if (constexpr int iSize = sizeof(uANSICP) / sizeof(wchar_t);
		::GetLocaleInfoW(lcid, LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
			reinterpret_cast<LPWSTR>(&uANSICP), iSize) == iSize) { //ANSI code page for the current KLID (if any).
		//Convert input wchar symbol to ASCII char according to the current Keyboard ANSI code page.
		const std::wstring_view wsv(reinterpret_cast<const wchar_t*>(&wChar), 1);
		strASCII = ut::WstrToStr(wsv, uANSICP);
	}

	//Set converted char if conversion (wchar_t->ASCII_char) was made, or zero otherwise. 
	const std::byte bByteToSet = static_cast<std::byte>(!strASCII.empty() ? strASCII[0] : 0);
	ModifyData({ .spnData { &bByteToSet, sizeof(bByteToSet) }, .vecSpan { { GetCaretPosImpl(), 1 } } });
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
	const POINT pt { .x { ut::GetXLPARAM(msg.lParam) }, .y { ut::GetYLPARAM(msg.lParam) } };

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
	m_DlgBkmMgr.DestroyDlg();
	m_DlgCodepage.DestroyDlg();
	m_DlgDataInterp.DestroyDlg();
	m_DlgModify.DestroyDlg();
	m_DlgGoTo.DestroyDlg();
	m_DlgSearch.DestroyDlg();
	m_DlgTemplMgr.DestroyDlg();
	m_DlgTemplMgr.UnloadAll(); //Templates could be loaded without creating the dialog itself.
	m_vecIconsMenu.clear();
	m_vecKeyBind.clear();
	m_vecUndo.clear();
	m_vecRedo.clear();
	m_vecCharsWidth.clear();
	m_MenuMain.DestroyMenu();
	::DeleteObject(m_hFntMain);
	::DeleteObject(m_hFntInfoBar);
	::DeleteObject(m_hPenLinesMain);
	::DeleteObject(m_hPenLinesTempl);
	m_ScrollV.DestroyWindow(); //Not a child of the IHexCtrl.
	m_ScrollH.DestroyWindow(); //Not a child of the IHexCtrl.
	ParentNotify(HEXCTRL_MSG_DESTROY);
	m_fCreated = false;

	return 0;
}

auto CHexCtrl::OnDPIChangedAfterParent()->LRESULT
{
	//Take the current font size, in points, with the old DPI.
	const auto flFontPointsMain = FontPointsFromScaledPixels(GetFontSizeInPixels(true));
	const auto flFontPointsInfo = FontPointsFromScaledPixels(GetFontSizeInPixels(false));

	UpdateDPIScale(); //Set new DPI scale.

	//Invoke all DPI dependent routines, with the new DPI.
	SetFontSizeInPoints(flFontPointsMain, true);
	SetFontSizeInPoints(flFontPointsInfo, false);
	CreateMenu();
	m_ScrollV.OnDPIChangedAfterParent();
	m_ScrollH.OnDPIChangedAfterParent();

	return 0;
}

auto CHexCtrl::OnEraseBkgnd([[maybe_unused]] const MSG& msg)->LRESULT
{
	return 1; //An application should return nonzero if it erases the background, or zero otherwise.
}

auto CHexCtrl::OnGetDlgCode([[maybe_unused]] const MSG& msg)->LRESULT
{
	return DLGC_WANTALLKEYS;
}

auto CHexCtrl::OnHelp([[maybe_unused]] const MSG& msg)->LRESULT
{
	return TRUE;
}

auto CHexCtrl::OnHScroll([[maybe_unused]] const MSG& msg)->LRESULT
{
	RedrawImpl();
	return 0;
}

auto CHexCtrl::OnInitMenuPopup(const MSG& msg)->LRESULT
{
	using enum EHexCmd;
	//The LOWORD(lParam) specifies zero-based relative position of the menu, that opens drop-down menu or submenu.
	switch (LOWORD(msg.lParam)) {
	case 0:	//Search.
		m_MenuMain.EnableItem(IDM_HEXCTRL_SEARCH_DLGSEARCH, IsCmdAvail(CMD_SEARCH_DLG));
		m_MenuMain.EnableItem(IDM_HEXCTRL_SEARCH_NEXT, IsCmdAvail(CMD_SEARCH_NEXT));
		m_MenuMain.EnableItem(IDM_HEXCTRL_SEARCH_PREV, IsCmdAvail(CMD_SEARCH_PREV));
		break;
	case 3:	//Navigation.
		m_MenuMain.EnableItem(IDM_HEXCTRL_NAV_DLGGOTO, IsCmdAvail(CMD_NAV_GOTO_DLG));
		m_MenuMain.EnableItem(IDM_HEXCTRL_NAV_REPFWD, IsCmdAvail(CMD_NAV_REPFWD));
		m_MenuMain.EnableItem(IDM_HEXCTRL_NAV_REPBKW, IsCmdAvail(CMD_NAV_REPBKW));
		m_MenuMain.EnableItem(IDM_HEXCTRL_NAV_DATABEG, IsCmdAvail(CMD_NAV_DATABEG));
		m_MenuMain.EnableItem(IDM_HEXCTRL_NAV_DATAEND, IsCmdAvail(CMD_NAV_DATAEND));
		m_MenuMain.EnableItem(IDM_HEXCTRL_NAV_PAGEBEG, IsCmdAvail(CMD_NAV_PAGEBEG));
		m_MenuMain.EnableItem(IDM_HEXCTRL_NAV_PAGEEND, IsCmdAvail(CMD_NAV_PAGEEND));
		m_MenuMain.EnableItem(IDM_HEXCTRL_NAV_LINEBEG, IsCmdAvail(CMD_NAV_LINEBEG));
		m_MenuMain.EnableItem(IDM_HEXCTRL_NAV_LINEEND, IsCmdAvail(CMD_NAV_LINEEND));
		break;
	case 4:	//Bookmarks.
		m_MenuMain.EnableItem(IDM_HEXCTRL_BKM_ADD, IsCmdAvail(CMD_BKM_ADD));
		m_MenuMain.EnableItem(IDM_HEXCTRL_BKM_REMOVE, IsCmdAvail(CMD_BKM_REMOVE));
		m_MenuMain.EnableItem(IDM_HEXCTRL_BKM_NEXT, IsCmdAvail(CMD_BKM_NEXT));
		m_MenuMain.EnableItem(IDM_HEXCTRL_BKM_PREV, IsCmdAvail(CMD_BKM_PREV));
		m_MenuMain.EnableItem(IDM_HEXCTRL_BKM_REMOVEALL, IsCmdAvail(CMD_BKM_REMOVEALL));
		m_MenuMain.EnableItem(IDM_HEXCTRL_BKM_DLGMGR, IsCmdAvail(CMD_BKM_DLG_MGR));
		break;
	case 5:	//Clipboard.
		m_MenuMain.EnableItem(IDM_HEXCTRL_CLPBRD_COPYHEX, IsCmdAvail(CMD_CLPBRD_COPY_HEX));
		m_MenuMain.EnableItem(IDM_HEXCTRL_CLPBRD_COPYHEXLE, IsCmdAvail(CMD_CLPBRD_COPY_HEXLE));
		m_MenuMain.EnableItem(IDM_HEXCTRL_CLPBRD_COPYHEXFMT, IsCmdAvail(CMD_CLPBRD_COPY_HEXFMT));
		m_MenuMain.EnableItem(IDM_HEXCTRL_CLPBRD_COPYTEXTCP, IsCmdAvail(CMD_CLPBRD_COPY_TEXTCP));
		m_MenuMain.EnableItem(IDM_HEXCTRL_CLPBRD_COPYBASE64, IsCmdAvail(CMD_CLPBRD_COPY_BASE64));
		m_MenuMain.EnableItem(IDM_HEXCTRL_CLPBRD_COPYCARR, IsCmdAvail(CMD_CLPBRD_COPY_CARR));
		m_MenuMain.EnableItem(IDM_HEXCTRL_CLPBRD_COPYGREPHEX, IsCmdAvail(CMD_CLPBRD_COPY_GREPHEX));
		m_MenuMain.EnableItem(IDM_HEXCTRL_CLPBRD_COPYPRNTSCRN, IsCmdAvail(CMD_CLPBRD_COPY_PRNTSCRN));
		m_MenuMain.EnableItem(IDM_HEXCTRL_CLPBRD_COPYOFFSET, IsCmdAvail(CMD_CLPBRD_COPY_OFFSET));
		m_MenuMain.EnableItem(IDM_HEXCTRL_CLPBRD_PASTEHEX, IsCmdAvail(CMD_CLPBRD_PASTE_HEX));
		m_MenuMain.EnableItem(IDM_HEXCTRL_CLPBRD_PASTETEXTUTF16, IsCmdAvail(CMD_CLPBRD_PASTE_TEXTUTF16));
		m_MenuMain.EnableItem(IDM_HEXCTRL_CLPBRD_PASTETEXTCP, IsCmdAvail(CMD_CLPBRD_PASTE_TEXTCP));
		break;
	case 6: //Modify.
		m_MenuMain.EnableItem(IDM_HEXCTRL_MODIFY_FILLZEROS, IsCmdAvail(CMD_MODIFY_FILLZEROS));
		m_MenuMain.EnableItem(IDM_HEXCTRL_MODIFY_DLGFILLDATA, IsCmdAvail(CMD_MODIFY_FILLDATA_DLG));
		m_MenuMain.EnableItem(IDM_HEXCTRL_MODIFY_DLGOPERS, IsCmdAvail(CMD_MODIFY_OPERS_DLG));
		m_MenuMain.EnableItem(IDM_HEXCTRL_MODIFY_UNDO, IsCmdAvail(CMD_MODIFY_UNDO));
		m_MenuMain.EnableItem(IDM_HEXCTRL_MODIFY_REDO, IsCmdAvail(CMD_MODIFY_REDO));
		break;
	case 7: //Selection.
		m_MenuMain.EnableItem(IDM_HEXCTRL_SEL_MARKSTARTEND, IsCmdAvail(CMD_SEL_MARKSTARTEND));
		m_MenuMain.EnableItem(IDM_HEXCTRL_SEL_ALL, IsCmdAvail(CMD_SEL_ALL));
		break;
	case 8: //Templates.
		m_MenuMain.EnableItem(IDM_HEXCTRL_TEMPL_APPLYCURR, IsCmdAvail(CMD_TEMPL_APPLYCURR));
		m_MenuMain.EnableItem(IDM_HEXCTRL_TEMPL_DISAPPLY, IsCmdAvail(CMD_TEMPL_DISAPPLY));
		m_MenuMain.EnableItem(IDM_HEXCTRL_TEMPL_DISAPPALL, IsCmdAvail(CMD_TEMPL_DISAPPALL));
		m_MenuMain.EnableItem(IDM_HEXCTRL_TEMPL_DLGMGR, IsCmdAvail(CMD_TEMPL_DLG_MGR));
		break;
	case 9: //Data Presentation.
		m_MenuMain.EnableItem(IDM_HEXCTRL_DLGDATAINTERP, IsCmdAvail(CMD_DATAINTERP_DLG));
		m_MenuMain.EnableItem(IDM_HEXCTRL_DLGCODEPAGE, IsCmdAvail(CMD_CODEPAGE_DLG));
		break;
	default:
		break;
	}

	return 0;
}

auto CHexCtrl::OnKeyDown(const MSG& msg)->LRESULT
{
	const auto wVKey = LOWORD(msg.wParam); //Virtual-key code (both: WM_KEYDOWN/WM_SYSKEYDOWN).

	//LORE: If some key combinations (e.g. Ctrl+Alt+Num Plus) do not work for seemingly no reason,
	//it might be due to global hotkeys hooks from some running app.
	if (const auto optCmd = GetCommandFromKey(wVKey, ::GetAsyncKeyState(VK_CONTROL) < 0,
		::GetAsyncKeyState(VK_SHIFT) < 0, ::GetAsyncKeyState(VK_MENU) < 0); optCmd) {
		ExecuteCmd(*optCmd);
		return 0;
	}

	if (!IsDataSetImpl() || !IsMutableImpl() || IsCurTextArea())
		return 0;

	//If caret is in the Hex area then just one part (High/Low) of the byte must be changed.
	//Normalizing all input in the Hex area to only [0x0-0xF] range, allowing only [0-9], [A-F], [NUM0-NUM9].
	int iByteInput;
	if (wVKey >= '0' && wVKey <= '9') {
		iByteInput = wVKey - '0'; //'1' - '0' = 0x01.
	}
	else if (wVKey >= 'A' && wVKey <= 'F') {
		iByteInput = wVKey - 0x37; //'A' - 0x37 = 0x0A.
	}
	else if (wVKey >= VK_NUMPAD0 && wVKey <= VK_NUMPAD9) {
		iByteInput = wVKey - VK_NUMPAD0;
	}
	else
		return 0;

	const auto u8ByteCurr = ut::GetIHexTData<std::uint8_t>(*this, GetCaretPosImpl());
	const auto bByteToSet = static_cast<std::byte>(m_fCaretHigh ? ((iByteInput << 4U) | (u8ByteCurr & 0x0FU))
		: ((iByteInput & 0x0FU) | (u8ByteCurr & 0xF0U)));
	ModifyData({ .eModifyMode { EHexModifyMode::MODIFY_ONCE }, .spnData { &bByteToSet, sizeof(bByteToSet) },
		.vecSpan { { GetCaretPosImpl(), 1 } } });
	CaretMoveRight();

	return 0;
}

auto CHexCtrl::OnKeyUp([[maybe_unused]] const MSG& msg)->LRESULT
{
	if (!IsDataSetImpl())
		return 0;

	m_DlgDataInterp.UpdateData();

	return 0;
}

auto CHexCtrl::OnLButtonDblClk(const MSG& msg)->LRESULT
{
	const POINT pt { .x { ut::GetXLPARAM(msg.lParam) }, .y { ut::GetYLPARAM(msg.lParam) } };

	if ((pt.x + static_cast<long>(m_ScrollH.GetScrollPos())) < m_iSecondVertLinePx) { //DblClick on "Offset" area.
		SetOffsetMode(!IsOffsetAsHexImpl());
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
		if (msg.wParam & MK_SHIFT) {
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
	const POINT pt { .x { ut::GetXLPARAM(msg.lParam) }, .y { ut::GetYLPARAM(msg.lParam) } };

	m_Wnd.SetFocus(); //SetFocus is vital to give proper keyboard input to the main HexCtrl window.

	if (m_fScrollCursor) {
		SetScrollCursor();
		return 0;
	}

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
	if (msg.wParam & MK_SHIFT) {
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
		RedrawImpl();
	}

	OnCaretPosChange(GetCaretPosImpl());

	return 0;
}

auto CHexCtrl::OnLButtonUp([[maybe_unused]] const MSG& msg)->LRESULT
{
	m_fLMousePressed = false;
	::ReleaseCapture();
	m_ScrollV.OnLButtonUp();
	m_ScrollH.OnLButtonUp();

	return 0;
}

auto CHexCtrl::OnMButtonDown(const MSG& msg)->LRESULT
{
	m_Wnd.SetFocus();
	const auto wFlags = GET_KEYSTATE_WPARAM(msg.wParam);

	if (const auto opt = GetCommandFromKey(m_dwVKMiddleButtonDown,
		wFlags & MK_CONTROL, wFlags & MK_SHIFT, false); opt) {
		ExecuteCmd(*opt);
	}

	return 0;
}

void CHexCtrl::OnModifyData()
{
	ParentNotify(HEXCTRL_MSG_SETDATA);
	m_DlgTemplMgr.UpdateData();
	m_DlgDataInterp.UpdateData();
}

auto CHexCtrl::OnMouseMove(const MSG& msg)->LRESULT
{
	const POINT pt { .x { ut::GetXLPARAM(msg.lParam) }, .y { ut::GetYLPARAM(msg.lParam) } };
	const auto optHit = HitTest(pt);

	if (m_fLMousePressed) {
		//If LMouse is pressed but cursor is outside of the client area (SetCapture() behavior).
		const auto rcClient = m_Wnd.GetClientRect();
		if (m_ScrollH.IsVisible()) { //Checking for scrollbars existence first.
			if (pt.x < rcClient.left) {
				m_ScrollH.ScrollLineLeft();
			}
			else if (pt.x >= rcClient.right) {
				m_ScrollH.ScrollLineRight();
			}
		}

		if (m_ScrollV.IsVisible()) {
			if (pt.y < m_iStartWorkAreaYPx) {
				m_ScrollV.ScrollLineUp();
			}
			else if (pt.y >= m_iEndWorkAreaPx) {
				m_ScrollV.ScrollLineDown();
			}
		}

		//Checking if the current cursor pos is at the same byte's half
		//that it was at the previous WM_MOUSEMOVE event.
		//Making selection of the byte only if the cursor has crossed byte's halves.
		//Doesn't apply when moving in the Text area.
		if (!optHit || (optHit->ullOffset == m_ullCursorNow && m_fCaretHigh == optHit->fIsHigh && !optHit->fIsText))
			return 0;

		m_ullCursorNow = optHit->ullOffset;
		const auto ullOffsetHit = optHit->ullOffset;
		const auto dwCapacity = GetCapacity();
		ULONGLONG ullClick;
		ULONGLONG ullStart;
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
			if (const auto pBkm = m_DlgBkmMgr.HitTest(optHit->ullOffset); pBkm != nullptr) {
				if (m_pBkmTTCurr != pBkm) {
					m_pBkmTTCurr = pBkm;
					m_ttiMain.lpszText = pBkm->wstrDesc.data();
					TTMainShow(true);
				}
			}
			else if (m_pBkmTTCurr != nullptr) {
				TTMainShow(false);
			}
			else if (const auto pField = m_DlgTemplMgr.HitTest(optHit->ullOffset);
				m_DlgTemplMgr.IsTooltips() && pField != nullptr) {
				if (m_pTFieldTTCurr != pField) {
					m_pTFieldTTCurr = pField;
					m_ttiMain.lpszText = const_cast<LPWSTR>(pField->wstrName.data());
					TTMainShow(true);
				}
			}
			else if (m_pTFieldTTCurr != nullptr) {
				TTMainShow(false);
			}
		}
		else {
			//If there is tooltip already shown, but cursor is outside of data chunks.
			if (m_pBkmTTCurr != nullptr || m_pTFieldTTCurr != nullptr) {
				TTMainShow(false);
			}
		}

		m_ScrollV.OnMouseMove(pt);
		m_ScrollH.OnMouseMove(pt);
	}

	return 0;
}

auto CHexCtrl::OnMouseWheel(const MSG& msg)->LRESULT
{
	const auto uwDelta = GET_WHEEL_DELTA_WPARAM(msg.wParam);
	const auto wFlags = GET_KEYSTATE_WPARAM(msg.wParam);

	if (const auto opt = GetCommandFromKey(uwDelta > 0 ? m_dwVKMouseWheelUp : m_dwVKMouseWheelDown,
		wFlags & MK_CONTROL, wFlags & MK_SHIFT, false); opt) {
		ExecuteCmd(*opt);
	}

	return 0;
}

auto CHexCtrl::OnNCActivate([[maybe_unused]] const MSG& msg)->LRESULT
{
	m_ScrollV.OnNCActivate();
	m_ScrollH.OnNCActivate();

	return TRUE;
}

auto CHexCtrl::OnNCCalcSize(const MSG& msg)->LRESULT
{
	GDIUT::DefWndProc(msg);
	const auto pNCSP = reinterpret_cast<LPNCCALCSIZE_PARAMS>(msg.lParam);

	//Sequence is important — H->V.
	m_ScrollH.OnNCCalcSize(pNCSP);
	m_ScrollV.OnNCCalcSize(pNCSP);

	return 0;
}

auto CHexCtrl::OnNCPaint(const MSG& msg)->LRESULT
{
	GDIUT::DefWndProc(msg);
	m_ScrollV.OnNCPaint();
	m_ScrollH.OnNCPaint();

	return 0;
}

auto CHexCtrl::OnPaint()->LRESULT
{
	GDIUT::CPaintDC dcPaint(m_Wnd);

	if (!IsDrawable()) //Control should not be rendered atm.
		return 0;

	const auto rcClient = m_Wnd.GetClientRect();

	if (!IsCreated()) {
		dcPaint.FillSolidRect(rcClient, m_stColors.clrBk);
		dcPaint.SetTextColor(m_stColors.clrFontHex);
		dcPaint.SetBkColor(m_stColors.clrBkHex);
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
	if (IsDataSetImpl()) {
		++iLines;
	}

	//Drawing through CMemDC to avoid flickering.
	GDIUT::CMemDC dcMem(dcPaint, rcClient);
	DrawWindow(dcMem);
	DrawInfoBar(dcMem);

	if (!IsDataSetImpl())
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

auto CHexCtrl::OnRButtonDown([[maybe_unused]] const MSG& msg)->LRESULT
{
	if (m_fScrollCursor) {
		SetScrollCursor();
		return 0;
	}

	return 0;
}

auto CHexCtrl::OnSetCursor(const MSG& msg)->LRESULT
{
	if (m_fScrollCursor) {
		static const auto hCurScroll = static_cast<HCURSOR>(::LoadImageW(nullptr, MAKEINTRESOURCEW(32654),
			IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED)); //Standard Windows scrolling cursor.
		::SetCursor(hCurScroll);
		return TRUE;
	}

	const auto wHitTest = LOWORD(msg.lParam);
	const auto wMessage = HIWORD(msg.lParam);
	m_ScrollV.OnSetCursor(wHitTest, wMessage);
	m_ScrollH.OnSetCursor(wHitTest, wMessage);

	return GDIUT::DefWndProc(msg); //To set appropriate cursor.
}

auto CHexCtrl::OnSetFocus([[maybe_unused]] const MSG& msg)->LRESULT
{
	m_DlgDataInterp.DisableHighlight();

	return 0;
}

auto CHexCtrl::OnSize(const MSG& msg)->LRESULT
{
	if (!IsCreated())
		return 0;

	const auto wWidth = LOWORD(msg.lParam);
	const auto wHeight = HIWORD(msg.lParam);
	RecalcClientArea(wWidth, wHeight);
	m_ScrollV.SetScrollPageSize(GetScrollPageSize());

	return 0;
}

auto CHexCtrl::OnTimer(const MSG& msg)->LRESULT
{
	const auto uIDTimer = msg.wParam;

	if (uIDTimer == m_uIDTTTMain) {
		constexpr auto dbSecToShow { 5000.0 }; //How many ms to show tooltips.
		auto rcClient = m_Wnd.GetClientRect();
		m_Wnd.ClientToScreen(rcClient);
		GDIUT::CPoint ptCur;
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

	if (uIDTimer == m_uIDTScrolCursor) {
		GDIUT::CPoint ptCur;
		::GetCursorPos(ptCur);
		const auto iSegmentPx = static_cast<int>(40 * GetDPIScale()); //Segment size in pixels, to accelerate against.
		const auto iSegmentsY = (ptCur.y - m_ptScrollCursorClick.y) / iSegmentPx; //How many vertical segments away from click.
		const auto i64NewScrollY = static_cast<std::int64_t>(m_ScrollV.GetScrollPos()
			+ m_ScrollV.GetScrollLineSize() / 4.F * iSegmentsY);
		m_ScrollV.SetScrollPos(i64NewScrollY);
		const auto iSegmentsX = (ptCur.x - m_ptScrollCursorClick.x) / iSegmentPx; //How many horizontal segments away from click.
		const auto i64NewScrollX = static_cast<std::int64_t>(m_ScrollH.GetScrollPos()
			+ m_ScrollH.GetScrollLineSize() / 4.F * iSegmentsX);
		m_ScrollH.SetScrollPos(i64NewScrollX);

		return 0;
	}

	return GDIUT::DefWndProc(msg);
}

auto CHexCtrl::OnVScroll([[maybe_unused]] const MSG& msg)->LRESULT
{
	bool fRedraw { true };
	if (m_fHighLatency) {
		fRedraw = m_ScrollV.IsThumbReleased();
		TTOffsetShow(!fRedraw);
	}

	if (fRedraw) {
		RedrawImpl();
	}

	return 0;
}

template<typename T> requires std::is_class_v<T>
void CHexCtrl::ParentNotify(const T& t)const
{
	if (const auto wndParent = m_Wnd.GetParent(); !wndParent.IsNull()) {
		wndParent.SendMsg(WM_NOTIFY, m_Wnd.GetDlgCtrlID(), reinterpret_cast<LPARAM>(&t));
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
		.Flags { static_cast<DWORD>(PD_RETURNDC | PD_NOCURRENTPAGE | (m_Selection.HasContiguousSel() ?
			PD_SELECTION : (PD_NOSELECTION | PD_PAGENUMS))) }, .nPageRanges { 1 }, .nMaxPageRanges { 1 },
		.lpPageRanges { &ppr }, .nMinPage { 1 }, .nMaxPage { 0xFFFFUL }, .nStartPage { START_PAGE_GENERAL } };

	if (::PrintDlgExW(&m_pdex) != S_OK) {
		ut::DBG_REPORT(L"PrintDlgExW error.");
		return;
	}

	//User pressed "Cancel", or "Apply" then "Cancel".
	if (m_pdex.dwResultAction != PD_RESULT_PRINT) {
		return;
	}

	if (m_pdex.hDC == nullptr) {
		ut::DBG_REPORT(L"No printer found (m_pdex.hDC == nullptr).");
		return;
	}

	const auto fPrintSelection = m_pdex.Flags & PD_SELECTION;
	const auto fPrintRange = m_pdex.Flags & PD_PAGENUMS;
	const auto fPrintAll = !fPrintSelection && !fPrintRange;

	if (!fPrintAll && !fPrintRange && !fPrintSelection) {
		::DeleteDC(m_pdex.hDC);
		return;
	}

	GDIUT::CDC dcPrint(m_pdex.hDC);
	if (const DOCINFOW di { .cbSize { sizeof(DOCINFOW) }, .lpszDocName { L"HexCtrl" } }; dcPrint.StartDocW(&di) < 0) {
		dcPrint.AbortDoc();
		dcPrint.DeleteDC();
		return;
	}

	constexpr auto iMarginX = 150;
	constexpr auto iMarginY = 150;
	const GDIUT::CRect rcPrint(POINT(0, 0), SIZE(::GetDeviceCaps(dcPrint, HORZRES) - (iMarginX * 2),
		::GetDeviceCaps(dcPrint, VERTRES) - (iMarginY * 2)));
	const SIZE sizePrintDpi { ::GetDeviceCaps(dcPrint, LOGPIXELSX), ::GetDeviceCaps(dcPrint, LOGPIXELSY) };
	const auto iFontSizeRatio { sizePrintDpi.cy / ::GetDpiForWindow(m_Wnd) };
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
		const auto ullSelStart = m_Selection.GetSelStart();
		const auto ullLinesToPrint = GetSelectedLines();
		const auto dwPagesToPrint = static_cast<DWORD>(ullLinesToPrint / iLinesInPage
			+ ((ullLinesToPrint % iLinesInPage) > 0 ? 1 : 0));
		auto ullStartLine = ullSelStart / dwCapacity;
		const auto ullLastLine = ullStartLine + ullLinesToPrint;
		const auto hcsOrig = GetColors();
		auto hcsPrint { hcsOrig }; //To print with normal text/bk colors, not white text on black bk.
		hcsPrint.clrBkSel = hcsOrig.clrBkHex;
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
		const auto ullTotalLines = GetDataSizeImpl() / dwCapacity + ((GetDataSizeImpl() % dwCapacity) ? 1 : 0);
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

			if (IsDataSetImpl()) {
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
	RedrawImpl();
}

void CHexCtrl::RecalcAll(bool fPrinter, HDC hDCPrinter, LPCRECT pRCPrinter)
{
	const GDIUT::CDC dcCurr = fPrinter ? hDCPrinter : m_Wnd.GetDC();
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
	m_iHeightInfoBarPx = m_fInfoBar ? m_sizeFontInfo.cy + (m_sizeFontInfo.cy / 3) : 0;
	constexpr auto iIndentBottomLine { 1 }; //Bottom line indent from window's bottom.
	m_iHeightBottomOffAreaPx = m_iHeightInfoBarPx + iIndentBottomLine;

	const auto iCharWidth = GetCharWidthNative();
	const auto iCharWidthExt = GetCharWidthExtras();
	const auto dwCapacity = GetCapacityImpl();

	//Approximately "dwCapacity * 3 + 1" size array of char's width, to be enough for the Hex area chars.
	m_vecCharsWidth.assign((dwCapacity * 3) + 1, iCharWidthExt);
	m_iSecondVertLinePx = m_iFirstVertLinePx + GetDigitsOffset() * iCharWidth + iCharWidth * 2;
	m_iSizeHexBytePx = iCharWidthExt * 2;
	m_iSpaceBetweenBlocksPx = (m_dwGroupSize == 1 && dwCapacity > 1) ? iCharWidthExt * 2 : 0;
	m_iDistanceGroupedHexChunkPx = m_iSizeHexBytePx * m_dwGroupSize + iCharWidthExt;
	m_iThirdVertLinePx = m_iSecondVertLinePx + m_iDistanceGroupedHexChunkPx * (dwCapacity / m_dwGroupSize)
		+ iCharWidth + m_iSpaceBetweenBlocksPx;
	m_iIndentTextXPx = m_iThirdVertLinePx + iCharWidth;
	m_iDistanceBetweenCharsPx = iCharWidthExt;
	m_iFourthVertLinePx = m_iIndentTextXPx + (m_iDistanceBetweenCharsPx * dwCapacity) + iCharWidth;
	m_iIndentFirstHexChunkXPx = m_iSecondVertLinePx + iCharWidth;
	m_iSizeFirstHalfPx = m_iIndentFirstHexChunkXPx + m_dwCapacityBlockSize * (iCharWidthExt * 2) +
		(m_dwCapacityBlockSize / m_dwGroupSize - 1) * iCharWidthExt;
	m_iHeightTopRectPx = std::lround(m_sizeFontMain.cy * 1.5);
	m_iStartWorkAreaYPx = m_iFirstHorzLinePx + m_iHeightTopRectPx;
	m_iSecondHorzLinePx = m_iStartWorkAreaYPx - 1;
	m_iIndentCapTextYPx = m_iHeightTopRectPx / 2 - (m_sizeFontMain.cy / 2);

	const GDIUT::CRect rc { fPrinter ? *pRCPrinter : m_Wnd.GetClientRect() };
	RecalcClientArea(rc.Width(), rc.Height());

	//Scrolls and ReleaseDC only for window DC, not for printer DC.
	if (!fPrinter) {
		const auto ullDataSize = GetDataSizeImpl();
		m_ScrollV.SetScrollSizes(m_sizeFontMain.cy, GetScrollPageSize(),
			static_cast<ULONGLONG>(m_iStartWorkAreaYPx) + m_iHeightBottomOffAreaPx
			+ (m_sizeFontMain.cy * (ullDataSize / dwCapacity + (ullDataSize % dwCapacity == 0 ? 1 : 2))));
		m_ScrollH.SetScrollSizes(iCharWidthExt, rc.Width(), static_cast<ULONGLONG>(m_iFourthVertLinePx) + 1);
		m_ScrollV.SetScrollPos(ullCurLineV * m_sizeFontMain.cy);
		m_Wnd.ReleaseDC(dcCurr);
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
		[](UNDO& undo) { return HEXSPAN { undo.ullOffset, undo.vecData.size() }; });
	SnapshotUndo(vecSpan); //Creating new Undo data snapshot.

	for (const auto& redo : *uptrRedo) {
		const auto& vecRedoData = redo.vecData;

		if (IsVirtualImpl() && vecRedoData.size() > GetCacheSize()) { //In VirtualData mode processing data chunk by chunk.
			const auto dwSizeChunk = GetCacheSize();
			const auto uzMod = vecRedoData.size() % dwSizeChunk;
			auto ullChunks = vecRedoData.size() / dwSizeChunk + (uzMod > 0 ? 1 : 0);
			std::size_t uzOffset = 0;
			while (ullChunks-- > 0) {
				const auto ullSize = (ullChunks == 1 && uzMod > 0) ? uzMod : dwSizeChunk;
				if (const auto spnData = GetData({ uzOffset, ullSize }); !spnData.empty()) {
					std::copy_n(vecRedoData.begin() + uzOffset, ullSize, spnData.data());
					SetDataVirtual(spnData, { uzOffset, ullSize });
				}
				uzOffset += ullSize;
			}
		}
		else {
			if (const auto spnData = GetData({ redo.ullOffset, vecRedoData.size() }); !spnData.empty()) {
				std::copy_n(vecRedoData.begin(), vecRedoData.size(), spnData.data());
				SetDataVirtual(spnData, { redo.ullOffset, vecRedoData.size() });
			}
		}
	}

	m_vecRedo.pop_back();
	OnModifyData();
	RedrawImpl();
}

void CHexCtrl::RedrawImpl() {
	m_Wnd.RedrawWindow();
}

void CHexCtrl::ReplaceUnprintable(std::wstring& wstr, bool fASCII, bool fCRLF)const
{
	//If fASCII is true, then only wchars in the 0x1F < ... < 0x7F range are considered printable.
	//If fCRLF is false, then CR(0x0D) and LF(0x0A) wchars remain untouched.
	if (fASCII) {
		std::replace_if(wstr.begin(), wstr.end(), [=](wchar_t wch) //All non ASCII.
			{ return (wch < 0x20 || wch > 0x7E) && (fCRLF || (wch != 0x0D && wch != 0x0A)); }, m_wchUnprintable);
	}
	else {
		std::replace_if(wstr.begin(), wstr.end(), [=](wchar_t wch) //All non printable wchars.
			{ return !std::iswprint(wch) && (fCRLF || (wch != 0x0D && wch != 0x0A)); }, m_wchUnprintable);
	}
}

void CHexCtrl::ScrollOffsetH(ULONGLONG ullOffset)
{
	//Horizontally-only scrolls to a given offset.
	if (!m_ScrollH.IsVisible())
		return;

	int iCx, iCy;
	HexChunkPoint(ullOffset, iCx, iCy);
	const auto rcClient = m_Wnd.GetClientRect();

	const auto ullCurrScrollH = m_ScrollH.GetScrollPos();
	auto ullNewScrollH { ullCurrScrollH };
	const auto iMaxClientX = rcClient.Width() - m_iSizeHexBytePx;
	const auto iAdditionalSpace { m_iSizeHexBytePx / 2 };
	if (iCx >= iMaxClientX) {
		ullNewScrollH += iCx - iMaxClientX + iAdditionalSpace;
	}
	else if (iCx < 0) {
		ullNewScrollH += iCx - iAdditionalSpace;
	}

	m_ScrollH.SetScrollPos(ullNewScrollH);
}

void CHexCtrl::SelAll()
{
	if (!IsDataSetImpl())
		return;

	SetSelection({ { 0, GetDataSizeImpl() } }); //Select all.
}

void CHexCtrl::SelAddDown()
{
	const auto fHasSel = HasSelection();
	const auto ullSelStart = fHasSel ? m_Selection.GetSelStart() : m_ullCaretPos;
	const auto ullSelSize = fHasSel ? m_Selection.GetSelSize() : 1;
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

	if (m_ullCaretPos == m_ullCursorPrev || m_ullCaretPos == ullSelStart || m_ullCaretPos == m_Selection.GetSelEnd()) {
		lmbSelection();
	}
	else {
		ullClick = ullStart = m_ullCaretPos;
		ullSize = 1;
	}

	if (ullStart + ullSize > GetDataSizeImpl()) { //To avoid overflow.
		ullSize = GetDataSizeImpl() - ullStart;
	}

	if (ullSize > 0) {
		m_ullCaretPos = ullStart;
		m_ullCursorPrev = ullClick;
		SetSelection({ { ullStart, ullSize } }, false);

		const auto stOld = IsOffsetVisible(ullOldPos);
		const auto stNew = IsOffsetVisible(ullNewPos);
		if (stNew.i8Vert != 0 && stOld.i8Vert == 0) {
			m_ScrollV.ScrollLineDown();
		}

		RedrawImpl();
		OnCaretPosChange(GetCaretPosImpl());
	}
}

void CHexCtrl::SelAddLeft()
{
	const auto fHasSel = HasSelection();
	const auto ullSelStart = fHasSel ? m_Selection.GetSelStart() : m_ullCaretPos;
	const auto ullSelSize = fHasSel ? m_Selection.GetSelSize() : 1;
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

	if (m_ullCaretPos == m_ullCursorPrev || m_ullCaretPos == ullSelStart || m_ullCaretPos == m_Selection.GetSelEnd()) {
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
			m_ScrollV.ScrollLineUp();
		}
		else if (stNew.i8Horz != 0 && !IsCurTextArea()) { //Do not horz scroll when in text area.
			ScrollOffsetH(ullNewPos);
		}
		else {
			RedrawImpl();
		}
		OnCaretPosChange(GetCaretPosImpl());
	}
}

void CHexCtrl::SelAddRight()
{
	const auto fHasSel = HasSelection();
	const auto ullSelStart = fHasSel ? m_Selection.GetSelStart() : m_ullCaretPos;
	const auto ullSelSize = fHasSel ? m_Selection.GetSelSize() : 1;
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

	if (m_ullCaretPos == m_ullCursorPrev || m_ullCaretPos == ullSelStart || m_ullCaretPos == m_Selection.GetSelEnd()) {
		lmbSelection();
	}
	else {
		ullClick = ullStart = m_ullCaretPos;
		ullSize = 1;
	}

	if (ullStart + ullSize > GetDataSizeImpl()) { //To avoid overflow.
		ullSize = GetDataSizeImpl() - ullStart;
	}

	if (ullSize > 0) {
		m_ullCaretPos = ullStart;
		m_ullCursorPrev = ullClick;
		SetSelection({ { ullStart, ullSize } }, false);

		const auto stOld = IsOffsetVisible(ullOldPos);
		const auto stNew = IsOffsetVisible(ullNewPos);
		if (stNew.i8Vert != 0 && stOld.i8Vert == 0) {
			m_ScrollV.ScrollLineDown();
		}
		else if (stNew.i8Horz != 0 && !IsCurTextArea()) { //Do not horz scroll when in text area.
			ScrollOffsetH(ullNewPos);
		}
		else {
			RedrawImpl();
		}
		OnCaretPosChange(GetCaretPosImpl());
	}
}

void CHexCtrl::SelAddUp()
{
	const auto fHasSel = HasSelection();
	const auto ullSelStart = fHasSel ? m_Selection.GetSelStart() : m_ullCaretPos;
	const auto ullSelSize = fHasSel ? m_Selection.GetSelSize() : 1;
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

	if (m_ullCaretPos == m_ullCursorPrev || m_ullCaretPos == ullSelStart || m_ullCaretPos == m_Selection.GetSelEnd()) {
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
			m_ScrollV.ScrollLineUp();
		}

		RedrawImpl();
		OnCaretPosChange(GetCaretPosImpl());
	}
}

void CHexCtrl::SetCapacityImpl(std::uint32_t dwCapacity, bool fRedraw, bool fNotify)
{
	//SetCapacityImpl can be called with the current capacity size. This needs for the 
	//SetGroupSizeImpl to recalc current capacity when group size has changed.
	if (dwCapacity < 1UL || dwCapacity > 100UL) //Restrict capacity size in the [1-100] range.
		return;

	const auto dwCurrCapacity = GetCapacityImpl();
	const auto dwMod = dwCapacity % m_dwGroupSize; //Setting the capacity according to the current data grouping size.

	if (dwCapacity < dwCurrCapacity) {
		dwCapacity -= dwMod;
	}
	else {
		dwCapacity += dwMod > 0 ? m_dwGroupSize - dwMod : 0; //Remaining part. Actual only if dwMod > 0.
	}

	if (dwCapacity < m_dwGroupSize) {
		dwCapacity = m_dwGroupSize;
	}
	else if (dwCapacity > 100UL) { //100 is the maximum allowed capacity.
		dwCapacity -= m_dwGroupSize;
	}

	m_dwCapacity = dwCapacity;
	m_dwCapacityBlockSize = dwCapacity / 2;

	RecalcAll();

	if (fRedraw) {
		RedrawImpl();
	}

	if (fNotify) {
		ParentNotify(HEXCTRL_MSG_SETCAPACITY);
	}
}

void CHexCtrl::SetCodepageImpl(int iCodepage, bool fRedraw, bool fNotify)
{
	if (iCodepage != -1 && iCodepage != 0) { //-1 - ASCII, 0 - UTF-16.
		if (CPINFOEXW stCP; ::GetCPInfoExW(static_cast<UINT>(iCodepage), 0, &stCP) == FALSE) {
			ut::DBG_REPORT(L"Unsupported codepage.");
			iCodepage = -1;
		}
	}

	m_iCodePage = iCodepage;

	if (fRedraw) { RedrawImpl(); }
	if (fNotify) { ParentNotify(HEXCTRL_MSG_SETCODEPAGE); }
}

bool CHexCtrl::SetConfigImpl(std::wstring_view wsvPath)
{
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
		{ "CMD_SCROLL_CURSOR", { CMD_SCROLL_CURSOR, 0 } },
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
		{ { "mouse_wheel_down" }, { m_dwVKMouseWheelDown, L"Mouse-Wheel Down" } },
		{ { "middle_button_down" }, { m_dwVKMiddleButtonDown, L"Middle Button Down" } }
	};

	//Filling m_vecKeyBind with ALL available commands/menus.
	//This is vital for ExecuteCmd to work properly.
	m_vecKeyBind.clear();
	m_vecKeyBind.reserve(umapCmdMenu.size());
	for (const auto& map : umapCmdMenu) {
		m_vecKeyBind.emplace_back(KEYBIND { .eCmd { map.second.first }, .wMenuID { static_cast<WORD>(map.second.second) } });
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

		const auto uzSize = static_cast<std::size_t>(::SizeofResource(m_hInstRes, hRes));
		const auto pData = static_cast<const char*>(::LockResource(hResData));
		docJSON.Parse(pData, uzSize); //Parse all default keybindings.
		if (docJSON.IsNull()) {
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
		const auto uzSize = sv.size();
		std::size_t uzPosStart { 0 }; //Next position to start search for '+' sign.
		const auto zSubWords = std::count(sv.begin(), sv.end(), '+') + 1; //How many sub-words (divided by '+')?
		for (auto itSubWords = 0; itSubWords < zSubWords; ++itSubWords) {
			const auto uzPosNext = sv.find('+', uzPosStart);
			const auto uzSizeSubWord = uzPosNext == std::string_view::npos ? uzSize - uzPosStart : uzPosNext - uzPosStart;
			const auto svSubWord = sv.substr(uzPosStart, uzSizeSubWord);
			uzPosStart = uzPosNext + 1;

			if (svSubWord.size() == 1) {
				stKB.uKey = static_cast<UCHAR>(std::toupper(svSubWord[0])); //Binding keys are in uppercase.
			}
			else if (const auto itKey = umapKeys.find(svSubWord); itKey != umapKeys.end()) {
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
						[&optKB](const KEYBIND& kb) { return kb.eCmd == optKB->eCmd; }); itKB != m_vecKeyBind.end()) {
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
	for (const auto& vecKB : m_vecKeyBind) {
		//Check for previous same menu ID. To assign only one, first, keybinding for menu name.
		//With `"ctrl+f", "ctrl+h"` in JSON, only the "Ctrl+F" will be assigned as the menu name.
		const auto itEnd = m_vecKeyBind.begin() + i++;
		if (const auto itTmp = std::find_if(m_vecKeyBind.begin(), itEnd, [&](const KEYBIND& kb) {
			return kb.wMenuID == vecKB.wMenuID; });
			itTmp == itEnd && vecKB.wMenuID != 0 && vecKB.uKey != 0) {
			auto wstr = m_MenuMain.GetItemWstr(vecKB.wMenuID);
			if (const auto uzPos = wstr.find('\t'); uzPos != std::wstring::npos) {
				wstr.erase(uzPos);
			}

			wstr += L'\t';
			if (vecKB.fCtrl) {
				wstr += L"Ctrl+";
			}
			if (vecKB.fShift) {
				wstr += L"Shift+";
			}
			if (vecKB.fAlt) {
				wstr += L"Alt+";
			}

			//Search for any special key names: 'Tab', 'Enter', etc... If not found then it's just a char.
			if (const auto itUmap = std::find_if(umapKeys.begin(), umapKeys.end(), [&](const auto& map) {
				return map.second.first == vecKB.uKey; }); itUmap != umapKeys.end()) {
				wstr += itUmap->second.second;
			}
			else {
				wstr += static_cast<unsigned char>(vecKB.uKey);
			}

			m_MenuMain.SetItemWstr(vecKB.wMenuID, wstr); //Modify the menu with the new name, with shortcut appended.
		}
	}

	return true;
}

void CHexCtrl::SetDateInfoImpl(std::uint32_t dwFormat, wchar_t wchSepar)
{
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

void CHexCtrl::SetDataVirtual(SpanByte spnData, const HEXSPAN& hss)const
{
	//Note: Since this method can be executed asynchronously (in search/replace, etc...),
	//the SendMesage(parent, ...) is impossible here because receiver window
	//must be run in the same thread as a sender.

	if (!IsVirtualImpl())
		return;

	m_pHexVirtData->OnHexSetData({ .hdr { m_Wnd, static_cast<UINT>(m_Wnd.GetDlgCtrlID()) },
		.stHexSpan { hss }, .spnData { spnData } });
}

void CHexCtrl::SetFontImpl(const LOGFONTW& lf, bool fMain, bool fRedraw, bool fNotify)
{
	if (fMain) {
		::DeleteObject(m_hFntMain);
		m_hFntMain = ::CreateFontIndirectW(&lf);
	}
	else {
		::DeleteObject(m_hFntInfoBar);
		m_hFntInfoBar = ::CreateFontIndirectW(&lf);
	}

	if (fRedraw) {
		RecalcAll();
		RedrawImpl();
	}

	if (fNotify) { ParentNotify(HEXCTRL_MSG_SETFONT); }
}

void CHexCtrl::SetFontSizeInPoints(float flSizePoints, bool fMain)
{
	if (flSizePoints < 4.F || flSizePoints > 64.F) //Prevent font size from being too small or too big.
		return;

	auto lf = GetFont(fMain);
	lf.lfHeight = -FontScaledPixelsFromPoints(flSizePoints);
	SetFont(lf, fMain);
}

void CHexCtrl::SetGroupSizeImpl(DWORD dwSize, bool fRedraw, bool fNotify)
{
	if (dwSize < 1UL || dwSize > 64UL || dwSize == m_dwGroupSize) //Restrict group size in the [1-64] range.
		return;

	m_dwGroupSize = dwSize;

	//Getting the "Group Data" menu pointer independent of position.
	const auto menuMain = m_MenuMain.GetSubMenu(0);
	HMENU hMenuGroupData { };
	for (auto i = 0; i < menuMain.GetItemsCount(); ++i) {
		//Searching through all submenus whose first menuID is IDM_HEXCTRL_GROUPDATA_BYTE.
		if (auto menuSub = menuMain.GetSubMenu(i); menuSub.IsMenu()) {
			if (menuSub.GetItemID(0) == IDM_HEXCTRL_GROUPDATA_BYTE) {
				hMenuGroupData = menuSub.GetHMENU();
				break;
			}
		}
	}

	if (hMenuGroupData != nullptr) {
		//Unchecking all menus and checking only the currently selected.
		GDIUT::CMenu menuGroup(hMenuGroupData);
		for (auto iIDGroupData = 0; iIDGroupData < menuGroup.GetItemsCount(); ++iIDGroupData) {
			menuGroup.SetItemCheck(iIDGroupData, false, false);
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
			menuGroup.SetItemCheck(uIDToCheck, true);
		}
	}

	if (fNotify) {
		ParentNotify(HEXCTRL_MSG_SETGROUPSIZE);
	}

	SetCapacityImpl(m_dwCapacity, fRedraw, fNotify); //To recalc current representation.
}

void CHexCtrl::SetScrollCursor()
{
	m_fScrollCursor = !m_fScrollCursor;

	if (m_fScrollCursor) {
		::GetCursorPos(m_ptScrollCursorClick);
		m_Wnd.SetTimer(m_uIDTScrolCursor, 20, nullptr);
	}
	else {
		m_Wnd.KillTimer(m_uIDTScrolCursor);
	}
}

void CHexCtrl::SetUnprintableCharImpl(wchar_t wch, bool fRedraw)
{
	m_wchUnprintable = wch;
	if (fRedraw) { RedrawImpl(); }
}

void CHexCtrl::SnapshotUndo(const VecSpan& vecSpan)
{
	constexpr auto dwUndoMax { 512U }; //Undo's max limit.
	const auto ullTotalSize = std::reduce(vecSpan.begin(), vecSpan.end(), 0ULL,
		[](ULONGLONG ullSumm, const HEXSPAN& hs) { return ullSumm + hs.ullSize; });

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
	const auto& uptrVec = m_vecUndo.emplace_back(std::make_unique<std::vector<UNDO>>());

	//Bad alloc may happen here!!!
	try {
		for (const auto& hs : vecSpan) { //vecSpan.size() amount of continuous areas to preserve.
			auto& undo = uptrVec->emplace_back(UNDO { hs.ullOffset, { } });
			undo.vecData.resize(static_cast<std::size_t>(hs.ullSize));

			//In VirtualData mode processing data chunk by chunk.
			if (IsVirtualImpl() && hs.ullSize > GetCacheSize()) {
				const auto dwSizeChunk = GetCacheSize();
				const auto ullMod = hs.ullSize % dwSizeChunk;
				auto ullChunks = hs.ullSize / dwSizeChunk + (ullMod > 0 ? 1 : 0);
				ULONGLONG ullOffset { 0 };
				while (ullChunks-- > 0) {
					const auto ullSize = (ullChunks == 1 && ullMod > 0) ? ullMod : dwSizeChunk;
					if (const auto spnData = GetData({ ullOffset, ullSize }); !spnData.empty()) {
						std::copy_n(spnData.data(), ullSize, undo.vecData.data() + static_cast<std::size_t>(ullOffset));
					}
					ullOffset += ullSize;
				}
			}
			else {
				if (const auto spnData = GetData(hs); !spnData.empty()) {
					std::copy_n(spnData.data(), hs.ullSize, undo.vecData.data());
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
	iCx = static_cast<int>((m_iIndentTextXPx + dwMod * GetCharWidthExtras()) - m_ScrollH.GetScrollPos());

	const auto ullScrollV = m_ScrollV.GetScrollPos();
	iCy = static_cast<int>((m_iStartWorkAreaYPx + (ullOffset / dwCapacity) * m_sizeFontMain.cy) -
		(ullScrollV - (ullScrollV % m_sizeFontMain.cy)));
}

void CHexCtrl::TTMainShow(bool fShow, bool fTimer)
{
	if (fShow) {
		m_tmTT = std::chrono::high_resolution_clock::now();
		POINT ptCur;
		::GetCursorPos(&ptCur);
		m_wndTTMain.SendMsg(TTM_TRACKPOSITION, 0, static_cast<LPARAM>(MAKELONG(ptCur.x + 9, ptCur.y - 20)));
		m_wndTTMain.SendMsg(TTM_UPDATETIPTEXTW, 0, reinterpret_cast<LPARAM>(&m_ttiMain));
		m_wndTTMain.SendMsg(TTM_TRACKACTIVATE, TRUE, reinterpret_cast<LPARAM>(&m_ttiMain));
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
		auto wstrOffset = (IsOffsetAsHexImpl() ? L"Offset: 0x" : L"Offset: ") + OffsetToWstr(GetTopLine() * GetCapacity());
		m_ttiOffset.lpszText = wstrOffset.data();
		m_wndTTOffset.SendMsg(TTM_TRACKPOSITION, 0, static_cast<LPARAM>(MAKELONG(ptCur.x - 5, ptCur.y - 20)));
		m_wndTTOffset.SendMsg(TTM_UPDATETIPTEXTW, 0, reinterpret_cast<LPARAM>(&m_ttiOffset));
		m_ttiOffset.lpszText = nullptr;
	}

	m_wndTTOffset.SendMsg(TTM_TRACKACTIVATE, fShow, reinterpret_cast<LPARAM>(&m_ttiOffset));
}

void CHexCtrl::Undo()
{
	if (m_vecUndo.empty())
		return;

	//Bad alloc may happen here! If there is no more free memory, just clear the vec and return.
	try {
		//Creating new Redo data snapshot.
		const auto& uptrVec = m_vecRedo.emplace_back(std::make_unique<std::vector<UNDO>>());
		for (const auto& undo : *m_vecUndo.back()) {
			auto& redo = uptrVec->emplace_back(UNDO { undo.ullOffset, { } });
			redo.vecData.resize(undo.vecData.size());
			const auto& vecUndoData = undo.vecData;

			if (IsVirtualImpl() && vecUndoData.size() > GetCacheSize()) { //In VirtualData mode processing data chunk by chunk.
				const auto dwSizeChunk = GetCacheSize();
				const auto uzMod = vecUndoData.size() % dwSizeChunk;
				auto ullChunks = vecUndoData.size() / dwSizeChunk + (uzMod > 0 ? 1 : 0);
				std::size_t ullOffset = 0;
				while (ullChunks-- > 0) {
					const auto ullSize = (ullChunks == 1 && uzMod > 0) ? uzMod : dwSizeChunk;
					if (const auto spnData = GetData({ ullOffset, ullSize }); !spnData.empty()) {
						std::copy_n(spnData.data(), ullSize, redo.vecData.begin() + ullOffset); //Fill Redo with the data.
						std::copy_n(vecUndoData.begin() + ullOffset, ullSize, spnData.data()); //Undo the data.
						SetDataVirtual(spnData, { ullOffset, ullSize });
					}
					ullOffset += ullSize;
				}
			}
			else {
				if (const auto spnData = GetData({ undo.ullOffset, vecUndoData.size() }); !spnData.empty()) {
					std::copy_n(spnData.data(), vecUndoData.size(), redo.vecData.begin()); //Fill Redo with the data.
					std::copy_n(vecUndoData.begin(), vecUndoData.size(), spnData.data()); //Undo the data.
					SetDataVirtual(spnData, { undo.ullOffset, vecUndoData.size() });
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
	RedrawImpl();
}

void CHexCtrl::UpdateDPIScale()
{
	m_flDPIScale = GDIUT::GetDPIScaleForHWND(m_Wnd);
}

void CHexCtrl::ModifyOperScalar(std::byte* pData, const HEXMODIFY& hms, [[maybe_unused]] SpanCByte)
{
	assert(pData != nullptr);
	using enum EHexDataType;
	using enum EHexOperMode;

	constexpr auto lmbOperScalarT = []<typename T>(T * pData, const HEXMODIFY & hms) {
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
			if constexpr (std::is_floating_point_v<T>) {
				tData = std::fmax(tData, tOper);
			}
			else {
				tData = (std::max)(tData, tOper);
			}
			break;
		case OPER_MAX:
			if constexpr (std::is_floating_point_v<T>) {
				tData = std::fmin(tData, tOper);
			}
			else {
				tData = (std::min)(tData, tOper);
			}
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
		lmbOperScalarT(reinterpret_cast<std::int8_t*>(pData), hms);
		break;
	case DATA_UINT8:
		lmbOperScalarT(reinterpret_cast<std::uint8_t*>(pData), hms);
		break;
	case DATA_INT16:
		lmbOperScalarT(reinterpret_cast<std::int16_t*>(pData), hms);
		break;
	case DATA_UINT16:
		lmbOperScalarT(reinterpret_cast<std::uint16_t*>(pData), hms);
		break;
	case DATA_INT32:
		lmbOperScalarT(reinterpret_cast<std::int32_t*>(pData), hms);
		break;
	case DATA_UINT32:
		lmbOperScalarT(reinterpret_cast<std::uint32_t*>(pData), hms);
		break;
	case DATA_INT64:
		lmbOperScalarT(reinterpret_cast<std::int64_t*>(pData), hms);
		break;
	case DATA_UINT64:
		lmbOperScalarT(reinterpret_cast<std::uint64_t*>(pData), hms);
		break;
	case DATA_FLOAT:
		lmbOperScalarT(reinterpret_cast<float*>(pData), hms);
		break;
	case DATA_DOUBLE:
		lmbOperScalarT(reinterpret_cast<double*>(pData), hms);
		break;
	default:
		break;
	}
}

auto CHexCtrl::SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIDSubclass, [[maybe_unused]] DWORD_PTR dwRefData) -> LRESULT {
	if (uMsg == WM_NCDESTROY) {
		::RemoveWindowSubclass(hWnd, SubclassProc, uIDSubclass);
	}

	const auto pHexCtrl = reinterpret_cast<CHexCtrl*>(uIDSubclass);
	return pHexCtrl->ProcessMsg({ .hwnd { hWnd }, .message { uMsg }, .wParam { wParam }, .lParam { lParam } });
}