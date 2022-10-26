/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "../../dep/rapidjson/rapidjson-amalgam.h"
#include "../../res/HexCtrlRes.h"
#include "../HexUtility.h"
#include "CHexDlgTemplMgr.h"
#include "strsafe.h"
#include <algorithm>
#include <format>
#include <fstream>
#include <numeric>

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

BEGIN_MESSAGE_MAP(CHexDlgTemplMgr, CDialogEx)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_BTN_LOAD, &CHexDlgTemplMgr::OnBnLoadTemplate)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_BTN_UNLOAD, &CHexDlgTemplMgr::OnBnUnloadTemplate)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_BTN_APPLY, &CHexDlgTemplMgr::OnBnApply)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_CHK_TTSHOW, &CHexDlgTemplMgr::OnCheckTtShow)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_CHK_HGLSEL, &CHexDlgTemplMgr::OnCheckHglSel)
	ON_NOTIFY(LVN_GETDISPINFOW, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListGetDispInfo)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListItemChanged)
	ON_NOTIFY(LISTEX::LISTEX_MSG_GETCOLOR, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListGetColor)
	ON_NOTIFY(NM_RCLICK, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListRClick)
	ON_NOTIFY(NM_DBLCLK, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListDblClick)
	ON_NOTIFY(NM_RETURN, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListEnterPressed)
	ON_NOTIFY(NM_RCLICK, IDC_HEXCTRL_TEMPLMGR_TREE_APPLIED, &CHexDlgTemplMgr::OnTreeRClick)
	ON_NOTIFY(TVN_GETDISPINFOW, IDC_HEXCTRL_TEMPLMGR_TREE_APPLIED, &CHexDlgTemplMgr::OnTreeGetDispInfo)
	ON_NOTIFY(TVN_SELCHANGEDW, IDC_HEXCTRL_TEMPLMGR_TREE_APPLIED, &CHexDlgTemplMgr::OnTreeItemChanged)
	ON_WM_DESTROY()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

int CHexDlgTemplMgr::ApplyTemplate(ULONGLONG ullOffset, int iTemplateID)
{
	const auto iterTempl = std::find_if(m_vecTemplates.begin(), m_vecTemplates.end(),
		[iTemplateID](const std::unique_ptr<SHEXTEMPLATE>& ref) { return ref->iTemplateID == iTemplateID; });
	if (iterTempl == m_vecTemplates.end())
		return 0;

	const auto pTemplate = iterTempl->get();

	auto iAppliedID = 1; //AppliedID starts at 1.
	if (const auto iter = std::max_element(m_vecTemplatesApplied.begin(), m_vecTemplatesApplied.end(),
		[](const std::unique_ptr<STEMPLATEAPPLIED>& ref1, const std::unique_ptr<STEMPLATEAPPLIED>& ref2)
		{ return ref1->iAppliedID < ref2->iAppliedID; }); iter != m_vecTemplatesApplied.end()) {
		iAppliedID = iter->get()->iAppliedID + 1; //Increasing next AppliedID by 1.
	}

	const auto pApplied = m_vecTemplatesApplied.emplace_back(
		std::make_unique<STEMPLATEAPPLIED>(ullOffset, pTemplate, iAppliedID)).get();

	TVINSERTSTRUCTW tvi { }; //Tree root node.
	tvi.hParent = TVI_ROOT;
	tvi.itemex.mask = TVIF_CHILDREN | TVIF_TEXT | TVIF_PARAM;
	tvi.itemex.cChildren = static_cast<int>(pTemplate->vecFields.size());
	tvi.itemex.pszText = LPSTR_TEXTCALLBACK;
	tvi.itemex.lParam = reinterpret_cast<LPARAM>(pApplied); //Tree root node has PSTEMPLATEAPPLIED ptr.
	const auto hTreeRootNode = m_stTreeApplied.InsertItem(&tvi);

	const auto lmbFill = [&](HTREEITEM hTreeRoot, VecFields& refVecFields)->void {
		const auto _lmbFill = [&](const auto& lmbSelf, HTREEITEM hTreeRoot, VecFields& refVecFields)->void {
			for (auto& field : refVecFields) {
				tvi.hParent = hTreeRoot;
				tvi.itemex.cChildren = static_cast<int>(field->vecInner.size());
				tvi.itemex.lParam = reinterpret_cast<LPARAM>(field.get()); //Tre child nodes have PSTEMPLATEAPPLIED ptr.
				const auto hCurrentRoot = m_stTreeApplied.InsertItem(&tvi);
				if (tvi.itemex.cChildren > 0) {
					lmbSelf(lmbSelf, hCurrentRoot, field->vecInner);
				}
			}
		};
		_lmbFill(_lmbFill, hTreeRoot, refVecFields);
	};
	lmbFill(hTreeRootNode, pTemplate->vecFields);

	if (m_pHexCtrl->IsDataSet()) {
		m_pHexCtrl->Redraw();
	}

	return iAppliedID;
}

void CHexDlgTemplMgr::ApplyCurr(ULONGLONG ullOffset)
{
	const auto iIndex = m_stComboTemplates.GetCurSel();
	if (iIndex == CB_ERR)
		return;

	const auto iTemplateID = static_cast<int>(m_stComboTemplates.GetItemData(iIndex));
	ApplyTemplate(ullOffset, iTemplateID);
}

