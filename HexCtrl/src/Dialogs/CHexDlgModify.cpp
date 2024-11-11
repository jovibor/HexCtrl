/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "../../res/HexCtrlRes.h"
#include "CHexDlgModify.h"
#include <cassert>
#include <unordered_map>

import HEXCTRL.HexUtility;

using namespace HEXCTRL::INTERNAL;

namespace HEXCTRL::INTERNAL {
	//CHexDlgOpers.
	class CHexDlgOpers final : public CDialogEx {
	public:
		void Create(CWnd* pParent, IHexCtrl* pHexCtrl);
		void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		void SetDlgProperties(std::uint64_t u64Flags);
	private:
		void DoDataExchange(CDataExchange* pDX)override;
		template<typename T> requires TSize1248<T>
		[[nodiscard]] bool FillVecOper(bool fCheckBE);
		[[nodiscard]] auto GetOperMode()const->EHexOperMode;
		[[nodiscard]] auto GetDataType()const->EHexDataType;
		void OnCancel()override;
		afx_msg void OnComboDataTypeSelChange();
		afx_msg void OnComboOperSelChange();
		afx_msg auto OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) -> HBRUSH;
		afx_msg void OnDestroy();
		afx_msg void OnEditOperChange();
		BOOL OnInitDialog()override;
		void OnOK()override;
		void SetControlsState();
		void UpdateDescr();
		DECLARE_MESSAGE_MAP();
	private:
		IHexCtrl* m_pHexCtrl { };
		CComboBox m_comboOper; //Operation combo-box.
		CComboBox m_comboType; //Data size combo-box.
		std::vector<std::byte> m_vecOperData; //Operand data vector.
		std::uint64_t m_u64Flags { };
		using enum EHexOperMode;
		inline static const std::unordered_map<EHexOperMode, std::wstring_view> m_umapNames {
			{ OPER_ASSIGN, L"Assign" }, { OPER_ADD, L"Add" }, { OPER_SUB, L"Subtract" },
			{ OPER_MUL, L"Multiply" }, { OPER_DIV, L"Divide" }, { OPER_MIN, L"Minimum" },
			{ OPER_MAX, L"Maximum" }, { OPER_OR, L"OR" }, { OPER_XOR, L"XOR" }, { OPER_AND, L"AND" },
			{ OPER_NOT, L"NOT" }, { OPER_SHL, L"SHL" }, { OPER_SHR, L"SHR" }, { OPER_ROTL, L"ROTL" },
			{ OPER_ROTR, L"ROTR" }, { OPER_SWAP, L"Swap Bytes" }, { OPER_BITREV, L"Reverse Bits" }
		};
	};
}

BEGIN_MESSAGE_MAP(CHexDlgOpers, CDialogEx)
	ON_CBN_SELCHANGE(IDC_HEXCTRL_OPERS_COMBO_OPER, &CHexDlgOpers::OnComboOperSelChange)
	ON_CBN_SELCHANGE(IDC_HEXCTRL_OPERS_COMBO_DTYPE, &CHexDlgOpers::OnComboDataTypeSelChange)
	ON_EN_CHANGE(IDC_HEXCTRL_OPERS_EDIT_OPERAND, &CHexDlgOpers::OnEditOperChange)
	ON_WM_ACTIVATE()
	ON_WM_CTLCOLOR()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CHexDlgOpers::Create(CWnd* pParent, IHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	assert(pParent);
	if (pHexCtrl == nullptr)
		return;

	m_pHexCtrl = pHexCtrl;
	CDialogEx::Create(IDD_HEXCTRL_OPERS, pParent);
}

void CHexDlgOpers::OnActivate(UINT nState, CWnd* /*pWndOther*/, BOOL /*bMinimized*/)
{
	if (m_pHexCtrl == nullptr || !m_pHexCtrl->IsCreated() || !m_pHexCtrl->IsDataSet())
		return;

	if (nState == WA_ACTIVE || nState == WA_CLICKACTIVE) {
		const auto fSelection { m_pHexCtrl->HasSelection() };
		CheckRadioButton(IDC_HEXCTRL_OPERS_RAD_ALL, IDC_HEXCTRL_OPERS_RAD_SEL,
			fSelection ? IDC_HEXCTRL_OPERS_RAD_SEL : IDC_HEXCTRL_OPERS_RAD_ALL);
		GetDlgItem(IDC_HEXCTRL_OPERS_RAD_SEL)->EnableWindow(fSelection);
	}
}

