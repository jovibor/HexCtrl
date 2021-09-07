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
#include "CHexDlgOpers.h"
#include <cassert>

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

BEGIN_MESSAGE_MAP(CHexDlgOpers, CDialogEx)
	ON_WM_ACTIVATE()
END_MESSAGE_MAP()

void CHexDlgOpers::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HEXCTRL_OPERS_COMBO_OPER, m_stComboOper);
}

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

	auto iIndex = m_stComboOper.AddString(L"Assign");
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(EHexOperMode::OPER_ASSIGN));
	m_stComboOper.SetCurSel(iIndex);
	iIndex = m_stComboOper.AddString(L"Add");
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(EHexOperMode::OPER_ADD));
	iIndex = m_stComboOper.AddString(L"Subtract");
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(EHexOperMode::OPER_SUBTRACT));
	iIndex = m_stComboOper.AddString(L"Multiply");
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(EHexOperMode::OPER_MULTIPLY));
	iIndex = m_stComboOper.AddString(L"Divide");
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(EHexOperMode::OPER_DIVIDE));
	iIndex = m_stComboOper.AddString(L"Ceiling");
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(EHexOperMode::OPER_CEILING));
	iIndex = m_stComboOper.AddString(L"Floor");
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(EHexOperMode::OPER_FLOOR));
	iIndex = m_stComboOper.AddString(L"OR");
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(EHexOperMode::OPER_OR));
	iIndex = m_stComboOper.AddString(L"XOR");
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(EHexOperMode::OPER_XOR));
	iIndex = m_stComboOper.AddString(L"AND");
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(EHexOperMode::OPER_AND));
	iIndex = m_stComboOper.AddString(L"NOT");
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(EHexOperMode::OPER_NOT));
	iIndex = m_stComboOper.AddString(L"SHL");
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(EHexOperMode::OPER_SHL));
	iIndex = m_stComboOper.AddString(L"SHR");
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(EHexOperMode::OPER_SHR));
	iIndex = m_stComboOper.AddString(L"ROTL");
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(EHexOperMode::OPER_ROTL));
	iIndex = m_stComboOper.AddString(L"ROTR");
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(EHexOperMode::OPER_ROTR));
	iIndex = m_stComboOper.AddString(L"Swap Bytes Order");
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(EHexOperMode::OPER_SWAP));

	CheckRadioButton(IDC_HEXCTRL_OPERS_RADIO_BYTE, IDC_HEXCTRL_OPERS_RADIO_QWORD, IDC_HEXCTRL_OPERS_RADIO_BYTE);
	CheckRadioButton(IDC_HEXCTRL_OPERS_RADIO_LE, IDC_HEXCTRL_OPERS_RADIO_BE, IDC_HEXCTRL_OPERS_RADIO_LE);
	CheckRadioButton(IDC_HEXCTRL_OPERS_RADIO_ALL, IDC_HEXCTRL_OPERS_RADIO_SEL, IDC_HEXCTRL_OPERS_RADIO_ALL);

	return TRUE;
}

void CHexDlgOpers::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if (!m_pHexCtrl->IsCreated() || !m_pHexCtrl->IsDataSet())
		return;

	if (nState == WA_ACTIVE || nState == WA_CLICKACTIVE)
	{
		const auto fSelection { m_pHexCtrl->HasSelection() };
		CheckRadioButton(IDC_HEXCTRL_OPERS_RADIO_ALL, IDC_HEXCTRL_OPERS_RADIO_SEL,
			fSelection ? IDC_HEXCTRL_OPERS_RADIO_SEL : IDC_HEXCTRL_OPERS_RADIO_ALL);
		GetDlgItem(IDC_HEXCTRL_OPERS_RADIO_SEL)->EnableWindow(fSelection);
	}

	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
}

