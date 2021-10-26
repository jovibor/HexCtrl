## **Hex Control, C++/MFC**
![](docs/img/hexctrl_mainwnd.jpg)
## Table of Contents
* [Introduction](#introduction)
* [Installation](#installation)
  * [Building From The Sources](#building-from-the-sources)
  * [Dynamic Link Library](#dynamic-link-library)
  * [IHexCtrlPtr](#ihexctrlptr)
  * [Namespace](#namespace)
* [Creating](#creating)
  * [Classic Approach](#classic-approach)
  * [In Dialog](#in-dialog)
* [Set The Data](#set-the-data)
* [Virtual Data Mode](#virtual-data-mode)
* [Virtual Bookmarks](#virtual-bookmarks)
* [Custom Colors](#custom-colors)
* [Methods](#methods) <details><summary>_Expand_</summary>
  * [BkmAdd](#bkmadd)
  * [BkmClearAll](#bkmclearall)
  * [BkmGetByID](#bkmgetbyid)
  * [BkmGetByIndex](#bkmgetbyindex)
  * [BkmGetCount](#bkmgetcount)
  * [BkmHitTest](#bkmhittest)
  * [BkmRemoveByID](#bkmremovebyid)
  * [BkmSetVirtual](#bkmsetvirtual)
  * [ClearData](#cleardata)
  * [Create](#create)
  * [CreateDialogCtrl](#createdialogctrl)
  * [Destroy](#destroy)
  * [ExecuteCmd](#executecmd)
  * [GetCacheSize](#getcachesize)
  * [GetCapacity](#getcapacity)
  * [GetCaretPos](#getcaretpos)
  * [GetColors](#getcolors)
  * [GetData](#getdata)
  * [GetDataSize](#getdatasize)
  * [GetEncoding](#getencoding)
  * [GetFont](#getfont)
  * [GetGroupMode](#getgroupmode)
  * [GetMenuHandle](#getmenuhandle)
  * [GetPagesCount](#getpagescount)
  * [GetPagePos](#getpagepos)
  * [GetPageSize](#getpagesize)
  * [GetSelection](#getselection)
  * [GetWindowHandle](#getwindowhandle)
  * [GoToOffset](#gotooffset)
  * [HasSelection](#hasselection)
  * [HitTest](#hittest)
  * [IsCmdAvail](#iscmdavail)
  * [IsCreated](#iscreated)
  * [IsDataSet](#isdataset)
  * [IsMutable](#ismutable)
  * [IsOffsetAsHex](#isoffsetashex)
  * [IsOffsetVisible](#isoffsetvisible)
  * [IsVirtual](#isvirtual)
  * [ModifyData](#modifydata)
  * [Redraw](#redraw)
  * [SetCapacity](#setcapacity)
  * [SetCaretPos](#setcaretpos)
  * [SetColors](#setcolors)
  * [SetConfig](#setconfig)
  * [SetData](#setdata)
  * [SetEncoding](#setencoding)
  * [SetFont](#setfont)
  * [SetGroupMode](#setgroupmode)
  * [SetMutable](#setmutable)
  * [SetOffsetMode](#setoffsetmode)
  * [SetPageSize](#setpagesize)
  * [SetRedraw](#setredraw)
  * [SetSelection](#setselection)
  * [SetWheelRatio](#setwheelratio)
   </details>
* [Structures](#structures) <details><summary>_Expand_</summary>
  * [HEXBKM](#hexbkm)
  * [HEXBKMINFO](#hexbkminfo)
  * [HEXCOLOR](#hexcolor)
  * [HEXCOLORINFO](#hexcolorinfo)
  * [HEXCOLORS](#hexcolors)
  * [HEXCREATE](#hexcreate)
  * [HEXDATA](#hexdata)
  * [HEXDATAINFO](#hexdatainfo)
  * [HEXHITTEST](#hexhittest)
  * [HEXMENUINFO](#hexmenuinfo)
  * [HEXMODIFY](#hexmodify)
  * [HEXSPAN](#hexspan)
  * [HEXVISION](#hexvision)
  </details>
* [Enums](#enums) <details><summary>_Expand_</summary>
  * [EHexCmd](#ehexcmd)
  * [EHexCreateMode](#ehexcreatemode)
  * [EHexDataSize](#ehexdatasize)
  * [EHexModifyMode](#ehexmodifymode)
  * [EHexOperMode](#ehexopermode)
  * [EHexWnd](#ehexwnd)
   </details>
* [Notification Messages](#notification-messages) <details><summary>_Expand_</summary>
  * [HEXCTRL_MSG_BKMCLICK](#hexctrl_msg_bkmclicked) 
  * [HEXCTRL_MSG_CARETCHANGE](#hexctrl_msg_caretchange)
  * [HEXCTRL_MSG_CONTEXTMENU](#hexctrl_msg_contextmenu)
  * [HEXCTRL_MSG_DESTROY](#hexctrl_msg_destroy)
  * [HEXCTRL_MSG_MENUCLICK](#hexctrl_msg_menuclick)
  * [HEXCTRL_MSG_SELECTION](#hexctrl_msg_selection)
  * [HEXCTRL_MSG_SETDATA](#hexctrl_msg_setdata)
  * [HEXCTRL_MSG_VIEWCHANGE](#hexctrl_msg_viewchange)
   </details>
* [Exported Functions](#exported-functions) <details><summary>_Expand_</summary>
  * [CreateRawHexCtrl](#createrawhexctrl)
  * [GetHexCtrlInfo](#gethexctrlinfo)
  * [HEXCTRLINFO](#hexctrlinfo)
   </details>
* [Positioning and Sizing](#positioning-and-sizing)
* [Appearance](#appearance)
* [Licensing](#licensing)

## [](#)Introduction
**HexCtrl** is a very featured hex viewer/editor control written in **C++** with the help of the **MFC** library.

It's implemented as a pure abstract interface and therefore can be used in your app even if you don't use **MFC** directly. It's written with **/std:c++20** standard in **Visual Studio 2019**.

### The main features of the **HexCtrl**:
* View and edit data up to **16EB** (exabyte)
* Works in two data modes, **Memory** and [**Virtual Data Mode**](#virtual-data-mode)
* Fully featured **Bookmarks Manager**
* Fully featured **Search and Replace**
* Changeable encoding for the text area
* Many options to **Copy/Paste** to/from clipboard
* **Undo/Redo**
* Modify data with **Filling** and many predefined **Operations** options
* Ability to visually divide data into [pages](#setpagesize)
* Print whole document/pages range/selection
* Set individual colors for the data chunks with [**Custom Colors**](#ihexvirtcolors)
* Customizable look and appearance
* [Assignable keyboard shortcuts](#setconfig) via external config file
* Written with **/std:c++20** standard conformance

![](docs/img/hexctrl_operationswnd.jpg)

## [](#)Installation
The **HexCtrl** can be used in two different ways:  
* Building from the sources as a part of your project 
* Using as a *.dll*.

### [](#)Building From The Sources
The building process is quite simple:
1. Copy *HexCtrl* folder into your project's directory.
2. Add all files from the *HexCtrl* folder into your project, except for  
*HexCtrl/dep/rapidjson/rapidjson-amalgam.h* (header-only lib).
3. Add `#include "HexCtrl/HexCtrl.h"` where you suppose to use the **HexCtrl**.
4. Declare [**HexCtrl**'s namespace](#namespace): `using namespace HEXCTRL;`
5. Declare [`IHexCtrlPtr`](#ihexctrlptr) member variable: `IHexCtrlPtr myHex { CreateHexCtrl() };`
6. [Create](#creating) control instance.

If you want to build **HexCtrl** from the sources in non **MFC** app you will have to:
1. Add support for **Use MFC in a Shared DLL** in your project settings.
2. Uncomment the line `//#define HEXCTRL_MANUAL_MFC_INIT` in `HexCtrl.h` header file.

### [](#)Dynamic Link Library
To use **HexCtrl** as the *.dll* do the following:
1. Copy *HexCtrl.h* and *HexCtrlDefs.h* files into your project's folder.
2. Copy *HexCtrl.lib* file into your project's folder, so that linker can see it.
3. Put *HexCtrl.dll* file next to your *.exe* file.
4. Add the following line where you suppose to use the control:
```cpp
#define HEXCTRL_SHARED_DLL //You can alternatively uncomment this line in HexCtrl.h.
#include "HexCtrl.h"` 
```
5. Declare [`IHexCtrlPtr`](#ihexctrlptr) member variable: `IHexCtrlPtr myHex { CreateHexCtrl() };`
6. [Create](#creating) control instance.

To build *HexCtrl.dll* and *HexCtrl.lib* use the *DLL Project/DLL Project.vcxproj* **Visual Studio** project file.

#### Remarks:
**HexCtrl**'s *.dll* is built with **MFC Static Linking**. So even if you are to use it in your own **MFC** project, even with different **MFC** version, there should not be any interferences

Building **HexCtrl** with **MFC Shared DLL** turned out to be a little tricky. Even with the help of `AFX_MANAGE_STATE(AfxGetStaticModuleState())` macro there always were **MFC** debug assertions, which origins quite hard to comprehend.

### [](#)IHexCtrlPtr
`IHexCtrlPtr` is, in fact, a pointer to a `IHexCtrl` pure abstract base class, wrapped either in `std::unique_ptr` or `std::shared_ptr`. You can choose whatever is best for your needs by define or undefine/comment-out the `HEXCTRL_IHEXCTRLPTR_UNIQUEPTR` macro in *HexCtrl.h*.  
By default `HEXCTRL_IHEXCTRLPTR_UNIQUEPTR` is defined, thus `IHexCtrlPtr` is an alias for `std::unique_ptr<IHexCtrl>`.

This wrapper is used mainly for convenience, so you don't have to bother about object lifetime, it will be destroyed automatically.
That's why there is a call to the factory function `CreateHexCtrl()` - to properly initialize a pointer.

If you, for some reason, need a raw interface pointer, you can directly call [`CreateRawHexCtrl`](#createrawhexctrl) function, which returns `IHexCtrl` interface pointer, but in this case you will need to call [`Destroy`](#destroy) method manually afterwards, to destroy `IHexCtrl` object.

### [](#)Namespace
**HexCtrl** uses its own namespace `HEXCTRL`.  
So it's up to you, whether to use namespace prefix before declarations:
```cpp
HEXCTRL::
```
or to define namespace globally, in the source file's beginning:
```cpp
using namespace HEXCTRL;
```

## [](#)Creating

### [](#)Classic Approach
[`Create`](#create) is the first method you call to create **HexCtrl** instance. It takes [`HEXCREATE`](#hexcreate) reference as an argument.

You can choose whether control will behave as a *child* or independent *popup* window, by setting `enCreateMode` member of this struct to [`EHexCreateMode::CREATE_CHILD`](#ehexcreatemode) or [`EHexCreateMode::CREATE_POPUP`](#ehexcreatemode) respectively.
```cpp
HEXCREATE hcs;
hcs.enCreateMode = EHexCreateMode::CREATE_POPUP;
hcs.hwndParent = m_hWnd;
m_myHex->Create(hcs);
```
For all available options see [`HEXCREATE`](#hexcreate) description.

### [](#)In Dialog
To use **HexCtrl** within *Dialog* you can, of course, create it with the [Classic Approach](#classic-approach), call [`Create`](#create) method and provide all the necessary information.

But there is another option you can use:
1. Put **Custom Control** from the **Toolbox** in **Visual Studio** dialog designer into your dialog template and make it desirable size.  
![](docs/img/hexctrl_vstoolbox.jpg) ![](docs/img/hexctrl_vscustomctrl.jpg)
2. Go to the **Properties** of that control and in the **Class** field, within the **Misc** section, type: <kbd>HexCtrl</kbd>.  
Give the control appropriate **ID** of your choice (<kbd>IDC_MY_HEX</kbd> in this example).  
Also, here you can set the control's **Dynamic Layout** properties, so that control behaves appropriately when dialog is being resized.  
![](docs/img/hexctrl_vsproperties.jpg)
3. Declare [`IHexCtrlPtr`](#ihexctrlptr) member variable within your dialog class:
```cpp
IHexCtrlPtr m_myHex { CreateHexCtrl() };
```
4. Call [`CreateDialogCtrl`](#createdialogctrl ) method from your dialog's `OnInitDialog` method.
```cpp
BOOL CMyDialog::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    
    m_myHex->CreateDialogCtrl(IDC_MY_HEX, m_hWnd);
}
```

## [](#)Set the Data
To set a data for the **HexCtrl** the [`SetData`](#setdata) method is used.  
The code below shows how to construct [`HexCtrl`](#ihexctrlptr) object and display first `0x1FF` bytes of the current app's memory:
```cpp
IHexCtrlPtr myHex { CreateHexCtrl() };

HEXCREATE hcs;
hcs.hwndParent = m_hWnd;
hcs.rect = {0, 0, 600, 400}; //Window rect.
myHex->Create(hcs);

HEXDATA hds;
hds.spnData = { reinterpret_cast<std::byte*>(GetModuleHandle(nullptr)), 0x1FF };
myHex->SetData(hds);
```
The next example shows how to display `std::string`'s text as hex data:
```cpp
std::string str = "My string";
HEXDATA hds;
hds.spnData = { reinterpret_cast<std::byte*>(str.data()), str.size() };
myHex->SetData(hds);
```

## [](#)Virtual Data Mode
Besides the standard default mode, when **HexCtrl** just holds a pointer to some bytes in memory, it also has additional Virtual mode.  
This mode can be quite useful, for instance in cases where you need to display a very large amount of data that can't fit in memory all at once.

If `HEXDATA::pHexVirtData` pointer is set then all the data routine will be done through it. This pointer is of `IHexVirtData` class type, which is a pure abstract base class.
You have to derive your own class from it and implement all its public methods.
```cpp
class IHexVirtData
{
public:
    virtual void OnHexGetData(HEXDATAINFO&) = 0;       //Data to get.
    virtual void OnHexSetData(const HEXDATAINFO&) = 0; //Data to set, if mutable.
};
```
Then provide a pointer to the created object of this derived class prior to call to [`SetData`](#setdata) method, through the `HEXDATA::pHexVirtData`.

## [](#)Virtual Bookmarks
**HexCtrl** has innate functional to work with any amount of bookmarked regions. These regions can be assigned with individual background and text colors and description.

But if you have some big and complicated data logic and want to handle all these regions yourself, you can do it with the help of Virtual Bookmarks. To enable it call the [`BkmSetVirtual`](#bkmsetvirtual) method.  

The main method of the `IHexVirtBkm` interface is `HitTest`. It takes data offset and returns pointer to [`HEXBKM`](#hexbkm) if there is a bookmark, or `nullptr` otherwise.

```cpp
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
```
## [](#)Custom Colors
### [](#)IHexVirtColors
This interface is used to set custom bk/text colors for the data offset.  
In order to use this feature the [`HEXDATA::pHexVirtColors`](#hexdata) member must be set to a valid class inherited from this interface prior to calling [`SetData`](#setdata) method.
```cpp
class IHexVirtColors
{
public:
    void OnHexGetColor(HEXCOLORINFO&) = 0;
};
```
The `OnHexGetColor` method of this interface takes [`HEXCOLORINFO`](#hexcolorinfo) struct as an argument. The `pClr` member of this struct must point to a valid [`HEXCOLOR`](#hexcolor) struct after method completes, or `nullptr` for default colors.

## [](#)Methods
The **HexCtrl** has plenty of methods that you can use to customize its appearance, and to manage its behavior.

### [](#)BkmAdd
```cpp
ULONGLONG BkmAdd(const HEXBKM& hbs, bool fRedraw = false)
```
Adds new bookmark to the control. Uses [`HEXBKM`](#hexbkm) as an argument. Returns created bookmark's id.
#### Example
```cpp
HEXBKM hbs;
hbs.vecSpan.emplace_back(HEXSPAN { 0x1, 10 });
hbs.clrBk = RGB(0, 255, 0);
hbs.clrText = RGB(255, 255, 255);
hbs.wstrDesc = L"My first bookmark, with green bk and white text.";

myHex.BkmAdd(hbs);
```

### [](#)BkmClearAll
```cpp
void BkmClearAll();
```
Clears all bookmarks.

### [](#)BkmGetByID
```cpp
BkmGetByID(ULONGLONG ullID)->HEXBKM*;
```
Get bookmark by ID.

### [](#)BkmGetByIndex
```cpp
auto BkmGetByIndex(ULONGLONG ullIndex)->HEXBKM*;
```
Get bookmark by Index.

### [](#)BkmGetCount
```cpp
ULONGLONG BkmGetCount()const;
```
Get bookmarks' count.

### [](#)BkmHitTest
```cpp
auto BkmHitTest(ULONGLONG ullOffset)->HEXBKM*;
```
Test given offset and retrieves pointer to [`HEXBKM`](#hexbkm) if it contains a bookmark.

### [](#)BkmRemoveByID
```cpp
void BkmRemoveByID(ULONGLONG ullID);
```
Removes bookmark by the given ID.

### [](#)BkmSetVirtual
```cpp
void BkmSetVirtual(bool fEnable, IHexVirtBkm* pVirtual = nullptr);
```
Enables or disables bookmarks virtual mode.

### [](#)ClearData
```cpp
void ClearData();
```
Clears data from the **HexCtrl** view, not touching data itself.

### [](#)Create
```cpp
bool Create(const HEXCREATE& hcs);
```
Main initialization method.  
Takes [`HEXCREATE`](#hexcreate) as argument. Returns `true` if created successfully, `false` otherwise.

### [](#)CreateDialogCtrl
```cpp
bool CreateDialogCtrl(UINT uCtrlID, HWND hwndDlg);
```
Creates **HexCtrl** from **Custom Control** dialog's template. Takes control's **id**, and dialog's window **handle** as arguments. See **[Creating](#in-dialog)** section for more info.

### [](#)Destroy
```cpp
void Destroy();
```
Destroys the control.  
You only invoke this method if you use a raw `IHexCtrl` pointer obtained by the call to `CreateRawHexCtrl` function. Otherwise don't use it.

**Remarks**  
You usually don't need to call this method unless you use **HexCtrl** through the raw pointer obtained by [`CreateRawHexCtrl`](#createrawhexctrl) factory function.  
If you use **HexCtrl** in standard way, through the [`IHexCtrlPtr`](#ihexctrlptr) pointer, obtained by `CreateHexCtrl` function, this method will be called automatically.

### [](#)ExecuteCmd
```cpp
void ExecuteCmd(EHexCmd enCmd)const;
```
Executes one of the predefined commands of [`EHexCmd`](#ehexcmd) enum. All these commands are basically replicating control's inner menu.

### [](#)GetCacheSize
```cpp
auto GetCacheSize()const->DWORD;
```
Returns current cache size set in [`HEXDATA`](#hexdata).

### [](#)GetCapacity
```cpp
DWORD GetCapacity()const;
```
Returns current capacity.

### [](#)GetCaretPos
```cpp
ULONGLONG GetCaretPos()const;
```
Retrieves current caret position offset.

### [](#)GetColors
```cpp
auto GetColors()const->HEXCOLORS;
```
Returns current [`HEXCOLORS`](#hexcolors).

### [](#)GetData
```cpp
auto GetData(HEXSPAN hss)const->std::byte*;
```
Returns a pointer to a data offset no matter what mode the control works in.  

Note that in the Virtual mode returned data size can not exceed current [cache size](#getcachesize), and therefore may be less than the size acquired.  
In the default mode returned pointer is just an offset from the data pointer set in the [`SetData`](#setdata) method.

### [](#)GetDataSize
```cpp
auto GetDataSize()const->ULONGLONG;
```
Returns currently set data size.

### [](#)GetEncoding
```cpp
int GetEncoding()const;
```
Get code page that is currently in use.

### [](#)GetFont
```cpp
void GetFont(LOGFONTW& lf);
```
Retrieves current font through `lf` arg.

### [](#)GetGroupMode
```cpp
auto GetGroupMode()const->EHexDataSize;
```
Retrieves current data grouping mode.

### [](#)GetMenuHandle
```cpp
HMENU GetMenuHandle()const;
```
Retrieves the `HMENU` handle of the control's context menu. You can use this handle to customize menu for your needs.

Control's internal menu uses menu `ID`s in range starting from `0x8001`. So if you wish to add your own new menu, assign menu `ID` starting from `0x9000` to not interfere.

When user clicks custom menu, control sends `WM_NOTIFY` message to its parent window with `LPARAM` pointing to [`HEXMENUINFO`](#hexmenuinfo) with its `hdr.code` member set to `HEXCTRL_MSG_MENUCLICK`. `wMenuID` field will be holding `ID` of the menu clicked.

### [](#)GetPagesCount
```cpp
DWORD GetPageSize()const;
```
Get current count of pages set by [`SetPageSize`](#setpagesize).

### [](#)GetPagePos
```cpp
auto GetPagePos()->ULONGLONG const;
```
Get current page a cursor stays at.

### [](#)GetPageSize
```cpp
DWORD GetPageSize()const;
```
Get current page size set by [`SetPageSize`](#setpagesize).

### [](#)GetSelection
```cpp
auto GetSelection()const->std::vector<HEXSPAN>;
```
Returns `std::vector` with the offsets and sizes of the current selection.

### [](#)GetWindowHandle
```cpp
HWND GetWindowHandle(EHexWnd enWnd)const
```
Retrieves window handle for one of the **HexCtrl**'s windows. Takes [`EHexWnd`](#ehexwnd) enum as an argument.

### [](#)GoToOffset
```cpp
void GoToOffset(ULONGLONG ullOffset, int iRelPos = 0)
```
Go to a given offset. The second argument `iRelPos` may take-in three different values:  
* `-1` - offset will appear at the top line.
* &nbsp; `0` - offset will appear at the middle.
* &nbsp; `1` - offset will appear at the bottom line.

### [](#)HasSelection
```cpp
bool HasSelection()const;
```
Returns `true` if **HexCtrl** has any area selected.

### [](#)HitTest
```cpp
auto HitTest(POINT pt, bool fScreen = true)const->std::optional<HEXHITTEST>
```
Hit testing of given point in a screen `fScreen = true`, or client `fScreen = false` coordinates. In case of success returns [`HEXHITTEST`](#hexhittest) structure.

### [](#)IsCmdAvail
```cpp
bool IsCmdAvail(EHexCmd enCmd)const;
```
Returns `true` if given command can be executed at the moment, `false` otherwise.

### [](#)IsCreated
```cpp
bool IsCreated()const;
```
Shows whether **HexCtrl** is created or not yet.

### [](#)IsDataSet
```cpp
bool IsDataSet()const;
```
Shows whether a data was set to **HexCtrl** or not

### [](#)IsMutable
```cpp
bool IsMutable()const;
```
Shows whether **HexCtrl** is currently in edit mode or not.

### [](#)IsOffsetAsHex
```cpp
bool IsOffsetAsHex()const;
```
Is "Offset" currently represented (shown) as Hex or as Decimal. It can be changed by double clicking at offset area.

### [](#)IsOffsetVisible
```cpp
HEXVISION IsOffsetVisible(ULONGLONG ullOffset)const;
```
Checks for offset visibility and returns [`HEXVISION`](#hexvision) as a result.

### [](#)IsVirtual
```cpp
bool IsVirtual()const;
```
Returns `true` if **HexCtrl** currently works in [Virtual Data Mode](#virtual-data-mode).

### [](#)ModifyData
```cpp
void ModifyData(const HEXMODIFY& hms);
```
Modify data currently set in **HexCtrl**, see the [`HEXMODIFY`](#hexmodify) struct for details.

### [](#)Redraw
```cpp
void Redraw();
```
Redraws main window.

### [](#)SetCapacity
```cpp
void SetCapacity(DWORD dwCapacity);
```
Sets **HexCtrl**'s current capacity.

### [](#)SetCaretPos
```cpp
void SetCaretPos(ULONGLONG ullOffset, bool fHighLow = true, bool fRedraw = true);
```
Sets the caret to the given offset. The `fHighLow` flag shows which part of the hex chunk, low or high, a caret must be set to.

### [](#)SetColors
```cpp
void SetColors(const HEXCOLORS& clr);
```
Sets all the colors for the control. Takes [`HEXCOLORS`](#hexcolors) as the argument.

### [](#)SetConfig
```cpp
bool SetConfig(std::wstring_view wstrPath);
```
Sets the path to a JSON config file with keybindings to use in **HexCtrl**, or empty path (`L""`) for default. This file is using [`EHexCmd`](#ehexcmd) enum values as keys and strings array as values:
```json
{
    "CMD_DLG_SEARCH": [ "ctrl+f", "ctrl+h" ],
    "CMD_SEARCH_NEXT": [ "f3" ],
    "CMD_SEARCH_PREV": [ "shift+f3" ]
}
```
For default values see the [`HexCtrl/res/keybind.json`](https://github.com/jovibor/HexCtrl/blob/master/HexCtrl/res/keybind.json) file from the project sources.

### [](#)SetData
```cpp
void SetData(const HEXDATA& hds);
```
Main method to set a data to display in read-only or mutable modes. Takes [`HEXDATA`](#hexdata) as an  argument.

### [](#)SetEncoding
```cpp
void SetEncoding(int iCodePage);
```
Sets the code page for the **HexCtrl**'s text area. Takes [code page identifier](https://docs.microsoft.com/en-us/windows/win32/intl/code-page-identifiers) as an argument, or `-1` for default ASCII-only characters.  

**Note:** Code page identifier must represent [Single-byte Character Set](https://docs.microsoft.com/en-us/windows/win32/intl/single-byte-character-sets). Multi-byte character sets are not currently supported.

### [](#)SetFont
```cpp
void SetFont(const LOGFONTW& lf);
```
Sets a new font for the **HexCtrl**. This font has to be monospaced.

### [](#)SetGroupMode
```cpp
void SetGroupMode(EHexDataSize enGroupMode);
```
Sets current data grouping mode. See [`EHexDataSize`](#ehexdatasize) for more info.

### [](#)SetMutable
```cpp
void SetMutable(bool fEnable);
```
Enables or disables mutable mode. In mutable mode all the data can be modified.

### [](#)SetOffsetMode
```cpp
void SetOffsetMode(bool fHex);
```
Sets offset area being shown as Hex (`fHex=true`) or as Decimal (`fHex=false`).

### [](#)SetPageSize
```cpp
void SetPageSize(DWORD dwSize, const wchar_t* wstrName = L"Page");
```
Sets the size of the page to draw the divider line between. This size should be multiple to the current [capacity](#setcapacity) size to take effect. The second argument sets the name to be displayed in the bottom info area of the **HexCtrl** ("Page", "Sector", etc...).  
To remove the divider just set `dwSize` to 0.

### [](#)SetRedraw
```cpp
void SetRedraw(bool fRedraw);
```
Should the main **HexCtrl** window be redrawn or not. E.g. should the `WM_PAINT` message be handled or not.

### [](#)SetSelection
```cpp
void SetSelection(const std::vector<HEXSPAN>& vecSel, bool fRedraw = true, bool fHighlight = false);
```
Sets current selection or highlight in the selection, if `fHighlight` is `true`.

### [](#)SetWheelRatio
```cpp
void SetWheelRatio(double dbRatio)
```
Sets the ratio for how much to scroll with mouse-wheel.

## [](#)Structures
Below are listed all **HexCtrl**'s structures.

### [](#)HEXBKM
Structure for bookmarks, used in [`BkmAdd`](#BkmAdd) method.  
```cpp
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
```
The member `vecSpan` is of a `std::vector<HEXSPAN>` type because a bookmark may have few non adjacent areas. For instance, when selection is made as a block, with <kbd>Alt</kbd> pressed.


### [](#)HEXBKMINFO
Bookmark information struct.
```cpp
struct HEXBKMINFO
{
    NMHDR   hdr { };  //Standard Windows header.
    PHEXBKM pBkm { }; //Bookmark pointer.
};
```

### [](#)HEXCOLOR
**HexCtrl** custom colors.
```cpp
struct HEXCOLOR
{
    COLORREF clrBk { };   //Bk color.
    COLORREF clrText { }; //Text color.
};
using PHEXCOLOR = HEXCOLOR*;
```

### [](#)HEXCOLORINFO
Struct for hex chunks' color information.
```cpp
struct HEXCOLORINFO
{
    NMHDR     hdr { };       //Standard Windows header.
    ULONGLONG ullOffset { }; //Offset for the color.
    PHEXCOLOR pClr { };      //Pointer to the color struct.
};
```

### [](#)HEXCOLORS
This structure describes all control's colors. All these colors have their default values.
```cpp
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
    COLORREF clrBkSelected { GetSysColor(COLOR_HIGHLIGHT) };     //Background color of the selected Hex/ASCII.
    COLORREF clrBkDataInterp { RGB(147, 58, 22) };               //Data Interpreter Bk color.
    COLORREF clrBkInfoRect { GetSysColor(COLOR_BTNFACE) };       //Background color of the bottom "Info" rect.
    COLORREF clrBkCaret { RGB(0, 0, 255) };                      //Caret background color.
    COLORREF clrBkCaretSelect { RGB(0, 0, 200) };                //Caret background color in selection.
};
```

### [](#)HEXCREATE
The main initialization struct used for control creation.
```cpp
struct HEXCREATE
{
    EHexCreateMode  enCreateMode { EHexCreateMode::CREATE_CHILD }; //Creation mode of the HexCtrl window.
    HEXCOLORS       stColor { };          //All the control's colors.
    HWND            hwndParent { };       //Parent window pointer.
    const LOGFONTW* pLogFont { };         //Font to be used instead of default, it has to be monospaced.
    RECT            rect { };             //Initial rect. If null, the window is screen centered.
    UINT            uID { };              //Control ID.
    DWORD           dwStyle { };          //Window styles, 0 for default.
    DWORD           dwExStyle { };        //Extended window styles, 0 for default.
    double          dbWheelRatio { 1.0 }; //Ratio for how much to scroll with mouse-wheel.
};
```

### [](#)HEXDATA
Main struct to set a data to display in the control.
```cpp
struct HEXDATA
{
    std::span<std::byte> spnData { };              //Data to display.
    IHexVirtData*        pHexVirtData { };         //Pointer for Virtual mode.
    IHexVirtColors*      pHexVirtColors { };       //Pointer for Custom Colors class.
    DWORD                dwCacheSize { 0x800000 }; //In Virtual mode max cached size of data to fetch.
    bool                 fMutable { false };       //Is data mutable (editable) or read-only.
    bool                 fHighLatency { false };   //Do not redraw window until scrolling completes.
};
```

### [](#)HEXDATAINFO
Struct for a data information used in [`IHexVirtData`](#virtual-data-mode).
```cpp
struct HEXDATAINFO
{
    NMHDR                hdr { };       //Standard Windows header.
    HEXSPAN              stHexSpan { }; //Offset and size of the data bytes.
    std::span<std::byte> spnData { };   //Data span.
};
```

### [](#)HEXHITTEST
Structure is used in [`HitTest`](#hittest) method.
```cpp
struct HEXHITTEST
{
    ULONGLONG ullOffset { };      //Offset.
    bool      fIsAscii { false }; //Is cursor at ASCII part or at Hex.
};
```

### [](#)HEXMODIFY
This struct is used to represent data modification parameters.  
When `enModifyMode` is set to `EHexModifyMode::MODIFY_ONCE`, bytes from `pData` just replace corresponding data bytes as is.  
If `enModifyMode` is equal to `EHexModifyMode::MODIFY_REPEAT` then block by block replacement takes place few times.  
For example, if:
* `SUM(vecSpan.ullSize) == 9`
* `spnData.size() == 3`
* `enModifyMode` is set to `EHexModifyMode::MODIFY_REPEAT`
* bytes in memory at `vecSpan.ullOffset` are `010203040506070809`
* bytes pointed to by `spnData.data()` are `030405`

then, after modification, bytes at `vecSpan.ullOffset` will become `030405030405030405`.  

If `enModifyMode` is equal to `EHexModifyMode::MODIFY_OPERATION` then `enOperMode` comes into play, showing what kind of operation must be performed on data.
```cpp
struct HEXMODIFY
{
    EHexModifyMode       enModifyMode { EHexModifyMode::MODIFY_ONCE }; //Modify mode.
    EHexOperMode         enOperMode { };       //Operation mode, used only in MODIFY_OPERATION mode.
    EHexDataSize         enOperSize { };       //Operation data size.
    std::span<std::byte> spnData { };          //Data span.
    std::vector<HEXSPAN> vecSpan { };          //Vector of data offsets and sizes.
    bool                 fBigEndian { false }; //Treat the data as a big endian, used only in MODIFY_OPERATION mode.
};
```

### [](#)HEXMENUINFO
Menu information struct.
```cpp
struct HEXMENUINFO
{
    NMHDR hdr { };     //Standard Windows header.
    POINT pt { };      //Mouse position when clicked.
    WORD  wMenuID { }; //Menu identifier.
};
```

### [](#)HEXSPAN
This struct is used mostly in selection and bookmarking routines. It holds offset and size of the data region.
```cpp
struct HEXSPAN
{
    ULONGLONG ullOffset { };
    ULONGLONG ullSize { };
};
```

### [](#)HEXVISION
This struct is returned from [`IsOffsetVisible`](#isoffsetvisible) method. Two members `i8Vert` and `i8Horz` represent vertical and horizontal visibility respectively. These members can be in three different states:
* `-1` — offset is higher, or at the left, of the visible area.
* &nbsp; `1` — offset is lower, or at the right.
* &nbsp; `0` — offset is visible.

```cpp
struct HEXVISION
{
    std::int8_t i8Vert { }; //Vertical offset.
    std::int8_t i8Horz { }; //Horizontal offset.
    operator bool() { return i8Vert == 0 && i8Horz == 0; }; //For test simplicity: if(IsOffsetVisible()).
};
```

## [](#)Enums

### [](#)EHexCmd
Enum of commands that can be executed within **HexCtrl**.
```cpp
enum class EHexCmd : std::uint8_t
{
    CMD_DLG_SEARCH = 0x01, CMD_SEARCH_NEXT, CMD_SEARCH_PREV,
    CMD_SHOWDATA_BYTE, CMD_SHOWDATA_WORD, CMD_SHOWDATA_DWORD, CMD_SHOWDATA_QWORD,
    CMD_BKM_ADD, CMD_BKM_REMOVE, CMD_BKM_NEXT, CMD_BKM_PREV, CMD_BKM_CLEARALL, CMD_DLG_BKM_MANAGER,
    CMD_CLIPBOARD_COPY_HEX, CMD_CLIPBOARD_COPY_HEXLE, CMD_CLIPBOARD_COPY_HEXFMT, CMD_CLIPBOARD_COPY_TEXT,
    CMD_CLIPBOARD_COPY_BASE64, CMD_CLIPBOARD_COPY_CARR, CMD_CLIPBOARD_COPY_GREPHEX, CMD_CLIPBOARD_COPY_PRNTSCRN,
    CMD_CLIPBOARD_PASTE_HEX, CMD_CLIPBOARD_PASTE_TEXT,
    CMD_DLG_MODIFY_OPERS, CMD_MODIFY_FILLZEROS, CMD_DLG_MODIFY_FILLDATA, CMD_MODIFY_UNDO, CMD_MODIFY_REDO,
    CMD_SEL_MARKSTART, CMD_SEL_MARKEND, CMD_SEL_ALL, CMD_SEL_ADDLEFT, CMD_SEL_ADDRIGHT, CMD_SEL_ADDUP, CMD_SEL_ADDDOWN,
    CMD_DLG_DATAINTERPRET, CMD_DLG_ENCODING,
    CMD_APPEARANCE_FONTINC, CMD_APPEARANCE_FONTDEC, CMD_APPEARANCE_CAPACINC, CMD_APPEARANCE_CAPACDEC,
    CMD_DLG_PRINT, CMD_DLG_ABOUT,
    CMD_CARET_LEFT, CMD_CARET_RIGHT, CMD_CARET_UP, CMD_CARET_DOWN,
    CMD_SCROLL_PAGEUP, CMD_SCROLL_PAGEDOWN, CMD_SCROLL_TOP, CMD_SCROLL_BOTTOM
};
```

### [](#)EHexCreateMode
Enum represents the modes that **HexCtrl**'s window can be created in.
```cpp
enum class EHexCreateMode : std::uint8_t
{
    CREATE_CHILD, CREATE_POPUP, CREATE_CUSTOMCTRL
};
```

### [](#)EHexDataSize
Data size to operate on, used in `EHexModifyMode::MODIFY_OPERATION` mode. Also used to set data grouping mode, in [`SetGroupMode`](#setgroupmode) method.
```cpp
enum class EHexDataSize : std::uint8_t
{
    SIZE_BYTE = 0x01, SIZE_WORD = 0x02, SIZE_DWORD = 0x04, SIZE_QWORD = 0x08
};
```

### [](#)EHexModifyMode
Enum of the data modification modes, used in [`HEXMODIFY`](#hexmodify).
```cpp
enum class EHexModifyMode : std::uint8_t
{
    MODIFY_ONCE, MODIFY_REPEAT, MODIFY_OPERATION, MODIFY_RAND_MT19937, MODIFY_RAND_FAST
};
```

### [](#)EHexOperMode
Enum of the data operation modes, used in [`HEXMODIFY`](#hexmodify) when `HEXMODIFY::enModifyMode` is set to `MODIFY_OPERATION`.
```cpp
enum class EHexOperMode : std::uint8_t
{
    OPER_ASSIGN, OPER_OR, OPER_XOR, OPER_AND, OPER_NOT, OPER_SHL, OPER_SHR, OPER_ROTL,
    OPER_ROTR, OPER_SWAP, OPER_ADD, OPER_SUBTRACT, OPER_MULTIPLY, OPER_DIVIDE,
    OPER_CEILING, OPER_FLOOR
};
```

### [](#)EHexWnd
Enum of all **HexCtrl**'s internal windows. This enum is used as an arg in [`GetWindowHandle`](#getwindowhandle) method to retrieve window's handle. 
```cpp
enum class EHexWnd : std::uint8_t
{
    WND_MAIN, DLG_BKMMANAGER, DLG_DATAINTERP, DLG_FILLDATA,
    DLG_OPERS, DLG_SEARCH, DLG_ENCODING, DLG_GOTO
};
```

## [](#)Notification Messages
During its work **HexCtrl** sends notification messages to [`HEXCREATE::hwndParent`](#hexcreate) window, about its different states, through **[WM_NOTIFY](https://docs.microsoft.com/en-us/windows/win32/controls/wm-notify)** mechanism.  
The `LPARAM` of the `WM_NOTIFY` message will contain a pointer to the **[NMHDR](https://docs.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-nmhdr)** standard windows struct. Depending on the notification message `LPARAM` can then be casted to a pointer to another struct, see message description for details.

### [](#)HEXCTRL_MSG_BKMCLICK
Sent if bookmark is clicked. `LPARAM` will contain pointer to a [`HEXBKMINFO`](#hexbkminfo) struct.

### [](#)HEXCTRL_MSG_CARETCHANGE
Sent when caret position has changed. `LPARAM` will contain pointer to a `NMHDR` struct.

### [](#)HEXCTRL_MSG_CONTEXTMENU
Sent when context menu is about to be displayed. `LPARAM` will contain pointer to a [`HEXMENUINFO`](#hexmenuinfo) struct.

### [](#)HEXCTRL_MSG_DESTROY
Sent to indicate that **HexCtrl** window is about to be destroyed. `LPARAM` will contain pointer to a `NMHDR` struct.

### [](#)HEXCTRL_MSG_MENUCLICK
Sent when user defined custom menu has been clicked. `LPARAM` will contain pointer to a [`HEXMENUINFO`](#hexmenuinfo) struct.

### [](#)HEXCTRL_MSG_SELECTION
Sent when selection has been made. `LPARAM` will contain pointer to a `NMHDR` struct.

### [](#)HEXCTRL_MSG_SETDATA
Sent to indicate that the data has changed. `LPARAM` will contain pointer to a `NMHDR` struct.

### [](#)HEXCTRL_MSG_VIEWCHANGE
Sent when **HexCtrl**'s view has changed, whether on resizing or scrolling. `LPARAM` will contain pointer to a `NMHDR` struct.

## [](#)Exported Functions
**HexCtrl** has few `"C"` interface functions which it exports when built as *.dll*.

### [](#)CreateRawHexCtrl
```cpp
extern "C" HEXCTRLAPI IHexCtrl* __cdecl CreateRawHexCtrl();
```
Main function that creates raw `IHexCtrl` interface pointer. You barely need to use this function in your code.  
See the [`IHexCtrlPtr`](#ihexctrlptr) section for more info.

### [](#)GetHexCtrlInfo
```cpp
extern "C" HEXCTRLAPI HEXCTRLINFO* __cdecl GetHexCtrlInfo();
```
Returns pointer to [`HEXCTRLINFO`](#hexctrlinfo), which is the **HexCtrl**'s service information structure.

### [](#)HEXCTRLINFO
Service structure for **HexCtrl**'s version information.
```cpp
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
```

## [](#)Positioning and Sizing
To properly resize and position your **HexCtrl**'s window you may have to handle `WM_SIZE` message in its parent window, in something like this way:
```cpp
void CMyWnd::OnSize(UINT nType, int cx, int cy)
{
    //...
    ::SetWindowPos(m_myHex->GetWindowHandle(EHexWnd::WND_MAIN), this->m_hWnd, 0, 0, cx, cy, SWP_NOACTIVATE | SWP_NOZORDER);
}
```

## [](#)Appearance
To change control's font size — <kbd>Ctrl+MouseWheel</kbd>  
To change control's capacity — <kbd>Ctrl+Shift+MouseWheel</kbd>

## [](#)Licensing
This software is available under **"The HexCtrl License"**, it is free for any **NON-COMMERCIAL** use.  
See the [LICENSE](https://github.com/jovibor/HexCtrl/blob/master/LICENSE) file.