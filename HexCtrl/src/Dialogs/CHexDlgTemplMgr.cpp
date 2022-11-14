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
#include <random>
#include <unordered_map>

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

namespace HEXCTRL::INTERNAL
{
	enum class CHexDlgTemplMgr::EMenuID : std::uint16_t {
		IDM_TREEAPPLIED_DISAPPLY = 0x8000, IDM_TREEAPPLIED_DISAPPLYALL = 0x8001,
		IDM_LISTAPPLIED_HDR_NAME = 0x8100, IDM_LISTAPPLIED_HDR_OFFSET, IDM_LISTAPPLIED_HDR_SIZE,
		IDM_LISTAPPLIED_HDR_DATA, IDM_LISTAPPLIED_HDR_ENDIANNESS, IDM_LISTAPPLIED_HDR_COLORS
	};

	constexpr auto ID_LISTAPPLIED_FIELD_TYPE { 0 };
	constexpr auto ID_LISTAPPLIED_FIELD_DATA { 4 };
	constexpr auto ID_LISTAPPLIED_FIELD_COLORS { 6 };
};

BEGIN_MESSAGE_MAP(CHexDlgTemplMgr, CDialogEx)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_BTN_LOAD, &CHexDlgTemplMgr::OnBnLoadTemplate)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_BTN_UNLOAD, &CHexDlgTemplMgr::OnBnUnloadTemplate)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_BTN_RNDCLR, &CHexDlgTemplMgr::OnBnRandomizeColors)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_BTN_APPLY, &CHexDlgTemplMgr::OnBnApply)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_CHK_TTSHOW, &CHexDlgTemplMgr::OnCheckTtShow)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_CHK_HGLSEL, &CHexDlgTemplMgr::OnCheckHglSel)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_CHK_HEX, &CHexDlgTemplMgr::OnCheckHexadecimal)
	ON_BN_CLICKED(IDC_HEXCTRL_TEMPLMGR_CHK_SWAP, &CHexDlgTemplMgr::OnCheckBigEndian)
	ON_NOTIFY(LVN_GETDISPINFOW, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListGetDispInfo)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListItemChanged)
	ON_NOTIFY(LISTEX::LISTEX_MSG_GETCOLOR, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListGetColor)
	ON_NOTIFY(LISTEX::LISTEX_MSG_EDITBEGIN, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListEditBegin)
	ON_NOTIFY(LISTEX::LISTEX_MSG_DATACHANGED, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListDataChanged)
	ON_NOTIFY(LISTEX::LISTEX_MSG_HDRRBTNUP, IDC_HEXCTRL_TEMPLMGR_LIST_APPLIED, &CHexDlgTemplMgr::OnListHdrRClick)
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
	ON_WM_ACTIVATE()
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
	m_pListApplied->InsertColumn(ID_LISTAPPLIED_FIELD_TYPE, L"Type", LVCFMT_LEFT, 85);
	m_pListApplied->InsertColumn(1, L"Name", LVCFMT_LEFT, 200);
	m_pListApplied->InsertColumn(2, L"Offset", LVCFMT_LEFT, 50);
	m_pListApplied->InsertColumn(3, L"Size", LVCFMT_LEFT, 50);
	m_pListApplied->InsertColumn(ID_LISTAPPLIED_FIELD_DATA, L"Data", LVCFMT_LEFT, 120);
	m_pListApplied->SetColumnEditable(ID_LISTAPPLIED_FIELD_DATA, true);
	m_pListApplied->InsertColumn(5, L"Endianness", LVCFMT_CENTER, 65, -1, LVCFMT_CENTER);
	m_pListApplied->InsertColumn(ID_LISTAPPLIED_FIELD_COLORS, L"Colors", LVCFMT_LEFT, 57);

	using enum EMenuID;
	m_menuHdr.CreatePopupMenu();
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

	m_stMenuTree.CreatePopupMenu();
	m_stMenuTree.AppendMenuW(MF_BYPOSITION, static_cast<UINT_PTR>(EMenuID::IDM_TREEAPPLIED_DISAPPLY), L"Disapply template");
	m_stMenuTree.AppendMenuW(MF_BYPOSITION, static_cast<UINT_PTR>(EMenuID::IDM_TREEAPPLIED_DISAPPLYALL), L"Disapply all");

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

