/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "../../res/HexCtrlRes.h"
#include "CHexDlgGoTo.h"
#include <cassert>
#include <format>

using namespace HEXCTRL::INTERNAL;

enum class CHexDlgGoTo::EGoMode : std::uint8_t {
	MODE_OFFSET, MODE_OFFSETFWD, MODE_OFFSETBACK, MODE_OFFSETEND,
	MODE_PAGE, MODE_PAGEFWD, MODE_PAGEBACK, MODE_PAGEEND
};

void CHexDlgGoTo::CreateDlg()
{
	//m_Wnd is set in the OnInitDialog().
	if (const auto hWnd = ::CreateDialogParamW(m_hInstRes, MAKEINTRESOURCEW(IDD_HEXCTRL_GOTO),
		m_pHexCtrl->GetWndHandle(EHexWnd::WND_MAIN), wnd::DlgProc<CHexDlgGoTo>, reinterpret_cast<LPARAM>(this));
		hWnd == nullptr) {
		ut::DBG_REPORT(L"CreateDialogParamW failed.");
	}
}

void CHexDlgGoTo::Initialize(IHexCtrl* pHexCtrl, HINSTANCE hInstRes)
{
	if (pHexCtrl == nullptr || hInstRes == nullptr) {
		ut::DBG_REPORT(L"Initialize == nullptr");
		return;
	}

	m_pHexCtrl = pHexCtrl;
	m_hInstRes = hInstRes;
}

auto CHexDlgGoTo::GetHWND()const->HWND
{
	return m_Wnd;
}

bool CHexDlgGoTo::IsRepeatAvail()const
{
	return m_fRepeat;
}

bool CHexDlgGoTo::PreTranslateMsg(MSG* pMsg)
{
	return m_Wnd.IsDlgMessage(pMsg);
}

