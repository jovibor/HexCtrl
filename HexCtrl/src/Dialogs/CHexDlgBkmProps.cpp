/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#include "stdafx.h"
#include "../Helper.h"
#include "CHexDlgBkmProps.h"
#include <cassert>
#include <numeric>

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

BEGIN_MESSAGE_MAP(CHexDlgBkmProps, CDialogEx)
	ON_COMMAND(IDC_HEXCTRL_BKMPROPS_RADIO_DEC, &CHexDlgBkmProps::OnClickRadioDec)
	ON_COMMAND(IDC_HEXCTRL_BKMPROPS_RADIO_HEX, &CHexDlgBkmProps::OnClickRadioHex)
END_MESSAGE_MAP()

INT_PTR CHexDlgBkmProps::DoModal(HEXBKMSTRUCT& hbs, bool fShowAsHex)
{
	m_pHBS = &hbs;
	m_fShowAsHex = fShowAsHex;

	return CDialogEx::DoModal();
}

void CHexDlgBkmProps::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BOOL CHexDlgBkmProps::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	
	if (m_fShowAsHex)
	{
		if (auto pRadio = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_RADIO_HEX)); pRadio)
			pRadio->SetCheck(BST_CHECKED);
	}
	else
	{
		if (auto pRadio = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_RADIO_DEC)); pRadio)
			pRadio->SetCheck(BST_CHECKED);
	}

	if (auto pClrBtn = static_cast<CMFCColorButton*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_COLOR_BK)); pClrBtn != nullptr)
		pClrBtn->SetColor(m_pHBS->clrBk);
	if (auto pClrBtn = static_cast<CMFCColorButton*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_COLOR_TEXT)); pClrBtn != nullptr)
		pClrBtn->SetColor(m_pHBS->clrText);

	wchar_t pwszBuff[32];
	if (!m_pHBS->vecSpan.empty())
	{
		m_ullOffset = m_pHBS->vecSpan.front().ullOffset;
		m_ullSize = std::accumulate(m_pHBS->vecSpan.begin(), m_pHBS->vecSpan.end(), ULONGLONG { },
			[](auto ullTotal, const HEXSPANSTRUCT& ref) {return ullTotal + ref.ullSize; });
	}
	else
	{
		m_ullOffset = 0;
		m_ullSize = 1;
		m_pHBS->vecSpan.emplace_back(HEXSPANSTRUCT { m_ullOffset, m_ullSize });
	}

	if (m_fShowAsHex)
		swprintf_s(pwszBuff, L"0x%llX", m_ullOffset);
	else
		swprintf_s(pwszBuff, L"%llu", m_ullOffset);
	auto pEdit = static_cast<CEdit*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_EDIT_OFFSET));
	pEdit->SetWindowTextW(pwszBuff);

	if (m_fShowAsHex)
		swprintf_s(pwszBuff, L"0x%llX", m_ullSize);
	else
		swprintf_s(pwszBuff, L"%llu", m_ullSize);
	pEdit = static_cast<CEdit*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_EDIT_LENGTH));
	pEdit->SetWindowTextW(pwszBuff);

	pEdit = static_cast<CEdit*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_EDIT_DESCR));
	pEdit->SetWindowTextW(m_pHBS->wstrDesc.data());

	return TRUE;
}

void CHexDlgBkmProps::OnOK()
{
	wchar_t pwszBuff[512];
	constexpr auto NUMBER_MAX_CHARS = 32; //Text limit 32 chars.

	auto pClrBtn = static_cast<CMFCColorButton*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_COLOR_BK));
	m_pHBS->clrBk = pClrBtn->GetColor();
	pClrBtn = static_cast<CMFCColorButton*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_COLOR_TEXT));
	m_pHBS->clrText = pClrBtn->GetColor();

	auto pEdit = static_cast<CEdit*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_EDIT_OFFSET));
	pEdit->GetWindowTextW(pwszBuff, NUMBER_MAX_CHARS);
	ULONGLONG ullOffset;
	if (!wstr2num(pwszBuff, ullOffset, m_fShowAsHex ? 16 : 10))
	{
		MessageBoxW(L"Invalid offset format", L"Format Error", MB_ICONERROR);
		return;
	}
	pEdit = static_cast<CEdit*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_EDIT_LENGTH));
	pEdit->GetWindowTextW(pwszBuff, NUMBER_MAX_CHARS);
	ULONGLONG ullSize;
	if (!wstr2num(pwszBuff, ullSize, m_fShowAsHex ? 16 : 10))
	{
		MessageBoxW(L"Invalid length format", L"Format Error", MB_ICONERROR);
		return;
	}
	if (ullSize == 0)
	{
		MessageBoxW(L"Length can not be zero!", L"Format Error", MB_ICONERROR);
		return;
	}

	if (m_ullOffset != ullOffset || m_ullSize != ullSize)
	{
		m_pHBS->vecSpan.clear();
		m_pHBS->vecSpan.emplace_back(HEXSPANSTRUCT { ullOffset, ullSize });
	}

	pEdit = static_cast<CEdit*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_EDIT_DESCR));
	pEdit->GetWindowTextW(pwszBuff, _countof(pwszBuff));
	m_pHBS->wstrDesc = pwszBuff;

	CDialogEx::OnOK();
}

//NB: This is somewhat convoluted. It would be better to use a common control for all numeric input
//    This would reduce code complexity and make the UI more consistent
//
void CHexDlgBkmProps::ChangeDisplayMode(bool fShowAsHex)
{
	wchar_t pwszBuff[512];
	constexpr auto NUMBER_MAX_CHARS = 32; //Text limit 32 chars.

	//Evaluate offset and length using current display mode
	//Invalid values are set to blank on conversion rather than display a pop-up
	//

	auto pOffset = static_cast<CEdit*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_EDIT_OFFSET));
	pOffset->GetWindowTextW(pwszBuff, NUMBER_MAX_CHARS);
	ULONGLONG ullOffset;	
	if (wstr2num(pwszBuff, ullOffset, m_fShowAsHex ? 16 : 10))
	{
		if (fShowAsHex)
			swprintf_s(pwszBuff, L"0x%llX", ullOffset);
		else
			swprintf_s(pwszBuff, L"%llu", ullOffset);
	}
	else
		pwszBuff[0] = L'\0';
	pOffset->SetWindowTextW(pwszBuff);

	auto pLength = static_cast<CEdit*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_EDIT_LENGTH));
	pLength->GetWindowTextW(pwszBuff, NUMBER_MAX_CHARS);
	ULONGLONG ullSize;	
	if (wstr2num(pwszBuff, ullSize, m_fShowAsHex ? 16 : 10))
	{
		if (fShowAsHex)
			swprintf_s(pwszBuff, L"0x%llX", ullSize);
		else
			swprintf_s(pwszBuff, L"%llu", ullSize);
	}	
	else
		pwszBuff[0] = L'\0';
	pLength->SetWindowTextW(pwszBuff);

	auto pHexRadio = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_RADIO_HEX));
	auto pDecRadio = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_RADIO_DEC));
	if (pHexRadio && pDecRadio)
	{
		pHexRadio->SetCheck(fShowAsHex ? BST_CHECKED : BST_UNCHECKED);
		pDecRadio->SetCheck(fShowAsHex ? BST_UNCHECKED : BST_CHECKED);
	}

	//Finally, change display mode
	m_fShowAsHex = fShowAsHex;
}

void CHexDlgBkmProps::OnClickRadioDec()
{	
	ChangeDisplayMode(false);
}

void CHexDlgBkmProps::OnClickRadioHex()
{
	ChangeDisplayMode(true);
}