void CHexDlgOpers::SetDlgProperties(std::uint64_t u64Flags)
{
	m_u64Flags = u64Flags;
}


//CHexDlgOpers private methods.

void CHexDlgOpers::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HEXCTRL_OPERS_COMBO_OPER, m_comboOper);
	DDX_Control(pDX, IDC_HEXCTRL_OPERS_COMBO_DTYPE, m_comboType);
}

template<typename T> requires TSize1248<T>
bool CHexDlgOpers::FillVecOper(bool fCheckBE)
{
	//To make sure the vector will have the size of the data type.
	//Even if operation doesn't need an operand (e.g. OPER_NOT), the spnData
	//in the HEXMODIFY must have appropriate size, equal to the data type size.
	m_vecOperData = RangeToVecBytes(T { });
	const auto pWndOper = GetDlgItem(IDC_HEXCTRL_OPERS_EDIT_OPERAND);
	if (!pWndOper->IsWindowEnabled())
		return true; //Some operations don't need operands.

	CStringW cwstrOper;
	pWndOper->GetWindowTextW(cwstrOper);
	const auto opt = stn::StrToNum<T>(cwstrOper.GetString());
	if (!opt) {
		MessageBoxW(L"Wrong operand.", L"Operand error", MB_ICONERROR);
		return false;
	}

	auto tOper = *opt;
	const auto eOperMode = GetOperMode();
	using enum EHexOperMode;
	if (eOperMode == OPER_DIV && tOper == 0) { //Division by zero check.
		MessageBoxW(L"Can't divide by zero.", L"Operand error", MB_ICONERROR);
		return { };
	}

	//Some operations don't need to swap the whole data in big-endian mode.
	//Instead, the operand itself can be swapped. Binary OR/XOR/AND are good examples.
	//Binary NOT and OPER_BITREV don't need to swap at all.
	if (fCheckBE && (eOperMode == OPER_OR || eOperMode == OPER_XOR
		|| eOperMode == OPER_AND || eOperMode == OPER_ASSIGN)) {
		tOper = ByteSwap(tOper);
	}

	m_vecOperData = RangeToVecBytes(tOper);

	return true;
}

auto CHexDlgOpers::GetOperMode()const->EHexOperMode
{
	return static_cast<EHexOperMode>(m_comboOper.GetItemData(m_comboOper.GetCurSel()));
}

auto CHexDlgOpers::GetDataType()const->EHexDataType
{
	return static_cast<EHexDataType>(m_comboType.GetItemData(m_comboType.GetCurSel()));
}

void CHexDlgOpers::OnCancel()
{
	if (m_u64Flags & HEXCTRL_FLAG_DLG_NOESC)
		return;

	static_cast<CDialogEx*>(GetParentOwner())->EndDialog(IDCANCEL);
}

void CHexDlgOpers::OnComboDataTypeSelChange()
{
	SetControlsState();
}

void CHexDlgOpers::OnComboOperSelChange()
{
	constexpr auto iIntegralTotal = 8; //Total amount of integral types.
	auto iCurrCount = m_comboType.GetCount();
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
		m_comboType.SetRedraw(FALSE);
		if (fShouldHaveFloats) {
			auto iIndex = m_comboType.AddString(L"Float");
			m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(DATA_FLOAT));
			iIndex = m_comboType.AddString(L"Double");
			m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(DATA_DOUBLE));
		}
		else {
			const auto iCurrSel = m_comboType.GetCurSel();

			for (auto iIndex = iCurrCount - 1; iIndex >= iIntegralTotal; --iIndex) {
				m_comboType.DeleteString(iIndex);
			}

			//When deleting float types, if selection was on one of float types change it to integral.
			if (iCurrSel > (iIntegralTotal - 1)) {
				m_comboType.SetCurSel(iIntegralTotal - 1);
			}
		}
		m_comboType.SetRedraw(TRUE);
		m_comboType.RedrawWindow();
	}

	SetControlsState();
}

