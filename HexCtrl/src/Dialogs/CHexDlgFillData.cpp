/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
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
	enum class CHexDlgFillData::EFillType : WORD
	{
		FILL_HEX, FILL_ASCII, FILL_WCHAR, FILL_RANDOM
	};
};

BEGIN_MESSAGE_MAP(CHexDlgFillData, CDialogEx)
	ON_WM_ACTIVATE()
END_MESSAGE_MAP()

void CHexDlgFillData::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HEXCTRL_FILLDATA_COMBO_TYPE, m_stComboType);
	DDX_Control(pDX, IDC_HEXCTRL_FILLDATA_COMBO_DATA, m_stComboData);
}

BOOL CHexDlgFillData::Create(UINT nIDTemplate, CWnd* pParent, IHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	if (pHexCtrl == nullptr)
		return FALSE;

	m_pHexCtrl = pHexCtrl;

	return CDialogEx::Create(nIDTemplate, pParent);
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
	iIndex = m_stComboType.AddString(L"Random Data (mt19937)");
	m_stComboType.SetItemData(iIndex, static_cast<DWORD_PTR>(EFillType::FILL_RANDOM));

	CheckRadioButton(IDC_HEXCTRL_FILLDATA_RADIO_ALL, IDC_HEXCTRL_FILLDATA_RADIO_SEL, IDC_HEXCTRL_FILLDATA_RADIO_ALL);
	m_stComboData.LimitText(512); //Max characters of combo-box.

	return TRUE;
}

void CHexDlgFillData::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if (!m_pHexCtrl->IsCreated() || !m_pHexCtrl->IsDataSet())
		return;

	if (nState == WA_ACTIVE || nState == WA_CLICKACTIVE)
	{
		const auto fSelection { m_pHexCtrl->HasSelection() };
		CheckRadioButton(IDC_HEXCTRL_FILLDATA_RADIO_ALL, IDC_HEXCTRL_FILLDATA_RADIO_SEL,
			fSelection ? IDC_HEXCTRL_FILLDATA_RADIO_SEL : IDC_HEXCTRL_FILLDATA_RADIO_ALL);
		GetDlgItem(IDC_HEXCTRL_FILLDATA_RADIO_SEL)->EnableWindow(fSelection);
	}

	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
}

BOOL CHexDlgFillData::OnCommand(WPARAM wParam, LPARAM /*lParam*/)
{
	switch (LOWORD(wParam))
	{
	case IDC_HEXCTRL_FILLDATA_COMBO_TYPE:
		m_stComboData.EnableWindow(GetFillType() != EFillType::FILL_RANDOM);
		break;
	case IDOK:
		OnOK();
		break;
	case IDCANCEL:
		CDialogEx::OnCancel();
		break;
	default:
		break;
	}

	return TRUE;
}

void CHexDlgFillData::OnOK()
{
	WCHAR pwszComboText[512];
	if (m_stComboData.IsWindowEnabled() && m_stComboData.GetWindowTextW(pwszComboText, MAX_PATH) == 0) //No text.
	{
		MessageBoxW(L"Missing Fill Data!", L"Data Error!", MB_ICONERROR);
		return;
	}

	const auto iRadioAllOrSel = GetCheckedRadioButton(IDC_HEXCTRL_FILLDATA_RADIO_ALL, IDC_HEXCTRL_FILLDATA_RADIO_SEL);
	HEXMODIFY hms;
	if (iRadioAllOrSel == IDC_HEXCTRL_FILLDATA_RADIO_ALL)
	{
		if (MessageBoxW(L"You are about to modify the entire data region.\r\nAre you sure?", L"Modify All data?", MB_YESNO | MB_ICONWARNING) == IDNO)
			return;
		hms.vecOffset.emplace_back(HEXOFFSET { 0, m_pHexCtrl->GetDataSize() });
	}
	else
		hms.vecOffset = m_pHexCtrl->GetSelection();

	std::wstring wstrComboText = pwszComboText;
	std::string strToFill = wstr2str(wstrComboText);
	switch (GetFillType())
	{
	case EFillType::FILL_HEX:
		if (!str2hex(strToFill, strToFill))
		{
			MessageBoxW(L"Wrong Hex format!", L"Format Error", MB_ICONERROR);
			return;
		}
		hms.spnData = { reinterpret_cast<std::byte*>(strToFill.data()), strToFill.size() };
		hms.enModifyMode = EHexModifyMode::MODIFY_REPEAT;
		break;
	case EFillType::FILL_ASCII:
		hms.spnData = { reinterpret_cast<std::byte*>(strToFill.data()), strToFill.size() };
		hms.enModifyMode = EHexModifyMode::MODIFY_REPEAT;
		break;
	case EFillType::FILL_WCHAR:
		hms.spnData = { reinterpret_cast<std::byte*>(wstrComboText.data()), wstrComboText.size() * sizeof(WCHAR) };
		hms.enModifyMode = EHexModifyMode::MODIFY_REPEAT;
		break;
	case EFillType::FILL_RANDOM:
		hms.enModifyMode = EHexModifyMode::MODIFY_RANDOM;
		break;
	default:
		break;
	}

	//Insert wstring into ComboBox only if it's not already presented.
	if (m_stComboData.FindStringExact(0, wstrComboText.data()) == CB_ERR)
	{
		//Keep max 50 strings in list.
		if (m_stComboData.GetCount() == 50)
			m_stComboData.DeleteString(49);
		m_stComboData.InsertString(0, wstrComboText.data());
	}

	m_pHexCtrl->ModifyData(hms);
	m_pHexCtrl->Redraw();
	::SetFocus(m_pHexCtrl->GetWindowHandle(EHexWnd::WND_MAIN));

	CDialogEx::OnOK();
}

CHexDlgFillData::EFillType CHexDlgFillData::GetFillType()const
{
	return static_cast<EFillType>(m_stComboType.GetItemData(m_stComboType.GetCurSel()));
}