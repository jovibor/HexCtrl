/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "../../res/HexCtrlRes.h"
#include "../HexUtility.h"
#include "CHexDlgDataInterp.h"
#include <algorithm>
#include <bit>
#include <cassert>
#include <format>

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

BEGIN_MESSAGE_MAP(CHexDlgDataInterp, CDialogEx)
	ON_WM_ACTIVATE()
	ON_BN_CLICKED(IDC_HEXCTRL_DATAINTERP_RADIO_LE, &CHexDlgDataInterp::OnClickRadioBeLe)
	ON_BN_CLICKED(IDC_HEXCTRL_DATAINTERP_RADIO_BE, &CHexDlgDataInterp::OnClickRadioBeLe)
	ON_BN_CLICKED(IDC_HEXCTRL_DATAINTERP_RADIO_DEC, &CHexDlgDataInterp::OnClickRadioHexDec)
	ON_BN_CLICKED(IDC_HEXCTRL_DATAINTERP_RADIO_HEX, &CHexDlgDataInterp::OnClickRadioHexDec)
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, &CHexDlgDataInterp::OnPropertyChanged)
	ON_WM_SHOWWINDOW()
	ON_WM_SIZE()
END_MESSAGE_MAP()

BOOL CHexDlgDataInterp::Create(UINT nIDTemplate, CWnd* pParent, IHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	if (pHexCtrl == nullptr)
		return FALSE;

	m_pHexCtrl = pHexCtrl;

	return CDialogEx::Create(nIDTemplate, pParent);
}

ULONGLONG CHexDlgDataInterp::GetDataSize()const
{
	return m_ullDataSize;
}

void CHexDlgDataInterp::InspectOffset(ULONGLONG ullOffset)
{
	const auto ullDataSize = m_pHexCtrl->GetDataSize();
	if (ullOffset >= ullDataSize) //Out of data bounds.
		return;

	const auto fMutable = m_pHexCtrl->IsMutable();
	for (const auto& iter : m_vecProp)
		iter.pProp->AllowEdit(fMutable ? TRUE : FALSE);

	m_ullOffset = ullOffset;
	const auto byte = GetIHexTData<BYTE>(*m_pHexCtrl, ullOffset);

	ShowValueBINARY(byte);
	ShowValueCHAR(byte);
	ShowValueUCHAR(byte);

	if (ullOffset + static_cast<unsigned>(ESize::SIZE_WORD) > ullDataSize)
	{
		for (const auto& iter : m_vecProp)
		{
			if (iter.eSize >= ESize::SIZE_WORD)
			{
				iter.pProp->SetValue(L"0");
				iter.pProp->Enable(FALSE);
			}
		}
		return;
	}

	//ESize::SIZE_WORD////////////////////////////////////////////
	for (const auto& iter : m_vecProp)
		if (iter.eSize == ESize::SIZE_WORD)
			iter.pProp->Enable(TRUE);

	auto word = GetIHexTData<WORD>(*m_pHexCtrl, ullOffset);
	if (m_fBigEndian)
		word = _byteswap_ushort(word);

	ShowValueSHORT(word);
	ShowValueUSHORT(word);

	if (ullOffset + static_cast<unsigned>(ESize::SIZE_DWORD) > ullDataSize)
	{
		for (const auto& iter : m_vecProp)
		{
			if (iter.eSize >= ESize::SIZE_DWORD)
			{
				iter.pProp->SetValue(L"0");
				iter.pProp->Enable(FALSE);
			}
		}
		return;
	}

	//ESize::SIZE_DWORD//////////////////////////////////////////////
	for (const auto& iter : m_vecProp)
		if (iter.eSize == ESize::SIZE_DWORD)
			iter.pProp->Enable(TRUE);

	auto dword = GetIHexTData<DWORD>(*m_pHexCtrl, ullOffset);
	if (m_fBigEndian)
		dword = _byteswap_ulong(dword);

	ShowValueLONG(dword);
	ShowValueULONG(dword);
	ShowValueFLOAT(dword);
	ShowValueTIME32(dword);
	ShowValueMSDOSTIME(dword);
	ShowValueMSDTTMTIME(dword);

	if (ullOffset + static_cast<unsigned>(ESize::SIZE_QWORD) > ullDataSize)
	{
		for (const auto& iter : m_vecProp)
		{
			if (iter.eSize >= ESize::SIZE_QWORD)
			{
				iter.pProp->SetValue(L"0");
				iter.pProp->Enable(FALSE);
			}
		}
		return;
	}

	//ESize::SIZE_QWORD//////////////////////////////////////////////////
	for (const auto& iter : m_vecProp)
		if (iter.eSize == ESize::SIZE_QWORD)
			iter.pProp->Enable(TRUE);

	auto qword = GetIHexTData<QWORD>(*m_pHexCtrl, ullOffset);
	if (m_fBigEndian)
		qword = _byteswap_uint64(qword);

	ShowValueLONGLONG(qword);
	ShowValueULONGLONG(qword);
	ShowValueDOUBLE(qword);
	ShowValueTIME64(qword);
	ShowValueFILETIME(qword);
	ShowValueOLEDATETIME(qword);
	ShowValueJAVATIME(qword);

	if (ullOffset + static_cast<unsigned>(ESize::SIZE_DQWORD) > ullDataSize)
	{
		for (const auto& iter : m_vecProp)
		{
			if (iter.eSize >= ESize::SIZE_DQWORD)
			{
				iter.pProp->SetValue(L"0");
				iter.pProp->Enable(FALSE);
			}
		}
		return;
	}

	//ESize::SIZE_DQWORD//////////////////////////////////////////////////
	for (const auto& iter : m_vecProp)
		if (iter.eSize == ESize::SIZE_DQWORD)
			iter.pProp->Enable(TRUE);

	auto dqword = GetIHexTData<UDQWORD>(*m_pHexCtrl, ullOffset);
	if (m_fBigEndian)
	{
		//TODO: Test this thoroughly
		const auto tmp = dqword.Value.qwLow;
		dqword.Value.qwLow = _byteswap_uint64(dqword.Value.qwHigh);
		dqword.Value.qwHigh = _byteswap_uint64(tmp);
	}

	ShowValueGUID(dqword);
	ShowValueGUIDTIME(dqword);
	ShowValueSYSTEMTIME(dqword);
}

