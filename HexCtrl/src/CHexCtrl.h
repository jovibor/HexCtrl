/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/                         *
* This is a Hex Control for MFC applications.                                           *
* Official git repository of the project: https://github.com/jovibor/HexCtrl/           *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information, or any questions, visit the project's official repository.      *
****************************************************************************************/
#pragma once
#include "../HexCtrl.h"
#include <memory>        //std::unique_ptr and related.
#include <unordered_map> //std::unordered_map and related.
#include <deque>         //std::deque and related.
#include <string>        //std::wstring and related.
#include <afxwin.h>      //MFC core and standard components.

namespace HEXCTRL {
	namespace INTERNAL
	{
		/*********************************
		* Forward declarations.          *
		*********************************/
		class CHexDlgSearch;
		struct UNDOSTRUCT;
		enum class EClipboard : DWORD;
		namespace SCROLLEX { class CScrollEx; }

		/***************************************************************************************
		* EShowMode - current data mode representation.                                       *
		***************************************************************************************/
		enum class EShowMode : DWORD
		{
			ASBYTE = 1, ASWORD = 2, ASDWORD = 4, ASQWORD = 8
		};

		/***************************************************************************************
		* ESearchType - type of the search, enum.                                             *
		***************************************************************************************/
		enum class ESearchType : DWORD
		{
			SEARCH_HEX, SEARCH_ASCII, SEARCH_UTF16
		};

		/****************************************************************************************
		* SEARCHSTRUCT - used for search routines.                                              *
		****************************************************************************************/
		struct SEARCHSTRUCT
		{
			std::wstring wstrSearch { };         //String search for.
			std::wstring wstrReplace { };        //SearchReplace with, string.
			ESearchType  enSearchType { };       //Hex, Ascii, Unicode, etc...
			ULONGLONG    ullIndex { };           //An offset search should start from.
			DWORD        dwCount { };            //How many, or what index number.
			DWORD        dwReplaced { };         //Replaced amount;
			int          iDirection { };         //Search direction: 1 = Forward, -1 = Backward.
			int          iWrap { };              //Wrap direction: -1 = Beginning, 1 = End.
			bool         fWrap { false };        //Was search wrapped?
			bool         fSecondMatch { false }; //First or subsequent match. 
			bool         fFound { false };       //Found or not.
			bool         fDoCount { true };      //Do we count matches or just print "Found".
			bool         fReplace { false };     //Find or Find and Replace with...?
			bool         fAll { false };         //Find/Replace one by one, or all?
		};

