/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "../../res/HexCtrlRes.h"
#include "../HexUtility.h"
#include "CHexDlgTemplMgr.h"
#include "strsafe.h"
#include <algorithm>
#include <format>
#include <fstream>
#include <numeric>
#include <optional>
#include <random>

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

namespace HEXCTRL::INTERNAL
{
	enum class CHexDlgTemplMgr::EMenuID : std::uint16_t {
		IDM_TREEAPPLIED_DISAPPLY = 0x8000, IDM_TREEAPPLIED_DISAPPLYALL = 0x8001,
		IDM_LISTAPPLIED_HDR_TYPE = 0x8100, IDM_LISTAPPLIED_HDR_NAME, IDM_LISTAPPLIED_HDR_OFFSET,
		IDM_LISTAPPLIED_HDR_SIZE, IDM_LISTAPPLIED_HDR_DATA, IDM_LISTAPPLIED_HDR_ENDIANNESS, IDM_LISTAPPLIED_HDR_COLORS
	};

	struct CHexDlgTemplMgr::FIELDSDEFPROPS { //Helper struct for convenient argument passing through recursive fields' parsing.
		COLORREF          clrBk { };
		COLORREF          clrText { };
		PHEXTEMPLATE      pTemplate { }; //Same for all fields.
		PHEXTEMPLATEFIELD pFieldParent { };
		bool              fBigEndian { false };
	};
};

BEGIN_MESSAGE_MAP(CHexDlgTemplMgr, CDialogEx)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_BTN_LOAD, &CHexDlgTemplMgr::OnBnLoadTemplate)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_BTN_UNLOAD, &CHexDlgTemplMgr::OnBnUnloadTemplate)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_BTN_RNDCLR, &CHexDlgTemplMgr::OnBnRandomizeColors)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_BTN_APPLY, &CHexDlgTemplMgr::OnBnApply)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_CHK_TTSHOW, &CHexDlgTemplMgr::OnCheckShowTt)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_CHK_HGLSEL, &CHexDlgTemplMgr::OnCheckHglSel)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_CHK_HEX, &CHexDlgTemplMgr::OnCheckHex)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_CHK_SWAP, &CHexDlgTemplMgr::OnCheckSwapEndian)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_CHK_MINMAX, &CHexDlgTemplMgr::OnCheckMinMax)
	ON_NOTIFY(LVN_GETDISPINFOW, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListGetDispInfo)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListItemChanged)
	ON_NOTIFY(LISTEX::LISTEX_MSG_EDITBEGIN, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListEditBegin)
	ON_NOTIFY(LISTEX::LISTEX_MSG_GETCOLOR, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListGetColor)
	ON_NOTIFY(LISTEX::LISTEX_MSG_HDRRBTNUP, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListHdrRClick)
	ON_NOTIFY(LISTEX::LISTEX_MSG_SETDATA, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListSetData)
	ON_NOTIFY(NM_RCLICK, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListRClick)
	ON_NOTIFY(NM_DBLCLK, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListDblClick)
	ON_NOTIFY(NM_RETURN, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListEnterPressed)
	ON_NOTIFY(NM_RCLICK, IDC_HEXCTRL_TEMPLMGR_TREE_APPLIED, &CHexDlgTemplMgr::OnTreeRClick)
	ON_NOTIFY(TVN_GETDISPINFOW, IDC_HEXCTRL_TEMPLMGR_TREE_APPLIED, &CHexDlgTemplMgr::OnTreeGetDispInfo)
	ON_NOTIFY(TVN_SELCHANGEDW, IDC_HEXCTRL_TEMPLMGR_TREE_APPLIED, &CHexDlgTemplMgr::OnTreeItemChanged)
	ON_WM_ACTIVATE()
	ON_WM_CTLCOLOR()
	ON_WM_DESTROY()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_NCHITTEST()
END_MESSAGE_MAP()


void CHexDlgTemplMgr::ApplyCurr(ULONGLONG ullOffset)
{
	if (!IsWindow(m_hWnd))
		return;

	const auto iIndex = m_comboTemplates.GetCurSel();
	if (iIndex == CB_ERR)
		return;

	const auto iTemplateID = static_cast<int>(m_comboTemplates.GetItemData(iIndex));
	ApplyTemplate(ullOffset, iTemplateID);
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

void CHexDlgTemplMgr::DisapplyAll()
{
	if (IsWindow(m_hWnd)) { //Dialog must be created and alive to work with its members.
		m_stTreeApplied.DeleteAllItems();
		m_pListApplied->SetItemCountEx(0);
		UpdateStaticText();
	}

	m_pAppliedCurr = nullptr;
	m_pVecFieldsCurr = nullptr;
	m_hTreeCurrParent = nullptr;
	m_vecTemplatesApplied.clear();

	if (m_pHexCtrl->IsDataSet()) {
		m_pHexCtrl->Redraw();
	}
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

auto CHexDlgTemplMgr::GetDlgData()const->std::uint64_t
{
	if (!IsWindow(m_hWnd)) {
		return { };
	}

	std::uint64_t ullData { };

	if (IsMinimized()) {
		ullData |= HEXCTRL_FLAG_TEMPLMGR_MINIMIZED;
	}

	if (IsShowAsHex()) {
		ullData |= HEXCTRL_FLAG_TEMPLMGR_HEXNUM;
	}

	if (IsTooltips()) {
		ullData |= HEXCTRL_FLAG_TEMPLMGR_SHOWTT;
	}

	if (IsHglSel()) {
		ullData |= HEXCTRL_FLAG_TEMPLMGR_HGLSEL;
	}

	if (IsSwapEndian()) {
		ullData |= HEXCTRL_FLAG_TEMPLMGR_SWAPENDIAN;
	}

	return ullData;
}

bool CHexDlgTemplMgr::HasApplied()const
{
	return !m_vecTemplatesApplied.empty();
}

bool CHexDlgTemplMgr::HasCurrent()const
{
	return IsWindow(m_hWnd) && HasTemplates();
}

bool CHexDlgTemplMgr::HasTemplates()const
{
	return !m_vecTemplates.empty();
}

auto CHexDlgTemplMgr::HitTest(ULONGLONG ullOffset)const->PHEXTEMPLATEFIELD
{
	const auto iterApplied = std::find_if(m_vecTemplatesApplied.rbegin(), m_vecTemplatesApplied.rend(),
		[ullOffset](const std::unique_ptr<HEXTEMPLATEAPPLIED>& refTempl) {
			return ullOffset >= refTempl->ullOffset && ullOffset < refTempl->ullOffset + refTempl->pTemplate->iSizeTotal; });
	if (iterApplied == m_vecTemplatesApplied.rend()) {
		return nullptr;
	}

	const auto pApplied = iterApplied->get();
	const auto ullOffsetApplied = pApplied->ullOffset;
	const auto& vecFields = pApplied->pTemplate->vecFields;

	const auto lmbFind = [ullOffset, ullOffsetApplied]
	(const VecFields& vecFields)->PHEXTEMPLATEFIELD {
		const auto _lmbFind = [ullOffset, ullOffsetApplied]
		(const auto& lmbSelf, const VecFields& vecFields)->PHEXTEMPLATEFIELD {
			for (const auto& refField : vecFields) {
				if (refField->vecNested.empty()) {
					const auto ullOffsetCurr = ullOffsetApplied + refField->iOffset;
					if (ullOffset < (ullOffsetCurr + refField->iSize)) {
						return refField.get();
					}
				}
				else {
					if (const auto pField = lmbSelf(lmbSelf, refField->vecNested); pField != nullptr) {
						return pField; //Return only if we Hit a pointer in the inner lambda, continue the loop otherwise.
					}
				}
			}
			return nullptr;
		};
		return _lmbFind(_lmbFind, vecFields);
	};

	return lmbFind(vecFields);
}

void CHexDlgTemplMgr::Initialize(IHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	if (pHexCtrl == nullptr)
		return;

	m_pHexCtrl = pHexCtrl;
}

bool CHexDlgTemplMgr::IsTooltips()const
{
	return m_fTooltips;
}

int CHexDlgTemplMgr::LoadTemplate(const wchar_t* pFilePath)
{
	auto pTemplateUnPtr = LoadFromFile(pFilePath);
	if (pTemplateUnPtr == nullptr) {
		return 0;
	}

	const auto pTemplate = pTemplateUnPtr.get();
	auto& refVecFields = pTemplate->vecFields;

	auto iTemplateID = 1; //TemplateID starts at 1.
	if (const auto iter = std::max_element(m_vecTemplates.begin(), m_vecTemplates.end(),
		[](const std::unique_ptr<HEXTEMPLATE>& ref1, const std::unique_ptr<HEXTEMPLATE>& ref2) {
			return ref1->iTemplateID < ref2->iTemplateID; }); iter != m_vecTemplates.end()) {
		iTemplateID = iter->get()->iTemplateID + 1; //Increasing next Template's ID by 1.
	}

	pTemplate->iTemplateID = iTemplateID;
	pTemplate->iSizeTotal = std::accumulate(refVecFields.begin(), refVecFields.end(), 0,
		[](auto iTotal, const std::unique_ptr<HEXTEMPLATEFIELD>& refField) { return iTotal + refField->iSize; });

	m_vecTemplates.emplace_back(std::move(pTemplateUnPtr));

	OnTemplateLoadUnload(iTemplateID, true);

	return iTemplateID;
}

void CHexDlgTemplMgr::UpdateData()
{
	if (::IsWindowVisible(m_hWnd)) {
		m_pListApplied->RedrawWindow();
	}
}

auto CHexDlgTemplMgr::SetDlgData(std::uint64_t ullData) -> HWND
{
	if (!IsWindow(m_hWnd)) {
		Create(IDD_HEXCTRL_TEMPLMGR, CWnd::FromHandle(m_pHexCtrl->GetWindowHandle(EHexWnd::WND_MAIN)));
	}

	if ((ullData & HEXCTRL_FLAG_TEMPLMGR_MINIMIZED) > 0 != IsMinimized()) {
		m_btnMinMax.SetCheck(m_btnMinMax.GetCheck() == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED);
		OnCheckMinMax();
	}

	if ((ullData & HEXCTRL_FLAG_TEMPLMGR_HEXNUM) > 0 != IsShowAsHex()) {
		m_btnHex.SetCheck(m_btnHex.GetCheck() == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED);
		OnCheckHex();
	}

	if ((ullData & HEXCTRL_FLAG_TEMPLMGR_SHOWTT) > 0 != IsTooltips()) {
		m_btnShowTT.SetCheck(m_btnShowTT.GetCheck() == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED);
		OnCheckShowTt();
	}

	if ((ullData & HEXCTRL_FLAG_TEMPLMGR_HGLSEL) > 0 != IsHglSel()) {
		m_btnHglSel.SetCheck(m_btnHglSel.GetCheck() == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED);
		OnCheckHglSel();
	}

	if ((ullData & HEXCTRL_FLAG_TEMPLMGR_SWAPENDIAN) > 0 != IsSwapEndian()) {
		m_btnSwapEndian.SetCheck(m_btnSwapEndian.GetCheck() == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED);
		OnCheckSwapEndian();
	}

	return m_hWnd;
}

void CHexDlgTemplMgr::ShowTooltips(bool fShow)
{
	m_fTooltips = fShow;
}

BOOL CHexDlgTemplMgr::ShowWindow(int nCmdShow)
{
	if (!IsWindow(m_hWnd)) {
		Create(IDD_HEXCTRL_TEMPLMGR, CWnd::FromHandle(m_pHexCtrl->GetWindowHandle(EHexWnd::WND_MAIN)));
	}

	return CDialogEx::ShowWindow(nCmdShow);
}

void CHexDlgTemplMgr::UnloadAll()
{
	DisapplyAll();

	for (const auto& refTempl : m_vecTemplates) {
		OnTemplateLoadUnload(refTempl->iTemplateID, false);
	}
	m_vecTemplates.clear();

	if (m_pHexCtrl->IsDataSet()) {
		m_pHexCtrl->Redraw();
	}
}


//Private methods.

void CHexDlgTemplMgr::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_COMBO_TEMPLATES, m_comboTemplates);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_EDIT_OFFSET, m_editOffset);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_TREE_APPLIED, m_stTreeApplied);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_CHK_MINMAX, m_btnMinMax);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_CHK_TTSHOW, m_btnShowTT);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_CHK_HGLSEL, m_btnHglSel);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_CHK_HEX, m_btnHex);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_CHK_SWAP, m_btnSwapEndian);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_STATIC_OFFSETNUM, m_wndStaticOffset);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_STATIC_SIZENUM, m_wndStaticSize);
}