void CHexDlgDataInterp::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HEXCTRL_DATAINTERP_PROPDATA, m_stCtrlGrid);
}

BOOL CHexDlgDataInterp::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	if (auto pRadio = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_DATAINTERP_RADIO_LE)); pRadio)
		pRadio->SetCheck(BST_CHECKED);
	if (auto pRadio = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_DATAINTERP_RADIO_HEX)); pRadio)
		pRadio->SetCheck(BST_CHECKED);

	m_vecProp.emplace_back(new CMFCPropertyGridProperty(L"binary:", L"0"), EGroup::DIGITS, EName::NAME_BINARY, ESize::SIZE_BYTE);
	m_vecProp.emplace_back(new CMFCPropertyGridProperty(L"char:", L"0"), EGroup::DIGITS, EName::NAME_CHAR, ESize::SIZE_BYTE);
	m_vecProp.emplace_back(new CMFCPropertyGridProperty(L"unsigned char:", L"0"), EGroup::DIGITS, EName::NAME_UCHAR, ESize::SIZE_BYTE);
	m_vecProp.emplace_back(new CMFCPropertyGridProperty(L"short:", L"0"), EGroup::DIGITS, EName::NAME_SHORT, ESize::SIZE_WORD);
	m_vecProp.emplace_back(new CMFCPropertyGridProperty(L"unsigned short:", L"0"), EGroup::DIGITS, EName::NAME_USHORT, ESize::SIZE_WORD);
	m_vecProp.emplace_back(new CMFCPropertyGridProperty(L"long:", L"0"), EGroup::DIGITS, EName::NAME_LONG, ESize::SIZE_DWORD);
	m_vecProp.emplace_back(new CMFCPropertyGridProperty(L"unsigned long:", L"0"), EGroup::DIGITS, EName::NAME_ULONG, ESize::SIZE_DWORD);
	m_vecProp.emplace_back(new CMFCPropertyGridProperty(L"long long:", L"0"), EGroup::DIGITS, EName::NAME_LONGLONG, ESize::SIZE_QWORD);
	m_vecProp.emplace_back(new CMFCPropertyGridProperty(L"unsigned long long:", L"0"), EGroup::DIGITS, EName::NAME_ULONGLONG, ESize::SIZE_QWORD);
	m_vecProp.emplace_back(new CMFCPropertyGridProperty(L"time32_t:", L"0"), EGroup::TIME, EName::NAME_TIME32T, ESize::SIZE_DWORD);
	m_vecProp.emplace_back(new CMFCPropertyGridProperty(L"time64_t:", L"0"), EGroup::TIME, EName::NAME_TIME64T, ESize::SIZE_QWORD);
	m_vecProp.emplace_back(new CMFCPropertyGridProperty(L"FILETIME:", L"0"), EGroup::TIME, EName::NAME_FILETIME, ESize::SIZE_QWORD);
	m_vecProp.emplace_back(new CMFCPropertyGridProperty(L"OLE time:", L"0"), EGroup::TIME, EName::NAME_OLEDATETIME, ESize::SIZE_QWORD);
	m_vecProp.emplace_back(new CMFCPropertyGridProperty(L"Java time:", L"0"), EGroup::TIME, EName::NAME_JAVATIME, ESize::SIZE_QWORD);
	m_vecProp.emplace_back(new CMFCPropertyGridProperty(L"MS-DOS time:", L"0"), EGroup::TIME, EName::NAME_MSDOSTIME, ESize::SIZE_QWORD);
	m_vecProp.emplace_back(new CMFCPropertyGridProperty(L"MS-UDTTM time:", L"0"), EGroup::TIME, EName::NAME_MSDTTMTIME, ESize::SIZE_DWORD);
	m_vecProp.emplace_back(new CMFCPropertyGridProperty(L"Windows SYSTEMTIME:", L"0"), EGroup::TIME, EName::NAME_SYSTEMTIME, ESize::SIZE_DQWORD);
	m_vecProp.emplace_back(new CMFCPropertyGridProperty(L"float:", L"0"), EGroup::FLOAT, EName::NAME_FLOAT, ESize::SIZE_DWORD);
	m_vecProp.emplace_back(new CMFCPropertyGridProperty(L"double:", L"0"), EGroup::FLOAT, EName::NAME_DOUBLE, ESize::SIZE_QWORD);
	m_vecProp.emplace_back(new CMFCPropertyGridProperty(L"GUID:", L"0"), EGroup::MISC, EName::NAME_GUID, ESize::SIZE_DQWORD);
	m_vecProp.emplace_back(new CMFCPropertyGridProperty(L"GUID v1 UTC time:", L"0"), EGroup::TIME, EName::NAME_GUIDTIME, ESize::SIZE_DQWORD, true);

	m_stCtrlGrid.EnableHeaderCtrl(TRUE, L"Data type", L"Value");
	HDITEMW hdItemPropGrid { .mask = HDI_WIDTH, .cxy = 150 };
	m_stCtrlGrid.GetHeaderCtrl().SetItem(0, &hdItemPropGrid); //Property grid column size.

	//Digits group.
	auto pDigits = new CMFCPropertyGridProperty(L"Integral types:");
	for (const auto& iter : m_vecProp)
		if (iter.eGroup == EGroup::DIGITS && !iter.fChild)
			pDigits->AddSubItem(iter.pProp);
	m_stCtrlGrid.AddProperty(pDigits);

	//Floats group.
	auto pFloats = new CMFCPropertyGridProperty(L"Floating-point types:");
	for (const auto& iter : m_vecProp)
		if (iter.eGroup == EGroup::FLOAT && !iter.fChild)
			pFloats->AddSubItem(iter.pProp);
	m_stCtrlGrid.AddProperty(pFloats);

	//Time group.
	auto pTime = new CMFCPropertyGridProperty(L"Time:");
	for (const auto& iter : m_vecProp)
		if (iter.eGroup == EGroup::TIME && !iter.fChild)
			pTime->AddSubItem(iter.pProp);
	m_stCtrlGrid.AddProperty(pTime);

	//Miscellaneous group.
	auto pMisc = new CMFCPropertyGridProperty(L"Misc:");
	for (const auto& iter : m_vecProp)
		if (iter.eGroup == EGroup::MISC && !iter.fChild)
		{
			pMisc->AddSubItem(iter.pProp);
			if (iter.eName == EName::NAME_GUID)
			{
				//GUID Time sub-group.
				auto pGUIDsub = new CMFCPropertyGridProperty(L"GUID Time (built in GUID):");
				if (auto iterTime = std::find_if(m_vecProp.begin(), m_vecProp.end(),
					[](const SGRIDDATA& ref) {return ref.eName == EName::NAME_GUIDTIME; }); iterTime != m_vecProp.end())
					pGUIDsub->AddSubItem(iterTime->pProp);
				pMisc->AddSubItem(pGUIDsub);
			}
		}
	m_stCtrlGrid.AddProperty(pMisc);

	return TRUE;
}

