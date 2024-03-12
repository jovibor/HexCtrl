/****************************************************************************************
* Copyright © 2018-2024 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "../../dep/StrToNum/StrToNum/StrToNum.h"
#include "../../res/HexCtrlRes.h"
#include "CHexDlgTemplMgr.h"
#include "strsafe.h"
#include <algorithm>
#include <format>
#include <fstream>
#include <numeric>
#include <optional>
#include <random>

using namespace HEXCTRL::INTERNAL;

struct CHexDlgTemplMgr::TEMPLAPPLIED {
	ULONGLONG     ullOffset;  //Offset, where to apply a template.
	PCHEXTEMPLATE pTemplate;  //Template pointer.
	int           iAppliedID; //Applied/runtime ID, assigned by framework. Any template can be applied more than once.
};

enum class CHexDlgTemplMgr::EMenuID : std::uint16_t {
	IDM_TREEAPPLIED_DISAPPLY = 0x8000, IDM_TREEAPPLIED_DISAPPLYALL = 0x8001,
	IDM_LISTAPPLIED_HDR_TYPE = 0x8100, IDM_LISTAPPLIED_HDR_NAME, IDM_LISTAPPLIED_HDR_OFFSET,
	IDM_LISTAPPLIED_HDR_SIZE, IDM_LISTAPPLIED_HDR_DATA, IDM_LISTAPPLIED_HDR_ENDIANNESS, IDM_LISTAPPLIED_HDR_COLORS
};

struct CHexDlgTemplMgr::FIELDSDEFPROPS { //Helper struct for convenient argument passing through recursive fields' parsing.
	HEXCOLOR        stClr { };
	PCHEXTEMPLATE   pTemplate { }; //Same for all fields.
	PCHEXTEMPLFIELD pFieldParent { };
	bool            fBigEndian { false };
};

BEGIN_MESSAGE_MAP(CHexDlgTemplMgr, CDialogEx)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_BTN_LOAD, &CHexDlgTemplMgr::OnBnLoadTemplate)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_BTN_UNLOAD, &CHexDlgTemplMgr::OnBnUnloadTemplate)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_BTN_RNDCLR, &CHexDlgTemplMgr::OnBnRandomizeColors)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_BTN_APPLY, &CHexDlgTemplMgr::OnBnApply)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_CHK_TT, &CHexDlgTemplMgr::OnCheckShowTt)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_CHK_HEX, &CHexDlgTemplMgr::OnCheckHex)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_CHK_SWAP, &CHexDlgTemplMgr::OnCheckSwapEndian)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_CHK_MIN, &CHexDlgTemplMgr::OnCheckMin)
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
	ON_WM_CLOSE()
	ON_WM_CTLCOLOR()
	ON_WM_DESTROY()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_NCHITTEST()
END_MESSAGE_MAP()

CHexDlgTemplMgr::CHexDlgTemplMgr() = default;
CHexDlgTemplMgr::~CHexDlgTemplMgr() = default;

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
	const auto pTemplate = GetTemplate(iTemplateID);
	if (pTemplate == nullptr)
		return 0;

	auto iAppliedID = 1; //AppliedID starts at 1.
	if (const auto iter = std::max_element(m_vecTemplatesAppl.begin(), m_vecTemplatesAppl.end(),
		[](const std::unique_ptr<TEMPLAPPLIED>& p1, const std::unique_ptr<TEMPLAPPLIED>& p2) {
			return p1->iAppliedID < p2->iAppliedID; }); iter != m_vecTemplatesAppl.end()) {
		iAppliedID = iter->get()->iAppliedID + 1; //Increasing next AppliedID by 1.
	}

	const auto pApplied = m_vecTemplatesAppl.emplace_back(
		std::make_unique<TEMPLAPPLIED>(ullOffset, pTemplate, iAppliedID)).get();

	//Tree root node.
	TVINSERTSTRUCTW tvi { .hParent = TVI_ROOT, .itemex { .mask = TVIF_CHILDREN | TVIF_TEXT | TVIF_PARAM,
		.pszText = LPSTR_TEXTCALLBACK, .cChildren = static_cast<int>(pTemplate->vecFields.size()),
		.lParam = reinterpret_cast<LPARAM>(pApplied) } }; //Tree root node has PCTEMPLAPPLIED ptr.
	const auto hTreeRootNode = m_treeApplied.InsertItem(&tvi);

	const auto lmbFill = [&](HTREEITEM hTreeRoot, const HexVecFields& refVecFields)->void {
		const auto _lmbFill = [&](const auto& lmbSelf, HTREEITEM hTreeRoot, const HexVecFields& refVecFields)->void {
			for (const auto& pField : refVecFields) {
				tvi.hParent = hTreeRoot;
				tvi.itemex.cChildren = static_cast<int>(pField->vecNested.size());
				tvi.itemex.lParam = reinterpret_cast<LPARAM>(pField.get()); //Tree child nodes have PCTEMPLAPPLIED ptr.
				const auto hCurrentRoot = m_treeApplied.InsertItem(&tvi);
				if (tvi.itemex.cChildren > 0) {
					lmbSelf(lmbSelf, hCurrentRoot, pField->vecNested);
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
		m_treeApplied.DeleteAllItems();
		m_pListApplied->SetItemCountEx(0);
		UpdateStaticText();
	}

	m_pAppliedCurr = nullptr;
	m_pVecFieldsCurr = nullptr;
	m_hTreeCurrParent = nullptr;
	m_vecTemplatesAppl.clear();

	if (m_pHexCtrl->IsDataSet()) {
		m_pHexCtrl->Redraw();
	}
}

void CHexDlgTemplMgr::DisapplyByID(int iAppliedID)
{
	if (const auto iter = std::find_if(m_vecTemplatesAppl.begin(), m_vecTemplatesAppl.end(),
		[iAppliedID](const std::unique_ptr<TEMPLAPPLIED>& pData) { return pData->iAppliedID == iAppliedID; });
		iter != m_vecTemplatesAppl.end()) {
		RemoveNodeWithAppliedID(iAppliedID);
		m_vecTemplatesAppl.erase(iter);
		if (m_pHexCtrl->IsDataSet()) {
			m_pHexCtrl->Redraw();
		}
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
		if (m_pHexCtrl->IsDataSet()) {
			m_pHexCtrl->Redraw();
		}
	}
}

auto CHexDlgTemplMgr::GetDlgItemHandle(EHexDlgItem eItem)const->HWND
{
	if (!IsWindow(m_hWnd)) {
		return { };
	}

	using enum EHexDlgItem;
	switch (eItem) {
	case TEMPLMGR_CHK_MIN:
		return m_btnMin;
	case TEMPLMGR_CHK_TT:
		return m_btnShowTT;
	case TEMPLMGR_CHK_HGL:
		return m_btnHglSel;
	case TEMPLMGR_CHK_HEX:
		return m_btnHex;
	case TEMPLMGR_CHK_SWAP:
		return m_btnSwapEndian;
	default:
		return { };
	}
}

bool CHexDlgTemplMgr::HasApplied()const
{
	return !m_vecTemplatesAppl.empty();
}

bool CHexDlgTemplMgr::HasCurrent()const
{
	return IsWindow(m_hWnd) && HasTemplates();
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

void CHexDlgTemplMgr::SetDlgProperties(std::uint64_t u64Flags)
{
	m_u64Flags = u64Flags;
}

void CHexDlgTemplMgr::ShowTooltips(bool fShow)
{
	m_fTooltips = fShow;
}

BOOL CHexDlgTemplMgr::ShowWindow(int nCmdShow)
{
	if (!IsWindow(m_hWnd)) {
		Create(IDD_HEXCTRL_TEMPLMGR, CWnd::FromHandle(m_pHexCtrl->GetWndHandle(EHexWnd::WND_MAIN)));
	}

	return CDialogEx::ShowWindow(nCmdShow);
}

void CHexDlgTemplMgr::UnloadAll()
{
	DisapplyAll();

	for (const auto& pTemplate : m_vecTemplates) {
		OnTemplateLoadUnload(pTemplate->iTemplateID, false);
	}
	m_vecTemplates.clear();

	if (m_pHexCtrl->IsDataSet()) {
		m_pHexCtrl->Redraw();
	}
}

void CHexDlgTemplMgr::UpdateData()
{
	if (!::IsWindowVisible(m_hWnd)) {
		return;
	}

	m_pListApplied->RedrawWindow();
}


//Private methods.

void CHexDlgTemplMgr::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_COMBO_TEMPLATES, m_comboTemplates);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_EDIT_OFFSET, m_editOffset);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_TREE_APPLIED, m_treeApplied);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_CHK_MIN, m_btnMin);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_CHK_TT, m_btnShowTT);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_CHK_HGL, m_btnHglSel);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_CHK_HEX, m_btnHex);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_CHK_SWAP, m_btnSwapEndian);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_STAT_OFFSETNUM, m_wndStaticOffset);
	DDX_Control(pDX, IDC_HEXCTRL_TEMPLMGR_STAT_SIZENUM, m_wndStaticSize);
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
		//In such cases when OnCheckMin is hit the tree/list are handled/sized wrong.
		pLayout->SetMinSize({ 0, m_iDynLayoutMinY });

		pLayout->AddItem(IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, CMFCDynamicLayout::MoveNone(),
			CMFCDynamicLayout::SizeHorizontalAndVertical(100, 100));
		pLayout->AddItem(IDC_HEXCTRL_TEMPLMGR_TREE_APPLIED, CMFCDynamicLayout::MoveNone(),
			CMFCDynamicLayout::SizeVertical(100));
		pLayout->AddItem(IDC_HEXCTRL_TEMPLMGR_GRB_TOP, CMFCDynamicLayout::MoveNone(),
			CMFCDynamicLayout::SizeHorizontal(100));
		pLayout->AddItem(IDC_HEXCTRL_TEMPLMGR_CHK_MIN, CMFCDynamicLayout::MoveHorizontal(100),
			CMFCDynamicLayout::SizeNone());
	}
}

auto CHexDlgTemplMgr::GetAppliedFromItem(HTREEITEM hTreeItem)->PCTEMPLAPPLIED
{
	auto hRoot = hTreeItem;
	while (hRoot != nullptr) { //Root node.
		hTreeItem = hRoot;
		hRoot = m_treeApplied.GetNextItem(hTreeItem, TVGN_PARENT);
	}

	return reinterpret_cast<PCTEMPLAPPLIED>(m_treeApplied.GetItemData(hTreeItem));
}

auto CHexDlgTemplMgr::GetIDForNewTemplate()const->int
{
	auto iTemplateID = 1; //TemplateID starts at 1.
	if (const auto iter = std::max_element(m_vecTemplates.begin(), m_vecTemplates.end(),
		[](const std::unique_ptr<HEXTEMPLATE>& ref1, const std::unique_ptr<HEXTEMPLATE>& ref2) {
			return ref1->iTemplateID < ref2->iTemplateID; }); iter != m_vecTemplates.end()) {
		iTemplateID = iter->get()->iTemplateID + 1; //Increasing next Template's ID by 1.
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
	return m_btnHglSel.GetCheck() == BST_CHECKED;
}

bool CHexDlgTemplMgr::IsMinimized()const
{
	return m_btnMin.GetCheck() == BST_CHECKED;
}

bool CHexDlgTemplMgr::IsNoEsc()const
{
	return m_u64Flags & HEXCTRL_FLAG_DLG_NOESC;
}

bool CHexDlgTemplMgr::IsShowAsHex()const
{
	return  m_btnHex.GetCheck() == BST_CHECKED;
}

bool CHexDlgTemplMgr::IsSwapEndian()const
{
	return m_btnSwapEndian.GetCheck() == BST_CHECKED;
}

void CHexDlgTemplMgr::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if (!m_pHexCtrl->IsCreated())
		return;

	if (nState == WA_ACTIVE || nState == WA_CLICKACTIVE) {
		const auto [dwFormat, wchSepar] = m_pHexCtrl->GetDateInfo();
		m_dwDateFormat = dwFormat;
		m_wchDateSepar = wchSepar;
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
	CStringW wstrText;
	m_editOffset.GetWindowTextW(wstrText);
	if (wstrText.IsEmpty())
		return;

	const auto opt = stn::StrToUInt64(wstrText.GetString());
	if (!opt)
		return;

	const auto iIndex = m_comboTemplates.GetCurSel();
	if (iIndex == CB_ERR)
		return;

	const auto iTemplateID = static_cast<int>(m_comboTemplates.GetItemData(iIndex));
	ApplyTemplate(*opt, iTemplateID);
}

void CHexDlgTemplMgr::OnCancel()
{
	if (IsNoEsc()) //Not closing Dialog on Escape key.
		return;

	CDialogEx::OnCancel();
}

void CHexDlgTemplMgr::OnCheckHex()
{
	UpdateStaticText();
	m_pListApplied->RedrawWindow();
}

void CHexDlgTemplMgr::OnCheckSwapEndian()
{
	m_pListApplied->RedrawWindow();
}

void CHexDlgTemplMgr::OnCheckShowTt()
{
	//Overriden IHexTemplates` interface method.
	ShowTooltips(m_btnShowTT.GetCheck() == BST_CHECKED);
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
	static constexpr int arrIDsToResize[] { IDC_HEXCTRL_TEMPLMGR_TREE_APPLIED, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED };
	const auto fMinimize = IsMinimized();

	for (const auto id : arrIDsToHide) { //Hiding.
		GetDlgItem(id)->ShowWindow(fMinimize ? SW_HIDE : SW_SHOW);
	}

	CRect rcGRBTop; //Top Group Box rect.
	GetDlgItem(IDC_HEXCTRL_TEMPLMGR_GRB_TOP)->GetClientRect(rcGRBTop);
	const auto iHeightGRB = rcGRBTop.Height();
	auto hdwp = BeginDeferWindowPos(static_cast<int>(std::size(arrIDsToMove) + std::size(arrIDsToResize)));
	for (const auto id : arrIDsToMove) { //Moving.
		const auto pWnd = GetDlgItem(id);
		CRect rcWnd;
		pWnd->GetWindowRect(rcWnd);
		ScreenToClient(rcWnd);
		const auto iNewPosY = fMinimize ? rcWnd.top - iHeightGRB : rcWnd.top + iHeightGRB;
		hdwp = DeferWindowPos(hdwp, pWnd->m_hWnd, nullptr, rcWnd.left, iNewPosY, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
	}

	for (const auto id : arrIDsToResize) { //Resizing.
		const auto pWnd = GetDlgItem(id);
		CRect rcWnd;
		pWnd->GetWindowRect(rcWnd);
		rcWnd.top += fMinimize ? -iHeightGRB : iHeightGRB;
		ScreenToClient(rcWnd);
		hdwp = DeferWindowPos(hdwp, pWnd->m_hWnd, nullptr, rcWnd.left, rcWnd.top, rcWnd.Width(),
			rcWnd.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
	}
	EnableDynamicLayoutHelper(false); //Otherwise DynamicLayout won't know that all dynamic windows have changed.
	EndDeferWindowPos(hdwp);
	EnableDynamicLayoutHelper(true);

	m_btnMin.SetBitmap(fMinimize ? m_hBITMAPMax : m_hBITMAPMin); //Set arrow bitmap to the min-max checkbox.
}

void CHexDlgTemplMgr::OnClose()
{
	EndDialog(IDCANCEL);
}

BOOL CHexDlgTemplMgr::OnCommand(WPARAM wParam, LPARAM lParam)
{
	using enum EMenuID;
	const auto wMenuID = LOWORD(wParam);
	switch (static_cast<EMenuID>(wMenuID)) {
	case IDM_TREEAPPLIED_DISAPPLY:
		if (const auto pApplied = GetAppliedFromItem(m_treeApplied.GetSelectedItem()); pApplied != nullptr) {
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
		const auto fChecked = m_menuHdr.GetMenuState(wMenuID, MF_BYCOMMAND) & MF_CHECKED;
		m_pListApplied->HideColumn(wMenuID - static_cast<int>(IDM_LISTAPPLIED_HDR_TYPE), fChecked);
		m_menuHdr.CheckMenuItem(wMenuID, (fChecked ? MF_UNCHECKED : MF_CHECKED) | MF_BYCOMMAND);
	}
	return TRUE;
	default:
		break;
	}

	return CDialogEx::OnCommand(wParam, lParam);
}

auto CHexDlgTemplMgr::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)->HBRUSH
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

	m_menuTree.DestroyMenu();
	m_menuHdr.DestroyMenu();
	m_pAppliedCurr = nullptr;
	m_pVecFieldsCurr = nullptr;
	m_hTreeCurrParent = nullptr;
	DeleteObject(m_hBITMAPMin);
	DeleteObject(m_hBITMAPMax);
	m_u64Flags = { };
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
	m_menuHdr.CreatePopupMenu();
	m_menuHdr.AppendMenuW(MF_STRING, static_cast<int>(IDM_LISTAPPLIED_HDR_TYPE), L"Type");
	m_menuHdr.CheckMenuItem(static_cast<int>(IDM_LISTAPPLIED_HDR_TYPE), MF_CHECKED | MF_BYCOMMAND);
	m_menuHdr.AppendMenuW(MF_STRING, static_cast<int>(IDM_LISTAPPLIED_HDR_NAME), L"Name");
	m_menuHdr.CheckMenuItem(static_cast<int>(IDM_LISTAPPLIED_HDR_NAME), MF_CHECKED | MF_BYCOMMAND);
	m_menuHdr.AppendMenuW(MF_STRING, static_cast<int>(IDM_LISTAPPLIED_HDR_OFFSET), L"Offset");
	m_menuHdr.CheckMenuItem(static_cast<int>(IDM_LISTAPPLIED_HDR_OFFSET), MF_CHECKED | MF_BYCOMMAND);
	m_menuHdr.AppendMenuW(MF_STRING, static_cast<int>(IDM_LISTAPPLIED_HDR_SIZE), L"Size");
	m_menuHdr.CheckMenuItem(static_cast<int>(IDM_LISTAPPLIED_HDR_SIZE), MF_CHECKED | MF_BYCOMMAND);
	m_menuHdr.AppendMenuW(MF_STRING, static_cast<int>(IDM_LISTAPPLIED_HDR_DATA), L"Data");
	m_menuHdr.CheckMenuItem(static_cast<int>(IDM_LISTAPPLIED_HDR_DATA), MF_CHECKED | MF_BYCOMMAND);
	m_menuHdr.AppendMenuW(MF_STRING, static_cast<int>(IDM_LISTAPPLIED_HDR_ENDIANNESS), L"Endianness");
	m_menuHdr.CheckMenuItem(static_cast<int>(IDM_LISTAPPLIED_HDR_ENDIANNESS), MF_CHECKED | MF_BYCOMMAND);
	m_menuHdr.AppendMenuW(MF_STRING, static_cast<int>(IDM_LISTAPPLIED_HDR_COLORS), L"Colors");
	m_menuHdr.CheckMenuItem(static_cast<int>(IDM_LISTAPPLIED_HDR_COLORS), MF_CHECKED | MF_BYCOMMAND);

	m_menuTree.CreatePopupMenu();
	m_menuTree.AppendMenuW(MF_STRING, static_cast<UINT_PTR>(EMenuID::IDM_TREEAPPLIED_DISAPPLY), L"Disapply template");
	m_menuTree.AppendMenuW(MF_STRING, static_cast<UINT_PTR>(EMenuID::IDM_TREEAPPLIED_DISAPPLYALL), L"Disapply all");

	m_editOffset.SetWindowTextW(L"0x0");
	m_btnShowTT.SetCheck(IsTooltips());
	m_btnHglSel.SetCheck(IsHglSel());
	m_btnHex.SetCheck(IsShowAsHex());

	m_hCurResize = static_cast<HCURSOR>(LoadImageW(nullptr, IDC_SIZEWE, IMAGE_CURSOR, 0, 0, LR_SHARED));
	m_hCurArrow = static_cast<HCURSOR>(LoadImageW(nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED));

	CRect rcTree;
	m_treeApplied.GetWindowRect(rcTree);
	ScreenToClient(rcTree);
	m_iDynLayoutMinY = rcTree.top + 5;
	EnableDynamicLayoutHelper(true);
	::SetWindowSubclass(m_treeApplied, TreeSubclassProc, 0, 0);
	SetDlgButtonsState();

	for (const auto& pTemplate : m_vecTemplates) {
		OnTemplateLoadUnload(pTemplate->iTemplateID, true);
	}

	CRect rcWnd;
	m_btnMin.GetWindowRect(rcWnd);
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
	m_btnMin.SetBitmap(m_hBITMAPMin); //Set the min arrow bitmap to the min-max checkbox.

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
	m_treeApplied.Expand(hItem, TVE_EXPAND);

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
		{ type_bool, L"bool" }, { type_int8, L"int8" }, { type_uint8, L"uint8" },
		{ type_int16, L"int16" }, { type_uint16, L"uint16" }, { type_int32, L"int32" },
		{ type_uint32, L"uint32" }, { type_int64, L"int64" }, { type_uint64, L"uint64" },
		{ type_float, L"float" }, { type_double, L"double" }, { type_time32, L"time32_t" },
		{ type_time64, L"time64_t" }, { type_filetime, L"FILETIME" }, { type_systemtime, L"SYSTEMTIME" },
		{ type_guid, L"GUID" }
	};

	switch (pItem->iSubItem) {
	case m_iIDListApplFieldType: //Type.
		if (pField->eType == type_custom) {
			const auto& refVecCT = m_pAppliedCurr->pTemplate->vecCustomType;
			if (const auto iter = std::find_if(refVecCT.begin(), refVecCT.end(),
				[uTypeID = pField->uTypeID](const HEXCUSTOMTYPE& ref) {
					return ref.uTypeID == uTypeID; }); iter != refVecCT.end()) {
				pItem->pszText = const_cast<LPWSTR>(iter->wstrTypeName.data());
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
			{
				const auto bData = GetIHexTData<BYTE>(*m_pHexCtrl, ullOffset);
				*std::vformat_to(pItem->pszText, wsvFmt, std::make_wformat_args(bData)) = L'\0';
			}
			break;
			case 2:
			{
				auto wData = GetIHexTData<WORD>(*m_pHexCtrl, ullOffset);
				if (fShouldSwap) {
					wData = ByteSwap(wData);
				}
				*std::vformat_to(pItem->pszText, wsvFmt, std::make_wformat_args(wData)) = L'\0';
			}
			break;
			case 4:
			{
				auto dwData = GetIHexTData<DWORD>(*m_pHexCtrl, ullOffset);
				if (fShouldSwap) {
					dwData = ByteSwap(dwData);
				}
				*std::vformat_to(pItem->pszText, wsvFmt, std::make_wformat_args(dwData)) = L'\0';
			}
			break;
			case 8:
			{
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
			ShowListDataBool(pItem->pszText, GetIHexTData<std::uint8_t>(*m_pHexCtrl, ullOffset));
			break;
		case type_int8:
			ShowListDataNUMBER(pItem->pszText, GetIHexTData<std::int8_t>(*m_pHexCtrl, ullOffset), false);
			break;
		case type_uint8:
			ShowListDataNUMBER(pItem->pszText, GetIHexTData<std::uint8_t>(*m_pHexCtrl, ullOffset), false);
			break;
		case type_int16:
			ShowListDataNUMBER(pItem->pszText, GetIHexTData<std::int16_t>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_uint16:
			ShowListDataNUMBER(pItem->pszText, GetIHexTData<std::uint16_t>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_int32:
			ShowListDataNUMBER(pItem->pszText, GetIHexTData<std::int32_t>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_uint32:
			ShowListDataNUMBER(pItem->pszText, GetIHexTData<std::uint32_t>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_int64:
			ShowListDataNUMBER(pItem->pszText, GetIHexTData<std::int64_t>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_uint64:
			ShowListDataNUMBER(pItem->pszText, GetIHexTData<std::uint64_t>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_float:
			ShowListDataNUMBER(pItem->pszText, GetIHexTData<float>(*m_pHexCtrl, ullOffset), fShouldSwap);
			break;
		case type_double:
			ShowListDataNUMBER(pItem->pszText, GetIHexTData<double>(*m_pHexCtrl, ullOffset), fShouldSwap);
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
		pLCI->stClr.clrBk = pField->stClr.clrBk;
		pLCI->stClr.clrText = pField->stClr.clrText;
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
	m_menuHdr.TrackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON, pt.x, pt.y, this);
}

void CHexDlgTemplMgr::OnListItemChanged(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	const auto iItem = pNMI->iItem;
	if (iItem < 0 || m_fListGuardEvent)
		return;

	m_treeApplied.SelectItem(TreeItemFromListItem(iItem));
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
	static constexpr auto iMinTreeWidth = 100;          //Tree control minimum allowed width.

	CRect rcList;
	m_pListApplied->GetWindowRect(rcList);
	ScreenToClient(rcList);

	if (m_fLMDownResize) {
		CRect rcTree;
		m_treeApplied.GetWindowRect(rcTree);
		ScreenToClient(rcTree);
		rcTree.right = point.x - iWidthBetweenTreeAndList;
		rcList.left = point.x;
		if (rcTree.Width() >= iMinTreeWidth) {
			auto hdwp = BeginDeferWindowPos(2); //Simultaneously resizing list and tree.
			hdwp = DeferWindowPos(hdwp, m_treeApplied, nullptr, rcTree.left, rcTree.top,
				rcTree.Width(), rcTree.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
			hdwp = DeferWindowPos(hdwp, m_pListApplied->m_hWnd, nullptr, rcList.left, rcList.top,
				rcList.Width(), rcList.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
			EndDeferWindowPos(hdwp);
		}
	}
	else {
		if (const CRect rcSplitter(rcList.left - iResAreaHalfWidth, rcList.top, rcList.left + iResAreaHalfWidth, rcList.bottom);
			rcSplitter.PtInRect(point)) {
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
		const auto pTemplate = GetTemplate(iTemplateID);
		if (pTemplate == nullptr)
			return;

		const auto iIndex = m_comboTemplates.AddString(pTemplate->wstrName.data());
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

	if (m_treeApplied.GetParentItem(pItem->hItem) == nullptr) { //Root node.
		const auto pTemplApplied = reinterpret_cast<PCTEMPLAPPLIED>(m_treeApplied.GetItemData(pItem->hItem));
		StringCchCopyW(pItem->pszText, pItem->cchTextMax, pTemplApplied->pTemplate->wstrName.data());
	}
	else {
		const auto pTemplField = reinterpret_cast<PCHEXTEMPLFIELD>(m_treeApplied.GetItemData(pItem->hItem));
		StringCchCopyW(pItem->pszText, pItem->cchTextMax, pTemplField->wstrName.data());
	}
}

void CHexDlgTemplMgr::OnTreeItemChanged(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pTree = reinterpret_cast<LPNMTREEVIEWW>(pNMHDR);
	const auto pItem = &pTree->itemNew;

	//Item was changed by m_treeApplied.SelectItem(...) or by m_treeApplied.DeleteItem(...);
	if (pTree->action == TVC_UNKNOWN) {
		if (m_treeApplied.GetParentItem(pItem->hItem) != nullptr) {
			const auto dwItemData = m_treeApplied.GetItemData(pItem->hItem);
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
	m_pListApplied->SetItemState(-1, 0, LVIS_SELECTED | LVIS_FOCUSED); //Deselect all items.

	if (m_treeApplied.GetParentItem(pItem->hItem) == nullptr) { //Root item.
		fRootNodeClick = true;
		pVecCurrFields = &m_pAppliedCurr->pTemplate->vecFields; //On Root item click, set pVecCurrFields to Template's main vecFields.
		m_hTreeCurrParent = pItem->hItem;
	}
	else { //Child items.
		pFieldCurr = reinterpret_cast<PCHEXTEMPLFIELD>(m_treeApplied.GetItemData(pItem->hItem));
		if (pFieldCurr->pFieldParent == nullptr) {
			if (pFieldCurr->vecNested.empty()) { //On first level child items, set pVecCurrFields to Template's main vecFields.
				pVecCurrFields = &m_pAppliedCurr->pTemplate->vecFields;
				m_hTreeCurrParent = m_treeApplied.GetParentItem(pItem->hItem);
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
				m_hTreeCurrParent = m_treeApplied.GetParentItem(pItem->hItem);
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
		m_pListApplied->SetItemCountEx(static_cast<int>(m_pVecFieldsCurr->size()), LVSICF_NOSCROLL);
	}

	UpdateStaticText();

	if (!fRootNodeClick) {
		int iIndexHighlight { 0 }; //Index to highlight in the list.
		auto hChild = m_treeApplied.GetNextItem(m_treeApplied.GetParentItem(pItem->hItem), TVGN_CHILD);
		while (hChild != pItem->hItem) { //Checking for currently selected item in the tree.
			++iIndexHighlight;
			hChild = m_treeApplied.GetNextSiblingItem(hChild);
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
	m_treeApplied.ScreenToClient(&ptTree);
	const auto hTreeItem = m_treeApplied.HitTest(ptTree);
	const auto fHitTest = hTreeItem != nullptr;
	const auto fHasApplied = HasApplied();

	if (hTreeItem != nullptr) {
		m_treeApplied.SelectItem(hTreeItem);
	}
	m_menuTree.EnableMenuItem(static_cast<UINT>(EMenuID::IDM_TREEAPPLIED_DISAPPLY),
		(fHasApplied && fHitTest ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
	m_menuTree.EnableMenuItem(static_cast<UINT>(EMenuID::IDM_TREEAPPLIED_DISAPPLYALL), (fHasApplied ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
	m_menuTree.TrackPopupMenuEx(TPM_LEFTALIGN, pt.x, pt.y, this, nullptr);
}

void CHexDlgTemplMgr::RandomizeTemplateColors(int iTemplateID)
{
	const auto pTemplate = GetTemplate(iTemplateID);
	if (pTemplate == nullptr)
		return;

	std::random_device rd;
	std::mt19937 gen(rd());
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
	m_pListApplied->RedrawWindow();
	m_pHexCtrl->Redraw();
}

void CHexDlgTemplMgr::RemoveNodesWithTemplateID(int iTemplateID)
{
	std::vector<HTREEITEM> vecToRemove;
	if (auto hItem = m_treeApplied.GetRootItem(); hItem != nullptr) {
		while (hItem != nullptr) {
			const auto pApplied = reinterpret_cast<PCTEMPLAPPLIED>(m_treeApplied.GetItemData(hItem));
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

			hItem = m_treeApplied.GetNextItem(hItem, TVGN_NEXT); //Get next Root sibling item.
		}
		for (const auto item : vecToRemove) {
			m_treeApplied.DeleteItem(item);
		}
	}
}

void CHexDlgTemplMgr::RemoveNodeWithAppliedID(int iAppliedID)
{
	auto hItem = m_treeApplied.GetRootItem();
	while (hItem != nullptr) {
		const auto pApplied = reinterpret_cast<PCTEMPLAPPLIED>(m_treeApplied.GetItemData(hItem));
		if (pApplied->iAppliedID == iAppliedID) {
			if (m_pAppliedCurr == pApplied) {
				m_pListApplied->SetItemCountEx(0);
				m_pListApplied->RedrawWindow();
				m_pAppliedCurr = nullptr;
				m_pVecFieldsCurr = nullptr;
				m_hTreeCurrParent = nullptr;
				UpdateStaticText();
			}
			m_treeApplied.DeleteItem(hItem);
			break;
		}
		hItem = m_treeApplied.GetNextItem(hItem, TVGN_NEXT); //Get next Root sibling item.
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

template<typename T> requires TSize1248<T>
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
		const auto optSysTime = StringToSystemTime(pwszText, m_dwDateFormat);
		if (!optSysTime)
			return false;

		//Unix times are signed but value before 1st January 1970 is not considered valid.
		//This is apparently because early compilers didn't support unsigned types. _mktime32() has the same limit.
		if (optSysTime->wYear < 1970)
			return false;

		FILETIME ftTime;
		if (!SystemTimeToFileTime(&*optSysTime, &ftTime))
			return false;

		//Convert ticks to seconds and adjust epoch.
		LARGE_INTEGER lTicks { .LowPart { ftTime.dwLowDateTime }, .HighPart { static_cast<LONG>(ftTime.dwHighDateTime) } };
		lTicks.QuadPart /= g_uFTTicksPerSec;
		lTicks.QuadPart -= g_ullUnixEpochDiff;
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
		const auto optSysTime = StringToSystemTime(pwszText, m_dwDateFormat);
		if (!optSysTime)
			return false;

		//Unix times are signed but value before 1st January 1970 is not considered valid.
		//This is apparently because early compilers didn't support unsigned types. _mktime64() has the same limit.
		if (optSysTime->wYear < 1970)
			return false;

		FILETIME ftTime;
		if (!SystemTimeToFileTime(&*optSysTime, &ftTime))
			return false;

		//Convert ticks to seconds and adjust epoch.
		LARGE_INTEGER lTicks { .LowPart { ftTime.dwLowDateTime }, .HighPart { static_cast<LONG>(ftTime.dwHighDateTime) } };
		lTicks.QuadPart /= g_uFTTicksPerSec;
		lTicks.QuadPart -= g_ullUnixEpochDiff;

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
		const auto optFileTime = StringToFileTime(pwszText, m_dwDateFormat);
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

template<TSize1248 T>
void CHexDlgTemplMgr::SetTData(T tData, ULONGLONG ullOffset, bool fShouldSwap)const
{
	if (fShouldSwap) {
		tData = ByteSwap(tData);
	}

	SetIHexTData(*m_pHexCtrl, ullOffset, tData);
}

void CHexDlgTemplMgr::ShowListDataBool(LPWSTR pwsz, std::uint8_t u8Data) const
{
	const auto fBool = static_cast<bool>(u8Data);
	*std::vformat_to(pwsz, IsShowAsHex() ? L"0x{0:02X}" : L"{1}",
		std::make_wformat_args(u8Data, fBool)) = L'\0';
}

template<typename T> requires TSize1248<T>
void CHexDlgTemplMgr::ShowListDataNUMBER(LPWSTR pwsz, T tData, bool fShouldSwap)const
{
	if (fShouldSwap) {
		tData = ByteSwap(tData);
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

	//Add seconds from epoch time.
	LARGE_INTEGER Time { .LowPart { g_ulFileTime1970_LOW }, .HighPart { g_ulFileTime1970_HIGH } };
	Time.QuadPart += static_cast<LONGLONG>(lTime32) * g_uFTTicksPerSec;

	//Convert to FILETIME.
	const FILETIME ftTime { .dwLowDateTime { Time.LowPart }, .dwHighDateTime = static_cast<DWORD>(Time.HighPart) };
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

	//Add seconds from epoch time.
	LARGE_INTEGER Time { .LowPart { g_ulFileTime1970_LOW }, .HighPart { g_ulFileTime1970_HIGH } };
	Time.QuadPart += llTime64 * g_uFTTicksPerSec;

	//Convert to FILETIME.
	const FILETIME ftTime { .dwLowDateTime { Time.LowPart }, .dwHighDateTime { static_cast<DWORD>(Time.HighPart) } };
	*std::format_to(pwsz, L"{}", FileTimeToString(ftTime, m_dwDateFormat, m_wchDateSepar)) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataFILETIME(LPWSTR pwsz, FILETIME stFTime, bool fShouldSwap)const
{
	if (fShouldSwap) {
		stFTime = ByteSwap(stFTime);
	}

	const auto ui64 = std::bit_cast<std::uint64_t>(stFTime);
	const auto wstrTime = FileTimeToString(stFTime, m_dwDateFormat, m_wchDateSepar);
	*std::vformat_to(pwsz, IsShowAsHex() ? L"0x{0:016X}" : L"{}", std::make_wformat_args(ui64, wstrTime)) = L'\0';
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
	auto hChildItem = m_treeApplied.GetNextItem(m_hTreeCurrParent, TVGN_CHILD);
	for (auto iterListItems = 0; iterListItems < iListItem; ++iterListItems) {
		hChildItem = m_treeApplied.GetNextSiblingItem(hChildItem);
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
		if (m_pHexCtrl->IsDataSet()) {
			m_pHexCtrl->Redraw();
		}
	}

	std::erase_if(m_vecTemplates, [iTemplateID](const std::unique_ptr<HEXTEMPLATE>& pData) {
		return pData->iTemplateID == iTemplateID; }); //Remove template itself.

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

auto CHexDlgTemplMgr::CloneTemplate(PCHEXTEMPLATE pTemplate)->std::unique_ptr<HEXTEMPLATE>
{
	assert(pTemplate != nullptr);
	if (pTemplate == nullptr)
		return { };

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

bool CHexDlgTemplMgr::JSONParseFields(const IterJSONMember iterFieldsArray, HexVecFields& refVecFields,
	const FIELDSDEFPROPS& refDefault, UmapCustomTypes& umapCustomT, int* pOffset)
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

	const auto lmbTotalSize = [](const HexVecFields& refVecFields)->int { //Counts total size of all Fields in HexVecFields, recursively.
		const auto _lmbTotalSize = [](const auto& lmbSelf, const HexVecFields& refVecFields)->int {
			return std::accumulate(refVecFields.begin(), refVecFields.end(), 0,
				[&lmbSelf](auto ullTotal, const std::unique_ptr<HEXTEMPLFIELD>& pField) {
					if (!pField->vecNested.empty()) {
						return ullTotal + lmbSelf(lmbSelf, pField->vecNested);
					}
					return ullTotal + pField->iSize; });
			};
		return _lmbTotalSize(_lmbTotalSize, refVecFields);
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

		const auto& pNewField = refVecFields.emplace_back(std::make_unique<HEXTEMPLFIELD>());
		pNewField->wstrName = wstrNameField;
		pNewField->pFieldParent = refDefault.pFieldParent;
		pNewField->stClr.clrBk = JSONColors(*pField, "clrBk").value_or(refDefault.stClr.clrBk);
		pNewField->stClr.clrText = JSONColors(*pField, "clrText").value_or(refDefault.stClr.clrText);
		pNewField->fBigEndian = JSONEndianness(*pField).value_or(refDefault.fBigEndian);
		pNewField->iOffset = *pOffset;
		pNewField->eType = type_custom;

		if (const auto iterNestedFields = pField->FindMember("Fields");
			iterNestedFields != pField->MemberEnd()) {
			if (!iterNestedFields->value.IsArray()) {
				return false; //Each "Fields" must be an Array.
			}

			//Setting defaults for the next nested fields.
			const FIELDSDEFPROPS stDefsNested { .stClr { pNewField->stClr }, .pTemplate { refDefault.pTemplate },
				.pFieldParent { pNewField.get() }, .fBigEndian { pNewField->fBigEndian } };

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
					const auto& refVecCTypes = refDefault.pTemplate->vecCustomType;
					const auto iterVecCT = std::find_if(refVecCTypes.begin(), refVecCTypes.end(),
						[=](const HEXCUSTOMTYPE& ref) { return ref.wstrTypeName == StrToWstr(iterType->value.GetString()); });
					if (iterVecCT == refVecCTypes.end()) {
						return false; //Undefined/unknown "type".
					}

					pNewField->uTypeID = iterVecCT->uTypeID; //Custom type ID.
					pNewField->eType = type_custom;
					const auto lmbCopyCustomType = [&refDefault]
					(const HexVecFields& refVecCustomFields, const HexPtrField& pField, int& iOffset)->void {
						const auto _lmbCustomTypeCopy = [&refDefault]
						(const auto& lmbSelf, const HexVecFields& refVecCustomFields, const HexPtrField& pField, int& iOffset)->void {
							for (const auto& pCustomField : refVecCustomFields) {
								const auto& pNewField = pField->vecNested.emplace_back(std::make_unique<HEXTEMPLFIELD>());
								const auto& refCFClr = pCustomField->stClr;
								pNewField->pFieldParent = pField.get();
								pNewField->stClr.clrBk = refCFClr.clrBk == -1 ? pField->stClr.clrBk : refCFClr.clrBk;
								pNewField->stClr.clrText = refCFClr.clrText == -1 ? pField->stClr.clrText : refCFClr.clrText;
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
						_lmbCustomTypeCopy(_lmbCustomTypeCopy, refVecCustomFields, pField, iOffset);
						};

					auto iOffsetCustomType = *pOffset;
					if (iArraySize <= 1) {
						lmbCopyCustomType(umapCustomT[iterVecCT->uTypeID], pNewField, iOffsetCustomType);
					}
					else { //Creating array of Custom Types.
						for (int iArrIndex = 0; iArrIndex < iArraySize; ++iArrIndex) {
							const auto& pFieldArray = pNewField->vecNested.emplace_back(std::make_unique<HEXTEMPLFIELD>());
							pFieldArray->pFieldParent = pNewField.get();
							pFieldArray->stClr = pNewField->stClr;
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
					const auto& pFieldArray = pNewField->vecNested.emplace_back(std::make_unique<HEXTEMPLFIELD>());
					pFieldArray->pFieldParent = pNewField.get();
					pFieldArray->stClr = pNewField->stClr;
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

	const auto R = *stn::StrToUInt32(sv.substr(1, 2), 16);
	const auto G = *stn::StrToUInt32(sv.substr(3, 2), 16);
	const auto B = *stn::StrToUInt32(sv.substr(5, 2), 16);

	return RGB(R, G, B);
}

LRESULT CHexDlgTemplMgr::TreeSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/,
	DWORD_PTR /*dwRefData*/)
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