void CHexDlgTemplMgr::ClearAll()
{
	m_stTreeApplied.DeleteAllItems();
	m_pListApplied->SetItemCountEx(0);
	m_vecTemplatesApplied.clear();

	if (m_pHexCtrl->IsDataSet()) {
		m_pHexCtrl->Redraw();
	}
}

int CHexDlgTemplMgr::LoadTemplate(const wchar_t* pFilePath)
{
	std::ifstream ifs(pFilePath);
	if (!ifs.is_open())
		return 0;

	rapidjson::IStreamWrapper isw { ifs };
	rapidjson::Document docJSON;
	docJSON.ParseStream(isw);
	if (docJSON.IsNull()) {
		return 0;
	}

	auto pTemplateUnPtr = std::make_unique<SHEXTEMPLATE>();
	const auto pTemplate = pTemplateUnPtr.get();
	auto& refFields = pTemplateUnPtr->vecFields;

	if (const auto it = docJSON.FindMember("TemplateName"); it != docJSON.MemberEnd()) {
		if (!it->value.IsString()) {
			return 0; //"TemplateName" must be a string.
		}
		pTemplateUnPtr->wstrName = StrToWstr(it->value.GetString());
	}
	else {
		return 0; //Template must have a name.
	}

	const auto lmbStrToRGB = [](std::string_view sv)->COLORREF {
		if (sv.empty() || sv.size() != 7 || sv[0] != '#')
			return { };

		const auto R = *StrToUInt(sv.substr(1, 2), 16);
		const auto G = *StrToUInt(sv.substr(3, 2), 16);
		const auto B = *StrToUInt(sv.substr(5, 2), 16);

		return RGB(R, G, B);
	};

	COLORREF clrBkDefault { };
	COLORREF clrTextDefault { };
	if (const auto it = docJSON.FindMember("clrBkDefault"); it != docJSON.MemberEnd()) {
		clrBkDefault = lmbStrToRGB(it->value.GetString());
	}
	if (const auto it = docJSON.FindMember("clrTextDefault"); it != docJSON.MemberEnd()) {
		clrTextDefault = lmbStrToRGB(it->value.GetString());
	}

	//Count total size of all Fields in VecFields, recursively.
	const auto lmbTotalSize = [](const VecFields& vecRef)->int {
		const auto _lmbCount = [](const auto& lmbSelf, const VecFields& vecRef)->int {
			return std::accumulate(vecRef.begin(), vecRef.end(), 0,
				[&lmbSelf](auto ullTotal, const std::unique_ptr<STEMPLATEFIELD>& refField) {
					if (!refField->vecInner.empty()) {
						return ullTotal + lmbSelf(lmbSelf, refField->vecInner);
					}
					return ullTotal + refField->iSize; });
		};
		return _lmbCount(_lmbCount, vecRef);
	};

	const auto lmbParseFields = [&lmbStrToRGB, &lmbTotalSize, clrBkDefault, clrTextDefault, pTemplate]
	(auto& refJSON, VecFields& vecFields)->bool {
		const auto _lmbParse = [&lmbStrToRGB, &lmbTotalSize, pTemplate](const auto& lmbSelf, auto& refJSON,
			VecFields& vecFields, COLORREF clrBkDefault, COLORREF clrTextDefault, int& iOffset,
			PSTEMPLATEFIELD pFieldParent = nullptr)->bool
		{
			for (auto iterFields = refJSON->value.MemberBegin(); iterFields != refJSON->value.MemberEnd(); ++iterFields)
			{
				auto& refBack = vecFields.emplace_back(std::make_unique<STEMPLATEFIELD>());
				refBack->wstrName = StrToWstr(iterFields->name.GetString()); //Name of the field with inner "Fields" field.
				refBack->pTemplate = pTemplate;
				refBack->pFieldParent = pFieldParent;
				refBack->iOffset = iOffset;

				if (const auto innerFields = iterFields->value.FindMember("Fields");
					innerFields != iterFields->value.MemberEnd() && innerFields->value.IsObject()) {

					COLORREF clrBkDefaultInner { };   //Default colors in inner structs.
					COLORREF clrTextDefaultInner { };
					if (const auto itClrBkDefault = iterFields->value.FindMember("clrBkDefault");
						itClrBkDefault != iterFields->value.MemberEnd()) {
						if (!itClrBkDefault->value.IsString()) {
							return false; //"clrBkDefault" is not a string.
						}
						clrBkDefaultInner = lmbStrToRGB(itClrBkDefault->value.GetString());
					}
					else { clrBkDefaultInner = clrBkDefault; }
					refBack->clrBk = clrBkDefaultInner;

					if (const auto itClrTextDefault = iterFields->value.FindMember("clrTextDefault");
						itClrTextDefault != iterFields->value.MemberEnd()) {
						if (!itClrTextDefault->value.IsString()) {
							return false; //"clrTextDefault" is not a string.
						}
						clrTextDefaultInner = lmbStrToRGB(itClrTextDefault->value.GetString());
					}
					else { clrTextDefaultInner = clrTextDefault; }
					refBack->clrText = clrTextDefaultInner;

					//Recursion lambda for nested structs starts here.
					if (!lmbSelf(lmbSelf, innerFields, refBack->vecInner,
						clrBkDefaultInner, clrTextDefaultInner, iOffset, refBack.get())) {
						return false;
					}
					refBack->iSize = lmbTotalSize(refBack->vecInner); //Total size of all inner fields.
				}
				else {
					if (const auto itFieldSize = iterFields->value.FindMember("size");
						itFieldSize != iterFields->value.MemberEnd()) {
						if (itFieldSize->value.IsInt()) {
							refBack->iSize = itFieldSize->value.GetInt();
						}
						else if (itFieldSize->value.IsString()) {
							if (const auto optInt = StrToInt(itFieldSize->value.GetString()); optInt) {
								refBack->iSize = *optInt;
							}
							else {
								return false; //"size" field is a non-convertible string to int.
							}
						}
						else {
							return false; //"size" field neither int nor string.
						}
						iOffset += refBack->iSize;
					}
					else {
						return false; //Every field must have a "size". }
					}

					if (const auto itClrBk = iterFields->value.FindMember("clrBk");
						itClrBk != iterFields->value.MemberEnd()) {
						if (!itClrBk->value.IsString()) {
							return false; //"clrBk" is not a string.
						}
						refBack->clrBk = lmbStrToRGB(itClrBk->value.GetString());
					}
					else {
						refBack->clrBk = clrBkDefault;
					}

					if (const auto itClrText = iterFields->value.FindMember("clrText");
						itClrText != iterFields->value.MemberEnd()) {
						if (!itClrText->value.IsString()) {
							return false; //"clrText" is not a string.
						}
						refBack->clrText = lmbStrToRGB(itClrText->value.GetString());
					}
					else {
						refBack->clrText = clrTextDefault;
					}
				}
			}

			return true;
		};

		int iOffset = 0;
		return _lmbParse(_lmbParse, refJSON, vecFields, clrBkDefault, clrTextDefault, iOffset);
	};

	auto iTemplateID = 1; //ID starts at 1.
	if (const auto iter = std::max_element(m_vecTemplates.begin(), m_vecTemplates.end(),
		[](const std::unique_ptr<SHEXTEMPLATE>& ref1, const std::unique_ptr<SHEXTEMPLATE>& ref2)
		{ return ref1->iTemplateID < ref2->iTemplateID; }); iter != m_vecTemplates.end()) {
		iTemplateID = iter->get()->iTemplateID + 1; //Increasing next Template's ID by 1.
	}

	if (const auto rootFields = docJSON.FindMember("Fields");
		rootFields != docJSON.MemberEnd() && rootFields->value.IsObject()) {
		if (lmbParseFields(rootFields, refFields)) {
			pTemplateUnPtr->iTemplateID = iTemplateID;
			pTemplateUnPtr->iSizeTotal = std::accumulate(pTemplateUnPtr->vecFields.begin(), pTemplateUnPtr->vecFields.end(), 0,
				[](auto iTotal, const std::unique_ptr<STEMPLATEFIELD>& refField) { return iTotal + refField->iSize; });
			const auto iIndex = m_stComboTemplates.AddString(pTemplateUnPtr->wstrName.data());
			m_vecTemplates.emplace_back(std::move(pTemplateUnPtr));
			m_stComboTemplates.SetItemData(iIndex, static_cast<DWORD_PTR>(iTemplateID));
			m_stComboTemplates.SetCurSel(iIndex);
			return iTemplateID;
		}
	}

	return 0;
}

