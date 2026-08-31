/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
module;
#include <SDKDDKVer.h>
#include "../HexCtrl.h"
#include "dep/rapidjson-amalgam.h"
#include "res/HexCtrlRes.h"
#include <Windows.h>
#include <ShObjIdl.h>
#include <commctrl.h>
#include <algorithm>
#include <bit>
#include <cmath>
#include <format>
#include <fstream>
#include <limits>
#include <memory>
#include <numeric>
#include <optional>
#include <random>
#include <ranges>
#include <string>
#include <vector>
#include <unordered_map>
export module HEXCTRL:CHexDlgTemplMgr;

import :HexUtility;

namespace HEXCTRL::INTERNAL {
	struct HEXTEMPLFIELD;
	using PtrHexTemplField = std::unique_ptr<HEXTEMPLFIELD>;
	using VecHexTemplFields = std::vector<PtrHexTemplField>; //Vector for the template fields.
	using PCHEXTEMPLFIELD = const HEXTEMPLFIELD*;

	//Predefined types of a template field.
	enum class EHexTemplFieldType : std::uint8_t {
		custom_size, type_custom,
		type_bool, type_int8, type_uint8, type_int16, type_uint16, type_int32,
		type_uint32, type_int64, type_uint64, type_float, type_double, type_time32,
		type_time64, type_filetime, type_systemtime, type_guid
	};

	//Custom type of a template field.
	struct HEXTEMPLCT {
		std::wstring wstrTypeName; //Custom type name.
		int          iTypeID { };  //Custom type ID.
	};
	using VecHexTemplCT = std::vector<HEXTEMPLCT>;

	//Template jump field anchor enum.
	enum class EHexTemplJumpAnchor : std::uint8_t {
		DATA_START, DATA_END, FIELD_THIS, FIELD_FIRST, OFFSET_CUSTOM
	};

	//Template jump field direction enum.
	enum class EHexTemplJumpDirection : std::uint8_t {
		JUMP_FORWARD, JUMP_BACKWARD
	};

	//Struct describes jumping properties of the field.
	struct HEXTEMPLJUMP {
		std::uint64_t          u64Anchor { };  //Field is used if the eAnchor==OFFSET_CUSTOM.
		std::uint32_t          u32Units { 1 }; //Default unit size is 1 byte, but can be any size.
		EHexTemplJumpAnchor    eAnchor { };
		EHexTemplJumpDirection eDirection { };
	};
	using PtrHexTemplJump = std::unique_ptr<HEXTEMPLJUMP>;

	//Template's field main struct.
	struct HEXTEMPLFIELD {
		std::wstring       wstrName;             //Field name.
		std::wstring       wstrDescr;            //Field description.
		VecHexTemplFields  vecNested;            //Vector for nested fields.
		HEXCOLOR           stClr;                //Field Bk and Text color.
		PCHEXTEMPLFIELD    pFieldParent { };     //Parent field, in case of nested.
		PtrHexTemplJump    pJump { };            //Pointer to a jump struct, if it's a "jump" field.
		int                iOffset { };          //Field offset relative to the Template's beginning.
		int                iSize { };            //Field size.
		int                iCustomTypeID { };    //Field custom-type ID, if eType==type_custom.
		EHexTemplFieldType eType { };            //Field type.
		bool               fBigEndian { false }; //Field endianness.
	};

	//Template main struct.
	struct HEXTEMPLATE {
		std::wstring      wstrName;      //Template name.
		std::wstring      wstrFilePath;  //Template file path.
		VecHexTemplFields vecFields;     //Template fields.
		VecHexTemplCT     vecCustomType; //Custom types of this template.
		std::uint64_t     u64Offset { }; //Offset where template is applied.
		int               iSizeTotal;    //Total size of all Template's fields, assigned internally by framework.
		int               iTemplateID;   //Template ID, assigned by framework.
	};
	using PCHEXTEMPLATE = const HEXTEMPLATE*;

	using IterJSONMember = rapidjson::Value::ConstMemberIterator;
	using UmapCustomTypes = std::unordered_map<int, VecHexTemplFields>;
	using PCHexVecTemplFields = const VecHexTemplFields*;

	class CHexDlgTemplMgr final : public IHexTemplates {
	public:
		struct FIELDSDEFPROPS; //Forward declarations.
		bool AddTemplateFile(const wchar_t* pwszFilePath)override;
		void ApplyCurr(std::uint64_t u64Offset);
		auto ApplyTemplate(const wchar_t* pwszFilePath, std::uint64_t u64Offset) -> int override;
		void CreateDlg()const;
		void DestroyDlg();
		void DisapplyAll()override;
		void DisapplyByID(int iTemplateID)override; //Disapply template with the given TemplateID.
		void DisapplyByOffset(std::uint64_t u64Offset)override;
		[[nodiscard]] auto GetAllApplied() -> VecHexTemplatesApplied override;
		[[nodiscard]] auto GetDlgItemHandle(EHexDlgItem eItem)const -> HWND;
		[[nodiscard]] auto GetHWND()const -> HWND;
		[[nodiscard]] bool HasCurrent()const;
		[[nodiscard]] bool HasApplied()const;
		[[nodiscard]] auto HitTest(std::uint64_t u64Offset)const -> PCHEXTEMPLFIELD; //Template hittest by offset.
		void Initialize(IHexCtrl &HexCtrl, HINSTANCE hInstRes);
		[[nodiscard]] bool IsShowTooltips()const;
		[[nodiscard]] bool PreTranslateMsg(MSG* pMsg);
		[[nodiscard]] auto ProcessMsg(const MSG& msg) -> INT_PTR;
		void RemoveAllTemplates()override;
		void RemoveTemplateFile(const wchar_t* pwszFilePath)override;
		void SetDlgProperties(std::uint64_t u64Flags);
		void ShowTooltips(bool fShow)override;
		void ShowWindow(int iCmdShow);
		void UpdateData();
		[[nodiscard]] static auto GetDataFromField(const HEXTEMPLFIELD* pField, PCHEXTEMPLATE pTemplate, IHexCtrl* pHexCtrl)
			-> std::optional<std::uint64_t>;
		[[nodiscard]] static auto JSONColors(const rapidjson::Value* pValue, const char* pszColorName) -> std::optional<COLORREF>;
		[[nodiscard]] static auto JSONIsBigEndianness(const rapidjson::Value* pValue) -> std::optional<bool>;
		[[nodiscard]] static auto JSONFindMember(const rapidjson::Value* pValue, const char* pszName) -> std::optional<IterJSONMember>;
		[[nodiscard]] static auto JSONFindMemberAsInt32(const rapidjson::Value* pValue, const char* pszName) -> std::optional<int>;
		[[nodiscard]] static auto JSONFindMemberAsUInt32(const rapidjson::Value* pValue, const char* pszName) -> std::optional<std::uint32_t>;
		[[nodiscard]] static auto JSONFindMemberAsObject(const rapidjson::Value* pValue, const char* pszName)
			-> std::optional<const rapidjson::Value*>;
		[[nodiscard]] static auto JSONFindMemberAsString(const rapidjson::Value* pValue, const char* pszName)
			-> std::optional<const char*>;
		[[nodiscard]] static bool JSONParseFields(IterJSONMember itFieldsArray, VecHexTemplFields& vecFields,
			const FIELDSDEFPROPS& defProps, UmapCustomTypes& umapCustomT, int* pFieldOffset, IHexCtrl* pHexCtrl);
		[[nodiscard]] static auto JSONProcessLimitObject(const rapidjson::Value* pValue, std::uint64_t u64ActualData) -> std::optional<std::uint64_t>;
		[[nodiscard]] static bool IsEqualNoCase(std::string_view sv1, std::string_view sv2);
		[[nodiscard]] static auto TMPLFindFieldName(PCHEXTEMPLATE pTemplate, std::wstring_view wsvFieldName) -> PCHEXTEMPLFIELD;
	private:
		void CreateArrows();
		[[nodiscard]] auto GetHexCtrl()const -> IHexCtrl*;
		[[nodiscard]] auto GUIGetTemplateIDFromTree(HTREEITEM hTreeItem) -> int;
		void GUIOnTemplateApplyDisapply(int iTemplateID, bool fApply);
		void GUIOnTemplateAddRemove(const wchar_t* pwszFilePath, bool fAdd);
		[[nodiscard]] auto GUITreeItemFromListItem(int iListItem)const -> HTREEITEM;
		[[nodiscard]] auto GUIGetTreeSelectedTemplate() -> PCHEXTEMPLATE; //Currently selected Template ptr in the tree.
		[[nodiscard]] auto GUIGetTreeSelectedTemplateID() -> int;         //Currently selected TemplateID in the tree.
		[[nodiscard]] auto GUIGetListCurrTemplateFilePath()const -> wchar_t*;
		void GUIUpdateDateTimeFormat();
		void GUIUpdateEditBoxOffsetToCurrHexCaret();
		void GUIUpdateStaticText();
		[[nodiscard]] bool IsHighlight()const;
		[[nodiscard]] bool IsMinimized()const;
		[[nodiscard]] bool IsNoEsc()const;
		[[nodiscard]] bool IsShowAsHex()const;
		[[nodiscard]] bool IsSwapEndian()const;
		void OnBnAddTemplate();
		void OnBnRemoveTemplate();
		void OnBnApply();
		void OnCancel();
		void OnCheckHex();
		void OnCheckSwapEndian();
		void OnCheckMin();
		void OnOK();
		void RedrawHexCtrl();
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
		auto TMPLAddTemplateFile(const wchar_t* pwszFilePath) -> const wchar_t*;
		[[nodiscard]] auto TMPLGetIDForNewTemplate()const -> int;
		[[nodiscard]] auto TMPLGetTemplateByFilePath(const wchar_t* pwszFilePath)const -> PCHEXTEMPLATE;
		[[nodiscard]] auto TMPLGetTemplateByID(int iTemplateID)const -> PCHEXTEMPLATE;
		[[nodiscard]] auto TMPLGetTemplateByOffset(ULONGLONG ullOffset)const -> PCHEXTEMPLATE;
		[[nodiscard]] bool TMPLHasTemplateFiles()const;
		void TMPLRandomizeTemplateColors(int iTemplateID);
		void TMPLRemoveAppliedByID(int iTemplateID);
		void TMPLRemoveAppliedByFilePath(const wchar_t* pwszFilePath);
		void TMPLRemoveTemplateFile(const wchar_t* pwszFilePath);
		auto WMActivate(const MSG& msg) -> INT_PTR;
		auto WMCommand(const MSG& msg) -> INT_PTR;
		auto WMClose() -> INT_PTR;
		auto WMCtlColorStatic(const MSG& msg) -> INT_PTR;
		auto WMDestroy() -> INT_PTR;
		auto WMDPIChanged(const MSG& msg) -> INT_PTR;
		auto WMDrawItem(const MSG& msg) -> INT_PTR;
		auto WMGetDPIScaledSize(const MSG& msg) -> INT_PTR;
		auto WMInitDialog(const MSG& msg) -> INT_PTR;
		auto WMLButtonDown(const MSG& msg) -> INT_PTR;
		auto WMLButtonUp(const MSG& msg) -> INT_PTR;
		auto WMMeasureItem(const MSG& msg) -> INT_PTR;
		auto WMMouseActivate(const MSG& msg) -> INT_PTR;
		auto WMNotify(const MSG& msg) -> INT_PTR;
		void WMNotifyListDblClick(NMHDR* pNMHDR);
		void WMNotifyListEditBegin(NMHDR* pNMHDR);
		void WMNotifyListEnterPressed(NMHDR* pNMHDR);
		void WMNotifyListGetColor(NMHDR* pNMHDR);
		void WMNotifyListGetDispInfo(NMHDR* pNMHDR);
		void WMNotifyListHdrRClick(NMHDR* pNMHDR);
		void WMNotifyListItemChanged(NMHDR* pNMHDR);
		void WMNotifyListLinkClick(NMHDR* pNMHDR);
		void WMNotifyListRClick(NMHDR* pNMHDR);
		void WMNotifyListSetData(NMHDR* pNMHDR);
		void WMNotifyTreeGetDispInfo(NMHDR* pNMHDR);
		void WMNotifyTreeItemChanged(NMHDR* pNMHDR);
		void WMNotifyTreeLClick(NMHDR* pNMHDR);
		void WMNotifyTreeRClick(NMHDR* pNMHDR);
		auto WMSize(const MSG& msg) -> INT_PTR;
		[[nodiscard]] static auto JSONGetTemplateNameProperty(const wchar_t* pwszFilePath) -> std::wstring;
		static auto CALLBACK TreeSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
			UINT_PTR uIDSubclass, DWORD_PTR dwRefData)->LRESULT;
	private:
		enum class EMenuID : std::uint16_t;
		enum EListColumns : std::int8_t;
		GDIUT::CSplitter m_SplitVert;
		GDIUT::CDynLayout m_DynLayout;
		HINSTANCE m_hInstRes { };
		GDIUT::CWnd m_Wnd;
		GDIUT::CWnd m_WndStatOffset;     //Static text "Template offset:".
		GDIUT::CWnd m_WndStatSize;       //Static text Template size:".
		GDIUT::CWndEdit m_WndEditOffset; //"Offset" edit box.
		GDIUT::CWndBtn m_WndBtnTT;       //Check-box "Show tooltips".
		GDIUT::CWndBtn m_WndBtnMin;      //Check-box min-max.
		GDIUT::CWndBtn m_WndBtnHighlight;   //Check-box "Highlight selected".
		GDIUT::CWndBtn m_WndBtnHex;      //Check-box "Hex numbers".
		GDIUT::CWndBtn m_WndBtnEndian;   //Check-box "Swap endian".
		GDIUT::CWndCombo m_WndCmbTempl;  //Currently available templates list.
		GDIUT::CWndTree m_WndTree;       //Tree control.
		GDIUT::CMenu m_MenuTree;         //Menu for the tree control.
		GDIUT::CMenu m_MenuListHdr;      //Menu for the list header.
		LISTEX::CListEx m_ListEx;
		std::vector<std::unique_ptr<std::wstring>> m_vecTemplateFiles; //Template files paths.
		std::vector<std::unique_ptr<HEXTEMPLATE>> m_vecTemplates;      //Applied templates.
		IHexCtrl* m_pHexCtrl { };
		PCHexVecTemplFields m_pVecFieldsCurr { }; //Currently selected Fields vector.
		HTREEITEM m_hTreeCurrParent { };   //Currently selected Tree node's parent.
		HBITMAP m_hBmpMin { };             //Bitmap for the min checkbox.
		HBITMAP m_hBmpMax { };             //Bitmap for the max checkbox.
		std::uint64_t m_u64Flags { };      //Data from SetDlgProperties.
		DWORD m_dwDateFormat { };          //Date format.
		wchar_t m_wchDateSepar { };        //Date separator.
		bool m_fListGuardEvent { false };  //To not proceed with OnListItemChanged, same as pTree->action == TVC_UNKNOWN.
		bool m_fTreeClickedWithMouse { false }; //Indicates that tree item was changed with a mouse.
	};
}

