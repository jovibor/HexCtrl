/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#pragma once
#include "../HexCtrl.h"
#include <afxwin.h>      //MFC core and standard components.
#include <algorithm>
#include <deque>         //std::deque and related.
#include <optional>      //std::optional
#include <string>        //std::wstring and related.
#include <unordered_map> //std::unordered_map and related.

#undef min //Undefining nasty windef.h macros
#undef max

namespace HEXCTRL::INTERNAL
{
	/*********************************
	* Forward declarations.          *
	*********************************/
	class CHexDlgBkmMgr;
	class CHexDlgEncoding;
	class CHexDlgDataInterpret;
	class CHexDlgFillData;
	class CHexDlgOperations;
	class CHexDlgSearch;
	class CHexBookmarks;
	class CHexSelection;
	namespace SCROLLEX { class CScrollEx; };

	/********************************************************************************************
	* EModifyMode - Enum of the data modification mode, used in SMODIFY.                        *
	********************************************************************************************/
	enum class EModifyMode : WORD
	{
		MODIFY_DEFAULT, MODIFY_REPEAT, MODIFY_OPERATION
	};

	/********************************************************************************************
	* EOperMode - Enum of the data operation mode, used in SMODIFY,                             *
	* when SMODIFY::enModifyMode is MODIFY_OPERATION.                                           *
	********************************************************************************************/
	enum class EOperMode : WORD
	{
		OPER_OR = 0x01, OPER_XOR, OPER_AND, OPER_NOT, OPER_SHL, OPER_SHR,
		OPER_ADD, OPER_SUBTRACT, OPER_MULTIPLY, OPER_DIVIDE
	};

	/********************************************************************************************
	* SMODIFY - used to represent data modification parameters.                            *
	* When enModifyMode is set to EModifyMode::MODIFY_DEFAULT, bytes from pData just replace    *
	* corresponding data bytes as is. If enModifyMode is equal to EModifyMode::MODIFY_REPEAT    *
	* then block by block replacement takes place few times.                                    *
	*   For example : if SUM(vecSpan.ullSize) = 9, ullDataSize = 3 and enModifyMode is set to   *
	* EModifyMode::MODIFY_REPEAT, bytes in memory at vecSpan.ullOffset position are             *
	* 123456789, and bytes pointed to by pData are 345, then, after modification, bytes at      *
	* vecSpan.ullOffset will be 345345345. If enModifyMode is equal to                          *
	* EModifyMode::MODIFY_OPERATION then enOperMode comes into play, showing what kind of       *
	* operation must be performed on data.                                                      *
	********************************************************************************************/
	struct SMODIFY
	{
		EModifyMode enModifyMode { EModifyMode::MODIFY_DEFAULT }; //Modify mode.
		EOperMode   enOperMode { };        //Operation mode enum. Used only if enModifyMode == MODIFY_OPERATION.
		std::byte*  pData { };             //Pointer to a data to be set.
		ULONGLONG   ullDataSize { };       //Size of the data pData is pointing to.
		std::vector<HEXSPANSTRUCT> vecSpan { }; //Vector of data offsets and sizes.
		bool        fParentNtfy { true };  //Notify parent about data change or not?
		bool        fRedraw { true };      //Redraw HexCtrl's window or not?
	};

