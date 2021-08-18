/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
****************************************************************************************/
#pragma once
#include <Windows.h>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace HEXCTRL
{
	/********************************************************************************************
	* EHexCmd - Enum of the commands that can be executed within HexCtrl, used in ExecuteCmd.   *
	********************************************************************************************/
	enum class EHexCmd : std::uint8_t
	{
		CMD_DLG_SEARCH = 0x01, CMD_SEARCH_NEXT, CMD_SEARCH_PREV,
		CMD_NAV_DLG_GOTO, CMD_NAV_REPFWD, CMD_NAV_REPBKW, CMD_NAV_DATABEG, CMD_NAV_DATAEND,
		CMD_NAV_PAGEBEG, CMD_NAV_PAGEEND, CMD_NAV_LINEBEG, CMD_NAV_LINEEND,
		CMD_GROUPBY_BYTE, CMD_GROUPBY_WORD, CMD_GROUPBY_DWORD, CMD_GROUPBY_QWORD,
		CMD_BKM_ADD, CMD_BKM_REMOVE, CMD_BKM_NEXT, CMD_BKM_PREV, CMD_BKM_CLEARALL, CMD_BKM_DLG_MANAGER,
		CMD_CLPBRD_COPYHEX, CMD_CLPBRD_COPYHEXLE, CMD_CLPBRD_COPYHEXFMT, CMD_CLPBRD_COPYTEXT,
		CMD_CLPBRD_COPYBASE64, CMD_CLPBRD_COPYCARR, CMD_CLPBRD_COPYGREPHEX, CMD_CLPBRD_COPYPRNTSCRN,
		CMD_CLPBRD_COPYOFFSET, CMD_CLPBRD_PASTEHEX, CMD_CLPBRD_PASTETEXT,
		CMD_MODIFY_DLG_OPERS, CMD_MODIFY_FILLZEROS, CMD_MODIFY_DLG_FILLDATA, CMD_MODIFY_UNDO, CMD_MODIFY_REDO,
		CMD_SEL_MARKSTART, CMD_SEL_MARKEND, CMD_SEL_ALL, CMD_SEL_ADDLEFT, CMD_SEL_ADDRIGHT, CMD_SEL_ADDUP, CMD_SEL_ADDDOWN,
		CMD_DLG_DATAINTERP, CMD_DLG_ENCODING,
		CMD_APPEAR_FONTINC, CMD_APPEAR_FONTDEC, CMD_APPEAR_CAPACINC, CMD_APPEAR_CAPACDEC,
		CMD_DLG_PRINT, CMD_DLG_ABOUT,
		CMD_CARET_LEFT, CMD_CARET_RIGHT, CMD_CARET_UP, CMD_CARET_DOWN,
		CMD_SCROLL_PAGEUP, CMD_SCROLL_PAGEDOWN
	};

	/********************************************************************************************
	* EHexCreateMode - Enum of HexCtrl creation mode.                                           *
	********************************************************************************************/
	enum class EHexCreateMode : std::uint8_t
	{
		CREATE_CHILD, CREATE_POPUP, CREATE_CUSTOMCTRL
	};

	/********************************************************************************************
	* EHexWnd - HexControl's windows.                                                           *
	********************************************************************************************/
	enum class EHexWnd : std::uint8_t
	{
		WND_MAIN, DLG_BKMMANAGER, DLG_DATAINTERP, DLG_FILLDATA,
		DLG_OPERS, DLG_SEARCH, DLG_ENCODING, DLG_GOTO
	};

	/********************************************************************************************
	* HEXOFFSET - Data offset and size, used in some data/size related routines.                *
	********************************************************************************************/
	struct HEXOFFSET
	{
		ULONGLONG ullOffset { };
		ULONGLONG ullSize { };
	};

	/********************************************************************************************
	* HEXDATAINFO - struct for a data information used in IHexVirtData.                         *
	********************************************************************************************/
	struct HEXDATAINFO
	{
		NMHDR                hdr { };      //Standard Windows header.
		HEXOFFSET            stOffset { }; //Offset and size of the data bytes.
		std::span<std::byte> spnData { };  //Data span.
	};

	/********************************************************************************************
	* IHexVirtData - Pure abstract data handler class, that can be implemented by client,       *
	* to set its own data handler routines.	Pointer to this class can be set in SetData method. *
	* All virtual functions must be defined in client's derived class.                          *
	********************************************************************************************/
	class IHexVirtData
	{
	public:
		virtual void OnHexGetData(HEXDATAINFO&) = 0;       //Data to get.
		virtual void OnHexSetData(const HEXDATAINFO&) = 0; //Data to set, if mutable.
	};

	/********************************************************************************************
	* HEXBKM - Bookmarks.                                                                       *
	********************************************************************************************/
	struct HEXBKM
	{
		std::vector<HEXOFFSET> vecOffset { };              //Vector of offsets and sizes.
		std::wstring           wstrDesc { };               //Bookmark description.
		ULONGLONG              ullID { };                  //Bookmark ID, assigned internally by framework.
		ULONGLONG              ullData { };                //User defined custom data.
		COLORREF               clrBk { RGB(240, 240, 0) }; //Bk color.
		COLORREF               clrText { RGB(0, 0, 0) };   //Text color.
	};
	using PHEXBKM = HEXBKM*;

	/********************************************************************************************
	* IHexVirtBkm - Pure abstract class for virtual bookmarks.                                  *
	********************************************************************************************/
	class IHexVirtBkm
	{
	public:
		virtual ULONGLONG OnHexBkmAdd(const HEXBKM& stBookmark) = 0; //Add new bookmark, return new bookmark's ID.
		virtual void OnHexBkmClearAll() = 0; //Clear all bookmarks.
		[[nodiscard]] virtual ULONGLONG OnHexBkmGetCount() = 0; //Get total bookmarks count.
		[[nodiscard]] virtual auto OnHexBkmGetByID(ULONGLONG ullID)->HEXBKM* = 0; //Bookmark by ID.
		[[nodiscard]] virtual auto OnHexBkmGetByIndex(ULONGLONG ullIndex)->HEXBKM* = 0; //Bookmark by index (in inner list).
		[[nodiscard]] virtual auto OnHexBkmHitTest(ULONGLONG ullOffset)->HEXBKM* = 0;   //Does given offset have a bookmark?
		virtual void OnHexBkmRemoveByID(ULONGLONG ullID) = 0; //Remove bookmark by given ID (returned by Add()).
	};

	/********************************************************************************************
	* HEXBKMINFO - Bookmarks info.                                                              *
	********************************************************************************************/
	struct HEXBKMINFO
	{
		NMHDR   hdr { };  //Standard Windows header.
		PHEXBKM pBkm { }; //Bookmark pointer.
	};

	/********************************************************************************************
	* HEXMENUINFO - Menu info.                                                                  *
	********************************************************************************************/
	struct HEXMENUINFO
	{
		NMHDR hdr { };     //Standard Windows header.
		POINT pt { };      //Mouse position when clicked.
		WORD  wMenuID { }; //Menu identifier.
	};

	/********************************************************************************************
	* HEXCOLOR - used with the IHexVirtColors interface.                                        *
	********************************************************************************************/
	struct HEXCOLOR
	{
		COLORREF clrBk { };   //Bk color.
		COLORREF clrText { }; //Text color.
	};
	using PHEXCOLOR = HEXCOLOR*;

	/********************************************************************************************
	* HEXCOLORINFO - struct for hex chunks' color information.                                  *
	********************************************************************************************/
	struct HEXCOLORINFO
	{
		NMHDR     hdr { };       //Standard Windows header.
		ULONGLONG ullOffset { }; //Offset for the color.
		PHEXCOLOR pClr { };      //Pointer to the color struct.
	};

	/********************************************************************************************
	* IHexVirtColors - Pure abstract class for chunk colors.                                    *
	********************************************************************************************/
	class IHexVirtColors
	{
	public:
		virtual void OnHexGetColor(HEXCOLORINFO&) = 0;
	};

	/********************************************************************************************
	* HEXCOLORS - All HexCtrl colors.                                                           *
	********************************************************************************************/
	struct HEXCOLORS
	{
		COLORREF clrTextHex { GetSysColor(COLOR_WINDOWTEXT) };       //Hex chunks text color.
		COLORREF clrTextASCII { GetSysColor(COLOR_WINDOWTEXT) };     //ASCII text color.
		COLORREF clrTextSelect { GetSysColor(COLOR_HIGHLIGHTTEXT) }; //Selected text color.
		COLORREF clrTextDataInterp { RGB(250, 250, 250) };           //Data Interpreter text color.
		COLORREF clrTextCaption { RGB(0, 0, 180) };                  //Caption text color
		COLORREF clrTextInfoRect { GetSysColor(COLOR_WINDOWTEXT) };  //Text color of the bottom "Info" rect.
		COLORREF clrTextCaret { RGB(255, 255, 255) };                //Caret text color.
		COLORREF clrBk { GetSysColor(COLOR_WINDOW) };                //Background color.
		COLORREF clrBkSelect { GetSysColor(COLOR_HIGHLIGHT) };       //Background color of the selected Hex/ASCII.
		COLORREF clrBkDataInterp { RGB(147, 58, 22) };               //Data Interpreter Bk color.
		COLORREF clrBkInfoRect { GetSysColor(COLOR_BTNFACE) };       //Background color of the bottom "Info" rect.
		COLORREF clrBkCaret { RGB(0, 0, 255) };                      //Caret background color.
		COLORREF clrBkCaretSelect { RGB(0, 0, 200) };                //Caret background color in selection.
	};

	/********************************************************************************************
	* HEXCREATE - for IHexCtrl::Create method.                                                  *
	********************************************************************************************/
	struct HEXCREATE
	{
		EHexCreateMode enCreateMode { EHexCreateMode::CREATE_CHILD }; //Creation mode of the HexCtrl window.
		HEXCOLORS      stColor { };          //All the control's colors.
		HWND           hwndParent { };       //Parent window handle.
		PLOGFONTW      pLogFont { };         //Font to be used instead of default. This font has to be monospaced.
		RECT           rect { };             //Initial rect. If null, the window is screen centered.
		UINT           uID { };              //Control ID.
		DWORD          dwStyle { };          //Window styles, 0 for default.
		DWORD          dwExStyle { };        //Extended window styles, 0 for default.
		double         dbWheelRatio { 1.0 }; //Ratio for how much to scroll with mouse-wheel.
	};

	/********************************************************************************************
	* HEXDATA - for IHexCtrl::SetData method.                                                   *
	********************************************************************************************/
	struct HEXDATA
	{
		std::span<std::byte> spnData { };              //Data to display.
		IHexVirtData*        pHexVirtData { };         //Pointer for Virtual mode.
		IHexVirtColors*      pHexVirtColors { };       //Pointer for Custom Colors class.
		DWORD                dwCacheSize { 0x800000 }; //In Virtual mode max cached size of data to fetch.
		bool                 fMutable { false };       //Is data mutable (editable) or read-only.
		bool                 fHighLatency { false };   //Do not redraw window until scrolling completes.
	};

	/********************************************************************************************
	* HEXHITTEST - used in HitTest method.                                                      *
	********************************************************************************************/
	struct HEXHITTEST
	{
		ULONGLONG ullOffset { };      //Offset.
		bool      fIsAscii { false }; //Is cursor at ASCII part or at Hex.
		bool      fIsHigh { false };  //Is it High or Low part of the byte.
	};

	/********************************************************************************************
	* HEXVISION - Offset visibility struct, used in IsOffsetVisible method.                     *
	* -1 - Offset is higher, or at the left, of the visible area.                               *
	*  1 - lower, or at the right.                                                              *
	*  0 - visible.                                                                             *
	********************************************************************************************/
	struct HEXVISION
	{
		std::int8_t i8Vert { }; //Vertical offset.
		std::int8_t i8Horz { }; //Horizontal offset.
		operator bool()const { return i8Vert == 0 && i8Horz == 0; }; //For test simplicity: if(IsOffsetVisible()).
	};

	/********************************************************************************************
	* EHexModifyMode - Enum of the data modification mode, used in HEXMODIFY.                   *
	********************************************************************************************/
	enum class EHexModifyMode : std::uint8_t
	{
		MODIFY_DEFAULT, MODIFY_REPEAT, MODIFY_OPERATION, MODIFY_RANDOM
	};

	/********************************************************************************************
	* EHexOperMode - Data Operation mode, used in EHexModifyMode::MODIFY_OPERATION mode.        *
	********************************************************************************************/
	enum class EHexOperMode : std::uint8_t
	{
		OPER_ASSIGN, OPER_OR, OPER_XOR, OPER_AND, OPER_NOT, OPER_SHL, OPER_SHR, OPER_ROTL,
		OPER_ROTR, OPER_SWAP, OPER_ADD, OPER_SUBTRACT, OPER_MULTIPLY, OPER_DIVIDE,
		OPER_CEILING, OPER_FLOOR
	};

	/********************************************************************************************
	* EHexDataSize - Data size to operate on, used in EHexModifyMode::MODIFY_OPERATION mode.    *
	* Also used to set data grouping mode, in SetGroupMode method.                              *
	********************************************************************************************/
	enum class EHexDataSize : std::uint8_t
	{
		SIZE_BYTE = 0x01, SIZE_WORD = 0x02, SIZE_DWORD = 0x04, SIZE_QWORD = 0x08
	};

	/********************************************************************************************
	* HEXMODIFY - used to represent data modification parameters.                               *
	* When enModifyMode is set to EHexModifyMode::MODIFY_DEFAULT, bytes from pData just replace *
	* corresponding data bytes as is. If enModifyMode is equal to EHexModifyMode::MODIFY_REPEAT *
	* then block by block replacement takes place few times.                                    *
	*   For example : if SUM(vecOffset.ullSize) = 9, ullDataSize = 3 and enModifyMode is set to *
	* EHexModifyMode::MODIFY_REPEAT, bytes in memory at vecOffset.ullOffset position are        *
	* 123456789, and bytes pointed to by spnData are 345, then, after modification, bytes at    *
	* vecOffset.ullOffset will be 345345345.                                                    *
	* If enModifyMode is equal to MODIFY_OPERATION then enOperMode comes into play, showing     *
	* what kind of operation must be performed on data, with the enOperSize showing the size.   *
	********************************************************************************************/
	struct HEXMODIFY
	{
		EHexModifyMode         enModifyMode { EHexModifyMode::MODIFY_DEFAULT }; //Modify mode.
		EHexOperMode           enOperMode { };       //Operation mode, used only in MODIFY_OPERATION mode.
		EHexDataSize           enOperSize { };       //Operation data size.
		std::span<std::byte>   spnData { };          //Data span.
		std::vector<HEXOFFSET> vecOffset { };        //Vector of data offsets and sizes.
		bool                   fBigEndian { false }; //Treat the data as a big endian, used only in MODIFY_OPERATION mode.
	};
};