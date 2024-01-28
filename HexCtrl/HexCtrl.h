/****************************************************************************************
* Copyright © 2018-2024 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include <afxwin.h>
#include <compare>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <tuple>
#include <vector>

#if !defined(__cpp_lib_format) || !defined(__cpp_lib_span) || !defined(__cpp_lib_bit_cast)
#error "C++20 compliant compiler is required to build HexCtrl."
#endif

#ifdef HEXCTRL_SHARED_DLL
#ifdef HEXCTRL_EXPORT
#define HEXCTRLAPI __declspec(dllexport)
#else //^^^ HEXCTRL_EXPORT / vvv !HEXCTRL_EXPORT
#define HEXCTRLAPI __declspec(dllimport)
#ifdef _WIN64
#ifdef _DEBUG
#define HEXCTRL_LIBNAME(x) x"64d.lib"
#else //^^^ _DEBUG / vvv !_DEBUG
#define HEXCTRL_LIBNAME(x) x"64.lib"
#endif //_DEBUG
#else //^^^ _WIN64 / vvv !_WIN64
#ifdef _DEBUG
#define HEXCTRL_LIBNAME(x) x"d.lib"
#else //^^^ _DEBUG / vvv !_DEBUG
#define HEXCTRL_LIBNAME(x) x".lib"
#endif //_DEBUG
#endif //_WIN64
#pragma comment(lib, HEXCTRL_LIBNAME("HexCtrl"))
#endif //HEXCTRL_EXPORT
#else //^^^ HEXCTRL_SHARED_DLL / vvv !HEXCTRL_SHARED_DLL
#define	HEXCTRLAPI
#endif //HEXCTRL_SHARED_DLL


namespace HEXCTRL {
	constexpr auto HEXCTRL_VERSION_MAJOR = 3;
	constexpr auto HEXCTRL_VERSION_MINOR = 6;
	constexpr auto HEXCTRL_VERSION_PATCH = 0;

	using SpanByte = std::span<std::byte>;
	using SpanCByte = std::span<const std::byte>;

	/********************************************************************************************
	* EHexCmd: Enum of the commands that can be executed within HexCtrl via the ExecuteCmd.     *
	********************************************************************************************/
	enum class EHexCmd : std::uint8_t {
		CMD_SEARCH_DLG = 0x01, CMD_SEARCH_NEXT, CMD_SEARCH_PREV,
		CMD_NAV_GOTO_DLG, CMD_NAV_REPFWD, CMD_NAV_REPBKW, CMD_NAV_DATABEG, CMD_NAV_DATAEND,
		CMD_NAV_PAGEBEG, CMD_NAV_PAGEEND, CMD_NAV_LINEBEG, CMD_NAV_LINEEND, CMD_GROUPDATA_BYTE,
		CMD_GROUPDATA_WORD, CMD_GROUPDATA_DWORD, CMD_GROUPDATA_QWORD, CMD_GROUPDATA_INC, CMD_GROUPDATA_DEC,
		CMD_BKM_ADD, CMD_BKM_REMOVE, CMD_BKM_NEXT, CMD_BKM_PREV, CMD_BKM_REMOVEALL, CMD_BKM_DLG_MGR,
		CMD_CLPBRD_COPY_HEX, CMD_CLPBRD_COPY_HEXLE, CMD_CLPBRD_COPY_HEXFMT, CMD_CLPBRD_COPY_TEXTCP,
		CMD_CLPBRD_COPY_BASE64, CMD_CLPBRD_COPY_CARR, CMD_CLPBRD_COPY_GREPHEX, CMD_CLPBRD_COPY_PRNTSCRN,
		CMD_CLPBRD_COPY_OFFSET, CMD_CLPBRD_PASTE_HEX, CMD_CLPBRD_PASTE_TEXTUTF16, CMD_CLPBRD_PASTE_TEXTCP,
		CMD_MODIFY_OPERS_DLG, CMD_MODIFY_FILLZEROS, CMD_MODIFY_FILLDATA_DLG, CMD_MODIFY_UNDO, CMD_MODIFY_REDO,
		CMD_SEL_MARKSTART, CMD_SEL_MARKEND, CMD_SEL_ALL, CMD_SEL_ADDLEFT, CMD_SEL_ADDRIGHT, CMD_SEL_ADDUP,
		CMD_SEL_ADDDOWN, CMD_DATAINTERP_DLG, CMD_CODEPAGE_DLG, CMD_APPEAR_FONT_DLG, CMD_APPEAR_FONTINC,
		CMD_APPEAR_FONTDEC, CMD_APPEAR_CAPACINC, CMD_APPEAR_CAPACDEC, CMD_PRINT_DLG, CMD_ABOUT_DLG,
		CMD_CARET_LEFT, CMD_CARET_RIGHT, CMD_CARET_UP, CMD_CARET_DOWN,
		CMD_SCROLL_PAGEUP, CMD_SCROLL_PAGEDOWN,
		CMD_TEMPL_APPLYCURR, CMD_TEMPL_DISAPPLY, CMD_TEMPL_DISAPPALL, CMD_TEMPL_DLG_MGR
	};

	/********************************************************************************************
	* EHexWnd: HexCtrl's internal windows.                                                      *
	********************************************************************************************/
	enum class EHexWnd : std::uint8_t {
		WND_MAIN, DLG_BKMMGR, DLG_DATAINTERP, DLG_MODIFY,
		DLG_SEARCH, DLG_CODEPAGE, DLG_GOTO, DLG_TEMPLMGR
	};

	/********************************************************************************************
	* HEXSPAN: Data offset and size, used in some data/size related routines.                   *
	********************************************************************************************/
	struct HEXSPAN {
		ULONGLONG ullOffset { };
		ULONGLONG ullSize { };
	};
	using VecSpan = std::vector<HEXSPAN>;

	/********************************************************************************************
	* HEXCOLOR: Background and Text color struct.                                               *
	********************************************************************************************/
	struct HEXCOLOR {
		COLORREF clrBk { };   //Bk color.
		COLORREF clrText { }; //Text color.
		auto operator<=>(const HEXCOLOR&)const = default;
	};
	using PHEXCOLOR = HEXCOLOR*;

	/********************************************************************************************
	* HEXDATAINFO: Data information used in the IHexVirtData interface.                         *
	********************************************************************************************/
	struct HEXDATAINFO {
		NMHDR    hdr { };       //Standard Windows header.
		HEXSPAN  stHexSpan { }; //Offset and size of the data bytes.
		SpanByte spnData { };   //Data span.
	};

	/********************************************************************************************
	* IHexVirtData: Pure abstract data handler class, that can be implemented by a client,      *
	* to set its own data handler routines.	Pointer to this class is set in the SetData method. *
	********************************************************************************************/
	class IHexVirtData {
	public:
		virtual void OnHexGetData(HEXDATAINFO&) = 0;       //Data to get.
		virtual void OnHexSetData(const HEXDATAINFO&) = 0; //Data to set, if mutable.
	};

	/********************************************************************************************
	* HEXBKM: Bookmarks main struct.                                                            *
	********************************************************************************************/
	struct HEXBKM {
		VecSpan      vecSpan { };  //Vector of offsets and sizes.
		std::wstring wstrDesc { }; //Bookmark description.
		ULONGLONG    ullID { };    //Bookmark ID, assigned internally by framework.
		ULONGLONG    ullData { };  //User defined custom data.
		HEXCOLOR     stClr { };    //Bookmark bk/text color.
	};
	using PHEXBKM = HEXBKM*;

	/********************************************************************************************
	* IHexBookmarks: Pure abstract bookmarks' interface, for the HexCtrl bookmarks machinery.   *
	********************************************************************************************/
	class IHexBookmarks {
	public:
		virtual auto AddBkm(const HEXBKM& hbs, bool fRedraw = true) -> ULONGLONG = 0; //Add new bookmark, returns the new bookmark's ID.
		[[nodiscard]] virtual auto GetByID(ULONGLONG ullID) -> PHEXBKM = 0;           //Get bookmark by ID.
		[[nodiscard]] virtual auto GetByIndex(ULONGLONG ullIndex) -> PHEXBKM = 0;     //Get bookmark by index.
		[[nodiscard]] virtual auto GetCount() -> ULONGLONG = 0;                       //Get bookmarks count.
		[[nodiscard]] virtual auto HitTest(ULONGLONG ullOffset) -> PHEXBKM = 0;       //HitTest for given offset.
		virtual void RemoveAll() = 0;                                                 //Remove all bookmarks.
		virtual void RemoveByID(ULONGLONG ullID) = 0;                                 //Remove by a given ID.
	};

	/********************************************************************************************
	* HEXBKMINFO: Bookmarks info.                                                               *
	********************************************************************************************/
	struct HEXBKMINFO {
		NMHDR   hdr { };  //Standard Windows header.
		PHEXBKM pBkm { }; //Bookmark pointer.
	};
	using PHEXBKMINFO = HEXBKMINFO*;

	/********************************************************************************************
	* HEXMENUINFO: Menu info.                                                                   *
	********************************************************************************************/
	struct HEXMENUINFO {
		NMHDR hdr { };        //Standard Windows header.
		POINT pt { };         //Mouse position when clicked.
		WORD  wMenuID { };    //Menu identifier.
		bool  fShow { true }; //Whether to show menu or not, in case of HEXCTRL_MSG_CONTEXTMENU.
	};
	using PHEXMENUINFO = HEXMENUINFO*;

	/********************************************************************************************
	* HEXCOLORINFO: Struct for HexCtrl custom colors, used in the IHexVirtColors interface.     *
	********************************************************************************************/
	struct HEXCOLORINFO {
		NMHDR     hdr { };       //Standard Windows header.
		ULONGLONG ullOffset { }; //Offset for the color.
		HEXCOLOR  stClr { };     //Colors of the given offset.
	};

	/********************************************************************************************
	* IHexVirtColors: Pure abstract class for HexCtrl custom colors.                            *
	********************************************************************************************/
	class IHexVirtColors {
	public:
		virtual bool OnHexGetColor(HEXCOLORINFO&) = 0; //Should return true if colors are set.
	};

	/********************************************************************************************
	* Templates related data structures, enums, and aliases                                     *
	********************************************************************************************/
	struct HEXTEMPLFIELD;
	using HexPtrField = std::unique_ptr<HEXTEMPLFIELD>;
	using HexVecFields = std::vector<HexPtrField>; //Vector for the Fields.
	using PCHEXTEMPLFIELD = const HEXTEMPLFIELD*;

	//Predefined types of a field.
	enum class EHexFieldType : std::uint8_t {
		custom_size, type_custom,
		type_bool, type_int8, type_uint8, type_int16, type_uint16, type_int32,
		type_uint32, type_int64, type_uint64, type_float, type_double, type_time32,
		type_time64, type_filetime, type_systemtime, type_guid
	};

	//Custom type of a field.
	struct HEXCUSTOMTYPE {
		std::wstring wstrTypeName; //Custom type name.
		std::uint8_t uTypeID { };  //Custom type ID.
	};
	using VecCT = std::vector<HEXCUSTOMTYPE>;

	//Template's field main struct.
	struct HEXTEMPLFIELD {
		std::wstring    wstrName { };     //Field name.
		std::wstring    wstrDescr { };    //Field description.
		int             iOffset { };      //Field offset relative to the Template's beginning.
		int             iSize { };        //Field size.
		HEXCOLOR        stClr { };        //Field Bk and Text color.
		HexVecFields    vecNested { };    //Vector for nested fields.
		PCHEXTEMPLFIELD pFieldParent { }; //Parent field, in case of nested.
		EHexFieldType   eType { };        //Field type.
		std::uint8_t    uTypeID { };      //Field type ID if, it's a custom type.
		bool            fBigEndian { };   //Field endianness.
	};

	//Template main struct.
	struct HEXTEMPLATE {
		std::wstring wstrName;      //Template name.
		HexVecFields vecFields;     //Template fields.
		VecCT        vecCustomType; //Custom types of this template.
		int          iSizeTotal;    //Total size of all Template's fields, assigned internally by framework.
		int          iTemplateID;   //Template ID, assigned by framework.
	};
	using PCHEXTEMPLATE = const HEXTEMPLATE*;

	/********************************************************************************************
	* IHexTemplates: Pure abstract base interface for HexCtrl templates.                        *
	********************************************************************************************/
	class IHexTemplates {
	public:
		virtual auto AddTemplate(const HEXTEMPLATE& stTempl) -> int = 0; //Adds existing template.
		virtual auto ApplyTemplate(ULONGLONG ullOffset, int iTemplateID) -> int = 0; //Applies template to offset, returns AppliedID.
		virtual void DisapplyAll() = 0;
		virtual void DisapplyByID(int iAppliedID) = 0;
		virtual void DisapplyByOffset(ULONGLONG ullOffset) = 0;
		virtual auto LoadTemplate(const wchar_t* pFilePath) -> int = 0; //Returns TemplateID on success, null otherwise.
		virtual void ShowTooltips(bool fShow) = 0;
		virtual void UnloadAll() = 0;                     //Unload all templates.
		virtual void UnloadTemplate(int iTemplateID) = 0; //Unload/remove loaded template from memory.
		[[nodiscard]] static HEXCTRLAPI auto __cdecl LoadFromFile(const wchar_t* pFilePath)->std::unique_ptr<HEXTEMPLATE>;
	};

	/********************************************************************************************
	* HEXCOLORS: HexCtrl internal colors.                                                       *
	********************************************************************************************/
	struct HEXCOLORS {
		COLORREF clrFontHex { GetSysColor(COLOR_WINDOWTEXT) };       //Hex-chunks font color.
		COLORREF clrFontText { GetSysColor(COLOR_WINDOWTEXT) };      //Text font color.
		COLORREF clrFontSel { GetSysColor(COLOR_HIGHLIGHTTEXT) };    //Selected hex/text font color.
		COLORREF clrFontBkm { RGB(0, 0, 0) };                        //Bookmarks font color.
		COLORREF clrFontDataInterp { RGB(250, 250, 250) };           //Data Interpreter text/hex font color.
		COLORREF clrFontCaption { RGB(0, 0, 180) };                  //Caption font color
		COLORREF clrFontInfoParam { GetSysColor(COLOR_WINDOWTEXT) }; //Font color of the Info bar parameters.
		COLORREF clrFontInfoData { RGB(0, 0, 150) };                 //Font color of the Info bar data.
		COLORREF clrFontCaret { RGB(255, 255, 255) };                //Caret font color.
		COLORREF clrBk { GetSysColor(COLOR_WINDOW) };                //Background color.
		COLORREF clrBkSel { GetSysColor(COLOR_HIGHLIGHT) };          //Background color of the selected Hex/Text.
		COLORREF clrBkBkm { RGB(240, 240, 0) };                      //Bookmarks background color.
		COLORREF clrBkDataInterp { RGB(147, 58, 22) };               //Data Interpreter Bk color.
		COLORREF clrBkInfoBar { GetSysColor(COLOR_BTNFACE) };        //Background color of the bottom Info bar.
		COLORREF clrBkCaret { RGB(0, 0, 255) };                      //Caret background color.
		COLORREF clrBkCaretSel { RGB(0, 0, 200) };                   //Caret background color in selection.
	};

	/********************************************************************************************
	* HEXCREATE: Main struct for the HexCtrl creation.                                          *
	********************************************************************************************/
	struct HEXCREATE {
		HWND             hWndParent { };         //Parent window handle.
		const HEXCOLORS* pColors { };            //HexCtrl colors, nullptr for default.
		const LOGFONTW*  pLogFont { };           //Monospaced font for HexCtrl, nullptr for default.
		RECT             rect { };               //Initial window rect.
		UINT             uID { };                //Control ID if it's a child window.
		DWORD            dwStyle { };            //Window styles.
		DWORD            dwExStyle { };          //Extended window styles.
		DWORD            dwCapacity { 16UL };    //Initial capacity size.
		DWORD            dwGroupSize { 1UL };    //Initial data grouping size.
		float            flScrollRatio { 1.0F }; //Either a screen-ratio or lines amount to scroll with Page-scroll (mouse-wheel).
		bool             fScrollLines { false }; //Treat flScrollRatio as screen-ratio (false) or as amount of lines (true).
		bool             fInfoBar { true };      //Show bottom Info bar or not.
		bool             fOffsetHex { true };    //Show offset digits as Hex or Decimal.
		bool             fCustom { false };      //If it's a custom control in a dialog.
	};

	/********************************************************************************************
	* HEXDATA: Main struct for the HexCtrl SetData method.                                      *
	********************************************************************************************/
	struct HEXDATA {
		SpanByte        spnData { };                //Data span to display.
		IHexVirtData*   pHexVirtData { };           //Pointer for VirtualData mode.
		IHexVirtColors* pHexVirtColors { };         //Pointer for Custom Colors class.
		DWORD           dwCacheSize { 0x800000UL }; //Data cache size for VirtualData mode.
		bool            fMutable { false };         //Is data mutable or read-only.
		bool            fHighLatency { false };     //Do not redraw until scroll thumb is released.
	};

	/********************************************************************************************
	* HEXHITTEST: Struct for the HitTest method.                                                *
	********************************************************************************************/
	struct HEXHITTEST {
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
	struct HEXVISION {
		std::int8_t i8Vert { }; //Vertical offset.
		std::int8_t i8Horz { }; //Horizontal offset.
		explicit operator bool()const { return i8Vert == 0 && i8Horz == 0; }; //For test simplicity.
	};

	/********************************************************************************************
	* EHexModifyMode: Enum of the data modification mode, used in HEXMODIFY.                    *
	********************************************************************************************/
	enum class EHexModifyMode : std::uint8_t {
		MODIFY_ONCE, MODIFY_REPEAT, MODIFY_OPERATION, MODIFY_RAND_MT19937, MODIFY_RAND_FAST
	};

	/********************************************************************************************
	* EHexOperMode: Data Operation mode, used in EHexModifyMode::MODIFY_OPERATION mode.         *
	********************************************************************************************/
	enum class EHexOperMode : std::uint8_t {
		OPER_ASSIGN, OPER_ADD, OPER_SUB, OPER_MUL, OPER_DIV, OPER_CEIL, OPER_FLOOR, OPER_OR,
		OPER_XOR, OPER_AND, OPER_NOT, OPER_SHL, OPER_SHR, OPER_ROTL, OPER_ROTR, OPER_SWAP,
		OPER_BITREV
	};

	/********************************************************************************************
	* EHexDataSize: Data size to operate on, used in EHexModifyMode::MODIFY_OPERATION mode.     *
	********************************************************************************************/
	enum class EHexDataSize : std::uint8_t {
		SIZE_BYTE = 0x1U, SIZE_WORD = 0x2U, SIZE_DWORD = 0x4U, SIZE_QWORD = 0x8U
	};

	/********************************************************************************************
	* HEXMODIFY: Main struct to represent data modification parameters.                         *
	* When enModifyMode is set to EHexModifyMode::MODIFY_ONCE, bytes from spnData.data() just   *
	* replace corresponding data bytes as is.                                                   *
	* If enModifyMode is equal to EHexModifyMode::MODIFY_REPEAT                                 *
	* then block by block replacement takes place few times.                                    *
	* For example: if SUM(vecSpan.ullSize) == 9, spnData.size() == 3 and enModifyMode is set    *
	* to EHexModifyMode::MODIFY_REPEAT, bytes in memory at vecSpan.ullOffset position are       *
	* `010203040506070809`, and bytes pointed to by spnData.data() are `030405`, then, after    *
	* a modification, bytes at vecSpan.ullOffset will be `030405030405030405.                   *
	* If enModifyMode is equal to MODIFY_OPERATION, then enOperMode comes into play, showing    *
	* what kind of operation must be performed on data, with the enDataSize showing the size.   *
	********************************************************************************************/
	struct HEXMODIFY {
		EHexModifyMode enModifyMode { EHexModifyMode::MODIFY_ONCE }; //Modify mode.
		EHexOperMode   enOperMode { };       //Operation mode, used only in MODIFY_OPERATION mode.
		EHexDataSize   enDataSize { };       //Operation data size.
		SpanCByte      spnData { };          //Span of the data to modify from.
		VecSpan        vecSpan { };          //Vector of data offsets and sizes.
		bool           fBigEndian { false }; //Treat the data as a big endian, used only in MODIFY_OPERATION mode.
	};


	/********************************************************************************************
	* IHexCtrl: Pure abstract HexCtrl base class.                                               *
	********************************************************************************************/
	class IHexCtrl {
	public:
		IHexCtrl() = default;
		IHexCtrl(const IHexCtrl&) = delete;
		IHexCtrl(IHexCtrl&&) = delete;
		IHexCtrl& operator=(const IHexCtrl&) = delete;
		IHexCtrl& operator=(IHexCtrl&&) = delete;
		virtual void ClearData() = 0; //Clears all data from HexCtrl's view (not touching data itself).
		virtual bool Create(const HEXCREATE& hcs) = 0;                       //Main initialization method.
		virtual bool CreateDialogCtrl(UINT uCtrlID, HWND hWndParent) = 0;    //Сreates custom dialog control.
		virtual void Destroy() = 0;                                          //Deleter.
		virtual void ExecuteCmd(EHexCmd eCmd) = 0;                           //Execute a command within the control.
		[[nodiscard]] virtual auto GetActualWidth()const->int = 0;           //Working area actual width.
		[[nodiscard]] virtual auto GetBookmarks()const->IHexBookmarks* = 0;  //Get Bookmarks interface.
		[[nodiscard]] virtual auto GetCacheSize()const->DWORD = 0;           //Returns VirtualData mode cache size.
		[[nodiscard]] virtual auto GetCapacity()const->DWORD = 0;            //Current capacity.
		[[nodiscard]] virtual auto GetCaretPos()const->ULONGLONG = 0;        //Caret position.
		[[nodiscard]] virtual auto GetCharsExtraSpace()const->DWORD = 0;     //Get extra space between chars, in pixels.
		[[nodiscard]] virtual auto GetCodepage()const->int = 0;              //Get current codepage ID.
		[[nodiscard]] virtual auto GetColors()const->const HEXCOLORS & = 0;  //All current colors.
		[[nodiscard]] virtual auto GetData(HEXSPAN hss)const->SpanByte = 0;  //Get pointer to data offset, no matter what mode the control works in.
		[[nodiscard]] virtual auto GetDataSize()const->ULONGLONG = 0;        //Get currently set data size.
		[[nodiscard]] virtual auto GetDateInfo()const->std::tuple<DWORD, wchar_t> = 0; //Get date format and separator info.
		[[nodiscard]] virtual auto GetDlgData(EHexWnd eWnd)const->std::uint64_t = 0; //Data from the internal dialogs.
		[[nodiscard]] virtual auto GetFont() -> LOGFONTW = 0;                //Get current font.
		[[nodiscard]] virtual auto GetGroupSize()const->DWORD = 0;           //Retrieves current data grouping size.
		[[nodiscard]] virtual auto GetMenuHandle()const->HMENU = 0;          //Context menu handle.
		[[nodiscard]] virtual auto GetPagesCount()const->ULONGLONG = 0;      //Get count of pages.
		[[nodiscard]] virtual auto GetPagePos()const->ULONGLONG = 0;         //Get a page number that the cursor stays at.
		[[nodiscard]] virtual auto GetPageSize()const->DWORD = 0;            //Current page size.
		[[nodiscard]] virtual auto GetScrollRatio()const->std::tuple<float, bool> = 0; //Get current scroll ratio.
		[[nodiscard]] virtual auto GetSelection()const->VecSpan = 0;         //Get current selection.
		[[nodiscard]] virtual auto GetTemplates()const->IHexTemplates* = 0;  //Get Templates interface.
		[[nodiscard]] virtual auto GetUnprintableChar()const->wchar_t = 0;   //Get unprintable replacement character.
		[[nodiscard]] virtual auto GetWndHandle(EHexWnd eWnd, bool fCreate = true)const->HWND = 0; //Get HWND of internal window/dialogs.
		virtual void GoToOffset(ULONGLONG ullOffset, int iRelPos = 0) = 0;   //Go (scroll) to a given offset.
		[[nodiscard]] virtual bool HasSelection()const = 0;    //Does currently have any selection or not.
		[[nodiscard]] virtual auto HitTest(POINT pt, bool fScreen = true)const->std::optional<HEXHITTEST> = 0; //HitTest given point.
		[[nodiscard]] virtual bool IsCmdAvail(EHexCmd eCmd)const = 0; //Is given Cmd currently available (can be executed)?
		[[nodiscard]] virtual bool IsCreated()const = 0;       //Shows whether control is created or not.
		[[nodiscard]] virtual bool IsDataSet()const = 0;       //Shows whether a data was set to the control or not.
		[[nodiscard]] virtual bool IsInfoBar()const = 0;       //Is InfoBar visible?
		[[nodiscard]] virtual bool IsMutable()const = 0;       //Is edit mode enabled or not.
		[[nodiscard]] virtual bool IsOffsetAsHex()const = 0;   //Is "Offset" currently shown as Hex or as Decimal.
		[[nodiscard]] virtual auto IsOffsetVisible(ULONGLONG ullOffset)const->HEXVISION = 0; //Ensures that the given offset is visible.
		[[nodiscard]] virtual bool IsVirtual()const = 0;       //Is working in VirtualData or default mode.		
		virtual void ModifyData(const HEXMODIFY& hms) = 0;     //Main routine to modify data in IsMutable()==true mode.
		virtual void Redraw() = 0;                             //Redraw the control's window.
		virtual void SetCapacity(DWORD dwCapacity) = 0;        //Set the control's current capacity.
		virtual void SetCaretPos(ULONGLONG ullOffset, bool fHighLow = true, bool fRedraw = true) = 0; //Set the caret position.
		virtual void SetCharsExtraSpace(DWORD dwSpace) = 0;    //Extra space to add between chars, in pixels.
		virtual void SetCodepage(int iCodepage) = 0;           //Codepage for text area.
		virtual void SetColors(const HEXCOLORS& hcs) = 0;      //Set all the control's colors.
		virtual bool SetConfig(std::wstring_view wsvPath) = 0; //Set configuration file, or "" for defaults.
		virtual void SetData(const HEXDATA& hds) = 0;          //Main method for setting data to display (and edit).
		virtual void SetDateInfo(DWORD dwFormat, wchar_t wchSepar) = 0; //Set date format and date separator.
		virtual auto SetDlgData(EHexWnd eWnd, std::uint64_t ullData, bool fCreate = true) -> HWND = 0; //Data for the internal dialogs.
		virtual void SetFont(const LOGFONTW& lf) = 0;          //Set the control's new font. This font has to be monospaced.
		virtual void SetGroupSize(DWORD dwSize) = 0;           //Set data grouping size.
		virtual void SetMutable(bool fEnable) = 0;             //Enable or disable mutable/editable mode.
		virtual void SetOffsetMode(bool fHex) = 0;             //Set offset being shown as Hex or as Decimal.
		virtual void SetPageSize(DWORD dwSize, std::wstring_view wsvName = L"Page") = 0; //Set page size and name to draw the lines in-between.
		virtual void SetRedraw(bool fRedraw) = 0;              //Handle WM_PAINT message or not.
		virtual void SetScrollRatio(float flRatio, bool fLines) = 0; //Set mouse-wheel scroll ratio in screens or in lines.
		virtual void SetSelection(const VecSpan& vecSel, bool fRedraw = true, bool fHighlight = false) = 0; //Set current selection.
		virtual void SetUnprintableChar(wchar_t wch) = 0;      //Set unprintable replacement character.
		virtual void SetVirtualBkm(IHexBookmarks* pVirtBkm) = 0; //Set pointer for Bookmarks Virtual Mode.
		virtual void ShowInfoBar(bool fShow) = 0;              //Show/hide bottom Info bar.
	};

#if defined(HEXCTRL_MANUAL_MFC_INIT) || defined(HEXCTRL_SHARED_DLL)
	//Because MFC's PreTranslateMessage routine doesn't work in DLLs 
	//this exported function should be called from an app's main message loop.
	//Something like this:
	// BOOL CMyApp::PreTranslateMessage(MSG* pMsg) {
	//	 if (HexCtrlPreTranslateMessage(pMsg))
	//	 	return TRUE;
	//	 return CWinApp::PreTranslateMessage(pMsg);
	// }
	extern "C" HEXCTRLAPI BOOL __cdecl HexCtrlPreTranslateMessage(MSG * pMsg);
#endif

	/********************************************************************************************
	* Factory function CreateHexCtrl returns IHexCtrlUnPtr, a unique_ptr with a custom deleter. *
	* If you, for some reason, need a raw pointer you can directly call the CreateRawHexCtrl    *
	* function, which returns IHexCtrl* interface pointer. But in this case you will need to    *
	* call IHexCtrl::Destroy method	afterwards, to manually delete created raw HexCtrl object.  *
	********************************************************************************************/

	extern "C" [[nodiscard]] HEXCTRLAPI IHexCtrl * __cdecl CreateRawHexCtrl();
	using IHexCtrlPtr = std::unique_ptr < IHexCtrl, decltype([](IHexCtrl* p) { p->Destroy(); }) > ;
	[[nodiscard]] inline IHexCtrlPtr CreateHexCtrl() { return IHexCtrlPtr { CreateRawHexCtrl() }; };

	/********************************************************************************************
	* WM_NOTIFY message codes (NMHDR.code values).                                              *
	* These codes are used to notify a parent window about HexCtrl's states.                    *
	********************************************************************************************/

	constexpr auto HEXCTRL_MSG_BKMCLICK { 0x0100U };      //Bookmark is clicked.
	constexpr auto HEXCTRL_MSG_CONTEXTMENU { 0x0101U };   //OnContextMenu has triggered.
	constexpr auto HEXCTRL_MSG_DESTROY { 0x0102U };       //Indicates that the HexCtrl is being destroyed.
	constexpr auto HEXCTRL_MSG_DLGBKMMGR { 0x0103U };     //"Bookmark manager" dialog is about to be displayed.
	constexpr auto HEXCTRL_MSG_DLGCODEPAGE { 0x0104U };   //"Codepage" dialog is about to be displayed.
	constexpr auto HEXCTRL_MSG_DLGDATAINTERP { 0x0105U }; //"Data Interpreter" dialog is about to be displayed.
	constexpr auto HEXCTRL_MSG_DLGGOTO { 0x0106U };       //"GoTo" dialog is about to be displayed.
	constexpr auto HEXCTRL_MSG_DLGMODIFY { 0x0107U };     //"Modify Data" dialog is about to be displayed.
	constexpr auto HEXCTRL_MSG_DLGSEARCH { 0x0108U };     //"Search" dialog is about to be displayed.
	constexpr auto HEXCTRL_MSG_DLGTEMPLMGR { 0x0109U };   //"Template manager" dialog is about to be displayed.
	constexpr auto HEXCTRL_MSG_MENUCLICK { 0x010AU };     //User defined custom menu has clicked.
	constexpr auto HEXCTRL_MSG_SETCAPACITY { 0x010BU };   //Capacity has changed.
	constexpr auto HEXCTRL_MSG_SETCARET { 0x010CU };      //Caret position has changed.
	constexpr auto HEXCTRL_MSG_SETCODEPAGE { 0x010DU };   //Codepage has changed.
	constexpr auto HEXCTRL_MSG_SETDATA { 0x010EU };       //Indicates that the data has changed.
	constexpr auto HEXCTRL_MSG_SETFONT { 0x010FU };       //Font has changed.
	constexpr auto HEXCTRL_MSG_SETGROUPSIZE { 0x0110U };  //Data grouping size has changed.
	constexpr auto HEXCTRL_MSG_SETSELECTION { 0x0111U };  //Selection has been made.


	/********************************************************************
	* Flags for the internal dialogs, used with the SetDlgData method.  *
	********************************************************************/

	//Flags common for all dialogs.
	constexpr auto HEXCTRL_FLAG_NOESC { 0x8000000000000000ULL };

	//Template Manager.
	constexpr auto HEXCTRL_FLAG_TEMPLMGR_MINIMIZED { 0x1ULL };
	constexpr auto HEXCTRL_FLAG_TEMPLMGR_HEXNUM { 0x2ULL };
	constexpr auto HEXCTRL_FLAG_TEMPLMGR_SHOWTT { 0x4ULL };
	constexpr auto HEXCTRL_FLAG_TEMPLMGR_HGLSEL { 0x8ULL };
	constexpr auto HEXCTRL_FLAG_TEMPLMGR_SWAPENDIAN { 0x10ULL };

	//Data Interpreter.
	constexpr auto HEXCTRL_FLAG_DATAINTERP_HEXNUM { 0x1ULL };
	constexpr auto HEXCTRL_FLAG_DATAINTERP_BE { 0x2ULL };

	//Bookmark Manager.
	constexpr auto HEXCTRL_FLAG_BKMMGR_HEXNUM { 0x1ULL };

	//Setting a manifest for the ComCtl32.dll version 6.
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
}