void CHexDlgTemplMgr::UnloadTemplate(int iTemplateID)
{
	const auto iterTempl = std::find_if(m_vecTemplates.begin(), m_vecTemplates.end(),
		[iTemplateID](const std::unique_ptr<SHEXTEMPLATE>& ref) { return ref->iTemplateID == iTemplateID; });
	if (iterTempl == m_vecTemplates.end())
		return;

	const auto pTemplate = iterTempl->get();
	RemoveNodesWithTemplateID(pTemplate->iTemplateID);

	//Remove all applied templates from m_vecTemplatesApplied, if any, with the given iTemplateID.
	if (std::erase_if(m_vecTemplatesApplied, [pTemplate](const std::unique_ptr<STEMPLATEAPPLIED>& ref)
		{ return ref->pTemplate == pTemplate; }) > 0) {
		if (m_pHexCtrl->IsDataSet()) {
			m_pHexCtrl->Redraw();
		}
	}

	m_vecTemplates.erase(iterTempl); //Remove template itself.

	//Remove Template name from ComboBox.
	for (auto iIndex = 0; iIndex < m_stComboTemplates.GetCount(); ++iIndex) {
		if (const auto iItemData = static_cast<int>(m_stComboTemplates.GetItemData(iIndex)); iItemData == iTemplateID) {
			m_stComboTemplates.DeleteString(iIndex);
			m_stComboTemplates.SetCurSel(0);
			break;
		}
	}
}

void CHexDlgTemplMgr::EnableDynamicLayoutHelper(bool fEnable)
{
	if (fEnable && IsDynamicLayoutEnabled())
		return;

	EnableDynamicLayout(fEnable ? TRUE : FALSE);

	if (fEnable) {
		const auto pLayout = GetDynamicLayout();
		pLayout->Create(this);
		pLayout->AddItem(IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, CMFCDynamicLayout::MoveNone(),
			CMFCDynamicLayout::SizeHorizontalAndVertical(100, 100));
		pLayout->AddItem(IDC_HEXCTRL_TEMPLMGR_TREE_APPLIED, CMFCDynamicLayout::MoveNone(),
			CMFCDynamicLayout::SizeVertical(100));
	}
}

