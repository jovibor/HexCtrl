/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
****************************************************************************************/
#pragma once
#include "HexCtrlDefs.h"
#include <memory>
#include <optional>

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
	* IHexCtrl - pure abstract base class.                                                      *
	********************************************************************************************/
	class IHexCtrl
	{
	public:
		virtual ULONGLONG BkmAdd(const HEXBKM& hbs, bool fRedraw = false) = 0;     //Adds new bookmark.
		virtual void BkmClearAll() = 0;                                            //Clear all bookmarks.
		[[nodiscard]] virtual auto BkmGetByID(ULONGLONG ullID)->HEXBKM* = 0;       //Get bookmark by ID.
		[[nodiscard]] virtual auto BkmGetByIndex(ULONGLONG ullIndex)->HEXBKM* = 0; //Get bookmark by Index.
		[[nodiscard]] virtual ULONGLONG BkmGetCount()const = 0;                    //Get bookmarks count.
		[[nodiscard]] virtual auto BkmHitTest(ULONGLONG ullOffset)->HEXBKM* = 0;   //HitTest for given offset.
		virtual void BkmRemoveByID(ULONGLONG ullID) = 0;                           //Remove bookmark by the given ID.
		virtual void BkmSetVirtual(bool fEnable, IHexVirtBkm* pVirtual = nullptr) = 0; //Enable/disable bookmarks virtual mode.
		virtual void ClearData() = 0; //Clears all data from HexCtrl's view (not touching data itself).
		virtual bool Create(const HEXCREATE& hcs) = 0;                       //Main initialization method.
		virtual bool CreateDialogCtrl(UINT uCtrlID, HWND hParent) = 0;       //Сreates custom dialog control.
		virtual void Destroy() = 0;                                          //Deleter.
		virtual void ExecuteCmd(EHexCmd enCmd) = 0;                          //Execute a command within the control.
		[[nodiscard]] virtual auto GetCacheSize()const->DWORD = 0;           //Returns Virtual/Message mode cache size.
		[[nodiscard]] virtual DWORD GetCapacity()const = 0;                  //Current capacity.
		[[nodiscard]] virtual ULONGLONG GetCaretPos()const = 0;              //Cursor position.
		[[nodiscard]] virtual auto GetColors()const->HEXCOLORS = 0;          //Current colors.
		[[nodiscard]] virtual auto GetData(HEXOFFSET ho)const->std::span<std::byte> = 0; //Get pointer to data offset, no matter what mode the control works in.
		[[nodiscard]] virtual auto GetDataSize()const->ULONGLONG = 0;        //Get currently set data size.
		[[nodiscard]] virtual int GetEncoding()const = 0;                    //Get current code page ID.
		[[nodiscard]] virtual long GetFontSize()const = 0;                   //Current font size.
		[[nodiscard]] virtual auto GetGroupMode()const->EHexDataSize = 0;    //Retrieves current data grouping mode.
		[[nodiscard]] virtual HMENU GetMenuHandle()const = 0;                //Context menu handle.
		[[nodiscard]] virtual auto GetPagesCount()const->ULONGLONG = 0;      //Get count of pages.
		[[nodiscard]] virtual auto GetPagePos()const->ULONGLONG = 0;         //Get current page a cursor stays at.
		[[nodiscard]] virtual DWORD GetPageSize()const = 0;                  //Current page size.
		[[nodiscard]] virtual auto GetSelection()const->std::vector<HEXOFFSET> = 0; //Get current selection.
		[[nodiscard]] virtual HWND GetWindowHandle(EHexWnd enWnd)const = 0;  //Retrieves control's window/dialog handle.
		virtual void GoToOffset(ULONGLONG ullOffset, int iRelPos = 0) = 0;   //Go (scroll) to a given offset.
		[[nodiscard]] virtual bool HasSelection()const = 0;    //Does currently have any selection or not.
		[[nodiscard]] virtual auto HitTest(POINT pt, bool fScreen = true)const->std::optional<HEXHITTEST> = 0; //HitTest given point.
		[[nodiscard]] virtual bool IsCmdAvail(EHexCmd enCmd)const = 0; //Is given Cmd currently available (can be executed)?
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
		virtual void SetColors(const HEXCOLORS& clr) = 0;      //Set all the control's colors.
		virtual bool SetConfig(std::wstring_view wstrPath) = 0;//Set configuration file, or "" for defaults.
		virtual void SetData(const HEXDATA& hds) = 0;          //Main method for setting data to display (and edit).	
		virtual void SetEncoding(int iCodePage) = 0;           //Code-page for text area.
		virtual void SetFont(const LOGFONTW* pLogFont) = 0;    //Set the control's new font. This font has to be monospaced.
		virtual void SetFontSize(UINT uiSize) = 0;             //Set the control's font size.
		virtual void SetGroupMode(EHexDataSize enMode) = 0;    //Set current "Group Data By" mode.
		virtual void SetMutable(bool fEnable) = 0;             //Enable or disable mutable/editable mode.
		virtual void SetOffsetMode(bool fHex) = 0;             //Set offset being shown as Hex or as Decimal.
		virtual void SetPageSize(DWORD dwSize, std::wstring_view wstrName = L"Page") = 0; //Set page size and name to draw the lines in-between.
		virtual void SetSelection(const std::vector<HEXOFFSET>& vecSel, bool fRedraw = true, bool fHighlight = false) = 0; //Set current selection.
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

	constexpr auto HEXCTRL_MSG_BKMCLICK { 0x0100U };    //Bookmark clicked.
	constexpr auto HEXCTRL_MSG_CARETCHANGE { 0x0101U }; //Caret position changed.
	constexpr auto HEXCTRL_MSG_CONTEXTMENU { 0x0102U }; //OnContextMenu triggered.
	constexpr auto HEXCTRL_MSG_DESTROY { 0x0103U };     //Indicates that HexCtrl is being destroyed.
	constexpr auto HEXCTRL_MSG_MENUCLICK { 0x0104U };   //User defined custom menu clicked.
	constexpr auto HEXCTRL_MSG_SELECTION { 0x0105U };   //Selection has been made.
	constexpr auto HEXCTRL_MSG_SETDATA { 0x0106U };     //Indicates that the data has changed.
	constexpr auto HEXCTRL_MSG_VIEWCHANGE { 0x0107U };  //View of the control has changed.

	/*******************Setting a manifest for ComCtl32.dll version 6.***********************/
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
}