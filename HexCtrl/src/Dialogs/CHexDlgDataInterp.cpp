/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "../../res/HexCtrlRes.h"
#include "CHexDlgDataInterp.h"
#include <algorithm>
#include <bit>
#include <cassert>
#include <format>

using namespace HEXCTRL::INTERNAL;

//MS-DOS Date+Time structure (as used in FAT file system directory entry).
//https://msdn.microsoft.com/en-us/library/ms724274(v=vs.85).aspx
union CHexDlgDataInterp::UMSDOSDateTime {
	struct {
		WORD wTime;	//Time component.
		WORD wDate;	//Date component.
	} TimeDate;
	DWORD dwTimeDate;
};

//Microsoft UDTTM time (as used by Microsoft Compound Document format).
//https://docs.microsoft.com/en-us/openspecs/office_file_formats/ms-doc/164c0c2e-6031-439e-88ad-69d00b69f414
union CHexDlgDataInterp::UDTTM {
	struct {
		unsigned long minute : 6; //6+5+5+4+9+3=32.
		unsigned long hour : 5;
		unsigned long dayofmonth : 5;
		unsigned long month : 4;
		unsigned long year : 9;
		unsigned long weekday : 3;
	} components;
	unsigned long dwValue;
};

enum class CHexDlgDataInterp::EGroup : std::uint8_t {
	GR_BINARY, GR_INTEGRAL, GR_FLOAT, GR_TIME, GR_MISC, GR_GUIDTIME
};

enum class CHexDlgDataInterp::EName : std::uint8_t {
	NAME_BINARY, NAME_INT8, NAME_UINT8, NAME_INT16, NAME_UINT16,
	NAME_INT32, NAME_UINT32, NAME_INT64, NAME_UINT64,
	NAME_FLOAT, NAME_DOUBLE, NAME_TIME32T, NAME_TIME64T,
	NAME_FILETIME, NAME_OLEDATETIME, NAME_JAVATIME, NAME_MSDOSTIME,
	NAME_MSDTTMTIME, NAME_SYSTEMTIME, NAME_GUIDTIME, NAME_GUID
};

enum class CHexDlgDataInterp::EDataSize : std::uint8_t {
	SIZE_BYTE = 1U, SIZE_WORD = 2U, SIZE_DWORD = 4U, SIZE_QWORD = 8U, SIZE_DQWORD = 16U
};

struct CHexDlgDataInterp::GRIDDATA {
	CMFCPropertyGridProperty* pProp { };
	EGroup eGroup { };
	EName eName { };
	EDataSize eSize { };
};

BEGIN_MESSAGE_MAP(CHexPropGridCtrl, CMFCPropertyGridCtrl)
	ON_WM_SIZE()
END_MESSAGE_MAP()

CHexDlgDataInterp::CHexDlgDataInterp() = default;
CHexDlgDataInterp::~CHexDlgDataInterp() = default;

void CHexDlgDataInterp::CreateDlg()
{
	//m_Wnd is set in the OnInitDialog().
	if (const auto hWnd = ::CreateDialogParamW(wnd::GetHinstance(), MAKEINTRESOURCEW(IDD_HEXCTRL_DATAINTERP),
		m_pHexCtrl->GetWndHandle(EHexWnd::WND_MAIN), wnd::DlgWndProc<CHexDlgDataInterp>, reinterpret_cast<LPARAM>(this));
		hWnd == nullptr) {
		DBG_REPORT(L"CreateDialogParamW failed.");
	}
}

auto CHexDlgDataInterp::GetDlgItemHandle(EHexDlgItem eItem)const->HWND
{
	if (!m_Wnd.IsWindow()) {
		return { };
	}

	using enum EHexDlgItem;
	switch (eItem) {
	case DATAINTERP_CHK_HEX:
		return m_WndBtnHex;
	case DATAINTERP_CHK_BE:
		return m_WndBtnBE;
	default:
		return { };
	}
}

auto CHexDlgDataInterp::GetHglDataSize()const->DWORD
{
	return m_dwHglDataSize;
}

auto CHexDlgDataInterp::GetHWND()const->HWND
{
	return m_Wnd;
}

void CHexDlgDataInterp::Initialize(IHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	if (pHexCtrl == nullptr)
		return;

	m_pHexCtrl = pHexCtrl;
}

bool CHexDlgDataInterp::HasHighlight()const
{
	return m_dwHglDataSize > 0;
}

bool CHexDlgDataInterp::PreTranslateMsg(MSG* pMsg)
{
	if (m_Wnd.IsNull())
		return false;

	//Check first if a message is for any of m_gridCtrl's child window (CMFCPropertyGridProperty),
	//to make sure we process only m_gridCtrl related messages.
	if (const auto hWndParent = ::GetParent(pMsg->hwnd); hWndParent == m_gridCtrl.m_hWnd) {
		if (m_gridCtrl.PreTranslateMessage(pMsg) != FALSE) { return true; }
	}

	return m_Wnd.IsDlgMessage(pMsg);
}

auto CHexDlgDataInterp::ProcessMsg(const MSG& msg)->INT_PTR
{
	switch (msg.message) {
	case WM_ACTIVATE: return OnActivate(msg);
	case WM_COMMAND: return OnCommand(msg);
	case WM_CLOSE: return OnClose();
	case WM_DESTROY: return OnDestroy();
	case WM_INITDIALOG: return OnInitDialog(msg);
	case WM_NOTIFY: return OnNotify(msg);
	case WM_SIZE: return OnSize(msg);
	default:
		return 0;
	}
}

void CHexDlgDataInterp::SetDlgProperties(std::uint64_t u64Flags)
{
	m_u64Flags = u64Flags;
}

void CHexDlgDataInterp::ShowWindow(int iCmdShow)
{
	if (!m_Wnd.IsWindow()) {
		CreateDlg();
	}

	m_Wnd.ShowWindow(iCmdShow);
	UpdateData();
}

