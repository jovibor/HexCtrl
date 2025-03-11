module;
/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include <SDKDDKVer.h>
#include "../../res/HexCtrlRes.h"
#include "../../HexCtrl.h"
#include <commctrl.h>
#include <unordered_map>
export module HEXCTRL:CHexDlgModify;

import :HexUtility;

using namespace HEXCTRL::INTERNAL;

namespace HEXCTRL::INTERNAL {
	class CHexDlgOpers final {
	public:
		void CreateDlg(HWND hWndParent, IHexCtrl* pHexCtrl, HINSTANCE hInstRes);
		[[nodiscard]] auto GetHWND()const->HWND;
		auto OnActivate(const MSG& msg) -> INT_PTR;
		[[nodiscard]] bool PreTranslateMsg(MSG* pMsg);
		[[nodiscard]] auto ProcessMsg(const MSG& msg) -> INT_PTR;
		void SetDlgProperties(std::uint64_t u64Flags);
		void ShowWindow(int iCmdShow);
	private:
		template<typename T> requires ut::TSize1248<T>
		[[nodiscard]] bool FillVecOper(bool fCheckBE);
		[[nodiscard]] auto GetOperMode()const->EHexOperMode;
		[[nodiscard]] auto GetDataType()const->EHexDataType;
		[[nodiscard]] bool IsNoEsc()const;
		void OnCancel();
		auto OnCommand(const MSG& msg) -> INT_PTR;
		void OnComboDataTypeSelChange();
		void OnComboOperSelChange();
		auto OnCtlClrStatic(const MSG& msg) -> INT_PTR;
		auto OnDestroy() -> INT_PTR;
		void OnEditOperChange();
		auto OnInitDialog(const MSG& msg) -> INT_PTR;
		void OnOK();
		void SetControlsState();
		void UpdateDescr();
	private:
		wnd::CWnd m_Wnd;                //Main window.
		wnd::CWnd m_WndStatDescr;       //Static control description.
		wnd::CWndEdit m_WndEditOperand; //Edit-box operand.
		wnd::CWndBtn m_WndBtnBE;        //Check-box bigendian.
		wnd::CWndBtn m_WndBtnOk;        //Ok.
		wnd::CWndCombo m_WndCmbOper;    //Operation combo-box.
		wnd::CWndCombo m_WndCmbType;    //Data size combo-box.
		std::vector<std::byte> m_vecOperData; //Operand data vector.
		IHexCtrl* m_pHexCtrl { };
		std::uint64_t m_u64Flags { };
		using enum EHexOperMode;
		inline static const std::unordered_map<EHexOperMode, const wchar_t*> m_umapNames {
			{ OPER_ASSIGN, L"Assign" }, { OPER_ADD, L"Add" }, { OPER_SUB, L"Subtract" },
			{ OPER_MUL, L"Multiply" }, { OPER_DIV, L"Divide" }, { OPER_MIN, L"Minimum" },
			{ OPER_MAX, L"Maximum" }, { OPER_OR, L"OR" }, { OPER_XOR, L"XOR" }, { OPER_AND, L"AND" },
			{ OPER_NOT, L"NOT" }, { OPER_SHL, L"SHL" }, { OPER_SHR, L"SHR" }, { OPER_ROTL, L"ROTL" },
			{ OPER_ROTR, L"ROTR" }, { OPER_SWAP, L"Swap Bytes" }, { OPER_BITREV, L"Reverse Bits" }
		};
	};
}

void CHexDlgOpers::CreateDlg(HWND hWndParent, IHexCtrl* pHexCtrl, HINSTANCE hInstRes)
{
	if (hWndParent == nullptr || pHexCtrl == nullptr) {
		ut::DBG_REPORT(L"hWndParent == nullptr || pHexCtrl == nullptr");
		return;
	}

	//m_Wnd is set in the OnInitDialog().
	if (const auto hWnd = ::CreateDialogParamW(hInstRes, MAKEINTRESOURCEW(IDD_HEXCTRL_OPERS),
		hWndParent, wnd::DlgProc<CHexDlgOpers>, reinterpret_cast<LPARAM>(this)); hWnd == nullptr) {
		ut::DBG_REPORT(L"CreateDialogParamW failed.");
	}

	m_pHexCtrl = pHexCtrl;
}

auto CHexDlgOpers::GetHWND()const->HWND
{
	return m_Wnd;
}

auto CHexDlgOpers::OnActivate(const MSG& msg)->INT_PTR
{
	if (m_pHexCtrl == nullptr || !m_pHexCtrl->IsCreated() || !m_pHexCtrl->IsDataSet())
		return FALSE;

	const auto nState = LOWORD(msg.wParam);
	if (nState == WA_ACTIVE || nState == WA_CLICKACTIVE) {
		const auto fSel { m_pHexCtrl->HasSelection() };
		m_Wnd.CheckRadioButton(IDC_HEXCTRL_OPERS_RAD_ALL, IDC_HEXCTRL_OPERS_RAD_SEL,
			fSel ? IDC_HEXCTRL_OPERS_RAD_SEL : IDC_HEXCTRL_OPERS_RAD_ALL);
		wnd::CWnd::FromHandle(m_Wnd.GetDlgItem(IDC_HEXCTRL_OPERS_RAD_SEL)).EnableWindow(fSel);
	}

	return FALSE; //Default handler.
}

