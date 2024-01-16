/****************************************************************************************
* Copyright © 2018-2024 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "../../dep/StrToNum/StrToNum/StrToNum.h"
#include "../../res/HexCtrlRes.h"
#include "CHexDlgModify.h"
#include <cassert>
#include <unordered_map>

import HEXCTRL.HexUtility;

namespace HEXCTRL::INTERNAL {
	//CHexDlgOpers.
	class CHexDlgOpers final : public CDialogEx {
	public:
		void Create(CWnd* pParent, IHexCtrl* pHexCtrl);
		void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	private:
		void DoDataExchange(CDataExchange* pDX)override;
		[[nodiscard]] auto GetOperMode()const->EHexOperMode;
		[[nodiscard]] auto GetDataSize()const->EHexDataSize;
		BOOL OnCommand(WPARAM wParam, LPARAM lParam)override;
		BOOL OnInitDialog()override;
		void OnOK()override;
		void SetControlsState()const;
		void SetOKButtonName()const;
		DECLARE_MESSAGE_MAP();
	private:
		IHexCtrl* m_pHexCtrl { };
		CComboBox m_stComboOper; //Operation combo-box.
		CComboBox m_stComboSize; //Data size combo-box.
		using enum EHexOperMode;
		inline static const std::unordered_map<EHexOperMode, std::wstring_view> m_mapNames {
			{ OPER_ASSIGN, L"Assign" }, { OPER_ADD, L"Add" }, { OPER_SUB, L"Subtract" },
			{ OPER_MUL, L"Multiply" }, { OPER_DIV, L"Divide" }, { OPER_CEIL, L"Ceiling" },
			{ OPER_FLOOR, L"Floor" }, { OPER_OR, L"OR" }, { OPER_XOR, L"XOR" }, { OPER_AND, L"AND" },
			{ OPER_NOT, L"NOT" }, { OPER_SHL, L"SHL" }, { OPER_SHR, L"SHR" }, { OPER_ROTL, L"ROTL" },
			{ OPER_ROTR, L"ROTR" }, { OPER_SWAP, L"Swap Bytes" }, { OPER_BITREV, L"Reverse Bits" }
		};
	};

	BEGIN_MESSAGE_MAP(CHexDlgOpers, CDialogEx)
		ON_WM_ACTIVATE()
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


	//CHexDlgOpers private methods.

	void CHexDlgOpers::DoDataExchange(CDataExchange* pDX)
	{
		CDialogEx::DoDataExchange(pDX);
		DDX_Control(pDX, IDC_HEXCTRL_OPERS_COMBO_OPER, m_stComboOper);
		DDX_Control(pDX, IDC_HEXCTRL_OPERS_COMBO_SIZE, m_stComboSize);
	}

	auto CHexDlgOpers::GetOperMode()const->EHexOperMode
	{
		return static_cast<EHexOperMode>(m_stComboOper.GetItemData(m_stComboOper.GetCurSel()));
	}

	auto CHexDlgOpers::GetDataSize()const->EHexDataSize
	{
		return static_cast<EHexDataSize>(m_stComboSize.GetItemData(m_stComboSize.GetCurSel()));
	}

	BOOL CHexDlgOpers::OnCommand(WPARAM wParam, LPARAM lParam)
	{
		switch (LOWORD(wParam)) //Control ID.
		{
		case IDC_HEXCTRL_OPERS_COMBO_OPER:
		case IDC_HEXCTRL_OPERS_COMBO_SIZE:
			SetControlsState();
			if (HIWORD(wParam) == CBN_SELCHANGE) { //Combo-box selection changed.
				SetOKButtonName();
			}
			return TRUE;
		default:
			break;
		}

		return CDialogEx::OnCommand(wParam, lParam);
	}

	BOOL CHexDlgOpers::OnInitDialog()
	{
		CDialogEx::OnInitDialog();

		using enum EHexOperMode;
		auto iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_ASSIGN).data());
		m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_ASSIGN));
		m_stComboOper.SetCurSel(iIndex);
		iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_ADD).data());
		m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_ADD));
		iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_SUB).data());
		m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_SUB));
		iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_MUL).data());
		m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_MUL));
		iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_DIV).data());
		m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_DIV));
		iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_CEIL).data());
		m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_CEIL));
		iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_FLOOR).data());
		m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_FLOOR));
		iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_OR).data());
		m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_OR));
		iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_XOR).data());
		m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_XOR));
		iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_AND).data());
		m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_AND));
		iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_NOT).data());
		m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_NOT));
		iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_SHL).data());
		m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_SHL));
		iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_SHR).data());
		m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_SHR));
		iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_ROTL).data());
		m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_ROTL));
		iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_ROTR).data());
		m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_ROTR));
		iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_SWAP).data());
		m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_SWAP));
		iIndex = m_stComboOper.AddString(m_mapNames.at(OPER_BITREV).data());
		m_stComboOper.SetItemData(iIndex, static_cast<DWORD_PTR>(OPER_BITREV));

		using enum EHexDataSize;
		iIndex = m_stComboSize.AddString(L"BYTE");
		m_stComboSize.SetItemData(iIndex, static_cast<DWORD_PTR>(SIZE_BYTE));
		m_stComboSize.SetCurSel(iIndex);
		iIndex = m_stComboSize.AddString(L"WORD");
		m_stComboSize.SetItemData(iIndex, static_cast<DWORD_PTR>(SIZE_WORD));
		iIndex = m_stComboSize.AddString(L"DWORD");
		m_stComboSize.SetItemData(iIndex, static_cast<DWORD_PTR>(SIZE_DWORD));
		iIndex = m_stComboSize.AddString(L"QWORD");
		m_stComboSize.SetItemData(iIndex, static_cast<DWORD_PTR>(SIZE_QWORD));

		CheckRadioButton(IDC_HEXCTRL_OPERS_RAD_ALL, IDC_HEXCTRL_OPERS_RAD_SEL, IDC_HEXCTRL_OPERS_RAD_ALL);
		SetOKButtonName();

		return TRUE;
	}

	void CHexDlgOpers::OnOK()
	{
		if (!m_pHexCtrl->IsCreated() || !m_pHexCtrl->IsDataSet())
			return;

		using enum EHexOperMode;
		using enum EHexDataSize;
		const auto eOperMode = GetOperMode();
		const auto eDataSize = GetDataSize();
		const auto fCheckBE = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_OPERS_CHK_BE))->GetCheck() == BST_CHECKED;
		auto fBigEndian = fCheckBE && eDataSize != SIZE_BYTE && eOperMode != OPER_NOT && eOperMode != OPER_SWAP
			&& eOperMode != OPER_BITREV;

		HEXMODIFY hms { .enModifyMode = EHexModifyMode::MODIFY_OPERATION, .enOperMode = eOperMode,
			.enDataSize = eDataSize };

		LONGLONG llOperand { };
		if (const auto pWndOperand = GetDlgItem(IDC_HEXCTRL_OPERS_EDIT_OPERAND); pWndOperand->IsWindowEnabled()) {
			wchar_t buffOperand[32];
			pWndOperand->GetWindowTextW(buffOperand, static_cast<int>(std::size(buffOperand)));

			std::wstring wstrErr;
			if (buffOperand[0] == L'\0') { //Operand field emptiness check.
				wstrErr = L"Missing Operand!";
			}
			else if (const auto optOperand = stn::StrToLL(buffOperand); !optOperand) {
				wstrErr = L"Wrong number format!";
			}
			else if (llOperand = *optOperand; hms.enOperMode == OPER_DIV && llOperand == 0) { //Division by zero check.
				wstrErr = L"Wrong number format! Can not divide by zero!";
			}

			if (!wstrErr.empty()) {
				MessageBoxW(wstrErr.data(), L"Operand Error!", MB_ICONERROR);
				return;
			}

			if (fBigEndian) {
				/***************************************************************************
				* Some operations don't need to swap the whole data in big-endian mode.
				* Instead, the operand can be swapped here just once.
				* Binary OR/XOR/AND are good examples, binary NOT doesn't need swap at all.
				* The fSwapHere flag shows exactly this, that the operand can be swapped here.
				***************************************************************************/
				if (eOperMode == OPER_OR || eOperMode == OPER_XOR || eOperMode == OPER_AND || eOperMode == OPER_ASSIGN) {
					fBigEndian = false;
					switch (hms.enDataSize) {
					case SIZE_WORD:
						llOperand = static_cast<LONGLONG>(ByteSwap(static_cast<WORD>(llOperand)));
						break;
					case SIZE_DWORD:
						llOperand = static_cast<LONGLONG>(ByteSwap(static_cast<DWORD>(llOperand)));
						break;
					case SIZE_QWORD:
						llOperand = static_cast<LONGLONG>(ByteSwap(static_cast<QWORD>(llOperand)));
						break;
					default:
						break;
					}
				}
			}
		}

		const auto iRadioAllOrSel = GetCheckedRadioButton(IDC_HEXCTRL_OPERS_RAD_ALL, IDC_HEXCTRL_OPERS_RAD_SEL);
		if (iRadioAllOrSel == IDC_HEXCTRL_OPERS_RAD_ALL) {
			if (MessageBoxW(L"You are about to modify the entire data region.\r\nAre you sure?",
				L"Modify All data?", MB_YESNO | MB_ICONWARNING) == IDNO)
				return;

			hms.vecSpan.emplace_back(0, m_pHexCtrl->GetDataSize());
		}
		else {
			if (!m_pHexCtrl->HasSelection())
				return;

			hms.vecSpan = m_pHexCtrl->GetSelection();
		}

		hms.fBigEndian = fBigEndian;
		hms.spnData = { reinterpret_cast<std::byte*>(&llOperand), sizeof(llOperand) };
		m_pHexCtrl->ModifyData(hms);
		m_pHexCtrl->Redraw();
	}

	void CHexDlgOpers::SetControlsState()const
	{
		using enum EHexOperMode; using enum EHexDataSize;
		const auto eDataSize = GetDataSize();
		auto fOperandEnable = TRUE;
		auto fBtnEnable = TRUE;

		switch (GetOperMode()) {
		case OPER_NOT:
		case OPER_BITREV:
			fOperandEnable = FALSE;
			break;
		case OPER_SWAP:
			fOperandEnable = FALSE;
			fBtnEnable = eDataSize != SIZE_BYTE;
			break;
		default:
			break;
		};

		GetDlgItem(IDC_HEXCTRL_OPERS_EDIT_OPERAND)->EnableWindow(fOperandEnable);
		GetDlgItem(IDC_HEXCTRL_OPERS_CHK_BE)->EnableWindow(eDataSize == SIZE_BYTE ? FALSE : fOperandEnable);
		GetDlgItem(IDOK)->EnableWindow(fBtnEnable);
	}

	void CHexDlgOpers::SetOKButtonName()const
	{
		GetDlgItem(IDOK)->SetWindowTextW(m_mapNames.at(GetOperMode()).data());
	}


	//CHexDlgFillData.
	class CHexDlgFillData final : public CDialogEx {
	public:
		void Create(CWnd* pParent, IHexCtrl* pHexCtrl);
		void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	private:
		enum class EFillType : std::uint8_t; //Forward declaration.
		void DoDataExchange(CDataExchange* pDX)override;
		[[nodiscard]] auto GetFillType()const->EFillType;
		BOOL OnCommand(WPARAM wParam, LPARAM lParam)override;
		BOOL OnInitDialog()override;
		void OnOK()override;
		DECLARE_MESSAGE_MAP();
	private:
		IHexCtrl* m_pHexCtrl { };
		CComboBox m_stComboType; //Fill type combo-box.
		CComboBox m_stComboData; //Data combo-box.
	};

	enum class CHexDlgFillData::EFillType : std::uint8_t {
		FILL_HEX, FILL_ASCII, FILL_WCHAR, FILL_RAND_MT19937, FILL_RAND_FAST
	};

	BEGIN_MESSAGE_MAP(CHexDlgFillData, CDialogEx)
		ON_WM_ACTIVATE()
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


	//CHexDlgFillData private methods.

	void CHexDlgFillData::DoDataExchange(CDataExchange* pDX)
	{
		CDialogEx::DoDataExchange(pDX);
		DDX_Control(pDX, IDC_HEXCTRL_FILLDATA_COMBO_TYPE, m_stComboType);
		DDX_Control(pDX, IDC_HEXCTRL_FILLDATA_COMBO_DATA, m_stComboData);
	}

	auto CHexDlgFillData::GetFillType()const->CHexDlgFillData::EFillType
	{
		return static_cast<EFillType>(m_stComboType.GetItemData(m_stComboType.GetCurSel()));
	}

	BOOL CHexDlgFillData::OnCommand(WPARAM wParam, LPARAM lParam)
	{
		using enum EFillType;
		switch (LOWORD(wParam)) {
		case IDC_HEXCTRL_FILLDATA_COMBO_TYPE:
		{
			const auto enFillType = GetFillType();
			m_stComboData.EnableWindow(enFillType != FILL_RAND_MT19937 && enFillType != FILL_RAND_FAST);
		}
		return TRUE;
		default:
			break;
		}

		return CDialogEx::OnCommand(wParam, lParam);
	}

	BOOL CHexDlgFillData::OnInitDialog()
	{
		CDialogEx::OnInitDialog();

		auto iIndex = m_stComboType.AddString(L"Hex Values");
		m_stComboType.SetItemData(iIndex, static_cast<DWORD_PTR>(EFillType::FILL_HEX));
		m_stComboType.SetCurSel(iIndex);
		iIndex = m_stComboType.AddString(L"ASCII Text");
		m_stComboType.SetItemData(iIndex, static_cast<DWORD_PTR>(EFillType::FILL_ASCII));
		iIndex = m_stComboType.AddString(L"UTF-16 Text");
		m_stComboType.SetItemData(iIndex, static_cast<DWORD_PTR>(EFillType::FILL_WCHAR));
		iIndex = m_stComboType.AddString(L"Random Data (MT19937)");
		m_stComboType.SetItemData(iIndex, static_cast<DWORD_PTR>(EFillType::FILL_RAND_MT19937));
		iIndex = m_stComboType.AddString(L"Pseudo Random Data (fast, but less secure)");
		m_stComboType.SetItemData(iIndex, static_cast<DWORD_PTR>(EFillType::FILL_RAND_FAST));

		CheckRadioButton(IDC_HEXCTRL_FILLDATA_RAD_ALL, IDC_HEXCTRL_FILLDATA_RAD_SEL, IDC_HEXCTRL_FILLDATA_RAD_ALL);
		m_stComboData.LimitText(512); //Max characters of combo-box.

		return TRUE;
	}

	void CHexDlgFillData::OnOK()
	{
		if (!m_pHexCtrl->IsCreated() || !m_pHexCtrl->IsDataSet())
			return;

		wchar_t pwszComboText[MAX_PATH * 2];
		if (m_stComboData.IsWindowEnabled() && m_stComboData.GetWindowTextW(pwszComboText, MAX_PATH) == 0) { //No text.
			MessageBoxW(L"Missing Fill data!", L"Data error!", MB_ICONERROR);
			return;
		}

		HEXMODIFY hms;
		const auto iRadioAllOrSel = GetCheckedRadioButton(IDC_HEXCTRL_FILLDATA_RAD_ALL, IDC_HEXCTRL_FILLDATA_RAD_SEL);
		if (iRadioAllOrSel == IDC_HEXCTRL_FILLDATA_RAD_ALL) {
			if (MessageBoxW(L"You are about to modify the entire data region.\r\nAre you sure?", L"Modify all data?",
				MB_YESNO | MB_ICONWARNING) == IDNO)
				return;

			hms.vecSpan.emplace_back(0, m_pHexCtrl->GetDataSize());
		}
		else {
			if (!m_pHexCtrl->HasSelection())
				return;

			hms.vecSpan = m_pHexCtrl->GetSelection();
		}

		std::wstring wstrFillWith = pwszComboText;
		std::string strFillWith; //Data holder for FILL_HEX and FILL_ASCII modes.
		switch (GetFillType()) {
		case EFillType::FILL_HEX:
		{
			auto optData = NumStrToHex(wstrFillWith);
			if (!optData) {
				MessageBoxW(L"Wrong Hex format!", L"Format Error", MB_ICONERROR);
				return;
			}
			strFillWith = std::move(*optData);
			hms.enModifyMode = EHexModifyMode::MODIFY_REPEAT;
			hms.spnData = { reinterpret_cast<const std::byte*>(strFillWith.data()), strFillWith.size() };
		}
		break;
		case EFillType::FILL_ASCII:
			strFillWith = WstrToStr(wstrFillWith);
			hms.enModifyMode = EHexModifyMode::MODIFY_REPEAT;
			hms.spnData = { reinterpret_cast<const std::byte*>(strFillWith.data()), strFillWith.size() };
			break;
		case EFillType::FILL_WCHAR:
			hms.enModifyMode = EHexModifyMode::MODIFY_REPEAT;
			hms.spnData = { reinterpret_cast<const std::byte*>(wstrFillWith.data()), wstrFillWith.size() * sizeof(WCHAR) };
			break;
		case EFillType::FILL_RAND_MT19937:
			hms.enModifyMode = EHexModifyMode::MODIFY_RAND_MT19937;
			break;
		case EFillType::FILL_RAND_FAST:
			hms.enModifyMode = EHexModifyMode::MODIFY_RAND_FAST;
			break;
		default:
			break;
		}

		//Insert wstring into ComboBox only if it's not already presented.
		if (m_stComboData.FindStringExact(0, wstrFillWith.data()) == CB_ERR) {
			//Keep max 50 strings in list.
			if (m_stComboData.GetCount() == 50) {
				m_stComboData.DeleteString(49);
			}
			m_stComboData.InsertString(0, wstrFillWith.data());
		}

		if (hms.spnData.size() > hms.vecSpan.back().ullSize) {
			MessageBoxW(L"Fill data size is bigger than the region selected for modification, please select a larger region.",
				L"Data region size error.", MB_ICONERROR);
			return;
		}

		m_pHexCtrl->ModifyData(hms);
		m_pHexCtrl->Redraw();
		::SetFocus(m_pHexCtrl->GetWndHandle(EHexWnd::WND_MAIN));
	}
}