using namespace HEXCTRL::INTERNAL;

enum class CHexDlgTemplMgr::EMenuID : std::uint16_t {
	IDM_TREE_RNDCOLORS = 0x8000, IDM_TREE_DISAPPLY, IDM_TREE_DISAPPLYALL,
	IDM_LIST_HDR_TYPE, IDM_LIST_HDR_NAME, IDM_LIST_HDR_OFFSET, IDM_LIST_HDR_SIZE,
	IDM_LIST_HDR_DATA, IDM_LIST_HDR_ENDIANNESS, IDM_LIST_HDR_DESCRIPTION, IDM_LIST_HDR_COLORS
};

enum CHexDlgTemplMgr::EListColumns : std::int8_t {
	COL_TYPE = 0, COL_NAME = 1, COL_OFFSET = 2, COL_SIZE = 3,
	COL_DATA = 4, COL_ENDIAN = 5, COL_DESCR = 6, COL_COLORS = 7
};

struct CHexDlgTemplMgr::FIELDSDEFPROPS { //Helper struct for convenient argument passing through recursive fields' parsing.
	HEXCOLOR        stClr;
	PCHEXTEMPLATE   pTemplate { }; //Same for all fields.
	PCHEXTEMPLFIELD pFieldParent { nullptr };
	bool            fBigEndian { false };
};


bool CHexDlgTemplMgr::AddTemplateFile(const wchar_t* pwszFilePath) {
	const auto pwszTemplatePathInternally = TMPLAddTemplateFile(pwszFilePath);
	if (pwszTemplatePathInternally == nullptr) {
		return false;
	}

	GUIOnTemplateAddRemove(pwszTemplatePathInternally, true);

	return true;
}

void CHexDlgTemplMgr::ApplyCurr(std::uint64_t u64Offset) {
	if (!m_Wnd.IsWindow() || !HasCurrent())
		return;

	ApplyTemplate(GUIGetListCurrTemplateFilePath(), u64Offset);
}

auto CHexDlgTemplMgr::ApplyTemplate(const wchar_t* pwszFilePath, std::uint64_t u64Offset)->int {
	if (pwszFilePath == nullptr) {
		ut::DBG_REPORT(L"pwszFilePath == nullptr");
		return -1;
	}

	std::ifstream ifs(pwszFilePath);
	if (!ifs.is_open()) {
		ut::DBG_REPORT(std::format(L"{}\r\n!ifs.is_open()", pwszFilePath).data());
		return -1;
	}

	rapidjson::IStreamWrapper isw { ifs };
	rapidjson::Document docJSON;
	docJSON.ParseStream(isw);
	if (docJSON.IsNull()) {
		ut::DBG_REPORT(std::format(L"{}\r\ndocJSON.IsNull()", pwszFilePath).data());
		return -1;
	}

	const auto itTName = docJSON.FindMember("TemplateName");
	if (itTName == docJSON.MemberEnd() || !itTName->value.IsString()) {
		ut::DBG_REPORT(std::format(L"{}\r\nTemplate must have a string type name.", pwszFilePath).data());
		return -1;
	}

	auto pTemplateUtr = std::make_unique<HEXTEMPLATE>();
	const auto pTemplate = pTemplateUtr.get();
	pTemplate->wstrFilePath = pwszFilePath;
	pTemplate->wstrName = ut::StrToWstr(itTName->value.GetString());
	pTemplate->u64Offset = u64Offset;

	UmapCustomTypes umapCT;
	std::uint8_t uCustomTypeID = 1; //ID starts at 1.
	if (const auto objCustomTypes = docJSON.FindMember("CustomTypes");
		objCustomTypes != docJSON.MemberEnd() && objCustomTypes->value.IsArray()) {
		for (auto pCustomType = objCustomTypes->value.Begin(); pCustomType != objCustomTypes->value.End();
			++pCustomType, ++uCustomTypeID) {
			if (!pCustomType->IsObject()) {
				ut::DBG_REPORT(std::format(L"{}\r\nEach CustomTypes' array entry must be an Object.", pwszFilePath).data());
				return -1;
			}

			const auto optName = CHexDlgTemplMgr::JSONFindMemberAsString(pCustomType, "TypeName");
			if (!optName) {
				ut::DBG_REPORT(std::format(L"{}\r\nEach array entry (Object) must have a string 'TypeName' property.", pwszFilePath).data());
				return -1;
			}

			const auto wstrTypeName = ut::StrToWstr(*optName);
			const auto clrBk = CHexDlgTemplMgr::JSONColors(pCustomType, "clrBk").value_or(-1);
			const auto clrText = CHexDlgTemplMgr::JSONColors(pCustomType, "clrText").value_or(-1);
			const auto fBigEndian = CHexDlgTemplMgr::JSONIsBigEndianness(pCustomType).value_or(false);

			const auto optFieldsArray = CHexDlgTemplMgr::JSONFindMember(pCustomType, "Fields");
			if (!optFieldsArray || !(*optFieldsArray)->value.IsArray()) {
				ut::DBG_REPORT(std::format(L"{}\r\nEach 'Fields' must be an array.", pwszFilePath).data());
				return -1;
			}

			umapCT.try_emplace(uCustomTypeID, VecHexTemplFields { });
			const CHexDlgTemplMgr::FIELDSDEFPROPS stDefTypes { .stClr { clrBk, clrText }, .pTemplate { pTemplate },
				.fBigEndian { fBigEndian } };
			if (!CHexDlgTemplMgr::JSONParseFields(*optFieldsArray, umapCT[uCustomTypeID], stDefTypes, umapCT, nullptr,
				GetHexCtrl())) {
				return { }; //Something went wrong during template parsing.
			}
			pTemplate->vecCustomType.emplace_back(std::move(wstrTypeName), uCustomTypeID);
		}
	}

	const auto objData = docJSON.FindMember("Data");
	if (objData == docJSON.MemberEnd() || !objData->value.IsObject()) {
		ut::DBG_REPORT(std::format(L"{}\r\nNo 'Data' object member in the template.", pwszFilePath).data());
		return -1;
	}

	const auto clrBk = CHexDlgTemplMgr::JSONColors(&objData->value, "clrBk").value_or(-1);
	const auto clrText = CHexDlgTemplMgr::JSONColors(&objData->value, "clrText").value_or(-1);
	const auto fBigEndian = CHexDlgTemplMgr::JSONIsBigEndianness(&objData->value).value_or(false);
	const CHexDlgTemplMgr::FIELDSDEFPROPS stDefFields { .stClr { clrBk, clrText }, .pTemplate { pTemplate },
		.fBigEndian { fBigEndian } };

	const auto optFieldsArray = CHexDlgTemplMgr::JSONFindMember(&objData->value, "Fields");
	auto& vecFields = pTemplate->vecFields;
	if (!CHexDlgTemplMgr::JSONParseFields(*optFieldsArray, vecFields, stDefFields, umapCT, nullptr, GetHexCtrl())) {
		return -1;
	}

	pTemplate->iSizeTotal = std::reduce(vecFields.begin(), vecFields.end(), 0,
		[](auto iTotal, const std::unique_ptr<HEXTEMPLFIELD>& pData) { return iTotal + pData->iSize; });
	pTemplate->iTemplateID = TMPLGetIDForNewTemplate();
	m_vecTemplates.emplace_back(std::move(pTemplateUtr));
	GUIOnTemplateApplyDisapply(pTemplate->iTemplateID, true);
	RedrawHexCtrl();

	return pTemplate->iTemplateID;
}

void CHexDlgTemplMgr::CreateDlg()const
{
	//m_Wnd is set in the WMInitDialog().
	if (const auto hWnd = ::CreateDialogParamW(m_hInstRes, MAKEINTRESOURCEW(IDD_HEXCTRL_TEMPLMGR),
		m_pHexCtrl->GetWndHandle(EHexWnd::WND_MAIN), GDIUT::DlgProc<CHexDlgTemplMgr>, reinterpret_cast<LPARAM>(this));
		hWnd == nullptr) {
		ut::DBG_REPORT(L"CreateDialogParamW failed.");
	}
}

void CHexDlgTemplMgr::DestroyDlg() {
	if (m_Wnd.IsWindow()) {
		m_Wnd.DestroyWindow();
	}
}

void CHexDlgTemplMgr::DisapplyAll() {
	if (m_Wnd.IsWindow()) { //Dialog must be created and alive to work with its members.
		m_WndTree.DeleteAllItems();
		m_ListEx.SetItemCountEx(0);
		GUIUpdateStaticText();
	}

	m_pVecFieldsCurr = nullptr;
	m_hTreeCurrParent = nullptr;
	m_vecTemplates.clear();
	RedrawHexCtrl();
}

void CHexDlgTemplMgr::DisapplyByID(int iTemplateID) {
	GUIOnTemplateApplyDisapply(iTemplateID, false);
	TMPLRemoveAppliedByID(iTemplateID);
	RedrawHexCtrl();
}

void CHexDlgTemplMgr::DisapplyByOffset(std::uint64_t u64Offset) {
	if (const auto pAppl = TMPLGetTemplateByOffset(u64Offset); pAppl != nullptr) {
		GUIOnTemplateApplyDisapply(pAppl->iTemplateID, false);
		TMPLRemoveAppliedByID(pAppl->iTemplateID);
		RedrawHexCtrl();
	}
}

auto CHexDlgTemplMgr::GetAllApplied()->VecHexTemplatesApplied {
	VecHexTemplatesApplied vec;
	for (const auto& uptr : m_vecTemplates) {
		vec.emplace_back(HEXTEMPLATEAPPLIED { .wstrFilePath { uptr->wstrFilePath }, .u64Offset { uptr->u64Offset } });
	}

	return vec;
}

auto CHexDlgTemplMgr::GetDlgItemHandle(EHexDlgItem eItem)const->HWND
{
	if (!m_Wnd.IsWindow()) {
		return { };
	}

	using enum EHexDlgItem;
	switch (eItem) {
	case TEMPLMGR_CHK_MIN: return m_WndBtnMin;
	case TEMPLMGR_CHK_TT: return m_WndBtnTT;
	case TEMPLMGR_CHK_HGL: return m_WndBtnHighlight;
	case TEMPLMGR_CHK_HEX: return m_WndBtnHex;
	case TEMPLMGR_CHK_SWAP: return m_WndBtnEndian;
	default: return { };
	}
}

auto CHexDlgTemplMgr::GetHWND()const->HWND {
	return m_Wnd;
}

bool CHexDlgTemplMgr::HasApplied()const {
	return !m_vecTemplates.empty();
}

bool CHexDlgTemplMgr::HasCurrent()const {
	return m_Wnd.IsWindow() && TMPLHasTemplateFiles();
}