void CHexDlgTemplMgr::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if (nState == WA_INACTIVE) {
	}
	else {
		if (m_pHexCtrl->IsCreated()) {
			const auto [dwFormat, wchSepar] = m_pHexCtrl->GetDateInfo();
			m_dwDateFormat = dwFormat;
			m_wchDateSepar = wchSepar;
		}
	}

	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
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
	case IDM_LISTAPPLIED_HDR_NAME:
	case IDM_LISTAPPLIED_HDR_OFFSET:
	case IDM_LISTAPPLIED_HDR_SIZE:
	case IDM_LISTAPPLIED_HDR_DATA:
	case IDM_LISTAPPLIED_HDR_ENDIANNESS:
	case IDM_LISTAPPLIED_HDR_COLORS:
	{
		const auto fChecked = m_menuHdr.GetMenuState(wMenuID, MF_BYCOMMAND) & MF_CHECKED;
		m_pListApplied->HideColumn(wMenuID - static_cast<int>(IDM_LISTAPPLIED_HDR_NAME), fChecked);
		m_menuHdr.CheckMenuItem(wMenuID, (fChecked ? MF_UNCHECKED : MF_CHECKED) | MF_BYCOMMAND);
	}
	break;
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

void CHexDlgTemplMgr::OnBnRandomizeColors()
{
	if (const auto iIndex = m_stComboTemplates.GetCurSel(); iIndex != CB_ERR) {
		const auto iTemplateID = static_cast<int>(m_stComboTemplates.GetItemData(iIndex));
		RandomizeTemplateColors(iTemplateID);
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
	m_fSwapEndian = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_TEMPLMGR_CHK_SWAP))->GetCheck() == BST_CHECKED;
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

	const auto iItem = static_cast<size_t>(pItem->iItem);
	const auto& refVecField = *m_pVecCurrFields;
	const auto wsvFmt = m_fShowAsHex ? L"0x{:X}" : L"{}";
	const auto fShouldSwap = refVecField[iItem]->fBigEndian == !m_fSwapEndian;

	using enum EType;
	//EType converter to actual wstring for the list.
	static const std::unordered_map<EType, const wchar_t*> umapETypeToWstr {
		{ custom_size, L"custom size" },
		{ type_bool, L"bool" }, { type_char, L"char" }, { type_uchar, L"unsigned char" },
		{ type_short, L"short" }, { type_ushort, L"unsigned short" }, { type_int, L"int" },
		{ type_uint, L"unsigned int" }, { type_ll, L"long long" }, { type_ull, L"unsigned long long" },
		{ type_float, L"float" }, { type_double, L"double" }, { type_time32, L"__time32_t" },
		{ type_time64, L"__time64_t" }, { type_filetime, L"FILETIME" }, { type_systemtime, L"SYSTEMTIME" },
		{ type_guid, L"GUID" }
	};

	switch (pItem->iSubItem) {
	case ID_LISTAPPLIED_FIELD_TYPE: //Type.
		if (refVecField[iItem]->vecNested.empty()) {
			pItem->pszText = const_cast<LPWSTR>(umapETypeToWstr.at(refVecField[iItem]->eType));
		}
		break;
	case 1: //Name.
		pItem->pszText = refVecField[iItem]->wstrName.data();
		break;
	case 2: //Offset.
		*std::vformat_to(pItem->pszText, wsvFmt, std::make_wformat_args(refVecField[iItem]->iOffset)) = L'\0';
		break;
	case 3: //Size.
		*std::vformat_to(pItem->pszText, wsvFmt, std::make_wformat_args(refVecField[iItem]->iSize)) = L'\0';
		break;
	case ID_LISTAPPLIED_FIELD_DATA: //Data.
	{
		if (!m_pHexCtrl->IsDataSet()
			|| m_pAppliedCurr->ullOffset + m_pAppliedCurr->pTemplate->iSizeTotal > m_pHexCtrl->GetDataSize()) //Size overflow check.
			break;

		const auto ullOffset = m_pAppliedCurr->ullOffset + refVecField[iItem]->iOffset;
		const auto eType = refVecField[iItem]->eType;
		switch (eType) {
		case custom_size: //If field of custom type, we cycling through the size field.
			switch (refVecField[iItem]->iSize) {
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
		}
	}
	break;
	case 5: //Endianness.
		*std::vformat_to(pItem->pszText, fShouldSwap ? L"big" : L"little",
			std::make_wformat_args()) = L'\0';
		break;
	case ID_LISTAPPLIED_FIELD_COLORS: //Colors.
		*std::format_to(pItem->pszText, L"#Text") = L'\0';
		break;
	default:
		break;
	}
}