void CHexDlgDataInterp::UpdateData()
{
	if (!m_Wnd.IsWindow() || !m_Wnd.IsWindowVisible()) {
		return;
	}

	using enum EDataSize;
	const auto ullOffset = m_ullOffset = m_pHexCtrl->GetCaretPos();
	const auto ullDataSize = m_pHexCtrl->GetDataSize();
	const auto fMutable = m_pHexCtrl->IsMutable();
	const auto pGridBin = GetGridData(EName::NAME_BINARY);
	const auto eSizeBin = pGridBin->eSize;

	SetGridRedraw(false);
	for (const auto& iter : m_vecGrid) {
		iter.pProp->AllowEdit(fMutable ? TRUE : FALSE);
	}

	const auto ui8Data = GetIHexTData<std::uint8_t>(*m_pHexCtrl, ullOffset);

	if (eSizeBin == SIZE_BYTE) {
		ShowValueBinary(ui8Data);
	}

	ShowValueInt8(ui8Data);
	ShowValueUInt8(ui8Data);

	//EDataSize::SIZE_WORD
	if (ullOffset + static_cast<std::uint8_t>(SIZE_WORD) > ullDataSize) {
		for (const auto& iter : m_vecGrid) {
			if (iter.eSize >= SIZE_WORD) {
				iter.pProp->SetValue(L"0");
				iter.pProp->Enable(FALSE);
			}
		}

		SetGridRedraw(true);
		return;
	}

	for (const auto& iter : m_vecGrid) {
		if (iter.eSize == SIZE_WORD) {
			iter.pProp->Enable(TRUE);
		}
	}

	auto ui16Data = GetIHexTData<std::uint16_t>(*m_pHexCtrl, ullOffset);
	if (IsBigEndian()) {
		ui16Data = ByteSwap(ui16Data);
	}

	if (eSizeBin == SIZE_WORD) {
		ShowValueBinary(ui16Data);
	}

	ShowValueInt16(ui16Data);
	ShowValueUInt16(ui16Data);

	//EDataSize::SIZE_DWORD
	if (ullOffset + static_cast<std::uint8_t>(SIZE_DWORD) > ullDataSize) {
		for (const auto& iter : m_vecGrid) {
			if (iter.eSize >= SIZE_DWORD) {
				iter.pProp->SetValue(L"0");
				iter.pProp->Enable(FALSE);
			}
		}

		SetGridRedraw(true);
		return;
	}

	for (const auto& iter : m_vecGrid) {
		if (iter.eSize == SIZE_DWORD) {
			iter.pProp->Enable(TRUE);
		}
	}

	auto ui32Data = GetIHexTData<std::uint32_t>(*m_pHexCtrl, ullOffset);
	if (IsBigEndian()) {
		ui32Data = ByteSwap(ui32Data);
	}

	if (eSizeBin == SIZE_DWORD) {
		ShowValueBinary(ui32Data);
	}

	ShowValueInt32(ui32Data);
	ShowValueUInt32(ui32Data);
	ShowValueFloat(ui32Data);
	ShowValueTime32(ui32Data);
	ShowValueMSDOSTIME(ui32Data);
	ShowValueMSDTTMTIME(ui32Data);

	//EDataSize::SIZE_QWORD
	if (ullOffset + static_cast<std::uint8_t>(SIZE_QWORD) > ullDataSize) {
		for (const auto& iter : m_vecGrid) {
			if (iter.eSize >= SIZE_QWORD) {
				iter.pProp->SetValue(L"0");
				iter.pProp->Enable(FALSE);
			}
		}

		SetGridRedraw(true);
		return;
	}

	for (const auto& iter : m_vecGrid) {
		if (iter.eSize == SIZE_QWORD) {
			iter.pProp->Enable(TRUE);
		}
	}

	auto ui64Data = GetIHexTData<std::uint64_t>(*m_pHexCtrl, ullOffset);
	if (IsBigEndian()) {
		ui64Data = ByteSwap(ui64Data);
	}

	if (eSizeBin == SIZE_QWORD) {
		ShowValueBinary(ui64Data);
	}

	ShowValueInt64(ui64Data);
	ShowValueUInt64(ui64Data);
	ShowValueDouble(ui64Data);
	ShowValueTime64(ui64Data);
	ShowValueFILETIME(ui64Data);
	ShowValueOLEDATETIME(ui64Data);
	ShowValueJAVATIME(ui64Data);

	//EDataSize::SIZE_DQWORD
	if (ullOffset + static_cast<std::uint8_t>(SIZE_DQWORD) > ullDataSize) {
		for (const auto& iter : m_vecGrid) {
			if (iter.eSize >= SIZE_DQWORD) {
				iter.pProp->SetValue(L"0");
				iter.pProp->Enable(FALSE);
			}
		}

		SetGridRedraw(true);
		return;
	}

	for (const auto& iter : m_vecGrid) {
		if (iter.eSize == SIZE_DQWORD) {
			iter.pProp->Enable(TRUE);
		}
	}

	//Getting DQWORD size of data in form of SYSTEMTIME.
	//GUID could be used instead, it doesn't matter, they are same size.
	const auto dblQWORD = GetIHexTData<SYSTEMTIME>(*m_pHexCtrl, ullOffset);

	//Note: big-endian swapping is INDIVIDUAL for every struct.
	ShowValueSYSTEMTIME(dblQWORD);
	ShowValueGUID(std::bit_cast<GUID>(dblQWORD));
	ShowValueGUIDTIME(std::bit_cast<GUID>(dblQWORD));
	SetGridRedraw(true);
}


//Private methods.

auto CHexDlgDataInterp::GetGridData(EName eName)const->const GRIDDATA*
{
	return &*std::find_if(m_vecGrid.cbegin(), m_vecGrid.cend(), [=](const GRIDDATA& refData) {
		return refData.eName == eName; });
}

auto CHexDlgDataInterp::GetGridData(EName eName)->GRIDDATA*
{
	return &*std::find_if(m_vecGrid.begin(), m_vecGrid.end(), [=](const GRIDDATA& refData) {
		return refData.eName == eName; });
}

bool CHexDlgDataInterp::IsBigEndian()const
{
	return m_WndBtnBE.IsChecked();
}

bool CHexDlgDataInterp::IsNoEsc()const
{
	return m_u64Flags & HEXCTRL_FLAG_DLG_NOESC;
}

bool CHexDlgDataInterp::IsShowAsHex()const
{
	return m_WndBtnHex.IsChecked();
}

auto CHexDlgDataInterp::OnActivate(const MSG& msg)->INT_PTR
{
	if (!m_pHexCtrl->IsCreated())
		return FALSE;

	const auto nState = LOWORD(msg.wParam);
	if (nState == WA_ACTIVE || nState == WA_CLICKACTIVE) {
		const auto [dwFormat, wchSepar] = m_pHexCtrl->GetDateInfo();
		m_dwDateFormat = dwFormat;
		m_wchDateSepar = wchSepar;
		const auto wstrTitle = L"Date/Time format is: " + GetDateFormatString(m_dwDateFormat, m_wchDateSepar);
		m_Wnd.SetWndText(wstrTitle); //Update dialog title to reflect current date format.

		if (m_pHexCtrl->IsDataSet()) {
			UpdateData();
		}
	}
	else {
		m_dwHglDataSize = 0; //Remove data highlighting when dialog window is inactive.
		RedrawHexCtrl();
	}

	return FALSE; //Default handler.
}

void CHexDlgDataInterp::OnCancel()
{
	if (IsNoEsc()) //Not closing Dialog on Escape key.
		return;

	ShowWindow(SW_HIDE);
}

void CHexDlgDataInterp::OnCheckHex()
{
	UpdateData();
}

void CHexDlgDataInterp::OnCheckBigEndian()
{
	UpdateData();
}

auto CHexDlgDataInterp::OnClose()->INT_PTR
{
	ShowWindow(SW_HIDE);
	return TRUE;
}