auto CHexDlgTemplMgr::HitTest(std::uint64_t u64Offset)const->PCHEXTEMPLFIELD {
	const auto rit = std::find_if(m_vecTemplates.rbegin(), m_vecTemplates.rend(),
		[u64Offset](const std::unique_ptr<HEXTEMPLATE>& uptr) {
			return u64Offset >= uptr->u64Offset && u64Offset < uptr->u64Offset + uptr->iSizeTotal; });
	if (rit == m_vecTemplates.rend()) {
		return nullptr;
	}

	const auto pTemplate = rit->get();
	const auto ullOffsetApplied = pTemplate->u64Offset;
	const auto& vecFields = pTemplate->vecFields;
	const auto lmbFind = [u64Offset, ullOffsetApplied](const VecHexTemplFields& vecFields)->PCHEXTEMPLFIELD {
		const auto _lmbFind = [u64Offset, ullOffsetApplied]
		(const auto& lmbSelf, const VecHexTemplFields& vecFields)->PCHEXTEMPLFIELD {
			for (const auto& pField : vecFields) {
				if (pField->vecNested.empty()) {
					const auto ullOffsetCurr = ullOffsetApplied + pField->iOffset;
					if (u64Offset < (ullOffsetCurr + pField->iSize)) {
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
		return _lmbFind(_lmbFind, vecFields);
		};

	return lmbFind(vecFields);
}

void CHexDlgTemplMgr::Initialize(IHexCtrl &HexCtrl, HINSTANCE hInstRes) {
	m_pHexCtrl = &HexCtrl;
	m_hInstRes = hInstRes;
}

bool CHexDlgTemplMgr::IsShowTooltips()const {
	return m_WndBtnTT.IsWindow() && m_WndBtnTT.IsChecked();
}

bool CHexDlgTemplMgr::PreTranslateMsg(MSG* pMsg) {
	return m_Wnd.IsDlgMessage(pMsg);
}

auto CHexDlgTemplMgr::ProcessMsg(const MSG& msg)->INT_PTR
{
	switch (msg.message) {
	case WM_ACTIVATE: return WMActivate(msg);
	case WM_CLOSE: return WMClose();
	case WM_COMMAND: return WMCommand(msg);
	case WM_CTLCOLORSTATIC: return WMCtlColorStatic(msg);
	case WM_DESTROY: return WMDestroy();
	case WM_DPICHANGED: return WMDPIChanged(msg);
	case WM_DRAWITEM: return WMDrawItem(msg);
	case WM_GETDPISCALEDSIZE: return WMGetDPIScaledSize(msg);
	case WM_INITDIALOG: return WMInitDialog(msg);
	case WM_LBUTTONDOWN: return WMLButtonDown(msg);
	case WM_LBUTTONUP: return WMLButtonUp(msg);
	case WM_MEASUREITEM: return WMMeasureItem(msg);
	case WM_MOUSEACTIVATE: return WMMouseActivate(msg);
	case WM_NOTIFY: return WMNotify(msg);
	case WM_SIZE: return WMSize(msg);
	default:
		return 0;
	}
}

void CHexDlgTemplMgr::RemoveAllTemplates() {
	DisapplyAll(); //m_vecTemplates is empty after that.

	for (const auto& uptr : m_vecTemplateFiles) {
		GUIOnTemplateAddRemove(uptr->data(), false);
	}

	m_vecTemplateFiles.clear();
}

void CHexDlgTemplMgr::RemoveTemplateFile(const wchar_t* pwszFilePath) {
	GUIOnTemplateAddRemove(pwszFilePath, false);
	TMPLRemoveAppliedByFilePath(pwszFilePath);
	TMPLRemoveTemplateFile(pwszFilePath);
	SetDlgButtonsState();
}

void CHexDlgTemplMgr::SetDlgProperties(std::uint64_t u64Flags) {
	m_u64Flags = u64Flags;
}

void CHexDlgTemplMgr::ShowTooltips(bool fShow) {
	m_WndBtnTT.SetCheck(fShow);
}

void CHexDlgTemplMgr::ShowWindow(int iCmdShow) {
	if (!m_Wnd.IsWindow()) {
		CreateDlg();
	}

	m_Wnd.ShowWindow(iCmdShow);
}

void CHexDlgTemplMgr::UpdateData() {
	if (!m_Wnd.IsWindow() || !m_Wnd.IsWindowVisible()) {
		return;
	}

	m_ListEx.RedrawWindow();
}

auto CHexDlgTemplMgr::GetDataFromField(const HEXTEMPLFIELD* pField, PCHEXTEMPLATE pTemplate, IHexCtrl* pHexCtrl)
->std::optional<std::uint64_t> {
	//Retrieves data from HexCtrl by template+pField offset and field size.

	assert(pHexCtrl != nullptr);
	const auto u64HexDataSize = pHexCtrl->GetDataSize();
	const auto u64FieldOffset = pTemplate->u64Offset + pField->iOffset;

	using enum EHexTemplFieldType;
	switch (pField->eType) {
	case type_int8:
	case type_uint8:
		if ((u64FieldOffset + 1) > u64HexDataSize) {
			return std::nullopt;
		}

		return ut::GetIHexTData<std::uint8_t>(*pHexCtrl, u64FieldOffset);
	case type_int16:
	case type_uint16:
	{
		if ((u64FieldOffset + 2) > u64HexDataSize) {
			return std::nullopt;
		}

		auto u16 = ut::GetIHexTData<std::uint16_t>(*pHexCtrl, u64FieldOffset);
		if (pField->fBigEndian) {
			u16 = ut::ByteSwap(u16);
		}
		return u16;
	}
	case type_int32:
	case type_uint32:
	{
		if ((u64FieldOffset + 4) > u64HexDataSize) {
			return std::nullopt;
		}

		auto u32 = ut::GetIHexTData<std::uint16_t>(*pHexCtrl, u64FieldOffset);
		if (pField->fBigEndian) {
			u32 = ut::ByteSwap(u32);
		}
		return u32;
	}
	case type_int64:
	case type_uint64:
	{
		if ((u64FieldOffset + 8) > u64HexDataSize) {
			return std::nullopt;
		}

		auto u64 = ut::GetIHexTData<std::uint16_t>(*pHexCtrl, u64FieldOffset);
		if (pField->fBigEndian) {
			u64 = ut::ByteSwap(u64);
		}
		return u64;
	}
	default:
		ut::DBG_REPORT(L"Only integral types are allowed as field types.");
		return std::nullopt;
	}
}

auto CHexDlgTemplMgr::JSONIsBigEndianness(const rapidjson::Value* pValue)->std::optional<bool> {
	const auto optEndianness = CHexDlgTemplMgr::JSONFindMemberAsString(pValue, "endianness");
	if (!optEndianness) {
		return std::nullopt;
	}

	const std::string_view svEndianness = *optEndianness;
	if (svEndianness != "big" && svEndianness != "little") {
		ut::DBG_REPORT(L"Unknown 'endianness'.");
		return std::nullopt;
	}

	return svEndianness == "big";
}

auto CHexDlgTemplMgr::JSONFindMember(const rapidjson::Value* pValue, const char* pszName)->std::optional<IterJSONMember> {
	const auto itMember = pValue->FindMember(pszName);
	return itMember != pValue->MemberEnd() ? std::optional<IterJSONMember>{ itMember } : std::nullopt;
}

auto CHexDlgTemplMgr::JSONFindMemberAsInt32(const rapidjson::Value* pValue, const char* pszName)->std::optional<int> {
	const auto optMember = JSONFindMember(pValue, pszName);
	return (optMember && (*optMember)->value.IsInt()) ? std::optional<int>{(*optMember)->value.GetInt()} : std::nullopt;
}

auto CHexDlgTemplMgr::JSONFindMemberAsUInt32(const rapidjson::Value* pValue, const char* pszName)->std::optional<std::uint32_t> {
	const auto optMember = JSONFindMember(pValue, pszName);
	return (optMember && (*optMember)->value.IsUint()) ? std::optional<int>{(*optMember)->value.GetUint()} : std::nullopt;
}

auto CHexDlgTemplMgr::JSONFindMemberAsObject(const rapidjson::Value* pValue, const char* pszName)
->std::optional<const rapidjson::Value*> {
	const auto optMember = JSONFindMember(pValue, pszName);
	return (optMember && (*optMember)->value.IsObject()) ?
		std::optional<const rapidjson::Value*>{&((*optMember)->value)} : std::nullopt;
}

auto CHexDlgTemplMgr::JSONFindMemberAsString(const rapidjson::Value* pValue, const char* pszName)->std::optional<const char*> {
	const auto optMember = JSONFindMember(pValue, pszName);
	return (optMember && (*optMember)->value.IsString()) ?
		std::optional<const char*>{(*optMember)->value.GetString()} : std::nullopt;
}

auto CHexDlgTemplMgr::JSONColors(const rapidjson::Value* pValue, const char* pszColorName)->std::optional<COLORREF> {
	const auto optClr = CHexDlgTemplMgr::JSONFindMemberAsString(pValue, pszColorName);
	if (!optClr) {
		return std::nullopt;
	}

	const std::string_view sv { *optClr };
	if (sv.empty() || sv.size() != 7 || sv[0] != '#')
		return std::nullopt;

	const auto R = *stn::StrToUInt32(sv.substr(1, 2), 16);
	const auto G = *stn::StrToUInt32(sv.substr(3, 2), 16);
	const auto B = *stn::StrToUInt32(sv.substr(5, 2), 16);

	return RGB(R, G, B);
}

bool CHexDlgTemplMgr::JSONParseFields(const IterJSONMember itFieldsArray, VecHexTemplFields& vecFields,
	const FIELDSDEFPROPS& defProps, UmapCustomTypes& umapCustomT, int* pFieldOffset, IHexCtrl* pHexCtrl) {
	using enum EHexTemplFieldType;

	//Mapping from the JSON 'type' string to EHexTemplFieldType enum.
	//All type names here are in lower case, but in JSON file they can be in any case.
	//We transform all names from JSON to lower case before comparing.
	static const std::unordered_map<std::string_view, EHexTemplFieldType> umapStrToEType {
		{ "bool", type_bool },
		{ "int8", type_int8 }, { "char", type_int8 },
		{ "uint8", type_uint8 }, { "unsigned char", type_uint8 }, { "byte", type_uint8 },
		{ "int16", type_int16 }, { "short", type_int16 },
		{ "uint16", type_uint16 }, { "unsigned short", type_uint16 }, { "word", type_uint16 },
		{ "int32", type_int32 }, { "long", type_int32 }, { "int", type_int32 },
		{ "uint32", type_uint32 }, { "unsigned long", type_uint32 }, { "unsigned int", type_uint32 }, { "dword", type_uint32 },
		{ "int64", type_int64 }, { "long long", type_int64 },
		{ "uint64", type_uint64 }, { "unsigned long long", type_uint64 }, { "qword", type_uint64 },
		{ "float", type_float }, { "double", type_double },
		{ "time32_t", type_time32 }, { "time64_t", type_time64 },
		{ "filetime", type_filetime }, { "systemtime", type_systemtime }, { "guid", type_guid } };
	static const std::unordered_map<EHexTemplFieldType, int> umapTypeToSize { //Types sizes.
		{ type_bool, static_cast<int>(sizeof(bool)) }, { type_int8, static_cast<int>(sizeof(char)) },
		{ type_uint8, static_cast<int>(sizeof(char)) }, { type_int16, static_cast<int>(sizeof(short)) },
		{ type_uint16, static_cast<int>(sizeof(short)) }, { type_int32, static_cast<int>(sizeof(int)) },
		{ type_uint32, static_cast<int>(sizeof(int)) }, { type_int64, static_cast<int>(sizeof(long long)) },
		{ type_uint64, static_cast<int>(sizeof(long long)) }, { type_float, static_cast<int>(sizeof(float)) },
		{ type_double, static_cast<int>(sizeof(double)) }, { type_time32, static_cast<int>(sizeof(__time32_t)) },
		{ type_time64, static_cast<int>(sizeof(__time64_t)) }, { type_filetime, static_cast<int>(sizeof(FILETIME)) },
		{ type_systemtime, static_cast<int>(sizeof(SYSTEMTIME)) }, { type_guid, static_cast<int>(sizeof(GUID)) }
	};

	const auto lmbTotalSize = [](const VecHexTemplFields& vecFields)->int {
		//Counts the total size of all fields in the VecHexTemplFields, recursively.
		const auto _lmbTotalSize = [](const auto& lmbSelf, const VecHexTemplFields& vecFields)->int {
			return std::reduce(vecFields.begin(), vecFields.end(), 0,
				[&lmbSelf](auto ullTotal, const std::unique_ptr<HEXTEMPLFIELD>& pField) {
					if (!pField->vecNested.empty()) {
						return ullTotal + lmbSelf(lmbSelf, pField->vecNested);
					}
					return ullTotal + pField->iSize; });
			};
		return _lmbTotalSize(_lmbTotalSize, vecFields);
		};

	int iOffset { 0 }; //Default starting field offset.
	if (pFieldOffset == nullptr) {
		pFieldOffset = &iOffset;
	}

	for (auto pField = itFieldsArray->value.Begin(); pField != itFieldsArray->value.End(); ++pField) {
		if (!pField->IsObject()) {
			ut::DBG_REPORT(L"Each 'Fields' array entry must be an Object {}.");
			return false;
		}

		std::wstring wstrNameField;
		if (const auto optName = JSONFindMemberAsString(pField, "name"); optName) {
			wstrNameField = ut::StrToWstr(*optName);
		}
		else {
			ut::DBG_REPORT(L"Each array entry (Object) must have a 'name' property.");
			return false;
		}

		const auto& pNewField = vecFields.emplace_back(std::make_unique<HEXTEMPLFIELD>());
		pNewField->wstrName = wstrNameField;
		pNewField->iOffset = *pFieldOffset;
		pNewField->stClr.clrBk = JSONColors(pField, "clrBk").value_or(defProps.stClr.clrBk);
		pNewField->stClr.clrText = JSONColors(pField, "clrText").value_or(defProps.stClr.clrText);
		pNewField->pFieldParent = defProps.pFieldParent;
		pNewField->eType = type_custom;
		pNewField->fBigEndian = JSONIsBigEndianness(pField).value_or(defProps.fBigEndian);

		if (const auto optNestedFields = JSONFindMember(pField, "Fields"); optNestedFields) {
			if (!(*optNestedFields)->value.IsArray()) {
				ut::DBG_REPORT(L"Each 'Fields' must be an Array.");
				return false;
			}

			//Setting defaults for the next nested fields.
			const FIELDSDEFPROPS stDefsNested { .stClr { pNewField->stClr }, .pTemplate { defProps.pTemplate },
				.pFieldParent { pNewField.get() }, .fBigEndian { pNewField->fBigEndian } };

			//Recursion for nested fields starts here.
			if (!JSONParseFields(*optNestedFields, pNewField->vecNested, stDefsNested, umapCustomT, pFieldOffset, pHexCtrl)) {
				return false;
			}

			pNewField->iSize = lmbTotalSize(pNewField->vecNested); //Total size of all nested fields.
		}
		else {
			if (const auto optDescr = JSONFindMemberAsString(pField, "description"); optDescr) {
				pNewField->wstrDescr = ut::StrToWstr(*optDescr);
			}

			//The "array" field can be an int or an Object.
			std::uint32_t uArraySize { 0 };
			if (const auto optArray = JSONFindMember(pField, "array"); optArray) {
				if ((*optArray)->value.IsInt() && ((*optArray)->value.GetInt() > 1)) { //Array is a plain int.
					uArraySize = (*optArray)->value.GetInt();
				}
				else if ((*optArray)->value.IsObject()) { //Array is an object.
					const auto pArrayObj = &((*optArray)->value);
					const auto optSizeFieldName = JSONFindMemberAsString(pArrayObj, "sizefield");
					if (!optSizeFieldName) {
						ut::DBG_REPORT(L"Each 'array' object entry must have a 'sizefield' property.");
						return false;
					}

					//Field name to get the size from.
					const auto wstrSizeFieldName = ut::StrToWstr(*optSizeFieldName);
					//Field to get the size of the array from.
					const auto pSizeField = TMPLFindFieldName(defProps.pTemplate, wstrSizeFieldName);
					if (pSizeField == nullptr) {
						ut::DBG_REPORT(std::format(L"Field '{}' could not be found.", wstrSizeFieldName).data());
						return false;
					}

					if (!pHexCtrl->IsDataSet()) {
						ut::DBG_REPORT(L"Dynamic templates require data to be set in order to work.");
						return false;
					}

					const auto optData = GetDataFromField(pSizeField, defProps.pTemplate, pHexCtrl);
					if (!optData) {
						ut::DBG_REPORT(L"Could not retrieve the data from the HexCtrl.");
						return false;
					}

					const auto optLimitData = JSONProcessLimitObject(pArrayObj, *optData);
					if (!optLimitData) {
						return false;
					}

					uArraySize = static_cast<std::uint32_t>(*optLimitData);
				}
			}

			if (const auto optJump = JSONFindMemberAsObject(pField, "jump"); optJump) {
				//The "anchor" is a mandatory field.
				if (const auto optAnchor = JSONFindMemberAsString(*optJump, "anchor"); optAnchor) {
					auto uptrJump = std::make_unique<HEXTEMPLJUMP>();
					using enum EHexTemplJumpAnchor;

					if (IsEqualNoCase(*optAnchor, "datastart")) {
						uptrJump->eAnchor = DATA_START;
					}
					else if (IsEqualNoCase(*optAnchor, "dataend")) {
						uptrJump->eAnchor = DATA_END;
					}
					else if (IsEqualNoCase(*optAnchor, "fieldthis") || IsEqualNoCase(*optAnchor, "here")) {
						uptrJump->eAnchor = FIELD_THIS;
					}
					else if (IsEqualNoCase(*optAnchor, "fieldfirst") || IsEqualNoCase(*optAnchor, "structstart")) {
						uptrJump->eAnchor = FIELD_FIRST;
					}
					else {
						uptrJump->eAnchor = OFFSET_CUSTOM;
						uptrJump->u64Anchor = stn::StrToUInt64(*optAnchor).value_or(0ULL);
					}

					if (const auto optDirection = JSONFindMemberAsString(*optJump, "direction"); optDirection) {
						using enum EHexTemplJumpDirection;
						uptrJump->eDirection = IsEqualNoCase(*optDirection, "backward") ? JUMP_BACKWARD : JUMP_FORWARD;
					}

					if (const auto optUnits = JSONFindMemberAsString(*optJump, "units"); optUnits) {
						if (IsEqualNoCase(*optUnits, "byte")) {
							uptrJump->u32Units = 1;
						}
						else if (IsEqualNoCase(*optUnits, "word")) {
							uptrJump->u32Units = 2;
						}
						else if (IsEqualNoCase(*optUnits, "dword")) {
							uptrJump->u32Units = 4;
						}
						else if (IsEqualNoCase(*optUnits, "qword")) {
							uptrJump->u32Units = 8;
						}
						else {
							uptrJump->u32Units = stn::StrToUInt32(*optUnits).value_or(1UL);
						}
					}

					pNewField->pJump = std::move(uptrJump);
				}
			}

			int iSize { 0 }; //Current field's size, via "type" or "size" property.
			if (const auto optType = JSONFindMember(pField, "type"); optType) {
				if (!(*optType)->value.IsString()) {
					ut::DBG_REPORT(L"Field 'type' must be a string.");
					return false;
				}

				const auto pszType = (*optType)->value.GetString();
				std::string strTypeLowerCase = pszType; //Lowering the case of the 'type' string.
				std::ranges::transform(strTypeLowerCase, strTypeLowerCase.begin(),
					[](char ch) { return static_cast<char>(std::tolower(ch)); });

				if (const auto itMapType = umapStrToEType.find(strTypeLowerCase); itMapType != umapStrToEType.end()) {
					pNewField->eType = itMapType->second;
					iSize = umapTypeToSize.at(itMapType->second);
				}
				else { //If it's not any standard type, we try to find custom type with the given name.
					const auto& vecCTypes = defProps.pTemplate->vecCustomType;
					const auto itVecCT = std::find_if(vecCTypes.begin(), vecCTypes.end(),
						[=](const HEXTEMPLCT& ct) { return ct.wstrTypeName == ut::StrToWstr(pszType); });
					if (itVecCT == vecCTypes.end()) {
						ut::DBG_REPORT(L"Unknown 'type' of the field.");
						return false;
					}

					pNewField->iCustomTypeID = itVecCT->iTypeID; //Custom type ID.
					pNewField->eType = type_custom;
					const auto lmbCopyCustomType = [](const VecHexTemplFields& vecCustomFields,
						const PtrHexTemplField& pField, int& iOffset)->void {
							const auto _lmbCustomTypeCopy = [](const auto& lmbSelf,
								const VecHexTemplFields& vecCustomFields, const PtrHexTemplField& pField, int& iOffset)->void {
									for (const auto& pCustomField : vecCustomFields) {
										const auto& pNewField = pField->vecNested.emplace_back(std::make_unique<HEXTEMPLFIELD>());
										const auto& CFClr = pCustomField->stClr;
										pNewField->wstrName = pCustomField->wstrName;
										pNewField->wstrDescr = pCustomField->wstrDescr;
										pNewField->iOffset = iOffset;
										pNewField->iSize = pCustomField->iSize;
										pNewField->stClr.clrBk = CFClr.clrBk == -1 ? pField->stClr.clrBk : CFClr.clrBk;
										pNewField->stClr.clrText = CFClr.clrText == -1 ? pField->stClr.clrText : CFClr.clrText;
										pNewField->pFieldParent = pField.get();
										pNewField->pJump = pField->pJump != nullptr ? std::make_unique<HEXTEMPLJUMP>(*pField->pJump) : nullptr;
										pNewField->eType = pCustomField->eType;
										pNewField->iCustomTypeID = pCustomField->iCustomTypeID;
										pNewField->fBigEndian = pCustomField->fBigEndian;

										if (pCustomField->vecNested.empty()) {
											iOffset += pCustomField->iSize;
										}
										else {
											lmbSelf(lmbSelf, pCustomField->vecNested, pNewField, iOffset);
										}
									}
								};
							_lmbCustomTypeCopy(_lmbCustomTypeCopy, vecCustomFields, pField, iOffset);
						};

					auto iOffsetCustomType = *pFieldOffset;
					if (uArraySize <= 1) {
						lmbCopyCustomType(umapCustomT[itVecCT->iTypeID], pNewField, iOffsetCustomType);
					}
					else { //Creating array of Custom Types.
						for (auto uArrIndex = 0U; uArrIndex < uArraySize; ++uArrIndex) {
							const auto& pFieldArray = pNewField->vecNested.emplace_back(std::make_unique<HEXTEMPLFIELD>());
							pFieldArray->wstrName = std::format(L"{}[{}]", wstrNameField, uArrIndex);
							pFieldArray->iOffset = iOffsetCustomType;
							pFieldArray->stClr = pNewField->stClr;
							pFieldArray->pFieldParent = pNewField.get();
							pFieldArray->pJump = pNewField->pJump != nullptr ?
								std::make_unique<HEXTEMPLJUMP>(*pNewField->pJump) : nullptr;
							pFieldArray->eType = pNewField->eType;
							pFieldArray->iCustomTypeID = pNewField->iCustomTypeID;
							pFieldArray->fBigEndian = pNewField->fBigEndian;

							//Copy Custom Type fields into the pFieldArray->vecNested.
							lmbCopyCustomType(umapCustomT[itVecCT->iTypeID], pFieldArray, iOffsetCustomType);
							pFieldArray->iSize = lmbTotalSize(pFieldArray->vecNested);
						}
						pNewField->wstrName = std::format(L"{}[{}]", wstrNameField, uArraySize);
					}

					iSize = lmbTotalSize(pNewField->vecNested);
				}
			}
			else { //No "type" property was found.
				const auto optSize = JSONFindMemberAsInt32(pField, "size");
				if (!optSize) {
					ut::DBG_REPORT(L"No integer 'size', or 'type' property was found.");
					return false;
				}

				if (*optSize < 1) {
					ut::DBG_REPORT(L"The 'size' must be > 0.");
					return false;
				}

				iSize = *optSize;
				pNewField->eType = custom_size;
			}

			if (uArraySize > 1 && pNewField->eType != type_custom) { //Creating array of standard/default types.
				for (auto uArrIndex = 0U; uArrIndex < uArraySize; ++uArrIndex) {
					const auto& pFieldArray = pNewField->vecNested.emplace_back(std::make_unique<HEXTEMPLFIELD>());
					pFieldArray->wstrName = std::format(L"{}[{}]", wstrNameField, uArrIndex);
					pFieldArray->iOffset = *pFieldOffset + (uArrIndex * iSize);
					pFieldArray->iSize = iSize;
					pFieldArray->stClr = pNewField->stClr;
					pFieldArray->pFieldParent = pNewField.get();
					pFieldArray->pJump = pNewField->pJump != nullptr ?
						std::make_unique<HEXTEMPLJUMP>(*pNewField->pJump) : nullptr;
					pFieldArray->eType = pNewField->eType;
					pFieldArray->iCustomTypeID = pNewField->iCustomTypeID;
					pFieldArray->fBigEndian = pNewField->fBigEndian;
				}
				pNewField->wstrName = std::format(L"{}[{}]", wstrNameField, uArraySize);
				iSize *= uArraySize;
			}

			pNewField->iSize = iSize;
			*pFieldOffset += iSize;
		}
	}

	return true;
}

auto CHexDlgTemplMgr::JSONProcessLimitObject(const rapidjson::Value* pValue, std::uint64_t u64ActualData)
->std::optional<std::uint64_t> {
	//Finds and parses "limit" object, comparing its min/max properties against u64ActualData.

	const auto optLimit = JSONFindMemberAsObject(pValue, "limit");
	if (!optLimit) { //If no "limit" object found we just return back the actual data.
		return u64ActualData;
	}

	const auto pLimitObj = *optLimit;

	std::uint64_t u64LimitMin { 0 };
	if (const auto optLimitMin = JSONFindMemberAsUInt32(pLimitObj, "min"); optLimitMin) {
		u64LimitMin = *optLimitMin;
	}

	std::uint64_t u64LimitMax { 0xFFFFFFFFFFFFFFFFULL };
	if (const auto optLimitMax = JSONFindMemberAsUInt32(pLimitObj, "max"); optLimitMax) {
		u64LimitMax = *optLimitMax;
	}

	//Ad hoc enum.
	enum class EOnBeyond : std::uint8_t { DO_STOP, DO_ASK, USE_LIMIT } eBeyond { EOnBeyond::DO_STOP };
	using enum EOnBeyond;
	if (const auto optOnBeyond = JSONFindMemberAsString(pLimitObj, "onbeyond"); optOnBeyond) {
		const std::string_view svOnBeyond = *optOnBeyond;
		if (svOnBeyond == "stop") {
			eBeyond = DO_STOP;
		}
		else if (svOnBeyond == "ask") {
			eBeyond = DO_ASK;
		}
		else if (svOnBeyond == "uselimit") {
			eBeyond = USE_LIMIT;
		}
	}

	if (u64ActualData < u64LimitMin) {
		switch (eBeyond) {
		case DO_STOP:
			return std::nullopt;
		case DO_ASK:
		{
			const auto optSizeFieldName = JSONFindMemberAsString(pValue, "sizefield");
			//Field name to get the size from.
			const auto wstrSizeFieldName = ut::StrToWstr(*optSizeFieldName);
			const auto wstr = std::format(L"The data of the `{}` field is smaller than the limit 'min'.\r\n"
				L"Min limit is: {}, actual data is: {}\r\n"
				L"Use this number anyway?", wstrSizeFieldName, u64LimitMin, u64ActualData);
			if (::MessageBoxW(0, wstr.data(), wstrSizeFieldName.data(), MB_YESNO | MB_ICONEXCLAMATION) == IDYES) {
				return u64ActualData;
			}
		}
		break;
		case USE_LIMIT:
			return u64LimitMin;
		default:
			break;
		}
	}
	else if (u64ActualData > u64LimitMax) {
		switch (eBeyond) {
		case DO_STOP:
			return std::nullopt;
		case DO_ASK:
		{
			const auto optSizeFieldName = JSONFindMemberAsString(pValue, "sizefield");
			//Field name to get the size from.
			const auto wstrSizeFieldName = ut::StrToWstr(*optSizeFieldName);

			const auto wstr = std::format(L"The data of the `{}` field is bigger than the limit 'max'.\r\n"
				L"Max limit is: {}, actual data is: {}\r\n"
				L"Use this number anyway?", wstrSizeFieldName, u64LimitMax, u64ActualData);
			if (::MessageBoxW(0, wstr.data(), wstrSizeFieldName.data(), MB_YESNO | MB_ICONEXCLAMATION) == IDYES) {
				return u64ActualData;
			}
		}
		break;
		case USE_LIMIT:
			return u64LimitMax;
		default:
			break;
		}
	}

	return std::nullopt;
}

bool CHexDlgTemplMgr::IsEqualNoCase(std::string_view sv1, std::string_view sv2) {
	return std::ranges::equal(sv1, sv2, { }, [](unsigned char c) { return std::tolower(c); },
		[](unsigned char c) { return std::tolower(c); });
}

auto CHexDlgTemplMgr::TMPLFindFieldName(PCHEXTEMPLATE pTemplate, std::wstring_view wsvFieldName)->PCHEXTEMPLFIELD {
	//Recursively searches for the field name wsvFieldName in all template fields (HEXTEMPLFIELD::wstrName).

	assert(pTemplate != nullptr);
	const auto lmbFindFieldName = [](const VecHexTemplFields& vecFields, std::wstring_view wsvFieldName)->PCHEXTEMPLFIELD {
		const auto _lmbFindFieldName = [](const auto& lmbSelf, const VecHexTemplFields& vecFields, std::wstring_view wsvFieldName)
			->PCHEXTEMPLFIELD {
			if (const auto it = std::ranges::find_if(vecFields, [wsvFieldName](const PtrHexTemplField& uptr) {
				return uptr->wstrName == wsvFieldName; });
				it != vecFields.end()) { //We found "sizefield"'s string as a field name.
				return it->get();
			}

			for (const auto& uptr : vecFields) {
				if (const auto pField = lmbSelf(lmbSelf, uptr->vecNested, wsvFieldName); pField != nullptr) {
					return pField;
				}
			}

			return nullptr;
			};

		return _lmbFindFieldName(_lmbFindFieldName, vecFields, wsvFieldName);
		};
	return lmbFindFieldName(pTemplate->vecFields, wsvFieldName);
}


//Private methods.

void CHexDlgTemplMgr::CreateArrows()
{
	const auto hDC = m_WndBtnMin.GetDC();
	const auto iWidth = m_WndBtnMin.GetWindowRect().Width();
	const auto iHeight = m_WndBtnMin.GetWindowRect().Height();
	::DeleteObject(m_hBmpMin);
	::DeleteObject(m_hBmpMax);
	m_hBmpMin = GDIUT::CreateArrowBitmap(hDC, iWidth, iHeight, 1, ::GetSysColor(COLOR_3DFACE), ::GetSysColor(COLOR_GRAYTEXT));
	m_hBmpMax = GDIUT::CreateArrowBitmap(hDC, iWidth, iHeight, -1, ::GetSysColor(COLOR_3DFACE), ::GetSysColor(COLOR_GRAYTEXT));
	m_WndBtnMin.ReleaseDC(hDC);
	m_WndBtnMin.SetBitmap(IsMinimized() ? m_hBmpMax : m_hBmpMin);
}

auto CHexDlgTemplMgr::GetHexCtrl()const->IHexCtrl* {
	return m_pHexCtrl;
}

auto CHexDlgTemplMgr::GUIGetTemplateIDFromTree(HTREEITEM hTreeItem)->int {
	auto hRoot = hTreeItem;
	while (hRoot != nullptr) { //Root node.
		hTreeItem = hRoot;
		hRoot = m_WndTree.GetNextItem(hTreeItem, TVGN_PARENT);
	}

	return static_cast<int>(m_WndTree.GetItemData(hTreeItem));
}

auto CHexDlgTemplMgr::GUIGetTreeSelectedTemplate()->PCHEXTEMPLATE {
	return TMPLGetTemplateByID(GUIGetTreeSelectedTemplateID());
}

auto CHexDlgTemplMgr::GUIGetTreeSelectedTemplateID()->int {
	return GUIGetTemplateIDFromTree(m_WndTree.GetSelectedItem());
}

auto CHexDlgTemplMgr::GUIGetListCurrTemplateFilePath()const->wchar_t* {
	const auto iIndex = m_WndCmbTempl.GetCurSel();
	if (iIndex == CB_ERR) {
		return { };
	}

	return reinterpret_cast<wchar_t*>(m_WndCmbTempl.GetItemData(iIndex));
}

void CHexDlgTemplMgr::GUIOnTemplateApplyDisapply(int iTemplateID, bool fApply)
{
	if (!m_Wnd.IsWindow()) //Only if dialog window is created and alive we proceed with its members.
		return;

	const auto pTemplate = TMPLGetTemplateByID(iTemplateID);
	if (pTemplate == nullptr)
		return;

	if (fApply) {
		//Tree root node.
		TVINSERTSTRUCTW tvi { .hParent { TVI_ROOT }, .itemex { .mask { TVIF_CHILDREN | TVIF_TEXT | TVIF_PARAM },
			.pszText { LPSTR_TEXTCALLBACK }, .cChildren { static_cast<int>(pTemplate->vecFields.size()) },
			.lParam { iTemplateID } } }; //Tree root node has iTemplateID in lParam.
		const auto hTreeRootNode = m_WndTree.InsertItem(&tvi);

		const auto lmbFill = [&](HTREEITEM hTreeRoot, const VecHexTemplFields& vecFields)->void {
			const auto _lmbFill = [&](const auto& lmbSelf, HTREEITEM hTreeRoot, const VecHexTemplFields& vecFields)->void {
				for (const auto& pField : vecFields) {
					tvi.hParent = hTreeRoot;
					tvi.itemex.cChildren = static_cast<int>(pField->vecNested.size());
					tvi.itemex.lParam = reinterpret_cast<LPARAM>(pField.get()); //Tree child nodes have PCHEXTEMPLFIELD.
					const auto hCurrentRoot = m_WndTree.InsertItem(&tvi);
					if (tvi.itemex.cChildren > 0) {
						lmbSelf(lmbSelf, hCurrentRoot, pField->vecNested);
					}
				}
				};
			_lmbFill(_lmbFill, hTreeRoot, vecFields);
			};
		lmbFill(hTreeRootNode, pTemplate->vecFields);
	}
	else {
		auto hItem = m_WndTree.GetRootItem();
		while (hItem != nullptr) {
			if (const auto iID = GUIGetTemplateIDFromTree(hItem); iID == iTemplateID) {
				if (iID == GUIGetTreeSelectedTemplateID()) {
					m_ListEx.SetItemCountEx(0);
					m_ListEx.RedrawWindow();
					m_pVecFieldsCurr = nullptr;
					m_hTreeCurrParent = nullptr;
				}
				m_WndTree.DeleteItem(hItem);
				GUIUpdateStaticText();
				break;
			}

			hItem = m_WndTree.GetNextItem(hItem, TVGN_NEXT); //Get next Root sibling item.
		}
	}
}

void CHexDlgTemplMgr::GUIOnTemplateAddRemove(const wchar_t* pwszFilePath, bool fAdd) {
	if (!m_Wnd.IsWindow()) //Only if dialog window is created and alive we proceed with its members.
		return;

	if (fAdd) {
		const auto wstrTemplateName = JSONGetTemplateNameProperty(pwszFilePath);
		if (wstrTemplateName.empty()) {
			return; //No "TemplateName" string property, or incorrect file.
		}

		for (auto iIndex = 0; iIndex < m_WndCmbTempl.GetCount(); ++iIndex) { //Check if such file path already exists.
			if (const std::wstring_view wsv = reinterpret_cast<wchar_t*>(m_WndCmbTempl.GetItemData(iIndex));
				wsv == pwszFilePath) {
				return; //Already exists in the list.
			}
		}

		const auto iIndex = m_WndCmbTempl.AddString(wstrTemplateName.data());
		m_WndCmbTempl.SetItemData(iIndex, reinterpret_cast<DWORD_PTR>(pwszFilePath));
		m_WndCmbTempl.SetCurSel(iIndex);
		SetDlgButtonsState();
	}
	else {
		//Remove tree nodes with such pwszFilePath.
		std::vector<HTREEITEM> vecToRemove;
		auto hItem = m_WndTree.GetRootItem();
		while (hItem != nullptr) {
			const auto pTemplate = TMPLGetTemplateByID(GUIGetTemplateIDFromTree(hItem));
			if (pTemplate->wstrFilePath == pwszFilePath) {
				vecToRemove.emplace_back(hItem);
			}
			hItem = m_WndTree.GetNextItem(hItem, TVGN_NEXT); //Get next Root sibling item.
		}
		for (const auto item : vecToRemove) {
			m_WndTree.DeleteItem(item);
		}

		//Remove combo-box item with such pwszFilePath.
		for (auto iIndex = 0; iIndex < m_WndCmbTempl.GetCount(); ++iIndex) { //Remove Template name from ComboBox.
			if (const std::wstring_view wsv = reinterpret_cast<wchar_t*>(m_WndCmbTempl.GetItemData(iIndex)); wsv == pwszFilePath) {
				m_WndCmbTempl.DeleteString(iIndex);
				m_WndCmbTempl.SetCurSel(0);
				break;
			}
		}

		m_ListEx.SetItemCountEx(0);
		m_pVecFieldsCurr = nullptr;
		m_hTreeCurrParent = nullptr;
		m_ListEx.RedrawWindow();
		GUIUpdateStaticText();
		SetDlgButtonsState();
		RedrawHexCtrl();
	}
}

auto CHexDlgTemplMgr::GUITreeItemFromListItem(int iListItem)const->HTREEITEM {
	auto hChildItem = m_WndTree.GetNextItem(m_hTreeCurrParent, TVGN_CHILD);
	for (auto itListItems = 0; itListItems < iListItem; ++itListItems) {
		hChildItem = m_WndTree.GetNextSiblingItem(hChildItem);
	}

	return hChildItem;
}

void CHexDlgTemplMgr::GUIUpdateDateTimeFormat() {
	const auto [dwFormat, wchSepar] = GetHexCtrl()->GetDateInfo();
	m_dwDateFormat = dwFormat;
	m_wchDateSepar = wchSepar;
	m_ListEx.RedrawWindow();
}

void CHexDlgTemplMgr::GUIUpdateEditBoxOffsetToCurrHexCaret() {
	if (const auto pHex = GetHexCtrl(); pHex != nullptr && pHex->IsCreated() && pHex->IsDataSet()) {
		const auto wstr = std::format(L"0x{:X}", GetHexCtrl()->GetCaretPos());
		m_WndEditOffset.SetWndText(wstr);
	}
}

void CHexDlgTemplMgr::GUIUpdateStaticText() {
	std::wstring wstrOffset;
	std::wstring wstrSize;
	const auto pTemplate = GUIGetTreeSelectedTemplate();

	if (pTemplate != nullptr && GetHexCtrl()->IsDataSet()) { //If pTemplate == nullptr set empty text.
		const auto ullOffset = GetHexCtrl()->GetOffset(pTemplate->u64Offset, true); //Show virtual offset.
		wstrOffset = std::vformat(IsShowAsHex() ? L"0x{:X}" : L"{}", std::make_wformat_args(ullOffset));
		wstrSize = std::vformat(IsShowAsHex() ? L"0x{:X}" : L"{}", std::make_wformat_args(pTemplate->iSizeTotal));
	}

	m_WndStatOffset.SetWndText(wstrOffset);
	m_WndStatSize.SetWndText(wstrSize);
}

bool CHexDlgTemplMgr::IsHighlight()const {
	return m_WndBtnHighlight.IsChecked();
}

bool CHexDlgTemplMgr::IsMinimized()const {
	return m_WndBtnMin.IsChecked();
}

bool CHexDlgTemplMgr::IsNoEsc()const {
	return m_u64Flags & HEXCTRL_FLAG_DLG_NOESC;
}

bool CHexDlgTemplMgr::IsShowAsHex()const {
	return m_WndBtnHex.IsChecked();
}

bool CHexDlgTemplMgr::IsSwapEndian()const {
	return m_WndBtnEndian.IsChecked();
}

void CHexDlgTemplMgr::OnBnAddTemplate()
{
	IFileOpenDialog *pFOD;
	if (::CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFOD)) != S_OK) {
		ut::DBG_REPORT(L"CoCreateInstance failed.");
		return;
	}

	DWORD dwFlags;
	pFOD->GetOptions(&dwFlags);
	pFOD->SetOptions(dwFlags | FOS_FORCEFILESYSTEM | FOS_ALLOWMULTISELECT | FOS_DONTADDTORECENT
		| FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST);
	COMDLG_FILTERSPEC arrFilter[] { { .pszName { L"Template files (*.json)" }, .pszSpec { L"*.json" } },
		{ .pszName { L"All files (*.*)" }, .pszSpec { L"*.*" } } };
	pFOD->SetFileTypes(2, arrFilter);

	if (pFOD->Show(m_Wnd) != S_OK) { //Cancel was pressed.
		pFOD->Release();
		return;
	}

	IShellItemArray* pShellItemArray;
	pFOD->GetResults(&pShellItemArray);
	if (pShellItemArray == nullptr) {
		pFOD->Release();
		ut::DBG_REPORT(L"ShellItemArray == nullptr");
		return;
	}

	DWORD dwCount { };
	pShellItemArray->GetCount(&dwCount);
	for (auto iFileIdx { 0U }; iFileIdx < dwCount; ++iFileIdx) {
		IShellItem* pShellItem;
		pShellItemArray->GetItemAt(iFileIdx, &pShellItem);
		wchar_t* pwszPath;
		pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &pwszPath);
		if (!AddTemplateFile(pwszPath)) {
			std::wstring wstrErr = L"Error adding template:\n";
			wstrErr += pwszPath;
			std::wstring_view wsvFileName = pwszPath;
			wsvFileName = wsvFileName.substr(wsvFileName.find_last_of('\\') + 1);
			::MessageBoxW(m_Wnd, wstrErr.data(), wsvFileName.data(), MB_ICONERROR);
		}

		::CoTaskMemFree(pwszPath);
		pShellItem->Release();
	}

	pShellItemArray->Release();
	pFOD->Release();
}

void CHexDlgTemplMgr::OnBnRemoveTemplate() {
	RemoveTemplateFile(GUIGetListCurrTemplateFilePath());
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

	ApplyTemplate(GUIGetListCurrTemplateFilePath(), GetHexCtrl()->GetOffset(*optOffset, false));
}

void CHexDlgTemplMgr::OnCancel() {
	if (IsNoEsc()) //Not closing Dialog on Escape key.
		return;

	WMClose();
}

void CHexDlgTemplMgr::OnCheckHex() {
	GUIUpdateStaticText();
	m_ListEx.RedrawWindow();
}

void CHexDlgTemplMgr::OnCheckSwapEndian() {
	m_ListEx.RedrawWindow();
}

void CHexDlgTemplMgr::OnCheckMin()
{
	static constexpr int arrIDsToHide[] { IDC_HEXCTRL_TEMPLMGR_STAT_AVAIL, IDC_HEXCTRL_TEMPLMGR_COMBO_TEMPLATES,
		IDC_HEXCTRL_TEMPLMGR_BTN_ADD, IDC_HEXCTRL_TEMPLMGR_BTN_REMOVE,
		IDC_HEXCTRL_TEMPLMGR_STAT_APPLY, IDC_HEXCTRL_TEMPLMGR_EDIT_OFFSET, IDC_HEXCTRL_TEMPLMGR_BTN_APPLY,
		IDC_HEXCTRL_TEMPLMGR_CHK_TT, IDC_HEXCTRL_TEMPLMGR_CHK_HGL, IDC_HEXCTRL_TEMPLMGR_CHK_HEX,
		IDC_HEXCTRL_TEMPLMGR_CHK_SWAP, IDC_HEXCTRL_TEMPLMGR_GRB_TOP };
	static constexpr int arrIDsToMove[] { IDC_HEXCTRL_TEMPLMGR_STAT_OFFSETTXT, IDC_HEXCTRL_TEMPLMGR_STAT_OFFSETNUM,
		IDC_HEXCTRL_TEMPLMGR_STAT_SIZETXT, IDC_HEXCTRL_TEMPLMGR_STAT_SIZENUM };
	static constexpr int arrIDsToResize[] { IDC_HEXCTRL_TEMPLMGR_TREE, IDC_HEXCTRL_TEMPLMGR_LIST };
	const auto fMinimize = IsMinimized();

	//Top Group Box rect.
	const auto iHeightGRB = m_Wnd.GetDlgItem(IDC_HEXCTRL_TEMPLMGR_GRB_TOP).GetClientRect().Height();
	auto hdwp = ::BeginDeferWindowPos(static_cast<int>(std::size(arrIDsToMove) + std::size(arrIDsToResize) + std::size(arrIDsToHide)));
	for (const auto id : arrIDsToHide) { //Hiding.
		hdwp = ::DeferWindowPos(hdwp, m_Wnd.GetDlgItem(id), nullptr, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE
			| (fMinimize ? SWP_HIDEWINDOW : SWP_SHOWWINDOW));
	}

	for (const auto id : arrIDsToMove) { //Moving.
		const auto hWnd = m_Wnd.GetDlgItem(id);
		auto rcWnd = hWnd.GetWindowRect();
		m_Wnd.ScreenToClient(rcWnd);
		const auto iNewPosY = fMinimize ? rcWnd.top - iHeightGRB : rcWnd.top + iHeightGRB;
		hdwp = ::DeferWindowPos(hdwp, hWnd, nullptr, rcWnd.left, iNewPosY, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
	}

	for (const auto id : arrIDsToResize) { //Resizing.
		const auto hWnd = m_Wnd.GetDlgItem(id);
		auto rcWnd = hWnd.GetWindowRect();
		rcWnd.top += fMinimize ? -iHeightGRB : iHeightGRB;
		m_Wnd.ScreenToClient(rcWnd);
		hdwp = ::DeferWindowPos(hdwp, hWnd, nullptr, rcWnd.left, rcWnd.top, rcWnd.Width(),
			rcWnd.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
	}

	m_DynLayout.Enable(false); //Otherwise DynamicLayout won't know that all dynamic windows have changed.
	::EndDeferWindowPos(hdwp);
	m_DynLayout.Enable(true);

	m_WndBtnMin.SetBitmap(fMinimize ? m_hBmpMax : m_hBmpMin); //Set arrow bitmap to the min-max checkbox.
}

void CHexDlgTemplMgr::OnOK()
{
	const auto wndFocus = GDIUT::CWnd::GetFocus();
	//When Enter is pressed anywhere in the dialog, and focus is on the m_ListEx,
	//we simulate pressing Enter in the list by sending WM_KEYDOWN/VK_RETURN to it.
	if (const auto hWndList = m_ListEx.GetHWND(); wndFocus == hWndList) {
		::SendMessageW(hWndList, WM_KEYDOWN, VK_RETURN, 0);
	}
	else if (wndFocus == m_WndEditOffset) { //Focus is on the "Offset" edit-box.
		OnBnApply();
	}
}

void CHexDlgTemplMgr::RedrawHexCtrl() {
	if (m_pHexCtrl != nullptr && m_pHexCtrl->IsDataSet()) {
		m_pHexCtrl->Redraw();
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
		else {
			return false;
		}

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
		if (lTicks.QuadPart >= (std::numeric_limits<long>::max)())
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
	const auto fHasTempl = TMPLHasTemplateFiles();
	if (const auto btnApply = m_Wnd.GetDlgItem(IDC_HEXCTRL_TEMPLMGR_BTN_APPLY); !btnApply.IsNull()) {
		btnApply.EnableWindow(fHasTempl);
	}
	if (const auto btnRemove = m_Wnd.GetDlgItem(IDC_HEXCTRL_TEMPLMGR_BTN_REMOVE); !btnRemove.IsNull()) {
		btnRemove.EnableWindow(fHasTempl);
	}
	m_WndEditOffset.EnableWindow(fHasTempl);
}

void CHexDlgTemplMgr::SetHexSelByField(PCHEXTEMPLFIELD pField)
{
	const auto pTemplate = GUIGetTreeSelectedTemplate();
	if (!IsHighlight() || !m_pHexCtrl->IsDataSet() || pField == nullptr || pTemplate == nullptr)
		return;

	const auto ullOffset = pTemplate->u64Offset + pField->iOffset;
	const auto ullSize = static_cast<ULONGLONG>(pField->iSize);
	const HEXSPAN hs { .ullOffset { ullOffset }, .ullSize { ullSize } };
	m_pHexCtrl->SetSelection({ &hs, 1 });
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

	const auto u64 = std::bit_cast<std::uint64_t>(stFTime);
	const auto wstrTime = ut::FileTimeToString(stFTime, m_dwDateFormat, m_wchDateSepar);
	*std::vformat_to(pwsz, IsShowAsHex() ? L"0x{0:016X}" : L"{}", std::make_wformat_args(u64, wstrTime)) = L'\0';
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

	*std::format_to(pwsz, L"{{{:08X}-{:04X}-{:04X}-{:02X}{:02X}-{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}}}",
		stGUID.Data1, stGUID.Data2, stGUID.Data3, stGUID.Data4[0], stGUID.Data4[1], stGUID.Data4[2],
		stGUID.Data4[3], stGUID.Data4[4], stGUID.Data4[5], stGUID.Data4[6], stGUID.Data4[7]) = L'\0';
}

auto CHexDlgTemplMgr::TMPLAddTemplateFile(const wchar_t* pwszFilePath)->const wchar_t* {
	if (const auto it = std::find_if(m_vecTemplateFiles.begin(), m_vecTemplateFiles.end(),
		[pwszFilePath](const std::unique_ptr<std::wstring>& uptr) {
			return *uptr == pwszFilePath; });
			it != m_vecTemplateFiles.end()) { //Already exists.
		return it->get()->data();
	}

	const auto wstrTemplateName = JSONGetTemplateNameProperty(pwszFilePath);
	if (wstrTemplateName.empty()) {
		return nullptr; //No "TemplateName" property, or wrong/incorrect file.
	}

	return m_vecTemplateFiles.emplace_back(std::make_unique<std::wstring>(pwszFilePath))->data();
}

auto CHexDlgTemplMgr::TMPLGetTemplateByOffset(std::uint64_t u64Offset)const->PCHEXTEMPLATE {
	const auto rit = std::find_if(m_vecTemplates.rbegin(), m_vecTemplates.rend(),
		[u64Offset](const std::unique_ptr<HEXTEMPLATE>& uptr) {
			return u64Offset >= uptr->u64Offset && u64Offset < (uptr->u64Offset + uptr->iSizeTotal); });
	return rit != m_vecTemplates.rend() ? rit->get() : nullptr;
}

auto CHexDlgTemplMgr::TMPLGetIDForNewTemplate()const->int {
	auto iTemplateID = 1; //TemplateID starts at 1.
	if (const auto it = std::max_element(m_vecTemplates.begin(), m_vecTemplates.end(),
		[](const std::unique_ptr<HEXTEMPLATE>& p1, const std::unique_ptr<HEXTEMPLATE>& p2) {
			return p1->iTemplateID < p2->iTemplateID; }); it != m_vecTemplates.end()) {
		iTemplateID = it->get()->iTemplateID + 1; //Increasing next Template's ID by 1.
	}

	return iTemplateID;
}

auto CHexDlgTemplMgr::TMPLGetTemplateByID(int iTemplateID)const->PCHEXTEMPLATE {
	const auto it = std::find_if(m_vecTemplates.begin(), m_vecTemplates.end(),
		[iTemplateID](const std::unique_ptr<HEXTEMPLATE>& uptr) { return uptr->iTemplateID == iTemplateID; });
	return it != m_vecTemplates.end() ? it->get() : nullptr;
}

auto CHexDlgTemplMgr::TMPLGetTemplateByFilePath(const wchar_t* pwszFilePath)const->PCHEXTEMPLATE {
	const auto it = std::find_if(m_vecTemplates.begin(), m_vecTemplates.end(),
		[pwszFilePath](const std::unique_ptr<HEXTEMPLATE>& uptr) { return uptr->wstrFilePath == pwszFilePath; });
	return it != m_vecTemplates.end() ? it->get() : nullptr;
}

bool CHexDlgTemplMgr::TMPLHasTemplateFiles()const {
	return !m_vecTemplateFiles.empty();
}

void CHexDlgTemplMgr::TMPLRandomizeTemplateColors(int iTemplateID) {
	const auto pTemplate = TMPLGetTemplateByID(iTemplateID);
	if (pTemplate == nullptr)
		return;

	std::mt19937 gen(std::random_device { }());
	std::uniform_int_distribution<unsigned int> distrib(50, 230);
	const auto lmbRndColors = [&distrib, &gen](const VecHexTemplFields& vecFields) {
		const auto _lmbCount = [&distrib, &gen](const auto& lmbSelf, const VecHexTemplFields& vecFields)->void {
			for (const auto& pField : vecFields) {
				if (pField->vecNested.empty()) {
					pField->stClr.clrBk = RGB(distrib(gen), distrib(gen), distrib(gen));
				}
				else { lmbSelf(lmbSelf, pField->vecNested); }
			}
			};
		return _lmbCount(_lmbCount, vecFields);
		};
	lmbRndColors(pTemplate->vecFields);
}

void CHexDlgTemplMgr::TMPLRemoveAppliedByID(int iTemplateID) {
	std::erase_if(m_vecTemplates, [iTemplateID](const std::unique_ptr<HEXTEMPLATE>& uptr) {
		return uptr->iTemplateID == iTemplateID; });
}

void CHexDlgTemplMgr::TMPLRemoveAppliedByFilePath(const wchar_t* pwszFilePath) {
	std::erase_if(m_vecTemplates, [pwszFilePath](const std::unique_ptr<HEXTEMPLATE>& uptr) {
		return uptr->wstrFilePath == pwszFilePath; });
}

void CHexDlgTemplMgr::TMPLRemoveTemplateFile(const wchar_t* pwszFilePath) {
	std::erase_if(m_vecTemplateFiles, [pwszFilePath](const std::unique_ptr<std::wstring>& uptr) {
		return *uptr == pwszFilePath; }); //Remove template file name.
}

auto CHexDlgTemplMgr::WMActivate(const MSG& msg)->INT_PTR
{
	if (const auto pHex = GetHexCtrl();
		pHex != nullptr && pHex->IsCreated() && pHex->IsDataSet() && LOWORD(msg.wParam) == WA_ACTIVE) {
		GUIUpdateDateTimeFormat();
	}

	return 0;
}

auto CHexDlgTemplMgr::WMClose()->INT_PTR
{
	ShowWindow(SW_HIDE);
	return TRUE;
}

auto CHexDlgTemplMgr::WMCommand(const MSG& msg)->INT_PTR
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
		case IDC_HEXCTRL_TEMPLMGR_BTN_ADD: OnBnAddTemplate(); break;
		case IDC_HEXCTRL_TEMPLMGR_BTN_REMOVE: OnBnRemoveTemplate(); break;
		case IDC_HEXCTRL_TEMPLMGR_CHK_HEX: OnCheckHex(); break;
		case IDC_HEXCTRL_TEMPLMGR_CHK_SWAP: OnCheckSwapEndian(); break;
		case IDC_HEXCTRL_TEMPLMGR_CHK_MIN: OnCheckMin(); break;
		default: return FALSE;
		}
	}
	else { //Notifications from menus.
		using enum EMenuID;
		switch (static_cast<EMenuID>(uCtrlID)) {
		case IDM_TREE_RNDCOLORS:
			TMPLRandomizeTemplateColors(GUIGetTreeSelectedTemplateID());
			m_ListEx.RedrawWindow();
			RedrawHexCtrl();
			break;
		case IDM_TREE_DISAPPLY:
			DisapplyByID(GUIGetTreeSelectedTemplateID());
			break;
		case IDM_TREE_DISAPPLYALL:
			DisapplyAll();
			break;
		case IDM_LIST_HDR_TYPE:
		case IDM_LIST_HDR_NAME:
		case IDM_LIST_HDR_OFFSET:
		case IDM_LIST_HDR_SIZE:
		case IDM_LIST_HDR_DATA:
		case IDM_LIST_HDR_ENDIANNESS:
		case IDM_LIST_HDR_DESCRIPTION:
		case IDM_LIST_HDR_COLORS:
		{
			const auto fChecked = m_MenuListHdr.IsItemChecked(uCtrlID);
			m_ListEx.HideColumn(uCtrlID - static_cast<int>(IDM_LIST_HDR_TYPE), fChecked);
			m_MenuListHdr.SetItemCheck(uCtrlID, !fChecked);
		}
		break;
		default: return FALSE;
		}
	}

	return TRUE;
}

auto CHexDlgTemplMgr::WMCtlColorStatic(const MSG& msg)->INT_PTR
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

auto CHexDlgTemplMgr::WMDestroy()->INT_PTR
{
	m_MenuTree.DestroyMenu();
	m_MenuListHdr.DestroyMenu();
	m_pVecFieldsCurr = nullptr;
	m_hTreeCurrParent = nullptr;
	m_pHexCtrl = nullptr;
	m_u64Flags = { };
	m_DynLayout.Uninitialize();
	::DeleteObject(m_hBmpMin);
	::DeleteObject(m_hBmpMax);

	return TRUE;
}

auto CHexDlgTemplMgr::WMDPIChanged([[maybe_unused]] const MSG& msg)->INT_PTR
{
	CreateArrows();
	m_DynLayout.Enable(true);

	return 0;
}

auto CHexDlgTemplMgr::WMDrawItem(const MSG& msg)->INT_PTR
{
	const auto pDIS = reinterpret_cast<LPDRAWITEMSTRUCT>(msg.lParam);
	if (pDIS->CtlID == static_cast<UINT>(IDC_HEXCTRL_TEMPLMGR_LIST)) {
		m_ListEx.DrawItem(pDIS);
	}

	return TRUE;
}

auto CHexDlgTemplMgr::WMGetDPIScaledSize([[maybe_unused]] const MSG& msg)->INT_PTR
{
	//This message is sent to top-level windows with a DPI_AWARENESS_CONTEXT
	//of Per Monitor v2 before a WM_DPICHANGED message is sent.
	//We use it to temporarily disable all dynamic layout resizes,
	//to re-enable it later in the WM_DPICHANGED handler.

	m_DynLayout.Enable(false);
	return 0;
}

auto CHexDlgTemplMgr::WMInitDialog(const MSG& msg)->INT_PTR
{
	m_Wnd.Attach(msg.hwnd);
	m_WndStatOffset.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_TEMPLMGR_STAT_OFFSETNUM));
	m_WndStatSize.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_TEMPLMGR_STAT_SIZENUM));
	m_WndEditOffset.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_TEMPLMGR_EDIT_OFFSET));
	m_WndBtnTT.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_TEMPLMGR_CHK_TT));
	m_WndBtnMin.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_TEMPLMGR_CHK_MIN));
	m_WndBtnHighlight.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_TEMPLMGR_CHK_HGL));
	m_WndBtnHex.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_TEMPLMGR_CHK_HEX));
	m_WndBtnEndian.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_TEMPLMGR_CHK_SWAP));
	m_WndCmbTempl.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_TEMPLMGR_COMBO_TEMPLATES));
	m_WndTree.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_TEMPLMGR_TREE));

	m_ListEx.Create({ .hWndParent { m_Wnd }, .uID { IDC_HEXCTRL_TEMPLMGR_LIST }, .flSizeFontList { 10.F },
		.flSizeFontHdr { 10.F }, .fDialogCtrl { true }, .fLinks { true } });
	m_ListEx.SetExtendedStyle(LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT);
	const auto flDPIScale = GDIUT::GetDPIScaleForHWND(m_ListEx);
	m_ListEx.InsertColumn(COL_TYPE, L"Type", LVCFMT_LEFT, std::lround(85 * flDPIScale));
	m_ListEx.InsertColumn(COL_NAME, L"Name", LVCFMT_LEFT, std::lround(200 * flDPIScale));
	m_ListEx.InsertColumn(COL_OFFSET, L"Offset", LVCFMT_LEFT, std::lround(50 * flDPIScale));
	m_ListEx.InsertColumn(COL_SIZE, L"Size", LVCFMT_LEFT, std::lround(50 * flDPIScale));
	m_ListEx.InsertColumn(COL_DATA, L"Data", LVCFMT_LEFT, std::lround(120 * flDPIScale), -1, LVCFMT_LEFT, true);
	m_ListEx.InsertColumn(COL_ENDIAN, L"Endianness", LVCFMT_CENTER, std::lround(75 * flDPIScale), -1, LVCFMT_CENTER);
	m_ListEx.InsertColumn(COL_DESCR, L"Description", LVCFMT_LEFT, std::lround(100 * flDPIScale), -1, LVCFMT_LEFT, true);
	m_ListEx.InsertColumn(COL_COLORS, L"Colors", LVCFMT_LEFT, std::lround(57 * flDPIScale));

	using enum EMenuID;
	m_MenuListHdr.CreatePopupMenu();
	m_MenuListHdr.AppendString(static_cast<int>(IDM_LIST_HDR_TYPE), L"Type");
	m_MenuListHdr.SetItemCheck(static_cast<int>(IDM_LIST_HDR_TYPE), true);
	m_MenuListHdr.AppendString(static_cast<int>(IDM_LIST_HDR_NAME), L"Name");
	m_MenuListHdr.SetItemCheck(static_cast<int>(IDM_LIST_HDR_NAME), true);
	m_MenuListHdr.AppendString(static_cast<int>(IDM_LIST_HDR_OFFSET), L"Offset");
	m_MenuListHdr.SetItemCheck(static_cast<int>(IDM_LIST_HDR_OFFSET), true);
	m_MenuListHdr.AppendString(static_cast<int>(IDM_LIST_HDR_SIZE), L"Size");
	m_MenuListHdr.SetItemCheck(static_cast<int>(IDM_LIST_HDR_SIZE), true);
	m_MenuListHdr.AppendString(static_cast<int>(IDM_LIST_HDR_DATA), L"Data");
	m_MenuListHdr.SetItemCheck(static_cast<int>(IDM_LIST_HDR_DATA), true);
	m_MenuListHdr.AppendString(static_cast<int>(IDM_LIST_HDR_ENDIANNESS), L"Endianness");
	m_MenuListHdr.SetItemCheck(static_cast<int>(IDM_LIST_HDR_ENDIANNESS), true);
	m_MenuListHdr.AppendString(static_cast<int>(IDM_LIST_HDR_DESCRIPTION), L"Description");
	m_MenuListHdr.SetItemCheck(static_cast<int>(IDM_LIST_HDR_DESCRIPTION), true);
	m_MenuListHdr.AppendString(static_cast<int>(IDM_LIST_HDR_COLORS), L"Colors");
	m_MenuListHdr.SetItemCheck(static_cast<int>(IDM_LIST_HDR_COLORS), true);

	m_MenuTree.CreatePopupMenu();
	m_MenuTree.AppendString(static_cast<UINT_PTR>(IDM_TREE_RNDCOLORS), L"Randomize colors");
	m_MenuTree.AppendString(static_cast<UINT_PTR>(IDM_TREE_DISAPPLY), L"Disapply template");
	m_MenuTree.AppendSepar();
	m_MenuTree.AppendString(static_cast<UINT_PTR>(IDM_TREE_DISAPPLYALL), L"Disapply all");

	m_WndEditOffset.SetWndText(L"0x0");
	m_WndBtnTT.SetCheck(true);
	m_WndBtnHighlight.SetCheck(IsHighlight());
	m_WndBtnHex.SetCheck(IsShowAsHex());

	m_SplitVert.Initialize(m_Wnd, m_ListEx, GDIUT::CSplitter::EAnchorSide::SIDE_LEFT);
	m_SplitVert.AddItem(m_WndTree, true);
	m_SplitVert.SetEdges(100, m_Wnd.GetClientRect().Width() - 10);

	m_DynLayout.Initialize(m_Wnd);
	m_DynLayout.AddItem(IDC_HEXCTRL_TEMPLMGR_LIST, GDIUT::CDynLayout::MoveNone(), GDIUT::CDynLayout::SizeHorzAndVert(100, 100));
	m_DynLayout.AddItem(IDC_HEXCTRL_TEMPLMGR_TREE, GDIUT::CDynLayout::MoveNone(), GDIUT::CDynLayout::SizeVert(100));
	m_DynLayout.AddItem(IDC_HEXCTRL_TEMPLMGR_GRB_TOP, GDIUT::CDynLayout::MoveNone(), GDIUT::CDynLayout::SizeHorz(100));
	m_DynLayout.AddItem(IDC_HEXCTRL_TEMPLMGR_CHK_MIN, GDIUT::CDynLayout::MoveHorz(100), GDIUT::CDynLayout::SizeNone());
	m_DynLayout.Enable(true);

	::SetWindowSubclass(m_WndTree, TreeSubclassProc, reinterpret_cast<UINT_PTR>(this), 0);
	SetDlgButtonsState();

	for (const auto& uptr : m_vecTemplateFiles) {
		GUIOnTemplateAddRemove(uptr.get()->data(), true);
	}

	CreateArrows();
	GUIUpdateDateTimeFormat();
	GUIUpdateEditBoxOffsetToCurrHexCaret();

	return TRUE;
}