	/********************************************************************************************
	* CHexCtrl class, is an implementation of the IHexCtrl interface.                           *
	********************************************************************************************/
	class CHexCtrl final : public CWnd, public IHexCtrl
	{
	public:
		explicit CHexCtrl();
		ULONGLONG BkmAdd(const HEXBKMSTRUCT& hbs, bool fRedraw)override; //Adds new bookmark.
		void BkmClearAll()override; //Clear all bookmarks.
		[[nodiscard]] auto BkmGetByID(ULONGLONG ullID)->HEXBKMSTRUCT* override; //Get bookmark by ID.
		[[nodiscard]] auto BkmGetByIndex(ULONGLONG ullIndex)->HEXBKMSTRUCT* override; //Get bookmark by Index.
		[[nodiscard]] ULONGLONG BkmGetCount()const override; //Get bookmarks count.
		[[nodiscard]] auto BkmHitTest(ULONGLONG ullOffset)->HEXBKMSTRUCT* override;
		void BkmRemoveByID(ULONGLONG ullID)override;        //Remove bookmark by the given ID.
		void BkmSetVirtual(bool fEnable, IHexVirtBkm* pVirtual)override; //Enable/disable bookmarks virtual mode.
		void ClearData()override;                           //Clears all data from HexCtrl's view (not touching data itself).
		bool Create(const HEXCREATESTRUCT& hcs)override;    //Main initialization method.
		bool CreateDialogCtrl(UINT uCtrlID, HWND hParent)override; //Сreates custom dialog control.
		void Destroy()override;                             //Deleter.
		void ExecuteCmd(EHexCmd enCmd)override;             //Execute a command within the control.
		[[nodiscard]] DWORD GetCapacity()const override;                  //Current capacity.
		[[nodiscard]] ULONGLONG GetCaretPos()const override;              //Cursor position.
		[[nodiscard]] auto GetColors()const->HEXCOLORSSTRUCT override;    //Current colors.
		[[nodiscard]] int GetEncoding()const override;                    //Get current code page ID.
		[[nodiscard]] long GetFontSize()const override;                   //Current font size.
		[[nodiscard]] HMENU GetMenuHandle()const override;                //Context menu handle.
		[[nodiscard]] DWORD GetSectorSize()const override;                //Current sector size.
		[[nodiscard]] auto GetSelection()const->std::vector<HEXSPANSTRUCT> override; //Gets current selection.
		[[nodiscard]] auto GetShowMode()const->EHexShowMode override;     //Retrieves current show mode.
		[[nodiscard]] HWND GetWindowHandle(EHexWnd enWnd)const override;  //Retrieves control's window/dialog handle.
		void GoToOffset(ULONGLONG ullOffset, bool fSelect, ULONGLONG ullSize)override; //Scrolls to given offset.
		[[nodiscard]] auto HitTest(POINT pt, bool fScreen)const->std::optional<HEXHITTESTSTRUCT> override; //HitTest given point.
		[[nodiscard]] bool IsCmdAvail(EHexCmd enCmd)const override;       //Is given Cmd currently available (can be executed)?
		[[nodiscard]] bool IsCreated()const override;       //Shows whether control is created or not.
		[[nodiscard]] bool IsDataSet()const override;       //Shows whether a data was set to the control or not.
		[[nodiscard]] bool IsMutable()const override;       //Is edit mode enabled or not.
		[[nodiscard]] bool IsOffsetAsHex()const override;   //Is "Offset" printed as Hex or as Decimal.
		[[nodiscard]] bool IsOffsetVisible(ULONGLONG ullOffset)const override; //Ensures that given offset is visible.
		void Redraw()override;                              //Redraw the control's window.
		void SetCapacity(DWORD dwCapacity)override;         //Sets the control's current capacity.
		void SetColors(const HEXCOLORSSTRUCT& clr)override; //Sets all the control's colors.
		void SetData(const HEXDATASTRUCT& hds)override;     //Main method for setting data to display (and edit).	
		void SetEncoding(int iCodePage)override;            //Code-page for text area.
		void SetFont(const LOGFONTW* pLogFont)override;     //Sets the control's new font. This font has to be monospaced.
		void SetFontSize(UINT uiSize)override;              //Sets the control's font size.
		void SetMutable(bool fEnable)override;              //Enable or disable edit mode.
		void SetSectorSize(DWORD dwSize, std::wstring_view wstrName)override; //Sets sector/page size and name to draw the line between. override;          //Sets sector/page size to draw the line between.
		void SetSelection(const std::vector<HEXSPANSTRUCT>& vecSel)override; //Sets current selection.
		void SetShowMode(EHexShowMode enShowMode)override;  //Sets current data show mode.
		void SetWheelRatio(double dbRatio)override;         //Sets the ratio for how much to scroll with mouse-wheel.
	private:
		friend class CHexDlgDataInterpret;
		friend class CHexDlgFillData;
		friend class CHexDlgOperations;
		friend class CHexDlgSearch;
		friend class CHexSelection;
		struct SHBITMAP;
		struct SUNDO;
		enum class EClipboard : WORD;
		void AsciiChunkPoint(ULONGLONG ullOffset, int& iCx, int& iCy)const; //Point of Ascii chunk.
		[[nodiscard]] auto BuildDataToDraw(ULONGLONG ullStartLine, int iLines)const->std::tuple<std::wstring, std::wstring>;
		void CalcChunksFromSize(ULONGLONG ullSize, ULONGLONG ullAlign, ULONGLONG& ullSizeChunk, ULONGLONG& ullChunks);
		void CaretMoveDown();
		void CaretMoveLeft();
		void CaretMoveRight();
		void CaretMoveUp();
		void ClearSelHighlight(); //Clear selection highlight.
		void ClipboardCopy(EClipboard enType)const;
		void ClipboardPaste(EClipboard enType);
		[[nodiscard]] auto CopyBase64()const->std::wstring;
		[[nodiscard]] auto CopyCArr()const->std::wstring;
		[[nodiscard]] auto CopyGrepHex()const->std::wstring;
		[[nodiscard]] auto CopyHex()const->std::wstring;
		[[nodiscard]] auto CopyHexFormatted()const->std::wstring;
		[[nodiscard]] auto CopyHexLE()const->std::wstring;
		[[nodiscard]] auto CopyPrintScreen()const->std::wstring;
		[[nodiscard]] auto CopyText()const->std::wstring;
		void DrawWindow(CDC* pDC, CFont* pFont, CFont* pFontInfo)const;
		void DrawOffsets(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, int iLines)const;
		void DrawHexAscii(CDC* pDC, CFont* pFont, int iLines, std::wstring_view wstrHex, std::wstring_view wstrText)const;
		void DrawBookmarks(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, int iLines, std::wstring_view wstrHex, std::wstring_view wstrText)const;
		void DrawCustomColors(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, int iLines, std::wstring_view wstrHex, std::wstring_view wstrText)const;
		void DrawSelection(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, int iLines, std::wstring_view wstrHex, std::wstring_view wstrText)const;
		void DrawSelHighlight(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, int iLines, std::wstring_view wstrHex, std::wstring_view wstrText)const;
		void DrawCursor(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, int iLines, std::wstring_view wstrHex, std::wstring_view wstrText)const;
		void DrawDataInterpret(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, int iLines, std::wstring_view wstrHex, std::wstring_view wstrText)const;
		void DrawSectorLines(CDC* pDC, ULONGLONG ullStartLine, int iLines);
		void FillWithZeros(); //Fill selection with zeros.
		[[nodiscard]] auto GetBottomLine()const->ULONGLONG;      //Returns current bottom line number in view.
		[[nodiscard]] auto GetCacheSize()const->DWORD;           //Returns Virtual/Message mode cache size.
		template<typename T>
		[[nodiscard]] auto GetData(ULONGLONG ullOffset)const->T; //Get T sized data from ullOffset.
		[[nodiscard]] auto GetData(HEXSPANSTRUCT hss)const->std::byte*; //Gets pointer to exact data offset, no matter what mode the control works in.
		[[nodiscard]] auto GetDataMode()const->EHexDataMode;     //Current Data mode.
		[[nodiscard]] auto GetDataSize()const->ULONGLONG;        //Gets m_ullDataSize.
		[[nodiscard]] auto GetMsgWindow()const->HWND;            //Returns pointer to the "Message" window. See HEXDATASTRUCT::pwndMessage.
		void GoToOffset(ULONGLONG ullOffset);                    //Scrolls to given offfset.
		[[nodiscard]] auto GetTopLine()const->ULONGLONG;         //Returns current top line number in view.
		void HexChunkPoint(ULONGLONG ullOffset, int& iCx, int& iCy)const;   //Point of Hex chunk.
		[[nodiscard]] auto HitTest(POINT pt)const->std::optional<HEXHITTESTSTRUCT>; //Is any hex chunk withing given point?
		[[nodiscard]] bool IsCurTextArea()const;                 //Whether last focus was set at Text or Hex chunks area.
		[[nodiscard]] bool IsSectorVisible()const;               //Returns m_fSectorVisible.
		void MakeSelection(ULONGLONG ullClick, ULONGLONG ullStart, ULONGLONG ullSize, ULONGLONG ullLines,
			bool fScroll = true, bool fGoToStart = false);
		void Modify(const SMODIFY& hms);                       //Main routine to modify data, in m_fMutable==true mode.
		void ModifyDefault(const SMODIFY& hms);                //EModifyMode::MODIFY_DEFAULT
		void ModifyOperation(const SMODIFY& hms);              //EModifyMode::MODIFY_OPERATION
		void ModifyRepeat(const SMODIFY& hms);                 //EModifyMode::MODIFY_REPEAT
		void MoveCaret(ULONGLONG ullPos, bool fHighPart);      //Sets the cursor position when in Edit mode.
		void MsgWindowNotify(const HEXNOTIFYSTRUCT& hns)const; //Notify routine used to send messages to Msg window.
		void MsgWindowNotify(UINT uCode)const;                 //Same as above, but only for notification code.
		void OnCaretPosChange(ULONGLONG ullOffset);            //On changing caret position.
		void OnKeyDownCtrl(UINT nChar);  //Key pressed with the Ctrl.
		void OnKeyDownShift(UINT nChar); //Key pressed with the Shift.
		template <typename T>
		void OperData(T* pData, EOperMode eMode, T tDataOper, ULONGLONG ullSizeData); //Immediate operations on pData.
		void ParentNotify(const HEXNOTIFYSTRUCT& hns)const;    //Notify routine used to send messages to Parent window.
		void ParentNotify(UINT uCode)const;                    //Same as above, but only for notification code.
		void Print();                                          //Printing routine.
		void RecalcAll();                                      //Recalcs all inner draw and data related values.
		void RecalcOffsetDigits();                             //How many digits in Offset (depends on Hex or Decimals).
		void RecalcPrint(CDC* pDC, CFont* pFontMain, CFont* pFontInfo, const CRect& rc);   //Recalc routine for printing.
		void RecalcTotalSectors();                             //Updates info about whether sector's lines printable atm or not.
		void RecalcWorkArea(int iHeight, int iWidth);
		void Redo();
		void SelAll();           //Select all.
		void SelAddDown();       //Down Key pressed with the Shift.
		void SelAddLeft();       //Left Key pressed with the Shift.
		void SelAddRight();      //Right Key pressed with the Shift.
		void SelAddUp();         //Up Key pressed with the Shift.
		template<typename T>
		void SetData(ULONGLONG ullOffset, T tData); //Set T sized data tData at ullOffset.
		void SetDataVirtual(std::byte* pData, const HEXSPANSTRUCT& hss); //Sets data (notifies back) in DATA_MSG and DATA_VIRTUAL.
		void SetSelHighlight(const std::vector<HEXSPANSTRUCT>& vecSelHighlight); //Set selection highlight.
		void SnapshotUndo(const std::vector<HEXSPANSTRUCT>& vecSpan); //Takes currently modifiable data snapshot.
		void TtBkmShow(bool fShow, POINT pt = { }); //Tooltip bookmark show/hide.
		void TtOffsetShow(bool fShow);              //Tooltip Offset show/hide.
		void Undo();
		void UpdateInfoText();                      //Updates text in the bottom "info" area according to currently selected data.
		void WstrCapacityFill();                    //Fill m_wstrCapacity according to current m_dwCapacity.
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
		const std::unique_ptr<CHexDlgBkmMgr> m_pDlgBookmarkMgr { std::make_unique<CHexDlgBkmMgr>() }; //Bookmark manager.
		const std::unique_ptr<CHexDlgEncoding> m_pDlgEncoding { std::make_unique<CHexDlgEncoding>() };     //Encoding dialog.
		const std::unique_ptr<CHexDlgDataInterpret> m_pDlgDataInterpret { std::make_unique<CHexDlgDataInterpret>() }; //Data Interpreter.
		const std::unique_ptr<CHexDlgFillData> m_pDlgFillData { std::make_unique<CHexDlgFillData>() };     //"Fill with..." dialog.
		const std::unique_ptr<CHexDlgOperations> m_pDlgOpers { std::make_unique<CHexDlgOperations>() };    //"Operations" dialog.
		const std::unique_ptr<CHexDlgSearch> m_pDlgSearch { std::make_unique<CHexDlgSearch>() };           //"Search..." dialog.
		const std::unique_ptr<CHexBookmarks> m_pBookmarks { std::make_unique<CHexBookmarks>() };           //Bookmarks.
		const std::unique_ptr<CHexSelection> m_pSelection { std::make_unique<CHexSelection>() };           //Selection class.
		const std::unique_ptr<SCROLLEX::CScrollEx> m_pScrollV { std::make_unique<SCROLLEX::CScrollEx>() }; //Vertical scroll bar.
		const std::unique_ptr<SCROLLEX::CScrollEx> m_pScrollH { std::make_unique<SCROLLEX::CScrollEx>() }; //Horizontal scroll bar.
		const int m_iIndentBottomLine { 1 };  //Bottom line indent from window's bottom.
		const int m_iFirstHorizLine { 0 };    //First horizontal line indent.
		const int m_iFirstVertLine { 0 };     //First vertical line indent.
		const DWORD m_dwCapacityMax { 99 };   //Maximum capacity.
		const DWORD m_dwUndoMax { 500 };      //How many Undo states to preserve.
		HEXCOLORSSTRUCT m_stColor;             //All control related colors.
		EHexDataMode m_enDataMode { };        //Control's data mode.
		EHexShowMode m_enShowMode { };        //Current "Show data" mode.
		std::byte* m_pData { };               //Main data pointer. Modifiable in "Edit" mode.
		IHexVirtData* m_pHexVirtData { };     //Data handler pointer for EHexDataMode::DATA_VIRTUAL
		IHexVirtColors* m_pHexVirtColors { }; //Pointer for custom colors class.
		HWND m_hwndMsg { };                   //Window handle the control messages will be sent to.
		CWnd m_wndTtBkm { };                  //Tooltip window for bookmarks description.
		TTTOOLINFOW m_stToolInfoBkm { };      //Tooltip Bookmarks struct.
		CWnd m_wndTtOffset { };               //Tooltip window for Offset in m_fHighLatency mode.
		TTTOOLINFOW m_stToolInfoOffset { };   //Tooltips struct.
		CFont m_fontMain;                     //Main Hex chunks font.
		CFont m_fontInfo;                     //Font for bottom Info rect.
		CMenu m_menuMain;                     //Main popup menu.
		POINT m_stMenuClickedPt { };          //RMouse coords when clicked.
		CPen m_penLines;                      //Pen for lines.
		HEXBKMSTRUCT* m_pBkmCurrTt { };  //Currently shown bookmark's tooltip;
		double m_dbWheelRatio { };            //Ratio for how much to scroll with mouse-wheel.
		ULONGLONG m_ullDataSize { };          //Size of the displayed data in bytes.
		ULONGLONG m_ullLMouseClick { };       //Left mouse button clicked chunk.
		std::optional<ULONGLONG> m_optRMouseClick { }; //Right mouse clicked chunk. Used in bookmarking.
		ULONGLONG m_ullCaretPos { };          //Current caret position.
		ULONGLONG m_ullCurCursor { };         //Current cursor pos, to avoid WM_MOUSEMOVE handle at the same chunk.
		ULONGLONG m_ullTotalSectors { };      //How many "Sectors" in m_ullDataSize.
		DWORD m_dwCapacity { 0x10 };          //How many bytes displayed in one row
		DWORD m_dwCapacityBlockSize { m_dwCapacity / 2 }; //Size of block before space delimiter.
		DWORD m_dwOffsetDigits { };           //Amount of digits in "Offset", depends on data size set in SetData.
		DWORD m_dwOffsetBytes { };            //How many bytes "Offset" number posesses;
		DWORD m_dwSectorSize { 0 };           //Size of a sector to print additional lines between.
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
		std::wstring m_wstrSectorName { };    //Name of the sector/page.
		std::wstring m_wstrTextTitle { };     //Text area title.
		std::deque<std::unique_ptr<std::vector<SUNDO>>> m_deqUndo; //Undo deque.
		std::deque<std::unique_ptr<std::vector<SUNDO>>> m_deqRedo; //Redo deque.
		std::unordered_map<int, SHBITMAP> m_umapHBITMAP;           //Images for the Menu.
		bool m_fCreated { false };            //Is control created or not yet.
		bool m_fDataSet { false };            //Is data set or not.
		bool m_fMutable { false };            //Is control works in Edit or Read mode.
		bool m_fCursorHigh { true };          //Cursor's High or Low bits position (first or last digit in hex chunk).
		bool m_fCursorTextArea { false };     //Whether last focus was set at ASCII or Hex chunks area.
		bool m_fLMousePressed { false };      //Is left mouse button pressed.
		bool m_fSelectionBlock { false };     //Is selection as block (with Alt) or classic.
		bool m_fOffsetAsHex { true };         //Print offset numbers as Hex or as Decimals.
		bool m_fHighLatency { false };        //Reflects HEXDATASTRUCT::fHighLatency.
		bool m_fKeyDownAtm { false };         //Whether some key is down/pressed at the moment.
	};

