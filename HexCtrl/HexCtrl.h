/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include <afxwin.h>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#if !defined(__cpp_lib_format) || !defined(__cpp_lib_span) || !defined(__cpp_lib_bit_cast)
#error "C++20 compiler is required for HexCtrl."
#endif

namespace HEXCTRL
{
	constexpr auto HEXCTRL_VERSION_MAJOR = 3;
	constexpr auto HEXCTRL_VERSION_MINOR = 5;
	constexpr auto HEXCTRL_VERSION_PATCH = 0;

	using SpanByte = std::span<std::byte>;
	using SpanCByte = std::span<const std::byte>;

	/********************************************************************************************
	* EHexCmd - Enum of the commands that can be executed within HexCtrl, used in ExecuteCmd.   *
	********************************************************************************************/
	enum class EHexCmd : std::uint8_t {
		CMD_SEARCH_DLG = 0x01, CMD_SEARCH_NEXT, CMD_SEARCH_PREV,
		CMD_NAV_GOTO_DLG, CMD_NAV_REPFWD, CMD_NAV_REPBKW, CMD_NAV_DATABEG, CMD_NAV_DATAEND,
		CMD_NAV_PAGEBEG, CMD_NAV_PAGEEND, CMD_NAV_LINEBEG, CMD_NAV_LINEEND,
		CMD_GROUPBY_BYTE, CMD_GROUPBY_WORD, CMD_GROUPBY_DWORD, CMD_GROUPBY_QWORD,
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
	* EHexWnd - HexControl's windows.                                                           *
	********************************************************************************************/
	enum class EHexWnd : std::uint8_t {
		WND_MAIN, DLG_BKMMGR, DLG_DATAINTERP, DLG_MODIFY,
		DLG_SEARCH, DLG_CODEPAGE, DLG_GOTO, DLG_TEMPLMGR
	};

	/********************************************************************************************
	* HEXSPAN - Data offset and size, used in some data/size related routines.                  *
	********************************************************************************************/
	struct HEXSPAN {
		ULONGLONG ullOffset { };
		ULONGLONG ullSize { };
	};
	using VecSpan = std::vector<HEXSPAN>;

	/********************************************************************************************
	* HEXDATAINFO - struct for a data information used in IHexVirtData.                         *
	********************************************************************************************/
	struct HEXDATAINFO {
		NMHDR    hdr { };       //Standard Windows header.
		HEXSPAN  stHexSpan { }; //Offset and size of the data bytes.
		SpanByte spnData { };   //Data span.
	};

	/********************************************************************************************
	* IHexVirtData - Pure abstract data handler class, that can be implemented by client,       *
	* to set its own data handler routines.	Pointer to this class can be set in SetData method. *
	* All virtual functions must be defined in client's derived class.                          *
	********************************************************************************************/
	class IHexVirtData {
	public:
		virtual void OnHexGetData(HEXDATAINFO&) = 0;       //Data to get.
		virtual void OnHexSetData(const HEXDATAINFO&) = 0; //Data to set, if mutable.
	};

	/********************************************************************************************
	* HEXBKM - Bookmarks main struct.                                                           *
	********************************************************************************************/
	struct HEXBKM {
		VecSpan      vecSpan { };                //Vector of offsets and sizes.
		std::wstring wstrDesc { };               //Bookmark description.
		ULONGLONG    ullID { };                  //Bookmark ID, assigned internally by framework.
		ULONGLONG    ullData { };                //User defined custom data.
		COLORREF     clrBk { RGB(240, 240, 0) }; //Bk color.
		COLORREF     clrText { RGB(0, 0, 0) };   //Text color.
	};
	using PHEXBKM = HEXBKM*;

	/********************************************************************************************
	* IHexBookmarks - Abstract base interface, represents HexCtrl bookmarks.                    *
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
	* HEXBKMINFO - Bookmarks info.                                                              *
	********************************************************************************************/
	struct HEXBKMINFO {
		NMHDR   hdr { };  //Standard Windows header.
		PHEXBKM pBkm { }; //Bookmark pointer.
	};
	using PHEXBKMINFO = HEXBKMINFO*;

