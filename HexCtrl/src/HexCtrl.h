/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/						    *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* This is a Hex control for MFC apps, implemented as CWnd derived class.				*
* The usage is quite simple:														    *
* 1. Construct CHexCtrl object — HEXCTRL::CHexCtrl myHex;							    *
* 2. Call myHex.Create member function to create an instance.   					    *
* 3. Call myHex.SetData method to set the data and its size to display as hex.	        *
****************************************************************************************/
#pragma once
#include "../IHexCtrl.h"
#include <memory>			//std::unique_ptr and related.
#include <unordered_map>	//std::unordered_map and related.
#include <deque>			//std::deque and related.

namespace HEXCTRL {
	/************************************************
	* Forward declarations.							*
	************************************************/
	namespace INTERNAL
	{
		struct UNDOSTRUCT;
		enum class ENCLIPBOARD : DWORD;
		enum class ENSHOWAS : DWORD;

		/********************************************************************************************
		* ENSEARCHTYPE - type of the search, enum.													*
		********************************************************************************************/
		enum class ENSEARCHTYPE : DWORD
		{
			SEARCH_HEX, SEARCH_ASCII, SEARCH_UNICODE
		};

		/********************************************************************************************
		* SEARCHSTRUCT - used for search routines.														*
		********************************************************************************************/
		struct SEARCHSTRUCT
		{
			std::wstring	wstrSearch { };			//String search for.
			ENSEARCHTYPE	enSearchType { };		//Hex, Ascii, Unicode, etc...
			ULONGLONG		ullStartAt { };			//An offset, search should start at.
			int				iDirection { };			//Search direction: 1 = Forward, -1 = Backward.
			int				iWrap { };				//Wrap direction: -1 = Beginning, 1 = End.
			bool			fWrap { false };		//Was search wrapped?
			bool			fSecondMatch { false }; //First or subsequent match. 
			bool			fFound { false };		//Found or not.
			bool			fCount { true };		//Do we count matches or just print "Found".
		};
	}
	namespace SCROLLEX { class CScrollEx; }

