/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../../dep/rapidjson/rapidjson-amalgam.h"
#include "../../HexCtrl.h"
#include <afxdialogex.h>
#include <unordered_map>

import HEXCTRL.HexUtility;

namespace HEXCTRL::INTERNAL {
	class CHexDlgTemplMgr final : public CDialogEx, public IHexTemplates {
	public:
		struct FIELDSDEFPROPS; //Forward declarations.
		struct TEMPLAPPLIED;
		enum class EMenuID : std::uint16_t;
		using IterJSONMember = rapidjson::Value::ConstMemberIterator;
		using UmapCustomTypes = std::unordered_map<std::uint8_t, HexVecFields>;
		using PCTEMPLAPPLIED = const TEMPLAPPLIED*;
		using PCVecFields = const HexVecFields*;

		CHexDlgTemplMgr();
		~CHexDlgTemplMgr();
		auto AddTemplate(const HEXTEMPLATE& stTempl) -> int override;
		void ApplyCurr(ULONGLONG ullOffset); //Apply currently selected template to offset.
		int ApplyTemplate(ULONGLONG ullOffset, int iTemplateID)override; //Apply template to a given offset.
		void DisapplyAll()override;
		void DisapplyByID(int iAppliedID)override; //Disapply template with the given AppliedID.
		void DisapplyByOffset(ULONGLONG ullOffset)override;
		[[nodiscard]] auto GetDlgItemHandle(EHexDlgItem eItem)const->HWND;
		[[nodiscard]] bool HasApplied()const;
		[[nodiscard]] bool HasCurrent()const;
		[[nodiscard]] bool HasTemplates()const;
		[[nodiscard]] auto HitTest(ULONGLONG ullOffset)const->PCHEXTEMPLFIELD; //Template hittest by offset.
		void Initialize(IHexCtrl* pHexCtrl);
		[[nodiscard]] bool IsTooltips()const;
		int LoadTemplate(const wchar_t* pFilePath)override; //Returns loaded template ID on success, zero otherwise.
		void SetDlgProperties(std::uint64_t u64Flags);
		void ShowTooltips(bool fShow)override;
		BOOL ShowWindow(int nCmdShow);
		void UnloadAll()override;
		void UpdateData();

		//Static functions.
		[[nodiscard]] static bool JSONParseFields(IterJSONMember iterFieldsArray, HexVecFields& refVecFields,
						const FIELDSDEFPROPS& refDefault, UmapCustomTypes& umapCustomT, int* pOffset = nullptr);
		[[nodiscard]] static auto JSONEndianness(const rapidjson::Value& value) -> std::optional<bool>;
		[[nodiscard]] static auto JSONColors(const rapidjson::Value& value, const char* pszColorName) -> std::optional<COLORREF>;
		[[nodiscard]] static auto CloneTemplate(PCHEXTEMPLATE pTemplate) -> std::unique_ptr<HEXTEMPLATE>;
		static LRESULT CALLBACK TreeSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
			UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	private:
		void DoDataExchange(CDataExchange* pDX)override;
		void EnableDynamicLayoutHelper(bool fEnable);
		[[nodiscard]] auto GetAppliedFromItem(HTREEITEM hTreeItem) -> PCTEMPLAPPLIED;
		[[nodiscard]] auto GetHexCtrl()const->IHexCtrl*;
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
		afx_msg auto OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) -> HBRUSH;
		afx_msg void OnCheckHex();
		afx_msg void OnCheckSwapEndian();
		afx_msg void OnCheckShowTt();
		afx_msg void OnCheckMin();
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
		template<typename T> requires TSize1248<T>
		[[nodiscard]] bool SetDataNUMBER(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const;
		[[nodiscard]] bool SetDataTime32(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const;
		[[nodiscard]] bool SetDataTime64(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const;
		[[nodiscard]] bool SetDataFILETIME(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const;
		[[nodiscard]] bool SetDataSYSTEMTIME(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const;
		[[nodiscard]] bool SetDataGUID(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const;
		void SetDlgButtonsState(); //Enable/disable button states depending on templates existence.
		void SetHexSelByField(PCHEXTEMPLFIELD pField);
		template <TSize1248 T> void SetTData(T tData, ULONGLONG ullOffset, bool fShouldSwap)const;
		void ShowListDataBool(LPWSTR pwsz, std::uint8_t u8Data)const;
		template<typename T> requires TSize1248<T>
		void ShowListDataNUMBER(LPWSTR pwsz, T tData, bool fShouldSwap)const;
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
		static constexpr auto m_iIDListApplFieldType { 0 }; //ID of the Type field in the m_pList.
		static constexpr auto m_iIDListApplFieldData { 4 };
		static constexpr auto m_iIDListApplFieldDescr { 6 };
		static constexpr auto m_iIDListApplFieldClrs { 7 };
		IHexCtrl* m_pHexCtrl { };
		std::vector<std::unique_ptr<HEXTEMPLATE>> m_vecTemplates;      //Loaded Templates.
		std::vector<std::unique_ptr<TEMPLAPPLIED>> m_vecTemplatesAppl; //Currently Applied Templates.
		CComboBox m_comboTemplates; //Currently available templates list.
		CEdit m_editOffset;         //"Offset" edit box.
		CButton m_btnMin;           //Check-box min-max.
		CButton m_btnShowTT;        //Check-box "Show tooltips".
		CButton m_btnHglSel;        //Check-box "Highlight selected".
		CButton m_btnHex;           //Check-box "Hex numbers".
		CButton m_btnSwapEndian;    //Check-box "Swap endian".
		CWnd m_wndStaticOffset;     //Static text "Template offset:".
		CWnd m_wndStaticSize;       //Static text Template size:".
		HBITMAP m_hBITMAPMin { };   //Bitmap for the min checkbox.
		HBITMAP m_hBITMAPMax { };   //Bitmap for the max checkbox.
		LISTEX::IListExPtr m_pList { LISTEX::CreateListEx() };
		CTreeCtrl m_treeApplied;
		CMenu m_menuTree;           //Menu for the tree control.
		CMenu m_menuHdr;            //Menu for the list header.
		PCTEMPLAPPLIED m_pAppliedCurr { }; //Currently selected template in the applied Tree.
		PCVecFields m_pVecFieldsCurr { };  //Currently selected Fields vector.
		HTREEITEM m_hTreeCurrParent { };   //Currently selected Tree node's parent.
		std::uint64_t m_u64Flags { };      //Data from SetDlgProperties.
		DWORD m_dwDateFormat { };          //Date format.
		int m_iDynLayoutMinY { };          //For DynamicLayout::SetMinSize.
		wchar_t m_wchDateSepar { };        //Date separator.
		bool m_fCurInSplitter { };         //Indicates that mouse cursor is in the splitter area.
		bool m_fLMDownResize { };          //Left mouse pressed in splitter area to resize.
		bool m_fListGuardEvent { false };  //To not proceed with OnListItemChanged, same as pTree->action == TVC_UNKNOWN.
		bool m_fTooltips { true };         //Show tooltips or not.
	};
}