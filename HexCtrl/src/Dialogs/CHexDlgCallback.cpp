/****************************************************************************************
* Copyright © 2018-2024 Jovibor https://github.com/jovibor/                             *
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
	ON_WM_CLOSE()
	ON_WM_CTLCOLOR()
	ON_WM_TIMER()
END_MESSAGE_MAP()

CHexDlgCallback::CHexDlgCallback(std::wstring_view wsvOperName, std::wstring_view wsvCountName,
	ULONGLONG ullProgBarMin, ULONGLONG ullProgBarMax, CWnd* pParent) : CDialogEx(IDD_HEXCTRL_CALLBACK, pParent),
	m_wstrOperName(wsvOperName), m_wstrCountName(wsvCountName), m_ullProgBarMin(ullProgBarMin),
	m_ullProgBarPrev(ullProgBarMin), m_ullProgBarCurr(ullProgBarMin), m_ullProgBarMax(ullProgBarMax)
{
	assert(ullProgBarMin <= ullProgBarMax);
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

void CHexDlgCallback::OnClose()
{
	OnCancel();
}

auto CHexDlgCallback::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)->HBRUSH
{
	const auto hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
	if (pWnd->GetDlgCtrlID() == IDC_HEXCTRL_CALLBACK_STATIC_COUNT) {
		pDC->SetTextColor(RGB(0, 200, 0));
	}

	return hbr;
}

BOOL CHexDlgCallback::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetWindowTextW(m_wstrOperName.data());
	GetDlgItem(IDC_HEXCTRL_CALLBACK_STATIC_OPER)->SetWindowTextW(m_wstrOperName.data());
	GetDlgItem(IDC_HEXCTRL_CALLBACK_STATIC_COUNT)->SetWindowTextW(L"");
	SetTimer(m_uTimerCancelCheck, m_iElapse, nullptr);

	constexpr auto iRange { 1000 };
	m_stProgBar.SetRange32(0, iRange);
	m_stProgBar.SetPos(0);

	const auto ullDiff = m_ullProgBarMax - m_ullProgBarMin;
	m_ullThousandth = ullDiff >= iRange ? ullDiff / iRange : 1;

	m_locale = std::locale("en_US.UTF-8");

	return TRUE;
}

void CHexDlgCallback::OnTimer(UINT_PTR nIDEvent)
{
	CDialogEx::OnTimer(nIDEvent);

	if (nIDEvent != m_uTimerCancelCheck)
		return;

	if (m_fCancel) {
		KillTimer(m_uTimerCancelCheck);
		return EndDialog(IDCANCEL);
	}

	const auto ullCurDiff = m_ullProgBarCurr - m_ullProgBarMin;
	const auto iPos = static_cast<int>(ullCurDiff / m_ullThousandth); //How many thousandth parts have already been passed.
	m_stProgBar.SetPos(iPos);

	constexpr auto uBInKB { 1024U };
	constexpr auto uBInMB { uBInKB * 1024U }; //Bytes in MB.
	constexpr auto uBInGB { uBInMB * 1024U }; //Bytes in GB.
	constexpr auto iTicksInSec = 1000 / m_iElapse;
	const auto ullSpeedBS = (m_ullProgBarCurr - m_ullProgBarPrev) * iTicksInSec; //Speed in B/s.
	m_ullProgBarPrev = m_ullProgBarCurr;

	std::wstring wstrDisplay;
	if (ullSpeedBS < uBInMB) { //Less than 1 MB/s.
		wstrDisplay = std::format(L"{}{} KB/s", m_wstrOperName, ullSpeedBS / uBInKB);
	}
	else if (ullSpeedBS < uBInGB) { //Less than 1 GB/s.
		wstrDisplay = std::format(L"{}{} MB/s", m_wstrOperName, ullSpeedBS / uBInMB);
	}
	else { //More than 1 GB/s.
		wstrDisplay = std::format(L"{}{:.2f} GB/s", m_wstrOperName, ullSpeedBS / static_cast<float>(uBInGB));
	}
	GetDlgItem(IDC_HEXCTRL_CALLBACK_STATIC_OPER)->SetWindowTextW(wstrDisplay.data());

	if (m_ullCount > 0) {
		GetDlgItem(IDC_HEXCTRL_CALLBACK_STATIC_COUNT)->
			SetWindowTextW(std::format(m_locale, L"{}{:L}", m_wstrCountName, m_ullCount).data());
	}
}