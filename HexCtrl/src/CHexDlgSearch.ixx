/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
module;
#include <SDKDDKVer.h>
#include "../HexCtrl.h"
#include "res/HexCtrlRes.h"
#include <Windows.h>
#include <commctrl.h>
#include <intrin.h>
#include <algorithm>
#include <bit>
#include <cassert>
#include <chrono>
#include <cstring>
#include <format>
#include <limits>
#include <optional>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>
export module HEXCTRL:CHexDlgSearch;

import :HexUtility;
import :CHexDlgProgress;

namespace HEXCTRL::INTERNAL {
	class CHexDlgSearch final {
	public:
		void ClearData();
		void CreateDlg()const;
		void DestroyDlg();
		[[nodiscard]] auto GetDlgItemHandle(EHexDlgItem eItem)const -> HWND;
		[[nodiscard]] auto GetHWND()const -> HWND;
		void Initialize(IHexCtrl &HexCtrl, HINSTANCE hInstRes);
		[[nodiscard]] bool IsSearchAvail()const; //Can we do search next/prev?
		[[nodiscard]] bool PreTranslateMsg(MSG* pMsg);
		[[nodiscard]] auto ProcessMsg(const MSG& msg) -> INT_PTR;
		void SearchNextPrev(bool fForward);
		void SetDlgProperties(std::uint64_t u64Flags);
		void ShowWindow(int iCmdShow);
	private:
		enum class ESearchMode : std::uint8_t; //Forward declarations.
		enum class ESearchType : std::uint8_t;
		enum class EMenuID : std::uint16_t;
		struct SEARCHFUNCDATA;
		struct FINDRESULT;
		struct SEARCHOFFSETS;
		using PtrSearchFunc = auto(*)(const SEARCHFUNCDATA&)->FINDRESULT;
		using VecSearchResult = std::vector<ULONGLONG>;

		void AddToList(ULONGLONG ullOffset);
		void CalcMemChunks(SEARCHFUNCDATA& sfd)const;
		void ClearComboSearchType();
		void ClearList();
		void ComboFindFill(LPCWSTR pwsz);
		void ComboReplaceFill(LPCWSTR pwsz);
		[[nodiscard]] auto CreateSearchData(CHexDlgProgress* pDlgProg = nullptr)const -> SEARCHFUNCDATA;
		void FindAll();
		void FindForward();
		void FindBackward();
		[[nodiscard]] auto GetHexCtrl()const -> IHexCtrl*;
		[[nodiscard]] auto GetLastSearchOffset()const -> ULONGLONG;
		[[nodiscard]] auto GetRngStart()const -> ULONGLONG;
		[[nodiscard]] auto GetRngEnd()const -> ULONGLONG;
		[[nodiscard]] auto GetRngSize()const -> ULONGLONG;     //Size of the range to search within.
		[[nodiscard]] auto GetReplaceDataSize()const -> DWORD; //Replace vec data size.
		[[nodiscard]] auto GetReplaceSpan()const -> SpanCByte;
		[[nodiscard]] auto GetSearchDataFrom()const -> SpanCByte;
		[[nodiscard]] auto GetSearchDataTo()const -> SpanCByte;
		[[nodiscard]] auto GetSearchDataSize()const -> DWORD;  //Search vec data size.
		[[nodiscard]] auto GetSearchRngSize()const -> ULONGLONG;
		[[nodiscard]] auto GetSearchFunc(bool fFwd, bool fDlgProg)const -> PtrSearchFunc;
		template<bool fDlgProg, simd::EVecType eVecType>
		[[nodiscard]] auto GetSearchFuncFwd()const -> PtrSearchFunc;
		template<bool fDlgProg, simd::EVecType eVecType>
		[[nodiscard]] auto GetSearchFuncBack()const -> PtrSearchFunc;
		[[nodiscard]] auto GetSearchMode()const -> ESearchMode; //Get current search mode.
		[[nodiscard]] auto GetSearchModePrev()const -> ESearchMode; //Get previous search mode.
		[[nodiscard]] auto GetSearchType()const -> ESearchType; //Get current search type.
		[[nodiscard]] auto GetSentinel()const -> ULONGLONG;
		[[nodiscard]] auto GetStartFrom()const -> ULONGLONG; //Start search from.
		[[nodiscard]] auto GetStep()const -> ULONGLONG;
		[[nodiscard]] auto GetWildcard()const -> std::byte;
		void HexCtrlHighlight(const VecHexSpan& vecSel); //Highlight found occurence in the HexCtrl.
		[[nodiscard]] bool IsBigEndian()const;
		[[nodiscard]] bool IsForward()const;
		[[nodiscard]] bool IsFreshSearch()const;
		[[nodiscard]] bool IsInverted()const;
		[[nodiscard]] bool IsMatchCase()const;
		[[nodiscard]] bool IsNoEsc()const;
		[[nodiscard]] bool IsNumRangeSearch()const; //Is search in numbers range (e.g. -1:15).
		[[nodiscard]] bool IsReplace()const;
		[[nodiscard]] bool IsSelection()const;
		[[nodiscard]] bool IsSmallSearch()const;
		[[nodiscard]] bool IsWildcard()const;
		void OnButtonSearchF();
		void OnButtonSearchB();
		void OnButtonFindAll();
		void OnButtonReplace();
		void OnButtonReplaceAll();
		void OnCancel();
		void OnCheckSel();
		void OnCheckWildcard();
		void OnComboSearchModeChange();
		void OnComboSearchModeHEXBYTES();
		void OnComboSearchModeNUMBERS();
		void OnComboSearchModeSTRUCTS();
		void OnComboSearchModeTEXT();
		void OnComboSearchTypeChange();
		void OnEditWildcardChange();
		void OnOK();
		[[nodiscard]] auto PrepareParseOffsets() -> std::optional<SEARCHOFFSETS>;
		void Prepare();
		[[nodiscard]] bool PrepareSearchData();
		[[nodiscard]] bool PrepareHexBytes();
		[[nodiscard]] bool PrepareTextASCII();
		[[nodiscard]] bool PrepareTextUTF16();
		[[nodiscard]] bool PrepareTextUTF8();
		template<typename T> requires ut::TSize1248<T>
		[[nodiscard]] bool PrepareNumbers();
		[[nodiscard]] bool PrepareFILETIME();
		void ReplaceAll();
		void ResetSearch();
		void Search();
		void SetEditStartFrom(ULONGLONG ullOffset); //Start search offset edit set.
		void UpdateControlsState();
		void UpdateCueBanners();
		void UpdateTTState();
		auto WMActivate(const MSG& msg) -> INT_PTR;
		auto WMClose() -> INT_PTR;
		auto WMCommand(const MSG& msg) -> INT_PTR;
		auto WMCtlColorStatic(const MSG& msg) -> INT_PTR;
		auto WMDestroy() -> INT_PTR;
		auto WMDrawItem(const MSG& msg) -> INT_PTR;
		auto WMInitDialog(const MSG& msg) -> INT_PTR;
		auto WMMeasureItem(const MSG& msg) -> INT_PTR;
		auto WMMouseActivate(const MSG& msg) -> INT_PTR;
		auto WMNotify(const MSG& msg) -> INT_PTR;
		void WMNotifyList(NMHDR* pNMHDR);
		void WMNotifyListGetDispInfo(NMHDR* pNMHDR);
		void WMNotifyListItemChanged(NMHDR* pNMHDR);
		void WMNotifyListRClick(NMHDR* pNMHDR);
		void WMNotifyTT(NMHDR* pNMHDR);
	private:
		enum class EMemCmp : std::uint8_t {
			DATA_INT8, DATA_UINT8, DATA_INT16, DATA_UINT16, DATA_INT32, DATA_UINT32,
			DATA_INT64, DATA_UINT64, DATA_FLOAT, DATA_DOUBLE, DATA_ASCII, DATA_WCHAR
		};
		struct SEARCHTYPE { //Compile time struct for template parameters in the SearchFunc and MemCmp*.
			constexpr SEARCHTYPE() = default;
			constexpr SEARCHTYPE(EMemCmp eMemCmp, simd::EVecType eVecType, bool fDlgProg = false, bool fMatchCase = false,
				bool fWildcard = false, bool fInverted = false) :
				eMemCmp { eMemCmp }, eVecType { eVecType }, fDlgProg { fDlgProg }, fMatchCase { fMatchCase },
				fWildcard { fWildcard }, fInverted { fInverted } { }
			constexpr ~SEARCHTYPE() = default;
			EMemCmp eMemCmp { };
			simd::EVecType eVecType { };
			bool fDlgProg { false };
			bool fMatchCase { false };
			bool fWildcard { false };

			//This compile time member is used exclusively in SearchFwdVec124, and SearchBackVec1.
			//Other templated functions utilize runtime fInverted from the SEARCHFUNCDATA,
			//and must have their fInverted=false to avoid templates bloating.
			bool fInverted { false };
		};

		//Static functions.
		static void Replace(IHexCtrl* pHexCtrl, ULONGLONG ullIndex, SpanCByte spnReplace);
		template<SEARCHTYPE st> //For comparing numbers.
		[[nodiscard]] static auto __forceinline MemCmpEQNum(const std::byte* pWhere, const std::byte* pWhat)->bool;
		template<SEARCHTYPE st> //For comparing numbers ranges.
		[[nodiscard]] static auto __forceinline MemCmpEQNumRng(const std::byte* pWhere, const std::byte* pRngFrom,
			const std::byte* pRngTo, bool fBigEndian)->bool;
		template<SEARCHTYPE st> //For comparing all kind of texts.
		[[nodiscard]] static auto __forceinline MemCmpEQText(const std::byte* pWhere, const std::byte* pWhat,
			std::size_t uzSize, std::byte bWildcard)->bool;
		template<SEARCHTYPE st, typename TPred>
		[[nodiscard]] static auto SearchScalarFwd(const SEARCHFUNCDATA& sfd, TPred&& pred) -> FINDRESULT;
		template<SEARCHTYPE st>
		[[nodiscard]] static auto SearchNumFwd(const SEARCHFUNCDATA& sfd) -> FINDRESULT;
		template<SEARCHTYPE st>
		[[nodiscard]] static auto SearchNumRngFwd(const SEARCHFUNCDATA& sfd) -> FINDRESULT;
		template<SEARCHTYPE st>
		[[nodiscard]] static auto SearchTextFwd(const SEARCHFUNCDATA& sfd) -> FINDRESULT;

		//Vector functions return index within the vector of found element,
		//index greater than sizeof(vec)-1 means not found.
		template<SEARCHTYPE st>
		[[nodiscard]] static bool __forceinline MatchAnchorCandidate(const std::byte* pCandidate, const std::byte* pSearch,
			std::size_t uzSize, std::uint8_t u8FirstUpper, std::byte bLastUpper)noexcept;
		template<SEARCHTYPE st>
		[[nodiscard]] static auto SearchFwdVec124(const SEARCHFUNCDATA& sfd) -> FINDRESULT;
		template<SEARCHTYPE st>
		[[nodiscard]] static auto SearchFwdVecAnchor(const SEARCHFUNCDATA& sfd) -> FINDRESULT;

		//Backward search.
		template<SEARCHTYPE st, typename TPred>
		[[nodiscard]] static auto SearchScalarBack(const SEARCHFUNCDATA& sfd, TPred&& pred) -> FINDRESULT;
		template<SEARCHTYPE st>
		[[nodiscard]] static auto SearchNumBack(const SEARCHFUNCDATA& sfd) -> FINDRESULT;
		template<SEARCHTYPE st>
		[[nodiscard]] static auto SearchNumRngBack(const SEARCHFUNCDATA& sfd) -> FINDRESULT;
		template<SEARCHTYPE st>
		[[nodiscard]] static auto SearchTextBack(const SEARCHFUNCDATA& sfd) -> FINDRESULT;
		template<SEARCHTYPE st>
		[[nodiscard]] static auto SearchBackVec1(const SEARCHFUNCDATA& sfd) -> FINDRESULT;
		template<SEARCHTYPE st>
		[[nodiscard]] static auto SearchBackVecAnchor(const SEARCHFUNCDATA& sfd) -> FINDRESULT;
	private:
		static constexpr auto s_uSearchSizeLimit { 256U }; //Search size limit.
		static constexpr auto s_pwszWrongInput { L"Incorrect input data format." };
		HINSTANCE m_hInstRes { };
		GDIUT::CWnd m_Wnd;                 //Main window.
		GDIUT::CWnd m_WndStatResult;       //Static text "Result:".
		GDIUT::CWnd m_WndTT;               //Tooltips window.
		GDIUT::CWndCombo m_WndCmbFind;     //Combo box "Search".
		GDIUT::CWndCombo m_WndCmbReplace;  //Combo box "Replace".
		GDIUT::CWndCombo m_WndCmbMode;     //Combo box "Search mode".
		GDIUT::CWndCombo m_WndCmbType;     //Combo box "Search type".
		GDIUT::CWndBtn m_WndBtnSel;        //Check box "In selection".
		GDIUT::CWndBtn m_WndBtnWC;         //Check box "Wildcard".
		GDIUT::CWndBtn m_WndBtnInv;        //Check box "Inverted".
		GDIUT::CWndBtn m_WndBtnBE;         //Check box "Big-endian".
		GDIUT::CWndBtn m_WndBtnMC;         //Check box "Match case".
		GDIUT::CWndEdit m_WndEditStart;    //Edit box "Start from".
		GDIUT::CWndEdit m_WndEditStep;     //Edit box "Step".
		GDIUT::CWndEdit m_WndEditRngBegin; //Edit box "Range begin".
		GDIUT::CWndEdit m_WndEditRngEnd;   //Edit box "Range end".
		GDIUT::CWndEdit m_WndEditLimit;    //Edit box "Limit search hits".
		GDIUT::CWndEdit m_WndEditWC;       //Edit box "Wildcard".
		GDIUT::CMenu m_MenuList;           //Menu for the list control.
		LISTEX::CListEx m_ListEx;
		VecSearchResult m_vecSearchRes; //Search results.
		//When it's in range search (e.g. -1:15) it's a data range start. In this case the size of the m_vecSearchDataFrom
		//is always equal to the m_vecSearchDataTo. When it's a single search, it's just search data.
		std::vector<std::byte> m_vecSearchDataFrom;
		std::vector<std::byte> m_vecSearchDataTo; //When search in range it's a range end. When a single search, it's empty.
		std::vector<std::byte> m_vecReplaceData;  //Data to replace with.
		std::wstring m_wstrSearch;      //Text from "Search" box.
		std::wstring m_wstrReplace;     //Text from "Replace with..." box.
		std::wstring m_wstrTT;          //Tooltip string.
		IHexCtrl* m_pHexCtrl { };
		std::uint64_t m_u64Flags { };   //Data from SetDlgProperties.
		std::uint64_t m_u64StartFrom { }; //"Start form" search offset.
		std::uint64_t m_u64RngBegin { };
		std::uint64_t m_u64RngEnd { };
		std::uint64_t m_u64Step { 1 };  //Search step (default is 1 byte).
		DWORD m_dwCount { };            //How many, or what index number.
		DWORD m_dwReplaced { };         //Replaced amount;
		DWORD m_dwLimit { 10000 };      //Maximum found search occurences.
		int m_iWrap { };                //Wrap direction: -1 = Beginning, 1 = End.
		ESearchMode m_eSearchModePrev { }; //Previous search mode.
		bool m_fForward { };            //Search direction, Forward/Backward.
		bool m_fSecondMatch { false };  //First or subsequent match. 
		bool m_fFound { false };        //Found or not.
		bool m_fDoCount { true };       //Do we count matches or just print "Found".
		bool m_fReplace { false };      //Find or Find and Replace with...?
		bool m_fAll { false };          //Find/Replace one by one, or all?
		bool m_fSearchNext { false };   //Search through Next/Prev menu.
		bool m_fFreshSearch { true };
		bool m_fSearchNumRng { false }; //Search in numbers range (e.g. -1:15).
	};
}

using namespace HEXCTRL::INTERNAL;

enum class CHexDlgSearch::ESearchMode : std::uint8_t {
	MODE_HEXBYTES, MODE_TEXT, MODE_NUMBERS, MODE_STRUCTS
};

enum class CHexDlgSearch::ESearchType : std::uint8_t {
	HEXBYTES, TEXT_ASCII, TEXT_UTF8, TEXT_UTF16, NUM_INT8, NUM_UINT8, NUM_INT16, NUM_UINT16,
	NUM_INT32, NUM_UINT32, NUM_INT64, NUM_UINT64, NUM_FLOAT, NUM_DOUBLE, STRUCT_FILETIME
};

enum class CHexDlgSearch::EMenuID : std::uint16_t {
	IDM_SEARCH_ADDBKM = 0x8000, IDM_SEARCH_SELECTALL, IDM_SEARCH_CLEARALL
};

struct CHexDlgSearch::FINDRESULT {
	std::uint64_t u64Offset { };
	bool          fFound { false };
	bool          fCanceled { false }; //Search was canceled by pressing "Cancel".
	explicit operator bool()const { return fFound; };
};

struct CHexDlgSearch::SEARCHFUNCDATA {
	std::uint64_t u64StartFrom { };
	std::uint64_t u64RngStart { };
	std::uint64_t u64RngEnd { };
	std::uint64_t u64Step { };
	std::uint64_t u64ChunkSize { };      //Size of one chunk.
	std::uint64_t u64ChunkMaxOffset { }; //Maximum offset to start search from, in the chunk.
	CHexDlgProgress* pDlgProg { };
	IHexCtrl* pHexCtrl { };
	SpanCByte spnFindFrom;   //When search in range it's a range start. When single search, it's just a search data.
	SpanCByte spnFindTo;     //When search in range it's a range end. When single search, it's empty.
	std::byte bWildcard { }; //Wildcard symbol when in text search mode and in Hexbytes.
	bool fBigStep { };
	bool fInverted { };  //Vectorized functions use templated st.fInverted, non-vectorized use runtime fInverted.
	bool fBigEndian { }; //For numbers range search, MemCmpEQNumRng.
};

struct CHexDlgSearch::SEARCHOFFSETS {
	std::uint64_t u64RngStart { };
	std::uint64_t u64RngEnd { };
	std::uint64_t u64StartFrom { };
};

