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
#include "CHexDlgOpers.h"
#include <cassert>

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

BEGIN_MESSAGE_MAP(CHexDlgOpers, CDialogEx)
END_MESSAGE_MAP()

BOOL CHexDlgOpers::Create(UINT nIDTemplate, CWnd* pParent, IHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	if (pHexCtrl == nullptr)
		return FALSE;

	m_pHexCtrl = pHexCtrl;

	return CDialogEx::Create(nIDTemplate, pParent);
}

BOOL CHexDlgOpers::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	CheckRadioButton(IDC_HEXCTRL_OPERS_RADIO_OR, IDC_HEXCTRL_OPERS_RADIO_FLOOR, IDC_HEXCTRL_OPERS_RADIO_OR);
	CheckRadioButton(IDC_HEXCTRL_OPERS_RADIO_BYTE, IDC_HEXCTRL_OPERS_RADIO_QWORD, IDC_HEXCTRL_OPERS_RADIO_BYTE);
	CheckRadioButton(IDC_HEXCTRL_OPERS_RADIO_LE, IDC_HEXCTRL_OPERS_RADIO_BE, IDC_HEXCTRL_OPERS_RADIO_LE);

	return TRUE;
}

void CHexDlgOpers::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BOOL CHexDlgOpers::OnCommand(WPARAM wParam, LPARAM lParam)
{
	//lParam holds HWND.
	const auto wMessage = HIWORD(wParam);
	const auto wID = LOWORD(wParam);
	bool fHere { true };
	if (wID >= IDC_HEXCTRL_OPERS_EDIT_OR && wID <= IDC_HEXCTRL_OPERS_EDIT_FLOOR && wMessage == EN_SETFOCUS)
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
		case IDC_HEXCTRL_OPERS_EDIT_CEILING:
			iRadioID = IDC_HEXCTRL_OPERS_RADIO_CEILING;
			break;
		case IDC_HEXCTRL_OPERS_EDIT_FLOOR:
			iRadioID = IDC_HEXCTRL_OPERS_RADIO_FLOOR;
			break;
		default:
			fHere = false;
		}
		CheckRadioButton(IDC_HEXCTRL_OPERS_RADIO_OR, IDC_HEXCTRL_OPERS_RADIO_FLOOR, iRadioID);
	}
	else if (wID >= IDC_HEXCTRL_OPERS_RADIO_BYTE && wID <= IDC_HEXCTRL_OPERS_RADIO_QWORD)
	{
		BOOL fEnabled { FALSE };
		switch (wID)
		{
		case IDC_HEXCTRL_OPERS_RADIO_WORD:
		case IDC_HEXCTRL_OPERS_RADIO_DWORD:
		case IDC_HEXCTRL_OPERS_RADIO_QWORD:
			fEnabled = TRUE;
			break;
		default:
			fHere = false;
		}
		GetDlgItem(IDC_HEXCTRL_OPERS_RADIO_LE)->EnableWindow(fEnabled);
		GetDlgItem(IDC_HEXCTRL_OPERS_RADIO_BE)->EnableWindow(fEnabled);
	}
	else
		fHere = false;

	return fHere ? TRUE : CDialogEx::OnCommand(wParam, lParam);
}

BOOL CHexDlgOpers::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	return CDialogEx::OnNotify(wParam, lParam, pResult);
}

