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
	ON_COMMAND(IDC_HEXCTRL_GOTO_RAD_ABS, &CHexDlgGoTo::OnClickRadioAbs)
	ON_COMMAND(IDC_HEXCTRL_GOTO_RAD_FWDCURR, &CHexDlgGoTo::OnClickRadioFwdCurr)
	ON_COMMAND(IDC_HEXCTRL_GOTO_RAD_BACKCURR, &CHexDlgGoTo::OnClickRadioBackCurr)
	ON_COMMAND(IDC_HEXCTRL_GOTO_RAD_BACKEND, &CHexDlgGoTo::OnClickRadioBackEnd)
	ON_WM_ACTIVATE()
END_MESSAGE_MAP()

BOOL CHexDlgGoTo::Create(UINT nIDTemplate, CWnd* pParent, IHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	if (pHexCtrl == nullptr)
		return FALSE;

	m_pHexCtrl = pHexCtrl;

	return CDialogEx::Create(nIDTemplate, pParent);
}

void CHexDlgGoTo::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BOOL CHexDlgGoTo::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	if (const auto pRadio = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_GOTO_RAD_OFFSET)); pRadio)
		pRadio->SetCheck(BST_CHECKED);

	if (const auto pRadio = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_GOTO_RAD_ABS)); pRadio)
		pRadio->SetCheck(BST_CHECKED);

	return TRUE;
}

void CHexDlgGoTo::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if (nState == WA_INACTIVE)
		SetLayeredWindowAttributes(0, 200, LWA_ALPHA);
	else {
		SetLayeredWindowAttributes(0, 255, LWA_ALPHA);

		const auto* const pHexCtrl = GetHexCtrl();
		if (pHexCtrl->IsCreated() && pHexCtrl->IsDataSet()) {
			std::wstring wstr;
			//In case of no page size, disable "Page" radio.
			if (pHexCtrl->GetPagesCount() == 0) {
				if (const auto pRadio = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_GOTO_RAD_PAGE)); pRadio)
					pRadio->EnableWindow(FALSE);
				wstr = L"Not available";
			}
			else
				wstr = std::format(L"0x{:X}", pHexCtrl->GetPagesCount());

			GetDlgItem(IDC_HEXCTRL_GOTO_STATIC_PAGETOTAL)->SetWindowTextW(wstr.data());
			wstr = std::format(L"0x{:X}", pHexCtrl->GetDataSize());
			GetDlgItem(IDC_HEXCTRL_GOTO_STATIC_OFFTOTAL)->SetWindowTextW(wstr.data());

			switch (GetCheckedRadioButton(IDC_HEXCTRL_GOTO_RAD_ABS, IDC_HEXCTRL_GOTO_RAD_BACKEND)) {
			case IDC_HEXCTRL_GOTO_RAD_ABS:
				OnClickRadioAbs();
				break;
			case IDC_HEXCTRL_GOTO_RAD_FWDCURR:
				OnClickRadioFwdCurr();
				break;
			case IDC_HEXCTRL_GOTO_RAD_BACKCURR:
				OnClickRadioBackCurr();
				break;
			case IDC_HEXCTRL_GOTO_RAD_BACKEND:
				OnClickRadioBackEnd();
				break;
			default:
				break;
			}
		}
	}

	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
}

void CHexDlgGoTo::OnOK()
{
	const auto* const pHexCtrl = GetHexCtrl();
	if (!pHexCtrl->IsCreated() || !pHexCtrl->IsDataSet())
		return;

	CStringW wstr;
	GetDlgItemTextW(IDC_HEXCTRL_GOTO_EDIT_GOTO, wstr);
	const auto optData = StrToULL(wstr.GetString());
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
	switch (GetCheckedRadioButton(IDC_HEXCTRL_GOTO_RAD_ABS, IDC_HEXCTRL_GOTO_RAD_BACKEND)) {
	case IDC_HEXCTRL_GOTO_RAD_ABS:
		OnClickRadioAbs();
		break;
	case IDC_HEXCTRL_GOTO_RAD_FWDCURR:
		OnClickRadioFwdCurr();
		break;
	case IDC_HEXCTRL_GOTO_RAD_BACKCURR:
		OnClickRadioBackCurr();
		break;
	case IDC_HEXCTRL_GOTO_RAD_BACKEND:
		OnClickRadioBackEnd();
		break;
	default:
		break;
	}
}

