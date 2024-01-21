/****************************************************************************************
* Copyright © 2018-2024 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../../dep/rapidjson/rapidjson-amalgam.h"
#include "../../dep/ListEx/ListEx.h"
#include "../../HexCtrl.h"
#include <afxdialogex.h>
#include <unordered_map>

namespace HEXCTRL::INTERNAL {
	class CHexDlgTemplMgr final : public CDialogEx, public IHexTemplates {
	public:
		CHexDlgTemplMgr();
		~CHexDlgTemplMgr();
		auto AddTemplate(PCHEXTEMPLATE pTemplate) -> int override;
		void ApplyCurr(ULONGLONG ullOffset); //Apply currently selected template to offset.
		int ApplyTemplate(ULONGLONG ullOffset, int iTemplateID)override; //Apply template to a given offset.
		void DisapplyAll()override;
		void DisapplyByID(int iAppliedID)override; //Disapply template with the given AppliedID.
		void DisapplyByOffset(ULONGLONG ullOffset)override;
		[[nodiscard]] auto GetDlgData()const->std::uint64_t;
		[[nodiscard]] bool HasApplied()const;
		[[nodiscard]] bool HasCurrent()const;
		[[nodiscard]] bool HasTemplates()const;
		[[nodiscard]] auto HitTest(ULONGLONG ullOffset)const->PCHEXTEMPLFIELD; //Template hittest by offset.
		void Initialize(IHexCtrl* pHexCtrl);
		[[nodiscard]] bool IsTooltips()const;
		int LoadFromFile(const wchar_t* pFilePath)override; //Returns loaded template ID on success, zero otherwise.
		auto SetDlgData(std::uint64_t ullData, bool fCreate) -> HWND;
		void ShowTooltips(bool fShow)override;
		BOOL ShowWindow(int nCmdShow);
		void UnloadAll()override;
		void UpdateData();

		//Static functions.
		struct FIELDSDEFPROPS; //Forward declaration.
		using IterJSONMember = rapidjson::Value::ConstMemberIterator;
		using UmapCustomTypes = std::unordered_map<std::uint8_t, VecFields>;
		[[nodiscard]] static bool JSONParseFields(const IterJSONMember iterFieldsArray, VecFields& refVecFields,
				const FIELDSDEFPROPS& refDefault, UmapCustomTypes& umapCustomT, int* pOffset = nullptr);
		[[nodiscard]] static auto JSONEndianness(const rapidjson::Value& value) -> std::optional<bool>;
		[[nodiscard]] static auto JSONColors(const rapidjson::Value& value, const char* pszColorName) -> std::optional<COLORREF>;
		[[nodiscard]] static auto CloneTemplate(PCHEXTEMPLATE pTemplate) -> std::unique_ptr<HEXTEMPLATE>;
		static LRESULT CALLBACK TreeSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
			UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	private:
		struct TEMPLAPPLIED;
		using PCTEMPLAPPLIED = const TEMPLAPPLIED*;
		void ApplyDlgData();
		void DoDataExchange(CDataExchange* pDX)override;
		void EnableDynamicLayoutHelper(bool fEnable);
		[[nodiscard]] auto GetAppliedFromItem(HTREEITEM hTreeItem) -> PCTEMPLAPPLIED;
		[[nodiscard]] auto GetIDForNewTemplate()const->int;
		[[nodiscard]] auto GetTemplate(int iTemplateID)const->PCHEXTEMPLATE;
		[[nodiscard]] bool IsHglSel()const;
		[[nodiscard]] bool IsMinimized()const;
		[[nodiscard]] bool IsNoEsc()const;
		[[nodiscard]] bool IsShowAsHex()const;
		[[nodiscard]] bool IsSwapEndian()const;
		afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		afx_msg void OnBnLoadTemplate();
		afx_msg void OnBnUnloadTemplate();
		afx_msg void OnBnRandomizeColors();
		afx_msg void OnBnApply();
		void OnCancel()override;
		BOOL OnCommand(WPARAM wParam, LPARAM lParam)override;
		afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
		afx_msg void OnCheckHex();
		afx_msg void OnCheckSwapEndian();
		afx_msg void OnCheckShowTt();
		afx_msg void OnCheckMinMax();
		afx_msg void OnClose();
		afx_msg void OnDestroy();
		BOOL OnInitDialog()override;
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
		afx_msg void OnListDblClick(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnListEditBegin(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnListEnterPressed(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnListGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnListGetColor(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnListHdrRClick(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnListItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnListRClick(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnListSetData(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnMouseMove(UINT nFlags, CPoint point);
		void OnOK()override;
		void OnTemplateLoadUnload(int iTemplateID, bool fLoad);
		afx_msg void OnTreeGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnTreeItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnTreeRClick(NMHDR* pNMHDR, LRESULT* pResult);
		void RandomizeTemplateColors(int iTemplateID);
		void RemoveNodesWithTemplateID(int iTemplateID);
		void RemoveNodeWithAppliedID(int iAppliedID);
		[[nodiscard]] bool SetDataBool(LPCWSTR pwszText, ULONGLONG ullOffset)const;
		[[nodiscard]] bool SetDataChar(LPCWSTR pwszText, ULONGLONG ullOffset)const;
		[[nodiscard]] bool SetDataUChar(LPCWSTR pwszText, ULONGLONG ullOffset)const;
		[[nodiscard]] bool SetDataShort(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const;
		[[nodiscard]] bool SetDataUShort(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const;
		[[nodiscard]] bool SetDataInt(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const;
		[[nodiscard]] bool SetDataUInt(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const;
		[[nodiscard]] bool SetDataLL(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const;
		[[nodiscard]] bool SetDataULL(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const;
		[[nodiscard]] bool SetDataFloat(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const;
		[[nodiscard]] bool SetDataDouble(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const;
		[[nodiscard]] bool SetDataTime32(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const;
		[[nodiscard]] bool SetDataTime64(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const;
		[[nodiscard]] bool SetDataFILETIME(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const;
		[[nodiscard]] bool SetDataSYSTEMTIME(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const;
		[[nodiscard]] bool SetDataGUID(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const;
		void SetDlgButtonsState(); //Enable/disable button states depending on templates existence.
		void SetHexSelByField(PCHEXTEMPLFIELD pField);
		void ShowListDataBool(LPWSTR pwsz, unsigned char uchData)const;
		void ShowListDataChar(LPWSTR pwsz, char chData)const;
		void ShowListDataUChar(LPWSTR pwsz, unsigned char uchData)const;
		void ShowListDataShort(LPWSTR pwsz, short shortData, bool fShouldSwap)const;
		void ShowListDataUShort(LPWSTR pwsz, unsigned short wData, bool fShouldSwap)const;
		void ShowListDataInt(LPWSTR pwsz, int intData, bool fShouldSwap)const;
		void ShowListDataUInt(LPWSTR pwsz, unsigned int dwData, bool fShouldSwap)const;
		void ShowListDataLL(LPWSTR pwsz, long long llData, bool fShouldSwap)const;
		void ShowListDataULL(LPWSTR pwsz, unsigned long long ullData, bool fShouldSwap)const;
		void ShowListDataFloat(LPWSTR pwsz, float flData, bool fShouldSwap)const;
		void ShowListDataDouble(LPWSTR pwsz, double dblData, bool fShouldSwap)const;
		void ShowListDataTime32(LPWSTR pwsz, __time32_t lTime32, bool fShouldSwap)const;
		void ShowListDataTime64(LPWSTR pwsz, __time64_t llTime64, bool fShouldSwap)const;
		void ShowListDataFILETIME(LPWSTR pwsz, FILETIME stFTime, bool fShouldSwap)const;
		void ShowListDataSYSTEMTIME(LPWSTR pwsz, SYSTEMTIME stSTime, bool fShouldSwap)const;
		void ShowListDataGUID(LPWSTR pwsz, GUID stGUID, bool fShouldSwap)const;
		[[nodiscard]] auto TreeItemFromListItem(int iListItem)const->HTREEITEM;
		void UnloadTemplate(int iTemplateID)override; //Unload/remove loaded template from memory.
		void UpdateStaticText();
		DECLARE_MESSAGE_MAP();
	private:
		static constexpr auto m_iIDListApplFieldType { 0 }; //ID of the Type field in the m_pListApplied.
		static constexpr auto m_iIDListApplFieldData { 4 };
		static constexpr auto m_iIDListApplFieldDescr { 6 };
		static constexpr auto m_iIDListApplFieldClrs { 7 };
		enum class EMenuID : std::uint16_t;
		IHexCtrl* m_pHexCtrl { };
		std::vector<std::unique_ptr<HEXTEMPLATE>> m_vecTemplates;      //Loaded Templates.
		std::vector<std::unique_ptr<TEMPLAPPLIED>> m_vecTemplatesAppl; //Currently Applied Templates.
		CComboBox m_comboTemplates;  //Currently available templates list.
		CEdit m_editOffset;          //"Offset" edit box.
		CButton m_btnMinMax;         //Check-box min-max.
		CButton m_btnShowTT;         //Check-box "Show tooltips".
		CButton m_btnHglSel;         //Check-box "Highlight selected".
		CButton m_btnHex;            //Check-box "Hex numbers".
		CButton m_btnSwapEndian;     //Check-box "Swap endian".
		CWnd m_wndStaticOffset;      //Static text "Template offset:".
		CWnd m_wndStaticSize;        //Static text Template size:".
		HBITMAP m_hBITMAPMin { };    //Bitmap for the min checkbox.
		HBITMAP m_hBITMAPMax { };    //Bitmap for the max checkbox.
		LISTEX::IListExPtr m_pListApplied { LISTEX::CreateListEx() };
		CTreeCtrl m_treeApplied;
		CMenu m_menuTree;            //Menu for the tree control.
		CMenu m_menuHdr;             //Menu for the list header.
		HCURSOR m_hCurResize;
		HCURSOR m_hCurArrow;
		PCTEMPLAPPLIED m_pAppliedCurr { }; //Currently selected template in the applied Tree.
		PCVecFields m_pVecFieldsCurr { };     //Currently selected Fields vector.
		HTREEITEM m_hTreeCurrParent { };      //Currently selected Tree node's parent.
		std::uint64_t m_u64DlgData { };       //Data from SetDlgData.
		DWORD m_dwDateFormat { };             //Date format.
		int m_iDynLayoutMinY { };             //For DynamicLayout::SetMinSize.
		wchar_t m_wchDateSepar { };           //Date separator.
		bool m_fCurInSplitter { };            //Indicates that mouse cursor is in the splitter area.
		bool m_fLMDownResize { };             //Left mouse pressed in splitter area to resize.
		bool m_fListGuardEvent { false };     //To not proceed with OnListItemChanged, same as pTree->action == TVC_UNKNOWN.
		bool m_fTooltips { true };            //Show tooltips or not.
	};
}