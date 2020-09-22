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
#include "../ListEx/ListEx.h"
#include <afxdialogex.h>  //Standard MFC's controls header.

namespace HEXCTRL::INTERNAL
{
	using namespace LISTEX;
	/********************************************
	* CHexDlgSearch class declaration.			*
	********************************************/
	class CHexDlgSearch final : public CDialogEx
	{
		enum class EMode : WORD {
			SEARCH_HEX, SEARCH_ASCII, SEARCH_WCHAR,
			SEARCH_BYTE, SEARCH_WORD, SEARCH_DWORD, SEARCH_QWORD,
			SEARCH_FLOAT, SEARCH_DOUBLE
		};
	public:
		BOOL Create(UINT nIDTemplate, CHexCtrl* pHexCtrl);
		void Search(bool fForward);
		[[nodiscard]] bool IsSearchAvail(); //Can we do search next/prev?
		BOOL ShowWindow(int nCmdShow);
	protected:
		void DoDataExchange(CDataExchange* pDX)override;
		BOOL OnInitDialog()override;
		afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		afx_msg void OnButtonSearchF();
		afx_msg void OnButtonSearchB();
		afx_msg void OnButtonFindAll();
		afx_msg void OnButtonReplace();
		afx_msg void OnButtonReplaceAll();
		afx_msg void OnButtonClear();
		afx_msg void OnCheckSel();
		afx_msg void OnComboModeSelChange();
		afx_msg void OnListGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnListItemChanged(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnOK()override;
		afx_msg void OnCancel()override;
		afx_msg void OnDestroy();
		afx_msg void OnListRClick(NMHDR *pNMHDR, LRESULT *pResult);
		BOOL OnCommand(WPARAM wParam, LPARAM lParam)override;
		HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
		void AddToList(ULONGLONG ullOffset);
		void HexCtrlHighlight(ULONGLONG ullOffset, ULONGLONG ullSize); //Highlight found in HexCtrl.
		[[nodiscard]] CHexCtrl* GetHexCtrl()const;
		void PrepareSearch();
		[[nodiscard]] bool PrepareHex();
		[[nodiscard]] bool PrepareASCII();
		[[nodiscard]] bool PrepareWCHAR();
		[[nodiscard]] bool PrepareBYTE();
		[[nodiscard]] bool PrepareWORD();
		[[nodiscard]] bool PrepareDWORD();
		[[nodiscard]] bool PrepareQWORD();
		[[nodiscard]] bool PrepareFloat();
		[[nodiscard]] bool PrepareDouble();
		void Search();
		//ullStart will return index of found occurence, if any.
		bool Find(ULONGLONG& ullStart, ULONGLONG ullEnd, std::byte* pSearch, size_t nSizeSearch,
			ULONGLONG ullEndSentinel, bool fForward = true, bool fThread = true);
		void Replace(ULONGLONG ullIndex, std::byte* pData, size_t nSizeData, size_t nSizeReplace,
			bool fRedraw = true, bool fParentNtfy = true);
		void ResetSearch();
		[[nodiscard]] EMode GetSearchMode(); //Returns current search mode.
		void ComboSearchFill(LPCWSTR pwsz);
		void ComboReplaceFill(LPCWSTR pwsz);
		void SetEditStartAt(ULONGLONG ullOffset); //Start search offset edit set.
		void SetEditStep(ULONGLONG ullStep);
		DECLARE_MESSAGE_MAP()
	private:
		CHexCtrl* m_pHexCtrl { };
		EMode m_eModeCurr { };
		IListExPtr m_pListMain { CreateListEx() };
		std::vector<ULONGLONG> m_vecSearchRes { };
		CComboBox m_stComboSearch;  //Combo box "Search".
		CComboBox m_stComboReplace; //Combo box "Replace".
		CComboBox m_stComboMode;    //Combo box "Search mode".
		CButton m_stCheckSel;       //Check box "Selection".
		CEdit m_stEditStart;        //Edit "Start search at".
		CEdit m_stEditStep;         //Edit "Step".
		const COLORREF m_clrSearchFailed { RGB(200, 0, 0) };
		const COLORREF m_clrSearchFound { RGB(0, 200, 0) };
		const COLORREF m_clrBkTextArea { GetSysColor(COLOR_MENU) };
		CBrush m_stBrushDefault;
		std::wstring m_wstrTextSearch { };  //String to search for.
		std::wstring m_wstrTextReplace { }; //Search "Replace with..." wstring.
		ULONGLONG m_ullOffsetStart { };  //Search start boundary.
		ULONGLONG m_ullOffsetEnd { };    //Search end boundary.
		ULONGLONG m_ullOffsetCurr { };   //Current offset search should start from.
		ULONGLONG m_ullEndSentinel { };  //Maximum offset search can't cross.
		ULONGLONG m_ullStep { 1 };       //Search step (default is 1 byte).
		DWORD m_dwCount { };             //How many, or what index number.
		DWORD m_dwReplaced { };          //Replaced amount;
		DWORD m_dwMaxSearch { 1000 };    //Maximum found search occurences.
		int m_iDirection { };            //Search direction: 1 = Forward, -1 = Backward.
		int m_iWrap { };                 //Wrap direction: -1 = Beginning, 1 = End.
		bool m_fSecondMatch { false };   //First or subsequent match. 
		bool m_fFound { false };         //Found or not.
		bool m_fDoCount { true };        //Do we count matches or just print "Found".
		bool m_fReplace { false };       //Find or Find and Replace with...?
		bool m_fAll { false };           //Find/Replace one by one, or all?
		bool m_fSelection { false };     //Search in selection.
		std::byte* m_pSearchData { };    //Pointer to the data for search.
		std::byte* m_pReplaceData { };   //Pointer to the data to replace with.
		size_t m_nSizeSearch { };
		size_t m_nSizeReplace { };
		std::string m_strSearch;
		std::string m_strReplace;
		std::wstring_view m_wstrWrongInput { L"Wrong input data!" };
		HEXSPANSTRUCT m_stSelSpan { };   //Previous selection.
		CMenu m_stMenuList;
		enum EMenuID {
			IDC_HEXCTRL_SEARCH_MENU_ADDBKM = 0x8000,
			IDC_HEXCTRL_SEARCH_MENU_SELECTALL = 0x8001,
			IDC_HEXCTRL_SEARCH_MENU_CLEARALL = 0x8002
		};
	};
}