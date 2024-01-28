/****************************************************************************************
* Copyright © 2018-2024 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../../HexCtrl.h"
#include "../../dep/ListEx/ListEx.h"
#include <afxdialogex.h>

import HEXCTRL.HexUtility;

namespace HEXCTRL::INTERNAL {
	class CHexDlgSearch final : public CDialogEx {
	public:
		[[nodiscard]] auto GetDlgData()const->std::uint64_t;
		void Initialize(IHexCtrl* pHexCtrl);
		[[nodiscard]] bool IsSearchAvail()const; //Can we do search next/prev?
		void SearchNextPrev(bool fForward);
		auto SetDlgData(std::uint64_t ullData, bool fCreate) -> HWND;
		BOOL ShowWindow(int nCmdShow);
	private:
		enum class ESearchMode : std::uint8_t; //Forward declarations.
		enum class ESearchType : std::uint8_t;
		enum class ECmpType : std::uint16_t;
		enum class EMenuID : std::uint16_t;
		struct FINDRESULT;
		struct SEARCHDATA;
		void AddToList(ULONGLONG ullOffset);
		void ApplyDlgData();
		void ClearComboType();
		void ClearList();
		void ComboSearchFill(LPCWSTR pwsz);
		void ComboReplaceFill(LPCWSTR pwsz);
		void DoDataExchange(CDataExchange* pDX)override;
		[[nodiscard]] auto Finder(ULONGLONG& ullStart, ULONGLONG ullEnd, SpanCByte spnSearch,
			bool fForward = true, CHexDlgCallback* pDlgClbk = nullptr, bool fDlgExit = true) -> FINDRESULT;
		[[nodiscard]] auto GetHexCtrl()const->IHexCtrl*;
		[[nodiscard]] auto GetSearchMode()const->ESearchMode; //Getcurrent search mode.
		[[nodiscard]] auto GetSearchType()const->ESearchType; //Get current search type.
		void HexCtrlHighlight(const VecSpan& vecSel);   //Highlight found occurence in HexCtrl.
		[[nodiscard]] bool IsBigEndian()const;
		[[nodiscard]] bool IsInverted()const;
		[[nodiscard]] bool IsMatchCase()const;
		[[nodiscard]] bool IsNoEsc()const;
		[[nodiscard]] bool IsSelection()const;
		[[nodiscard]] bool IsWildcard()const;
		afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		afx_msg void OnButtonSearchF();
		afx_msg void OnButtonSearchB();
		afx_msg void OnButtonFindAll();
		afx_msg void OnButtonReplace();
		afx_msg void OnButtonReplaceAll();
		afx_msg void OnCancel()override;
		afx_msg void OnCheckSel();
		afx_msg void OnClose();
		afx_msg void OnComboModeSelChange();
		afx_msg void OnComboTypeSelChange();
		BOOL OnCommand(WPARAM wParam, LPARAM lParam)override;
		auto OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) -> HBRUSH;
		afx_msg void OnDestroy();
		BOOL OnInitDialog()override;
		afx_msg void OnListGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnListItemChanged(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnListRClick(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnOK()override;
		void OnSelectModeHEXBYTES();
		void OnSelectModeNUMBERS();
		void OnSelectModeSTRUCT();
		void OnSearchModeTEXT();
		void Prepare();
		[[nodiscard]] bool PrepareHexBytes();
		[[nodiscard]] bool PrepareTextASCII();
		[[nodiscard]] bool PrepareTextUTF16();
		[[nodiscard]] bool PrepareTextUTF8();
		template<typename T> requires TSize1248<T>
		[[nodiscard]] bool PrepareNumber();
		[[nodiscard]] bool PrepareFILETIME();
		void Replace(ULONGLONG ullIndex, SpanCByte spnReplace)const;
		void ResetSearch();
		void Search();
		void SetEditStartAt(ULONGLONG ullOffset); //Start search offset edit set.
		void UpdateSearchReplaceControls();
		DECLARE_MESSAGE_MAP();

		//Static functions.
		template<std::uint16_t u16CmpType>
		[[nodiscard]] static bool MemCmp(const std::byte* pBuf1, const std::byte* pBuf2, std::size_t nSize);
		template<typename T>
		[[nodiscard]] static auto RangeToVecBytes(const T& tData) -> std::vector<std::byte>;
		template<std::uint16_t u16CmpType, bool tfDlgClbck>
		static void SearchFunc(SEARCHDATA* pSearch);
	private:
		static constexpr std::byte m_uWildcard { '?' }; //Wildcard symbol.
		static constexpr auto m_pwszWrongInput { L"Wrong input data." };
		IHexCtrl* m_pHexCtrl { };
		ESearchMode m_eSearchMode { };
		LISTEX::IListExPtr m_pListMain { LISTEX::CreateListEx() };
		CMenu m_menuList;                    //Menu for the list control.
		CComboBox m_comboSearch;             //Combo box "Search".
		CComboBox m_comboReplace;            //Combo box "Replace".
		CComboBox m_comboMode;               //Combo box "Search mode".
		CComboBox m_comboType;               //Combo box "Search type".
		CButton m_btnSel;                    //Check box "In selection".
		CButton m_btnWC;                     //Check box "Wildcard".
		CButton m_btnInv;                    //Check box "Inverted".
		CButton m_btnBE;                     //Check box "Big-endian".
		CButton m_btnMC;                     //Check box "Match case".
		CEdit m_editStart;                   //Edit box "Start offset".
		CEdit m_editEnd;                     //Edit box "End offset".
		CEdit m_editStep;                    //Edit box "Step".
		CEdit m_editLimit;                   //Edit box "Limit search hits".
		ULONGLONG m_ullOffsetBoundBegin { }; //Search-start offset boundary.
		ULONGLONG m_ullOffsetBoundEnd { };   //Search-end offset boundary.
		ULONGLONG m_ullOffsetCurr { };       //Current offset that search should start from.
		ULONGLONG m_ullOffsetSentinel { };   //The maximum offset that search can't cross.
		ULONGLONG m_ullStep { 1 };           //Search step (default is 1 byte).
		std::uint64_t m_u64DlgData { };      //Data from SetDlgData.
		DWORD m_dwCount { };                 //How many, or what index number.
		DWORD m_dwReplaced { };              //Replaced amount;
		DWORD m_dwFoundLimit { 10000 };      //Maximum found search occurences.
		int m_iDirection { };                //Search direction: 1 = Forward, -1 = Backward.
		int m_iWrap { };                     //Wrap direction: -1 = Beginning, 1 = End.
		std::vector<ULONGLONG> m_vecSearchRes;   //Search results.
		std::vector<std::byte> m_vecSearchData;  //Data to search for.
		std::vector<std::byte> m_vecReplaceData; //Data to replace with.
		std::wstring m_wstrTextSearch;       //Text from "Search" box.
		std::wstring m_wstrTextReplace;      //Text from "Replace with..." box.
		HEXSPAN m_stSelSpan { };             //Previous selection.
		void(*m_pfnSearchDlgClbck)(SEARCHDATA* pSearch); //Func pointer to the main SearchFunc<> with the callback dialog.
		void(*m_pfnSearchNODlg)(SEARCHDATA* pSearch); //Func pointer to the main SearchFunc<> without callback dialog.
		bool m_fSecondMatch { false };       //First or subsequent match. 
		bool m_fFound { false };             //Found or not.
		bool m_fDoCount { true };            //Do we count matches or just print "Found".
		bool m_fReplace { false };           //Find or Find and Replace with...?
		bool m_fAll { false };               //Find/Replace one by one, or all?
		bool m_fReplaceWarn { true };        //Show "Replace string size exceeds..." warning message or not.
		bool m_fSearchNext { false };        //Search through Next/Prev menu.
	};
}