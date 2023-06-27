/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "../../res/HexCtrlRes.h"
#include "CHexDlgModify.h"
#include <cassert>

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

BEGIN_MESSAGE_MAP(CHexDlgModify, CDialogEx)
	ON_NOTIFY(TCN_SELCHANGE, IDC_HEXCTRL_MODIFY_TAB, &CHexDlgModify::OnTabSelChanged)
	ON_WM_ACTIVATE()
END_MESSAGE_MAP()

auto CHexDlgModify::GetDlgData()const->std::uint64_t
{
	if (!IsWindow(m_hWnd)) {
		return { };
	}

	return { };
}

void CHexDlgModify::Initialize(IHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	m_pHexCtrl = pHexCtrl;
}

auto CHexDlgModify::SetDlgData(std::uint64_t /*ullData*/)->HWND
{
	if (!IsWindow(m_hWnd)) {
		Create(IDD_HEXCTRL_MODIFY, CWnd::FromHandle(m_pHexCtrl->GetWindowHandle(EHexWnd::WND_MAIN)));
	}

	return m_hWnd;
}

BOOL CHexDlgModify::ShowWindow(int nCmdShow, int iTab)
{
	if (!IsWindow(m_hWnd)) {
		Create(IDD_HEXCTRL_MODIFY, CWnd::FromHandle(m_pHexCtrl->GetWindowHandle(EHexWnd::WND_MAIN)));
	}

	SetCurrentTab(iTab);

	return CDialogEx::ShowWindow(nCmdShow);
}


//Private methods.

void CHexDlgModify::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HEXCTRL_MODIFY_TAB, m_tabMain);
}

void CHexDlgModify::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CDialogEx::OnActivate(nState, pWndOther, bMinimized);

	m_pDlgOpers->OnActivate(nState, pWndOther, bMinimized);
	m_pDlgFillData->OnActivate(nState, pWndOther, bMinimized);
}

BOOL CHexDlgModify::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	assert(m_pHexCtrl != nullptr);
	if (m_pHexCtrl == nullptr)
		return FALSE;

	m_tabMain.InsertItem(0, L"Operations");
	m_tabMain.InsertItem(1, L"Fill with Data");
	CRect rcTab;
	m_tabMain.GetItemRect(0, rcTab);

	m_pDlgOpers->Create(this, m_pHexCtrl);
	m_pDlgOpers->SetWindowPos(nullptr, rcTab.left, rcTab.bottom + 1, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
	m_pDlgOpers->OnActivate(WA_ACTIVE, nullptr, 0); //To properly activate "All-Selection" radios on the first launch.
	m_pDlgFillData->Create(this, m_pHexCtrl);
	m_pDlgFillData->SetWindowPos(nullptr, rcTab.left, rcTab.bottom + 1, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_HIDEWINDOW);
	m_pDlgFillData->OnActivate(WA_ACTIVE, nullptr, 0); //To properly activate "All-Selection" radios on the first launch.

	return TRUE;
}

void CHexDlgModify::OnTabSelChanged(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	SetCurrentTab(m_tabMain.GetCurSel());
}

void CHexDlgModify::SetCurrentTab(int iTab)
{
	m_tabMain.SetCurSel(iTab);
	switch (iTab) {
	case 0:
		m_pDlgFillData->ShowWindow(SW_HIDE);
		m_pDlgOpers->ShowWindow(SW_SHOW);
		break;
	case 1:
		m_pDlgOpers->ShowWindow(SW_HIDE);
		m_pDlgFillData->ShowWindow(SW_SHOW);
		break;
	default:
		break;
	}
}