void CHexDlgSearch::ClearData() {
	if (!m_Wnd.IsWindow())
		return;

	ClearList();
}

void CHexDlgSearch::CreateDlg()const {
	//m_Wnd is set in the WMInitDialog().
	if (const auto hWnd = ::CreateDialogParamW(m_hInstRes, MAKEINTRESOURCEW(IDD_HEXCTRL_SEARCH),
		m_pHexCtrl->GetWndHandle(EHexWnd::WND_MAIN), GDIUT::DlgProc<CHexDlgSearch>, reinterpret_cast<LPARAM>(this));
		hWnd == nullptr) {
		ut::DBG_REPORT(L"CreateDialogParamW failed.");
	}
}

void CHexDlgSearch::DestroyDlg() {
	if (m_Wnd.IsWindow()) {
		m_Wnd.DestroyWindow();
	}
}

auto CHexDlgSearch::GetDlgItemHandle(EHexDlgItem eItem)const->HWND {
	if (!m_Wnd.IsWindow()) {
		return { };
	}

	using enum EHexDlgItem;
	switch (eItem) {
	case SEARCH_COMBO_FIND:
		return m_WndCmbFind;
	case SEARCH_COMBO_REPLACE:
		return m_WndCmbReplace;
	case SEARCH_EDIT_START:
		return m_WndEditStart;
	case SEARCH_EDIT_STEP:
		return m_WndEditStep;
	case SEARCH_EDIT_RNGBEG:
		return m_WndEditRngBegin;
	case SEARCH_EDIT_RNGEND:
		return m_WndEditRngEnd;
	case SEARCH_EDIT_LIMIT:
		return m_WndEditLimit;
	case SEARCH_EDIT_WILDCARD:
		return m_WndEditWC;
	default:
		return { };
	}
}

auto CHexDlgSearch::GetHWND()const->HWND {
	return m_Wnd;
}

void CHexDlgSearch::Initialize(IHexCtrl &HexCtrl, HINSTANCE hInstRes) {
	m_pHexCtrl = &HexCtrl;
	m_hInstRes = hInstRes;
}

bool CHexDlgSearch::IsSearchAvail()const {
	return m_Wnd.IsWindow() && GetHexCtrl()->IsDataSet();
}

bool CHexDlgSearch::PreTranslateMsg(MSG* pMsg) {
	return m_Wnd.IsDlgMessage(pMsg);
}

auto CHexDlgSearch::ProcessMsg(const MSG& msg)->INT_PTR {
	switch (msg.message) {
	case WM_ACTIVATE: return WMActivate(msg);
	case WM_CLOSE: return WMClose();
	case WM_COMMAND: return WMCommand(msg);
	case WM_CTLCOLORSTATIC: return WMCtlColorStatic(msg);
	case WM_DESTROY: return WMDestroy();
	case WM_DRAWITEM: return WMDrawItem(msg);
	case WM_INITDIALOG: return WMInitDialog(msg);
	case WM_MEASUREITEM: return WMMeasureItem(msg);
	case WM_MOUSEACTIVATE: return WMMouseActivate(msg);
	case WM_NOTIFY: return WMNotify(msg);
	default:
		return 0;
	}
}

void CHexDlgSearch::SearchNextPrev(bool fForward) {
	m_fForward = fForward;
	m_fReplace = false;
	m_fAll = false;
	m_fSearchNext = true;
	Prepare();
}

void CHexDlgSearch::SetDlgProperties(std::uint64_t u64Flags) {
	m_u64Flags = u64Flags;
}

void CHexDlgSearch::ShowWindow(int iCmdShow) {
	if (!m_Wnd.IsWindow()) {
		CreateDlg();
	}

	m_Wnd.ShowWindow(iCmdShow);
}


//Private methods.

void CHexDlgSearch::AddToList(ULONGLONG ullOffset) {
	int iHighlight { -1 };
	if (const auto it = std::find(m_vecSearchRes.begin(), m_vecSearchRes.end(), ullOffset);
		it == m_vecSearchRes.end()) { //Max-found search occurences.
		if (m_vecSearchRes.size() < static_cast<std::size_t>(m_dwLimit)) {
			m_vecSearchRes.emplace_back(ullOffset);
			iHighlight = static_cast<int>(m_vecSearchRes.size());
			m_ListEx.SetItemCountEx(iHighlight--);
		}
	}
	else {
		iHighlight = static_cast<int>(it - m_vecSearchRes.begin());
	}

	if (iHighlight != -1) {
		m_ListEx.SetItemState(-1, 0, LVIS_SELECTED);
		m_ListEx.SetItemState(iHighlight, LVIS_SELECTED, LVIS_SELECTED);
		m_ListEx.EnsureVisible(iHighlight, true);
	}
}

void CHexDlgSearch::CalcMemChunks(SEARCHFUNCDATA& sfd)const {
	const auto uzSizeSearch = sfd.spnFindFrom.size();
	if (sfd.u64StartFrom + uzSizeSearch > GetSentinel()) {
		sfd.u64ChunkSize = { };
		sfd.u64ChunkMaxOffset = { };
		return;
	}

	const auto u64SizeTotal = IsForward() ? (GetSentinel() - sfd.u64StartFrom)
		: ((sfd.u64StartFrom - GetRngStart()) + uzSizeSearch);
	const auto pHexCtrl = sfd.pHexCtrl;
	const auto u64Step = sfd.u64Step;
	std::uint64_t u64ChunkSize { };
	std::uint64_t u64ChunkMaxOffset { };
	bool fBigStep { false };

	if (!pHexCtrl->IsVirtual()) {
		u64ChunkSize = u64SizeTotal;
		u64ChunkMaxOffset = u64SizeTotal - uzSizeSearch;
	}
	else {
		u64ChunkSize = (std::min)(static_cast<std::uint64_t>(pHexCtrl->GetCacheSize()), u64SizeTotal);
		u64ChunkMaxOffset = u64ChunkSize - uzSizeSearch;
		if (u64Step > u64ChunkMaxOffset) { //For very big steps.
			fBigStep = true;
		}
	}

	sfd.u64ChunkSize = u64ChunkSize;
	sfd.u64ChunkMaxOffset = u64ChunkMaxOffset;
	sfd.fBigStep = fBigStep;
}

void CHexDlgSearch::ClearComboSearchType() {
	m_WndCmbType.SetRedraw(false);
	for (auto iIndex = m_WndCmbType.GetCount() - 1; iIndex >= 0; --iIndex) {
		m_WndCmbType.DeleteString(iIndex);
	}
	m_WndCmbType.SetRedraw(true);
}

void CHexDlgSearch::ClearList() {
	m_ListEx.SetItemCountEx(0);
	m_vecSearchRes.clear();
}

void CHexDlgSearch::ComboFindFill(LPCWSTR pwsz) {
	//Insert text into ComboBox only if it's not already there.
	if (!m_WndCmbFind.HasString(pwsz)) {
		if (m_WndCmbFind.GetCount() == 50) { //Keep max 50 strings in the list.
			m_WndCmbFind.DeleteString(49);
		}
		m_WndCmbFind.InsertString(0, pwsz);
	}
}

void CHexDlgSearch::ComboReplaceFill(LPCWSTR pwsz) {
	//Insert wstring into ComboBox only if it's not already presented.
	if (!m_WndCmbReplace.HasString(pwsz)) {
		if (m_WndCmbReplace.GetCount() == 50) { //Keep max 50 strings in the list.
			m_WndCmbReplace.DeleteString(49);
		}
		m_WndCmbReplace.InsertString(0, pwsz);
	}
}

auto CHexDlgSearch::CreateSearchData(CHexDlgProgress* pDlgProg)const->SEARCHFUNCDATA {
	SEARCHFUNCDATA stData { .u64StartFrom { GetStartFrom() }, .u64RngStart { GetRngStart() },
		.u64RngEnd { GetRngEnd() }, .u64Step { GetStep() }, .pDlgProg { pDlgProg }, .pHexCtrl { GetHexCtrl() },
		.spnFindFrom { GetSearchDataFrom() }, .spnFindTo { GetSearchDataTo() }, .bWildcard { GetWildcard() },
		.fInverted { IsInverted() }, .fBigEndian { IsBigEndian() }
	};

	CalcMemChunks(stData);

	return stData;
}

void CHexDlgSearch::FindAll() {
	ClearList();
	m_dwCount = 0;
	m_u64StartFrom = GetRngStart();
	const auto fSmall = IsSmallSearch();
	const auto pSearchFunc = GetSearchFunc(true, !fSmall);
	const auto u64Step = GetStep();
	const auto u64LastOffset = GetLastSearchOffset();
	const auto u32Limit = static_cast<std::uint32_t>(m_dwLimit);
	VecSearchResult vecLocalRes; //Local vector, to avoid any thread data races.
	vecLocalRes.reserve(128); //Approximate pre-reserve, to avoid many small reallocations.
	std::uint64_t u64StartFrom = m_u64StartFrom;

	const auto lmbRunFind = [&](CHexDlgProgress* pDlgProg) {
		auto stFuncData = CreateSearchData(pDlgProg);

		while (true) {
			const auto findRes = pSearchFunc(stFuncData);
			if (!findRes)
				break;

			vecLocalRes.emplace_back(findRes.u64Offset);

			if (pDlgProg != nullptr) {
				pDlgProg->SetCurrent(findRes.u64Offset);
				pDlgProg->SetCount(vecLocalRes.size());
				if (pDlgProg->IsCanceled())
					break;
			}

			if (findRes.u64Offset > u64LastOffset || u64Step > (u64LastOffset - findRes.u64Offset)
				|| vecLocalRes.size() >= u32Limit) {
				break;
			}

			u64StartFrom = stFuncData.u64StartFrom = findRes.u64Offset + u64Step;
			CalcMemChunks(stFuncData);
		}

		if (pDlgProg != nullptr) {
			pDlgProg->OnCancel();
		}
		};

	if (fSmall) {
		lmbRunFind(nullptr);
	}
	else {
		CHexDlgProgress dlgProg(L"Searching...", L"Found:", GetStartFrom(), u64LastOffset);
		std::thread thrd(lmbRunFind, &dlgProg);
		dlgProg.DoModal(m_Wnd, m_hInstRes);
		thrd.join();
	}

	m_u64StartFrom = u64StartFrom;
	m_vecSearchRes = std::move(vecLocalRes);

	if (!m_vecSearchRes.empty()) {
		m_fFound = true;
		m_dwCount = static_cast<DWORD>(m_vecSearchRes.size());
	}

	m_ListEx.SetItemCountEx(static_cast<int>(m_vecSearchRes.size()));
}

void CHexDlgSearch::FindForward() {
	FINDRESULT findRes;
	const auto lmbFind = [&]() {
		const auto pSearchFunc = GetSearchFunc(true, !IsSmallSearch());
		auto stFuncData = CreateSearchData();

		if (IsSmallSearch()) {
			findRes = pSearchFunc(stFuncData);
		}
		else {
			CHexDlgProgress dlgProg(L"Searching...", L"", GetStartFrom(), GetLastSearchOffset());
			stFuncData.pDlgProg = &dlgProg;
			const auto lmbWrapper = [&]() {
				findRes = pSearchFunc(stFuncData);
				dlgProg.OnCancel();
				};
			std::thread thrd(lmbWrapper);
			dlgProg.DoModal(m_Wnd, m_hInstRes);
			thrd.join();
		}
		};

	if (lmbFind(); findRes) {
		m_fSecondMatch = true;
		++m_dwCount;
		m_u64StartFrom = findRes.u64Offset;
	}

	if (!findRes) {
		m_iWrap = 1;
		if (m_fSecondMatch && !findRes.fCanceled) {
			m_u64StartFrom = GetRngStart(); //Starting from the beginning.
			if (lmbFind(); findRes) {
				m_fDoCount = true;
				m_dwCount = 1;
				m_u64StartFrom = findRes.u64Offset;
			}
		}
	}
	m_fFound = findRes.fFound;
}

void CHexDlgSearch::FindBackward() {
	FINDRESULT findRes;
	const auto u64SavedStart = m_u64StartFrom;

	const auto lmbFind = [&](std::uint64_t u64From) {
		m_u64StartFrom = u64From;
		const auto pSearchFunc = GetSearchFunc(false, !IsSmallSearch());
		auto stFuncData = CreateSearchData();

		if (IsSmallSearch()) {
			findRes = pSearchFunc(stFuncData);
		}
		else {
			CHexDlgProgress dlgProg(L"Searching...", L"", GetRngStart(), u64From);
			stFuncData.pDlgProg = &dlgProg;
			const auto lmbWrapper = [&]() {
				findRes = pSearchFunc(stFuncData);
				dlgProg.OnCancel();
				};
			std::thread thrd(lmbWrapper);
			dlgProg.DoModal(m_Wnd, m_hInstRes);
			thrd.join();
		}
		};

	if (!m_fSecondMatch) {
		if (lmbFind(m_u64StartFrom); findRes) {
			m_fSecondMatch = true;
			m_fDoCount = false;
			m_dwCount = 0;
			m_u64StartFrom = findRes.u64Offset;
		}
	}
	else {
		const auto u64Step = GetStep();
		if (m_u64StartFrom >= u64Step && (m_u64StartFrom - u64Step) >= GetRngStart()) {
			if (lmbFind(m_u64StartFrom - u64Step); findRes) {
				if (m_fDoCount && m_dwCount > 1) {
					--m_dwCount;
				}
				else {
					m_fDoCount = false;
				}
				m_u64StartFrom = findRes.u64Offset;
			}
		}
	}

	if (!findRes) {
		m_iWrap = -1;
		if (findRes.fCanceled) {
			m_u64StartFrom = u64SavedStart;
		}
		else {
			const auto u64LastOffset = GetLastSearchOffset();
			if (lmbFind(u64LastOffset); findRes) {
				m_fSecondMatch = true;
				m_fDoCount = false;
				m_dwCount = 0;
				m_u64StartFrom = findRes.u64Offset;
			}
			else if (findRes.fCanceled) {
				m_u64StartFrom = u64SavedStart;
			}
		}
	}

	m_fFound = findRes.fFound;
}

auto CHexDlgSearch::GetHexCtrl()const->IHexCtrl* {
	return m_pHexCtrl;
}

auto CHexDlgSearch::GetLastSearchOffset()const->ULONGLONG {
	return GetSentinel() - GetSearchDataSize();
}

auto CHexDlgSearch::GetReplaceSpan()const->SpanCByte {
	return m_vecReplaceData;
}

auto CHexDlgSearch::GetReplaceDataSize()const->DWORD {
	return static_cast<DWORD>(m_vecReplaceData.size());
}

auto CHexDlgSearch::GetRngStart()const->ULONGLONG {
	return m_u64RngBegin;
}

auto CHexDlgSearch::GetRngEnd()const->ULONGLONG {
	return m_u64RngEnd;
}

auto CHexDlgSearch::GetRngSize()const->ULONGLONG {
	return m_u64RngEnd - m_u64RngBegin + 1;
}

auto CHexDlgSearch::GetSearchDataFrom()const->SpanCByte {
	return m_vecSearchDataFrom;
}

auto CHexDlgSearch::GetSearchDataTo()const->SpanCByte {
	return m_vecSearchDataTo;
}

auto CHexDlgSearch::GetSearchDataSize()const->DWORD {
	return static_cast<DWORD>(m_vecSearchDataFrom.size());
}

auto CHexDlgSearch::GetSearchFunc(bool fFwd, bool fDlgProg)const->PtrSearchFunc {
	using enum simd::EVecType;
	switch (simd::GetVectorType()) {
	case VECTOR_128:
		return fFwd ? (fDlgProg ? GetSearchFuncFwd<true, VECTOR_128>() : GetSearchFuncFwd<false, VECTOR_128>()) :
			(fDlgProg ? GetSearchFuncBack<true, VECTOR_128>() : GetSearchFuncBack<false, VECTOR_128>());
	case VECTOR_256:
		return fFwd ? (fDlgProg ? GetSearchFuncFwd<true, VECTOR_256>() : GetSearchFuncFwd<false, VECTOR_256>()) :
			(fDlgProg ? GetSearchFuncBack<true, VECTOR_256>() : GetSearchFuncBack<false, VECTOR_256>());
	default: return nullptr;
	}
}