auto CHexDlgDataInterp::OnCommand(const MSG& msg)->INT_PTR
{
	const auto uCtrlID = LOWORD(msg.wParam);
	const auto uCode = HIWORD(msg.wParam);   //Control code, zero for menu.

	if (uCode != BN_CLICKED) { return FALSE; }
	switch (uCtrlID) {
	case IDOK: OnOK(); break;
	case IDCANCEL: OnCancel(); break;
	case IDC_HEXCTRL_DATAINTERP_CHK_HEX: OnCheckHex(); break;
	case IDC_HEXCTRL_DATAINTERP_CHK_BE: OnCheckBigEndian(); break;
	default:
		return FALSE;
	}
	return TRUE;
}

auto CHexDlgDataInterp::OnDestroy()->INT_PTR
{
	m_vecGrid.clear();
	m_u64Flags = { };
	m_pHexCtrl = nullptr;
	m_DynLayout.RemoveAll();

	return TRUE;
}

auto CHexDlgDataInterp::OnInitDialog(const MSG& msg)->INT_PTR
{
	m_Wnd.Attach(msg.hwnd);
	m_WndBtnHex.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_DATAINTERP_CHK_HEX));
	m_WndBtnBE.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_DATAINTERP_CHK_BE));
	m_WndBtnHex.SetCheck(true);
	m_WndBtnBE.SetCheck(false);
	m_gridCtrl.SubclassDlgItem(IDC_HEXCTRL_DATAINTERP_GRID, CWnd::FromHandle(m_Wnd));
	m_gridCtrl.SetVSDotNetLook();

	using enum EGroup; using enum EName; using enum EDataSize;
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Binary:", L"0"), GR_BINARY, NAME_BINARY, SIZE_BYTE);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Int8:", L"0"), GR_INTEGRAL, NAME_INT8, SIZE_BYTE);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Unsigned Int8:", L"0"), GR_INTEGRAL, NAME_UINT8, SIZE_BYTE);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Int16:", L"0"), GR_INTEGRAL, NAME_INT16, SIZE_WORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Unsigned Int16:", L"0"), GR_INTEGRAL, NAME_UINT16, SIZE_WORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Int32:", L"0"), GR_INTEGRAL, NAME_INT32, SIZE_DWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Unsigned Int32:", L"0"), GR_INTEGRAL, NAME_UINT32, SIZE_DWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Int64:", L"0"), GR_INTEGRAL, NAME_INT64, SIZE_QWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Unsigned Int64:", L"0"), GR_INTEGRAL, NAME_UINT64, SIZE_QWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"time32_t:", L"0"), GR_TIME, NAME_TIME32T, SIZE_DWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"time64_t:", L"0"), GR_TIME, NAME_TIME64T, SIZE_QWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"FILETIME:", L"0"), GR_TIME, NAME_FILETIME, SIZE_QWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"OLE time:", L"0"), GR_TIME, NAME_OLEDATETIME, SIZE_QWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Java time:", L"0"), GR_TIME, NAME_JAVATIME, SIZE_QWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"MS-DOS time:", L"0"), GR_TIME, NAME_MSDOSTIME, SIZE_QWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"MS-UDTTM time:", L"0"), GR_TIME, NAME_MSDTTMTIME, SIZE_DWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Windows SYSTEMTIME:", L"0"), GR_TIME, NAME_SYSTEMTIME, SIZE_DQWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Float:", L"0"), GR_FLOAT, NAME_FLOAT, SIZE_DWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Double:", L"0"), GR_FLOAT, NAME_DOUBLE, SIZE_QWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"GUID:", L"0"), GR_MISC, NAME_GUID, SIZE_DQWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"GUID v1 UTC time:", L"0"), GR_GUIDTIME, NAME_GUIDTIME, SIZE_DQWORD);

	m_gridCtrl.EnableHeaderCtrl(TRUE, L"Data type", L"Value");
	HDITEMW hdItemPropGrid { .mask = HDI_WIDTH, .cxy = 150 };
	m_gridCtrl.GetHeaderCtrl().SetItem(0, &hdItemPropGrid); //Property grid column size.

	//Binary group.
	const auto pBinarys = new CMFCPropertyGridProperty(L"Binary:");
	for (const auto& iter : m_vecGrid) {
		if (iter.eGroup == GR_BINARY) {
			pBinarys->AddSubItem(iter.pProp);
		}
	}
	m_gridCtrl.AddProperty(pBinarys);

	//Integrals group.
	const auto pIntegrals = new CMFCPropertyGridProperty(L"Integral types:");
	for (const auto& iter : m_vecGrid) {
		if (iter.eGroup == GR_INTEGRAL) {
			pIntegrals->AddSubItem(iter.pProp);
		}
	}
	m_gridCtrl.AddProperty(pIntegrals);

	//Floats group.
	const auto pFloats = new CMFCPropertyGridProperty(L"Floating-point types:");
	for (const auto& iter : m_vecGrid) {
		if (iter.eGroup == GR_FLOAT) {
			pFloats->AddSubItem(iter.pProp);
		}
	}
	m_gridCtrl.AddProperty(pFloats);

	//Time group.
	const auto pTime = new CMFCPropertyGridProperty(L"Time:");
	for (const auto& iter : m_vecGrid) {
		if (iter.eGroup == GR_TIME) {
			pTime->AddSubItem(iter.pProp);
		}
	}
	m_gridCtrl.AddProperty(pTime);

	//Miscellaneous group.
	const auto pMisc = new CMFCPropertyGridProperty(L"Misc:");
	for (const auto& iter : m_vecGrid) {
		if (iter.eGroup == GR_MISC) {
			pMisc->AddSubItem(iter.pProp);
			if (iter.eName == NAME_GUID) {
				auto pGUIDsub = new CMFCPropertyGridProperty(L"GUID Time (built in GUID):"); //GUID Time sub-group.
				for (const auto& itGUIDTime : m_vecGrid) {
					if (itGUIDTime.eGroup == GR_GUIDTIME) {
						pGUIDsub->AddSubItem(itGUIDTime.pProp);
					}
				}
				pMisc->AddSubItem(pGUIDsub);
			}
		}
	}
	m_gridCtrl.AddProperty(pMisc);

	m_DynLayout.SetHost(m_Wnd);
	m_DynLayout.AddItem(IDC_HEXCTRL_DATAINTERP_GRID, wnd::CDynLayout::MoveNone(), wnd::CDynLayout::SizeHorzAndVert(100, 100));
	m_DynLayout.AddItem(IDC_HEXCTRL_DATAINTERP_CHK_HEX, wnd::CDynLayout::MoveVert(100), wnd::CDynLayout::SizeNone());
	m_DynLayout.AddItem(IDC_HEXCTRL_DATAINTERP_CHK_BE, wnd::CDynLayout::MoveVert(100), wnd::CDynLayout::SizeNone());
	m_DynLayout.Enable(true);

	return TRUE;
}

