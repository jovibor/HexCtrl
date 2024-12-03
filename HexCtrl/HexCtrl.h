/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
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
#endif //^^^ !_DEBUG
#else //^^^ _WIN64 / vvv !_WIN64
#ifdef _DEBUG
#define HEXCTRL_LIBNAME(x) x"d.lib"
#else //^^^ _DEBUG / vvv !_DEBUG
#define HEXCTRL_LIBNAME(x) x".lib"
#endif //^^^ !_DEBUG
#endif //^^^ !_WIN64
#pragma comment(lib, HEXCTRL_LIBNAME("HexCtrl"))
#endif //^^^ !HEXCTRL_EXPORT
#else //^^^ HEXCTRL_SHARED_DLL / vvv !HEXCTRL_SHARED_DLL
#define	HEXCTRLAPI
#endif //^^^ !HEXCTRL_SHARED_DLL

namespace HEXCTRL {
	constexpr auto HEXCTRL_VERSION_MAJOR = 3;
	constexpr auto HEXCTRL_VERSION_MINOR = 7;
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
	* EHexDlgItem: HexCtrl's internal dialogs' items.                                           *
	********************************************************************************************/
	enum class EHexDlgItem : std::uint8_t {
		BKMMGR_CHK_HEX, DATAINTERP_CHK_HEX, DATAINTERP_CHK_BE, TEMPLMGR_CHK_MIN,
		TEMPLMGR_CHK_TT, TEMPLMGR_CHK_HGL, TEMPLMGR_CHK_HEX, TEMPLMGR_CHK_SWAP,
		SEARCH_COMBO_FIND, SEARCH_COMBO_REPLACE, SEARCH_EDIT_START, SEARCH_EDIT_STEP,
		SEARCH_EDIT_RNGBEG, SEARCH_EDIT_RNGEND, SEARCH_EDIT_LIMIT, FILLDATA_COMBO_DATA
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
		NMHDR    hdr { };   //Standard Windows header.
		HEXSPAN  stHexSpan; //Offset and size of the data.
		SpanByte spnData;   //Data span.
	};

	/********************************************************************************************
	* IHexVirtData: Pure abstract data handler class, that can be implemented by a client,      *
	* to set its own data handler routines.	Pointer to this class is set in the SetData method. *
	********************************************************************************************/
	class IHexVirtData {
	public:
		virtual void OnHexGetData(HEXDATAINFO&) = 0; //Data to get.
		virtual void OnHexGetOffset(HEXDATAINFO& hdi, bool fGetVirt) = 0; //Offset<->VirtOffset conversion.
		virtual void OnHexSetData(const HEXDATAINFO&) = 0; //Data to set, if mutable.
	};

	/********************************************************************************************
	* HEXBKM: Bookmarks main struct.                                                            *
	********************************************************************************************/
	struct HEXBKM {
		VecSpan      vecSpan;     //Vector of offsets and sizes.
		std::wstring wstrDesc;    //Bookmark description.
		ULONGLONG    ullID { };   //Bookmark ID, assigned internally by framework.
		ULONGLONG    ullData { }; //User defined custom data.
		HEXCOLOR     stClr;       //Bookmark bk/text color.
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
		HEXCOLOR  stClr;         //Colors of the given offset.
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
		std::wstring    wstrName;             //Field name.
		std::wstring    wstrDescr;            //Field description.
		int             iOffset { };          //Field offset relative to the Template's beginning.
		int             iSize { };            //Field size.
		HEXCOLOR        stClr;                //Field Bk and Text color.
		HexVecFields    vecNested;            //Vector for nested fields.
		PCHEXTEMPLFIELD pFieldParent { };     //Parent field, in case of nested.
		EHexFieldType   eType { };            //Field type.
		std::uint8_t    uTypeID { };          //Field type ID if, it's a custom type.
		bool            fBigEndian { false }; //Field endianness.
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
		virtual auto AddTemplate(const HEXTEMPLATE& hts) -> int = 0; //Adds existing template.
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
	using PCHEXCOLORS = const HEXCOLORS*;

