/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#pragma once
#include <Windows.h> //Standard Windows header.
#include <deque>
#include <memory>    //std::shared/unique_ptr and related.
#include <optional>
#include <string>
#include <vector>

#ifndef __cpp_lib_byte
static_assert(false, "std::byte compliant compiler required.");
#endif

/**********************************************************************
* If HEXCTRL_IHEXCTRLPTR_UNIQUEPTR defined then IHexCtrlPtr is        *
* resolved to std::unique_ptr. Otherwise it's std::shared_ptr.        *
**********************************************************************/
#define HEXCTRL_IHEXCTRLPTR_UNIQUEPTR

/**********************************************************************
* If HexCtrl is to be used as a .dll, then include this header,       *
* and uncomment the line below.                                       *
**********************************************************************/
//#define HEXCTRL_SHARED_DLL

/**********************************************************************
* For manually initialize MFC.                                        *
* This macro is used only for Win32 non MFC projects, with HexCtrl    *
* built from sources, with "Use MFC in a Shared DLL" setting.         *
**********************************************************************/
//#define HEXCTRL_MANUAL_MFC_INIT

namespace HEXCTRL
{
	/********************************************************************************************
	* EHexCmd - Enum of the commands that can be executed within HexCtrl, used in ExecuteCmd.   *
	********************************************************************************************/
	enum class EHexCmd : WORD
	{
		CMD_DLG_SEARCH = 0x01, CMD_SEARCH_NEXT, CMD_SEARCH_PREV,
		CMD_NAV_DLG_GOTO, CMD_NAV_REPFWD, CMD_NAV_REPBKW, CMD_NAV_DATABEG, CMD_NAV_DATAEND, 
		CMD_NAV_PAGEBEG, CMD_NAV_PAGEEND, CMD_NAV_LINEBEG, CMD_NAV_LINEEND,
		CMD_GROUPBY_BYTE, CMD_GROUPBY_WORD, CMD_GROUPBY_DWORD, CMD_GROUPBY_QWORD,
		CMD_BKM_ADD, CMD_BKM_REMOVE, CMD_BKM_NEXT, CMD_BKM_PREV, CMD_BKM_CLEARALL, CMD_BKM_DLG_MANAGER,
		CMD_CLPBRD_COPYHEX, CMD_CLPBRD_COPYHEXLE, CMD_CLPBRD_COPYHEXFMT, CMD_CLPBRD_COPYTEXT,
		CMD_CLPBRD_COPYBASE64, CMD_CLPBRD_COPYCARR, CMD_CLPBRD_COPYGREPHEX, CMD_CLPBRD_COPYPRNTSCRN,
		CMD_CLPBRD_PASTEHEX, CMD_CLPBRD_PASTETEXT,
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
	enum class EHexCreateMode : WORD
	{
		CREATE_CHILD, CREATE_POPUP, CREATE_CUSTOMCTRL
	};

	/********************************************************************************************
	* EHexGroupMode - current data mode representation.                                          *
	********************************************************************************************/
	enum class EHexGroupMode : WORD
	{
		ASBYTE = 1, ASWORD = 2, ASDWORD = 4, ASQWORD = 8
	};

	/********************************************************************************************
	* EHexDataMode - Enum of the working data mode, used in HEXDATASTRUCT in SetData.           *
	* DATA_MEMORY: Default standard data mode.                                                  *
	* DATA_MSG: Data is handled through WM_NOTIFY messages in handler window.				    *
	* DATA_VIRTUAL: Data is handled through IHexVirtData interface by derived class.            *
	********************************************************************************************/
	enum class EHexDataMode : WORD
	{
		DATA_MEMORY, DATA_MSG, DATA_VIRTUAL
	};

	/********************************************************************************************
	* EHexWnd - HexControl's windows.                                                           *
	********************************************************************************************/
	enum class EHexWnd : WORD
	{
		WND_MAIN, DLG_BKMMANAGER, DLG_DATAINTERP, DLG_FILLDATA, 
		DLG_OPERS, DLG_SEARCH, DLG_ENCODING, DLG_GOTO
	};

	/********************************************************************************************
	* HEXSPANSTRUCT - Data offset and size, used in some data/size related routines.            *
	********************************************************************************************/
	struct HEXSPANSTRUCT
	{
		ULONGLONG ullOffset { };
		ULONGLONG ullSize { };
	};