auto CHexDlgDataInterp::OnNotify(const MSG& msg)->INT_PTR
{
	const auto pNMHDR = reinterpret_cast<NMHDR*>(msg.lParam);
	switch (pNMHDR->idFrom) {
	case IDC_HEXCTRL_DATAINTERP_GRID:
		switch (pNMHDR->code) {
		case PROPGRID_MSG_CHANGESELECTION: return OnNotifyPropSelected(pNMHDR);
		case PROPGRID_MSG_PROPDATACHANGED: return OnNotifyPropDataChanged(pNMHDR);
		default: break;
		}
	default: break;
	}

	return TRUE;
}

auto CHexDlgDataInterp::OnNotifyPropDataChanged(NMHDR* pNMHDR)->INT_PTR
{
	const auto pPropNew = reinterpret_cast<CMFCPropertyGridProperty*>(pNMHDR->hwndFrom);
	if (!m_pHexCtrl->IsCreated() || !m_pHexCtrl->IsDataSet() || !m_pHexCtrl->IsMutable() || pPropNew == nullptr)
		return TRUE;

	const auto itGrid = std::find_if(m_vecGrid.begin(), m_vecGrid.end(),
		[pPropNew](const GRIDDATA& refData) { return refData.pProp == pPropNew; });
	if (itGrid == m_vecGrid.end())
		return TRUE;

	const auto& refStrProp = static_cast<const CStringW&>(pPropNew->GetValue());
	const std::wstring_view wsv = refStrProp.GetString();

	using enum EName;
	bool fSuccess { false };
	switch (itGrid->eName) {
	case NAME_BINARY:
		fSuccess = SetDataBinary(wsv);
		break;
	case NAME_INT8:
		fSuccess = SetDataNUMBER<std::int8_t>(wsv);
		break;
	case NAME_UINT8:
		fSuccess = SetDataNUMBER<std::uint8_t>(wsv);
		break;
	case NAME_INT16:
		fSuccess = SetDataNUMBER<std::int16_t>(wsv);
		break;
	case NAME_UINT16:
		fSuccess = SetDataNUMBER<std::uint16_t>(wsv);
		break;
	case NAME_INT32:
		fSuccess = SetDataNUMBER<std::int32_t>(wsv);
		break;
	case NAME_UINT32:
		fSuccess = SetDataNUMBER<std::uint32_t>(wsv);
		break;
	case NAME_INT64:
		fSuccess = SetDataNUMBER<std::int64_t>(wsv);
		break;
	case NAME_UINT64:
		fSuccess = SetDataNUMBER<std::uint64_t>(wsv);
		break;
	case NAME_FLOAT:
		fSuccess = SetDataNUMBER<float>(wsv);
		break;
	case NAME_DOUBLE:
		fSuccess = SetDataNUMBER<double>(wsv);
		break;
	case NAME_TIME32T:
		fSuccess = SetDataTime32(wsv);
		break;
	case NAME_TIME64T:
		fSuccess = SetDataTime64(wsv);
		break;
	case NAME_FILETIME:
		fSuccess = SetDataFILETIME(wsv);
		break;
	case NAME_OLEDATETIME:
		fSuccess = SetDataOLEDATETIME(wsv);
		break;
	case NAME_JAVATIME:
		fSuccess = SetDataJAVATIME(wsv);
		break;
	case NAME_MSDOSTIME:
		fSuccess = SetDataMSDOSTIME(wsv);
		break;
	case NAME_MSDTTMTIME:
		fSuccess = SetDataMSDTTMTIME(wsv);
		break;
	case NAME_SYSTEMTIME:
		fSuccess = SetDataSYSTEMTIME(wsv);
		break;
	case NAME_GUIDTIME:
		fSuccess = SetDataGUIDTIME(wsv);
		break;
	case NAME_GUID:
		fSuccess = SetDataGUID(wsv);
		break;
	default:
		break;
	};

	if (!fSuccess) {
		::MessageBoxW(m_Wnd, L"Wrong number format or out of range.", L"Data error...", MB_ICONERROR);
	}
	else {
		RedrawHexCtrl();
	}

	UpdateData();

	return TRUE;
}

auto CHexDlgDataInterp::OnNotifyPropSelected(NMHDR* pNMHDR)->INT_PTR
{
	const auto pProp = reinterpret_cast<CMFCPropertyGridProperty*>(pNMHDR->hwndFrom);
	const auto itGrid = std::find_if(m_vecGrid.begin(), m_vecGrid.end(),
		[pProp](const GRIDDATA& refData) { return refData.pProp == pProp; });
	if (itGrid == m_vecGrid.end())
		return TRUE;

	const auto ui8DataSize = static_cast<std::uint8_t>(itGrid->eSize);
	using enum EName; using enum EDataSize;
	if (itGrid->eName != NAME_BINARY) {
		const auto pGridBin = GetGridData(NAME_BINARY);
		pGridBin->eSize = itGrid->eSize;
		const auto ullDataSize = m_pHexCtrl->GetDataSize();

		if (m_ullOffset + ui8DataSize > ullDataSize || ui8DataSize == 16) {
			pGridBin->pProp->SetValue(L"0");
			pGridBin->pProp->Enable(FALSE);
		}
		else {
			pGridBin->pProp->Enable(TRUE);
			switch (ui8DataSize) {
			case 1:
				ShowValueBinary(GetIHexTData<std::uint8_t>(*m_pHexCtrl, m_ullOffset));
				break;
			case 2:
				ShowValueBinary(GetIHexTData<std::uint16_t>(*m_pHexCtrl, m_ullOffset));
				break;
			case 4:
				ShowValueBinary(GetIHexTData<std::uint32_t>(*m_pHexCtrl, m_ullOffset));
				break;
			case 8:
				ShowValueBinary(GetIHexTData<std::uint64_t>(*m_pHexCtrl, m_ullOffset));
				break;
			default:
				break;
			};
		}
	}

	m_dwHglDataSize = ui8DataSize;
	RedrawHexCtrl();

	return TRUE;
}

void CHexDlgDataInterp::OnOK()
{
	//Just an empty handler to prevent Dialog closing on Enter key.
	//SetDefID() doesn't always work for no particular reason.
}

auto CHexDlgDataInterp::OnSize(const MSG& msg)->INT_PTR
{
	const auto iWidth = LOWORD(msg.lParam);
	const auto iHeight = HIWORD(msg.lParam);
	m_DynLayout.OnSize(iWidth, iHeight);
	return TRUE;
}

void CHexDlgDataInterp::RedrawHexCtrl()const
{
	if (m_pHexCtrl != nullptr && m_pHexCtrl->IsCreated()) {
		m_pHexCtrl->Redraw();
	}
}

