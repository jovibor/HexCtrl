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
	ON_WM_ACTIVATE()
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
	CheckRadioButton(IDC_HEXCTRL_OPERS_RADIO_ALL, IDC_HEXCTRL_OPERS_RADIO_SELECTION, IDC_HEXCTRL_OPERS_RADIO_ALL);

	return TRUE;
}

void CHexDlgOpers::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if (!m_pHexCtrl->IsCreated() || !m_pHexCtrl->IsDataSet())
		return;

	if (nState == WA_ACTIVE || nState == WA_CLICKACTIVE)
	{
		const auto fSelection { m_pHexCtrl->HasSelection() };
		CheckRadioButton(IDC_HEXCTRL_OPERS_RADIO_ALL, IDC_HEXCTRL_OPERS_RADIO_SELECTION,
			fSelection ? IDC_HEXCTRL_OPERS_RADIO_SELECTION : IDC_HEXCTRL_OPERS_RADIO_ALL);
		GetDlgItem(IDC_HEXCTRL_OPERS_RADIO_SELECTION)->EnableWindow(fSelection);
	}

	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
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
	const bool fMsgHere { !(wID == IDOK || wID == IDCANCEL) };

	if (wID >= IDC_HEXCTRL_OPERS_EDIT_OR && wID <= IDC_HEXCTRL_OPERS_EDIT_FLOOR && wMessage == EN_SETFOCUS)
	{
		int iRadioID { 0 };
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
			break;
		}

		if (iRadioID > 0)
			CheckRadioButton(IDC_HEXCTRL_OPERS_RADIO_OR, IDC_HEXCTRL_OPERS_RADIO_FLOOR, iRadioID);
	}

	//Enable or disable big/little-endian radio buttons.
	const auto iRadioOperation = GetCheckedRadioButton(IDC_HEXCTRL_OPERS_RADIO_OR, IDC_HEXCTRL_OPERS_RADIO_FLOOR);
	const auto iRadioDataSize = GetCheckedRadioButton(IDC_HEXCTRL_OPERS_RADIO_BYTE, IDC_HEXCTRL_OPERS_RADIO_QWORD);
	const BOOL fBeLeEnable { iRadioDataSize != IDC_HEXCTRL_OPERS_RADIO_BYTE && iRadioOperation != IDC_HEXCTRL_OPERS_RADIO_NOT
		&& iRadioOperation != IDC_HEXCTRL_OPERS_RADIO_SWAP };
	GetDlgItem(IDC_HEXCTRL_OPERS_RADIO_LE)->EnableWindow(fBeLeEnable);
	GetDlgItem(IDC_HEXCTRL_OPERS_RADIO_BE)->EnableWindow(fBeLeEnable);
	GetDlgItem(IDC_HEXCTRL_OPERS_RADIO_SWAP)->EnableWindow(iRadioDataSize != IDC_HEXCTRL_OPERS_RADIO_BYTE);

	return fMsgHere ? TRUE : CDialogEx::OnCommand(wParam, lParam);
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
	const auto iRadioAllSelection = GetCheckedRadioButton(IDC_HEXCTRL_OPERS_RADIO_ALL, IDC_HEXCTRL_OPERS_RADIO_SELECTION);
	const auto fBigEndian = iRadioByteOrder == IDC_HEXCTRL_OPERS_RADIO_BE && iRadioDataSize != IDC_HEXCTRL_OPERS_RADIO_BYTE
		&& iRadioOperation != IDC_HEXCTRL_OPERS_RADIO_NOT && iRadioOperation != IDC_HEXCTRL_OPERS_RADIO_SWAP;

	HEXMODIFY hms;
	hms.enModifyMode = EHexModifyMode::MODIFY_OPERATION;

	/***************************************************************************
	* Some operations don't need to swap the whole data in big-endian mode.
	* Instead the Operational data-bytes can be swapped here just once.
	* Binary OR/XOR/AND are good examples, binary NOT doesn't need swap at all.
	* The fSwapHere flag shows exactly this, that data can be swapped here.
	***************************************************************************/
	bool fSwapHere { false };
	int iEditID { 0 };
	switch (iRadioOperation)
	{
	case IDC_HEXCTRL_OPERS_RADIO_OR:
		hms.enOperMode = EHexOperMode::OPER_OR;
		iEditID = IDC_HEXCTRL_OPERS_EDIT_OR;
		fSwapHere = true;
		break;
	case IDC_HEXCTRL_OPERS_RADIO_XOR:
		hms.enOperMode = EHexOperMode::OPER_XOR;
		iEditID = IDC_HEXCTRL_OPERS_EDIT_XOR;
		fSwapHere = true;
		break;
	case IDC_HEXCTRL_OPERS_RADIO_AND:
		hms.enOperMode = EHexOperMode::OPER_AND;
		iEditID = IDC_HEXCTRL_OPERS_EDIT_AND;
		fSwapHere = true;
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
	case IDC_HEXCTRL_OPERS_RADIO_SWAP:
		hms.enOperMode = EHexOperMode::OPER_SWAP;
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

	LONGLONG llData { };
	if (iEditID != 0) //All the switches that have edit fields.
	{
		WCHAR pwszEditText[32];
		GetDlgItemTextW(iEditID, pwszEditText, static_cast<int>(std::size(pwszEditText)));

		std::wstring wstrErr { };
		if (pwszEditText[0] == L'\0') //Edit field emptiness check.
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

		if (fBigEndian && fSwapHere)
		{
			switch (hms.ullDataSize)
			{
			case (sizeof(WORD)):
				llData = static_cast<LONGLONG>(_byteswap_ushort(static_cast<WORD>(llData)));
				break;
			case (sizeof(DWORD)):
				llData = static_cast<LONGLONG>(_byteswap_ulong(static_cast<DWORD>(llData)));
				break;
			case (sizeof(QWORD)):
				llData = static_cast<LONGLONG>(_byteswap_uint64(static_cast<QWORD>(llData)));
				break;
			default:
				break;
			}
		}
	}

	if (iRadioAllSelection == IDC_HEXCTRL_OPERS_RADIO_ALL)
	{
		if (MessageBoxW(L"You are about to modify the entire data region.\r\nAre you sure?", L"Modify All data?", MB_YESNO | MB_ICONWARNING) == IDNO)
			return;
		hms.vecSpan.emplace_back(HEXSPANSTRUCT { 0, m_pHexCtrl->GetDataSize() });
	}
	else
		hms.vecSpan = m_pHexCtrl->GetSelection();

	hms.fBigEndian = fBigEndian && !fSwapHere;
	hms.pData = reinterpret_cast<std::byte*>(&llData);
	m_pHexCtrl->ModifyData(hms);
	m_pHexCtrl->Redraw();
	::SetFocus(m_pHexCtrl->GetWindowHandle(EHexWnd::WND_MAIN));

	CDialogEx::OnOK();
}