template<bool fDlgProg, simd::EVecType eVecType>
auto CHexDlgSearch::GetSearchFuncFwd()const->PtrSearchFunc {
	//The `fDlgProg` arg ensures that no runtime check will be performed for
	//'SEARCHFUNCDATA::pDlgProg == nullptr' at the hot path inside the SearchFunc function.
	//Scalar search functions pass VECTOR_128 unconditionally to avoid duplicate template instantiations.

	using enum ESearchType;
	using enum EMemCmp;
	constexpr auto eScalarVec = simd::EVecType::VECTOR_128;

	if (IsNumRangeSearch()) { //Special case for search in numbers range (e.g. -1:15).
		switch (GetSearchType()) {
		case NUM_INT8:   return SearchNumRngFwd<SEARCHTYPE(DATA_INT8, eScalarVec, fDlgProg)>;
		case NUM_UINT8:  return SearchNumRngFwd<SEARCHTYPE(DATA_UINT8, eScalarVec, fDlgProg)>;
		case NUM_INT16:  return SearchNumRngFwd<SEARCHTYPE(DATA_INT16, eScalarVec, fDlgProg)>;
		case NUM_UINT16: return SearchNumRngFwd<SEARCHTYPE(DATA_UINT16, eScalarVec, fDlgProg)>;
		case NUM_INT32:  return SearchNumRngFwd<SEARCHTYPE(DATA_INT32, eScalarVec, fDlgProg)>;
		case NUM_UINT32: return SearchNumRngFwd<SEARCHTYPE(DATA_UINT32, eScalarVec, fDlgProg)>;
		case NUM_INT64:  return SearchNumRngFwd<SEARCHTYPE(DATA_INT64, eScalarVec, fDlgProg)>;
		case NUM_UINT64: return SearchNumRngFwd<SEARCHTYPE(DATA_UINT64, eScalarVec, fDlgProg)>;
		case NUM_FLOAT:  return SearchNumRngFwd<SEARCHTYPE(DATA_FLOAT, eScalarVec, fDlgProg)>;
		case NUM_DOUBLE: return SearchNumRngFwd<SEARCHTYPE(DATA_DOUBLE, eScalarVec, fDlgProg)>;
		default:
			assert(false); //Should never get here.
			return nullptr;
		}
	}

	const auto fInverted = IsInverted();
	const auto fMatchCase = IsMatchCase();
	const auto fWildcard = IsWildcard();
	const auto u64Step = GetStep();
	const auto dwSearchSize = GetSearchDataSize();

	//If search step is 1 and data size is 1, 2, or 4 bytes, we vectorize it with special vector functions.
	//All other data sizes are handled by universal SearchFwdVecAnchor, which works only for sizes >= 3.
	//Vector search doesn't work for Wildcard or case-insensitive non-ASCII search cases.
	if (u64Step == 1 && !fWildcard && fMatchCase) {
		switch (dwSearchSize) {
		case 1:
			return fInverted ?
				SearchFwdVec124<SEARCHTYPE(DATA_UINT8, eVecType, fDlgProg, false, false, true)> :
				SearchFwdVec124<SEARCHTYPE(DATA_UINT8, eVecType, fDlgProg, false, false, false)>;
		case 2:
			return fInverted ?
				SearchFwdVec124<SEARCHTYPE(DATA_UINT16, eVecType, fDlgProg, false, false, true)> :
				SearchFwdVec124<SEARCHTYPE(DATA_UINT16, eVecType, fDlgProg, false, false, false)>;
		case 4:
			return fInverted ?
				SearchFwdVec124<SEARCHTYPE(DATA_UINT32, eVecType, fDlgProg, false, false, true)> :
				SearchFwdVec124<SEARCHTYPE(DATA_UINT32, eVecType, fDlgProg, false, false, false)>;
		default:
			if (!fInverted && dwSearchSize >= 3) {
				return SearchFwdVecAnchor<SEARCHTYPE(DATA_ASCII, eVecType, fDlgProg, true)>;
			}
			break;
		}
	}

	switch (GetSearchType()) {
	case HEXBYTES:
		return fWildcard ?
			SearchTextFwd<SEARCHTYPE(DATA_ASCII, eScalarVec, fDlgProg, true, true)> :
			SearchTextFwd<SEARCHTYPE(DATA_ASCII, eScalarVec, fDlgProg, true, false)>;

	case TEXT_ASCII:
		if (fMatchCase) {
			return fWildcard ?
				SearchTextFwd<SEARCHTYPE(DATA_ASCII, eScalarVec, fDlgProg, true, true)> :
				SearchTextFwd<SEARCHTYPE(DATA_ASCII, eScalarVec, fDlgProg, true, false)>;
		}
		if (fWildcard) {
			return SearchTextFwd<SEARCHTYPE(DATA_ASCII, eScalarVec, fDlgProg, false, true)>;
		}
		if (u64Step == 1 && !fInverted && dwSearchSize >= 3) {
			//Case when fMatchCase == false.
			return SearchFwdVecAnchor<SEARCHTYPE(DATA_ASCII, eVecType, fDlgProg, false)>;
		}
		return SearchTextFwd<SEARCHTYPE(DATA_ASCII, eScalarVec, fDlgProg, false, false)>;

	case TEXT_UTF8:
		//Search UTF-8 as plain chars, with Match-case = true.
		return SearchTextFwd<SEARCHTYPE(DATA_ASCII, eScalarVec, fDlgProg, true, false)>;

	case TEXT_UTF16:
		if (fMatchCase) {
			return fWildcard ?
				SearchTextFwd<SEARCHTYPE(DATA_WCHAR, eScalarVec, fDlgProg, true, true)> :
				SearchTextFwd<SEARCHTYPE(DATA_WCHAR, eScalarVec, fDlgProg, true, false)>;
		}
		return fWildcard ?
			SearchTextFwd<SEARCHTYPE(DATA_WCHAR, eScalarVec, fDlgProg, false, true)> :
			SearchTextFwd<SEARCHTYPE(DATA_WCHAR, eScalarVec, fDlgProg, false, false)>;

	//All numerics, including floats, are searched as plain unsigned data of respective size.
	case NUM_INT8:
	case NUM_UINT8:
		return SearchNumFwd<SEARCHTYPE(DATA_UINT8, eScalarVec, fDlgProg)>;
	case NUM_INT16:
	case NUM_UINT16:
		return SearchNumFwd<SEARCHTYPE(DATA_UINT16, eScalarVec, fDlgProg)>;
	case NUM_INT32:
	case NUM_UINT32:
	case NUM_FLOAT:
		return SearchNumFwd<SEARCHTYPE(DATA_UINT32, eScalarVec, fDlgProg)>;
	case NUM_INT64:
	case NUM_UINT64:
	case NUM_DOUBLE:
	case STRUCT_FILETIME:
		return SearchNumFwd<SEARCHTYPE(DATA_UINT64, eScalarVec, fDlgProg)>;
	default:
		assert(false);
		return nullptr;
	}
}

template<bool fDlgProg, simd::EVecType eVecType>
auto CHexDlgSearch::GetSearchFuncBack()const->PtrSearchFunc {
	//The `fDlgProg` arg ensures that no runtime check will be performed for
	//'SEARCHFUNCDATA::pDlgProg == nullptr' at the hot path inside the SearchFunc function.
	//Scalar search functions pass VECTOR_128 unconditionally to avoid duplicate template instantiations.

	using enum ESearchType;
	using enum EMemCmp;
	constexpr auto eScalarVec = simd::EVecType::VECTOR_128;

	if (IsNumRangeSearch()) { //Special case for search in numbers range (e.g. -1:15).
		switch (GetSearchType()) {
		case NUM_INT8:   return SearchNumRngBack<SEARCHTYPE(DATA_INT8, eScalarVec, fDlgProg)>;
		case NUM_UINT8:  return SearchNumRngBack<SEARCHTYPE(DATA_UINT8, eScalarVec, fDlgProg)>;
		case NUM_INT16:  return SearchNumRngBack<SEARCHTYPE(DATA_INT16, eScalarVec, fDlgProg)>;
		case NUM_UINT16: return SearchNumRngBack<SEARCHTYPE(DATA_UINT16, eScalarVec, fDlgProg)>;
		case NUM_INT32:  return SearchNumRngBack<SEARCHTYPE(DATA_INT32, eScalarVec, fDlgProg)>;
		case NUM_UINT32: return SearchNumRngBack<SEARCHTYPE(DATA_UINT32, eScalarVec, fDlgProg)>;
		case NUM_INT64:  return SearchNumRngBack<SEARCHTYPE(DATA_INT64, eScalarVec, fDlgProg)>;
		case NUM_UINT64: return SearchNumRngBack<SEARCHTYPE(DATA_UINT64, eScalarVec, fDlgProg)>;
		case NUM_FLOAT:  return SearchNumRngBack<SEARCHTYPE(DATA_FLOAT, eScalarVec, fDlgProg)>;
		case NUM_DOUBLE: return SearchNumRngBack<SEARCHTYPE(DATA_DOUBLE, eScalarVec, fDlgProg)>;
		default:
			assert(false); //Should never get here.
			return nullptr;
		}
	}

	const auto fInverted = IsInverted();
	const auto fMatchCase = IsMatchCase();
	const auto fWildcard = IsWildcard();
	const auto u64Step = GetStep();
	const auto dwSearchSize = GetSearchDataSize();

	//If search step is 1 and data size is 1 byte, we vectorize it with SearchBackVec1.
	//All other data sizes >= 2 are handled by universal SearchBackVecAnchor.
	//Vector search doesn't work for Wildcard or case-insensitive non-ASCII search cases.
	if (u64Step == 1 && !fWildcard && fMatchCase) {
		if (dwSearchSize == 1) {
			return fInverted ?
				SearchBackVec1<SEARCHTYPE(DATA_UINT8, eVecType, fDlgProg, false, false, true)> :
				SearchBackVec1<SEARCHTYPE(DATA_UINT8, eVecType, fDlgProg, false, false, false)>;
		}

		if (!fInverted && dwSearchSize >= 2) {
			return SearchBackVecAnchor<SEARCHTYPE(DATA_ASCII, eVecType, fDlgProg, true)>;
		}
	}

	switch (GetSearchType()) {
	case HEXBYTES:
		return fWildcard ?
			SearchTextBack<SEARCHTYPE(DATA_ASCII, eScalarVec, fDlgProg, true, true)> :
			SearchTextBack<SEARCHTYPE(DATA_ASCII, eScalarVec, fDlgProg, true, false)>;

	case TEXT_ASCII:
		if (fMatchCase) {
			return fWildcard ?
				SearchTextBack<SEARCHTYPE(DATA_ASCII, eScalarVec, fDlgProg, true, true)> :
				SearchTextBack<SEARCHTYPE(DATA_ASCII, eScalarVec, fDlgProg, true, false)>;
		}
		if (fWildcard) {
			return SearchTextBack<SEARCHTYPE(DATA_ASCII, eScalarVec, fDlgProg, false, true)>;
		}
		if (u64Step == 1 && !fInverted && dwSearchSize >= 2) {
			//Case when fMatchCase == false.
			return SearchBackVecAnchor<SEARCHTYPE(DATA_ASCII, eVecType, fDlgProg, false)>;
		}
		return SearchTextBack<SEARCHTYPE(DATA_ASCII, eScalarVec, fDlgProg, false, false)>;

	case TEXT_UTF8:
		//Search UTF-8 as plain chars, with Match-case = true.
		return SearchTextBack<SEARCHTYPE(DATA_ASCII, eScalarVec, fDlgProg, true, false)>;

	case TEXT_UTF16:
		if (fMatchCase) {
			return fWildcard ?
				SearchTextBack<SEARCHTYPE(DATA_WCHAR, eScalarVec, fDlgProg, true, true)> :
				SearchTextBack<SEARCHTYPE(DATA_WCHAR, eScalarVec, fDlgProg, true, false)>;
		}
		return fWildcard ?
			SearchTextBack<SEARCHTYPE(DATA_WCHAR, eScalarVec, fDlgProg, false, true)> :
			SearchTextBack<SEARCHTYPE(DATA_WCHAR, eScalarVec, fDlgProg, false, false)>;

	//All numerics, including floats, are searched as plain unsigned data of respective size.
	case NUM_INT8:
	case NUM_UINT8:
		return SearchNumBack<SEARCHTYPE(DATA_UINT8, eScalarVec, fDlgProg)>;
	case NUM_INT16:
	case NUM_UINT16:
		return SearchNumBack<SEARCHTYPE(DATA_UINT16, eScalarVec, fDlgProg)>;
	case NUM_INT32:
	case NUM_UINT32:
	case NUM_FLOAT:
		return SearchNumBack<SEARCHTYPE(DATA_UINT32, eScalarVec, fDlgProg)>;
	case NUM_INT64:
	case NUM_UINT64:
	case NUM_DOUBLE:
	case STRUCT_FILETIME:
		return SearchNumBack<SEARCHTYPE(DATA_UINT64, eScalarVec, fDlgProg)>;
	default:
		assert(false);
		return nullptr;
	}
}

auto CHexDlgSearch::GetSearchMode()const->CHexDlgSearch::ESearchMode {
	const auto iSel = m_WndCmbMode.GetCurSel();
	return (iSel != CB_ERR) ? static_cast<ESearchMode>(m_WndCmbMode.GetItemData(iSel)) : ESearchMode::MODE_HEXBYTES;
}

auto CHexDlgSearch::GetSearchModePrev()const->ESearchMode {
	return m_eSearchModePrev;
}

auto CHexDlgSearch::GetSearchRngSize()const->ULONGLONG {
	return IsForward() ? GetSentinel() - GetStartFrom() :
		(GetStartFrom() - GetRngStart()) + GetSearchDataSize();
}

auto CHexDlgSearch::GetSearchType()const->ESearchType {
	if (GetSearchMode() == ESearchMode::MODE_HEXBYTES) {
		return ESearchType::HEXBYTES;
	}

	const auto iSel = m_WndCmbType.GetCurSel();
	return (iSel != CB_ERR) ? static_cast<ESearchType>(m_WndCmbType.GetItemData(iSel)) : ESearchType::TEXT_ASCII;
}

auto CHexDlgSearch::GetSentinel()const->ULONGLONG {
	return m_u64RngEnd + 1; //This offset is non-dereferenceable.
}

auto CHexDlgSearch::GetStartFrom()const->ULONGLONG {
	return m_u64StartFrom;
}

auto CHexDlgSearch::GetStep()const->ULONGLONG {
	return m_u64Step;
}

auto CHexDlgSearch::GetWildcard()const->std::byte {
	if (m_WndEditWC.IsWndTextEmpty()) {
		return std::byte { '?' };
	}

	return static_cast<std::byte>(m_WndEditWC.GetWndText()[0]); //Use only the first character.
}

void CHexDlgSearch::HexCtrlHighlight(const VecHexSpan& vecSel) {
	const auto pHexCtrl = GetHexCtrl();
	pHexCtrl->SetSelection(vecSel, true, IsSelection()); //Highlight selection?

	if (!pHexCtrl->IsOffsetVisible(vecSel.back().ullOffset)) {
		pHexCtrl->GoToOffset(vecSel.back().ullOffset);
	}
}

bool CHexDlgSearch::IsBigEndian()const {
	return m_WndBtnBE.IsChecked();
}

bool CHexDlgSearch::IsForward()const {
	return m_fForward;
}

bool CHexDlgSearch::IsFreshSearch()const {
	return m_fFreshSearch;
}

bool CHexDlgSearch::IsInverted()const {
	return m_WndBtnInv.IsChecked();
}

bool CHexDlgSearch::IsMatchCase()const {
	//If window is disabled we also return true, to ensure 
	//that no lower-case conversion is ever performed anywhere.
	return !m_WndBtnMC.IsWindowEnabled() || m_WndBtnMC.IsChecked();
}

bool CHexDlgSearch::IsNoEsc()const {
	return m_u64Flags & HEXCTRL_FLAG_DLG_NOESC;
}

bool CHexDlgSearch::IsNumRangeSearch()const {
	return m_fSearchNumRng;
}

bool CHexDlgSearch::IsReplace()const {
	return m_fReplace;
}

bool CHexDlgSearch::IsSelection()const {
	return m_WndBtnSel.IsChecked();
}

bool CHexDlgSearch::IsSmallSearch()const {
	constexpr auto uSizeQuick { 1024U * 1024U * 50U }; //50MB without creating a new thread.
	return GetSearchRngSize() <= uSizeQuick;
}

bool CHexDlgSearch::IsWildcard()const {
	return m_WndBtnWC.IsWindowEnabled() && m_WndBtnWC.IsChecked();
}

void CHexDlgSearch::OnButtonSearchF() {
	m_fForward = true;
	m_fReplace = false;
	m_fAll = false;
	Prepare();
}

void CHexDlgSearch::OnButtonSearchB() {
	m_fForward = false;
	m_fReplace = false;
	m_fAll = false;
	Prepare();
}

void CHexDlgSearch::OnButtonFindAll() {
	m_fForward = true;
	m_fReplace = false;
	m_fAll = true;
	Prepare();
}

void CHexDlgSearch::OnButtonReplace() {
	m_fForward = true;
	m_fReplace = true;
	m_fAll = false;
	Prepare();
}

void CHexDlgSearch::OnButtonReplaceAll() {
	m_fForward = true;
	m_fReplace = true;
	m_fAll = true;
	Prepare();
}

void CHexDlgSearch::OnCancel() {
	if (IsNoEsc()) //Not closing Dialog on Escape key.
		return;

	WMClose();
}

void CHexDlgSearch::OnCheckSel() {
	m_WndEditStart.EnableWindow(!IsSelection());
	m_WndEditRngBegin.EnableWindow(!IsSelection());
	m_WndEditRngEnd.EnableWindow(!IsSelection());
}

void CHexDlgSearch::OnCheckWildcard() {
	m_WndEditWC.EnableWindow(IsWildcard());
}

void CHexDlgSearch::OnComboSearchModeChange() {
	UpdateControlsState();
	UpdateTTState();

	if (GetSearchMode() != GetSearchModePrev()) {
		ResetSearch();
		m_eSearchModePrev = GetSearchMode();
	}
}

void CHexDlgSearch::OnComboSearchModeHEXBYTES() {
	if (GetSearchMode() != GetSearchModePrev()) {
		ClearComboSearchType();
	}

	m_WndCmbType.EnableWindow(false);
	m_WndBtnBE.EnableWindow(false);
	m_WndBtnMC.EnableWindow(false);
	m_WndBtnWC.EnableWindow(true);
	UpdateCueBanners();
}

void CHexDlgSearch::OnComboSearchModeNUMBERS() {
	if (GetSearchMode() != GetSearchModePrev()) {
		ClearComboSearchType();
		using enum ESearchType;
		auto iIndex = m_WndCmbType.AddString(L"int8");
		m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(NUM_INT8));
		m_WndCmbType.SetCurSel(iIndex);
		iIndex = m_WndCmbType.AddString(L"uint8");
		m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(NUM_UINT8));
		iIndex = m_WndCmbType.AddString(L"int16");
		m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(NUM_INT16));
		iIndex = m_WndCmbType.AddString(L"uint16");
		m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(NUM_UINT16));
		iIndex = m_WndCmbType.AddString(L"int32");
		m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(NUM_INT32));
		iIndex = m_WndCmbType.AddString(L"uint32");
		m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(NUM_UINT32));
		iIndex = m_WndCmbType.AddString(L"int64");
		m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(NUM_INT64));
		iIndex = m_WndCmbType.AddString(L"uint64");
		m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(NUM_UINT64));
		iIndex = m_WndCmbType.AddString(L"float");
		m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(NUM_FLOAT));
		iIndex = m_WndCmbType.AddString(L"double");
		m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(NUM_DOUBLE));
	}

	m_WndCmbType.EnableWindow(true);
	m_WndBtnBE.EnableWindow(true);
	m_WndBtnMC.EnableWindow(false);
	m_WndBtnWC.EnableWindow(false);
	UpdateCueBanners();
}