bool CHexDlgDataInterp::SetDataBinary(std::wstring_view wsv)const
{
	const auto pGrid = GetGridData(EName::NAME_BINARY);
	const auto eSize = pGrid->eSize;
	auto wstr = std::wstring { wsv };
	std::erase(wstr, L' ');
	const auto uiSizeBits = static_cast<std::uint32_t>(static_cast<std::uint8_t>(eSize) * 8);

	if (wstr.size() != uiSizeBits || wstr.find_first_not_of(L"01") != std::wstring::npos)
		return false;

	using enum EDataSize;
	switch (eSize) {
	case SIZE_BYTE:
		if (const auto opt = stn::StrToUInt8(wstr, 2); opt) {
			SetTData(*opt);
			return true;
		}
		return false;
	case SIZE_WORD:
		if (const auto opt = stn::StrToUInt16(wstr, 2); opt) {
			SetTData(*opt);
			return true;
		}
		return false;
	case SIZE_DWORD:
		if (const auto opt = stn::StrToUInt32(wstr, 2); opt) {
			SetTData(*opt);
			return true;
		}
		return false;
	case SIZE_QWORD:
		if (const auto opt = stn::StrToUInt64(wstr, 2); opt) {
			SetTData(*opt);
			return true;
		}
		return false;
	default:
		return false;
	};
}

template<typename T> requires TSize1248<T>
bool CHexDlgDataInterp::SetDataNUMBER(std::wstring_view wsv)const
{
	if (IsShowAsHex()) {
		//Unsigned type in case of float or double.
		using UTF = std::conditional_t<std::is_same_v<T, float>, std::uint32_t, std::uint64_t>;
		//Unsigned type in case of Integral or float/double.
		using UT = typename std::conditional_t<std::is_integral_v<T>, std::make_unsigned<T>, std::type_identity<UTF>>::type;
		if (const auto opt = stn::StrToNum<UT>(wsv); opt) {
			SetTData(*opt);
			return true;
		}
	}
	else {
		if (const auto opt = stn::StrToNum<T>(wsv); opt) {
			SetTData(*opt);
			return true;
		}
	}
	return false;
}

bool CHexDlgDataInterp::SetDataTime32(std::wstring_view wsv)const
{
	//The number of seconds since midnight January 1st 1970 UTC (32-bit). This wraps on 19 January 2038.
	const auto optSysTime = StringToSystemTime(wsv, m_dwDateFormat);
	if (!optSysTime)
		return false;

	//Unix times are signed but value before 1st January 1970 is not considered valid.
	//This is apparently because early compilers didn't support unsigned types. _mktime32() has the same limit.
	if (optSysTime->wYear < 1970)
		return false;

	FILETIME ftTime;
	if (!SystemTimeToFileTime(&*optSysTime, &ftTime))
		return false;

	//Convert ticks to seconds and adjust epoch.
	LARGE_INTEGER lTicks { .LowPart { ftTime.dwLowDateTime }, .HighPart { static_cast<LONG>(ftTime.dwHighDateTime) } };
	lTicks.QuadPart /= g_uFTTicksPerSec;
	lTicks.QuadPart -= g_ullUnixEpochDiff;
	if (lTicks.QuadPart >= LONG_MAX)
		return false;

	const auto lTime32 = static_cast<__time32_t>(lTicks.QuadPart);
	SetTData(lTime32);

	return true;
}

bool CHexDlgDataInterp::SetDataTime64(std::wstring_view wsv)const
{
	//The number of seconds since midnight January 1st 1970 UTC (32-bit). This wraps on 19 January 2038.
	const auto optSysTime = StringToSystemTime(wsv, m_dwDateFormat);
	if (!optSysTime)
		return false;

	//Unix times are signed but value before 1st January 1970 is not considered valid.
	//This is apparently because early compilers didn't support unsigned types. _mktime64() has the same limit.
	if (optSysTime->wYear < 1970)
		return false;

	FILETIME ftTime;
	if (!SystemTimeToFileTime(&*optSysTime, &ftTime))
		return false;

	//Convert ticks to seconds and adjust epoch.
	LARGE_INTEGER lTicks { .LowPart { ftTime.dwLowDateTime }, .HighPart { static_cast<LONG>(ftTime.dwHighDateTime) } };
	lTicks.QuadPart /= g_uFTTicksPerSec;
	lTicks.QuadPart -= g_ullUnixEpochDiff;
	SetTData(lTicks.QuadPart);

	return true;
}

bool CHexDlgDataInterp::SetDataFILETIME(std::wstring_view wsv)const
{
	const auto optFileTime = StringToFileTime(wsv, m_dwDateFormat);
	if (!optFileTime)
		return false;

	const ULARGE_INTEGER uliTime { .LowPart { optFileTime->dwLowDateTime }, .HighPart { optFileTime->dwHighDateTime } };
	SetTData(uliTime.QuadPart);

	return true;
}

bool CHexDlgDataInterp::SetDataOLEDATETIME(std::wstring_view wsv)const
{
	const auto optSysTime = StringToSystemTime(wsv, m_dwDateFormat);
	if (!optSysTime)
		return false;

	const COleDateTime dt(*optSysTime);
	if (dt.GetStatus() != COleDateTime::valid)
		return false;

	SetTData(dt.m_dt);

	return true;
}

bool CHexDlgDataInterp::SetDataJAVATIME(std::wstring_view wsv)const
{
	const auto optFileTime = StringToFileTime(wsv, m_dwDateFormat);
	if (!optFileTime)
		return false;

	//Number of milliseconds after/before January 1, 1970, 00:00:00 UTC.
	const LARGE_INTEGER lJavaTicks { .LowPart { optFileTime->dwLowDateTime },
		.HighPart { static_cast<LONG>(optFileTime->dwHighDateTime) } };
	const LARGE_INTEGER lEpochTicks { .LowPart { g_ulFileTime1970_LOW }, .HighPart { g_ulFileTime1970_HIGH } };
	const LONGLONG llDiffTicks { lEpochTicks.QuadPart > lJavaTicks.QuadPart ?
		-(lEpochTicks.QuadPart - lJavaTicks.QuadPart) : lJavaTicks.QuadPart - lEpochTicks.QuadPart };
	const auto llDiffMillis = llDiffTicks / g_uFTTicksPerMS;
	SetTData(llDiffMillis);

	return true;
}

bool CHexDlgDataInterp::SetDataMSDOSTIME(std::wstring_view wsv)const
{
	const auto optFileTime = StringToFileTime(wsv, m_dwDateFormat);
	if (!optFileTime)
		return false;

	UMSDOSDateTime msdosDateTime;
	if (!FileTimeToDosDateTime(&*optFileTime, &msdosDateTime.TimeDate.wDate, &msdosDateTime.TimeDate.wTime))
		return false;

	//Note: Big-endian is not currently supported. This has never existed in the "wild".
	SetIHexTData(*m_pHexCtrl, m_ullOffset, msdosDateTime.dwTimeDate);

	return true;
}

