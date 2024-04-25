/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
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
		[[nodiscard]] auto GetDlgItemHandle(EHexDlgItem eItem)const->HWND;
		void Initialize(IHexCtrl* pHexCtrl);
		[[nodiscard]] bool IsSearchAvail()const; //Can we do search next/prev?
		void SearchNextPrev(bool fForward);
		void SetDlgProperties(std::uint64_t u64Flags);
		BOOL ShowWindow(int nCmdShow);
	private:
		enum class ESearchMode : std::uint8_t; //Forward declarations.
		enum class ESearchType : std::uint8_t;
		enum class EMenuID : std::uint16_t;
		struct SEARCHFUNCDATA;
		struct FINDRESULT;
		using PtrSearchFunc = auto(*)(const SEARCHFUNCDATA&)->FINDRESULT;
		using VecSearchResult = std::vector<ULONGLONG>;

		void AddToList(ULONGLONG ullOffset);
		void CalcMemChunks(SEARCHFUNCDATA& refData)const;
		void ClearComboType();
		void ClearList();
		void ComboSearchFill(LPCWSTR pwsz);
		void ComboReplaceFill(LPCWSTR pwsz);
		[[nodiscard]] auto CreateSearchData(CHexDlgCallback* pDlgClbk = nullptr)const->SEARCHFUNCDATA;
		void DoDataExchange(CDataExchange* pDX)override;
		void FindAll();
		void FindForward();
		void FindBackward();
		[[nodiscard]] auto GetHexCtrl()const->IHexCtrl*;
		[[nodiscard]] auto GetLastSearchOffset()const->ULONGLONG;
		[[nodiscard]] auto GetRngStart()const->ULONGLONG;
		[[nodiscard]] auto GetRngEnd()const->ULONGLONG;
		[[nodiscard]] auto GetRngSize()const->ULONGLONG;     //Size of the range to search within.
		[[nodiscard]] auto GetReplaceDataSize()const->DWORD; //Replace vec data size.
		[[nodiscard]] auto GetReplaceSpan()const->SpanCByte;
		[[nodiscard]] auto GetSearchDataSize()const->DWORD;  //Search vec data size.
		[[nodiscard]] auto GetSearchRngSize()const->ULONGLONG;
		[[nodiscard]] auto GetSearchFunc(bool fFwd, bool fDlgClbck)const->PtrSearchFunc;
		template<bool fDlgClbck>
		[[nodiscard]] auto GetSearchFuncFwd()const->PtrSearchFunc;
		template<bool fDlgClbck>
		[[nodiscard]] auto GetSearchFuncBack()const->PtrSearchFunc;
		[[nodiscard]] auto GetSearchMode()const->ESearchMode; //Getcurrent search mode.
		[[nodiscard]] auto GetSearchSpan()const->SpanCByte;
		[[nodiscard]] auto GetSearchType()const->ESearchType; //Get current search type.
		[[nodiscard]] auto GetSentinel()const->ULONGLONG;
		[[nodiscard]] auto GetStartFrom()const->ULONGLONG;   //Start search from.
		[[nodiscard]] auto GetStep()const->ULONGLONG;
		void HexCtrlHighlight(const VecSpan& vecSel); //Highlight found occurence in the HexCtrl.
		[[nodiscard]] bool IsBigEndian()const;
		[[nodiscard]] bool IsForward()const;
		[[nodiscard]] bool IsFreshSearch()const;
		[[nodiscard]] bool IsInverted()const;
		[[nodiscard]] bool IsMatchCase()const;
		[[nodiscard]] bool IsNoEsc()const;
		[[nodiscard]] bool IsReplace()const;
		[[nodiscard]] bool IsSelection()const;
		[[nodiscard]] bool IsSmallSearch()const;
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
		void ReplaceAll();
		void ResetSearch();
		void Search();
		void SetControlsState();
		void SetEditStartFrom(ULONGLONG ullOffset); //Start search offset edit set.
		DECLARE_MESSAGE_MAP();

		//Static functions.
		static void Replace(IHexCtrl* pHexCtrl, ULONGLONG ullIndex, SpanCByte spnReplace);
		enum class EMemCmp : std::uint8_t {
			DATA_BYTE1, DATA_BYTE2, DATA_BYTE4, DATA_BYTE8, CHAR_STR, WCHAR_STR
		};
		struct SEARCHTYPE { //Compile time struct for template parameters in the SearchFunc and MemCmp.
			constexpr SEARCHTYPE() = default;
			constexpr SEARCHTYPE(EMemCmp eMemCmp, bool fDlgClbck = false, bool fMatchCase = false,
				bool fWildcard = false, bool fInverted = false) :
				eMemCmp { eMemCmp }, fDlgClbck { fDlgClbck }, fMatchCase { fMatchCase }, fWildcard { fWildcard },
				fInverted { fInverted } {}
			constexpr ~SEARCHTYPE() = default;
			EMemCmp eMemCmp { };
			bool fDlgClbck { false };
			bool fMatchCase { false };
			bool fWildcard { false };
			bool fInverted { false };
		};
		template<SEARCHTYPE stType>
		[[nodiscard]] static auto __forceinline MemCmp(const std::byte* pWhere, const std::byte* pWhat, std::size_t nSize)->bool;
		//Vector functions return index in vector of found element. Index greater than 15 means not found.
		[[nodiscard]] static auto __forceinline MemCmpVecEQByte1(const __m128i* pWhere, std::byte bWhat)->int;
		[[nodiscard]] static auto __forceinline MemCmpVecNEQByte1(const __m128i* pWhere, std::byte bWhat)->int;
		[[nodiscard]] static auto __forceinline MemCmpVecEQByte2(const __m128i* pWhere, std::uint16_t ui16What)->int;
		[[nodiscard]] static auto __forceinline MemCmpVecNEQByte2(const __m128i* pWhere, std::uint16_t ui16What)->int;
		[[nodiscard]] static auto __forceinline MemCmpVecEQByte4(const __m128i* pWhere, std::uint32_t ui32What)->int;
		[[nodiscard]] static auto __forceinline MemCmpVecNEQByte4(const __m128i* pWhere, std::uint32_t ui32What)->int;
		template<SEARCHTYPE stType>
		[[nodiscard]] static auto SearchFuncFwd(const SEARCHFUNCDATA& refSearch) -> FINDRESULT;
		template<SEARCHTYPE stType>
		[[nodiscard]] static auto SearchFuncFwdByte1(const SEARCHFUNCDATA& refSearch) -> FINDRESULT;
		template<SEARCHTYPE stType>
		[[nodiscard]] static auto SearchFuncFwdByte2(const SEARCHFUNCDATA& refSearch) -> FINDRESULT;
		template<SEARCHTYPE stType>
		[[nodiscard]] static auto SearchFuncFwdByte4(const SEARCHFUNCDATA& refSearch) -> FINDRESULT;
		template<SEARCHTYPE stType>
		[[nodiscard]] static auto SearchFuncBack(const SEARCHFUNCDATA& refSearch) -> FINDRESULT;
	private:
		static constexpr std::byte m_uWildcard { '?' }; //Wildcard symbol.
		static constexpr auto m_pwszWrongInput { L"Wrong input data." };
		IHexCtrl* m_pHexCtrl { };
		ESearchMode m_eSearchMode { };
		LISTEX::IListExPtr m_pListMain { LISTEX::CreateListEx() };
		CMenu m_menuList;                    //Menu for the list control.
		CComboBox m_comboFind;             //Combo box "Search".
		CComboBox m_comboReplace;            //Combo box "Replace".
		CComboBox m_comboMode;               //Combo box "Search mode".
		CComboBox m_comboType;               //Combo box "Search type".
		CButton m_btnSel;                    //Check box "In selection".
		CButton m_btnWC;                     //Check box "Wildcard".
		CButton m_btnInv;                    //Check box "Inverted".
		CButton m_btnBE;                     //Check box "Big-endian".
		CButton m_btnMC;                     //Check box "Match case".
		CEdit m_editStartFrom;               //Edit box "Start from".
		CEdit m_editStep;                    //Edit box "Step".
		CEdit m_editRngBegin;                //Edit box "Range begin".
		CEdit m_editRngEnd;                  //Edit box "Range end".
		CEdit m_editLimit;                   //Edit box "Limit search hits".
		ULONGLONG m_ullStartFrom { };        //"Start form" search offset.
		ULONGLONG m_ullRngBegin { };
		ULONGLONG m_ullRngEnd { };
		ULONGLONG m_ullStep { 1 };           //Search step (default is 1 byte).
		std::uint64_t m_u64Flags { };      //Data from SetDlgProperties.
		DWORD m_dwCount { };                 //How many, or what index number.
		DWORD m_dwReplaced { };              //Replaced amount;
		DWORD m_dwLimit { 10000 };           //Maximum found search occurences.
		int m_iWrap { };                     //Wrap direction: -1 = Beginning, 1 = End.
		VecSearchResult m_vecSearchRes;      //Search results.
		std::vector<std::byte> m_vecSearchData;  //Data to search for.
		std::vector<std::byte> m_vecReplaceData; //Data to replace with.
		std::wstring m_wstrSearch;           //Text from "Search" box.
		std::wstring m_wstrReplace;          //Text from "Replace with..." box.
		bool m_fForward { };                 //Search direction, Forward/Backward.
		bool m_fSecondMatch { false };       //First or subsequent match. 
		bool m_fFound { false };             //Found or not.
		bool m_fDoCount { true };            //Do we count matches or just print "Found".
		bool m_fReplace { false };           //Find or Find and Replace with...?
		bool m_fAll { false };               //Find/Replace one by one, or all?
		bool m_fSearchNext { false };        //Search through Next/Prev menu.
		bool m_fFreshSearch { true };
	};
}