HEXCTRLAPI auto __cdecl HEXCTRL::IHexTemplates::LoadFromFile(const wchar_t* pFilePath)->std::unique_ptr<HEXCTRL::HEXTEMPLATE>
{
	assert(pFilePath != nullptr);
	if (pFilePath == nullptr) {
		return { };
	}

	std::ifstream ifs(pFilePath);
	if (!ifs.is_open()) {
		return { };
	}

	rapidjson::IStreamWrapper isw { ifs };
	rapidjson::Document docJSON;
	docJSON.ParseStream(isw);
	if (docJSON.IsNull())
		return { };

	auto pTemplateUtr = std::make_unique<HEXTEMPLATE>();
	const auto pTemplate = pTemplateUtr.get();
	const auto itTName = docJSON.FindMember("TemplateName");
	if (itTName == docJSON.MemberEnd() || !itTName->value.IsString())
		return { }; //Template must have a string type name.

	pTemplate->wstrName = StrToWstr(itTName->value.GetString());

	CHexDlgTemplMgr::UmapCustomTypes umapCT;
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

			const auto clrBk = CHexDlgTemplMgr::JSONColors(*pCustomType, "clrBk").value_or(-1);
			const auto clrText = CHexDlgTemplMgr::JSONColors(*pCustomType, "clrText").value_or(-1);
			const auto fBigEndian = CHexDlgTemplMgr::JSONEndianness(*pCustomType).value_or(false);

			const auto iterFieldsArray = pCustomType->FindMember("Fields");
			if (iterFieldsArray == pCustomType->MemberEnd() || !iterFieldsArray->value.IsArray()) {
				return { }; //Each "Fields" must be an array.
			}

			umapCT.try_emplace(uCustomTypeID, HexVecFields { });
			const CHexDlgTemplMgr::FIELDSDEFPROPS stDefTypes { .stClr { clrBk, clrText }, .pTemplate { pTemplate },
				.fBigEndian { fBigEndian } };
			if (!CHexDlgTemplMgr::JSONParseFields(iterFieldsArray, umapCT[uCustomTypeID], stDefTypes, umapCT)) {
				return { }; //Something went wrong during template parsing.
			}
			pTemplate->vecCustomType.emplace_back(std::move(wstrTypeName), uCustomTypeID);
		}
	}

	const auto objData = docJSON.FindMember("Data");
	if (objData == docJSON.MemberEnd() || !objData->value.IsObject())
		return { }; //No "Data" object member in the template.

	const auto clrBk = CHexDlgTemplMgr::JSONColors(objData->value, "clrBk").value_or(-1);
	const auto clrText = CHexDlgTemplMgr::JSONColors(objData->value, "clrText").value_or(-1);
	const auto fBigEndian = CHexDlgTemplMgr::JSONEndianness(objData->value).value_or(false);
	const CHexDlgTemplMgr::FIELDSDEFPROPS stDefFields { .stClr { clrBk, clrText }, .pTemplate { pTemplate },
		.fBigEndian { fBigEndian } };

	const auto iterFieldsArray = objData->value.FindMember("Fields");
	auto& refVecFields = pTemplate->vecFields;
	if (!CHexDlgTemplMgr::JSONParseFields(iterFieldsArray, refVecFields, stDefFields, umapCT)) {
		return { }; //Something went wrong during template parsing.
	}

	pTemplate->iSizeTotal = std::accumulate(refVecFields.begin(), refVecFields.end(), 0,
		[](auto iTotal, const std::unique_ptr<HEXTEMPLFIELD>& pData) { return iTotal + pData->iSize; });

	return pTemplateUtr;
}