void CHexDlgOpers::OnOK()
{
	const auto iRadioOperation = GetCheckedRadioButton(IDC_HEXCTRL_OPERS_RADIO_OR, IDC_HEXCTRL_OPERS_RADIO_FLOOR);
	const auto iRadioDataSize = GetCheckedRadioButton(IDC_HEXCTRL_OPERS_RADIO_BYTE, IDC_HEXCTRL_OPERS_RADIO_QWORD);
	const auto iRadioByteOrder = GetCheckedRadioButton(IDC_HEXCTRL_OPERS_RADIO_LE, IDC_HEXCTRL_OPERS_RADIO_BE);

	HEXMODIFY hms;
	hms.enModifyMode = EHexModifyMode::MODIFY_OPERATION;
	hms.fBigEndian = iRadioDataSize != IDC_HEXCTRL_OPERS_RADIO_BYTE && iRadioByteOrder == IDC_HEXCTRL_OPERS_RADIO_BE;
	hms.vecSpan = m_pHexCtrl->GetSelection();
	if (hms.vecSpan.empty())
		return;

	int iEditID { 0 };
	switch (iRadioOperation)
	{
	case IDC_HEXCTRL_OPERS_RADIO_OR:
		hms.enOperMode = EHexOperMode::OPER_OR;
		iEditID = IDC_HEXCTRL_OPERS_EDIT_OR;
		break;
	case IDC_HEXCTRL_OPERS_RADIO_XOR:
		hms.enOperMode = EHexOperMode::OPER_XOR;
		iEditID = IDC_HEXCTRL_OPERS_EDIT_XOR;
		break;
	case IDC_HEXCTRL_OPERS_RADIO_AND:
		hms.enOperMode = EHexOperMode::OPER_AND;
		iEditID = IDC_HEXCTRL_OPERS_EDIT_AND;
		break;
	case IDC_HEXCTRL_OPERS_RADIO_NOT:
		hms.enOperMode = EHexOperMode::OPER_NOT;
		break;
	case IDC_HEXCTRL_OPERS_RADIO_SHL:
		hms.enOperMode = EHexOperMode::OPER_SHL;
		iEditID = IDC_HEXCTRL_OPERS_EDIT_SHL;
		break;
	case IDC_HEXCTRL_OPERS_RADIO_SHR:
		hms.enOperMode = EHexOperMode::OPER_SHR;
		iEditID = IDC_HEXCTRL_OPERS_EDIT_SHR;
		break;
	case IDC_HEXCTRL_OPERS_RADIO_ADD:
		hms.enOperMode = EHexOperMode::OPER_ADD;
		iEditID = IDC_HEXCTRL_OPERS_EDIT_ADD;
		break;
	case IDC_HEXCTRL_OPERS_RADIO_SUB:
		hms.enOperMode = EHexOperMode::OPER_SUBTRACT;
		iEditID = IDC_HEXCTRL_OPERS_EDIT_SUB;
		break;
	case IDC_HEXCTRL_OPERS_RADIO_MUL:
		hms.enOperMode = EHexOperMode::OPER_MULTIPLY;
		iEditID = IDC_HEXCTRL_OPERS_EDIT_MUL;
		break;
	case IDC_HEXCTRL_OPERS_RADIO_DIV:
		hms.enOperMode = EHexOperMode::OPER_DIVIDE;
		iEditID = IDC_HEXCTRL_OPERS_EDIT_DIV;
		break;
	case IDC_HEXCTRL_OPERS_RADIO_CEILING:
		hms.enOperMode = EHexOperMode::OPER_CEILING;
		iEditID = IDC_HEXCTRL_OPERS_EDIT_CEILING;
		break;
	case IDC_HEXCTRL_OPERS_RADIO_FLOOR:
		hms.enOperMode = EHexOperMode::OPER_FLOOR;
		iEditID = IDC_HEXCTRL_OPERS_EDIT_FLOOR;
		break;
	default:
		break;
	}

	switch (iRadioDataSize)
	{
	case IDC_HEXCTRL_OPERS_RADIO_BYTE:
		hms.ullDataSize = sizeof(BYTE);
		break;
	case IDC_HEXCTRL_OPERS_RADIO_WORD:
		hms.ullDataSize = sizeof(WORD);
		break;
	case IDC_HEXCTRL_OPERS_RADIO_DWORD:
		hms.ullDataSize = sizeof(DWORD);
		break;
	case IDC_HEXCTRL_OPERS_RADIO_QWORD:
		hms.ullDataSize = sizeof(QWORD);
		break;
	default:
		break;
	}

	LONGLONG llData;
	if (iEditID > 0)
	{
		WCHAR pwszEditText[32];
		GetDlgItemTextW(iEditID, pwszEditText, static_cast<int>(std::size(pwszEditText)));

		std::wstring wstrErr { };
		if (wcsnlen(pwszEditText, std::size(pwszEditText)) == 0)
			wstrErr = L"Missing value!";
		else if (!wstr2num(pwszEditText, llData))
			wstrErr = L"Wrong number format!";
		else if (hms.enOperMode == EHexOperMode::OPER_DIVIDE && llData == 0) //Division by zero check.
			wstrErr = L"Wrong number format! Can not divide by zero!";
		if (!wstrErr.empty())
		{
			MessageBoxW(wstrErr.data(), L"Format Error!", MB_ICONERROR);
			return;
		}

		hms.pData = reinterpret_cast<std::byte*>(&llData);
	}

	m_pHexCtrl->ModifyData(hms);
	::SetFocus(m_pHexCtrl->GetWindowHandle(EHexWnd::WND_MAIN));

	CDialogEx::OnOK();
}