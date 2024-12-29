/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../../HexCtrl.h"
#include <chrono>

import HEXCTRL.HexUtility;
import HEXCTRL.CHexDlgProgress;

namespace HEXCTRL::INTERNAL {
	class CHexDlgSearch final {
	public:
		void ClearData();
		void CreateDlg();
		[[nodiscard]] auto GetDlgItemHandle(EHexDlgItem eItem)const->HWND;
		[[nodiscard]] auto GetHWND()const->HWND;
		void Initialize(IHexCtrl* pHexCtrl, HINSTANCE hInstRes);
		[[nodiscard]] bool IsSearchAvail()const; //Can we do search next/prev?
		[[nodiscard]] bool PreTranslateMsg(MSG* pMsg);
		[[nodiscard]] auto ProcessMsg(const MSG& msg) -> INT_PTR;
		void SearchNextPrev(bool fForward);
		void SetDlgProperties(std::uint64_t u64Flags);
		void ShowWindow(int iCmdShow);
	private:
		enum class ESearchMode : std::uint8_t; //Forward declarations.
		enum class ESearchType : std::uint8_t;
		enum class EVecSize : std::uint8_t;
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
		[[nodiscard]] auto CreateSearchData(CHexDlgProgress* pDlgProg = nullptr)const->SEARCHFUNCDATA;
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
		[[nodiscard]] auto GetSearchFunc(bool fFwd, bool fDlgProg)const->PtrSearchFunc;
		template<bool fDlgProg, EVecSize eVecSize>
		[[nodiscard]] auto GetSearchFuncFwd()const->PtrSearchFunc;
		template<bool fDlgProg, EVecSize eVecSize>
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
		auto OnActivate(const MSG& msg) -> INT_PTR;
		void OnButtonSearchF();
		void OnButtonSearchB();
		void OnButtonFindAll();
		void OnButtonReplace();
		void OnButtonReplaceAll();
		void OnCancel();
		void OnCheckSel();
		auto OnClose() -> INT_PTR;
		void OnComboModeSelChange();
		void OnComboTypeSelChange();
		auto OnCommand(const MSG& msg) -> INT_PTR;
		auto OnCtlClrStatic(const MSG& msg) -> INT_PTR;
		auto OnDestroy() -> INT_PTR;
		auto OnDrawItem(const MSG& msg) -> INT_PTR;
		auto OnInitDialog(const MSG& msg) -> INT_PTR;
		auto OnMeasureItem(const MSG& msg) -> INT_PTR;
		auto OnNotify(const MSG& msg) -> INT_PTR;
		void OnNotifyListGetDispInfo(NMHDR *pNMHDR);
		void OnNotifyListItemChanged(NMHDR *pNMHDR);
		void OnNotifyListRClick(NMHDR *pNMHDR);
		void OnOK();
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
	private:
		//Static functions.
		static void Replace(IHexCtrl* pHexCtrl, ULONGLONG ullIndex, SpanCByte spnReplace);
		enum class EMemCmp : std::uint8_t { DATA_BYTE1, DATA_BYTE2, DATA_BYTE4, DATA_BYTE8, CHAR_STR, WCHAR_STR };
		enum class EVecSize : std::uint8_t { VEC128 = 16, VEC256 = 32 /*SSE4.2 = sizeof(__m128), AVX2 = sizeof(__m256).*/ };
		struct SEARCHTYPE { //Compile time struct for template parameters in the SearchFunc and MemCmp.
			constexpr SEARCHTYPE() = default;
			constexpr SEARCHTYPE(EMemCmp eMemCmp, EVecSize eVecSize, bool fDlgProg = false, bool fMatchCase = false,
				bool fWildcard = false, bool fInverted = false) :
				eMemCmp { eMemCmp }, eVecSize { eVecSize }, fDlgProg { fDlgProg }, fMatchCase { fMatchCase },
				fWildcard { fWildcard }, fInverted { fInverted } {
			}
			constexpr ~SEARCHTYPE() = default;
			EMemCmp eMemCmp { };
			EVecSize eVecSize { };
			bool fDlgProg { false };
			bool fMatchCase { false };
			bool fWildcard { false };
			bool fInverted { false };
		};
		template<SEARCHTYPE stType>
		[[nodiscard]] static auto __forceinline MemCmp(const std::byte* pWhere, const std::byte* pWhat, std::size_t nSize)->bool;
		//Vector functions return index within the vector of found element.
		//Index greater than sizeof(vec)-1 means not found.
		template<EVecSize eVecSize>
		[[nodiscard]] static auto __forceinline MemCmpVecEQByte1(const std::byte* pWhere, std::byte bWhat)->int;
		template<EVecSize eVecSize>
		[[nodiscard]] static auto __forceinline MemCmpVecNEQByte1(const std::byte* pWhere, std::byte bWhat)->int;
		template<EVecSize eVecSize>
		[[nodiscard]] static auto __forceinline MemCmpVecEQByte2(const std::byte* pWhere, std::uint16_t ui16What)->int;
		template<EVecSize eVecSize>
		[[nodiscard]] static auto __forceinline MemCmpVecNEQByte2(const std::byte* pWhere, std::uint16_t ui16What)->int;
		template<EVecSize eVecSize>
		[[nodiscard]] static auto __forceinline MemCmpVecEQByte4(const std::byte* pWhere, std::uint32_t ui32What)->int;
		template<EVecSize eVecSize>
		[[nodiscard]] static auto __forceinline MemCmpVecNEQByte4(const std::byte* pWhere, std::uint32_t ui32What)->int;
		template<SEARCHTYPE stType>
		[[nodiscard]] static auto SearchFuncFwd(const SEARCHFUNCDATA& refSearch) -> FINDRESULT;
		template<SEARCHTYPE stType>
		[[nodiscard]] static auto SearchFuncVecFwdByte1(const SEARCHFUNCDATA& refSearch) -> FINDRESULT;
		template<SEARCHTYPE stType>
		[[nodiscard]] static auto SearchFuncVecFwdByte2(const SEARCHFUNCDATA& refSearch) -> FINDRESULT;
		template<SEARCHTYPE stType>
		[[nodiscard]] static auto SearchFuncVecFwdByte4(const SEARCHFUNCDATA& refSearch) -> FINDRESULT;
		template<SEARCHTYPE stType>
		[[nodiscard]] static auto SearchFuncBack(const SEARCHFUNCDATA& refSearch) -> FINDRESULT;
	private:
		static constexpr std::byte m_uWildcard { '?' }; //Wildcard symbol.
		static constexpr auto m_pwszWrongInput { L"Wrong input data." };
		HINSTANCE m_hInstRes { };
		wnd::CWnd m_Wnd;                 //Main window.
		wnd::CWnd m_WndStatResult;       //Static text "Result:".
		wnd::CWndCombo m_WndCmbFind;     //Combo box "Search".
		wnd::CWndCombo m_WndCmbReplace;  //Combo box "Replace".
		wnd::CWndCombo m_WndCmbMode;     //Combo box "Search mode".
		wnd::CWndCombo m_WndCmbType;     //Combo box "Search type".
		wnd::CWndBtn m_WndBtnSel;        //Check box "In selection".
		wnd::CWndBtn m_WndBtnWC;         //Check box "Wildcard".
		wnd::CWndBtn m_WndBtnInv;        //Check box "Inverted".
		wnd::CWndBtn m_WndBtnBE;         //Check box "Big-endian".
		wnd::CWndBtn m_WndBtnMC;         //Check box "Match case".
		wnd::CWndEdit m_WndEditStart;    //Edit box "Start from".
		wnd::CWndEdit m_WndEditStep;     //Edit box "Step".
		wnd::CWndEdit m_WndEditRngBegin; //Edit box "Range begin".
		wnd::CWndEdit m_WndEditRngEnd;   //Edit box "Range end".
		wnd::CWndEdit m_WndEditLimit;    //Edit box "Limit search hits".
		wnd::CMenu m_MenuList;           //Menu for the list control.
		IHexCtrl* m_pHexCtrl { };
		ESearchMode m_eSearchMode { };
		std::uint64_t m_u64Flags { };   //Data from SetDlgProperties.
		LISTEX::CListEx m_ListEx;
		ULONGLONG m_ullStartFrom { };   //"Start form" search offset.
		ULONGLONG m_ullRngBegin { };
		ULONGLONG m_ullRngEnd { };
		ULONGLONG m_ullStep { 1 };      //Search step (default is 1 byte).
		DWORD m_dwCount { };            //How many, or what index number.
		DWORD m_dwReplaced { };         //Replaced amount;
		DWORD m_dwLimit { 10000 };      //Maximum found search occurences.
		int m_iWrap { };                //Wrap direction: -1 = Beginning, 1 = End.
		VecSearchResult m_vecSearchRes; //Search results.
		std::vector<std::byte> m_vecSearchData;  //Data to search for.
		std::vector<std::byte> m_vecReplaceData; //Data to replace with.
		std::wstring m_wstrSearch;      //Text from "Search" box.
		std::wstring m_wstrReplace;     //Text from "Replace with..." box.
		bool m_fForward { };            //Search direction, Forward/Backward.
		bool m_fSecondMatch { false };  //First or subsequent match. 
		bool m_fFound { false };        //Found or not.
		bool m_fDoCount { true };       //Do we count matches or just print "Found".
		bool m_fReplace { false };      //Find or Find and Replace with...?
		bool m_fAll { false };          //Find/Replace one by one, or all?
		bool m_fSearchNext { false };   //Search through Next/Prev menu.
		bool m_fFreshSearch { true };
	};
}