auto CHexDlgTemplMgr::GetAppliedFromItem(HTREEITEM hTreeItem)->PSTEMPLATEAPPLIED
{
	auto hRoot = hTreeItem;
	while (hRoot != nullptr) { //Root node.
		hTreeItem = hRoot;
		hRoot = m_stTreeApplied.GetNextItem(hTreeItem, TVGN_PARENT);
	}

	return reinterpret_cast<PSTEMPLATEAPPLIED>(m_stTreeApplied.GetItemData(hTreeItem));
}

void CHexDlgTemplMgr::DisapplyByID(int iAppliedID)
{
	if (const auto iter = std::find_if(m_vecTemplatesApplied.begin(), m_vecTemplatesApplied.end(),
		[iAppliedID](const std::unique_ptr<STEMPLATEAPPLIED>& ref) { return ref->iAppliedID == iAppliedID; });
		iter != m_vecTemplatesApplied.end()) {
		RemoveNodeWithAppliedID(iAppliedID);
		m_vecTemplatesApplied.erase(iter);
		if (m_pHexCtrl->IsDataSet()) {
			m_pHexCtrl->Redraw();
		}
	}
}

bool CHexDlgTemplMgr::IsHighlight()const
{
	return m_fHighlightSel;
}

void CHexDlgTemplMgr::DisapplyByOffset(ULONGLONG ullOffset)
{
	if (const auto rIter = std::find_if(m_vecTemplatesApplied.rbegin(), m_vecTemplatesApplied.rend(),
		[ullOffset](const std::unique_ptr<STEMPLATEAPPLIED>& refTempl)
		{return ullOffset >= refTempl->ullOffset && ullOffset < refTempl->ullOffset + refTempl->pTemplate->iSizeTotal; });
		rIter != m_vecTemplatesApplied.rend()) {
		RemoveNodeWithAppliedID(rIter->get()->iAppliedID);
		m_vecTemplatesApplied.erase(std::next(rIter).base());
		if (m_pHexCtrl->IsDataSet()) {
			m_pHexCtrl->Redraw();
		}
	}
}

bool CHexDlgTemplMgr::HasApplied()const
{
	return !m_vecTemplatesApplied.empty();
}

bool CHexDlgTemplMgr::HasTemplates()const
{
	return !m_vecTemplates.empty();
}

auto CHexDlgTemplMgr::HitTest(ULONGLONG ullOffset)const->PSTEMPLATEFIELD
{
	if (const auto itTemplApplied = std::find_if(m_vecTemplatesApplied.rbegin(), m_vecTemplatesApplied.rend(),
		[ullOffset](const std::unique_ptr<STEMPLATEAPPLIED>& refTempl)
		{ return ullOffset >= refTempl->ullOffset && ullOffset < refTempl->ullOffset + refTempl->pTemplate->iSizeTotal; });
		itTemplApplied != m_vecTemplatesApplied.rend())
	{
		const auto pTemplApplied = itTemplApplied->get();
		const auto ullTemplStartOffset = pTemplApplied->ullOffset;
		auto& refVec = pTemplApplied->pTemplate->vecFields;
		auto ullOffsetCurr = pTemplApplied->ullOffset;

		const auto lmbFind = [ullTemplStartOffset, ullOffset, &ullOffsetCurr]
		(VecFields& vecRef)->PSTEMPLATEFIELD {
			const auto _lmbFind = [ullTemplStartOffset, ullOffset, &ullOffsetCurr]
			(const auto& lmbSelf, VecFields& vecRef)->PSTEMPLATEFIELD {
				for (auto& refField : vecRef) {
					if (!refField->vecInner.empty()) {
						if (const auto pRet = lmbSelf(lmbSelf, refField->vecInner); pRet != nullptr) {
							return pRet; //Return only if we Hit a pointer in the inner lambda, continue the loop otherwise.
						}
					}
					else {
						if (ullOffset < (ullOffsetCurr + refField->iSize)) {
							return refField.get();
						}

						ullOffsetCurr += refField->iSize;
					}
				}
				return nullptr;
			};
			return _lmbFind(_lmbFind, vecRef);
		};

		return lmbFind(refVec);
	}

	return nullptr;
}

bool CHexDlgTemplMgr::IsTooltips()const
{
	return m_fTooltips;
}

void CHexDlgTemplMgr::RefreshData()
{
	if (IsWindowVisible()) {
		m_pListApplied->RedrawWindow();
	}
}

void CHexDlgTemplMgr::RemoveNodesWithTemplateID(int iTemplateID)
{
	std::vector<HTREEITEM> vecToRemove;
	if (auto hItem = m_stTreeApplied.GetRootItem(); hItem != nullptr) {
		while (hItem != nullptr) {
			const auto pApplied = reinterpret_cast<PSTEMPLATEAPPLIED>(m_stTreeApplied.GetItemData(hItem));
			if (pApplied->pTemplate->iTemplateID == iTemplateID) {
				vecToRemove.emplace_back(hItem);
			}

			if (m_pAppliedCurr == pApplied) {
				m_pListApplied->SetItemCountEx(0);
				m_pListApplied->RedrawWindow();
				m_pAppliedCurr = nullptr;
				m_pVecCurrFields = nullptr;
			}

			hItem = m_stTreeApplied.GetNextItem(hItem, TVGN_NEXT); //Get next Root sibling item.
		}
		for (const auto item : vecToRemove) {
			m_stTreeApplied.DeleteItem(item);
		}
	}
}