void CHexDlgDataInterp::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if (nState == WA_INACTIVE)
	{
		m_ullDataSize = 0; //Remove Data Interpreter data highlighting when its window is inactive.
		RedrawHexCtrl();
	}
	else
	{
		if (m_pHexCtrl->IsCreated())
		{
			const auto [dwFormat, wchSepar] = m_pHexCtrl->GetDateInfo();
			m_dwDateFormat = dwFormat;
			m_wchDateSepar = wchSepar;

			const auto wstrTitle = L"Date/Time format is: " + GetDateFormatString(m_dwDateFormat, m_wchDateSepar);
			SetWindowTextW(wstrTitle.data()); //Update dialog title to reflect current date format.
			if (m_pHexCtrl->IsDataSet())
				InspectOffset(m_pHexCtrl->GetCaretPos());
		}
	}

	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
}

void CHexDlgDataInterp::OnOK()
{
	if (!m_pHexCtrl->IsCreated() || !m_pHexCtrl->IsDataSet() || !m_pHexCtrl->IsMutable() || !m_pPropChanged)
		return;

	const auto itGridData = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[this](const SGRIDDATA& refData) {return refData.pProp == m_pPropChanged; });
	if (itGridData == m_vecProp.end())
		return;

	std::wstring wstr = static_cast<CStringW>(m_pPropChanged->GetValue()).GetString();

	bool fSuccess { false };
	switch (itGridData->eName)
	{
	case EName::NAME_BINARY:
		fSuccess = SetDataBINARY(wstr);
		break;
	case EName::NAME_CHAR:
		fSuccess = SetDataCHAR(wstr);
		break;
	case EName::NAME_UCHAR:
		fSuccess = SetDataUCHAR(wstr);
		break;
	case EName::NAME_SHORT:
		fSuccess = SetDataSHORT(wstr);
		break;
	case EName::NAME_USHORT:
		fSuccess = SetDataUSHORT(wstr);
		break;
	case EName::NAME_LONG:
		fSuccess = SetDataLONG(wstr);
		break;
	case EName::NAME_ULONG:
		fSuccess = SetDataULONG(wstr);
		break;
	case EName::NAME_LONGLONG:
		fSuccess = SetDataLONGLONG(wstr);
		break;
	case EName::NAME_ULONGLONG:
		fSuccess = SetDataULONGLONG(wstr);
		break;
	case EName::NAME_FLOAT:
		fSuccess = SetDataFLOAT(wstr);
		break;
	case EName::NAME_DOUBLE:
		fSuccess = SetDataDOUBLE(wstr);
		break;
	case EName::NAME_TIME32T:
		fSuccess = SetDataTIME32T(wstr);
		break;
	case EName::NAME_TIME64T:
		fSuccess = SetDataTIME64T(wstr);
		break;
	case EName::NAME_FILETIME:
		fSuccess = SetDataFILETIME(wstr);
		break;
	case EName::NAME_OLEDATETIME:
		fSuccess = SetDataOLEDATETIME(wstr);
		break;
	case EName::NAME_JAVATIME:
		fSuccess = SetDataJAVATIME(wstr);
		break;
	case EName::NAME_MSDOSTIME:
		fSuccess = SetDataMSDOSTIME(wstr);
		break;
	case EName::NAME_MSDTTMTIME:
		fSuccess = SetDataMSDTTMTIME(wstr);
		break;
	case EName::NAME_SYSTEMTIME:
		fSuccess = SetDataSYSTEMTIME(wstr);
		break;
	case EName::NAME_GUIDTIME:
		fSuccess = SetDataGUIDTIME(wstr);
		break;
	case EName::NAME_GUID:
		fSuccess = SetDataGUID(wstr);
		break;
	};

	if (!fSuccess)
		MessageBoxW(L"Wrong number format or out of range.", L"Data error...", MB_ICONERROR);
	else
		RedrawHexCtrl();

	InspectOffset(m_ullOffset);
}

