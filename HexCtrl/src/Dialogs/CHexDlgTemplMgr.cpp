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
	ON_NOTIFY(LVN_GETDISPINFOW, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListGetDispInfo)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListItemChanged)
	ON_NOTIFY(NM_RCLICK, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListRClick)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL CHexDlgTemplMgr::Create(UINT nIDTemplate, CWnd* pParent, IHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	if (pHexCtrl == nullptr)
		return FALSE;

	m_pHexCtrl = pHexCtrl;

	return CDialogEx::Create(nIDTemplate, pParent);
}

void CHexDlgTemplMgr::ApplyCurr(ULONGLONG ullOffset)
{
	const auto iIndex = m_stComboTemplates.GetCurSel();
	if (iIndex == CB_ERR)
		return;

	const auto iTemplateID = static_cast<int>(m_stComboTemplates.GetItemData(iIndex));
	ApplyTemplate(ullOffset, iTemplateID);
}

void CHexDlgTemplMgr::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_COMBO_TEMPLATES, m_stComboTemplates);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_EDIT_OFFSET, m_stEditOffset);
}

BOOL CHexDlgTemplMgr::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_pListApplied->CreateDialogCtrl(IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, this);
	m_pListApplied->SetExtendedStyle(LVS_EX_HEADERDRAGDROP);
	m_pListApplied->InsertColumn(0, L"\u2116", 0, 40);
	m_pListApplied->InsertColumn(1, L"Template", LVCFMT_LEFT, 200);
	m_pListApplied->InsertColumn(2, L"Offset", LVCFMT_LEFT, 150);
	m_pListApplied->InsertColumn(3, L"Size", LVCFMT_LEFT, 150);

	m_stMenuList.CreatePopupMenu();
	m_stMenuList.AppendMenuW(MF_BYPOSITION, static_cast<UINT_PTR>(EMenuID::IDM_APPLIED_DISAPPLY), L"Disapply template(s)");
	m_stMenuList.AppendMenuW(MF_BYPOSITION, static_cast<UINT_PTR>(EMenuID::IDM_APPLIED_CLEARALL), L"Clear all");

	m_stEditOffset.SetWindowTextW(L"0x0");

	return TRUE;
}

