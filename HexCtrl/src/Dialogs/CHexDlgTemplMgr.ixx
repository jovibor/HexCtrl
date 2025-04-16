/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
module;
#include <SDKDDKVer.h>
#include "../../dep/rapidjson/rapidjson-amalgam.h"
#include "../../res/HexCtrlRes.h"
#include "../../HexCtrl.h"
#include <ShObjIdl.h>
#include <algorithm>
#include <commctrl.h>
#include <format>
#include <fstream>
#include <numeric>
#include <optional>
#include <random>
#include <unordered_map>
export module HEXCTRL:CHexDlgTemplMgr;

import :HexUtility;

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
		auto AddTemplate(const HEXTEMPLATE& stTempl) -> int override;
		void ApplyCurr(ULONGLONG ullOffset); //Apply currently selected template to offset.
		int ApplyTemplate(ULONGLONG ullOffset, int iTemplateID)override; //Apply template to a given offset.
		void CreateDlg();
		void DestroyDlg();
		void DisapplyAll()override;
		void DisapplyByID(int iAppliedID)override; //Disapply template with the given AppliedID.
		void DisapplyByOffset(ULONGLONG ullOffset)override;
		[[nodiscard]] auto GetDlgItemHandle(EHexDlgItem eItem)const->HWND;
		[[nodiscard]] auto GetHWND()const->HWND;
		[[nodiscard]] bool HasApplied()const;
		[[nodiscard]] bool HasCurrent()const;
		[[nodiscard]] bool HasTemplates()const;
		[[nodiscard]] auto HitTest(ULONGLONG ullOffset)const->PCHEXTEMPLFIELD; //Template hittest by offset.
		void Initialize(IHexCtrl* pHexCtrl, HINSTANCE hInstRes);
		[[nodiscard]] bool IsTooltips()const;
		int LoadTemplate(const wchar_t* pFilePath)override; //Returns loaded template ID on success, zero otherwise.
		[[nodiscard]] bool PreTranslateMsg(MSG* pMsg);
		[[nodiscard]] auto ProcessMsg(const MSG& msg) -> INT_PTR;
		void SetDlgProperties(std::uint64_t u64Flags);
		void ShowTooltips(bool fShow)override;
		void ShowWindow(int iCmdShow);
		void UnloadAll()override;
		void UpdateData();

		//Static functions.
		[[nodiscard]] static bool JSONParseFields(IterJSONMember itFieldsArray, HexVecFields& vecFields,
			const FIELDSDEFPROPS& defProps, UmapCustomTypes& umapCustomT, int* pOffset = nullptr);
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
		auto OnActivate(const MSG& msg) -> INT_PTR;
		void OnBnLoadTemplate();
		void OnBnUnloadTemplate();
		void OnBnRandomizeColors();
		void OnBnApply();
		void OnCancel();
		auto OnCommand(const MSG& msg) -> INT_PTR;
		auto OnCtlClrStatic(const MSG& msg) -> INT_PTR;
		void OnCheckHex();
		void OnCheckSwapEndian();
		void OnCheckShowTt();
		void OnCheckMin();
		auto OnClose() -> INT_PTR;
		auto OnDestroy() -> INT_PTR;
		auto OnDrawItem(const MSG& msg) -> INT_PTR;
		auto OnInitDialog(const MSG& msg) -> INT_PTR;
		auto OnLButtonDown(const MSG& msg) -> INT_PTR;
		auto OnLButtonUp(const MSG& msg) -> INT_PTR;
		auto OnMeasureItem(const MSG& msg) -> INT_PTR;
		auto OnMouseMove(const MSG& msg) -> INT_PTR;
		auto OnNotify(const MSG& msg) -> INT_PTR;
		void OnNotifyListDblClick(NMHDR* pNMHDR);
		void OnNotifyListEditBegin(NMHDR* pNMHDR);
		void OnNotifyListEnterPressed(NMHDR* pNMHDR);
		void OnNotifyListGetColor(NMHDR* pNMHDR);
		void OnNotifyListGetDispInfo(NMHDR* pNMHDR);
		void OnNotifyListHdrRClick(NMHDR* pNMHDR);
		void OnNotifyListItemChanged(NMHDR* pNMHDR);
		void OnNotifyListRClick(NMHDR* pNMHDR);
		void OnNotifyListSetData(NMHDR* pNMHDR);
		void OnNotifyTreeGetDispInfo(NMHDR* pNMHDR);
		void OnNotifyTreeItemChanged(NMHDR* pNMHDR);
		void OnNotifyTreeRClick(NMHDR* pNMHDR);
		void OnOK();
		auto OnSize(const MSG& msg) -> INT_PTR;
		void OnTemplateLoadUnload(int iTemplateID, bool fLoad);
		void RandomizeTemplateColors(int iTemplateID);
		void RedrawHexCtrl();
		void RemoveNodesWithTemplateID(int iTemplateID);
		void RemoveNodeWithAppliedID(int iAppliedID);
		[[nodiscard]] bool SetDataBool(LPCWSTR pwszText, ULONGLONG ullOffset)const;
		template<typename T> requires ut::TSize1248<T>
		[[nodiscard]] bool SetDataNUMBER(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const;
		[[nodiscard]] bool SetDataTime32(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const;
		[[nodiscard]] bool SetDataTime64(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const;
		[[nodiscard]] bool SetDataFILETIME(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const;
		[[nodiscard]] bool SetDataSYSTEMTIME(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const;
		[[nodiscard]] bool SetDataGUID(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const;
		void SetDlgButtonsState(); //Enable/disable button states depending on templates existence.
		void SetHexSelByField(PCHEXTEMPLFIELD pField);
		template <ut::TSize1248 T> void SetTData(T tData, ULONGLONG ullOffset, bool fShouldSwap)const;
		void ShowListDataBool(LPWSTR pwsz, std::uint8_t u8Data)const;
		template<typename T> requires ut::TSize1248<T>
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
		enum EListColumns : std::int8_t {
			COL_TYPE = 0, COL_NAME = 1, COL_OFFSET = 2, COL_SIZE = 3,
			COL_DATA = 4, COL_ENDIAN = 5, COL_DESCR = 6, COL_COLORS = 7
		};
		HINSTANCE m_hInstRes { };
		wnd::CWnd m_Wnd;               //Main window.
		wnd::CWnd m_WndStatOffset;     //Static text "Template offset:".
		wnd::CWnd m_WndStatSize;       //Static text Template size:".
		wnd::CWndEdit m_WndEditOffset; //"Offset" edit box.
		wnd::CWndBtn m_WndBtnShowTT;   //Check-box "Show tooltips".
		wnd::CWndBtn m_WndBtnMin;      //Check-box min-max.
		wnd::CWndBtn m_WndBtnHglSel;   //Check-box "Highlight selected".
		wnd::CWndBtn m_WndBtnHex;      //Check-box "Hex numbers".
		wnd::CWndBtn m_WndBtnEndian;   //Check-box "Swap endian".
		wnd::CWndCombo m_WndCmbTempl;  //Currently available templates list.
		wnd::CWndTree m_WndTree;       //Tree control.
		wnd::CMenu m_MenuTree;         //Menu for the tree control.
		wnd::CMenu m_MenuHdr;          //Menu for the list header.
		wnd::CDynLayout m_DynLayout;
		LISTEX::CListEx m_ListEx;
		std::vector<std::unique_ptr<HEXTEMPLATE>> m_vecTemplates;      //Loaded Templates.
		std::vector<std::unique_ptr<TEMPLAPPLIED>> m_vecTemplatesAppl; //Currently Applied Templates.
		IHexCtrl* m_pHexCtrl { };
		PCTEMPLAPPLIED m_pAppliedCurr { }; //Currently selected template in the applied Tree.
		PCVecFields m_pVecFieldsCurr { };  //Currently selected Fields vector.
		HTREEITEM m_hTreeCurrParent { };   //Currently selected Tree node's parent.
		HBITMAP m_hBmpMin { };             //Bitmap for the min checkbox.
		HBITMAP m_hBmpMax { };             //Bitmap for the max checkbox.
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

using namespace HEXCTRL::INTERNAL;

struct CHexDlgTemplMgr::TEMPLAPPLIED {
	ULONGLONG     ullOffset { };  //Offset, where to apply a template.
	PCHEXTEMPLATE pTemplate { };  //Template pointer.
	int           iAppliedID { }; //Applied/runtime ID, assigned by framework. Any template can be applied more than once.
};

enum class CHexDlgTemplMgr::EMenuID : std::uint16_t {
	IDM_TREE_DISAPPLY = 0x8000, IDM_TREE_DISAPPLYALL,
	IDM_LIST_HDR_TYPE, IDM_LIST_HDR_NAME, IDM_LIST_HDR_OFFSET, IDM_LIST_HDR_SIZE,
	IDM_LIST_HDR_DATA, IDM_LIST_HDR_ENDIANNESS, IDM_LIST_HDR_DESCRIPTION, IDM_LIST_HDR_COLORS
};

struct CHexDlgTemplMgr::FIELDSDEFPROPS { //Helper struct for convenient argument passing through recursive fields' parsing.
	HEXCOLOR        stClr;
	PCHEXTEMPLATE   pTemplate { }; //Same for all fields.
	PCHEXTEMPLFIELD pFieldParent { };
	bool            fBigEndian { false };
};

auto CHexDlgTemplMgr::AddTemplate(const HEXTEMPLATE& stTempl)->int
{
	auto pClonedTemplate = CloneTemplate(&stTempl);
	const auto iNewTemplateID = GetIDForNewTemplate();
	pClonedTemplate->iTemplateID = iNewTemplateID;
	m_vecTemplates.emplace_back(std::move(pClonedTemplate));
	OnTemplateLoadUnload(iNewTemplateID, true);

	return iNewTemplateID;
}

void CHexDlgTemplMgr::ApplyCurr(ULONGLONG ullOffset)
{
	if (!m_Wnd.IsWindow())
		return;

	const auto iIndex = m_WndCmbTempl.GetCurSel();
	if (iIndex == CB_ERR)
		return;

	const auto iTemplateID = static_cast<int>(m_WndCmbTempl.GetItemData(iIndex));
	ApplyTemplate(ullOffset, iTemplateID);
}

int CHexDlgTemplMgr::ApplyTemplate(ULONGLONG ullOffset, int iTemplateID)
{
	const auto pTemplate = GetTemplate(iTemplateID);
	if (pTemplate == nullptr)
		return 0;

	auto iAppliedID = 1; //AppliedID starts at 1.
	if (const auto it = std::max_element(m_vecTemplatesAppl.begin(), m_vecTemplatesAppl.end(),
		[](const std::unique_ptr<TEMPLAPPLIED>& p1, const std::unique_ptr<TEMPLAPPLIED>& p2) {
			return p1->iAppliedID < p2->iAppliedID; }); it != m_vecTemplatesAppl.end()) {
		iAppliedID = it->get()->iAppliedID + 1; //Increasing next AppliedID by 1.
	}

	const auto pApplied = m_vecTemplatesAppl.emplace_back(
		std::make_unique<TEMPLAPPLIED>(ullOffset, pTemplate, iAppliedID)).get();

	//Tree root node.
	TVINSERTSTRUCTW tvi { .hParent = TVI_ROOT, .itemex { .mask = TVIF_CHILDREN | TVIF_TEXT | TVIF_PARAM,
		.pszText = LPSTR_TEXTCALLBACK, .cChildren = static_cast<int>(pTemplate->vecFields.size()),
		.lParam = reinterpret_cast<LPARAM>(pApplied) } }; //Tree root node has PCTEMPLAPPLIED ptr.
	const auto hTreeRootNode = m_WndTree.InsertItem(&tvi);

	const auto lmbFill = [&](HTREEITEM hTreeRoot, const HexVecFields& refVecFields)->void {
		const auto _lmbFill = [&](const auto& lmbSelf, HTREEITEM hTreeRoot, const HexVecFields& refVecFields)->void {
			for (const auto& pField : refVecFields) {
				tvi.hParent = hTreeRoot;
				tvi.itemex.cChildren = static_cast<int>(pField->vecNested.size());
				tvi.itemex.lParam = reinterpret_cast<LPARAM>(pField.get()); //Tree child nodes have PCTEMPLAPPLIED ptr.
				const auto hCurrentRoot = m_WndTree.InsertItem(&tvi);
				if (tvi.itemex.cChildren > 0) {
					lmbSelf(lmbSelf, hCurrentRoot, pField->vecNested);
				}
			}
			};
		_lmbFill(_lmbFill, hTreeRoot, refVecFields);
		};
	lmbFill(hTreeRootNode, pTemplate->vecFields);

	RedrawHexCtrl();

	return iAppliedID;
}

void CHexDlgTemplMgr::CreateDlg()
{
	//m_Wnd is set in the OnInitDialog().
	if (const auto hWnd = ::CreateDialogParamW(m_hInstRes, MAKEINTRESOURCEW(IDD_HEXCTRL_TEMPLMGR),
		m_pHexCtrl->GetWndHandle(EHexWnd::WND_MAIN), wnd::DlgProc<CHexDlgTemplMgr>, reinterpret_cast<LPARAM>(this));
		hWnd == nullptr) {
		ut::DBG_REPORT(L"CreateDialogParamW failed.");
	}
}

void CHexDlgTemplMgr::DestroyDlg()
{
	if (m_Wnd.IsWindow()) {
		m_Wnd.DestroyWindow();
	}
}

void CHexDlgTemplMgr::DisapplyAll()
{
	if (m_Wnd.IsWindow()) { //Dialog must be created and alive to work with its members.
		m_WndTree.DeleteAllItems();
		m_ListEx.SetItemCountEx(0);
		UpdateStaticText();
	}

	m_pAppliedCurr = nullptr;
	m_pVecFieldsCurr = nullptr;
	m_hTreeCurrParent = nullptr;
	m_vecTemplatesAppl.clear();

	RedrawHexCtrl();
}

void CHexDlgTemplMgr::DisapplyByID(int iAppliedID)
{
	if (const auto it = std::find_if(m_vecTemplatesAppl.begin(), m_vecTemplatesAppl.end(),
		[iAppliedID](const std::unique_ptr<TEMPLAPPLIED>& pData) { return pData->iAppliedID == iAppliedID; });
		it != m_vecTemplatesAppl.end()) {
		RemoveNodeWithAppliedID(iAppliedID);
		m_vecTemplatesAppl.erase(it);
		RedrawHexCtrl();
	}
}

void CHexDlgTemplMgr::DisapplyByOffset(ULONGLONG ullOffset)
{
	if (const auto rit = std::find_if(m_vecTemplatesAppl.rbegin(), m_vecTemplatesAppl.rend(),
		[ullOffset](const std::unique_ptr<TEMPLAPPLIED>& pData) {
			return ullOffset >= pData->ullOffset && ullOffset < pData->ullOffset + pData->pTemplate->iSizeTotal; });
			rit != m_vecTemplatesAppl.rend()) {
		RemoveNodeWithAppliedID(rit->get()->iAppliedID);
		m_vecTemplatesAppl.erase(std::next(rit).base());
		RedrawHexCtrl();
	}
}

auto CHexDlgTemplMgr::GetDlgItemHandle(EHexDlgItem eItem)const->HWND
{
	if (!m_Wnd.IsWindow()) {
		return { };
	}

	using enum EHexDlgItem;
	switch (eItem) {
	case TEMPLMGR_CHK_MIN: return m_WndBtnMin;
	case TEMPLMGR_CHK_TT: return m_WndBtnShowTT;
	case TEMPLMGR_CHK_HGL: return m_WndBtnHglSel;
	case TEMPLMGR_CHK_HEX: return m_WndBtnHex;
	case TEMPLMGR_CHK_SWAP: return m_WndBtnEndian;
	default: return { };
	}
}

auto CHexDlgTemplMgr::GetHWND()const->HWND
{
	return m_Wnd;
}

bool CHexDlgTemplMgr::HasApplied()const
{
	return !m_vecTemplatesAppl.empty();
}

bool CHexDlgTemplMgr::HasCurrent()const
{
	return m_Wnd.IsWindow() && HasTemplates();
}

bool CHexDlgTemplMgr::HasTemplates()const
{
	return !m_vecTemplates.empty();
}

auto CHexDlgTemplMgr::HitTest(ULONGLONG ullOffset)const->PCHEXTEMPLFIELD
{
	const auto rit = std::find_if(m_vecTemplatesAppl.rbegin(), m_vecTemplatesAppl.rend(),
		[ullOffset](const std::unique_ptr<TEMPLAPPLIED>& pData) {
			return ullOffset >= pData->ullOffset && ullOffset < pData->ullOffset + pData->pTemplate->iSizeTotal; });
	if (rit == m_vecTemplatesAppl.rend()) {
		return nullptr;
	}

	const auto pApplied = rit->get();
	const auto ullOffsetApplied = pApplied->ullOffset;
	const auto& vecFields = pApplied->pTemplate->vecFields;

	const auto lmbFind = [ullOffset, ullOffsetApplied]
		(const HexVecFields& refVecFields)->PCHEXTEMPLFIELD {
		const auto _lmbFind = [ullOffset, ullOffsetApplied]
			(const auto& lmbSelf, const HexVecFields& refVecFields)->PCHEXTEMPLFIELD {
			for (const auto& pField : refVecFields) {
				if (pField->vecNested.empty()) {
					const auto ullOffsetCurr = ullOffsetApplied + pField->iOffset;
					if (ullOffset < (ullOffsetCurr + pField->iSize)) {
						return pField.get();
					}
				}
				else {
					if (const auto pFieldInner = lmbSelf(lmbSelf, pField->vecNested); pFieldInner != nullptr) {
						return pFieldInner; //Return only if we Hit a pointer in the inner lambda, continue the loop otherwise.
					}
				}
			}
			return nullptr;
			};
			return _lmbFind(_lmbFind, refVecFields);
		};

		return lmbFind(vecFields);
}

void CHexDlgTemplMgr::Initialize(IHexCtrl* pHexCtrl, HINSTANCE hInstRes)
{
	if (pHexCtrl == nullptr || hInstRes == nullptr) {
		ut::DBG_REPORT(L"Initialize == nullptr");
		return;
	}

	m_pHexCtrl = pHexCtrl;
	m_hInstRes = hInstRes;
}

bool CHexDlgTemplMgr::IsTooltips()const
{
	return m_fTooltips;
}

int CHexDlgTemplMgr::LoadTemplate(const wchar_t* pFilePath)
{
	auto pTemplate = LoadFromFile(pFilePath);
	if (pTemplate == nullptr) {
		return 0;
	}

	const auto iNewTemplateID = GetIDForNewTemplate();
	pTemplate->iTemplateID = iNewTemplateID;
	m_vecTemplates.emplace_back(std::move(pTemplate));
	OnTemplateLoadUnload(iNewTemplateID, true);

	return iNewTemplateID;
}

bool CHexDlgTemplMgr::PreTranslateMsg(MSG* pMsg)
{
	return m_Wnd.IsDlgMessage(pMsg);
}

auto CHexDlgTemplMgr::ProcessMsg(const MSG& msg)->INT_PTR
{
	switch (msg.message) {
	case WM_ACTIVATE: return OnActivate(msg);
	case WM_CLOSE: return OnClose();
	case WM_COMMAND: return OnCommand(msg);
	case WM_CTLCOLORSTATIC: return OnCtlClrStatic(msg);
	case WM_DESTROY: return OnDestroy();
	case WM_DRAWITEM: return OnDrawItem(msg);
	case WM_INITDIALOG: return OnInitDialog(msg);
	case WM_LBUTTONDOWN: return OnLButtonDown(msg);
	case WM_LBUTTONUP: return OnLButtonUp(msg);
	case WM_MEASUREITEM: return OnMeasureItem(msg);
	case WM_MOUSEMOVE: return OnMouseMove(msg);
	case WM_NOTIFY: return OnNotify(msg);
	case WM_SIZE: return OnSize(msg);
	default:
		return 0;
	}
}

void CHexDlgTemplMgr::SetDlgProperties(std::uint64_t u64Flags)
{
	m_u64Flags = u64Flags;
}

void CHexDlgTemplMgr::ShowTooltips(bool fShow)
{
	m_fTooltips = fShow;
}

void CHexDlgTemplMgr::ShowWindow(int iCmdShow)
{
	if (!m_Wnd.IsWindow()) {
		CreateDlg();
	}

	m_Wnd.ShowWindow(iCmdShow);
}

void CHexDlgTemplMgr::UnloadAll()
{
	DisapplyAll();

	for (const auto& pTemplate : m_vecTemplates) {
		OnTemplateLoadUnload(pTemplate->iTemplateID, false);
	}
	m_vecTemplates.clear();

	RedrawHexCtrl();
}

void CHexDlgTemplMgr::UpdateData()
{
	if (!m_Wnd.IsWindow() || !m_Wnd.IsWindowVisible()) {
		return;
	}

	m_ListEx.RedrawWindow();
}


//Private methods.

auto CHexDlgTemplMgr::GetAppliedFromItem(HTREEITEM hTreeItem)->PCTEMPLAPPLIED
{
	auto hRoot = hTreeItem;
	while (hRoot != nullptr) { //Root node.
		hTreeItem = hRoot;
		hRoot = m_WndTree.GetNextItem(hTreeItem, TVGN_PARENT);
	}

	return reinterpret_cast<PCTEMPLAPPLIED>(m_WndTree.GetItemData(hTreeItem));
}

auto CHexDlgTemplMgr::GetHexCtrl()const->IHexCtrl*
{
	return m_pHexCtrl;
}

auto CHexDlgTemplMgr::GetIDForNewTemplate()const->int
{
	auto iTemplateID = 1; //TemplateID starts at 1.
	if (const auto it = std::max_element(m_vecTemplates.begin(), m_vecTemplates.end(),
		[](const std::unique_ptr<HEXTEMPLATE>& ref1, const std::unique_ptr<HEXTEMPLATE>& ref2) {
			return ref1->iTemplateID < ref2->iTemplateID; }); it != m_vecTemplates.end()) {
		iTemplateID = it->get()->iTemplateID + 1; //Increasing next Template's ID by 1.
	}

	return iTemplateID;
}

auto CHexDlgTemplMgr::GetTemplate(int iTemplateID)const->PCHEXTEMPLATE
{
	const auto it = std::find_if(m_vecTemplates.begin(), m_vecTemplates.end(),
		[iTemplateID](const std::unique_ptr<HEXTEMPLATE>& pData) { return pData->iTemplateID == iTemplateID; });

	return it != m_vecTemplates.end() ? it->get() : nullptr;
}

bool CHexDlgTemplMgr::IsHglSel()const
{
	return m_WndBtnHglSel.IsChecked();
}

bool CHexDlgTemplMgr::IsMinimized()const
{
	return m_WndBtnMin.IsChecked();
}

bool CHexDlgTemplMgr::IsNoEsc()const
{
	return m_u64Flags & HEXCTRL_FLAG_DLG_NOESC;
}

bool CHexDlgTemplMgr::IsShowAsHex()const
{
	return  m_WndBtnHex.IsChecked();
}

bool CHexDlgTemplMgr::IsSwapEndian()const
{
	return m_WndBtnEndian.IsChecked();
}

auto CHexDlgTemplMgr::OnActivate(const MSG& msg)->INT_PTR
{
	if (m_pHexCtrl == nullptr || !m_pHexCtrl->IsCreated())
		return FALSE;

	const auto nState = LOWORD(msg.wParam);
	if (nState == WA_ACTIVE || nState == WA_CLICKACTIVE) {
		const auto [dwFormat, wchSepar] = m_pHexCtrl->GetDateInfo();
		m_dwDateFormat = dwFormat;
		m_wchDateSepar = wchSepar;
	}

	return FALSE; //Default handler.
}

void CHexDlgTemplMgr::OnBnLoadTemplate()
{
	IFileOpenDialog *pIFOD { };
	if (::CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pIFOD)) != S_OK) {
		ut::DBG_REPORT(L"CoCreateInstance failed.");
		return;
	}

	DWORD dwFlags;
	pIFOD->GetOptions(&dwFlags);
	pIFOD->SetOptions(dwFlags | FOS_FORCEFILESYSTEM | FOS_OVERWRITEPROMPT | FOS_ALLOWMULTISELECT
		| FOS_DONTADDTORECENT | FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST);
	COMDLG_FILTERSPEC arrFilter[] { { .pszName { L"Template files (*.json)" }, .pszSpec { L"*.json" } },
		{ .pszName { L"All files (*.*)" }, .pszSpec { L"*.*" } } };
	pIFOD->SetFileTypes(2, arrFilter);

	if (pIFOD->Show(m_Wnd) != S_OK) //Cancel was pressed.
		return;

	IShellItemArray* pResults { };
	pIFOD->GetResults(&pResults);
	if (pResults == nullptr) {
		ut::DBG_REPORT(L"pResults == nullptr");
		return;
	}

	DWORD dwCount { };
	pResults->GetCount(&dwCount);
	for (auto itFile { 0U }; itFile < dwCount; ++itFile) {
		IShellItem* pItem { };
		pResults->GetItemAt(itFile, &pItem);
		if (pItem == nullptr) {
			ut::DBG_REPORT(L"pItem == nullptr");
			return;
		}

		wchar_t* pwszPath;
		pItem->GetDisplayName(SIGDN_FILESYSPATH, &pwszPath);
		if (LoadTemplate(pwszPath) == 0) {
			std::wstring wstrErr = L"Error loading a template:\n";
			wstrErr += pwszPath;
			std::wstring_view wsvFileName = pwszPath;
			wsvFileName = wsvFileName.substr(wsvFileName.find_last_of('\\') + 1);
			::MessageBoxW(m_Wnd, wstrErr.data(), wsvFileName.data(), MB_ICONERROR);
		}
		pItem->Release();
		::CoTaskMemFree(pwszPath);
	}

	pResults->Release();
	pIFOD->Release();
}

void CHexDlgTemplMgr::OnBnUnloadTemplate()
{
	if (const auto iIndex = m_WndCmbTempl.GetCurSel(); iIndex != CB_ERR) {
		const auto iTemplateID = static_cast<int>(m_WndCmbTempl.GetItemData(iIndex));
		UnloadTemplate(iTemplateID);
	}
}

void CHexDlgTemplMgr::OnBnRandomizeColors()
{
	if (const auto iIndex = m_WndCmbTempl.GetCurSel(); iIndex != CB_ERR) {
		const auto iTemplateID = static_cast<int>(m_WndCmbTempl.GetItemData(iIndex));
		RandomizeTemplateColors(iTemplateID);
	}
}

void CHexDlgTemplMgr::OnBnApply()
{
	if (!GetHexCtrl()->IsDataSet())
		return;

	const auto wstrOffset = m_WndEditOffset.GetWndText();
	if (wstrOffset.empty()) {
		m_WndEditOffset.SetFocus();
		return;
	}

	const auto optOffset = stn::StrToUInt64(wstrOffset);
	if (!optOffset) {
		m_WndEditOffset.SetFocus();
		return;
	}

	const auto iIndex = m_WndCmbTempl.GetCurSel();
	if (iIndex == CB_ERR)
		return;

	const auto iTemplateID = static_cast<int>(m_WndCmbTempl.GetItemData(iIndex));
	ApplyTemplate(GetHexCtrl()->GetOffset(*optOffset, false), iTemplateID);
}

void CHexDlgTemplMgr::OnCancel()
{
	if (IsNoEsc()) //Not closing Dialog on Escape key.
		return;

	OnClose();
}

void CHexDlgTemplMgr::OnCheckHex()
{
	UpdateStaticText();
	m_ListEx.RedrawWindow();
}

void CHexDlgTemplMgr::OnCheckSwapEndian()
{
	m_ListEx.RedrawWindow();
}

void CHexDlgTemplMgr::OnCheckShowTt()
{
	//Overriden IHexTemplates` interface method.
	ShowTooltips(m_WndBtnShowTT.IsChecked());
}

void CHexDlgTemplMgr::OnCheckMin()
{
	static constexpr int arrIDsToHide[] { IDC_HEXCTRL_TEMPLMGR_STAT_AVAIL, IDC_HEXCTRL_TEMPLMGR_COMBO_TEMPLATES,
		IDC_HEXCTRL_TEMPLMGR_BTN_LOAD, IDC_HEXCTRL_TEMPLMGR_BTN_UNLOAD, IDC_HEXCTRL_TEMPLMGR_BTN_RNDCLR,
		IDC_HEXCTRL_TEMPLMGR_STAT_APPLY, IDC_HEXCTRL_TEMPLMGR_EDIT_OFFSET, IDC_HEXCTRL_TEMPLMGR_BTN_APPLY,
		IDC_HEXCTRL_TEMPLMGR_CHK_TT, IDC_HEXCTRL_TEMPLMGR_CHK_HGL, IDC_HEXCTRL_TEMPLMGR_CHK_HEX,
		IDC_HEXCTRL_TEMPLMGR_CHK_SWAP, IDC_HEXCTRL_TEMPLMGR_GRB_TOP };
	static constexpr int arrIDsToMove[] { IDC_HEXCTRL_TEMPLMGR_STAT_OFFSETTXT, IDC_HEXCTRL_TEMPLMGR_STAT_OFFSETNUM,
		IDC_HEXCTRL_TEMPLMGR_STAT_SIZETXT, IDC_HEXCTRL_TEMPLMGR_STAT_SIZENUM };
	static constexpr int arrIDsToResize[] { IDC_HEXCTRL_TEMPLMGR_TREE, IDC_HEXCTRL_TEMPLMGR_LIST };
	const auto fMinimize = IsMinimized();

	for (const auto id : arrIDsToHide) { //Hiding.
		const wnd::CWnd wnd = m_Wnd.GetDlgItem(id);
		wnd.ShowWindow(fMinimize ? SW_HIDE : SW_SHOW);
	}

	//Top Group Box rect.
	const wnd::CWnd wndGrbTop = m_Wnd.GetDlgItem(IDC_HEXCTRL_TEMPLMGR_GRB_TOP);
	const auto iHeightGRB = wndGrbTop.GetClientRect().Height();
	auto hdwp = ::BeginDeferWindowPos(static_cast<int>(std::size(arrIDsToMove) + std::size(arrIDsToResize)));
	for (const auto id : arrIDsToMove) { //Moving.
		const wnd::CWnd wnd = m_Wnd.GetDlgItem(id);
		auto rcWnd = wnd.GetWindowRect();
		m_Wnd.ScreenToClient(rcWnd);
		const auto iNewPosY = fMinimize ? rcWnd.top - iHeightGRB : rcWnd.top + iHeightGRB;
		hdwp = ::DeferWindowPos(hdwp, wnd, nullptr, rcWnd.left, iNewPosY, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
	}

	for (const auto id : arrIDsToResize) { //Resizing.
		const wnd::CWnd wnd = m_Wnd.GetDlgItem(id);
		auto rcWnd = wnd.GetWindowRect();
		rcWnd.top += fMinimize ? -iHeightGRB : iHeightGRB;
		m_Wnd.ScreenToClient(rcWnd);
		hdwp = ::DeferWindowPos(hdwp, wnd, nullptr, rcWnd.left, rcWnd.top, rcWnd.Width(),
			rcWnd.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
	}
	m_DynLayout.Enable(false); //Otherwise DynamicLayout won't know that all dynamic windows have changed.
	::EndDeferWindowPos(hdwp);
	m_DynLayout.Enable(true);

	m_WndBtnMin.SetBitmap(fMinimize ? m_hBmpMax : m_hBmpMin); //Set arrow bitmap to the min-max checkbox.
}

auto CHexDlgTemplMgr::OnClose()->INT_PTR
{
	ShowWindow(SW_HIDE);
	return TRUE;
}

auto CHexDlgTemplMgr::OnCommand(const MSG& msg)->INT_PTR
{
	const auto uCtrlID = LOWORD(msg.wParam); //Control ID or menu ID.
	const auto uCode = HIWORD(msg.wParam);   //Control code, zero for menu.
	const auto hWndCtrl = reinterpret_cast<HWND>(msg.lParam); //Control HWND, zero for menu.

	//IDOK and IDCANCEL don't have HWND in lParam, if send as result of
	//IsDialogMessage and no button with such ID presents in the dialog.
	if (hWndCtrl != nullptr || uCtrlID == IDOK || uCtrlID == IDCANCEL) { //Notifications from controls.
		if (uCode != BN_CLICKED) { return FALSE; }
		switch (uCtrlID) {
		case IDOK: OnOK(); break;
		case IDCANCEL: OnCancel(); break;
		case IDC_HEXCTRL_TEMPLMGR_BTN_APPLY: OnBnApply(); break;
		case IDC_HEXCTRL_TEMPLMGR_BTN_LOAD: OnBnLoadTemplate(); break;
		case IDC_HEXCTRL_TEMPLMGR_BTN_UNLOAD: OnBnUnloadTemplate(); break;
		case IDC_HEXCTRL_TEMPLMGR_BTN_RNDCLR: OnBnRandomizeColors(); break;
		case IDC_HEXCTRL_TEMPLMGR_CHK_TT: OnCheckShowTt(); break;
		case IDC_HEXCTRL_TEMPLMGR_CHK_HEX: OnCheckHex(); break;
		case IDC_HEXCTRL_TEMPLMGR_CHK_SWAP: OnCheckSwapEndian(); break;
		case IDC_HEXCTRL_TEMPLMGR_CHK_MIN: OnCheckMin(); break;
		default: return FALSE;
		}
	}
	else { //Notifications from menus.
		using enum EMenuID;
		switch (static_cast<EMenuID>(uCtrlID)) {
		case IDM_TREE_DISAPPLY:
			if (const auto pApplied = GetAppliedFromItem(m_WndTree.GetSelectedItem()); pApplied != nullptr) {
				DisapplyByID(pApplied->iAppliedID);
			}
			break;
		case IDM_TREE_DISAPPLYALL:DisapplyAll(); break;
		case IDM_LIST_HDR_TYPE:
		case IDM_LIST_HDR_NAME:
		case IDM_LIST_HDR_OFFSET:
		case IDM_LIST_HDR_SIZE:
		case IDM_LIST_HDR_DATA:
		case IDM_LIST_HDR_ENDIANNESS:
		case IDM_LIST_HDR_DESCRIPTION:
		case IDM_LIST_HDR_COLORS:
		{
			const auto fChecked = m_MenuHdr.IsChecked(uCtrlID);
			m_ListEx.HideColumn(uCtrlID - static_cast<int>(IDM_LIST_HDR_TYPE), fChecked);
			m_MenuHdr.CheckMenuItem(uCtrlID, !fChecked);
		}
		break;
		default: return FALSE;
		}
	}

	return TRUE;
}

auto CHexDlgTemplMgr::OnCtlClrStatic(const MSG& msg)->INT_PTR
{
	if (const auto hWndFrom = reinterpret_cast<HWND>(msg.lParam);
		hWndFrom == m_WndStatOffset || hWndFrom == m_WndStatSize) {
		const auto hDC = reinterpret_cast<HDC>(msg.wParam);
		::SetTextColor(hDC, RGB(0, 50, 250));
		::SetBkColor(hDC, ::GetSysColor(COLOR_3DFACE));
		return reinterpret_cast<INT_PTR>(::GetSysColorBrush(COLOR_3DFACE));
	}

	return FALSE; //Default handler.
}

auto CHexDlgTemplMgr::OnDestroy()->INT_PTR
{
	m_MenuTree.DestroyMenu();
	m_MenuHdr.DestroyMenu();
	m_pAppliedCurr = nullptr;
	m_pVecFieldsCurr = nullptr;
	m_hTreeCurrParent = nullptr;
	m_pHexCtrl = nullptr;
	m_u64Flags = { };
	m_DynLayout.RemoveAll();
	::DeleteObject(m_hBmpMin);
	::DeleteObject(m_hBmpMax);

	return TRUE;
}

auto CHexDlgTemplMgr::OnDrawItem(const MSG& msg)->INT_PTR
{
	const auto pDIS = reinterpret_cast<LPDRAWITEMSTRUCT>(msg.lParam);
	if (pDIS->CtlID == static_cast<UINT>(IDC_HEXCTRL_TEMPLMGR_LIST)) {
		m_ListEx.DrawItem(pDIS);
	}

	return TRUE;
}

auto CHexDlgTemplMgr::OnInitDialog(const MSG& msg)->INT_PTR
{
	m_Wnd.Attach(msg.hwnd);
	m_WndStatOffset.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_TEMPLMGR_STAT_OFFSETNUM));
	m_WndStatSize.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_TEMPLMGR_STAT_SIZENUM));
	m_WndEditOffset.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_TEMPLMGR_EDIT_OFFSET));
	m_WndBtnShowTT.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_TEMPLMGR_CHK_TT));
	m_WndBtnMin.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_TEMPLMGR_CHK_MIN));
	m_WndBtnHglSel.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_TEMPLMGR_CHK_HGL));
	m_WndBtnHex.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_TEMPLMGR_CHK_HEX));
	m_WndBtnEndian.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_TEMPLMGR_CHK_SWAP));
	m_WndCmbTempl.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_TEMPLMGR_COMBO_TEMPLATES));
	m_WndTree.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_TEMPLMGR_TREE));

	m_ListEx.Create({ .hWndParent { m_Wnd }, .uID { IDC_HEXCTRL_TEMPLMGR_LIST }, .dwSizeFontList { 10 },
		.dwSizeFontHdr { 10 }, .fDialogCtrl { true } });
	m_ListEx.SetExtendedStyle(LVS_EX_HEADERDRAGDROP);
	m_ListEx.InsertColumn(COL_TYPE, L"Type", LVCFMT_LEFT, 85);
	m_ListEx.InsertColumn(COL_NAME, L"Name", LVCFMT_LEFT, 200);
	m_ListEx.InsertColumn(COL_OFFSET, L"Offset", LVCFMT_LEFT, 50);
	m_ListEx.InsertColumn(COL_SIZE, L"Size", LVCFMT_LEFT, 50);
	m_ListEx.InsertColumn(COL_DATA, L"Data", LVCFMT_LEFT, 120, -1, LVCFMT_LEFT, true);
	m_ListEx.InsertColumn(COL_ENDIAN, L"Endianness", LVCFMT_CENTER, 75, -1, LVCFMT_CENTER);
	m_ListEx.InsertColumn(COL_DESCR, L"Description", LVCFMT_LEFT, 100, -1, LVCFMT_LEFT, true);
	m_ListEx.InsertColumn(COL_COLORS, L"Colors", LVCFMT_LEFT, 57);

	using enum EMenuID;
	m_MenuHdr.CreatePopupMenu();
	m_MenuHdr.AppendString(static_cast<int>(IDM_LIST_HDR_TYPE), L"Type");
	m_MenuHdr.CheckMenuItem(static_cast<int>(IDM_LIST_HDR_TYPE), true);
	m_MenuHdr.AppendString(static_cast<int>(IDM_LIST_HDR_NAME), L"Name");
	m_MenuHdr.CheckMenuItem(static_cast<int>(IDM_LIST_HDR_NAME), true);
	m_MenuHdr.AppendString(static_cast<int>(IDM_LIST_HDR_OFFSET), L"Offset");
	m_MenuHdr.CheckMenuItem(static_cast<int>(IDM_LIST_HDR_OFFSET), true);
	m_MenuHdr.AppendString(static_cast<int>(IDM_LIST_HDR_SIZE), L"Size");
	m_MenuHdr.CheckMenuItem(static_cast<int>(IDM_LIST_HDR_SIZE), true);
	m_MenuHdr.AppendString(static_cast<int>(IDM_LIST_HDR_DATA), L"Data");
	m_MenuHdr.CheckMenuItem(static_cast<int>(IDM_LIST_HDR_DATA), true);
	m_MenuHdr.AppendString(static_cast<int>(IDM_LIST_HDR_ENDIANNESS), L"Endianness");
	m_MenuHdr.CheckMenuItem(static_cast<int>(IDM_LIST_HDR_ENDIANNESS), true);
	m_MenuHdr.AppendString(static_cast<int>(IDM_LIST_HDR_DESCRIPTION), L"Description");
	m_MenuHdr.CheckMenuItem(static_cast<int>(IDM_LIST_HDR_DESCRIPTION), true);
	m_MenuHdr.AppendString(static_cast<int>(IDM_LIST_HDR_COLORS), L"Colors");
	m_MenuHdr.CheckMenuItem(static_cast<int>(IDM_LIST_HDR_COLORS), true);

	m_MenuTree.CreatePopupMenu();
	m_MenuTree.AppendString(static_cast<UINT_PTR>(IDM_TREE_DISAPPLY), L"Disapply template");
	m_MenuTree.AppendString(static_cast<UINT_PTR>(IDM_TREE_DISAPPLYALL), L"Disapply all");

	m_WndEditOffset.SetWndText(L"0x0");
	m_WndBtnShowTT.SetCheck(IsTooltips());
	m_WndBtnHglSel.SetCheck(IsHglSel());
	m_WndBtnHex.SetCheck(IsShowAsHex());

	auto rcTree = m_WndTree.GetWindowRect();
	m_Wnd.ScreenToClient(rcTree);
	m_iDynLayoutMinY = rcTree.top + 5;

	m_DynLayout.SetHost(m_Wnd);
	m_DynLayout.AddItem(IDC_HEXCTRL_TEMPLMGR_LIST, wnd::CDynLayout::MoveNone(), wnd::CDynLayout::SizeHorzAndVert(100, 100));
	m_DynLayout.AddItem(IDC_HEXCTRL_TEMPLMGR_TREE, wnd::CDynLayout::MoveNone(), wnd::CDynLayout::SizeVert(100));
	m_DynLayout.AddItem(IDC_HEXCTRL_TEMPLMGR_GRB_TOP, wnd::CDynLayout::MoveNone(), wnd::CDynLayout::SizeHorz(100));
	m_DynLayout.AddItem(IDC_HEXCTRL_TEMPLMGR_CHK_MIN, wnd::CDynLayout::MoveHorz(100), wnd::CDynLayout::SizeNone());
	m_DynLayout.Enable(true);

	::SetWindowSubclass(m_WndTree, TreeSubclassProc, reinterpret_cast<UINT_PTR>(this), 0);
	SetDlgButtonsState();

	for (const auto& pTemplate : m_vecTemplates) {
		OnTemplateLoadUnload(pTemplate->iTemplateID, true);
	}

	const auto rcWnd = m_WndBtnMin.GetWindowRect();
	m_hBmpMin = static_cast<HBITMAP>(::LoadImageW(m_hInstRes, MAKEINTRESOURCEW(IDB_HEXCTRL_SCROLL_ARROW),
		IMAGE_BITMAP, rcWnd.Width(), rcWnd.Width(), 0));

	//Flipping m_hBmpMin bits vertically and creating m_hBmpMax bitmap.
	BITMAP stBMP { };
	::GetObjectW(m_hBmpMin, sizeof(BITMAP), &stBMP); //stBMP.bmBits is nullptr here.
	const auto dwWidth = static_cast<DWORD>(stBMP.bmWidth);
	const auto dwHeight = static_cast<DWORD>(stBMP.bmHeight);
	const auto dwPixels = dwWidth * dwHeight;
	const auto dwBytesBmp = stBMP.bmWidthBytes * stBMP.bmHeight;
	const auto pPixelsOrig = std::make_unique<COLORREF[]>(dwPixels);
	::GetBitmapBits(m_hBmpMin, dwBytesBmp, pPixelsOrig.get());
	for (auto itWidth = 0UL; itWidth < dwWidth; ++itWidth) { //Flip matrix' columns (flip vert).
		for (auto itHeight = 0UL, itHeightBack = dwHeight - 1; itHeight < itHeightBack; ++itHeight, --itHeightBack) {
			std::swap(pPixelsOrig[(itHeight * dwHeight) + itWidth], pPixelsOrig[(itHeightBack * dwWidth) + itWidth]);
		}
	}
	m_hBmpMax = ::CreateBitmapIndirect(&stBMP);
	::SetBitmapBits(m_hBmpMax, dwBytesBmp, pPixelsOrig.get());
	m_WndBtnMin.SetBitmap(m_hBmpMin); //Set the min arrow bitmap to the min-max checkbox.

	return TRUE;
}

