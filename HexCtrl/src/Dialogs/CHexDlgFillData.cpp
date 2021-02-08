/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
****************************************************************************************/
#include "stdafx.h"
#include "../../res/HexCtrlRes.h"
#include "../Helper.h"
#include "CHexDlgFillData.h"
#include <cassert>

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

BEGIN_MESSAGE_MAP(CHexDlgFillData, CDialogEx)
	ON_WM_ACTIVATE()
END_MESSAGE_MAP()

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

	CheckRadioButton(IDC_HEXCTRL_FILLDATA_RADIO_HEX, IDC_HEXCTRL_FILLDATA_RADIO_UTF16, IDC_HEXCTRL_FILLDATA_RADIO_HEX);
	CheckRadioButton(IDC_HEXCTRL_FILLDATA_ALL, IDC_HEXCTRL_FILLDATA_SELECTION, IDC_HEXCTRL_FILLDATA_ALL);

	if (const auto pCombo = static_cast<CComboBox*>(GetDlgItem(IDC_HEXCTRL_FILLDATA_COMBO_HEXTEXT)); pCombo != nullptr)
		pCombo->LimitText(MAX_PATH); //Max characters count in combo's edit box.

	return TRUE;
}

void CHexDlgFillData::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if (nState == WA_ACTIVE || nState == WA_CLICKACTIVE)
	{
		if (m_pHexCtrl->IsCreated())
		{
			if (m_pHexCtrl->GetSelection().empty())
			{
				CheckRadioButton(IDC_HEXCTRL_FILLDATA_ALL, IDC_HEXCTRL_FILLDATA_SELECTION, IDC_HEXCTRL_FILLDATA_ALL);
				GetDlgItem(IDC_HEXCTRL_FILLDATA_SELECTION)->EnableWindow(false);
			}
			else
			{
				CheckRadioButton(IDC_HEXCTRL_FILLDATA_ALL, IDC_HEXCTRL_FILLDATA_SELECTION, IDC_HEXCTRL_FILLDATA_SELECTION);
				GetDlgItem(IDC_HEXCTRL_FILLDATA_SELECTION)->EnableWindow(true);
			}
		}
	}

	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
}

void CHexDlgFillData::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

void CHexDlgFillData::OnOK()
{
	const auto iRadioType = GetCheckedRadioButton(IDC_HEXCTRL_FILLDATA_RADIO_HEX, IDC_HEXCTRL_FILLDATA_RADIO_UTF16);
	const auto iRadioAllSelection = GetCheckedRadioButton(IDC_HEXCTRL_FILLDATA_ALL, IDC_HEXCTRL_FILLDATA_SELECTION);

	HEXMODIFY hms;
	hms.enModifyMode = EHexModifyMode::MODIFY_REPEAT;
	hms.vecSpan = m_pHexCtrl->GetSelection();
	if (iRadioAllSelection == IDC_HEXCTRL_FILLDATA_SELECTION && hms.vecSpan.empty())
	{
		//Should never happen
		assert(false);
		return;
	}
	else if (iRadioAllSelection == IDC_HEXCTRL_FILLDATA_ALL)
	{
		hms.vecSpan.clear();
		hms.vecSpan.push_back({ 0, m_pHexCtrl->GetDataSize() });

		//Prompt user
		if (MessageBoxW(L"Are you sure?", L"Fill ALL data?", MB_YESNO | MB_ICONQUESTION) == IDNO)
			return;
	}

	const auto pCombo = static_cast<CComboBox*>(GetDlgItem(IDC_HEXCTRL_FILLDATA_COMBO_HEXTEXT));
	if (!pCombo)
		return;

	WCHAR pwszComboText[MAX_PATH + 1];
	if (pCombo->GetWindowTextW(pwszComboText, MAX_PATH) == 0) //No text
		return;

	std::wstring wstrComboText = pwszComboText;
	std::string strToFill = wstr2str(wstrComboText);
	switch (iRadioType)
	{
	case IDC_HEXCTRL_FILLDATA_RADIO_HEX:
		if (!str2hex(strToFill, strToFill))
		{
			MessageBoxW(L"Wrong Hex format!", L"Format Error", MB_ICONERROR);
			return;
		}
		hms.pData = reinterpret_cast<std::byte*>(strToFill.data());
		hms.ullDataSize = strToFill.size();
		break;
	case IDC_HEXCTRL_FILLDATA_RADIO_TEXT:
		hms.pData = reinterpret_cast<std::byte*>(strToFill.data());
		hms.ullDataSize = strToFill.size();
		break;
	case IDC_HEXCTRL_FILLDATA_RADIO_UTF16:
		hms.pData = reinterpret_cast<std::byte*>(wstrComboText.data());
		hms.ullDataSize = static_cast<ULONGLONG>(wstrComboText.size()) * sizeof(WCHAR);
		break;
	default:
		break;
	}

	//Insert wstring into ComboBox only if it's not already presented.
	if (pCombo->FindStringExact(0, wstrComboText.data()) == CB_ERR)
	{
		//Keep max 50 strings in list.
		if (pCombo->GetCount() == 50)
			pCombo->DeleteString(49);
		pCombo->InsertString(0, wstrComboText.data());
	}

	m_pHexCtrl->ModifyData(hms);
	::SetFocus(m_pHexCtrl->GetWindowHandle(EHexWnd::WND_MAIN));

	CDialogEx::OnOK();
}