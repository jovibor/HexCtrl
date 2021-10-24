/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "../../res/HexCtrlRes.h"
#include "CHexDlgCallback.h"
#include <cassert>

using namespace HEXCTRL::INTERNAL;

BEGIN_MESSAGE_MAP(CHexDlgCallback, CDialogEx)
	ON_COMMAND(IDCANCEL, &CHexDlgCallback::OnBtnCancel)
	ON_WM_TIMER()
END_MESSAGE_MAP()

CHexDlgCallback::CHexDlgCallback(std::wstring_view wstrOperName, ULONGLONG ullProgBarMin,
	ULONGLONG ullProgBarMax, CWnd* pParent) : CDialogEx(IDD_HEXCTRL_CALLBACK, pParent),
	m_wstrOperName(wstrOperName), m_ullProgBarMin(ullProgBarMin), m_ullProgBarCurr(ullProgBarMin),
	m_ullProgBarMax(ullProgBarMax)
{
	assert(ullProgBarMin <= ullProgBarMax);
}

BOOL CHexDlgCallback::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetWindowTextW(m_wstrOperName.data());
	GetDlgItem(IDC_HEXCTRL_CALLBACK_STATIC_OPERNAME)->SetWindowTextW(m_wstrOperName.data());

	constexpr auto iElapse { 100 }; //Milliseconds for the timer.
	SetTimer(IDT_EXITCHECK, iElapse, nullptr);
	m_iTicksInSecond = 1000 / iElapse;

	constexpr auto iRange { 1000 };
	m_stProgBar.SetRange32(0, iRange);
	m_stProgBar.SetPos(0);

	m_ullThousandth = (m_ullProgBarMax - m_ullProgBarMin) >= iRange ? (m_ullProgBarMax - m_ullProgBarMin) / iRange : 1;

	return TRUE;
}

void CHexDlgCallback::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_HEXCTRL_CALLBACK_PROGBAR, m_stProgBar);
}

void CHexDlgCallback::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == IDT_EXITCHECK && m_fCancel)
	{
		KillTimer(IDT_EXITCHECK);
		OnCancel();
	}

	const auto ullCurDiff = m_ullProgBarCurr - m_ullProgBarMin;
	//How many thousandth parts have already passed.
	auto iPos = static_cast<int>(ullCurDiff / m_ullThousandth);
	m_stProgBar.SetPos(iPos);

	constexpr auto iBytesInMB = 1024 * 1024; //(B in KB) * (KB in MB)
	const auto iSpeedMBS = ((ullCurDiff / ++m_llTicks) * m_iTicksInSecond) / iBytesInMB; //Speed in MB/s.
	std::wstring wstrDisplay = m_wstrOperName;
	constexpr auto iMBInGB = 1024; //How many MB in one GB.
	wchar_t buff[64];

	if (iSpeedMBS < 1)//If speed is less than 1 MB/s.
	{
		wstrDisplay += L"< 1 MB/s";
	}
	else if (iSpeedMBS > iMBInGB) //More than 1 GB/s.
	{
		swprintf_s(buff, std::size(buff), L"%.2f GB/s", static_cast<float>(iSpeedMBS) / iMBInGB);
		wstrDisplay += buff;
	}
	else
	{
		swprintf_s(buff, std::size(buff), L"%lld MB/s", iSpeedMBS);
		wstrDisplay += buff;
	}
	GetDlgItem(IDC_HEXCTRL_CALLBACK_STATIC_OPERNAME)->SetWindowTextW(wstrDisplay.data());

	CDialogEx::OnTimer(nIDEvent);
}

void CHexDlgCallback::OnBtnCancel()
{
	m_fCancel = true;
}

bool CHexDlgCallback::IsCanceled()const
{
	return m_fCancel;
}

void CHexDlgCallback::SetProgress(ULONGLONG ullProgCurr)
{
	m_ullProgBarCurr = ullProgCurr;
}

void CHexDlgCallback::ExitDlg()
{
	OnBtnCancel();
}