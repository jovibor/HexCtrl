/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
****************************************************************************************/
#pragma once
#include "../HexCtrl.h"
#include <afxwin.h>      //MFC core and standard components.
#include <algorithm>
#include <deque>
#include <optional>
#include <string>
#include <unordered_map>

namespace HEXCTRL::INTERNAL
{
	/*********************************
	* Forward declarations.          *
	*********************************/
	class CHexDlgBkmMgr;
	class CHexDlgEncoding;
	class CHexDlgDataInterp;
	class CHexDlgFillData;
	class CHexDlgOpers;
	class CHexDlgSearch;
	class CHexDlgGoTo;
	class CHexBookmarks;
	class CHexSelection;
	namespace SCROLLEX { class CScrollEx; };

	/********************************************************************************************
	* CHexCtrl class is an implementation of the IHexCtrl interface.                            *
	********************************************************************************************/
	class CHexCtrl final : public CWnd, public IHexCtrl
	{
	public:
		explicit CHexCtrl();
		ULONGLONG BkmAdd(const HEXBKM& hbs, bool fRedraw)override;              //Adds new bookmark.
		void BkmClearAll()override;                                             //Clear all bookmarks.
		[[nodiscard]] auto BkmGetByID(ULONGLONG ullID)->HEXBKM* override;       //Get bookmark by ID.
		[[nodiscard]] auto BkmGetByIndex(ULONGLONG ullIndex)->HEXBKM* override; //Get bookmark by Index.
		[[nodiscard]] ULONGLONG BkmGetCount()const override;                    //Get bookmarks count.
		[[nodiscard]] auto BkmHitTest(ULONGLONG ullOffset)->HEXBKM* override;   //HitTest for given offset.
		void BkmRemoveByID(ULONGLONG ullID)override;                     //Remove bookmark by the given ID.
		void BkmSetVirtual(bool fEnable, IHexVirtBkm* pVirtual)override; //Enable/disable bookmarks virtual mode.
		void ClearData()override; //Clears all data from HexCtrl's view (not touching data itself).
		bool Create(const HEXCREATE& hcs)override;                        //Main initialization method.
		bool CreateDialogCtrl(UINT uCtrlID, HWND hParent)override;        //Сreates custom dialog control.
		void Destroy()override;                                           //Deleter.
		void ExecuteCmd(EHexCmd eCmd)override;                            //Execute a command within the control.
		[[nodiscard]] auto GetCacheSize()const->DWORD override;           //Returns Virtual/Message mode cache size.
		[[nodiscard]] DWORD GetCapacity()const override;                  //Current capacity.
		[[nodiscard]] ULONGLONG GetCaretPos()const override;              //Cursor position.
		[[nodiscard]] auto GetColors()const->HEXCOLORS override;          //Current colors.
		[[nodiscard]] auto GetData(HEXOFFSET ho)const->std::span<std::byte> override; //Get pointer to data offset, no matter what mode the control works in.
		[[nodiscard]] auto GetDataSize()const->ULONGLONG override;        //Get currently set data size.
		[[nodiscard]] int GetEncoding()const override;                    //Get current code page ID.
		[[nodiscard]] long GetFontSize()const override;                   //Current font size.
		[[nodiscard]] auto GetGroupMode()const->EHexDataSize override;    //Retrieves current data grouping mode.
		[[nodiscard]] HMENU GetMenuHandle()const override;                //Context menu handle.
		[[nodiscard]] auto GetPagesCount()const->ULONGLONG override;      //Get count of pages.
		[[nodiscard]] auto GetPagePos()const->ULONGLONG override;         //Get current page a cursor stays at.
		[[nodiscard]] DWORD GetPageSize()const override;                  //Current page size.
		[[nodiscard]] auto GetSelection()const->std::vector<HEXOFFSET> override; //Gets current selection.
		[[nodiscard]] HWND GetWindowHandle(EHexWnd enWnd)const override;  //Retrieves control's window/dialog handle.
		void GoToOffset(ULONGLONG ullOffset, int iRelPos = 0)override;    //Go (scroll) to a given offset.
		[[nodiscard]] bool HasSelection()const override;    //Does currently have any selection or not.
		[[nodiscard]] auto HitTest(POINT pt, bool fScreen)const->std::optional<HEXHITTEST> override; //HitTest given point.
		[[nodiscard]] bool IsCmdAvail(EHexCmd eCmd)const override;        //Is given Cmd currently available (can be executed)?
		[[nodiscard]] bool IsCreated()const override;       //Shows whether control is created or not.
		[[nodiscard]] bool IsDataSet()const override;       //Shows whether a data was set to the control or not.
		[[nodiscard]] bool IsMutable()const override;       //Is edit mode enabled or not.
		[[nodiscard]] bool IsOffsetAsHex()const override;   //Is "Offset" printed as Hex or as Decimal.
		[[nodiscard]] auto IsOffsetVisible(ULONGLONG ullOffset)const->HEXVISION override; //Ensures that the given offset is visible.
		[[nodiscard]] bool IsVirtual()const override;       //Is working in Virtual or default mode.
		void ModifyData(const HEXMODIFY& hms)override;      //Main routine to modify data in IsMutable()==true mode.
		void Redraw()override;                              //Redraw the control's window.
		void SetCapacity(DWORD dwCapacity)override;         //Set the control's current capacity.
		void SetCaretPos(ULONGLONG ullOffset, bool fHighLow = true, bool fRedraw = true)override; //Set the caret position.
		void SetColors(const HEXCOLORS& clr)override;       //Set all the control's colors.
		bool SetConfig(std::wstring_view wstrPath)override; //Set configuration file, or "" for defaults.
		void SetData(const HEXDATA& hds)override;           //Main method for setting data to display (and edit).	
		void SetEncoding(int iCodePage)override;            //Code-page for text area.
		void SetFont(const LOGFONTW* pLogFont)override;     //Set the control's new font. This font has to be monospaced.
		void SetFontSize(UINT uiSize)override;              //Set the control's font size.
		void SetGroupMode(EHexDataSize enGroupMode)override; //Set current "Group Data By" mode.
		void SetMutable(bool fEnable)override;              //Enable or disable mutable/editable mode.
		void SetOffsetMode(bool fHex)override;              //Set offset being shown as Hex or as Decimal.
		void SetPageSize(DWORD dwSize, std::wstring_view wstrName)override;  //Set page size and name to draw the line between.
		void SetSelection(const std::vector<HEXOFFSET>& vecSel, bool fRedraw = true, bool fHighlight = false)override; //Set current selection.
		void SetWheelRatio(double dbRatio)override;         //Set the ratio for how much to scroll with mouse-wheel.
	private:
		struct SHBITMAP;
		struct SUNDO;
		struct SKEYBIND;
		enum class EClipboard : WORD;
		void AsciiChunkPoint(ULONGLONG ullOffset, int& iCx, int& iCy)const; //Point of Ascii chunk.
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
		void ClipboardCopy(EClipboard enType)const;
		void ClipboardPaste(EClipboard enType);
		[[nodiscard]] auto CopyBase64()const->std::wstring;
		[[nodiscard]] auto CopyCArr()const->std::wstring;
		[[nodiscard]] auto CopyGrepHex()const->std::wstring;
		[[nodiscard]] auto CopyHex()const->std::wstring;
		[[nodiscard]] auto CopyHexFormatted()const->std::wstring;
		[[nodiscard]] auto CopyHexLE()const->std::wstring;
		[[nodiscard]] auto CopyOffset()const->std::wstring;
		[[nodiscard]] auto CopyPrintScreen()const->std::wstring;
		[[nodiscard]] auto CopyText()const->std::wstring;
		void DrawWindow(CDC* pDC, CFont* pFont, CFont* pFontInfo)const;
		void DrawOffsets(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, int iLines)const;
		void DrawHexAscii(CDC* pDC, CFont* pFont, int iLines, std::wstring_view wstrHex, std::wstring_view wstrText)const;
		void DrawBookmarks(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, int iLines, std::wstring_view wstrHex, std::wstring_view wstrText)const;
		void DrawCustomColors(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, int iLines, std::wstring_view wstrHex, std::wstring_view wstrText)const;
		void DrawSelection(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, int iLines, std::wstring_view wstrHex, std::wstring_view wstrText)const;
		void DrawSelHighlight(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, int iLines, std::wstring_view wstrHex, std::wstring_view wstrText)const;
		void DrawCaret(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, int iLines, std::wstring_view wstrHex, std::wstring_view wstrText)const;
		void DrawDataInterp(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, int iLines, std::wstring_view wstrHex, std::wstring_view wstrText)const;
		void DrawPageLines(CDC* pDC, ULONGLONG ullStartLine, int iLines);
		void FillWithZeros(); //Fill selection with zeros.
		[[nodiscard]] auto GetBottomLine()const->ULONGLONG;    //Returns current bottom line number in view.
		[[nodiscard]] auto GetCommand(UCHAR uChar, bool fCtrl, bool fShift, bool fAlt)const->std::optional<EHexCmd>; //Get command from keybinding.
		[[nodiscard]] auto GetTopLine()const->ULONGLONG;       //Returns current top line number in view.
		void HexChunkPoint(ULONGLONG ullOffset, int& iCx, int& iCy)const; //Point of Hex chunk.
		[[nodiscard]] auto HitTest(POINT pt)const->std::optional<HEXHITTEST>; //Is any hex chunk withing given point?
		[[nodiscard]] bool IsCurTextArea()const;               //Whether last focus was set at Text or Hex chunks area.
		[[nodiscard]] bool IsDrawable()const;                  //Should WM_PAINT be handled atm or not.
		[[nodiscard]] bool IsPageVisible()const;               //Returns m_fSectorVisible.
		template<typename T>
		void ModifyWorker(const HEXMODIFY& hms, T& lmbWorker, ULONGLONG ullSizeToOperWith); //Main "modify" method with different workers.
		void OnCaretPosChange(ULONGLONG ullOffset);            //On changing caret position.
		template<typename T>
		void ParentNotify(const T& t)const;                    //Notify routine used to send messages to Parent window.
		void ParentNotify(UINT uCode)const;                    //Same as above, but only for notification code.
		void Print();                                          //Printing routine.
		void RecalcAll();                                      //Recalcs all inner draw and data related values.
		void RecalcOffsetDigits();                             //How many digits in Offset (depends on Hex or Decimals).
		void RecalcPrint(CDC* pDC, CFont* pFontMain, CFont* pFontInfo, const CRect& rc); //Recalc routine for printing.
		void RecalcWorkArea(int iHeight, int iWidth);
		void Redo();
		void ScrollOffsetH(ULONGLONG ullOffset); //Scroll horizontally to given offset.
		void SelAll();           //Select all.
		void SelAddDown();       //Down Key pressed with the Shift.
		void SelAddLeft();       //Left Key pressed with the Shift.
		void SelAddRight();      //Right Key pressed with the Shift.
		void SelAddUp();         //Up Key pressed with the Shift.
		void SetDataVirtual(std::span<std::byte> spnData, const HEXOFFSET& ho); //Sets data (notifies back) in Virtual mode.
		void SetRedraw(bool fRedraw); //Handle WM_PAINT message or not.
		void SnapshotUndo(const std::vector<HEXOFFSET>& vecSpan); //Takes currently modifiable data snapshot.
		void TtBkmShow(bool fShow, POINT pt = { }, bool fTimerCancel = false); //Tooltip bookmark show/hide.
		void TtOffsetShow(bool fShow); //Tooltip Offset show/hide.
		void Undo();
		void WstrCapacityFill();       //Fill m_wstrCapacity according to current m_dwCapacity.
		DECLARE_MESSAGE_MAP()
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
		afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnMouseMove(UINT nFlags, CPoint point);
		afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
		afx_msg BOOL OnNcActivate(BOOL bActive);
		afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
		afx_msg void OnNcPaint();
		afx_msg void OnPaint();
		afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
		afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
		afx_msg void OnSize(UINT nType, int cx, int cy);
		afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void OnTimer(UINT_PTR nIDEvent);
		afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	private:
		const std::unique_ptr<CHexDlgBkmMgr> m_pDlgBkmMgr { std::make_unique<CHexDlgBkmMgr>() };             //"Bookmark manager" dialog.
		const std::unique_ptr<CHexDlgEncoding> m_pDlgEncoding { std::make_unique<CHexDlgEncoding>() };       //"Encoding" dialog.
		const std::unique_ptr<CHexDlgDataInterp> m_pDlgDataInterp { std::make_unique<CHexDlgDataInterp>() }; //"Data interpreter" dialog.
		const std::unique_ptr<CHexDlgFillData> m_pDlgFillData { std::make_unique<CHexDlgFillData>() };       //"Fill with..." dialog.
		const std::unique_ptr<CHexDlgOpers> m_pDlgOpers { std::make_unique<CHexDlgOpers>() };    //"Operations" dialog.
		const std::unique_ptr<CHexDlgSearch> m_pDlgSearch { std::make_unique<CHexDlgSearch>() }; //"Search..." dialog.
		const std::unique_ptr<CHexDlgGoTo> m_pDlgGoTo { std::make_unique<CHexDlgGoTo>() };       //"GoTo..." dialog.
		const std::unique_ptr<CHexBookmarks> m_pBookmarks { std::make_unique<CHexBookmarks>() }; //Bookmarks.
		const std::unique_ptr<CHexSelection> m_pSelection { std::make_unique<CHexSelection>() }; //Selection class.
		const std::unique_ptr<SCROLLEX::CScrollEx> m_pScrollV { std::make_unique<SCROLLEX::CScrollEx>() }; //Vertical scroll bar.
		const std::unique_ptr<SCROLLEX::CScrollEx> m_pScrollH { std::make_unique<SCROLLEX::CScrollEx>() }; //Horizontal scroll bar.
		const int m_iIndentBottomLine { 1 };  //Bottom line indent from window's bottom.
		const int m_iFirstHorizLine { 0 };    //First horizontal line indent.
		const int m_iFirstVertLine { 0 };     //First vertical line indent.
		std::span<std::byte> m_spnData { };   //Main data span.
		HEXCOLORS m_stColor;                  //All control related colors.
		EHexDataSize m_enGroupMode { EHexDataSize::SIZE_BYTE }; //Current "Group Data By" mode.
		IHexVirtData* m_pHexVirtData { };     //Data handler pointer for Virtual mode.
		IHexVirtColors* m_pHexVirtColors { }; //Pointer for custom colors class.
		CWnd m_wndTtBkm { };                  //Tooltip window for bookmarks description.
		TTTOOLINFOW m_stToolInfoBkm { };      //Tooltip Bookmarks struct.
		std::time_t m_tmBkmTt { };            //A beginning time to calc the diff, for hiding bkm tooltip after.
		CWnd m_wndTtOffset { };               //Tooltip window for Offset in m_fHighLatency mode.
		TTTOOLINFOW m_stToolInfoOffset { };   //Tooltips struct.
		CFont m_fontMain;                     //Main Hex chunks font.
		CFont m_fontInfo;                     //Font for bottom Info rect.
		CMenu m_menuMain;                     //Main popup menu.
		POINT m_stMenuClickedPt { };          //RMouse coords when clicked.
		CPen m_penLines;                      //Pen for lines.
		HEXBKM* m_pBkmTtCurr { };       //Currently shown bookmark's tooltip;
		double m_dbWheelRatio { };            //Ratio for how much to scroll with mouse-wheel.
		std::optional<ULONGLONG> m_optRMouseClick { }; //Right mouse clicked chunk. Used in bookmarking.
		ULONGLONG m_ullCaretPos { };          //Current caret position.
		ULONGLONG m_ullCursorNow { };         //The cursor's current clicked pos.
		ULONGLONG m_ullCursorPrev { };        //The cursor's previously clicked pos, used in selection resolutions.
		DWORD m_dwCapacity { 0x10 };          //How many bytes displayed in one row
		DWORD m_dwCapacityBlockSize { m_dwCapacity / 2 }; //Size of block before space delimiter.
		DWORD m_dwOffsetDigits { };           //Amount of digits in "Offset", depends on data size set in SetData.
		DWORD m_dwOffsetBytes { };            //How many bytes "Offset" number posesses;
		DWORD m_dwPageSize { 0 };             //Size of a page to print additional lines between.
		DWORD m_dwCacheSize { };              //Cache size for virtual and message modes, set in SetData.
		SIZE m_sizeLetter { 1, 1 };           //Current font's letter size (width, height).
		long m_lFontSize { };                 //Current font size.
		int m_iSizeFirstHalf { };             //Size in px of the first half of the capacity.
		int m_iSizeHexByte { };               //Size in px of two hex letters representing one byte.
		int m_iIndentAscii { };               //Indent in px of Ascii text begining.
		int m_iIndentFirstHexChunk { };       //First hex chunk indent in px.
		int m_iIndentTextCapacityY { };       //Caption text (0 1 2... D E F...) vertical offset.
		int m_iDistanceBetweenHexChunks { };  //Distance between begining of the two hex chunks in px.
		int m_iSpaceBetweenHexChunks { };     //Space between Hex chunks in px.
		int m_iSpaceBetweenAscii { };         //Space between beginning of the two Ascii chars in px.
		int m_iSpaceBetweenBlocks { };        //Additional space between hex chunks after half of capacity, in px.
		int m_iHeightClientArea { };          //Height of the Control's window client area.
		int m_iWidthClientArea { };           //Width of the Control's window client area.
		int m_iHeightTopRect { };             //Height of the header where offsets (0 1 2... D E F...) reside.
		int m_iStartWorkAreaY { };            //Start Y of the area where all drawing occurs.
		int m_iEndWorkArea { };               //End of the area where all drawing occurs.
		int m_iHeightWorkArea { };            //Height in px of the working area where all drawing occurs.
		int m_iHeightBottomRect { };          //Height of bottom Info rect.
		int m_iHeightBottomOffArea { };       //Height of the not visible rect from window's bottom to m_iThirdHorizLine.
		int m_iSecondVertLine { }, m_iThirdVertLine { }, m_iFourthVertLine { }; //Vertical lines indent.
		int m_iCodePage { -1 };               //Current code-page for Text area. -1 for default.
		std::wstring m_wstrCapacity { };      //Top Capacity string.
		std::wstring m_wstrInfo { };          //Info text (bottom rect).
		std::wstring m_wstrPageName { };      //Name of the sector/page.
		std::wstring m_wstrTextTitle { };     //Text area title.
		std::deque<std::unique_ptr<std::vector<SUNDO>>> m_deqUndo; //Undo deque.
		std::deque<std::unique_ptr<std::vector<SUNDO>>> m_deqRedo; //Redo deque.
		std::unordered_map<int, SHBITMAP> m_umapHBITMAP;           //Images for the Menu.
		std::vector<SKEYBIND> m_vecKeyBind { }; //Vector of key bindings.
		bool m_fCreated { false };            //Is control created or not yet.
		bool m_fDataSet { false };            //Is data set or not.
		bool m_fMutable { false };            //Is control works in Edit or Read mode.
		bool m_fCaretHigh { true };           //Caret's High or Low bits position (first or last digit in hex chunk).
		bool m_fCursorTextArea { false };     //Whether last focus was set at ASCII or Hex chunks area.
		bool m_fLMousePressed { false };      //Is left mouse button pressed.
		bool m_fSelectionBlock { false };     //Is selection as block (with Alt) or classic.
		bool m_fOffsetAsHex { true };         //Print offset numbers as Hex or as Decimals.
		bool m_fHighLatency { false };        //Reflects HEXDATA::fHighLatency.
		bool m_fKeyDownAtm { false };         //Whether some key is down/pressed at the moment.
		bool m_fMenuCMD { false };            //Command to be executed through menu, not through key-shortcut.
		bool m_fRedraw { true };            //Should WM_PAINT be handled or not.
	};
}