bool CHexDlgOpers::PreTranslateMsg(MSG* pMsg)
{
	return m_Wnd.IsDlgMessage(pMsg);
}

auto CHexDlgOpers::ProcessMsg(const MSG& msg)->INT_PTR
{
	switch (msg.message) {
	case WM_ACTIVATE: return OnActivate(msg);
	case WM_COMMAND: return OnCommand(msg);
	case WM_CTLCOLORSTATIC: return OnCtlClrStatic(msg);
	case WM_DESTROY: return OnDestroy();
	case WM_INITDIALOG: return OnInitDialog(msg);
	default:
		return 0;
	}
}

void CHexDlgOpers::SetDlgProperties(std::uint64_t u64Flags)
{
	m_u64Flags = u64Flags;
}

void CHexDlgOpers::ShowWindow(int iCmdShow)
{
	m_Wnd.ShowWindow(iCmdShow);
}


//CHexDlgOpers private methods.

template<typename T> requires ut::TSize1248<T>
bool CHexDlgOpers::FillVecOper(bool fCheckBE)
{
	//To make sure the vector will have the size of the data type.
	//Even if operation doesn't need an operand (e.g. OPER_NOT), the spnData
	//in the HEXMODIFY must have appropriate size, equal to the data type size.
	m_vecOperData = ut::RangeToVecBytes(T { });
	if (!m_WndEditOperand.IsWindowEnabled())
		return true; //Some operations don't need operands.

	const auto opt = stn::StrToNum<T>(m_WndEditOperand.GetWndText());
	if (!opt) {
		::MessageBoxW(m_Wnd, L"Wrong operand.", L"Operand error", MB_ICONERROR);
		return false;
	}

	auto tOper = *opt;
	const auto eOperMode = GetOperMode();
	using enum EHexOperMode;
	if (eOperMode == OPER_DIV && tOper == 0) { //Division by zero check.
		::MessageBoxW(m_Wnd, L"Can't divide by zero.", L"Operand error", MB_ICONERROR);
		return { };
	}

	//Some operations don't need to swap the whole data in big-endian mode.
	//Instead, the operand itself can be swapped. Binary OR/XOR/AND are good examples.
	//Binary NOT and OPER_BITREV don't need to swap at all.
	if (fCheckBE && (eOperMode == OPER_OR || eOperMode == OPER_XOR
		|| eOperMode == OPER_AND || eOperMode == OPER_ASSIGN)) {
		tOper = ut::ByteSwap(tOper);
	}

	m_vecOperData = ut::RangeToVecBytes(tOper);

	return true;
}

auto CHexDlgOpers::GetOperMode()const->EHexOperMode
{
	return static_cast<EHexOperMode>(m_WndCmbOper.GetItemData(m_WndCmbOper.GetCurSel()));
}

auto CHexDlgOpers::GetDataType()const->EHexDataType
{
	return static_cast<EHexDataType>(m_WndCmbType.GetItemData(m_WndCmbType.GetCurSel()));
}

bool CHexDlgOpers::IsNoEsc()const
{
	return m_u64Flags & HEXCTRL_FLAG_DLG_NOESC;
}

void CHexDlgOpers::OnCancel()
{
	if (m_u64Flags & HEXCTRL_FLAG_DLG_NOESC)
		return;

	::ShowWindow(m_Wnd.GetParent(), SW_HIDE);
}

auto CHexDlgOpers::OnCommand(const MSG& msg)->INT_PTR
{
	const auto uCtrlID = LOWORD(msg.wParam); //Control ID or menu ID.
	const auto uCode = HIWORD(msg.wParam);   //Control code, zero for menu.

	switch (uCtrlID) {
	case IDOK: OnOK(); break;
	case IDCANCEL: OnCancel(); break;
	case IDC_HEXCTRL_OPERS_COMBO_OPER:
		if (uCode == CBN_SELCHANGE) { OnComboOperSelChange(); } break;
	case IDC_HEXCTRL_OPERS_COMBO_DTYPE:
		if (uCode == CBN_SELCHANGE) { OnComboDataTypeSelChange(); } break;
	case IDC_HEXCTRL_OPERS_EDIT_OPERAND:
		if (uCode == EN_CHANGE) { OnEditOperChange(); } break;
	default: return FALSE;
	}

	return TRUE;
}

void CHexDlgOpers::OnComboDataTypeSelChange()
{
	SetControlsState();
}

void CHexDlgOpers::OnComboOperSelChange()
{
	constexpr auto iIntegralTotal = 8; //Total amount of integral types.
	auto iCurrCount = m_WndCmbType.GetCount();
	const auto fHasFloats = iCurrCount > iIntegralTotal;
	auto fShouldHaveFloats { false };

	using enum EHexDataType;
	switch (GetOperMode()) { //Operations that can be performed on floats.
	case OPER_ASSIGN: case OPER_ADD: case OPER_SUB: case OPER_MUL:
	case OPER_DIV: case OPER_MAX: case OPER_MIN: case OPER_SWAP:
		fShouldHaveFloats = true;
	default:
		break;
	}

	if (fShouldHaveFloats != fHasFloats) {
		m_WndCmbType.SetRedraw(FALSE);
		if (fShouldHaveFloats) {
			auto iIndex = m_WndCmbType.AddString(L"Float");
			m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(DATA_FLOAT));
			iIndex = m_WndCmbType.AddString(L"Double");
			m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(DATA_DOUBLE));
		}
		else {
			const auto iCurrSel = m_WndCmbType.GetCurSel();

			for (auto iIndex = iCurrCount - 1; iIndex >= iIntegralTotal; --iIndex) {
				m_WndCmbType.DeleteString(iIndex);
			}

			//When deleting float types, if selection was on one of float types change it to integral.
			if (iCurrSel > (iIntegralTotal - 1)) {
				m_WndCmbType.SetCurSel(iIntegralTotal - 1);
			}
		}
		m_WndCmbType.SetRedraw(TRUE);
		m_WndCmbType.RedrawWindow();
	}

	SetControlsState();
}

