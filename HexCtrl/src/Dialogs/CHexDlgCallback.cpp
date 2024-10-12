/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "../../res/HexCtrlRes.h"
#include "CHexDlgCallback.h"
#include <cassert>
#include <format>

import HEXCTRL.HexUtility;

using namespace HEXCTRL::INTERNAL;

BEGIN_MESSAGE_MAP(CHexDlgCallback, CDialogEx)
	ON_WM_CLOSE()
	ON_WM_CTLCOLOR()
	ON_WM_TIMER()
END_MESSAGE_MAP()

CHexDlgCallback::CHexDlgCallback(std::wstring_view wsvOperName, std::wstring_view wsvCountName,
	ULONGLONG ullMin, ULONGLONG ullMax, CWnd* pParent) : CDialogEx(IDD_HEXCTRL_CALLBACK, pParent),
	m_wstrOperName(wsvOperName), m_wstrCountName(wsvCountName), m_ullMin(ullMin),
	m_ullPrev(ullMin), m_ullCurr(ullMin), m_ullMax(ullMax)
{
	assert(ullMin <= ullMax);
}

bool CHexDlgCallback::IsCanceled()const
{
	return m_fCancel;
}

void CHexDlgCallback::OnCancel()
{
	m_fCancel = true;
}

void CHexDlgCallback::SetCount(ULONGLONG ullCount)
{
	m_ullCount = ullCount;
}

void CHexDlgCallback::SetCurrent(ULONGLONG ullCurr)
{
	m_ullCurr = ullCurr;
}


//Private methods.

void CHexDlgCallback::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HEXCTRL_CALLBACK_PROGBAR, m_stProgBar);
}

void CHexDlgCallback::OnClose()
{
	OnCancel();
}

auto CHexDlgCallback::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)->HBRUSH
{
	const auto hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	if (pWnd->GetDlgCtrlID() == IDC_HEXCTRL_CALLBACK_STAT_COUNT) {
		pDC->SetTextColor(RGB(0, 200, 0));
	}

	return hbr;
}

BOOL CHexDlgCallback::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetWindowTextW(m_wstrOperName.data());
	GetDlgItem(IDC_HEXCTRL_CALLBACK_STAT_OPER)->SetWindowTextW(m_wstrOperName.data());
	GetDlgItem(IDC_HEXCTRL_CALLBACK_STAT_COUNT)->SetWindowTextW(L"");
	SetTimer(m_uIDTCancelCheck, m_uElapse, nullptr);

	constexpr auto iRange1000 { 1000 };
	m_stProgBar.SetRange32(0, iRange1000);
	m_stProgBar.SetPos(0);

	//How many thousands in the whole diapason.
	const auto ullDiapason = m_ullMax - m_ullMin;
	m_ullThousands = ullDiapason >= iRange1000 ? ullDiapason / iRange1000 : 1;

	return TRUE;
}

void CHexDlgCallback::OnTimer(UINT_PTR nIDEvent)
{
	CDialogEx::OnTimer(nIDEvent);

	if (nIDEvent != m_uIDTCancelCheck)
		return;

	if (m_fCancel) {
		KillTimer(m_uIDTCancelCheck);
		EndDialog(IDCANCEL);
		return;
	}

	const auto ullCurr = m_ullCurr - m_ullMin;
	const auto iPos = static_cast<int>(ullCurr / m_ullThousands); //How many thousandth parts have already been passed.
	m_stProgBar.SetPos(iPos);

	static constexpr auto uBInKB { 1024U };          //Bytes in KB.
	static constexpr auto uBInMB { uBInKB * 1024U }; //Bytes in MB.
	static constexpr auto uBInGB { uBInMB * 1024U }; //Bytes in GB.
	static constexpr auto uTicksInSec = 1000U / m_uElapse;
	const auto ullSpeedBS = (m_ullCurr - m_ullPrev) * uTicksInSec; //Speed in Bytes/s.
	m_ullPrev = m_ullCurr;

	std::wstring wstrDisplay;
	if (ullSpeedBS < uBInMB) { //Less than 1 MB/s.
		wstrDisplay = std::format(L"{}{} KB/s", m_wstrOperName, ullSpeedBS / uBInKB);
	}
	else if (ullSpeedBS < uBInGB) { //Less than 1 GB/s.
		wstrDisplay = std::format(L"{}{} MB/s", m_wstrOperName, ullSpeedBS / uBInMB);
	}
	else { //More than or equal to 1 GB/s.
		wstrDisplay = std::format(L"{}{:.2f} GB/s", m_wstrOperName, ullSpeedBS / static_cast<float>(uBInGB));
	}
	GetDlgItem(IDC_HEXCTRL_CALLBACK_STAT_OPER)->SetWindowTextW(wstrDisplay.data());

	if (m_ullCount > 0) {
		GetDlgItem(IDC_HEXCTRL_CALLBACK_STAT_COUNT)->
			SetWindowTextW(std::format(GetLocale(), L"{}{:L}", m_wstrCountName, m_ullCount).data());
	}
}