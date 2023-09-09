/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../../HexCtrl.h"
#include "../../dep/ListEx/ListEx.h"
#include <afxdialogex.h>

namespace HEXCTRL::INTERNAL
{
	class CHexDlgSearch final : public CDialogEx {
	public:
		[[nodiscard]] auto GetDlgData()const->std::uint64_t;
		void Initialize(IHexCtrl* pHexCtrl);
		[[nodiscard]] bool IsSearchAvail()const; //Can we do search next/prev?
		void SearchNextPrev(bool fForward);
		auto SetDlgData(std::uint64_t ullData) -> HWND;
		BOOL ShowWindow(int nCmdShow);
	private:
		enum class EMode : std::uint8_t; //Forward declarations.
		enum class ECmpType : std::uint16_t;
		enum class EMenuID : std::uint16_t;
		struct SFINDRESULT;
		struct STHREADRUN;
		void DoDataExchange(CDataExchange* pDX)override;
		void AddToList(ULONGLONG ullOffset);
		void ClearList();
		void ComboSearchFill(LPCWSTR pwsz);
		void ComboReplaceFill(LPCWSTR pwsz);
		//Main routine for finding stuff.
		[[nodiscard]] SFINDRESULT Finder(ULONGLONG& ullStart, ULONGLONG ullEnd, SpanCByte spnSearch,
			bool fForward = true, CHexDlgCallback* pDlgClbk = nullptr, bool fDlgExit = true);
		[[nodiscard]] IHexCtrl* GetHexCtrl()const;
		[[nodiscard]] EMode GetSearchMode()const; //Returns current search mode.
		void HexCtrlHighlight(const VecSpan& vecSel); //Highlight found occurence in HexCtrl.
		BOOL OnInitDialog()override;
		afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		afx_msg void OnButtonSearchF();
		afx_msg void OnButtonSearchB();
		afx_msg void OnButtonFindAll();
		afx_msg void OnButtonReplace();
		afx_msg void OnButtonReplaceAll();
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
		void Prepare();
		[[nodiscard]] bool PrepareHexBytes();
		[[nodiscard]] bool PrepareTextASCII();
		[[nodiscard]] bool PrepareTextUTF16();
		[[nodiscard]] bool PrepareTextUTF8();
		[[nodiscard]] bool PrepareINT8();
		[[nodiscard]] bool PrepareINT16();
		[[nodiscard]] bool PrepareINT32();
		[[nodiscard]] bool PrepareINT64();
		[[nodiscard]] bool PrepareFloat();
		[[nodiscard]] bool PrepareDouble();
		[[nodiscard]] bool PrepareFILETIME();
		void Replace(ULONGLONG ullIndex, SpanCByte spnReplace)const;
		void ResetSearch();
		void Search();
		void SetEditStartAt(ULONGLONG ullOffset); //Start search offset edit set.
		template<std::uint16_t uCmpType> void ThreadRun(STHREADRUN* pStThread);
		void UpdateSearchReplaceControls();
		template<std::uint16_t uCmpType>
		[[nodiscard]] static bool MemCmp(const std::byte* pBuf1, const std::byte* pBuf2, std::size_t nSize);
		[[nodiscard]] static auto RangeToVecBytes(const std::string& str) -> std::vector<std::byte>;
		[[nodiscard]] static auto RangeToVecBytes(const std::wstring& wstr) -> std::vector<std::byte>;
		template<typename T>
		[[nodiscard]] static auto RangeToVecBytes(T tData) -> std::vector<std::byte>;
		DECLARE_MESSAGE_MAP();
	private:
		static constexpr std::byte m_uWildcard { '?' }; //Wildcard symbol.
		static constexpr auto m_pwszWrongInput { L"Wrong input data!" };
		IHexCtrl* m_pHexCtrl { };
		EMode m_eSearchMode { };
		LISTEX::IListExPtr m_pListMain { LISTEX::CreateListEx() };
		std::vector<ULONGLONG> m_vecSearchRes { }; //Search results.
		CMenu m_stMenuList;         //Menu for the list control.
		CComboBox m_stComboSearch;  //Combo box "Search".
		CComboBox m_stComboReplace; //Combo box "Replace".
		CComboBox m_stComboMode;    //Combo box "Search mode".
		CButton m_stCheckSel;       //Check box "In selection".
		CButton m_stCheckWcard;     //Check box "Wildcard".
		CButton m_stCheckBE;        //Check box "Big-endian".
		CButton m_stCheckMatchC;    //Check box "Match case".
		CEdit m_stEditStart;        //Edit box "Start search at".
		CEdit m_stEditStep;         //Edit box "Step".
		CEdit m_stEditLimit;        //Edit box "Limit search hit".
		ULONGLONG m_ullBoundBegin { };       //Search start boundary.
		ULONGLONG m_ullBoundEnd { };         //Search end boundary.
		ULONGLONG m_ullOffsetCurr { };       //Current offset a search should start from.
		ULONGLONG m_ullSizeSentinel { };     //Maximum size that search can't cross.
		ULONGLONG m_ullStep { 1 };           //Search step (default is 1 byte).
		DWORD m_dwCount { };                 //How many, or what index number.
		DWORD m_dwReplaced { };              //Replaced amount;
		DWORD m_dwFoundLimit { 10000 };      //Maximum found search occurences.
		int m_iDirection { };                //Search direction: 1 = Forward, -1 = Backward.
		int m_iWrap { };                     //Wrap direction: -1 = Beginning, 1 = End.
		std::vector<std::byte> m_vecSearchData;  //Data to search for.
		std::vector<std::byte> m_vecReplaceData; //Data to replace with.
		std::wstring m_wstrTextSearch;       //Text from "Search" box.
		std::wstring m_wstrTextReplace;      //Text from "Replace with..." box.
		HEXSPAN m_stSelSpan { };             //Previous selection.
		void(CHexDlgSearch::*m_pfnThread)(STHREADRUN* pThread); //Func pointer to the ThreadRun<> for the Search thread.
		bool m_fSecondMatch { false };       //First or subsequent match. 
		bool m_fFound { false };             //Found or not.
		bool m_fDoCount { true };            //Do we count matches or just print "Found".
		bool m_fReplace { false };           //Find or Find and Replace with...?
		bool m_fAll { false };               //Find/Replace one by one, or all?
		bool m_fSelection { false };         //"In selection" check box.
		bool m_fWildcard { false };          //"Wildcard" check box.
		bool m_fBigEndian { false };         //"Big-endian" check box.
		bool m_fMatchCase { false };         //"Match case" check box.
		bool m_fInverted { false };          //"Inverted" check box
		bool m_fReplaceWarn { true };        //Show "Replace string size exceeds..." warning message or not.
		bool m_fSearchNext { false };        //Search through Next/Prev menu.
	};
}