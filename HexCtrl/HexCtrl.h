/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/						    *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                 *
* This is a Hex control for MFC apps, implemented as CWnd derived class.				*
* The usage is quite simple:														    *
* 1. Construct CHexCtrl object — HEXCTRL::CHexCtrl myHex;							    *
* 2. Call myHex.Create member function to create an instance.   					    *
* 3. Call myHex.SetData method to set the data and its size to display as hex.	        *
****************************************************************************************/
#pragma once
#include <memory>			//std::unique_ptr and related.
#include <unordered_map>	//std::unordered_map and related.
#include <deque>			//std::deque and related.
#include <afxwin.h>			//MFC core and standard components.

namespace HEXCTRL {
	/************************************************
	* Forward declarations.							*
	************************************************/
	class CHexDlgSearch;
	struct HEXMODIFYSTRUCT;
	namespace INTERNAL
	{
		struct STUNDO;
		enum class ENCLIPBOARD : DWORD;
		enum class ENSHOWAS : DWORD;
	}
	namespace SCROLLEX { class CScrollEx; }

	/********************************************************************************************
	* HEXDATAMODEEN - Enum to set the data working mode. Used in HEXDATASTRUCT in SetData.		*
	********************************************************************************************/
	enum class HEXDATAMODEEN : DWORD
	{
		HEXNORMAL, HEXMSG, HEXCUSTOM
	};

	/********************************************************************************************
	* HEXMODIFYASEN - enum to represent data modification type.									*
	********************************************************************************************/
	enum class HEXMODIFYASEN : DWORD
	{
		AS_MODIFY, AS_FILL, AS_UNDO, AS_REDO
	};

	/********************************************************************************************
	* CHexCustom - Pure abstract data handler class, that can be implemented by client,			*
	* to set its own data handler routines.	Works in HEXDATAMODEEN::HEXCUSTOM mode.				*
	* Pointer to this class can be set in SetData methods.										*
	* Its usage is very similar to pwndMsg logic, where control sends WM_NOTIFY messages		*
	* to CWnd* class to get/set data. But in this case it's just a pointer to a custom			*
	* routines implementation.																	*
	* All virtual functions must be defined in client derived class.							*
	********************************************************************************************/
	class CHexCustom
	{
	public:
		virtual BYTE GetByte(ULONGLONG ullIndex) = 0; //Gets the byte data by index.
		virtual	void ModifyData(const HEXMODIFYSTRUCT& hmd) = 0; //Main routine to modify data, in fMutable=true mode.
	};

	/********************************************************************************************
	* HEXCOLORSTRUCT - All HexCtrl colors.														*
	********************************************************************************************/
	struct HEXCOLORSTRUCT
	{
		COLORREF clrTextHex { GetSysColor(COLOR_WINDOWTEXT) };		//Hex chunks color.
		COLORREF clrTextAscii { GetSysColor(COLOR_WINDOWTEXT) };	//Ascii text color.
		COLORREF clrTextSelected { GetSysColor(COLOR_WINDOWTEXT) }; //Selected text color.
		COLORREF clrTextCaption { RGB(0, 0, 180) };					//Caption color
		COLORREF clrTextInfoRect { GetSysColor(COLOR_WINDOWTEXT) };	//Text color of the bottom "Info" rect.
		COLORREF clrTextCursor { RGB(255, 255, 255) };				//Cursor text color.
		COLORREF clrBk { GetSysColor(COLOR_WINDOW) };				//Background color.
		COLORREF clrBkSelected { RGB(200, 200, 255) };				//Background color of the selected Hex/Ascii.
		COLORREF clrBkInfoRect { RGB(250, 250, 250) };				//Background color of the bottom "Info" rect.
		COLORREF clrBkCursor { RGB(0, 0, 250) };					//Cursor background color.
	};
	using PHEXCOLORSTRUCT = HEXCOLORSTRUCT * ;

	/********************************************************************************************
	* HEXCREATESTRUCT - for CHexCtrl::Create method.											*
	********************************************************************************************/
	struct HEXCREATESTRUCT
	{
		PHEXCOLORSTRUCT pstColor { };			//Pointer to HEXCOLORSTRUCT, if nullptr default colors are used.
		CWnd*		    pParent { };			//Parent window's pointer.
		UINT		    uId { };				//Hex control Id.
		DWORD			dwExStyles { };			//Extended window styles.
		CRect			rect { };				//Initial rect. If null, the window is screen centered.
		const LOGFONTW* pLogFont { };			//Font to be used, nullptr for default.
		bool			fFloat { false };		//Is float or child (incorporated into another window)?.
		bool			fCustomCtrl { false };	//It's a custom dialog control.
	};