using namespace HEXCTRL::INTERNAL;

BEGIN_MESSAGE_MAP(CHexDlgModify, CDialogEx)
	ON_NOTIFY(TCN_SELCHANGE, IDC_HEXCTRL_MODIFY_TAB, &CHexDlgModify::OnTabSelChanged)
	ON_WM_ACTIVATE()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

CHexDlgModify::CHexDlgModify() : m_pDlgOpers { std::make_unique<CHexDlgOpers>() },
m_pDlgFillData { std::make_unique<CHexDlgFillData>() } {}

CHexDlgModify::~CHexDlgModify() = default;

auto CHexDlgModify::GetDlgData()const->std::uint64_t
{
	if (!IsWindow(m_hWnd)) {
		return { };
	}

	std::uint64_t ullData { };

	if (IsNoEsc()) {
		ullData |= HEXCTRL_FLAG_NOESC;
	}

	return ullData;
}

void CHexDlgModify::Initialize(IHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	m_pHexCtrl = pHexCtrl;
}

auto CHexDlgModify::SetDlgData(std::uint64_t ullData, bool fCreate)->HWND
{
	m_u64DlgData = ullData;

	if (!IsWindow(m_hWnd)) {
		if (fCreate) {
			Create(IDD_HEXCTRL_MODIFY, CWnd::FromHandle(m_pHexCtrl->GetWndHandle(EHexWnd::WND_MAIN)));
		}
	}
	else {
		ApplyDlgData();
	}

	return m_hWnd;
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

void CHexDlgModify::ApplyDlgData()
{
}

void CHexDlgModify::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HEXCTRL_MODIFY_TAB, m_tabMain);
}

bool CHexDlgModify::IsNoEsc()const
{
	return m_u64DlgData & HEXCTRL_FLAG_NOESC;
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
	m_u64DlgData = { };
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

	ApplyDlgData();

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