auto CHexDlgTemplMgr::WMLButtonDown([[maybe_unused]] const MSG& msg)->INT_PTR {
	if (m_SplitVert.IsSplitting()) {
		m_DynLayout.Enable(false);
	}

	return TRUE;
}

auto CHexDlgTemplMgr::WMLButtonUp([[maybe_unused]] const MSG& msg)->INT_PTR {
	m_DynLayout.Enable(true);

	return TRUE;
}

auto CHexDlgTemplMgr::WMMeasureItem(const MSG& msg)->INT_PTR
{
	const auto pMIS = reinterpret_cast<LPMEASUREITEMSTRUCT>(msg.lParam);
	if (pMIS->CtlID == static_cast<UINT>(IDC_HEXCTRL_TEMPLMGR_LIST)) {
		m_ListEx.MeasureItem(pMIS);
	}

	return TRUE;
}

auto CHexDlgTemplMgr::WMMouseActivate([[maybe_unused]] const MSG& msg)->INT_PTR
{
	if (const auto pHex = GetHexCtrl(); pHex != nullptr && pHex->IsCreated() && pHex->IsDataSet()) {
		GUIUpdateDateTimeFormat();
		GUIUpdateEditBoxOffsetToCurrHexCaret();
	}

	return MA_ACTIVATE;
}