	/********************************************************************************************
	* HEXDATASTRUCT - for CHexCtrl::SetData method.												*
	********************************************************************************************/
	struct HEXDATASTRUCT
	{
		ULONGLONG		ullDataSize { };					//Size of the data to display, in bytes.
		ULONGLONG		ullSelectionStart { };				//Set selection at this position. Works only if ullSelectionSize > 0.
		ULONGLONG		ullSelectionSize { };				//How many bytes to set as selected.
		CWnd*			pwndMsg { };						//Window to send the control messages to. If nullptr then the parent window is used.
		CHexCustom*		pHexHandler { };					//Pointer to Virtual data handler class.
		PBYTE			pData { };							//Pointer to the data. Not used if it's virtual control.
		HEXDATAMODEEN	enMode { HEXDATAMODEEN::HEXNORMAL };//Working mode of control.
		bool			fMutable { false };					//Will data be mutable (editable) or just read mode.
	};

	/********************************************************************************************
	* HEXNOTIFYSTRUCT - used in notifications routines.											*
	********************************************************************************************/
	struct HEXNOTIFYSTRUCT
	{
		NMHDR			hdr { };		//Standard Windows header. For hdr.code values see HEXCTRL_MSG_* messages.
		ULONGLONG		ullIndex { };	//Index of the start byte to get/send.
		ULONGLONG		ullSize { };	//Size of the bytes to get/send.
		PBYTE			pData { };		//Pointer to a data to get/send.
		BYTE			chByte { };		//Single byte data - used for simplicity, when ullModifySize==1.
	};
	using PHEXNOTIFYSTRUCT = HEXNOTIFYSTRUCT * ;

	/********************************************************************************************
	* HEXMODIFYSTRUCT - used to represent data modification parameters.							*
	********************************************************************************************/
	struct HEXMODIFYSTRUCT {
		ULONGLONG		ullIndex { };		//Index of the start byte to modify.
		ULONGLONG		ullModifySize { };	//Size in bytes.
		PBYTE			pData { };			//Pointer to data to be set.
		HEXMODIFYASEN	enType { HEXMODIFYASEN::AS_MODIFY }; //Modification type.
		DWORD			dwFillDataSize { }; //Size of pData if enType==AS_FILL.
		bool			fWhole { true };	//Is a whole byte or just a part of it to be modified.
		bool			fHighPart { true };	//Shows whether High or Low part of byte should be modified (If fWhole is false).
		bool			fMoveNext { true };	//Should cursor be moved to the next byte.
	};

	/********************************************************************************************
	* HEXSEARCHSTRUCT - used for search routines.												*
	********************************************************************************************/
	struct HEXSEARCHSTRUCT
	{
		std::wstring	wstrSearch { };			//String search for.
		DWORD			dwSearchType { };		//Hex, Ascii, Unicode, etc...
		ULONGLONG		ullStartAt { };			//An offset, search should start at.
		int				iDirection { };			//Search direction: Forward <-> Backward.
		bool			fWrap { false };		//Was search wrapped?
		int				iWrap { };				//Wrap direction.
		bool			fSecondMatch { false }; //First or subsequent match. 
		bool			fFound { false };
		bool			fCount { true };		//Do we count matches or just print "Found".
	};

