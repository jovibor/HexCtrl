/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/                         *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#include "stdafx.h"
#include "CHexDlgOperations.h"
#include "../Helper.h"

using namespace HEXCTRL::INTERNAL;

BEGIN_MESSAGE_MAP(CHexDlgOperations, CDialogEx)
	ON_BN_CLICKED(IDOK, &CHexDlgOperations::OnBnClickedOk)
END_MESSAGE_MAP()

BOOL CHexDlgOperations::Create(UINT nIDTemplate, CHexCtrl * pHexCtrl)
{
	m_pHexCtrl = pHexCtrl;

	return CDialog::Create(nIDTemplate, m_pHexCtrl);
}

BOOL CHexDlgOperations::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	CheckRadioButton(IDC_HEXCTRL_OPERATIONS_RADIO_OR, IDC_HEXCTRL_OPERATIONS_RADIO_DIVIDE, IDC_HEXCTRL_OPERATIONS_RADIO_OR);
	CheckRadioButton(IDC_HEXCTRL_OPERATIONS_RADIO_BYTE, IDC_HEXCTRL_OPERATIONS_RADIO_QWORD, IDC_HEXCTRL_OPERATIONS_RADIO_BYTE);

	return TRUE;  // return TRUE unless you set the focus to a control
}

CHexCtrl* CHexDlgOperations::GetHexCtrl()const
{
	return m_pHexCtrl;
}

void CHexDlgOperations::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BOOL CHexDlgOperations::OnCommand(WPARAM wParam, LPARAM lParam)
{
	//lParam holds HWND.
	WORD wID = LOWORD(wParam);
	WORD wMessage = HIWORD(wParam);
	if (wID >= IDC_HEXCTRL_OPERATIONS_EDIT_OR && wID <= IDC_HEXCTRL_OPERATIONS_EDIT_DIVIDE && wMessage == EN_SETFOCUS)
	{
		int iRadioID { };
		switch (wID)
		{
		case IDC_HEXCTRL_OPERATIONS_EDIT_OR:
			iRadioID = IDC_HEXCTRL_OPERATIONS_RADIO_OR;
			break;
		case IDC_HEXCTRL_OPERATIONS_EDIT_XOR:
			iRadioID = IDC_HEXCTRL_OPERATIONS_RADIO_XOR;
			break;
		case IDC_HEXCTRL_OPERATIONS_EDIT_AND:
			iRadioID = IDC_HEXCTRL_OPERATIONS_RADIO_AND;
			break;
		case IDC_HEXCTRL_OPERATIONS_EDIT_SHL:
			iRadioID = IDC_HEXCTRL_OPERATIONS_RADIO_SHL;
			break;
		case IDC_HEXCTRL_OPERATIONS_EDIT_SHR:
			iRadioID = IDC_HEXCTRL_OPERATIONS_RADIO_SHR;
			break;
		case IDC_HEXCTRL_OPERATIONS_EDIT_ADD:
			iRadioID = IDC_HEXCTRL_OPERATIONS_RADIO_ADD;
			break;
		case IDC_HEXCTRL_OPERATIONS_EDIT_SUBTRACT:
			iRadioID = IDC_HEXCTRL_OPERATIONS_RADIO_SUBTRACT;
			break;
		case IDC_HEXCTRL_OPERATIONS_EDIT_MULTIPLY:
			iRadioID = IDC_HEXCTRL_OPERATIONS_RADIO_MULTIPLY;
			break;
		case IDC_HEXCTRL_OPERATIONS_EDIT_DIVIDE:
			iRadioID = IDC_HEXCTRL_OPERATIONS_RADIO_DIVIDE;
			break;
		}
		CheckRadioButton(IDC_HEXCTRL_OPERATIONS_RADIO_OR, IDC_HEXCTRL_OPERATIONS_RADIO_DIVIDE, iRadioID);
	}

	return CDialogEx::OnCommand(wParam, lParam);
}