auto CHexDlgTemplMgr::WMNotify(const MSG& msg)->INT_PTR
{
	const auto pNMHDR = reinterpret_cast<NMHDR*>(msg.lParam);
	switch (pNMHDR->idFrom) {
	case IDC_HEXCTRL_TEMPLMGR_LIST:
		switch (pNMHDR->code) {
		case LVN_GETDISPINFOW: WMNotifyListGetDispInfo(pNMHDR); break;
		case LVN_ITEMCHANGED: WMNotifyListItemChanged(pNMHDR); break;
		case NM_DBLCLK: WMNotifyListDblClick(pNMHDR); break;
		case NM_RCLICK: WMNotifyListRClick(pNMHDR); break;
		case NM_RETURN: WMNotifyListEnterPressed(pNMHDR); break;
		case LISTEX::LISTEX_MSG_EDITBEGIN: WMNotifyListEditBegin(pNMHDR); break;
		case LISTEX::LISTEX_MSG_GETCOLOR: WMNotifyListGetColor(pNMHDR); break;
		case LISTEX::LISTEX_MSG_HDRRBTNUP: WMNotifyListHdrRClick(pNMHDR); break;
		case LISTEX::LISTEX_MSG_LINKCLICK: WMNotifyListLinkClick(pNMHDR); break;
		case LISTEX::LISTEX_MSG_SETDATA: WMNotifyListSetData(pNMHDR); break;
		default: break;
		}
		break;
	case IDC_HEXCTRL_TEMPLMGR_TREE:
		switch (pNMHDR->code) {
		case NM_CLICK: WMNotifyTreeLClick(pNMHDR); break;
		case NM_RCLICK: WMNotifyTreeRClick(pNMHDR); break;
		case TVN_GETDISPINFOW: WMNotifyTreeGetDispInfo(pNMHDR); break;
		case TVN_SELCHANGEDW: WMNotifyTreeItemChanged(pNMHDR); break;
		default: break;
		}
		break;
	default: break;
	}

	return TRUE;
}