	/********************************************************************************************
	* CHexCtrl class declaration.																*
	********************************************************************************************/
	class CHexCtrl : public CWnd
	{
	public:
		CHexCtrl();
		virtual ~CHexCtrl();
		bool Create(const HEXCREATESTRUCT& hcs); //Main initialization method, CHexCtrl::Create.
		bool IsCreated();						 //Shows whether control is created or not.
		void SetData(const HEXDATASTRUCT& hds);  //Main method for setting data to display (and edit).	
		bool IsDataSet();						 //Is data set or not.
		void ClearData();						 //Clears all data from HexCtrl's view (not touching data itself).
		void EnableEdit(bool fEnable);			 //Enable or disable edit mode.
		void ShowOffset(ULONGLONG ullOffset, ULONGLONG ullSize = 1); //Shows (selects) given offset.
		void SetFont(const LOGFONT* pLogFontNew);//Sets the control's font.
		void SetFontSize(UINT uiSize);			 //Sets the control's font size.
		long GetFontSize();						 //Gets the control's font size.
		void SetColor(const HEXCOLORSTRUCT& clr);//Sets all the colors for the control.
		void SetCapacity(DWORD dwCapacity);		 //Sets the control's current capacity.
		UINT GetDlgCtrlID()const;
		CWnd* GetParent()const;
		void Search(HEXSEARCHSTRUCT& rSearch);	 //Search through currently set data.
	protected:
		DECLARE_MESSAGE_MAP()
		bool RegisterWndClass();
		void OnDraw(CDC* pDC) {} //All drawing is in OnPaint.
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
		afx_msg BOOL OnNcActivate(BOOL bActive);
		afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
		afx_msg void OnNcPaint();
		afx_msg void OnDestroy();
	protected:
		BYTE GetByte(ULONGLONG ullIndex); //Gets the byte data by index.
		void ModifyData(const HEXMODIFYSTRUCT& hmd); //Main routine to modify data, in fMutable=true mode.
		void RecalcAll();
		void RecalcWorkAreaHeight(int iClientHeight);
		void RecalcScrollSizes(int iClientHeight = 0, int iClientWidth = 0);
		ULONGLONG GetCurrentLineV();
		ULONGLONG HitTest(LPPOINT); //Is any hex chunk withing given point?
		void ChunkPoint(ULONGLONG ullChunk, ULONGLONG& ullCx, ULONGLONG& ullCy); //Point of Hex chunk.
		void ClipboardCopy(INTERNAL::ENCLIPBOARD enType);
		void ClipboardPaste(INTERNAL::ENCLIPBOARD enType);
		void SetSelection(ULONGLONG ullClick, ULONGLONG ullStart, ULONGLONG ullSize, bool fHighlight = false, bool fMouse = false);
		void SelectAll();
		void UpdateInfoText();
		void SetShowAs(INTERNAL::ENSHOWAS enShowAs);
		void MsgNotify(const HEXNOTIFYSTRUCT& hns); //Notify routine use in HEXDATAMODEEN::HEXMSG.
		void SetCursorPos(ULONGLONG ullPos, bool fHighPart); //Sets the cursor position when in Edit mode.
		void CursorMoveRight();
		void CursorMoveLeft();
		void CursorMoveUp();
		void CursorMoveDown();
		void CursorScroll(); //Replicates SetSelection, but for cursor.
		void Undo();
		void Redo();
		void SnapshotUndo(ULONGLONG ullIndex, ULONGLONG ullSize); //Takes currently modifiable data snapshot.
	private:
		bool m_fCreated { false };			//Is control created or not yet.
		bool m_fDataSet { false };			//Is data set or not.
		bool m_fFloat { false };			//Is control window float or not.
		bool m_fMutable { false };			//Is control works in Edit mode.
		HEXDATAMODEEN m_enMode { HEXDATAMODEEN::HEXNORMAL }; //Control's mode.
		PBYTE m_pData { };					//Main data pointer. Modifiable in "Edit" mode.
		ULONGLONG m_ullDataSize { };		//Size of the displayed data in bytes.
		DWORD m_dwCapacity { 16 };			//How many bytes displayed in one row
		const DWORD m_dwCapacityMax { 64 }; //Maximum capacity.
		DWORD m_dwCapacityBlockSize { m_dwCapacity / 2 }; //Size of block before space delimiter.
		INTERNAL::ENSHOWAS m_enShowAs { };  //Show data mode.
		CWnd* m_pwndParentOwner { };		//Parent or owner window pointer.
		CWnd* m_pwndMsg { };				//Window the control messages will be sent to.
		CHexCustom* m_pCustom { };			//Data handler pointer for HEXDATAMODEEN::HEXCUSTOM
		SIZE m_sizeLetter { 1, 1 };			//Current font's letter size (width, height).
		CFont m_fontHexView;				//Main Hex chunks font.
		CFont m_fontBottomRect;				//Font for bottom Info rect.
		std::unique_ptr<CHexDlgSearch> m_pDlgSearch { std::make_unique<CHexDlgSearch>() }; //Search dialog.
		std::unique_ptr<SCROLLEX::CScrollEx> m_pstScrollV { std::make_unique<SCROLLEX::CScrollEx>() }; //Vertical scroll object.
		std::unique_ptr<SCROLLEX::CScrollEx> m_pstScrollH { std::make_unique<SCROLLEX::CScrollEx>() }; //Horizontal scroll object.
		CMenu m_menuMain;					//Main popup menu.
		CMenu m_menuShowAs;					//Submenu "Show as..."
		HEXCOLORSTRUCT m_stColor;			//All control related colors.
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
		UINT m_dwCtrlId { };				//Id of the control.
		DWORD m_dwOffsetDigits { 8 };		//Amount of digits in "Offset", depends on data size set in SetData.
		ULONGLONG m_ullCursorPos { };		//Current cursor position.
		bool m_fCursorHigh { true };		//Cursor's High or Low bits position (first or last digit in hex chunk).
		bool m_fCursorAscii { false };		//Whether cursor at Ascii or Hex chunks area.
		DWORD m_dwUndoMax { 50 };			//How many Undo states to preserve.
		std::deque<std::unique_ptr<INTERNAL::STUNDO>> m_deqUndo; //Undo deque.
		std::deque<std::unique_ptr<INTERNAL::STUNDO>> m_deqRedo; //Redo deque.
		std::unordered_map<int, HBITMAP> m_umapHBITMAP; //Images for the Menu.
	};

	/********************************************************************************************
	* WM_NOTIFY message codes (NMHDR.code values).												*
	* These codes are used to notify m_pwndMsg window about control's current states.			*
	********************************************************************************************/

	constexpr auto HEXCTRL_MSG_DESTROY = 0x00FF;		//Inicates that HexCtrl is being destroyed.
	constexpr auto HEXCTRL_MSG_GETDATA = 0x0100;		//Used in Virtual mode to demand the next byte to display.
	constexpr auto HEXCTRL_MSG_MODIFYDATA = 0x0101;		//Indicates that the byte in memory has changed, used in edit mode.
	constexpr auto HEXCTRL_MSG_SETSELECTION = 0x0102;	//A selection has been made for some bytes.
};