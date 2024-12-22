/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../../dep/rapidjson/rapidjson-amalgam.h"
#include "../../HexCtrl.h"
#include <unordered_map>

import HEXCTRL.HexUtility;

namespace HEXCTRL::INTERNAL {
	class CHexDlgTemplMgr final : public IHexTemplates {
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
		void CreateDlg();
		void DisapplyAll()override;
		void DisapplyByID(int iAppliedID)override; //Disapply template with the given AppliedID.
		void DisapplyByOffset(ULONGLONG ullOffset)override;
		[[nodiscard]] auto GetDlgItemHandle(EHexDlgItem eItem)const->HWND;
		[[nodiscard]] auto GetHWND()const->HWND;
		[[nodiscard]] bool HasApplied()const;
		[[nodiscard]] bool HasCurrent()const;
		[[nodiscard]] bool HasTemplates()const;
		[[nodiscard]] auto HitTest(ULONGLONG ullOffset)const->PCHEXTEMPLFIELD; //Template hittest by offset.
		void Initialize(IHexCtrl* pHexCtrl);
		[[nodiscard]] bool IsTooltips()const;
		int LoadTemplate(const wchar_t* pFilePath)override; //Returns loaded template ID on success, zero otherwise.
		[[nodiscard]] bool PreTranslateMsg(MSG* pMsg);
		[[nodiscard]] auto ProcessMsg(const MSG& stMsg) -> INT_PTR;
		void SetDlgProperties(std::uint64_t u64Flags);
		void ShowTooltips(bool fShow)override;
		void ShowWindow(int iCmdShow);
		void UnloadAll()override;
		void UpdateData();