auto CHexDlgOpers::OnCtlClrStatic(const MSG& msg)->INT_PTR
{
	if (const auto hWndFrom = reinterpret_cast<HWND>(msg.lParam); hWndFrom == m_WndStatDescr) {
		const auto hDC = reinterpret_cast<HDC>(msg.wParam);
		::SetTextColor(hDC, RGB(0, 50, 250));
		::SetBkColor(hDC, ::GetSysColor(COLOR_3DFACE));
		return reinterpret_cast<INT_PTR>(::GetSysColorBrush(COLOR_3DFACE));
	}

	return FALSE; //Default handler.
}

auto CHexDlgOpers::OnDestroy()->INT_PTR
{
	m_u64Flags = { };
	m_pHexCtrl = nullptr;
	m_vecOperData.clear();

	return TRUE;
}

void CHexDlgOpers::OnEditOperChange()
{
	SetControlsState();
}

auto CHexDlgOpers::OnInitDialog(const MSG& msg)->INT_PTR
{
	m_Wnd.Attach(msg.hwnd);
	m_WndBtnBE.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_OPERS_CHK_BE));
	m_WndBtnOk.Attach(m_Wnd.GetDlgItem(IDOK));
	m_WndCmbOper.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_OPERS_COMBO_OPER));
	m_WndCmbType.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_OPERS_COMBO_DTYPE));
	m_WndEditOperand.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_OPERS_EDIT_OPERAND));
	m_WndStatDescr.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_OPERS_STAT_DESCR));

	using enum EHexOperMode;
	auto iIndex = m_WndCmbOper.AddString(m_umapNames.at(OPER_ASSIGN));
	m_WndCmbOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_ASSIGN));
	m_WndCmbOper.SetCurSel(iIndex);
	iIndex = m_WndCmbOper.AddString(m_umapNames.at(OPER_ADD));
	m_WndCmbOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_ADD));
	iIndex = m_WndCmbOper.AddString(m_umapNames.at(OPER_SUB));
	m_WndCmbOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_SUB));
	iIndex = m_WndCmbOper.AddString(m_umapNames.at(OPER_MUL));
	m_WndCmbOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_MUL));
	iIndex = m_WndCmbOper.AddString(m_umapNames.at(OPER_DIV));
	m_WndCmbOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_DIV));
	iIndex = m_WndCmbOper.AddString(m_umapNames.at(OPER_MIN));
	m_WndCmbOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_MIN));
	iIndex = m_WndCmbOper.AddString(m_umapNames.at(OPER_MAX));
	m_WndCmbOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_MAX));
	iIndex = m_WndCmbOper.AddString(m_umapNames.at(OPER_OR));
	m_WndCmbOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_OR));
	iIndex = m_WndCmbOper.AddString(m_umapNames.at(OPER_XOR));
	m_WndCmbOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_XOR));
	iIndex = m_WndCmbOper.AddString(m_umapNames.at(OPER_AND));
	m_WndCmbOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_AND));
	iIndex = m_WndCmbOper.AddString(m_umapNames.at(OPER_NOT));
	m_WndCmbOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_NOT));
	iIndex = m_WndCmbOper.AddString(m_umapNames.at(OPER_SHL));
	m_WndCmbOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_SHL));
	iIndex = m_WndCmbOper.AddString(m_umapNames.at(OPER_SHR));
	m_WndCmbOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_SHR));
	iIndex = m_WndCmbOper.AddString(m_umapNames.at(OPER_ROTL));
	m_WndCmbOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_ROTL));
	iIndex = m_WndCmbOper.AddString(m_umapNames.at(OPER_ROTR));
	m_WndCmbOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_ROTR));
	iIndex = m_WndCmbOper.AddString(m_umapNames.at(OPER_SWAP));
	m_WndCmbOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_SWAP));
	iIndex = m_WndCmbOper.AddString(m_umapNames.at(OPER_BITREV));
	m_WndCmbOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_BITREV));

	using enum EHexDataType;
	iIndex = m_WndCmbType.AddString(L"Int8");
	m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(DATA_INT8));
	m_WndCmbType.SetCurSel(iIndex);
	iIndex = m_WndCmbType.AddString(L"Unsigned Int8");
	m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(DATA_UINT8));
	iIndex = m_WndCmbType.AddString(L"Int16");
	m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(DATA_INT16));
	iIndex = m_WndCmbType.AddString(L"Unsigned Int16");
	m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(DATA_UINT16));
	iIndex = m_WndCmbType.AddString(L"Int32");
	m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(DATA_INT32));
	iIndex = m_WndCmbType.AddString(L"Unsigned Int32");
	m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(DATA_UINT32));
	iIndex = m_WndCmbType.AddString(L"Int64");
	m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(DATA_INT64));
	iIndex = m_WndCmbType.AddString(L"Unsigned Int64");
	m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(DATA_UINT64));

	m_Wnd.CheckRadioButton(IDC_HEXCTRL_OPERS_RAD_ALL, IDC_HEXCTRL_OPERS_RAD_SEL, IDC_HEXCTRL_OPERS_RAD_ALL);
	OnComboOperSelChange();

	return TRUE;
}

