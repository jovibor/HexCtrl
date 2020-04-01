/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#include "stdafx.h"
#include "CHexDlgFillWith.h"
#include "../Helper.h"

using namespace HEXCTRL::INTERNAL;

BEGIN_MESSAGE_MAP(CHexDlgFillWith, CDialogEx)
END_MESSAGE_MAP()

BOOL CHexDlgFillWith::Create(UINT nIDTemplate, CHexCtrl* pHexCtrl)
{
	m_pHexCtrl = pHexCtrl;

	return CDialogEx::Create(nIDTemplate, pHexCtrl);
}

BOOL CHexDlgFillWith::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CheckRadioButton(IDC_HEXCTRL_FILLWITHDATA_RADIO_HEX, IDC_HEXCTRL_FILLWITHDATA_RADIO_UTF16, IDC_HEXCTRL_FILLWITHDATA_RADIO_HEX);
	CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_HEXCTRL_FILLWITHDATA_COMBO_HEXTEXT);
	if (pCombo)
		pCombo->LimitText(MAX_PATH); //Max characters count in combo's edit box.

	return TRUE;
}

CHexCtrl* CHexDlgFillWith::GetHexCtrl() const
{
	return m_pHexCtrl;
}

void CHexDlgFillWith::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

void CHexDlgFillWith::OnOK()
{
	CHexCtrl* pHex = GetHexCtrl();
	if (!pHex)
		return;

	int iRadioType = GetCheckedRadioButton(IDC_HEXCTRL_FILLWITHDATA_RADIO_HEX, IDC_HEXCTRL_FILLWITHDATA_RADIO_UTF16);

	MODIFYSTRUCT hms;
	hms.enModifyMode = EModifyMode::MODIFY_REPEAT;
	hms.vecSpan = pHex->GetSelection();
	if (hms.vecSpan.empty())
		return;

	CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_HEXCTRL_FILLWITHDATA_COMBO_HEXTEXT);
	if (!pCombo)
		return;

	WCHAR pwszComboText[MAX_PATH + 1];
	if (pCombo->GetWindowTextW(pwszComboText, MAX_PATH) == 0) //No text
		return;

	std::wstring wstrComboText = pwszComboText;
	std::string strToFill = WstrToStr(wstrComboText);
	switch (iRadioType)
	{
	case IDC_HEXCTRL_FILLWITHDATA_RADIO_HEX:
	{
		if (!StrToHex(strToFill, strToFill))
		{
			MessageBoxW(L"Wrong Hex format!", L"Format Error", MB_ICONERROR);
			return;
		}
		hms.pData = reinterpret_cast<std::byte*>(strToFill.data());
		hms.ullDataSize = strToFill.size();
	}
	break;
	case IDC_HEXCTRL_FILLWITHDATA_RADIO_TEXT:
		hms.pData = reinterpret_cast<std::byte*>(strToFill.data());
		hms.ullDataSize = strToFill.size();
		break;
	case IDC_HEXCTRL_FILLWITHDATA_RADIO_UTF16:
		hms.pData = reinterpret_cast<std::byte*>(wstrComboText.data());
		hms.ullDataSize = static_cast<ULONGLONG>(wstrComboText.size()) * sizeof(WCHAR);
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

	pHex->Modify(hms);
	pHex->SetFocus();

	CDialogEx::OnOK();
}