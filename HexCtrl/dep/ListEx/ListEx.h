/*************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                          *
* This is very extended and featured version of CMFCListCtrl class.                  *
* Official git repository: https://github.com/jovibor/ListEx/                        *
* This code is available under the "MIT License".                                    *
*************************************************************************************/
#pragma once
#include <afxcontrolbars.h>
#include <memory>
#include <string>

namespace HEXCTRL::LISTEX
{
	/*********************************************************************************
	* EListExSortMode - Sorting mode.                                                *
	*********************************************************************************/
	enum class EListExSortMode : std::uint8_t {
		SORT_LEX, SORT_NUMERIC
	};

	/********************************************
	* LISTEXCOLOR - colors for the cell.        *
	********************************************/
	struct LISTEXCOLOR {
		COLORREF clrBk { };
		COLORREF clrText { };
	};
	using PLISTEXCOLOR = const LISTEXCOLOR*;

	/****************************************************************
	* LISTEXCOLORINFO - struct for the LISTEX_MSG_GETCOLOR message. *
	****************************************************************/
	struct LISTEXCOLORINFO {
		NMHDR       hdr { };
		int         iItem { };
		int         iSubItem { };
		LISTEXCOLOR stClr { };
	};
	using PLISTEXCOLORINFO = LISTEXCOLORINFO*;

	/****************************************************************
	* LISTEXDATAINFO - struct for the LISTEX_MSG_SETDATA message.   *
	****************************************************************/
	struct LISTEXDATAINFO {
		NMHDR   hdr { };
		int     iItem { };
		int     iSubItem { };
		LPCWSTR pwszData { };        //Text that has been set for a cell.
		bool    fAllowEdit { true }; //Allow cell editing or not, in case of LISTEX_MSG_EDITBEGIN.
	};
	using PLISTEXDATAINFO = LISTEXDATAINFO*;

	/****************************************************************
	* LISTEXICONINFO - struct for the LISTEX_MSG_GETICON message.   *
	****************************************************************/
	struct LISTEXICONINFO {
		NMHDR hdr { };
		int   iItem { };
		int   iSubItem { };
		int   iIconIndex { -1 }; //Icon index in the list internal image-list.
	};
	using PLISTEXICONINFO = LISTEXICONINFO*;

	/****************************************************************
	* LISTEXTTINFO - struct for the LISTEX_MSG_GETTOOLTIP message.  *
	****************************************************************/
	//Tooltips data.
	struct LISTEXTTDATA {
		LPCWSTR pwszText { };    //Tooltip text.
		LPCWSTR pwszCaption { }; //Tooltip caption.
	};
	struct LISTEXTTINFO {
		NMHDR        hdr { };
		int          iItem { };
		int          iSubItem { };
		LISTEXTTDATA stData { };
	};
	using PLISTEXTTINFO = LISTEXTTINFO*;

	/****************************************************************
	* LISTEXLINKINFO - struct for the LISTEX_MSG_LINKCLICK message. *
	****************************************************************/
	struct LISTEXLINKINFO {
		NMHDR   hdr { };
		int     iItem { };
		int     iSubItem { };
		POINT   ptClick { };  //A point where the link was clicked.
		LPCWSTR pwszText { }; //Link text.
	};
	using PLISTEXLINKINFO = LISTEXLINKINFO*;