void CHexDlgDataInterp::OnClickRadioBeLe()
{
	m_fBigEndian = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_DATAINTERP_RADIO_BE))->GetCheck() == BST_CHECKED;
	InspectOffset(m_ullOffset);
}

void CHexDlgDataInterp::OnClickRadioHexDec()
{
	m_fShowAsHex = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_DATAINTERP_RADIO_HEX))->GetCheck() == BST_CHECKED;
	InspectOffset(m_ullOffset);
}

void CHexDlgDataInterp::OnClose()
{
	m_ullDataSize = 0;

	CDialogEx::OnClose();
}

void CHexDlgDataInterp::OnDestroy()
{
	CDialogEx::OnDestroy();

	m_stCtrlGrid.DestroyWindow();
	m_vecProp.clear();
}

BOOL CHexDlgDataInterp::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	if (wParam == IDC_HEXCTRL_DATAINTERP_PROPDATA)
	{
		const auto* const pHdr = reinterpret_cast<NMHDR*>(lParam);
		if (pHdr->code != MSG_PROPGRIDCTRL_SELCHANGED)
			return FALSE;

		if (auto pData = std::find_if(m_vecProp.begin(), m_vecProp.end(),
			[this](const SGRIDDATA& refData)
			{return refData.pProp == m_stCtrlGrid.GetCurrentProp(); }); pData != m_vecProp.end())
		{
			m_ullDataSize = static_cast<ULONGLONG>(pData->eSize);
			RedrawHexCtrl();
		}
	}

	return CDialogEx::OnNotify(wParam, lParam, pResult);
}

LRESULT CHexDlgDataInterp::OnPropertyChanged(WPARAM wParam, LPARAM lParam)
{
	if (wParam == IDC_HEXCTRL_DATAINTERP_PROPDATA)
	{
		m_pPropChanged = reinterpret_cast<CMFCPropertyGridProperty*>(lParam);
		OnOK();
	}

	return 0;
}

void CHexDlgDataInterp::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
}

void CHexDlgDataInterp::RedrawHexCtrl()const
{
	if (m_pHexCtrl != nullptr && m_pHexCtrl->IsCreated())
		m_pHexCtrl->Redraw();
}

template<typename T>
void CHexDlgDataInterp::SetTData(T tData)const
{
	if (m_fBigEndian)
	{
		if constexpr (sizeof(T) == sizeof(WORD))
			tData = static_cast<T>(_byteswap_ushort(static_cast<WORD>(tData)));
		else if constexpr (sizeof(T) == sizeof(DWORD))
			tData = static_cast<T>(_byteswap_ulong(static_cast<DWORD>(tData)));
		else if constexpr (sizeof(T) == sizeof(QWORD))
			tData = static_cast<T>(_byteswap_uint64(static_cast<QWORD>(tData)));
	}

	SetIHexTData(*m_pHexCtrl, m_ullOffset, tData);
}

bool CHexDlgDataInterp::SetDataBINARY(const std::wstring& wstr)const
{
	if (wstr.size() != 8 || wstr.find_first_not_of(L"01") != std::wstring_view::npos)
		return false;

	const auto optData = StringToNum<unsigned char>(wstr, 2);
	if (!optData)
		return false;

	SetTData(*optData);
	return true;
}

bool CHexDlgDataInterp::SetDataCHAR(const std::wstring& wstr)const
{
	const auto optData = StringToNum<char>(wstr);
	if (!optData)
		return false;

	SetTData(*optData);
	return true;
}

bool CHexDlgDataInterp::SetDataUCHAR(const std::wstring& wstr)const
{
	const auto optData = StringToNum<unsigned char>(wstr);
	if (!optData)
		return false;

	SetTData(*optData);
	return true;
}

bool CHexDlgDataInterp::SetDataSHORT(const std::wstring& wstr)const
{
	const auto optData = StringToNum<short>(wstr);
	if (!optData)
		return false;

	SetTData(*optData);
	return true;
}

bool CHexDlgDataInterp::SetDataUSHORT(const std::wstring& wstr)const
{
	const auto optData = StringToNum<unsigned short>(wstr);
	if (!optData)
		return false;

	SetTData(*optData);
	return true;
}

bool CHexDlgDataInterp::SetDataLONG(const std::wstring& wstr)const
{
	const auto optData = StringToNum<int>(wstr);
	if (!optData)
		return false;

	SetTData(*optData);
	return true;
}

bool CHexDlgDataInterp::SetDataULONG(const std::wstring& wstr)const
{
	const auto optData = StringToNum<unsigned int>(wstr);
	if (!optData)
		return false;

	SetTData(*optData);
	return true;
}

bool CHexDlgDataInterp::SetDataLONGLONG(const std::wstring& wstr)const
{
	const auto optData = StringToNum<long long>(wstr);
	if (!optData)
		return false;

	SetTData(*optData);
	return true;
}

bool CHexDlgDataInterp::SetDataULONGLONG(const std::wstring& wstr)const
{
	const auto optData = StringToNum<unsigned long long>(wstr);
	if (!optData)
		return false;

	SetTData(*optData);
	return true;
}

