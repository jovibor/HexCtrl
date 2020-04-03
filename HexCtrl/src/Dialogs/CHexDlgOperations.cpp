/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#include "stdafx.h"
#include "CHexDlgOperations.h"
#include "../Helper.h"

using namespace HEXCTRL::INTERNAL;

BEGIN_MESSAGE_MAP(CHexDlgOperations, CDialogEx)
END_MESSAGE_MAP()

BOOL CHexDlgOperations::Create(UINT nIDTemplate, CHexCtrl * pHexCtrl)
{
	m_pHexCtrl = pHexCtrl;

	return CDialogEx::Create(nIDTemplate, pHexCtrl);
}

BOOL CHexDlgOperations::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	CheckRadioButton(IDC_HEXCTRL_OPERATIONS_RADIO_OR, IDC_HEXCTRL_OPERATIONS_RADIO_DIV, IDC_HEXCTRL_OPERATIONS_RADIO_OR);
	CheckRadioButton(IDC_HEXCTRL_OPERATIONS_RADIO_BYTE, IDC_HEXCTRL_OPERATIONS_RADIO_QWORD, IDC_HEXCTRL_OPERATIONS_RADIO_BYTE);

	return TRUE; // return TRUE unless you set the focus to a control
}

CHexCtrl* CHexDlgOperations::GetHexCtrl()const
{
	return m_pHexCtrl;
}

void CHexDlgOperations::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_HEXCTRL_OPERATIONS_EDIT_OR, m_editOR);
	DDX_Control(pDX, IDC_HEXCTRL_OPERATIONS_EDIT_XOR, m_editXOR);
	DDX_Control(pDX, IDC_HEXCTRL_OPERATIONS_EDIT_AND, m_editAND);
	DDX_Control(pDX, IDC_HEXCTRL_OPERATIONS_EDIT_SHL, m_editSHL);
	DDX_Control(pDX, IDC_HEXCTRL_OPERATIONS_EDIT_SHR, m_editSHR);
	DDX_Control(pDX, IDC_HEXCTRL_OPERATIONS_EDIT_ADD, m_editAdd);
	DDX_Control(pDX, IDC_HEXCTRL_OPERATIONS_EDIT_SUB, m_editSub);
	DDX_Control(pDX, IDC_HEXCTRL_OPERATIONS_EDIT_MUL, m_editMul);
	DDX_Control(pDX, IDC_HEXCTRL_OPERATIONS_EDIT_DIV, m_editDiv);
}

BOOL CHexDlgOperations::OnCommand(WPARAM wParam, LPARAM lParam)
{
	//lParam holds HWND.
	WORD wID = LOWORD(wParam);
	WORD wMessage = HIWORD(wParam);
	bool fHere { true };
	if (wID >= IDC_HEXCTRL_OPERATIONS_EDIT_OR && wID <= IDC_HEXCTRL_OPERATIONS_EDIT_DIV && wMessage == EN_SETFOCUS)
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
		case IDC_HEXCTRL_OPERATIONS_EDIT_SUB:
			iRadioID = IDC_HEXCTRL_OPERATIONS_RADIO_SUB;
			break;
		case IDC_HEXCTRL_OPERATIONS_EDIT_MUL:
			iRadioID = IDC_HEXCTRL_OPERATIONS_RADIO_MUL;
			break;
		case IDC_HEXCTRL_OPERATIONS_EDIT_DIV:
			iRadioID = IDC_HEXCTRL_OPERATIONS_RADIO_DIV;
			break;
		default:
			fHere = false;
		}
		CheckRadioButton(IDC_HEXCTRL_OPERATIONS_RADIO_OR, IDC_HEXCTRL_OPERATIONS_RADIO_DIV, iRadioID);
	}
	else
		fHere = false;

	if (fHere)
		return TRUE;

	return CDialogEx::OnCommand(wParam, lParam);
}

