/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/                         *
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
#include <afxwin.h>      //MFC core and standard components.
#include "../HexCtrl.h"

namespace HEXCTRL::INTERNAL
{
	/*********************************
	* Forward declarations.          *
	*********************************/
	class CHexDlgSearch;
	class CHexBookmarks;
	class CHexDlgOperations;
	class CHexDlgFillWith;
	class CHexSelect;
	struct UNDOSTRUCT;
	enum class EClipboard : DWORD;
	namespace SCROLLEX { class CScrollEx; }

	/***************************************************************************************
	* EShowMode - current data mode representation.                                        *
	***************************************************************************************/
	enum class EShowMode : DWORD
	{
		ASBYTE = 1, ASWORD = 2, ASDWORD = 4, ASQWORD = 8
	};

	/********************************************************************************************
	* CHexCtrl class declaration.																*
	********************************************************************************************/
	class CHexCtrl : public CWnd, public IHexCtrl
	{
	public:
		CHexCtrl();
		virtual ~CHexCtrl();
		bool Create(const HEXCREATESTRUCT& hcs)override;
		bool CreateDialogCtrl(UINT uCtrlID, HWND hwndDlg)override;
		void SetData(const HEXDATASTRUCT& hds)override;
		void ClearData()override;
		void SetEditMode(bool fEnable)override;
		void SetFont(const LOGFONTW* pLogFontNew)override;
		void SetFontSize(UINT uiSize)override;
		void SetColor(const HEXCOLORSTRUCT& clr)override;
		void SetCapacity(DWORD dwCapacity)override;
		void GoToOffset(ULONGLONG ullOffset, bool fSelect, ULONGLONG ullSize)override;
		void SetSelection(ULONGLONG ullOffset, ULONGLONG ullSize)override;
		bool IsCreated()const override;
		bool IsDataSet()const override;
		bool IsMutable()const override;
		long GetFontSize()const override;
		auto GetSelection()const->std::vector<HEXSPANSTRUCT> & override;
		HWND GetWindowHandle()const override;
		HMENU GetMenuHandle()const override;
		void Destroy()override;
	protected:
		DECLARE_MESSAGE_MAP()
		afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
		afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnMouseMove(UINT nFlags, CPoint point);
		afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
		virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
		afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
		afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
		afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
		afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
		afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
		void OnKeyDownShift(UINT nChar); //Key pressed with the Shift.
		void OnKeyDownShiftLeft();       //Left Key pressed with the Shift.
		void OnKeyDownShiftRight();      //Right Key pressed with the Shift.
		void OnKeyDownShiftUp();         //Up Key pressed with the Shift.
		void OnKeyDownShiftDown();       //Down Key pressed with the Shift.
		void OnKeyDownCtrl(UINT nChar);  //Key pressed with the Ctrl.
		afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg UINT OnGetDlgCode();     //To properly work in dialogs.
		afx_msg BOOL OnEraseBkgnd(CDC* pDC);
		afx_msg void OnPaint();
		afx_msg void OnSize(UINT nType, int cx, int cy);
		afx_msg void OnDestroy();
		afx_msg BOOL OnNcActivate(BOOL bActive);
		afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
		afx_msg void OnNcPaint();
	public:
		PBYTE GetData(ULONGLONG* pUllSize = nullptr);          //Gets current data pointer and data size.
		[[nodiscard]] BYTE GetByte(ULONGLONG ullIndex)const;   //Gets the BYTE data by index.
		[[nodiscard]] WORD GetWord(ULONGLONG ullIndex)const;   //Gets the WORD data by index.
		[[nodiscard]] DWORD GetDword(ULONGLONG ullIndex)const; //Gets the DWORD data by index.
		[[nodiscard]] QWORD GetQword(ULONGLONG ullIndex)const; //Gets the QWORD data by index.
		bool SetByte(ULONGLONG ullIndex, BYTE bData);          //Sets the BYTE data by index.
		bool SetWord(ULONGLONG ullIndex, WORD wData);		   //Sets the WORD data by index.
		bool SetDword(ULONGLONG ullIndex, DWORD dwData);	   //Sets the DWORD data by index.
		bool SetQword(ULONGLONG ullIndex, QWORD qwData);	   //Sets the QWORD data by index.
		void ModifyData(const HEXMODIFYSTRUCT& hms, bool fRedraw = true); //Main routine to modify data, in m_fMutable==true mode.
		[[nodiscard]] HWND GetMsgWindow()const;                //Returns pointer to the "Message" window. See HEXDATASTRUCT::pwndMessage.
		void RecalcAll();                                      //Recalcs all inner draw and data related values.
		void RecalcWorkAreaHeight(int iClientHeight);
		void RecalcScrollSizes(int iClientHeight = 0, int iClientWidth = 0);
		[[nodiscard]] ULONGLONG GetTopLine()const;             //Returns current top line's number in view.
		[[nodiscard]] ULONGLONG HitTest(const POINT*);         //Is any hex chunk withing given point?
		void HexChunkPoint(ULONGLONG ullChunk, int& iCx, int& iCy)const; //Point of Hex chunk.
		void AsciiChunkPoint(ULONGLONG ullChunk, int& iCx, int& iCy)const; //Point of Ascii chunk.
		void ClipboardCopy(EClipboard enType);
		void ClipboardPaste(EClipboard enType);
		void UpdateInfoText();                                 //Updates text in the bottom "info" area according to currently selected data.
		void SetShowMode(EShowMode enShowMode);                //Set current data representation mode.
		void ParentNotify(const HEXNOTIFYSTRUCT& hns)const;    //Notify routine used to send messages to Parent window.
		void ParentNotify(UINT uCode)const;                    //Same as above, but only for notification code.
		void MsgWindowNotify(const HEXNOTIFYSTRUCT& hns)const; //Notify routine used to send messages to Msg window.
		void MsgWindowNotify(UINT uCode)const;                 //Same as above, but only for notification code.
		[[nodiscard]] ULONGLONG GetCursorPos();                //Cursor or selection_click depending on edit mode.
		void SetCursorPos(ULONGLONG ullPos, bool fHighPart);   //Sets the cursor position when in Edit mode.
		void CursorMoveRight();
		void CursorMoveLeft();
		void CursorMoveUp();
		void CursorMoveDown();
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
	private:
		const DWORD m_dwCapacityMax { 128 };  //Maximum capacity.
		const std::unique_ptr<CHexSelect> m_pSelect { std::make_unique<CHexSelect>() };                    //Selection class.
		const std::unique_ptr<CHexDlgSearch> m_pDlgSearch { std::make_unique<CHexDlgSearch>() };           //"Search..." dialog.
		const std::unique_ptr<CHexBookmarks> m_pBookmarks { std::make_unique<CHexBookmarks>() };           //Bookmarks.
		const std::unique_ptr<CHexDlgOperations> m_pDlgOpers { std::make_unique<CHexDlgOperations>() };    //"Operations" dialog.
		const std::unique_ptr<CHexDlgFillWith> m_pDlgFillWith { std::make_unique<CHexDlgFillWith>() };     //"Fill with..." dialog.
		const std::unique_ptr<SCROLLEX::CScrollEx> m_pScrollV { std::make_unique<SCROLLEX::CScrollEx>() }; //Vertical scroll bar.
		const std::unique_ptr<SCROLLEX::CScrollEx> m_pScrollH { std::make_unique<SCROLLEX::CScrollEx>() }; //Horizontal scroll bar.
		const int m_iIndentBottomLine { 1 };  //Bottom line indent from window's bottom.
		const int m_iHeightBottomRect { 22 }; //Height of bottom Info rect.
		const int m_iHeightBottomOffArea { m_iHeightBottomRect + m_iIndentBottomLine }; //Height of not visible rect from window's bottom to m_iThirdHorizLine.
		const int m_iFirstHorizLine { 0 };    //First horizontal line indent.
		const int m_iFirstVertLine { 0 };     //First vertical line indent.
		const size_t m_dwsUndoMax { 500 };    //How many Undo states to preserve.
		HEXCOLORSTRUCT m_stColor;             //All control related colors.
		EHexDataMode m_enMode { EHexDataMode::DATA_MEMORY }; //Control's mode.
		EShowMode m_enShowMode { EShowMode::ASBYTE };        //Current "Show data" mode.
		PBYTE m_pData { };                    //Main data pointer. Modifiable in "Edit" mode.
		IHexVirtual* m_pHexVirtual { };       //Data handler pointer for EHexDataMode::DATA_VIRTUAL
		HWND m_hwndMsg { };                   //Window handle the control messages will be sent to.
		ULONGLONG m_ullDataSize { };          //Size of the displayed data in bytes.
		ULONGLONG m_ullSelectionClick { };
		ULONGLONG m_ullCursorPos { };         //Current cursor position.
		ULONGLONG m_ullRMouseChunk { 0xFFFFFFFFFFFFFFFFull }; //Right mouse clicked chunk (hex or ascii) index. Used in bookmarking.
		DWORD m_dwCapacity { 16 };            //How many bytes displayed in one row
		DWORD m_dwCapacityBlockSize { m_dwCapacity / 2 }; //Size of block before space delimiter.
		DWORD m_dwOffsetDigits { 8 };         //Amount of digits in "Offset", depends on data size set in SetData.
		SIZE m_sizeLetter { 1, 1 };           //Current font's letter size (width, height).
		CFont m_fontHexView;                  //Main Hex chunks font.
		CFont m_fontBottomRect;               //Font for bottom Info rect.
		CMenu m_menuMain;                     //Main popup menu.
		CPen m_penLines { PS_SOLID, 1, RGB(200, 200, 200) }; //Pen for lines.
		long m_lFontSize { };                 //Current font size.
		int m_iSizeFirstHalf { };             //Size of first half of capacity.
		int m_iSizeHexByte { };               //Size of two hex letters representing one byte.
		int m_iIndentAscii { };               //Indent of Ascii text begining.
		int m_iIndentFirstHexChunk { };       //First hex chunk indent.
		int m_iIndentTextCapacityY { };       //Caption text (0 1 2... D E F...) vertical offset.
		int m_iDistanceBetweenHexChunks { };  //Distance between begining of the two hex chunks.
		int m_iSpaceBetweenHexChunks { };     //Space between Hex chunks.
		int m_iSpaceBetweenAscii { };         //Space between two Ascii chars.
		int m_iSpaceBetweenBlocks { };        //Additional space between hex chunks after half of capacity.
		int m_iHeightTopRect { };             //Height of the header where offsets (0 1 2... D E F...) reside.
		int m_iStartWorkAreaY { };            //Start Y of the area where all drawing occurs.
		int m_iEndWorkArea { };               //End of the area where all drawing occurs.
		int m_iHeightWorkArea { };            //Height of the working area where all drawing occurs.
		int m_iSecondVertLine { }, m_iThirdVertLine { }, m_iFourthVertLine { }; //Vertical lines indent.
		std::wstring m_wstrCapacity { };      //Top Capacity string.
		std::wstring m_wstrBottomText { };    //Info text (bottom rect).
		std::deque<std::unique_ptr<std::vector<UNDOSTRUCT>>> m_deqUndo; //Undo deque.
		std::deque<std::unique_ptr<std::vector<UNDOSTRUCT>>> m_deqRedo; //Redo deque.
		std::unordered_map<int, HBITMAP> m_umapHBITMAP;    //Images for the Menu.
		bool m_fCreated { false };            //Is control created or not yet.
		bool m_fDataSet { false };            //Is data set or not.
		bool m_fMutable { false };            //Is control works in Edit or Read mode.
		bool m_fCursorHigh { true };          //Cursor's High or Low bits position (first or last digit in hex chunk).
		bool m_fCursorTextArea { false };     //Whether cursor at Ascii or Hex chunks area.
		bool m_fLMousePressed { false };      //Is left mouse button pressed.
		bool m_fSelectionBlock { false };     //Is selection as block (with Alt) or classic.
	};
}