BOOL CHexDlgOpers::OnCommand(WPARAM wParam, LPARAM /*lParam*/)
{
	switch (LOWORD(wParam)) //Control ID.
	{
	case IDC_HEXCTRL_OPERS_RADIO_BYTE:
	case IDC_HEXCTRL_OPERS_RADIO_WORD:
	case IDC_HEXCTRL_OPERS_RADIO_DWORD:
	case IDC_HEXCTRL_OPERS_RADIO_QWORD:
	case IDC_HEXCTRL_OPERS_COMBO_OPER:
		CheckWndAvail();
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

BOOL CHexDlgOpers::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	return CDialogEx::OnNotify(wParam, lParam, pResult);
}

void CHexDlgOpers::OnOK()
{
	const auto eOperMode = GetOperMode();
	const auto iRadioDataSize = GetCheckedRadioButton(IDC_HEXCTRL_OPERS_RADIO_BYTE, IDC_HEXCTRL_OPERS_RADIO_QWORD);
	const auto iRadioByteOrder = GetCheckedRadioButton(IDC_HEXCTRL_OPERS_RADIO_LE, IDC_HEXCTRL_OPERS_RADIO_BE);
	const auto iRadioAllOrSel = GetCheckedRadioButton(IDC_HEXCTRL_OPERS_RADIO_ALL, IDC_HEXCTRL_OPERS_RADIO_SEL);
	auto fBigEndian = iRadioByteOrder == IDC_HEXCTRL_OPERS_RADIO_BE && iRadioDataSize != IDC_HEXCTRL_OPERS_RADIO_BYTE
		&& eOperMode != EHexOperMode::OPER_NOT && eOperMode != EHexOperMode::OPER_SWAP;

	HEXMODIFY hms;
	hms.enModifyMode = EHexModifyMode::MODIFY_OPERATION;
	hms.enOperMode = eOperMode;
	switch (iRadioDataSize)
	{
	case IDC_HEXCTRL_OPERS_RADIO_BYTE:
		hms.enOperSize = EHexDataSize::SIZE_BYTE;
		break;
	case IDC_HEXCTRL_OPERS_RADIO_WORD:
		hms.enOperSize = EHexDataSize::SIZE_WORD;
		break;
	case IDC_HEXCTRL_OPERS_RADIO_DWORD:
		hms.enOperSize = EHexDataSize::SIZE_DWORD;
		break;
	case IDC_HEXCTRL_OPERS_RADIO_QWORD:
		hms.enOperSize = EHexDataSize::SIZE_QWORD;
		break;
	default:
		break;
	}

	LONGLONG llData { };
	if (GetDlgItem(IDC_HEXCTRL_OPERS_EDIT_DATA)->IsWindowEnabled())
	{
		WCHAR pwszEditText[32];
		GetDlgItemTextW(IDC_HEXCTRL_OPERS_EDIT_DATA, pwszEditText, static_cast<int>(std::size(pwszEditText)));

		std::wstring wstrErr { };
		if (pwszEditText[0] == L'\0') //Edit field emptiness check.
			wstrErr = L"Missing Operand!";
		else if (!wstr2num(pwszEditText, llData))
			wstrErr = L"Wrong number format!";
		else if (hms.enOperMode == EHexOperMode::OPER_DIVIDE && llData == 0) //Division by zero check.
			wstrErr = L"Wrong number format! Can not divide by zero!";
		if (!wstrErr.empty())
		{
			MessageBoxW(wstrErr.data(), L"Operand Error!", MB_ICONERROR);
			return;
		}

		if (fBigEndian)
		{
			/***************************************************************************
			* Some operations don't need to swap the whole data in big-endian mode.
			* Instead the Operand data-bytes can be swapped here just once.
			* Binary OR/XOR/AND are good examples, binary NOT doesn't need swap at all.
			* The fSwapHere flag shows exactly this, that data can be swapped here.
			***************************************************************************/
			if (eOperMode == EHexOperMode::OPER_OR || eOperMode == EHexOperMode::OPER_XOR
				|| eOperMode == EHexOperMode::OPER_AND || eOperMode == EHexOperMode::OPER_ASSIGN)
			{
				fBigEndian = false;
				switch (hms.enOperSize)
				{
				case EHexDataSize::SIZE_WORD:
					llData = static_cast<LONGLONG>(_byteswap_ushort(static_cast<WORD>(llData)));
					break;
				case EHexDataSize::SIZE_DWORD:
					llData = static_cast<LONGLONG>(_byteswap_ulong(static_cast<DWORD>(llData)));
					break;
				case EHexDataSize::SIZE_QWORD:
					llData = static_cast<LONGLONG>(_byteswap_uint64(static_cast<QWORD>(llData)));
					break;
				default:
					break;
				}
			}
		}
	}

	if (iRadioAllOrSel == IDC_HEXCTRL_OPERS_RADIO_ALL)
	{
		if (MessageBoxW(L"You are about to modify the entire data region.\r\nAre you sure?", L"Modify All data?", MB_YESNO | MB_ICONWARNING) == IDNO)
			return;
		hms.vecSpan.emplace_back(HEXSPAN { 0, m_pHexCtrl->GetDataSize() });
	}
	else
		hms.vecSpan = m_pHexCtrl->GetSelection();

	hms.fBigEndian = fBigEndian;
	hms.spnData = { reinterpret_cast<std::byte*>(&llData), sizeof(llData) };
	m_pHexCtrl->ModifyData(hms);
	m_pHexCtrl->Redraw();
	::SetFocus(m_pHexCtrl->GetWindowHandle(EHexWnd::WND_MAIN));

	CDialogEx::OnOK();
}

EHexOperMode CHexDlgOpers::GetOperMode()const
{
	return static_cast<EHexOperMode>(m_stComboOper.GetItemData(m_stComboOper.GetCurSel()));
}

void CHexDlgOpers::CheckWndAvail()const
{
	BOOL fEditEnable = TRUE;
	BOOL fBELEEnable = TRUE;
	switch (GetOperMode())
	{
	case EHexOperMode::OPER_NOT:
	case EHexOperMode::OPER_SWAP:
		fEditEnable = FALSE;
		fBELEEnable = FALSE;
		break;
	default:
		break;
	};
	GetDlgItem(IDC_HEXCTRL_OPERS_EDIT_DATA)->EnableWindow(fEditEnable);

	const auto iRadioDataSize = GetCheckedRadioButton(IDC_HEXCTRL_OPERS_RADIO_BYTE, IDC_HEXCTRL_OPERS_RADIO_QWORD);
	if (fBELEEnable)
		fBELEEnable = iRadioDataSize != IDC_HEXCTRL_OPERS_RADIO_BYTE;

	GetDlgItem(IDC_HEXCTRL_OPERS_RADIO_LE)->EnableWindow(fBELEEnable);
	GetDlgItem(IDC_HEXCTRL_OPERS_RADIO_BE)->EnableWindow(fBELEEnable);
}