void CHexDlgOperations::OnBnClickedOk()
{
	CHexCtrl* pHex = GetHexCtrl();
	if (!pHex)
		return;

	int iRadioOperation = GetCheckedRadioButton(IDC_HEXCTRL_OPERATIONS_RADIO_OR, IDC_HEXCTRL_OPERATIONS_RADIO_DIVIDE);
	int iRadioDataSize = GetCheckedRadioButton(IDC_HEXCTRL_OPERATIONS_RADIO_BYTE, IDC_HEXCTRL_OPERATIONS_RADIO_QWORD);

	HEXMODIFYSTRUCT hms;
	hms.enMode = EHexModifyMode::MODIFY_OPERATION;
	pHex->GetSelection(hms.ullIndex, hms.ullSize);
	if (hms.ullSize == 0)
		return;

	int iEditId = 0;
	switch (iRadioOperation)
	{
	case IDC_HEXCTRL_OPERATIONS_RADIO_OR:
		hms.enOperMode = EHexOperMode::OPER_OR;
		iEditId = IDC_HEXCTRL_OPERATIONS_EDIT_OR;
		break;
	case IDC_HEXCTRL_OPERATIONS_RADIO_XOR:
		hms.enOperMode = EHexOperMode::OPER_XOR;
		iEditId = IDC_HEXCTRL_OPERATIONS_EDIT_XOR;
		break;
	case IDC_HEXCTRL_OPERATIONS_RADIO_AND:
		hms.enOperMode = EHexOperMode::OPER_AND;
		iEditId = IDC_HEXCTRL_OPERATIONS_EDIT_AND;
		break;
	case IDC_HEXCTRL_OPERATIONS_RADIO_NOT:
		hms.enOperMode = EHexOperMode::OPER_NOT;
		break;
	case IDC_HEXCTRL_OPERATIONS_RADIO_SHL:
		hms.enOperMode = EHexOperMode::OPER_SHL;
		iEditId = IDC_HEXCTRL_OPERATIONS_EDIT_SHL;
		break;
	case IDC_HEXCTRL_OPERATIONS_RADIO_SHR:
		hms.enOperMode = EHexOperMode::OPER_SHR;
		iEditId = IDC_HEXCTRL_OPERATIONS_EDIT_SHR;
		break;
	case IDC_HEXCTRL_OPERATIONS_RADIO_ADD:
		hms.enOperMode = EHexOperMode::OPER_ADD;
		iEditId = IDC_HEXCTRL_OPERATIONS_EDIT_ADD;
		break;
	case IDC_HEXCTRL_OPERATIONS_RADIO_SUBTRACT:
		hms.enOperMode = EHexOperMode::OPER_SUBTRACT;
		iEditId = IDC_HEXCTRL_OPERATIONS_EDIT_SUBTRACT;
		break;
	case IDC_HEXCTRL_OPERATIONS_RADIO_MULTIPLY:
		hms.enOperMode = EHexOperMode::OPER_MULTIPLY;
		iEditId = IDC_HEXCTRL_OPERATIONS_EDIT_MULTIPLY;
		break;
	case IDC_HEXCTRL_OPERATIONS_RADIO_DIVIDE:
		hms.enOperMode = EHexOperMode::OPER_DIVIDE;
		iEditId = IDC_HEXCTRL_OPERATIONS_EDIT_DIVIDE;
		break;
	default:
		break;
	}

	LONGLONG ullData;
	if (iEditId)
	{
		WCHAR pwszEditText[9];
		GetDlgItemTextW(iEditId, pwszEditText, 8);
		std::wstring wstr;
		STIF_FLAGS stifFlag = STIF_DEFAULT;

		CButton* pCheck = (CButton*)GetDlgItem(IDC_HEXCTRL_OPERATIONS_CHECK_HEXDECIMAL);
		if (pCheck && pCheck->GetCheck())
		{
			stifFlag = STIF_SUPPORT_HEX;
			wstr = L"0x";
		}
		wstr += pwszEditText;

		if (!StrToInt64ExW(wstr.data(), stifFlag, &ullData))
		{
			MessageBoxW(L"Wrong number format!", L"Format Error", MB_ICONERROR);
			return;
		}
		hms.pData = (PBYTE)& ullData;
	}

	switch (iRadioDataSize)
	{
	case IDC_HEXCTRL_OPERATIONS_RADIO_BYTE:
		hms.ullDataSize = 1;
		break;
	case IDC_HEXCTRL_OPERATIONS_RADIO_WORD:
		hms.ullDataSize = 2;
		break;
	case IDC_HEXCTRL_OPERATIONS_RADIO_DWORD:
		hms.ullDataSize = 4;
		break;
	case IDC_HEXCTRL_OPERATIONS_RADIO_QWORD:
		hms.ullDataSize = 8;
		break;
	}

	pHex->ModifyData(hms);
	pHex->SetFocus();

	CDialogEx::OnOK();
}