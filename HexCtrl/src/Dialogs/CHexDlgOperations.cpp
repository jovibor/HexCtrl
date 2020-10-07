/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#include "stdafx.h"
#include "../../res/HexCtrlRes.h"
#include "../Helper.h"
#include "CHexDlgOperations.h"
#include <cassert>

using namespace HEXCTRL::INTERNAL;

BEGIN_MESSAGE_MAP(CHexDlgOperations, CDialogEx)
END_MESSAGE_MAP()

BOOL CHexDlgOperations::Create(UINT nIDTemplate, CHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	if (pHexCtrl == nullptr)
		return FALSE;

	m_pHexCtrl = pHexCtrl;

	return CDialogEx::Create(nIDTemplate, pHexCtrl);
}

BOOL CHexDlgOperations::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	CheckRadioButton(IDC_HEXCTRL_OPERS_RADIO_OR, IDC_HEXCTRL_OPERS_RADIO_DIV, IDC_HEXCTRL_OPERS_RADIO_OR);
	CheckRadioButton(IDC_HEXCTRL_OPERS_RADIO_BYTE, IDC_HEXCTRL_OPERS_RADIO_QWORD, IDC_HEXCTRL_OPERS_RADIO_BYTE);

	return TRUE; // return TRUE unless you set the focus to a control
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
	WORD wMessage = HIWORD(wParam);
	bool fHere { true };
	if (auto wID = LOWORD(wParam); wID >= IDC_HEXCTRL_OPERS_EDIT_OR
		&& wID <= IDC_HEXCTRL_OPERS_EDIT_DIV
		&& wMessage == EN_SETFOCUS)
	{
		int iRadioID { };
		switch (wID)
		{
		case IDC_HEXCTRL_OPERS_EDIT_OR:
			iRadioID = IDC_HEXCTRL_OPERS_RADIO_OR;
			break;
		case IDC_HEXCTRL_OPERS_EDIT_XOR:
			iRadioID = IDC_HEXCTRL_OPERS_RADIO_XOR;
			break;
		case IDC_HEXCTRL_OPERS_EDIT_AND:
			iRadioID = IDC_HEXCTRL_OPERS_RADIO_AND;
			break;
		case IDC_HEXCTRL_OPERS_EDIT_SHL:
			iRadioID = IDC_HEXCTRL_OPERS_RADIO_SHL;
			break;
		case IDC_HEXCTRL_OPERS_EDIT_SHR:
			iRadioID = IDC_HEXCTRL_OPERS_RADIO_SHR;
			break;
		case IDC_HEXCTRL_OPERS_EDIT_ADD:
			iRadioID = IDC_HEXCTRL_OPERS_RADIO_ADD;
			break;
		case IDC_HEXCTRL_OPERS_EDIT_SUB:
			iRadioID = IDC_HEXCTRL_OPERS_RADIO_SUB;
			break;
		case IDC_HEXCTRL_OPERS_EDIT_MUL:
			iRadioID = IDC_HEXCTRL_OPERS_RADIO_MUL;
			break;
		case IDC_HEXCTRL_OPERS_EDIT_DIV:
			iRadioID = IDC_HEXCTRL_OPERS_RADIO_DIV;
			break;
		default:
			fHere = false;
		}
		CheckRadioButton(IDC_HEXCTRL_OPERS_RADIO_OR, IDC_HEXCTRL_OPERS_RADIO_DIV, iRadioID);
	}
	else
		fHere = false;

	if (fHere)
		return TRUE;

	return CDialogEx::OnCommand(wParam, lParam);
}

BOOL CHexDlgOperations::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	return CDialogEx::OnNotify(wParam, lParam, pResult);
}