	/********************************************************************************************
	* HEXMENUINFO - Menu info.                                                                  *
	********************************************************************************************/
	struct HEXMENUINFO {
		NMHDR hdr { };        //Standard Windows header.
		POINT pt { };         //Mouse position when clicked.
		WORD  wMenuID { };    //Menu identifier.
		bool  fShow { true }; //Whether to show menu or not, in case of HEXCTRL_MSG_CONTEXTMENU.
	};
	using PHEXMENUINFO = HEXMENUINFO*;

	/********************************************************************************************
	* HEXCOLOR - used with the IHexVirtColors interface.                                        *
	********************************************************************************************/
	struct HEXCOLOR {
		COLORREF clrBk { };   //Bk color.
		COLORREF clrText { }; //Text color.
	};
	using PHEXCOLOR = HEXCOLOR*;

	/********************************************************************************************
	* HEXCOLORINFO - struct for hex chunks' color information.                                  *
	********************************************************************************************/
	struct HEXCOLORINFO {
		NMHDR     hdr { };       //Standard Windows header.
		ULONGLONG ullOffset { }; //Offset for the color.
		HEXCOLOR  stClr { };     //Colors of the given offset.
	};

	/********************************************************************************************
	* IHexVirtColors - Pure abstract class for chunk colors.                                    *
	********************************************************************************************/
	class IHexVirtColors {
	public:
		virtual bool OnHexGetColor(HEXCOLORINFO&) = 0; //Should return true if colors are set.
	};

	/********************************************************************************************
	* IHexTemplates - Abstract base interface for HexCtrl data templates.                       *
	********************************************************************************************/
	class IHexTemplates {
	public:
		virtual auto ApplyTemplate(ULONGLONG ullOffset, int iTemplateID) -> int = 0; //Applies template to an offset, returns AppliedID.
		virtual void DisapplyAll() = 0;
		virtual void DisapplyByID(int iAppliedID) = 0;
		virtual void DisapplyByOffset(ULONGLONG ullOffset) = 0;
		virtual auto LoadTemplate(const wchar_t* pFilePath) -> int = 0; //Returns template ID on success, zero otherwise.
		virtual void ShowTooltips(bool fShow) = 0;
		virtual void UnloadAll() = 0;                     //Unload all templates.
		virtual void UnloadTemplate(int iTemplateID) = 0; //Unload/remove loaded template from memory.
	};

	/********************************************************************************************
	* HEXCOLORS - All HexCtrl colors.                                                           *
	********************************************************************************************/
	struct HEXCOLORS {
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
	struct HEXCREATE {
		HWND             hWndParent { };         //Parent window handle.
		const HEXCOLORS* pColors { };            //HexCtrl colors, nullptr for default.
		const LOGFONTW*  pLogFont { };           //Monospaced font for HexCtrl, nullptr for default.
		RECT             rect { };               //Initial window rect.
		UINT             uID { };                //Control ID if it's a child window.
		DWORD            dwStyle { };            //Window styles.
		DWORD            dwExStyle { };          //Extended window styles.
		float            flScrollRatio { 1.0F }; //Either a screen-ratio or lines amount to scroll with Page-scroll (mouse-wheel).
		bool             fScrollLines { false }; //Treat flScrollRatio as screen-ratio (false) or as amount of lines (true).
		bool             fInfoBar { true };      //Show bottom Info bar or not.
		bool             fCustom { false };      //If it's a custom control in a dialog.
	};

	/********************************************************************************************
	* HEXDATA - for IHexCtrl::SetData method.                                                   *
	********************************************************************************************/
	struct HEXDATA {
		SpanByte        spnData { };               //Data to display.
		IHexVirtData*   pHexVirtData { };          //Pointer for Virtual mode.
		IHexVirtColors* pHexVirtColors { };        //Pointer for Custom Colors class.
		DWORD           dwCacheSize { 0x800000U }; //In Virtual mode max cached size of data to fetch.
		bool            fMutable { false };        //Is data mutable (editable) or read-only.
		bool            fHighLatency { false };    //Do not redraw window until scrolling completes.
	};

	/********************************************************************************************
	* HEXHITTEST - used in HitTest method.                                                      *
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
		operator bool()const { return i8Vert == 0 && i8Horz == 0; }; //For test simplicity: if(IsOffsetVisible()).
	};

	/********************************************************************************************
	* EHexModifyMode - Enum of the data modification mode, used in HEXMODIFY.                   *
	********************************************************************************************/
	enum class EHexModifyMode : std::uint8_t {
		MODIFY_ONCE, MODIFY_REPEAT, MODIFY_OPERATION, MODIFY_RAND_MT19937, MODIFY_RAND_FAST
	};