bool CHexDlgDataInterp::SetDataMSDTTMTIME(std::wstring_view wsv)const
{
	const auto optSysTime = StringToSystemTime(wsv, m_dwDateFormat);
	if (!optSysTime)
		return false;

	//Microsoft UDTTM time (as used by Microsoft Compound Document format).
	const UDTTM dttm { .components { .minute { optSysTime->wMinute }, .hour { optSysTime->wHour },
		.dayofmonth { optSysTime->wDay }, .month { optSysTime->wMonth }, .year { optSysTime->wYear - 1900UL },
		.weekday { optSysTime->wDayOfWeek } } };

	//Note: Big-endian is not currently supported. This has never existed in the "wild".
	SetIHexTData(*m_pHexCtrl, m_ullOffset, dttm.dwValue);

	return true;
}

bool CHexDlgDataInterp::SetDataSYSTEMTIME(std::wstring_view wsv)const
{
	const auto optSysTime = StringToSystemTime(wsv, m_dwDateFormat);
	if (!optSysTime)
		return false;

	auto stSTime = *optSysTime;
	if (IsBigEndian()) {
		stSTime.wYear = ByteSwap(stSTime.wYear);
		stSTime.wMonth = ByteSwap(stSTime.wMonth);
		stSTime.wDayOfWeek = ByteSwap(stSTime.wDayOfWeek);
		stSTime.wDay = ByteSwap(stSTime.wDay);
		stSTime.wHour = ByteSwap(stSTime.wHour);
		stSTime.wMinute = ByteSwap(stSTime.wMinute);
		stSTime.wSecond = ByteSwap(stSTime.wSecond);
		stSTime.wMilliseconds = ByteSwap(stSTime.wMilliseconds);
	}
	SetIHexTData(*m_pHexCtrl, m_ullOffset, stSTime);

	return true;
}

bool CHexDlgDataInterp::SetDataGUID(std::wstring_view wsv)const
{
	GUID stGUID;
	if (IIDFromString(wsv.data(), &stGUID) != S_OK)
		return false;

	if (IsBigEndian()) {
		stGUID.Data1 = ByteSwap(stGUID.Data1);
		stGUID.Data2 = ByteSwap(stGUID.Data2);
		stGUID.Data3 = ByteSwap(stGUID.Data3);
	}
	SetIHexTData(*m_pHexCtrl, m_ullOffset, stGUID);

	return true;
}

bool CHexDlgDataInterp::SetDataGUIDTIME(std::wstring_view wsv)const
{
	//This time is within NAME_GUID structure, and it depends on it.
	//We can not just set a NAME_GUIDTIME for data range if it's not 
	//a valid NAME_GUID range, so checking first.

	auto dqword = GetIHexTData<GUID>(*m_pHexCtrl, m_ullOffset);

	if (IsBigEndian()) { //Swap before any processing.
		dqword.Data1 = ByteSwap(dqword.Data1);
		dqword.Data2 = ByteSwap(dqword.Data2);
		dqword.Data3 = ByteSwap(dqword.Data3);
	}

	const unsigned short unGuidVersion = (dqword.Data3 & 0xF000UL) >> 12;
	if (unGuidVersion != 1)
		return false;

	//RFC4122: The timestamp is a 60-bit value. For UUID version 1, this is represented by Coordinated Universal Time (UTC)
	//as a count of 100-nanosecond intervals since 00:00:00.00, 15 October 1582 (the date of Gregorian reform to the Christian calendar).
	//Both FILETIME and GUID time are based upon 100ns intervals.
	//FILETIME is based upon 1 Jan 1601 whilst GUID time is from 1582. Add 6653 days to convert to GUID time.
	//NB: 6653 days from 15 Oct 1582 to 1 Jan 1601.
	const auto optFTime = StringToFileTime(wsv, m_dwDateFormat);
	if (!optFTime)
		return false;

	const auto ftTime = *optFTime;
	LARGE_INTEGER qwGUIDTime { .LowPart { ftTime.dwLowDateTime }, .HighPart { static_cast<LONG>(ftTime.dwHighDateTime) } };
	const ULARGE_INTEGER ullAddTicks { .QuadPart = static_cast<QWORD>(g_uFTTicksPerSec) * static_cast<QWORD>(g_uSecondsPerHour)
		* static_cast<QWORD>(g_uHoursPerDay) * static_cast<QWORD>(g_uFileTime1582OffsetDays) };
	qwGUIDTime.QuadPart += ullAddTicks.QuadPart;

	//Encode version 1 GUID with time.
	dqword.Data1 = qwGUIDTime.LowPart;
	dqword.Data2 = qwGUIDTime.HighPart & 0xFFFFUL;
	dqword.Data3 = ((qwGUIDTime.HighPart >> 16) & 0x0FFFUL) | 0x1000UL; //Including Type 1 flag (0x1000).

	if (IsBigEndian()) { //After processing swap back.
		dqword.Data1 = ByteSwap(dqword.Data1);
		dqword.Data2 = ByteSwap(dqword.Data2);
		dqword.Data3 = ByteSwap(dqword.Data3);
	}
	SetIHexTData(*m_pHexCtrl, m_ullOffset, dqword);

	return true;
}

void CHexDlgDataInterp::SetGridRedraw(bool fRedraw)
{
	m_gridCtrl.SetRedraw(fRedraw);
	if (fRedraw) {
		m_gridCtrl.RedrawWindow();
	}
}

template<TSize1248 T>
void CHexDlgDataInterp::SetTData(T tData)const
{
	if (IsBigEndian()) {
		tData = ByteSwap(tData);
	}

	SetIHexTData(*m_pHexCtrl, m_ullOffset, tData);
}

template<TSize1248 T>
void CHexDlgDataInterp::ShowValueBinary(T tData)const
{
	const auto pGrid = GetGridData(EName::NAME_BINARY);
	constexpr auto pSepar { L"  " };
	if constexpr (sizeof(T) == sizeof(std::uint8_t)) {
		pGrid->pProp->SetValue(std::format(L"{:08b}", tData).data());
	}
	if constexpr (sizeof(T) == sizeof(std::uint16_t)) {
		auto wstr = std::format(L"{:016b}", tData);
		wstr.insert(8, pSepar);
		pGrid->pProp->SetValue(wstr.data());
	}
	if constexpr (sizeof(T) == sizeof(std::uint32_t)) {
		auto wstr = std::format(L"{:032b}", tData);
		int iIndex = 0;
		for (auto i = 1; i < sizeof(std::uint32_t); ++i) {
			wstr.insert(i * 8 + iIndex++ * 2, pSepar);
		}
		pGrid->pProp->SetValue(wstr.data());
	}
	if constexpr (sizeof(T) == sizeof(std::uint64_t)) {
		auto wstr = std::format(L"{:064b}", tData);
		int iIndex = 0;
		for (auto i = 1; i < sizeof(std::uint64_t); ++i) {
			wstr.insert(i * 8 + iIndex++ * 2, pSepar);
		}
		pGrid->pProp->SetValue(wstr.data());
	}
}