	template<typename T>
	inline auto CHexCtrl::GetData(ULONGLONG ullOffset)const->T
	{
		if (ullOffset >= m_ullDataSize)
			return T { };

		if (auto pData = GetData({ ullOffset, sizeof(T) }); pData != nullptr)
			return *reinterpret_cast<T*>(pData);

		return T { };
	}

	template<typename T>
	inline void CHexCtrl::SetData(ULONGLONG ullOffset, T tData)
	{
		//Data overflow check.
		if (ullOffset + sizeof(T) > m_ullDataSize)
			return;

		auto pData = GetData({ ullOffset, sizeof(T) });
		std::copy_n(&tData, 1, reinterpret_cast<T*>(pData));
		SetDataVirtual(pData, { ullOffset, sizeof(T) });
	}

	/************************************************************
	* OperData - function for Modify/Operations
	* pData - Starting address of the data to operate on.
	* eMode - Operation mode.
	* tDataOper - The data to apply the operation with.
	* ullSizeData - Size of the data (selection) to operate on.
	************************************************************/
	template<typename T>
	inline void CHexCtrl::OperData(T* pData, EOperMode eMode, T tDataOper, ULONGLONG ullSizeData)
	{
		auto nChunks = ullSizeData / sizeof(T);
		for (auto pDataEnd = pData + nChunks; pData < pDataEnd; ++pData)
		{
			switch (eMode)
			{
			case EOperMode::OPER_OR:
				*pData |= tDataOper;
				break;
			case EOperMode::OPER_XOR:
				*pData ^= tDataOper;
				break;
			case EOperMode::OPER_AND:
				*pData &= tDataOper;
				break;
			case EOperMode::OPER_NOT:
				*pData = ~*pData;
				break;
			case EOperMode::OPER_SHL:
				*pData <<= tDataOper;
				break;
			case EOperMode::OPER_SHR:
				*pData >>= tDataOper;
				break;
			case EOperMode::OPER_ADD:
				*pData += tDataOper;
				break;
			case EOperMode::OPER_SUBTRACT:
				*pData -= tDataOper;
				break;
			case EOperMode::OPER_MULTIPLY:
				*pData *= tDataOper;
				break;
			case EOperMode::OPER_DIVIDE:
				if (tDataOper > 0) //Division by Zero check.
					*pData /= tDataOper;
				break;
			}
		}
	}
}