bool CHexDlgDataInterp::SetDataFLOAT(const std::wstring& wstr)const
{
	const auto optData = StringToNum<float>(wstr);
	if (!optData)
		return false;

	SetTData(*optData);
	return true;
}

bool CHexDlgDataInterp::SetDataDOUBLE(const std::wstring& wstr)const
{
	const auto optData = StringToNum<double>(wstr);
	if (!optData)
		return false;

	SetTData(*optData);
	return true;
}

bool CHexDlgDataInterp::SetDataTIME32T(std::wstring_view wstr)const
{
	//The number of seconds since midnight January 1st 1970 UTC (32-bit). This wraps on 19 January 2038 
	const auto optSysTime = StringToSystemTime(wstr, m_dwDateFormat);
	if (!optSysTime)
		return false;

	//Unix times are signed but value before 1st January 1970 is not considered valid
	//This is apparently because early complilers didn't support unsigned types. _mktime32() has the same limit
	if (optSysTime->wYear < 1970)
		return false;

	FILETIME ftTime;
	if (!SystemTimeToFileTime(&*optSysTime, &ftTime))
		return false;

	//Convert ticks to seconds and adjust epoch
	LARGE_INTEGER lTicks;
	lTicks.HighPart = ftTime.dwHighDateTime;
	lTicks.LowPart = ftTime.dwLowDateTime;
	lTicks.QuadPart /= m_uFTTicksPerSec;
	lTicks.QuadPart -= m_ullUnixEpochDiff;
	if (lTicks.QuadPart >= LONG_MAX)
		return false;

	const auto lTime32 = static_cast<__time32_t>(lTicks.QuadPart);
	SetTData(lTime32);

	return true;
}

bool CHexDlgDataInterp::SetDataTIME64T(std::wstring_view wstr)const
{
	//The number of seconds since midnight January 1st 1970 UTC (32-bit). This wraps on 19 January 2038 
	const auto optSysTime = StringToSystemTime(wstr, m_dwDateFormat);
	if (!optSysTime)
		return false;

	//Unix times are signed but value before 1st January 1970 is not considered valid
	//This is apparently because early complilers didn't support unsigned types. _mktime64() has the same limit
	if (optSysTime->wYear < 1970)
		return false;

	FILETIME ftTime;
	if (!SystemTimeToFileTime(&*optSysTime, &ftTime))
		return false;

	//Convert ticks to seconds and adjust epoch
	LARGE_INTEGER lTicks;
	lTicks.HighPart = ftTime.dwHighDateTime;
	lTicks.LowPart = ftTime.dwLowDateTime;
	lTicks.QuadPart /= m_uFTTicksPerSec;
	lTicks.QuadPart -= m_ullUnixEpochDiff;
	const auto llTime64 = static_cast<__time64_t>(lTicks.QuadPart);
	SetTData(llTime64);

	return true;
}

bool CHexDlgDataInterp::SetDataFILETIME(std::wstring_view wstr)const
{
	const auto optFileTime = StringToFileTime(wstr, m_dwDateFormat);
	if (!optFileTime)
		return false;

	ULARGE_INTEGER stLITime;
	stLITime.LowPart = optFileTime->dwLowDateTime;
	stLITime.HighPart = optFileTime->dwHighDateTime;
	SetTData(stLITime.QuadPart);

	return true;
}

bool CHexDlgDataInterp::SetDataOLEDATETIME(std::wstring_view wstr)const
{
	const auto optSysTime = StringToSystemTime(wstr, m_dwDateFormat);
	if (!optSysTime)
		return false;

	const COleDateTime dt(*optSysTime);
	if (dt.GetStatus() != COleDateTime::valid)
		return false;

	ULONGLONG ullValue;
	std::memcpy(&ullValue, &dt.m_dt, sizeof(dt.m_dt));
	SetTData(ullValue);

	return true;
}

bool CHexDlgDataInterp::SetDataJAVATIME(std::wstring_view wstr)const
{
	const auto optFileTime = StringToFileTime(wstr, m_dwDateFormat);
	if (!optFileTime)
		return false;

	//Number of milliseconds after/before January 1, 1970, 00:00:00 UTC
	LARGE_INTEGER lJavaTicks;
	lJavaTicks.HighPart = optFileTime->dwHighDateTime;
	lJavaTicks.LowPart = optFileTime->dwLowDateTime;

	LARGE_INTEGER lEpochTicks;
	lEpochTicks.HighPart = m_ulFileTime1970_HIGH;
	lEpochTicks.LowPart = m_ulFileTime1970_LOW;

	LONGLONG llDiffTicks;
	if (lEpochTicks.QuadPart > lJavaTicks.QuadPart)
		llDiffTicks = -(lEpochTicks.QuadPart - lJavaTicks.QuadPart);
	else
		llDiffTicks = lJavaTicks.QuadPart - lEpochTicks.QuadPart;

	const auto llDiffMillis = llDiffTicks / m_uFTTicksPerMS;
	SetTData(llDiffMillis);

	return true;
}

bool CHexDlgDataInterp::SetDataMSDOSTIME(std::wstring_view wstr)const
{
	const auto optFileTime = StringToFileTime(wstr, m_dwDateFormat);
	if (!optFileTime)
		return false;

	UMSDOSDATETIME msdosDateTime;
	if (!FileTimeToDosDateTime(&*optFileTime, &msdosDateTime.TimeDate.wDate, &msdosDateTime.TimeDate.wTime))
		return false;

	//Note: Big-endian is not currently supported. This has never existed in the "wild".
	SetIHexTData(*m_pHexCtrl, m_ullOffset, msdosDateTime.dwTimeDate);

	return true;
}