void CHexDlgDataInterp::ShowValueInt8(BYTE byte)const
{
	const auto i8 = static_cast<std::int8_t>(byte);
	GetGridData(EName::NAME_INT8)->pProp->SetValue(std::vformat(IsShowAsHex() ? L"0x{0:02X}" : L"{1}",
		std::make_wformat_args(byte, i8)).data());
}

void CHexDlgDataInterp::ShowValueUInt8(BYTE byte)const
{
	GetGridData(EName::NAME_UINT8)->pProp->SetValue(std::vformat(IsShowAsHex() ? L"0x{:02X}" : L"{}",
		std::make_wformat_args(byte)).data());
}

void CHexDlgDataInterp::ShowValueInt16(WORD word)const
{
	const auto i16 = static_cast<std::int16_t>(word);
	GetGridData(EName::NAME_INT16)->pProp->SetValue(std::vformat(IsShowAsHex() ? L"0x{0:04X}" : L"{1}",
		std::make_wformat_args(word, i16)).data());
}

void CHexDlgDataInterp::ShowValueUInt16(WORD word)const
{
	GetGridData(EName::NAME_UINT16)->pProp->SetValue(std::vformat(IsShowAsHex() ? L"0x{:04X}" : L"{}",
		std::make_wformat_args(word)).data());
}

void CHexDlgDataInterp::ShowValueInt32(DWORD dword)const
{
	const auto i32 = static_cast<std::int32_t>(dword);
	GetGridData(EName::NAME_INT32)->pProp->SetValue(std::vformat(IsShowAsHex() ? L"0x{0:08X}" : L"{1}",
		std::make_wformat_args(dword, i32)).data());
}

void CHexDlgDataInterp::ShowValueUInt32(DWORD dword)const
{
	GetGridData(EName::NAME_UINT32)->pProp->SetValue(std::vformat(IsShowAsHex() ? L"0x{:08X}" : L"{}",
		std::make_wformat_args(dword)).data());
}

void CHexDlgDataInterp::ShowValueInt64(QWORD qword)const
{
	const auto i64 = static_cast<std::int64_t>(qword);
	GetGridData(EName::NAME_INT64)->pProp->SetValue(std::vformat(IsShowAsHex() ? L"0x{0:016X}" : L"{1}",
		std::make_wformat_args(qword, i64)).data());
}

void CHexDlgDataInterp::ShowValueUInt64(QWORD qword)const
{
	GetGridData(EName::NAME_UINT64)->pProp->SetValue(std::vformat(IsShowAsHex() ? L"0x{:016X}" : L"{}",
		std::make_wformat_args(qword)).data());
}

void CHexDlgDataInterp::ShowValueFloat(DWORD dword)const
{
	const auto fl = std::bit_cast<float>(dword);
	GetGridData(EName::NAME_FLOAT)->pProp->SetValue(std::vformat(IsShowAsHex() ? L"0x{0:08X}" : L"{1:.9e}",
		std::make_wformat_args(dword, fl)).data());
}

void CHexDlgDataInterp::ShowValueDouble(QWORD qword)const
{
	const auto dbl = std::bit_cast<double>(qword);
	GetGridData(EName::NAME_DOUBLE)->pProp->SetValue(std::vformat(IsShowAsHex() ? L"0x{0:016X}" : L"{1:.18e}",
		std::make_wformat_args(qword, dbl)).data());
}

void CHexDlgDataInterp::ShowValueTime32(DWORD dword)const
{
	std::wstring wstrTime = L"N/A";
	//The number of seconds since midnight January 1st 1970 UTC (32-bit). This is signed and wraps on 19 January 2038.
	const auto lTime32 = static_cast<__time32_t>(dword);

	//Unix times are signed and value before 1st January 1970 is not considered valid.
	//This is apparently because early compilers didn't support unsigned types. _mktime32() has the same limit.
	if (lTime32 >= 0) {
		//Add seconds from epoch time.
		LARGE_INTEGER Time { .LowPart { g_ulFileTime1970_LOW }, .HighPart { g_ulFileTime1970_HIGH } };
		Time.QuadPart += static_cast<LONGLONG>(lTime32) * g_uFTTicksPerSec;

		//Convert to FILETIME.
		const FILETIME ftTime { .dwLowDateTime { Time.LowPart }, .dwHighDateTime { static_cast<DWORD>(Time.HighPart) } };
		wstrTime = FileTimeToString(ftTime, m_dwDateFormat, m_wchDateSepar);
	}

	GetGridData(EName::NAME_TIME32T)->pProp->SetValue(wstrTime.data());
}

void CHexDlgDataInterp::ShowValueTime64(QWORD qword)const
{
	std::wstring wstrTime = L"N/A";
	const auto llTime64 = static_cast<__time64_t>(qword); //The number of seconds since midnight January 1st 1970 UTC (64-bit).

	//Unix times are signed and value before 1st January 1970 is not considered valid.
	//This is apparently because early compilers didn't support unsigned types. _mktime64() has the same limit.
	if (llTime64 >= 0) {
		//Add seconds from epoch time.
		LARGE_INTEGER Time { { .LowPart { g_ulFileTime1970_LOW }, .HighPart { g_ulFileTime1970_HIGH } } };
		Time.QuadPart += llTime64 * g_uFTTicksPerSec;

		//Convert to FILETIME.
		const FILETIME ftTime { .dwLowDateTime { Time.LowPart }, .dwHighDateTime { static_cast<DWORD>(Time.HighPart) } };
		wstrTime = FileTimeToString(ftTime, m_dwDateFormat, m_wchDateSepar);
	}

	GetGridData(EName::NAME_TIME64T)->pProp->SetValue(wstrTime.data());
}

void CHexDlgDataInterp::ShowValueFILETIME(QWORD qword)const
{
	GetGridData(EName::NAME_FILETIME)->pProp->SetValue(FileTimeToString(std::bit_cast<FILETIME>(qword),
		m_dwDateFormat, m_wchDateSepar).data());
}

void CHexDlgDataInterp::ShowValueOLEDATETIME(QWORD qword)const
{
	//OLE (including MS Office) date/time.
	//Implemented using an 8-byte floating-point number. 
	//Days are represented as whole number increments starting with 30 December 1899, midnight as time zero.
	//See: https://docs.microsoft.com/en-us/cpp/atl-mfc-shared/date-type?view=vs-2019

	std::wstring wstrTime = L"N/A";
	const COleDateTime dt(std::bit_cast<DATE>(qword));
	if (SYSTEMTIME stSysTime; dt.GetAsSystemTime(stSysTime)) {
		wstrTime = SystemTimeToString(stSysTime, m_dwDateFormat, m_wchDateSepar);
	}

	GetGridData(EName::NAME_OLEDATETIME)->pProp->SetValue(wstrTime.data());
}