BOOL CHexDlgOperations::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	if (wParam == HEXCTRL_EDITCTRL)
	{
		auto pnmh = reinterpret_cast<NMHDR*>(lParam);
		switch (pnmh->code)
		{
		case VK_RETURN:
			OnOK();
			break;
		case VK_ESCAPE:
			OnCancel();
			break;
		case VK_UP:
			PostMessageW(WM_NEXTDLGCTL, TRUE);
			break;
		case VK_TAB:
		case VK_DOWN:
			PostMessageW(WM_NEXTDLGCTL);
			break;
		}
	}

	return CDialogEx::OnNotify(wParam, lParam, pResult);
}

void CHexDlgOperations::OnOK()
{
	CHexCtrl* pHex = GetHexCtrl();
	if (!pHex)
		return;

	int iRadioOperation = GetCheckedRadioButton(IDC_HEXCTRL_OPERATIONS_RADIO_OR, IDC_HEXCTRL_OPERATIONS_RADIO_DIV);
	int iRadioDataSize = GetCheckedRadioButton(IDC_HEXCTRL_OPERATIONS_RADIO_BYTE, IDC_HEXCTRL_OPERATIONS_RADIO_QWORD);

	MODIFYSTRUCT hms;
	hms.enModifyMode = EModifyMode::MODIFY_OPERATION;
	hms.vecSpan = pHex->GetSelection();
	if (hms.vecSpan.empty())
		return;

	int iEditId = 0;
	switch (iRadioOperation)
	{
	case IDC_HEXCTRL_OPERATIONS_RADIO_OR:
		hms.enOperMode = EOperMode::OPER_OR;
		iEditId = IDC_HEXCTRL_OPERATIONS_EDIT_OR;
		break;
	case IDC_HEXCTRL_OPERATIONS_RADIO_XOR:
		hms.enOperMode = EOperMode::OPER_XOR;
		iEditId = IDC_HEXCTRL_OPERATIONS_EDIT_XOR;
		break;
	case IDC_HEXCTRL_OPERATIONS_RADIO_AND:
		hms.enOperMode = EOperMode::OPER_AND;
		iEditId = IDC_HEXCTRL_OPERATIONS_EDIT_AND;
		break;
	case IDC_HEXCTRL_OPERATIONS_RADIO_NOT:
		hms.enOperMode = EOperMode::OPER_NOT;
		break;
	case IDC_HEXCTRL_OPERATIONS_RADIO_SHL:
		hms.enOperMode = EOperMode::OPER_SHL;
		iEditId = IDC_HEXCTRL_OPERATIONS_EDIT_SHL;
		break;
	case IDC_HEXCTRL_OPERATIONS_RADIO_SHR:
		hms.enOperMode = EOperMode::OPER_SHR;
		iEditId = IDC_HEXCTRL_OPERATIONS_EDIT_SHR;
		break;
	case IDC_HEXCTRL_OPERATIONS_RADIO_ADD:
		hms.enOperMode = EOperMode::OPER_ADD;
		iEditId = IDC_HEXCTRL_OPERATIONS_EDIT_ADD;
		break;
	case IDC_HEXCTRL_OPERATIONS_RADIO_SUB:
		hms.enOperMode = EOperMode::OPER_SUBTRACT;
		iEditId = IDC_HEXCTRL_OPERATIONS_EDIT_SUB;
		break;
	case IDC_HEXCTRL_OPERATIONS_RADIO_MUL:
		hms.enOperMode = EOperMode::OPER_MULTIPLY;
		iEditId = IDC_HEXCTRL_OPERATIONS_EDIT_MUL;
		break;
	case IDC_HEXCTRL_OPERATIONS_RADIO_DIV:
		hms.enOperMode = EOperMode::OPER_DIVIDE;
		iEditId = IDC_HEXCTRL_OPERATIONS_EDIT_DIV;
		break;
	default:
		break;
	}

	LONGLONG llData;
	if (iEditId)
	{
		WCHAR pwszEditText[32];
		GetDlgItemTextW(iEditId, pwszEditText, 32);

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

	pHex->Modify(hms);
	pHex->SetFocus();

	CDialogEx::OnOK();
}