void CHexDlgOpers::OnOK()
{
	if (!m_WndBtnOk.IsWindowEnabled() || !m_pHexCtrl->IsCreated() || !m_pHexCtrl->IsDataSet())
		return;

	using enum EHexOperMode; using enum EHexDataType; using enum EHexModifyMode;
	const auto eOperMode = GetOperMode();
	const auto eDataType = GetDataType();
	const auto fDataOneByte = eDataType == DATA_INT8 || eDataType == DATA_UINT8;
	const auto fCheckBE = m_WndBtnBE.IsChecked();
	//Should data be treated as Big-endian or there is no need in that.
	const auto fSwapData = fCheckBE && !fDataOneByte && eOperMode != OPER_NOT && eOperMode != OPER_SWAP
		&& eOperMode != OPER_BITREV && eOperMode != OPER_OR && eOperMode != OPER_XOR && eOperMode != OPER_AND
		&& eOperMode != OPER_ASSIGN;

	bool fFillOk { false };
	switch (eDataType) {
	case DATA_INT8:
		fFillOk = FillVecOper<std::int8_t>(fCheckBE);
		break;
	case DATA_UINT8:
		fFillOk = FillVecOper<std::uint8_t>(fCheckBE);
		break;
	case DATA_INT16:
		fFillOk = FillVecOper<std::int16_t>(fCheckBE);
		break;
	case DATA_UINT16:
		fFillOk = FillVecOper<std::uint16_t>(fCheckBE);
		break;
	case DATA_INT32:
		fFillOk = FillVecOper<std::int32_t>(fCheckBE);
		break;
	case DATA_UINT32:
		fFillOk = FillVecOper<std::uint32_t>(fCheckBE);
		break;
	case DATA_INT64:
		fFillOk = FillVecOper<std::int64_t>(fCheckBE);
		break;
	case DATA_UINT64:
		fFillOk = FillVecOper<std::uint64_t>(fCheckBE);
		break;
	case DATA_FLOAT:
		fFillOk = FillVecOper<float>(fCheckBE);
		break;
	case DATA_DOUBLE:
		fFillOk = FillVecOper<double>(fCheckBE);
		break;
	default:
		return;
	}
	if (!fFillOk)
		return;

	VecSpan vecSpan;
	const auto iRadioAllOrSel = m_Wnd.GetCheckedRadioButton(IDC_HEXCTRL_OPERS_RAD_ALL, IDC_HEXCTRL_OPERS_RAD_SEL);
	if (iRadioAllOrSel == IDC_HEXCTRL_OPERS_RAD_ALL) {
		if (::MessageBoxW(m_Wnd, L"You are about to modify the entire data region.\r\nAre you sure?",
			L"Modify all data?", MB_YESNO | MB_ICONWARNING) == IDNO)
			return;

		vecSpan.emplace_back(0, m_pHexCtrl->GetDataSize());
	}
	else {
		if (!m_pHexCtrl->HasSelection())
			return;

		vecSpan = m_pHexCtrl->GetSelection();
	}

	const HEXMODIFY hms { .eModifyMode { MODIFY_OPERATION }, .eOperMode { eOperMode }, .eDataType { eDataType },
		.spnData { m_vecOperData }, .vecSpan { std::move(vecSpan) }, .fBigEndian { fSwapData } };
	m_pHexCtrl->ModifyData(hms);
	m_pHexCtrl->Redraw();
}

void CHexDlgOpers::SetControlsState()
{
	using enum EHexOperMode;
	using enum EHexDataType;
	const auto eDataType = GetDataType();
	const auto fDataOneByte = eDataType == DATA_INT8 || eDataType == DATA_UINT8;
	auto fEditOperand = true;
	auto fBtnEnter = !m_WndEditOperand.IsWndTextEmpty();

	switch (GetOperMode()) {
	case OPER_NOT:
	case OPER_BITREV:
		fEditOperand = false;
		fBtnEnter = true;
		break;
	case OPER_SWAP:
		fEditOperand = false;
		fBtnEnter = !fDataOneByte;
		break;
	default:
		break;
	};

	m_WndEditOperand.EnableWindow(fEditOperand);
	m_WndBtnBE.EnableWindow(fDataOneByte ? false : fEditOperand);
	m_WndBtnOk.EnableWindow(fBtnEnter);
	m_WndBtnOk.SetWndText(m_umapNames.at(GetOperMode()));
	UpdateDescr();
}

