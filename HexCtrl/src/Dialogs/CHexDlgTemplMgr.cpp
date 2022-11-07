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
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_CHK_HEX, &CHexDlgTemplMgr::OnCheckHexadecimal)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_CHK_BE, &CHexDlgTemplMgr::OnCheckBigEndian)
	ON_NOTIFY(LVN_GETDISPINFOW, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListGetDispInfo)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListItemChanged)
	ON_NOTIFY(LISTEX::LISTEX_MSG_GETCOLOR, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListGetColor)
	ON_NOTIFY(LISTEX::LISTEX_MSG_EDITBEGIN, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListEditBegin)
	ON_NOTIFY(LISTEX::LISTEX_MSG_DATACHANGED, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListDataChanged)
	ON_NOTIFY(NM_RCLICK, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListRClick)
	ON_NOTIFY(NM_DBLCLK, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListDblClick)
	ON_NOTIFY(NM_RETURN, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListEnterPressed)
	ON_NOTIFY(NM_RCLICK, IDC_HEXCTRL_TEMPLMGR_TREE_APPLIED, &CHexDlgTemplMgr::OnTreeRClick)
	ON_NOTIFY(TVN_GETDISPINFOW, IDC_HEXCTRL_TEMPLMGR_TREE_APPLIED, &CHexDlgTemplMgr::OnTreeGetDispInfo)
	ON_NOTIFY(TVN_SELCHANGEDW, IDC_HEXCTRL_TEMPLMGR_TREE_APPLIED, &CHexDlgTemplMgr::OnTreeItemChanged)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_DESTROY()
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

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
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_CHK_HEX, m_stCheckHex);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_STATIC_OFFSET, m_stStaticOffset);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_STATIC_SIZE, m_stStaticSize);
}

BOOL CHexDlgTemplMgr::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_pListApplied->CreateDialogCtrl(IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, this);
	m_pListApplied->SetExtendedStyle(LVS_EX_HEADERDRAGDROP);
	m_pListApplied->InsertColumn(0, L"Name", LVCFMT_LEFT, 300);
	m_pListApplied->InsertColumn(1, L"Offset", LVCFMT_LEFT, 50);
	m_pListApplied->InsertColumn(2, L"Size", LVCFMT_LEFT, 50);
	m_pListApplied->InsertColumn(3, L"Data", LVCFMT_LEFT, 120);
	m_pListApplied->SetColumnEditable(3, true);
	m_pListApplied->InsertColumn(4, L"Colors", LVCFMT_LEFT, 57);

	m_stMenuTree.CreatePopupMenu();
	m_stMenuTree.AppendMenuW(MF_BYPOSITION, static_cast<UINT_PTR>(EMenuID::IDM_APPLIED_DISAPPLY), L"Disapply template");
	m_stMenuTree.AppendMenuW(MF_BYPOSITION, static_cast<UINT_PTR>(EMenuID::IDM_APPLIED_DISAPPALL), L"Clear all");

	m_stEditOffset.SetWindowTextW(L"0x0");
	m_stCheckTtShow.SetCheck(IsTooltips() ? BST_CHECKED : BST_UNCHECKED);
	m_stCheckHglSel.SetCheck(IsHighlight() ? BST_CHECKED : BST_UNCHECKED);
	m_stCheckHex.SetCheck(m_fShowAsHex ? BST_CHECKED : BST_UNCHECKED);

	m_hCurResize = static_cast<HCURSOR>(LoadImageW(nullptr, IDC_SIZEWE, IMAGE_CURSOR, 0, 0, LR_SHARED));
	m_hCurArrow = static_cast<HCURSOR>(LoadImageW(nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED));

	EnableDynamicLayoutHelper(true);

	::SetWindowSubclass(m_stTreeApplied, TreeSubclassProc, 0, 0);

	return TRUE;
}

BOOL CHexDlgTemplMgr::OnCommand(WPARAM wParam, LPARAM lParam)
{
	using enum EMenuID;
	switch (static_cast<EMenuID>(LOWORD(wParam))) {
	case IDM_APPLIED_DISAPPLY:
		if (const auto pApplied = GetAppliedFromItem(m_stTreeApplied.GetSelectedItem()); pApplied != nullptr) {
			DisapplyByID(pApplied->iAppliedID);
		}
		return TRUE;
	case IDM_APPLIED_DISAPPALL:
		DisapplyAll();
		return TRUE;
	}

	return CDialogEx::OnCommand(wParam, lParam);
}

