/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#pragma once
#include <unordered_map> //std::unordered_map and related.
#include <deque>         //std::deque and related.
#include <string>        //std::wstring and related.
#include <optional>      ///std::optional
#include <afxwin.h>      //MFC core and standard components.
#include "../HexCtrl.h"

namespace HEXCTRL::INTERNAL
{
	/*********************************
	* Forward declarations.          *
	*********************************/
	class CHexDlgBookmarkMgr;
	class CHexDlgDataInterpret;
	class CHexDlgFillWith;
	class CHexDlgOperations;
	class CHexDlgSearch;
	class CHexBookmarks;
	class CHexSelection;
	struct UNDOSTRUCT;
	struct HBITMAPSTRUCT;
	struct HITTESTSTRUCT;
	enum class EClipboard : DWORD;
	namespace SCROLLEX { class CScrollEx; }

	/********************************************************************************************
	* EModifyMode - Enum of the data modification mode, used in HEXMODIFYSTRUCT.             *
	********************************************************************************************/
	enum class EModifyMode : WORD
	{
		MODIFY_DEFAULT, MODIFY_REPEAT, MODIFY_OPERATION
	};

	/********************************************************************************************
	* EOperMode - Enum of the data operation mode, used in HEXMODIFYSTRUCT,                  *
	* when HEXMODIFYSTRUCT::enModifyMode is MODIFY_OPERATION.                                   *
	********************************************************************************************/
	enum class EOperMode : WORD
	{
		OPER_OR = 0x01, OPER_XOR, OPER_AND, OPER_NOT, OPER_SHL, OPER_SHR,
		OPER_ADD, OPER_SUBTRACT, OPER_MULTIPLY, OPER_DIVIDE
	};