void CHexDlgOpers::UpdateDescr()
{
	using enum EHexOperMode;
	using enum EHexDataType;
	std::wstring_view wsvDescr;
	switch (GetOperMode()) {
	case OPER_ASSIGN:
		wsvDescr = L"Data = Operand";
		break;
	case OPER_ADD:
		wsvDescr = L"Data += Operand";
		break;
	case OPER_SUB:
		wsvDescr = L"Data -= Operand";
		break;
	case OPER_MUL:
		wsvDescr = L"Data *= Operand";
		break;
	case OPER_DIV:
		wsvDescr = L"Data /= Operand";
		break;
	case OPER_MIN:
		wsvDescr = L"Data = max(Data, Operand)";
		break;
	case OPER_MAX:
		wsvDescr = L"Data = min(Data, Operand)";
		break;
	case OPER_OR:
		wsvDescr = L"Data |= Operand";
		break;
	case OPER_XOR:
		wsvDescr = L"Data ^= Operand";
		break;
	case OPER_AND:
		wsvDescr = L"Data &&= Operand";
		break;
	case OPER_NOT:
		wsvDescr = L"Data = ~Data";
		break;
	case OPER_SHL:
		wsvDescr = L"Data <<= Operand\r\n(Logical Left Shift)";
		break;
	case OPER_SHR:
		switch (GetDataType()) {
		case DATA_INT8:
		case DATA_INT16:
		case DATA_INT32:
		case DATA_INT64:
			wsvDescr = L"Data >>= Operand\r\n(Arithmetical Right Shift)";
			break;
		case DATA_UINT8:
		case DATA_UINT16:
		case DATA_UINT32:
		case DATA_UINT64:
			wsvDescr = L"Data >>= Operand\r\n(Logical Right Shift)";
			break;
		default:
			break;
		};
		break;
	case OPER_ROTL:
		wsvDescr = L"Data = std::rotl(Data, Operand)";
		break;
	case OPER_ROTR:
		wsvDescr = L"Data = std::rotr(Data, Operand)";
		break;
	case OPER_SWAP:
		switch (GetDataType()) {
		case DATA_INT8:
		case DATA_UINT8:
			break;
		default:
			wsvDescr = L"Data = SwapBytes(Data)\r\n(Change Endianness)";
			break;
		};
		break;
	case OPER_BITREV:
		wsvDescr = L"11000001 -> 10000011";
		break;
	default:
		break;
	};

	m_WndStatDescr.SetWndText(wsvDescr.data());
}


namespace HEXCTRL::INTERNAL {
	class CHexDlgFillData final {
	public:
		void CreateDlg(HWND hWndParent, IHexCtrl* pHexCtrl, HINSTANCE hInstRes);
		[[nodiscard]] auto GetDlgItemHandle(EHexDlgItem eItem)const->HWND;
		[[nodiscard]] auto GetHWND()const->HWND;
		auto OnActivate(const MSG& msg) -> INT_PTR;
		[[nodiscard]] bool PreTranslateMsg(MSG* pMsg);
		[[nodiscard]] auto ProcessMsg(const MSG& msg) -> INT_PTR;
		void SetDlgProperties(std::uint64_t u64Flags);
		void ShowWindow(int iCmdShow);
	private:
		enum class EFillType : std::uint8_t; //Forward declaration.
		[[nodiscard]] auto GetFillType()const->EFillType;
		[[nodiscard]] bool IsNoEsc()const;
		void OnCancel();
		void OnComboDataEditChange();
		void OnComboTypeSelChange();
		auto OnCommand(const MSG& msg) -> INT_PTR;
		auto OnDestroy() -> INT_PTR;
		auto OnInitDialog(const MSG& msg) -> INT_PTR;
		void OnOK();
		void SetControlsState();
	private:
		wnd::CWnd m_Wnd;             //Main window.
		wnd::CWndBtn m_WndBtnOk;     //Ok.
		wnd::CWndCombo m_WndCmbType; //Fill type combo-box.
		wnd::CWndCombo m_WndCmbData; //Data combo-box.
		std::vector<std::byte> m_vecFillData; //Fill data vector.
		IHexCtrl* m_pHexCtrl { };
		std::uint64_t m_u64Flags { };
	};
}

enum class CHexDlgFillData::EFillType : std::uint8_t {
	FILL_HEX, FILL_ASCII, FILL_WCHAR, FILL_RAND_MT19937, FILL_RAND_FAST
};

void CHexDlgFillData::CreateDlg(HWND hWndParent, IHexCtrl* pHexCtrl, HINSTANCE hInstRes)
{
	if (hWndParent == nullptr || pHexCtrl == nullptr) {
		ut::DBG_REPORT(L"hWndParent == nullptr || pHexCtrl == nullptr");
		return;
	}

	//m_Wnd is set in the OnInitDialog().
	if (const auto hWnd = ::CreateDialogParamW(hInstRes, MAKEINTRESOURCEW(IDD_HEXCTRL_FILLDATA),
		hWndParent, wnd::DlgProc<CHexDlgFillData>, reinterpret_cast<LPARAM>(this)); hWnd == nullptr) {
		ut::DBG_REPORT(L"CreateDialogParamW failed.");
	}

	m_pHexCtrl = pHexCtrl;
}

auto CHexDlgFillData::GetDlgItemHandle(EHexDlgItem eItem)const->HWND
{
	using enum EHexDlgItem;
	switch (eItem) {
	case FILLDATA_COMBO_DATA:
		return m_WndCmbData;
	default:
		return { };
	}
}

auto CHexDlgFillData::GetHWND()const->HWND
{
	return m_Wnd;
}

