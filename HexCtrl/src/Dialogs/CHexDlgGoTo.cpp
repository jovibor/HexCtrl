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
#include "CHexDlgGoTo.h"
#include <cassert>

using namespace HEXCTRL::INTERNAL;

BEGIN_MESSAGE_MAP(CHexDlgGoTo, CDialogEx)
	ON_COMMAND(IDC_HEXCTRL_GOTO_RADIO_ABS, &CHexDlgGoTo::OnClickRadioAbs)
	ON_COMMAND(IDC_HEXCTRL_GOTO_RADIO_FWDCURR, &CHexDlgGoTo::OnClickRadioFwdCurr)
	ON_COMMAND(IDC_HEXCTRL_GOTO_RADIO_BACKCURR, &CHexDlgGoTo::OnClickRadioBackCurr)
	ON_COMMAND(IDC_HEXCTRL_GOTO_RADIO_BACKEND, &CHexDlgGoTo::OnClickRadioBackEnd)
	ON_WM_ACTIVATE()
END_MESSAGE_MAP()

BOOL CHexDlgGoTo::Create(UINT nIDTemplate, CHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	if (pHexCtrl == nullptr)
		return FALSE;

	m_pHexCtrl = pHexCtrl;

	return CDialogEx::Create(nIDTemplate, pHexCtrl);
}

void CHexDlgGoTo::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BOOL CHexDlgGoTo::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	if (const auto pRadio = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_GOTO_RADIO_OFFSET)); pRadio)
		pRadio->SetCheck(BST_CHECKED);

	if (const auto pRadio = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_GOTO_RADIO_ABS)); pRadio)
		pRadio->SetCheck(BST_CHECKED);

	return TRUE;
}

void CHexDlgGoTo::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	const auto* const pHexCtrl = GetHexCtrl();
	if (!pHexCtrl->IsCreated() || !pHexCtrl->IsDataSet())
		return;

	if (nState == WA_INACTIVE)
		SetLayeredWindowAttributes(0, 200, LWA_ALPHA);
	else
	{
		SetLayeredWindowAttributes(0, 255, LWA_ALPHA);

		wchar_t buff[32];
		//In case of no page size, disable "Page" radio.
		if (pHexCtrl->GetPagesCount() == 0)
		{
			if (const auto pRadio = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_GOTO_RADIO_PAGE)); pRadio)
				pRadio->EnableWindow(FALSE);
			swprintf_s(buff, std::size(buff), L"Not available");
		}
		else
			swprintf_s(buff, std::size(buff), L"0x%llX", pHexCtrl->GetPagesCount());

		GetDlgItem(IDC_HEXCTRL_GOTO_STATIC_PAGETOTAL)->SetWindowTextW(buff);
		swprintf_s(buff, std::size(buff), L"0x%llX", pHexCtrl->GetDataSize());
		GetDlgItem(IDC_HEXCTRL_GOTO_STATIC_OFFTOTAL)->SetWindowTextW(buff);

		switch (GetCheckedRadioButton(IDC_HEXCTRL_GOTO_RADIO_ABS, IDC_HEXCTRL_GOTO_RADIO_BACKEND))
		{
		case IDC_HEXCTRL_GOTO_RADIO_ABS:
			OnClickRadioAbs();
			break;
		case IDC_HEXCTRL_GOTO_RADIO_FWDCURR:
			OnClickRadioFwdCurr();
			break;
		case IDC_HEXCTRL_GOTO_RADIO_BACKCURR:
			OnClickRadioBackCurr();
			break;
		case IDC_HEXCTRL_GOTO_RADIO_BACKEND:
			OnClickRadioBackEnd();
			break;
		}
	}

	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
}