	/********************************************************************************************
	* HEXCREATE: Main struct for the HexCtrl creation.                                          *
	********************************************************************************************/
	struct HEXCREATE {
		HWND            hWndParent { };         //Parent window handle.
		PCHEXCOLORS     pColors { };            //HexCtrl colors, nullptr for default.
		const LOGFONTW* pLogFont { };           //Monospaced font for HexCtrl, nullptr for default.
		RECT            rect { };               //Initial window rect.
		UINT            uID { };                //Control ID if it's a child window.
		DWORD           dwStyle { };            //Window styles.
		DWORD           dwExStyle { };          //Extended window styles.
		DWORD           dwCapacity { 16UL };    //Initial capacity size.
		DWORD           dwGroupSize { 1UL };    //Initial data grouping size.
		float           flScrollRatio { 1.0F }; //Either a screen-ratio or lines amount to scroll with Page-scroll (mouse-wheel).
		bool            fScrollLines { false }; //Treat flScrollRatio as screen-ratio (false) or as amount of lines (true).
		bool            fInfoBar { true };      //Show bottom Info bar or not.
		bool            fOffsetHex { true };    //Show offset digits as Hex or Decimal.
		bool            fCustom { false };      //If it's a custom control in a dialog.
	};

	/********************************************************************************************
	* HEXDATA: Main struct for the HexCtrl SetData method.                                      *
	********************************************************************************************/
	struct HEXDATA {
		SpanByte        spnData;                    //Data span to display.
		IHexVirtData*   pHexVirtData { };           //Pointer for VirtualData mode.
		IHexVirtColors* pHexVirtColors { };         //Pointer for Custom Colors class.
		ULONGLONG       ullMaxVirtOffset { };       //Maximum virtual offset.
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
	* EHexModifyMode: Enum of the data modification mode, used in the HEXMODIFY.                *
	********************************************************************************************/
	enum class EHexModifyMode : std::uint8_t {
		MODIFY_ONCE, MODIFY_REPEAT, MODIFY_OPERATION, MODIFY_RAND_MT19937, MODIFY_RAND_FAST
	};

	/********************************************************************************************
	* EHexOperMode: Data Operation mode, used in the EHexModifyMode::MODIFY_OPERATION mode.     *
	********************************************************************************************/
	enum class EHexOperMode : std::uint8_t {
		OPER_ASSIGN, OPER_ADD, OPER_SUB, OPER_MUL, OPER_DIV, OPER_MIN, OPER_MAX, OPER_OR,
		OPER_XOR, OPER_AND, OPER_NOT, OPER_SHL, OPER_SHR, OPER_ROTL, OPER_ROTR, OPER_SWAP,
		OPER_BITREV
	};

	/********************************************************************************************
	* EHexDataType: Data type, used in the EHexModifyMode::MODIFY_OPERATION mode.               *
	********************************************************************************************/
	enum class EHexDataType : std::uint8_t {
		DATA_INT8, DATA_UINT8, DATA_INT16, DATA_UINT16, DATA_INT32,
		DATA_UINT32, DATA_INT64, DATA_UINT64, DATA_FLOAT, DATA_DOUBLE
	};

