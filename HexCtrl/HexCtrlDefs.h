/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include <afxwin.h>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#if !defined(__cpp_lib_format) || !defined(__cpp_lib_span) || !defined(__cpp_lib_bit_cast)
#error "C++20 compiler is required for HexCtrl."
#endif

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
		CMD_CLPBRD_COPY_HEX, CMD_CLPBRD_COPY_HEXLE, CMD_CLPBRD_COPY_HEXFMT, CMD_CLPBRD_COPY_TEXTUTF16,
		CMD_CLPBRD_COPY_BASE64, CMD_CLPBRD_COPY_CARR, CMD_CLPBRD_COPY_GREPHEX, CMD_CLPBRD_COPY_PRNTSCRN,
		CMD_CLPBRD_COPY_OFFSET, CMD_CLPBRD_PASTE_HEX, CMD_CLPBRD_PASTE_TEXTUTF16, CMD_CLPBRD_PASTE_TEXTCP,
		CMD_MODIFY_DLG_OPERS, CMD_MODIFY_FILLZEROS, CMD_MODIFY_DLG_FILLDATA, CMD_MODIFY_UNDO, CMD_MODIFY_REDO,
		CMD_SEL_MARKSTART, CMD_SEL_MARKEND, CMD_SEL_ALL, CMD_SEL_ADDLEFT, CMD_SEL_ADDRIGHT, CMD_SEL_ADDUP, CMD_SEL_ADDDOWN,
		CMD_DLG_DATAINTERP, CMD_DLG_ENCODING, CMD_APPEAR_FONTCHOOSE, CMD_APPEAR_FONTINC, CMD_APPEAR_FONTDEC,
		CMD_APPEAR_CAPACINC, CMD_APPEAR_CAPACDEC, CMD_DLG_PRINT, CMD_DLG_ABOUT,
		CMD_CARET_LEFT, CMD_CARET_RIGHT, CMD_CARET_UP, CMD_CARET_DOWN,
		CMD_SCROLL_PAGEUP, CMD_SCROLL_PAGEDOWN
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
	* HEXSPAN - Data offset and size, used in some data/size related routines.                  *
	********************************************************************************************/
	struct HEXSPAN
	{
		ULONGLONG ullOffset { };
		ULONGLONG ullSize { };
	};

	/********************************************************************************************
	* HEXDATAINFO - struct for a data information used in IHexVirtData.                         *
	********************************************************************************************/
	struct HEXDATAINFO
	{
		NMHDR                hdr { };       //Standard Windows header.
		HEXSPAN              stHexSpan { }; //Offset and size of the data bytes.
		std::span<std::byte> spnData { };   //Data span.
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
	* HEXBKM - Bookmarks main struct.                                                           *
	********************************************************************************************/
	struct HEXBKM
	{
		std::vector<HEXSPAN> vecSpan { };                //Vector of offsets and sizes.
		std::wstring         wstrDesc { };               //Bookmark description.
		ULONGLONG            ullID { };                  //Bookmark ID, assigned internally by framework.
		ULONGLONG            ullData { };                //User defined custom data.
		COLORREF             clrBk { RGB(240, 240, 0) }; //Bk color.
		COLORREF             clrText { RGB(0, 0, 0) };   //Text color.
	};
	using PHEXBKM = HEXBKM*;

	/********************************************************************************************
	* IHexBookmarks - Abstract base interface, represents HexCtrl bookmarks.                    *
	********************************************************************************************/
	class IHexBookmarks
	{
	public:
		virtual ULONGLONG AddBkm(const HEXBKM& hbs, bool fRedraw = true) = 0;   //Add new bookmark, returns the new bookmark's ID.
		virtual void ClearAll() = 0;                                            //Clear all bookmarks.
		[[nodiscard]] virtual auto GetByID(ULONGLONG ullID)->PHEXBKM = 0;       //Get bookmark by ID.
		[[nodiscard]] virtual auto GetByIndex(ULONGLONG ullIndex)->PHEXBKM = 0; //Get bookmark by index.
		[[nodiscard]] virtual ULONGLONG GetCount() = 0;                         //Get bookmarks count.
		[[nodiscard]] virtual auto HitTest(ULONGLONG ullOffset)->PHEXBKM = 0;   //HitTest for given offset.
		virtual void RemoveByID(ULONGLONG ullID) = 0;                           //Remove bookmark by a given ID.
	};

	/********************************************************************************************
	* HEXBKMINFO - Bookmarks info.                                                              *
	********************************************************************************************/
	struct HEXBKMINFO
	{
		NMHDR   hdr { };  //Standard Windows header.
		PHEXBKM pBkm { }; //Bookmark pointer.
	};
	using PHEXBKMINFO = HEXBKMINFO*;

	/********************************************************************************************
	* HEXMENUINFO - Menu info.                                                                  *
	********************************************************************************************/
	struct HEXMENUINFO
	{
		NMHDR hdr { };     //Standard Windows header.
		POINT pt { };      //Mouse position when clicked.
		WORD  wMenuID { }; //Menu identifier.
	};
	using PHEXMENUINFO = HEXMENUINFO*;

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
		COLORREF clrFontHex { GetSysColor(COLOR_WINDOWTEXT) };       //Hex-chunks font color.
		COLORREF clrFontText { GetSysColor(COLOR_WINDOWTEXT) };      //Text font color.
		COLORREF clrFontSel { GetSysColor(COLOR_HIGHLIGHTTEXT) };    //Selected hex/text font color.
		COLORREF clrFontDataInterp { RGB(250, 250, 250) };           //Data Interpreter text/hex font color.
		COLORREF clrFontCaption { RGB(0, 0, 180) };                  //Caption font color
		COLORREF clrFontInfoParam { GetSysColor(COLOR_WINDOWTEXT) }; //Font color of the Info bar parameters.
		COLORREF clrFontInfoData { RGB(0, 0, 150) };                 //Font color of the Info bar data.
		COLORREF clrFontCaret { RGB(255, 255, 255) };                //Caret font color.
		COLORREF clrBk { GetSysColor(COLOR_WINDOW) };                //Background color.
		COLORREF clrBkSel { GetSysColor(COLOR_HIGHLIGHT) };          //Background color of the selected Hex/ASCII.
		COLORREF clrBkDataInterp { RGB(147, 58, 22) };               //Data Interpreter Bk color.
		COLORREF clrBkInfoBar { GetSysColor(COLOR_BTNFACE) };        //Background color of the bottom Info bar.
		COLORREF clrBkCaret { RGB(0, 0, 255) };                      //Caret background color.
		COLORREF clrBkCaretSel { RGB(0, 0, 200) };                   //Caret background color in selection.
	};

	/********************************************************************************************
	* HEXCREATE - for IHexCtrl::Create method.                                                  *
	********************************************************************************************/
	struct HEXCREATE
	{
		HEXCOLORS stColor { };          //All HexCtrl colors.
		HWND      hWndParent { };       //Parent window handle.
		PLOGFONTW pLogFont { };         //Monospaced font to be used, or nullptr for default.
		RECT      rect { };             //Initial window rect.
		UINT      uID { };              //Control ID if it's a child window.
		DWORD     dwStyle { };          //Window styles.
		DWORD     dwExStyle { };        //Extended window styles.
		double    dbWheelRatio { 1.0 }; //Ratio for how much to scroll with mouse-wheel.
		bool      fInfoBar { true };    //Show bottom Info bar or not.
		bool      fCustom { false };    //If it's a custom control in a dialog.
	};

	/********************************************************************************************
	* HEXDATA - for IHexCtrl::SetData method.                                                   *
	********************************************************************************************/
	struct HEXDATA
	{
		std::span<std::byte> spnData { };               //Data to display.
		IHexVirtData*        pHexVirtData { };          //Pointer for Virtual mode.
		IHexVirtColors*      pHexVirtColors { };        //Pointer for Custom Colors class.
		DWORD                dwCacheSize { 0x800000U }; //In Virtual mode max cached size of data to fetch.
		bool                 fMutable { false };        //Is data mutable (editable) or read-only.
		bool                 fHighLatency { false };    //Do not redraw window until scrolling completes.
	};

	/********************************************************************************************
	* HEXHITTEST - used in HitTest method.                                                      *
	********************************************************************************************/
	struct HEXHITTEST
	{
		ULONGLONG ullOffset { };     //Offset.
		bool      fIsText { false }; //Is cursor at Text or Hex area.
		bool      fIsHigh { false }; //Is it High or Low part of the byte.
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
		MODIFY_ONCE, MODIFY_REPEAT, MODIFY_OPERATION, MODIFY_RAND_MT19937, MODIFY_RAND_FAST
	};

	/********************************************************************************************
	* EHexOperMode - Data Operation mode, used in EHexModifyMode::MODIFY_OPERATION mode.        *
	********************************************************************************************/
	enum class EHexOperMode : std::uint8_t
	{
		OPER_ASSIGN, OPER_OR, OPER_XOR, OPER_AND, OPER_NOT, OPER_SHL, OPER_SHR, OPER_ROTL,
		OPER_ROTR, OPER_SWAP, OPER_ADD, OPER_SUB, OPER_MUL, OPER_DIV, OPER_CEIL, OPER_FLOOR
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
	* When enModifyMode is set to EHexModifyMode::MODIFY_ONCE, bytes from spnData.data() just   *
	* replace corresponding data bytes as is.                                                   *
	* If enModifyMode is equal to EHexModifyMode::MODIFY_REPEAT                                 *
	* then block by block replacement takes place few times.                                    *
	*   For example : if SUM(vecSpan.ullSize) == 9, spnData.size() == 3 and enModifyMode is set *
	* to EHexModifyMode::MODIFY_REPEAT, bytes in memory at vecSpan.ullOffset position are       *
	* `010203040506070809`, and bytes pointed to by spnData.data() are `030405`, then after     *
	* modification bytes at vecSpan.ullOffset will be `030405030405030405.                      *
	* If enModifyMode is equal to MODIFY_OPERATION then enOperMode comes into play, showing     *
	* what kind of operation must be performed on data, with the enDataSize showing the size.   *
	********************************************************************************************/
	struct HEXMODIFY
	{
		EHexModifyMode       enModifyMode { EHexModifyMode::MODIFY_ONCE }; //Modify mode.
		EHexOperMode         enOperMode { };       //Operation mode, used only in MODIFY_OPERATION mode.
		EHexDataSize         enDataSize { };       //Operation data size.
		std::span<std::byte> spnData { };          //Data span.
		std::vector<HEXSPAN> vecSpan { };          //Vector of data offsets and sizes.
		bool                 fBigEndian { false }; //Treat the data as a big endian, used only in MODIFY_OPERATION mode.
	};
};