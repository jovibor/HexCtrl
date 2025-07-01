/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
module;
#include <SDKDDKVer.h>
#include "../res/HexCtrlRes.h"
#include <Windows.h>
#include <cassert>
#include <format>
#include <string>
export module HEXCTRL:CHexDlgProgress;

import :HexUtility;
using namespace HEXCTRL::INTERNAL;

namespace HEXCTRL::INTERNAL {
	class CHexDlgProgress final {
	public:
		explicit CHexDlgProgress(std::wstring_view wsvOperName, std::wstring_view wsvCountName,
			ULONGLONG ullMin, ULONGLONG ullMax);
		auto DoModal(HWND hWndParent, HINSTANCE hInstRes) -> INT_PTR;
		[[nodiscard]] bool IsCanceled()const;
		void OnCancel();
		[[nodiscard]] auto ProcessMsg(const MSG& msg) -> INT_PTR;
		void SetCount(ULONGLONG ullCount);
		void SetCurrent(ULONGLONG ullCurr); //Set current data in the whole data diapason.
	private:
		auto OnClose() -> INT_PTR;
		auto OnCommand(const MSG& msg) -> INT_PTR;
		auto OnCtlClrStatic(const MSG& msg) -> INT_PTR;
		auto OnInitDialog(const MSG& msg) -> INT_PTR;
		auto OnTimer(const MSG& msg) -> INT_PTR;
	private:
		static constexpr UINT_PTR m_uIDTCancelCheck { 0x1 };
		static constexpr auto m_uElapse { 100U }; //Milliseconds for the timer.
		GDIUT::CWnd m_Wnd;
		GDIUT::CWnd m_WndOper;          //Static Operation.
		GDIUT::CWnd m_WndCount;         //Static Count.
		GDIUT::CWndProgBar m_stProgBar; //Progress bar.
		std::wstring m_wstrOperName;
		std::wstring m_wstrCountName; //Count name (e.g. Found, Replaced, etc...).
		ULONGLONG m_ullMin { };       //Minimum data amount. 
		ULONGLONG m_ullPrev { };      //Previous data in bytes.
		ULONGLONG m_ullCurr { };      //Current data amount processed.
		ULONGLONG m_ullMax { };       //Max data amount.
		ULONGLONG m_ullThousands { }; //How many thousands in the whole data diapason.
		ULONGLONG m_ullCount { };     //Count of found/replaced items.
		bool m_fCancel { false };     //"Cancel" button pressed.
	};
}

CHexDlgProgress::CHexDlgProgress(std::wstring_view wsvOperName, std::wstring_view wsvCountName,
	ULONGLONG ullMin, ULONGLONG ullMax) : m_wstrOperName(wsvOperName), m_wstrCountName(wsvCountName),
	m_ullMin(ullMin), m_ullPrev(ullMin), m_ullCurr(ullMin), m_ullMax(ullMax) {
	assert(ullMin <= ullMax);
}

auto CHexDlgProgress::DoModal(HWND hWndParent, HINSTANCE hInstRes)->INT_PTR {
	return ::DialogBoxParamW(hInstRes, MAKEINTRESOURCEW(IDD_HEXCTRL_PROGRESS),
		hWndParent, GDIUT::DlgProc<CHexDlgProgress>, reinterpret_cast<LPARAM>(this));
}

bool CHexDlgProgress::IsCanceled()const
{
	return m_fCancel;
}

void CHexDlgProgress::OnCancel()
{
	m_fCancel = true;
}

auto CHexDlgProgress::ProcessMsg(const MSG& msg)->INT_PTR
{
	switch (msg.message) {
	case WM_CLOSE: return OnClose();
	case WM_COMMAND: return OnCommand(msg);
	case WM_CTLCOLORSTATIC: return OnCtlClrStatic(msg);
	case WM_INITDIALOG: return OnInitDialog(msg);
	case WM_TIMER: return OnTimer(msg);
	default:
		return 0;
	}
}

void CHexDlgProgress::SetCount(ULONGLONG ullCount)
{
	m_ullCount = ullCount;
}

void CHexDlgProgress::SetCurrent(ULONGLONG ullCurr)
{
	m_ullCurr = ullCurr;
}


//Private methods.

auto CHexDlgProgress::OnClose()->INT_PTR
{
	OnCancel();
	return TRUE;
}

auto CHexDlgProgress::OnCommand(const MSG& msg)->INT_PTR
{
	const auto uCtrlID = LOWORD(msg.wParam);
	switch (uCtrlID) {
	case IDOK:
	case IDCANCEL:
		OnCancel();
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

auto CHexDlgProgress::OnCtlClrStatic(const MSG& msg)->INT_PTR
{
	if (const auto hWndFrom = reinterpret_cast<HWND>(msg.lParam); hWndFrom == m_WndCount) {
		const auto hDC = reinterpret_cast<HDC>(msg.wParam);
		::SetTextColor(hDC, RGB(0, 200, 0));
		::SetBkColor(hDC, ::GetSysColor(COLOR_3DFACE));
		return reinterpret_cast<INT_PTR>(::GetSysColorBrush(COLOR_3DFACE));
	}

	return FALSE; //Default handler.
}

auto CHexDlgProgress::OnInitDialog(const MSG& msg)->INT_PTR
{
	m_Wnd.Attach(msg.hwnd);
	m_stProgBar.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_CALLBACK_PROGBAR));
	m_Wnd.SetWndText(m_wstrOperName.data());
	m_WndOper.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_CALLBACK_STAT_OPER));
	m_WndOper.SetWndText(m_wstrOperName.data());
	m_WndCount.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_CALLBACK_STAT_COUNT));
	m_WndCount.SetWndText(L"");
	m_Wnd.SetTimer(m_uIDTCancelCheck, m_uElapse, nullptr);

	constexpr auto iRange1000 { 1000 };
	m_stProgBar.SetRange(0, iRange1000);
	m_stProgBar.SetPos(0);

	//How many thousands in the whole diapason.
	const auto ullDiapason = m_ullMax - m_ullMin;
	m_ullThousands = ullDiapason >= iRange1000 ? ullDiapason / iRange1000 : 1;

	return TRUE;
}

auto CHexDlgProgress::OnTimer(const MSG& msg)->INT_PTR
{
	if (msg.wParam != m_uIDTCancelCheck)
		return FALSE;

	if (m_fCancel) {
		m_Wnd.KillTimer(m_uIDTCancelCheck);
		m_Wnd.EndDialog(IDCANCEL);
		return TRUE;
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
	m_WndOper.SetWndText(wstrDisplay);

	if (m_ullCount > 0) {
		m_WndCount.SetWndText(std::format(ut::GetLocale(), L"{}{:L}", m_wstrCountName, m_ullCount));
	}

	return TRUE;
}