	/********************************************************************************************
	* HEXMODIFYSTRUCT - used to represent data modification parameters.                         *
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
	struct HEXMODIFYSTRUCT
	{
		EModifyMode      enModifyMode { EModifyMode::MODIFY_DEFAULT }; //Modify mode.
		EOperMode        enOperMode { };        //Operation mode enum. Used only if enModifyMode == MODIFY_OPERATION.
		const std::byte* pData { };             //Pointer to a data to be set.
		ULONGLONG        ullDataSize { };       //Size of the data pData is pointing to.
		std::vector<HEXSPANSTRUCT> vecSpan { }; //Vector of data offsets and sizes.
	};

	/********************************************************************************************
	* CHexCtrl class declaration.																*
	********************************************************************************************/
	class CHexCtrl final : public CWnd, public IHexCtrl
	{
	public:
		explicit CHexCtrl();
		~CHexCtrl() = default;
		DWORD AddBookmark(const HEXBOOKMARKSTRUCT& hbs)override; //Adds new bookmark.
		void ClearData()override;                           //Clears all data from HexCtrl's view (not touching data itself).
		bool Create(const HEXCREATESTRUCT& hcs)override;    //Main initialization method.
		bool CreateDialogCtrl(UINT uCtrlID, HWND hwndDlg)override; //Сreates custom dialog control.
		void Destroy()override;                             //Deleter.
		void ExecuteCmd(EHexCmd enCmd)const override;       //Execute a command within the control.
		[[nodiscard]] DWORD GetCapacity()const override;                  //Current capacity.
		[[nodiscard]] ULONGLONG GetCaretPos()const override;              //Cursor position.
		[[nodiscard]] auto GetColor()const->HEXCOLORSTRUCT override;      //Current colors.
		[[nodiscard]] long GetFontSize()const override;                   //Current font size.
		[[nodiscard]] HMENU GetMenuHandle()const override;                //Context menu handle.
		[[nodiscard]] auto GetSelection()const->std::vector<HEXSPANSTRUCT> override; //Gets current selection.
		[[nodiscard]] auto GetShowMode()const->EHexShowMode override;     //Retrieves current show mode.
		[[nodiscard]] HWND GetWindowHandle()const override;               //Retrieves control's window handle.
		void GoToOffset(ULONGLONG ullOffset, bool fSelect, ULONGLONG ullSize) override; //Scrolls to given offset.
		[[nodiscard]] bool IsCmdAvail(EHexCmd enCmd)const override;       //Is given Cmd currently available (can be executed)?
		[[nodiscard]] bool IsCreated()const override;       //Shows whether control is created or not.
		[[nodiscard]] bool IsDataSet()const override;       //Shows whether a data was set to the control or not.
		[[nodiscard]] bool IsMutable()const override;       //Is edit mode enabled or not.
		[[nodiscard]] bool IsOffsetAsHex()const override;   //Is "Offset" printed as Hex or as Decimal.
		void Print()override;                               //Printing routine.
		void RemoveBookmark(DWORD dwId)override;            //Removes bookmark by the given Id.
		void SetCapacity(DWORD dwCapacity)override;         //Sets the control's current capacity.
		void SetColor(const HEXCOLORSTRUCT& clr)override;   //Sets all the control's colors.
		void SetData(const HEXDATASTRUCT& hds)override;     //Main method for setting data to display (and edit).	
		void SetFont(const LOGFONTW* pLogFontNew)override;  //Sets the control's new font. This font has to be monospaced.
		void SetFontSize(UINT uiSize)override;              //Sets the control's font size.
		void SetMutable(bool fEnable)override;              //Enable or disable edit mode.
		void SetSectorSize(DWORD dwSize, const wchar_t* wstrName)override; //Sets sector/page size and name to draw the line between. override;          //Sets sector/page size to draw the line between.
		void SetSelection(ULONGLONG ullOffset, ULONGLONG ullSize)override; //Sets current selection.
		void SetShowMode(EHexShowMode enShowMode)override;  //Sets current data show mode.
		void SetWheelRatio(double dbRatio)override;         //Sets the ratio for how much to scroll with mouse-wheel.
	protected:
		DECLARE_MESSAGE_MAP()
		afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
		afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
		afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnMouseMove(UINT nFlags, CPoint point);
		afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
		afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
		BOOL OnCommand(WPARAM wParam, LPARAM lParam)override;
		afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
		afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
		afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
		afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
		afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
		void OnKeyDownCtrl(UINT nChar);  //Key pressed with the Ctrl.
		void OnKeyDownShift(UINT nChar); //Key pressed with the Shift.
		void OnKeyDownShiftLeft();       //Left Key pressed with the Shift.
		void OnKeyDownShiftRight();      //Right Key pressed with the Shift.
		void OnKeyDownShiftUp();         //Up Key pressed with the Shift.
		void OnKeyDownShiftDown();       //Down Key pressed with the Shift.
		afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg UINT OnGetDlgCode();     //To properly work in dialogs.
		afx_msg BOOL OnEraseBkgnd(CDC* pDC);
		afx_msg void OnPaint();
		void DrawWindow(CDC* pDC, CFont* pFont, CFont* pFontInfo);
		void DrawOffsets(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, ULONGLONG ullEndLine);
		void DrawHexAscii(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, ULONGLONG ullEndLine);
		void DrawBookmarks(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, ULONGLONG ullEndLine);
		void DrawSelection(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, ULONGLONG ullEndLine);
		void DrawCursor(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, ULONGLONG ullEndLine);
		void DrawDataInterpret(CDC* pDC, CFont* pFont, ULONGLONG ullStartLine, ULONGLONG ullEndLine);
		void DrawSectorLines(CDC* pDC, ULONGLONG ullStartLine, ULONGLONG ullEndLine);
		afx_msg void OnSize(UINT nType, int cx, int cy);
		afx_msg void OnDestroy();
		afx_msg BOOL OnNcActivate(BOOL bActive);
		afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
		afx_msg void OnNcPaint();
	public:
		[[nodiscard]] PBYTE GetDataPtr(const HEXSPANSTRUCT& hss); //Gets pointer to exact data offset, no matter what mode the control works in.
		[[nodiscard]] PBYTE GetDataPtr();                      //Gets m_pData.
		[[nodiscard]] ULONGLONG GetDataSize();                 //Gets m_ullDataSize.
		template<typename T> [[nodiscard]] auto GetData(ULONGLONG ullOffset)->T; //Get T sized data from ullOffset.
		template<typename T>void SetData(ULONGLONG ullOffset, T tData); //Set T sized data tData at ullOffset.
		void ModifyData(HEXMODIFYSTRUCT& hms, bool fRedraw = true);     //Main routine to modify data, in m_fMutable==true mode.
		[[nodiscard]] HWND GetMsgWindow()const;                //Returns pointer to the "Message" window. See HEXDATASTRUCT::pwndMessage.
		void RecalcAll();                                      //Recalcs all inner draw and data related values.
		void RecalcPrint(CDC* pDC, CFont* pFontMain, CFont* pFontInfo, const CRect& rc);   //Recalc routine for printing.
		void RecalcWorkArea(int iHeight, int iWidth);
		void RecalcOffsetDigits();                             //How many digits in Offset (depends on Hex or Decimals).
		[[nodiscard]] ULONGLONG GetTopLine()const;             //Returns current top line number in view.
		[[nodiscard]] ULONGLONG GetBottomLine()const;          //Returns current bottom line number in view.
		[[nodiscard]] auto HitTest(const POINT& pt)const->std::optional<HITTESTSTRUCT>; //Is any hex chunk withing given point?
		void HexChunkPoint(ULONGLONG ullOffset, int& iCx, int& iCy)const;   //Point of Hex chunk.
		void AsciiChunkPoint(ULONGLONG ullOffset, int& iCx, int& iCy)const; //Point of Ascii chunk.
		void ClipboardCopy(EClipboard enType);
		void ClipboardPaste(EClipboard enType);
		void UpdateInfoText();                                 //Updates text in the bottom "info" area according to currently selected data.
		void ParentNotify(const HEXNOTIFYSTRUCT& hns)const;    //Notify routine used to send messages to Parent window.
		void ParentNotify(UINT uCode)const;                    //Same as above, but only for notification code.
		void MsgWindowNotify(const HEXNOTIFYSTRUCT& hns)const; //Notify routine used to send messages to Msg window.
		void MsgWindowNotify(UINT uCode)const;                 //Same as above, but only for notification code.
		void SetCaretPos(ULONGLONG ullPos, bool fHighPart);    //Sets the cursor position when in Edit mode.
		void OnCaretPosChange(ULONGLONG ullOffset);            //On changing caret position.
		void CaretMoveRight();
		void CaretMoveLeft();
		void CaretMoveUp();
		void CaretMoveDown();
		void Undo();
		void Redo();
		void SnapshotUndo(const std::vector<HEXSPANSTRUCT>& vecSpan); //Takes currently modifiable data snapshot.
		[[nodiscard]] bool IsCurTextArea()const;                  //Whether click was made in Text or Hex area.
		void SetSelection(ULONGLONG ullClick, ULONGLONG ullStart, ULONGLONG ullSize, ULONGLONG ullLines,
			bool fScroll = true, bool fGoToStart = false);
		void GoToOffset(ULONGLONG ullOffset);                     //Scrolls to given offfset.
		void SelectAll();                                         //Selects all current bytes.
		void FillWithZeros();                                     //Fill selection with zeros.
		void WstrCapacityFill();                                  //Fill m_wstrCapacity according to current m_dwCapacity.
		[[nodiscard]] bool IsSectorVisible();                     //Returns m_fSectorsPrintable.
		void UpdateSectorVisible();                               //Updates info about whether sectors lines printable atm or not.
	private:
		const std::unique_ptr<CHexDlgBookmarkMgr> m_pDlgBookmarkMgr { std::make_unique<CHexDlgBookmarkMgr>() }; //Bookmark manager.
		const std::unique_ptr<CHexDlgDataInterpret> m_pDlgDataInterpret { std::make_unique<CHexDlgDataInterpret>() }; //Data Interpreter.
		const std::unique_ptr<CHexDlgFillWith> m_pDlgFillWith { std::make_unique<CHexDlgFillWith>() };     //"Fill with..." dialog.
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
		HEXCOLORSTRUCT m_stColor;             //All control related colors.
		EHexDataMode m_enDataMode { EHexDataMode::DATA_MEMORY }; //Control's data mode.
		EHexShowMode m_enShowMode { };        //Current "Show data" mode.
		PBYTE m_pData { };                    //Main data pointer. Modifiable in "Edit" mode.
		IHexVirtual* m_pHexVirtual { };       //Data handler pointer for EHexDataMode::DATA_VIRTUAL
		HWND m_hwndMsg { };                   //Window handle the control messages will be sent to.
		CWnd m_wndTtBkm { };                  //Tooltip window for bookmarks description.
		TOOLINFO m_stToolInfo { };            //Tooltips struct.
		CFont m_fontMain;                     //Main Hex chunks font.
		CFont m_fontInfo;                     //Font for bottom Info rect.
		CMenu m_menuMain;                     //Main popup menu.
		CPen m_penLines;                      //Pen for lines.
		HEXBOOKMARKSTRUCT* m_pBkmCurrTt { };  //Currently shown bookmark's tooltip;
		double m_dbWheelRatio { };            //Ratio for how much to scroll with mouse-wheel.
		ULONGLONG m_ullDataSize { };          //Size of the displayed data in bytes.
		ULONGLONG m_ullLMouseClick { 0xFFFFFFFFFFFFFFFFULL }; //Left mouse button clicked chunk.
		ULONGLONG m_ullRMouseClick { 0xFFFFFFFFFFFFFFFFULL }; //Right mouse clicked chunk. Used in bookmarking.
		ULONGLONG m_ullCaretPos { };          //Current cursor position.
		DWORD m_dwCapacity { 0x10 };          //How many bytes displayed in one row
		DWORD m_dwCapacityBlockSize { m_dwCapacity / 2 }; //Size of block before space delimiter.
		DWORD m_dwOffsetDigits { };           //Amount of digits in "Offset", depends on data size set in SetData.
		DWORD m_dwOffsetBytes { };            //How many bytes "Offset" number posesses;
		DWORD m_dwSectorSize { 0 };           //Size of a sector to print additional lines between.
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
		int m_iHeightBottomOffArea { };      //Height of the not visible rect from window's bottom to m_iThirdHorizLine.
		int m_iSecondVertLine { }, m_iThirdVertLine { }, m_iFourthVertLine { }; //Vertical lines indent.
		std::wstring m_wstrCapacity { };      //Top Capacity string.
		std::wstring m_wstrInfo { };          //Info text (bottom rect).
		std::wstring m_wstrSectorName { };    //Name of the sector/page.
		std::deque<std::unique_ptr<std::vector<UNDOSTRUCT>>> m_deqUndo; //Undo deque.
		std::deque<std::unique_ptr<std::vector<UNDOSTRUCT>>> m_deqRedo; //Redo deque.
		std::unordered_map<int, HBITMAPSTRUCT> m_umapHBITMAP;           //Images for the Menu.
		bool m_fCreated { false };            //Is control created or not yet.
		bool m_fDataSet { false };            //Is data set or not.
		bool m_fMutable { false };            //Is control works in Edit or Read mode.
		bool m_fCursorHigh { true };          //Cursor's High or Low bits position (first or last digit in hex chunk).
		bool m_fCursorTextArea { false };     //Whether cursor at Ascii or Hex chunks area.
		bool m_fLMousePressed { false };      //Is left mouse button pressed.
		bool m_fSelectionBlock { false };     //Is selection as block (with Alt) or classic.
		bool m_fOffsetAsHex { true };         //Print offset numbers as Hex or as Decimals.
		bool m_fSectorsPrintable { false };   //Print lines between sectors or not.
	};
}