	/********************************************************************************************
	* EHexOperMode - Data Operation mode, used in EHexModifyMode::MODIFY_OPERATION mode.        *
	********************************************************************************************/
	enum class EHexOperMode : std::uint8_t {
		OPER_ASSIGN, OPER_ADD, OPER_SUB, OPER_MUL, OPER_DIV, OPER_CEIL, OPER_FLOOR, OPER_OR,
		OPER_XOR, OPER_AND, OPER_NOT, OPER_SHL, OPER_SHR, OPER_ROTL, OPER_ROTR, OPER_SWAP,
		OPER_BITREV
	};

	/********************************************************************************************
	* EHexDataSize - Data size to operate on, used in EHexModifyMode::MODIFY_OPERATION mode.    *
	* Also used to set data grouping mode, in SetGroupMode method.                              *
	********************************************************************************************/
	enum class EHexDataSize : std::uint8_t {
		SIZE_BYTE = 0x1U, SIZE_WORD = 0x2U, SIZE_DWORD = 0x4U, SIZE_QWORD = 0x8U
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
	struct HEXMODIFY {
		EHexModifyMode enModifyMode { EHexModifyMode::MODIFY_ONCE }; //Modify mode.
		EHexOperMode   enOperMode { };       //Operation mode, used only in MODIFY_OPERATION mode.
		EHexDataSize   enDataSize { };       //Operation data size.
		SpanCByte      spnData { };          //Span of the data to modify from.
		VecSpan        vecSpan { };          //Vector of data offsets and sizes.
		bool           fBigEndian { false }; //Treat the data as a big endian, used only in MODIFY_OPERATION mode.
	};