void CHexDlgGoTo::OnOK()
{
	CStringW wstr;
	GetDlgItemTextW(IDC_HEXCTRL_GOTO_EDIT_GOTO, wstr);
	if (!wstr2num(wstr.GetString(), m_ullData)) {
		MessageBox(L"Invalid number format", L"Error", MB_ICONERROR);
		return;
	}

	const auto* const pHexCtrl { GetHexCtrl() };
	auto ullRangeFrom { 0ULL };
	auto ullRangeTo { 0ULL };
	auto ullMul { 0ULL };
	switch (GetCheckedRadioButton(IDC_HEXCTRL_GOTO_RADIO_OFFSET, IDC_HEXCTRL_GOTO_RADIO_PAGE))
	{
	case IDC_HEXCTRL_GOTO_RADIO_OFFSET:
		ullRangeFrom = m_ullOffsetsFrom;
		ullRangeTo = m_ullOffsetsTo;
		ullMul = 1;
		break;
	case IDC_HEXCTRL_GOTO_RADIO_PAGE:
		ullRangeFrom = m_ullPagesFrom;
		ullRangeTo = m_ullPagesTo;
		ullMul = pHexCtrl->GetPageSize();
		break;
	}

	if (m_ullData < ullRangeFrom || m_ullData > ullRangeTo || m_ullData == 0)
	{
		MessageBox(L"Incorrect input range", L"Error", MB_ICONERROR);
		return;
	}

	ULONGLONG ullResult { };
	switch (GetCheckedRadioButton(IDC_HEXCTRL_GOTO_RADIO_ABS, IDC_HEXCTRL_GOTO_RADIO_BACKEND))
	{
	case IDC_HEXCTRL_GOTO_RADIO_ABS:
		ullResult = m_ullData * ullMul;
		m_iRepeat = 0;
		break;
	case IDC_HEXCTRL_GOTO_RADIO_FWDCURR:
		ullResult = pHexCtrl->GetCaretPos() + (m_ullData * ullMul);
		m_iRepeat = 1;
		break;
	case IDC_HEXCTRL_GOTO_RADIO_BACKCURR:
		ullResult = pHexCtrl->GetCaretPos() - (m_ullData * ullMul);
		m_iRepeat = -1;
		break;
	case IDC_HEXCTRL_GOTO_RADIO_BACKEND:
		ullResult = pHexCtrl->GetDataSize() - (m_ullData * ullMul) - 1;
		m_iRepeat = 0;
		break;
	}

	HexCtrlGoOffset(ullResult);

	//To renew static text.
	switch (GetCheckedRadioButton(IDC_HEXCTRL_GOTO_RADIO_ABS, IDC_HEXCTRL_GOTO_RADIO_BACKEND))
	{
	case IDC_HEXCTRL_GOTO_RADIO_ABS:
		OnClickRadioAbs();
		break;
	case IDC_HEXCTRL_GOTO_RADIO_FWDCURR:
		OnClickRadioFwdCurr();
		break;
	case IDC_HEXCTRL_GOTO_RADIO_BACKCURR:
		OnClickRadioBackCurr();
		break;
	case IDC_HEXCTRL_GOTO_RADIO_BACKEND:
		OnClickRadioBackEnd();
		break;
	}
}

void CHexDlgGoTo::OnClickRadioAbs()
{
	const auto* const pHexCtrl = GetHexCtrl();
	m_ullOffsetsFrom = 0ULL;
	m_ullOffsetsTo = pHexCtrl->GetDataSize() - 1;
	m_ullPagesFrom = 0;
	m_ullPagesTo = pHexCtrl->GetPagesCount() > 0 ? pHexCtrl->GetPagesCount() - 1 : 0ULL;

	SetRangesText();
}

void CHexDlgGoTo::OnClickRadioFwdCurr()
{
	const auto* const pHexCtrl = GetHexCtrl();
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
	m_ullOffsetsFrom = pHexCtrl->GetCaretPos() > 0 ? 1ULL : 0ULL;
	m_ullOffsetsTo = pHexCtrl->GetCaretPos();
	m_ullPagesFrom = pHexCtrl->GetPagesCount() > 0 ? (pHexCtrl->GetPagePos() > 0 ? 1ULL : 0ULL) : 0ULL;
	m_ullPagesTo = pHexCtrl->GetPagesCount() > 0 ? pHexCtrl->GetPagePos() : 0ULL;

	SetRangesText();
}

void CHexDlgGoTo::OnClickRadioBackEnd()
{
	const auto* const pHexCtrl = GetHexCtrl();
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
	if (fFwd) //Repeat the last command (forward or backward) as is.
	{
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
		}
	}

	HexCtrlGoOffset(m_ullCurrOffset);
}

CHexCtrl* CHexDlgGoTo::GetHexCtrl()const
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
	wchar_t buff[32];
	swprintf_s(buff, std::size(buff), L"%llu-%llX", m_ullOffsetsFrom, m_ullOffsetsTo);
	GetDlgItem(IDC_HEXCTRL_GOTO_STATIC_OFFRANGE)->SetWindowTextW(buff);

	if (GetHexCtrl()->GetPagesCount() > 0)
		swprintf_s(buff, std::size(buff), L"%llu-%llX", m_ullPagesFrom, m_ullPagesTo);
	else
		swprintf_s(buff, std::size(buff), L"Not available");

	GetDlgItem(IDC_HEXCTRL_GOTO_STATIC_PAGERANGE)->SetWindowTextW(buff);
}