	/********************************************************************************************
	* HEXBKMSTRUCT - Bookmarks.                                                                 *
	********************************************************************************************/
	struct HEXBKMSTRUCT
	{
		std::vector<HEXSPANSTRUCT> vecSpan { };                //Vector of offsets and sizes.
		std::wstring               wstrDesc { };               //Bookmark description.
		ULONGLONG                  ullID { };                  //Bookmark ID, assigned internally by framework.
		ULONGLONG                  ullData { };                //User defined custom data.
		COLORREF                   clrBk { RGB(240, 240, 0) }; //Bk color.
		COLORREF                   clrText { RGB(0, 0, 0) };   //Text color.
	};
	using PHEXBKMSTRUCT = HEXBKMSTRUCT*;

	/********************************************************************************************
	* IHexVirtData - Pure abstract data handler class, that can be implemented by client,       *
	* to set its own data handler routines.	Works in EHexDataMode::DATA_VIRTUAL mode.           *
	* Pointer to this class can be set in IHexCtrl::SetData method.                             *
	* Its usage is very similar to DATA_MSG logic, where control sends WM_NOTIFY messages       *
	* to set window to get/set data. But in this case it's just a pointer to a custom           *
	* routine's implementation.                                                                 *
	* All virtual functions must be defined in client's derived class.                          *
	********************************************************************************************/
	class IHexVirtData
	{
	public:
		[[nodiscard]] virtual std::byte* GetData(const HEXSPANSTRUCT&) = 0; //Data index and size to get.
		virtual	void SetData(std::byte*, const HEXSPANSTRUCT&) = 0; //Routine to modify data, if HEXDATASTRUCT::fMutable == true.
	};

	/********************************************************************************************
	* IHexVirtBkm - Pure abstract class for virtual bookmarks.                                  *
	********************************************************************************************/
	class IHexVirtBkm
	{
	public:
		virtual ULONGLONG Add(const HEXBKMSTRUCT& stBookmark) = 0; //Add new bookmark, return new bookmark's ID.
		virtual void ClearAll() = 0; //Clear all bookmarks.
		[[nodiscard]] virtual ULONGLONG GetCount() = 0; //Get total bookmarks count.
		[[nodiscard]] virtual auto GetByID(ULONGLONG ullID)->HEXBKMSTRUCT* = 0; //Bookmark by ID.
		[[nodiscard]] virtual auto GetByIndex(ULONGLONG ullIndex)->HEXBKMSTRUCT* = 0; //Bookmark by index (in inner list).
		[[nodiscard]] virtual auto HitTest(ULONGLONG ullOffset)->HEXBKMSTRUCT* = 0;   //Does given offset have a bookmark?
		virtual void RemoveByID(ULONGLONG ullID) = 0;   //Remove bookmark by given ID (returned by Add()).
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
	* IHexVirtColors - Pure abstract class for chunk colors.                                    *
	********************************************************************************************/
	class IHexVirtColors
	{
	public:
		[[nodiscard]] virtual PHEXCOLOR GetColor(ULONGLONG ullOffset) = 0;
	};

	/********************************************************************************************
	* HEXCOLORSSTRUCT - All HexCtrl colors.                                                     *
	********************************************************************************************/
	struct HEXCOLORSSTRUCT
	{
		COLORREF clrTextHex { GetSysColor(COLOR_WINDOWTEXT) };         //Hex chunks text color.
		COLORREF clrTextAscii { GetSysColor(COLOR_WINDOWTEXT) };       //Ascii text color.
		COLORREF clrTextSelected { GetSysColor(COLOR_HIGHLIGHTTEXT) }; //Selected text color.
		COLORREF clrTextDataInterpret { RGB(250, 250, 250) };          //Data Interpreter text color.
		COLORREF clrTextCaption { RGB(0, 0, 180) };                    //Caption text color
		COLORREF clrTextInfoRect { GetSysColor(COLOR_WINDOWTEXT) };    //Text color of the bottom "Info" rect.
		COLORREF clrTextCursor { RGB(255, 255, 255) };                 //Cursor text color.
		COLORREF clrTextTooltip { GetSysColor(COLOR_INFOTEXT) };       //Tooltip text color.
		COLORREF clrBk { GetSysColor(COLOR_WINDOW) };                  //Background color.
		COLORREF clrBkSelected { GetSysColor(COLOR_HIGHLIGHT) };       //Background color of the selected Hex/Ascii.
		COLORREF clrBkDataInterpret { RGB(147, 58, 22) };              //Data Interpreter Bk color.
		COLORREF clrBkInfoRect { GetSysColor(COLOR_BTNFACE) };         //Background color of the bottom "Info" rect.
		COLORREF clrBkCursor { RGB(0, 0, 255) };                       //Cursor background color.
		COLORREF clrBkCursorSelected { RGB(0, 0, 200) };               //Cursor background color in selection.
		COLORREF clrBkTooltip { GetSysColor(COLOR_INFOBK) };           //Tooltip background color.
	};