	/********************************************************************************************
	* IHexCtrl - pure abstract base class.                                                      *
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
		[[nodiscard]] virtual auto GetCacheSize()const->DWORD = 0;           //Returns Virtual mode cache size.
		[[nodiscard]] virtual auto GetCapacity()const->DWORD = 0;            //Current capacity.
		[[nodiscard]] virtual auto GetCaretPos()const->ULONGLONG = 0;        //Cursor position.
		[[nodiscard]] virtual auto GetCodepage()const->int = 0;              //Get current codepage ID.
		[[nodiscard]] virtual auto GetColors()const->HEXCOLORS = 0;          //Current colors.
		[[nodiscard]] virtual auto GetData(HEXSPAN hss)const->SpanByte = 0;  //Get pointer to data offset, no matter what mode the control works in.
		[[nodiscard]] virtual auto GetDataSize()const->ULONGLONG = 0;        //Get currently set data size.
		[[nodiscard]] virtual auto GetDateInfo()const->std::tuple<DWORD, wchar_t> = 0; //Get date format and separator info.
		[[nodiscard]] virtual auto GetDlgData(EHexWnd eWnd)const->std::uint64_t = 0; //Data from the internal dialogs.
		[[nodiscard]] virtual auto GetFont() -> LOGFONTW = 0;                //Get current font.
		[[nodiscard]] virtual auto GetGroupMode()const->EHexDataSize = 0;    //Retrieves current data grouping mode.
		[[nodiscard]] virtual auto GetMenuHandle()const->HMENU = 0;          //Context menu handle.
		[[nodiscard]] virtual auto GetPagesCount()const->ULONGLONG = 0;      //Get count of pages.
		[[nodiscard]] virtual auto GetPagePos()const->ULONGLONG = 0;         //Get current page a cursor stays at.
		[[nodiscard]] virtual auto GetPageSize()const->DWORD = 0;            //Current page size.
		[[nodiscard]] virtual auto GetSelection()const->VecSpan = 0;         //Get current selection.
		[[nodiscard]] virtual auto GetTemplates()const->IHexTemplates* = 0;  //Get Templates interface.
		[[nodiscard]] virtual auto GetUnprintableChar()const->wchar_t = 0;   //Get unprintable replacement character.
		[[nodiscard]] virtual auto GetWindowHandle(EHexWnd eWnd)const->HWND = 0; //Retrieves control's window/dialog handle.
		virtual void GoToOffset(ULONGLONG ullOffset, int iRelPos = 0) = 0;   //Go (scroll) to a given offset.
		[[nodiscard]] virtual bool HasSelection()const = 0;    //Does currently have any selection or not.
		[[nodiscard]] virtual auto HitTest(POINT pt, bool fScreen = true)const->std::optional<HEXHITTEST> = 0; //HitTest given point.
		[[nodiscard]] virtual bool IsCmdAvail(EHexCmd eCmd)const = 0; //Is given Cmd currently available (can be executed)?
		[[nodiscard]] virtual bool IsCreated()const = 0;       //Shows whether control is created or not.
		[[nodiscard]] virtual bool IsDataSet()const = 0;       //Shows whether a data was set to the control or not.
		[[nodiscard]] virtual bool IsMutable()const = 0;       //Is edit mode enabled or not.
		[[nodiscard]] virtual bool IsOffsetAsHex()const = 0;   //Is "Offset" currently represented (shown) as Hex or as Decimal.
		[[nodiscard]] virtual auto IsOffsetVisible(ULONGLONG ullOffset)const->HEXVISION = 0; //Ensures that the given offset is visible.
		[[nodiscard]] virtual bool IsVirtual()const = 0;       //Is working in Virtual or default mode.		
		virtual void ModifyData(const HEXMODIFY& hms) = 0;     //Main routine to modify data in IsMutable()==true mode.
		virtual void Redraw() = 0;                             //Redraw the control's window.
		virtual void SetCapacity(DWORD dwCapacity) = 0;        //Set the control's current capacity.
		virtual void SetCaretPos(ULONGLONG ullOffset, bool fHighLow = true, bool fRedraw = true) = 0; //Set the caret position.
		virtual void SetCodepage(int iCodepage) = 0;           //Codepage for text area.
		virtual void SetColors(const HEXCOLORS& clr) = 0;      //Set all the control's colors.
		virtual bool SetConfig(std::wstring_view wsvPath) = 0; //Set configuration file, or "" for defaults.
		virtual void SetData(const HEXDATA& hds) = 0;          //Main method for setting data to display (and edit).
		virtual void SetDateInfo(DWORD dwFormat, wchar_t wchSepar) = 0; //Set date format and date separator.
		virtual auto SetDlgData(EHexWnd eWnd, std::uint64_t ullData) -> HWND = 0; //Data for the internal dialogs.
		virtual void SetFont(const LOGFONTW& lf) = 0;          //Set the control's new font. This font has to be monospaced.
		virtual void SetGroupMode(EHexDataSize eMode) = 0;     //Set current "Group Data By" mode.
		virtual void SetMutable(bool fEnable) = 0;             //Enable or disable mutable/editable mode.
		virtual void SetOffsetMode(bool fHex) = 0;             //Set offset being shown as Hex or as Decimal.
		virtual void SetPageSize(DWORD dwSize, std::wstring_view wsvName = L"Page") = 0; //Set page size and name to draw the lines in-between.
		virtual void SetRedraw(bool fRedraw) = 0;              //Handle WM_PAINT message or not.
		virtual void SetScrollRatio(float flRatio, bool fLines) = 0; //Set mouse-wheel scroll ratio in screens or in lines, if fLines==true.
		virtual void SetSelection(const VecSpan& vecSel, bool fRedraw = true, bool fHighlight = false) = 0; //Set current selection.
		virtual void SetUnprintableChar(wchar_t wch) = 0;      //Set unprintable replacement character.
		virtual void SetVirtualBkm(IHexBookmarks* pVirtBkm) = 0; //Set pointer for Bookmarks Virtual Mode.
		virtual void ShowInfoBar(bool fShow) = 0;              //Show/hide bottom Info bar.
	};

	/********************************************************************************************
	* Factory function CreateHexCtrl returns IHexCtrlUnPtr - unique_ptr with custom deleter.    *
	* In client code you should use IHexCtrlPtr type which is an alias to either IHexCtrlUnPtr  *
	* - a unique_ptr, or IHexCtrlShPtr - a shared_ptr. Uncomment what serves best for you,      *
	* and comment out the other.                                                                *
	* If you, for some reason, need raw pointer, you can directly call CreateRawHexCtrl         *
	* function, which returns IHexCtrl interface pointer, but in this case you will need to     *
	* call IHexCtrl::Destroy method	afterwards - to manually delete HexCtrl object.             *
	********************************************************************************************/
#ifdef HEXCTRL_SHARED_DLL
#ifdef HEXCTRL_EXPORT
#define HEXCTRLAPI __declspec(dllexport)
#else
#define HEXCTRLAPI __declspec(dllimport)

#ifdef _WIN64
#ifdef _DEBUG
#define LIBNAME_PROPER(x) x"64d.lib"
#else
#define LIBNAME_PROPER(x) x"64.lib"
#endif
#else
#ifdef _DEBUG
#define LIBNAME_PROPER(x) x"d.lib"
#else
#define LIBNAME_PROPER(x) x".lib"
#endif
#endif

#pragma comment(lib, LIBNAME_PROPER("HexCtrl"))
#endif
#else
#define	HEXCTRLAPI
#endif

#if defined HEXCTRL_MANUAL_MFC_INIT || defined HEXCTRL_SHARED_DLL
	//Because MFC PreTranslateMessage doesn't work in a DLLs
	//this exported function should be called from the app's main message loop.
	//Something like that:
	//BOOL CMyApp::PreTranslateMessage(MSG* pMsg) {
	//	if (HexCtrlPreTranslateMessage(pMsg))
	//		return TRUE;
	//	return CWinApp::PreTranslateMessage(pMsg);
	//}
	extern "C" HEXCTRLAPI BOOL __cdecl HexCtrlPreTranslateMessage(MSG * pMsg);
#endif

