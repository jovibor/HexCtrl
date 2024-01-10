/****************************************************************************************
* Copyright © 2018-2024 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "../../dep/StrToNum/StrToNum/StrToNum.h"
#include "../../res/HexCtrlRes.h"
#include "CHexDlgDataInterp.h"
#include <algorithm>
#include <bit>
#include <cassert>
#include <format>

import HEXCTRL.HexUtility;

using namespace HEXCTRL::INTERNAL;

namespace HEXCTRL::INTERNAL {
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
		GR_INTEGRAL, GR_FLOAT, GR_TIME, GR_MISC, GR_GUIDTIME
	};

	enum class CHexDlgDataInterp::EName : std::uint8_t {
		NAME_BINARY, NAME_CHAR, NAME_UCHAR, NAME_SHORT, NAME_USHORT,
		NAME_INT, NAME_UINT, NAME_LONGLONG, NAME_ULONGLONG,
		NAME_FLOAT, NAME_DOUBLE, NAME_TIME32T, NAME_TIME64T,
		NAME_FILETIME, NAME_OLEDATETIME, NAME_JAVATIME, NAME_MSDOSTIME,
		NAME_MSDTTMTIME, NAME_SYSTEMTIME, NAME_GUIDTIME, NAME_GUID
	};

	enum class CHexDlgDataInterp::ESize : std::uint8_t {
		SIZE_BYTE = 0x1, SIZE_WORD = 0x2, SIZE_DWORD = 0x4,
		SIZE_QWORD = 0x8, SIZE_DQWORD = 0x10
	};

	struct CHexDlgDataInterp::GRIDDATA {
		CMFCPropertyGridProperty* pProp { };
		EGroup eGroup { };
		EName eName { };
		ESize eSize { };
	};
}

BEGIN_MESSAGE_MAP(CHexPropGridCtrl, CMFCPropertyGridCtrl)
	ON_WM_SIZE()
END_MESSAGE_MAP()


BEGIN_MESSAGE_MAP(CHexDlgDataInterp, CDialogEx)
	ON_BN_CLICKED(IDC_HEXCTRL_DATAINTERP_CHK_HEX, &CHexDlgDataInterp::OnCheckHex)
	ON_BN_CLICKED(IDC_HEXCTRL_DATAINTERP_CHK_BE, &CHexDlgDataInterp::OnCheckBigEndian)
	ON_MESSAGE(WM_PROPGRID_PROPERTY_SELECTED, &CHexDlgDataInterp::OnPropertySelected)
	ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, &CHexDlgDataInterp::OnPropertyDataChanged)
	ON_WM_ACTIVATE()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()

CHexDlgDataInterp::CHexDlgDataInterp() = default;

CHexDlgDataInterp::~CHexDlgDataInterp() = default;

auto CHexDlgDataInterp::GetDataSize()const->ULONGLONG
{
	return m_ullDataSize;
}

auto CHexDlgDataInterp::GetDlgData()const->std::uint64_t
{
	if (!IsWindow(m_hWnd)) {
		return { };
	}

	std::uint64_t ullData { };

	if (IsShowAsHex()) {
		ullData |= HEXCTRL_FLAG_DATAINTERP_HEXNUM;
	}

	if (IsBigEndian()) {
		ullData |= HEXCTRL_FLAG_DATAINTERP_BE;
	}

	if (IsNoEsc()) {
		ullData |= HEXCTRL_FLAG_DATAINTERP_NOESC;
	}

	return ullData;
}

void CHexDlgDataInterp::Initialize(IHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	if (pHexCtrl == nullptr)
		return;

	m_pHexCtrl = pHexCtrl;
}

auto CHexDlgDataInterp::SetDlgData(std::uint64_t ullData)->HWND
{
	if (!IsWindow(m_hWnd)) {
		Create(IDD_HEXCTRL_DATAINTERP, CWnd::FromHandle(m_pHexCtrl->GetWndHandle(EHexWnd::WND_MAIN)));
	}

	if ((ullData & HEXCTRL_FLAG_DATAINTERP_HEXNUM) > 0 != IsShowAsHex()) {
		m_btnHex.SetCheck(!IsShowAsHex());
		OnCheckHex();
	}

	if ((ullData & HEXCTRL_FLAG_DATAINTERP_BE) > 0 != IsBigEndian()) {
		m_btnBE.SetCheck(!IsBigEndian());
		OnCheckBigEndian();
	}

	m_fNoEsc = ullData & HEXCTRL_FLAG_DATAINTERP_NOESC;

	return m_hWnd;
}

BOOL CHexDlgDataInterp::ShowWindow(int nCmdShow)
{
	if (!IsWindow(m_hWnd)) {
		Create(IDD_HEXCTRL_DATAINTERP, CWnd::FromHandle(m_pHexCtrl->GetWndHandle(EHexWnd::WND_MAIN)));
	}

	const auto ret = CDialogEx::ShowWindow(nCmdShow);
	UpdateData();

	return ret;
}

void CHexDlgDataInterp::UpdateData()
{
	if (!::IsWindowVisible(m_hWnd)) {
		return;
	}

	SetRedraw(false);

	const auto ullOffset = m_ullOffset = m_pHexCtrl->GetCaretPos();
	const auto ullDataSize = m_pHexCtrl->GetDataSize();
	const auto fMutable = m_pHexCtrl->IsMutable();

	for (const auto& iter : m_vecGrid) {
		iter.pProp->AllowEdit(fMutable ? TRUE : FALSE);
	}

	const auto byte = GetIHexTData<BYTE>(*m_pHexCtrl, ullOffset);
	ShowValueBinary(byte);
	ShowValueChar(byte);
	ShowValueUChar(byte);

	if (ullOffset + static_cast<unsigned>(ESize::SIZE_WORD) > ullDataSize) {
		for (const auto& iter : m_vecGrid) {
			if (iter.eSize >= ESize::SIZE_WORD) {
				iter.pProp->SetValue(L"0");
				iter.pProp->Enable(FALSE);
			}
		}
		return;
	}

	//ESize::SIZE_WORD
	for (const auto& iter : m_vecGrid) {
		if (iter.eSize == ESize::SIZE_WORD) {
			iter.pProp->Enable(TRUE);
		}
	}

	auto word = GetIHexTData<WORD>(*m_pHexCtrl, ullOffset);
	if (IsBigEndian()) {
		word = ByteSwap(word);
	}

	ShowValueShort(word);
	ShowValueUShort(word);

	if (ullOffset + static_cast<unsigned>(ESize::SIZE_DWORD) > ullDataSize) {
		for (const auto& iter : m_vecGrid) {
			if (iter.eSize >= ESize::SIZE_DWORD) {
				iter.pProp->SetValue(L"0");
				iter.pProp->Enable(FALSE);
			}
		}
		return;
	}

	//ESize::SIZE_DWORD
	for (const auto& iter : m_vecGrid) {
		if (iter.eSize == ESize::SIZE_DWORD) {
			iter.pProp->Enable(TRUE);
		}
	}

	auto dword = GetIHexTData<DWORD>(*m_pHexCtrl, ullOffset);
	if (IsBigEndian()) {
		dword = ByteSwap(dword);
	}

	ShowValueInt(dword);
	ShowValueUInt(dword);
	ShowValueFloat(dword);
	ShowValueTime32(dword);
	ShowValueMSDOSTIME(dword);
	ShowValueMSDTTMTIME(dword);

	if (ullOffset + static_cast<unsigned>(ESize::SIZE_QWORD) > ullDataSize) {
		for (const auto& iter : m_vecGrid) {
			if (iter.eSize >= ESize::SIZE_QWORD) {
				iter.pProp->SetValue(L"0");
				iter.pProp->Enable(FALSE);
			}
		}
		return;
	}

	//ESize::SIZE_QWORD
	for (const auto& iter : m_vecGrid) {
		if (iter.eSize == ESize::SIZE_QWORD) {
			iter.pProp->Enable(TRUE);
		}
	}

	auto qword = GetIHexTData<QWORD>(*m_pHexCtrl, ullOffset);
	if (IsBigEndian()) {
		qword = ByteSwap(qword);
	}

	ShowValueLL(qword);
	ShowValueULL(qword);
	ShowValueDouble(qword);
	ShowValueTime64(qword);
	ShowValueFILETIME(qword);
	ShowValueOLEDATETIME(qword);
	ShowValueJAVATIME(qword);

	if (ullOffset + static_cast<unsigned>(ESize::SIZE_DQWORD) > ullDataSize) {
		for (const auto& iter : m_vecGrid) {
			if (iter.eSize >= ESize::SIZE_DQWORD) {
				iter.pProp->SetValue(L"0");
				iter.pProp->Enable(FALSE);
			}
		}
		return;
	}

	//ESize::SIZE_DQWORD
	for (const auto& iter : m_vecGrid) {
		if (iter.eSize == ESize::SIZE_DQWORD) {
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
	SetRedraw(true);
}


//Private methods.

void CHexDlgDataInterp::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HEXCTRL_DATAINTERP_GRID, m_gridCtrl);
	DDX_Control(pDX, IDC_HEXCTRL_DATAINTERP_CHK_HEX, m_btnHex);
	DDX_Control(pDX, IDC_HEXCTRL_DATAINTERP_CHK_BE, m_btnBE);
}

auto CHexDlgDataInterp::GetGridData(EName eName)const->const GRIDDATA*
{
	return &*std::find_if(m_vecGrid.begin(), m_vecGrid.end(), [=](const GRIDDATA& refData) {
		return refData.eName == eName; });
}

bool CHexDlgDataInterp::IsBigEndian()const
{
	return m_btnBE.GetCheck() == BST_CHECKED;
}

bool CHexDlgDataInterp::IsNoEsc()const
{
	return m_fNoEsc;
}

bool CHexDlgDataInterp::IsShowAsHex()const
{
	return m_btnHex.GetCheck() == BST_CHECKED;
}

void CHexDlgDataInterp::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if (!m_pHexCtrl->IsCreated())
		return;

	if (nState == WA_ACTIVE || nState == WA_CLICKACTIVE) {
		const auto [dwFormat, wchSepar] = m_pHexCtrl->GetDateInfo();
		m_dwDateFormat = dwFormat;
		m_wchDateSepar = wchSepar;
		const auto wstrTitle = L"Date/Time format is: " + GetDateFormatString(m_dwDateFormat, m_wchDateSepar);
		SetWindowTextW(wstrTitle.data()); //Update dialog title to reflect current date format.
		if (m_pHexCtrl->IsDataSet()) {
			UpdateData();
		}
	}
	else {
		m_ullDataSize = 0; //Remove Data Interpreter data highlighting when its window is inactive.
		RedrawHexCtrl();
	}

	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
}

void CHexDlgDataInterp::OnCancel()
{
	if (IsNoEsc()) //Not closing Dialog on Escape key.
		return;

	CDialogEx::OnCancel();
}

void CHexDlgDataInterp::OnCheckHex()
{
	UpdateData();
}

void CHexDlgDataInterp::OnCheckBigEndian()
{
	UpdateData();
}

void CHexDlgDataInterp::OnClose()
{
	//Not calling base class CDialogEx::OnClose, to prevent calling OnCancel().
	EndDialog(IDCANCEL);
}

void CHexDlgDataInterp::OnDestroy()
{
	CDialogEx::OnDestroy();

	m_vecGrid.clear();
}

BOOL CHexDlgDataInterp::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_btnHex.SetCheck(BST_CHECKED);
	m_btnBE.SetCheck(BST_UNCHECKED);

	using enum EGroup; using enum EName; using enum ESize;
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"binary:", L"0"), GR_INTEGRAL, NAME_BINARY, SIZE_BYTE);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"char:", L"0"), GR_INTEGRAL, NAME_CHAR, SIZE_BYTE);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"unsigned char:", L"0"), GR_INTEGRAL, NAME_UCHAR, SIZE_BYTE);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"short:", L"0"), GR_INTEGRAL, NAME_SHORT, SIZE_WORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"unsigned short:", L"0"), GR_INTEGRAL, NAME_USHORT, SIZE_WORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"int:", L"0"), GR_INTEGRAL, NAME_INT, SIZE_DWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"unsigned int:", L"0"), GR_INTEGRAL, NAME_UINT, SIZE_DWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"long long:", L"0"), GR_INTEGRAL, NAME_LONGLONG, SIZE_QWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"unsigned long long:", L"0"), GR_INTEGRAL, NAME_ULONGLONG, SIZE_QWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"time32_t:", L"0"), GR_TIME, NAME_TIME32T, SIZE_DWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"time64_t:", L"0"), GR_TIME, NAME_TIME64T, SIZE_QWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"FILETIME:", L"0"), GR_TIME, NAME_FILETIME, SIZE_QWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"OLE time:", L"0"), GR_TIME, NAME_OLEDATETIME, SIZE_QWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Java time:", L"0"), GR_TIME, NAME_JAVATIME, SIZE_QWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"MS-DOS time:", L"0"), GR_TIME, NAME_MSDOSTIME, SIZE_QWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"MS-UDTTM time:", L"0"), GR_TIME, NAME_MSDTTMTIME, SIZE_DWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Windows SYSTEMTIME:", L"0"), GR_TIME, NAME_SYSTEMTIME, SIZE_DQWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"float:", L"0"), GR_FLOAT, NAME_FLOAT, SIZE_DWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"double:", L"0"), GR_FLOAT, NAME_DOUBLE, SIZE_QWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"GUID:", L"0"), GR_MISC, NAME_GUID, SIZE_DQWORD);
	m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"GUID v1 UTC time:", L"0"), GR_GUIDTIME, NAME_GUIDTIME, SIZE_DQWORD);

	m_gridCtrl.EnableHeaderCtrl(TRUE, L"Data type", L"Value");
	HDITEMW hdItemPropGrid { .mask = HDI_WIDTH, .cxy = 150 };
	m_gridCtrl.GetHeaderCtrl().SetItem(0, &hdItemPropGrid); //Property grid column size.

	//Digits group.
	const auto pDigits = new CMFCPropertyGridProperty(L"Integral types:");
	for (const auto& iter : m_vecGrid) {
		if (iter.eGroup == GR_INTEGRAL) {
			pDigits->AddSubItem(iter.pProp);
		}
	}
	m_gridCtrl.AddProperty(pDigits);

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

	if (const auto pDL = GetDynamicLayout(); pDL != nullptr) {
		pDL->SetMinSize({ 0, 0 });
	}

	return TRUE;
}

void CHexDlgDataInterp::OnOK()
{
	//Just an empty handler to prevent Dialog closing on Enter key.
	//SetDefID() doesn't always work for no particular reason.
}

auto CHexDlgDataInterp::OnPropertyDataChanged(WPARAM wParam, LPARAM lParam)->LRESULT
{
	if (wParam != IDC_HEXCTRL_DATAINTERP_GRID)
		return FALSE;

	const auto pPropNew = reinterpret_cast<CMFCPropertyGridProperty*>(lParam);
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
	case NAME_CHAR:
		fSuccess = SetDataChar(wsv);
		break;
	case NAME_UCHAR:
		fSuccess = SetDataUChar(wsv);
		break;
	case NAME_SHORT:
		fSuccess = SetDataShort(wsv);
		break;
	case NAME_USHORT:
		fSuccess = SetDataUShort(wsv);
		break;
	case NAME_INT:
		fSuccess = SetDataInt(wsv);
		break;
	case NAME_UINT:
		fSuccess = SetDataUInt(wsv);
		break;
	case NAME_LONGLONG:
		fSuccess = SetDataLL(wsv);
		break;
	case NAME_ULONGLONG:
		fSuccess = SetDataULL(wsv);
		break;
	case NAME_FLOAT:
		fSuccess = SetDataFloat(wsv);
		break;
	case NAME_DOUBLE:
		fSuccess = SetDataDouble(wsv);
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
	};

	if (!fSuccess) {
		MessageBoxW(L"Wrong number format or out of range.", L"Data error...", MB_ICONERROR);
	}
	else {
		RedrawHexCtrl();
	}

	UpdateData();

	return TRUE;
}

auto CHexDlgDataInterp::OnPropertySelected(WPARAM wParam, LPARAM lParam)->LRESULT
{
	if (wParam != IDC_HEXCTRL_DATAINTERP_GRID)
		return FALSE;

	const auto pPropSel = reinterpret_cast<CMFCPropertyGridProperty*>(lParam);
	const auto itGrid = std::find_if(m_vecGrid.begin(), m_vecGrid.end(),
		[pPropSel](const GRIDDATA& refData) { return refData.pProp == pPropSel; });
	if (itGrid == m_vecGrid.end())
		return TRUE;

	m_ullDataSize = static_cast<ULONGLONG>(itGrid->eSize);
	RedrawHexCtrl();

	return TRUE;
}

void CHexDlgDataInterp::RedrawHexCtrl()const
{
	if (m_pHexCtrl != nullptr && m_pHexCtrl->IsCreated()) {
		m_pHexCtrl->Redraw();
	}
}

template<typename T>
void CHexDlgDataInterp::SetTData(T tData)const
{
	if (IsBigEndian()) {
		tData = ByteSwap(tData);
	}

	SetIHexTData(*m_pHexCtrl, m_ullOffset, tData);
}

bool CHexDlgDataInterp::SetDataBinary(std::wstring_view wsv)const
{
	if (wsv.size() != 8 || wsv.find_first_not_of(L"01") != std::wstring_view::npos)
		return false;

	const auto opt = stn::StrToUChar(wsv, 2);
	if (!opt)
		return false;

	SetTData(*opt);
	return true;
}

bool CHexDlgDataInterp::SetDataChar(std::wstring_view wsv)const
{
	if (IsShowAsHex()) {
		if (const auto opt = stn::StrToUChar(wsv); opt) {
			SetTData(*opt);
			return true;
		}
	}
	else {
		if (const auto opt = stn::StrToChar(wsv); opt) {
			SetTData(*opt);
			return true;
		}
	}
	return false;
}

bool CHexDlgDataInterp::SetDataUChar(std::wstring_view wsv)const
{
	const auto opt = stn::StrToUChar(wsv);
	if (!opt)
		return false;

	SetTData(*opt);
	return true;
}

bool CHexDlgDataInterp::SetDataShort(std::wstring_view wsv)const
{
	if (IsShowAsHex()) {
		if (const auto opt = stn::StrToUShort(wsv); opt) {
			SetTData(*opt);
			return true;
		}
	}
	else {
		if (const auto opt = stn::StrToShort(wsv); opt) {
			SetTData(*opt);
			return true;
		}
	}
	return false;
}

bool CHexDlgDataInterp::SetDataUShort(std::wstring_view wsv)const
{
	const auto opt = stn::StrToUShort(wsv);
	if (!opt)
		return false;

	SetTData(*opt);
	return true;
}

bool CHexDlgDataInterp::SetDataInt(std::wstring_view wsv)const
{
	if (IsShowAsHex()) {
		if (const auto opt = stn::StrToUInt(wsv); opt) {
			SetTData(*opt);
			return true;
		}
	}
	else {
		if (const auto opt = stn::StrToNum<int>(wsv); opt) {
			SetTData(*opt);
			return true;
		}
	}
	return false;
}

bool CHexDlgDataInterp::SetDataUInt(std::wstring_view wsv)const
{
	const auto opt = stn::StrToUInt(wsv);
	if (!opt)
		return false;

	SetTData(*opt);
	return true;
}

bool CHexDlgDataInterp::SetDataLL(std::wstring_view wsv)const
{
	if (IsShowAsHex()) {
		if (const auto opt = stn::StrToULL(wsv); opt) {
			SetTData(*opt);
			return true;
		}
	}
	else {
		if (const auto opt = stn::StrToLL(wsv); opt) {
			SetTData(*opt);
			return true;
		}
	}
	return false;
}

bool CHexDlgDataInterp::SetDataULL(std::wstring_view wsv)const
{
	const auto opt = stn::StrToULL(wsv);
	if (!opt)
		return false;

	SetTData(*opt);
	return true;
}

bool CHexDlgDataInterp::SetDataFloat(std::wstring_view wsv)const
{
	if (IsShowAsHex()) {
		if (const auto opt = stn::StrToUInt(wsv); opt) {
			SetTData(*opt);
			return true;
		}
	}
	else {
		if (const auto opt = stn::StrToFloat(wsv); opt) {
			SetTData(*opt);
			return true;
		}
	}

	return false;
}

bool CHexDlgDataInterp::SetDataDouble(std::wstring_view wsv)const
{
	if (IsShowAsHex()) {
		if (const auto opt = stn::StrToULL(wsv); opt) {
			SetTData(*opt);
			return true;
		}
	}
	else {
		if (const auto opt = stn::StrToDouble(wsv); opt) {
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
	const UDTTM dttm { .components { .minute { optSysTime->wMinute }, .hour { optSysTime->wHour }, .dayofmonth { optSysTime->wDay },
		.month { optSysTime->wMonth }, .year { optSysTime->wYear - 1900U }, .weekday { optSysTime->wDayOfWeek } } };

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

	const unsigned short unGuidVersion = (dqword.Data3 & 0xf000) >> 12;
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
	dqword.Data2 = qwGUIDTime.HighPart & 0xffff;
	dqword.Data3 = ((qwGUIDTime.HighPart >> 16) & 0x0fff) | 0x1000; //Including Type 1 flag (0x1000).

	if (IsBigEndian()) { //After processing swap back.
		dqword.Data1 = ByteSwap(dqword.Data1);
		dqword.Data2 = ByteSwap(dqword.Data2);
		dqword.Data3 = ByteSwap(dqword.Data3);
	}
	SetIHexTData(*m_pHexCtrl, m_ullOffset, dqword);

	return true;
}

void CHexDlgDataInterp::SetRedraw(bool fRedraw)
{
	m_gridCtrl.SetRedraw(fRedraw);
	if (fRedraw) {
		m_gridCtrl.RedrawWindow();
	}
}

void CHexDlgDataInterp::ShowValueBinary(BYTE byte)const
{
	GetGridData(EName::NAME_BINARY)->pProp->SetValue(std::format(L"{:08b}", byte).data());
}

void CHexDlgDataInterp::ShowValueChar(BYTE byte)const
{
	GetGridData(EName::NAME_CHAR)->pProp->SetValue(std::vformat(IsShowAsHex() ? L"0x{0:02X}" : L"{1}",
		std::make_wformat_args(byte, static_cast<int>(static_cast<char>(byte)))).data());
}

void CHexDlgDataInterp::ShowValueUChar(BYTE byte)const
{
	GetGridData(EName::NAME_UCHAR)->pProp->SetValue(std::vformat(IsShowAsHex() ? L"0x{:02X}" : L"{}",
		std::make_wformat_args(byte)).data());
}

void CHexDlgDataInterp::ShowValueShort(WORD word)const
{
	GetGridData(EName::NAME_SHORT)->pProp->SetValue(std::vformat(IsShowAsHex() ? L"0x{0:04X}" : L"{1}",
		std::make_wformat_args(word, static_cast<short>(word))).data());
}

void CHexDlgDataInterp::ShowValueUShort(WORD word)const
{
	GetGridData(EName::NAME_USHORT)->pProp->SetValue(std::vformat(IsShowAsHex() ? L"0x{:04X}" : L"{}",
		std::make_wformat_args(word)).data());
}

void CHexDlgDataInterp::ShowValueInt(DWORD dword)const
{
	GetGridData(EName::NAME_INT)->pProp->SetValue(std::vformat(IsShowAsHex() ? L"0x{0:08X}" : L"{1}",
		std::make_wformat_args(dword, static_cast<int>(dword))).data());
}

void CHexDlgDataInterp::ShowValueUInt(DWORD dword)const
{
	GetGridData(EName::NAME_UINT)->pProp->SetValue(std::vformat(IsShowAsHex() ? L"0x{:08X}" : L"{}",
		std::make_wformat_args(dword)).data());
}

void CHexDlgDataInterp::ShowValueLL(QWORD qword)const
{
	GetGridData(EName::NAME_LONGLONG)->pProp->SetValue(std::vformat(IsShowAsHex() ? L"0x{0:016X}" : L"{1}",
		std::make_wformat_args(qword, static_cast<long long>(qword))).data());
}

void CHexDlgDataInterp::ShowValueULL(QWORD qword)const
{
	GetGridData(EName::NAME_ULONGLONG)->pProp->SetValue(std::vformat(IsShowAsHex() ? L"0x{:016X}" : L"{}",
		std::make_wformat_args(qword)).data());
}

void CHexDlgDataInterp::ShowValueFloat(DWORD dword)const
{
	GetGridData(EName::NAME_FLOAT)->pProp->SetValue(std::vformat(IsShowAsHex() ? L"0x{0:08X}" : L"{1:.9e}",
		std::make_wformat_args(dword, std::bit_cast<float>(dword))).data());
}

void CHexDlgDataInterp::ShowValueDouble(QWORD qword)const
{
	GetGridData(EName::NAME_DOUBLE)->pProp->SetValue(std::vformat(IsShowAsHex() ? L"0x{0:08X}" : L"{1:.18e}",
		std::make_wformat_args(qword, std::bit_cast<double>(qword))).data());
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
	const unsigned short unGuidVersion = (stGUID.Data3 & 0xf000) >> 12;
	if (unGuidVersion == 1) {
		LARGE_INTEGER qwGUIDTime { .LowPart { stGUID.Data1 }, .HighPart { stGUID.Data3 & 0x0fff } };
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