bool CHexDlgDataInterp::SetDataMSDTTMTIME(std::wstring_view wstr)const
{
	const auto optSysTime = StringToSystemTime(wstr, m_dwDateFormat);
	if (!optSysTime)
		return false;

	//Microsoft UDTTM time (as used by Microsoft Compound Document format)
	UDTTM dttm;
	dttm.components.year = optSysTime->wYear - 1900;
	dttm.components.month = optSysTime->wMonth;
	dttm.components.weekday = optSysTime->wDayOfWeek;
	dttm.components.dayofmonth = optSysTime->wDay;
	dttm.components.hour = optSysTime->wHour;
	dttm.components.minute = optSysTime->wMinute;

	//Note: Big-endian is not currently supported. This has never existed in the "wild".
	SetIHexTData(*m_pHexCtrl, m_ullOffset, dttm.dwValue);

	return true;
}

bool CHexDlgDataInterp::SetDataSYSTEMTIME(std::wstring_view wstr)const
{
	const auto optSysTime = StringToSystemTime(wstr, m_dwDateFormat);
	if (!optSysTime)
		return false;

	//Note: Big-endian is not currently supported. This has never existed in the "wild".
	SetIHexTData(*m_pHexCtrl, m_ullOffset, *optSysTime);

	return true;
}

bool CHexDlgDataInterp::SetDataGUIDTIME(std::wstring_view wstr)const
{
	//This time is within NAME_GUID structure, and it depends on it.
	//We can not just set a NAME_GUIDTIME for data range if it's not 
	//a valid NAME_GUID range, so checking first.

	auto dqword = GetIHexTData<UDQWORD>(*m_pHexCtrl, m_ullOffset);
	const unsigned short unGuidVersion = (dqword.gGUID.Data3 & 0xf000) >> 12;
	if (unGuidVersion != 1)
		return false;

	//RFC4122: The timestamp is a 60-bit value.  For UUID version 1, this is represented by Coordinated Universal Time (UTC) as a count of 100-
	//nanosecond intervals since 00:00:00.00, 15 October 1582 (the date of Gregorian reform to the Christian calendar).
	//
	//Both FILETIME and GUID time are based upon 100ns intervals
	//FILETIME is based upon 1 Jan 1601 whilst GUID time is from 1582. Add 6653 days to convert to GUID time
	const auto optFTime = StringToFileTime(wstr, m_dwDateFormat);
	if (!optFTime)
		return false;

	const auto ftTime = *optFTime;

	LARGE_INTEGER qwGUIDTime;
	qwGUIDTime.HighPart = ftTime.dwHighDateTime;
	qwGUIDTime.LowPart = ftTime.dwLowDateTime;

	ULARGE_INTEGER ullAddTicks;
	ullAddTicks.QuadPart = static_cast<QWORD>(m_uFTTicksPerSec) * static_cast<QWORD>(m_uSecondsPerHour)
		* static_cast<QWORD>(m_uHoursPerDay) * static_cast<QWORD>(m_uFileTime1582OffsetDays);
	qwGUIDTime.QuadPart += ullAddTicks.QuadPart;

	//Encode version 1 GUID with time
	dqword.gGUID.Data1 = qwGUIDTime.LowPart;
	dqword.gGUID.Data2 = qwGUIDTime.HighPart & 0xffff;
	dqword.gGUID.Data3 = ((qwGUIDTime.HighPart >> 16) & 0x0fff) | 0x1000; //Including Type 1 flag (0x1000)

	SetIHexTData(*m_pHexCtrl, m_ullOffset, dqword);

	return true;
}

bool CHexDlgDataInterp::SetDataGUID(const std::wstring& wstr)const
{
	GUID guid;
	if (IIDFromString(wstr.data(), &guid) != S_OK)
		return false;

	SetIHexTData(*m_pHexCtrl, m_ullOffset, guid);

	return true;
}

void CHexDlgDataInterp::ShowValueBINARY(BYTE byte)const
{
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_BINARY; }); iter != m_vecProp.end())
		iter->pProp->SetValue(std::format(L"{:08b}", byte).data());
}

void CHexDlgDataInterp::ShowValueCHAR(BYTE byte)const
{
	//TODO: Remove static_cast<int> when bug in <format> is resolved.
	//https://github.com/microsoft/STL/issues/2320
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_CHAR; }); iter != m_vecProp.end())
		iter->pProp->SetValue(std::vformat(m_fShowAsHex ? L"{:#04X}" : L"{:d}",
			std::make_wformat_args(static_cast<int>(static_cast<char>(byte)))).data());
}

void CHexDlgDataInterp::ShowValueUCHAR(BYTE byte)const
{
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_UCHAR; }); iter != m_vecProp.end())
		iter->pProp->SetValue(std::vformat(m_fShowAsHex ? L"{:#04X}" : L"{:d}", std::make_wformat_args(byte)).data());
}

void CHexDlgDataInterp::ShowValueSHORT(WORD word)const
{
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_SHORT; }); iter != m_vecProp.end())
		iter->pProp->SetValue(std::vformat(m_fShowAsHex ? L"{:#06X}" : L"{:d}", std::make_wformat_args(static_cast<short>(word))).data());
}

void CHexDlgDataInterp::ShowValueUSHORT(WORD word)const
{
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_USHORT; }); iter != m_vecProp.end())
		iter->pProp->SetValue(std::vformat(m_fShowAsHex ? L"{:#06X}" : L"{:d}", std::make_wformat_args(word)).data());
}

void CHexDlgDataInterp::ShowValueLONG(DWORD dword)const
{
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_LONG; }); iter != m_vecProp.end())
		iter->pProp->SetValue(std::vformat(m_fShowAsHex ? L"{:#010X}" : L"{:d}", std::make_wformat_args(static_cast<int>(dword))).data());
}