void CHexDlgTemplMgr::RemoveNodeWithAppliedID(int iAppliedID)
{
	if (auto hItem = m_stTreeApplied.GetRootItem(); hItem != nullptr) {
		while (hItem != nullptr) {
			const auto pApplied = reinterpret_cast<PSTEMPLATEAPPLIED>(m_stTreeApplied.GetItemData(hItem));
			if (pApplied->iAppliedID == iAppliedID) {
				if (m_pAppliedCurr == pApplied) {
					m_pListApplied->SetItemCountEx(0);
					m_pListApplied->RedrawWindow();
					m_pAppliedCurr = nullptr;
					m_pVecCurrFields = nullptr;
					m_hTreeCurrNode = nullptr;
					m_hTreeCurrParent = nullptr;
				}
				m_stTreeApplied.DeleteItem(hItem);
				break;
			}
			hItem = m_stTreeApplied.GetNextItem(hItem, TVGN_NEXT); //Get next Root sibling item.
		}
	}
}

auto CHexDlgTemplMgr::TreeItemFromListItem(int iListItem)const->HTREEITEM
{
	auto hChildItem = m_stTreeApplied.GetNextItem(m_hTreeCurrParent, TVGN_CHILD);
	for (auto iterListItems = 0; iterListItems < iListItem; ++iterListItems) {
		hChildItem = m_stTreeApplied.GetNextSiblingItem(hChildItem);
	}

	return hChildItem;
}

void CHexDlgTemplMgr::SetHexSelByField(PSTEMPLATEFIELD pField)
{
	if (!IsHighlight() || !m_pHexCtrl->IsDataSet() || pField == nullptr)
		return;

	const auto ullOffset = m_pAppliedCurr->ullOffset + pField->iOffset;
	const auto ullSize = static_cast<ULONGLONG>(pField->iSize);

	m_pHexCtrl->SetSelection({ { ullOffset, ullSize } });
	if (!m_pHexCtrl->IsOffsetVisible(ullOffset)) {
		m_pHexCtrl->GoToOffset(ullOffset, -1);
	}
}

void CHexDlgTemplMgr::ShowTooltips(bool fShow)
{
	m_fTooltips = fShow;
}

LRESULT CHexDlgTemplMgr::TreeSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	UINT_PTR /*uIdSubclass*/, DWORD_PTR /*dwRefData*/)
{
	switch (uMsg) {
	case WM_KILLFOCUS:
		return 0; //Do nothing when Tree loses focus, to save current selection.
	case WM_LBUTTONDOWN:
		::SetFocus(hWnd);
		break;
	case WM_NCDESTROY:
		RemoveWindowSubclass(hWnd, TreeSubclassProc, 0);
		break;
	default:
		break;
	}

	return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

BOOL CHexDlgTemplMgr::Create(UINT nIDTemplate, CWnd* pParent, IHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	if (pHexCtrl == nullptr)
		return FALSE;

	m_pHexCtrl = pHexCtrl;

	return CDialogEx::Create(nIDTemplate, pParent);
}

void CHexDlgTemplMgr::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_COMBO_TEMPLATES, m_stComboTemplates);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_EDIT_OFFSET, m_stEditOffset);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_TREE_APPLIED, m_stTreeApplied);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_CHK_TTSHOW, m_stCheckTtShow);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_CHK_HGLSEL, m_stCheckHglSel);
}

BOOL CHexDlgTemplMgr::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_pListApplied->CreateDialogCtrl(IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, this);
	m_pListApplied->SetExtendedStyle(LVS_EX_HEADERDRAGDROP);
	m_pListApplied->InsertColumn(0, L"Name", 0, 150);
	m_pListApplied->InsertColumn(1, L"Offset", 0, 50);
	m_pListApplied->InsertColumn(2, L"Size", LVCFMT_LEFT, 50);
	m_pListApplied->InsertColumn(3, L"Data", LVCFMT_LEFT, 100);
	m_pListApplied->InsertColumn(4, L"Bk color", LVCFMT_LEFT, 60);
	m_pListApplied->InsertColumn(5, L"Text color", LVCFMT_LEFT, 60);

	m_stMenuList.CreatePopupMenu();
	m_stMenuList.AppendMenuW(MF_BYPOSITION, static_cast<UINT_PTR>(EMenuID::IDM_APPLIED_DISAPPLY), L"Disapply template");
	m_stMenuList.AppendMenuW(MF_BYPOSITION, static_cast<UINT_PTR>(EMenuID::IDM_APPLIED_CLEARALL), L"Clear all");

	m_stEditOffset.SetWindowTextW(L"0x0");
	m_stCheckTtShow.SetCheck(IsTooltips() ? BST_CHECKED : BST_UNCHECKED);
	m_stCheckHglSel.SetCheck(IsHighlight() ? BST_CHECKED : BST_UNCHECKED);

	m_hCurResize = static_cast<HCURSOR>(LoadImageW(nullptr, IDC_SIZEWE, IMAGE_CURSOR, 0, 0, LR_SHARED));
	m_hCurArrow = static_cast<HCURSOR>(LoadImageW(nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED));

	EnableDynamicLayoutHelper(true);

	::SetWindowSubclass(m_stTreeApplied, TreeSubclassProc, 0, 0);

	return TRUE;
}