void CHexDlgTemplMgr::WMNotifyListDblClick(NMHDR* pNMHDR)
{
	const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	const auto iItem = pNMI->iItem;
	if (iItem < 0)
		return;

	const auto& vec = *m_pVecFieldsCurr;
	if (vec[iItem]->vecNested.empty())
		return;

	m_fListGuardEvent = true; //To prevent nasty OnListItemChanged to fire after this method ends.
	m_pVecFieldsCurr = &vec[iItem]->vecNested;

	const auto hItem = GUITreeItemFromListItem(iItem);
	m_hTreeCurrParent = hItem;
	m_WndTree.Expand(hItem, TVE_EXPAND);

	m_ListEx.SetItemState(-1, 0, LVIS_SELECTED | LVIS_FOCUSED); //Deselect all items.
	m_ListEx.SetItemCountEx(static_cast<int>(m_pVecFieldsCurr->size()));
	m_ListEx.RedrawWindow();
	m_fListGuardEvent = false;
}

void CHexDlgTemplMgr::WMNotifyListEditBegin(NMHDR* pNMHDR)
{
	const auto pLDI = reinterpret_cast<LISTEX::PLISTEXDATAINFO>(pNMHDR);
	if (pLDI->iSubItem == COL_DESCR) { //Allow editing a description at any field.
		pLDI->fAllowEdit = true;
		return;
	}

	const auto& pField = (*m_pVecFieldsCurr)[pLDI->iItem];
	if (!pField->vecNested.empty() || (pField->eType == EHexTemplFieldType::custom_size
		&& pField->iSize != 1 && pField->iSize != 2 && pField->iSize != 4 && pField->iSize != 8)) {
		pLDI->fAllowEdit = false; //Do not show an edit-box if clicked on nested fields.
	}
}