HBRUSH CHexDlgTemplMgr::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	const auto hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	//Template's applied offset and total size static text color.
	if (pWnd == &m_stStaticOffset || pWnd == &m_stStaticSize) {
		pDC->SetTextColor(RGB(0, 0, 150));
	}

	return hbr;
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

void CHexDlgTemplMgr::OnCheckHexadecimal()
{
	m_fShowAsHex = m_stCheckHex.GetCheck() == BST_CHECKED;
	UpdateStaticText();
	m_pListApplied->RedrawWindow();
}

void CHexDlgTemplMgr::OnCheckBigEndian()
{
	m_fBigEndian = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_TEMPLMGR_CHK_BE))->GetCheck() == BST_CHECKED;
	m_pListApplied->RedrawWindow();
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
	const auto wsvFmt = m_fShowAsHex ? L"0x{:X}" : L"{}";
	switch (pItem->iSubItem) {
	case 0: //Name.
		pItem->pszText = refVecField[nItemID]->wstrName.data();
		break;
	case 1: //Offset.
		*std::vformat_to(pItem->pszText, wsvFmt, std::make_wformat_args(refVecField[nItemID]->iOffset)) = L'\0';
		break;
	case 2: //Size.
		*std::vformat_to(pItem->pszText, wsvFmt, std::make_wformat_args(refVecField[nItemID]->iSize)) = L'\0';
		break;
	case 3: { //Data.
		if (!m_pHexCtrl->IsDataSet()
			|| m_pAppliedCurr->ullOffset + m_pAppliedCurr->pTemplate->iSizeTotal > m_pHexCtrl->GetDataSize()) //Size overflow check.
			break;

		const auto ullOffset = m_pAppliedCurr->ullOffset + refVecField[nItemID]->iOffset;
		switch (refVecField[nItemID]->iSize) {
		case 1:
			*std::vformat_to(pItem->pszText, wsvFmt, std::make_wformat_args(GetIHexTData<BYTE>(*m_pHexCtrl, ullOffset))) = L'\0';
			break;
		case 2: {
			auto wData = GetIHexTData<WORD>(*m_pHexCtrl, ullOffset);
			if (m_fBigEndian) {
				wData = ByteSwap(wData);
			}
			*std::vformat_to(pItem->pszText, wsvFmt, std::make_wformat_args(wData)) = L'\0';
		}
			  break;
		case 4: {
			auto dwData = GetIHexTData<DWORD>(*m_pHexCtrl, ullOffset);
			if (m_fBigEndian) {
				dwData = ByteSwap(dwData);
			}
			*std::vformat_to(pItem->pszText, wsvFmt, std::make_wformat_args(dwData)) = L'\0';
		}
			  break;
		case 8: {
			auto ullData = GetIHexTData<QWORD>(*m_pHexCtrl, ullOffset);
			if (m_fBigEndian) {
				ullData = ByteSwap(ullData);
			}
			*std::vformat_to(pItem->pszText, wsvFmt, std::make_wformat_args(ullData)) = L'\0';
		}
			  break;
		default:
			break;
		}
	}
		  break;
	case 4: { //Colors.
		const auto bR = static_cast<unsigned char>(refVecField[nItemID]->clrText & 0x000000FFU);
		const auto bG = static_cast<unsigned char>((refVecField[nItemID]->clrText & 0x0000FF00U) >> 8);
		const auto bB = static_cast<unsigned char>((refVecField[nItemID]->clrText & 0x00FF0000U) >> 16);
		*std::format_to(pItem->pszText, L"#{:02X}{:02X}{:02X}", bR, bG, bB) = L'\0';
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
	case 4: //Colors.
		m_stCellClr.clrBk = refVecField[pNMI->iItem]->clrBk;
		m_stCellClr.clrText = refVecField[pNMI->iItem]->clrText;
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

void CHexDlgTemplMgr::OnListEditBegin(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	if (pNMI->iSubItem != 3) { //Data.
		pNMI->lParam = FALSE;
		return;
	}

	const auto iItem = pNMI->iItem;
	const auto& refVec = *m_pVecCurrFields;
	if (!refVec[iItem]->vecNested.empty()) {
		pNMI->lParam = FALSE; //Do not show edit-box in nested fields.
	}
}

void CHexDlgTemplMgr::OnListDataChanged(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	if (pNMI->iSubItem != 3) //Data.
		return;

	const auto iItem = pNMI->iItem;
	const auto pwszText = reinterpret_cast<LPCWSTR>(pNMI->lParam);
	const auto& refVecField = *m_pVecCurrFields;
	const auto ullOffset = m_pAppliedCurr->ullOffset + refVecField[iItem]->iOffset;

	switch (refVecField[iItem]->iSize) {
	case 1:
		if (const auto opt = StrToUChar(pwszText); opt) {
			SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
		}
		break;
	case 2:
		if (const auto opt = StrToUShort(pwszText); opt) {
			SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
		}
		break;
	case 4:
		if (const auto opt = StrToUInt(pwszText); opt) {
			SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
		}
		break;
	case 8:
		if (const auto opt = StrToULL(pwszText); opt) {
			SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
		}
		break;
	default:
		return;
	}
	m_pHexCtrl->Redraw();
}

void CHexDlgTemplMgr::OnListDblClick(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	const auto iItem = pNMI->iItem;
	if (iItem < 0)
		return;

	const auto& refVec = *m_pVecCurrFields;
	if (refVec[iItem]->vecNested.empty())
		return;

	m_fListGuardEvent = true; //To prevent nasty OnListItemChanged to fire after this method ends.
	m_pVecCurrFields = &refVec[iItem]->vecNested;

	const auto hItem = TreeItemFromListItem(iItem);
	m_hTreeCurrParent = hItem;
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
		const auto pTemplApplied = reinterpret_cast<PHEXTEMPLATEAPPLIED>(m_stTreeApplied.GetItemData(pItem->hItem));
		StringCchCopyW(pItem->pszText, pItem->cchTextMax, pTemplApplied->pTemplate->wstrName.data());
	}
	else {
		const auto pTemplField = reinterpret_cast<PHEXTEMPLATEFIELD>(m_stTreeApplied.GetItemData(pItem->hItem));
		StringCchCopyW(pItem->pszText, pItem->cchTextMax, pTemplField->wstrName.data());
	}
}

void CHexDlgTemplMgr::OnTreeItemChanged(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pTree = reinterpret_cast<LPNMTREEVIEWW>(pNMHDR);
	const auto pItem = &pTree->itemNew;

	//Item was changed by m_stTreeApplied.SelectItem(...) or by m_stTreeApplied.DeleteItem(...);
	if (pTree->action == TVC_UNKNOWN) {
		if (m_stTreeApplied.GetParentItem(pItem->hItem) != nullptr) {
			const auto dwItemData = m_stTreeApplied.GetItemData(pItem->hItem);
			SetHexSelByField(reinterpret_cast<PHEXTEMPLATEFIELD>(dwItemData));
		}
		return;
	}

	m_fListGuardEvent = true; //To not trigger OnListItemChanged on the way.
	bool fRootNodeClick { false };
	PHEXTEMPLATEFIELD pFieldCurr { };

	if (m_stTreeApplied.GetParentItem(pItem->hItem) == nullptr) { //Root item.
		fRootNodeClick = true;
		const auto pApplied = reinterpret_cast<PHEXTEMPLATEAPPLIED>(m_stTreeApplied.GetItemData(pItem->hItem));
		m_pVecCurrFields = &pApplied->pTemplate->vecFields; //On Root item click, set m_pVecCurrFields to Template's main vecFields.
		m_hTreeCurrParent = pItem->hItem;
	}
	else { //Child items.
		pFieldCurr = reinterpret_cast<PHEXTEMPLATEFIELD>(m_stTreeApplied.GetItemData(pItem->hItem));
		if (pFieldCurr->pFieldParent == nullptr) {
			if (pFieldCurr->vecNested.empty()) { //On first level child items, set m_pVecCurrFields to Template's main vecFields.
				m_pVecCurrFields = &pFieldCurr->pTemplate->vecFields;
				m_hTreeCurrParent = m_stTreeApplied.GetParentItem(pItem->hItem);
			}
			else { //If it's nested Fields vector, set m_pVecCurrFields to it.
				fRootNodeClick = true;
				m_pVecCurrFields = &pFieldCurr->vecNested;
				m_hTreeCurrParent = pItem->hItem;
			}
		}
		else { //If it's nested Field, set m_pVecCurrFields to parent Fields' vecNested.
			if (pFieldCurr->vecNested.empty()) {
				m_pVecCurrFields = &pFieldCurr->pFieldParent->vecNested;
				m_hTreeCurrParent = m_stTreeApplied.GetParentItem(pItem->hItem);
			}
			else {
				fRootNodeClick = true;
				m_pVecCurrFields = &pFieldCurr->vecNested;
				m_hTreeCurrParent = pItem->hItem;
			}
		}
	}

	m_pAppliedCurr = GetAppliedFromItem(pItem->hItem);
	m_pListApplied->SetItemState(-1, 0, LVIS_SELECTED | LVIS_FOCUSED); //Deselect all items.
	m_pListApplied->SetItemCountEx(static_cast<int>(m_pVecCurrFields->size()), LVSICF_NOSCROLL);
	UpdateStaticText();

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
	const auto hTreeItem = m_stTreeApplied.HitTest(ptTree);
	const auto fHitTest = hTreeItem != nullptr;
	const auto fHasApplied = HasApplied();

	if (hTreeItem != nullptr) {
		m_stTreeApplied.SelectItem(hTreeItem);
	}
	m_stMenuTree.EnableMenuItem(static_cast<UINT>(EMenuID::IDM_APPLIED_DISAPPLY),
		(fHasApplied && fHitTest ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
	m_stMenuTree.EnableMenuItem(static_cast<UINT>(EMenuID::IDM_APPLIED_DISAPPALL), (fHasApplied ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
	m_stMenuTree.TrackPopupMenuEx(TPM_LEFTALIGN, pt.x, pt.y, this, nullptr);
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
	constexpr auto iResAreaHalfWidth = 15;       //Area where cursor turns into resizable (IDC_SIZEWE).
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
		const CRect rcSplitter(rcList.left - iResAreaHalfWidth, rcList.top,
			rcList.left + iResAreaHalfWidth, rcList.bottom);
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
	const auto pWndFocus = GetFocus();
	//When Enter is pressed anywhere in the dialog, and focus is on the m_pListApplied,
	//we simulate pressing Enter in the list by sending WM_KEYDOWN/VK_RETURN to it.
	if (pWndFocus == &*m_pListApplied) {
		m_pListApplied->SendMessageW(WM_KEYDOWN, VK_RETURN);
	}
	else if (pWndFocus == &m_stEditOffset) { //Focus is on the "Offset" edit-box.
		OnBnApply();
	}
}

void CHexDlgTemplMgr::OnDestroy()
{
	CDialogEx::OnDestroy();

	DisapplyAll();
	m_pListApplied->DestroyWindow();
	m_stMenuTree.DestroyMenu();
	m_vecTemplates.clear();
}

int CHexDlgTemplMgr::ApplyTemplate(ULONGLONG ullOffset, int iTemplateID)
{
	const auto iterTempl = std::find_if(m_vecTemplates.begin(), m_vecTemplates.end(),
		[iTemplateID](const std::unique_ptr<HEXTEMPLATE>& ref) { return ref->iTemplateID == iTemplateID; });
	if (iterTempl == m_vecTemplates.end())
		return 0;

	const auto pTemplate = iterTempl->get();

	auto iAppliedID = 1; //AppliedID starts at 1.
	if (const auto iter = std::max_element(m_vecTemplatesApplied.begin(), m_vecTemplatesApplied.end(),
		[](const std::unique_ptr<HEXTEMPLATEAPPLIED>& ref1, const std::unique_ptr<HEXTEMPLATEAPPLIED>& ref2) {
			return ref1->iAppliedID < ref2->iAppliedID; }); iter != m_vecTemplatesApplied.end()) {
		iAppliedID = iter->get()->iAppliedID + 1; //Increasing next AppliedID by 1.
	}

	const auto pApplied = m_vecTemplatesApplied.emplace_back(
		std::make_unique<HEXTEMPLATEAPPLIED>(ullOffset, pTemplate, iAppliedID)).get();

	TVINSERTSTRUCTW tvi { }; //Tree root node.
	tvi.hParent = TVI_ROOT;
	tvi.itemex.mask = TVIF_CHILDREN | TVIF_TEXT | TVIF_PARAM;
	tvi.itemex.cChildren = static_cast<int>(pTemplate->vecFields.size());
	tvi.itemex.pszText = LPSTR_TEXTCALLBACK;
	tvi.itemex.lParam = reinterpret_cast<LPARAM>(pApplied); //Tree root node has PHEXTEMPLATEAPPLIED ptr.
	const auto hTreeRootNode = m_stTreeApplied.InsertItem(&tvi);

	const auto lmbFill = [&](HTREEITEM hTreeRoot, VecFields& refVecFields)->void {
		const auto _lmbFill = [&](const auto& lmbSelf, HTREEITEM hTreeRoot, VecFields& refVecFields)->void {
			for (auto& field : refVecFields) {
				tvi.hParent = hTreeRoot;
				tvi.itemex.cChildren = static_cast<int>(field->vecNested.size());
				tvi.itemex.lParam = reinterpret_cast<LPARAM>(field.get()); //Tree child nodes have PHEXTEMPLATEAPPLIED ptr.
				const auto hCurrentRoot = m_stTreeApplied.InsertItem(&tvi);
				if (tvi.itemex.cChildren > 0) {
					lmbSelf(lmbSelf, hCurrentRoot, field->vecNested);
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

void CHexDlgTemplMgr::DisapplyAll()
{
	m_stTreeApplied.DeleteAllItems();
	m_pListApplied->SetItemCountEx(0);
	m_vecTemplatesApplied.clear();
	m_pAppliedCurr = nullptr;
	m_pVecCurrFields = nullptr;
	m_hTreeCurrParent = nullptr;
	UpdateStaticText();

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

	auto pTemplateUnPtr = std::make_unique<HEXTEMPLATE>();
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

	//Convert color string #AABBCC to RGB COLORREF.
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
	if (const auto it = docJSON.FindMember("clrBk"); it != docJSON.MemberEnd() && it->value.IsString()) {
		clrBkDefault = lmbStrToRGB(it->value.GetString());
	}
	if (const auto it = docJSON.FindMember("clrText"); it != docJSON.MemberEnd() && it->value.IsString()) {
		clrTextDefault = lmbStrToRGB(it->value.GetString());
	}

	//Count total size of all Fields in VecFields, recursively.
	const auto lmbTotalSize = [](const VecFields& vecRef)->int {
		const auto _lmbCount = [](const auto& lmbSelf, const VecFields& vecRef)->int {
			return std::accumulate(vecRef.begin(), vecRef.end(), 0,
				[&lmbSelf](auto ullTotal, const std::unique_ptr<HEXTEMPLATEFIELD>& refField) {
					if (!refField->vecNested.empty()) {
						return ullTotal + lmbSelf(lmbSelf, refField->vecNested);
					}
					return ullTotal + refField->iSize; });
		};
		return _lmbCount(_lmbCount, vecRef);
	};

	using IterJSONMember = rapidjson::Value::ConstMemberIterator;
	const auto lmbParseFields = [&lmbStrToRGB, &lmbTotalSize, clrBkDefault, clrTextDefault, pTemplate]
	(const IterJSONMember iterMemberFields, VecFields& vecFields)->bool {
		const auto _lmbParse = [&lmbStrToRGB, &lmbTotalSize, pTemplate]
		(const auto& lmbSelf, const IterJSONMember iterMemberFields,
			VecFields& vecFields, COLORREF clrBkDefault, COLORREF clrTextDefault, int& iOffset,
			PHEXTEMPLATEFIELD pFieldParent = nullptr)->bool {
				for (auto iterArrCurr = iterMemberFields->value.Begin(); iterArrCurr != iterMemberFields->value.End(); ++iterArrCurr) {
					if (!iterArrCurr->IsObject()) {
						return false; //Each array entry must be an Object {}.
					}

					auto& refBack = vecFields.emplace_back(std::make_unique<HEXTEMPLATEFIELD>());

					if (const auto itName = iterArrCurr->FindMember("name");
						itName != iterArrCurr->MemberEnd() && itName->value.IsString()) {
						refBack->wstrName = StrToWstr(itName->value.GetString());
					}
					else {
						return false; //Each array entry (Object) must have a "name" string.
					}

					refBack->pTemplate = pTemplate;
					refBack->pFieldParent = pFieldParent;
					refBack->iOffset = iOffset;

					if (const auto iterNestedFields = iterArrCurr->FindMember("Fields");
						iterNestedFields != iterArrCurr->MemberEnd()) {
						if (!iterNestedFields->value.IsArray()) {
							return false; //Each "Fields" must be an Array.
						}

						COLORREF clrBkDefaultNested { };   //Default colors in nested structs.
						COLORREF clrTextDefaultNested { };
						if (const auto itClrBkDefault = iterArrCurr->FindMember("clrBk");
							itClrBkDefault != iterArrCurr->MemberEnd()) {
							if (!itClrBkDefault->value.IsString()) {
								return false; //"clrBk" must be a string.
							}
							clrBkDefaultNested = lmbStrToRGB(itClrBkDefault->value.GetString());
						}
						else { clrBkDefaultNested = clrBkDefault; }
						refBack->clrBk = clrBkDefaultNested;

						if (const auto itClrTextDefault = iterArrCurr->FindMember("clrText");
							itClrTextDefault != iterArrCurr->MemberEnd()) {
							if (!itClrTextDefault->value.IsString()) {
								return false; //"clrText" must be a string.
							}
							clrTextDefaultNested = lmbStrToRGB(itClrTextDefault->value.GetString());
						}
						else { clrTextDefaultNested = clrTextDefault; }
						refBack->clrText = clrTextDefaultNested;

						//Recursion lambda for nested structs starts here.
						if (!lmbSelf(lmbSelf, iterNestedFields, refBack->vecNested,
							clrBkDefaultNested, clrTextDefaultNested, iOffset, refBack.get())) {
							return false;
						}
						refBack->iSize = lmbTotalSize(refBack->vecNested); //Total size of all nested fields.
					}
					else {
						if (const auto iterSize = iterArrCurr->FindMember("size");
							iterSize != iterArrCurr->MemberEnd()) {
							if (iterSize->value.IsInt()) {
								refBack->iSize = iterSize->value.GetInt();
							}
							else if (iterSize->value.IsString()) {
								if (const auto optInt = StrToInt(iterSize->value.GetString()); optInt) {
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
							return false; //Every Object must have a "size". }
						}

						if (const auto itClrBk = iterArrCurr->FindMember("clrBk");
							itClrBk != iterArrCurr->MemberEnd()) {
							if (!itClrBk->value.IsString()) {
								return false; //"clrBk" must be a string.
							}
							refBack->clrBk = lmbStrToRGB(itClrBk->value.GetString());
						}
						else {
							refBack->clrBk = clrBkDefault;
						}

						if (const auto itClrText = iterArrCurr->FindMember("clrText");
							itClrText != iterArrCurr->MemberEnd()) {
							if (!itClrText->value.IsString()) {
								return false; //"clrText" must be a string.
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
		return _lmbParse(_lmbParse, iterMemberFields, vecFields, clrBkDefault, clrTextDefault, iOffset);
	};

	auto iTemplateID = 1; //ID starts at 1.
	if (const auto iter = std::max_element(m_vecTemplates.begin(), m_vecTemplates.end(),
		[](const std::unique_ptr<HEXTEMPLATE>& ref1, const std::unique_ptr<HEXTEMPLATE>& ref2) {
			return ref1->iTemplateID < ref2->iTemplateID; }); iter != m_vecTemplates.end()) {
		iTemplateID = iter->get()->iTemplateID + 1; //Increasing next Template's ID by 1.
	}

	const auto rootFields = docJSON.FindMember("Fields");
	if (rootFields == docJSON.MemberEnd()) {
		return 0; //No "Fields" member in template.
	}

	if (!rootFields->value.IsArray()) {
		return 0; //"Fields" member must be an Array.
	}

	if (!lmbParseFields(rootFields, refFields)) {
		return 0; //Something went wrong during template parsing.
	}

	pTemplateUnPtr->iTemplateID = iTemplateID;
	pTemplateUnPtr->iSizeTotal = std::accumulate(pTemplateUnPtr->vecFields.begin(), pTemplateUnPtr->vecFields.end(), 0,
		[](auto iTotal, const std::unique_ptr<HEXTEMPLATEFIELD>& refField) { return iTotal + refField->iSize; });
	const auto iIndex = m_stComboTemplates.AddString(pTemplateUnPtr->wstrName.data());
	m_stComboTemplates.SetItemData(iIndex, static_cast<DWORD_PTR>(iTemplateID));
	m_stComboTemplates.SetCurSel(iIndex);
	m_vecTemplates.emplace_back(std::move(pTemplateUnPtr));
	OnTemplateLoadUnload(true);

	return iTemplateID;
}

void CHexDlgTemplMgr::UnloadTemplate(int iTemplateID)
{
	const auto iterTempl = std::find_if(m_vecTemplates.begin(), m_vecTemplates.end(),
		[iTemplateID](const std::unique_ptr<HEXTEMPLATE>& ref) { return ref->iTemplateID == iTemplateID; });
	if (iterTempl == m_vecTemplates.end())
		return;

	const auto pTemplate = iterTempl->get();
	RemoveNodesWithTemplateID(pTemplate->iTemplateID);

	//Remove all applied templates from m_vecTemplatesApplied, if any, with the given iTemplateID.
	if (std::erase_if(m_vecTemplatesApplied, [pTemplate](const std::unique_ptr<HEXTEMPLATEAPPLIED>& ref) {
		return ref->pTemplate == pTemplate; }) > 0) {
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

	OnTemplateLoadUnload(false);
}

void CHexDlgTemplMgr::OnTemplateLoadUnload(bool /*fLoad*/)
{
	if (const auto pBtnApply = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_TEMPLMGR_BTN_APPLY)); pBtnApply != nullptr) {
		pBtnApply->EnableWindow(HasTemplates());
	}
	if (const auto pBtnUnload = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_TEMPLMGR_BTN_UNLOAD)); pBtnUnload != nullptr) {
		pBtnUnload->EnableWindow(HasTemplates());
	}
	m_stEditOffset.EnableWindow(HasTemplates());
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
		pLayout->AddItem(IDC_HEXCTRL_TEMPLMGR_GRB_TOP, CMFCDynamicLayout::MoveNone(),
			CMFCDynamicLayout::SizeHorizontal(100));
	}
}

auto CHexDlgTemplMgr::GetAppliedFromItem(HTREEITEM hTreeItem)->PHEXTEMPLATEAPPLIED
{
	auto hRoot = hTreeItem;
	while (hRoot != nullptr) { //Root node.
		hTreeItem = hRoot;
		hRoot = m_stTreeApplied.GetNextItem(hTreeItem, TVGN_PARENT);
	}

	return reinterpret_cast<PHEXTEMPLATEAPPLIED>(m_stTreeApplied.GetItemData(hTreeItem));
}

void CHexDlgTemplMgr::DisapplyByID(int iAppliedID)
{
	if (const auto iter = std::find_if(m_vecTemplatesApplied.begin(), m_vecTemplatesApplied.end(),
		[iAppliedID](const std::unique_ptr<HEXTEMPLATEAPPLIED>& ref) { return ref->iAppliedID == iAppliedID; });
		iter != m_vecTemplatesApplied.end()) {
		RemoveNodeWithAppliedID(iAppliedID);
		m_vecTemplatesApplied.erase(iter);
		if (m_pHexCtrl->IsDataSet()) {
			m_pHexCtrl->Redraw();
		}
	}
}

void CHexDlgTemplMgr::DisapplyByOffset(ULONGLONG ullOffset)
{
	if (const auto rIter = std::find_if(m_vecTemplatesApplied.rbegin(), m_vecTemplatesApplied.rend(),
		[ullOffset](const std::unique_ptr<HEXTEMPLATEAPPLIED>& refTempl) {
			return ullOffset >= refTempl->ullOffset && ullOffset < refTempl->ullOffset + refTempl->pTemplate->iSizeTotal; });
		rIter != m_vecTemplatesApplied.rend()) {
		RemoveNodeWithAppliedID(rIter->get()->iAppliedID);
		m_vecTemplatesApplied.erase(std::next(rIter).base());
		if (m_pHexCtrl->IsDataSet()) {
			m_pHexCtrl->Redraw();
		}
	}
}

bool CHexDlgTemplMgr::IsHighlight()const
{
	return m_fHighlightSel;
}

bool CHexDlgTemplMgr::HasApplied()const
{
	return !m_vecTemplatesApplied.empty();
}

bool CHexDlgTemplMgr::HasTemplates()const
{
	return !m_vecTemplates.empty();
}

auto CHexDlgTemplMgr::HitTest(ULONGLONG ullOffset)const->PHEXTEMPLATEFIELD
{
	if (const auto itTemplApplied = std::find_if(m_vecTemplatesApplied.rbegin(), m_vecTemplatesApplied.rend(),
		[ullOffset](const std::unique_ptr<HEXTEMPLATEAPPLIED>& refTempl) {
			return ullOffset >= refTempl->ullOffset && ullOffset < refTempl->ullOffset + refTempl->pTemplate->iSizeTotal; });
		itTemplApplied != m_vecTemplatesApplied.rend()) {
		const auto pTemplApplied = itTemplApplied->get();
		const auto ullTemplStartOffset = pTemplApplied->ullOffset;
		auto& refVec = pTemplApplied->pTemplate->vecFields;
		auto ullOffsetCurr = pTemplApplied->ullOffset;

		const auto lmbFind = [ullTemplStartOffset, ullOffset, &ullOffsetCurr]
		(VecFields& vecRef)->PHEXTEMPLATEFIELD {
			const auto _lmbFind = [ullTemplStartOffset, ullOffset, &ullOffsetCurr]
			(const auto& lmbSelf, VecFields& vecRef)->PHEXTEMPLATEFIELD {
				for (auto& refField : vecRef) {
					if (!refField->vecNested.empty()) {
						if (const auto pRet = lmbSelf(lmbSelf, refField->vecNested); pRet != nullptr) {
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
			const auto pApplied = reinterpret_cast<PHEXTEMPLATEAPPLIED>(m_stTreeApplied.GetItemData(hItem));
			if (pApplied->pTemplate->iTemplateID == iTemplateID) {
				vecToRemove.emplace_back(hItem);
			}

			if (m_pAppliedCurr == pApplied) {
				m_pListApplied->SetItemCountEx(0);
				m_pListApplied->RedrawWindow();
				m_pAppliedCurr = nullptr;
				m_pVecCurrFields = nullptr;
				m_hTreeCurrParent = nullptr;
				UpdateStaticText();
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
	auto hItem = m_stTreeApplied.GetRootItem();
	while (hItem != nullptr) {
		const auto pApplied = reinterpret_cast<PHEXTEMPLATEAPPLIED>(m_stTreeApplied.GetItemData(hItem));
		if (pApplied->iAppliedID == iAppliedID) {
			if (m_pAppliedCurr == pApplied) {
				m_pListApplied->SetItemCountEx(0);
				m_pListApplied->RedrawWindow();
				m_pAppliedCurr = nullptr;
				m_pVecCurrFields = nullptr;
				m_hTreeCurrParent = nullptr;
				UpdateStaticText();
			}
			m_stTreeApplied.DeleteItem(hItem);
			break;
		}
		hItem = m_stTreeApplied.GetNextItem(hItem, TVGN_NEXT); //Get next Root sibling item.
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

void CHexDlgTemplMgr::SetHexSelByField(PHEXTEMPLATEFIELD pField)
{
	if (!IsHighlight() || !m_pHexCtrl->IsDataSet() || pField == nullptr || m_pAppliedCurr == nullptr)
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

void CHexDlgTemplMgr::UpdateStaticText()
{
	std::wstring wstrOffset { };
	std::wstring wstrSize { };

	if (m_pAppliedCurr != nullptr) { //If m_pAppliedCurr == nullptr set empty text.
		wstrOffset = std::vformat(m_fShowAsHex ? L"0x{:X}" : L"{}", std::make_wformat_args(m_pAppliedCurr->ullOffset));
		wstrSize = std::vformat(m_fShowAsHex ? L"0x{:X}" : L"{}", std::make_wformat_args(m_pAppliedCurr->pTemplate->iSizeTotal));
	}

	m_stStaticOffset.SetWindowTextW(wstrOffset.data());
	m_stStaticSize.SetWindowTextW(wstrSize.data());
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