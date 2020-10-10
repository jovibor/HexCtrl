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
END_MESSAGE_MAP()

CHexDlgGoTo::CHexDlgGoTo(CHexCtrl* pHexCtrl) : CDialogEx(IDD_HEXCTRL_GOTO, pHexCtrl)
{
	assert(pHexCtrl);
	if (pHexCtrl == nullptr)
		return;

	m_pHexCtrl = pHexCtrl;
}

void CHexDlgGoTo::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BOOL CHexDlgGoTo::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	auto pHexCtrl = GetHexCtrl();
	if (auto pRadio = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_GOTO_RADIO_OFFSET)); pRadio)
		pRadio->SetCheck(BST_CHECKED);

	wchar_t buff[32];
	//In case of no page size, disable "Page" radio.
	if (pHexCtrl->GetPagesCount() == 0)
	{
		if (auto pRadio = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_GOTO_RADIO_PAGE)); pRadio)
			pRadio->EnableWindow(FALSE);
		swprintf_s(buff, std::size(buff), L"Not available");
	}
	else
		swprintf_s(buff, std::size(buff), L"0x%llX", pHexCtrl->GetPagesCount());

	GetDlgItem(IDC_HEXCTRL_GOTO_STATIC_PAGETOTAL)->SetWindowTextW(buff);
	swprintf_s(buff, std::size(buff), L"0x%llX", pHexCtrl->GetDataSize());
	GetDlgItem(IDC_HEXCTRL_GOTO_STATIC_OFFTOTAL)->SetWindowTextW(buff);

	if (auto pRadio = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_GOTO_RADIO_ABS)); pRadio)
		pRadio->SetCheck(BST_CHECKED);

	OnClickRadioAbs();

	return TRUE;
}

void CHexDlgGoTo::OnOK()
{
	auto pHexCtrl { GetHexCtrl() };
	CStringW wstr;
	GetDlgItemTextW(IDC_HEXCTRL_GOTO_EDIT_GOTO, wstr);
	auto ullData { 0ULL };
	if (!wstr2num(wstr.GetString(), ullData)) {
		MessageBox(L"Invalid number format", L"Error", MB_ICONERROR);
		return;
	}

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

	if (ullData < ullRangeFrom || ullData>ullRangeTo || ullData == 0)
	{
		MessageBox(L"Incorrect input range", L"Error", MB_ICONERROR);
		return;
	}

	switch (GetCheckedRadioButton(IDC_HEXCTRL_GOTO_RADIO_ABS, IDC_HEXCTRL_GOTO_RADIO_BACKEND))
	{
	case IDC_HEXCTRL_GOTO_RADIO_ABS:
		m_ullResult = ullData * ullMul;
		break;
	case IDC_HEXCTRL_GOTO_RADIO_FWDCURR:
		m_ullResult = pHexCtrl->GetCaretPos() + (ullData * ullMul);
		break;
	case IDC_HEXCTRL_GOTO_RADIO_BACKCURR:
		m_ullResult = pHexCtrl->GetCaretPos() - (ullData * ullMul);
		break;
	case IDC_HEXCTRL_GOTO_RADIO_BACKEND:
		m_ullResult = pHexCtrl->GetDataSize() - (ullData * ullMul) - 1;
		break;
	}

	CDialogEx::OnOK();
}

void CHexDlgGoTo::OnClickRadioAbs()
{
	auto pHexCtrl = GetHexCtrl();
	m_ullOffsetsFrom = 0ULL;
	m_ullOffsetsTo = pHexCtrl->GetDataSize() - 1;
	m_ullPagesFrom = 0;
	m_ullPagesTo = pHexCtrl->GetPagesCount() > 0 ? pHexCtrl->GetPagesCount() - 1 : 0ULL;

	SetRangesText();
}

void CHexDlgGoTo::OnClickRadioFwdCurr()
{
	auto pHexCtrl = GetHexCtrl();
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
	auto pHexCtrl = GetHexCtrl();
	m_ullOffsetsFrom = pHexCtrl->GetCaretPos() > 0 ? 1ULL : 0ULL;
	m_ullOffsetsTo = pHexCtrl->GetCaretPos();
	m_ullPagesFrom = pHexCtrl->GetPagesCount() > 0 ? (pHexCtrl->GetPagePos() > 0 ? 1ULL : 0ULL) : 0ULL;
	m_ullPagesTo = pHexCtrl->GetPagesCount() > 0 ? pHexCtrl->GetPagePos() : 0ULL;

	SetRangesText();
}

void CHexDlgGoTo::OnClickRadioBackEnd()
{
	auto pHexCtrl = GetHexCtrl();
	m_ullOffsetsFrom = pHexCtrl->GetDataSize() > 1 ? 1ULL : 0ULL;
	m_ullOffsetsTo = pHexCtrl->GetDataSize() - 1;
	m_ullPagesFrom = pHexCtrl->GetPagesCount() > 1 ? 1ULL : 0ULL;
	m_ullPagesTo = pHexCtrl->GetPagesCount() - 1;

	SetRangesText();
}

ULONGLONG CHexDlgGoTo::GetResult()const
{
	return m_ullResult;
}

CHexCtrl* CHexDlgGoTo::GetHexCtrl()const
{
	return m_pHexCtrl;
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