auto CHexDlgTemplMgr::OnLButtonDown([[maybe_unused]] const MSG& msg)->INT_PTR
{
	if (m_fCurInSplitter) {
		m_fLMDownResize = true;
		m_Wnd.SetCapture();
		m_DynLayout.Enable(false);
	}

	return TRUE;
}

auto CHexDlgTemplMgr::OnLButtonUp([[maybe_unused]] const MSG& msg)->INT_PTR
{
	m_fLMDownResize = false;
	::ReleaseCapture();
	m_DynLayout.Enable(true);

	return TRUE;
}

auto CHexDlgTemplMgr::OnMeasureItem(const MSG& msg)->INT_PTR
{
	const auto pMIS = reinterpret_cast<LPMEASUREITEMSTRUCT>(msg.lParam);
	if (pMIS->CtlID == static_cast<UINT>(IDC_HEXCTRL_TEMPLMGR_LIST)) {
		m_ListEx.MeasureItem(pMIS);
	}

	return TRUE;
}

auto CHexDlgTemplMgr::OnMouseMove(const MSG& msg)->INT_PTR
{
	static constexpr auto iResAreaHalfWidth = 15;       //Area where cursor turns into resizable (IDC_SIZEWE).
	static constexpr auto iWidthBetweenTreeAndList = 1; //Width between tree and list after resizing.
	static constexpr auto iMinTreeWidth = 100;          //Tree control minimum allowed width.
	static const auto hCurResize = static_cast<HCURSOR>(::LoadImageW(nullptr, IDC_SIZEWE, IMAGE_CURSOR, 0, 0, LR_SHARED));
	static const auto hCurArrow = static_cast<HCURSOR>(::LoadImageW(nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED));
	const POINT pt { .x { wnd::GetXLPARAM(msg.lParam) }, .y { wnd::GetYLPARAM(msg.lParam) } };
	const auto hWndList = wnd::CWnd::FromHandle(m_ListEx.GetHWND());
	auto rcList = hWndList.GetWindowRect();
	m_Wnd.ScreenToClient(rcList);

	if (m_fLMDownResize) {
		auto rcTree = m_WndTree.GetWindowRect();
		m_Wnd.ScreenToClient(rcTree);
		rcTree.right = pt.x - iWidthBetweenTreeAndList;
		rcList.left = pt.x;
		if (rcTree.Width() >= iMinTreeWidth) {
			auto hdwp = ::BeginDeferWindowPos(2); //Simultaneously resizing list and tree.
			hdwp = ::DeferWindowPos(hdwp, m_WndTree, nullptr, rcTree.left, rcTree.top,
				rcTree.Width(), rcTree.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
			hdwp = ::DeferWindowPos(hdwp, hWndList, nullptr, rcList.left, rcList.top,
				rcList.Width(), rcList.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
			::EndDeferWindowPos(hdwp);
		}
	}
	else {
		if (const wnd::CRect rcSplitter(rcList.left - iResAreaHalfWidth, rcList.top, rcList.left + iResAreaHalfWidth,
			rcList.bottom);
			rcSplitter.PtInRect(pt)) {
			m_fCurInSplitter = true;
			::SetCursor(hCurResize);
			m_Wnd.SetCapture();
		}
		else {
			m_fCurInSplitter = false;
			::SetCursor(hCurArrow);
			::ReleaseCapture();
		}
	}

	return TRUE;
}

auto CHexDlgTemplMgr::OnNotify(const MSG& msg)->INT_PTR
{
	const auto pNMHDR = reinterpret_cast<NMHDR*>(msg.lParam);
	switch (pNMHDR->idFrom) {
	case IDC_HEXCTRL_TEMPLMGR_LIST:
		switch (pNMHDR->code) {
		case LVN_GETDISPINFOW: OnNotifyListGetDispInfo(pNMHDR); break;
		case LVN_ITEMCHANGED: OnNotifyListItemChanged(pNMHDR); break;
		case NM_DBLCLK: OnNotifyListDblClick(pNMHDR); break;
		case NM_RCLICK: OnNotifyListRClick(pNMHDR); break;
		case NM_RETURN: OnNotifyListEnterPressed(pNMHDR); break;
		case LISTEX::LISTEX_MSG_EDITBEGIN: OnNotifyListEditBegin(pNMHDR); break;
		case LISTEX::LISTEX_MSG_GETCOLOR: OnNotifyListGetColor(pNMHDR); break;
		case LISTEX::LISTEX_MSG_HDRRBTNUP: OnNotifyListHdrRClick(pNMHDR); break;
		case LISTEX::LISTEX_MSG_SETDATA: OnNotifyListSetData(pNMHDR); break;
		default: break;
		}
	case IDC_HEXCTRL_TEMPLMGR_TREE:
		switch (pNMHDR->code) {
		case NM_RCLICK: OnNotifyTreeRClick(pNMHDR); break;
		case TVN_GETDISPINFOW: OnNotifyTreeGetDispInfo(pNMHDR); break;
		case TVN_SELCHANGEDW: OnNotifyTreeItemChanged(pNMHDR); break;
		default: break;
		}
	default: break;
	}

	return TRUE;
}

void CHexDlgTemplMgr::OnNotifyListDblClick(NMHDR* pNMHDR)
{
	const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	const auto iItem = pNMI->iItem;
	if (iItem < 0)
		return;

	const auto& refVec = *m_pVecFieldsCurr;
	if (refVec[iItem]->vecNested.empty())
		return;

	m_fListGuardEvent = true; //To prevent nasty OnListItemChanged to fire after this method ends.
	m_pVecFieldsCurr = &refVec[iItem]->vecNested;

	const auto hItem = TreeItemFromListItem(iItem);
	m_hTreeCurrParent = hItem;
	m_WndTree.Expand(hItem, TVE_EXPAND);

	m_ListEx.SetItemState(-1, 0, LVIS_SELECTED | LVIS_FOCUSED); //Deselect all items.
	m_ListEx.SetItemCountEx(static_cast<int>(m_pVecFieldsCurr->size()));
	m_ListEx.RedrawWindow();
	m_fListGuardEvent = false;
}

void CHexDlgTemplMgr::OnNotifyListEditBegin(NMHDR* pNMHDR)
{
	const auto pLDI = reinterpret_cast<LISTEX::PLISTEXDATAINFO>(pNMHDR);
	const auto& pField = (*m_pVecFieldsCurr)[pLDI->iItem];

	if (!pField->vecNested.empty() || (pField->eType == EHexFieldType::custom_size
		&& pField->iSize != 1 && pField->iSize != 2 && pField->iSize != 4 && pField->iSize != 8)) {
		pLDI->fAllowEdit = false; //Do not show edit-box if clicked on nested fields.
	}
}

void CHexDlgTemplMgr::OnNotifyListEnterPressed(NMHDR* /*pNMHDR*/)
{
	const auto uSelected = m_ListEx.GetSelectedCount();
	if (uSelected != 1)
		return;

	//Simulate DblClick in List with Enter key.
	NMITEMACTIVATE nmii { .iItem = m_ListEx.GetSelectionMark() };
	OnNotifyListDblClick(&nmii.hdr);
}

void CHexDlgTemplMgr::OnNotifyListGetColor(NMHDR* pNMHDR)
{
	static constexpr auto clrTextBluish { RGB(16, 42, 255) };  //Bluish text.
	static constexpr auto clrTextGreenish { RGB(0, 110, 0) };  //Green text.
	static constexpr auto clrBkGreyish { RGB(235, 235, 235) }; //Grayish bk.

	const auto pLCI = reinterpret_cast<LISTEX::PLISTEXCOLORINFO>(pNMHDR);
	const auto& pField = (*m_pVecFieldsCurr)[pLCI->iItem];
	const auto eType = pField->eType;
	using enum EHexFieldType;

	pLCI->stClr.clrText = static_cast<COLORREF>(-1); //Default text color.

	//List items with nested structs colored separately with greyish bk.
	if (!pField->vecNested.empty() && pLCI->iSubItem != COL_COLORS) {
		pLCI->stClr.clrBk = clrBkGreyish;
		if (pLCI->iSubItem == COL_TYPE) {
			if (eType == type_custom) {
				if (pField->uTypeID > 0) {
					pLCI->stClr.clrText = clrTextGreenish;
				}
			}
			else if (eType != custom_size) {
				pLCI->stClr.clrText = clrTextBluish;
			}
		}

		return;
	}

	switch (pLCI->iSubItem) {
	case COL_TYPE:
		if (eType != type_custom && eType != custom_size) {
			pLCI->stClr.clrText = clrTextBluish;
			pLCI->stClr.clrBk = static_cast<COLORREF>(-1); //Default bk color.
			return;
		}
		break;
	case COL_COLORS:
		pLCI->stClr.clrBk = pField->stClr.clrBk;
		pLCI->stClr.clrText = pField->stClr.clrText;
		return;
	default:
		break;
	}
}

void CHexDlgTemplMgr::OnNotifyListGetDispInfo(NMHDR* pNMHDR)
{
	const auto pDispInfo = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
	const auto pItem = &pDispInfo->item;
	if ((pItem->mask & LVIF_TEXT) == 0)
		return;

	const auto& pField = (*m_pVecFieldsCurr)[pItem->iItem];
	const auto wsvFmt = IsShowAsHex() ? L"0x{:X}" : L"{}";
	const auto fShouldSwap = pField->fBigEndian == !IsSwapEndian();
	using enum EHexFieldType;

	//EHexFieldType converter to actual wstring for the list.
	static const std::unordered_map<EHexFieldType, const wchar_t* const> umapETypeToWstr {
		{ custom_size, L"custom size" }, { type_custom, L"custom type" },
		{ type_bool, L"bool" }, { type_int8, L"int8" }, { type_uint8, L"uint8" },
		{ type_int16, L"int16" }, { type_uint16, L"uint16" }, { type_int32, L"int32" },
		{ type_uint32, L"uint32" }, { type_int64, L"int64" }, { type_uint64, L"uint64" },
		{ type_float, L"float" }, { type_double, L"double" }, { type_time32, L"time32_t" },
		{ type_time64, L"time64_t" }, { type_filetime, L"FILETIME" }, { type_systemtime, L"SYSTEMTIME" },
		{ type_guid, L"GUID" }
	};

	switch (pItem->iSubItem) {
	case COL_TYPE:
		if (pField->eType == type_custom) {
			const auto& vecCT = m_pAppliedCurr->pTemplate->vecCustomType;
			if (const auto it = std::find_if(vecCT.begin(), vecCT.end(),
				[uTypeID = pField->uTypeID](const HEXCUSTOMTYPE& ref) {
					return ref.uTypeID == uTypeID; }); it != vecCT.end()) {
				pItem->pszText = const_cast<LPWSTR>(it->wstrTypeName.data());
			}
			else {
				pItem->pszText = const_cast<LPWSTR>(umapETypeToWstr.at(pField->eType));
			}
		}
		else {
			pItem->pszText = const_cast<LPWSTR>(umapETypeToWstr.at(pField->eType));
		}
		break;
	case COL_NAME:
		pItem->pszText = pField->wstrName.data();
		break;
	case COL_OFFSET:
		*std::vformat_to(pItem->pszText, wsvFmt, std::make_wformat_args(pField->iOffset)) = L'\0';
		break;
	case COL_SIZE:
		*std::vformat_to(pItem->pszText, wsvFmt, std::make_wformat_args(pField->iSize)) = L'\0';
		break;
	case COL_DATA:
	{
		if (!m_pHexCtrl->IsDataSet()
			|| m_pAppliedCurr->ullOffset + m_pAppliedCurr->pTemplate->iSizeTotal > m_pHexCtrl->GetDataSize()) //Size overflow check.
			break;

		if (!pField->vecNested.empty()) {
			break; //Doing nothing (no data fetch) for nested structs.
		}

		const auto ullOffset = m_pAppliedCurr->ullOffset + pField->iOffset;
		const auto eType = pField->eType;
		switch (eType) {
		case custom_size: //If field of a custom size we cycling through the size field.
			switch (pField->iSize) {
			case 1:
			{
				const auto bData = ut::GetIHexTData<BYTE>(*m_pHexCtrl, ullOffset);
				*std::vformat_to(pItem->pszText, wsvFmt, std::make_wformat_args(bData)) = L'\0';
			}
			break;
			case 2:
			{
				auto wData = ut::GetIHexTData<WORD>(*m_pHexCtrl, ullOffset);
				if (fShouldSwap) {
					wData = ut::ByteSwap(wData);
				}
				*std::vformat_to(pItem->pszText, wsvFmt, std::make_wformat_args(wData)) = L'\0';
			}
			break;
			case 4:
			{
				auto dwData = ut::GetIHexTData<DWORD>(*m_pHexCtrl, ullOffset);
				if (fShouldSwap) {
					dwData = ut::ByteSwap(dwData);
				}
				*std::vformat_to(pItem->pszText, wsvFmt, std::make_wformat_args(dwData)) = L'\0';
			}
			break;
			case 8:
			{
				auto ullData = ut::GetIHexTData<std::uint64_t>(*m_pHexCtrl, ullOffset);
				if (fShouldSwap) {
					ullData = ut::ByteSwap(ullData);
				}
				*std::vformat_to(pItem->pszText, wsvFmt, std::make_wformat_args(ullData)) = L'\0';
			}
			break;
			default:
				break;
			}
			break;
		case type_bool:
			ShowListDataBool(pItem->pszText, ut::GetIHexTData<std::uint8_t>(*m_pHexCtrl, ullOffset));
			break;
		case type_int8:
			ShowListDataNUMBER(pItem->pszText, ut::GetIHexTData<std::int8_t>(*m_pHexCtrl, ullOffset), false);
			break;
		case type_uint8:
			ShowListDataNUMBER(pItem->pszText, ut::GetIHexTData<std::uint8_t>(*m_pHexCtrl, ullOffset), false);
			break;
		case type_int16:
			ShowListDataNUMBER(pItem->pszText, ut::GetIHexTData<std::int16_t>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_uint16:
			ShowListDataNUMBER(pItem->pszText, ut::GetIHexTData<std::uint16_t>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_int32:
			ShowListDataNUMBER(pItem->pszText, ut::GetIHexTData<std::int32_t>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_uint32:
			ShowListDataNUMBER(pItem->pszText, ut::GetIHexTData<std::uint32_t>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_int64:
			ShowListDataNUMBER(pItem->pszText, ut::GetIHexTData<std::int64_t>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_uint64:
			ShowListDataNUMBER(pItem->pszText, ut::GetIHexTData<std::uint64_t>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_float:
			ShowListDataNUMBER(pItem->pszText, ut::GetIHexTData<float>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_double:
			ShowListDataNUMBER(pItem->pszText, ut::GetIHexTData<double>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_time32:
			ShowListDataTime32(pItem->pszText, ut::GetIHexTData<__time32_t>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_time64:
			ShowListDataTime64(pItem->pszText, ut::GetIHexTData<__time64_t>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_filetime:
			ShowListDataFILETIME(pItem->pszText, ut::GetIHexTData<FILETIME>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_systemtime:
			ShowListDataSYSTEMTIME(pItem->pszText, ut::GetIHexTData<SYSTEMTIME>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_guid:
			ShowListDataGUID(pItem->pszText, ut::GetIHexTData<GUID>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		default:
			break;
		}
	}
	break;
	case COL_ENDIAN:
		*std::vformat_to(pItem->pszText, fShouldSwap ? L"big" : L"little",
			std::make_wformat_args()) = L'\0';
		break;
	case COL_DESCR:
		*std::format_to(pItem->pszText, L"{}", pField->wstrDescr) = L'\0';
		break;
	case COL_COLORS:
		*std::format_to(pItem->pszText, L"#Text") = L'\0';
		break;
	default:
		break;
	}
}

void CHexDlgTemplMgr::OnNotifyListHdrRClick(NMHDR* /*pNMHDR*/)
{
	POINT ptCur;
	::GetCursorPos(&ptCur);
	m_MenuHdr.TrackPopupMenu(ptCur.x, ptCur.y, m_Wnd);
}

void CHexDlgTemplMgr::OnNotifyListItemChanged(NMHDR* pNMHDR)
{
	const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	const auto iItem = pNMI->iItem;
	if (iItem < 0 || m_fListGuardEvent)
		return;

	m_WndTree.SelectItem(TreeItemFromListItem(iItem));
}

void CHexDlgTemplMgr::OnNotifyListRClick(NMHDR* /*pNMHDR*/)
{
}

void CHexDlgTemplMgr::OnNotifyListSetData(NMHDR* pNMHDR)
{
	const auto pLDI = reinterpret_cast<LISTEX::PLISTEXDATAINFO>(pNMHDR);
	const auto pwszText = pLDI->pwszData;
	const auto& pField = (*m_pVecFieldsCurr)[pLDI->iItem];

	if (pLDI->iSubItem == COL_DATA) {
		if (!m_pHexCtrl->IsDataSet()) {
			return;
		}

		const auto ullOffset = m_pAppliedCurr->ullOffset + pField->iOffset;
		const auto fShouldSwap = pField->fBigEndian == !IsSwapEndian();

		bool fSetRet { };
		using enum EHexFieldType;
		switch (pField->eType) {
		case custom_size:
			fSetRet = true;
			switch (pField->iSize) {
			case 1:
				if (const auto opt = stn::StrToUInt8(pwszText); opt) {
					SetTData(*opt, ullOffset, false);
				}
				break;
			case 2:
				if (const auto opt = stn::StrToUInt16(pwszText); opt) {
					SetTData(*opt, ullOffset, false);
				}
				break;
			case 4:
				if (const auto opt = stn::StrToUInt32(pwszText); opt) {
					SetTData(*opt, ullOffset, false);
				}
				break;
			case 8:
				if (const auto opt = stn::StrToUInt64(pwszText); opt) {
					SetTData(*opt, ullOffset, false);
				}
				break;
			default:
				fSetRet = false;
				break;
			}
			break;
		case type_bool:
			fSetRet = SetDataBool(pwszText, ullOffset);
			break;
		case type_int8:
			fSetRet = SetDataNUMBER<std::int8_t>(pwszText, ullOffset, false);
			break;
		case type_uint8:
			fSetRet = SetDataNUMBER<std::uint8_t>(pwszText, ullOffset, false);
			break;
		case type_int16:
			fSetRet = SetDataNUMBER<std::int16_t>(pwszText, ullOffset, fShouldSwap);
			break;
		case type_uint16:
			fSetRet = SetDataNUMBER<std::uint16_t>(pwszText, ullOffset, fShouldSwap);
			break;
		case type_int32:
			fSetRet = SetDataNUMBER<std::int32_t>(pwszText, ullOffset, fShouldSwap);
			break;
		case type_uint32:
			fSetRet = SetDataNUMBER<std::uint32_t>(pwszText, ullOffset, fShouldSwap);
			break;
		case type_int64:
			fSetRet = SetDataNUMBER<std::int64_t>(pwszText, ullOffset, fShouldSwap);
			break;
		case type_uint64:
			fSetRet = SetDataNUMBER<std::uint64_t>(pwszText, ullOffset, fShouldSwap);
			break;
		case type_float:
			fSetRet = SetDataNUMBER<float>(pwszText, ullOffset, fShouldSwap);
			break;
		case type_double:
			fSetRet = SetDataNUMBER<double>(pwszText, ullOffset, fShouldSwap);
			break;
		case type_time32:
			fSetRet = SetDataTime32(pwszText, ullOffset, fShouldSwap);
			break;
		case type_time64:
			fSetRet = SetDataTime64(pwszText, ullOffset, fShouldSwap);
			break;
		case type_filetime:
			fSetRet = SetDataFILETIME(pwszText, ullOffset, fShouldSwap);
			break;
		case type_systemtime:
			fSetRet = SetDataSYSTEMTIME(pwszText, ullOffset, fShouldSwap);
			break;
		case type_guid:
			fSetRet = SetDataGUID(pwszText, ullOffset, fShouldSwap);
			break;
		default:
			break;
		}

		if (!fSetRet) {
			::MessageBoxW(m_Wnd, L"Incorrect input data.", L"Incorrect input", MB_ICONERROR);
			return;
		}
	}
	else if (pLDI->iSubItem == COL_DESCR) {
		pField->wstrDescr = pwszText;
	}

	RedrawHexCtrl();
}

void CHexDlgTemplMgr::OnNotifyTreeGetDispInfo(NMHDR* pNMHDR)
{
	const auto pDispInfo = reinterpret_cast<NMTVDISPINFOW*>(pNMHDR);
	const auto pItem = &pDispInfo->item;
	if ((pItem->mask & TVIF_TEXT) == 0)
		return;

	if (m_WndTree.GetParentItem(pItem->hItem) == nullptr) { //Root node.
		const auto& wstr = reinterpret_cast<PCTEMPLAPPLIED>(m_WndTree.GetItemData(pItem->hItem))->pTemplate->wstrName;
		std::copy(wstr.begin(), wstr.end(), pItem->pszText);
	}
	else {
		const auto& wstr = reinterpret_cast<PCHEXTEMPLFIELD>(m_WndTree.GetItemData(pItem->hItem))->wstrName;
		std::copy(wstr.begin(), wstr.end(), pItem->pszText);
	}
}

void CHexDlgTemplMgr::OnNotifyTreeItemChanged(NMHDR* pNMHDR)
{
	const auto pTree = reinterpret_cast<LPNMTREEVIEWW>(pNMHDR);
	const auto pItem = &pTree->itemNew;

	//Item was changed by m_tree.SelectItem(...) or by m_tree.DeleteItem(...);
	if (pTree->action == TVC_UNKNOWN) {
		if (m_WndTree.GetParentItem(pItem->hItem) != nullptr) {
			const auto dwItemData = m_WndTree.GetItemData(pItem->hItem);
			SetHexSelByField(reinterpret_cast<PCHEXTEMPLFIELD>(dwItemData));
		}
		return;
	}

	m_fListGuardEvent = true; //To not trigger OnListItemChanged on the way.
	bool fRootNodeClick { false };
	PCHEXTEMPLFIELD pFieldCurr { };
	PCVecFields pVecCurrFields { };

	const auto pAppliedPrev = m_pAppliedCurr;
	m_pAppliedCurr = GetAppliedFromItem(pItem->hItem);
	m_ListEx.SetItemState(-1, 0, LVIS_SELECTED | LVIS_FOCUSED); //Deselect all items.

	if (m_WndTree.GetParentItem(pItem->hItem) == nullptr) { //Root item.
		fRootNodeClick = true;
		pVecCurrFields = &m_pAppliedCurr->pTemplate->vecFields; //On Root item click, set pVecCurrFields to Template's main vecFields.
		m_hTreeCurrParent = pItem->hItem;
	}
	else { //Child items.
		pFieldCurr = reinterpret_cast<PCHEXTEMPLFIELD>(m_WndTree.GetItemData(pItem->hItem));
		if (pFieldCurr->pFieldParent == nullptr) {
			if (pFieldCurr->vecNested.empty()) { //On first level child items, set pVecCurrFields to Template's main vecFields.
				pVecCurrFields = &m_pAppliedCurr->pTemplate->vecFields;
				m_hTreeCurrParent = m_WndTree.GetParentItem(pItem->hItem);
			}
			else { //If it's nested Fields vector, set pVecCurrFields to it.
				fRootNodeClick = true;
				pVecCurrFields = &pFieldCurr->vecNested;
				m_hTreeCurrParent = pItem->hItem;
			}
		}
		else { //If it's nested Field, set pVecCurrFields to parent Fields' vecNested.
			if (pFieldCurr->vecNested.empty()) {
				pVecCurrFields = &pFieldCurr->pFieldParent->vecNested;
				m_hTreeCurrParent = m_WndTree.GetParentItem(pItem->hItem);
			}
			else {
				fRootNodeClick = true;
				pVecCurrFields = &pFieldCurr->vecNested;
				m_hTreeCurrParent = pItem->hItem;
			}
		}
	}

	//To not trigger SetItemCountEx, which is slow, every time the Tree item changes.
	//But only if Fields vector changes, or other applied template has been clicked.
	if ((pVecCurrFields != m_pVecFieldsCurr) || (pAppliedPrev != m_pAppliedCurr)) {
		m_pVecFieldsCurr = pVecCurrFields;
		m_ListEx.SetItemCountEx(static_cast<int>(m_pVecFieldsCurr->size()), LVSICF_NOSCROLL);
	}

	UpdateStaticText();

	if (!fRootNodeClick) {
		int iIndexHighlight { 0 }; //Index to highlight in the list.
		auto hChild = m_WndTree.GetNextItem(m_WndTree.GetParentItem(pItem->hItem), TVGN_CHILD);
		while (hChild != pItem->hItem) { //Checking for currently selected item in the tree.
			++iIndexHighlight;
			hChild = m_WndTree.GetNextSiblingItem(hChild);
		}
		m_ListEx.SetItemState(iIndexHighlight, LVIS_SELECTED, LVIS_SELECTED);
		m_ListEx.EnsureVisible(iIndexHighlight, FALSE);
	}

	SetHexSelByField(pFieldCurr);
	m_fListGuardEvent = false;
}

void CHexDlgTemplMgr::OnNotifyTreeRClick(NMHDR* /*pNMHDR*/)
{
	POINT pt;
	::GetCursorPos(&pt);
	POINT ptTree = pt;
	m_WndTree.ScreenToClient(&ptTree);
	const auto hTreeItem = m_WndTree.HitTest(ptTree);
	const auto fHitTest = hTreeItem != nullptr;
	const auto fHasApplied = HasApplied();

	if (hTreeItem != nullptr) {
		m_WndTree.SelectItem(hTreeItem);
	}

	m_MenuTree.EnableMenuItem(static_cast<UINT>(EMenuID::IDM_TREE_DISAPPLY), fHasApplied && fHitTest);
	m_MenuTree.EnableMenuItem(static_cast<UINT>(EMenuID::IDM_TREE_DISAPPLYALL), fHasApplied);
	m_MenuTree.TrackPopupMenu(pt.x, pt.y, m_Wnd);
}

void CHexDlgTemplMgr::OnOK()
{
	const auto wndFocus = wnd::CWnd::GetFocus();
	//When Enter is pressed anywhere in the dialog, and focus is on the m_ListEx,
	//we simulate pressing Enter in the list by sending WM_KEYDOWN/VK_RETURN to it.
	if (const auto hWndList = m_ListEx.GetHWND(); wndFocus == hWndList) {
		::SendMessageW(hWndList, WM_KEYDOWN, VK_RETURN, 0);
	}
	else if (wndFocus == m_WndEditOffset) { //Focus is on the "Offset" edit-box.
		OnBnApply();
	}
}

auto CHexDlgTemplMgr::OnSize(const MSG& msg)->INT_PTR
{
	const auto wWidth = LOWORD(msg.lParam);
	const auto wHeight = HIWORD(msg.lParam);
	m_DynLayout.OnSize(wWidth, wHeight);
	return TRUE;
}

void CHexDlgTemplMgr::OnTemplateLoadUnload(int iTemplateID, bool fLoad)
{
	if (!m_Wnd.IsWindow()) //Only if dialog window is created and alive we proceed with its members.
		return;

	if (fLoad) {
		const auto pTemplate = GetTemplate(iTemplateID);
		if (pTemplate == nullptr)
			return;

		const auto iIndex = m_WndCmbTempl.AddString(pTemplate->wstrName);
		m_WndCmbTempl.SetItemData(iIndex, static_cast<DWORD_PTR>(iTemplateID));
		m_WndCmbTempl.SetCurSel(iIndex);
		SetDlgButtonsState();
	}
	else {
		RemoveNodesWithTemplateID(iTemplateID);

		for (auto iIndex = 0; iIndex < m_WndCmbTempl.GetCount(); ++iIndex) { //Remove Template name from ComboBox.
			if (const auto iItemData = static_cast<int>(m_WndCmbTempl.GetItemData(iIndex)); iItemData == iTemplateID) {
				m_WndCmbTempl.DeleteString(iIndex);
				m_WndCmbTempl.SetCurSel(0);
				break;
			}
		}

		SetDlgButtonsState();
	}
}

void CHexDlgTemplMgr::RandomizeTemplateColors(int iTemplateID)
{
	const auto pTemplate = GetTemplate(iTemplateID);
	if (pTemplate == nullptr)
		return;

	std::mt19937 gen(std::random_device { }());
	std::uniform_int_distribution<unsigned int> distrib(50, 230);

	const auto lmbRndColors = [&distrib, &gen](const HexVecFields& refVecFields) {
		const auto _lmbCount = [&distrib, &gen](const auto& lmbSelf, const HexVecFields& refVecFields)->void {
			for (const auto& pField : refVecFields) {
				if (pField->vecNested.empty()) {
					pField->stClr.clrBk = RGB(distrib(gen), distrib(gen), distrib(gen));
				}
				else { lmbSelf(lmbSelf, pField->vecNested); }
			}
			};
		return _lmbCount(_lmbCount, refVecFields);
		};

	lmbRndColors(pTemplate->vecFields);
	m_ListEx.RedrawWindow();
	RedrawHexCtrl();
}

void CHexDlgTemplMgr::RedrawHexCtrl()
{
	if (m_pHexCtrl != nullptr && m_pHexCtrl->IsDataSet()) {
		m_pHexCtrl->Redraw();
	}
}

void CHexDlgTemplMgr::RemoveNodesWithTemplateID(int iTemplateID)
{
	std::vector<HTREEITEM> vecToRemove;
	if (auto hItem = m_WndTree.GetRootItem(); hItem != nullptr) {
		while (hItem != nullptr) {
			const auto pApplied = reinterpret_cast<PCTEMPLAPPLIED>(m_WndTree.GetItemData(hItem));
			if (pApplied->pTemplate->iTemplateID == iTemplateID) {
				vecToRemove.emplace_back(hItem);
			}

			if (m_pAppliedCurr == pApplied) {
				m_ListEx.SetItemCountEx(0);
				m_ListEx.RedrawWindow();
				m_pAppliedCurr = nullptr;
				m_pVecFieldsCurr = nullptr;
				m_hTreeCurrParent = nullptr;
				UpdateStaticText();
			}

			hItem = m_WndTree.GetNextItem(hItem, TVGN_NEXT); //Get next Root sibling item.
		}
		for (const auto item : vecToRemove) {
			m_WndTree.DeleteItem(item);
		}
	}
}

void CHexDlgTemplMgr::RemoveNodeWithAppliedID(int iAppliedID)
{
	auto hItem = m_WndTree.GetRootItem();
	while (hItem != nullptr) {
		const auto pApplied = reinterpret_cast<PCTEMPLAPPLIED>(m_WndTree.GetItemData(hItem));
		if (pApplied->iAppliedID == iAppliedID) {
			if (m_pAppliedCurr == pApplied) {
				m_ListEx.SetItemCountEx(0);
				m_ListEx.RedrawWindow();
				m_pAppliedCurr = nullptr;
				m_pVecFieldsCurr = nullptr;
				m_hTreeCurrParent = nullptr;
				UpdateStaticText();
			}
			m_WndTree.DeleteItem(hItem);
			break;
		}
		hItem = m_WndTree.GetNextItem(hItem, TVGN_NEXT); //Get next Root sibling item.
	}
}

bool CHexDlgTemplMgr::SetDataBool(LPCWSTR pwszText, ULONGLONG ullOffset) const
{
	if (IsShowAsHex()) {
		if (const auto opt = stn::StrToUInt8(pwszText); opt) {
			SetTData(*opt, ullOffset, false);
			return true;
		}
	}
	else {
		const std::wstring_view wsv { pwszText };
		bool fToSet;
		if (wsv == L"true") {
			fToSet = true;
		}
		else if (wsv == L"false") {
			fToSet = false;
		}
		else
			return false;

		SetTData(fToSet, ullOffset, false);
		return true;
	}
	return false;
}

template<typename T> requires ut::TSize1248<T>
bool CHexDlgTemplMgr::SetDataNUMBER(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const
{
	if (IsShowAsHex()) {
		//Unsigned type in case of float or double.
		using UTF = std::conditional_t<std::is_same_v<T, float>, std::uint32_t, std::uint64_t>;
		//Unsigned type in case of Integral or float/double.
		using UT = typename std::conditional_t<std::is_integral_v<T>, std::make_unsigned<T>, std::type_identity<UTF>>::type;
		if (const auto opt = stn::StrToNum<UT>(pwszText); opt) {
			SetTData(*opt, ullOffset, fShouldSwap);
			return true;
		}
	}
	else {
		if (const auto opt = stn::StrToNum<T>(pwszText); opt) {
			SetTData(*opt, ullOffset, fShouldSwap);
			return true;
		}
	}
	return false;
}

bool CHexDlgTemplMgr::SetDataTime32(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const
{
	if (IsShowAsHex()) {
		if (const auto opt = stn::StrToUInt32(pwszText); opt) {
			SetTData(*opt, ullOffset, fShouldSwap);
			return true;
		}
	}
	else {
		//The number of seconds since midnight January 1st 1970 UTC (32-bit). This wraps on 19 January 2038.
		const auto optSysTime = ut::StringToSystemTime(pwszText, m_dwDateFormat);
		if (!optSysTime)
			return false;

		//Unix times are signed but value before 1st January 1970 is not considered valid.
		//This is apparently because early compilers didn't support unsigned types. _mktime32() has the same limit.
		if (optSysTime->wYear < 1970)
			return false;

		FILETIME ftTime;
		if (::SystemTimeToFileTime(&*optSysTime, &ftTime) == FALSE)
			return false;

		//Convert ticks to seconds and adjust epoch.
		LARGE_INTEGER lTicks { .LowPart { ftTime.dwLowDateTime }, .HighPart { static_cast<LONG>(ftTime.dwHighDateTime) } };
		lTicks.QuadPart /= ut::g_uFTTicksPerSec;
		lTicks.QuadPart -= ut::g_ullUnixEpochDiff;
		if (lTicks.QuadPart >= LONG_MAX)
			return false;

		const auto lTime32 = static_cast<__time32_t>(lTicks.QuadPart);
		SetTData(lTime32, ullOffset, fShouldSwap);

		return true;
	}

	return false;
}

bool CHexDlgTemplMgr::SetDataTime64(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const
{
	if (IsShowAsHex()) {
		if (const auto opt = stn::StrToUInt64(pwszText); opt) {
			SetTData(*opt, ullOffset, fShouldSwap);
			return true;
		}
	}
	else {
		//The number of seconds since midnight January 1st 1970 UTC (32-bit). This wraps on 19 January 2038.
		const auto optSysTime = ut::StringToSystemTime(pwszText, m_dwDateFormat);
		if (!optSysTime)
			return false;

		//Unix times are signed but value before 1st January 1970 is not considered valid.
		//This is apparently because early compilers didn't support unsigned types. _mktime64() has the same limit.
		if (optSysTime->wYear < 1970)
			return false;

		FILETIME ftTime;
		if (::SystemTimeToFileTime(&*optSysTime, &ftTime) == FALSE)
			return false;

		//Convert ticks to seconds and adjust epoch.
		LARGE_INTEGER lTicks { .LowPart { ftTime.dwLowDateTime }, .HighPart { static_cast<LONG>(ftTime.dwHighDateTime) } };
		lTicks.QuadPart /= ut::g_uFTTicksPerSec;
		lTicks.QuadPart -= ut::g_ullUnixEpochDiff;

		const auto llTime64 = static_cast<__time64_t>(lTicks.QuadPart);
		SetTData(llTime64, ullOffset, fShouldSwap);

		return true;
	}

	return false;
}

bool CHexDlgTemplMgr::SetDataFILETIME(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const
{
	if (IsShowAsHex()) {
		if (const auto opt = stn::StrToUInt64(pwszText); opt) {
			SetTData(*opt, ullOffset, fShouldSwap);
			return true;
		}
	}
	else {
		const auto optFileTime = ut::StringToFileTime(pwszText, m_dwDateFormat);
		if (!optFileTime)
			return false;

		const ULARGE_INTEGER uliTime { .LowPart { optFileTime->dwLowDateTime }, .HighPart { optFileTime->dwHighDateTime } };
		SetTData(uliTime.QuadPart, ullOffset, fShouldSwap);
	}

	return false;
}

bool CHexDlgTemplMgr::SetDataSYSTEMTIME(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const
{
	//No Hex representation for this type because size is too big, > ULONGLONG.
	const auto optSysTime = ut::StringToSystemTime(pwszText, m_dwDateFormat);
	if (!optSysTime)
		return false;

	auto stSTime = *optSysTime;
	if (fShouldSwap) {
		stSTime.wYear = ut::ByteSwap(stSTime.wYear);
		stSTime.wMonth = ut::ByteSwap(stSTime.wMonth);
		stSTime.wDayOfWeek = ut::ByteSwap(stSTime.wDayOfWeek);
		stSTime.wDay = ut::ByteSwap(stSTime.wDay);
		stSTime.wHour = ut::ByteSwap(stSTime.wHour);
		stSTime.wMinute = ut::ByteSwap(stSTime.wMinute);
		stSTime.wSecond = ut::ByteSwap(stSTime.wSecond);
		stSTime.wMilliseconds = ut::ByteSwap(stSTime.wMilliseconds);
	}

	ut::SetIHexTData(*m_pHexCtrl, ullOffset, stSTime);

	return true;
}

bool CHexDlgTemplMgr::SetDataGUID(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap) const
{
	//No Hex representation for this type because size is too big, > ULONGLONG.
	GUID stGUID;
	if (::IIDFromString(pwszText, &stGUID) != S_OK)
		return false;

	if (fShouldSwap) {
		stGUID.Data1 = ut::ByteSwap(stGUID.Data1);
		stGUID.Data2 = ut::ByteSwap(stGUID.Data2);
		stGUID.Data3 = ut::ByteSwap(stGUID.Data3);
	}

	ut::SetIHexTData(*m_pHexCtrl, ullOffset, stGUID);

	return true;
}

void CHexDlgTemplMgr::SetDlgButtonsState()
{
	const auto fHasTempl = HasTemplates();
	if (const wnd::CWnd btnApply = m_Wnd.GetDlgItem(IDC_HEXCTRL_TEMPLMGR_BTN_APPLY); !btnApply.IsNull()) {
		btnApply.EnableWindow(fHasTempl);
	}
	if (const wnd::CWnd btnUnload = m_Wnd.GetDlgItem(IDC_HEXCTRL_TEMPLMGR_BTN_UNLOAD); !btnUnload.IsNull()) {
		btnUnload.EnableWindow(fHasTempl);
	}
	if (const wnd::CWnd btnRandom = m_Wnd.GetDlgItem(IDC_HEXCTRL_TEMPLMGR_BTN_RNDCLR); !btnRandom.IsNull()) {
		btnRandom.EnableWindow(fHasTempl);
	}
	m_WndEditOffset.EnableWindow(fHasTempl);
}

void CHexDlgTemplMgr::SetHexSelByField(PCHEXTEMPLFIELD pField)
{
	if (!IsHglSel() || !m_pHexCtrl->IsDataSet() || pField == nullptr || m_pAppliedCurr == nullptr)
		return;

	const auto ullOffset = m_pAppliedCurr->ullOffset + pField->iOffset;
	const auto ullSize = static_cast<ULONGLONG>(pField->iSize);

	m_pHexCtrl->SetSelection({ { ullOffset, ullSize } });
	if (!m_pHexCtrl->IsOffsetVisible(ullOffset)) {
		m_pHexCtrl->GoToOffset(ullOffset, -1);
	}
}

template<ut::TSize1248 T>
void CHexDlgTemplMgr::SetTData(T tData, ULONGLONG ullOffset, bool fShouldSwap)const
{
	if (fShouldSwap) {
		tData = ut::ByteSwap(tData);
	}

	ut::SetIHexTData(*m_pHexCtrl, ullOffset, tData);
}

void CHexDlgTemplMgr::ShowListDataBool(LPWSTR pwsz, std::uint8_t u8Data) const
{
	const auto fBool = static_cast<bool>(u8Data);
	*std::vformat_to(pwsz, IsShowAsHex() ? L"0x{0:02X}" : L"{1}",
		std::make_wformat_args(u8Data, fBool)) = L'\0';
}

template<typename T> requires ut::TSize1248<T>
void CHexDlgTemplMgr::ShowListDataNUMBER(LPWSTR pwsz, T tData, bool fShouldSwap)const
{
	if (fShouldSwap) {
		tData = ut::ByteSwap(tData);
	}

	if (IsShowAsHex()) {
		//Unsigned type in case of float or double.
		using UTF = std::conditional_t<std::is_same_v<T, float>, std::uint32_t, std::uint64_t>;
		//Unsigned type in case of Integral or float/double.
		using UT = typename std::conditional_t<std::is_integral_v<T>, std::make_unsigned<T>, std::type_identity<UTF>>::type;
		UT utData;
		if constexpr (std::is_same_v<T, float>) {
			utData = std::bit_cast<std::uint32_t>(tData);
		}
		else if constexpr (std::is_same_v<T, double>) {
			utData = std::bit_cast<std::uint64_t>(tData);
		}
		else {
			utData = tData;
		}

		std::wstring_view wsvFmt;
		switch (sizeof(T)) {
		case 1:
			wsvFmt = L"0x{:02X}";
			break;
		case 2:
			wsvFmt = L"0x{:04X}";
			break;
		case 4:
			wsvFmt = L"0x{:08X}";
			break;
		case 8:
			wsvFmt = L"0x{:016X}";
			break;
		}

		*std::vformat_to(pwsz, wsvFmt, std::make_wformat_args(utData)) = L'\0';
	}
	else {
		std::wstring_view wsvFmt = L"{}";
		if constexpr (std::is_same_v<T, float>) {
			wsvFmt = L"{:.9e}";
		}
		else if constexpr (std::is_same_v<T, double>) {
			wsvFmt = L"{:.18e}";
		}
		*std::vformat_to(pwsz, wsvFmt, std::make_wformat_args(tData)) = L'\0';
	}
}

void CHexDlgTemplMgr::ShowListDataTime32(LPWSTR pwsz, __time32_t lTime32, bool fShouldSwap)const
{
	if (fShouldSwap) {
		lTime32 = ut::ByteSwap(lTime32);
	}

	if (IsShowAsHex()) {
		*std::format_to(pwsz, L"0x{:08X}", static_cast<unsigned long>(lTime32)) = L'\0';
		return;
	}

	if (lTime32 < 0) {
		*std::format_to(pwsz, L"N/A") = L'\0';
		return;
	}

	//Add seconds from epoch time.
	LARGE_INTEGER Time { .LowPart { ut::g_ulFileTime1970_LOW }, .HighPart { ut::g_ulFileTime1970_HIGH } };
	Time.QuadPart += static_cast<LONGLONG>(lTime32) * ut::g_uFTTicksPerSec;

	//Convert to FILETIME.
	const FILETIME ftTime { .dwLowDateTime { Time.LowPart }, .dwHighDateTime = static_cast<DWORD>(Time.HighPart) };
	*std::format_to(pwsz, L"{}", ut::FileTimeToString(ftTime, m_dwDateFormat, m_wchDateSepar)) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataTime64(LPWSTR pwsz, __time64_t llTime64, bool fShouldSwap)const
{
	if (fShouldSwap) {
		llTime64 = ut::ByteSwap(llTime64);
	}

	if (IsShowAsHex()) {
		*std::format_to(pwsz, L"0x{:016X}", static_cast<unsigned long long>(llTime64)) = L'\0';
		return;
	}

	if (llTime64 < 0) {
		*std::format_to(pwsz, L"N/A") = L'\0';
		return;
	}

	//Add seconds from epoch time.
	LARGE_INTEGER Time { .LowPart { ut::g_ulFileTime1970_LOW }, .HighPart { ut::g_ulFileTime1970_HIGH } };
	Time.QuadPart += llTime64 * ut::g_uFTTicksPerSec;

	//Convert to FILETIME.
	const FILETIME ftTime { .dwLowDateTime { Time.LowPart }, .dwHighDateTime { static_cast<DWORD>(Time.HighPart) } };
	*std::format_to(pwsz, L"{}", ut::FileTimeToString(ftTime, m_dwDateFormat, m_wchDateSepar)) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataFILETIME(LPWSTR pwsz, FILETIME stFTime, bool fShouldSwap)const
{
	if (fShouldSwap) {
		stFTime = ut::ByteSwap(stFTime);
	}

	const auto ui64 = std::bit_cast<std::uint64_t>(stFTime);
	const auto wstrTime = ut::FileTimeToString(stFTime, m_dwDateFormat, m_wchDateSepar);
	*std::vformat_to(pwsz, IsShowAsHex() ? L"0x{0:016X}" : L"{}", std::make_wformat_args(ui64, wstrTime)) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataSYSTEMTIME(LPWSTR pwsz, SYSTEMTIME stSTime, bool fShouldSwap)const
{
	//No Hex representation for this type because size is too big, > ULONGLONG.
	if (fShouldSwap) {
		stSTime.wYear = ut::ByteSwap(stSTime.wYear);
		stSTime.wMonth = ut::ByteSwap(stSTime.wMonth);
		stSTime.wDayOfWeek = ut::ByteSwap(stSTime.wDayOfWeek);
		stSTime.wDay = ut::ByteSwap(stSTime.wDay);
		stSTime.wHour = ut::ByteSwap(stSTime.wHour);
		stSTime.wMinute = ut::ByteSwap(stSTime.wMinute);
		stSTime.wSecond = ut::ByteSwap(stSTime.wSecond);
		stSTime.wMilliseconds = ut::ByteSwap(stSTime.wMilliseconds);
	}

	*std::format_to(pwsz, L"{}", ut::SystemTimeToString(stSTime, m_dwDateFormat, m_wchDateSepar)) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataGUID(LPWSTR pwsz, GUID stGUID, bool fShouldSwap)const
{
	//No Hex representation for this type because size is too big, > ULONGLONG.
	if (fShouldSwap) {
		stGUID.Data1 = ut::ByteSwap(stGUID.Data1);
		stGUID.Data2 = ut::ByteSwap(stGUID.Data2);
		stGUID.Data3 = ut::ByteSwap(stGUID.Data3);
	}

	*std::format_to(pwsz, L"{{{:0>8x}-{:0>4x}-{:0>4x}-{:0>2x}{:0>2x}-{:0>2x}{:0>2x}{:0>2x}{:0>2x}{:0>2x}{:0>2x}}}",
		stGUID.Data1, stGUID.Data2, stGUID.Data3, stGUID.Data4[0], stGUID.Data4[1], stGUID.Data4[2],
		stGUID.Data4[3], stGUID.Data4[4], stGUID.Data4[5], stGUID.Data4[6], stGUID.Data4[7]) = L'\0';
}

auto CHexDlgTemplMgr::TreeItemFromListItem(int iListItem)const->HTREEITEM
{
	auto hChildItem = m_WndTree.GetNextItem(m_hTreeCurrParent, TVGN_CHILD);
	for (auto itListItems = 0; itListItems < iListItem; ++itListItems) {
		hChildItem = m_WndTree.GetNextSiblingItem(hChildItem);
	}

	return hChildItem;
}

void CHexDlgTemplMgr::UnloadTemplate(int iTemplateID)
{
	OnTemplateLoadUnload(iTemplateID, false); //All the GUI stuff first, only then delete template.

	const auto pTemplate = GetTemplate(iTemplateID);
	if (pTemplate == nullptr)
		return;

	//Remove all applied templates from m_vecTemplatesAppl, if any, with the given iTemplateID.
	if (std::erase_if(m_vecTemplatesAppl, [pTemplate](const std::unique_ptr<TEMPLAPPLIED>& pData) {
		return pData->pTemplate == pTemplate; }) > 0) {
		RedrawHexCtrl();
	}

	std::erase_if(m_vecTemplates, [iTemplateID](const std::unique_ptr<HEXTEMPLATE>& pData) {
		return pData->iTemplateID == iTemplateID; }); //Remove template itself.

	//This needed because SetDlgButtonsState checks m_vecTemplates.empty(), which was erased just line above.
	//OnTemplateLoadUnload at the beginning doesn't know yet that it's empty (when all templates are unloaded).
	SetDlgButtonsState();
}

void CHexDlgTemplMgr::UpdateStaticText()
{
	std::wstring wstrOffset;
	std::wstring wstrSize;

	if (m_pAppliedCurr != nullptr && GetHexCtrl()->IsDataSet()) { //If m_pAppliedCurr == nullptr set empty text.
		const auto ullOffset = GetHexCtrl()->GetOffset(m_pAppliedCurr->ullOffset, true); //Show virtual offset.
		wstrOffset = std::vformat(IsShowAsHex() ? L"0x{:X}" : L"{}", std::make_wformat_args(ullOffset));
		wstrSize = std::vformat(IsShowAsHex() ? L"0x{:X}" : L"{}", std::make_wformat_args(m_pAppliedCurr->pTemplate->iSizeTotal));
	}

	m_WndStatOffset.SetWndText(wstrOffset);
	m_WndStatSize.SetWndText(wstrSize);
}

auto CHexDlgTemplMgr::CloneTemplate(PCHEXTEMPLATE pTemplate)->std::unique_ptr<HEXTEMPLATE>
{
	if (pTemplate == nullptr) {
		ut::DBG_REPORT(L"pTemplate == nullptr");
		return { };
	}

	auto pNew = std::make_unique<HEXTEMPLATE>();
	pNew->wstrName = pTemplate->wstrName;
	pNew->vecCustomType = pTemplate->vecCustomType;
	pNew->iSizeTotal = pTemplate->iSizeTotal;
	pNew->iTemplateID = pTemplate->iTemplateID;

	//Deep copy of all nested HexVecFields.
	const auto lmbCopyVecFields = [](HexVecFields& refVecFieldsNew, const HexVecFields& refVecFieldsOld)->void {
		const auto _lmbCopyVecFields = [](const auto& lmbSelf, HexVecFields& refVecFieldsNew, const HexVecFields& refVecFieldsOld,
			PCHEXTEMPLFIELD pFieldParent)->void {
				for (const auto& pOldField : refVecFieldsOld) {
					const auto& pNewField = refVecFieldsNew.emplace_back(std::make_unique<HEXTEMPLFIELD>());
					pNewField->wstrName = pOldField->wstrName;
					pNewField->wstrDescr = pOldField->wstrDescr;
					pNewField->iOffset = pOldField->iOffset;
					pNewField->iSize = pOldField->iSize;
					pNewField->stClr = pOldField->stClr;
					pNewField->pFieldParent = pFieldParent;
					pNewField->eType = pOldField->eType;
					pNewField->uTypeID = pOldField->uTypeID;
					pNewField->fBigEndian = pOldField->fBigEndian;
					lmbSelf(lmbSelf, pNewField->vecNested, pOldField->vecNested, pNewField.get());
				}
			};
		_lmbCopyVecFields(_lmbCopyVecFields, refVecFieldsNew, refVecFieldsOld, nullptr);
		};
	lmbCopyVecFields(pNew->vecFields, pTemplate->vecFields);

	return pNew;
}

bool CHexDlgTemplMgr::JSONParseFields(const IterJSONMember itFieldsArray, HexVecFields& vecFields,
	const FIELDSDEFPROPS& defProps, UmapCustomTypes& umapCustomT, int* pOffset)
{
	using enum EHexFieldType;
	static const std::unordered_map<std::string_view, EHexFieldType> umapStrToEType { //From JSON string to EHexFieldType conversion.
		{ "bool", type_bool },
		{ "int8", type_int8 }, { "char", type_int8 },
		{ "uint8", type_uint8 }, { "unsigned char", type_uint8 }, { "byte", type_uint8 },
		{ "int16", type_int16 }, { "short", type_int16 },
		{ "uint16", type_uint16 }, { "unsigned short", type_uint16 }, { "WORD", type_uint16 },
		{ "int32", type_int32 }, { "long", type_int32 }, { "int", type_int32 },
		{ "uint32", type_uint32 }, { "unsigned long", type_uint32 }, { "unsigned int", type_uint32 }, { "DWORD", type_uint32 },
		{ "int64", type_int64 }, { "long long", type_int64 },
		{ "uint64", type_uint64 }, { "unsigned long long", type_uint64 }, { "QWORD", type_uint64 },
		{ "float", type_float }, { "double", type_double },
		{ "time32_t", type_time32 }, { "time64_t", type_time64 },
		{ "FILETIME", type_filetime }, { "SYSTEMTIME", type_systemtime }, { "GUID", type_guid } };
	static const std::unordered_map<EHexFieldType, int> umapTypeToSize { //Types sizes.
		{ type_bool, static_cast<int>(sizeof(bool)) }, { type_int8, static_cast<int>(sizeof(char)) },
		{ type_uint8, static_cast<int>(sizeof(char)) }, { type_int16, static_cast<int>(sizeof(short)) },
		{ type_uint16, static_cast<int>(sizeof(short)) }, { type_int32, static_cast<int>(sizeof(int)) },
		{ type_uint32, static_cast<int>(sizeof(int)) }, { type_int64, static_cast<int>(sizeof(long long)) },
		{ type_uint64, static_cast<int>(sizeof(long long)) }, { type_float, static_cast<int>(sizeof(float)) },
		{ type_double, static_cast<int>(sizeof(double)) }, { type_time32, static_cast<int>(sizeof(__time32_t)) },
		{ type_time64, static_cast<int>(sizeof(__time64_t)) }, { type_filetime, static_cast<int>(sizeof(FILETIME)) },
		{ type_systemtime, static_cast<int>(sizeof(SYSTEMTIME)) }, { type_guid, static_cast<int>(sizeof(GUID)) }
	};

	const auto lmbTotalSize = [](const HexVecFields& vecFields)->int { //Counts total size of all Fields in HexVecFields, recursively.
		const auto _lmbTotalSize = [](const auto& lmbSelf, const HexVecFields& refVecFields)->int {
			return std::reduce(refVecFields.begin(), refVecFields.end(), 0,
				[&lmbSelf](auto ullTotal, const std::unique_ptr<HEXTEMPLFIELD>& pField) {
					if (!pField->vecNested.empty()) {
						return ullTotal + lmbSelf(lmbSelf, pField->vecNested);
					}
					return ullTotal + pField->iSize; });
			};
		return _lmbTotalSize(_lmbTotalSize, vecFields);
		};

	int iOffset { 0 }; //Default starting offset.
	if (pOffset == nullptr) {
		pOffset = &iOffset;
	}

	for (auto pField = itFieldsArray->value.Begin(); pField != itFieldsArray->value.End(); ++pField) {
		if (!pField->IsObject()) {
			ut::DBG_REPORT(L"Each array entry must be an Object {}.");
			return false;
		}

		std::wstring wstrNameField;
		if (const auto itName = pField->FindMember("name");
			itName != pField->MemberEnd() && itName->value.IsString()) {
			wstrNameField = ut::StrToWstr(itName->value.GetString());
		}
		else {
			ut::DBG_REPORT(L"Each array entry (Object) must have a \"name\" string.");
			return false;
		}

		const auto& pNewField = vecFields.emplace_back(std::make_unique<HEXTEMPLFIELD>());
		pNewField->wstrName = wstrNameField;
		pNewField->iOffset = *pOffset;
		pNewField->stClr.clrBk = JSONColors(*pField, "clrBk").value_or(defProps.stClr.clrBk);
		pNewField->stClr.clrText = JSONColors(*pField, "clrText").value_or(defProps.stClr.clrText);
		pNewField->pFieldParent = defProps.pFieldParent;
		pNewField->eType = type_custom;
		pNewField->fBigEndian = JSONEndianness(*pField).value_or(defProps.fBigEndian);

		if (const auto itNestedFields = pField->FindMember("Fields");
			itNestedFields != pField->MemberEnd()) {
			if (!itNestedFields->value.IsArray()) {
				ut::DBG_REPORT(L"Each \"Fields\" must be an Array.");
				return false;
			}

			//Setting defaults for the next nested fields.
			const FIELDSDEFPROPS stDefsNested { .stClr { pNewField->stClr }, .pTemplate { defProps.pTemplate },
				.pFieldParent { pNewField.get() }, .fBigEndian { pNewField->fBigEndian } };

			//Recursion for nested fields starts here.
			if (!JSONParseFields(itNestedFields, pNewField->vecNested, stDefsNested, umapCustomT, pOffset)) {
				return false;
			}

			pNewField->iSize = lmbTotalSize(pNewField->vecNested); //Total size of all nested fields.
		}
		else {
			if (const auto itDescr = pField->FindMember("description");
				itDescr != pField->MemberEnd() && itDescr->value.IsString()) {
				pNewField->wstrDescr = ut::StrToWstr(itDescr->value.GetString());
			}

			int iArraySize { 0 };
			if (const auto itArray = pField->FindMember("array");
				itArray != pField->MemberEnd() && itArray->value.IsInt() && itArray->value.GetInt() > 1) {
				iArraySize = itArray->value.GetInt();
			}

			int iSize { 0 }; //Current field's size, via "type" or "size" property.
			if (const auto itType = pField->FindMember("type"); itType != pField->MemberEnd()) {
				if (!itType->value.IsString()) {
					ut::DBG_REPORT(L"Field \"type\" must be a string.");
					return false;
				}

				if (const auto itMapType = umapStrToEType.find(itType->value.GetString());
					itMapType != umapStrToEType.end()) {
					pNewField->eType = itMapType->second;
					iSize = umapTypeToSize.at(itMapType->second);
				}
				else { //If it's not any standard type, we try to find custom type with given name.
					const auto& vecCTypes = defProps.pTemplate->vecCustomType;
					const auto itVecCT = std::find_if(vecCTypes.begin(), vecCTypes.end(),
						[=](const HEXCUSTOMTYPE& ref) { return ref.wstrTypeName == ut::StrToWstr(itType->value.GetString()); });
					if (itVecCT == vecCTypes.end()) {
						ut::DBG_REPORT(L"Unknown field \"type\".");
						return false;
					}

					pNewField->uTypeID = itVecCT->uTypeID; //Custom type ID.
					pNewField->eType = type_custom;
					const auto lmbCopyCustomType = [](const HexVecFields& refVecCustomFields,
						const HexPtrField& pField, int& iOffset)->void {
							const auto _lmbCustomTypeCopy = [](const auto& lmbSelf,
								const HexVecFields& refVecCustomFields, const HexPtrField& pField, int& iOffset)->void {
									for (const auto& pCustomField : refVecCustomFields) {
										const auto& pNewField = pField->vecNested.emplace_back(std::make_unique<HEXTEMPLFIELD>());
										const auto& refCFClr = pCustomField->stClr;
										pNewField->wstrName = pCustomField->wstrName;
										pNewField->wstrDescr = pCustomField->wstrDescr;
										pNewField->iOffset = iOffset;
										pNewField->iSize = pCustomField->iSize;
										pNewField->stClr.clrBk = refCFClr.clrBk == -1 ? pField->stClr.clrBk : refCFClr.clrBk;
										pNewField->stClr.clrText = refCFClr.clrText == -1 ? pField->stClr.clrText : refCFClr.clrText;
										pNewField->pFieldParent = pField.get();
										pNewField->eType = pCustomField->eType;
										pNewField->uTypeID = pCustomField->uTypeID;
										pNewField->fBigEndian = pCustomField->fBigEndian;

										if (pCustomField->vecNested.empty()) {
											iOffset += pCustomField->iSize;
										}
										else {
											lmbSelf(lmbSelf, pCustomField->vecNested, pNewField, iOffset);
										}
									}
								};
							_lmbCustomTypeCopy(_lmbCustomTypeCopy, refVecCustomFields, pField, iOffset);
						};

					auto iOffsetCustomType = *pOffset;
					if (iArraySize <= 1) {
						lmbCopyCustomType(umapCustomT[itVecCT->uTypeID], pNewField, iOffsetCustomType);
					}
					else { //Creating array of Custom Types.
						for (int iArrIndex = 0; iArrIndex < iArraySize; ++iArrIndex) {
							const auto& pFieldArray = pNewField->vecNested.emplace_back(std::make_unique<HEXTEMPLFIELD>());
							pFieldArray->wstrName = std::format(L"{}[{}]", wstrNameField, iArrIndex);
							pFieldArray->iOffset = iOffsetCustomType;
							pFieldArray->stClr = pNewField->stClr;
							pFieldArray->pFieldParent = pNewField.get();
							pFieldArray->eType = pNewField->eType;
							pFieldArray->uTypeID = pNewField->uTypeID;
							pFieldArray->fBigEndian = pNewField->fBigEndian;

							//Copy Custom Type fields into the pFieldArray->vecNested.
							lmbCopyCustomType(umapCustomT[itVecCT->uTypeID], pFieldArray, iOffsetCustomType);
							pFieldArray->iSize = lmbTotalSize(pFieldArray->vecNested);
						}
						pNewField->wstrName = std::format(L"{}[{}]", wstrNameField, iArraySize);
					}

					iSize = lmbTotalSize(pNewField->vecNested);
				}
			}
			else { //No "type" property was found.
				const auto itSize = pField->FindMember("size");
				if (itSize == pField->MemberEnd()) {
					ut::DBG_REPORT(L"No \"size\" or \"type\" property was found.");
					return false;
				}

				if (!itSize->value.IsInt()) {
					ut::DBG_REPORT(L"The \"size\" must be an int.");
					return false;
				}

				const auto iFieldSize = itSize->value.GetInt();
				if (iFieldSize < 1) {
					ut::DBG_REPORT(L"The \"size\" must be > 0.");
					return false;
				}

				iSize = iFieldSize;
				pNewField->eType = custom_size;
			}

			if (iArraySize > 1 && pNewField->eType != type_custom) { //Creating array of standard/default types.
				for (auto iArrIndex = 0; iArrIndex < iArraySize; ++iArrIndex) {
					const auto& pFieldArray = pNewField->vecNested.emplace_back(std::make_unique<HEXTEMPLFIELD>());
					pFieldArray->wstrName = std::format(L"{}[{}]", wstrNameField, iArrIndex);
					pFieldArray->iOffset = *pOffset + iArrIndex * iSize;
					pFieldArray->iSize = iSize;
					pFieldArray->stClr = pNewField->stClr;
					pFieldArray->pFieldParent = pNewField.get();
					pFieldArray->eType = pNewField->eType;
					pFieldArray->uTypeID = pNewField->uTypeID;
					pFieldArray->fBigEndian = pNewField->fBigEndian;
				}
				pNewField->wstrName = std::format(L"{}[{}]", wstrNameField, iArraySize);
				iSize *= iArraySize;
			}

			pNewField->iSize = iSize;
			*pOffset += iSize;
		}
	}

	return true;
}

auto CHexDlgTemplMgr::JSONEndianness(const rapidjson::Value& value)->std::optional<bool>
{
	const auto itEndianness = value.FindMember("endianness");
	if (itEndianness == value.MemberEnd()) {
		return false; //If no "endianness" property then it's "little" by default.
	}

	if (!itEndianness->value.IsString()) {
		ut::DBG_REPORT(L"Field \"endianness\" must be a string.");
		return std::nullopt;
	}

	const std::string_view svEndianness = itEndianness->value.GetString();
	if (svEndianness != "big" && svEndianness != "little") {
		ut::DBG_REPORT(L"Unknown \"endianness\" type.");
		return std::nullopt;
	}

	return svEndianness == "big";
}

auto CHexDlgTemplMgr::JSONColors(const rapidjson::Value& value, const char* pszColorName)->std::optional<COLORREF>
{
	const auto itClr = value.FindMember(pszColorName);
	if (itClr == value.MemberEnd() || !itClr->value.IsString()) {
		return std::nullopt;
	}

	const std::string_view sv { itClr->value.GetString() };
	if (sv.empty() || sv.size() != 7 || sv[0] != '#')
		return std::nullopt;

	const auto R = *stn::StrToUInt32(sv.substr(1, 2), 16);
	const auto G = *stn::StrToUInt32(sv.substr(3, 2), 16);
	const auto B = *stn::StrToUInt32(sv.substr(5, 2), 16);

	return RGB(R, G, B);
}

auto CHexDlgTemplMgr::TreeSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass,
	DWORD_PTR /*dwRefData*/)->LRESULT
{
	switch (uMsg) {
	case WM_KILLFOCUS:
		return 0; //Do nothing when Tree loses focus, to save current selection.
	case WM_LBUTTONDOWN:
		::SetFocus(hWnd);
		break;
	case WM_NCDESTROY:
		::RemoveWindowSubclass(hWnd, TreeSubclassProc, uIdSubclass);
		break;
	default:
		break;
	}

	return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

HEXCTRLAPI auto __cdecl HEXCTRL::IHexTemplates::LoadFromFile(const wchar_t* pFilePath)->std::unique_ptr<HEXCTRL::HEXTEMPLATE>
{
	if (pFilePath == nullptr) {
		ut::DBG_REPORT(L"pFilePath == nullptr");
		return { };
	}

	std::ifstream ifs(pFilePath);
	if (!ifs.is_open()) {
		ut::DBG_REPORT(L"!ifs.is_open()");
		return { };
	}

	rapidjson::IStreamWrapper isw { ifs };
	rapidjson::Document docJSON;
	docJSON.ParseStream(isw);
	if (docJSON.IsNull()) {
		ut::DBG_REPORT(L"docJSON.IsNull()");
		return { };
	}

	auto pTemplateUtr = std::make_unique<HEXTEMPLATE>();
	const auto pTemplate = pTemplateUtr.get();
	const auto itTName = docJSON.FindMember("TemplateName");
	if (itTName == docJSON.MemberEnd() || !itTName->value.IsString()) {
		ut::DBG_REPORT(L"Template must have a string type name.");
		return { };
	}

	pTemplate->wstrName = ut::StrToWstr(itTName->value.GetString());

	CHexDlgTemplMgr::UmapCustomTypes umapCT;
	std::uint8_t uCustomTypeID = 1; //ID starts at 1.
	if (const auto objCustomTypes = docJSON.FindMember("CustomTypes");
		objCustomTypes != docJSON.MemberEnd() && objCustomTypes->value.IsArray()) {
		for (auto pCustomType = objCustomTypes->value.Begin(); pCustomType != objCustomTypes->value.End(); ++pCustomType, ++uCustomTypeID) {
			if (!pCustomType->IsObject()) {
				ut::DBG_REPORT(L"Each CustomTypes' array entry must be an Object.");
				return { };
			}

			std::wstring wstrTypeName;
			if (const auto itName = pCustomType->FindMember("TypeName");
				itName != pCustomType->MemberEnd() && itName->value.IsString()) {
				wstrTypeName = ut::StrToWstr(itName->value.GetString());
			}
			else {
				ut::DBG_REPORT(L"Each array entry (Object) must have a \"TypeName\" property.");
				return { };
			}

			const auto clrBk = CHexDlgTemplMgr::JSONColors(*pCustomType, "clrBk").value_or(-1);
			const auto clrText = CHexDlgTemplMgr::JSONColors(*pCustomType, "clrText").value_or(-1);
			const auto fBigEndian = CHexDlgTemplMgr::JSONEndianness(*pCustomType).value_or(false);

			const auto itFieldsArray = pCustomType->FindMember("Fields");
			if (itFieldsArray == pCustomType->MemberEnd() || !itFieldsArray->value.IsArray()) {
				ut::DBG_REPORT(L"Each \"Fields\" must be an array.");
				return { };
			}

			umapCT.try_emplace(uCustomTypeID, HexVecFields { });
			const CHexDlgTemplMgr::FIELDSDEFPROPS stDefTypes { .stClr { clrBk, clrText }, .pTemplate { pTemplate },
				.fBigEndian { fBigEndian } };
			if (!CHexDlgTemplMgr::JSONParseFields(itFieldsArray, umapCT[uCustomTypeID], stDefTypes, umapCT)) {
				return { }; //Something went wrong during template parsing.
			}
			pTemplate->vecCustomType.emplace_back(std::move(wstrTypeName), uCustomTypeID);
		}
	}

	const auto objData = docJSON.FindMember("Data");
	if (objData == docJSON.MemberEnd() || !objData->value.IsObject()) {
		ut::DBG_REPORT(L"No \"Data\" object member in the template.");
		return { };
	}

	const auto clrBk = CHexDlgTemplMgr::JSONColors(objData->value, "clrBk").value_or(-1);
	const auto clrText = CHexDlgTemplMgr::JSONColors(objData->value, "clrText").value_or(-1);
	const auto fBigEndian = CHexDlgTemplMgr::JSONEndianness(objData->value).value_or(false);
	const CHexDlgTemplMgr::FIELDSDEFPROPS stDefFields { .stClr { clrBk, clrText }, .pTemplate { pTemplate },
		.fBigEndian { fBigEndian } };

	const auto itFieldsArray = objData->value.FindMember("Fields");
	auto& refVecFields = pTemplate->vecFields;
	if (!CHexDlgTemplMgr::JSONParseFields(itFieldsArray, refVecFields, stDefFields, umapCT)) {
		ut::DBG_REPORT(L"Something went wrong during template parsing.");
		return { };
	}

	pTemplate->iSizeTotal = std::reduce(refVecFields.begin(), refVecFields.end(), 0,
		[](auto iTotal, const std::unique_ptr<HEXTEMPLFIELD>& pData) { return iTotal + pData->iSize; });

	return pTemplateUtr;
}