void CHexDlgSearch::OnComboSearchModeSTRUCTS() {
	if (GetSearchMode() != GetSearchModePrev()) {
		ClearComboSearchType();
		using enum ESearchType;
		auto iIndex = m_WndCmbType.AddString(L"FILETIME");
		m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(STRUCT_FILETIME));
		m_WndCmbType.SetCurSel(iIndex);
	}

	m_WndCmbType.EnableWindow(true);
	m_WndBtnMC.EnableWindow(false);
	m_WndBtnBE.EnableWindow(true);
	m_WndBtnWC.EnableWindow(false);
	UpdateCueBanners();
}

void CHexDlgSearch::OnComboSearchModeTEXT() {
	using enum ESearchType;

	if (GetSearchMode() != GetSearchModePrev()) {
		ClearComboSearchType();
		auto iIndex = m_WndCmbType.AddString(L"ASCII");
		m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(TEXT_ASCII));
		m_WndCmbType.SetCurSel(iIndex);
		iIndex = m_WndCmbType.AddString(L"UTF-8");
		m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(TEXT_UTF8));
		iIndex = m_WndCmbType.AddString(L"UTF-16");
		m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(TEXT_UTF16));
	}

	m_WndCmbType.EnableWindow(true);
	m_WndBtnBE.EnableWindow(false);

	switch (GetSearchType()) {
	case TEXT_ASCII:
	case TEXT_UTF16:
		m_WndBtnMC.EnableWindow(true);
		m_WndBtnWC.EnableWindow(true);
		break;
	case TEXT_UTF8:
		m_WndBtnMC.EnableWindow(false);
		m_WndBtnWC.EnableWindow(false);
		break;
	default:
		break;
	}

	UpdateCueBanners();
}

void CHexDlgSearch::OnComboSearchTypeChange() {
	UpdateControlsState();
}

void CHexDlgSearch::OnEditWildcardChange() {
	if (m_WndEditWC.IsWndTextEmpty()) {
		return;
	}

	const auto wstrWC = m_WndEditWC.GetWndText();
	const auto wch = wstrWC[0];
	//Restrict to printable non-space ASCII characters (0x21 '!' through 0x7E '~').
	if (wch < L'!' || wch > L'~') {
		m_WndEditWC.SetWndText(L"?");
	}
}

void CHexDlgSearch::OnOK() {
	OnButtonSearchF();
}