void CHexDlgTemplMgr::EnableDynamicLayoutHelper(bool fEnable)
{
	if (fEnable && IsDynamicLayoutEnabled())
		return;

	EnableDynamicLayout(fEnable ? TRUE : FALSE);

	if (fEnable) {
		const auto pLayout = GetDynamicLayout();
		pLayout->Create(this);

		//This is needed for cases when dialog size is so small that tree/list aren't visible.
		//In such cases when OnCheckMinMax is hit the tree/list are handled/sized wrong.
		pLayout->SetMinSize({ 0, m_iDynLayoutMinY });

		pLayout->AddItem(IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, CMFCDynamicLayout::MoveNone(),
			CMFCDynamicLayout::SizeHorizontalAndVertical(100, 100));
		pLayout->AddItem(IDC_HEXCTRL_TEMPLMGR_TREE_APPLIED, CMFCDynamicLayout::MoveNone(),
			CMFCDynamicLayout::SizeVertical(100));
		pLayout->AddItem(IDC_HEXCTRL_TEMPLMGR_GRB_TOP, CMFCDynamicLayout::MoveNone(),
			CMFCDynamicLayout::SizeHorizontal(100));
		pLayout->AddItem(IDC_HEXCTRL_TEMPLMGR_CHK_MINMAX, CMFCDynamicLayout::MoveHorizontal(100),
			CMFCDynamicLayout::SizeNone());
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

bool CHexDlgTemplMgr::IsHglSel()const
{
	return m_fHglSel;
}

bool CHexDlgTemplMgr::IsMinimized()const
{
	return m_btnMinMax.GetCheck() == BST_CHECKED;
}

bool CHexDlgTemplMgr::IsShowAsHex()const
{
	return m_fShowAsHex;
}

bool CHexDlgTemplMgr::IsSwapEndian()const
{
	return m_fSwapEndian;
}

void CHexDlgTemplMgr::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if (nState != WA_INACTIVE) {
		if (m_pHexCtrl->IsCreated()) {
			const auto [dwFormat, wchSepar] = m_pHexCtrl->GetDateInfo();
			m_dwDateFormat = dwFormat;
			m_wchDateSepar = wchSepar;
		}
	}

	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
}

void CHexDlgTemplMgr::OnBnLoadTemplate()
{
	CFileDialog fd(TRUE, nullptr, nullptr,
		OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_ALLOWMULTISELECT | OFN_DONTADDTORECENT | OFN_ENABLESIZING
		| OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, L"Template files (*.json)|*.json|All files (*.*)|*.*||");

	if (fd.DoModal() != IDOK)
		return;

	const CComPtr<IFileOpenDialog> pIFOD = fd.GetIFileOpenDialog();
	CComPtr<IShellItemArray> pResults;
	pIFOD->GetResults(&pResults);

	DWORD dwCount { };
	pResults->GetCount(&dwCount);
	for (auto iterFiles { 0U }; iterFiles < dwCount; ++iterFiles) {
		CComPtr<IShellItem> pItem;
		pResults->GetItemAt(iterFiles, &pItem);
		CComHeapPtr<wchar_t> pwstrPath;
		pItem->GetDisplayName(SIGDN_FILESYSPATH, &pwstrPath);
		if (LoadTemplate(pwstrPath) == 0) {
			std::wstring wstrErr = L"Error loading a template:\n";
			wstrErr += pwstrPath;
			std::wstring_view wsvFileName = pwstrPath.m_pData;
			wsvFileName = wsvFileName.substr(wsvFileName.find_last_of('\\') + 1);
			MessageBoxW(wstrErr.data(), wsvFileName.data(), MB_ICONERROR);
		}
	}
}

void CHexDlgTemplMgr::OnBnUnloadTemplate()
{
	if (const auto iIndex = m_comboTemplates.GetCurSel(); iIndex != CB_ERR) {
		const auto iTemplateID = static_cast<int>(m_comboTemplates.GetItemData(iIndex));
		UnloadTemplate(iTemplateID);
	}
}

void CHexDlgTemplMgr::OnBnRandomizeColors()
{
	if (const auto iIndex = m_comboTemplates.GetCurSel(); iIndex != CB_ERR) {
		const auto iTemplateID = static_cast<int>(m_comboTemplates.GetItemData(iIndex));
		RandomizeTemplateColors(iTemplateID);
	}
}

void CHexDlgTemplMgr::OnBnApply()
{
	CString wstrText;
	m_editOffset.GetWindowTextW(wstrText);
	if (wstrText.IsEmpty())
		return;

	const auto opt = stn::StrToULL(wstrText.GetString());
	if (!opt)
		return;

	const auto iIndex = m_comboTemplates.GetCurSel();
	if (iIndex == CB_ERR)
		return;

	const auto iTemplateID = static_cast<int>(m_comboTemplates.GetItemData(iIndex));
	ApplyTemplate(*opt, iTemplateID);
}

void CHexDlgTemplMgr::OnCheckHex()
{
	m_fShowAsHex = m_btnHex.GetCheck() == BST_CHECKED;
	UpdateStaticText();
	m_pListApplied->RedrawWindow();
}

void CHexDlgTemplMgr::OnCheckSwapEndian()
{
	m_fSwapEndian = m_btnSwapEndian.GetCheck() == BST_CHECKED;
	m_pListApplied->RedrawWindow();
}

void CHexDlgTemplMgr::OnCheckShowTt()
{
	ShowTooltips(m_btnShowTT.GetCheck() == BST_CHECKED);
}

void CHexDlgTemplMgr::OnCheckHglSel()
{
	m_fHglSel = m_btnHglSel.GetCheck() == BST_CHECKED;
}

void CHexDlgTemplMgr::OnCheckMinMax()
{
	const auto fMinimize = IsMinimized();
	static constexpr int iIDsToHide[] { IDC_HEXCTRL_TEMPLMGR_STATIC_AVAIL, IDC_HEXCTRL_TEMPLMGR_COMBO_TEMPLATES,
		IDC_HEXCTRL_TEMPLMGR_BTN_LOAD, IDC_HEXCTRL_TEMPLMGR_BTN_UNLOAD, IDC_HEXCTRL_TEMPLMGR_BTN_RNDCLR,
		IDC_HEXCTRL_TEMPLMGR_STATIC_APPLY, IDC_HEXCTRL_TEMPLMGR_EDIT_OFFSET, IDC_HEXCTRL_TEMPLMGR_BTN_APPLY,
		IDC_HEXCTRL_TEMPLMGR_CHK_TTSHOW, IDC_HEXCTRL_TEMPLMGR_CHK_HGLSEL, IDC_HEXCTRL_TEMPLMGR_CHK_HEX,
		IDC_HEXCTRL_TEMPLMGR_CHK_SWAP, IDC_HEXCTRL_TEMPLMGR_GRB_TOP };
	for (const auto id : iIDsToHide) {
		GetDlgItem(id)->ShowWindow(fMinimize ? SW_HIDE : SW_SHOW);
	}

	static constexpr int iIDsToMove[] { IDC_HEXCTRL_TEMPLMGR_STATIC_OFFSETTXT, IDC_HEXCTRL_TEMPLMGR_STATIC_OFFSETNUM,
		IDC_HEXCTRL_TEMPLMGR_STATIC_SIZETXT, IDC_HEXCTRL_TEMPLMGR_STATIC_SIZENUM };

	CRect rcGRB; //Top Group Box rect.
	GetDlgItem(IDC_HEXCTRL_TEMPLMGR_GRB_TOP)->GetClientRect(rcGRB);
	const auto iHeightGRB = rcGRB.Height();
	for (const auto id : iIDsToMove) {
		const auto pWnd = GetDlgItem(id);
		CRect rcWnd;
		pWnd->GetWindowRect(rcWnd);
		ScreenToClient(rcWnd);
		const auto iNewPosY = fMinimize ? rcWnd.top - iHeightGRB : rcWnd.top + iHeightGRB;
		pWnd->SetWindowPos(nullptr, rcWnd.left, iNewPosY, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
	}

	static constexpr int iIDsToResize[] { IDC_HEXCTRL_TEMPLMGR_TREE_APPLIED, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED };
	EnableDynamicLayoutHelper(false); //Otherwise DynamicLayout won't know that all dynamic windows have changed.
	for (const auto id : iIDsToResize) {
		const auto pWnd = GetDlgItem(id);
		CRect rcWnd;
		pWnd->GetWindowRect(rcWnd);
		rcWnd.top += fMinimize ? -iHeightGRB : iHeightGRB;
		ScreenToClient(rcWnd);
		pWnd->SetWindowPos(nullptr, rcWnd.left, rcWnd.top, rcWnd.Width(), rcWnd.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
	}
	EnableDynamicLayoutHelper(true);

	m_btnMinMax.SetBitmap(fMinimize ? m_hBITMAPMax : m_hBITMAPMin); //Set arrow bitmap to the min-max checkbox.
}

BOOL CHexDlgTemplMgr::OnCommand(WPARAM wParam, LPARAM lParam)
{
	using enum EMenuID;
	const auto wMenuID = LOWORD(wParam);
	switch (static_cast<EMenuID>(wMenuID)) {
	case IDM_TREEAPPLIED_DISAPPLY:
		if (const auto pApplied = GetAppliedFromItem(m_stTreeApplied.GetSelectedItem()); pApplied != nullptr) {
			DisapplyByID(pApplied->iAppliedID);
		}
		return TRUE;
	case IDM_TREEAPPLIED_DISAPPLYALL:
		DisapplyAll();
		return TRUE;
	case IDM_LISTAPPLIED_HDR_TYPE:
	case IDM_LISTAPPLIED_HDR_NAME:
	case IDM_LISTAPPLIED_HDR_OFFSET:
	case IDM_LISTAPPLIED_HDR_SIZE:
	case IDM_LISTAPPLIED_HDR_DATA:
	case IDM_LISTAPPLIED_HDR_ENDIANNESS:
	case IDM_LISTAPPLIED_HDR_COLORS:
	{
		const auto fChecked = m_stMenuHdr.GetMenuState(wMenuID, MF_BYCOMMAND) & MF_CHECKED;
		m_pListApplied->HideColumn(wMenuID - static_cast<int>(IDM_LISTAPPLIED_HDR_TYPE), fChecked);
		m_stMenuHdr.CheckMenuItem(wMenuID, (fChecked ? MF_UNCHECKED : MF_CHECKED) | MF_BYCOMMAND);
	}
	return TRUE;
	default:
		break;
	}

	return CDialogEx::OnCommand(wParam, lParam);
}

HBRUSH CHexDlgTemplMgr::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	const auto hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	//Template's applied offset and total size static text color.
	if (pWnd == &m_wndStaticOffset || pWnd == &m_wndStaticSize) {
		pDC->SetTextColor(RGB(0, 0, 150));
	}

	return hbr;
}

void CHexDlgTemplMgr::OnDestroy()
{
	CDialogEx::OnDestroy();

	m_stMenuTree.DestroyMenu();
	m_stMenuHdr.DestroyMenu();
	m_pAppliedCurr = nullptr;
	m_pVecFieldsCurr = nullptr;
	m_hTreeCurrParent = nullptr;
	DeleteObject(m_hBITMAPMin);
	DeleteObject(m_hBITMAPMax);
}

BOOL CHexDlgTemplMgr::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_pListApplied->CreateDialogCtrl(IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, this);
	m_pListApplied->SetExtendedStyle(LVS_EX_HEADERDRAGDROP);
	m_pListApplied->InsertColumn(m_iIDListApplFieldType, L"Type", LVCFMT_LEFT, 85);
	m_pListApplied->InsertColumn(1, L"Name", LVCFMT_LEFT, 200);
	m_pListApplied->InsertColumn(2, L"Offset", LVCFMT_LEFT, 50);
	m_pListApplied->InsertColumn(3, L"Size", LVCFMT_LEFT, 50);
	m_pListApplied->InsertColumn(m_iIDListApplFieldData, L"Data", LVCFMT_LEFT, 120, -1, LVCFMT_LEFT, true);
	m_pListApplied->InsertColumn(5, L"Endianness", LVCFMT_CENTER, 65, -1, LVCFMT_CENTER);
	m_pListApplied->InsertColumn(m_iIDListApplFieldDescr, L"Description", LVCFMT_LEFT, 100, -1, LVCFMT_LEFT, true);
	m_pListApplied->InsertColumn(m_iIDListApplFieldClrs, L"Colors", LVCFMT_LEFT, 57);

	using enum EMenuID;
	m_stMenuHdr.CreatePopupMenu();
	m_stMenuHdr.AppendMenuW(MF_STRING, static_cast<int>(IDM_LISTAPPLIED_HDR_TYPE), L"Type");
	m_stMenuHdr.CheckMenuItem(static_cast<int>(IDM_LISTAPPLIED_HDR_TYPE), MF_CHECKED | MF_BYCOMMAND);
	m_stMenuHdr.AppendMenuW(MF_STRING, static_cast<int>(IDM_LISTAPPLIED_HDR_NAME), L"Name");
	m_stMenuHdr.CheckMenuItem(static_cast<int>(IDM_LISTAPPLIED_HDR_NAME), MF_CHECKED | MF_BYCOMMAND);
	m_stMenuHdr.AppendMenuW(MF_STRING, static_cast<int>(IDM_LISTAPPLIED_HDR_OFFSET), L"Offset");
	m_stMenuHdr.CheckMenuItem(static_cast<int>(IDM_LISTAPPLIED_HDR_OFFSET), MF_CHECKED | MF_BYCOMMAND);
	m_stMenuHdr.AppendMenuW(MF_STRING, static_cast<int>(IDM_LISTAPPLIED_HDR_SIZE), L"Size");
	m_stMenuHdr.CheckMenuItem(static_cast<int>(IDM_LISTAPPLIED_HDR_SIZE), MF_CHECKED | MF_BYCOMMAND);
	m_stMenuHdr.AppendMenuW(MF_STRING, static_cast<int>(IDM_LISTAPPLIED_HDR_DATA), L"Data");
	m_stMenuHdr.CheckMenuItem(static_cast<int>(IDM_LISTAPPLIED_HDR_DATA), MF_CHECKED | MF_BYCOMMAND);
	m_stMenuHdr.AppendMenuW(MF_STRING, static_cast<int>(IDM_LISTAPPLIED_HDR_ENDIANNESS), L"Endianness");
	m_stMenuHdr.CheckMenuItem(static_cast<int>(IDM_LISTAPPLIED_HDR_ENDIANNESS), MF_CHECKED | MF_BYCOMMAND);
	m_stMenuHdr.AppendMenuW(MF_STRING, static_cast<int>(IDM_LISTAPPLIED_HDR_COLORS), L"Colors");
	m_stMenuHdr.CheckMenuItem(static_cast<int>(IDM_LISTAPPLIED_HDR_COLORS), MF_CHECKED | MF_BYCOMMAND);

	m_stMenuTree.CreatePopupMenu();
	m_stMenuTree.AppendMenuW(MF_STRING, static_cast<UINT_PTR>(EMenuID::IDM_TREEAPPLIED_DISAPPLY), L"Disapply template");
	m_stMenuTree.AppendMenuW(MF_STRING, static_cast<UINT_PTR>(EMenuID::IDM_TREEAPPLIED_DISAPPLYALL), L"Disapply all");

	m_editOffset.SetWindowTextW(L"0x0");
	m_btnShowTT.SetCheck(IsTooltips() ? BST_CHECKED : BST_UNCHECKED);
	m_btnHglSel.SetCheck(IsHglSel() ? BST_CHECKED : BST_UNCHECKED);
	m_btnHex.SetCheck(IsShowAsHex() ? BST_CHECKED : BST_UNCHECKED);

	m_hCurResize = static_cast<HCURSOR>(LoadImageW(nullptr, IDC_SIZEWE, IMAGE_CURSOR, 0, 0, LR_SHARED));
	m_hCurArrow = static_cast<HCURSOR>(LoadImageW(nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED));

	CRect rcTree;
	m_stTreeApplied.GetWindowRect(rcTree);
	ScreenToClient(rcTree);
	m_iDynLayoutMinY = rcTree.top + 5;
	EnableDynamicLayoutHelper(true);
	::SetWindowSubclass(m_stTreeApplied, TreeSubclassProc, 0, 0);
	SetDlgButtonsState();

	for (const auto& pTempl : m_vecTemplates) {
		OnTemplateLoadUnload(pTempl->iTemplateID, true);
	}

	CRect rcWnd;
	m_btnMinMax.GetWindowRect(rcWnd);
	m_hBITMAPMin = static_cast<HBITMAP>(LoadImageW(AfxGetInstanceHandle(),
		MAKEINTRESOURCEW(IDB_HEXCTRL_SCROLL_ARROW), IMAGE_BITMAP, rcWnd.Width(), rcWnd.Width(), 0));

	//Flipping m_hBITMAPMin bits vertically and creating m_hBITMAPMax bitmap.
	BITMAP stBMP { };
	GetObjectW(m_hBITMAPMin, sizeof(BITMAP), &stBMP); //stBMP.bmBits is nullptr here.
	const auto dwWidth = static_cast<DWORD>(stBMP.bmWidth);
	const auto dwHeight = static_cast<DWORD>(stBMP.bmHeight);
	const auto dwPixels = dwWidth * dwHeight;
	const auto dwBytesBmp = stBMP.bmWidthBytes * stBMP.bmHeight;
	const auto pPixelsOrig = std::make_unique<COLORREF[]>(dwPixels);
	GetBitmapBits(m_hBITMAPMin, dwBytesBmp, pPixelsOrig.get());
	for (auto itWidth = 0UL; itWidth < dwWidth; ++itWidth) { //Flip matrix' columns (flip vert).
		for (auto itHeight = 0UL, itHeightBack = dwHeight - 1; itHeight < itHeightBack; ++itHeight, --itHeightBack) {
			std::swap(pPixelsOrig[itHeight * dwHeight + itWidth], pPixelsOrig[itHeightBack * dwWidth + itWidth]);
		}
	}
	m_hBITMAPMax = CreateBitmapIndirect(&stBMP);
	SetBitmapBits(m_hBITMAPMax, dwBytesBmp, pPixelsOrig.get());

	m_btnMinMax.SetBitmap(m_hBITMAPMin); //Set the min arrow bitmap to the min-max checkbox.

	return TRUE;
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

void CHexDlgTemplMgr::OnListDblClick(NMHDR* pNMHDR, LRESULT* /*pResult*/)
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
	m_stTreeApplied.Expand(hItem, TVE_EXPAND);

	m_pListApplied->SetItemState(-1, 0, LVIS_SELECTED | LVIS_FOCUSED); //Deselect all items.
	m_pListApplied->SetItemCountEx(static_cast<int>(m_pVecFieldsCurr->size()));
	m_pListApplied->RedrawWindow();
	m_fListGuardEvent = false;
}

void CHexDlgTemplMgr::OnListEditBegin(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pLDI = reinterpret_cast<LISTEX::PLISTEXDATAINFO>(pNMHDR);
	const auto& pField = (*m_pVecFieldsCurr)[pLDI->iItem];

	if (!pField->vecNested.empty() || (pField->eType == EHexFieldType::custom_size
		&& pField->iSize != 1 && pField->iSize != 2 && pField->iSize != 4 && pField->iSize != 8)) {
		pLDI->fAllowEdit = false; //Do not show edit-box if clicked on nested fields.
	}
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

void CHexDlgTemplMgr::OnListGetDispInfo(NMHDR* pNMHDR, LRESULT* /*pResult*/)
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
		{ type_bool, L"bool" }, { type_char, L"char" }, { type_uchar, L"unsigned char" },
		{ type_short, L"short" }, { type_ushort, L"unsigned short" }, { type_int, L"int" },
		{ type_uint, L"unsigned int" }, { type_ll, L"long long" }, { type_ull, L"unsigned long long" },
		{ type_float, L"float" }, { type_double, L"double" }, { type_time32, L"time32_t" },
		{ type_time64, L"time64_t" }, { type_filetime, L"FILETIME" }, { type_systemtime, L"SYSTEMTIME" },
		{ type_guid, L"GUID" }
	};

	switch (pItem->iSubItem) {
	case m_iIDListApplFieldType: //Type.
		if (pField->eType == type_custom) {
			auto& refVecCT = pField->pTemplate->vecCustomType;
			if (const auto iter = std::find_if(refVecCT.begin(), refVecCT.end(),
				[uTypeID = pField->uTypeID](const HEXCUSTOMTYPE& ref) {
					return ref.uTypeID == uTypeID; }); iter != refVecCT.end()) {
				pItem->pszText = iter->wstrTypeName.data();
			}
			else {
				pItem->pszText = const_cast<LPWSTR>(umapETypeToWstr.at(pField->eType));
			}
		}
		else {
			pItem->pszText = const_cast<LPWSTR>(umapETypeToWstr.at(pField->eType));
		}
		break;
	case 1: //Name.
		pItem->pszText = pField->wstrName.data();
		break;
	case 2: //Offset.
		*std::vformat_to(pItem->pszText, wsvFmt, std::make_wformat_args(pField->iOffset)) = L'\0';
		break;
	case 3: //Size.
		*std::vformat_to(pItem->pszText, wsvFmt, std::make_wformat_args(pField->iSize)) = L'\0';
		break;
	case m_iIDListApplFieldData: //Data.
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
				*std::vformat_to(pItem->pszText, wsvFmt, std::make_wformat_args(GetIHexTData<BYTE>(*m_pHexCtrl, ullOffset))) = L'\0';
				break;
			case 2: {
				auto wData = GetIHexTData<WORD>(*m_pHexCtrl, ullOffset);
				if (fShouldSwap) {
					wData = ByteSwap(wData);
				}
				*std::vformat_to(pItem->pszText, wsvFmt, std::make_wformat_args(wData)) = L'\0';
			}
				  break;
			case 4: {
				auto dwData = GetIHexTData<DWORD>(*m_pHexCtrl, ullOffset);
				if (fShouldSwap) {
					dwData = ByteSwap(dwData);
				}
				*std::vformat_to(pItem->pszText, wsvFmt, std::make_wformat_args(dwData)) = L'\0';
			}
				  break;
			case 8: {
				auto ullData = GetIHexTData<QWORD>(*m_pHexCtrl, ullOffset);
				if (fShouldSwap) {
					ullData = ByteSwap(ullData);
				}
				*std::vformat_to(pItem->pszText, wsvFmt, std::make_wformat_args(ullData)) = L'\0';
			}
				  break;
			default:
				break;
			}
			break;
		case type_bool:
			ShowListDataBool(pItem->pszText, GetIHexTData<unsigned char>(*m_pHexCtrl, ullOffset));
			break;
		case type_char:
			ShowListDataChar(pItem->pszText, GetIHexTData<char>(*m_pHexCtrl, ullOffset));
			break;
		case type_uchar:
			ShowListDataUChar(pItem->pszText, GetIHexTData<unsigned char>(*m_pHexCtrl, ullOffset));
			break;
		case type_short:
			ShowListDataShort(pItem->pszText, GetIHexTData<short>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_ushort:
			ShowListDataUShort(pItem->pszText, GetIHexTData<unsigned short>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_int:
			ShowListDataInt(pItem->pszText, GetIHexTData<int>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_uint:
			ShowListDataUInt(pItem->pszText, GetIHexTData<unsigned int>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_ll:
			ShowListDataLL(pItem->pszText, GetIHexTData<long long>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_ull:
			ShowListDataULL(pItem->pszText, GetIHexTData<unsigned long long>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_float:
			ShowListDataFloat(pItem->pszText, GetIHexTData<float>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_double:
			ShowListDataDouble(pItem->pszText, GetIHexTData<double>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_time32:
			ShowListDataTime32(pItem->pszText, GetIHexTData<__time32_t>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_time64:
			ShowListDataTime64(pItem->pszText, GetIHexTData<__time64_t>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_filetime:
			ShowListDataFILETIME(pItem->pszText, GetIHexTData<FILETIME>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_systemtime:
			ShowListDataSYSTEMTIME(pItem->pszText, GetIHexTData<SYSTEMTIME>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_guid:
			ShowListDataGUID(pItem->pszText, GetIHexTData<GUID>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		default:
			break;
		}
	}
	break;
	case 5: //Endianness.
		*std::vformat_to(pItem->pszText, fShouldSwap ? L"big" : L"little",
			std::make_wformat_args()) = L'\0';
		break;
	case 6: //Description.
		*std::format_to(pItem->pszText, L"{}", pField->wstrDescr) = L'\0';
		break;
	case m_iIDListApplFieldClrs: //Colors.
		*std::format_to(pItem->pszText, L"#Text") = L'\0';
		break;
	default:
		break;
	}
}

void CHexDlgTemplMgr::OnListGetColor(NMHDR* pNMHDR, LRESULT* pResult)
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
	if (!pField->vecNested.empty() && pLCI->iSubItem != m_iIDListApplFieldClrs) {
		pLCI->stClr.clrBk = clrBkGreyish;
		if (pLCI->iSubItem == m_iIDListApplFieldType) {
			if (eType == type_custom) {
				if (pField->uTypeID > 0) {
					pLCI->stClr.clrText = clrTextGreenish;
				}
			}
			else if (eType != custom_size) {
				pLCI->stClr.clrText = clrTextBluish;
			}
		}
		*pResult = TRUE;

		return;
	}

	switch (pLCI->iSubItem) {
	case m_iIDListApplFieldType:
		if (eType != type_custom && eType != custom_size) {
			pLCI->stClr.clrText = clrTextBluish;
			pLCI->stClr.clrBk = static_cast<COLORREF>(-1); //Default bk color.
			*pResult = TRUE;
			return;
		}
		break;
	case m_iIDListApplFieldClrs:
		pLCI->stClr.clrBk = pField->clrBk;
		pLCI->stClr.clrText = pField->clrText;
		*pResult = TRUE;
		return;
	default:
		break;
	}
}

void CHexDlgTemplMgr::OnListHdrRClick(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	CPoint pt;
	GetCursorPos(&pt);
	m_stMenuHdr.TrackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON, pt.x, pt.y, this);
}

void CHexDlgTemplMgr::OnListItemChanged(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	const auto iItem = pNMI->iItem;
	if (iItem < 0 || m_fListGuardEvent)
		return;

	m_stTreeApplied.SelectItem(TreeItemFromListItem(iItem));
}

void CHexDlgTemplMgr::OnListRClick(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
}

void CHexDlgTemplMgr::OnListSetData(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pLDI = reinterpret_cast<LISTEX::PLISTEXDATAINFO>(pNMHDR);
	const auto pwszText = pLDI->pwszData;
	const auto& pField = (*m_pVecFieldsCurr)[pLDI->iItem];

	if (pLDI->iSubItem == m_iIDListApplFieldData) { //Data.
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
				if (const auto opt = stn::StrToUChar(pwszText); opt) {
					SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
				}
				break;
			case 2:
				if (const auto opt = stn::StrToUShort(pwszText); opt) {
					SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
				}
				break;
			case 4:
				if (const auto opt = stn::StrToUInt(pwszText); opt) {
					SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
				}
				break;
			case 8:
				if (const auto opt = stn::StrToULL(pwszText); opt) {
					SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
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
		case type_char:
			fSetRet = SetDataChar(pwszText, ullOffset);
			break;
		case type_uchar:
			fSetRet = SetDataUChar(pwszText, ullOffset);
			break;
		case type_short:
			fSetRet = SetDataShort(pwszText, ullOffset, fShouldSwap);
			break;
		case type_ushort:
			fSetRet = SetDataUShort(pwszText, ullOffset, fShouldSwap);
			break;
		case type_int:
			fSetRet = SetDataInt(pwszText, ullOffset, fShouldSwap);
			break;
		case type_uint:
			fSetRet = SetDataUInt(pwszText, ullOffset, fShouldSwap);
			break;
		case type_ll:
			fSetRet = SetDataLL(pwszText, ullOffset, fShouldSwap);
			break;
		case type_ull:
			fSetRet = SetDataULL(pwszText, ullOffset, fShouldSwap);
			break;
		case type_float:
			fSetRet = SetDataFloat(pwszText, ullOffset, fShouldSwap);
			break;
		case type_double:
			fSetRet = SetDataDouble(pwszText, ullOffset, fShouldSwap);
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
			MessageBoxW(L"Incorrect input data.", L"Incorrect input", MB_ICONERROR);
			return;
		}
	}
	else if (pLDI->iSubItem == m_iIDListApplFieldDescr) { //Description.
		pField->wstrDescr = pwszText;
	}

	m_pHexCtrl->Redraw();
}

void CHexDlgTemplMgr::OnMouseMove(UINT nFlags, CPoint point)
{
	static constexpr auto iResAreaHalfWidth = 15;       //Area where cursor turns into resizable (IDC_SIZEWE).
	static constexpr auto iWidthBetweenTreeAndList = 1; //Width between tree and list after resizing.
	static constexpr auto iMinTreeWidth = 100;          //Tree control minimum width.

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
	else if (pWndFocus == &m_editOffset) { //Focus is on the "Offset" edit-box.
		OnBnApply();
	}
}

void CHexDlgTemplMgr::OnTemplateLoadUnload(int iTemplateID, bool fLoad)
{
	if (!IsWindow(m_hWnd)) //Only if dialog window is created and alive we proceed with its members.
		return;

	if (fLoad) {
		const auto iterTempl = std::find_if(m_vecTemplates.begin(), m_vecTemplates.end(),
			[iTemplateID](const std::unique_ptr<HEXTEMPLATE>& ref) { return ref->iTemplateID == iTemplateID; });
		if (iterTempl == m_vecTemplates.end())
			return;

		const auto iIndex = m_comboTemplates.AddString(iterTempl->get()->wstrName.data());
		m_comboTemplates.SetItemData(iIndex, static_cast<DWORD_PTR>(iTemplateID));
		m_comboTemplates.SetCurSel(iIndex);
		SetDlgButtonsState();
	}
	else {
		RemoveNodesWithTemplateID(iTemplateID);

		for (auto iIndex = 0; iIndex < m_comboTemplates.GetCount(); ++iIndex) { //Remove Template name from ComboBox.
			if (const auto iItemData = static_cast<int>(m_comboTemplates.GetItemData(iIndex)); iItemData == iTemplateID) {
				m_comboTemplates.DeleteString(iIndex);
				m_comboTemplates.SetCurSel(0);
				break;
			}
		}

		SetDlgButtonsState();
	}
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
			SetHexSelByField(reinterpret_cast<PCHEXTEMPLATEFIELD>(dwItemData));
		}
		return;
	}

	m_fListGuardEvent = true; //To not trigger OnListItemChanged on the way.
	bool fRootNodeClick { false };
	PHEXTEMPLATEFIELD pFieldCurr { };
	PVecFields pVecCurrFields { };

	if (m_stTreeApplied.GetParentItem(pItem->hItem) == nullptr) { //Root item.
		fRootNodeClick = true;
		const auto pApplied = reinterpret_cast<PHEXTEMPLATEAPPLIED>(m_stTreeApplied.GetItemData(pItem->hItem));
		pVecCurrFields = &pApplied->pTemplate->vecFields; //On Root item click, set pVecCurrFields to Template's main vecFields.
		m_hTreeCurrParent = pItem->hItem;
	}
	else { //Child items.
		pFieldCurr = reinterpret_cast<PHEXTEMPLATEFIELD>(m_stTreeApplied.GetItemData(pItem->hItem));
		if (pFieldCurr->pFieldParent == nullptr) {
			if (pFieldCurr->vecNested.empty()) { //On first level child items, set pVecCurrFields to Template's main vecFields.
				pVecCurrFields = &pFieldCurr->pTemplate->vecFields;
				m_hTreeCurrParent = m_stTreeApplied.GetParentItem(pItem->hItem);
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
				m_hTreeCurrParent = m_stTreeApplied.GetParentItem(pItem->hItem);
			}
			else {
				fRootNodeClick = true;
				pVecCurrFields = &pFieldCurr->vecNested;
				m_hTreeCurrParent = pItem->hItem;
			}
		}
	}

	m_pAppliedCurr = GetAppliedFromItem(pItem->hItem);
	m_pListApplied->SetItemState(-1, 0, LVIS_SELECTED | LVIS_FOCUSED); //Deselect all items.
	if (pVecCurrFields != m_pVecFieldsCurr) { //To not trigger SetItemCountEx (which is slow) if it's the same count.
		m_pVecFieldsCurr = pVecCurrFields;
		m_pListApplied->SetItemCountEx(static_cast<int>(m_pVecFieldsCurr->size()), LVSICF_NOSCROLL);
	}

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
	m_stMenuTree.EnableMenuItem(static_cast<UINT>(EMenuID::IDM_TREEAPPLIED_DISAPPLY),
		(fHasApplied && fHitTest ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
	m_stMenuTree.EnableMenuItem(static_cast<UINT>(EMenuID::IDM_TREEAPPLIED_DISAPPLYALL), (fHasApplied ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
	m_stMenuTree.TrackPopupMenuEx(TPM_LEFTALIGN, pt.x, pt.y, this, nullptr);
}

void CHexDlgTemplMgr::RandomizeTemplateColors(int iTemplateID)
{
	const auto iterTempl = std::find_if(m_vecTemplates.begin(), m_vecTemplates.end(),
		[iTemplateID](const std::unique_ptr<HEXTEMPLATE>& ref) { return ref->iTemplateID == iTemplateID; });
	if (iterTempl == m_vecTemplates.end())
		return;

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<unsigned int> distrib(50, 230);

	const auto lmbRndColors = [&distrib, &gen](const VecFields& vecRef) {
		const auto _lmbCount = [&distrib, &gen](const auto& lmbSelf, const VecFields& vecRef)->void {
			for (auto& refField : vecRef) {
				if (refField->vecNested.empty()) {
					refField->clrBk = RGB(distrib(gen), distrib(gen), distrib(gen));
				}
				else { lmbSelf(lmbSelf, refField->vecNested); }
			}
		};
		return _lmbCount(_lmbCount, vecRef);
	};

	lmbRndColors(iterTempl->get()->vecFields);
	m_pListApplied->RedrawWindow();
	m_pHexCtrl->Redraw();
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
				m_pVecFieldsCurr = nullptr;
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
				m_pVecFieldsCurr = nullptr;
				m_hTreeCurrParent = nullptr;
				UpdateStaticText();
			}
			m_stTreeApplied.DeleteItem(hItem);
			break;
		}
		hItem = m_stTreeApplied.GetNextItem(hItem, TVGN_NEXT); //Get next Root sibling item.
	}
}

bool CHexDlgTemplMgr::SetDataBool(LPCWSTR pwszText, ULONGLONG ullOffset) const
{
	if (IsShowAsHex()) {
		if (const auto opt = stn::StrToUChar(pwszText); opt) {
			SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
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

		SetIHexTData(*m_pHexCtrl, ullOffset, fToSet);
		return true;
	}
	return false;
}

bool CHexDlgTemplMgr::SetDataChar(LPCWSTR pwszText, ULONGLONG ullOffset)const
{
	//When IsShowAsHex() is true, we deliberately convert text to an 'unsigned' corresponding type.
	//Otherwise, setting a values like 0xFF in hex mode would be impossible, because such values
	//are bigger then `signed` type's max value, they can't be converted to it.
	if (IsShowAsHex()) {
		if (const auto opt = stn::StrToUChar(pwszText); opt) {
			SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
			return true;
		}
	}
	else {
		if (const auto opt = stn::StrToChar(pwszText); opt) {
			SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
			return true;
		}
	}
	return false;
}

bool CHexDlgTemplMgr::SetDataUChar(LPCWSTR pwszText, ULONGLONG ullOffset)const
{
	if (const auto opt = stn::StrToUChar(pwszText); opt) {
		SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
		return true;
	}
	return false;
}

bool CHexDlgTemplMgr::SetDataShort(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const
{
	if (IsShowAsHex()) {
		if (auto opt = stn::StrToUShort(pwszText); opt) {
			if (fShouldSwap) {
				*opt = ByteSwap(*opt);
			}
			SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
			return true;
		}
	}
	else {
		if (auto opt = stn::StrToShort(pwszText); opt) {
			if (fShouldSwap) {
				*opt = ByteSwap(*opt);
			}
			SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
			return true;
		}
	}
	return false;
}

bool CHexDlgTemplMgr::SetDataUShort(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const
{
	if (auto opt = stn::StrToUShort(pwszText); opt) {
		if (fShouldSwap) {
			*opt = ByteSwap(*opt);
		}
		SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
		return true;
	}
	return false;
}

bool CHexDlgTemplMgr::SetDataInt(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const
{
	if (IsShowAsHex()) {
		if (auto opt = stn::StrToUInt(pwszText); opt) {
			if (fShouldSwap) {
				*opt = ByteSwap(*opt);
			}
			SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
			return true;
		}
	}
	else {
		if (auto opt = stn::StrToNum<int>(pwszText); opt) {
			if (fShouldSwap) {
				*opt = ByteSwap(*opt);
			}
			SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
			return true;
		}
	}
	return false;
}

bool CHexDlgTemplMgr::SetDataUInt(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const
{
	if (auto opt = stn::StrToUInt(pwszText); opt) {
		if (fShouldSwap) {
			*opt = ByteSwap(*opt);
		}
		SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
		return true;
	}
	return false;
}

bool CHexDlgTemplMgr::SetDataLL(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const
{
	if (IsShowAsHex()) {
		if (auto opt = stn::StrToULL(pwszText); opt) {
			if (fShouldSwap) {
				*opt = ByteSwap(*opt);
			}
			SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
			return true;
		}
	}
	else {
		if (auto opt = stn::StrToLL(pwszText); opt) {
			if (fShouldSwap) {
				*opt = ByteSwap(*opt);
			}
			SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
			return true;
		}
	}
	return false;
}

bool CHexDlgTemplMgr::SetDataULL(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const
{
	if (auto opt = stn::StrToULL(pwszText); opt) {
		if (fShouldSwap) {
			*opt = ByteSwap(*opt);
		}
		SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
		return true;
	}
	return false;
}

bool CHexDlgTemplMgr::SetDataFloat(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const
{
	//Convert hex text to float is impossible, therefore we convert it to
	//unsigned type of the same size - uint.
	if (IsShowAsHex()) {
		if (auto opt = stn::StrToUInt(pwszText); opt) {
			if (fShouldSwap) {
				*opt = ByteSwap(*opt);
			}
			SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
			return true;
		}
	}
	else {
		if (auto opt = stn::StrToFloat(pwszText); opt) {
			if (fShouldSwap) {
				*opt = ByteSwap(*opt);
			}
			SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
			return true;
		}
	}
	return false;
}

bool CHexDlgTemplMgr::SetDataDouble(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const
{
	//Convert hex text to double is impossible, therefore we convert it to
	//unsigned type of the same size - ULL.
	if (IsShowAsHex()) {
		if (auto opt = stn::StrToULL(pwszText); opt) {
			if (fShouldSwap) {
				*opt = ByteSwap(*opt);
			}
			SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
			return true;
		}
	}
	else {
		if (auto opt = stn::StrToDouble(pwszText); opt) {
			if (fShouldSwap) {
				*opt = ByteSwap(*opt);
			}
			SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
			return true;
		}
	}
	return false;
}

bool CHexDlgTemplMgr::SetDataTime32(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const
{
	if (IsShowAsHex()) {
		if (auto opt = stn::StrToUInt(pwszText); opt) {
			if (fShouldSwap) {
				*opt = ByteSwap(*opt);
			}
			SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
			return true;
		}
	}
	else {
		//The number of seconds since midnight January 1st 1970 UTC (32-bit). This wraps on 19 January 2038.
		const auto optSysTime = StringToSystemTime(pwszText, m_dwDateFormat);
		if (!optSysTime)
			return false;

		//Unix times are signed but value before 1st January 1970 is not considered valid.
		//This is apparently because early complilers didn't support unsigned types. _mktime32() has the same limit.
		if (optSysTime->wYear < 1970)
			return false;

		FILETIME ftTime;
		if (!SystemTimeToFileTime(&*optSysTime, &ftTime))
			return false;

		//Convert ticks to seconds and adjust epoch.
		LARGE_INTEGER lTicks;
		lTicks.HighPart = ftTime.dwHighDateTime;
		lTicks.LowPart = ftTime.dwLowDateTime;
		lTicks.QuadPart /= g_uFTTicksPerSec;
		lTicks.QuadPart -= g_ullUnixEpochDiff;
		if (lTicks.QuadPart >= LONG_MAX)
			return false;

		auto lTime32 = static_cast<__time32_t>(lTicks.QuadPart);
		if (fShouldSwap) {
			lTime32 = ByteSwap(lTime32);
		}

		SetIHexTData(*m_pHexCtrl, ullOffset, lTime32);
		return true;
	}

	return false;
}

bool CHexDlgTemplMgr::SetDataTime64(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const
{
	if (IsShowAsHex()) {
		if (auto opt = stn::StrToULL(pwszText); opt) {
			if (fShouldSwap) {
				*opt = ByteSwap(*opt);
			}
			SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
			return true;
		}
	}
	else {
		//The number of seconds since midnight January 1st 1970 UTC (32-bit). This wraps on 19 January 2038.
		const auto optSysTime = StringToSystemTime(pwszText, m_dwDateFormat);
		if (!optSysTime)
			return false;

		//Unix times are signed but value before 1st January 1970 is not considered valid.
		//This is apparently because early complilers didn't support unsigned types. _mktime64() has the same limit.
		if (optSysTime->wYear < 1970)
			return false;

		FILETIME ftTime;
		if (!SystemTimeToFileTime(&*optSysTime, &ftTime))
			return false;

		//Convert ticks to seconds and adjust epoch.
		LARGE_INTEGER lTicks;
		lTicks.HighPart = ftTime.dwHighDateTime;
		lTicks.LowPart = ftTime.dwLowDateTime;
		lTicks.QuadPart /= g_uFTTicksPerSec;
		lTicks.QuadPart -= g_ullUnixEpochDiff;

		auto llTime64 = static_cast<__time64_t>(lTicks.QuadPart);
		if (fShouldSwap) {
			llTime64 = ByteSwap(llTime64);
		}

		SetIHexTData(*m_pHexCtrl, ullOffset, llTime64);
		return true;
	}

	return false;
}

bool CHexDlgTemplMgr::SetDataFILETIME(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const
{
	if (IsShowAsHex()) {
		if (auto opt = stn::StrToULL(pwszText); opt) {
			if (fShouldSwap) {
				*opt = ByteSwap(*opt);
			}
			SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
			return true;
		}
	}
	else {
		const auto optFileTime = StringToFileTime(pwszText, m_dwDateFormat);
		if (!optFileTime)
			return false;

		ULARGE_INTEGER stLITime;
		stLITime.LowPart = optFileTime->dwLowDateTime;
		stLITime.HighPart = optFileTime->dwHighDateTime;

		if (fShouldSwap) {
			stLITime.QuadPart = ByteSwap(stLITime.QuadPart);
		}
		SetIHexTData(*m_pHexCtrl, ullOffset, stLITime.QuadPart);
	}

	return false;
}

bool CHexDlgTemplMgr::SetDataSYSTEMTIME(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const
{
	//No Hex representation for this type because size is too big, > ULONGLONG.
	const auto optSysTime = StringToSystemTime(pwszText, m_dwDateFormat);
	if (!optSysTime)
		return false;

	auto stSTime = *optSysTime;
	if (fShouldSwap) {
		stSTime.wYear = ByteSwap(stSTime.wYear);
		stSTime.wMonth = ByteSwap(stSTime.wMonth);
		stSTime.wDayOfWeek = ByteSwap(stSTime.wDayOfWeek);
		stSTime.wDay = ByteSwap(stSTime.wDay);
		stSTime.wHour = ByteSwap(stSTime.wHour);
		stSTime.wMinute = ByteSwap(stSTime.wMinute);
		stSTime.wSecond = ByteSwap(stSTime.wSecond);
		stSTime.wMilliseconds = ByteSwap(stSTime.wMilliseconds);
	}

	SetIHexTData(*m_pHexCtrl, ullOffset, stSTime);

	return true;
}

bool CHexDlgTemplMgr::SetDataGUID(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap) const
{
	//No Hex representation for this type because size is too big, > ULONGLONG.
	GUID stGUID;
	if (IIDFromString(pwszText, &stGUID) != S_OK)
		return false;

	if (fShouldSwap) {
		stGUID.Data1 = ByteSwap(stGUID.Data1);
		stGUID.Data2 = ByteSwap(stGUID.Data2);
		stGUID.Data3 = ByteSwap(stGUID.Data3);
	}

	SetIHexTData(*m_pHexCtrl, ullOffset, stGUID);

	return true;
}

void CHexDlgTemplMgr::SetDlgButtonsState()
{
	const auto fHasTempl = HasTemplates();
	if (const auto pBtnApply = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_TEMPLMGR_BTN_APPLY));
		pBtnApply != nullptr) {
		pBtnApply->EnableWindow(fHasTempl);
	}
	if (const auto pBtnUnload = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_TEMPLMGR_BTN_UNLOAD));
		pBtnUnload != nullptr) {
		pBtnUnload->EnableWindow(fHasTempl);
	}
	if (const auto pBtnRandom = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_TEMPLMGR_BTN_RNDCLR));
		pBtnRandom != nullptr) {
		pBtnRandom->EnableWindow(fHasTempl);
	}
	m_editOffset.EnableWindow(fHasTempl);
}

void CHexDlgTemplMgr::SetHexSelByField(PCHEXTEMPLATEFIELD pField)
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

void CHexDlgTemplMgr::ShowListDataBool(LPWSTR pwsz, unsigned char uchData) const
{
	*std::vformat_to(pwsz, IsShowAsHex() ? L"0x{0:02X}" : L"{1}",
		std::make_wformat_args(uchData, static_cast<bool>(uchData))) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataChar(LPWSTR pwsz, char chData)const
{
	*std::vformat_to(pwsz, IsShowAsHex() ? L"0x{0:02X}" : L"{1}",
		std::make_wformat_args(static_cast<unsigned char>(chData), static_cast<int>(chData))) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataUChar(LPWSTR pwsz, unsigned char uchData)const
{
	*std::vformat_to(pwsz, IsShowAsHex() ? L"0x{:02X}" : L"{}", std::make_wformat_args(uchData)) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataShort(LPWSTR pwsz, short shortData, bool fShouldSwap)const
{
	if (fShouldSwap) {
		shortData = ByteSwap(shortData);
	}
	*std::vformat_to(pwsz, IsShowAsHex() ? L"0x{0:04X}" : L"{1}",
		std::make_wformat_args(static_cast<unsigned short>(shortData), shortData)) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataUShort(LPWSTR pwsz, unsigned short wData, bool fShouldSwap)const
{
	if (fShouldSwap) {
		wData = ByteSwap(wData);
	}
	*std::vformat_to(pwsz, IsShowAsHex() ? L"0x{:04X}" : L"{}", std::make_wformat_args(wData)) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataInt(LPWSTR pwsz, int intData, bool fShouldSwap)const
{
	if (fShouldSwap) {
		intData = ByteSwap(intData);
	}
	*std::vformat_to(pwsz, IsShowAsHex() ? L"0x{0:08X}" : L"{1}",
		std::make_wformat_args(static_cast<unsigned int>(intData), intData)) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataUInt(LPWSTR pwsz, unsigned int dwData, bool fShouldSwap)const
{
	if (fShouldSwap) {
		dwData = ByteSwap(dwData);
	}
	*std::vformat_to(pwsz, IsShowAsHex() ? L"0x{:08X}" : L"{}", std::make_wformat_args(dwData)) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataLL(LPWSTR pwsz, long long llData, bool fShouldSwap)const
{
	if (fShouldSwap) {
		llData = ByteSwap(llData);
	}
	*std::vformat_to(pwsz, IsShowAsHex() ? L"0x{0:016X}" : L"{1}",
		std::make_wformat_args(static_cast<unsigned long long>(llData), llData)) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataULL(LPWSTR pwsz, unsigned long long ullData, bool fShouldSwap)const
{
	if (fShouldSwap) {
		ullData = ByteSwap(ullData);
	}
	*std::vformat_to(pwsz, IsShowAsHex() ? L"0x{:016X}" : L"{}", std::make_wformat_args(ullData)) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataFloat(LPWSTR pwsz, float flData, bool fShouldSwap)const
{
	if (fShouldSwap) {
		flData = ByteSwap(flData);
	}
	*std::vformat_to(pwsz, IsShowAsHex() ? L"0x{0:08X}" : L"{1:.9e}",
		std::make_wformat_args(std::bit_cast<unsigned long>(flData), flData)) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataDouble(LPWSTR pwsz, double dblData, bool fShouldSwap)const
{
	if (fShouldSwap) {
		dblData = ByteSwap(dblData);
	}
	*std::vformat_to(pwsz, IsShowAsHex() ? L"0x{0:016X}" : L"{1:.18e}",
		std::make_wformat_args(std::bit_cast<unsigned long long>(dblData), dblData)) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataTime32(LPWSTR pwsz, __time32_t lTime32, bool fShouldSwap)const
{
	if (fShouldSwap) {
		lTime32 = ByteSwap(lTime32);
	}

	if (IsShowAsHex()) {
		*std::format_to(pwsz, L"0x{:08X}", static_cast<unsigned long>(lTime32)) = L'\0';
		return;
	}

	if (lTime32 < 0) {
		*std::format_to(pwsz, L"N/A") = L'\0';
		return;
	}

	//Add seconds from epoch time
	LARGE_INTEGER Time;
	Time.HighPart = g_ulFileTime1970_HIGH;
	Time.LowPart = g_ulFileTime1970_LOW;
	Time.QuadPart += static_cast<LONGLONG>(lTime32) * g_uFTTicksPerSec;

	//Convert to FILETIME
	FILETIME ftTime;
	ftTime.dwHighDateTime = Time.HighPart;
	ftTime.dwLowDateTime = Time.LowPart;

	*std::format_to(pwsz, L"{}", FileTimeToString(ftTime, m_dwDateFormat, m_wchDateSepar)) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataTime64(LPWSTR pwsz, __time64_t llTime64, bool fShouldSwap)const
{
	if (fShouldSwap) {
		llTime64 = ByteSwap(llTime64);
	}

	if (IsShowAsHex()) {
		*std::format_to(pwsz, L"0x{:016X}", static_cast<unsigned long long>(llTime64)) = L'\0';
		return;
	}

	if (llTime64 < 0) {
		*std::format_to(pwsz, L"N/A") = L'\0';
		return;
	}

	//Add seconds from epoch time
	LARGE_INTEGER Time;
	Time.HighPart = g_ulFileTime1970_HIGH;
	Time.LowPart = g_ulFileTime1970_LOW;
	Time.QuadPart += llTime64 * g_uFTTicksPerSec;

	//Convert to FILETIME
	FILETIME ftTime;
	ftTime.dwHighDateTime = Time.HighPart;
	ftTime.dwLowDateTime = Time.LowPart;

	*std::format_to(pwsz, L"{}", FileTimeToString(ftTime, m_dwDateFormat, m_wchDateSepar)) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataFILETIME(LPWSTR pwsz, FILETIME stFTime, bool fShouldSwap)const
{
	if (fShouldSwap) {
		stFTime = ByteSwap(stFTime);
	}
	*std::vformat_to(pwsz, IsShowAsHex() ? L"0x{0:016X}" : L"{}",
		std::make_wformat_args(std::bit_cast<unsigned long long>(stFTime),
			FileTimeToString(stFTime, m_dwDateFormat, m_wchDateSepar))) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataSYSTEMTIME(LPWSTR pwsz, SYSTEMTIME stSTime, bool fShouldSwap)const
{
	//No Hex representation for this type because size is too big, > ULONGLONG.
	if (fShouldSwap) {
		stSTime.wYear = ByteSwap(stSTime.wYear);
		stSTime.wMonth = ByteSwap(stSTime.wMonth);
		stSTime.wDayOfWeek = ByteSwap(stSTime.wDayOfWeek);
		stSTime.wDay = ByteSwap(stSTime.wDay);
		stSTime.wHour = ByteSwap(stSTime.wHour);
		stSTime.wMinute = ByteSwap(stSTime.wMinute);
		stSTime.wSecond = ByteSwap(stSTime.wSecond);
		stSTime.wMilliseconds = ByteSwap(stSTime.wMilliseconds);
	}

	*std::format_to(pwsz, L"{}", SystemTimeToString(stSTime, m_dwDateFormat, m_wchDateSepar)) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataGUID(LPWSTR pwsz, GUID stGUID, bool fShouldSwap)const
{
	//No Hex representation for this type because size is too big, > ULONGLONG.
	if (fShouldSwap) {
		stGUID.Data1 = ByteSwap(stGUID.Data1);
		stGUID.Data2 = ByteSwap(stGUID.Data2);
		stGUID.Data3 = ByteSwap(stGUID.Data3);
	}

	*std::format_to(pwsz, L"{{{:0>8x}-{:0>4x}-{:0>4x}-{:0>2x}{:0>2x}-{:0>2x}{:0>2x}{:0>2x}{:0>2x}{:0>2x}{:0>2x}}}",
		stGUID.Data1, stGUID.Data2, stGUID.Data3, stGUID.Data4[0], stGUID.Data4[1], stGUID.Data4[2],
		stGUID.Data4[3], stGUID.Data4[4], stGUID.Data4[5], stGUID.Data4[6], stGUID.Data4[7]) = L'\0';
}

auto CHexDlgTemplMgr::TreeItemFromListItem(int iListItem)const->HTREEITEM
{
	auto hChildItem = m_stTreeApplied.GetNextItem(m_hTreeCurrParent, TVGN_CHILD);
	for (auto iterListItems = 0; iterListItems < iListItem; ++iterListItems) {
		hChildItem = m_stTreeApplied.GetNextSiblingItem(hChildItem);
	}

	return hChildItem;
}

void CHexDlgTemplMgr::UnloadTemplate(int iTemplateID)
{
	OnTemplateLoadUnload(iTemplateID, false); //All the GUI stuff first, only then delete template.

	const auto iterTempl = std::find_if(m_vecTemplates.begin(), m_vecTemplates.end(),
		[iTemplateID](const std::unique_ptr<HEXTEMPLATE>& ref) { return ref->iTemplateID == iTemplateID; });
	if (iterTempl == m_vecTemplates.end())
		return;

	//Remove all applied templates from m_vecTemplatesApplied, if any, with the given iTemplateID.
	if (std::erase_if(m_vecTemplatesApplied, [pTemplate = iterTempl->get()](const std::unique_ptr<HEXTEMPLATEAPPLIED>& ref) {
		return ref->pTemplate == pTemplate; }) > 0) {
		if (m_pHexCtrl->IsDataSet()) {
			m_pHexCtrl->Redraw();
		}
	}
	m_vecTemplates.erase(iterTempl); //Remove template itself.

	//This needed because SetDlgButtonsState checks m_vecTemplates.empty(), which was erased just line above.
	//OnTemplateLoadUnload at the beginning doesn't know yet that it's empty (when all templates are unloaded).
	SetDlgButtonsState();
}

void CHexDlgTemplMgr::UpdateStaticText()
{
	std::wstring wstrOffset { };
	std::wstring wstrSize { };

	if (m_pAppliedCurr != nullptr) { //If m_pAppliedCurr == nullptr set empty text.
		wstrOffset = std::vformat(IsShowAsHex() ? L"0x{:X}" : L"{}", std::make_wformat_args(m_pAppliedCurr->ullOffset));
		wstrSize = std::vformat(IsShowAsHex() ? L"0x{:X}" : L"{}", std::make_wformat_args(m_pAppliedCurr->pTemplate->iSizeTotal));
	}

	m_wndStaticOffset.SetWindowTextW(wstrOffset.data());
	m_wndStaticSize.SetWindowTextW(wstrSize.data());
}

bool CHexDlgTemplMgr::JSONParseFields(const IterJSONMember iterFieldsArray, VecFields& vecFields,
	const FIELDSDEFPROPS& stDefault, UmapCustomTypes& umapCustomT, int* pOffset)
{
	using enum EHexFieldType;
	static const std::unordered_map<std::string_view, EHexFieldType> umapStrToEType { //From JSON string to EHexFieldType conversion.
		{ "bool", type_bool },
		{ "char", type_char }, { "unsigned char", type_uchar }, { "byte", type_uchar },
		{ "short", type_short }, { "unsigned short", type_ushort }, { "WORD", type_ushort },
		{ "long", type_int }, { "unsigned long", type_uint }, { "int", type_int },
		{ "unsigned int", type_uint }, { "DWORD", type_uint }, { "long long", type_ll },
		{ "unsigned long long", type_ull }, { "QWORD", type_ull }, { "float", type_float },
		{ "double", type_double }, { "time32_t", type_time32 }, { "time64_t", type_time64 },
		{ "FILETIME", type_filetime }, { "SYSTEMTIME", type_systemtime }, { "GUID", type_guid } };
	static const std::unordered_map<EHexFieldType, int> umapTypeToSize { //Types sizes.
		{ type_bool, static_cast<int>(sizeof(bool)) }, { type_char, static_cast<int>(sizeof(char)) },
		{ type_uchar, static_cast<int>(sizeof(char)) }, { type_short, static_cast<int>(sizeof(short)) },
		{ type_ushort, static_cast<int>(sizeof(short)) }, { type_int, static_cast<int>(sizeof(int)) },
		{ type_uint, static_cast<int>(sizeof(int)) }, { type_ll, static_cast<int>(sizeof(long long)) },
		{ type_ull, static_cast<int>(sizeof(long long)) }, { type_float, static_cast<int>(sizeof(float)) },
		{ type_double, static_cast<int>(sizeof(double)) }, { type_time32, static_cast<int>(sizeof(__time32_t)) },
		{ type_time64, static_cast<int>(sizeof(__time64_t)) }, { type_filetime, static_cast<int>(sizeof(FILETIME)) },
		{ type_systemtime, static_cast<int>(sizeof(SYSTEMTIME)) }, { type_guid, static_cast<int>(sizeof(GUID)) }
	};

	const auto lmbTotalSize = [](const VecFields& vecFields)->int { //Counts total size of all Fields in VecFields, recursively.
		const auto _lmbTotalSize = [](const auto& lmbSelf, const VecFields& vecFields)->int {
			return std::accumulate(vecFields.begin(), vecFields.end(), 0,
				[&lmbSelf](auto ullTotal, const std::unique_ptr<HEXTEMPLATEFIELD>& refField) {
					if (!refField->vecNested.empty()) {
						return ullTotal + lmbSelf(lmbSelf, refField->vecNested);
					}
					return ullTotal + refField->iSize; });
		};
		return _lmbTotalSize(_lmbTotalSize, vecFields);
	};

	int iOffset { 0 }; //Default starting offset.
	if (pOffset == nullptr) {
		pOffset = &iOffset;
	}

	for (auto pField = iterFieldsArray->value.Begin(); pField != iterFieldsArray->value.End(); ++pField) {
		if (!pField->IsObject()) {
			return false; //Each array entry must be an Object {}.
		}

		std::wstring wstrNameField;
		if (const auto itName = pField->FindMember("name");
			itName != pField->MemberEnd() && itName->value.IsString()) {
			wstrNameField = StrToWstr(itName->value.GetString());
		}
		else {
			return false; //Each array entry (Object) must have a "name" string.
		}

		const auto& pNewField = vecFields.emplace_back(std::make_unique<HEXTEMPLATEFIELD>());
		pNewField->wstrName = wstrNameField;
		pNewField->pTemplate = stDefault.pTemplate;
		pNewField->pFieldParent = stDefault.pFieldParent;
		pNewField->clrBk = JSONColors(*pField, "clrBk").value_or(stDefault.clrBk);
		pNewField->clrText = JSONColors(*pField, "clrText").value_or(stDefault.clrText);
		pNewField->fBigEndian = JSONEndianness(*pField).value_or(stDefault.fBigEndian);
		pNewField->iOffset = *pOffset;
		pNewField->eType = type_custom;

		if (const auto iterNestedFields = pField->FindMember("Fields");
			iterNestedFields != pField->MemberEnd()) {
			if (!iterNestedFields->value.IsArray()) {
				return false; //Each "Fields" must be an Array.
			}

			//Setting defaults for the next nested fields.
			const FIELDSDEFPROPS stDefsNested { .clrBk { pNewField->clrBk }, .clrText { pNewField->clrText },
				.pTemplate { stDefault.pTemplate }, .pFieldParent { pNewField.get() }, .fBigEndian { pNewField->fBigEndian } };

			//Recursion lambda for nested fields starts here.
			if (!JSONParseFields(iterNestedFields, pNewField->vecNested, stDefsNested, umapCustomT, pOffset)) {
				return false;
			}

			pNewField->iSize = lmbTotalSize(pNewField->vecNested); //Total size of all nested fields.
		}
		else {
			if (const auto iterDescr = pField->FindMember("description");
				iterDescr != pField->MemberEnd() && iterDescr->value.IsString()) {
				pNewField->wstrDescr = StrToWstr(iterDescr->value.GetString());
			}

			int iArraySize { 0 };
			if (const auto itArray = pField->FindMember("array");
				itArray != pField->MemberEnd() && itArray->value.IsInt() && itArray->value.GetInt() > 1) {
				iArraySize = itArray->value.GetInt();
			}

			int iSize { 0 }; //Current field's size, via "type" or "size" property.
			if (const auto iterType = pField->FindMember("type"); iterType != pField->MemberEnd()) {
				if (!iterType->value.IsString()) {
					return false; //"type" is not a string.
				}

				if (const auto iterMapType = umapStrToEType.find(iterType->value.GetString());
					iterMapType != umapStrToEType.end()) {
					pNewField->eType = iterMapType->second;
					iSize = umapTypeToSize.at(iterMapType->second);
				}
				else { //If it's not any standard type, we try to find custom type with given name.
					const auto& vecCT = stDefault.pTemplate->vecCustomType;
					const auto iterVecCT = std::find_if(vecCT.begin(), vecCT.end(),
						[=](const HEXCUSTOMTYPE& ref) { return ref.wstrTypeName == StrToWstr(iterType->value.GetString()); });
					if (iterVecCT == vecCT.end()) {
						return false; //Undefined/unknown "type".
					}

					pNewField->uTypeID = iterVecCT->uTypeID; //Custom type ID.
					pNewField->eType = type_custom;
					const auto lmbCopyCustomType = [&stDefault]
					(const VecFields& vecCustomFields, const PtrField& pField, int& iOffset)->void {
						const auto _lmbCustomTypeCopy = [&stDefault]
						(auto& lmbSelf, const VecFields& vecCustomFields, const PtrField& pField, int& iOffset)->void {
							for (const auto& pCustomField : vecCustomFields) {
								const auto& pNewField = pField->vecNested.emplace_back(std::make_unique<HEXTEMPLATEFIELD>());
								pNewField->pTemplate = stDefault.pTemplate;
								pNewField->pFieldParent = pField.get();
								pNewField->clrBk = pCustomField->clrBk == -1 ? pField->clrBk : pCustomField->clrBk;
								pNewField->clrText = pCustomField->clrText == -1 ? pField->clrText : pCustomField->clrText;
								pNewField->fBigEndian = pCustomField->fBigEndian;
								pNewField->eType = pCustomField->eType;
								pNewField->uTypeID = pCustomField->uTypeID;
								pNewField->iOffset = iOffset;
								pNewField->iSize = pCustomField->iSize;
								pNewField->wstrName = pCustomField->wstrName;

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

					auto iOffsetCustomType = *pOffset;
					if (iArraySize <= 1) {
						lmbCopyCustomType(umapCustomT[iterVecCT->uTypeID], pNewField, iOffsetCustomType);
					}
					else { //Creating array of Custom Types.
						for (int iArrIndex = 0; iArrIndex < iArraySize; ++iArrIndex) {
							const auto& pFieldArray = pNewField->vecNested.emplace_back(std::make_unique<HEXTEMPLATEFIELD>());
							pFieldArray->pTemplate = pNewField->pTemplate;
							pFieldArray->pFieldParent = pNewField.get();
							pFieldArray->clrBk = pNewField->clrBk;
							pFieldArray->clrText = pNewField->clrText;
							pFieldArray->fBigEndian = pNewField->fBigEndian;
							pFieldArray->eType = pNewField->eType;
							pFieldArray->uTypeID = pNewField->uTypeID;
							pFieldArray->iOffset = iOffsetCustomType;
							lmbCopyCustomType(umapCustomT[iterVecCT->uTypeID], pFieldArray, iOffsetCustomType);
							pFieldArray->iSize = lmbTotalSize(pFieldArray->vecNested);
							pFieldArray->wstrName = std::format(L"{}[{}]", wstrNameField, iArrIndex);
						}
						pNewField->wstrName = std::format(L"{}[{}]", wstrNameField, iArraySize);
					}

					iSize = lmbTotalSize(pNewField->vecNested);
				}
			}
			else { //No "type" property was found.
				const auto iterSize = pField->FindMember("size");
				if (iterSize == pField->MemberEnd()) {
					return false; //No "size" property was found.
				}

				if (!iterSize->value.IsInt()) {
					return false; //"size" must be an int.
				}

				const auto iFieldSize = iterSize->value.GetInt();
				assert(iFieldSize > 0);
				if (iFieldSize < 1) {
					return false; //"size" must be > 0.
				}

				iSize = iFieldSize;
				pNewField->eType = custom_size;
			}

			if (iArraySize > 1 && pNewField->eType != type_custom) { //Creating array of standard/default types.
				for (auto iArrIndex = 0; iArrIndex < iArraySize; ++iArrIndex) {
					const auto& pFieldArray = pNewField->vecNested.emplace_back(std::make_unique<HEXTEMPLATEFIELD>());
					pFieldArray->pTemplate = pNewField->pTemplate;
					pFieldArray->pFieldParent = pNewField.get();
					pFieldArray->clrBk = pNewField->clrBk;
					pFieldArray->clrText = pNewField->clrText;
					pFieldArray->fBigEndian = pNewField->fBigEndian;
					pFieldArray->eType = pNewField->eType;
					pFieldArray->uTypeID = pNewField->uTypeID;
					pFieldArray->iOffset = *pOffset + iArrIndex * iSize;
					pFieldArray->iSize = iSize;
					pFieldArray->wstrName = std::format(L"{}[{}]", wstrNameField, iArrIndex);
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
		return std::nullopt; //"endianness" must be a string.
	}

	const std::string_view svEndianness = itEndianness->value.GetString();
	if (svEndianness != "big" && svEndianness != "little") {
		return std::nullopt; //Wrong "endianness" type.
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

	const auto R = *stn::StrToUInt(sv.substr(1, 2), 16);
	const auto G = *stn::StrToUInt(sv.substr(3, 2), 16);
	const auto B = *stn::StrToUInt(sv.substr(5, 2), 16);

	return RGB(R, G, B);
}

auto CHexDlgTemplMgr::LoadFromFile(const wchar_t* pFilePath)->std::unique_ptr<HEXTEMPLATE>
{
	std::ifstream ifs(pFilePath);
	if (!ifs.is_open())
		return { };

	rapidjson::IStreamWrapper isw { ifs };
	rapidjson::Document docJSON;
	docJSON.ParseStream(isw);
	if (docJSON.IsNull()) {
		return { };
	}

	auto pTemplateUnPtr = std::make_unique<HEXTEMPLATE>();
	const auto pTemplate = pTemplateUnPtr.get();
	auto& refVecFields = pTemplate->vecFields;

	if (const auto it = docJSON.FindMember("TemplateName"); it != docJSON.MemberEnd()) {
		if (!it->value.IsString()) {
			return { }; //"TemplateName" must be a string.
		}
		pTemplate->wstrName = StrToWstr(it->value.GetString());
	}
	else {
		return { }; //Template must have a name.
	}

	UmapCustomTypes umapCT;
	std::uint8_t uCustomTypeID = 1; //ID starts at 1.
	if (const auto objCustomTypes = docJSON.FindMember("CustomTypes");
		objCustomTypes != docJSON.MemberEnd() && objCustomTypes->value.IsArray()) {
		for (auto pCustomType = objCustomTypes->value.Begin(); pCustomType != objCustomTypes->value.End(); ++pCustomType, ++uCustomTypeID) {
			if (!pCustomType->IsObject()) {
				return { }; //Each CustomTypes' array entry must be an Object.
			}

			std::wstring wstrTypeName;
			if (const auto itName = pCustomType->FindMember("TypeName");
				itName != pCustomType->MemberEnd() && itName->value.IsString()) {
				wstrTypeName = StrToWstr(itName->value.GetString());
			}
			else {
				return { }; //Each array entry (Object) must have a "TypeName" property.
			}

			const auto clrBk = JSONColors(*pCustomType, "clrBk").value_or(-1);
			const auto clrText = JSONColors(*pCustomType, "clrText").value_or(-1);
			const auto fBigEndian = JSONEndianness(*pCustomType).value_or(false);

			const auto iterFieldsArray = pCustomType->FindMember("Fields");
			if (iterFieldsArray == pCustomType->MemberEnd() || !iterFieldsArray->value.IsArray()) {
				return { }; //Each "Fields" must be an array.
			}

			umapCT.try_emplace(uCustomTypeID, VecFields { });
			const FIELDSDEFPROPS stDefTypes { .clrBk { clrBk }, .clrText { clrText },
				.pTemplate { pTemplate }, .fBigEndian { fBigEndian } };
			if (!JSONParseFields(iterFieldsArray, umapCT[uCustomTypeID], stDefTypes, umapCT)) {
				return { }; //Something went wrong during template parsing.
			}
			pTemplate->vecCustomType.emplace_back(std::move(wstrTypeName), uCustomTypeID);
		}
	}

	if (const auto objData = docJSON.FindMember("Data"); objData != docJSON.MemberEnd() && objData->value.IsObject()) {
		const auto clrBk = JSONColors(objData->value, "clrBk").value_or(-1);
		const auto clrText = JSONColors(objData->value, "clrText").value_or(-1);
		const auto fBigEndian = JSONEndianness(objData->value).value_or(false);
		const FIELDSDEFPROPS stDefsFields { .clrBk { clrBk }, .clrText { clrText },
			.pTemplate { pTemplate }, .fBigEndian { fBigEndian } };

		const auto iterFieldsArray = objData->value.FindMember("Fields");
		if (!JSONParseFields(iterFieldsArray, refVecFields, stDefsFields, umapCT)) {
			return { }; //Something went wrong during template parsing.
		}
	}
	else {
		return { }; //No "Data" member in template.
	}

	return pTemplateUnPtr;
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