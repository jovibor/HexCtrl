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

	swprintf_s(pwszBuff, m_fShowAsHex ? L"0x%llX" : L"%llu", m_ullOffset);
	auto pEdit = static_cast<CEdit*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_EDIT_OFFSET));
	pEdit->SetWindowTextW(pwszBuff);

	swprintf_s(pwszBuff, m_fShowAsHex ? L"0x%llX" : L"%llu", m_ullSize);
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
	if (!wstr2num(pwszBuff, ullOffset)) {
		MessageBoxW(L"Invalid offset format", L"Format Error", MB_ICONERROR);
		return;
	}
	pEdit = static_cast<CEdit*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_EDIT_LENGTH));
	pEdit->GetWindowTextW(pwszBuff, NUMBER_MAX_CHARS);
	ULONGLONG ullSize;
	if (!wstr2num(pwszBuff, ullSize)) {
		MessageBoxW(L"Invalid length format", L"Format Error", MB_ICONERROR);
		return;
	}
	if (ullSize == 0) {
		MessageBoxW(L"Length can not be zero!", L"Format Error", MB_ICONERROR);
		return;
	}

	if (m_ullOffset != ullOffset || m_ullSize != ullSize)
	{
		m_pHBS->vecSpan.clear();
		m_pHBS->vecSpan.emplace_back(HEXSPANSTRUCT { ullOffset, ullSize });
	}

	pEdit = static_cast<CEdit*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_EDIT_DESCR));
	pEdit->GetWindowTextW(pwszBuff, static_cast<int>(std::size(pwszBuff)));
	m_pHBS->wstrDesc = pwszBuff;

	CDialogEx::OnOK();
}