BOOL CHexDlgTemplMgr::OnCommand(WPARAM wParam, LPARAM lParam)
{
	using enum EMenuID;
	switch (static_cast<EMenuID>(LOWORD(wParam)))
	{
	case IDM_APPLIED_DISAPPLY:
		if (const auto pApplied = GetAppliedFromItem(m_stTreeApplied.GetSelectedItem()); pApplied != nullptr) {
			DisapplyByID(pApplied->iAppliedID);
		}
		return TRUE;
	case IDM_APPLIED_CLEARALL:
		ClearAll();
		return TRUE;
	}

	return CDialogEx::OnCommand(wParam, lParam);
}

void CHexDlgTemplMgr::OnBnLoadTemplate()
{
	CFileDialog fd(TRUE, nullptr, nullptr,
		OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_ALLOWMULTISELECT | OFN_DONTADDTORECENT | OFN_ENABLESIZING
		| OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, L"All files (*.*)|*.*||");

	if (fd.DoModal() != IDOK)
		return;

	const CComPtr<IFileOpenDialog> pIFOD = fd.GetIFileOpenDialog();
	CComPtr<IShellItemArray> pResults;
	pIFOD->GetResults(&pResults);

	DWORD dwCount { };
	pResults->GetCount(&dwCount);
	for (unsigned iterFiles = 0; iterFiles < dwCount; ++iterFiles) {
		CComPtr<IShellItem> pItem;
		pResults->GetItemAt(iterFiles, &pItem);
		CComHeapPtr<wchar_t> pwstrPath;
		pItem->GetDisplayName(SIGDN_FILESYSPATH, &pwstrPath);
		if (LoadTemplate(pwstrPath) == 0) {
			MessageBoxW(L"Error when trying to load a template.", pwstrPath, MB_ICONERROR);
		}
	}
}

void CHexDlgTemplMgr::OnBnUnloadTemplate()
{
	if (const auto iIndex = m_stComboTemplates.GetCurSel(); iIndex != CB_ERR) {
		const auto iTemplateID = static_cast<int>(m_stComboTemplates.GetItemData(iIndex));
		UnloadTemplate(iTemplateID);
	}
}

void CHexDlgTemplMgr::OnBnApply()
{
	CString wstrText;
	m_stEditOffset.GetWindowTextW(wstrText);
	const auto opt = StrToULL(wstrText.GetString());
	if (!opt)
		return;

	const auto iIndex = m_stComboTemplates.GetCurSel();
	if (iIndex == CB_ERR)
		return;

	const auto iTemplateID = static_cast<int>(m_stComboTemplates.GetItemData(iIndex));
	ApplyTemplate(*opt, iTemplateID);
}

void CHexDlgTemplMgr::OnCheckTtShow()
{
	ShowTooltips(m_stCheckTtShow.GetCheck() == BST_CHECKED);
}

void CHexDlgTemplMgr::OnCheckHglSel()
{
	m_fHighlightSel = m_stCheckHglSel.GetCheck() == BST_CHECKED;
}

void CHexDlgTemplMgr::OnListGetDispInfo(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pDispInfo = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
	const auto pItem = &pDispInfo->item;

	if ((pItem->mask & LVIF_TEXT) == 0)
		return;

	const auto nItemID = static_cast<size_t>(pItem->iItem);
	const auto& refVecField = *m_pVecCurrFields;
	switch (pItem->iSubItem)
	{
	case 0: //Name.
		pItem->pszText = refVecField[nItemID]->wstrName.data();
		break;
	case 1: //Offset.
		*std::format_to(pItem->pszText, L"{}", refVecField[nItemID]->iOffset) = L'\0';
		break;
	case 2: //Size.
		*std::format_to(pItem->pszText, L"{}", refVecField[nItemID]->iSize) = L'\0';
		break;
	case 3: { //Data.
		if (!m_pHexCtrl->IsDataSet()
			|| m_pAppliedCurr->ullOffset + m_pAppliedCurr->pTemplate->iSizeTotal > m_pHexCtrl->GetDataSize()) //Size overflow check.
			break;

		const auto ullOffset = m_pAppliedCurr->ullOffset + refVecField[nItemID]->iOffset;
		switch (refVecField[nItemID]->iSize) {
		case 1:
			*std::format_to(pItem->pszText, L"0x{:X}", GetIHexTData<BYTE>(*m_pHexCtrl, ullOffset)) = L'\0';
			break;
		case 2:
			*std::format_to(pItem->pszText, L"0x{:X}", GetIHexTData<WORD>(*m_pHexCtrl, ullOffset)) = L'\0';
			break;
		case 4:
			*std::format_to(pItem->pszText, L"0x{:X}", GetIHexTData<DWORD>(*m_pHexCtrl, ullOffset)) = L'\0';
			break;
		case 8:
			*std::format_to(pItem->pszText, L"0x{:X}", GetIHexTData<QWORD>(*m_pHexCtrl, ullOffset)) = L'\0';
			break;
		}
	}
		  break;
	default:
		break;
	}
}