auto CHexDlgOpers::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)->HBRUSH
{
	const auto hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	if (pWnd->GetDlgCtrlID() == IDC_HEXCTRL_OPERS_STAT_DESCR) {
		pDC->SetTextColor(RGB(0, 0, 200));
	}

	return hbr;
}

void CHexDlgOpers::OnDestroy()
{
	CDialogEx::OnDestroy();
	m_u64Flags = { };
	m_pHexCtrl = nullptr;
	m_vecOperData.clear();
}

void CHexDlgOpers::OnEditOperChange()
{
	SetControlsState();
}

BOOL CHexDlgOpers::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	using enum EHexOperMode;
	auto iIndex = m_comboOper.AddString(m_umapNames.at(OPER_ASSIGN).data());
	m_comboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_ASSIGN));
	m_comboOper.SetCurSel(iIndex);
	iIndex = m_comboOper.AddString(m_umapNames.at(OPER_ADD).data());
	m_comboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_ADD));
	iIndex = m_comboOper.AddString(m_umapNames.at(OPER_SUB).data());
	m_comboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_SUB));
	iIndex = m_comboOper.AddString(m_umapNames.at(OPER_MUL).data());
	m_comboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_MUL));
	iIndex = m_comboOper.AddString(m_umapNames.at(OPER_DIV).data());
	m_comboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_DIV));
	iIndex = m_comboOper.AddString(m_umapNames.at(OPER_MIN).data());
	m_comboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_MIN));
	iIndex = m_comboOper.AddString(m_umapNames.at(OPER_MAX).data());
	m_comboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_MAX));
	iIndex = m_comboOper.AddString(m_umapNames.at(OPER_OR).data());
	m_comboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_OR));
	iIndex = m_comboOper.AddString(m_umapNames.at(OPER_XOR).data());
	m_comboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_XOR));
	iIndex = m_comboOper.AddString(m_umapNames.at(OPER_AND).data());
	m_comboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_AND));
	iIndex = m_comboOper.AddString(m_umapNames.at(OPER_NOT).data());
	m_comboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_NOT));
	iIndex = m_comboOper.AddString(m_umapNames.at(OPER_SHL).data());
	m_comboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_SHL));
	iIndex = m_comboOper.AddString(m_umapNames.at(OPER_SHR).data());
	m_comboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_SHR));
	iIndex = m_comboOper.AddString(m_umapNames.at(OPER_ROTL).data());
	m_comboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_ROTL));
	iIndex = m_comboOper.AddString(m_umapNames.at(OPER_ROTR).data());
	m_comboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_ROTR));
	iIndex = m_comboOper.AddString(m_umapNames.at(OPER_SWAP).data());
	m_comboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_SWAP));
	iIndex = m_comboOper.AddString(m_umapNames.at(OPER_BITREV).data());
	m_comboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_BITREV));

	using enum EHexDataType;
	iIndex = m_comboType.AddString(L"Int8");
	m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(DATA_INT8));
	m_comboType.SetCurSel(iIndex);
	iIndex = m_comboType.AddString(L"Unsigned Int8");
	m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(DATA_UINT8));
	iIndex = m_comboType.AddString(L"Int16");
	m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(DATA_INT16));
	iIndex = m_comboType.AddString(L"Unsigned Int16");
	m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(DATA_UINT16));
	iIndex = m_comboType.AddString(L"Int32");
	m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(DATA_INT32));
	iIndex = m_comboType.AddString(L"Unsigned Int32");
	m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(DATA_UINT32));
	iIndex = m_comboType.AddString(L"Int64");
	m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(DATA_INT64));
	iIndex = m_comboType.AddString(L"Unsigned Int64");
	m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(DATA_UINT64));

	CheckRadioButton(IDC_HEXCTRL_OPERS_RAD_ALL, IDC_HEXCTRL_OPERS_RAD_SEL, IDC_HEXCTRL_OPERS_RAD_ALL);
	OnComboOperSelChange();

	return TRUE;
}