auto CHexDlgFillData::OnActivate(const MSG& msg)->INT_PTR
{
	if (m_pHexCtrl == nullptr || !m_pHexCtrl->IsCreated() || !m_pHexCtrl->IsDataSet())
		return FALSE;

	const auto nState = LOWORD(msg.wParam);
	if (nState == WA_ACTIVE || nState == WA_CLICKACTIVE) {
		const auto fSelection { m_pHexCtrl->HasSelection() };
		m_Wnd.CheckRadioButton(IDC_HEXCTRL_FILLDATA_RAD_ALL, IDC_HEXCTRL_FILLDATA_RAD_SEL,
			fSelection ? IDC_HEXCTRL_FILLDATA_RAD_SEL : IDC_HEXCTRL_FILLDATA_RAD_ALL);
		wnd::CWnd::FromHandle(m_Wnd.GetDlgItem(IDC_HEXCTRL_FILLDATA_RAD_SEL)).EnableWindow(fSelection);
	}

	return FALSE; //Default handler.
}

bool CHexDlgFillData::PreTranslateMsg(MSG* pMsg)
{
	return m_Wnd.IsDlgMessage(pMsg);
}

auto CHexDlgFillData::ProcessMsg(const MSG& msg)->INT_PTR
{
	switch (msg.message) {
	case WM_ACTIVATE: return OnActivate(msg);
	case WM_COMMAND: return OnCommand(msg);
	case WM_DESTROY: return OnDestroy();
	case WM_INITDIALOG: return OnInitDialog(msg);
	default:
		return 0;
	}
}

void CHexDlgFillData::SetDlgProperties(std::uint64_t u64Flags)
{
	m_u64Flags = u64Flags;
}

void CHexDlgFillData::ShowWindow(int iCmdShow)
{
	m_Wnd.ShowWindow(iCmdShow);
}


//CHexDlgFillData private methods.

auto CHexDlgFillData::GetFillType()const->CHexDlgFillData::EFillType
{
	return static_cast<EFillType>(m_WndCmbType.GetItemData(m_WndCmbType.GetCurSel()));
}

bool CHexDlgFillData::IsNoEsc()const
{
	return m_u64Flags & HEXCTRL_FLAG_DLG_NOESC;
}

void CHexDlgFillData::OnCancel()
{
	if (IsNoEsc())
		return;

	::ShowWindow(m_Wnd.GetParent(), SW_HIDE);
}

void CHexDlgFillData::OnComboDataEditChange()
{
	SetControlsState();
}

void CHexDlgFillData::OnComboTypeSelChange()
{
	SetControlsState();
}

auto CHexDlgFillData::OnCommand(const MSG& msg)->INT_PTR
{
	const auto uCtrlID = LOWORD(msg.wParam); //Control ID or menu ID.
	const auto uCode = HIWORD(msg.wParam);   //Control code, zero for menu.

	switch (uCtrlID) {
	case IDOK: OnOK(); break;
	case IDCANCEL: OnCancel(); break;
	case IDC_HEXCTRL_FILLDATA_COMBO_TYPE:
		if (uCode == CBN_SELCHANGE) { OnComboTypeSelChange(); } break;
	case IDC_HEXCTRL_FILLDATA_COMBO_DATA:
		if (uCode == CBN_EDITCHANGE) { OnComboDataEditChange(); } break;
	default: return FALSE;
	}

	return TRUE;
}

auto CHexDlgFillData::OnDestroy()->INT_PTR
{
	m_u64Flags = { };
	m_pHexCtrl = nullptr;
	m_vecFillData.clear();

	return TRUE;
}

auto CHexDlgFillData::OnInitDialog(const MSG& msg)->INT_PTR
{
	m_Wnd.Attach(msg.hwnd);
	m_WndBtnOk.Attach(m_Wnd.GetDlgItem(IDOK));
	m_WndCmbType.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_FILLDATA_COMBO_TYPE));
	m_WndCmbData.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_FILLDATA_COMBO_DATA));

	auto iIndex = m_WndCmbType.AddString(L"Hex Bytes");
	m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(EFillType::FILL_HEX));
	m_WndCmbType.SetCurSel(iIndex);
	iIndex = m_WndCmbType.AddString(L"Text ASCII");
	m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(EFillType::FILL_ASCII));
	iIndex = m_WndCmbType.AddString(L"Text UTF-16");
	m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(EFillType::FILL_WCHAR));
	iIndex = m_WndCmbType.AddString(L"Random Data (MT19937)");
	m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(EFillType::FILL_RAND_MT19937));
	iIndex = m_WndCmbType.AddString(L"Pseudo Random Data (fast, but less secure)");
	m_WndCmbType.SetItemData(iIndex, static_cast<DWORD_PTR>(EFillType::FILL_RAND_FAST));

	m_Wnd.CheckRadioButton(IDC_HEXCTRL_FILLDATA_RAD_ALL, IDC_HEXCTRL_FILLDATA_RAD_SEL, IDC_HEXCTRL_FILLDATA_RAD_ALL);
	m_WndCmbData.LimitText(256); //Max characters of the combo-box.
	SetControlsState();

	return TRUE;
}