BOOL CHexDlgTemplMgr::OnCommand(WPARAM wParam, LPARAM lParam)
{
	using enum EMenuID;
	switch (static_cast<EMenuID>(LOWORD(wParam)))
	{
	case IDM_APPLIED_DISAPPLY:
		while (m_pListApplied->GetSelectedCount() > 0) {
			const auto nItem = m_pListApplied->GetNextItem(-1, LVNI_SELECTED);
			DisapplyByID(m_vecTemplatesApplied.at(nItem).iAppliedID);
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
			MessageBoxW(L"Error when trying parsing a template.", pwstrPath, MB_ICONERROR);
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

void CHexDlgTemplMgr::OnListGetDispInfo(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto* const pDispInfo = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
	const auto* const pItem = &pDispInfo->item;

	if (pItem->mask & LVIF_TEXT)
	{
		const auto nItemID = static_cast<size_t>(pItem->iItem);
		switch (pItem->iSubItem)
		{
		case 0: //Index value.
			*std::format_to(pItem->pszText, L"{}", nItemID + 1) = L'\0';
			break;
		case 1: //Template name.
			*std::format_to(pItem->pszText, L"{}", m_vecTemplatesApplied[nItemID].pTemplate->wstrName) = L'\0';
			break;
		case 2: //Offset.
			*std::format_to(pItem->pszText, L"0x{:X}", m_vecTemplatesApplied[nItemID].ullOffset) = L'\0';
			break;
		case 3: //Size.
			*std::format_to(pItem->pszText, L"0x{}", m_vecTemplatesApplied[nItemID].ullSizeTotal) = L'\0';
			break;
		default:
			break;
		}
	}
}

void CHexDlgTemplMgr::OnListItemChanged(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	const auto iItem = pNMI->iItem;
	if (iItem < 0)
		return;

	const auto ullOffset = m_vecTemplatesApplied.at(iItem).ullOffset;
	if (m_pHexCtrl->IsDataSet()) {
		m_pHexCtrl->GoToOffset(ullOffset, -1);
	}
}

void CHexDlgTemplMgr::OnListRClick(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	const auto fEnabled = m_pListApplied->GetItemCount() > 0;
	const auto fSelected = m_pListApplied->GetSelectedCount() > 0;
	m_stMenuList.EnableMenuItem(static_cast<UINT>(EMenuID::IDM_APPLIED_DISAPPLY),
		(fEnabled && fSelected ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
	m_stMenuList.EnableMenuItem(static_cast<UINT>(EMenuID::IDM_APPLIED_CLEARALL), (fEnabled ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);

	POINT pt;
	GetCursorPos(&pt);
	m_stMenuList.TrackPopupMenuEx(TPM_LEFTALIGN, pt.x, pt.y, this, nullptr);
}

void CHexDlgTemplMgr::OnDestroy()
{
	CDialogEx::OnDestroy();

	m_pListApplied->DestroyWindow();
	m_stMenuList.DestroyMenu();
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

	STEMPLATE stTempl;
	auto& refFields = stTempl.vecFields;

	if (const auto it = docJSON.FindMember("TemplateName"); it != docJSON.MemberEnd()) {
		stTempl.wstrName = StrToWstr(it->value.GetString());
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

	const auto lmbParseFields = [&lmbStrToRGB, clrBkDefault, clrTextDefault]
	(auto& refJSON, std::vector<STEMPLATEFIELD>& vecFields)->bool {
		const auto _lmbParse = [&lmbStrToRGB](const auto& lmbSelf, auto& refJSON,
			std::vector<STEMPLATEFIELD>& vecFields, COLORREF clrBkDefault, COLORREF clrTextDefault)->bool
		{
			for (auto iterFields = refJSON->value.MemberBegin(); iterFields != refJSON->value.MemberEnd(); ++iterFields)
			{
				if (const auto innerFields = iterFields->value.FindMember("Fields");
					innerFields != iterFields->value.MemberEnd() && innerFields->value.IsObject()) {
					auto& refBack = vecFields.emplace_back();

					COLORREF clrBkDefaultInner { };   //Default colors in inner structs.
					COLORREF clrTextDefaultInner { };
					if (const auto itClrBkDefault = iterFields->value.FindMember("clrBkDefault");
						itClrBkDefault != iterFields->value.MemberEnd()) {
						clrBkDefaultInner = lmbStrToRGB(itClrBkDefault->value.GetString());
					}
					else { clrBkDefaultInner = clrBkDefault; }

					if (const auto itClrTextDefault = iterFields->value.FindMember("clrTextDefault");
						itClrTextDefault != iterFields->value.MemberEnd()) {
						clrTextDefaultInner = lmbStrToRGB(itClrTextDefault->value.GetString());
					}
					else { clrTextDefaultInner = clrTextDefault; }

					//Recursion lambda for nested structs starts here.
					if (!lmbSelf(lmbSelf, innerFields, refBack.vecInner, clrBkDefaultInner, clrTextDefaultInner)) {
						return false;
					}
				}
				else
				{
					auto& refBack = vecFields.emplace_back();
					refBack.wstrName = StrToWstr(iterFields->name.GetString());

					if (const auto itFieldSize = iterFields->value.FindMember("size");
						itFieldSize != iterFields->value.MemberEnd()) {
						refBack.iSize = itFieldSize->value.GetInt();
					}
					else {
						return false; //Every field must have a "size". }
					}

					if (const auto itClrBkDefault = iterFields->value.FindMember("clrBk");
						itClrBkDefault != iterFields->value.MemberEnd()) {
						refBack.clrBk = lmbStrToRGB(itClrBkDefault->value.GetString());
					}
					else {
						refBack.clrBk = clrBkDefault;
					}

					if (const auto itClrTextDefault = iterFields->value.FindMember("clrText");
						itClrTextDefault != iterFields->value.MemberEnd()) {
						refBack.clrText = lmbStrToRGB(itClrTextDefault->value.GetString());
					}
					else {
						refBack.clrText = clrTextDefault;
					}
				}
			}

			return true;
		};

		return _lmbParse(_lmbParse, refJSON, vecFields, clrBkDefault, clrTextDefault);
	};

	auto iTemplateID = 1; //ID starts at 1.
	if (const auto iter = std::max_element(m_vecTemplates.begin(), m_vecTemplates.end(),
		[](const std::unique_ptr<STEMPLATE>& ref1, const std::unique_ptr<STEMPLATE>& ref2)
		{ return ref1->iTemplateID < ref2->iTemplateID; }); iter != m_vecTemplates.end()) {
		iTemplateID = iter->get()->iTemplateID + 1; //Increasing next Template's ID by 1.
	}

	if (const auto rootFields = docJSON.FindMember("Fields");
		rootFields != docJSON.MemberEnd() && rootFields->value.IsObject()) {
		if (lmbParseFields(rootFields, refFields)) {
			stTempl.iTemplateID = iTemplateID;
			m_vecTemplates.emplace_back(std::make_unique<STEMPLATE>(stTempl));
			const auto iIndex = m_stComboTemplates.AddString(stTempl.wstrName.data());
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
		[iTemplateID](const std::unique_ptr<STEMPLATE>& ref) { return ref->iTemplateID == iTemplateID; });
	if (iterTempl == m_vecTemplates.end())
		return;

	//Remove all applied templates from m_vecTemplatesApplied, if any,  with the given iTemplateID.
	if (std::erase_if(m_vecTemplatesApplied, [ptr = iterTempl->get()](const STEMPLATESAPPLIED& ref)
		{ return ref.pTemplate == ptr; }) > 0)
	{
		m_pListApplied->SetItemCountEx(static_cast<int>(m_vecTemplatesApplied.size()));
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

int CHexDlgTemplMgr::ApplyTemplate(ULONGLONG ullOffset, int iTemplateID)
{
	const auto iterTempl = std::find_if(m_vecTemplates.begin(), m_vecTemplates.end(),
		[iTemplateID](const std::unique_ptr<STEMPLATE>& ref) { return ref->iTemplateID == iTemplateID; });
	if (iterTempl == m_vecTemplates.end())
		return 0;

	//Count total size of all Fields.
	const auto lmbTotalSize = [](const std::vector<STEMPLATEFIELD>& vecRef)->ULONGLONG {
		const auto _lmbCount = [](const auto& lmbSelf, const std::vector<STEMPLATEFIELD>& vecRef)->ULONGLONG {
			return std::accumulate(vecRef.begin(), vecRef.end(), 0ULL, [&lmbSelf](auto ullTotal, const STEMPLATEFIELD& refField) {
				if (!refField.vecInner.empty()) {
					return ullTotal + lmbSelf(lmbSelf, refField.vecInner);
				}
				return ullTotal + refField.iSize; });
		};
		return _lmbCount(_lmbCount, vecRef);
	};
	const auto ullTotalSize = lmbTotalSize(iterTempl->get()->vecFields);

	auto iAppliedID = 1; //AppliedID starts at 1.
	if (const auto iter = std::max_element(m_vecTemplatesApplied.begin(), m_vecTemplatesApplied.end(),
		[](const STEMPLATESAPPLIED& ref1, const STEMPLATESAPPLIED& ref2)
		{ return ref1.iAppliedID < ref2.iAppliedID; }); iter != m_vecTemplatesApplied.end()) {
		iTemplateID = iter->iAppliedID + 1; //Increasing next AppliedID by 1.
	}
	m_vecTemplatesApplied.emplace_back(ullOffset, ullTotalSize, iterTempl->get(), iAppliedID);
	m_pListApplied->SetItemCountEx(static_cast<int>(m_vecTemplatesApplied.size()));
	if (m_pHexCtrl->IsDataSet()) {
		m_pHexCtrl->Redraw();
	}

	return iAppliedID;
}

void CHexDlgTemplMgr::ClearAll()
{
	m_vecTemplatesApplied.clear();
	m_pListApplied->SetItemCountEx(0);
	if (m_pHexCtrl->IsDataSet()) {
		m_pHexCtrl->Redraw();
	}
}

void CHexDlgTemplMgr::DisapplyByID(int iAppliedID)
{
	if (const auto iter = std::find_if(m_vecTemplatesApplied.begin(), m_vecTemplatesApplied.end(),
		[iAppliedID](const STEMPLATESAPPLIED& ref) { return ref.iAppliedID == iAppliedID; }); iter != m_vecTemplatesApplied.end()) {
		m_vecTemplatesApplied.erase(iter);
		m_pListApplied->SetItemCountEx(static_cast<int>(m_vecTemplatesApplied.size()));
		if (m_pHexCtrl->IsDataSet()) {
			m_pHexCtrl->Redraw();
		}
	}
}

void CHexDlgTemplMgr::DisapplyByOffset(ULONGLONG ullOffset)
{
	if (const auto rIter = std::find_if(m_vecTemplatesApplied.rbegin(), m_vecTemplatesApplied.rend(),
		[ullOffset](const STEMPLATESAPPLIED& refTempl)
		{ return ullOffset >= refTempl.ullOffset && ullOffset < refTempl.ullOffset + refTempl.ullSizeTotal; });
		rIter != m_vecTemplatesApplied.rend()) {
		m_vecTemplatesApplied.erase(std::next(rIter).base());
		m_pListApplied->SetItemCountEx(static_cast<int>(m_vecTemplatesApplied.size()));
		if (m_pHexCtrl->IsDataSet()) {
			m_pHexCtrl->Redraw();
		}
	}
}

bool CHexDlgTemplMgr::HasApplied()const
{
	return !m_vecTemplatesApplied.empty();
}

bool CHexDlgTemplMgr::HasTemplates() const
{
	return !m_vecTemplates.empty();
}

auto CHexDlgTemplMgr::HitTest(ULONGLONG ullOffset)const->STEMPLATEFIELD*
{
	if (const auto iterTemplate = std::find_if(m_vecTemplatesApplied.rbegin(), m_vecTemplatesApplied.rend(),
		[ullOffset](const STEMPLATESAPPLIED& refTempl)
		{ return ullOffset >= refTempl.ullOffset && ullOffset < refTempl.ullOffset + refTempl.ullSizeTotal; });
		iterTemplate != m_vecTemplatesApplied.rend())
	{
		const auto ullTemplStartOffset = iterTemplate->ullOffset;
		auto& refVec = iterTemplate->pTemplate->vecFields;
		auto ullOffsetCurr = iterTemplate->ullOffset;

		const auto lmbFind = [ullTemplStartOffset, ullOffset, &ullOffsetCurr]
		(std::vector<STEMPLATEFIELD>& vecRef)->STEMPLATEFIELD*
		{
			const auto _lmbFind = [ullTemplStartOffset, ullOffset, &ullOffsetCurr]
			(const auto& lmbSelf, std::vector<STEMPLATEFIELD>& vecRef)->STEMPLATEFIELD* {
				for (auto& refField : vecRef) {
					if (!refField.vecInner.empty()) {
						if (const auto pRet = lmbSelf(lmbSelf, refField.vecInner); pRet != nullptr) {
							return pRet; //Return only if we Hit a pointer in the inner lambda, continue the loop otherwise.
						}
					}
					else {
						if (ullOffset < (ullOffsetCurr + refField.iSize)) {
							return &refField;
						}

						ullOffsetCurr += refField.iSize;
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