void CHexDlgTemplMgr::WMNotifyListEnterPressed([[maybe_unused]] NMHDR* pNMHDR)
{
	const auto uSelected = m_ListEx.GetSelectedCount();
	if (uSelected != 1)
		return;

	//Simulate DblClick in List with Enter key.
	NMITEMACTIVATE nmii { .iItem = m_ListEx.GetSelectionMark() };
	WMNotifyListDblClick(&nmii.hdr);
}

void CHexDlgTemplMgr::WMNotifyListGetColor(NMHDR* pNMHDR)
{
	constexpr auto clrTextBluish { RGB(16, 42, 255) };  //Bluish text.
	constexpr auto clrTextGreenish { RGB(0, 110, 0) };  //Green text.
	constexpr auto clrBkGreyish { RGB(235, 235, 235) }; //Grayish bk.

	const auto pLCI = reinterpret_cast<LISTEX::PLISTEXCOLORINFO>(pNMHDR);
	const auto& pField = (*m_pVecFieldsCurr)[pLCI->iItem];
	const auto eType = pField->eType;
	using enum EHexTemplFieldType;

	pLCI->stClr.clrText = static_cast<COLORREF>(-1); //Default text color.

	//List items with nested structs colored separately with greyish bk.
	if (!pField->vecNested.empty() && pLCI->iSubItem != COL_COLORS) {
		pLCI->stClr.clrBk = clrBkGreyish;
		if (pLCI->iSubItem == COL_TYPE) {
			if (eType == type_custom) {
				if (pField->iCustomTypeID > 0) {
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

void CHexDlgTemplMgr::WMNotifyListGetDispInfo(NMHDR* pNMHDR)
{
	const auto pDispInfo = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
	const auto pItem = &pDispInfo->item;
	if ((pItem->mask & LVIF_TEXT) == 0)
		return;

	const auto& pField = (*m_pVecFieldsCurr)[pItem->iItem];
	const auto wsvFmt = IsShowAsHex() ? L"0x{:X}" : L"{}";
	const auto fShouldSwap = pField->fBigEndian == !IsSwapEndian();
	using enum EHexTemplFieldType;

	//EHexTemplFieldType converter to actual wstring for the list.
	static const std::unordered_map<EHexTemplFieldType, const wchar_t* const> umapETypeToWstr {
		{ custom_size, L"custom size" }, { type_custom, L"custom type" },
		{ type_bool, L"bool" }, { type_int8, L"int8" }, { type_uint8, L"uint8" },
		{ type_int16, L"int16" }, { type_uint16, L"uint16" }, { type_int32, L"int32" },
		{ type_uint32, L"uint32" }, { type_int64, L"int64" }, { type_uint64, L"uint64" },
		{ type_float, L"float" }, { type_double, L"double" }, { type_time32, L"time32_t" },
		{ type_time64, L"time64_t" }, { type_filetime, L"FILETIME" }, { type_systemtime, L"SYSTEMTIME" },
		{ type_guid, L"GUID" }
	};

	const auto pTemplateCurr = GUIGetTreeSelectedTemplate();
	switch (pItem->iSubItem) {
	case COL_TYPE:
		if (pField->eType == type_custom) {
			const auto& vecCT = pTemplateCurr->vecCustomType;
			if (const auto it = std::find_if(vecCT.begin(), vecCT.end(),
				[iCustomTypeID = pField->iCustomTypeID](const HEXTEMPLCT& ct) {
					return ct.iTypeID == iCustomTypeID; }); it != vecCT.end()) {
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
	{
		//Do not put a <link=...> tag to the array root entry, only to elements of the array.
		if (pField->pJump == nullptr || (pField->pJump != nullptr && !pField->vecNested.empty())) {
			const std::wstring_view wsv = pField->wstrName.size() < pItem->cchTextMax ? pField->wstrName :
				std::wstring_view { pField->wstrName.data(), static_cast<std::size_t>(pItem->cchTextMax - 1) };
			*std::format_to(pItem->pszText, L"{}", wsv) = L'\0';
		}
		else {
			const auto wstr = std::format(L"<link=\"\">{}</link>", pField->wstrName);
			const std::wstring_view wsv = wstr.size() < pItem->cchTextMax ? wstr :
				std::wstring_view { wstr.data(), static_cast<std::size_t>(pItem->cchTextMax - 1) };
			*std::format_to(pItem->pszText, L"{}", wsv) = L'\0';
		}
	}
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
			|| pTemplateCurr->u64Offset + pTemplateCurr->iSizeTotal > m_pHexCtrl->GetDataSize()) //Size overflow check.
			break;

		if (!pField->vecNested.empty()) {
			break; //Doing nothing (no data fetch) for nested structs.
		}

		const auto ullOffset = pTemplateCurr->u64Offset + pField->iOffset;
		const auto eType = pField->eType;
		switch (eType) {
		case custom_size: //If field of a custom size we cycling through the size field.
			switch (pField->iSize) {
			case 1:
			{
				const auto bData = ut::GetIHexTData<std::uint8_t>(*m_pHexCtrl, ullOffset);
				*std::vformat_to(pItem->pszText, wsvFmt, std::make_wformat_args(bData)) = L'\0';
			}
			break;
			case 2:
			{
				auto wData = ut::GetIHexTData<std::uint16_t>(*m_pHexCtrl, ullOffset);
				if (fShouldSwap) {
					wData = ut::ByteSwap(wData);
				}
				*std::vformat_to(pItem->pszText, wsvFmt, std::make_wformat_args(wData)) = L'\0';
			}
			break;
			case 4:
			{
				auto dwData = ut::GetIHexTData<std::uint32_t>(*m_pHexCtrl, ullOffset);
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
	{
		//List internal buffer overrun check.
		std::wstring_view wsv = pField->wstrDescr.size() < pItem->cchTextMax ? pField->wstrDescr :
			std::wstring_view { pField->wstrDescr.data(), static_cast<std::size_t>(pItem->cchTextMax - 1) };
		*std::format_to(pItem->pszText, L"{}", wsv) = L'\0';
	}
	break;
	case COL_COLORS:
		*std::format_to(pItem->pszText, L"#Text") = L'\0';
		break;
	default:
		break;
	}
}

void CHexDlgTemplMgr::WMNotifyListHdrRClick([[maybe_unused]] NMHDR* pNMHDR)
{
	POINT ptCur;
	::GetCursorPos(&ptCur);
	m_MenuListHdr.TrackPopupMenu(ptCur.x, ptCur.y, m_Wnd);
}

void CHexDlgTemplMgr::WMNotifyListItemChanged(NMHDR* pNMHDR)
{
	const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	const auto iItem = pNMI->iItem;
	if (iItem < 0 || m_fListGuardEvent)
		return;

	m_WndTree.SelectItem(GUITreeItemFromListItem(iItem));
}

void CHexDlgTemplMgr::WMNotifyListLinkClick(NMHDR* pNMHDR) {
	const auto* const pLLI = reinterpret_cast<LISTEX::PLISTEXLINKINFO>(pNMHDR);
	const auto& pField = (*m_pVecFieldsCurr)[pLLI->iItem];

	if (pField->pJump == nullptr)
		return;

	const auto& jump = pField->pJump;
	const auto u64FieldOffset = GUIGetTreeSelectedTemplate()->u64Offset + pField->iOffset;
	std::uint64_t u64FieldData { };

	using enum EHexTemplFieldType;
	switch (pField->eType) {
	case custom_size: //If field is of a custom size we cycling through the size field.
		switch (pField->iSize) {
		case 1:
			u64FieldData = ut::GetIHexTData<std::uint8_t>(*m_pHexCtrl, u64FieldOffset);
			break;
		case 2:
		{
			const auto u16 = ut::GetIHexTData<std::uint16_t>(*m_pHexCtrl, u64FieldOffset);
			u64FieldData = pField->fBigEndian ? ut::ByteSwap(u16) : u16;
		}
		break;
		case 4:
		{
			const auto u32 = ut::GetIHexTData<std::uint32_t>(*m_pHexCtrl, u64FieldOffset);
			u64FieldData = pField->fBigEndian ? ut::ByteSwap(u32) : u32;
		}
		break;
		case 8:
		{
			const auto u64 = ut::GetIHexTData<std::uint64_t>(*m_pHexCtrl, u64FieldOffset);
			u64FieldData = pField->fBigEndian ? ut::ByteSwap(u64) : u64;
		}
		break;
		default:
			break;
		}
		break;
	case type_int8:
	case type_uint8:
		u64FieldData = ut::GetIHexTData<std::uint8_t>(*m_pHexCtrl, u64FieldOffset);
		break;
	case type_int16:
	case type_uint16:
	{
		const auto u16 = ut::GetIHexTData<std::uint16_t>(*m_pHexCtrl, u64FieldOffset);
		u64FieldData = pField->fBigEndian ? ut::ByteSwap(u16) : u16;
	}
	break;
	case type_int32:
	case type_uint32:
	{
		const auto u32 = ut::GetIHexTData<std::uint32_t>(*m_pHexCtrl, u64FieldOffset);
		u64FieldData = pField->fBigEndian ? ut::ByteSwap(u32) : u32;
	}
	break;
	case type_int64:
	case type_uint64:
	{
		const auto u64 = ut::GetIHexTData<std::uint64_t>(*m_pHexCtrl, u64FieldOffset);
		u64FieldData = pField->fBigEndian ? ut::ByteSwap(u64) : u64;
	}
	break;
	default:
		break;
	}

	using enum EHexTemplJumpDirection;
	u64FieldData *= jump->u32Units;
	const auto pHex = GetHexCtrl();
	const auto u64HexDataSize = pHex->GetDataSize();

	std::uint64_t u64OffsetToJump { 0xFFFFFFFFFFFFFFFFULL }; //Max value as a sentinel.
	using enum EHexTemplJumpAnchor;
	switch (jump->eAnchor) {
	case DATA_START:
		if (jump->eDirection == JUMP_FORWARD) {
			u64OffsetToJump = u64FieldData;
		}
		else {
			ut::DBG_REPORT(L"Jump backward from the data beginnig attempted.");
		}
		break;
	case DATA_END:
		if (jump->eDirection == JUMP_FORWARD) {
			ut::DBG_REPORT(L"Jump forward from the data end attempted.");
		}
		else {
			if (u64FieldData < u64HexDataSize) {
				u64OffsetToJump = u64HexDataSize - u64FieldData - 1; //Last offset is always one smaller than data size.
			}
		}
		break;
	case FIELD_THIS:
		if (jump->eDirection == JUMP_FORWARD) {
			u64OffsetToJump = u64FieldOffset + u64FieldData;
		}
		else {
			if (u64FieldOffset >= u64FieldData) {
				u64OffsetToJump = u64FieldOffset - u64FieldData;
			}
		}
		break;
	case FIELD_FIRST:
	{
		const auto u64FirstFieldOffset = GUIGetTreeSelectedTemplate()->u64Offset + (*m_pVecFieldsCurr)[0]->iOffset;
		if (jump->eDirection == JUMP_FORWARD) {
			u64OffsetToJump = u64FirstFieldOffset + u64FieldData;
		}
		else {
			if (u64FirstFieldOffset >= u64FieldData) {
				u64OffsetToJump = u64FirstFieldOffset - u64FieldData;
			}
		}
	}
	break;
	case OFFSET_CUSTOM:
		if (jump->eDirection == JUMP_FORWARD) {
			u64OffsetToJump = jump->u64Anchor + u64FieldData;
		}
		else {
			if (jump->u64Anchor >= u64FieldData) {
				u64OffsetToJump = jump->u64Anchor - u64FieldData;
			}
		}
		break;
	default:
		break;
	}

	if (u64OffsetToJump < u64HexDataSize) {
		pHex->GoToOffset(u64OffsetToJump);
		pHex->SetCaretPos(u64OffsetToJump, true, true);
		GUIUpdateEditBoxOffsetToCurrHexCaret();
	}
}

void CHexDlgTemplMgr::WMNotifyListRClick([[maybe_unused]] NMHDR* pNMHDR) { }

void CHexDlgTemplMgr::WMNotifyListSetData(NMHDR* pNMHDR)
{
	const auto pLDI = reinterpret_cast<LISTEX::PLISTEXDATAINFO>(pNMHDR);
	const auto pwszText = pLDI->pwszData;
	const auto& pField = (*m_pVecFieldsCurr)[pLDI->iItem];

	if (pLDI->iSubItem == COL_DATA) {
		if (!m_pHexCtrl->IsDataSet()) {
			return;
		}

		const auto ullOffset = GUIGetTreeSelectedTemplate()->u64Offset + pField->iOffset;
		const auto fShouldSwap = pField->fBigEndian == !IsSwapEndian();

		bool fSetRet { };
		using enum EHexTemplFieldType;
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

void CHexDlgTemplMgr::WMNotifyTreeGetDispInfo(NMHDR* pNMHDR)
{
	const auto pDispInfo = reinterpret_cast<NMTVDISPINFOW*>(pNMHDR);
	const auto pItem = &pDispInfo->item;
	if ((pItem->mask & TVIF_TEXT) == 0)
		return;

	const std::wstring* pwstr;
	const auto uzItemData = m_WndTree.GetItemData(pItem->hItem);
	if (m_WndTree.GetParentItem(pItem->hItem) == nullptr) { //Root node.
		pwstr = &TMPLGetTemplateByID(static_cast<int>(uzItemData))->wstrName;
	}
	else {
		pwstr = &reinterpret_cast<PCHEXTEMPLFIELD>(uzItemData)->wstrName;
	}
	std::copy(pwstr->begin(), pwstr->end(), pItem->pszText);
}

void CHexDlgTemplMgr::WMNotifyTreeItemChanged(NMHDR* pNMHDR)
{
	const auto pTree = reinterpret_cast<LPNMTREEVIEWW>(pNMHDR);
	const auto pItemNew = &pTree->itemNew;
	if (pItemNew->hItem == nullptr) { //Null item was selected (SelectItem(nullptr)).
		return;
	}

	const auto pItemOld = &pTree->itemOld;
	const auto hItemParent = m_WndTree.GetParentItem(pItemNew->hItem);
	const auto pTemplateField = reinterpret_cast<PCHEXTEMPLFIELD>(m_WndTree.GetItemData(pItemNew->hItem));

	//Item was changed by m_WndTree.SelectItem (from list) or by m_WndTree.DeleteItem.
	//If hItemParent==nullptr and action==TVC_UNKNOWN, it was RClick on root node.
	if (pTree->action == TVC_UNKNOWN && hItemParent != nullptr && !m_fTreeClickedWithMouse) {
		SetHexSelByField(pTemplateField);
		return;
	}

	m_fTreeClickedWithMouse = false;
	m_fListGuardEvent = true; //To not trigger OnListItemChanged on the way.
	bool fRootNodeClick { false };
	PCHEXTEMPLFIELD pFieldCurr { };
	PCHexVecTemplFields pVecCurrFields { };
	const auto iTemplateIDPrev = GUIGetTemplateIDFromTree(pItemOld->hItem);
	const auto iTemplateIDCurr = GUIGetTemplateIDFromTree(pItemNew->hItem);
	const auto pTemplateCurr = GUIGetTreeSelectedTemplate();
	m_ListEx.SetItemState(-1, 0, LVIS_SELECTED | LVIS_FOCUSED); //Deselect all items.

	if (hItemParent == nullptr) { //Root item.
		fRootNodeClick = true;
		pVecCurrFields = &pTemplateCurr->vecFields; //On Root item click, set pVecCurrFields to Template's main vecFields.
		m_hTreeCurrParent = pItemNew->hItem;
	}
	else { //Child items.
		pFieldCurr = pTemplateField;
		if (pFieldCurr->pFieldParent == nullptr) {
			if (pFieldCurr->vecNested.empty()) { //On first level child items, set pVecCurrFields to Template's main vecFields.
				pVecCurrFields = &pTemplateCurr->vecFields;
				m_hTreeCurrParent = hItemParent;
			}
			else { //If it's nested Fields vector, set pVecCurrFields to it.
				fRootNodeClick = true;
				pVecCurrFields = &pFieldCurr->vecNested;
				m_hTreeCurrParent = pItemNew->hItem;
			}
		}
		else { //If it's nested Field, set pVecCurrFields to parent Fields' vecNested.
			if (pFieldCurr->vecNested.empty()) {
				pVecCurrFields = &pFieldCurr->pFieldParent->vecNested;
				m_hTreeCurrParent = hItemParent;
			}
			else {
				fRootNodeClick = true;
				pVecCurrFields = &pFieldCurr->vecNested;
				m_hTreeCurrParent = pItemNew->hItem;
			}
		}
	}

	//To not trigger SetItemCountEx, which is slow, every time the Tree item changes.
	//But only if Fields vector changes, or other applied template has been clicked.
	if ((pVecCurrFields != m_pVecFieldsCurr) || (iTemplateIDPrev != iTemplateIDCurr)) {
		m_pVecFieldsCurr = pVecCurrFields;
		m_ListEx.SetItemCountEx(static_cast<int>(m_pVecFieldsCurr->size()), LVSICF_NOSCROLL);
	}

	GUIUpdateStaticText();

	if (!fRootNodeClick) {
		int iIndexHighlight { 0 }; //Index to highlight in the list.
		auto hChild = m_WndTree.GetNextItem(hItemParent, TVGN_CHILD);
		while (hChild != pItemNew->hItem) { //Checking for currently selected item in the tree.
			++iIndexHighlight;
			hChild = m_WndTree.GetNextSiblingItem(hChild);
		}
		m_ListEx.SetItemState(iIndexHighlight, LVIS_SELECTED, LVIS_SELECTED);
		m_ListEx.EnsureVisible(iIndexHighlight, FALSE);
	}

	SetHexSelByField(pFieldCurr);
	m_fListGuardEvent = false;
}

void CHexDlgTemplMgr::WMNotifyTreeLClick([[maybe_unused]] NMHDR* pNMHDR) {
	POINT ptScreen;
	::GetCursorPos(&ptScreen);
	m_WndTree.ScreenToClient(&ptScreen);
	UINT uFlags { };
	if (const auto hItem = m_WndTree.HitTest(ptScreen, &uFlags);
		(hItem != nullptr) && (uFlags & TVHT_ONITEM)) { //Some item was clicked.
		m_fTreeClickedWithMouse = true;

		//The code below is to trigger the TVN_SELCHANGEDW event,
		//when already selected tree item is left-mouse clicked.
		if (m_WndTree.GetSelectedItem() == hItem) { //Already selected item was left-mouse clicked.
			m_WndTree.SelectItem(nullptr);
			m_WndTree.SelectItem(hItem);
		}
	}
}

void CHexDlgTemplMgr::WMNotifyTreeRClick([[maybe_unused]] NMHDR* pNMHDR) {
	POINT ptScreen;
	::GetCursorPos(&ptScreen);
	POINT ptTree = ptScreen;
	m_WndTree.ScreenToClient(&ptTree);
	const auto hTreeItem = m_WndTree.HitTest(ptTree);
	const auto fHitTest = hTreeItem != nullptr;
	const auto fHasApplied = HasApplied();

	if (fHitTest) {
		m_fTreeClickedWithMouse = true;
		m_WndTree.SelectItem(hTreeItem);
	}

	m_MenuTree.EnableItem(static_cast<UINT>(EMenuID::IDM_TREE_RNDCOLORS), fHasApplied && fHitTest);
	m_MenuTree.EnableItem(static_cast<UINT>(EMenuID::IDM_TREE_DISAPPLY), fHasApplied && fHitTest);
	m_MenuTree.EnableItem(static_cast<UINT>(EMenuID::IDM_TREE_DISAPPLYALL), fHasApplied);
	m_MenuTree.TrackPopupMenu(ptScreen.x, ptScreen.y, m_Wnd);
}

auto CHexDlgTemplMgr::WMSize(const MSG& msg)->INT_PTR
{
	const auto wWidth = LOWORD(msg.lParam);
	m_SplitVert.SetEdges(100, wWidth - 10);

	return TRUE;
}

auto CHexDlgTemplMgr::JSONGetTemplateNameProperty(const wchar_t* pwszFilePath)->std::wstring {
	if (pwszFilePath == nullptr) {
		ut::DBG_REPORT(L"pwszFilePath == nullptr");
		return { };
	}

	std::ifstream ifs(pwszFilePath);
	if (!ifs.is_open()) {
		ut::DBG_REPORT(std::format(L"{}\r\n!ifs.is_open()", pwszFilePath).data());
		return { };
	}

	rapidjson::IStreamWrapper isw { ifs };
	rapidjson::Document docJSON;
	docJSON.ParseStream(isw);
	if (docJSON.IsNull()) {
		ut::DBG_REPORT(std::format(L"{}\r\ndocJSON.IsNull()", pwszFilePath).data());
		return { };
	}

	const auto itTName = docJSON.FindMember("TemplateName");
	if (itTName == docJSON.MemberEnd() || !itTName->value.IsString()) {
		ut::DBG_REPORT(std::format(L"{}\r\nTemplate must have a string type name.", pwszFilePath).data());
		return { };
	}

	return ut::StrToWstr(itTName->value.GetString());
}

auto CHexDlgTemplMgr::TreeSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIDSubclass,
	 [[maybe_unused]] DWORD_PTR dwRefData)->LRESULT {
	switch (uMsg) {
	case WM_KILLFOCUS:
		return 0; //Do nothing when Tree loses focus, to save current selection.
	case WM_LBUTTONDOWN:
		::SetFocus(hWnd);
		break;
	case WM_NCDESTROY:
		::RemoveWindowSubclass(hWnd, TreeSubclassProc, uIDSubclass);
		break;
	default:
		break;
	}

	return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
}