void CHexDlgDataInterp::ShowValueULONG(DWORD dword)const
{
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_ULONG; }); iter != m_vecProp.end())
		iter->pProp->SetValue(std::vformat(m_fShowAsHex ? L"{:#010X}" : L"{:d}", std::make_wformat_args(dword)).data());
}

void CHexDlgDataInterp::ShowValueFLOAT(DWORD dword)const
{
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_FLOAT; }); iter != m_vecProp.end())
		iter->pProp->SetValue(std::format(L"{:.9e}", std::bit_cast<float>(dword)).data());
}

void CHexDlgDataInterp::ShowValueTIME32(DWORD dword)const
{
	std::wstring wstrTime = L"N/A";

	//The number of seconds since midnight January 1st 1970 UTC (32-bit). This is signed and wraps on 19 January 2038 
	const auto lTime32 = static_cast<__time32_t>(dword);

	//Unix times are signed and value before 1st January 1970 is not considered valid
	//This is apparently because early complilers didn't support unsigned types. _mktime32() has the same limit
	if (lTime32 >= 0)
	{
		//Add seconds from epoch time
		LARGE_INTEGER Time;
		Time.HighPart = m_ulFileTime1970_HIGH;
		Time.LowPart = m_ulFileTime1970_LOW;
		Time.QuadPart += static_cast<LONGLONG>(lTime32) * m_uFTTicksPerSec;

		//Convert to FILETIME
		FILETIME ftTime;
		ftTime.dwHighDateTime = Time.HighPart;
		ftTime.dwLowDateTime = Time.LowPart;

		wstrTime = FileTimeToString(ftTime, m_dwDateFormat, m_wchDateSepar);
	}

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_TIME32T; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());
}

void CHexDlgDataInterp::ShowValueMSDOSTIME(DWORD dword)const
{
	std::wstring wstrTime = L"N/A";
	FILETIME ftMSDOS;
	UMSDOSDATETIME msdosDateTime;
	msdosDateTime.dwTimeDate = dword;
	if (DosDateTimeToFileTime(msdosDateTime.TimeDate.wDate, msdosDateTime.TimeDate.wTime, &ftMSDOS))
	{
		wstrTime = FileTimeToString(ftMSDOS, m_dwDateFormat, m_wchDateSepar);
	}

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_MSDOSTIME; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());
}

void CHexDlgDataInterp::ShowValueMSDTTMTIME(DWORD dword)const
{
	//Microsoft UDTTM time (as used by Microsoft Compound Document format)
	std::wstring wstrTime = L"N/A";
	UDTTM dttm;
	dttm.dwValue = dword;
	if (dttm.components.dayofmonth > 0 && dttm.components.dayofmonth < 32
		&& dttm.components.hour < 24 && dttm.components.minute < 60
		&& dttm.components.month>0 && dttm.components.month < 13 && dttm.components.weekday < 7)
	{
		SYSTEMTIME stSysTime { };
		stSysTime.wYear = 1900 + static_cast<WORD>(dttm.components.year);
		stSysTime.wMonth = dttm.components.month;
		stSysTime.wDayOfWeek = dttm.components.weekday;
		stSysTime.wDay = dttm.components.dayofmonth;
		stSysTime.wHour = dttm.components.hour;
		stSysTime.wMinute = dttm.components.minute;
		stSysTime.wSecond = 0;
		stSysTime.wMilliseconds = 0;

		wstrTime = SystemTimeToString(stSysTime, m_dwDateFormat, m_wchDateSepar);
	}

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_MSDTTMTIME; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());
}

void CHexDlgDataInterp::ShowValueLONGLONG(QWORD qword)const
{
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_LONGLONG; }); iter != m_vecProp.end())
		iter->pProp->SetValue(std::vformat(m_fShowAsHex ? L"{:#018X}" : L"{:d}", std::make_wformat_args(static_cast<long long>(qword))).data());
}

void CHexDlgDataInterp::ShowValueULONGLONG(QWORD qword)const
{
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_ULONGLONG; }); iter != m_vecProp.end())
		iter->pProp->SetValue(std::vformat(m_fShowAsHex ? L"{:#018X}" : L"{:d}", std::make_wformat_args(qword)).data());
}

void CHexDlgDataInterp::ShowValueDOUBLE(QWORD qword)const
{
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_DOUBLE; }); iter != m_vecProp.end())
		iter->pProp->SetValue(std::format(L"{:.18e}", std::bit_cast<double>(qword)).data());
}

void CHexDlgDataInterp::ShowValueTIME64(QWORD qword)const
{
	std::wstring wstrTime = L"N/A";

	//The number of seconds since midnight January 1st 1970 UTC (64-bit). This is signed
	const auto llTime64 = static_cast<__time64_t>(qword);

	//Unix times are signed and value before 1st January 1970 is not considered valid
	//This is apparently because early complilers didn't support unsigned types. _mktime64() has the same limit
	if (llTime64 >= 0)
	{
		//Add seconds from epoch time
		LARGE_INTEGER Time;
		Time.HighPart = m_ulFileTime1970_HIGH;
		Time.LowPart = m_ulFileTime1970_LOW;
		Time.QuadPart += llTime64 * m_uFTTicksPerSec;

		//Convert to FILETIME
		FILETIME ftTime;
		ftTime.dwHighDateTime = Time.HighPart;
		ftTime.dwLowDateTime = Time.LowPart;

		wstrTime = FileTimeToString(ftTime, m_dwDateFormat, m_wchDateSepar);
	}

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_TIME64T; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());
}