void CHexDlgSearch::Prepare() {
	if (!m_Wnd.IsWindow() || !GetHexCtrl()->IsDataSet())
		return;

	m_vecSearchDataTo.clear();
	m_fSearchNumRng = false;

	//Normalize wildcard edit-box if empty, avoiding UI mutation inside const GetWildcard().
	if (m_WndEditWC.IsWndTextEmpty()) {
		m_WndEditWC.SetWndText(L"?");
	}

	//Validate Search and Replace text.
	const auto i32SearchSize = m_WndCmbFind.GetWndTextSize();
	if (i32SearchSize == 0 || i32SearchSize > static_cast<int>(s_uSearchSizeLimit)) {
		m_WndCmbFind.SetFocus();
		return;
	}

	const auto wstrSearch = m_WndCmbFind.GetWndText();
	if (wstrSearch != m_wstrSearch) {
		ResetSearch();
		m_wstrSearch = wstrSearch;
	}

	if (IsReplace()) {
		if (m_WndCmbReplace.IsWndTextEmpty()) {
			m_WndCmbReplace.SetFocus();
			::MessageBoxW(m_Wnd, s_pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
			return;
		}
		m_wstrReplace = m_WndCmbReplace.GetWndText();
		ComboReplaceFill(m_wstrReplace.data());
	}
	ComboFindFill(wstrSearch.data());

	//Parse and validate offsets.
	const auto optOffsets = PrepareParseOffsets();
	if (!optOffsets) {
		return;
	}

	if (m_fSecondMatch && optOffsets->u64StartFrom != m_u64StartFrom) {
		ResetSearch();
	}

	//Parse Step and Limit.
	//Clamp step to INT64_MAX / 8 to prevent signed overflow in 8x unrolled backward loops (i64Step * 8).
	constexpr auto u64MaxStep = static_cast<std::uint64_t>((std::numeric_limits<std::int64_t>::max)() / 8);
	auto u64Step = stn::StrToUInt64(m_WndEditStep.GetWndText()).value_or(1ULL);
	u64Step = std::clamp(u64Step, 1ULL, u64MaxStep);
	m_WndEditStep.SetWndText(std::format(L"{}", u64Step));

	auto u32LimitHits = stn::StrToUInt32(m_WndEditLimit.GetWndText()).value_or((std::numeric_limits<int>::max)());
	u32LimitHits = std::clamp(u32LimitHits, 1U, static_cast<std::uint32_t>((std::numeric_limits<int>::max)()));
	m_WndEditLimit.SetWndText(std::format(L"{}", u32LimitHits));

	m_u64RngBegin = optOffsets->u64RngStart;
	m_u64RngEnd = optOffsets->u64RngEnd;
	m_u64StartFrom = optOffsets->u64StartFrom;
	m_u64Step = u64Step;
	m_dwLimit = u32LimitHits;

	//Convert input strings to byte buffers.
	if (!PrepareSearchData()) {
		return;
	}

	const auto u64SearchDataSize = static_cast<ULONGLONG>(GetSearchDataSize());
	if (u64SearchDataSize == 0 || u64SearchDataSize > GetRngSize()) {
		ResetSearch();
		return;
	}

	const auto ullLastOffset = GetLastSearchOffset();
	if (!IsForward()) {
		//Clamp backward start offset so searching backward from the end of the range works.
		m_u64StartFrom = (std::min)(m_u64StartFrom, ullLastOffset);
	}
	else if (m_u64StartFrom > ullLastOffset) {
		ResetSearch();
		return;
	}

	if (IsReplace() && GetReplaceDataSize() > GetSearchDataSize()) {
		static constexpr auto wstrReplaceWarning { L"The replacing data is longer than search data.\r\n"
			"Do you want to overwrite bytes following search occurrence?\r\nChoosing No will cancel replace." };
		if (::MessageBoxW(m_Wnd, wstrReplaceWarning, L"Warning", MB_YESNO | MB_ICONQUESTION | MB_TOPMOST) == IDNO) {
			return;
		}
	}

	if (!m_fSecondMatch) {
		m_fDoCount = IsForward() && (m_u64StartFrom == GetRngStart());
		m_dwCount = 0;
	}
	Search();
	m_Wnd.SetActiveWindow();
}

auto CHexDlgSearch::PrepareParseOffsets()->std::optional<SEARCHOFFSETS> {
	const auto pHexCtrl = GetHexCtrl();
	const auto ullHexDataSize = pHexCtrl->GetDataSize();
	if (ullHexDataSize == 0) {
		return std::nullopt;
	}

	SEARCHOFFSETS offs;

	if (!IsSelection()) {
		const auto lmbParseEdit = [&](GDIUT::CWndEdit& edit, std::uint64_t ullDefault, LPCWSTR pwszErr)->std::optional<std::uint64_t> {
			if (edit.IsWndTextEmpty())
				return ullDefault;

			const auto optVal = stn::StrToUInt64(edit.GetWndText());
			if (!optVal) {
				edit.SetFocus();
				::MessageBoxW(m_Wnd, pwszErr, L"Incorrect offset", MB_ICONERROR);
				return std::nullopt;
			}
			return pHexCtrl->GetOffset(*optVal, false);
			};

		const auto optStart = lmbParseEdit(m_WndEditRngBegin, 0ULL, L"Search range start offset is incorrect.");
		if (!optStart) return std::nullopt;

		const auto optEnd = lmbParseEdit(m_WndEditRngEnd, ullHexDataSize - 1, L"Search range end offset is incorrect.");
		if (!optEnd) return std::nullopt;

		const auto optFrom = lmbParseEdit(m_WndEditStart, *optStart, L"Start offset is incorrect.");
		if (!optFrom) return std::nullopt;

		offs = { *optStart, *optEnd, *optFrom };
	}
	else {
		const auto vecSel = pHexCtrl->GetSelection();
		if (vecSel.empty() || vecSel.front().ullSize == 0) {
			return std::nullopt;
		}

		const auto& hs = vecSel.front();
		offs.u64RngStart = hs.ullOffset;
		offs.u64RngEnd = offs.u64RngStart + hs.ullSize - 1;
		assert(offs.u64RngStart <= offs.u64RngEnd);

		if (IsFreshSearch() || offs.u64RngStart != GetRngStart() || offs.u64RngEnd != GetRngEnd() || m_fAll) {
			ResetSearch();
			offs.u64StartFrom = offs.u64RngStart;
			m_fFreshSearch = false;
		}
		else {
			offs.u64StartFrom = GetStartFrom();
		}
	}

	if (offs.u64RngStart > offs.u64RngEnd || offs.u64RngStart >= ullHexDataSize || offs.u64RngEnd >= ullHexDataSize) {
		m_WndEditRngEnd.SetFocus();
		::MessageBoxW(m_Wnd, L"Search range is out of data bounds.", L"Incorrect offset", MB_ICONERROR);
		return std::nullopt;
	}

	if (offs.u64StartFrom < offs.u64RngStart || offs.u64StartFrom > offs.u64RngEnd) {
		m_WndEditStart.SetFocus();
		::MessageBoxW(m_Wnd, L"Start offset is not within the search range.", L"Incorrect offset", MB_ICONERROR);
		return std::nullopt;
	}

	return offs;
}

bool CHexDlgSearch::PrepareSearchData() {
	if (GetSearchMode() == ESearchMode::MODE_HEXBYTES) {
		return PrepareHexBytes();
	}

	using enum ESearchType;
	switch (GetSearchType()) {
	case TEXT_ASCII:      return PrepareTextASCII();
	case TEXT_UTF8:       return PrepareTextUTF8();
	case TEXT_UTF16:      return PrepareTextUTF16();
	case NUM_INT8:        return PrepareNumbers<std::int8_t>();
	case NUM_UINT8:       return PrepareNumbers<std::uint8_t>();
	case NUM_INT16:       return PrepareNumbers<std::int16_t>();
	case NUM_UINT16:      return PrepareNumbers<std::uint16_t>();
	case NUM_INT32:       return PrepareNumbers<std::int32_t>();
	case NUM_UINT32:      return PrepareNumbers<std::uint32_t>();
	case NUM_INT64:       return PrepareNumbers<std::int64_t>();
	case NUM_UINT64:      return PrepareNumbers<std::uint64_t>();
	case NUM_FLOAT:       return PrepareNumbers<float>();
	case NUM_DOUBLE:      return PrepareNumbers<double>();
	case STRUCT_FILETIME: return PrepareFILETIME();
	default:              return false;
	}
}

bool CHexDlgSearch::PrepareHexBytes() {
	static constexpr auto pwszWrongInput { L"Wrong input.\r\nAllowed characters are: 0123456789AaBbCcDdEeFf" };
	auto optData = ut::WstrToHexBytes(m_wstrSearch, IsWildcard() ? static_cast<wchar_t>(GetWildcard()) : 0);
	if (!optData) {
		::MessageBoxW(m_Wnd, pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		m_WndCmbFind.SetFocus();
		return false;
	}

	m_vecSearchDataFrom = std::move(*optData);

	if (m_fReplace) {
		auto optDataRepl = ut::WstrToHexBytes(m_wstrReplace);
		if (!optDataRepl) {
			::MessageBoxW(m_Wnd, pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
			m_WndCmbReplace.SetFocus();
			return false;
		}

		m_vecReplaceData = std::move(*optDataRepl);
	}

	return true;
}

bool CHexDlgSearch::PrepareTextASCII() {
	auto strSearch = ut::WstrToStr(m_wstrSearch, CP_ACP); //Convert to the system default Windows ANSI code page.
	if (!IsMatchCase()) { //Make ASCII letters lowercase to match MemCmpEQText and SIMD anchor folding.
		std::transform(strSearch.begin(), strSearch.end(), strSearch.begin(), [](char ch) noexcept {
			return (ch >= 'A' && ch <= 'Z') ? static_cast<char>(ch + 32) : ch; });
	}

	m_vecSearchDataFrom = ut::RangeToVecBytes(strSearch);
	m_vecReplaceData = ut::RangeToVecBytes(ut::WstrToStr(m_wstrReplace, CP_ACP));

	return true;
}

bool CHexDlgSearch::PrepareTextUTF16() {
	auto wstrSearch = m_wstrSearch;
	if (!IsMatchCase()) { //Make wchar_t ASCII letters lowercase to match MemCmpEQText and SIMD anchor folding.
		std::transform(wstrSearch.begin(), wstrSearch.end(), wstrSearch.begin(), [](wchar_t wch) noexcept {
			return (wch >= 'A' && wch <= 'Z') ? static_cast<wchar_t>(wch + 32) : wch; });
	}

	m_vecSearchDataFrom = ut::RangeToVecBytes(wstrSearch);
	m_vecReplaceData = ut::RangeToVecBytes(m_wstrReplace);

	return true;
}

bool CHexDlgSearch::PrepareTextUTF8() {
	m_vecSearchDataFrom = ut::RangeToVecBytes(ut::WstrToStr(m_wstrSearch, CP_UTF8)); //Convert to UTF-8 string.
	m_vecReplaceData = ut::RangeToVecBytes(ut::WstrToStr(m_wstrReplace, CP_UTF8));

	return true;
}

template<typename T> requires ut::TSize1248<T>
bool CHexDlgSearch::PrepareNumbers() {
	if (const auto uzRngColon = m_wstrSearch.find(L':'); uzRngColon != std::wstring::npos) { //Search in numbers range (e.g. -1:15).
		const auto wsvRngFrom = std::wstring_view { m_wstrSearch }.substr(0, uzRngColon);
		const auto wsvRngTo = std::wstring_view { m_wstrSearch }.substr(uzRngColon + 1);
		const auto optDataFrom = stn::StrToNum<T>(wsvRngFrom);
		const auto optDataTo = stn::StrToNum<T>(wsvRngTo);
		if (!optDataFrom || !optDataTo) {
			m_WndCmbFind.SetFocus();
			::MessageBoxW(m_Wnd, s_pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
			return false;
		}

		//Do NOT byte-swap range bounds, MemCmpEQNumRng byte-swaps the loaded value instead.
		const T tDataFrom = (std::min)(*optDataFrom, *optDataTo);
		const T tDataTo = (std::max)(*optDataFrom, *optDataTo);
		m_vecSearchDataFrom = ut::RangeToVecBytes(tDataFrom);
		m_vecSearchDataTo = ut::RangeToVecBytes(tDataTo);
		m_fSearchNumRng = true;

	}
	else {
		const auto optData = stn::StrToNum<T>(m_wstrSearch);
		if (!optData) {
			m_WndCmbFind.SetFocus();
			::MessageBoxW(m_Wnd, s_pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
			return false;
		}

		T tData = *optData;
		if (IsBigEndian()) {
			tData = ut::ByteSwap(tData);
		}
		m_vecSearchDataFrom = ut::RangeToVecBytes(tData);
	}

	if (m_fReplace) {
		T tDataRep { };
		const auto optDataRepl = stn::StrToNum<T>(m_wstrReplace);
		if (!optDataRepl) {
			m_WndCmbReplace.SetFocus();
			::MessageBoxW(m_Wnd, s_pwszWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		tDataRep = *optDataRepl;

		if (IsBigEndian()) {
			tDataRep = ut::ByteSwap(tDataRep);
		}
		m_vecReplaceData = ut::RangeToVecBytes(tDataRep);
	}

	return true;
}

bool CHexDlgSearch::PrepareFILETIME() {
	const auto [dwFormat, wchSepar] = GetHexCtrl()->GetDateInfo();
	const std::wstring wstrErr = L"Wrong FILETIME format.\r\nA correct format is: " + ut::GetDateFormatString(dwFormat, wchSepar);
	const auto optFTSearch = ut::StringToFileTime(m_wstrSearch, dwFormat);
	if (!optFTSearch) {
		::MessageBoxW(m_Wnd, wstrErr.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
		return false;
	}

	FILETIME ftSearch = *optFTSearch;
	FILETIME ftReplace { };

	if (m_fReplace) {
		const auto optFTReplace = ut::StringToFileTime(m_wstrReplace, dwFormat);
		if (!optFTReplace) {
			::MessageBoxW(m_Wnd, wstrErr.data(), L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		ftReplace = *optFTReplace;
	}

	if (IsBigEndian()) {
		ftSearch.dwLowDateTime = ut::ByteSwap(ftSearch.dwLowDateTime);
		ftSearch.dwHighDateTime = ut::ByteSwap(ftSearch.dwHighDateTime);
		ftReplace.dwLowDateTime = ut::ByteSwap(ftReplace.dwLowDateTime);
		ftReplace.dwHighDateTime = ut::ByteSwap(ftReplace.dwHighDateTime);
	}

	m_vecSearchDataFrom = ut::RangeToVecBytes(ftSearch);
	m_vecReplaceData = ut::RangeToVecBytes(ftReplace);

	return true;
}

void CHexDlgSearch::ReplaceAll() {
	ClearList();
	m_dwCount = 0;
	m_dwReplaced = 0;
	m_u64StartFrom = GetRngStart();
	const auto fSmall = IsSmallSearch();
	const auto pSearchFunc = GetSearchFunc(true, !fSmall);
	const auto u32SizeSearch = static_cast<std::uint32_t>(GetSearchDataSize());
	const auto u32SizeRepl = static_cast<std::uint32_t>(GetReplaceDataSize());
	const auto spnRepl = GetReplaceSpan();
	const auto u64Sentinel = GetSentinel();
	const auto u64LastOffset = GetLastSearchOffset();
	const auto u32Limit = static_cast<std::uint32_t>(m_dwLimit);
	VecSearchResult vecLocalRes; //Local vector, to avoid any thread data races.
	vecLocalRes.reserve(128); //Approximate pre-reserve, to avoid many small reallocations.
	std::uint64_t u64StartFrom = m_u64StartFrom;

	const auto lmbRunReplace = [&](CHexDlgProgress* pDlgProg) {
		auto stFuncData = CreateSearchData(pDlgProg);
		const auto u64Advance = (std::max)({ static_cast<std::uint64_t>(u32SizeRepl),
			static_cast<std::uint64_t>(u32SizeSearch), stFuncData.u64Step });

		while (true) {
			const auto findRes = pSearchFunc(stFuncData);
			if (!findRes || (findRes.u64Offset + u32SizeRepl) > u64Sentinel)
				break;

			Replace(stFuncData.pHexCtrl, findRes.u64Offset, spnRepl);
			vecLocalRes.emplace_back(findRes.u64Offset);

			if (pDlgProg != nullptr) {
				pDlgProg->SetCurrent(findRes.u64Offset);
				pDlgProg->SetCount(vecLocalRes.size());
				if (pDlgProg->IsCanceled())
					break;
			}

			if (findRes.u64Offset > u64LastOffset || u64Advance > (u64LastOffset - findRes.u64Offset)
				|| vecLocalRes.size() >= u32Limit) {
				break;
			}

			u64StartFrom = stFuncData.u64StartFrom = findRes.u64Offset + u64Advance;
			CalcMemChunks(stFuncData);
		}

		if (pDlgProg != nullptr) {
			pDlgProg->OnCancel();
		}
		};

	if (fSmall) {
		lmbRunReplace(nullptr);
	}
	else {
		CHexDlgProgress dlgProg(L"Replacing...", L"Replaced:", GetStartFrom(), u64LastOffset);
		std::thread thrd(lmbRunReplace, &dlgProg);
		dlgProg.DoModal(m_Wnd, m_hInstRes);
		thrd.join();
	}

	m_u64StartFrom = u64StartFrom;
	m_vecSearchRes = std::move(vecLocalRes);

	if (!m_vecSearchRes.empty()) {
		m_fFound = true;
		m_dwCount = m_dwReplaced = static_cast<DWORD>(m_vecSearchRes.size());
	}
	m_ListEx.SetItemCountEx(static_cast<int>(m_vecSearchRes.size()));
}

void CHexDlgSearch::ResetSearch() {
	m_fFreshSearch = true;
	m_u64StartFrom = { };
	m_dwCount = { };
	m_dwReplaced = { };
	m_iWrap = { };
	m_fSecondMatch = { false };
	m_fFound = { false };
	m_fDoCount = { true };
	m_fSearchNumRng = { false };
	ClearList();
	GetHexCtrl()->SetSelection({ }, false, true); //Clear selection highlight.
	m_WndStatResult.SetWndText(L"");
}

void CHexDlgSearch::Search() {
	const auto pHexCtrl = GetHexCtrl();
	m_fFound = false;
	bool fReplacedSingle { false };

	{ //ut::SafeSetRedraw scope.
		const ut::SafeSetRedraw srguard(pHexCtrl);
		if (m_fReplace) {
			if (m_fAll) {
				ReplaceAll();
			}
			else if (IsForward()) {
				FindForward();
				if (m_fFound) {
					const auto dwSizeRepl = GetReplaceDataSize();
					if (GetStartFrom() + dwSizeRepl <= GetSentinel()) {
						Replace(pHexCtrl, GetStartFrom(), GetReplaceSpan());
						++m_dwReplaced;
						fReplacedSingle = true;
					}
				}
			}
		}
		else {
			if (m_fAll) {
				FindAll();
			}
			else if (IsForward()) {
				m_u64StartFrom += m_fSecondMatch ? GetStep() : 0;
				FindForward();
			}
			else {
				FindBackward();
			}
		}
	}

	std::wstring wstrInfo;
	if (m_fFound) {
		if (m_fAll) {
			if (m_fReplace) {
				wstrInfo = std::format(ut::GetLocale(), L"{:L} occurrence(s) replaced.", m_dwReplaced);
				m_dwReplaced = 0;
				pHexCtrl->Redraw(); //Redraw after SafeSetRedraw has restored drawing.
			}
			else {
				wstrInfo = std::format(ut::GetLocale(), L"Found {:L} occurrences.", m_dwCount);
				m_dwCount = 0;
			}
		}
		else {
			wstrInfo = m_fDoCount ? std::format(ut::GetLocale(), L"Found occurrence № {:L} from the search beginning.", m_dwCount)
				: L"Search found occurrence.";
			const auto ullMatchOffset = GetStartFrom();
			const auto dwMatchSize = fReplacedSingle ? GetReplaceDataSize() : GetSearchDataSize();
			HexCtrlHighlight({ { ullMatchOffset, dwMatchSize } });
			AddToList(ullMatchOffset);

			auto ullEditOffset = ullMatchOffset;
			if (m_fReplace) { //Advance past replaced/searched bytes and prevent double-stepping on next Find.
				const auto ullAdvance = (std::max)({ static_cast<std::uint64_t>(dwMatchSize),
					static_cast<std::uint64_t>(GetSearchDataSize()), GetStep() });
				const auto ullLastOffset = GetLastSearchOffset();

				//Guard against unsigned overflow and check if we passed the last searchable offset.
				if (ullMatchOffset > ullLastOffset || ullAdvance > (ullLastOffset - ullMatchOffset)) {
					m_u64StartFrom = GetRngStart(); //Wrap cleanly to the start of the search range.
					ullEditOffset = m_u64StartFrom;
				}
				else {
					m_u64StartFrom = ullMatchOffset + ullAdvance;
					ullEditOffset = m_u64StartFrom;
				}
				m_fSecondMatch = false;
			}

			SetEditStartFrom(pHexCtrl->GetOffset(ullEditOffset, true));
		}
	}
	else {
		wstrInfo = (m_iWrap == -1) ? L"Didn't find any occurrence, the beginning is reached."
			: L"Didn't find any occurrence, the end is reached.";

		ResetSearch();
	}

	m_WndStatResult.SetWndText(wstrInfo);

	if (m_fSearchNext) {
		m_fSearchNext = false;
	}
	else {
		m_Wnd.SetForegroundWindow();
		m_Wnd.SetFocus();
	}
}

void CHexDlgSearch::SetEditStartFrom(ULONGLONG ullOffset) {
	m_WndEditStart.SetWndText(std::format(L"0x{:X}", ullOffset));
}

void CHexDlgSearch::UpdateControlsState() {
	const auto pHexCtrl = GetHexCtrl();
	if (!pHexCtrl->IsCreated() || !pHexCtrl->IsDataSet())
		return;

	using enum ESearchMode;
	switch (GetSearchMode()) {
	case MODE_HEXBYTES:
		OnComboSearchModeHEXBYTES();
		break;
	case MODE_TEXT:
		OnComboSearchModeTEXT();
		break;
	case MODE_NUMBERS:
		OnComboSearchModeNUMBERS();
		break;
	case MODE_STRUCTS:
		OnComboSearchModeSTRUCTS();
		break;
	default:
		break;
	}

	const auto fMutable = pHexCtrl->IsMutable();
	const auto fSearchEnabled = !m_WndCmbFind.IsWndTextEmpty();
	const auto fReplaceEnabled = fMutable && fSearchEnabled && !m_WndCmbReplace.IsWndTextEmpty();
	m_WndCmbReplace.EnableWindow(fMutable);
	m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_SEARCHF).EnableWindow(fSearchEnabled);
	m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_SEARCHB).EnableWindow(fSearchEnabled);
	m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_FINDALL).EnableWindow(fSearchEnabled);
	m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_REPL).EnableWindow(fReplaceEnabled);
	m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_BTN_REPLALL).EnableWindow(fReplaceEnabled);
	m_WndEditWC.EnableWindow(IsWildcard());
}

void CHexDlgSearch::UpdateCueBanners() {
	using enum ESearchType;
	switch (GetSearchType()) {
	case STRUCT_FILETIME:
	{
		const auto [dwFormat, wchSepar] = GetHexCtrl()->GetDateInfo();
		const auto wstr = ut::GetDateFormatString(dwFormat, wchSepar);
		m_WndCmbFind.SetCueBanner(wstr);
		m_WndCmbReplace.SetCueBanner(wstr);
	}
	break;
	default:
		m_WndCmbFind.SetCueBanner(L"");
		m_WndCmbReplace.SetCueBanner(L"");
		break;
	}
}

void CHexDlgSearch::UpdateTTState() {
	const TTTOOLINFOW ti { .cbSize { sizeof(TTTOOLINFOW) }, .uFlags { TTF_IDISHWND | TTF_SUBCLASS }, .hwnd { m_Wnd },
		.uId { reinterpret_cast<UINT_PTR>(m_WndCmbFind.GetComboBoxInfo().hwndItem) }, .lpszText { LPSTR_TEXTCALLBACKW } };
	m_WndTT.SendMsg(GetSearchMode() == ESearchMode::MODE_NUMBERS ? TTM_ADDTOOLW : TTM_DELTOOLW, 0, reinterpret_cast<LPARAM>(&ti));
}

auto CHexDlgSearch::WMActivate(const MSG& msg)->INT_PTR {
	if (const auto pHex = GetHexCtrl();
		pHex != nullptr && pHex->IsCreated() && pHex->IsDataSet() && LOWORD(msg.wParam) == WA_ACTIVE) {
		UpdateControlsState();
	}

	return 0;
}

auto CHexDlgSearch::WMClose()->INT_PTR {
	ShowWindow(SW_HIDE);
	return TRUE;
}

auto CHexDlgSearch::WMCommand(const MSG& msg)->INT_PTR {
	const auto uCtrlID = LOWORD(msg.wParam); //Control ID or menu ID.
	const auto uCode = HIWORD(msg.wParam);   //Control code, zero for menu.
	const auto hWndCtrl = reinterpret_cast<HWND>(msg.lParam); //Control HWND, zero for menu.

	//IDOK and IDCANCEL don't have HWND in lParam, if send as result of
	//IsDialogMessage and no button with such ID presents in the dialog.
	if (hWndCtrl != nullptr || uCtrlID == IDOK || uCtrlID == IDCANCEL) { //Notifications from controls.
		switch (uCtrlID) {
		case IDOK: OnOK(); break;
		case IDCANCEL: OnCancel(); break;
		case IDC_HEXCTRL_SEARCH_BTN_SEARCHF: OnButtonSearchF(); break;
		case IDC_HEXCTRL_SEARCH_BTN_SEARCHB: OnButtonSearchB(); break;
		case IDC_HEXCTRL_SEARCH_BTN_FINDALL: OnButtonFindAll(); break;
		case IDC_HEXCTRL_SEARCH_BTN_REPL: OnButtonReplace(); break;
		case IDC_HEXCTRL_SEARCH_BTN_REPLALL: OnButtonReplaceAll(); break;
		case IDC_HEXCTRL_SEARCH_CHK_SEL: OnCheckSel(); break;
		case IDC_HEXCTRL_SEARCH_CHK_WC: OnCheckWildcard(); break;
		case IDC_HEXCTRL_SEARCH_COMBO_MODE: if (uCode == CBN_SELCHANGE) { OnComboSearchModeChange(); } break;
		case IDC_HEXCTRL_SEARCH_COMBO_TYPE: if (uCode == CBN_SELCHANGE) { OnComboSearchTypeChange(); } break;
		case IDC_HEXCTRL_SEARCH_COMBO_FIND:
		case IDC_HEXCTRL_SEARCH_COMBO_REPL:
			if (uCode == CBN_EDITUPDATE) { UpdateControlsState(); } break;
		case IDC_HEXCTRL_SEARCH_EDIT_WC:
			if (uCode == EN_CHANGE) { OnEditWildcardChange(); } break;
		default: return FALSE;
		}
	}
	else {
		switch (static_cast<EMenuID>(uCtrlID)) {
		case EMenuID::IDM_SEARCH_ADDBKM:
		{
			int iItem { -1 };
			for (auto i = 0UL; i < m_ListEx.GetSelectedCount(); ++i) {
				iItem = m_ListEx.GetNextItem(iItem, LVNI_SELECTED);
				if (iItem < 0) break;
				const HEXBKM hbs { .vecSpan { HEXSPAN { m_vecSearchRes.at(static_cast<std::size_t>(iItem)),
					m_fReplace ? GetReplaceDataSize() : GetSearchDataSize() } }, .wstrDesc { m_wstrSearch },
					.stClr { GetHexCtrl()->GetColors().clrBkBkm, GetHexCtrl()->GetColors().clrFontBkm } };
				GetHexCtrl()->GetBookmarks()->AddBkm(hbs);
			}
			GetHexCtrl()->Redraw();
		}
		break;
		case EMenuID::IDM_SEARCH_SELECTALL: m_ListEx.SetItemState(-1, LVIS_SELECTED, LVIS_SELECTED); break;
		case EMenuID::IDM_SEARCH_CLEARALL:
			ClearList();
			m_fSecondMatch = false; //To be able to search from the zero offset.
			break;
		default: return FALSE;
		}
	}

	return TRUE;
}

auto CHexDlgSearch::WMCtlColorStatic(const MSG& msg)->INT_PTR {
	if (const auto hWndFrom = reinterpret_cast<HWND>(msg.lParam); hWndFrom == m_WndStatResult) {
		const auto hDC = reinterpret_cast<HDC>(msg.wParam);
		::SetTextColor(hDC, m_fFound ? RGB(0, 200, 0) : RGB(200, 0, 0));
		::SetBkColor(hDC, ::GetSysColor(COLOR_3DFACE));
		return reinterpret_cast<INT_PTR>(::GetSysColorBrush(COLOR_3DFACE));
	}

	return FALSE; //Default handler.
}

auto CHexDlgSearch::WMDestroy()->INT_PTR {
	m_MenuList.DestroyMenu();
	m_vecSearchRes.clear();
	m_vecSearchDataFrom.clear();
	m_vecSearchDataTo.clear();
	m_vecReplaceData.clear();
	m_wstrSearch.clear();
	m_wstrReplace.clear();
	m_u64Flags = { };
	m_pHexCtrl = nullptr;

	return TRUE;
}

auto CHexDlgSearch::WMDrawItem(const MSG& msg)->INT_PTR {
	const auto pDIS = reinterpret_cast<LPDRAWITEMSTRUCT>(msg.lParam);
	if (pDIS->CtlID == static_cast<UINT>(IDC_HEXCTRL_SEARCH_LIST)) {
		m_ListEx.DrawItem(pDIS);
		return TRUE;
	}

	return FALSE;
}

auto CHexDlgSearch::WMInitDialog(const MSG& msg)->INT_PTR {
	m_Wnd.Attach(msg.hwnd);
	m_WndStatResult.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_STAT_RESULT));
	m_WndCmbFind.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_COMBO_FIND));
	m_WndCmbReplace.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_COMBO_REPL));
	m_WndCmbMode.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_COMBO_MODE));
	m_WndCmbType.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_COMBO_TYPE));
	m_WndBtnSel.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_CHK_SEL));
	m_WndBtnWC.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_CHK_WC));
	m_WndBtnInv.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_CHK_INV));
	m_WndBtnBE.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_CHK_BE));
	m_WndBtnMC.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_CHK_MC));
	m_WndEditStart.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_EDIT_START));
	m_WndEditStep.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_EDIT_STEP));
	m_WndEditRngBegin.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_EDIT_RNGBEG));
	m_WndEditRngEnd.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_EDIT_RNGEND));
	m_WndEditLimit.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_EDIT_LIMIT));
	m_WndEditWC.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_SEARCH_EDIT_WC));

	m_WndCmbFind.LimitText(s_uSearchSizeLimit);
	m_WndCmbReplace.LimitText(s_uSearchSizeLimit);
	m_WndEditWC.SetLimitText(1);
	m_WndEditWC.SetWndText(L"?");

	using enum ESearchMode;
	auto iIndex = m_WndCmbMode.AddString(L"Hex Bytes");
	m_WndCmbMode.SetItemData(iIndex, static_cast<DWORD_PTR>(MODE_HEXBYTES));
	m_WndCmbMode.SetCurSel(iIndex);
	iIndex = m_WndCmbMode.AddString(L"Text");
	m_WndCmbMode.SetItemData(iIndex, static_cast<DWORD_PTR>(MODE_TEXT));
	iIndex = m_WndCmbMode.AddString(L"Numbers");
	m_WndCmbMode.SetItemData(iIndex, static_cast<DWORD_PTR>(MODE_NUMBERS));
	iIndex = m_WndCmbMode.AddString(L"Structs");
	m_WndCmbMode.SetItemData(iIndex, static_cast<DWORD_PTR>(MODE_STRUCTS));

	m_ListEx.Create({ .hWndParent { m_Wnd }, .uID { IDC_HEXCTRL_SEARCH_LIST }, .flSizeFontList { 10.F },
		.flSizeFontHdr { 10.F }, .fDialogCtrl { true } });
	m_ListEx.SetExtendedStyle(LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT);
	const auto flDPIScale = GDIUT::GetDPIScaleForHWND(m_ListEx);
	m_ListEx.InsertColumn(0, L"№", LVCFMT_LEFT, std::lround(50 * flDPIScale));
	m_ListEx.InsertColumn(1, L"Offset", LVCFMT_LEFT, std::lround(455 * flDPIScale));

	m_MenuList.CreatePopupMenu();
	m_MenuList.AppendString(static_cast<UINT_PTR>(EMenuID::IDM_SEARCH_ADDBKM), L"Add bookmark(s)");
	m_MenuList.AppendString(static_cast<UINT_PTR>(EMenuID::IDM_SEARCH_SELECTALL), L"Select All");
	m_MenuList.AppendSepar();
	m_MenuList.AppendString(static_cast<UINT_PTR>(EMenuID::IDM_SEARCH_CLEARALL), L"Clear All");

	m_WndEditStep.SetWndText(std::format(L"{}", GetStep())); //"Step" edit-box text.
	m_WndEditLimit.SetWndText(std::format(L"{}", m_dwLimit)); //"Limit search hits" edit-box text.
	m_WndEditRngBegin.SetCueBanner(L"range begin");
	m_WndEditRngEnd.SetCueBanner(L"range end");

	m_WndTT = ::CreateWindowExW(0, TOOLTIPS_CLASSW, nullptr, WS_POPUP | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_Wnd, nullptr, nullptr, nullptr);
	m_WndTT.SendMsg(TTM_SETDELAYTIME, TTDT_AUTOPOP, static_cast<LPARAM>(LOWORD(0x7FFF)));
	m_WndTT.SendMsg(TTM_SETMAXTIPWIDTH, 0, 1000);

	TTTOOLINFOW ti { .cbSize { sizeof(TTTOOLINFOW) }, .uFlags { TTF_IDISHWND | TTF_SUBCLASS }, .hwnd { m_Wnd } };

	ti.uId = reinterpret_cast<UINT_PTR>(m_WndBtnWC.GetHWND()); //"Wildcard" check-box tooltip.
	ti.lpszText = LPSTR_TEXTCALLBACKW; //Text is obtained from WMNotifyTT.
	m_WndTT.SendMsg(TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&ti));

	ti.uId = reinterpret_cast<UINT_PTR>(m_WndBtnInv.GetHWND()); //"Inverted" check-box tooltip.
	ti.lpszText = const_cast<LPWSTR>(L"Search for inverted results.\r\nIt's anything that does NOT match the search conditions.");
	m_WndTT.SendMsg(TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&ti));

	ti.uId = reinterpret_cast<UINT_PTR>(m_WndEditStart.GetHWND()); //"Start from:" edit-box tooltip.
	ti.lpszText = const_cast<LPWSTR>(L"Starting search offset within the search range.");
	m_WndTT.SendMsg(TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&ti));

	ti.uId = reinterpret_cast<UINT_PTR>(m_WndEditRngEnd.GetHWND()); //"Search range: (end)" edit-box tooltip.
	ti.lpszText = const_cast<LPWSTR>(L"Last offset for a search, or empty for the whole data range.");
	m_WndTT.SendMsg(TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&ti));

	UpdateControlsState();
	UpdateTTState();

	return TRUE;
}