	/**********************************************************************************
	* LISTEXCOLORS - All ListEx colors.                                               *
	**********************************************************************************/
	struct LISTEXCOLORS {
		COLORREF clrListText { GetSysColor(COLOR_WINDOWTEXT) };       //List text color.
		COLORREF clrListTextLink { RGB(0, 0, 200) };                  //List hyperlink text color.
		COLORREF clrListTextSel { GetSysColor(COLOR_HIGHLIGHTTEXT) }; //Selected item text color.
		COLORREF clrListTextLinkSel { RGB(250, 250, 250) };           //List hyperlink text color in selected cell.
		COLORREF clrListTextCellTt { GetSysColor(COLOR_WINDOWTEXT) }; //Text color of a cell that has tooltip.
		COLORREF clrListBkOdd { GetSysColor(COLOR_WINDOW) };          //List Bk color of the odd rows.
		COLORREF clrListBkEven { GetSysColor(COLOR_WINDOW) };         //List Bk color of the even rows.
		COLORREF clrListBkSel { GetSysColor(COLOR_HIGHLIGHT) };       //Selected item bk color.
		COLORREF clrListBkCellTt { RGB(170, 170, 230) };              //Bk color of a cell that has tooltip.
		COLORREF clrListGrid { RGB(220, 220, 220) };                  //List grid color.
		COLORREF clrTooltipText { 0xFFFFFFFFUL };                     //Tooltip text color, 0xFFFFFFFFUL for current Theme color.
		COLORREF clrTooltipBk { 0xFFFFFFFFUL };                       //Tooltip bk color, 0xFFFFFFFFUL for current Theme color.
		COLORREF clrHdrText { GetSysColor(COLOR_WINDOWTEXT) };        //List header text color.
		COLORREF clrHdrBk { GetSysColor(COLOR_WINDOW) };              //List header bk color.
		COLORREF clrHdrHglInact { GetSysColor(COLOR_GRADIENTINACTIVECAPTION) };//Header highlight inactive.
		COLORREF clrHdrHglAct { GetSysColor(COLOR_GRADIENTACTIVECAPTION) };    //Header highlight active.
		COLORREF clrNWABk { GetSysColor(COLOR_WINDOW) };              //Bk of Non Working Area.
	};

	/**********************************************************************************
	* LISTEXCREATE - Main initialization helper struct for CListEx::Create method.    *
	**********************************************************************************/
	struct LISTEXCREATE {
		LISTEXCOLORS stColor { };             //All control's colors.
		CRect        rect { };                //Initial rect.
		CWnd*        pParent { };             //Parent window.
		LOGFONTW*    pListLogFont { };        //List font.
		LOGFONTW*    pHdrLogFont { };         //Header font.
		UINT         uID { };                 //List control ID.
		DWORD        dwStyle { };             //Control's styles. Zero for default.
		DWORD        dwListGridWidth { 1 };   //Width of the list grid.
		DWORD        dwHdrHeight { };         //Header height.
		bool         fDialogCtrl { false };   //If it's a list within dialog.
		bool         fSortable { false };     //Is list sortable, by clicking on the header column?
		bool         fLinkUnderline { true }; //Links are displayed underlined or not.
		bool         fLinkTooltip { true };   //Show links' toolips or not.
		bool         fTooltipBaloon { true }; //Baloon type tooltip for cells.
		bool         fHighLatency { false };  //Do not redraw window until scrolling completes.
	};

	/********************************************
	* LISTEXHDRICON - Icon for header column.   *
	********************************************/
	struct LISTEXHDRICON {
		POINT pt { };              //Coords of the icon's top-left corner in the header item's rect.
		int   iIndex { };          //Icon index in the header's image list.
		bool  fClickable { true }; //Is icon sending LISTEX_MSG_HDRICONCLICK message when clicked.
	};

