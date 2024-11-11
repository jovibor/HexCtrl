/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "../../res/HexCtrlRes.h"
#include "CHexDlgGoTo.h"
#include <cassert>
#include <format>

import HEXCTRL.HexUtility;

using namespace HEXCTRL::INTERNAL;

enum class CHexDlgGoTo::EGoMode : std::uint8_t {
	MODE_OFFSET, MODE_OFFSETFWD, MODE_OFFSETBACK, MODE_OFFSETEND,
	MODE_PAGE, MODE_PAGEFWD, MODE_PAGEBACK, MODE_PAGEEND
};

BEGIN_MESSAGE_MAP(CHexDlgGoTo, CDialogEx)
	ON_WM_ACTIVATE()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CHexDlgGoTo::Initialize(IHexCtrl* pHexCtrl)
{
	assert(pHexCtrl != nullptr);
	if (pHexCtrl == nullptr) {
		return;
	}

	m_pHexCtrl = pHexCtrl;
}

bool CHexDlgGoTo::IsRepeatAvail()const
{
	return m_fRepeat;
}

void CHexDlgGoTo::Repeat(bool fFwd)
{
	if (!IsRepeatAvail()) {
		return;
	}

	GoTo(fFwd);
}

void CHexDlgGoTo::SetDlgProperties(std::uint64_t u64Flags)
{
	m_u64Flags = u64Flags;
}

BOOL CHexDlgGoTo::ShowWindow(int nCmdShow)
{
	if (!IsWindow(m_hWnd)) {
		Create(IDD_HEXCTRL_GOTO, CWnd::FromHandle(m_pHexCtrl->GetWndHandle(EHexWnd::WND_MAIN)));
	}

	return CDialogEx::ShowWindow(nCmdShow);
}


//Private methods.

void CHexDlgGoTo::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HEXCTRL_GOTO_COMBO_MODE, m_comboMode);
}

auto CHexDlgGoTo::GetHexCtrl()const->IHexCtrl*
{
	return m_pHexCtrl;
}

auto CHexDlgGoTo::GetGoMode()const->EGoMode
{
	return static_cast<EGoMode>(m_comboMode.GetItemData(m_comboMode.GetCurSel()));
}

void CHexDlgGoTo::GoTo(bool fForward)
{
	m_fRepeat = false;
	const auto pHexCtrl = GetHexCtrl();

	if (!pHexCtrl->IsCreated() || !pHexCtrl->IsDataSet()) {
		return;
	}

	const auto pEdit = GetDlgItem(IDC_HEXCTRL_GOTO_EDIT_GOTO);
	if (pEdit->GetWindowTextLengthW() == 0) {
		pEdit->SetFocus();
		return;
	}

	CStringW cstr;
	pEdit->GetWindowTextW(cstr);
	const auto optGoTo = stn::StrToUInt64(cstr.GetString());
	if (!optGoTo) {
		pEdit->SetFocus();
		MessageBoxW(L"Invalid number", L"Error", MB_ICONERROR);
		return;
	}

	const auto ullGoTo = *optGoTo;
	const auto iFwdBack = fForward ? 1 : -1;
	const auto ullOffsetCurr = pHexCtrl->GetCaretPos();
	const auto ullDataSize = pHexCtrl->GetDataSize();
	const auto llPageSize = static_cast<LONGLONG>(pHexCtrl->GetPageSize()); //Cast to signed.
	const auto ullPageRem = ullDataSize % llPageSize; //Page remaining size.
	auto ullOffsetResult { 0ULL };

	using enum EGoMode;
	switch (GetGoMode()) {
	case MODE_OFFSET:
		ullOffsetResult = pHexCtrl->GetOffset(ullGoTo, false); //Always use flat offset.
		break;
	case MODE_OFFSETFWD:
		ullOffsetResult = ullOffsetCurr + ullGoTo * iFwdBack;
		break;
	case MODE_OFFSETBACK:
		ullOffsetResult = ullOffsetCurr - ullGoTo * iFwdBack;
		break;
	case MODE_OFFSETEND:
		ullOffsetResult = ullDataSize - ullGoTo - 1;
		break;
	case MODE_PAGE:
		ullOffsetResult = ullGoTo * llPageSize;
		break;
	case MODE_PAGEFWD:
		ullOffsetResult = ullOffsetCurr + ullGoTo * llPageSize * iFwdBack;
		break;
	case MODE_PAGEBACK:
		ullOffsetResult = ullOffsetCurr - ullGoTo * llPageSize * iFwdBack;
		break;
	case MODE_PAGEEND:
		ullOffsetResult = ullDataSize - (ullPageRem > 0 ? ullPageRem : llPageSize) - ullGoTo * llPageSize;
		break;
	default:
		break;
	};

	m_fRepeat = true;

	if (ullOffsetResult >= ullDataSize) { //Overflow.
		return;
	}

	pHexCtrl->SetCaretPos(ullOffsetResult);
	if (!pHexCtrl->IsOffsetVisible(ullOffsetResult)) {
		pHexCtrl->GoToOffset(ullOffsetResult);
	}
}