auto CHexDlgSearch::WMMeasureItem(const MSG& msg)->INT_PTR {
	const auto pMIS = reinterpret_cast<LPMEASUREITEMSTRUCT>(msg.lParam);
	if (pMIS->CtlID == static_cast<UINT>(IDC_HEXCTRL_SEARCH_LIST)) {
		m_ListEx.MeasureItem(pMIS);
		return TRUE;
	}

	return FALSE;
}

auto CHexDlgSearch::WMMouseActivate([[maybe_unused]] const MSG& msg)->INT_PTR {
	if (const auto pHex = GetHexCtrl(); pHex != nullptr && pHex->IsCreated() && pHex->IsDataSet()) {
		UpdateControlsState();
	}

	return MA_ACTIVATE;
}

auto CHexDlgSearch::WMNotify(const MSG& msg)->INT_PTR {
	const auto pNMHDR = reinterpret_cast<NMHDR*>(msg.lParam);

	if (pNMHDR->code == TTN_GETDISPINFOW) { //Tooltips notifications.
		WMNotifyTT(pNMHDR);
	}
	else {
		switch (pNMHDR->idFrom) {
		case IDC_HEXCTRL_SEARCH_LIST: WMNotifyList(pNMHDR);
			break;
		default: break;
		}
	}

	return TRUE;
}

void CHexDlgSearch::WMNotifyList(NMHDR* pNMHDR) {
	switch (pNMHDR->code) {
	case LVN_GETDISPINFOW: WMNotifyListGetDispInfo(pNMHDR); break;
	case LVN_ITEMCHANGED: WMNotifyListItemChanged(pNMHDR); break;
	case NM_RCLICK: WMNotifyListRClick(pNMHDR); break;
	default: break;
	}
}

void CHexDlgSearch::WMNotifyListGetDispInfo(NMHDR* pNMHDR) {
	const auto* const pDispInfo = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
	const auto* const pItem = &pDispInfo->item;
	if (((pItem->mask & LVIF_TEXT) == 0) || pItem->cchTextMax <= 0) {
		return;
	}

	const auto iItem = static_cast<std::size_t>(pItem->iItem);
	switch (pItem->iSubItem) {
	case 0: //Index value.
		*std::format_to_n(pItem->pszText, pItem->cchTextMax - 1, L"{}", iItem + 1).out = L'\0';
		break;
	case 1: //Offset.
		*std::format_to_n(pItem->pszText, pItem->cchTextMax - 1, L"0x{:X}",
			GetHexCtrl()->GetOffset(m_vecSearchRes[iItem], true)).out = L'\0';
		break;
	default:
		break;
	}
}

void CHexDlgSearch::WMNotifyListItemChanged(NMHDR* pNMHDR) {
	const auto* const pLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if (pLV->iSubItem < 0 || !(pLV->uChanged & LVIF_STATE) || m_vecSearchRes.empty()) {
		return;
	}

	//Only react when the LVIS_SELECTED state actually changed.
	if (!((pLV->uNewState ^ pLV->uOldState) & LVIS_SELECTED)) {
		return;
	}

	const auto uSelCount = m_ListEx.GetSelectedCount();
	if (uSelCount == 0) {
		GetHexCtrl()->SetSelection({ }, false, true);
		return;
	}

	const auto dwMatchSize = m_fReplace ? GetReplaceDataSize() : GetSearchDataSize();
	VecHexSpan vecSpan;
	std::uint64_t u64ActiveOffset { };

	//Virtual list controls send iItem == -1 when all items are selected at once (e.g. Select All).
	if (pLV->iItem == -1) {
		vecSpan.reserve(m_vecSearchRes.size());
		for (const auto ullOffset : m_vecSearchRes) {
			vecSpan.emplace_back(ullOffset, dwMatchSize);
		}
		u64ActiveOffset = m_vecSearchRes.back();
	}
	else {
		const auto uzItem = static_cast<std::size_t>(pLV->iItem);
		if (uzItem >= m_vecSearchRes.size()) {
			return;
		}

		vecSpan.reserve(uSelCount);
		const bool fItemSelected = (pLV->uNewState & LVIS_SELECTED) != 0;

		int iItem = -1;
		for (auto i = 0UL; i < uSelCount; ++i) {
			iItem = m_ListEx.GetNextItem(iItem, LVNI_SELECTED);
			if (iItem < 0 || static_cast<std::size_t>(iItem) >= m_vecSearchRes.size()) {
				break;
			}

			//If the clicked item was selected, defer adding it so it appears last in vecSpan.
			if (!fItemSelected || pLV->iItem != iItem) {
				vecSpan.emplace_back(m_vecSearchRes[static_cast<std::size_t>(iItem)], dwMatchSize);
			}
		}

		if (fItemSelected) {
			u64ActiveOffset = m_vecSearchRes[uzItem];
			vecSpan.emplace_back(u64ActiveOffset, dwMatchSize);
		}
		else if (!vecSpan.empty()) {
			u64ActiveOffset = vecSpan.back().ullOffset;
		}
		else {
			return;
		}
	}

	HexCtrlHighlight(vecSpan);
	SetEditStartFrom(GetHexCtrl()->GetOffset(u64ActiveOffset, true)); //Show virtual offset.
	m_u64StartFrom = u64ActiveOffset;
	m_fSecondMatch = true;

	//When item clicked mid-list and then search continues, the m_dwCount
	//will recount already matched entries, so just disabling it here.
	m_fDoCount = false;
}

void CHexDlgSearch::WMNotifyListRClick([[maybe_unused]] NMHDR* pNMHDR) {
	const auto fEnabled { m_ListEx.GetItemCount() > 0 };
	m_MenuList.EnableItem(static_cast<UINT>(EMenuID::IDM_SEARCH_ADDBKM), fEnabled);
	m_MenuList.EnableItem(static_cast<UINT>(EMenuID::IDM_SEARCH_SELECTALL), fEnabled);
	m_MenuList.EnableItem(static_cast<UINT>(EMenuID::IDM_SEARCH_CLEARALL), fEnabled);

	POINT pt;
	::GetCursorPos(&pt);
	m_MenuList.TrackPopupMenu(pt.x, pt.y, m_Wnd);
}

void CHexDlgSearch::WMNotifyTT(NMHDR* pNMHDR) {
	const auto pTTDI = reinterpret_cast<NMTTDISPINFOW*>(pNMHDR);

	if (pNMHDR->idFrom == reinterpret_cast<UINT_PTR>(m_WndBtnWC.GetHWND())) { //Wildcard check-box tooltip.
		const auto wchWildcard = static_cast<wchar_t>(GetWildcard());
		m_wstrTT = std::format(L"Use a wildcard ASCII character '{0}' to match any letter in text search,\r\n"
				L"or double wildcard '{0}{0}' to match any byte in Hex Bytes search.\r\n"
				L"Example:\r\n"
				L"  Hex Bytes: 11{0}{0}11 will match 112211, 113311, 114511, etc...\r\n"
				L"  Text: sa{0}{0}le will match sample, saMPle, saMple, saddle, etc...", wchWildcard);
		pTTDI->lpszText = m_wstrTT.data();
	}
	else if (pNMHDR->idFrom == reinterpret_cast<UINT_PTR>(m_WndCmbFind.GetComboBoxInfo().hwndItem)) {
		//"Find" combo-box tooltip.
		pTTDI->lpszText = const_cast<LPWSTR>(L"To search for numbers range, use a colon (e.g. -1:15).");
	}
}


//Static functions.

void CHexDlgSearch::Replace(IHexCtrl* pHexCtrl, ULONGLONG ullIndex, SpanCByte spnReplace) {
	pHexCtrl->ModifyData({ .eModifyMode { EHexModifyMode::MODIFY_ONCE }, .spnData { spnReplace },
		.vecSpan { { ullIndex, spnReplace.size() } } });
}

template<CHexDlgSearch::SEARCHTYPE st>
bool CHexDlgSearch::MemCmpEQNum(const std::byte* pWhere, const std::byte* pWhat) {
	using enum EMemCmp;
	if constexpr (st.eMemCmp == DATA_UINT8) {
		return *pWhere == *pWhat;
	}
	else if constexpr (st.eMemCmp == DATA_UINT16) {
		return ut::LoadUnaligned<std::uint16_t>(pWhere) == ut::LoadUnaligned<std::uint16_t>(pWhat);
	}
	else if constexpr (st.eMemCmp == DATA_UINT32) {
		return ut::LoadUnaligned<std::uint32_t>(pWhere) == ut::LoadUnaligned<std::uint32_t>(pWhat);
	}
	else if constexpr (st.eMemCmp == DATA_UINT64) {
		return ut::LoadUnaligned<std::uint64_t>(pWhere) == ut::LoadUnaligned<std::uint64_t>(pWhat);
	}
	else { static_assert(false, "Unsupported EMemCmp"); }
}

template<CHexDlgSearch::SEARCHTYPE st>
auto CHexDlgSearch::MemCmpEQNumRng(const std::byte* pWhere, const std::byte* pRngFrom,
	const std::byte* pRngTo, bool fBigEndian)->bool {
	using enum EMemCmp;
	const auto lmbInRange = [fBigEndian]<typename T>(const std::byte * pW, const std::byte * pF, const std::byte * pT)noexcept->bool {
		auto tWhere = ut::LoadUnaligned<T>(pW);
		if constexpr (sizeof(T) > 1) {
			if (fBigEndian) {
				tWhere = ut::ByteSwap(tWhere);
			}
		}
		const auto tFrom = ut::LoadUnaligned<T>(pF);
		const auto tTo = ut::LoadUnaligned<T>(pT);
		return tWhere >= tFrom && tWhere <= tTo;
	};

	if constexpr (st.eMemCmp == DATA_INT8) {
		return lmbInRange.template operator() < std::int8_t > (pWhere, pRngFrom, pRngTo);
	}
	else if constexpr (st.eMemCmp == DATA_UINT8) {
		return lmbInRange.template operator() < std::uint8_t > (pWhere, pRngFrom, pRngTo);
	}
	else if constexpr (st.eMemCmp == DATA_INT16) {
		return lmbInRange.template operator() < std::int16_t > (pWhere, pRngFrom, pRngTo);
	}
	else if constexpr (st.eMemCmp == DATA_UINT16) {
		return lmbInRange.template operator() < std::uint16_t > (pWhere, pRngFrom, pRngTo);
	}
	else if constexpr (st.eMemCmp == DATA_INT32) {
		return lmbInRange.template operator() < std::int32_t > (pWhere, pRngFrom, pRngTo);
	}
	else if constexpr (st.eMemCmp == DATA_UINT32) {
		return lmbInRange.template operator() < std::uint32_t > (pWhere, pRngFrom, pRngTo);
	}
	else if constexpr (st.eMemCmp == DATA_INT64) {
		return lmbInRange.template operator() < std::int64_t > (pWhere, pRngFrom, pRngTo);
	}
	else if constexpr (st.eMemCmp == DATA_UINT64) {
		return lmbInRange.template operator() < std::uint64_t > (pWhere, pRngFrom, pRngTo);
	}
	else if constexpr (st.eMemCmp == DATA_FLOAT) {
		return lmbInRange.template operator() < float > (pWhere, pRngFrom, pRngTo);
	}
	else if constexpr (st.eMemCmp == DATA_DOUBLE) {
		return lmbInRange.template operator() < double > (pWhere, pRngFrom, pRngTo);
	}
	else { static_assert(false, "Unsupported EMemCmp"); }
}

template<CHexDlgSearch::SEARCHTYPE st>
auto CHexDlgSearch::MemCmpEQText(const std::byte* pWhere, const std::byte* pWhat, std::size_t uzSize, std::byte bWildcard)->bool {
	using enum EMemCmp;
	if constexpr (st.eMemCmp == DATA_ASCII) {
		for (std::size_t i { 0 }; i < uzSize; ++i, ++pWhere, ++pWhat) {
			if constexpr (st.fWildcard) {
				if (*pWhat == bWildcard) //Checking for wildcard match.
					continue;
			}

			if constexpr (!st.fMatchCase) {
				auto ch = static_cast<char>(*pWhere);
				if (ch >= 'A' && ch <= 'Z') { //If it's a capital letter.
					ch += 32; //Lowering this letter ('a' - 'A' = 32).
				}

				if (ch != static_cast<char>(*pWhat))
					return false;
			}
			else {
				if (*pWhere != *pWhat)
					return false;
			}
		}
		return true;
	}
	else if constexpr (st.eMemCmp == DATA_WCHAR) {
		for (std::size_t i { 0 }; i < (uzSize / sizeof(wchar_t)); ++i, pWhere += sizeof(wchar_t), pWhat += sizeof(wchar_t)) {
			const auto wchWhat = ut::LoadUnaligned<wchar_t>(pWhat);
			if constexpr (st.fWildcard) {
				if (wchWhat == static_cast<wchar_t>(bWildcard))
					continue;
			}
			auto wch = ut::LoadUnaligned<wchar_t>(pWhere);
			if constexpr (!st.fMatchCase) {
				if (wch >= L'A' && wch <= L'Z') {
					wch += 32;
				}
			}
			if (wch != wchWhat)
				return false;
		}
		return true;
	}
	else { static_assert(false, "Unsupported EMemCmp"); }
}

template<CHexDlgSearch::SEARCHTYPE st, typename TPred>
auto CHexDlgSearch::SearchScalarFwd(const SEARCHFUNCDATA& sfd, TPred&& pred)->FINDRESULT {
	if (sfd.u64ChunkSize == 0) {
		return { };
	}

	//Members locality is important for the best performance of the tight search loop below.
	constexpr auto LOOP_UNROLL_SIZE = 8U; //How many comparisons we do at one loop cycle.
	const auto ullOffsetSentinel = sfd.u64RngEnd + 1;
	const auto u64Step = sfd.u64Step;
	const auto pHexCtrl = sfd.pHexCtrl;
	const auto pDlgProg = sfd.pDlgProg;
	const auto uzSizeSearch = sfd.spnFindFrom.size();
	const auto ullEnd = ullOffsetSentinel - uzSizeSearch;
	const auto fBigStep = sfd.fBigStep;
	const auto fInverted = sfd.fInverted;
	auto u64ChunkSize = sfd.u64ChunkSize;
	auto u64ChunkMaxOffset = sfd.u64ChunkMaxOffset;
	auto ullOffsetSearch = sfd.u64StartFrom;

	while (true) {
		const auto spnData = pHexCtrl->GetData({ ullOffsetSearch, u64ChunkSize });
		assert(!spnData.empty());
		assert(spnData.size() >= u64ChunkSize);

		const auto pChunkBase = spnData.data();

		//Unrolling the loop, making LOOP_UNROLL_SIZE comparisons at one cycle.
		for (auto ullOffsetData = 0ULL; ullOffsetData <= u64ChunkMaxOffset; ullOffsetData += u64Step * LOOP_UNROLL_SIZE) {
			//First memory comparison is always unconditional.
			if (pred(pChunkBase + ullOffsetData) == !fInverted) {
				return { ullOffsetSearch + ullOffsetData, true, false };
			}

			//This branch allows to avoid the "ullOffsetData + u64Step * 'nstep' <= u64ChunkMaxOffset" check every cycle.
			if ((ullOffsetData + u64Step * (LOOP_UNROLL_SIZE - 1)) <= u64ChunkMaxOffset) {
				if (pred(pChunkBase + ullOffsetData + u64Step * 1) == !fInverted) {
					return { ullOffsetSearch + ullOffsetData + u64Step * 1, true, false };
				}
				if (pred(pChunkBase + ullOffsetData + u64Step * 2) == !fInverted) {
					return { ullOffsetSearch + ullOffsetData + u64Step * 2, true, false };
				}
				if (pred(pChunkBase + ullOffsetData + u64Step * 3) == !fInverted) {
					return { ullOffsetSearch + ullOffsetData + u64Step * 3, true, false };
				}
				if (pred(pChunkBase + ullOffsetData + u64Step * 4) == !fInverted) {
					return { ullOffsetSearch + ullOffsetData + u64Step * 4, true, false };
				}
				if (pred(pChunkBase + ullOffsetData + u64Step * 5) == !fInverted) {
					return { ullOffsetSearch + ullOffsetData + u64Step * 5, true, false };
				}
				if (pred(pChunkBase + ullOffsetData + u64Step * 6) == !fInverted) {
					return { ullOffsetSearch + ullOffsetData + u64Step * 6, true, false };
				}
				if (pred(pChunkBase + ullOffsetData + u64Step * 7) == !fInverted) {
					return { ullOffsetSearch + ullOffsetData + u64Step * 7, true, false };
				}
			}
			//In this branch we check every comparison for the u64ChunkMaxOffset exceeding.
			//The one less comparison is needed here, otherwise we would've hit the branch above.
			else {
				if ((ullOffsetData + u64Step * 1 <= u64ChunkMaxOffset)
					&& (pred(pChunkBase + ullOffsetData + u64Step * 1) == !fInverted)) {
					return { ullOffsetSearch + ullOffsetData + u64Step * 1, true, false };
				}
				if ((ullOffsetData + u64Step * 2 <= u64ChunkMaxOffset)
					&& (pred(pChunkBase + ullOffsetData + u64Step * 2) == !fInverted)) {
					return { ullOffsetSearch + ullOffsetData + u64Step * 2, true, false };
				}
				if ((ullOffsetData + u64Step * 3 <= u64ChunkMaxOffset)
					&& (pred(pChunkBase + ullOffsetData + u64Step * 3) == !fInverted)) {
					return { ullOffsetSearch + ullOffsetData + u64Step * 3, true, false };
				}
				if ((ullOffsetData + u64Step * 4 <= u64ChunkMaxOffset)
					&& (pred(pChunkBase + ullOffsetData + u64Step * 4) == !fInverted)) {
					return { ullOffsetSearch + ullOffsetData + u64Step * 4, true, false };
				}
				if ((ullOffsetData + u64Step * 5 <= u64ChunkMaxOffset)
					&& (pred(pChunkBase + ullOffsetData + u64Step * 5) == !fInverted)) {
					return { ullOffsetSearch + ullOffsetData + u64Step * 5, true, false };
				}
				if ((ullOffsetData + u64Step * 6 <= u64ChunkMaxOffset)
					&& (pred(pChunkBase + ullOffsetData + u64Step * 6) == !fInverted)) {
					return { ullOffsetSearch + ullOffsetData + u64Step * 6, true, false };
				}
			}

			if constexpr (st.fDlgProg) {
				if (pDlgProg->IsCanceled()) {
					return { { }, false, true };
				}
				pDlgProg->SetCurrent(ullOffsetSearch + ullOffsetData);
			}
		}

		if (fBigStep) [[unlikely]] {
			if ((ullOffsetSearch + u64Step) > ullEnd) {
				break; //Upper bound reached.
			}

			ullOffsetSearch += u64Step;
		}
		else {
			const auto ullStepsInChunk = u64ChunkMaxOffset / u64Step;
			const auto ullStride = (ullStepsInChunk + 1) * u64Step;
			if (ullOffsetSearch > ((std::numeric_limits<ULONGLONG>::max)() - ullStride)) {
				break; //Prevent unsigned wrap-around.
			}

			ullOffsetSearch += ullStride;
			if (ullOffsetSearch + uzSizeSearch > ullOffsetSentinel) {
				break; //Upper bound reached.
			}
		}

		if (ullOffsetSearch + u64ChunkSize > ullOffsetSentinel) {
			u64ChunkSize = ullOffsetSentinel - ullOffsetSearch;
			u64ChunkMaxOffset = u64ChunkSize - uzSizeSearch;
		}
	}

	return { };
}