		/********************************************************************************************
		* CHexCtrl class declaration.																*
		********************************************************************************************/
		class CHexCtrl : public CWnd, public IHexCtrl
		{
			friend class CHexDlgSearch; //For private SearchCallback routine.
		public:
			CHexCtrl();
			virtual ~CHexCtrl();
			bool Create(const HEXCREATESTRUCT& hcs)override;
			bool CreateDialogCtrl(UINT uCtrlID, HWND hwndDlg)override;
			void SetData(const HEXDATASTRUCT& hds)override;
			void ClearData()override;
			void SetEditMode(bool fEnable)override;
			void ShowOffset(ULONGLONG ullOffset, ULONGLONG ullSize = 1)override;
			void SetFont(const LOGFONTW* pLogFontNew)override;
			void SetFontSize(UINT uiSize)override;
			void SetColor(const HEXCOLORSTRUCT& clr)override;
			void SetCapacity(DWORD dwCapacity)override;
			bool IsCreated()const override;
			bool IsDataSet()const override;
			bool IsMutable()const override;
			long GetFontSize()const override;
			void GetSelection(ULONGLONG& ullOffset, ULONGLONG& ullSize)const override;
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
			bool RegisterWndClass();                               //Registering HexCtrl window class.
			[[nodiscard]] BYTE GetByte(ULONGLONG ullIndex)const;   //Gets the byte data by index.
			void ModifyData(const HEXMODIFYSTRUCT& hms, bool fRedraw = true); //Main routine to modify data, in m_fMutable==true mode.
			[[nodiscard]] HWND GetMsgWindow()const;                //Returns pointer to the "Message" window. See HEXDATASTRUCT::pwndMessage.
			void RecalcAll();                                      //Recalcs all inner draw and data related values.
			void RecalcWorkAreaHeight(int iClientHeight);
			void RecalcScrollSizes(int iClientHeight = 0, int iClientWidth = 0);
			[[nodiscard]] ULONGLONG GetTopLine()const;             //Returns current top line's number in view.
			[[nodiscard]] ULONGLONG HitTest(const POINT*);         //Is any hex chunk withing given point?
			void HexChunkPoint(ULONGLONG ullChunk, ULONGLONG& ullCx, ULONGLONG& ullCy)const;   //Point of Hex chunk.
			void AsciiChunkPoint(ULONGLONG ullChunk, ULONGLONG& ullCx, ULONGLONG& ullCy)const; //Point of Ascii chunk.
			void ClipboardCopy(EClipboard enType);
			void ClipboardPaste(EClipboard enType);
			void OnKeyDownShift(UINT nChar);                       //Key pressed with the Shift.
			void OnKeyDownCtrl(UINT nChar);                        //Key pressed with the Ctrl.
			void UpdateInfoText();                                 //Updates text in the bottom "info" area according to currently selected data.
			void SetShowMode(EShowMode enShowMode);                //Set current data representation mode.
			void MsgWindowNotify(const HEXNOTIFYSTRUCT& hns)const; //Notify routine used to send messages to Msg window.
			void MsgWindowNotify(UINT uCode)const;                 //Same as above, but only for notifications.
			void SetCursorPos(ULONGLONG ullPos, bool fHighPart);   //Sets the cursor position when in Edit mode.
			void CursorMoveRight();
			void CursorMoveLeft();
			void CursorMoveUp();
			void CursorMoveDown();
			void Undo();
			void Redo();
			void SnapshotUndo(ULONGLONG ullIndex, ULONGLONG ullSize); //Takes currently modifiable data snapshot.
			[[nodiscard]] bool IsCurTextArea()const;                  //Whether click was made in Text or Hex area.
			void SearchCallback(SEARCHSTRUCT& rSearch);               //Search through currently set data.
			void SearchReplace(ULONGLONG ullIndex, PBYTE pData, size_t nSizeData, size_t nSizeReplace, bool fRedraw = true);
			void SetSelection(ULONGLONG ullClick, ULONGLONG ullStart, ULONGLONG ullSize, bool fHighlight = false, bool fMouse = false);
			void SelectAll();
			void FillWithZeros();                                     //Fill selection with zeros.
			void FillCapacity();                                      //Fill m_wstrCapacity according to current m_dwCapacity.
		private:
			bool m_fCreated { false };          //Is control created or not yet.
			bool m_fDataSet { false };          //Is data set or not.
			bool m_fFloat { false };            //Is control window float or not.
			bool m_fMutable { false };          //Is control works in Edit or Read mode.
			EHexDataMode m_enMode { EHexDataMode::DATA_DEFAULT }; //Control's mode.
			HEXCOLORSTRUCT m_stColor;           //All control related colors.
			PBYTE m_pData { };                  //Main data pointer. Modifiable in "Edit" mode.
			ULONGLONG m_ullDataSize { };        //Size of the displayed data in bytes.
			DWORD m_dwCapacity { 16 };          //How many bytes displayed in one row
			const DWORD m_dwCapacityMax { 128 };//Maximum capacity.
			DWORD m_dwCapacityBlockSize { m_dwCapacity / 2 }; //Size of block before space delimiter.
			EShowMode m_enShowMode { EShowMode::ASBYTE }; //Show data mode.
			HWND m_hwndMsg { };                //Window, the control messages will be sent to.
			IHexVirtual* m_pHexVirtual { };     //Data handler pointer for EHexDataMode::DATA_VIRTUAL
			SIZE m_sizeLetter { 1, 1 };         //Current font's letter size (width, height).
			CFont m_fontHexView;                //Main Hex chunks font.
			CFont m_fontBottomRect;             //Font for bottom Info rect.
			const std::unique_ptr<CHexDlgSearch> m_pDlgSearch { std::make_unique<CHexDlgSearch>() };             //Search dialog.
			const std::unique_ptr<SCROLLEX::CScrollEx> m_pstScrollV { std::make_unique<SCROLLEX::CScrollEx>() }; //Vertical scroll bar.
			const std::unique_ptr<SCROLLEX::CScrollEx> m_pstScrollH { std::make_unique<SCROLLEX::CScrollEx>() }; //Horizontal scroll bar.
			CMenu m_menuMain;                   //Main popup menu.
			CMenu m_menuShowAs;                 //Submenu "Show as..."
			CBrush m_stBrushBkSelected;         //Brush for "selected" background.
			CPen m_penLines { PS_SOLID, 1, RGB(200, 200, 200) }; //Pen for lines.
			long m_lFontSize { };               //Current font size.
			int m_iSizeFirstHalf { };           //Size of first half of capacity.
			int m_iSizeHexByte { };             //Size of two hex letters representing one byte.
			int m_iIndentAscii { };             //Indent of Ascii text begining.
			int m_iIndentFirstHexChunk { };     //First hex chunk indent.
			int m_iIndentTextCapacityY { };     //Caption text (0 1 2... D E F...) vertical offset.
			const int m_iIndentBottomLine { 1 };//Bottom line indent from window's bottom.
			int m_iDistanceBetweenHexChunks { };//Distance between begining of the two hex chunks.
			int m_iSpaceBetweenHexChunks { };   //Space between Hex chunks.
			int m_iSpaceBetweenAscii { };       //Space between two Ascii chars.
			int m_iSpaceBetweenBlocks { };      //Additional space between hex chunks after half of capacity.
			int m_iHeightTopRect { };           //Height of the header where offsets (0 1 2... D E F...) reside.
			const int m_iHeightBottomRect { 22 }; //Height of bottom Info rect.
			const int m_iHeightBottomOffArea { m_iHeightBottomRect + m_iIndentBottomLine }; //Height of not visible rect from window's bottom to m_iThirdHorizLine.
			int m_iStartWorkArea { };           //Beginning of the area where all drawing occurs.
			int m_iEndWorkArea { };             //End of the area where all drawing occurs.
			int m_iHeightWorkArea { };          //Height of the working area where all drawing occurs.
			const int m_iFirstHorizLine { 0 };  //First horizontal line indent.
			int m_iSecondHorizLine { };         //Second horizontal line indent.
			const int m_iFirstVertLine { 0 };   //First vertical line indent.
			int m_iSecondVertLine { }, m_iThirdVertLine { }, m_iFourthVertLine { }; //Vertical lines indent.
			ULONGLONG m_ullSelectionStart { }, m_ullSelectionEnd { }, m_ullSelectionClick { }, m_ullSelectionSize { };
			std::wstring m_wstrCapacity { };    //Top Capacity string.
			std::wstring m_wstrBottomText { };  //Info text (bottom rect).
			bool m_fLMousePressed { false };    //Is left mouse button pressed.
			DWORD m_dwOffsetDigits { 8 };       //Amount of digits in "Offset", depends on data size set in SetData.
			ULONGLONG m_ullCursorPos { };       //Current cursor position.
			bool m_fCursorHigh { true };        //Cursor's High or Low bits position (first or last digit in hex chunk).
			bool m_fCursorTextArea { false };   //Whether cursor at Ascii or Hex chunks area.
			const DWORD m_dwUndoMax { 500 };    //How many Undo states to preserve.
			std::deque<std::unique_ptr<UNDOSTRUCT>> m_deqUndo; //Undo deque.
			std::deque<std::unique_ptr<UNDOSTRUCT>> m_deqRedo; //Redo deque.
			std::unordered_map<int, HBITMAP> m_umapHBITMAP;    //Images for the Menu.
			bool m_fReplaceWarning { true };    //Show Replace string size exceeds Warning or not.
		};
	}
}

/*******************Setting a manifest for ComCtl32.dll version 6.***********************/
#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif