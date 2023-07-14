/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "../../res/HexCtrlRes.h"
#include "../HexUtility.h"
#include "CHexDlgFillData.h"
#include <cassert>

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

namespace HEXCTRL::INTERNAL
{
	enum class CHexDlgFillData::EFillType : std::uint8_t {
		FILL_HEX, FILL_ASCII, FILL_WCHAR, FILL_RAND_MT19937, FILL_RAND_FAST
	};
};

BEGIN_MESSAGE_MAP(CHexDlgFillData, CDialogEx)
	ON_WM_ACTIVATE()
END_MESSAGE_MAP()

void CHexDlgFillData::Create(CWnd* pParent, IHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	assert(pParent);
	if (pHexCtrl == nullptr)
		return;

	m_pHexCtrl = pHexCtrl;
	CDialogEx::Create(IDD_HEXCTRL_FILLDATA, pParent);
}

void CHexDlgFillData::OnActivate(UINT nState, CWnd* /*pWndOther*/, BOOL /*bMinimized*/)
{
	if (m_pHexCtrl == nullptr)
		return;

	if (nState == WA_ACTIVE || nState == WA_CLICKACTIVE) {
		if (m_pHexCtrl->IsCreated() && m_pHexCtrl->IsDataSet()) {
			const auto fSelection { m_pHexCtrl->HasSelection() };
			CheckRadioButton(IDC_HEXCTRL_FILLDATA_RAD_ALL, IDC_HEXCTRL_FILLDATA_RAD_SEL,
				fSelection ? IDC_HEXCTRL_FILLDATA_RAD_SEL : IDC_HEXCTRL_FILLDATA_RAD_ALL);
			GetDlgItem(IDC_HEXCTRL_FILLDATA_RAD_SEL)->EnableWindow(fSelection);
		}
	}
}


//Private methods.

void CHexDlgFillData::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HEXCTRL_FILLDATA_COMBO_TYPE, m_stComboType);
	DDX_Control(pDX, IDC_HEXCTRL_FILLDATA_COMBO_DATA, m_stComboData);
}

auto CHexDlgFillData::GetFillType()const->CHexDlgFillData::EFillType
{
	return static_cast<EFillType>(m_stComboType.GetItemData(m_stComboType.GetCurSel()));
}

void CHexDlgFillData::OnCancel()
{
	static_cast<CDialogEx*>(GetParentOwner())->EndDialog(IDCANCEL);
}

BOOL CHexDlgFillData::OnCommand(WPARAM wParam, LPARAM lParam)
{
	using enum EFillType;
	switch (LOWORD(wParam)) {
	case IDC_HEXCTRL_FILLDATA_COMBO_TYPE:
	{
		const auto enFillType = GetFillType();
		m_stComboData.EnableWindow(enFillType != FILL_RAND_MT19937 && enFillType != FILL_RAND_FAST);
	}
	return TRUE;
	default:
		break;
	}

	return CDialogEx::OnCommand(wParam, lParam);
}

BOOL CHexDlgFillData::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	auto iIndex = m_stComboType.AddString(L"Hex Values");
	m_stComboType.SetItemData(iIndex, static_cast<DWORD_PTR>(EFillType::FILL_HEX));
	m_stComboType.SetCurSel(iIndex);
	iIndex = m_stComboType.AddString(L"ASCII Text");
	m_stComboType.SetItemData(iIndex, static_cast<DWORD_PTR>(EFillType::FILL_ASCII));
	iIndex = m_stComboType.AddString(L"UTF-16 Text");
	m_stComboType.SetItemData(iIndex, static_cast<DWORD_PTR>(EFillType::FILL_WCHAR));
	iIndex = m_stComboType.AddString(L"Random Data (MT19937)");
	m_stComboType.SetItemData(iIndex, static_cast<DWORD_PTR>(EFillType::FILL_RAND_MT19937));
	iIndex = m_stComboType.AddString(L"Pseudo Random Data (fast, but less secure)");
	m_stComboType.SetItemData(iIndex, static_cast<DWORD_PTR>(EFillType::FILL_RAND_FAST));

	CheckRadioButton(IDC_HEXCTRL_FILLDATA_RAD_ALL, IDC_HEXCTRL_FILLDATA_RAD_SEL, IDC_HEXCTRL_FILLDATA_RAD_ALL);
	m_stComboData.LimitText(512); //Max characters of combo-box.

	return TRUE;
}