template<CHexDlgSearch::SEARCHTYPE st>
auto CHexDlgSearch::SearchNumFwd(const SEARCHFUNCDATA& sfd)->FINDRESULT {
	const auto pDataSearch = sfd.spnFindFrom.data();
	return SearchScalarFwd<st>(sfd, [pDataSearch](const std::byte* pWhere) noexcept {
		return MemCmpEQNum<st>(pWhere, pDataSearch);
		});
}

template<CHexDlgSearch::SEARCHTYPE st>
auto CHexDlgSearch::SearchNumRngFwd(const SEARCHFUNCDATA& sfd)->FINDRESULT {
	const auto pDataSearchFrom = sfd.spnFindFrom.data();
	const auto pDataSearchTo = sfd.spnFindTo.data();
	const auto fBigEndian = sfd.fBigEndian;
	return SearchScalarFwd<st>(sfd, [pDataSearchFrom, pDataSearchTo, fBigEndian](const std::byte* pWhere) noexcept {
		return MemCmpEQNumRng<st>(pWhere, pDataSearchFrom, pDataSearchTo, fBigEndian);
		});
}

template<CHexDlgSearch::SEARCHTYPE st>
auto CHexDlgSearch::SearchTextFwd(const SEARCHFUNCDATA& sfd)->FINDRESULT {
	const auto pDataSearch = sfd.spnFindFrom.data();
	const auto uzSizeSearch = sfd.spnFindFrom.size();
	const auto bWildcard = sfd.bWildcard;
	return SearchScalarFwd<st>(sfd, [pDataSearch, uzSizeSearch, bWildcard](const std::byte* pWhere) noexcept {
		return MemCmpEQText<st>(pWhere, pDataSearch, uzSizeSearch, bWildcard);
		});
}

template<CHexDlgSearch::SEARCHTYPE st>
bool CHexDlgSearch::MatchAnchorCandidate(const std::byte* pCandidate, const std::byte* pSearch,
	std::size_t uzSize, std::uint8_t u8FirstUpper, std::byte bLastUpper)noexcept {
	const auto uzLastIdx = uzSize - 1;
	const auto uzMidLen = uzSize - 2;
	if constexpr (st.fMatchCase) {
		return *pCandidate == *pSearch && pCandidate[uzLastIdx] == pSearch[uzLastIdx]
			&& (uzMidLen == 0 || std::memcmp(pCandidate + 1, pSearch + 1, uzMidLen) == 0);
	}
	else {
		return (*pCandidate == *pSearch || static_cast<std::uint8_t>(*pCandidate) == u8FirstUpper)
			&& (pCandidate[uzLastIdx] == pSearch[uzLastIdx] || pCandidate[uzLastIdx] == bLastUpper)
			&& (uzMidLen == 0 || MemCmpEQText<SEARCHTYPE(EMemCmp::DATA_ASCII, st.eVecType, false, false, false)>(
				pCandidate + 1, pSearch + 1, uzMidLen, std::byte { }));
	}
}

template<CHexDlgSearch::SEARCHTYPE st>
auto CHexDlgSearch::SearchFwdVec124(const SEARCHFUNCDATA& sfd)->FINDRESULT {
	if (sfd.u64ChunkSize == 0 || sfd.u64StartFrom > sfd.u64RngEnd) {
		return { };
	}

	assert(sfd.u64Step == 1); //This Vector function works only for step==1.
	using T = decltype([]() {
		using enum EMemCmp;
		if constexpr (st.eMemCmp == DATA_UINT8) {
			return std::uint8_t { };
		}
		else if constexpr (st.eMemCmp == DATA_UINT16) {
			return std::uint16_t { };
		}
		else if constexpr (st.eMemCmp == DATA_UINT32) {
			return std::uint32_t { };
		}
		else {
			static_assert(false, "Unsupported EMemCmp");
		}
	}());


	//Members locality is important for the best performance of the tight search loop below.
	constexpr auto u32VecSize = simd::VecTypeToSize(st.eVecType); //Vector size 128/256.
	constexpr auto u32VecStride = u32VecSize - (sizeof(T) - 1);
	constexpr auto fInverted = st.fInverted;
	const auto u64OffsetSentinel = sfd.u64RngEnd + 1;
	const auto pHexCtrl = sfd.pHexCtrl;
	const auto pDlgProg = sfd.pDlgProg;
	const auto pDataSearch = sfd.spnFindFrom.data();
	const auto tSearch = ut::LoadUnaligned<T>(pDataSearch);
	const auto uzSizeSearch = sfd.spnFindFrom.size();
	auto u64ChunkSize = sfd.u64ChunkSize;
	auto u64ChunkMaxOffset = sfd.u64ChunkMaxOffset;
	auto u64OffsetSearch = sfd.u64StartFrom;

	while (true) {
		const auto spnData = pHexCtrl->GetData({ u64OffsetSearch, u64ChunkSize });
		assert(!spnData.empty());
		assert(spnData.size() >= u64ChunkSize);

		const auto pChunkBase = spnData.data();
		std::uint64_t u64OffsetData { 0 };
		std::uint32_t u32Iter { 0 };

		//Vector loop: stride accounts for multi-byte values crossing vector boundaries.
		for (; (u64OffsetData + u32VecSize) <= u64ChunkSize; u64OffsetData += u32VecStride) {
			const auto u64Res = [&]() noexcept {
				if constexpr (sizeof(T) == 1) {
					return simd::MemCmpEQ1<st.eVecType, !fInverted>(pChunkBase + u64OffsetData, tSearch);
				}
				else if constexpr (sizeof(T) == 2) {
					return simd::MemCmpEQ2<st.eVecType, !fInverted>(pChunkBase + u64OffsetData, tSearch);
				}
				else {
					return simd::MemCmpEQ4<st.eVecType, !fInverted>(pChunkBase + u64OffsetData, tSearch);
				}
				}();

			if (u64Res < u32VecSize) {
				return { u64OffsetSearch + u64OffsetData + u64Res, true, false };
			}

			if constexpr (st.fDlgProg) {
				if ((u32Iter++ & 0x7FFU) == 0) { //Every 2,048 vector iterations (~64 KB).
					if (pDlgProg->IsCanceled()) {
						return { { }, false, true };
					}
					pDlgProg->SetCurrent(u64OffsetSearch + u64OffsetData);
				}
			}
		}

		//Scalar tail for remaining trailing bytes (< u32VecSize).
		for (; u64OffsetData <= u64ChunkMaxOffset; ++u64OffsetData) {
			if (MemCmpEQNum<st>(pChunkBase + u64OffsetData, pDataSearch) == !fInverted) {
				return { u64OffsetSearch + u64OffsetData, true, false };
			}
		}

		const auto u64Stride = u64ChunkMaxOffset + 1;
		if (u64OffsetSearch > ((std::numeric_limits<std::uint64_t>::max)() - u64Stride)) {
			break; //Prevent unsigned wrap-around.
		}

		u64OffsetSearch += u64Stride;
		if (u64OffsetSearch + uzSizeSearch > u64OffsetSentinel) {
			break;
		}

		if (u64OffsetSearch + u64ChunkSize > u64OffsetSentinel) {
			u64ChunkSize = u64OffsetSentinel - u64OffsetSearch;
			u64ChunkMaxOffset = u64ChunkSize - uzSizeSearch;
		}
	}

	return { };
}

template<CHexDlgSearch::SEARCHTYPE st>
auto CHexDlgSearch::SearchFwdVecAnchor(const SEARCHFUNCDATA& sfd)->FINDRESULT {
//SIMD Anchor-Byte Forward Search (for patterns of 3 bytes or >= 5 bytes, step == 1, non-inverted, non-wildcard).
//Supports both exact-case (st.fMatchCase == true) and case-insensitive ASCII (st.fMatchCase == false).
//How it works:
//1. Vector Scan (First-Byte Filter): Loads 16 or 32 bytes per iteration via simd::MemCmpMask1 and compares every
//   lane against the pattern's first byte (u8First, plus u8FirstUpper when !st.fMatchCase), producing a bitmask
//   where bit N == 1 indicates a candidate match at offset (u64OffsetData + N).
//2. Candidate Verification (MatchAnchorCandidate): Iterates set bits from lowest to highest using std::countr_zero
//   and clears each processed bit via (u32Mask &= u32Mask - 1). Each candidate is verified via two-stage rejection:
//     a) Checks the pattern's last byte (exact or case-folded ASCII) in one cycle to reject false positives early.
//     b) Verifies interior bytes [1 .. uzSizeSearch - 2] using std::memcmp (exact) or MemCmpEQText (case-insensitive).
//3. Safe Stride & Scalar Tail: Steps by the full vector width (u32VecSize) while both the vector load stays within
//   u64ChunkSize and the last lane still falls within u64ChunkMaxOffset. Remaining trailing bytes are checked
//   by a scalar loop, and Virtual Data Mode chunks advance by (u64ChunkMaxOffset + 1) to seamlessly catch matches
//   crossing chunk boundaries.

	const auto uzSizeSearch = sfd.spnFindFrom.size();
	if (sfd.u64ChunkSize == 0 || sfd.u64StartFrom > sfd.u64RngEnd || uzSizeSearch < 3) {
		return { };
	}

	assert(sfd.u64Step == 1); //This Vector function works only for step==1.
	constexpr auto u32VecSize = simd::VecTypeToSize(st.eVecType);
	constexpr auto fMatchCase = st.fMatchCase;
	const auto u64OffsetSentinel = sfd.u64RngEnd + 1;
	const auto pHexCtrl = sfd.pHexCtrl;
	const auto pDlgProg = sfd.pDlgProg;
	const auto pDataSearch = sfd.spnFindFrom.data();

	constexpr auto lmbToUpperASCII = [](std::byte b)noexcept->std::uint8_t {
		const auto u8 = static_cast<std::uint8_t>(b);
		return (!fMatchCase && u8 >= 'a' && u8 <= 'z') ? static_cast<std::uint8_t>(u8 - 32) : u8;
		};

	const auto u8First = static_cast<std::uint8_t>(pDataSearch[0]);
	const auto u8FirstUpper = lmbToUpperASCII(pDataSearch[0]);
	const auto bLastUpper = static_cast<std::byte>(lmbToUpperASCII(pDataSearch[uzSizeSearch - 1]));

	auto u64ChunkSize = sfd.u64ChunkSize;
	auto u64ChunkMaxOffset = sfd.u64ChunkMaxOffset;
	auto u64OffsetSearch = sfd.u64StartFrom;

	while (true) {
		const auto spnData = pHexCtrl->GetData({ u64OffsetSearch, u64ChunkSize });
		assert(!spnData.empty());
		assert(spnData.size() >= u64ChunkSize);

		const auto pChunkBase = spnData.data();
		std::uint64_t u64OffsetData { 0 };
		std::uint32_t u32Iter { 0 };

		//Ensure both the SIMD read [u64OffsetData + u32VecSize <= u64ChunkSize]
		//and the last candidate start [u64OffsetData + u32VecSize - 1 <= u64ChunkMaxOffset] are in bounds.
		for (; (u64OffsetData + u32VecSize) <= u64ChunkSize
			&& (u64OffsetData + u32VecSize - 1) <= u64ChunkMaxOffset; u64OffsetData += u32VecSize) {
			auto u32Mask = [&]()noexcept {
				if constexpr (fMatchCase) {
					return simd::MemCmpMask1<st.eVecType>(pChunkBase + u64OffsetData, u8First);
				}
				else {
					return simd::MemCmpMask1<st.eVecType>(pChunkBase + u64OffsetData, u8First, u8FirstUpper);
				}
				}();

			while (u32Mask != 0) {
				const auto u32Bit = static_cast<std::uint32_t>(std::countr_zero(u32Mask));
				if (MatchAnchorCandidate<st>(pChunkBase + u64OffsetData + u32Bit, pDataSearch, uzSizeSearch, u8FirstUpper, bLastUpper)) {
					return { u64OffsetSearch + u64OffsetData + u32Bit, true, false };
				}
				u32Mask &= (u32Mask - 1);
			}

			if constexpr (st.fDlgProg) {
				if ((u32Iter++ & 0x7FFU) == 0) { //Every 2,048 vector iterations (~64 KB).
					if (pDlgProg->IsCanceled()) {
						return { { }, false, true };
					}
					pDlgProg->SetCurrent(u64OffsetSearch + u64OffsetData);
				}
			}
		}

		//Scalar tail for remaining trailing bytes (< u32VecSize).
		for (; u64OffsetData <= u64ChunkMaxOffset; ++u64OffsetData) {
			if (MatchAnchorCandidate<st>(pChunkBase + u64OffsetData, pDataSearch, uzSizeSearch, u8FirstUpper, bLastUpper)) {
				return { u64OffsetSearch + u64OffsetData, true, false };
			}
		}

		const auto u64Stride = u64ChunkMaxOffset + 1;
		if (u64OffsetSearch > ((std::numeric_limits<std::uint64_t>::max)() - u64Stride)) {
			break; //Prevent unsigned wrap-around.
		}

		u64OffsetSearch += u64Stride;
		if (u64OffsetSearch + uzSizeSearch > u64OffsetSentinel) {
			break;
		}

		if (u64OffsetSearch + u64ChunkSize > u64OffsetSentinel) {
			u64ChunkSize = u64OffsetSentinel - u64OffsetSearch;
			u64ChunkMaxOffset = u64ChunkSize - uzSizeSearch;
		}
	}

	return { };
}


//Backward search.