bool CHexDlgGoTo::IsNoEsc()const
{
	return m_u64Flags & HEXCTRL_FLAG_DLG_NOESC;
}

void CHexDlgGoTo::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	const auto* const pHexCtrl = GetHexCtrl();
	if (!pHexCtrl->IsCreated() || !pHexCtrl->IsDataSet())
		return;

	if (nState == WA_ACTIVE || nState == WA_CLICKACTIVE) {
		UpdateComboMode();
	}

	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
}

void CHexDlgGoTo::OnCancel()
{
	if (IsNoEsc()) //Not closing Dialog on Escape key.
		return;

	CDialogEx::OnCancel();
}

void CHexDlgGoTo::OnClose()
{
	EndDialog(IDCANCEL);
}

void CHexDlgGoTo::OnDestroy()
{
	CDialogEx::OnDestroy();
	m_u64Flags = { };
	m_pHexCtrl = nullptr;
	m_fRepeat = false;
}

BOOL CHexDlgGoTo::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	using enum EGoMode;
	auto iIndex = m_comboMode.AddString(L"Offset");
	m_comboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(MODE_OFFSET));
	m_comboMode.SetCurSel(iIndex);
	iIndex = m_comboMode.AddString(L"Offset forward from the current");
	m_comboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(MODE_OFFSETFWD));
	iIndex = m_comboMode.AddString(L"Offset back from the current");
	m_comboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(MODE_OFFSETBACK));
	iIndex = m_comboMode.AddString(L"Offset from the end");
	m_comboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(MODE_OFFSETEND));
	UpdateComboMode();

	return TRUE;
}

void CHexDlgGoTo::OnOK()
{
	GoTo(true);
}

void CHexDlgGoTo::UpdateComboMode()
{
	constexpr auto iOffsetsTotal = 4; //Total amount of Offset's modes.
	auto iCurrCount = m_comboMode.GetCount();
	const auto fHasPages = iCurrCount > iOffsetsTotal;
	auto fShouldHavePages = GetHexCtrl()->GetPageSize() > 0;
	using enum EGoMode;

	if (fShouldHavePages != fHasPages) {
		m_comboMode.SetRedraw(FALSE);
		if (fShouldHavePages) {
			auto iIndex = m_comboMode.AddString(L"Page");
			m_comboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(MODE_PAGE));
			iIndex = m_comboMode.AddString(L"Page forward from the current");
			m_comboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(MODE_PAGEFWD));
			iIndex = m_comboMode.AddString(L"Page back from the current");
			m_comboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(MODE_PAGEBACK));
			iIndex = m_comboMode.AddString(L"Page from the end");
			m_comboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(MODE_PAGEEND));
		}
		else {
			const auto iCurrSel = m_comboMode.GetCurSel();
			for (auto iIndex = iCurrCount - 1; iIndex >= iOffsetsTotal; --iIndex) {
				m_comboMode.DeleteString(iIndex);
			}

			if (iCurrSel > (iOffsetsTotal - 1)) {
				m_comboMode.SetCurSel(iOffsetsTotal - 1);
				m_fRepeat = false;
			}
		}
		m_comboMode.SetRedraw(TRUE);
		m_comboMode.RedrawWindow();
	}
}