/********************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/					*
* Github repository URL: https://github.com/jovibor/ListEx						*
* This software is available under the "MIT License".							*
* This is an extended and featured version of CMFCListCtrl class.				*
* CListEx - list control class with the ability to set tooltips on arbitrary	*
* cells, and also with a lots of other stuff to customize your control in many	*
* different aspects. For more info see official documentation on github.		*
********************************************************************************/
#pragma once
#include <afxwin.h>
#include <memory>

namespace HEXCTRL::INTERNAL::LISTEX {
	/********************************************************************************************
	* LISTEXCOLORSTRUCT - All ListEx colors.													*
	********************************************************************************************/
	struct LISTEXCOLORSTRUCT
	{
		COLORREF clrListText { GetSysColor(COLOR_WINDOWTEXT) };            //List text color.
		COLORREF clrListBkRow1 { GetSysColor(COLOR_WINDOW) };              //List Bk color of the odd rows.
		COLORREF clrListBkRow2 { GetSysColor(COLOR_WINDOW) };              //List Bk color of the even rows.
		COLORREF clrListGrid { RGB(220, 220, 220) };                       //List grid color.
		COLORREF clrListTextSelected { GetSysColor(COLOR_HIGHLIGHTTEXT) }; //Selected item text color.
		COLORREF clrListBkSelected { GetSysColor(COLOR_HIGHLIGHT) };       //Selected item bk color.
		COLORREF clrTooltipText { GetSysColor(COLOR_INFOTEXT) };           //Tooltip window text color.
		COLORREF clrTooltipBk { GetSysColor(COLOR_INFOBK) };               //Tooltip window bk color.
		COLORREF clrListTextCellTt { GetSysColor(COLOR_WINDOWTEXT) };      //Text color of a cell that has tooltip.
		COLORREF clrListBkCellTt { RGB(170, 170, 230) };                   //Bk color of a cell that has tooltip.
		COLORREF clrHdrText { GetSysColor(COLOR_WINDOWTEXT) };             //List header text color.
		COLORREF clrHdrBk { GetSysColor(COLOR_WINDOW) };                   //List header bk color.
		COLORREF clrHdrHglInactive { GetSysColor(COLOR_GRADIENTINACTIVECAPTION) };//Header highlight inactive.
		COLORREF clrHdrHglActive { GetSysColor(COLOR_GRADIENTACTIVECAPTION) };    //Header highlight active.
		COLORREF clrBkNWA { GetSysColor(COLOR_WINDOW) };                   //Bk of non working area.
	};

	/********************************************************************************************
	* LISTEXCREATESTRUCT - Main initialization helper struct for CListEx::Create method.		*
	********************************************************************************************/
	struct LISTEXCREATESTRUCT {
		LISTEXCOLORSTRUCT stColor { };           //All control's colors.
		CRect             rect;                  //Initial rect.
		CWnd*             pwndParent { };        //Parent window.
		const LOGFONTW*   pListLogFont { };      //List font.
		const LOGFONTW*   pHeaderLogFont { };    //List header font.
		DWORD             dwStyle { };           //Control's styles. Zero for default.
		UINT              uID { };               //Control Id.
		DWORD             dwListGridWidth { 1 }; //Width of the list grid.
		DWORD             dwHeaderHeight { 20 }; //List header height.
		bool              fDialogCtrl { false }; //If it's a list within dialog.
	};

	/********************************************
	* CListEx class definition.					*
	********************************************/
	class IListEx : public CMFCListCtrl
	{
	public:
		IListEx() = default;
		virtual ~IListEx() = default;
		virtual bool Create(const LISTEXCREATESTRUCT& lcs) = 0;
		virtual void CreateDialogCtrl(UINT uCtrlID, CWnd* pwndDlg) = 0;
		virtual BOOL DeleteAllItems() = 0;
		virtual BOOL DeleteItem(int nItem) = 0;
		virtual void Destroy() = 0;
		virtual DWORD_PTR GetCellData(int iItem, int iSubitem) = 0;
		virtual UINT GetFontSize() = 0;
		virtual int GetSortColumn()const = 0;
		virtual bool GetSortAscending()const = 0;
		virtual bool IsCreated()const = 0;
		virtual void SetCellColor(int iItem, int iSubitem, COLORREF clr) = 0;
		virtual void SetCellData(int iItem, int iSubitem, DWORD_PTR dwData) = 0;
		virtual void SetCellMenu(int iItem, int iSubitem, CMenu* pMenu) = 0;
		virtual void SetCellTooltip(int iItem, int iSubitem, const wchar_t* pwszTooltip, const wchar_t* pwszCaption = nullptr) = 0;
		virtual void SetColor(const LISTEXCOLORSTRUCT& lcs) = 0;
		virtual void SetFont(const LOGFONTW* pLogFontNew) = 0;
		virtual void SetFontSize(UINT uiSize) = 0;
		virtual void SetHeaderHeight(DWORD dwHeight) = 0;
		virtual void SetHeaderFont(const LOGFONT* pLogFontNew) = 0;
		virtual void SetHeaderColumnColor(DWORD nColumn, COLORREF clr) = 0;
		virtual void SetListMenu(CMenu* pMenu) = 0;
		virtual void SetSortFunc(int (CALLBACK *pfCompareFunc)(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)) = 0;
	};

	/********************************************************************************************
	* Factory function CreateListEx returns IListExUnPtr - unique_ptr with custom deleter .		*
	* In client code you should use IListExPtr type which is an alias to either IListExUnPtr	*
	* - a unique_ptr, or IListExShPtr - a shared_ptr. Uncomment what serves best for you,		*
	* and comment out the other.																*
	* If you, for some reason, need raw pointer, you can directly call CreateRawListEx			*
	* function, which returns IListEx interface pointer, but in this case you will need to		*
	* call IListEx::Destroy method	afterwards - to manually delete ListEx object.				*
	********************************************************************************************/
	IListEx* CreateRawListEx();
	using IListExUnPtr = std::unique_ptr<IListEx, void(*)(IListEx*)>;
	using IListExShPtr = std::shared_ptr<IListEx>;

	inline IListExUnPtr CreateListEx()
	{
		return IListExUnPtr(CreateRawListEx(), [](IListEx * p) { p->Destroy(); });
	};

	using IListExPtr = IListExUnPtr;
	//using IListExPtr = IListExShPtr;

	/****************************************************************************
	* WM_NOTIFY codes (NMHDR.code values)										*
	****************************************************************************/

	constexpr auto LISTEX_MSG_MENUSELECTED = 0x00001000;
}