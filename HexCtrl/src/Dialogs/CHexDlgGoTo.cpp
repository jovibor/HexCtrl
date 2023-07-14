/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "../../res/HexCtrlRes.h"
#include "../HexUtility.h"
#include "CHexDlgGoTo.h"
#include <cassert>
#include <format>

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

BEGIN_MESSAGE_MAP(CHexDlgGoTo, CDialogEx)
	ON_COMMAND_RANGE(IDC_HEXCTRL_GOTO_RAD_ABS, IDC_HEXCTRL_GOTO_RAD_BACKEND, &CHexDlgGoTo::OnRadioRangeAddrType)
	ON_WM_ACTIVATE()
END_MESSAGE_MAP()

auto CHexDlgGoTo::GetDlgData()const->std::uint64_t
{
	if (!IsWindow(m_hWnd)) {
		return { };
	}

	return { };
}

void CHexDlgGoTo::Initialize(IHexCtrl* pHexCtrl)
{
	assert(pHexCtrl != nullptr);
	if (pHexCtrl == nullptr) {
		return;
	}

	m_pHexCtrl = pHexCtrl;
}

bool CHexDlgGoTo::IsRepeatAvail()const
{
	return m_iRepeat != 0;
}

void CHexDlgGoTo::Repeat(bool fFwd)
{
	if (!IsRepeatAvail()) {
		return;
	}

	const auto* const pHexCtrl = GetHexCtrl();
	if (fFwd) { //Repeat the last command (forward or backward) as is.
		switch (m_iRepeat) {
		case -1:
			if (m_ullCurrOffset < m_ullData) { //To avoid underflow.
				return;
			}
			m_ullCurrOffset -= m_ullData;
			break;
		case 1:
			if (m_ullCurrOffset + m_ullData >= pHexCtrl->GetDataSize()) { //To avoid overflow.
				return;
			}
			m_ullCurrOffset += m_ullData;
			break;
		default:
			break;
		}
	}
	else { //Repeat opposite of the last command (forward<->backward).
		switch (m_iRepeat) {
		case -1:
			if (m_ullCurrOffset + m_ullData >= pHexCtrl->GetDataSize()) { //To avoid overflow.
				return;
			}
			m_ullCurrOffset += m_ullData;
			break;
		case 1:
			if (m_ullCurrOffset < m_ullData) { //To avoid underflow.
				return;
			}
			m_ullCurrOffset -= m_ullData;
			break;
		default:
			break;
		}
	}

	HexCtrlGoOffset(m_ullCurrOffset);
}

auto CHexDlgGoTo::SetDlgData(std::uint64_t /*ullData*/)->HWND
{
	if (!IsWindow(m_hWnd)) {
		Create(IDD_HEXCTRL_GOTO, CWnd::FromHandle(m_pHexCtrl->GetWindowHandle(EHexWnd::WND_MAIN)));
	}

	return m_hWnd;
}

BOOL CHexDlgGoTo::ShowWindow(int nCmdShow)
{
	if (!IsWindow(m_hWnd)) {
		Create(IDD_HEXCTRL_GOTO, CWnd::FromHandle(m_pHexCtrl->GetWindowHandle(EHexWnd::WND_MAIN)));
	}

	return CDialogEx::ShowWindow(nCmdShow);
}


//Private methods.

void CHexDlgGoTo::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

auto CHexDlgGoTo::GetHexCtrl()const->IHexCtrl*
{
	return m_pHexCtrl;
}

void CHexDlgGoTo::HexCtrlGoOffset(ULONGLONG ullOffset)
{
	const auto pHexCtrl = GetHexCtrl();
	pHexCtrl->SetCaretPos(ullOffset);
	if (!pHexCtrl->IsOffsetVisible(ullOffset)) {
		pHexCtrl->GoToOffset(ullOffset);
	}

	m_ullCurrOffset = ullOffset;
}

void CHexDlgGoTo::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if (nState != WA_INACTIVE) {
		const auto* const pHexCtrl = GetHexCtrl();
		if (!pHexCtrl->IsCreated() || !pHexCtrl->IsDataSet()) {
			return;
		}

		if (const auto pRadio = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_GOTO_RAD_PAGE)); pRadio) {
			pRadio->EnableWindow(pHexCtrl->GetPagesCount() > 0);
		}

		GetDlgItem(IDC_HEXCTRL_GOTO_STATIC_PAGETOTAL)->SetWindowTextW(std::format(L"0x{:X}", pHexCtrl->GetPagesCount()).data());
		GetDlgItem(IDC_HEXCTRL_GOTO_STATIC_OFFTOTAL)->SetWindowTextW(std::format(L"0x{:X}", pHexCtrl->GetDataSize()).data());
		OnRadioRangeAddrType(GetCheckedRadioButton(IDC_HEXCTRL_GOTO_RAD_ABS, IDC_HEXCTRL_GOTO_RAD_BACKEND));
	}

	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
}

BOOL CHexDlgGoTo::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	if (const auto pRadio = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_GOTO_RAD_OFFSET)); pRadio) {
		pRadio->SetCheck(BST_CHECKED);
	}

	if (const auto pRadio = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_GOTO_RAD_ABS)); pRadio) {
		pRadio->SetCheck(BST_CHECKED);
	}

	return TRUE;
}