	/********************************************
	* IListEx pure virtual base class.          *
	********************************************/
	class IListEx : public CMFCListCtrl {
	public:
		virtual bool Create(const LISTEXCREATE& lcs) = 0;
		virtual void CreateDialogCtrl(UINT uCtrlID, CWnd* pParent) = 0;
		virtual BOOL DeleteAllItems() = 0;
		virtual BOOL DeleteColumn(int nCol) = 0;
		virtual BOOL DeleteItem(int nItem) = 0;
		virtual void Destroy() = 0;
		[[nodiscard]] virtual ULONGLONG GetCellData(int iItem, int iSubitem)const = 0;
		[[nodiscard]] virtual LISTEXCOLORS GetColors()const = 0;
		[[nodiscard]] virtual EListExSortMode GetColumnSortMode(int iColumn)const = 0;
		[[nodiscard]] virtual int GetSortColumn()const = 0;
		[[nodiscard]] virtual bool GetSortAscending()const = 0;
		virtual void HideColumn(int iIndex, bool fHide) = 0;
		virtual int InsertColumn(int nCol, const LVCOLUMNW* pColumn, int iDataAlign = LVCFMT_LEFT, bool fEditable = false) = 0;
		virtual int InsertColumn(int nCol, LPCWSTR pwszName, int nFormat = LVCFMT_LEFT, int nWidth = -1,
			int nSubItem = -1, int iDataAlign = LVCFMT_LEFT, bool fEditable = false) = 0;
		[[nodiscard]] virtual bool IsCreated()const = 0;
		[[nodiscard]] virtual bool IsColumnSortable(int iColumn) = 0;
		virtual void ResetSort() = 0; //Reset all the sort by any column to its default state.
		virtual void SetCellColor(int iItem, int iSubitem, COLORREF clrBk, COLORREF clrText = -1) = 0;
		virtual void SetCellData(int iItem, int iSubitem, ULONGLONG ullData) = 0;
		virtual void SetCellIcon(int iItem, int iSubitem, int iIndex) = 0;
		virtual void SetCellTooltip(int iItem, int iSubitem, std::wstring_view wsvTooltip, std::wstring_view wsvCaption = L"") = 0;
		virtual void SetColors(const LISTEXCOLORS& lcs) = 0;
		virtual void SetColumnColor(int iColumn, COLORREF clrBk, COLORREF clrText = -1) = 0;
		virtual void SetColumnEditable(int iColumn, bool fEditable) = 0;
		virtual void SetColumnSortMode(int iColumn, bool fSortable, EListExSortMode enSortMode = { }) = 0;
		virtual void SetFont(const LOGFONTW* pLogFont) = 0;
		virtual void SetHdrColumnColor(int iColumn, COLORREF clrBk, COLORREF clrText = -1) = 0;
		virtual void SetHdrColumnIcon(int iColumn, const LISTEXHDRICON& stIcon) = 0; //Icon for a given column.
		virtual void SetHdrFont(const LOGFONTW* pLogFont) = 0;
		virtual void SetHdrHeight(DWORD dwHeight) = 0;
		virtual void SetHdrImageList(CImageList* pList) = 0;
		virtual void SetRowColor(DWORD dwRow, COLORREF clrBk, COLORREF clrText = -1) = 0;
		virtual void SetSortable(bool fSortable, PFNLVCOMPARE pfnCompare = nullptr,
			EListExSortMode enSortMode = EListExSortMode::SORT_LEX) = 0;
	};

	/***************************************************************************************
	* Factory function CreateListEx returns IListExPtr - unique_ptr with custom deleter.   *
	* If you, for some reason, need raw pointer, you can directly call CreateRawListEx()   *
	* function, which returns IListEx interface pointer, but in this case you will need to *
	* call IListEx::Destroy method	afterwards - to manually delete ListEx object.         *
	***************************************************************************************/
	IListEx* CreateRawListEx();
	using IListExPtr = std::unique_ptr<IListEx, void(*)(IListEx*)>;

	inline IListExPtr CreateListEx() {
		return IListExPtr(CreateRawListEx(), [](IListEx* p) { p->Destroy(); });
	};

	/****************************************************************************
	* WM_NOTIFY codes (NMHDR.code values)										*
	****************************************************************************/

	constexpr auto LISTEX_MSG_EDITBEGIN { 0x1000U };    //Edit in-place field is about to display.
	constexpr auto LISTEX_MSG_GETCOLOR { 0x1001U };     //Get cell color.
	constexpr auto LISTEX_MSG_GETICON { 0x1002U };      //Get cell icon.
	constexpr auto LISTEX_MSG_GETTOOLTIP { 0x1003U };   //Get cell tool-tip data.
	constexpr auto LISTEX_MSG_HDRICONCLICK { 0x1004U }; //Header's icon has been clicked.
	constexpr auto LISTEX_MSG_HDRRBTNDOWN { 0x1005U };  //Header's WM_RBUTTONDOWN message.
	constexpr auto LISTEX_MSG_HDRRBTNUP { 0x1006U };    //Header's WM_RBUTTONUP message.
	constexpr auto LISTEX_MSG_LINKCLICK { 0x1007U };    //Hyperlink has been clicked.
	constexpr auto LISTEX_MSG_SETDATA { 0x1008U };      //Item text has been edited/changed.

	//Setting a manifest for the ComCtl32.dll version 6.
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
}