auto CHexDlgGoTo::ProcessMsg(const MSG& msg)->INT_PTR
{
	switch (msg.message) {
	case WM_ACTIVATE: return OnActivate(msg);
	case WM_CLOSE: return OnClose();
	case WM_COMMAND: return OnCommand(msg);
	case WM_DESTROY: return OnDestroy();
	case WM_INITDIALOG: return OnInitDialog(msg);
	default:
		return 0;
	}
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

void CHexDlgGoTo::ShowWindow(int iCmdShow)
{
	if (!m_Wnd.IsWindow()) {
		CreateDlg();
	}

	m_Wnd.ShowWindow(iCmdShow);
}


//Private methods.

auto CHexDlgGoTo::GetHexCtrl()const->IHexCtrl*
{
	return m_pHexCtrl;
}

auto CHexDlgGoTo::GetGoMode()const->EGoMode
{
	return static_cast<EGoMode>(m_WndCmbMode.GetItemData(m_WndCmbMode.GetCurSel()));
}

void CHexDlgGoTo::GoTo(bool fForward)
{
	m_fRepeat = false;
	const auto pHexCtrl = GetHexCtrl();

	if (!pHexCtrl->IsCreated() || !pHexCtrl->IsDataSet()) {
		return;
	}

	wnd::CWnd wndEdit(m_Wnd.GetDlgItem(IDC_HEXCTRL_GOTO_EDIT_GOTO));
	if (wndEdit.IsWndTextEmpty()) {
		wndEdit.SetFocus();
		return;
	}

	const auto optGoTo = stn::StrToUInt64(wndEdit.GetWndText());
	if (!optGoTo) {
		wndEdit.SetFocus();
		MessageBoxW(m_Wnd, L"Invalid number", L"Error", MB_ICONERROR);
		return;
	}

	const auto ullGoTo = *optGoTo;
	const auto iFwdBack = fForward ? 1 : -1;
	const auto ullOffsetCurr = pHexCtrl->GetCaretPos();
	const auto ullDataSize = pHexCtrl->GetDataSize();
	const auto llPageSize = static_cast<LONGLONG>(pHexCtrl->GetPageSize()); //Cast to signed.
	const auto ullPageRem = llPageSize > 0 ? (ullDataSize % llPageSize) : 0; //Page remaining size. Zero division check.
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

auto CHexDlgGoTo::OnActivate(const MSG& msg)->INT_PTR
{
	const auto pHexCtrl = GetHexCtrl();
	if (!pHexCtrl->IsCreated() || !pHexCtrl->IsDataSet())
		return FALSE;

	const auto nState = LOWORD(msg.wParam);
	if (nState == WA_ACTIVE || nState == WA_CLICKACTIVE) {
		UpdateComboMode();
	}

	return FALSE; //Default handler.
}

void CHexDlgGoTo::OnCancel()
{
	if (IsNoEsc()) //Not closing Dialog on Escape key.
		return;

	ShowWindow(SW_HIDE);
}

auto CHexDlgGoTo::OnClose()->INT_PTR
{
	ShowWindow(SW_HIDE);
	return TRUE;
}

auto CHexDlgGoTo::OnCommand(const MSG& msg)->INT_PTR
{
	const auto uCtrlID = LOWORD(msg.wParam);
	switch (uCtrlID) {
	case IDOK:
		OnOK();
		break;
	case IDCANCEL:
		OnCancel();
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

auto CHexDlgGoTo::OnDestroy()->INT_PTR
{
	m_u64Flags = { };
	m_pHexCtrl = nullptr;
	m_fRepeat = false;

	return TRUE;
}

auto CHexDlgGoTo::OnInitDialog(const MSG& msg)->INT_PTR
{
	using enum EGoMode;
	m_Wnd.Attach(msg.hwnd);
	m_WndCmbMode.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_GOTO_COMBO_MODE));
	auto iIndex = m_WndCmbMode.AddString(L"Offset");
	m_WndCmbMode.SetItemData(iIndex, static_cast<DWORD_PTR>(MODE_OFFSET));
	m_WndCmbMode.SetCurSel(iIndex);
	iIndex = m_WndCmbMode.AddString(L"Offset forward from the current");
	m_WndCmbMode.SetItemData(iIndex, static_cast<DWORD_PTR>(MODE_OFFSETFWD));
	iIndex = m_WndCmbMode.AddString(L"Offset back from the current");
	m_WndCmbMode.SetItemData(iIndex, static_cast<DWORD_PTR>(MODE_OFFSETBACK));
	iIndex = m_WndCmbMode.AddString(L"Offset from the end");
	m_WndCmbMode.SetItemData(iIndex, static_cast<DWORD_PTR>(MODE_OFFSETEND));
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
	auto iCurrCount = m_WndCmbMode.GetCount();
	const auto fHasPages = iCurrCount > iOffsetsTotal;
	auto fShouldHavePages = GetHexCtrl()->GetPageSize() > 0;
	using enum EGoMode;

	if (fShouldHavePages != fHasPages) {
		m_WndCmbMode.SetRedraw(FALSE);
		if (fShouldHavePages) {
			auto iIndex = m_WndCmbMode.AddString(L"Page");
			m_WndCmbMode.SetItemData(iIndex, static_cast<DWORD_PTR>(MODE_PAGE));
			iIndex = m_WndCmbMode.AddString(L"Page forward from the current");
			m_WndCmbMode.SetItemData(iIndex, static_cast<DWORD_PTR>(MODE_PAGEFWD));
			iIndex = m_WndCmbMode.AddString(L"Page back from the current");
			m_WndCmbMode.SetItemData(iIndex, static_cast<DWORD_PTR>(MODE_PAGEBACK));
			iIndex = m_WndCmbMode.AddString(L"Page from the end");
			m_WndCmbMode.SetItemData(iIndex, static_cast<DWORD_PTR>(MODE_PAGEEND));
		}
		else {
			const auto iCurrSel = m_WndCmbMode.GetCurSel();
			for (auto iIndex = iCurrCount - 1; iIndex >= iOffsetsTotal; --iIndex) {
				m_WndCmbMode.DeleteString(iIndex);
			}

			if (iCurrSel > (iOffsetsTotal - 1)) {
				m_WndCmbMode.SetCurSel(iOffsetsTotal - 1);
				m_fRepeat = false;
			}
		}
		m_WndCmbMode.SetRedraw(TRUE);
		m_WndCmbMode.RedrawWindow();
	}
}