void CHexDlgGoTo::OnClickRadioAbs()
{
	const auto* const pHexCtrl = GetHexCtrl();
	if (!pHexCtrl->IsDataSet())
		return;

	m_ullOffsetsFrom = 0ULL;
	m_ullOffsetsTo = pHexCtrl->GetDataSize() - 1;
	m_ullPagesFrom = 0;
	m_ullPagesTo = pHexCtrl->GetPagesCount() > 0 ? pHexCtrl->GetPagesCount() - 1 : 0ULL;

	SetRangesText();
}

void CHexDlgGoTo::OnClickRadioFwdCurr()
{
	const auto* const pHexCtrl = GetHexCtrl();
	if (!pHexCtrl->IsDataSet())
		return;

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
	SetRangesText();
}

void CHexDlgGoTo::OnClickRadioBackCurr()
{
	const auto* const pHexCtrl = GetHexCtrl();
	if (!pHexCtrl->IsDataSet())
		return;

	m_ullOffsetsFrom = pHexCtrl->GetCaretPos() > 0 ? 1ULL : 0ULL;
	m_ullOffsetsTo = pHexCtrl->GetCaretPos();
	m_ullPagesFrom = pHexCtrl->GetPagesCount() > 0 ? (pHexCtrl->GetPagePos() > 0 ? 1ULL : 0ULL) : 0ULL;
	m_ullPagesTo = pHexCtrl->GetPagesCount() > 0 ? pHexCtrl->GetPagePos() : 0ULL;

	SetRangesText();
}

void CHexDlgGoTo::OnClickRadioBackEnd()
{
	const auto* const pHexCtrl = GetHexCtrl();
	if (!pHexCtrl->IsDataSet())
		return;

	m_ullOffsetsFrom = pHexCtrl->GetDataSize() > 1 ? 1ULL : 0ULL;
	m_ullOffsetsTo = pHexCtrl->GetDataSize() - 1;
	m_ullPagesFrom = pHexCtrl->GetPagesCount() > 1 ? 1ULL : 0ULL;
	m_ullPagesTo = pHexCtrl->GetPagesCount() - 1;

	SetRangesText();
}

bool CHexDlgGoTo::IsRepeatAvail()const
{
	return m_iRepeat != 0;
}

void CHexDlgGoTo::Repeat(bool fFwd)
{
	if (!IsRepeatAvail())
		return;

	const auto* const pHexCtrl = GetHexCtrl();
	if (fFwd) { //Repeat the last command (forward or backward) as is.
		switch (m_iRepeat) {
		case -1:
			if (m_ullCurrOffset < m_ullData) //To avoid underflow.
				return;
			m_ullCurrOffset -= m_ullData;
			break;
		case 1:
			if (m_ullCurrOffset + m_ullData >= pHexCtrl->GetDataSize()) //To avoid overflow.
				return;
			m_ullCurrOffset += m_ullData;
			break;
		default:
			break;
		}
	}
	else //Repeat opposite of the last command (forward<->backward).
	{
		switch (m_iRepeat) {
		case -1:
			if (m_ullCurrOffset + m_ullData >= pHexCtrl->GetDataSize()) //To avoid overflow.
				return;
			m_ullCurrOffset += m_ullData;
			break;
		case 1:
			if (m_ullCurrOffset < m_ullData) //To avoid underflow.
				return;
			m_ullCurrOffset -= m_ullData;
			break;
		default:
			break;
		}
	}

	HexCtrlGoOffset(m_ullCurrOffset);
}

IHexCtrl* CHexDlgGoTo::GetHexCtrl()const
{
	return m_pHexCtrl;
}

void CHexDlgGoTo::HexCtrlGoOffset(ULONGLONG ullOffset)
{
	const auto pHexCtrl = GetHexCtrl();

	pHexCtrl->SetCaretPos(ullOffset);
	if (!pHexCtrl->IsOffsetVisible(ullOffset))
		pHexCtrl->GoToOffset(ullOffset);

	m_ullCurrOffset = ullOffset;
}

void CHexDlgGoTo::SetRangesText()const
{
	std::wstring wstr = std::format(L"{}-{:X}", m_ullOffsetsFrom, m_ullOffsetsTo);
	GetDlgItem(IDC_HEXCTRL_GOTO_STATIC_OFFRANGE)->SetWindowTextW(wstr.data());

	if (GetHexCtrl()->GetPagesCount() > 0)
		wstr = std::format(L"{}-{:X}", m_ullPagesFrom, m_ullPagesTo);
	else
		wstr = L"Not available";

	GetDlgItem(IDC_HEXCTRL_GOTO_STATIC_PAGERANGE)->SetWindowTextW(wstr.data());
}