void CHexDlgFillData::OnOK()
{
	if (!m_WndBtnOk.IsWindowEnabled() || !m_pHexCtrl->IsCreated() || !m_pHexCtrl->IsDataSet())
		return;

	using enum EFillType;
	const auto eType = GetFillType();
	if (eType == FILL_HEX || eType == FILL_ASCII || eType == FILL_WCHAR) {
		if (m_WndCmbData.IsWndTextEmpty()) {
			::MessageBoxW(m_Wnd, L"Missing fill data.", L"Data error", MB_ICONERROR);
			return;
		}
	}

	VecSpan vecSpan;
	const auto iRadioAllOrSel = m_Wnd.GetCheckedRadioButton(IDC_HEXCTRL_FILLDATA_RAD_ALL, IDC_HEXCTRL_FILLDATA_RAD_SEL);
	if (iRadioAllOrSel == IDC_HEXCTRL_FILLDATA_RAD_ALL) {
		if (::MessageBoxW(m_Wnd, L"You are about to modify the entire data region.\r\nAre you sure?", L"Modify all data?",
			MB_YESNO | MB_ICONWARNING) == IDNO)
			return;

		vecSpan.emplace_back(0, m_pHexCtrl->GetDataSize());
	}
	else {
		if (!m_pHexCtrl->HasSelection())
			return;

		vecSpan = m_pHexCtrl->GetSelection();
	}

	using enum EHexModifyMode;
	EHexModifyMode eModifyMode;
	const auto wstrText = m_WndCmbData.GetWndText();
	switch (eType) {
	case FILL_HEX:
	{
		auto optData = ut::NumStrToHex(wstrText);
		if (!optData) {
			::MessageBoxW(m_Wnd, L"Wrong Hex format.", L"Format error", MB_ICONERROR);
			return;
		}

		m_vecFillData = ut::RangeToVecBytes(*optData);
		eModifyMode = MODIFY_REPEAT;
	}
	break;
	case FILL_ASCII:
		m_vecFillData = ut::RangeToVecBytes(ut::WstrToStr(wstrText));
		eModifyMode = MODIFY_REPEAT;
		break;
	case FILL_WCHAR:
		eModifyMode = MODIFY_REPEAT;
		m_vecFillData = ut::RangeToVecBytes(wstrText);
		break;
	case FILL_RAND_MT19937:
		eModifyMode = MODIFY_RAND_MT19937;
		break;
	case FILL_RAND_FAST:
		eModifyMode = MODIFY_RAND_FAST;
		break;
	default:
		return;
	}

	//Insert wstring into ComboBox, only if it's not already there.
	if (!m_WndCmbData.HasString(wstrText)) {
		if (m_WndCmbData.GetCount() == 50) { //Keep max 50 strings in list.
			m_WndCmbData.DeleteString(49); //Delete last string.
		}
		m_WndCmbData.InsertString(0, wstrText);
	}

	if (m_vecFillData.size() > vecSpan.back().ullSize) {
		::MessageBoxW(m_Wnd, L"Fill data size is bigger than the region selected for modification, please select a larger region.",
			L"Data region size error", MB_ICONERROR);
		return;
	}

	const HEXMODIFY hms { .eModifyMode { eModifyMode }, .spnData { m_vecFillData }, .vecSpan { std::move(vecSpan) } };
	m_pHexCtrl->ModifyData(hms);
	m_pHexCtrl->Redraw();
}

void CHexDlgFillData::SetControlsState()
{
	using enum EFillType;
	const auto eFillType = GetFillType();
	const auto fRND = eFillType == FILL_RAND_MT19937 || eFillType == FILL_RAND_FAST;
	const auto fIsData = !m_WndCmbData.IsWndTextEmpty();
	m_WndCmbData.EnableWindow(!fRND);
	m_WndBtnOk.EnableWindow(fRND ? true : fIsData);
}


namespace HEXCTRL::INTERNAL {
	class CHexDlgModify final {
	public:
		void CreateDlg();
		void DestroyDlg();
		[[nodiscard]] auto GetDlgItemHandle(EHexDlgItem eItem)const->HWND;
		[[nodiscard]] auto GetHWND()const->HWND;
		void Initialize(IHexCtrl* pHexCtrl, HINSTANCE hInstRes);
		[[nodiscard]] bool PreTranslateMsg(MSG* pMsg);
		[[nodiscard]] auto ProcessMsg(const MSG& msg) -> INT_PTR;
		void SetDlgProperties(std::uint64_t u64Flags);
		void ShowWindow(int iCmdShow, int iTab = -1);
	private:
		auto OnActivate(const MSG& msg) -> INT_PTR;
		auto OnClose() -> INT_PTR;
		auto OnDestroy() -> INT_PTR;
		auto OnInitDialog(const MSG& msg) -> INT_PTR;
		auto OnNotify(const MSG& msg) -> INT_PTR;
		void OnNotifyTabSelChanged(NMHDR* pNMHDR);
		void SetCurrentTab(int iTab);
	private:
		HINSTANCE m_hInstRes { };
		wnd::CWnd m_Wnd;              //Main window.
		wnd::CWndTab m_WndTab;        //Tab control.
		CHexDlgOpers m_dlgOpers;       //"Operations" tab dialog.
		CHexDlgFillData m_dlgFillData; //"Fill with" tab dialog.
		IHexCtrl* m_pHexCtrl { };
		std::uint64_t m_u64Flags { }; //Data from SetDlgProperties.
	};
}


//CHexDlgModify methods.

void CHexDlgModify::CreateDlg()
{
	//m_Wnd is set in the OnInitDialog().
	if (const auto hWnd = ::CreateDialogParamW(m_hInstRes, MAKEINTRESOURCEW(IDD_HEXCTRL_MODIFY),
		m_pHexCtrl->GetWndHandle(EHexWnd::WND_MAIN), wnd::DlgProc<CHexDlgModify>, reinterpret_cast<LPARAM>(this));
		hWnd == nullptr) {
		ut::DBG_REPORT(L"CreateDialogParamW failed.");
	}
}