		//Static functions.
		[[nodiscard]] static bool JSONParseFields(IterJSONMember iterFieldsArray, HexVecFields& refVecFields,
			const FIELDSDEFPROPS& refDefault, UmapCustomTypes& umapCustomT, int* pOffset = nullptr);
		[[nodiscard]] static auto JSONEndianness(const rapidjson::Value& value) -> std::optional<bool>;
		[[nodiscard]] static auto JSONColors(const rapidjson::Value& value, const char* pszColorName) -> std::optional<COLORREF>;
		[[nodiscard]] static auto CloneTemplate(PCHEXTEMPLATE pTemplate) -> std::unique_ptr<HEXTEMPLATE>;
		static auto CALLBACK TreeSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
			UINT_PTR uIdSubclass, DWORD_PTR dwRefData)->LRESULT;
	private:
		[[nodiscard]] auto GetAppliedFromItem(HTREEITEM hTreeItem) -> PCTEMPLAPPLIED;
		[[nodiscard]] auto GetHexCtrl()const->IHexCtrl*;
		[[nodiscard]] auto GetIDForNewTemplate()const->int;
		[[nodiscard]] auto GetTemplate(int iTemplateID)const->PCHEXTEMPLATE;
		[[nodiscard]] bool IsHglSel()const;
		[[nodiscard]] bool IsMinimized()const;
		[[nodiscard]] bool IsNoEsc()const;
		[[nodiscard]] bool IsShowAsHex()const;
		[[nodiscard]] bool IsSwapEndian()const;
		auto OnActivate(const MSG& stMsg) -> INT_PTR;
		void OnBnLoadTemplate();
		void OnBnUnloadTemplate();
		void OnBnRandomizeColors();
		void OnBnApply();
		void OnCancel();
		auto OnCommand(const MSG& stMsg) -> INT_PTR;
		auto OnCtlClrStatic(const MSG& stMsg) -> INT_PTR;
		void OnCheckHex();
		void OnCheckSwapEndian();
		void OnCheckShowTt();
		void OnCheckMin();
		auto OnClose() -> INT_PTR;
		auto OnDestroy() -> INT_PTR;
		auto OnDrawItem(const MSG& stMsg) -> INT_PTR;
		auto OnInitDialog(const MSG& stMsg) -> INT_PTR;
		auto OnLButtonDown(const MSG& stMsg) -> INT_PTR;
		auto OnLButtonUp(const MSG& stMsg) -> INT_PTR;
		auto OnMeasureItem(const MSG& stMsg) -> INT_PTR;
		auto OnMouseMove(const MSG& stMsg) -> INT_PTR;
		auto OnNotify(const MSG& stMsg) -> INT_PTR;
		void OnNotifyListDblClick(NMHDR* pNMHDR);
		void OnNotifyListEditBegin(NMHDR* pNMHDR);
		void OnNotifyListEnterPressed(NMHDR* pNMHDR);
		void OnNotifyListGetDispInfo(NMHDR* pNMHDR);
		void OnNotifyListGetColor(NMHDR* pNMHDR);
		void OnNotifyListHdrRClick(NMHDR* pNMHDR);
		void OnNotifyListItemChanged(NMHDR* pNMHDR);
		void OnNotifyListRClick(NMHDR* pNMHDR);
		void OnNotifyListSetData(NMHDR* pNMHDR);
		void OnNotifyTreeGetDispInfo(NMHDR* pNMHDR);
		void OnNotifyTreeItemChanged(NMHDR* pNMHDR);
		void OnNotifyTreeRClick(NMHDR* pNMHDR);
		void OnOK();
		auto OnSize(const MSG& stMsg) -> INT_PTR;
		void OnTemplateLoadUnload(int iTemplateID, bool fLoad);
		void RandomizeTemplateColors(int iTemplateID);
		void RedrawHexCtrl();
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
	private:
		static constexpr auto m_iIDListColType { 0 };  //ID of the Type column in the m_pList.
		static constexpr auto m_iIDListColData { 4 };  //Data.
		static constexpr auto m_iIDListColDescr { 6 }; //Description.
		static constexpr auto m_iIDListColClrs { 7 };  //Colors.
		wnd::CWnd m_Wnd;             //Main window.
		wnd::CWnd m_WndStatOffset;   //Static text "Template offset:".
		wnd::CWnd m_WndStatSize;     //Static text Template size:".
		wnd::CWnd m_WndEditOffset;   //"Offset" edit box.
		wnd::CWndBtn m_WndBtnShowTT; //Check-box "Show tooltips".
		wnd::CWndBtn m_WndBtnMin;    //Check-box min-max.
		wnd::CWndBtn m_WndBtnHglSel; //Check-box "Highlight selected".
		wnd::CWndBtn m_WndBtnHex;    //Check-box "Hex numbers".
		wnd::CWndBtn m_WndBtnEndian; //Check-box "Swap endian".
		wnd::CWndCombo m_WndCmbTemplates; //Currently available templates list.
		wnd::CWndTree m_WndTree;
		wnd::CMenu m_MenuTree;       //Menu for the tree control.
		wnd::CMenu m_MenuHdr;        //Menu for the list header.
		wnd::CDynLayout m_DynLayout;
		IHexCtrl* m_pHexCtrl { };
		std::uint64_t m_u64Flags { };      //Data from SetDlgProperties.
		std::vector<std::unique_ptr<HEXTEMPLATE>> m_vecTemplates;      //Loaded Templates.
		std::vector<std::unique_ptr<TEMPLAPPLIED>> m_vecTemplatesAppl; //Currently Applied Templates.
		LISTEX::IListExPtr m_pList { LISTEX::CreateListEx() };
		PCTEMPLAPPLIED m_pAppliedCurr { }; //Currently selected template in the applied Tree.
		PCVecFields m_pVecFieldsCurr { };  //Currently selected Fields vector.
		HTREEITEM m_hTreeCurrParent { };   //Currently selected Tree node's parent.
		HBITMAP m_hBmpMin { };             //Bitmap for the min checkbox.
		HBITMAP m_hBmpMax { };             //Bitmap for the max checkbox.
		DWORD m_dwDateFormat { };          //Date format.
		int m_iDynLayoutMinY { };          //For DynamicLayout::SetMinSize.
		wchar_t m_wchDateSepar { };        //Date separator.
		bool m_fCurInSplitter { };         //Indicates that mouse cursor is in the splitter area.
		bool m_fLMDownResize { };          //Left mouse pressed in splitter area to resize.
		bool m_fListGuardEvent { false };  //To not proceed with OnListItemChanged, same as pTree->action == TVC_UNKNOWN.
		bool m_fTooltips { true };         //Show tooltips or not.
	};
}