	/********************************************************************************************
	* HEXCREATESTRUCT - for IHexCtrl::Create method.                                            *
	********************************************************************************************/
	struct HEXCREATESTRUCT
	{
		EHexCreateMode  enCreateMode { EHexCreateMode::CREATE_CHILD }; //Creation mode of the HexCtrl window.
		HEXCOLORSSTRUCT stColor { };          //All the control's colors.
		HWND            hwndParent { };       //Parent window handle.
		const LOGFONTW* pLogFont { };         //Font to be used, nullptr for default. This font has to be monospaced.
		RECT            rect { };             //Initial rect. If null, the window is screen centered.
		UINT            uID { };              //Control ID.
		DWORD           dwStyle { };          //Window styles, 0 for default.
		DWORD           dwExStyle { };        //Extended window styles, 0 for default.
		double          dbWheelRatio { 1.0 }; //Ratio for how much to scroll with mouse-wheel.
	};

	/********************************************************************************************
	* HEXDATASTRUCT - for IHexCtrl::SetData method.                                             *
	********************************************************************************************/
	struct HEXDATASTRUCT
	{
		EHexDataMode    enDataMode { EHexDataMode::DATA_MEMORY }; //Working data mode.
		ULONGLONG       ullDataSize { };          //Size of the data to display, in bytes.
		HWND            hwndMsg { };              //Window for DATA_MSG mode. Parent is used by default.
		IHexVirtData*   pHexVirtData { };         //Pointer for DATA_VIRTUAL mode.
		IHexVirtColors* pHexVirtColors { };       //Pointer for Custom Colors class.
		std::byte*      pData { };                //Data pointer for DATA_MEMORY mode. Not used in other modes.
		DWORD           dwCacheSize { 0x800000 }; //In DATA_MSG and DATA_VIRTUAL max cached size of data to fetch.
		bool            fMutable { false };       //Is data mutable (editable) or read-only.
		bool            fHighLatency { false };   //Do not redraw window until scrolling completes.
	};

	/********************************************************************************************
	* HEXNOTIFYSTRUCT - used in notifications routine.                                          *
	********************************************************************************************/
	struct HEXNOTIFYSTRUCT
	{
		NMHDR         hdr { };     //Standard Windows header. For hdr.code values see HEXCTRL_MSG_* messages.
		HEXSPANSTRUCT stSpan { };  //Offset and size of the bytes.
		ULONGLONG     ullData { }; //Data depending on message (e.g. user defined custom menu ID/caret pos).
		std::byte*    pData { };   //Pointer to a data to get/send.
		POINT         point { };   //Mouse position for menu notifications.
	};
	using PHEXNOTIFYSTRUCT = HEXNOTIFYSTRUCT*;

	/********************************************************************************************
	* HEXHITTESTSTRUCT - used in HitTest method.                                                *
	********************************************************************************************/
	struct HEXHITTESTSTRUCT
	{
		ULONGLONG ullOffset { };      //Offset.
		bool      fIsAscii { false }; //Is cursor at ASCII part or at Hex.
	};

	/********************************************************************************************
	* HEXVISSTRUCT - Offset visibility struct, used in IsOffsetVisible method.                  *
	********************************************************************************************/
	struct HEXVISSTRUCT
	{
		std::int8_t i8Vert { }; //Vertical offset.
		std::int8_t i8Horz { }; //Horizontal offset.
		operator bool() { return i8Vert == 0 && i8Horz == 0; }; //For test simplicity: if(IsOffsetVisible()).
	};