void CHexDlgTemplMgr::OnListGetColor(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	if (pNMI->iSubItem != 4 && pNMI->iSubItem != 5)
		return;

	const auto& refVecField = *m_pVecCurrFields;
	switch (pNMI->iSubItem) {
	case 4: //Bk color.
		m_stCellClr.clrBk = refVecField[pNMI->iItem]->clrBk;
		break;
	case 5: //Text color
		m_stCellClr.clrBk = refVecField[pNMI->iItem]->clrText;
		break;
	}
	pNMI->lParam = reinterpret_cast<LPARAM>(&m_stCellClr);
}

void CHexDlgTemplMgr::OnListItemChanged(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	const auto iItem = pNMI->iItem;
	if (iItem < 0 || m_fListGuardEvent)
		return;

	m_stTreeApplied.SelectItem(TreeItemFromListItem(iItem));
}

void CHexDlgTemplMgr::OnListDblClick(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	const auto iItem = pNMI->iItem;
	if (iItem < 0)
		return;

	const auto& refVec = *m_pVecCurrFields;
	if (refVec[iItem]->vecInner.empty())
		return;

	m_fListGuardEvent = true; //To prevent nasty OnListItemChanged to fire after this method ends.
	m_pVecCurrFields = &refVec[iItem]->vecInner;

	const auto hItem = TreeItemFromListItem(iItem);
	m_hTreeCurrParent = hItem;
	m_hTreeCurrNode = hItem;
	m_stTreeApplied.Expand(hItem, TVE_EXPAND);

	m_pListApplied->SetItemState(-1, 0, LVIS_SELECTED | LVIS_FOCUSED); //Deselect all items.
	m_pListApplied->SetItemCountEx(static_cast<int>(m_pVecCurrFields->size()));
	m_pListApplied->RedrawWindow();
	m_fListGuardEvent = false;
}

void CHexDlgTemplMgr::OnListEnterPressed(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	const auto uSelected = m_pListApplied->GetSelectedCount();
	if (uSelected != 1)
		return;

	//Simulate DblClick in List with Enter key.
	NMITEMACTIVATE nmii { .iItem = m_pListApplied->GetSelectionMark() };
	OnListDblClick(&nmii.hdr, nullptr);
}

void CHexDlgTemplMgr::OnListRClick(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
}

void CHexDlgTemplMgr::OnTreeGetDispInfo(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pDispInfo = reinterpret_cast<NMTVDISPINFOW*>(pNMHDR);
	const auto pItem = &pDispInfo->item;
	if ((pItem->mask & TVIF_TEXT) == 0)
		return;

	if (m_stTreeApplied.GetParentItem(pItem->hItem) == nullptr) { //Root node.
		const auto pTemplApplied = reinterpret_cast<PSTEMPLATEAPPLIED>(m_stTreeApplied.GetItemData(pItem->hItem));
		StringCchCopyW(pItem->pszText, pItem->cchTextMax, pTemplApplied->pTemplate->wstrName.data());
	}
	else {
		const auto pTemplField = reinterpret_cast<PSTEMPLATEFIELD>(m_stTreeApplied.GetItemData(pItem->hItem));
		StringCchCopyW(pItem->pszText, pItem->cchTextMax, pTemplField->wstrName.data());
	}
}

void CHexDlgTemplMgr::OnTreeItemChanged(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pTree = reinterpret_cast<LPNMTREEVIEWW>(pNMHDR);
	const auto pItem = &pTree->itemNew;
	m_hTreeCurrNode = pItem->hItem;

	if (pTree->action == TVC_UNKNOWN) { //Item was changed by m_stTreeApplied.SelectItem(...) from the list.
		SetHexSelByField(reinterpret_cast<PSTEMPLATEFIELD>(m_stTreeApplied.GetItemData(m_hTreeCurrNode)));
		return;
	}

	m_fListGuardEvent = true; //To not trigger OnListItemChanged on the way.
	bool fRootNodeClick { false };
	PSTEMPLATEFIELD pFieldCurr { };

	if (m_stTreeApplied.GetParentItem(pItem->hItem) == nullptr) { //Root item.
		fRootNodeClick = true;
		const auto pApplied = reinterpret_cast<PSTEMPLATEAPPLIED>(m_stTreeApplied.GetItemData(pItem->hItem));
		m_pVecCurrFields = &pApplied->pTemplate->vecFields; //On Root item click, set m_pVecCurrFields to Template's main vecFields.
		m_hTreeCurrParent = pItem->hItem;
	}
	else { //Child items.
		pFieldCurr = reinterpret_cast<PSTEMPLATEFIELD>(m_stTreeApplied.GetItemData(pItem->hItem));
		if (pFieldCurr->pFieldParent == nullptr) {
			if (pFieldCurr->vecInner.empty()) { //On first level child items, set m_pVecCurrFields to Template's main vecFields.
				m_pVecCurrFields = &pFieldCurr->pTemplate->vecFields;
				m_hTreeCurrParent = m_stTreeApplied.GetParentItem(pItem->hItem);
			}
			else { //If it's nested Fields vector, set m_pVecCurrFields to it.
				fRootNodeClick = true;
				m_pVecCurrFields = &pFieldCurr->vecInner;
				m_hTreeCurrParent = pItem->hItem;
			}
		}
		else { //If it's inner Field, set m_pVecCurrFields to parent Fields' vecInner.
			if (pFieldCurr->vecInner.empty()) {
				m_pVecCurrFields = &pFieldCurr->pFieldParent->vecInner;
				m_hTreeCurrParent = m_stTreeApplied.GetParentItem(pItem->hItem);
			}
			else {
				fRootNodeClick = true;
				m_pVecCurrFields = &pFieldCurr->vecInner;
				m_hTreeCurrParent = pItem->hItem;
			}
		}
	}

	m_pAppliedCurr = GetAppliedFromItem(pItem->hItem);
	m_pListApplied->SetItemState(-1, 0, LVIS_SELECTED | LVIS_FOCUSED); //Deselect all items.
	m_pListApplied->SetItemCountEx(static_cast<int>(m_pVecCurrFields->size()), LVSICF_NOSCROLL);

	if (!fRootNodeClick) {
		int iIndexHighlight { 0 }; //Index to highlight in the list.
		auto hChild = m_stTreeApplied.GetNextItem(m_stTreeApplied.GetParentItem(pItem->hItem), TVGN_CHILD);
		while (hChild != pItem->hItem) { //Checking for currently selected item in the tree.
			++iIndexHighlight;
			hChild = m_stTreeApplied.GetNextSiblingItem(hChild);
		}
		m_pListApplied->SetItemState(iIndexHighlight, LVIS_SELECTED, LVIS_SELECTED);
		m_pListApplied->EnsureVisible(iIndexHighlight, FALSE);
	}

	SetHexSelByField(pFieldCurr);

	m_pListApplied->RedrawWindow();
	m_fListGuardEvent = false;
}

