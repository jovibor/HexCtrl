/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
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

void CHexDlgOpers::Create(CWnd* pParent, IHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	assert(pParent);
	if (pHexCtrl == nullptr)
		return;

	m_pHexCtrl = pHexCtrl;
	CDialogEx::Create(IDD_HEXCTRL_OPERS, pParent);
}

void CHexDlgOpers::OnActivate(UINT nState, CWnd* /*pWndOther*/, BOOL /*bMinimized*/)
{
	if (m_pHexCtrl == nullptr)
		return;

	if (nState == WA_ACTIVE || nState == WA_CLICKACTIVE) {
		if (m_pHexCtrl->IsCreated() && m_pHexCtrl->IsDataSet()) {
			const auto fSelection { m_pHexCtrl->HasSelection() };
			CheckRadioButton(IDC_HEXCTRL_OPERS_RAD_ALL, IDC_HEXCTRL_OPERS_RAD_SEL,
				fSelection ? IDC_HEXCTRL_OPERS_RAD_SEL : IDC_HEXCTRL_OPERS_RAD_ALL);
			GetDlgItem(IDC_HEXCTRL_OPERS_RAD_SEL)->EnableWindow(fSelection);
		}
	}
}


//Private methods.

void CHexDlgOpers::CheckWndAvail()const
{
	using enum EHexOperMode;
	using enum EHexDataSize;
	BOOL fOperandEnable = TRUE;
	switch (GetOperMode()) {
	case OPER_NOT:
	case OPER_SWAP:
	case OPER_BITREV:
		fOperandEnable = FALSE;
		break;
	default:
		break;
	};

	GetDlgItem(IDC_HEXCTRL_OPERS_EDIT_OPERAND)->EnableWindow(fOperandEnable);
	GetDlgItem(IDC_HEXCTRL_OPERS_CHK_BE)->EnableWindow(GetDataSize() == SIZE_BYTE ? FALSE : fOperandEnable);
}

void CHexDlgOpers::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HEXCTRL_OPERS_COMBO_OPER, m_stComboOper);
	DDX_Control(pDX, IDC_HEXCTRL_OPERS_COMBO_SIZE, m_stComboSize);
}

auto CHexDlgOpers::GetOperMode()const->EHexOperMode
{
	return static_cast<EHexOperMode>(m_stComboOper.GetItemData(m_stComboOper.GetCurSel()));
}

auto CHexDlgOpers::GetDataSize()const->EHexDataSize
{
	return static_cast<EHexDataSize>(m_stComboSize.GetItemData(m_stComboSize.GetCurSel()));
}

void CHexDlgOpers::OnCancel()
{
	static_cast<CDialogEx*>(GetParentOwner())->EndDialog(IDCANCEL);
}

BOOL CHexDlgOpers::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam)) //Control ID.
	{
	case IDC_HEXCTRL_OPERS_COMBO_OPER:
	case IDC_HEXCTRL_OPERS_COMBO_SIZE:
		CheckWndAvail();
		if (HIWORD(wParam) == CBN_SELCHANGE) { //Combo-box selection changed.
			SetOKButtonName();
		}
		return TRUE;
	default:
		break;
	}

	return CDialogEx::OnCommand(wParam, lParam);
}

BOOL CHexDlgOpers::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	using enum EHexOperMode;
	auto iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_ASSIGN).data());
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_ASSIGN));
	m_stComboOper.SetCurSel(iIndex);
	iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_ADD).data());
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_ADD));
	iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_SUB).data());
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_SUB));
	iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_MUL).data());
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_MUL));
	iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_DIV).data());
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_DIV));
	iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_CEIL).data());
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_CEIL));
	iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_FLOOR).data());
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_FLOOR));
	iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_OR).data());
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_OR));
	iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_XOR).data());
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_XOR));
	iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_AND).data());
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_AND));
	iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_NOT).data());
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_NOT));
	iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_SHL).data());
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_SHL));
	iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_SHR).data());
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_SHR));
	iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_ROTL).data());
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_ROTL));
	iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_ROTR).data());
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_ROTR));
	iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_SWAP).data());
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_SWAP));
	iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_BITREV).data());
	m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_BITREV));

	using enum EHexDataSize;
	iIndex = m_stComboSize.AddString(L"BYTE");
	m_stComboSize.SetItemData(iIndex, static_cast<DWORD_PTR>(SIZE_BYTE));
	m_stComboSize.SetCurSel(iIndex);
	iIndex = m_stComboSize.AddString(L"WORD");
	m_stComboSize.SetItemData(iIndex, static_cast<DWORD_PTR>(SIZE_WORD));
	iIndex = m_stComboSize.AddString(L"DWORD");
	m_stComboSize.SetItemData(iIndex, static_cast<DWORD_PTR>(SIZE_DWORD));
	iIndex = m_stComboSize.AddString(L"QWORD");
	m_stComboSize.SetItemData(iIndex, static_cast<DWORD_PTR>(SIZE_QWORD));

	CheckRadioButton(IDC_HEXCTRL_OPERS_RAD_ALL, IDC_HEXCTRL_OPERS_RAD_SEL, IDC_HEXCTRL_OPERS_RAD_ALL);
	SetOKButtonName();

	return TRUE;
}