void CHexDlgDataInterp::ShowValueJAVATIME(QWORD qword)const
{
	//Javatime (signed).
	//Number of milliseconds after/before January 1, 1970, 00:00:00 UTC.

	//Add/subtract milliseconds from epoch time.
	LARGE_INTEGER Time { { .LowPart { g_ulFileTime1970_LOW }, .HighPart { g_ulFileTime1970_HIGH } } };
	Time.QuadPart += qword * g_uFTTicksPerMS * (static_cast<LONGLONG>(qword) >= 0 ? 1 : -1);

	//Convert to FILETIME.
	const FILETIME ftJavaTime { .dwLowDateTime { Time.LowPart }, .dwHighDateTime { static_cast<DWORD>(Time.HighPart) } };
	GetGridData(EName::NAME_JAVATIME)->pProp->SetValue(FileTimeToString(ftJavaTime, m_dwDateFormat, m_wchDateSepar).data());
}

void CHexDlgDataInterp::ShowValueMSDOSTIME(DWORD dword)const
{
	std::wstring wstrTime = L"N/A";
	const UMSDOSDateTime msdosDateTime { .dwTimeDate { dword } };
	if (FILETIME ftMSDOS; DosDateTimeToFileTime(msdosDateTime.TimeDate.wDate, msdosDateTime.TimeDate.wTime, &ftMSDOS)) {
		wstrTime = FileTimeToString(ftMSDOS, m_dwDateFormat, m_wchDateSepar);
	}

	GetGridData(EName::NAME_MSDOSTIME)->pProp->SetValue(wstrTime.data());
}

void CHexDlgDataInterp::ShowValueMSDTTMTIME(DWORD dword)const
{
	//Microsoft UDTTM time (as used by Microsoft Compound Document format).
	std::wstring wstrTime = L"N/A";
	const UDTTM dttm { .dwValue { dword } };
	if (dttm.components.dayofmonth > 0 && dttm.components.dayofmonth < 32
		&& dttm.components.hour < 24 && dttm.components.minute < 60
		&& dttm.components.month>0 && dttm.components.month < 13 && dttm.components.weekday < 7) {
		const SYSTEMTIME stSysTime { .wYear { static_cast<WORD>(1900 + dttm.components.year) },
			.wMonth { static_cast<WORD>(dttm.components.month) }, .wDayOfWeek { static_cast<WORD>(dttm.components.weekday) },
			.wDay { static_cast<WORD>(dttm.components.dayofmonth) }, .wHour { static_cast<WORD>(dttm.components.hour) },
			.wMinute { static_cast<WORD>(dttm.components.minute) }, .wSecond { 0 }, .wMilliseconds { 0 } };

		wstrTime = SystemTimeToString(stSysTime, m_dwDateFormat, m_wchDateSepar);
	}

	GetGridData(EName::NAME_MSDTTMTIME)->pProp->SetValue(wstrTime.data());
}

void CHexDlgDataInterp::ShowValueSYSTEMTIME(SYSTEMTIME stSysTime)const
{
	if (IsBigEndian()) {
		stSysTime.wYear = ByteSwap(stSysTime.wYear);
		stSysTime.wMonth = ByteSwap(stSysTime.wMonth);
		stSysTime.wDayOfWeek = ByteSwap(stSysTime.wDayOfWeek);
		stSysTime.wDay = ByteSwap(stSysTime.wDay);
		stSysTime.wHour = ByteSwap(stSysTime.wHour);
		stSysTime.wMinute = ByteSwap(stSysTime.wMinute);
		stSysTime.wSecond = ByteSwap(stSysTime.wSecond);
		stSysTime.wMilliseconds = ByteSwap(stSysTime.wMilliseconds);
	}

	GetGridData(EName::NAME_SYSTEMTIME)->pProp->SetValue(SystemTimeToString(stSysTime, m_dwDateFormat, m_wchDateSepar).data());
}

void CHexDlgDataInterp::ShowValueGUID(GUID stGUID)const
{
	if (IsBigEndian()) {
		stGUID.Data1 = ByteSwap(stGUID.Data1);
		stGUID.Data2 = ByteSwap(stGUID.Data2);
		stGUID.Data3 = ByteSwap(stGUID.Data3);
	}

	const auto wstr = std::format(L"{{{:0>8x}-{:0>4x}-{:0>4x}-{:0>2x}{:0>2x}-{:0>2x}{:0>2x}{:0>2x}{:0>2x}{:0>2x}{:0>2x}}}",
		stGUID.Data1, stGUID.Data2, stGUID.Data3, stGUID.Data4[0],
		stGUID.Data4[1], stGUID.Data4[2], stGUID.Data4[3], stGUID.Data4[4],
		stGUID.Data4[5], stGUID.Data4[6], stGUID.Data4[7]);
	GetGridData(EName::NAME_GUID)->pProp->SetValue(wstr.data());
}

void CHexDlgDataInterp::ShowValueGUIDTIME(GUID stGUID)const
{
	if (IsBigEndian()) {
		stGUID.Data1 = ByteSwap(stGUID.Data1);
		stGUID.Data2 = ByteSwap(stGUID.Data2);
		stGUID.Data3 = ByteSwap(stGUID.Data3);
	}

	//Guid v1 Datetime UTC. The time structure within the NAME_GUID.
	//First, verify GUID is actually version 1 style.
	std::wstring wstrTime = L"N/A";
	const unsigned short unGuidVersion = (stGUID.Data3 & 0xF000UL) >> 12;
	if (unGuidVersion == 1) {
		LARGE_INTEGER qwGUIDTime { .LowPart { stGUID.Data1 }, .HighPart { stGUID.Data3 & 0x0FFFUL } };
		qwGUIDTime.HighPart = (qwGUIDTime.HighPart << 16) | stGUID.Data2;

		//RFC4122: The timestamp is a 60-bit value. For UUID version 1, this is represented by Coordinated Universal Time (UTC)
		//as a count of 100-nanosecond intervals since 00:00:00.00, 15 October 1582 (the date of Gregorian reform to the Christian calendar).
		//Both FILETIME and GUID time are based upon 100ns intervals.
		//FILETIME is based upon 1 Jan 1601 whilst GUID time is from 1582. Subtract 6653 days to convert from GUID time.
		//NB: 6653 days from 15 Oct 1582 to 1 Jan 1601.
		const ULARGE_INTEGER ullSubtractTicks { .QuadPart = static_cast<QWORD>(g_uFTTicksPerSec) * static_cast<QWORD>(g_uSecondsPerHour)
			* static_cast<QWORD>(g_uHoursPerDay) * static_cast<QWORD>(g_uFileTime1582OffsetDays) };
		qwGUIDTime.QuadPart -= ullSubtractTicks.QuadPart;

		//Convert to SYSTEMTIME.
		const FILETIME ftGUIDTime { .dwLowDateTime { qwGUIDTime.LowPart }, .dwHighDateTime { static_cast<DWORD>(qwGUIDTime.HighPart) } };
		wstrTime = FileTimeToString(ftGUIDTime, m_dwDateFormat, m_wchDateSepar);
	}

	GetGridData(EName::NAME_GUIDTIME)->pProp->SetValue(wstrTime.data());
}