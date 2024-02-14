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
#include <locale>

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
		enum class EMenuID : std::uint16_t;
		struct FINDERDATA;
		struct SEARCHFUNCDATA;
		using PtrSearchFunc = void(*)(SEARCHFUNCDATA*);
		using VecSearchResult = std::vector<ULONGLONG>;
		void AddToList(ULONGLONG ullOffset);
		void ApplyDlgData();
		void ClearComboType();
		void ClearList();
		void ComboSearchFill(LPCWSTR pwsz);
		void ComboReplaceFill(LPCWSTR pwsz);
		void DoDataExchange(CDataExchange* pDX)override;
		void FindAll(FINDERDATA& refFinder);
		[[nodiscard]] auto GetHexCtrl()const->IHexCtrl*;
		auto GetSearchFunc(bool fFwd, bool fDlgClbck)const->PtrSearchFunc;
		template<bool fDlgClbck>
		[[nodiscard]] auto GetSearchFuncFwd()const->PtrSearchFunc;
		template<bool fDlgClbck>
		[[nodiscard]] auto GetSearchFuncBack()const->PtrSearchFunc;
		[[nodiscard]] auto GetSearchMode()const->ESearchMode; //Getcurrent search mode.
		[[nodiscard]] auto GetSearchType()const->ESearchType; //Get current search type.
		void HexCtrlHighlight(const VecSpan& vecSel); //Highlight found occurence in the HexCtrl.
		[[nodiscard]] bool IsBigEndian()const;
		[[nodiscard]] bool IsForward()const;
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
		void ReplaceAll(FINDERDATA& refFinder);
		void ResetSearch();
		void Search();
		void SearchAfter(bool fCanceled);
		void SetEditStartAt(ULONGLONG ullOffset); //Start search offset edit set.
		void UpdateSearchReplaceControls();
		DECLARE_MESSAGE_MAP();

		//Static functions.
		static void Replace(IHexCtrl* pHexCtrl, ULONGLONG ullIndex, SpanCByte spnReplace);
		struct FINDRESULT;
		static auto Finder(const FINDERDATA& stFinder) -> FINDRESULT;
		enum class EMemCmp : std::uint8_t {
			DATA_BYTE1, DATA_BYTE2, DATA_BYTE4, DATA_BYTE8, CHAR_STR, WCHAR_STR
		};
		struct SEARCHTYPE { //Compile time struct for template parameters in the SearchFunc and MemCmp.
			constexpr SEARCHTYPE() = default;
			constexpr SEARCHTYPE(EMemCmp eMemCmp, bool fDlgClbck = false, bool fMatchCase = false,
				bool fWildcard = false, bool fSIMD = false, bool fInverted = false) :
				eMemCmp { eMemCmp }, fDlgClbck { fDlgClbck }, fMatchCase { fMatchCase }, fWildcard { fWildcard },
				fSIMD { fSIMD }, fInverted { fInverted } {}
			constexpr ~SEARCHTYPE() = default;
			EMemCmp eMemCmp { };
			bool fDlgClbck { false };
			bool fMatchCase { false };
			bool fWildcard { false };
			bool fSIMD { false };
			bool fInverted { false };
		};
		template<SEARCHTYPE stType>
		[[nodiscard]] static bool MemCmp(const std::byte* pWhere, const std::byte* pWhat, std::size_t nSize);
		[[nodiscard]] static int __forceinline MemCmpSIMDEQByte1(const __m128i* pWhere, std::byte bWhat);
		[[nodiscard]] static int __forceinline MemCmpSIMDEQByte1Inv(const __m128i* pWhere, std::byte bWhat);
		template<SEARCHTYPE stType>
		static void SearchFuncFwd(SEARCHFUNCDATA* pSearch);
		template<SEARCHTYPE stType>
		static void SearchFuncBack(SEARCHFUNCDATA* pSearch);
	private:
		static constexpr std::byte m_uWildcard { '?' }; //Wildcard symbol.
		static constexpr auto m_pwszWrongInput { L"Wrong input data." };
		IHexCtrl* m_pHexCtrl { };
		ESearchMode m_eSearchMode { };
		std::locale m_locale;
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
		int m_iWrap { };                     //Wrap direction: -1 = Beginning, 1 = End.
		VecSearchResult m_vecSearchRes;      //Search results.
		std::vector<std::byte> m_vecSearchData;  //Data to search for.
		std::vector<std::byte> m_vecReplaceData; //Data to replace with.
		std::wstring m_wstrTextSearch;       //Text from "Search" box.
		std::wstring m_wstrTextReplace;      //Text from "Replace with..." box.
		HEXSPAN m_stSelSpan { };             //Previous selection.
		bool m_fForward { };                 //Search direction, Forward/Backward.
		bool m_fSecondMatch { false };       //First or subsequent match. 
		bool m_fFound { false };             //Found or not.
		bool m_fDoCount { true };            //Do we count matches or just print "Found".
		bool m_fReplace { false };           //Find or Find and Replace with...?
		bool m_fAll { false };               //Find/Replace one by one, or all?
		bool m_fSearchNext { false };        //Search through Next/Prev menu.
	};
}