void CHexDlgModify::DestroyDlg()
{
	if (m_Wnd.IsWindow()) {
		m_Wnd.DestroyWindow();
	}
}

auto CHexDlgModify::GetDlgItemHandle(EHexDlgItem eItem)const->HWND
{
	if (!m_Wnd.IsWindow()) {
		return { };
	}

	using enum EHexDlgItem;
	switch (eItem) {
	case FILLDATA_COMBO_DATA:
		return m_dlgFillData.GetDlgItemHandle(eItem);
	default:
		return { };
	}
}

auto CHexDlgModify::GetHWND()const->HWND
{
	return m_Wnd;
}

void CHexDlgModify::Initialize(IHexCtrl* pHexCtrl, HINSTANCE hInstRes)
{
	if (pHexCtrl == nullptr || hInstRes == nullptr) {
		ut::DBG_REPORT(L"Initialize == nullptr");
		return;
	}

	m_pHexCtrl = pHexCtrl;
	m_hInstRes = hInstRes;
}

bool CHexDlgModify::PreTranslateMsg(MSG* pMsg)
{
	return m_dlgOpers.PreTranslateMsg(pMsg) || m_dlgFillData.PreTranslateMsg(pMsg);
}

auto CHexDlgModify::ProcessMsg(const MSG& msg)->INT_PTR
{
	switch (msg.message) {
	case WM_ACTIVATE: return OnActivate(msg);
	case WM_CLOSE: return OnClose();
	case WM_DESTROY: return OnDestroy();
	case WM_INITDIALOG: return OnInitDialog(msg);
	case WM_NOTIFY: return OnNotify(msg);
	default:
		return 0;
	}
}

void CHexDlgModify::SetDlgProperties(std::uint64_t u64Flags)
{
	m_u64Flags = u64Flags;
	m_dlgOpers.SetDlgProperties(u64Flags);
	m_dlgFillData.SetDlgProperties(u64Flags);
}

void CHexDlgModify::ShowWindow(int iCmdShow, int iTab)
{
	if (!m_Wnd.IsWindow()) {
		CreateDlg();
	}

	SetCurrentTab(iTab);

	m_Wnd.ShowWindow(iCmdShow);
}


//CHexDlgModify private methods.

auto CHexDlgModify::OnActivate(const MSG& msg)->INT_PTR
{
	m_dlgOpers.OnActivate(msg);
	m_dlgFillData.OnActivate(msg);

	return FALSE; //Default handler.
}

auto CHexDlgModify::OnClose()->INT_PTR
{
	ShowWindow(SW_HIDE);
	return TRUE;
}

auto CHexDlgModify::OnDestroy()->INT_PTR
{
	m_u64Flags = { };
	m_pHexCtrl = nullptr;

	return TRUE;
}

auto CHexDlgModify::OnInitDialog(const MSG& msg)->INT_PTR
{
	m_Wnd.Attach(msg.hwnd);
	m_WndTab.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_MODIFY_TAB));

	m_WndTab.InsertItem(0, L"Operations");
	m_WndTab.InsertItem(1, L"Fill with Data");
	const auto rcTab = m_WndTab.GetItemRect(0);

	m_dlgOpers.CreateDlg(m_Wnd, m_pHexCtrl, m_hInstRes);
	::SetWindowPos(m_dlgOpers.GetHWND(), nullptr, rcTab.left, rcTab.bottom + 1, 0, 0,
		SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
	m_dlgOpers.OnActivate({ .wParam { WA_ACTIVE } }); //To properly activate "All-Selection" radios on the first launch.
	m_dlgFillData.CreateDlg(m_Wnd, m_pHexCtrl, m_hInstRes);
	::SetWindowPos(m_dlgFillData.GetHWND(), nullptr, rcTab.left, rcTab.bottom + 1, 0, 0,
		SWP_NOSIZE | SWP_NOZORDER | SWP_HIDEWINDOW);
	m_dlgFillData.OnActivate({ .wParam { WA_ACTIVE } }); //To properly activate "All-Selection" radios on the first launch.

	return TRUE;
}

auto CHexDlgModify::OnNotify(const MSG& msg)->INT_PTR
{
	const auto pNMHDR = reinterpret_cast<NMHDR*>(msg.lParam);
	switch (pNMHDR->idFrom) {
	case IDC_HEXCTRL_MODIFY_TAB: if (pNMHDR->code == TCN_SELCHANGE) { OnNotifyTabSelChanged(pNMHDR); } break;
	default: break;
	}

	return TRUE;
}

void CHexDlgModify::OnNotifyTabSelChanged(NMHDR* /*pNMHDR*/)
{
	SetCurrentTab(m_WndTab.GetCurSel());
}

void CHexDlgModify::SetCurrentTab(int iTab)
{
	if (iTab < 0) {
		return;
	}

	m_WndTab.SetCurSel(iTab);
	HWND hWndFocus { };
	switch (iTab) {
	case 0:
		m_dlgFillData.ShowWindow(SW_HIDE);
		m_dlgOpers.ShowWindow(SW_SHOW);
		hWndFocus = m_dlgOpers.GetHWND();
		break;
	case 1:
		m_dlgOpers.ShowWindow(SW_HIDE);
		m_dlgFillData.ShowWindow(SW_SHOW);
		hWndFocus = m_dlgFillData.GetHWND();
		break;
	default:
		break;
	}

	::SetFocus(hWndFocus);
}