void CHexDlgTemplMgr::OnTreeRClick(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	POINT pt;
	GetCursorPos(&pt);
	POINT ptTree = pt;
	m_stTreeApplied.ScreenToClient(&ptTree);
	const auto fHitTest = m_stTreeApplied.HitTest(ptTree) != nullptr;
	const auto fHasApplied = HasApplied();
	m_stMenuList.EnableMenuItem(static_cast<UINT>(EMenuID::IDM_APPLIED_DISAPPLY),
		(fHasApplied && fHitTest ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
	m_stMenuList.EnableMenuItem(static_cast<UINT>(EMenuID::IDM_APPLIED_CLEARALL), (fHasApplied ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
	m_stMenuList.TrackPopupMenuEx(TPM_LEFTALIGN, pt.x, pt.y, this, nullptr);
}

void CHexDlgTemplMgr::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_fCurInSplitter) {
		m_fLMDownResize = true;
		SetCapture();
		EnableDynamicLayoutHelper(false);
	}

	CDialogEx::OnLButtonDown(nFlags, point);
}

void CHexDlgTemplMgr::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_fLMDownResize = false;
	ReleaseCapture();
	EnableDynamicLayoutHelper(true);

	CDialogEx::OnLButtonUp(nFlags, point);
}

void CHexDlgTemplMgr::OnMouseMove(UINT nFlags, CPoint point)
{
	constexpr auto iResizeAreaWidth = 15;        //Area where cursor turns into resizable (IDC_SIZEWE).
	constexpr auto iWidthBetweenTreeAndList = 1; //Width between tree and list after resizing.
	constexpr auto iMinTreeWidth = 100;          //Tree control minimum width.

	CRect rcList;
	m_pListApplied->GetWindowRect(rcList);
	ScreenToClient(rcList);

	if (m_fLMDownResize) {
		CRect rcTree;
		m_stTreeApplied.GetWindowRect(rcTree);
		ScreenToClient(rcTree);
		rcTree.right = point.x - iWidthBetweenTreeAndList;
		if (rcTree.Width() >= iMinTreeWidth) {
			m_stTreeApplied.SetWindowPos(nullptr, rcTree.left, rcTree.top,
				rcTree.Width(), rcTree.Height(), SWP_NOACTIVATE);

			rcList.left = point.x;
			m_pListApplied->SetWindowPos(nullptr, rcList.left, rcList.top,
				rcList.Width(), rcList.Height(), SWP_NOACTIVATE);
		}
	}
	else {
		const CRect rcSplitter(rcList.left - iResizeAreaWidth, rcList.top, rcList.left, rcList.bottom);
		if (rcSplitter.PtInRect(point)) {
			m_fCurInSplitter = true;
			SetCursor(m_hCurResize);
			SetCapture();
		}
		else {
			m_fCurInSplitter = false;
			SetCursor(m_hCurArrow);
			ReleaseCapture();
		}
	}

	CDialogEx::OnMouseMove(nFlags, point);
}

void CHexDlgTemplMgr::OnOK()
{
	//When Enter is pressed anywhere in dialog and focus is on the m_pListApplied,
	//we simulate pressing Enter in the list by sending WM_KEYDOWN/VK_RETURN to it.
	if (GetFocus() == &*m_pListApplied) {
		m_pListApplied->SendMessageW(WM_KEYDOWN, VK_RETURN);
	}
}

void CHexDlgTemplMgr::OnDestroy()
{
	CDialogEx::OnDestroy();

	ClearAll();
	m_pListApplied->DestroyWindow();
	m_stMenuList.DestroyMenu();
}