	extern "C" [[nodiscard]] HEXCTRLAPI IHexCtrl * __cdecl CreateRawHexCtrl();

	using IHexCtrlPtr = std::unique_ptr < IHexCtrl, decltype([](IHexCtrl* p) { p->Destroy(); }) > ;

	[[nodiscard]] inline IHexCtrlPtr CreateHexCtrl() {
		return IHexCtrlPtr { CreateRawHexCtrl() };
	};

	/********************************************************************************************
	* WM_NOTIFY message codes (NMHDR.code values).                                              *
	* These codes are used to notify m_hwndMsg window about control's states.                   *
	********************************************************************************************/

	constexpr auto HEXCTRL_MSG_BKMCLICK { 0x0100U };      //Bookmark is clicked.
	constexpr auto HEXCTRL_MSG_CONTEXTMENU { 0x0101U };   //OnContextMenu has triggered.
	constexpr auto HEXCTRL_MSG_DESTROY { 0x0102U };       //Indicates that the HexCtrl is being destroyed.
	constexpr auto HEXCTRL_MSG_DLGBKMMGR { 0x0103U };     //"Bookmark manager" dialog displayed.
	constexpr auto HEXCTRL_MSG_DLGCODEPAGE { 0x0104U };   //"Codepage" dialog displayed.
	constexpr auto HEXCTRL_MSG_DLGDATAINTERP { 0x0105U }; //"Data Interpreter" dialog displayed.
	constexpr auto HEXCTRL_MSG_DLGGOTO { 0x0106U };       //"GoTo" dialog displayed.
	constexpr auto HEXCTRL_MSG_DLGMODIFY { 0x0107U };     //"Modify Data" dialog displayed.
	constexpr auto HEXCTRL_MSG_DLGSEARCH { 0x0108U };     //"Search" dialog displayed.
	constexpr auto HEXCTRL_MSG_DLGTEMPLMGR { 0x0109U };   //"Template manager" dialog displayed.
	constexpr auto HEXCTRL_MSG_MENUCLICK { 0x010AU };     //User defined custom menu has clicked.
	constexpr auto HEXCTRL_MSG_SETCAPACITY { 0x010BU };   //Capacity has changed.
	constexpr auto HEXCTRL_MSG_SETCARET { 0x010CU };      //Caret position has changed.
	constexpr auto HEXCTRL_MSG_SETCODEPAGE { 0x010DU };   //Codepage has changed.
	constexpr auto HEXCTRL_MSG_SETDATA { 0x010EU };       //Indicates that the data has changed.
	constexpr auto HEXCTRL_MSG_SETFONT { 0x010FU };       //Font has changed.
	constexpr auto HEXCTRL_MSG_SETGROUPMODE { 0x0110U };  //Data group mode has changed.
	constexpr auto HEXCTRL_MSG_SETSELECTION { 0x0111U };  //Selection has been made.


	/*********************************************************
	* Flags for the internal dialogs, used with SetDlgData.  *
	*********************************************************/

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