void CHexDlgOpers::OnOK()
{
	if (!m_pHexCtrl->IsCreated() || !m_pHexCtrl->IsDataSet())
		return;

	using enum EHexOperMode; using enum EHexDataType; using enum EHexModifyMode;
	const auto eOperMode = GetOperMode();
	const auto eDataType = GetDataType();
	const auto fDataOneByte = eDataType == DATA_INT8 || eDataType == DATA_UINT8;
	const auto fCheckBE = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_OPERS_CHK_BE))->GetCheck() == BST_CHECKED;
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
	const auto iRadioAllOrSel = GetCheckedRadioButton(IDC_HEXCTRL_OPERS_RAD_ALL, IDC_HEXCTRL_OPERS_RAD_SEL);
	if (iRadioAllOrSel == IDC_HEXCTRL_OPERS_RAD_ALL) {
		if (MessageBoxW(L"You are about to modify the entire data region.\r\nAre you sure?",
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
	const auto pComboOper = GetDlgItem(IDC_HEXCTRL_OPERS_EDIT_OPERAND);
	auto fEditOper = true;
	auto fBtnEnter = pComboOper->GetWindowTextLengthW() > 0;

	switch (GetOperMode()) {
	case OPER_NOT:
	case OPER_BITREV:
		fEditOper = false;
		fBtnEnter = true;
		break;
	case OPER_SWAP:
		fEditOper = false;
		fBtnEnter = !fDataOneByte;
		break;
	default:
		break;
	};

	pComboOper->EnableWindow(fEditOper);
	GetDlgItem(IDC_HEXCTRL_OPERS_CHK_BE)->EnableWindow(fDataOneByte ? FALSE : fEditOper);
	const auto pBtnEnter = GetDlgItem(IDOK);
	pBtnEnter->EnableWindow(fBtnEnter);
	pBtnEnter->SetWindowTextW(m_umapNames.at(GetOperMode()).data());
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

	GetDlgItem(IDC_HEXCTRL_OPERS_STAT_DESCR)->SetWindowTextW(wsvDescr.data());
}


namespace HEXCTRL::INTERNAL {
	//CHexDlgFillData.
	class CHexDlgFillData final : public CDialogEx {
	public:
		void Create(CWnd* pParent, IHexCtrl* pHexCtrl);
		[[nodiscard]] auto GetDlgItemHandle(EHexDlgItem eItem)const->HWND;
		void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		void SetDlgProperties(std::uint64_t u64Flags);
	private:
		enum class EFillType : std::uint8_t; //Forward declaration.
		void DoDataExchange(CDataExchange* pDX)override;
		[[nodiscard]] auto GetFillType()const->EFillType;
		void OnCancel()override;
		afx_msg void OnComboDataEditChange();
		afx_msg void OnComboTypeSelChange();
		afx_msg void OnDestroy();
		BOOL OnInitDialog()override;
		void OnOK()override;
		void SetControlsState();
		DECLARE_MESSAGE_MAP();
	private:
		IHexCtrl* m_pHexCtrl { };
		CComboBox m_comboType; //Fill type combo-box.
		CComboBox m_comboData; //Data combo-box.
		std::vector<std::byte> m_vecFillData; //Fill data vector.
		std::uint64_t m_u64Flags { };
	};
}

enum class CHexDlgFillData::EFillType : std::uint8_t {
	FILL_HEX, FILL_ASCII, FILL_WCHAR, FILL_RAND_MT19937, FILL_RAND_FAST
};

BEGIN_MESSAGE_MAP(CHexDlgFillData, CDialogEx)
	ON_CBN_SELCHANGE(IDC_HEXCTRL_FILLDATA_COMBO_TYPE, &CHexDlgFillData::OnComboTypeSelChange)
	ON_CBN_EDITCHANGE(IDC_HEXCTRL_FILLDATA_COMBO_DATA, &CHexDlgFillData::OnComboDataEditChange)
	ON_WM_ACTIVATE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CHexDlgFillData::Create(CWnd* pParent, IHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	assert(pParent);
	if (pHexCtrl == nullptr)
		return;

	m_pHexCtrl = pHexCtrl;
	CDialogEx::Create(IDD_HEXCTRL_FILLDATA, pParent);
}

auto CHexDlgFillData::GetDlgItemHandle(EHexDlgItem eItem)const->HWND
{
	using enum EHexDlgItem;
	switch (eItem) {
	case FILLDATA_COMBO_DATA:
		return m_comboData;
	default:
		return { };
	}
}

void CHexDlgFillData::OnActivate(UINT nState, CWnd* /*pWndOther*/, BOOL /*bMinimized*/)
{
	if (m_pHexCtrl == nullptr || !m_pHexCtrl->IsCreated() || !m_pHexCtrl->IsDataSet())
		return;

	if (nState == WA_ACTIVE || nState == WA_CLICKACTIVE) {
		const auto fSelection { m_pHexCtrl->HasSelection() };
		CheckRadioButton(IDC_HEXCTRL_FILLDATA_RAD_ALL, IDC_HEXCTRL_FILLDATA_RAD_SEL,
			fSelection ? IDC_HEXCTRL_FILLDATA_RAD_SEL : IDC_HEXCTRL_FILLDATA_RAD_ALL);
		GetDlgItem(IDC_HEXCTRL_FILLDATA_RAD_SEL)->EnableWindow(fSelection);
	}
}

void CHexDlgFillData::SetDlgProperties(std::uint64_t u64Flags)
{
	m_u64Flags = u64Flags;
}


//CHexDlgFillData private methods.

void CHexDlgFillData::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HEXCTRL_FILLDATA_COMBO_TYPE, m_comboType);
	DDX_Control(pDX, IDC_HEXCTRL_FILLDATA_COMBO_DATA, m_comboData);
}