void CHexDlgDataInterp::ShowValueFILETIME(QWORD qword)const
{
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_FILETIME; }); iter != m_vecProp.end())
		iter->pProp->SetValue(FileTimeToString(std::bit_cast<FILETIME>(qword), m_dwDateFormat, m_wchDateSepar).data());
}

void CHexDlgDataInterp::ShowValueOLEDATETIME(QWORD qword)const
{
	//OLE (including MS Office) date/time
	//Implemented using an 8-byte floating-point number. Days are represented as whole number increments starting with 30 December 1899, midnight as time zero.
	//See: https://docs.microsoft.com/en-us/cpp/atl-mfc-shared/date-type?view=vs-2019

	std::wstring wstrTime = L"N/A";

	DATE date;
	std::memcpy(&date, &qword, sizeof(date));
	const COleDateTime dt(date);

	SYSTEMTIME stSysTime { };
	if (dt.GetAsSystemTime(stSysTime))
		wstrTime = SystemTimeToString(stSysTime, m_dwDateFormat, m_wchDateSepar);

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_OLEDATETIME; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());
}

void CHexDlgDataInterp::ShowValueJAVATIME(QWORD qword)const
{
	//Javatime (signed)
	//Number of milliseconds after/before January 1, 1970, 00:00:00 UTC

	//Add/subtract milliseconds from epoch time
	LARGE_INTEGER Time;
	Time.HighPart = m_ulFileTime1970_HIGH;
	Time.LowPart = m_ulFileTime1970_LOW;
	if (static_cast<LONGLONG>(qword) >= 0)
		Time.QuadPart += qword * m_uFTTicksPerMS;
	else
		Time.QuadPart -= qword * m_uFTTicksPerMS;

	//Convert to FILETIME
	FILETIME ftJavaTime;
	ftJavaTime.dwHighDateTime = Time.HighPart;
	ftJavaTime.dwLowDateTime = Time.LowPart;

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_JAVATIME; }); iter != m_vecProp.end())
		iter->pProp->SetValue(FileTimeToString(ftJavaTime, m_dwDateFormat, m_wchDateSepar).data());
}

void CHexDlgDataInterp::ShowValueGUID(const UDQWORD& dqword)const
{
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_GUID; }); iter != m_vecProp.end())
	{
		const auto wstr = std::format(L"{{{:0>8x}-{:0>4x}-{:0>4x}-{:0>2x}{:0>2x}-{:0>2x}{:0>2x}{:0>2x}{:0>2x}{:0>2x}{:0>2x}}}",
			dqword.gGUID.Data1, dqword.gGUID.Data2, dqword.gGUID.Data3, dqword.gGUID.Data4[0],
			dqword.gGUID.Data4[1], dqword.gGUID.Data4[2], dqword.gGUID.Data4[3], dqword.gGUID.Data4[4],
			dqword.gGUID.Data4[5], dqword.gGUID.Data4[6], dqword.gGUID.Data4[7]);
		iter->pProp->SetValue(wstr.data());
	}
}

void CHexDlgDataInterp::ShowValueGUIDTIME(const UDQWORD& dqword)const
{
	//Guid v1 Datetime UTC
	//The time structure within the NAME_GUID.
	//First, verify GUID is actually version 1 style
	std::wstring wstrTime = L"N/A";
	const unsigned short unGuidVersion = (dqword.gGUID.Data3 & 0xf000) >> 12;

	if (unGuidVersion == 1)
	{
		LARGE_INTEGER qwGUIDTime;
		qwGUIDTime.LowPart = dqword.gGUID.Data1;
		qwGUIDTime.HighPart = dqword.gGUID.Data3 & 0x0fff;
		qwGUIDTime.HighPart = (qwGUIDTime.HighPart << 16) | dqword.gGUID.Data2;

		//RFC4122: The timestamp is a 60-bit value.  For UUID version 1, this is represented by Coordinated Universal Time (UTC) as a count of 100-
		//nanosecond intervals since 00:00:00.00, 15 October 1582 (the date of Gregorian reform to the Christian calendar).
		//
		//Both FILETIME and GUID time are based upon 100ns intervals
		//FILETIME is based upon 1 Jan 1601 whilst GUID time is from 1582. Subtract 6653 days to convert from GUID time
		//NB: 6653 days from 15 Oct 1582 to 1 Jan 1601
		//
		ULARGE_INTEGER ullSubtractTicks;
		ullSubtractTicks.QuadPart = static_cast<QWORD>(m_uFTTicksPerSec) * static_cast<QWORD>(m_uSecondsPerHour)
			* static_cast<QWORD>(m_uHoursPerDay) * static_cast<QWORD>(m_uFileTime1582OffsetDays);
		qwGUIDTime.QuadPart -= ullSubtractTicks.QuadPart;

		//Convert to SYSTEMTIME
		FILETIME ftGUIDTime;
		ftGUIDTime.dwHighDateTime = qwGUIDTime.HighPart;
		ftGUIDTime.dwLowDateTime = qwGUIDTime.LowPart;

		wstrTime = FileTimeToString(ftGUIDTime, m_dwDateFormat, m_wchDateSepar);
	}

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_GUIDTIME; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());
}

void CHexDlgDataInterp::ShowValueSYSTEMTIME(const UDQWORD& dqword)const
{
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_SYSTEMTIME; }); iter != m_vecProp.end())
		iter->pProp->SetValue(SystemTimeToString(std::bit_cast<SYSTEMTIME>(dqword), m_dwDateFormat, m_wchDateSepar).data());
}