void CHexDlgOperations::OnOK()
{
	auto pHex = GetHexCtrl();
	int iRadioOperation = GetCheckedRadioButton(IDC_HEXCTRL_OPERS_RADIO_OR, IDC_HEXCTRL_OPERS_RADIO_DIV);
	int iRadioDataSize = GetCheckedRadioButton(IDC_HEXCTRL_OPERS_RADIO_BYTE, IDC_HEXCTRL_OPERS_RADIO_QWORD);

	SMODIFY hms;
	hms.enModifyMode = EModifyMode::MODIFY_OPERATION;
	hms.vecSpan = pHex->GetSelection();
	if (hms.vecSpan.empty())
		return;

	int iEditID { 0 };
	switch (iRadioOperation)
	{
	case IDC_HEXCTRL_OPERS_RADIO_OR:
		hms.enOperMode = EOperMode::OPER_OR;
		iEditID = IDC_HEXCTRL_OPERS_EDIT_OR;
		break;
	case IDC_HEXCTRL_OPERS_RADIO_XOR:
		hms.enOperMode = EOperMode::OPER_XOR;
		iEditID = IDC_HEXCTRL_OPERS_EDIT_XOR;
		break;
	case IDC_HEXCTRL_OPERS_RADIO_AND:
		hms.enOperMode = EOperMode::OPER_AND;
		iEditID = IDC_HEXCTRL_OPERS_EDIT_AND;
		break;
	case IDC_HEXCTRL_OPERS_RADIO_NOT:
		hms.enOperMode = EOperMode::OPER_NOT;
		break;
	case IDC_HEXCTRL_OPERS_RADIO_SHL:
		hms.enOperMode = EOperMode::OPER_SHL;
		iEditID = IDC_HEXCTRL_OPERS_EDIT_SHL;
		break;
	case IDC_HEXCTRL_OPERS_RADIO_SHR:
		hms.enOperMode = EOperMode::OPER_SHR;
		iEditID = IDC_HEXCTRL_OPERS_EDIT_SHR;
		break;
	case IDC_HEXCTRL_OPERS_RADIO_ADD:
		hms.enOperMode = EOperMode::OPER_ADD;
		iEditID = IDC_HEXCTRL_OPERS_EDIT_ADD;
		break;
	case IDC_HEXCTRL_OPERS_RADIO_SUB:
		hms.enOperMode = EOperMode::OPER_SUBTRACT;
		iEditID = IDC_HEXCTRL_OPERS_EDIT_SUB;
		break;
	case IDC_HEXCTRL_OPERS_RADIO_MUL:
		hms.enOperMode = EOperMode::OPER_MULTIPLY;
		iEditID = IDC_HEXCTRL_OPERS_EDIT_MUL;
		break;
	case IDC_HEXCTRL_OPERS_RADIO_DIV:
		hms.enOperMode = EOperMode::OPER_DIVIDE;
		iEditID = IDC_HEXCTRL_OPERS_EDIT_DIV;
		break;
	default:
		break;
	}

	LONGLONG llData;
	if (iEditID)
	{
		WCHAR pwszEditText[32];
		GetDlgItemTextW(iEditID, pwszEditText, static_cast<int>(std::size(pwszEditText)));

		if (!wstr2num(pwszEditText, llData))
		{
			MessageBoxW(L"Wrong number format!", L"Format Error", MB_ICONERROR);
			return;
		}
		if (hms.enOperMode == EOperMode::OPER_DIVIDE && llData == 0) //Division by zero check.
		{
			MessageBoxW(L"Wrong number format!\r\nCan not divide by zero.", L"Format Error", MB_ICONERROR);
			return;
		}

		hms.pData = reinterpret_cast<std::byte*>(&llData);
	}

	switch (iRadioDataSize)
	{
	case IDC_HEXCTRL_OPERS_RADIO_BYTE:
		hms.ullDataSize = 1;
		break;
	case IDC_HEXCTRL_OPERS_RADIO_WORD:
		hms.ullDataSize = 2;
		break;
	case IDC_HEXCTRL_OPERS_RADIO_DWORD:
		hms.ullDataSize = 4;
		break;
	case IDC_HEXCTRL_OPERS_RADIO_QWORD:
		hms.ullDataSize = 8;
		break;
	}

	pHex->Modify(hms);
	pHex->SetFocus();

	CDialogEx::OnOK();
}