	/********************************************************************************************
	* HEXMODIFY: Main struct to represent data modification parameters.                         *
	* When eModifyMode is set to MODIFY_ONCE, bytes from spnData.data() just replace            *
	* corresponding data bytes as is.                                                           *
	* If eModifyMode is equal to MODIFY_REPEAT then block by block replacement takes place      *
	* few times.                                                                                *
	* For example: if SUM(vecSpan.ullSize) == 9, spnData.size() == 3 and eModifyMode is set     *
	* to MODIFY_REPEAT, bytes in memory at vecSpan.ullOffset position are `010203040506070809`  *
	* and bytes pointed to by spnData.data() are `030405`, then after the modification, bytes   *
	* at vecSpan.ullOffset will be `030405030405030405.                                         *
	* If eModifyMode is equal to MODIFY_OPERATION, then eOperMode comes into play, showing      *
	* what kind of operation must be performed on the data.                                     *
	********************************************************************************************/
	struct HEXMODIFY {
		EHexModifyMode eModifyMode { };      //Modify mode.
		EHexOperMode   eOperMode { };        //Operation mode, used if eModifyMode == MODIFY_OPERATION.
		EHexDataType   eDataType { };        //Data type of the underlying data, used if eModifyMode == MODIFY_OPERATION.
		SpanCByte      spnData;              //Span of the data to modify with.
		VecSpan        vecSpan;              //Vector of data offsets and sizes to modify.
		bool           fBigEndian { false }; //Treat data as the big endian, used if eModifyMode == MODIFY_OPERATION.
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
		virtual ~IHexCtrl() = default;
		virtual void ClearData() = 0; //Clears all data from HexCtrl's view (not touching data itself).
		virtual bool Create(const HEXCREATE& hcs) = 0;                       //Main initialization method.
		virtual bool CreateDialogCtrl(UINT uCtrlID, HWND hWndParent) = 0;    //Сreates custom dialog control.
		virtual void Destroy() = 0;                                          //Deleter.
		virtual void ExecuteCmd(EHexCmd eCmd) = 0;                           //Execute a command within HexCtrl.
		[[nodiscard]] virtual auto GetActualWidth()const->int = 0;           //Working area actual width.
		[[nodiscard]] virtual auto GetBookmarks()const->IHexBookmarks* = 0;  //Get Bookmarks interface.
		[[nodiscard]] virtual auto GetCacheSize()const->DWORD = 0;           //Returns VirtualData mode cache size.
		[[nodiscard]] virtual auto GetCapacity()const->DWORD = 0;            //Current capacity.
		[[nodiscard]] virtual auto GetCaretPos()const->ULONGLONG = 0;        //Caret position.
		[[nodiscard]] virtual auto GetCharsExtraSpace()const->DWORD = 0;     //Get extra space between chars, in pixels.
		[[nodiscard]] virtual auto GetCodepage()const->int = 0;              //Get current codepage ID.
		[[nodiscard]] virtual auto GetColors()const->const HEXCOLORS & = 0;  //All current colors.
		[[nodiscard]] virtual auto GetData(HEXSPAN hss)const->SpanByte = 0;  //Get pointer to data offset, no matter what mode HexCtrl works in.
		[[nodiscard]] virtual auto GetDataSize()const->ULONGLONG = 0;        //Get currently set data size.
		[[nodiscard]] virtual auto GetDateInfo()const->std::tuple<DWORD, wchar_t> = 0; //Get date format and separator info.
		[[nodiscard]] virtual auto GetDlgItemHandle(EHexWnd eWnd, EHexDlgItem eItem)const->HWND = 0; //Dialogs' items.
		[[nodiscard]] virtual auto GetFont()const->LOGFONTW = 0;             //Get current font.
		[[nodiscard]] virtual auto GetGroupSize()const->DWORD = 0;           //Retrieves current data grouping size.
		[[nodiscard]] virtual auto GetMenuHandle()const->HMENU = 0;          //Context menu handle.
		[[nodiscard]] virtual auto GetOffset(ULONGLONG ullOffset, bool fGetVirt)const->ULONGLONG = 0; //Offset<->VirtOffset conversion.
		[[nodiscard]] virtual auto GetPagesCount()const->ULONGLONG = 0;      //Get count of pages.
		[[nodiscard]] virtual auto GetPagePos()const->ULONGLONG = 0;         //Get a page number that the cursor stays at.
		[[nodiscard]] virtual auto GetPageSize()const->DWORD = 0;            //Current page size.
		[[nodiscard]] virtual auto GetScrollRatio()const->std::tuple<float, bool> = 0; //Get current scroll ratio.
		[[nodiscard]] virtual auto GetSelection()const->VecSpan = 0;         //Get current selection.
		[[nodiscard]] virtual auto GetTemplates()const->IHexTemplates* = 0;  //Get Templates interface.
		[[nodiscard]] virtual auto GetUnprintableChar()const->wchar_t = 0;   //Get unprintable replacement character.
		[[nodiscard]] virtual auto GetWndHandle(EHexWnd eWnd, bool fCreate = true)const->HWND = 0; //Get HWND of internal window/dialogs.
		virtual void GoToOffset(ULONGLONG ullOffset, int iPosAt = 0) = 0;    //Go to the given offset.
		[[nodiscard]] virtual bool HasSelection()const = 0;    //Does currently have any selection or not.
		[[nodiscard]] virtual auto HitTest(POINT pt, bool fScreen = true)const->std::optional<HEXHITTEST> = 0; //HitTest given point.
		[[nodiscard]] virtual bool IsCmdAvail(EHexCmd eCmd)const = 0; //Is given Cmd currently available (can be executed)?
		[[nodiscard]] virtual bool IsCreated()const = 0;       //Shows whether HexCtrl is created or not.
		[[nodiscard]] virtual bool IsDataSet()const = 0;       //Shows whether a data was set to HexCtrl or not.
		[[nodiscard]] virtual bool IsInfoBar()const = 0;       //Is InfoBar visible?
		[[nodiscard]] virtual bool IsMutable()const = 0;       //Is data mutable or not.
		[[nodiscard]] virtual bool IsOffsetAsHex()const = 0;   //Are offsets shown as Hex or as Decimal.
		[[nodiscard]] virtual auto IsOffsetVisible(ULONGLONG ullOffset)const->HEXVISION = 0; //Ensures that the given offset is visible.
		[[nodiscard]] virtual bool IsVirtual()const = 0;       //Is working in VirtualData or default mode.		
		virtual void ModifyData(const HEXMODIFY& hms) = 0;     //Main routine to modify data in IsMutable()==true mode.
		virtual void Redraw() = 0;                             //Redraw HexCtrl's window.
		virtual void SetCapacity(DWORD dwCapacity) = 0;        //Set current capacity.
		virtual void SetCaretPos(ULONGLONG ullOffset, bool fHighLow = true, bool fRedraw = true) = 0; //Set the caret position.
		virtual void SetCharsExtraSpace(DWORD dwSpace) = 0;    //Extra space to add between chars, in pixels.
		virtual void SetCodepage(int iCodepage) = 0;           //Codepage for text area.
		virtual void SetColors(const HEXCOLORS& hcs) = 0;      //Set HexCtrl's colors.
		virtual bool SetConfig(std::wstring_view wsvPath) = 0; //Set configuration file, or "" for defaults.
		virtual void SetData(const HEXDATA& hds, bool fAdjust = false) = 0; //Main method to set data for HexCtrl.
		virtual void SetDateInfo(DWORD dwFormat, wchar_t wchSepar) = 0; //Set date format and date separator.
		virtual void SetDlgProperties(EHexWnd eWnd, std::uint64_t u64Flags) = 0; //Properties for the internal dialogs.
		virtual void SetFont(const LOGFONTW& lf) = 0;          //Set HexCtrl's font, this font has to be monospaced.
		virtual void SetGroupSize(DWORD dwSize) = 0;           //Set data grouping size.
		virtual void SetMutable(bool fMutable) = 0;            //Enable or disable mutable/editable mode.
		virtual void SetOffsetMode(bool fHex) = 0;             //Set offset being shown as Hex or as Decimal.
		virtual void SetPageSize(DWORD dwSize, std::wstring_view wsvName = L"Page") = 0; //Set page size and name to draw the lines in-between.
		virtual void SetRedraw(bool fRedraw) = 0;              //Handle WM_PAINT message or not.
		virtual void SetScrollRatio(float flRatio, bool fLines) = 0; //Set mouse-wheel scroll ratio in screens or in lines.
		virtual void SetSelection(const VecSpan& vecSel, bool fRedraw = true, bool fHighlight = false) = 0; //Set current selection.
		virtual void SetUnprintableChar(wchar_t wch) = 0;      //Set unprintable replacement character.
		virtual void SetVirtualBkm(IHexBookmarks* pVirtBkm) = 0; //Set pointer for Bookmarks Virtual Mode.
		virtual void ShowInfoBar(bool fShow) = 0;              //Show/hide bottom Info bar.
	};

	struct IHexCtrlDeleter { void operator()(IHexCtrl* p)const { p->Destroy(); } };
	using IHexCtrlPtr = std::unique_ptr<IHexCtrl, IHexCtrlDeleter>;
	[[nodiscard]] HEXCTRLAPI IHexCtrlPtr CreateHexCtrl(HINSTANCE hInstClass = nullptr);

#if defined(HEXCTRL_SHARED_DLL) || defined(HEXCTRL_MANUAL_MFC_INIT)
	extern "C" HEXCTRLAPI BOOL __cdecl HexCtrlPreTranslateMessage(MSG * pMsg);
#endif

	/**************************************************************************
	* WM_NOTIFY message codes (NMHDR.code values).                            *
	* These codes are used to notify parent window about HexCtrl's states.    *
	**************************************************************************/

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
	constexpr auto HEXCTRL_MSG_WIDTHCHANGED { 0x0112U };  //Control width has changed due to font or display settings.


	/**************************************************************************
	* Flags for the internal dialogs, used with the SetDlgProperties method.  *
	**************************************************************************/

	constexpr auto HEXCTRL_FLAG_DLG_NOESC { 0x01ULL };

	//Setting a manifest for the ComCtl32.dll version 6.
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
}