auto CHexDlgFillData::GetFillType()const->CHexDlgFillData::EFillType
{
	return static_cast<EFillType>(m_comboType.GetItemData(m_comboType.GetCurSel()));
}

void CHexDlgFillData::OnCancel()
{
	if (m_u64Flags & HEXCTRL_FLAG_DLG_NOESC)
		return;

	static_cast<CDialogEx*>(GetParentOwner())->EndDialog(IDCANCEL);
}

void CHexDlgFillData::OnComboDataEditChange()
{
	SetControlsState();
}

void CHexDlgFillData::OnComboTypeSelChange()
{
	SetControlsState();
}

void CHexDlgFillData::OnDestroy()
{
	CDialogEx::OnDestroy();
	m_u64Flags = { };
	m_pHexCtrl = nullptr;
	m_vecFillData.clear();
}

BOOL CHexDlgFillData::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	auto iIndex = m_comboType.AddString(L"Hex Bytes");
	m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(EFillType::FILL_HEX));
	m_comboType.SetCurSel(iIndex);
	iIndex = m_comboType.AddString(L"Text ASCII");
	m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(EFillType::FILL_ASCII));
	iIndex = m_comboType.AddString(L"Text UTF-16");
	m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(EFillType::FILL_WCHAR));
	iIndex = m_comboType.AddString(L"Random Data (MT19937)");
	m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(EFillType::FILL_RAND_MT19937));
	iIndex = m_comboType.AddString(L"Pseudo Random Data (fast, but less secure)");
	m_comboType.SetItemData(iIndex, static_cast<DWORD_PTR>(EFillType::FILL_RAND_FAST));

	CheckRadioButton(IDC_HEXCTRL_FILLDATA_RAD_ALL, IDC_HEXCTRL_FILLDATA_RAD_SEL, IDC_HEXCTRL_FILLDATA_RAD_ALL);
	m_comboData.LimitText(256); //Max characters of the combo-box.
	SetControlsState();

	return TRUE;
}

