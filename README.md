## **Hex Control, C++/MFC**
![](docs/img/hexctrl_mainwnd.jpg)
## Table of Contents
* [Introduction](#introduction)
* [Installation](#installation)
  * [Building From The Sources](#building-from-the-sources)
  * [Dynamic Link Library](#dynamic-link-library)
  * [Namespace](#namespace)
* [Creating](#creating)
  * [Classic Approach](#classic-approach)
  * [In Dialog](#in-dialog)
* [Set The Data](#set-the-data)
* [Virtual Data Mode](#virtual-data-mode)
* [Virtual Bookmarks](#virtual-bookmarks)
* [Custom Colors](#custom-colors)
* [Templates](#templates)
* [Methods](#methods) <details><summary>_Expand_</summary>
  * [ClearData](#cleardata)
  * [Create](#create)
  * [CreateDialogCtrl](#createdialogctrl)
  * [Destroy](#destroy)
  * [ExecuteCmd](#executecmd)
  * [GetActualWidth](#getactualwidth)
  * [GetBookmarks](#getbookmarks)
  * [GetCacheSize](#getcachesize)
  * [GetCapacity](#getcapacity)
  * [GetCaretPos](#getcaretpos)
  * [GetColors](#getcolors)
  * [GetData](#getdata)
  * [GetDataSize](#getdatasize)
  * [GetDateInfo](#getdateinfo)
  * [GetEncoding](#getencoding)
  * [GetFont](#getfont)
  * [GetGroupMode](#getgroupmode)
  * [GetMenuHandle](#getmenuhandle)
  * [GetPagesCount](#getpagescount)
  * [GetPagePos](#getpagepos)
  * [GetPageSize](#getpagesize)
  * [GetSelection](#getselection)
  * [GetTemplates](#gettemplates)
  * [GetUnprintableChar](#getunprintablechar)
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
  * [SetDateInfo](#setdateinfo)
  * [SetEncoding](#setencoding)
  * [SetFont](#setfont)
  * [SetGroupMode](#setgroupmode)
  * [SetMutable](#setmutable)
  * [SetOffsetMode](#setoffsetmode)
  * [SetPageSize](#setpagesize)
  * [SetVirtualBkm](#setvirtualbkm)
  * [SetRedraw](#setredraw)
  * [SetSelection](#setselection)
  * [SetUnprintableChar](#setunprintablechar)
  * [SetWheelRatio](#setwheelratio)
  * [ShowInfoBar](#showinfobar)
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
* [Interfaces](#interfaces) <details><summary>_Expand_</summary>
  * [IHexBookmarks](#ihexbookmarks)
  * [IHexTemplates](#ihextemplates)
  * [IHexVirtColors](#ihexvirtcolors)
  * [IHexVirtData](#ihexvirtdata)
  </details>
* [Enums](#enums) <details><summary>_Expand_</summary>
  * [EHexCmd](#ehexcmd)
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
   </details>
* [Exported Functions](#exported-functions) <details><summary>_Expand_</summary>
  * [CreateRawHexCtrl](#createrawhexctrl)
  * [GetHexCtrlInfo](#gethexctrlinfo)
  * [HEXCTRLINFO](#hexctrlinfo)
   </details>
* [Appearance](#appearance)
* [Licensing](#licensing)

## [](#)Introduction
**HexCtrl** is a very featured hex editor control, written in **C++/MFC**.

It's implemented as a pure abstract interface and therefore can be used in your app even if you don't use **MFC** directly. It's written with **/std:c++20** standard in **Visual Studio 2022**.

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
* Powerful system of customizable [Templates](#templates)
* [Assignable keyboard shortcuts](#setconfig) via external config file
* Customizable look and appearance
* Written with **/std:c++20** standard conformance

![](docs/img/hexctrl_operationswnd.jpg)

## [](#)Installation
The **HexCtrl** can be used in two different ways:  
* Building from the sources as a part of your project 
* Using as a Dynamic Link Library *.dll*.

### [](#)Building From The Sources
The building process is quite simple:
1. Copy *HexCtrl* folder into your project's directory.
1. Add all files from the *HexCtrl* folder into your project (you can skip  
*HexCtrl/dep/rapidjson/rapidjson-amalgam.h* header-only lib).
1. Add `#include "HexCtrl/HexCtrl.h"` where you suppose to use the **HexCtrl**.
1. Declare `IHexCtrlPtr` member variable: `IHexCtrlPtr myHex { HEXCTRL::CreateHexCtrl() };`
1. [Create](#creating) control instance.

If you want to build **HexCtrl** from the sources in non **MFC** app you will have to:
1. Add support for **Use MFC in a Shared DLL** in your project settings.
1. Uncomment the line `//#define HEXCTRL_MANUAL_MFC_INIT` in `HexCtrl.h` header file.

### [](#)Dynamic Link Library
To use **HexCtrl** as the *.dll* do the following:
1. Copy *HexCtrl.h* and *HexCtrlDefs.h* files into your project's folder.
1. Copy *HexCtrl.lib* file into your project's folder, so that linker can see it.
1. Put *HexCtrl.dll* file next to your *.exe* file.
1. Add the following line where you suppose to use the control:
```cpp
#define HEXCTRL_SHARED_DLL //You can alternatively uncomment this line in HexCtrl.h.
#include "HexCtrl.h"` 
```
1. Declare `IHexCtrlPtr` member variable: `IHexCtrlPtr myHex { HEXCTRL::CreateHexCtrl() };`
1. [Create](#creating) control instance.

To build *HexCtrl.dll* and *HexCtrl.lib* use the *DLL Project/DLL Project.vcxproj* **Visual Studio** project file.

#### Remarks:
**HexCtrl**'s *.dll* is built with **MFC Static Linking**. So even if you are to use it in your own **MFC** project, even with different **MFC** version, there should not be any interferences

Building **HexCtrl** with **MFC Shared DLL** turned out to be a little tricky. Even with the help of `AFX_MANAGE_STATE(AfxGetStaticModuleState())` macro there always were **MFC** debug assertions, which origins quite hard to comprehend.

### [](#)Namespace
**HexCtrl** uses its own namespace `HEXCTRL`. So it's up to you, whether to use namespace prefix before declarations:
```cpp
HEXCTRL::
```
or to define namespace globally, in the source file's beginning:
```cpp
using namespace HEXCTRL;
```

## [](#)Creating

### [](#)Classic Approach
The [`Create`](#create) method is the first method you call to create **HexCtrl** instance. It takes [`HEXCREATE`](#hexcreate) struct as an argument which provides all necessary information for the control.  
The `HEXCREATE::dwStyle` and `HEXCREATE::dwExStyle` are [Window](https://docs.microsoft.com/en-us/windows/win32/winmsg/window-styles) and [Extended Window](https://docs.microsoft.com/en-us/windows/win32/winmsg/extended-window-styles) styles respectively, they will be passed to the **HexCtrl**'s window creating function, set these styles according to your needs.  
For all available creation options see the [`HEXCREATE`](#hexcreate) struct description.

### [](#)In Dialog
To use **HexCtrl** within *Dialog* you can, of course, create it with the [Classic Approach](#classic-approach), call [`Create`](#create) method and provide all the necessary information.

But there is another option you can use:
1. Put **Custom Control** from the **Toolbox** in **Visual Studio** dialog designer into your dialog template and make it desirable size.  
![](docs/img/hexctrl_vstoolbox.jpg) ![](docs/img/hexctrl_vscustomctrl.jpg)
2. Go to the **Properties** of that control and in the **Class** field, within the **Misc** section, type: <kbd>HexCtrl</kbd>.  
Give the control appropriate **ID** of your choice (<kbd>IDC_MY_HEX</kbd> in this example).  
Also, here you can set the control's **Dynamic Layout** properties, so that control behaves appropriately when dialog is being resized.  
![](docs/img/hexctrl_vsproperties.jpg)
3. Declare `IHexCtrlPtr` member variable within your dialog class:
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
The code below shows how to construct `HexCtrl` object and display first `0x1FF` bytes of the current app's memory:
```cpp
IHexCtrlPtr myHex { CreateHexCtrl() };

HEXCREATE hcs;
hcs.hWndParent = m_hWnd;
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

If `HEXDATA::pHexVirtData` pointer is set then all the data routine will be done through it. This pointer is of [`IHexVirtData`](#ihexvirtdata) class type, which is a pure abstract base class.
You have to derive your own class from it and implement all its public methods.
Then provide a pointer to the created object of this derived class prior to call to [`SetData`](#setdata) method, through the `HEXDATA::pHexVirtData`.

## [](#)Virtual Bookmarks
**HexCtrl** has innate functional to work with any amount of bookmarked regions. These regions can be assigned with individual background and text colors and description.

But if you have big and complicated data logic and want to handle all these bookmarks yourself, you can do it with the help of the virtual bookmarks mode. In this mode all bookmark's burden is handled by yourself, by implementing the [`IHexBookmarks`](#ihexbookmarks) interface and providing pointer to this implementation to the **HexCtrl** by calling [`SetVirtualBkm`](#setvirtualbkm) method.

## [](#)Custom Colors
If you'd like to colorize your data with custom bk/text colors, you have to resort to the [`IHexVirtColors`](#ihexvirtcolors) interface help. Please see the interface description, it's very simple to use.

## [](#)Templates
**HexCtrl**'s templates is a powerful system of a data structures' description with a simple `.json` file. These files can be loaded through the **HexControl**'s internal template manager, or through the [API](#gettemplates).  
```json
{
    "$schema": "https://raw.githubusercontent.com/jovibor/HexCtrl/master/docs/HexCtrl.Templates.Schema.json",
    "TemplateName": "SampleTemplate",
    "Data": {
        "endianness": "little",
        "clrBk": "#999999",
        "clrText": "#FFFFFF",
        "Fields": [
            {
                "name": "MyCustomDataSingle",
                "type": "MyCustomType"
            },
            {
                "name": "CustomComplexData",
                "type": "MyCustomComplexType"
            },
            {
                "name": "ArrayOfDWORDs",
                "type": "DWORD",
                "array": 10
            },
            {
                "name": "MyCustomDataArray",
                "type": "MyCustomType",
                "array": 4
            }
        ]
    },
    "CustomTypes": [
        {
            "TypeName": "MyCustomType",
            "Fields": [
                {
                    "name": "myCustomTypeField1",
                    "type": "DWORD"
                },
                {
                    "name": "myCustomTypeField2",
                    "type": "DWORD"
                }
            ]
        },
        {
            "TypeName": "MyCustomComplexType",
            "Fields": [
                {
                    "name": "MyCustomTypeData1",
                    "type": "MyCustomType"
                },
                {
                    "name": "MyCustomTypeData2",
                    "type": "MyCustomType"
                }
            ]
        }
    ]
}
```
Every such file contains the following properties:  
- **TemplateName** [mandatory, string] - the name of the template
- **Data** [mandatory, object] - main object that contains all template field's information
- **CustomTypes** [optional, array] - array of a user defined types, that can be then referenced in the **type** property of the **Fields**

**CustomTypes** objects are the same as **Fields** objects, the only difference is the **TypeName** property which **Fields** objects don't have.  

Every **Data** or **CustomType** object contains **Fields** property with actual struct members.  

**Fields** [array] - is an array of objects where every object represents a struct data member. Any such object can have its own **Fields** sub-objects, which will represent nested structs.  
The **Fields**'s properties include:
- **name** - [mandatory, string] - name of the field
- **description** - [optional, string] - field description
- **type** - [optional, string] - field type, such as:  
`bool`, `char`, `unsigned char`, `byte`, `short`, `unsigned short`, `WORD`, `long`, `unsigned long`, `int`, `unsigned int`, `DWORD`, `long long`, `unsigned long long`, `QWORD`, `float`, `double`, `time32_t`, `time64_t`, `FILETIME`, `SYSTEMTIME`, `GUID`, or any custom type defined in the **CustomTypes** section
- **size** - [optional, int] - size of the field in bytes, if the **type** field is not provided
- **array** - [optional, int] - size of the array, if the given field is an array of fields
- **endianness** - [optional, string] - field endianness, "little" or "big". By default all fields are little-endian.
- **clrBk** - [optional, string] - field background color
- **clrText** - [optional, string] - field text color

The **endianness**, **clrBk** and **clrText** properties that locate at the same level as the **Fields** property, would mean the default properties for all the **Fields** objects of that level and below the line, unless they explicitly redefined in the field itself.  

For the available templates check the `Templates` directory.

## [](#)Methods
The **HexCtrl** has plenty of methods that you can use to customize its appearance, and to manage its behavior.

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
You usually don't need to call this method unless you use **HexCtrl** through the raw pointer obtained by [`CreateRawHexCtrl`](#createrawhexctrl) factory function. If you use **HexCtrl** in the standard way, through the `IHexCtrlPtr` pointer obtained by `CreateHexCtrl` function, this method will be called automatically.

### [](#)ExecuteCmd
```cpp
void ExecuteCmd(EHexCmd enCmd)const;
```
Executes one of the predefined commands of [`EHexCmd`](#ehexcmd) enum. All these commands are basically replicating control's inner menu.

### [](#)GetActualWidth
```cpp
int GetActualWidth()const;
```
Returns the width of the **HexCtrl** bounding rectangle, i.e. the width of the drawn working area.

### [](#)GetBookmarks
```cpp
auto GetBookmarks()const->IHexBookmarks*;
```
Returns pointer to the [`IHexBookmarks`](#ihexbookmarks) interface, which responds for the bookmarks machinery.

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

### [](#)GetDateInfo
```cpp
GetDateInfo()const->std::tuple<DWORD, wchar_t>;
```
Returns [date format-ordering specifier](https://docs.microsoft.com/en-us/windows/win32/intl/locale-idate), and date separator.

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

### [](#)GetTemplates
```cpp
auto GetTemplates()const->IHexTemplates*;
```
Returns pointer to the internal [`IHexTemplates`](#ihextemplates) interface that is responsible for templates machinery.

### [](#)GetUnprintableChar
```cpp
wchar_t GetUnprintableChar()const;
```
Returns replacement char for unprintable characters.

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
For default values see the [`IDR_HEXCTRL_JSON_KEYBIND.json`](https://github.com/jovibor/HexCtrl/blob/master/HexCtrl/res/IDR_HEXCTRL_JSON_KEYBIND.json) file from the project sources.

### [](#)SetData
```cpp
void SetData(const HEXDATA& hds);
```
Main method to set a data to display in read-only or mutable modes. Takes [`HEXDATA`](#hexdata) as an  argument.

## [](#)SetDateInfo
```cpp
void SetDateInfo(DWORD dwFormat, wchar_t wchSepar);
```
Sets [date format-ordering specifier](https://docs.microsoft.com/en-us/windows/win32/intl/locale-idate), along with date separator.

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

### [](#)SetVirtualBkm
```cpp
void SetVirtualBkm(IHexBookmarks* pVirtBkm);
```
Sets a pointer for the [Virtual Bookmarks](#virtual-bookmarks) mode, or disables this mode if `nullptr` is set.

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

### [](#)SetUnprintableChar
```cpp
void SetUnprintableChar(wchar_t wch);
```
Sets replacement char for unprintable characters.

### [](#)SetWheelRatio
```cpp
void SetWheelRatio(double dbRatio, bool fLines);
```
Sets the scroll amount for one scroll-page.  
Page is one mouse-wheel tick or a Page-down key.   When `fLines` is `true` the `dbRatio` is the amount of text lines to scroll. When it's `false` `dbRatio` shows a ratio of visible screen height to scroll.

### [](#)ShowInfoBar
```cpp
void ShowInfoBar(bool fShow);
```
Show/hide bottom Info bar.

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
This structure describes all **HexCtrl** colors. All these colors have their default values.
```cpp
struct HEXCOLORS
{
    COLORREF clrFontHex { GetSysColor(COLOR_WINDOWTEXT) };      //Hex-chunks font color.
    COLORREF clrFontText { GetSysColor(COLOR_WINDOWTEXT) };     //Text font color.
    COLORREF clrFontSel { GetSysColor(COLOR_HIGHLIGHTTEXT) };   //Selected hex/text font color.
    COLORREF clrFontDataInterp { RGB(250, 250, 250) };          //Data Interpreter text/hex font color.
    COLORREF clrFontCaption { RGB(0, 0, 180) };                 //Caption font color
    COLORREF clrFontInfoName { GetSysColor(COLOR_WINDOWTEXT) }; //Font color of the Info bar parameters.
    COLORREF clrFontInfoData { RGB(0, 0, 180) };                //Font color of the Info bar data.
    COLORREF clrFontCaret { RGB(255, 255, 255) };               //Caret font color.
    COLORREF clrBk { GetSysColor(COLOR_WINDOW) };               //Background color.
    COLORREF clrBkSel { GetSysColor(COLOR_HIGHLIGHT) };         //Background color of the selected Hex/ASCII.
    COLORREF clrBkDataInterp { RGB(147, 58, 22) };              //Data Interpreter Bk color.
    COLORREF clrBkInfoBar { GetSysColor(COLOR_BTNFACE) };       //Background color of the bottom Info bar.
    COLORREF clrBkCaret { RGB(0, 0, 255) };                     //Caret background color.
    COLORREF clrBkCaretSel { RGB(0, 0, 200) };                  //Caret background color in selection.
};
```

### [](#)HEXCREATE
The main initialization struct used for **HexCtrl** creation.
```cpp
struct HEXCREATE
{
    HEXCOLORS stColor { };          //All HexCtrl colors.
    HWND      hWndParent { };       //Parent window handle.
    PLOGFONTW pLogFont { };         //Monospaced font to be used, or nullptr for default.
    RECT      rect { };             //Initial window rect.
    UINT      uID { };              //Control ID if it's a child window.
    DWORD     dwStyle { };          //Window styles.
    DWORD     dwExStyle { };        //Extended window styles.
    double    dbWheelRatio { 1.0 }; //Either screen-ratio or lines amount to scroll with Page-scroll.
    bool      fPageLines { false }; //Treat dbWheelRatio as a screen-ratio (false) or as amount of lines.
    bool      fInfoBar { true };    //Show bottom Info bar or not.
    bool      fCustom { false };    //If it's a custom control in a dialog.
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
    ULONGLONG ullOffset { };     //Offset.
    bool      fIsText { false }; //Is cursor at Text or Hex area.
    bool      fIsHigh { false }; //Is it High or Low part of the byte.
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
    EHexDataSize         enDataSize { };       //Operation data size.
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

## [](#)Interfaces

### [](#)IHexBookmarks
The `IHexBookmarks` interface responds for the **HexCtrl**'s bookmarks machinery. To obtain pointer to this interface use the [`GetBookmarks`](#getbookmarks) method.
```cpp
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
```

#### [](#)IHexBookmarks::AddBkm
```cpp
ULONGLONG AddBkm(const HEXBKM& hbs, bool fRedraw = false)
```
Adds new bookmark to the control, returns created bookmark's ID.

**Example:**
```cpp
HEXBKM hbs;
hbs.vecSpan.emplace_back(0x1, 10);
hbs.clrBk = RGB(0, 255, 0);
hbs.clrText = RGB(255, 255, 255);
hbs.wstrDesc = L"My bookmark, with green bk and white text.";
myHex->GetBookmarks()->Add(hbs);
```

#### [](#)IHexBookmarks::ClearAll
```cpp
void ClearAll();
```
Clear all bookmarks.

#### [](#)IHexBookmarks::GetByID
```cpp
GetByID(ULONGLONG ullID)->HEXBKM*;
```
Get bookmark by ID.

#### [](#)IHexBookmarks::GetByIndex
```cpp
auto GetByIndex(ULONGLONG ullIndex)->HEXBKM*;
```
Get bookmark by index.

#### [](#)IHexBookmarks::GetCount
```cpp
ULONGLONG GetCount();
```
Get bookmarks' count.

#### [](#)IHexBookmarks::HitTest
```cpp
auto HitTest(ULONGLONG ullOffset)->HEXBKM*;
```
Test given offset and retrieves a pointer to [`HEXBKM`](#hexbkm) if offset contains a bookmark.

#### [](#)IHexBookmarks::RemoveByID
```cpp
void RemoveByID(ULONGLONG ullID);
```
Removes bookmark with the given ID.

### [](#)IHexTemplates
```cpp
class IHexTemplates
{
public:
    virtual int ApplyTemplate(ULONGLONG ullOffset, int iTemplateID) = 0; //Apply template to a given offset.
    virtual void DisapplyByID(int iAppliedID) = 0;
    virtual void DisapplyByOffset(ULONGLONG ullOffset) = 0;
    virtual int LoadTemplate(const wchar_t* pFilePath) = 0; //Returns loaded template ID on success, zero otherwise.
    virtual void ShowTooltips(bool fShow) = 0;
    virtual void UnloadTemplate(int iTemplateID) = 0;       //Unload/remove loaded template from memory.
};
```

### [](#)IHexVirtColors
```cpp
class IHexVirtColors
{
public:
    void OnHexGetColor(HEXCOLORINFO&) = 0;
};
```
This interface is used to set custom bk/text colors for the data regions.  
To use this feature set the [`HEXDATA::pHexVirtColors`](#hexdata) member to a valid instance of your class implementing this interface, prior to calling the [`SetData`](#setdata) method.

The `OnHexGetColor` method of this interface takes [`HEXCOLORINFO`](#hexcolorinfo) struct as an argument. The `pClr` member of this struct must point to a valid [`HEXCOLOR`](#hexcolor) struct after method completes, or `nullptr` for default colors.

### [](#)IHexVirtData
```cpp
class IHexVirtData
{
public:
    virtual void OnHexGetData(HEXDATAINFO&) = 0;       //Data to get.
    virtual void OnHexSetData(const HEXDATAINFO&) = 0; //Data to set, if mutable.
};
```

## [](#)Enums

### [](#)EHexCmd
Enum of commands that can be executed within **HexCtrl**.
```cpp
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
    CMD_SCROLL_PAGEUP, CMD_SCROLL_PAGEDOWN,
    CMD_TEMPL_APPLYCURR, CMD_TEMPL_DISAPPLY, CMD_TEMPL_DISAPPALL, CMD_DLG_TEMPLMGR
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
    OPER_ROTR, OPER_SWAP, OPER_ADD, OPER_SUB, OPER_MUL, OPER_DIV, OPER_CEIL, OPER_FLOOR
};
```

### [](#)EHexWnd
Enum of all **HexCtrl**'s internal windows. This enum is used as an arg in [`GetWindowHandle`](#getwindowhandle) method to retrieve window's handle. 
```cpp
enum class EHexWnd : std::uint8_t
{
    WND_MAIN, DLG_BKMMANAGER, DLG_DATAINTERP, DLG_FILLDATA,
    DLG_OPERS, DLG_SEARCH, DLG_ENCODING, DLG_GOTO, DLG_TEMPLMGR
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

## [](#)Exported Functions
**HexCtrl** has few `"C"` interface functions which it exports when built as *.dll*.

### [](#)CreateRawHexCtrl
```cpp
extern "C" HEXCTRLAPI IHexCtrl* __cdecl CreateRawHexCtrl();
```
Main function that creates raw `IHexCtrl` interface pointer. You barely need to use this function in your code. Although if you do, you need to manually call the [`Destroy`](#destroy) method afterwards.

### [](#)GetHexCtrlInfo
```cpp
extern "C" HEXCTRLAPI HEXCTRLINFO __cdecl GetHexCtrlInfo();
```
Returns [`HEXCTRLINFO`](#hexctrlinfo) struct, which contains the **HexCtrl**'s version information.

### [](#)HEXCTRLINFO
Service structure with the **HexCtrl**'s version information.
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
        }stVersion;
    };
};
```

## [](#)Appearance
To change control's font size — <kbd>Ctrl+MouseWheel</kbd>  
To change control's capacity — <kbd>Ctrl+Shift+MouseWheel</kbd>

## [](#)Licensing
This software is available under **"The HexCtrl License"**, it is free for any **NON-COMMERCIAL** use.  
See the [LICENSE](https://github.com/jovibor/HexCtrl/blob/master/LICENSE) file.