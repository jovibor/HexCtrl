/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../HexCtrl.h"
#include <afxwin.h>      //MFC core and standard components.
#include <algorithm>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace HEXCTRL::INTERNAL
{
	//Forward declarations.
	class CHexDlgBkmMgr;
	class CHexDlgCodepage;
	class CHexDlgDataInterp;
	class CHexDlgModify;
	class CHexDlgGoTo;
	class CHexDlgSearch;
	class CHexDlgTemplMgr;
	class CHexSelection;
	struct HEXTEMPLATEFIELD;
	namespace SCROLLEX { class CScrollEx; };

	/********************************************************************************************
	* CHexCtrl class is an implementation of the IHexCtrl interface.                            *
	********************************************************************************************/
	class CHexCtrl final : public CWnd, public IHexCtrl {
	public:
		explicit CHexCtrl();
		CHexCtrl(const CHexCtrl&) = delete;
		CHexCtrl(CHexCtrl&&) = delete;
		CHexCtrl& operator=(const CHexCtrl&) = delete;
		CHexCtrl& operator=(CHexCtrl&&) = delete;
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
		[[nodiscard]] auto GetCodepage()const->int override;
		[[nodiscard]] auto GetColors()const->HEXCOLORS override;
		[[nodiscard]] auto GetData(HEXSPAN hss)const->SpanByte override;
		[[nodiscard]] auto GetDataSize()const->ULONGLONG override;
		[[nodiscard]] auto GetDateInfo()const->std::tuple<DWORD, wchar_t> override;
		[[nodiscard]] auto GetDlgData(EHexWnd eWnd)const->std::uint64_t override;
		[[nodiscard]] auto GetFont() -> LOGFONTW override;
		[[nodiscard]] auto GetGroupMode()const->EHexDataSize override;
		[[nodiscard]] auto GetMenuHandle()const->HMENU override;
		[[nodiscard]] auto GetPagesCount()const->ULONGLONG override;
		[[nodiscard]] auto GetPagePos()const->ULONGLONG override;
		[[nodiscard]] auto GetPageSize()const->DWORD override;
		[[nodiscard]] auto GetSelection()const->VecSpan override;
		[[nodiscard]] auto GetTemplates()const->IHexTemplates* override;
		[[nodiscard]] auto GetUnprintableChar()const->wchar_t override;
		[[nodiscard]] auto GetWindowHandle(EHexWnd eWnd)const->HWND override;
		void GoToOffset(ULONGLONG ullOffset, int iRelPos = 0)override;
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
		void Redraw()override;
		void SetCapacity(DWORD dwCapacity)override;
		void SetCaretPos(ULONGLONG ullOffset, bool fHighLow = true, bool fRedraw = true)override;
		void SetCodepage(int iCodepage)override;
		void SetColors(const HEXCOLORS& clr)override;
		bool SetConfig(std::wstring_view wsvPath)override;
		void SetData(const HEXDATA& hds)override;
		void SetDateInfo(DWORD dwFormat, wchar_t wchSepar)override;
		auto SetDlgData(EHexWnd eWnd, std::uint64_t ullData) -> HWND override;
		void SetFont(const LOGFONTW& lf)override;
		void SetGroupMode(EHexDataSize eGroupMode)override;
		void SetMutable(bool fEnable)override;
		void SetOffsetMode(bool fHex)override;
		void SetPageSize(DWORD dwSize, std::wstring_view wsvName)override;
		void SetVirtualBkm(IHexBookmarks* pVirtBkm)override;
		void SetRedraw(bool fRedraw)override;
		void SetSelection(const VecSpan& vecSel, bool fRedraw = true, bool fHighlight = false)override;
		void SetUnprintableChar(wchar_t wch)override;
		void SetWheelRatio(double dbRatio, bool fLines)override;
		void ShowInfoBar(bool fShow)override;
	private:
		struct SUNDO;
		struct SKEYBIND;
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
		void DrawHexText(CDC* pDC, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const;
		void DrawTemplates(CDC* pDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const;
		void DrawBookmarks(CDC* pDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const;
		void DrawCustomColors(CDC* pDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const;
		void DrawSelection(CDC* pDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const;
		void DrawSelHighlight(CDC* pDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const;
		void DrawCaret(CDC* pDC, ULONGLONG ullStartLine, std::wstring_view wsvHex, std::wstring_view wsvText)const;
		void DrawDataInterp(CDC* pDC, ULONGLONG ullStartLine, int iLines, std::wstring_view wsvHex, std::wstring_view wsvText)const;
		void DrawPageLines(CDC* pDC, ULONGLONG ullStartLine, int iLines);
		void FillCapacityString();  //Fill m_wstrCapacity according to current m_dwCapacity.
		void FillWithZeros();       //Fill selection with zeros.
		void FontSizeIncDec(bool fInc = true); //Increase os decrease font size by minimum amount.
		[[nodiscard]] auto GetBottomLine()const->ULONGLONG;    //Returns current bottom line number in view.
		[[nodiscard]] auto GetCommand(UINT uKey, bool fCtrl, bool fShift, bool fAlt)const->std::optional<EHexCmd>; //Get command from keybinding.
		[[nodiscard]] long GetFontSize();
		[[nodiscard]] CRect GetRectTextCaption()const;         //Returns rect of the text caption area.
		[[nodiscard]] auto GetTopLine()const->ULONGLONG;       //Returns current top line number in view.
		void HexChunkPoint(ULONGLONG ullOffset, int& iCx, int& iCy)const; //Point of Hex chunk.
		[[nodiscard]] auto HitTest(POINT pt)const->std::optional<HEXHITTEST>; //Is any hex chunk withing given point?
		[[nodiscard]] bool IsCurTextArea()const;               //Whether last focus was set at Text or Hex chunks area.
		[[nodiscard]] bool IsDrawable()const;                  //Should WM_PAINT be handled atm or not.
		[[nodiscard]] bool IsInfoBar()const;                   //Should bottom Info rect be painted or not.
		[[nodiscard]] bool IsPageVisible()const;               //Returns m_fSectorVisible.
		template<typename T>
		void ModifyWorker(const HEXMODIFY& hms, const T& lmbWorker, SpanCByte spnDataToOperWith); //Main "modify" method with different workers.
		void OffsetToString(ULONGLONG ullOffset, wchar_t* buffOut)const; //Format offset to wchar_t string.
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
		void SetDataVirtual(SpanByte spnData, const HEXSPAN& hss)const; //Sets data (notifies back) in Virtual mode.
		void SetFontSize(long lSize); //Set current font size.
		void SnapshotUndo(const VecSpan& vecSpan); //Takes currently modifiable data snapshot.
		void TextChunkPoint(ULONGLONG ullOffset, int& iCx, int& iCy)const;     //Point of the text chunk.
		void ToolTipBkmShow(bool fShow, POINT pt = { }, bool fTimerCancel = false); //Tooltip for bookmark show/hide.
		void ToolTipTemplShow(bool fShow, POINT pt = { }, bool fTimerCancel = false); //Tooltip for templates show/hide.
		void ToolTipOffsetShow(bool fShow); //Tooltip Offset show/hide.
		void Undo();
		DECLARE_MESSAGE_MAP();
		afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
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
	private:
		static constexpr auto m_pwszHexChars { L"0123456789ABCDEF" }; //Hex digits wchars for fast lookup.
		static constexpr auto m_pwszClassName { L"HexCtrl" }; //HexControl Class name.
		static constexpr auto m_uiIDTTBkm { 0x01UL };    //Tooltip ID for Bookmarks.
		static constexpr auto m_uiIDTTTempl { 0x03UL };  //Tooltip ID for Templates.
		static constexpr auto m_iIndentBottomLine { 1 }; //Bottom line indent from window's bottom.
		static constexpr auto m_iFirstHorzLine { 0 };    //First horizontal line indent.
		static constexpr auto m_iFirstVertLine { 0 };    //First vertical line indent.
		const std::unique_ptr<CHexDlgBkmMgr> m_pDlgBkmMgr { std::make_unique<CHexDlgBkmMgr>() };             //"Bookmark manager" dialog.
		const std::unique_ptr<CHexDlgCodepage> m_pDlgCodepage { std::make_unique<CHexDlgCodepage>() };       //"Codepage" dialog.
		const std::unique_ptr<CHexDlgDataInterp> m_pDlgDataInterp { std::make_unique<CHexDlgDataInterp>() }; //"Data interpreter" dialog.
		const std::unique_ptr<CHexDlgModify> m_pDlgModify { std::make_unique<CHexDlgModify>() };             //"Modify..." dialog.
		const std::unique_ptr<CHexDlgGoTo> m_pDlgGoTo { std::make_unique<CHexDlgGoTo>() };                   //"GoTo..." dialog.
		const std::unique_ptr<CHexDlgSearch> m_pDlgSearch { std::make_unique<CHexDlgSearch>() };             //"Search..." dialog.
		const std::unique_ptr<CHexDlgTemplMgr> m_pDlgTemplMgr { std::make_unique<CHexDlgTemplMgr>() };       //"Template manager..." dialog.
		const std::unique_ptr<CHexSelection> m_pSelection { std::make_unique<CHexSelection>() };             //Selection class.
		const std::unique_ptr<SCROLLEX::CScrollEx> m_pScrollV { std::make_unique<SCROLLEX::CScrollEx>() };   //Vertical scroll bar.
		const std::unique_ptr<SCROLLEX::CScrollEx> m_pScrollH { std::make_unique<SCROLLEX::CScrollEx>() };   //Horizontal scroll bar.
		SpanByte m_spnData { };               //Main data span.
		HEXCOLORS m_stColor;                  //All control related colors.
		EHexDataSize m_enGroupMode { EHexDataSize::SIZE_BYTE }; //Current "Group Data By" mode.
		IHexVirtData* m_pHexVirtData { };     //Data handler pointer for Virtual mode.
		IHexVirtColors* m_pHexVirtColors { }; //Pointer for custom colors class.
		CWnd m_wndTtBkm { };                  //Tooltip window for bookmarks description.
		TTTOOLINFOW m_stToolInfoBkm { };      //Tooltip info for Bookmarks.
		std::time_t m_tmTtBkm { };            //Time beginning to calc the diff for hiding bkm tooltip after.
		PHEXBKM m_pBkmTtCurr { };             //Currently shown bookmark's tooltip;
		CWnd m_wndTtTempl { };                //Tooltip window for Templates' fields.
		TTTOOLINFOW m_stToolInfoTempl { };    //Tooltip info for Templates.
		std::time_t m_tmTtTempl { };          //Time beginning to calc the diff for hiding template tooltip after.
		HEXTEMPLATEFIELD* m_pTFieldTtCurr { };//Currently shown Template field's tooltip;
		CWnd m_wndTtOffset { };               //Tooltip window for Offset in m_fHighLatency mode.
		TTTOOLINFOW m_stToolInfoOffset { };   //Tooltip info for Offset.
		CFont m_fontMain;                     //Main Hex chunks font.
		CFont m_fontInfoBar;                  //Font for bottom Info bar.
		CMenu m_menuMain;                     //Main popup menu.
		POINT m_stMenuClickedPt { };          //RMouse coords when clicked.
		CPen m_penLines;                      //Pen for lines.
		CPen m_penDataTempl;                  //Pen for templates' fields (vertical lines).
		double m_dbWheelRatio { };            //Ratio for how much to scroll with mouse-wheel.
		ULONGLONG m_ullCaretPos { };          //Current caret position.
		ULONGLONG m_ullCursorNow { };         //The cursor's current clicked pos.
		ULONGLONG m_ullCursorPrev { };        //The cursor's previously clicked pos, used in selection resolutions.
		DWORD m_dwCapacity { 0x10 };          //How many bytes displayed in one row
		DWORD m_dwCapacityBlockSize { m_dwCapacity / 2 }; //Size of the block before a space delimiter.
		DWORD m_dwOffsetDigits { };           //Amount of digits in "Offset", depends on data size set in SetData.
		DWORD m_dwPageSize { 0 };             //Size of a page to print additional lines between.
		DWORD m_dwCacheSize { };              //Cache size for virtual and message modes, set in SetData.
		DWORD m_dwDateFormat { 0xFFFFFFFF };  //Current date format. See https://docs.microsoft.com/en-gb/windows/win32/intl/locale-idate
		SIZE m_sizeFontMain { 1, 1 };         //Main font letter's size (width, height).
		SIZE m_sizeFontInfo { 1, 1 };         //Info window font letter's size (width, height).
		int m_iSizeFirstHalf { };             //Size in px of the first half of the capacity.
		int m_iSizeHexByte { };               //Size in px of two hex letters representing one byte.
		int m_iIndentTextX { };               //Indent in px of the text (ASCII) beginning.
		int m_iIndentFirstHexChunkX { };      //First hex chunk indent in px.
		int m_iIndentCapTextY { };            //Caption text (0 1 2... D E F...) vertical offset.
		int m_iDistanceBetweenHexChunks { };  //Distance between begining of the two hex chunks in px.
		int m_iSpaceBetweenHexChunks { };     //Space between Hex chunks in px.
		int m_iDistanceBetweenChars { };      //Distance between beginning of the two text chars in px.
		int m_iSpaceBetweenBlocks { };        //Additional space between hex chunks after half of capacity, in px.
		int m_iWidthClientArea { };           //Width of the Control's window client area.
		int m_iStartWorkAreaY { };            //Start Y of the area where all drawing occurs.
		int m_iEndWorkArea { };               //End of the area where all drawing occurs.
		int m_iHeightClientArea { };          //Height of the Control's window client area.
		int m_iHeightTopRect { };             //Height of the header where offsets (0 1 2... D E F...) reside.
		int m_iHeightWorkArea { };            //Height in px of the working area where all drawing occurs.
		int m_iHeightInfoBar { };             //Height of the bottom Info rect.
		int m_iHeightBottomOffArea { };       //Height of the not visible rect from window's bottom to m_iThirdHorizLine.
		int m_iSecondHorzLine { };            //Second horizontal line indent.
		int m_iThirdHorzLine { };             //Third horizontal line indent.
		int m_iFourthHorzLine { };            //Fourth horizontal line indent.
		int m_iSecondVertLine { };            //Second vert line indent.
		int m_iThirdVertLine { };             //Third vert line indent.
		int m_iFourthVertLine { };            //Fourth vert line indent.
		int m_iCodePage { -1 };               //Current code-page for Text area. -1 for default.
		int m_iLOGPIXELSY { };                //GetDeviceCaps(LOGPIXELSY) constant.
		std::wstring m_wstrCapacity { };      //Top Capacity string.
		std::wstring m_wstrInfoBar { };       //Info bar text.
		std::wstring m_wstrPageName { };      //Name of the sector/page.
		std::wstring m_wstrTextTitle { };     //Text area title.
		std::vector<std::unique_ptr<std::vector<SUNDO>>> m_vecUndo; //Undo data.
		std::vector<std::unique_ptr<std::vector<SUNDO>>> m_vecRedo; //Redo data.
		std::vector < std::unique_ptr < std::remove_pointer<HBITMAP>::type,
			decltype([](const HBITMAP hBmp) { DeleteObject(hBmp); }) >> m_vecHBITMAP { }; //Icons for the Menu.
		std::vector<SKEYBIND> m_vecKeyBind { }; //Vector of key bindings.
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
		bool m_fOffsetAsHex { true };         //Print offset numbers as Hex or as Decimals.
		bool m_fHighLatency { false };        //Reflects HEXDATA::fHighLatency.
		bool m_fKeyDownAtm { false };         //Whether some key is down/pressed at the moment.
		bool m_fRedraw { true };              //Should WM_PAINT be handled or not.
		bool m_fPageLines { false };          //Page scroll in screens * m_dbWheelRatio or in lines. 
	};
}