void CHexDlgFillData::OnOK()
{
	if (!m_pHexCtrl->IsCreated() || !m_pHexCtrl->IsDataSet())
		return;

	using enum EFillType;
	const auto eType = GetFillType();
	if (eType == FILL_HEX || eType == FILL_ASCII || eType == FILL_WCHAR) {
		if (m_comboData.GetWindowTextLengthW() == 0) {
			MessageBoxW(L"Missing fill data.", L"Data error", MB_ICONERROR);
			return;
		}
	}

	VecSpan vecSpan;
	const auto iRadioAllOrSel = GetCheckedRadioButton(IDC_HEXCTRL_FILLDATA_RAD_ALL, IDC_HEXCTRL_FILLDATA_RAD_SEL);
	if (iRadioAllOrSel == IDC_HEXCTRL_FILLDATA_RAD_ALL) {
		if (MessageBoxW(L"You are about to modify the entire data region.\r\nAre you sure?", L"Modify all data?",
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
	CStringW cwstrText;
	m_comboData.GetWindowTextW(cwstrText);
	switch (eType) {
	case FILL_HEX:
	{
		auto optData = NumStrToHex(cwstrText.GetString());
		if (!optData) {
			MessageBoxW(L"Wrong Hex format.", L"Format error", MB_ICONERROR);
			return;
		}

		m_vecFillData = RangeToVecBytes(*optData);
		eModifyMode = MODIFY_REPEAT;
	}
	break;
	case FILL_ASCII:
		m_vecFillData = RangeToVecBytes(WstrToStr(cwstrText.GetString()));
		eModifyMode = MODIFY_REPEAT;
		break;
	case FILL_WCHAR:
		eModifyMode = MODIFY_REPEAT;
		m_vecFillData = RangeToVecBytes(cwstrText);
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
	if (m_comboData.FindStringExact(0, cwstrText) == CB_ERR) {
		if (m_comboData.GetCount() == 50) { //Keep max 50 strings in list.
			m_comboData.DeleteString(49);
		}
		m_comboData.InsertString(0, cwstrText);
	}

	if (m_vecFillData.size() > vecSpan.back().ullSize) {
		MessageBoxW(L"Fill data size is bigger than the region selected for modification, please select a larger region.",
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
	const auto fIsData = m_comboData.GetWindowTextLengthW() > 0;
	m_comboData.EnableWindow(!fRND);
	GetDlgItem(IDOK)->EnableWindow(fRND ? TRUE : fIsData);
}


//CHexDlgModify methods.

BEGIN_MESSAGE_MAP(CHexDlgModify, CDialogEx)
	ON_NOTIFY(TCN_SELCHANGE, IDC_HEXCTRL_MODIFY_TAB, &CHexDlgModify::OnTabSelChanged)
	ON_WM_ACTIVATE()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

CHexDlgModify::CHexDlgModify() : m_pDlgOpers { std::make_unique<CHexDlgOpers>() },
m_pDlgFillData { std::make_unique<CHexDlgFillData>() }
{
}

CHexDlgModify::~CHexDlgModify() = default;

auto CHexDlgModify::GetDlgItemHandle(EHexDlgItem eItem)const->HWND
{
	if (!IsWindow(m_hWnd)) {
		return { };
	}

	using enum EHexDlgItem;
	switch (eItem) {
	case FILLDATA_COMBO_DATA:
		return m_pDlgFillData->GetDlgItemHandle(eItem);
	default:
		return { };
	}
}

void CHexDlgModify::Initialize(IHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	m_pHexCtrl = pHexCtrl;
}

void CHexDlgModify::SetDlgProperties(std::uint64_t u64Flags)
{
	m_u64Flags = u64Flags;
	m_pDlgOpers->SetDlgProperties(u64Flags);
	m_pDlgFillData->SetDlgProperties(u64Flags);
}

BOOL CHexDlgModify::ShowWindow(int nCmdShow, int iTab)
{
	if (!IsWindow(m_hWnd)) {
		Create(IDD_HEXCTRL_MODIFY, CWnd::FromHandle(m_pHexCtrl->GetWndHandle(EHexWnd::WND_MAIN)));
	}

	SetCurrentTab(iTab);

	return CDialogEx::ShowWindow(nCmdShow);
}


//CHexDlgModify private methods.

void CHexDlgModify::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HEXCTRL_MODIFY_TAB, m_tabMain);
}

bool CHexDlgModify::IsNoEsc()const
{
	return m_u64Flags & HEXCTRL_FLAG_DLG_NOESC;
}

void CHexDlgModify::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CDialogEx::OnActivate(nState, pWndOther, bMinimized);

	m_pDlgOpers->OnActivate(nState, pWndOther, bMinimized);
	m_pDlgFillData->OnActivate(nState, pWndOther, bMinimized);
}

void CHexDlgModify::OnCancel()
{
	if (IsNoEsc()) //Not closing Dialog on Escape key.
		return;

	CDialogEx::OnCancel();
}

void CHexDlgModify::OnClose()
{
	EndDialog(IDCANCEL);
}

void CHexDlgModify::OnDestroy()
{
	CDialogEx::OnDestroy();
	m_u64Flags = { };
	m_pHexCtrl = nullptr;
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