/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC applications.                                           *
* Official git repository of the project: https://github.com/jovibor/HexCtrl/           *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#pragma once
#include "../CHexCtrl.h"
#include <afxdialogex.h>  //Standard MFC's controls header.

namespace HEXCTRL::INTERNAL
{
	/********************************************
	* CHexDlgSearch class declaration.			*
	********************************************/
	class CHexDlgSearch final : public CDialogEx
	{
		enum class EMode : WORD { SEARCH_HEX, SEARCH_ASCII, SEARCH_UTF16 };
	public:
		BOOL Create(UINT nIDTemplate, CHexCtrl* pHexCtrl);
		void Search(bool fForward);
		[[nodiscard]] bool IsSearchAvail(); //Can we do search next/prev?
		BOOL ShowWindow(int nCmdShow);
	protected:
		void DoDataExchange(CDataExchange* pDX)override;
		BOOL OnInitDialog()override;
		afx_msg void OnButtonSearchF();
		afx_msg void OnButtonSearchB();
		afx_msg void OnButtonReplace();
		afx_msg void OnButtonReplaceAll();
		afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		afx_msg void OnCancel()override;
		afx_msg void OnDestroy();
		HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
		void OnRadioBnRange(UINT nID);
		[[nodiscard]] CHexCtrl* GetHexCtrl()const;
		void PrepareSearch();
		void Search();
		//ullStart will return index of found occurence, if any.
		bool Find(ULONGLONG& ullStart, ULONGLONG ullEnd, std::byte* pSearch, size_t nSizeSearch, ULONGLONG ullEndSentinel, bool fForward = true);
		void Replace(ULONGLONG ullIndex, std::byte* pData, size_t nSizeData, size_t nSizeReplace, bool fRedraw = true);
		void ResetSearch();
		[[nodiscard]] EMode GetSearchMode(); //Returns current search mode.
		void ComboSearchFill(LPCWSTR pwsz);
		void ComboReplaceFill(LPCWSTR pwsz);
		DECLARE_MESSAGE_MAP()
	private:
		CHexCtrl* m_pHexCtrl { };
		UINT m_uRadioCurrent { };
		CButton m_stChkSel;              //Checkbox "Selection".
		const COLORREF m_clrSearchFailed { RGB(200, 0, 0) };
		const COLORREF m_clrSearchFound { RGB(0, 200, 0) };
		const COLORREF m_clrBkTextArea { GetSysColor(COLOR_MENU) };
		CBrush m_stBrushDefault;
		std::wstring m_wstrTextSearch { };  //String to search for.
		std::wstring m_wstrTextReplace { }; //Search "Replace with..." wstring.
		ULONGLONG m_ullSearchStart { };
		ULONGLONG m_ullSearchEnd { };
		ULONGLONG m_ullOffset { };       //An offset search should start from.
		ULONGLONG m_ullEndSentinel { };  //Maximum offset search can't cross.
		DWORD m_dwCount { };             //How many, or what index number.
		DWORD m_dwReplaced { };          //Replaced amount;
		int m_iDirection { };            //Search direction: 1 = Forward, -1 = Backward.
		int m_iWrap { };                 //Wrap direction: -1 = Beginning, 1 = End.
		bool m_fSecondMatch { false };   //First or subsequent match. 
		bool m_fFound { false };         //Found or not.
		bool m_fDoCount { true };        //Do we count matches or just print "Found".
		bool m_fReplace { false };       //Find or Find and Replace with...?
		bool m_fAll { false };           //Find/Replace one by one, or all?
		bool m_fReplaceWarning { true }; //Show Replace string size exceeds Warning or not.
		bool m_fSelection { false };     //Search in selection.
		std::byte* m_pSearchData { };    //Pointer to the data for search.
		std::byte* m_pReplaceData { };   //Pointer to the data to replace with.
		size_t m_nSizeSearch { };
		size_t m_nSizeReplace { };
		std::string m_strSearch;
		std::string m_strReplace;
	};
}