void CHexDlgTemplMgr::OnListGetColor(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	const auto& refVecField = *m_pVecCurrFields;

	//List items with nested structs color separately with greyish bk.
	if (!refVecField[pNMI->iItem]->vecNested.empty() && pNMI->iSubItem != ID_LISTAPPLIED_FIELD_COLORS) {
		m_stCellClr.clrBk = RGB(235, 235, 235);          //Greyish.
		m_stCellClr.clrText = static_cast<COLORREF>(-1); //Default text color.
		pNMI->lParam = reinterpret_cast<LPARAM>(&m_stCellClr);
		return;
	}

	switch (pNMI->iSubItem) {
	case ID_LISTAPPLIED_FIELD_TYPE:
		if (refVecField[pNMI->iItem]->eType != EType::custom_size) {
			m_stCellClr.clrText = RGB(16, 42, 255);        //Bluish text.
			m_stCellClr.clrBk = static_cast<COLORREF>(-1); //Default bk color.
			pNMI->lParam = reinterpret_cast<LPARAM>(&m_stCellClr);
		}
		break;
	case ID_LISTAPPLIED_FIELD_COLORS:
		m_stCellClr.clrBk = refVecField[pNMI->iItem]->clrBk;
		m_stCellClr.clrText = refVecField[pNMI->iItem]->clrText;
		pNMI->lParam = reinterpret_cast<LPARAM>(&m_stCellClr);
		break;
	default:
		break;
	}
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
	if (pNMI->iSubItem != ID_LISTAPPLIED_FIELD_DATA) { //Data.
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
	if (pNMI->iSubItem != ID_LISTAPPLIED_FIELD_DATA) //Data.
		return;

	const auto iItem = pNMI->iItem;
	const auto pwszText = reinterpret_cast<LPCWSTR>(pNMI->lParam);
	const auto& refVecField = *m_pVecCurrFields;
	const auto ullOffset = m_pAppliedCurr->ullOffset + refVecField[iItem]->iOffset;
	const auto fShouldSwap = refVecField[iItem]->fBigEndian == !m_fSwapEndian;

	bool fSetRet { };
	using enum EType;
	switch (refVecField[iItem]->eType) {
	case custom_size:
		fSetRet = true;
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
	}

	if (!fSetRet) {
		MessageBoxW(L"Incorrect input data.", L"Incorrect input", MB_ICONERROR);
		return;
	}

	m_pHexCtrl->Redraw();
}

void CHexDlgTemplMgr::OnListHdrRClick(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	CPoint pt;
	GetCursorPos(&pt);
	m_menuHdr.TrackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON, pt.x, pt.y, this);
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
	m_stMenuTree.EnableMenuItem(static_cast<UINT>(EMenuID::IDM_TREEAPPLIED_DISAPPLY),
		(fHasApplied && fHitTest ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
	m_stMenuTree.EnableMenuItem(static_cast<UINT>(EMenuID::IDM_TREEAPPLIED_DISAPPLYALL), (fHasApplied ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
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

void CHexDlgTemplMgr::ShowListDataBool(LPWSTR pwsz, unsigned char uchData) const
{
	*std::vformat_to(pwsz, m_fShowAsHex ? L"0x{0:02X}" : L"{1}",
		std::make_wformat_args(uchData, static_cast<bool>(uchData))) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataChar(LPWSTR pwsz, char chData)const
{
	*std::vformat_to(pwsz, m_fShowAsHex ? L"0x{0:02X}" : L"{1}",
		std::make_wformat_args(static_cast<unsigned char>(chData), static_cast<int>(chData))) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataUChar(LPWSTR pwsz, unsigned char uchData)const
{
	*std::vformat_to(pwsz, m_fShowAsHex ? L"0x{:02X}" : L"{}", std::make_wformat_args(uchData)) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataShort(LPWSTR pwsz, short shortData, bool fShouldSwap)const
{
	if (fShouldSwap) {
		shortData = ByteSwap(shortData);
	}
	*std::vformat_to(pwsz, m_fShowAsHex ? L"0x{0:04X}" : L"{1}",
		std::make_wformat_args(static_cast<unsigned short>(shortData), shortData)) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataUShort(LPWSTR pwsz, unsigned short wData, bool fShouldSwap)const
{
	if (fShouldSwap) {
		wData = ByteSwap(wData);
	}
	*std::vformat_to(pwsz, m_fShowAsHex ? L"0x{:04X}" : L"{}", std::make_wformat_args(wData)) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataInt(LPWSTR pwsz, int intData, bool fShouldSwap)const
{
	if (fShouldSwap) {
		intData = ByteSwap(intData);
	}
	*std::vformat_to(pwsz, m_fShowAsHex ? L"0x{0:08X}" : L"{1}",
		std::make_wformat_args(static_cast<unsigned int>(intData), intData)) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataUInt(LPWSTR pwsz, unsigned int dwData, bool fShouldSwap)const
{
	if (fShouldSwap) {
		dwData = ByteSwap(dwData);
	}
	*std::vformat_to(pwsz, m_fShowAsHex ? L"0x{:08X}" : L"{}", std::make_wformat_args(dwData)) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataLL(LPWSTR pwsz, long long llData, bool fShouldSwap)const
{
	if (fShouldSwap) {
		llData = ByteSwap(llData);
	}
	*std::vformat_to(pwsz, m_fShowAsHex ? L"0x{0:16X}" : L"{1}",
		std::make_wformat_args(static_cast<unsigned long long>(llData), llData)) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataULL(LPWSTR pwsz, unsigned long long ullData, bool fShouldSwap)const
{
	if (fShouldSwap) {
		ullData = ByteSwap(ullData);
	}
	*std::vformat_to(pwsz, m_fShowAsHex ? L"0x{:016X}" : L"{}", std::make_wformat_args(ullData)) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataFloat(LPWSTR pwsz, float flData, bool fShouldSwap)const
{
	if (fShouldSwap) {
		flData = ByteSwap(flData);
	}
	*std::vformat_to(pwsz, m_fShowAsHex ? L"0x{0:08X}" : L"{1:.9e}",
		std::make_wformat_args(std::bit_cast<unsigned long>(flData), flData)) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataDouble(LPWSTR pwsz, double dblData, bool fShouldSwap)const
{
	if (fShouldSwap) {
		dblData = ByteSwap(dblData);
	}
	*std::vformat_to(pwsz, m_fShowAsHex ? L"0x{0:016X}" : L"{1:.18e}",
		std::make_wformat_args(std::bit_cast<unsigned long long>(dblData), dblData)) = L'\0';
}

void CHexDlgTemplMgr::ShowListDataTime32(LPWSTR pwsz, __time32_t lTime32, bool fShouldSwap)const
{
	if (fShouldSwap) {
		lTime32 = ByteSwap(lTime32);
	}

	if (m_fShowAsHex) {
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

	if (m_fShowAsHex) {
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
	*std::vformat_to(pwsz, m_fShowAsHex ? L"0x{0:016X}" : L"{}",
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

bool CHexDlgTemplMgr::SetDataBool(LPCWSTR pwszText, ULONGLONG ullOffset) const
{
	if (m_fShowAsHex) {
		if (const auto opt = StrToUChar(pwszText); opt) {
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
	//When m_fShowAsHex is true, we deliberately convert text to an 'unsigned' corresponding type.
	//Otherwise, setting a values like 0xFF in hex mode would be impossible, because such values
	//are bigger then `signed` type's max value, they can't be converted to it.
	if (m_fShowAsHex) {
		if (const auto opt = StrToUChar(pwszText); opt) {
			SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
			return true;
		}
	}
	else {
		if (const auto opt = StrToChar(pwszText); opt) {
			SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
			return true;
		}
	}
	return false;
}

bool CHexDlgTemplMgr::SetDataUChar(LPCWSTR pwszText, ULONGLONG ullOffset)const
{
	if (const auto opt = StrToUChar(pwszText); opt) {
		SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
		return true;
	}
	return false;
}

bool CHexDlgTemplMgr::SetDataShort(LPCWSTR pwszText, ULONGLONG ullOffset, bool fShouldSwap)const
{
	if (m_fShowAsHex) {
		if (auto opt = StrToUShort(pwszText); opt) {
			if (fShouldSwap) {
				*opt = ByteSwap(*opt);
			}
			SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
			return true;
		}
	}
	else {
		if (auto opt = StrToShort(pwszText); opt) {
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
	if (auto opt = StrToUShort(pwszText); opt) {
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
	if (m_fShowAsHex) {
		if (auto opt = StrToUInt(pwszText); opt) {
			if (fShouldSwap) {
				*opt = ByteSwap(*opt);
			}
			SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
			return true;
		}
	}
	else {
		if (auto opt = stn::StrToInt(pwszText); opt) {
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
	if (auto opt = StrToUInt(pwszText); opt) {
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
	if (m_fShowAsHex) {
		if (auto opt = StrToULL(pwszText); opt) {
			if (fShouldSwap) {
				*opt = ByteSwap(*opt);
			}
			SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
			return true;
		}
	}
	else {
		if (auto opt = StrToLL(pwszText); opt) {
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
	if (auto opt = StrToULL(pwszText); opt) {
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
	if (m_fShowAsHex) {
		if (auto opt = StrToUInt(pwszText); opt) {
			if (fShouldSwap) {
				*opt = ByteSwap(*opt);
			}
			SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
			return true;
		}
	}
	else {
		if (auto opt = StrToFloat(pwszText); opt) {
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
	if (m_fShowAsHex) {
		if (auto opt = StrToULL(pwszText); opt) {
			if (fShouldSwap) {
				*opt = ByteSwap(*opt);
			}
			SetIHexTData(*m_pHexCtrl, ullOffset, *opt);
			return true;
		}
	}
	else {
		if (auto opt = StrToDouble(pwszText); opt) {
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
	if (m_fShowAsHex) {
		if (auto opt = StrToUInt(pwszText); opt) {
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
	if (m_fShowAsHex) {
		if (auto opt = StrToULL(pwszText); opt) {
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
	if (m_fShowAsHex) {
		if (auto opt = StrToULL(pwszText); opt) {
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

	using enum EType;
	static const std::unordered_map<std::string_view, EType> umapStrToEType { //From JSON string to EType conversion.
		{ "custom", custom_size }, { "bool", type_bool },
		{ "char", type_char }, { "unsigned char", type_uchar }, { "byte", type_uchar },
		{ "short", type_short }, { "unsigned short", type_ushort }, { "WORD", type_ushort },
		{ "long", type_int }, { "unsigned long", type_uint }, { "int", type_int },
		{ "unsigned int", type_uint }, { "DWORD", type_uint }, { "long long", type_ll },
		{ "unsigned long long", type_ull }, { "QWORD", type_ull }, { "float", type_float },
		{ "double", type_double }, { "__time32_t", type_time32 }, { "__time64_t", type_time64 },
		{ "FILETIME", type_filetime }, { "SYSTEMTIME", type_systemtime }, { "GUID", type_guid } };

	static const std::unordered_map<EType, int> umapTypeToSize { //Types sizes.
		{ custom_size, -1 },
		{ type_bool, static_cast<int>(sizeof(bool)) }, { type_char, static_cast<int>(sizeof(char)) },
		{ type_uchar, static_cast<int>(sizeof(char)) }, { type_short, static_cast<int>(sizeof(short)) },
		{ type_ushort, static_cast<int>(sizeof(short)) }, { type_int, static_cast<int>(sizeof(int)) },
		{ type_uint, static_cast<int>(sizeof(int)) }, { type_ll, static_cast<int>(sizeof(long long)) },
		{ type_ull, static_cast<int>(sizeof(long long)) }, { type_float, static_cast<int>(sizeof(float)) },
		{ type_double, static_cast<int>(sizeof(double)) }, { type_time32, static_cast<int>(sizeof(__time32_t)) },
		{ type_time64, static_cast<int>(sizeof(__time64_t)) }, { type_filetime, static_cast<int>(sizeof(FILETIME)) },
		{ type_systemtime, static_cast<int>(sizeof(SYSTEMTIME)) }, { type_guid, static_cast<int>(sizeof(GUID)) }
	};

	auto pTemplateUnPtr = std::make_unique<HEXTEMPLATE>();
	const auto pTemplate = pTemplateUnPtr.get();
	auto& refFields = pTemplateUnPtr->vecFields;

	//TemplateName.
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

	//clrBk.
	COLORREF clrBkDefault { };
	if (const auto it = docJSON.FindMember("clrBk"); it != docJSON.MemberEnd() && it->value.IsString()) {
		clrBkDefault = lmbStrToRGB(it->value.GetString());
	}

	//clrText.
	COLORREF clrTextDefault { };
	if (const auto it = docJSON.FindMember("clrText"); it != docJSON.MemberEnd() && it->value.IsString()) {
		clrTextDefault = lmbStrToRGB(it->value.GetString());
	}

	//endianness.
	bool fBigEndian { false };
	if (const auto it = docJSON.FindMember("endianness"); it != docJSON.MemberEnd() && it->value.IsString()) {
		fBigEndian = std::string_view { it->value.GetString() } == "big";
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

	//Helper struct for convenient argument passing through recursive lambda.
	struct DEFPROPERTIES {
		COLORREF          clrBkDefault { };
		COLORREF          clrTextDefault { };
		PHEXTEMPLATE      pTemplate { }; //Same for all fields.
		PHEXTEMPLATEFIELD pFieldParent { };
		bool              fBigEndian { false };
	};

	DEFPROPERTIES stDefs { .clrBkDefault { clrBkDefault }, .clrTextDefault { clrTextDefault },
		.pTemplate { pTemplate }, .fBigEndian { fBigEndian } };

	using IterJSONMember = rapidjson::Value::ConstMemberIterator;
	const auto lmbParseFields = [&lmbStrToRGB, &lmbTotalSize, &stDefs]
	(const IterJSONMember iterMemberFields, VecFields& vecFields)->bool {
		const auto _lmbParse = [&lmbStrToRGB, &lmbTotalSize]
		(const auto& lmbSelf, const IterJSONMember iterMemberFields, VecFields& vecFields, int& iOffset, const DEFPROPERTIES& refDefs)->bool {
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

				refBack->pTemplate = refDefs.pTemplate;
				refBack->pFieldParent = refDefs.pFieldParent;
				refBack->iOffset = iOffset;

				if (const auto iterNestedFields = iterArrCurr->FindMember("Fields");
					iterNestedFields != iterArrCurr->MemberEnd()) {
					if (!iterNestedFields->value.IsArray()) {
						return false; //Each "Fields" must be an Array.
					}

					COLORREF clrBkDefaultNested { }; //Default color in nested struct.
					if (const auto itClrBkDefault = iterArrCurr->FindMember("clrBk");
						itClrBkDefault != iterArrCurr->MemberEnd()) {
						if (!itClrBkDefault->value.IsString()) {
							return false; //"clrBk" must be a string.
						}
						clrBkDefaultNested = lmbStrToRGB(itClrBkDefault->value.GetString());
					}
					else { clrBkDefaultNested = refDefs.clrBkDefault; }
					refBack->clrBk = clrBkDefaultNested;

					COLORREF clrTextDefaultNested { }; //Default color in nested struct.
					if (const auto itClrTextDefault = iterArrCurr->FindMember("clrText");
						itClrTextDefault != iterArrCurr->MemberEnd()) {
						if (!itClrTextDefault->value.IsString()) {
							return false; //"clrText" must be a string.
						}
						clrTextDefaultNested = lmbStrToRGB(itClrTextDefault->value.GetString());
					}
					else { clrTextDefaultNested = refDefs.clrTextDefault; }
					refBack->clrText = clrTextDefaultNested;

					bool fBigEndianDefaultNested { false };
					if (const auto itEndiannessDefaultNested = iterArrCurr->FindMember("endianness");
						itEndiannessDefaultNested != iterArrCurr->MemberEnd()) {
						if (!itEndiannessDefaultNested->value.IsString()) {
							return false; //"Endianness" must be a string.
						}
						fBigEndianDefaultNested = std::string_view { itEndiannessDefaultNested->value.GetString() } == "big";
					}
					else { fBigEndianDefaultNested = refDefs.fBigEndian; }
					refBack->fBigEndian = fBigEndianDefaultNested;

					//Setting defaults for the next nested struct.
					const DEFPROPERTIES stDefsNested { .clrBkDefault { clrBkDefaultNested }, .clrTextDefault { clrTextDefaultNested },
						.pFieldParent { refBack.get() }, .fBigEndian { fBigEndianDefaultNested } };
					//Recursion lambda for nested structs starts here.
					if (!lmbSelf(lmbSelf, iterNestedFields, refBack->vecNested, iOffset, stDefsNested)) {
						return false;
					}
					refBack->iSize = lmbTotalSize(refBack->vecNested); //Total size of all nested fields.
				}
				else {
					const auto lmbSize = [&iterArrCurr]() {
						const auto iterSize = iterArrCurr->FindMember("size");
						if (iterSize == iterArrCurr->MemberEnd()) {
							return 0; //No "size" field.
						}

						if (iterSize->value.IsInt()) {
							assert(iterSize->value.GetInt() > 0); //Size must be > 0.
							if (iterSize->value.GetInt() < 1) {
								return 0;
							}

							return iterSize->value.GetInt();
						}

						if (!iterSize->value.IsString()) {
							return 0; //"size" field neither int nor string.
						}

						const auto optInt = StrToInt(iterSize->value.GetString());
						if (!optInt) {
							return 0; //"size" field is a non-convertible string to int.
						}

						assert(*optInt > 0); //Size must be > 0.
						if (*optInt < 1) {
							return 0;
						}

						return *optInt;
					};

					auto iSize { 0 }; //Current field size, via "type" or "size" property.
					EType eType = custom_size; //Current field default "type".
					if (const auto iterType = iterArrCurr->FindMember("type");
						iterType != iterArrCurr->MemberEnd()) {
						if (!iterType->value.IsString()) {
							return false; //"type" is not a string.
						}

						const auto iterMapType = umapStrToEType.find(iterType->value.GetString());
						if (iterMapType == umapStrToEType.end()) {
							return false; //Unsupported "type".
						}

						if (iterMapType->second == custom_size) {
							if (iSize = lmbSize(); iSize == 0) {
								return false; //Property "size" was not found or data is wrong.
							}
						}
						else {
							eType = iterMapType->second;
							iSize = umapTypeToSize.at(iterMapType->second);
						}
					}
					else { //No "type" property was found.
						if (iSize = lmbSize(); iSize == 0) {
							return false; //Neither property "type" nor "size" was found.
						}
					}

					refBack->eType = eType;
					refBack->iSize = iSize;
					iOffset += iSize;

					if (const auto itClrBk = iterArrCurr->FindMember("clrBk");
						itClrBk != iterArrCurr->MemberEnd()) {
						if (!itClrBk->value.IsString()) {
							return false; //"clrBk" must be a string.
						}
						refBack->clrBk = lmbStrToRGB(itClrBk->value.GetString());
					}
					else {
						refBack->clrBk = refDefs.clrBkDefault;
					}

					if (const auto itClrText = iterArrCurr->FindMember("clrText");
						itClrText != iterArrCurr->MemberEnd()) {
						if (!itClrText->value.IsString()) {
							return false; //"clrText" must be a string.
						}
						refBack->clrText = lmbStrToRGB(itClrText->value.GetString());
					}
					else {
						refBack->clrText = refDefs.clrTextDefault;
					}

					if (const auto itEndianness = iterArrCurr->FindMember("endianness");
						itEndianness != iterArrCurr->MemberEnd()) {
						if (!itEndianness->value.IsString()) {
							return false; //"Endianness" must be a string.
						}
						refBack->fBigEndian = std::string_view { itEndianness->value.GetString() } == "big";
					}
					else {
						refBack->fBigEndian = refDefs.fBigEndian;
					}
				}
			}

			return true;
		};

		int iOffset = 0;
		return _lmbParse(_lmbParse, iterMemberFields, vecFields, iOffset, stDefs);
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

void CHexDlgTemplMgr::OnTemplateLoadUnload(bool /*fLoad*/)
{
	if (const auto pBtnApply = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_TEMPLMGR_BTN_APPLY));
		pBtnApply != nullptr) {
		pBtnApply->EnableWindow(HasTemplates());
	}
	if (const auto pBtnUnload = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_TEMPLMGR_BTN_UNLOAD));
		pBtnUnload != nullptr) {
		pBtnUnload->EnableWindow(HasTemplates());
	}
	if (const auto pBtnRandom = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_TEMPLMGR_BTN_RNDCLR));
		pBtnRandom != nullptr) {
		pBtnRandom->EnableWindow(HasTemplates());
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