void CHexDlgGoTo::OnOK()
{
	const auto* const pHexCtrl = GetHexCtrl();
	if (!pHexCtrl->IsCreated() || !pHexCtrl->IsDataSet()) {
		return;
	}

	CStringW cstr;
	GetDlgItemTextW(IDC_HEXCTRL_GOTO_EDIT_GOTO, cstr);
	if (cstr.GetLength() == 0) {
		return;
	}

	const auto optData = stn::StrToULL(cstr.GetString());
	if (!optData) {
		MessageBoxW(L"Invalid number format", L"Error", MB_ICONERROR);
		return;
	}

	m_ullData = *optData;
	auto ullRangeFrom { 0ULL };
	auto ullRangeTo { 0ULL };
	auto ullMul { 0ULL };

	switch (GetCheckedRadioButton(IDC_HEXCTRL_GOTO_RAD_OFFSET, IDC_HEXCTRL_GOTO_RAD_PAGE)) {
	case IDC_HEXCTRL_GOTO_RAD_OFFSET:
		ullRangeFrom = m_ullOffsetsFrom;
		ullRangeTo = m_ullOffsetsTo;
		ullMul = 1;
		break;
	case IDC_HEXCTRL_GOTO_RAD_PAGE:
		ullRangeFrom = m_ullPagesFrom;
		ullRangeTo = m_ullPagesTo;
		ullMul = pHexCtrl->GetPageSize();
		break;
	default:
		break;
	}

	if (m_ullData < ullRangeFrom || m_ullData > ullRangeTo || m_ullData == 0) {
		MessageBoxW(L"Incorrect input range", L"Error", MB_ICONERROR);
		return;
	}

	ULONGLONG ullResult { };
	switch (GetCheckedRadioButton(IDC_HEXCTRL_GOTO_RAD_ABS, IDC_HEXCTRL_GOTO_RAD_BACKEND)) {
	case IDC_HEXCTRL_GOTO_RAD_ABS:
		ullResult = m_ullData * ullMul;
		m_iRepeat = 0;
		break;
	case IDC_HEXCTRL_GOTO_RAD_FWDCURR:
		ullResult = pHexCtrl->GetCaretPos() + (m_ullData * ullMul);
		m_iRepeat = 1;
		break;
	case IDC_HEXCTRL_GOTO_RAD_BACKCURR:
		ullResult = pHexCtrl->GetCaretPos() - (m_ullData * ullMul);
		m_iRepeat = -1;
		break;
	case IDC_HEXCTRL_GOTO_RAD_BACKEND:
		ullResult = pHexCtrl->GetDataSize() - (m_ullData * ullMul) - 1;
		m_iRepeat = 0;
		break;
	default:
		break;
	}

	HexCtrlGoOffset(ullResult);

	//To renew static text.
	OnRadioRangeAddrType(GetCheckedRadioButton(IDC_HEXCTRL_GOTO_RAD_ABS, IDC_HEXCTRL_GOTO_RAD_BACKEND));
}

void CHexDlgGoTo::OnRadioRangeAddrType(UINT uID)
{
	const auto* const pHexCtrl = GetHexCtrl();
	if (!pHexCtrl->IsCreated() || !pHexCtrl->IsDataSet()) {
		return;
	}

	switch (uID) {
	case IDC_HEXCTRL_GOTO_RAD_ABS:
		m_ullOffsetsFrom = 0ULL;
		m_ullOffsetsTo = pHexCtrl->GetDataSize() - 1;
		m_ullPagesFrom = 0;
		m_ullPagesTo = pHexCtrl->GetPagesCount() > 0 ? pHexCtrl->GetPagesCount() - 1 : 0ULL;
		break;
	case IDC_HEXCTRL_GOTO_RAD_FWDCURR:
		m_ullOffsetsFrom = (pHexCtrl->GetDataSize() - pHexCtrl->GetCaretPos() - 1) == 0 ? 0ULL : 1ULL;
		m_ullOffsetsTo = pHexCtrl->GetDataSize() - pHexCtrl->GetCaretPos() - 1;

		if (pHexCtrl->GetPagesCount() > 0) {
			m_ullPagesFrom = (pHexCtrl->GetPagesCount() - pHexCtrl->GetPagePos() - 1) == 0 ? 0ULL : 1ULL;
			m_ullPagesTo = pHexCtrl->GetPagesCount() - pHexCtrl->GetPagePos() - 1;
		}
		else {
			m_ullPagesFrom = 0ULL;
			m_ullPagesTo = 0ULL;
		}
		break;
	case IDC_HEXCTRL_GOTO_RAD_BACKCURR:
		m_ullOffsetsFrom = pHexCtrl->GetCaretPos() > 0 ? 1ULL : 0ULL;
		m_ullOffsetsTo = pHexCtrl->GetCaretPos();
		m_ullPagesFrom = pHexCtrl->GetPagesCount() > 0 ? (pHexCtrl->GetPagePos() > 0 ? 1ULL : 0ULL) : 0ULL;
		m_ullPagesTo = pHexCtrl->GetPagesCount() > 0 ? pHexCtrl->GetPagePos() : 0ULL;
		break;
	case IDC_HEXCTRL_GOTO_RAD_BACKEND:
		m_ullOffsetsFrom = pHexCtrl->GetDataSize() > 1 ? 1ULL : 0ULL;
		m_ullOffsetsTo = pHexCtrl->GetDataSize() - 1;
		m_ullPagesFrom = pHexCtrl->GetPagesCount() > 1 ? 1ULL : 0ULL;
		m_ullPagesTo = pHexCtrl->GetPagesCount() - 1;
		break;
	default:
		break;
	}

	SetRangesText();
}

void CHexDlgGoTo::SetRangesText()const
{
	GetDlgItem(IDC_HEXCTRL_GOTO_STATIC_OFFRANGE)->SetWindowTextW(std::format(L"{}-{:X}", m_ullOffsetsFrom, m_ullOffsetsTo).data());
	GetDlgItem(IDC_HEXCTRL_GOTO_STATIC_PAGERANGE)->SetWindowTextW(GetHexCtrl()->GetPagesCount() > 0 ?
	std::format(L"{}-{:X}", m_ullPagesFrom, m_ullPagesTo).data() : L"N/A");
}