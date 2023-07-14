/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "../../res/HexCtrlRes.h"
#include "CHexDlgCallback.h"
#include <cassert>
#include <format>

using namespace HEXCTRL::INTERNAL;

BEGIN_MESSAGE_MAP(CHexDlgCallback, CDialogEx)
	ON_COMMAND(IDCANCEL, &CHexDlgCallback::OnBtnCancel)
	ON_WM_TIMER()
END_MESSAGE_MAP()

CHexDlgCallback::CHexDlgCallback(std::wstring_view wsvOperName, ULONGLONG ullProgBarMin,
	ULONGLONG ullProgBarMax, CWnd* pParent) : CDialogEx(IDD_HEXCTRL_CALLBACK, pParent),
	m_wstrOperName(wsvOperName), m_ullProgBarMin(ullProgBarMin), m_ullProgBarCurr(ullProgBarMin), m_ullProgBarMax(ullProgBarMax)
{
	assert(ullProgBarMin <= ullProgBarMax);
}

void CHexDlgCallback::ExitDlg()
{
	OnBtnCancel();
}

bool CHexDlgCallback::IsCanceled()const
{
	return m_fCancel;
}

void CHexDlgCallback::SetProgress(ULONGLONG ullProgCurr)
{
	m_ullProgBarCurr = ullProgCurr;
}


//Private methods.

void CHexDlgCallback::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HEXCTRL_CALLBACK_PROGBAR, m_stProgBar);
}

BOOL CHexDlgCallback::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetWindowTextW(m_wstrOperName.data());
	GetDlgItem(IDC_HEXCTRL_CALLBACK_STATIC_OPERNAME)->SetWindowTextW(m_wstrOperName.data());

	constexpr auto iElapse { 100 }; //Milliseconds for the timer.
	SetTimer(m_uTIDExitCheck, iElapse, nullptr);
	m_iTicksInSecond = 1000 / iElapse;

	constexpr auto iRange { 1000 };
	m_stProgBar.SetRange32(0, iRange);
	m_stProgBar.SetPos(0);

	m_ullThousandth = (m_ullProgBarMax - m_ullProgBarMin) >= iRange ? (m_ullProgBarMax - m_ullProgBarMin) / iRange : 1;

	return TRUE;
}

void CHexDlgCallback::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == m_uTIDExitCheck && m_fCancel) {
		KillTimer(m_uTIDExitCheck);
		OnCancel();
	}

	const auto ullCurDiff = m_ullProgBarCurr - m_ullProgBarMin;
	const auto iPos = static_cast<int>(ullCurDiff / m_ullThousandth); //How many thousandth parts have already passed.
	m_stProgBar.SetPos(iPos);

	constexpr auto iBInMB { 1024 * 1024 }; //Bytes in MB.
	constexpr auto iMBInGB { 1024 };       //MB in GB.
	const auto iSpeedMBS = ((ullCurDiff / ++m_llTicks) * m_iTicksInSecond) / iBInMB; //Speed in MB/s.

	std::wstring wstrDisplay = m_wstrOperName;
	if (iSpeedMBS < 1) { //If speed is less than 1 MB/s.
		wstrDisplay += L"< 1 MB/s";
	}
	else if (iSpeedMBS > iMBInGB) { //More than 1 GB/s.
		wstrDisplay += std::format(L"{:.2f} GB/s", static_cast<float>(iSpeedMBS) / iMBInGB);
	}
	else {
		wstrDisplay += std::format(L"{} MB/s", iSpeedMBS);
	}
	GetDlgItem(IDC_HEXCTRL_CALLBACK_STATIC_OPERNAME)->SetWindowTextW(wstrDisplay.data());

	CDialogEx::OnTimer(nIDEvent);
}

void CHexDlgCallback::OnBtnCancel()
{
	m_fCancel = true;
}