	/********************************************************************************************
	* IHexCtrl - pure abstract base class.                                                      *
	********************************************************************************************/
	class IHexCtrl
	{
	public:
		virtual ULONGLONG BkmAdd(const HEXBKMSTRUCT& hbs, bool fRedraw = false) = 0;     //Adds new bookmark.
		virtual void BkmClearAll() = 0;                                                  //Clear all bookmarks.
		[[nodiscard]] virtual auto BkmGetByID(ULONGLONG ullID)->HEXBKMSTRUCT* = 0;       //Get bookmark by ID.
		[[nodiscard]] virtual auto BkmGetByIndex(ULONGLONG ullIndex)->HEXBKMSTRUCT* = 0; //Get bookmark by Index.
		[[nodiscard]] virtual ULONGLONG BkmGetCount()const = 0;                          //Get bookmarks count.
		[[nodiscard]] virtual auto BkmHitTest(ULONGLONG ullOffset)->HEXBKMSTRUCT* = 0;   //HitTest for given offset.
		virtual void BkmRemoveByID(ULONGLONG ullID) = 0;                                 //Remove bookmark by the given ID.
		virtual void BkmSetVirtual(bool fEnable, IHexVirtBkm* pVirtual = nullptr) = 0;   //Enable/disable bookmarks virtual mode.
		virtual void ClearData() = 0;                           //Clears all data from HexCtrl's view (not touching data itself).
		virtual bool Create(const HEXCREATESTRUCT& hcs) = 0;    //Main initialization method.
		virtual bool CreateDialogCtrl(UINT uCtrlID, HWND hParent) = 0; //Сreates custom dialog control.
		virtual void Destroy() = 0;                             //Deleter.
		virtual void ExecuteCmd(EHexCmd enCmd) = 0;             //Execute a command within the control.
		[[nodiscard]] virtual DWORD GetCapacity()const = 0;                  //Current capacity.
		[[nodiscard]] virtual ULONGLONG GetCaretPos()const = 0;              //Cursor position.
		[[nodiscard]] virtual auto GetColors()const->HEXCOLORSSTRUCT = 0;    //Current colors.
		[[nodiscard]] virtual auto GetDataSize()const->ULONGLONG = 0;        //Get currently set data size.
		[[nodiscard]] virtual int GetEncoding()const = 0;                    //Get current code page ID.
		[[nodiscard]] virtual long GetFontSize()const = 0;                   //Current font size.
		[[nodiscard]] virtual auto GetGroupMode()const->EHexGroupMode = 0;   //Retrieves current data grouping mode.
		[[nodiscard]] virtual HMENU GetMenuHandle()const = 0;                //Context menu handle.
		[[nodiscard]] virtual auto GetPagesCount()const->ULONGLONG = 0;      //Get count of pages.
		[[nodiscard]] virtual auto GetPagePos()const->ULONGLONG = 0;         //Get current page a cursor stays at.
		[[nodiscard]] virtual DWORD GetPageSize()const = 0;                  //Current page size.
		[[nodiscard]] virtual auto GetSelection()const->std::vector<HEXSPANSTRUCT> = 0; //Gets current selection.
		[[nodiscard]] virtual HWND GetWindowHandle(EHexWnd enWnd)const = 0;  //Retrieves control's window/dialog handle.
		virtual void GoToOffset(ULONGLONG ullOffset, int iRelPos = 0) = 0;   //Go (scroll) to a given offset.
		[[nodiscard]] virtual auto HitTest(POINT pt, bool fScreen = true)const->std::optional<HEXHITTESTSTRUCT> = 0; //HitTest given point.
		[[nodiscard]] virtual bool IsCmdAvail(EHexCmd enCmd)const = 0; //Is given Cmd currently available (can be executed)?
		[[nodiscard]] virtual bool IsCreated()const = 0;       //Shows whether control is created or not.
		[[nodiscard]] virtual bool IsDataSet()const = 0;       //Shows whether a data was set to the control or not.
		[[nodiscard]] virtual bool IsMutable()const = 0;       //Is edit mode enabled or not.
		[[nodiscard]] virtual bool IsOffsetAsHex()const = 0;   //Is "Offset" currently represented (shown) as Hex or as Decimal.
		[[nodiscard]] virtual HEXVISSTRUCT IsOffsetVisible(ULONGLONG ullOffset)const = 0; //Ensures that the given offset is visible.
		virtual void Redraw() = 0;                             //Redraw the control's window.
		virtual void SetCapacity(DWORD dwCapacity) = 0;        //Set the control's current capacity.
		virtual void SetCaretPos(ULONGLONG ullOffset, bool fHighLow = true, bool fRedraw = true) = 0; //Set the caret position.
		virtual void SetColors(const HEXCOLORSSTRUCT& clr) = 0;//Set all the control's colors.
		virtual bool SetConfig(std::wstring_view wstrPath) = 0;//Set configuration file, or "" for defaults.
		virtual void SetData(const HEXDATASTRUCT& hds) = 0;    //Main method for setting data to display (and edit).	
		virtual void SetEncoding(int iCodePage) = 0;           //Code-page for text area.
		virtual void SetFont(const LOGFONTW* pLogFont) = 0;    //Set the control's new font. This font has to be monospaced.
		virtual void SetFontSize(UINT uiSize) = 0;             //Set the control's font size.
		virtual void SetGroupMode(EHexGroupMode enMode) = 0;   //Set current "Group Data By" mode.
		virtual void SetMutable(bool fEnable) = 0;             //Enable or disable mutable/editable mode.
		virtual void SetOffsetMode(bool fHex) = 0;             //Set offset being shown as Hex or as Decimal.
		virtual void SetPageSize(DWORD dwSize, std::wstring_view wstrName = L"Page") = 0; //Set page size and name to draw the lines in-between.
		virtual void SetSelection(const std::vector<HEXSPANSTRUCT>& vecSel, bool fRedraw = true) = 0; //Set current selection.
		virtual void SetWheelRatio(double dbRatio) = 0;        //Set the ratio for how much to scroll with mouse-wheel.
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