void CHexDlgOpers::OnOK()
{
	if (!m_pHexCtrl->IsCreated() || !m_pHexCtrl->IsDataSet())
		return;

	using enum EHexOperMode;
	using enum EHexDataSize;
	const auto eOperMode = GetOperMode();
	const auto eDataSize = GetDataSize();
	const auto fCheckBE = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_OPERS_CHK_BE))->GetCheck() == BST_CHECKED;
	auto fBigEndian = fCheckBE && eDataSize != SIZE_BYTE && eOperMode != OPER_NOT && eOperMode != OPER_SWAP
		&& eOperMode != OPER_BITREV;

	HEXMODIFY hms { .enModifyMode = EHexModifyMode::MODIFY_OPERATION, .enOperMode = eOperMode,
		.enDataSize = eDataSize };

	LONGLONG llOperand { };
	if (const auto pWndOperand = GetDlgItem(IDC_HEXCTRL_OPERS_EDIT_OPERAND); pWndOperand->IsWindowEnabled()) {
		wchar_t buffOperand[32];
		pWndOperand->GetWindowTextW(buffOperand, static_cast<int>(std::size(buffOperand)));

		std::wstring wstrErr;
		if (buffOperand[0] == L'\0') { //Operand field emptiness check.
			wstrErr = L"Missing Operand!";
		}
		else if (const auto optOperand = stn::StrToLL(buffOperand); !optOperand) {
			wstrErr = L"Wrong number format!";
		}
		else if (llOperand = *optOperand; hms.enOperMode == OPER_DIV && llOperand == 0) { //Division by zero check.
			wstrErr = L"Wrong number format! Can not divide by zero!";
		}

		if (!wstrErr.empty()) {
			MessageBoxW(wstrErr.data(), L"Operand Error!", MB_ICONERROR);
			return;
		}

		if (fBigEndian) {
			/***************************************************************************
			* Some operations don't need to swap the whole data in big-endian mode.
			* Instead, the operand can be swapped here just once.
			* Binary OR/XOR/AND are good examples, binary NOT doesn't need swap at all.
			* The fSwapHere flag shows exactly this, that the operand can be swapped here.
			***************************************************************************/
			if (eOperMode == OPER_OR || eOperMode == OPER_XOR || eOperMode == OPER_AND || eOperMode == OPER_ASSIGN) {
				fBigEndian = false;
				switch (hms.enDataSize) {
				case SIZE_WORD:
					llOperand = static_cast<LONGLONG>(ByteSwap(static_cast<WORD>(llOperand)));
					break;
				case SIZE_DWORD:
					llOperand = static_cast<LONGLONG>(ByteSwap(static_cast<DWORD>(llOperand)));
					break;
				case SIZE_QWORD:
					llOperand = static_cast<LONGLONG>(ByteSwap(static_cast<QWORD>(llOperand)));
					break;
				default:
					break;
				}
			}
		}
	}

	const auto iRadioAllOrSel = GetCheckedRadioButton(IDC_HEXCTRL_OPERS_RAD_ALL, IDC_HEXCTRL_OPERS_RAD_SEL);
	if (iRadioAllOrSel == IDC_HEXCTRL_OPERS_RAD_ALL) {
		if (MessageBoxW(L"You are about to modify the entire data region.\r\nAre you sure?",
			L"Modify All data?", MB_YESNO | MB_ICONWARNING) == IDNO)
			return;

		hms.vecSpan.emplace_back(0, m_pHexCtrl->GetDataSize());
	}
	else {
		if (!m_pHexCtrl->HasSelection())
			return;

		hms.vecSpan = m_pHexCtrl->GetSelection();
	}

	hms.fBigEndian = fBigEndian;
	hms.spnData = { reinterpret_cast<std::byte*>(&llOperand), sizeof(llOperand) };
	m_pHexCtrl->ModifyData(hms);
	m_pHexCtrl->Redraw();
}

void CHexDlgOpers::SetOKButtonName()
{
	GetDlgItem(IDOK)->SetWindowTextW(m_mapNames.at(GetOperMode()).data());
}