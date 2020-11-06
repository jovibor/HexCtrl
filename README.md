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
* [Data Modes](#data-modes)
  * [Memory Data](#memory-data)
  * [Message Window](#message-window)
  * [Virtual Handler](#virtual-handler)
* [Virtual Bookmarks](#virtual-bookmarks)
* [IHexVirtColors](#ihexvirtcolors)
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
  * [GetCapacity](#getcapacity)
  * [GetCaretPos](#getcaretpos)
  * [GetColors](#getcolors)
  * [GetDataSize](#getdatasize)
  * [GetEncoding](#getencoding)
  * [GetFontSize](#getfontsize)
  * [GetGroupMode](#getgroupmode)
  * [GetMenuHandle](#getmenuhandle)
  * [GetPagesCount](#getpagescount)
  * [GetPagePos](#getpagepos)
  * [GetPageSize](#getpagesize)
  * [GetSelection](#getselection)
  * [GetWindowHandle](#getwindowhandle)
  * [GoToOffset](#gotooffset) 
  * [HitTest](#hittest)
  * [IsCmdAvail](#iscmdavail)
  * [IsCreated](#iscreated)
  * [IsDataSet](#isdataset)
  * [IsMutable](#ismutable)
  * [IsOffsetAsHex](#isoffsetashex)
  * [IsOffsetVisible](#isoffsetvisible)
  * [Redraw](#redraw)
  * [SetCapacity](#setcapacity)
  * [SetCaretPos](#setcaretpos)
  * [SetColors](#setcolors)
  * [SetConfig](#setconfig)
  * [SetData](#setdata)
  * [SetEncoding](#setencoding)
  * [SetFont](#setfont)
  * [SetFontSize](#setfontsize)
  * [SetGroupMode](#setgroupmode)
  * [SetMutable](#setmutable)
  * [SetOffsetMode](#setoffsetmode)
  * [SetPageSize](#setpagesize)
  * [SetSelection](#setselection)
  * [SetWheelRatio](#setwheelratio)
   </details>
* [Structures](#structures) <details><summary>_Expand_</summary>
  * [HEXBKMSTRUCT](#hexbkmstruct)
  * [HEXCOLOR](#hexcolor)
  * [HEXCOLORSSTRUCT](#hexcolorsstruct)
  * [HEXCREATESTRUCT](#hexcreatestruct)
  * [HEXDATASTRUCT](#hexdatastruct)
  * [HEXHITTESTSTRUCT](#hexhitteststruct)
  * [HEXNOTIFYSTRUCT](#hexnotifystruct)
  * [HEXSPANSTRUCT](#hexspanstruct)
  * [HEXVISSTRUCT](#hexvisstruct)
  * [EHexCmd](#ehexcmd)
  * [EHexCreateMode](#ehexcreatemode)
  * [EHexDataMode](#ehexdatamode)
  * [EHexGroupMode](#ehexgroupmode)
  * [EHexWnd](#ehexwnd)
   </details>
* [Notification Messages](#notification-messages) <details><summary>_Expand_</summary>
  * [HEXCTRL_MSG_BKMCLICK](#hexctrl_msg_bkmclicked) 
  * [HEXCTRL_MSG_CARETCHANGE](#hexctrl_msg_caretchange)
  * [HEXCTRL_MSG_CONTEXTMENU](#hexctrl_msg_contextmenu)
  * [HEXCTRL_MSG_DESTROY](#hexctrl_msg_destroy)
  * [HEXCTRL_MSG_GETDATA](#hexctrl_msg_getdata)
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
Being good low level library on top of **Windows API** in general, **MFC** was always lacking a good native controls support. This **HexControl** is an attempt to expand standard **MFC** functionality, because at the moment **MFC** doesn't provide support for such control.

This doesn't mean that **HexControl** is limited to use only in **MFC** environment. The control is implemented as a pure abstract interface, and can be used as a *child* or *popup* window in any place of your application.  
It is written and tested with **/std:c++17** in **Visual Studio 2019**, under the **Windows 10**.

### The main features of the **HexControl**:
* View and edit data up to **16EB** (exabyte)
* Work in three different data modes: **Memory**, **Message**, **Virtual**.
* Fully featured **Bookmarks manager**
* **Search and Replace...** for **Hex** values, **ASCII** or **UTF-16** text
* Change encoding for the text area
* Many options to **Copy/Paste** to/from clipboard
* **Undo/Redo**
* Modify data with **Filling** and many predefined **Operations** options
* Ability to visually divide data into [pages](#setpagesize)
* Print whole document/pages range/selection
* Set individual colors for the data chunks with [`IHexVirtColors`](#ihexvirtcolors) interface
* Cutomizable look and appearance
* [Assign keyboard shortcuts](#setconfig) via external config file
* Written with **/std:c++17** standard conformance

![](docs/img/hexctrl_operationswnd.jpg)

## [](#)Installation
The **HexControl** can be used in two different ways:  
* Building from the sources as a part of your project 
* Using as a *.dll*.

### [](#)Building From The Sources
The building process is quite simple:
1. Add all files from the *HexCtrl* folder (except for `/dep/` subfolder) into your project.
2. Add `#include "HexCtrl/HexCtrl.h"` where you suppose to use the **HexControl**.
3. Declare [`IHexCtrlPtr`](#ihexctrlptr) member variable: `IHexCtrlPtr myHex { CreateHexCtrl() };`
4. [Create](#creating) control instance.

If you want to build **HexControl** from the sources in non **MFC** app you will have to:
1. Add support for **Use MFC in a Shared DLL** in your project settings.
2. Uncomment the line `//#define HEXCTRL_MANUAL_MFC_INIT` in `HexCtrl.h` header file.

### [](#)Dynamic Link Library
To use **HexControl** as the *.dll* do the following:
1. Copy *HexCtrl.h* file into your project's folder.
2. Copy *HexCtrl.lib* file into your project's folder, so that linker can see it.
3. Put *HexCtrl.dll* file next to your *.exe* file.
4. Add the following line where you suppose to use the control:
```cpp
#define HEXCTRL_SHARED_DLL //You can alternatively uncomment this line in HexCtrl.h.
#include "HexCtrl.h"` 
```
5. Declare [`IHexCtrlPtr`](#ihexctrlptr) member variable: `IHexCtrlPtr myHex { CreateHexCtrl() };`
5. [Create](#creating) control instance.

To build *HexCtrl.dll* and *HexCtrl.lib* use the *DLL Project/DLL Project.vcxproj* **Visual Studio** project.

#### Remarks:
**HexControl**'s *.dll* is built with **MFC Static Linking**. So even if you are to use it in your own **MFC** project, even with different **MFC** version, there should not be any interferences

Building **HexControl** with **MFC Shared DLL** turned out to be a little tricky. Even with the help of `AFX_MANAGE_STATE(AfxGetStaticModuleState())` macro there always were **MFC** debug assertions, which origins quite hard to comprehend.

### [](#)IHexCtrlPtr
`IHexCtrlPtr` is, in fact, a pointer to a `IHexCtrl` pure abstract base class, wrapped either in `std::unique_ptr` or `std::shared_ptr`. You can choose whatever is best for your needs by define or undefine/comment-out the `HEXCTRL_IHEXCTRLPTR_UNIQUEPTR` macro in *HexCtrl.h*.  
By default `HEXCTRL_IHEXCTRLPTR_UNIQUEPTR` is defined, thus `IHexCtrlPtr` is an alias for `std::unique_ptr<IHexCtrl>`.

This wrapper is used mainly for convenience, so you don't have to bother about object lifetime, it will be destroyed automatically.
That's why there is a call to the factory function `CreateHexCtrl()` - to properly initialize a pointer.

If you, for some reason, need a raw interface pointer, you can directly call [`CreateRawHexCtrl`](#createrawhexctrl) function, which returns `IHexCtrl` interface pointer, but in this case you will need to call [`Destroy`](#destroy) method manually afterwards, to destroy `IHexCtrl` object.

### [](#)Namespace
**HexControl** uses its own namespace `HEXCTRL`.  
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
[`Create`](#create) is the first method you call to create **HexControl** instance. It takes [`HEXCREATESTRUCT`](#hexcreatestruct) reference as an argument.

You can choose whether control will behave as a *child* or independent *popup* window, by setting `enCreateMode` member of this struct to [`EHexCreateMode::CREATE_CHILD`](#ehexcreatemode) or [`EHexCreateMode::CREATE_POPUP`](#ehexcreatemode) respectively.
```cpp
HEXCREATESTRUCT hcs;
hcs.enCreateMode = EHexCreateMode::CREATE_POPUP;
hcs.hwndParent = m_hWnd;
m_myHex->Create(hcs);
```
For all available options see [`HEXCREATESTRUCT`](#hexcreatestruct) description.

### [](#)In Dialog
To use **HexControl** within *Dialog* you can, of course, create it with the [Classic Approach](#classic-approach), call [`Create`](#create) method and provide all the necessary information.

But there is another option you can use:
1. Put **Custom Control** from the **Toolbox** in **Visual Studio** dialog designer into your dialog template and make it desirable size.  
![](docs/img/hexctrl_vstoolbox.jpg) ![](docs/img/hexctrl_vscustomctrl.jpg)
2. Go to the **Properties** of that control and in the **Class** field, within the **Misc** section, type: <kbd>HexCtrl</kbd>.  
Give the control appropriate **ID** of your choise (<kbd>IDC_MY_HEX</kbd> in this example).  
Also, here you can set the control's **Dynamic Layout** properties, so that control behaves appropriately when dialog is being resized.  
![](docs/img/hexctrl_vsproperties.jpg)
3. Declare [`IHexCtrlPtr`](#ihexctrlptr) member varable within your dialog class:
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
To set a data to display in the **HexControl** use [`SetData`](#setdata) method.
The code below shows how to construct [`IHexCtrlPtr`](#ihexctrlptr) object and display first `0x1FF` bytes of the current app's memory:
```cpp
IHexCtrlPtr myHex { CreateHexCtrl() };

HEXCREATESTRUCT hcs;
hcs.hwndParent = m_hWnd;
hcs.rect = {0, 0, 600, 400}; //Window rect.

myHex->Create(hcs);

HEXDATASTRUCT hds;
hds.pData = (unsigned char*)GetModuleHandle(0);
hds.ullDataSize = 0x1FF;

myHex->SetData(hds);
```
The next example displays `std::string`'s text as hex:
```cpp
std::string str = "My string";
HEXDATASTRUCT hds;
hds.pData = (unsigned char*)str.data();
hds.ullDataSize = str.size();

myHex->SetData(hds);
```

## [](#)Data Modes
Besides the standard classical mode, when **HexControl** just holds a pointer to some array of bytes in memory, it also has additional advanced modes it can be running in.  
These modes can be quite useful for instance in cases where you need to display a very large amount of data that can't fit in memory all at once.

These modes are ruled over through the [`enDataMode`](#ehexdatamode) member of [`HEXDATASTRUCT`](#hexdatastruct).

### [](#)Memory Data
It's the default data mode the control works in.  
The [`enDataMode`](#ehexdatamode) member of the [`HEXDATASTRUCT`](#hexdatastruct) is set to `DATA_MEMORY`, and `pData` just points to bytes in memory.

### [](#)Message Window
If [`enDataMode`](#ehexdatamode) member of [`HEXDATASTRUCT`](#hexdatastruct) is set to `DATA_MSG`, the control works in, so called, **Message Window mode**.

What it means is that when control is about to display data, it will first ask for this data from the [`HEXDATASTRUCT::hwndMsg`](#hexdatastruct) window, in the form of **[WM_NOTIFY](https://docs.microsoft.com/en-us/windows/win32/controls/wm-notify)** Windows' message. This is pretty much the same as the standard **MFC List Control** works when created with `LVS_OWNERDATA` flag.  
By default the [`HEXDATASTRUCT::hwndMsg`](#hexdatastruct) is equal to the control's parent window.

To properly handle this mode, you must process `WM_NOTIFY` messages in `hwndMsg` window as follows:
```cpp
BOOL CMyWnd::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    PHEXNOTIFYSTRUCT pHexNtfy = (PHEXNOTIFYSTRUCT)lParam;
    if (pHexNtfy->hdr.idFrom == IDC_MY_HEX)
    {
        switch (pHexNtfy->hdr.code)
        {
        case HEXCTRL_MSG_GETDATA:
            /**************************************************************************
            * pHexNtfy->stSpan at this moment shows offset and size of the data 
            * that HexCtrl requests. You have to provide pointer to it.
            ***************************************************************************
            
            pHexNtfy->pData =  /*Set this pointer to an actual data*/;
            break;
        }
   }
}
```
`lParam` will hold a pointer to the [`HEXNOTIFYSTRUCT`](#hexnotifystruct) structure.

The first member of this structure is a standard Windows' **[NMHDR](https://docs.microsoft.com/en-us/windows/win32/api/richedit/ns-richedit-nmhdr)** struct, it will have its `code` member equal to `HEXCTRL_MSG_GETDATA`, indicating that **HexControl**'s byte request has arrived.  
The `ullIndex` member of the structure is an index of the byte to be displayed. And the `pData` is the pointer to an actual byte that you have to set in response.

### [](#)Virtual Handler
If [`enDataMode`](#ehexdatamode) member of [`HEXDATASTRUCT`](#hexdatastruct) is set to `DATA_VIRTUAL` then all the data routine will be done through `HEXDATASTRUCT::pHexVirtual` pointer.

This pointer is of `IHexVirtData` class type, which is a pure abstract base class.
You have to derive your own class from it and implement all its public methods:
```cpp
class IHexVirtData
{
public:
    [[nodiscard]] virtual std::byte* GetData(const HEXSPANSTRUCT&) = 0; //Data index and size to get.
    virtual void SetData(std::byte*, const HEXSPANSTRUCT&) = 0; //Routine to modify data, if HEXDATASTRUCT::fMutable == true.
};
```
Then provide a pointer to created object of this derived class prior to call to [`SetData`](#setdata) method in form of `HEXDATASTRUCT::pHexVirtData = &yourDerivedObject`.

## [](#)Virtual Bookmarks
**HexControl** has innate functional to work with any amount of bookmarked regions. These regions can be assigned with individual background and text color and description.

But if you have some big and complicated data logic and want to handle all these regions yourself, you can do it.  
    virtuvoid SetData(std::byte*, const HEXSPANSTRUCT&) = 0; //Routine to modify data, if HEXDATASTRUCT::fMutable == true.


To enable virtual bookmarks call the [`BkmSetVirtual`](#bkmsetvirtual) method.  

The main method of the `IHexVirtBkm` interface is `HitTest`. It takes byte's offset and returns pointer to [`HEXBKMSTRUCT`](#HEXBKMSTRUCT) if there is a bookmark withing this byte, or `nullptr` otherwise.

```cpp
class IHexVirtBkm
{
public:
    virtual ULONGLONG Add(const HEXBKMSTRUCT& stBookmark) = 0; //Add new bookmark, return new bookmark's ID.
    virtual void ClearAll() = 0; //Clear all bookmarks.
    [[nodiscard]] virtual ULONGLONG GetCount() = 0; //Get total bookmarks count.
    [[nodiscard]] virtual auto GetByID(ULONGLONG ullID)->HEXBKMSTRUCT* = 0; //Bookmark by ID.
    [[nodiscard]] virtual auto GetByIndex(ULONGLONG ullIndex)->HEXBKMSTRUCT* = 0; //Bookmark by index (in inner list).
    [[nodiscard]] virtual auto HitTest(ULONGLONG ullOffset)->HEXBKMSTRUCT* = 0; //Has given offset the bookmark?
    virtual void RemoveByID(ULONGLONG ullID) = 0;   //Remove bookmark by given ID (returned by Add()).
};
```

## [](#)IHexVirtColors
This interface is used to set custom bk/text colors for the given data offset.  
To provide the color a pointer to the valid [`HEXCOLOR`](#hexcolor) structure must be returned from the `IHexVirtColors::GetColor` method. If `nullptr` is returned the default colors will be used.  

In order to use this interface the [`HEXDATASTRUCT::pHexVirtColors`](#hexdatastruct) member must be set to the valid class inherited from this interface, prior to calling [`SetData`](#setdata) method.
```cpp
class IHexVirtColors
{
public:
    [[nodiscard]] virtual PHEXCOLOR GetColor(ULONGLONG ullOffset) = 0;
};
```

## [](#)Methods
The **HexControl** has plenty of methods that you can use to customize its appearance, and to manage its behaviour.

### [](#)BkmAdd
```cpp
ULONGLONG BkmAdd(const HEXBKMSTRUCT& hbs, bool fRedraw = false)
```
Adds new bookmark to the control. Uses [`HEXBKMSTRUCT`](#HEXBKMSTRUCT) as an argument. Returns created bookmark's id.
#### Example
```cpp
HEXBKMSTRUCT hbs;
hbs.vecSpan.emplace_back(HEXSPANSTRUCT { 0x1, 10 });
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
BkmGetByID(ULONGLONG ullID)->HEXBKMSTRUCT*;
```
Get bookmark by ID.

### [](#)BkmGetByIndex
```cpp
auto BkmGetByIndex(ULONGLONG ullIndex)->HEXBKMSTRUCT*;
```
Get bookmark by Index.

### [](#)BkmGetCount
```cpp
ULONGLONG BkmGetCount()const;
```
Get bookmarks' count.

### [](#)BkmHitTest
```cpp
auto BkmHitTest(ULONGLONG ullOffset)->HEXBKMSTRUCT*;
```
Test given offset and retrives pointer to [`HEXBKMSTRUCT`](#HEXBKMSTRUCT) if it contains bookmark.

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
Clears data from the **HexControl** view, not touching data itself.

### [](#)Create
```cpp
bool Create(const HEXCREATESTRUCT& hcs);
```
Main initialization method.  
Takes [`HEXCREATESTRUCT`](#hexcreatestruct) as argument. Returns `true` if created successfully, `false` otherwise.

### [](#)CreateDialogCtrl
```cpp
bool CreateDialogCtrl(UINT uCtrlID, HWND hwndDlg);
```
Creates **HexControl** from **Custom Control** dialog's template. Takes control's **id**, and dialog's window **handle** as arguments. See **[Creating](#in-dialog)** section for more info.

### [](#)Destroy
```cpp
void Destroy();
```
Destroys the control.  
You only invoke this method if you use a raw `IHexCtrl` pointer obtained by the call to `CreateRawHexCtrl` function. Otherwise don't use it.

**Remarks**  
You usually don't need to call this method unless you use **HexControl** through the raw pointer obtained by [`CreateRawHexCtrl`](#createrawhexctrl) factory function.  
If you use **HexControl** in standard way, through the [`IHexCtrlPtr`](#ihexctrlptr) pointer, obtained by `CreateHexCtrl` function, this method will be called automatically.

### [](#)ExecuteCmd
```cpp
void ExecuteCmd(EHexCmd enCmd)const;
```
Executes one of the predefined commands of [`EHexCmd`](#ehexcmd) enum. All these commands are basically replicating control's inner menu.

### [](#)GetCapacity
```cpp
DWORD GetCapacity()const
```
Returns current capacity.

### [](#)GetCaretPos
```cpp
ULONGLONG GetCaretPos()const;
```
Retrieves current caret position offset.

### [](#)GetColors
```cpp
auto GetColors()const->HEXCOLORSSTRUCT
```
Returns current [`HEXCOLORSSTRUCT`](#hexcolorsstruct).

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

### [](#)GetGroupMode
```cpp
auto GetGroupMode()const->EHexGroupMode;
```
Retrieves current data grouping mode.


### [](#)GetFontSize
```cpp
long GetFontSize()const;
```
Returns current font size.

### [](#)GetMenuHandle
```cpp
HMENU GetMenuHandle()const;
```
Retrives the `HMENU` handle of the control's context menu. You can use this handle to customize menu for your needs.

Control's internal menu uses menu `ID`s in range starting from `0x8001`. So if you wish to add your own new menu, assign menu `ID` starting from `0x9000` to not interfere.

When user clicks custom menu, control sends `WM_NOTIFY` message to its parent window with `LPARAM` pointing to [`HEXNOTIFYSTRUCT`](#hexnotifystruct) with its `hdr.code` member set to `HEXCTRL_MSG_MENUCLICK`. `ullData` field of the [`HEXNOTIFYSTRUCT`](#hexnotifystruct) will be holding `ID` of the menu clicked.

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
auto GetSelection()const->std::vector<HEXSPANSTRUCT>&;
```
Returns `std::vector` of offsets and sizes of the current selection.

### [](#)GetWindowHandle
```cpp
HWND GetWindowHandle(EHexWnd enWnd)const
```
Retrieves window handle for one of the **HexControl**'s windows. Takes [`EHexWnd`](#ehexwnd) enum as an argument.

### [](#)GoToOffset
```cpp
void GoToOffset(ULONGLONG ullOffset, int iRelPos = 0)
```
Go to a given offset. The second argument `iRelPos` may take-in three different values:  
* `-1` - offset will appear at the top line.
* &nbsp; `0` - offset will appear at the middle.
* &nbsp; `1` - offset will appear at the bottom line.

### [](#)HitTest
```cpp
auto HitTest(POINT pt, bool fScreen = true)const->std::optional<HEXHITTESTSTRUCT>
```
Hit testing of given point in screen `fScreen = true`, or client `fScreen = false` coordinates. In case of success returns [`HEXHITTESTSTRUCT`](#hexhitteststruct) structure.

### [](#)IsCmdAvail
```cpp
bool IsCmdAvail(EHexCmd enCmd)const;
```
Returns `true` if given command can be executed at the moment, `false` otherwise.

### [](#)IsCreated
```cpp
bool IsCreated()const;
```
Shows whether **HexControl** is created or not yet.

### [](#)IsDataSet
```cpp
bool IsDataSet()const;
```
Shows whether a data was set to **HexControl** or not

### [](#)IsMutable
```cpp
bool IsMutable()const;
```
Shows whether **HexControl** is currently in edit mode or not.

### [](#)IsOffsetAsHex
```cpp
bool IsOffsetAsHex()const;
```
Is "Offset" currently represented (shown) as Hex or as Decimal. It can be changed by double clicking at offset area.

### [](#)IsOffsetVisible
```cpp
HEXVISSTRUCT IsOffsetVisible(ULONGLONG ullOffset)const;
```
Checks for offset visibility and returns [`HEXVISSTRUCT`](#hexvisstruct) as a result.

### [](#)Redraw
```cpp
void Redraw();
```
Redraws the main window.

### [](#)SetCapacity
```cpp
void SetCapacity(DWORD dwCapacity);
```
Sets the **HexControl** current capacity.

### [](#)SetCaretPos
```cpp
void SetCaretPos(ULONGLONG ullOffset, bool fHighLow = true, bool fRedraw = true);
```
Set the caret to the given offset. The `fHighLow` flag shows which part of the hex chunk, low or high, a caret must be set to, it only makes sense in mutable mode.

### [](#)SetColors
```cpp
void SetColors(const HEXCOLORSSTRUCT& clr);
```
Sets all the colors for the control. Takes [`HEXCOLORSSTRUCT`](#hexcolorsstruct) as the argument.

### [](#)SetConfig
```cpp
bool SetConfig(std::wstring_view wstrPath);
```
Set the path to a JSON config file with keybindings to use in **HexControl**, or `L""` for default. This file is using [`EHexCmd`](#ehexcmd) enum values as keys and strings array as values:
```json
{
    "CMD_DLG_SEARCH": [ "ctrl+f", "ctrl+h" ],
    "CMD_SEARCH_NEXT": [ "f3" ],
    "CMD_SEARCH_PREV": [ "shift+f3" ]
}
```
For default values see `HexCtrl/res/keybind.json` file from the project source folder.

### [](#)SetData
```cpp
void SetData(const HEXDATASTRUCT& hds);
```
Main method to set data to display in read-only or edit modes. Takes [`HEXDATASTRUCT`](#hexdatastruct) as an  argument.

### [](#)SetEncoding
```cpp
void SetEncoding(int iCodePage);
```
Sets the code page for the **HexControl**'s text area. Takes [code page identifier](https://docs.microsoft.com/en-us/windows/win32/intl/code-page-identifiers) as an argument, or `-1` for default ASCII-only characters.  

**Note:** Code page identifier must represent [Single-byte Character Set](https://docs.microsoft.com/en-us/windows/win32/intl/single-byte-character-sets). Multi-byte character sets are not currently supported.

### [](#)SetFont
```cpp
void SetFont(const LOGFONTW* pLogFontNew);
```
Sets a new font for the **HexControl**. This font has to be monospaced.

### [](#)SetFontSize
```cpp
void SetFontSize(UINT uiSize);
```
Sets a new font size to the **HexControl**.

### [](#)SetGroupMode
```cpp
void SetGroupMode(EHexGroupMode enGroupMode);
```
Sets current data grouping mode. See [`EHexGroupMode`](#ehexgroupmode) for more info.

### [](#)SetMutable
```cpp
void SetMutable(bool fEnable);
```
Enables or disables mutable mode. In mutable mode all the data can be modified.

### [](#)SetOffsetMode
```cpp
void SetOffsetMode(bool fHex);
```
Set offset area being shown as Hex (`fHex=true`) or as Decimal (`fHex=false`).

### [](#)SetPageSize
```cpp
void SetPageSize(DWORD dwSize, const wchar_t* wstrName = L"Page");
```
Sets the size of the page to draw the divider line between. This size should be multiple to the current [capacity](#setcapacity) size to take effect. The second argument sets the name to be displayed in the bottom info area of the **HexControl** ("Page", "Sector", etc...).  
To remove the divider just set `dwSize` to 0.

### [](#)SetSelection
```cpp
void SetSelection(const std::vector<HEXSPANSTRUCT>& vecSel, bool fRedraw = true);
```
Sets current selection.

### [](#)SetWheelRatio
```cpp
void SetWheelRatio(double dbRatio)
```
Sets the ratio for how much to scroll with mouse-wheel.

## [](#)Structures
Below are listed all **HexControl**'s structures.

### [](#)HEXBKMSTRUCT
Structure for bookmarks, used in [`BkmAdd`](#BkmAdd) method.  
```cpp
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
```
The member `vecSpan` is of a `std::vector<HEXSPANSTRUCT>` type because a bookmark may have few non adjacent areas. For instance, when selection is made as a block, with <kbd>Alt</kbd> pressed.

### [](#)HEXCOLOR
**HexControl** custom colors.
```cpp
struct HEXCOLOR
{
    COLORREF clrBk { };   //Bk color.
    COLORREF clrText { }; //Text color.
};
using PHEXCOLOR = HEXCOLOR*;
```

### [](#)HEXCOLORSSTRUCT
This structure describes all control's colors. All these colors have their default values.
```cpp
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
```

### [](#)HEXCREATESTRUCT
The main initialization struct used for control creation.
```cpp
struct HEXCREATESTRUCT
{
    EHexCreateMode  enCreateMode { EHexCreateMode::CREATE_CHILD }; //Creation mode of the HexCtrl window.
    HEXCOLORSSTRUCT stColor { };          //All the control's colors.
    HWND            hwndParent { };       //Parent window pointer.
    const LOGFONTW* pLogFont { };         //Font to be used, nullptr for default. This font has to be monospaced.
    RECT            rect { };             //Initial rect. If null, the window is screen centered.
    UINT            uID { };              //Control ID.
    DWORD           dwStyle { };          //Window styles, 0 for default.
    DWORD           dwExStyle { };        //Extended window styles, 0 for default.
    double          dbWheelRatio { 1.0 }; //Ratio for how much to scroll with mouse-wheel.
};
```

### [](#)HEXDATASTRUCT
Main struct to set a data to display in the control.
```cpp
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
```

### [](#)HEXHITTESTSTRUCT
Structure is used in [`HitTest`](#hittest) method.
```cpp
struct HEXHITTESTSTRUCT
{
    ULONGLONG ullOffset { };      //Offset.
    bool      fIsAscii { false }; //Is cursor at ASCII part or at Hex.
};
```

### [](#)HEXNOTIFYSTRUCT
This struct is used in notification purposes, to notify parent window about **HexControl**'s states.
```cpp
struct HEXNOTIFYSTRUCT
{
    NMHDR         hdr { };     //Standard Windows header. For hdr.code values see HEXCTRL_MSG_* messages.
    HEXSPANSTRUCT stSpan { };  //Offset and size of the bytes. 
    ULONGLONG     ullData { }; //Data depending on message (e.g. user defined custom menu id/cursor pos).
    std::byte*    pData { };   //Pointer to a data to get/send.
    POINT         point { };   //Mouse position for menu notifications.
};
using PHEXNOTIFYSTRUCT = HEXNOTIFYSTRUCT*;
```

### [](#)HEXSPANSTRUCT
This struct is used mostly in selection and bookmarking routines. It holds offset and size of the data region.
```cpp
struct HEXSPANSTRUCT
{
    ULONGLONG ullOffset { };
    ULONGLONG ullSize { };
};
```

### [](#)HEXVISSTRUCT
This struct is returned from [`IsOffsetVisible`](#isoffsetvisible) method. Two members `i8Vert` and `i8Horz` represent vertical and horizontal visibility respectively. These members can be in three different states:
* `-1` — offset is higher, or at the left, of the visible area.
* &nbsp; `1` — offset is lower, or at the right.
* &nbsp; `0` — offset is visible.

```cpp
struct HEXVISSTRUCT
{
    std::int8_t i8Vert { }; //Vertical offset.
    std::int8_t i8Horz { }; //Horizontal offset.
    operator bool() { return i8Vert == 0 && i8Horz == 0; }; //For test simplicity: if(IsOffsetVisible()).
};
```

### [](#)EHexCmd
Enum of commands that can be executed within **HexControl**.
```cpp
enum class EHexCmd : WORD
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
Enum that represents mode the **HexControl**'s window will be created in.
```cpp
enum class EHexCreateMode : WORD
{
    CREATE_CHILD, CREATE_POPUP, CREATE_CUSTOMCTRL
};
```

### [](#)EHexDataMode
`Enum` that represents current data [mode](#data-modes) **HexControl** works in. It's used as [`HEXDATASTRUCT`](#hexdatastruct) member in [`SetData`](#setdata) method.
```cpp
enum class EHexDataMode : WORD
{
    DATA_MEMORY, DATA_MSG, DATA_VIRTUAL
};
```

### [](#)EHexGroupMode
Enum that represents available data grouping modes.
```cpp
enum class EHexGroupMode : WORD
{
    ASBYTE = 1, ASWORD = 2, ASDWORD = 4, ASQWORD = 8
};
```

### [](#)EHexWnd
Enum of all **HexControl**'s internal windows. This enum is used as an arg in [`GetWindowHandle`](#getwindowhandle) method to retrieve window's handle. 
```cpp
enum class EHexWnd : WORD
{
	WND_MAIN, DLG_BKMMANAGER, DLG_DATAINTERP, DLG_FILLDATA,
	DLG_OPERS, DLG_SEARCH, DLG_ENCODING, DLG_GOTO
};
```

## [](#)Notification Messages
During its work **HexControl** sends notification messages through **[WM_NOTIFY](https://docs.microsoft.com/en-us/windows/win32/controls/wm-notify)** mechanism to indicate its states. These messages are sent either to [`HEXCREATESTRUCT::hwndParent`](#hexcreatestruct) or to [`HEXDATASTRUCT::hwndMsg`](#hexdatastruct) window, depending on whether the latter is set.  
The `LPARAM` of the `WM_NOTIFY` message will hold pointer to the [`HEXNOTIFYSTRUCT`](#hexnotifystruct).

### [](#)HEXCTRL_MSG_BKMCLICK
Sent if bookmark is clicked. [`HEXNOTIFYSTRUCT::pData`](#hexnotifystruct) will contain [`HEXBKMSTRUCT`](#HEXBKMSTRUCT) pointer.

### [](#)HEXCTRL_MSG_CARETCHANGE
Sent when caret position has changed. [`HEXNOTIFYSTRUCT::ullData`](#hexnotifystruct) will contain current caret position.

### [](#)HEXCTRL_MSG_CONTEXTMENU
Sent when context menu is about to be displayed.

### [](#)HEXCTRL_MSG_DESTROY
Sent to indicate that **HexControl** window is about to be destroyed.

### [](#)HEXCTRL_MSG_GETDATA
Used in [`DATA_MSG`](#ehexdatamode) mode to acquire the data range to display.

### [](#)HEXCTRL_MSG_MENUCLICK
Sent when user defined custom menu has been clicked.  
[`HEXNOTIFYSTRUCT`](#hexnotifystruct) `ullData` member contains menu ID, while `point` member contains position of the cursor, in screen coordinates, at the time of the mouse click.

### [](#)HEXCTRL_MSG_SELECTION
Sent when selection has been made.

### [](#)HEXCTRL_MSG_SETDATA
Sent to indicate that the data has changed.

### [](#)HEXCTRL_MSG_VIEWCHANGE
Sent when **HexControl**'s view has changed, whether on resizing or scrolling. [`HEXNOTIFYSTRUCT::stSpan`](#hexnotifystruct) will contain starting offset and size of the visible data.

## [](#)Exported Functions
**HexControl** has few `"C"` interface functions which it exports when built as *.dll*.

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
Returns pointer to [`HEXCTRLINFO`](#hexctrlinfo), which is the **HexControl**'s service information structure.

### [](#)HEXCTRLINFO
Service structure, keeps **HexControl**'s version information.
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
To properly resize and position your **HexControl**'s window you may have to handle `WM_SIZE` message in its parent window, in something like this way:
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
This software is available under the **"MIT License modified with The Commons Clause".**  
Briefly: It is free for any **non commercial** use.  
[https://github.com/jovibor/HexCtrl/blob/master/LICENSE](https://github.com/jovibor/HexCtrl/blob/master/LICENSE)