	extern "C" HEXCTRLAPI IHexCtrl * __cdecl CreateRawHexCtrl();
	using IHexCtrlUnPtr = std::unique_ptr<IHexCtrl, void(*)(IHexCtrl*)>;
	using IHexCtrlShPtr = std::shared_ptr<IHexCtrl>;

	inline IHexCtrlUnPtr CreateHexCtrl()
	{
		return IHexCtrlUnPtr(CreateRawHexCtrl(), [](IHexCtrl* p) { p->Destroy(); });
	};

#ifdef HEXCTRL_IHEXCTRLPTR_UNIQUEPTR
	using IHexCtrlPtr = IHexCtrlUnPtr;
#else
	using IHexCtrlPtr = IHexCtrlShPtr;
#endif

	/********************************************
	* HEXCTRLINFO: service info structure.      *
	********************************************/
	struct HEXCTRLINFO
	{
		const wchar_t* pwszVersion { };        //WCHAR version string.
		union {
			unsigned long long ullVersion { }; //ULONGLONG version number.
			struct {
				short wMajor;
				short wMinor;
				short wMaintenance;
				short wRevision;
			}stVersion;
		};
	};

	/*********************************************
	* Service info export/import function.       *
	* Returns pointer to PCHEXCTRL_INFO struct.  *
	*********************************************/
	extern "C" HEXCTRLAPI HEXCTRLINFO * __cdecl GetHexCtrlInfo();

	/********************************************************************************************
	* WM_NOTIFY message codes (NMHDR.code values).                                              *
	* These codes are used to notify m_hwndMsg window about control's states.                   *
	********************************************************************************************/

	constexpr auto HEXCTRL_MSG_BKMCLICK { 0x0100U };     //Bookmark clicked.
	constexpr auto HEXCTRL_MSG_CARETCHANGE { 0x0101U };  //Caret position changed.
	constexpr auto HEXCTRL_MSG_CONTEXTMENU { 0x0102U };  //OnContextMenu triggered.
	constexpr auto HEXCTRL_MSG_DESTROY { 0x0103U };      //Indicates that HexCtrl is being destroyed.
	constexpr auto HEXCTRL_MSG_GETDATA { 0x0104U };      //Used in DATA_MSG mode to request the data to display.
	constexpr auto HEXCTRL_MSG_MENUCLICK { 0x0105U };    //User defined custom menu clicked.
	constexpr auto HEXCTRL_MSG_SELECTION { 0x0106U };    //Selection has been made.
	constexpr auto HEXCTRL_MSG_SETDATA { 0x0107U };      //Indicates that the data has changed.
	constexpr auto HEXCTRL_MSG_VIEWCHANGE { 0x0108U };   //View of the control has changed.

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
}