template<CHexDlgSearch::SEARCHTYPE st, typename TPred>
auto CHexDlgSearch::SearchScalarBack(const SEARCHFUNCDATA& sfd, TPred&& pred)->FINDRESULT {
	if (sfd.u64ChunkSize == 0) {
		return { };
	}

	constexpr auto LOOP_UNROLL_SIZE = 8U; //How many comparisons we do at one loop cycle.
	assert(sfd.u64Step > 0);
	assert(sfd.u64Step <= static_cast<std::uint64_t>((std::numeric_limits<std::int64_t>::max)() / LOOP_UNROLL_SIZE));

	//Members locality is important for the best performance of the tight search loop below.
	const auto u64StartFrom = sfd.u64StartFrom;
	const auto u64End = sfd.u64RngStart;
	//Step is signed here to make the "i64OffsetData - i64Step" arithmetic also signed.
	const auto i64Step = static_cast<std::int64_t>(sfd.u64Step);
	const auto pHexCtrl = sfd.pHexCtrl;
	const auto pDlgProg = sfd.pDlgProg;
	const auto uzSizeSearch = sfd.spnFindFrom.size();
	const auto fBigStep = sfd.fBigStep;
	const auto fInverted = sfd.fInverted;
	auto u64ChunkSize = sfd.u64ChunkSize;
	auto u64ChunkMaxOffset = sfd.u64ChunkMaxOffset;
	std::uint64_t u64OffsetSearch { };

	if ((u64StartFrom - u64End) < u64ChunkMaxOffset) {
		u64ChunkSize = (u64StartFrom - u64End) + uzSizeSearch;
		u64ChunkMaxOffset = u64ChunkSize - uzSizeSearch;
		u64OffsetSearch = u64End;
	}
	else {
		u64OffsetSearch = u64StartFrom - u64ChunkMaxOffset;
	}

	while (true) {
		const auto spnData = pHexCtrl->GetData({ u64OffsetSearch, u64ChunkSize });
		assert(!spnData.empty());
		assert(spnData.size() >= u64ChunkSize);

		const auto pChunkBase = spnData.data();

		for (auto i64OffsetData = static_cast<std::int64_t>(u64ChunkMaxOffset); i64OffsetData >= 0;
			i64OffsetData -= i64Step * LOOP_UNROLL_SIZE) { //i64OffsetData might be negative.
			//First memory comparison is always unconditional.
			if (pred(pChunkBase + i64OffsetData) == !fInverted) {
				return { u64OffsetSearch + static_cast<std::uint64_t>(i64OffsetData), true, false };
			}

			if ((i64OffsetData - i64Step * (LOOP_UNROLL_SIZE - 1)) >= 0) {
				if (pred(pChunkBase + (i64OffsetData - i64Step * 1)) == !fInverted) {
					return { u64OffsetSearch + static_cast<std::uint64_t>(i64OffsetData - i64Step * 1), true, false };
				}
				if (pred(pChunkBase + (i64OffsetData - i64Step * 2)) == !fInverted) {
					return { u64OffsetSearch + static_cast<std::uint64_t>(i64OffsetData - i64Step * 2), true, false };
				}
				if (pred(pChunkBase + (i64OffsetData - i64Step * 3)) == !fInverted) {
					return { u64OffsetSearch + static_cast<std::uint64_t>(i64OffsetData - i64Step * 3), true, false };
				}
				if (pred(pChunkBase + (i64OffsetData - i64Step * 4)) == !fInverted) {
					return { u64OffsetSearch + static_cast<std::uint64_t>(i64OffsetData - i64Step * 4), true, false };
				}
				if (pred(pChunkBase + (i64OffsetData - i64Step * 5)) == !fInverted) {
					return { u64OffsetSearch + static_cast<std::uint64_t>(i64OffsetData - i64Step * 5), true, false };
				}
				if (pred(pChunkBase + (i64OffsetData - i64Step * 6)) == !fInverted) {
					return { u64OffsetSearch + static_cast<std::uint64_t>(i64OffsetData - i64Step * 6), true, false };
				}
				if (pred(pChunkBase + (i64OffsetData - i64Step * 7)) == !fInverted) {
					return { u64OffsetSearch + static_cast<std::uint64_t>(i64OffsetData - i64Step * 7), true, false };
				}
			}
			else {
				if ((i64OffsetData - i64Step * 1 >= 0)
					&& (pred(pChunkBase + (i64OffsetData - i64Step * 1)) == !fInverted)) {
					return { u64OffsetSearch + static_cast<std::uint64_t>(i64OffsetData - i64Step * 1), true, false };
				}
				if ((i64OffsetData - i64Step * 2 >= 0)
					&& (pred(pChunkBase + (i64OffsetData - i64Step * 2)) == !fInverted)) {
					return { u64OffsetSearch + static_cast<std::uint64_t>(i64OffsetData - i64Step * 2), true, false };
				}
				if ((i64OffsetData - i64Step * 3 >= 0)
					&& (pred(pChunkBase + (i64OffsetData - i64Step * 3)) == !fInverted)) {
					return { u64OffsetSearch + static_cast<std::uint64_t>(i64OffsetData - i64Step * 3), true, false };
				}
				if ((i64OffsetData - i64Step * 4 >= 0)
					&& (pred(pChunkBase + (i64OffsetData - i64Step * 4)) == !fInverted)) {
					return { u64OffsetSearch + static_cast<std::uint64_t>(i64OffsetData - i64Step * 4), true, false };
				}
				if ((i64OffsetData - i64Step * 5 >= 0)
					&& (pred(pChunkBase + (i64OffsetData - i64Step * 5)) == !fInverted)) {
					return { u64OffsetSearch + static_cast<std::uint64_t>(i64OffsetData - i64Step * 5), true, false };
				}
				if ((i64OffsetData - i64Step * 6 >= 0)
					&& (pred(pChunkBase + (i64OffsetData - i64Step * 6)) == !fInverted)) {
					return { u64OffsetSearch + static_cast<std::uint64_t>(i64OffsetData - i64Step * 6), true, false };
				}
			}

			if constexpr (st.fDlgProg) {
				if (pDlgProg->IsCanceled()) {
					return { { }, false, true };
				}
				pDlgProg->SetCurrent(u64End + (u64StartFrom - (u64OffsetSearch + static_cast<std::uint64_t>(i64OffsetData))));
			}
		}

		if (u64OffsetSearch <= u64End) {
			break;
		}

		if (fBigStep) [[unlikely]] {
			if ((u64OffsetSearch - u64End) < sfd.u64Step) {
				break; //Lower bound reached.
			}

			u64OffsetSearch -= sfd.u64Step;
		}
		else {
			const auto u64StepsInChunk = u64ChunkMaxOffset / sfd.u64Step;
			const auto u64Stride = (u64StepsInChunk + 1) * sfd.u64Step;

			if ((u64OffsetSearch - u64End) < u64Stride) {
				//Lowest offset already checked in the current chunk.
				const auto u64LastChecked = u64OffsetSearch + (u64ChunkMaxOffset % sfd.u64Step);
				if ((u64LastChecked - u64End) < sfd.u64Step) {
					break; //No aligned step remains at or above u64End.
				}

				const auto u64NextLastOffset = u64LastChecked - sfd.u64Step;
				u64ChunkSize = (u64NextLastOffset - u64End) + uzSizeSearch;
				u64ChunkMaxOffset = u64ChunkSize - uzSizeSearch;
				u64OffsetSearch = u64End;
			}
			else {
				u64OffsetSearch -= u64Stride;
			}
		}
	}

	return { };
}

template<CHexDlgSearch::SEARCHTYPE st>
auto CHexDlgSearch::SearchNumBack(const SEARCHFUNCDATA& sfd)->FINDRESULT {
	const auto pDataSearch = sfd.spnFindFrom.data();
	return SearchScalarBack<st>(sfd, [pDataSearch](const std::byte* pWhere) noexcept {
		return MemCmpEQNum<st>(pWhere, pDataSearch);
		});
}

template<CHexDlgSearch::SEARCHTYPE st>
auto CHexDlgSearch::SearchNumRngBack(const SEARCHFUNCDATA& sfd)->FINDRESULT {
	const auto pDataSearch = sfd.spnFindFrom.data();
	const auto pDataSearchTo = sfd.spnFindTo.data();
	const auto fBigEndian = sfd.fBigEndian;
	return SearchScalarBack<st>(sfd, [pDataSearch, pDataSearchTo, fBigEndian](const std::byte* pWhere) noexcept {
		return MemCmpEQNumRng<st>(pWhere, pDataSearch, pDataSearchTo, fBigEndian);
		});
}

template<CHexDlgSearch::SEARCHTYPE st>
auto CHexDlgSearch::SearchTextBack(const SEARCHFUNCDATA& sfd)->FINDRESULT {
	const auto pDataSearch = sfd.spnFindFrom.data();
	const auto uzSizeSearch = sfd.spnFindFrom.size();
	const auto bWildcard = sfd.bWildcard;
	return SearchScalarBack<st>(sfd, [pDataSearch, uzSizeSearch, bWildcard](const std::byte* pWhere) noexcept {
		return MemCmpEQText<st>(pWhere, pDataSearch, uzSizeSearch, bWildcard);
		});
}

template<CHexDlgSearch::SEARCHTYPE st>
auto CHexDlgSearch::SearchBackVec1(const SEARCHFUNCDATA& sfd) -> FINDRESULT {
	if (sfd.u64ChunkSize == 0 || sfd.u64StartFrom < sfd.u64RngStart) {
		return { };
	}

	assert(sfd.u64Step == 1); //This Vector function works only for step == 1.
	constexpr auto u32VecSize = simd::VecTypeToSize(st.eVecType);
	constexpr auto fInverted = st.fInverted;
	constexpr auto u32FullMask = (st.eVecType == simd::EVecType::VECTOR_128) ? 0xFFFFU : 0xFFFFFFFFU;

	const auto u64StartFrom = sfd.u64StartFrom;
	const auto u64End = sfd.u64RngStart;
	const auto pHexCtrl = sfd.pHexCtrl;
	const auto pDlgProg = sfd.pDlgProg;
	const auto pDataSearch = sfd.spnFindFrom.data();
	const auto u8Search = ut::LoadUnaligned<std::uint8_t>(pDataSearch);

	auto u64ChunkSize = sfd.u64ChunkSize;
	auto u64ChunkMaxOffset = sfd.u64ChunkMaxOffset;
	std::uint64_t u64OffsetSearch { };

	if ((u64StartFrom - u64End) < u64ChunkMaxOffset) {
		u64ChunkSize = (u64StartFrom - u64End) + 1;
		u64ChunkMaxOffset = u64ChunkSize - 1;
		u64OffsetSearch = u64End;
	}
	else {
		u64OffsetSearch = u64StartFrom - u64ChunkMaxOffset;
	}

	while (true) {
		const auto spnData = pHexCtrl->GetData({ u64OffsetSearch, u64ChunkSize });
		assert(!spnData.empty());
		assert(spnData.size() >= u64ChunkSize);

		const auto pChunkBase = spnData.data();
		auto i64OffsetData = static_cast<std::int64_t>(u64ChunkMaxOffset);
		std::uint32_t u32Iter { 0 };

		//Vector loop: processes u32VecSize bytes ending at i64OffsetData.
		while (i64OffsetData >= static_cast<std::int64_t>(u32VecSize - 1)) {
			const auto u64VecStart = static_cast<std::uint64_t>(i64OffsetData - (u32VecSize - 1));
			auto u32Mask = simd::MemCmpMask1<st.eVecType>(pChunkBase + u64VecStart, u8Search);
			if constexpr (fInverted) {
				u32Mask ^= u32FullMask;
			}

			if (u32Mask != 0) {
				const auto u32HighestBit = 31U - static_cast<std::uint32_t>(std::countl_zero(u32Mask));
				return { u64OffsetSearch + u64VecStart + u32HighestBit, true, false };
			}

			if constexpr (st.fDlgProg) {
				if ((u32Iter++ & 0x7FFU) == 0) { //Every 2,048 vector iterations (~64 KB).
					if (pDlgProg->IsCanceled()) {
						return { { }, false, true };
					}
					pDlgProg->SetCurrent(u64End + (u64StartFrom - (u64OffsetSearch + u64VecStart)));
				}
			}

			i64OffsetData -= static_cast<std::int64_t>(u32VecSize);
		}

		//Scalar tail for remaining leading bytes (< u32VecSize).
		for (; i64OffsetData >= 0; --i64OffsetData) {
			if (MemCmpEQNum<st>(pChunkBase + i64OffsetData, pDataSearch) == !fInverted) {
				return { u64OffsetSearch + static_cast<std::uint64_t>(i64OffsetData), true, false };
			}
		}

		if (u64OffsetSearch <= u64End) {
			break;
		}

		if ((u64OffsetSearch - u64End) < u64ChunkSize) {
			u64ChunkSize = u64OffsetSearch - u64End;
			u64ChunkMaxOffset = u64ChunkSize - 1;
			u64OffsetSearch = u64End;
		}
		else {
			u64OffsetSearch -= u64ChunkSize;
		}
	}

	return { };
}

template<CHexDlgSearch::SEARCHTYPE st>
auto CHexDlgSearch::SearchBackVecAnchor(const SEARCHFUNCDATA& sfd) -> FINDRESULT {
//SIMD Anchor-Byte Backward Search (for patterns >= 2 bytes, step == 1, non-inverted, non-wildcard).
//Supports both exact-case (st.fMatchCase == true) and case-insensitive ASCII (st.fMatchCase == false).
//How it works:
//1. Vector Scan (First-Byte Filter): Scans backward in 16- or 32-byte blocks ending at i64OffsetData via
//   simd::MemCmpMask1 (matching u8First, plus u8FirstUpper when !st.fMatchCase), producing a bitmask of
//   candidate starting positions [u64VecStart .. i64OffsetData].
//2. Reverse Candidate Verification (MatchAnchorCandidate): Iterates set bits from HIGHEST to LOWEST using
//   (31 - std::countl_zero(u32Mask)) and clears each tested bit via (u32Mask &= ~(1U << u32Bit)). Because
//   i64OffsetData <= u64ChunkMaxOffset (u64ChunkSize - uzSizeSearch), even the highest lane
//   (u64VecStart + u32VecSize - 1 == i64OffsetData) has at least uzSizeSearch valid bytes remaining in the chunk.
//3. Two-Stage Rejection & Exact Chunk Overlap: MatchAnchorCandidate checks the last byte first, then interior
//   bytes via std::memcmp (exact) or MemCmpEQText (case-insensitive). Virtual Data Mode chunks step backward by
//   (u64ChunkMaxOffset + 1) so chunks overlap by exact (uzSizeSearch - 1) bytes without re-checking offsets.

	const auto uzSizeSearch = sfd.spnFindFrom.size();
	if (sfd.u64ChunkSize == 0 || sfd.u64StartFrom < sfd.u64RngStart || uzSizeSearch < 2) {
		return { };
	}

	assert(sfd.u64Step == 1); //This Vector function works only for step==1.
	constexpr auto u32VecSize = simd::VecTypeToSize(st.eVecType);
	constexpr auto fMatchCase = st.fMatchCase;
	const auto u64StartFrom = sfd.u64StartFrom;
	const auto u64End = sfd.u64RngStart;
	const auto pHexCtrl = sfd.pHexCtrl;
	const auto pDlgProg = sfd.pDlgProg;
	const auto pDataSearch = sfd.spnFindFrom.data();

	constexpr auto lmbToUpperASCII = [](std::byte b) noexcept -> std::uint8_t {
		const auto u8 = static_cast<std::uint8_t>(b);
		return (!fMatchCase && u8 >= 'a' && u8 <= 'z') ? static_cast<std::uint8_t>(u8 - 32) : u8;
		};

	const auto u8First = static_cast<std::uint8_t>(pDataSearch[0]);
	const auto u8FirstUpper = lmbToUpperASCII(pDataSearch[0]);
	const auto bLastUpper = static_cast<std::byte>(lmbToUpperASCII(pDataSearch[uzSizeSearch - 1]));

	auto u64ChunkSize = sfd.u64ChunkSize;
	auto u64ChunkMaxOffset = sfd.u64ChunkMaxOffset;
	std::uint64_t u64OffsetSearch { };

	if ((u64StartFrom - u64End) < u64ChunkMaxOffset) {
		u64ChunkSize = (u64StartFrom - u64End) + uzSizeSearch;
		u64ChunkMaxOffset = u64ChunkSize - uzSizeSearch;
		u64OffsetSearch = u64End;
	}
	else {
		u64OffsetSearch = u64StartFrom - u64ChunkMaxOffset;
	}

	while (true) {
		const auto spnData = pHexCtrl->GetData({ u64OffsetSearch, u64ChunkSize });
		assert(!spnData.empty());
		assert(spnData.size() >= u64ChunkSize);

		const auto pChunkBase = spnData.data();
		auto i64OffsetData = static_cast<std::int64_t>(u64ChunkMaxOffset);
		std::uint32_t u32Iter { 0 };

		while (i64OffsetData >= static_cast<std::int64_t>(u32VecSize - 1)) {
			const auto u64VecStart = static_cast<std::uint64_t>(i64OffsetData - (u32VecSize - 1));
			assert(u64VecStart + u32VecSize <= u64ChunkSize);

			auto u32Mask = [&]() noexcept {
				if constexpr (fMatchCase) {
					return simd::MemCmpMask1<st.eVecType>(pChunkBase + u64VecStart, u8First);
				}
				else {
					return simd::MemCmpMask1<st.eVecType>(pChunkBase + u64VecStart, u8First, u8FirstUpper);
				}
				}();

			while (u32Mask != 0) {
				const auto u32Bit = 31U - static_cast<std::uint32_t>(std::countl_zero(u32Mask));
				if (MatchAnchorCandidate<st>(pChunkBase + u64VecStart + u32Bit, pDataSearch, uzSizeSearch, u8FirstUpper, bLastUpper)) {
					return { u64OffsetSearch + u64VecStart + u32Bit, true, false };
				}
				u32Mask &= ~(1U << u32Bit); //Clear highest set bit.
			}

			if constexpr (st.fDlgProg) {
				if ((u32Iter++ & 0x7FFU) == 0) { //Every 2,048 vector iterations (~64 KB).
					if (pDlgProg->IsCanceled()) {
						return { { }, false, true };
					}
					pDlgProg->SetCurrent(u64End + (u64StartFrom - (u64OffsetSearch + u64VecStart)));
				}
			}

			i64OffsetData -= static_cast<std::int64_t>(u32VecSize);
		}

		//Scalar tail for remaining leading bytes (< u32VecSize).
		for (; i64OffsetData >= 0; --i64OffsetData) {
			if (MatchAnchorCandidate<st>(pChunkBase + i64OffsetData, pDataSearch, uzSizeSearch, u8FirstUpper, bLastUpper)) {
				return { u64OffsetSearch + static_cast<std::uint64_t>(i64OffsetData), true, false };
			}
		}

		if (u64OffsetSearch <= u64End) {
			break;
		}

		//Step back by (u64ChunkMaxOffset + 1) so the next chunk ends at (u64OffsetSearch + uzSizeSearch - 2),
		//providing exact (uzSizeSearch - 1) overlap without re-checking u64OffsetSearch.
		const auto u64Stride = u64ChunkMaxOffset + 1;
		if ((u64OffsetSearch - u64End) < u64Stride) {
			u64ChunkSize = (u64OffsetSearch - u64End) + (uzSizeSearch - 1);
			u64ChunkMaxOffset = u64ChunkSize - uzSizeSearch;
			u64OffsetSearch = u64End;
		}
		else {
			u64OffsetSearch -= u64Stride;
		}
	}

	return { };
}