	/********************************************************************************************
	* CHexCtrl class declaration.																*
	********************************************************************************************/
	class CHexCtrl : public IHexCtrl
	{
		friend class CHexDlgSearch; //For private Search routine.
	public:
		CHexCtrl();
		virtual ~CHexCtrl();
		bool Create(const HEXCREATESTRUCT& hcs)override; //Main initialization method.
		bool CreateDialogCtrl()override;				 //Сreates custom dialog control.
		bool IsCreated()override;						 //Shows whether control is created or not.
		void SetData(const HEXDATASTRUCT& hds)override;  //Main method for setting data to display (and edit).	
		bool IsDataSet()override;						 //Shows whether a data was set to control or not.
		void ClearData()override;						 //Clears all data from HexCtrl's view (not touching data itself).
		void EditEnable(bool fEnable)override;			 //Enable or disable edit mode.
		void ShowOffset(ULONGLONG ullOffset, ULONGLONG ullSize = 1)override; //Shows (selects) given offset.
		void SetFont(const LOGFONT* pLogFontNew)override;//Sets the control's font.
		void SetFontSize(UINT uiSize)override;			 //Sets the control's font size.
		long GetFontSize()override;						 //Gets the control's font size.
		void SetColor(const HEXCOLORSTRUCT& clr)override;//Sets all the colors for the control.
		void SetCapacity(DWORD dwCapacity)override;		 //Sets the control's current capacity.
	protected:
		DECLARE_MESSAGE_MAP()
		bool RegisterWndClass();
		afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
		afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnMouseMove(UINT nFlags, CPoint point);
		afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
		virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
		afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
		afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
		afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
		afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg UINT OnGetDlgCode(); //To properly work in dialogs.
		afx_msg BOOL OnEraseBkgnd(CDC* pDC);
		afx_msg void OnPaint();
		afx_msg void OnSize(UINT nType, int cx, int cy);
		afx_msg void OnDestroy();
		afx_msg BOOL OnNcActivate(BOOL bActive);
		afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
		afx_msg void OnNcPaint();
	protected:
		[[nodiscard]] BYTE GetByte(ULONGLONG ullIndex); //Gets the byte data by index.
		void ModifyData(const HEXMODIFYSTRUCT& hms); //Main routine to modify data, in fMutable=true mode.
		[[nodiscard]] CWnd* GetMsgWindow(); //Returns pointer to the "Message" window. See HEXDATASTRUCT::pwndMessage.
		void Search(INTERNAL::SEARCHSTRUCT& rSearch); //Search through currently set data.
		void RecalcAll(); //Recalcs all inner draw and data related values.
		void RecalcWorkAreaHeight(int iClientHeight);
		void RecalcScrollSizes(int iClientHeight = 0, int iClientWidth = 0);
		[[nodiscard]] ULONGLONG GetTopLine(); //Returns current top line's number in view.
		[[nodiscard]] ULONGLONG HitTest(LPPOINT); //Is any hex chunk withing given point?
		void ChunkPoint(ULONGLONG ullChunk, ULONGLONG& ullCx, ULONGLONG& ullCy); //Point of Hex chunk.
		void ClipboardCopy(INTERNAL::ENCLIPBOARD enType);
		void ClipboardPaste(INTERNAL::ENCLIPBOARD enType);
		void SetSelection(ULONGLONG ullClick, ULONGLONG ullStart, ULONGLONG ullSize, bool fHighlight = false, bool fMouse = false);
		void SelectAll();
		void UpdateInfoText(); //Updates text in the bottom "info" area according to currently selected data.
		void SetShowAs(INTERNAL::ENSHOWAS enShowAs); //Current data representation type.
		void MsgWindowNotify(const HEXNOTIFYSTRUCT& hns); //Notify routine use in HEXDATAMODEEN::HEXMSG.
		void SetCursorPos(ULONGLONG ullPos, bool fHighPart); //Sets the cursor position when in Edit mode.
		void CursorMoveRight();
		void CursorMoveLeft();
		void CursorMoveUp();
		void CursorMoveDown();
		void CursorScroll(); //Replicates SetSelection, but for cursor.
		void Undo();
		void Redo();
		void SnapshotUndo(ULONGLONG ullIndex, ULONGLONG ullSize); //Takes currently modifiable data snapshot.
		[[nodiscard]] bool IsCurTextArea(); //Whether click was made in Text or Hex area.
	private:
		bool m_fCreated { false };			//Is control created or not yet.
		bool m_fDataSet { false };			//Is data set or not.
		bool m_fFloat { false };			//Is control window float or not.
		bool m_fMutable { false };			//Is control works in Edit mode.
		HEXDATAMODEEN m_enMode { HEXDATAMODEEN::HEXNORMAL }; //Control's mode.
		HEXCOLORSTRUCT m_stColor;			//All control related colors.
		PBYTE m_pData { };					//Main data pointer. Modifiable in "Edit" mode.
		ULONGLONG m_ullDataSize { };		//Size of the displayed data in bytes.
		DWORD m_dwCapacity { 16 };			//How many bytes displayed in one row
		const DWORD m_dwCapacityMax { 64 }; //Maximum capacity.
		DWORD m_dwCapacityBlockSize { m_dwCapacity / 2 }; //Size of block before space delimiter.
		INTERNAL::ENSHOWAS m_enShowAs { };  //Show data mode.
		CWnd* m_pwndMsg { };				//Window the control messages will be sent to.
		IHexVirtual* m_pHexVirtual { };		//Data handler pointer for HEXDATAMODEEN::HEXVIRTUAL
		SIZE m_sizeLetter { 1, 1 };			//Current font's letter size (width, height).
		CFont m_fontHexView;				//Main Hex chunks font.
		CFont m_fontBottomRect;				//Font for bottom Info rect.
		std::unique_ptr<CHexDlgSearch> m_pDlgSearch { std::make_unique<CHexDlgSearch>() }; //Search dialog.
		std::unique_ptr<SCROLLEX::CScrollEx> m_pstScrollV { std::make_unique<SCROLLEX::CScrollEx>() }; //Vertical scroll object.
		std::unique_ptr<SCROLLEX::CScrollEx> m_pstScrollH { std::make_unique<SCROLLEX::CScrollEx>() }; //Horizontal scroll object.
		CMenu m_menuMain;					//Main popup menu.
		CMenu m_menuShowAs;					//Submenu "Show as..."
		CBrush m_stBrushBkSelected;			//Brush for "selected" background.
		CPen m_penLines { PS_SOLID, 1, RGB(200, 200, 200) };
		int m_iSizeFirstHalf { };		    //Size of first half of capacity.
		int m_iSizeHexByte { };			    //Size of two hex letters representing one byte.
		int m_iIndentAscii { };			    //Indent of Ascii text begining.
		int m_iIndentFirstHexChunk { };	    //First hex chunk indent.
		int m_iIndentTextCapacityY { };	    //Caption text (0 1 2... D E F...) vertical offset.
		int m_iIndentBottomLine { 1 };	    //Bottom line indent from window's bottom.
		int m_iDistanceBetweenHexChunks { };//Distance between begining of the two hex chunks.
		int m_iSpaceBetweenHexChunks { };   //Space between Hex chunks.
		int m_iSpaceBetweenAscii { };	    //Space between two Ascii chars.
		int m_iSpaceBetweenBlocks { };	    //Additional space between hex chunks after half of capacity.
		int m_iHeightTopRect { };		    //Height of the header where offsets (0 1 2... D E F...) reside.
		int m_iHeightBottomRect { 22 };	    //Height of bottom Info rect.
		int m_iHeightBottomOffArea { m_iHeightBottomRect + m_iIndentBottomLine }; //Height of not visible rect from window's bottom to m_iThirdHorizLine.
		int m_iHeightWorkArea { };		    //Needed for mouse selection point.y calculation.
		int m_iFirstVertLine { }, m_iSecondVertLine { }, m_iThirdVertLine { }, m_iFourthVertLine { }; //Vertical lines indent.
		ULONGLONG m_ullSelectionStart { }, m_ullSelectionEnd { }, m_ullSelectionClick { }, m_ullSelectionSize { };
		std::unordered_map<unsigned, std::wstring> m_umapCapacityWstr; //"Capacity" letters for fast lookup.
		std::wstring m_wstrBottomText { };  //Info text (bottom rect).
		const std::wstring m_wstrErrVirtual { L"This function isn't supported in Virtual mode!" };
		bool m_fLMousePressed { false };
		DWORD m_dwOffsetDigits { 8 };		//Amount of digits in "Offset", depends on data size set in SetData.
		ULONGLONG m_ullCursorPos { };		//Current cursor position.
		bool m_fCursorHigh { true };		//Cursor's High or Low bits position (first or last digit in hex chunk).
		bool m_fCursorTextArea { false };	//Whether cursor at Ascii or Hex chunks area.
		DWORD m_dwUndoMax { 500 };			//How many Undo states to preserve.
		std::deque<std::unique_ptr<INTERNAL::UNDOSTRUCT>> m_deqUndo; //Undo deque.
		std::deque<std::unique_ptr<INTERNAL::UNDOSTRUCT>> m_deqRedo; //Redo deque.
		std::unordered_map<int, HBITMAP> m_umapHBITMAP; //Images for the Menu.
	};
};