void CHexDlgFillData::OnOK()
{
	if (!m_pHexCtrl->IsCreated() || !m_pHexCtrl->IsDataSet())
		return;

	wchar_t pwszComboText[MAX_PATH * 2];
	if (m_stComboData.IsWindowEnabled() && m_stComboData.GetWindowTextW(pwszComboText, MAX_PATH) == 0) { //No text.
		MessageBoxW(L"Missing Fill data!", L"Data error!", MB_ICONERROR);
		return;
	}

	HEXMODIFY hms;
	const auto iRadioAllOrSel = GetCheckedRadioButton(IDC_HEXCTRL_FILLDATA_RAD_ALL, IDC_HEXCTRL_FILLDATA_RAD_SEL);
	if (iRadioAllOrSel == IDC_HEXCTRL_FILLDATA_RAD_ALL) {
		if (MessageBoxW(L"You are about to modify the entire data region.\r\nAre you sure?", L"Modify all data?",
			MB_YESNO | MB_ICONWARNING) == IDNO)
			return;

		hms.vecSpan.emplace_back(0, m_pHexCtrl->GetDataSize());
	}
	else {
		if (!m_pHexCtrl->HasSelection())
			return;

		hms.vecSpan = m_pHexCtrl->GetSelection();
	}

	std::wstring wstrFillWith = pwszComboText;
	std::string strFillWith; //Data holder for FILL_HEX and FILL_ASCII modes.
	switch (GetFillType()) {
	case EFillType::FILL_HEX:
	{
		auto optData = NumStrToHex(wstrFillWith);
		if (!optData) {
			MessageBoxW(L"Wrong Hex format!", L"Format Error", MB_ICONERROR);
			return;
		}
		strFillWith = std::move(*optData);
		hms.enModifyMode = EHexModifyMode::MODIFY_REPEAT;
		hms.spnData = { reinterpret_cast<const std::byte*>(strFillWith.data()), strFillWith.size() };
	}
	break;
	case EFillType::FILL_ASCII:
		strFillWith = WstrToStr(wstrFillWith);
		hms.enModifyMode = EHexModifyMode::MODIFY_REPEAT;
		hms.spnData = { reinterpret_cast<const std::byte*>(strFillWith.data()), strFillWith.size() };
		break;
	case EFillType::FILL_WCHAR:
		hms.enModifyMode = EHexModifyMode::MODIFY_REPEAT;
		hms.spnData = { reinterpret_cast<const std::byte*>(wstrFillWith.data()), wstrFillWith.size() * sizeof(WCHAR) };
		break;
	case EFillType::FILL_RAND_MT19937:
		hms.enModifyMode = EHexModifyMode::MODIFY_RAND_MT19937;
		break;
	case EFillType::FILL_RAND_FAST:
		hms.enModifyMode = EHexModifyMode::MODIFY_RAND_FAST;
		break;
	default:
		break;
	}

	//Insert wstring into ComboBox only if it's not already presented.
	if (m_stComboData.FindStringExact(0, wstrFillWith.data()) == CB_ERR) {
		//Keep max 50 strings in list.
		if (m_stComboData.GetCount() == 50) {
			m_stComboData.DeleteString(49);
		}
		m_stComboData.InsertString(0, wstrFillWith.data());
	}

	if (hms.spnData.size() > hms.vecSpan.back().ullSize) {
		MessageBoxW(L"Fill data size is bigger than the region selected for modification, please select a larger region.",
			L"Data region size error.", MB_ICONERROR);
		return;
	}

	m_pHexCtrl->ModifyData(hms);
	m_pHexCtrl->Redraw();
	::SetFocus(m_pHexCtrl->GetWindowHandle(EHexWnd::WND_MAIN));
}