/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../HexCtrl.h"
#include <afxwin.h>      //MFC core and standard components.
#include <algorithm>
#include <chrono>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace HEXCTRL::INTERNAL {
	//Forward declarations.
	class CHexDlgBkmMgr;
	class CHexDlgCodepage;
	class CHexDlgDataInterp;
	class CHexDlgModify;
	class CHexDlgGoTo;
	class CHexDlgSearch;
	class CHexDlgTemplMgr;
	class CHexScroll;
	class CHexSelection;

	/********************************************************************************************
	* CHexCtrl class is an implementation of the IHexCtrl interface.                            *
	********************************************************************************************/
	class CHexCtrl final : public CWnd, public IHexCtrl {
	public:
		explicit CHexCtrl() = default;
		explicit CHexCtrl(HINSTANCE hInstClass);
		CHexCtrl(const CHexCtrl&) = delete;
		CHexCtrl(CHexCtrl&&) = delete;
		CHexCtrl& operator=(const CHexCtrl&) = delete;
		CHexCtrl& operator=(CHexCtrl&&) = delete;
		~CHexCtrl() = default;
		void ClearData()override;
		bool Create(const HEXCREATE& hcs)override;
		bool CreateDialogCtrl(UINT uCtrlID, HWND hWndParent)override;
		void Destroy()override;
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
		[[nodiscard]] auto GetDlgItemHandle(EHexWnd eWnd, EHexDlgItem eItem)const->HWND override;
		[[nodiscard]] auto GetFont() -> LOGFONTW override;
		[[nodiscard]] auto GetGroupSize()const->DWORD override;
		[[nodiscard]] auto GetMenuHandle()const->HMENU override;
		[[nodiscard]] auto GetPagesCount()const->ULONGLONG override;
		[[nodiscard]] auto GetPagePos()const->ULONGLONG override;
		[[nodiscard]] auto GetPageSize()const->DWORD override;
		[[nodiscard]] auto GetScrollRatio()const->std::tuple<float, bool> override;
		[[nodiscard]] auto GetSelection()const->VecSpan override;
		[[nodiscard]] auto GetTemplates()const->IHexTemplates* override;
		[[nodiscard]] auto GetUnprintableChar()const->wchar_t override;
		[[nodiscard]] auto GetWndHandle(EHexWnd eWnd, bool fCreate)const->HWND override;
		void GoToOffset(ULONGLONG ullOffset, int iRelPos = 0)override;
		[[nodiscard]] bool HasSelection()const override;
		[[nodiscard]] auto HitTest(POINT pt, bool fScreen)const->std::optional<HEXHITTEST> override;
		[[nodiscard]] bool IsCmdAvail(EHexCmd eCmd)const override;
		[[nodiscard]] bool IsCreated()const override;
		[[nodiscard]] bool IsDataSet()const override;
		[[nodiscard]] bool IsInfoBar()const override;
		[[nodiscard]] bool IsMutable()const override;
		[[nodiscard]] bool IsOffsetAsHex()const override;
		[[nodiscard]] auto IsOffsetVisible(ULONGLONG ullOffset)const->HEXVISION override;
		[[nodiscard]] bool IsVirtual()const override;
		void ModifyData(const HEXMODIFY& hms)override;
		void Redraw()override;
		void SetCapacity(DWORD dwCapacity)override;
		void SetCaretPos(ULONGLONG ullOffset, bool fHighLow = true, bool fRedraw = true)override;
		void SetCharsExtraSpace(DWORD dwSpace)override;
		void SetCodepage(int iCodepage)override;
		void SetColors(const HEXCOLORS& hcs)override;
		bool SetConfig(std::wstring_view wsvPath)override;
		void SetData(const HEXDATA& hds)override;
		void SetDateInfo(DWORD dwFormat, wchar_t wchSepar)override;
		void SetDlgProperties(EHexWnd eWnd, std::uint64_t u64Flags)override;
		void SetFont(const LOGFONTW& lf)override;
		void SetGroupSize(DWORD dwSize)override;
		void SetMutable(bool fEnable)override;
		void SetOffsetMode(bool fHex)override;
		void SetPageSize(DWORD dwSize, std::wstring_view wsvName)override;
		void SetRedraw(bool fRedraw)override;
		void SetScrollRatio(float flRatio, bool fLines)override;
		void SetSelection(const VecSpan& vecSel, bool fRedraw = true, bool fHighlight = false)override;
		void SetUnprintableChar(wchar_t wch)override;
		void SetVirtualBkm(IHexBookmarks* pVirtBkm)override;
		void ShowInfoBar(bool fShow)override;
	private:
		struct UNDO;
		struct KEYBIND;
		enum class EClipboard : std::uint8_t;
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
		void DrawWindow(CDC* pDC)const;
		void DrawInfoBar(CDC* pDC)const;
		void DrawOffsets(CDC* pDC, ULONGLONG ullStartLine, int iLines)const;
		void DrawHexText(CDC* pDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const;
		void DrawTemplates(CDC* pDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const;
		void DrawBookmarks(CDC* pDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const;
		void DrawSelection(CDC* pDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const;
		void DrawSelHighlight(CDC* pDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const;
		void DrawCaret(CDC* pDC, ULONGLONG ullStartLine, std::wstring_view wsvHex, std::wstring_view wsvText)const;
		void DrawDataInterp(CDC* pDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const;
		void DrawPageLines(CDC* pDC, ULONGLONG ullStartLine, int iLines);
		void FillCapacityString();  //Fill m_wstrCapacity according to current m_dwCapacity.
		void FillWithZeros();       //Fill selection with zeros.
		void FontSizeIncDec(bool fInc = true); //Increase os decrease font size by minimum amount.
		[[nodiscard]] auto GetBottomLine()const->ULONGLONG;    //Returns current bottom line number in view.
		[[nodiscard]] auto GetCharsWidthArray()const->int*;
		[[nodiscard]] auto GetCharWidthExtras()const->int;     //Width of the one char with extra space, in px.
		[[nodiscard]] auto GetCharWidthNative()const->int;     //Width of the one char, in px.
		[[nodiscard]] auto GetCommandFromKey(UINT uKey, bool fCtrl, bool fShift, bool fAlt)const->std::optional<EHexCmd>; //Get command from keybinding.
		[[nodiscard]] auto GetCommandFromMenu(WORD wMenuID)const->std::optional<EHexCmd>; //Get command from menuID.
		[[nodiscard]] long GetFontSize();
		[[nodiscard]] auto GetRectTextCaption()const->CRect;   //Returns rect of the text caption area.
		[[nodiscard]] auto GetScrollPageSize()const->ULONGLONG; //Get the "Page" size of the scroll.
		[[nodiscard]] auto GetTopLine()const->ULONGLONG;       //Returns current top line number in view.
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
		void RecalcAll(CDC* pDC = nullptr, const CRect* pRect = nullptr); //Recalculates all drawing sizes for given DC.
		void RecalcClientArea(int iHeight, int iWidth);
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
		void TextChunkPoint(ULONGLONG ullOffset, int& iCx, int& iCy)const;     //Point of the text chunk.
		void TTBkmShow(bool fShow, bool fTimer = false);   //Tooltip for bookmark show/hide.
		void TTTemplShow(bool fShow, bool fTimer = false); //Tooltip for templates show/hide.
		void TTOffsetShow(bool fShow); //Tooltip Offset show/hide.
		void Undo();

		//MFC message handlers.
		afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
		BOOL OnCommand(WPARAM wParam, LPARAM lParam)override;
		afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
		afx_msg void OnDestroy();
		afx_msg BOOL OnEraseBkgnd(CDC* pDC);
		afx_msg UINT OnGetDlgCode(); //To properly work in dialogs.
		afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
		afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
		afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
		afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
		afx_msg void OnMouseMove(UINT nFlags, CPoint point);
		afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
		afx_msg BOOL OnNcActivate(BOOL bActive);
		afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
		afx_msg void OnNcPaint();
		afx_msg void OnPaint();
		afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
		afx_msg void OnSize(UINT nType, int cx, int cy);
		afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void OnTimer(UINT_PTR nIDEvent);
		afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
		DECLARE_MESSAGE_MAP();
	private:
		static constexpr auto m_pwszHexChars { L"0123456789ABCDEF" }; //Hex digits wchars for fast lookup.
		static constexpr auto m_pwszClassName { L"HexCtrl" }; //HexCtrl Window Class name.
		static constexpr auto m_uIDTTBkm { 0x01UL };     //Tooltip ID for Bookmarks.
		static constexpr auto m_uIDTTTempl { 0x02UL };   //Tooltip ID for Templates.
		static constexpr auto m_iIndentBottomLine { 1 }; //Bottom line indent from window's bottom.
		static constexpr auto m_iFirstHorzLinePx { 0 };  //First horizontal line indent.
		static constexpr auto m_iFirstVertLinePx { 0 };  //First vertical line indent.
		static constexpr auto m_dwVKMouseWheelUp { 0x0100UL };   //Artificial Virtual Key for a Mouse-Wheel Up event.
		static constexpr auto m_dwVKMouseWheelDown { 0x0101UL }; //Artificial Virtual Key for a Mouse-Wheel Down event.
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
		SpanByte m_spnData { };               //Main data span.
		HEXCOLORS m_stColors;                 //All HexCtrl colors.
		IHexVirtData* m_pHexVirtData { };     //Data handler pointer for Virtual mode.
		IHexVirtColors* m_pHexVirtColors { }; //Pointer for custom colors class.
		TTTOOLINFOW m_stToolInfoBkm { };      //Tooltip info for Bookmarks.
		TTTOOLINFOW m_stToolInfoTempl { };    //Tooltip info for Templates.
		TTTOOLINFOW m_stToolInfoOffset { };   //Tooltip info for Offset.
		std::chrono::steady_clock::time_point m_tmTT { }; //Start time of the tooltip.
		CWnd m_wndTTBkm { };                  //Tooltip window for bookmarks description.
		CWnd m_wndTTTempl { };                //Tooltip window for Templates' fields.
		CWnd m_wndTTOffset { };               //Tooltip window for Offset in m_fHighLatency mode.
		PHEXBKM m_pBkmTTCurr { };             //Currently shown bookmark's tooltip;
		PCHEXTEMPLFIELD m_pTFieldTTCurr { };  //Currently shown Template field's tooltip;
		CFont m_fontMain;                     //Main Hex chunks font.
		CFont m_fontInfoBar;                  //Font for bottom Info bar.
		CMenu m_menuMain;                     //Main popup menu.
		POINT m_stMenuClickedPt { };          //RMouse coords when clicked.
		CPen m_penLines;                      //Pen for lines.
		CPen m_penDataTempl;                  //Pen for templates' fields (vertical lines).
		float m_flScrollRatio { };            //Ratio for how much to scroll with mouse-wheel.
		ULONGLONG m_ullCaretPos { };          //Current caret position.
		ULONGLONG m_ullCursorNow { };         //The cursor's current clicked pos.
		ULONGLONG m_ullCursorPrev { };        //The cursor's previously clicked pos, used in selection resolutions.
		DWORD m_dwGroupSize { };              //Current data grouping size.
		DWORD m_dwCapacity { };               //How many bytes are displayed in one row.
		DWORD m_dwCapacityBlockSize { m_dwCapacity / 2 }; //Size of the block before a space delimiter.
		DWORD m_dwOffsetDigits { };           //Amount of digits in "Offset", depends on data size set in SetData.
		DWORD m_dwPageSize { 0UL };           //Size of a page to print additional lines between.
		DWORD m_dwCacheSize { };              //Data cache size for VirtualData mode.
		DWORD m_dwDateFormat { 0xFFFFFFFFUL };//Current date format. See https://docs.microsoft.com/en-gb/windows/win32/intl/locale-idate
		DWORD m_dwCharsExtraSpace { };        //Extra space between chars.
		SIZE m_sizeFontMain { 1, 1 };         //Main font letter's size (width, height).
		SIZE m_sizeFontInfo { 1, 1 };         //Info window font letter's size (width, height).
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
		int m_iCodePage { -1 };               //Current code-page for Text area. -1 for default.
		int m_iLOGPIXELSY { };                //GetDeviceCaps(LOGPIXELSY) constant.
		std::wstring m_wstrCapacity { };      //Top Capacity string.
		std::wstring m_wstrInfoBar { };       //Info bar text.
		std::wstring m_wstrPageName { };      //Name of the sector/page.
		std::wstring m_wstrTextTitle { };     //Text area title.
		std::vector<std::unique_ptr<std::vector<UNDO>>> m_vecUndo; //Undo data.
		std::vector<std::unique_ptr<std::vector<UNDO>>> m_vecRedo; //Redo data.
		std::vector < std::unique_ptr < std::remove_pointer<HBITMAP>::type,
			decltype([](const HBITMAP hBmp) { DeleteObject(hBmp); }) >> m_vecHBITMAP { }; //Icons for the Menu.
		std::vector<KEYBIND> m_vecKeyBind { };//Vector of key bindings.
		std::vector<int> m_vecCharsWidth { }; //Vector of chars widths.
		wchar_t m_wchUnprintable { L'.' };    //Replacement char for unprintable characters.
		wchar_t m_wchDateSepar { L'/' };      //Date separator.
		bool m_fCreated { false };            //Is control created or not yet.
		bool m_fDataSet { false };            //Is data set or not.
		bool m_fMutable { };                  //Does control work in Edit or ReadOnly mode.
		bool m_fInfoBar { };                  //Show bottom Info window or not.
		bool m_fCaretHigh { true };           //Caret's High or Low bits position (first or last digit in hex chunk).
		bool m_fCursorTextArea { false };     //Whether last focus was set at ASCII or Hex chunks area.
		bool m_fLMousePressed { false };      //Is left mouse button pressed.
		bool m_fSelectionBlock { false };     //Is selection as block (with Alt) or classic.
		bool m_fOffsetHex { };                //Print offset numbers as Hex or as Decimals.
		bool m_fHighLatency { false };        //Reflects HEXDATA::fHighLatency.
		bool m_fKeyDownAtm { false };         //Whether a key is pressed at the moment.
		bool m_fRedraw { true };              //Should WM_PAINT be handled or not.
		bool m_fScrollLines { false };        //Page scroll in "Screen * m_flScrollRatio" or in lines.
		bool m_fTTHiding { false };           //Flag is set when any Tooltip window is hiding.
	};
}