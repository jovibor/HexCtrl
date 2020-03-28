/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#include "stdafx.h"
#include "CHexDlgDataInterpret.h"
#include "../Helper.h"
#include <algorithm>
#include "strsafe.h"

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

//Time calculation constants
//
#define FTTICKSPERMS				10000			//Number of 100ns intervals in a milli-second
#define FTTICKSPERSECOND			10000000		//Number of 100ns intervals in a second
#define HOURSPERDAY					24				//24 hours per day
#define SECONDSPERHOUR				3600			//3600 seconds per hour
#define FILETIME1582OFFSETDAYS		6653			//FILETIME is based upon 1 Jan 1601 whilst GUID time is from 15 Oct 1582. Add 6653 days to convert to GUID time
#define FILETIME1970_LOW			0xd53e8000		//1st Jan 1970 as FILETIME
#define FILETIME1970_HIGH			0x019db1de		//Used for Unix and Java times

BEGIN_MESSAGE_MAP(CHexDlgDataInterpret, CDialogEx)
	ON_WM_CLOSE()
	ON_WM_ACTIVATE()
	ON_WM_SIZE()
	ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, &CHexDlgDataInterpret::OnPropertyChanged)
	ON_COMMAND(IDC_HEXCTRL_DATAINTERPRET_RADIO_LE, &CHexDlgDataInterpret::OnClickRadioLe)
	ON_COMMAND(IDC_HEXCTRL_DATAINTERPRET_RADIO_BE, &CHexDlgDataInterpret::OnClickRadioBe)
	ON_COMMAND(IDC_HEXCTRL_DATAINTERPRET_RADIO_DEC, &CHexDlgDataInterpret::OnClickRadioDec)
	ON_COMMAND(IDC_HEXCTRL_DATAINTERPRET_RADIO_HEX, &CHexDlgDataInterpret::OnClickRadioHex)
END_MESSAGE_MAP()

void CHexDlgDataInterpret::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HEXCTRL_DATAINTERPRET_PROPERTY_DATA, m_stCtrlGrid);
}

BOOL CHexDlgDataInterpret::Create(UINT nIDTemplate, CHexCtrl* pHexCtrl)
{
	if (!pHexCtrl)
		return FALSE;

	m_pHexCtrl = pHexCtrl;

	return CDialogEx::Create(nIDTemplate, pHexCtrl);
}

BOOL CHexDlgDataInterpret::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	//Determine locale specific date format. Default to UK/European if unable to determine
	//See: https://docs.microsoft.com/en-gb/windows/win32/intl/locale-idate
	int nResult = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IDATE | LOCALE_RETURN_NUMBER, (LPTSTR)&m_dwDateFormat, sizeof(m_dwDateFormat));
	if (!nResult)
		m_dwDateFormat = 1;

	//Determine 'short' date seperator character. Default to UK/European if unable to determine	
	nResult = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDATE, m_wszDateSeparator, _countof(m_wszDateSeparator));
	if (!nResult)
		_stprintf_s(m_wszDateSeparator, _countof(m_wszDateSeparator), L"/");

	if (auto pRadio = (CButton*)GetDlgItem(IDC_HEXCTRL_DATAINTERPRET_RADIO_LE); pRadio)
		pRadio->SetCheck(1);
	if (auto pRadio = (CButton*)GetDlgItem(IDC_HEXCTRL_DATAINTERPRET_RADIO_DEC); pRadio)
		pRadio->SetCheck(1);

	m_hdItemPropGrid.mask = HDI_WIDTH;
	m_hdItemPropGrid.cxy = 120;
	m_stCtrlGrid.EnableHeaderCtrl(TRUE, L"Data type", L"Value");
	m_stCtrlGrid.GetHeaderCtrl().SetItem(0, &m_hdItemPropGrid); //Property grid column size.

	m_vecProp.emplace_back(GRIDDATA { EGroup::DIGITS, EName::NAME_BINARY, ESize::SIZE_BYTE, new CMFCPropertyGridProperty(L"binary:", L"0") });
	m_vecProp.emplace_back(GRIDDATA { EGroup::DIGITS, EName::NAME_CHAR, ESize::SIZE_BYTE, new CMFCPropertyGridProperty(L"char:", L"0") });
	m_vecProp.emplace_back(GRIDDATA { EGroup::DIGITS, EName::NAME_UCHAR, ESize::SIZE_BYTE, new CMFCPropertyGridProperty(L"unsigned char:", L"0") });
	m_vecProp.emplace_back(GRIDDATA { EGroup::DIGITS, EName::NAME_SHORT, ESize::SIZE_WORD, new CMFCPropertyGridProperty(L"short:", L"0") });
	m_vecProp.emplace_back(GRIDDATA { EGroup::DIGITS, EName::NAME_USHORT, ESize::SIZE_WORD, new CMFCPropertyGridProperty(L"unsigned short:", L"0") });
	m_vecProp.emplace_back(GRIDDATA { EGroup::DIGITS, EName::NAME_LONG, ESize::SIZE_DWORD, new CMFCPropertyGridProperty(L"long:", L"0") });
	m_vecProp.emplace_back(GRIDDATA { EGroup::DIGITS, EName::NAME_ULONG, ESize::SIZE_DWORD, new CMFCPropertyGridProperty(L"unsigned long:", L"0") });
	m_vecProp.emplace_back(GRIDDATA { EGroup::DIGITS, EName::NAME_LONGLONG, ESize::SIZE_QWORD, new CMFCPropertyGridProperty(L"long long:", L"0") });
	m_vecProp.emplace_back(GRIDDATA { EGroup::DIGITS, EName::NAME_ULONGLONG, ESize::SIZE_QWORD, new CMFCPropertyGridProperty(L"unsigned long long:", L"0") });
	m_vecProp.emplace_back(GRIDDATA{ EGroup::DIGITS, EName::NAME_GUID, ESize::SIZE_DQWORD, new CMFCPropertyGridProperty(L"GUID:", L"0") });
	auto pDigits = new CMFCPropertyGridProperty(L"Digits:");
	for (auto& iter : m_vecProp)
		if (iter.eGroup == EGroup::DIGITS)
			pDigits->AddSubItem(iter.pProp);
	m_stCtrlGrid.AddProperty(pDigits);

	m_vecProp.emplace_back(GRIDDATA { EGroup::FLOAT, EName::NAME_FLOAT, ESize::SIZE_DWORD, new CMFCPropertyGridProperty(L"Float:", L"0") });
	m_vecProp.emplace_back(GRIDDATA { EGroup::FLOAT, EName::NAME_DOUBLE, ESize::SIZE_QWORD, new CMFCPropertyGridProperty(L"Double:", L"0") });
	auto pFloats = new CMFCPropertyGridProperty(L"Floats:");
	for (auto& iter : m_vecProp)
		if (iter.eGroup == EGroup::FLOAT)
			pFloats->AddSubItem(iter.pProp);
	m_stCtrlGrid.AddProperty(pFloats);

	m_vecProp.emplace_back(GRIDDATA { EGroup::TIME, EName::NAME_TIME32T, ESize::SIZE_DWORD, new CMFCPropertyGridProperty(L"time32_t:", L"0") });
	m_vecProp.emplace_back(GRIDDATA { EGroup::TIME, EName::NAME_TIME64T, ESize::SIZE_QWORD, new CMFCPropertyGridProperty(L"time64_t:", L"0") });
	m_vecProp.emplace_back(GRIDDATA { EGroup::TIME, EName::NAME_FILETIME, ESize::SIZE_QWORD, new CMFCPropertyGridProperty(L"FILETIME:", L"0") });
	m_vecProp.emplace_back(GRIDDATA{ EGroup::TIME, EName::NAME_JAVATIME, ESize::SIZE_QWORD, new CMFCPropertyGridProperty(L"Java time:", L"0") });
	m_vecProp.emplace_back(GRIDDATA{ EGroup::TIME, EName::NAME_MSDOSTIME, ESize::SIZE_QWORD, new CMFCPropertyGridProperty(L"MS-DOS time:", L"0") });
	m_vecProp.emplace_back(GRIDDATA{ EGroup::TIME, EName::NAME_SYSTEMTIME, ESize::SIZE_DQWORD, new CMFCPropertyGridProperty(L"Windows SYSTEMTIME:", L"0") });
	m_vecProp.emplace_back(GRIDDATA{ EGroup::TIME, EName::NAME_GUIDTIME, ESize::SIZE_DQWORD, new CMFCPropertyGridProperty(L"GUID v1 UTC time:", L"0") });
	auto pTime = new CMFCPropertyGridProperty(L"Time:");
	for (auto& iter : m_vecProp)
		if (iter.eGroup == EGroup::TIME)
			pTime->AddSubItem(iter.pProp);
	m_stCtrlGrid.AddProperty(pTime);

	return TRUE;
}

void CHexDlgDataInterpret::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if (nState == WA_INACTIVE)
	{
		m_ullSize = 0;
		UpdateHexCtrl();
	}

	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
}

void CHexDlgDataInterpret::OnOK()
{
	if (!m_pHexCtrl->IsMutable() || !m_pPropChanged)
		return;

	CStringW wstrValue = m_pPropChanged->GetValue();
	const auto& refGridData = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[this](const GRIDDATA& refData) {return refData.pProp == m_pPropChanged; });
	if (refGridData == m_vecProp.end())
		return;

	bool fSuccess { false };
	switch (refGridData->eGroup)
	{
	case EGroup::DIGITS:
	{
		LONGLONG llData;
		ULONGLONG ullData;
		if (!StrToInt64ExW(wstrValue, STIF_SUPPORT_HEX, &llData))
			break;
		switch (refGridData->eName)
		{
		case EName::NAME_CHAR:
			fSuccess = SetDigitData<CHAR>(llData);
			break;
		case EName::NAME_UCHAR:
			fSuccess = SetDigitData<UCHAR>(llData);
			break;
		case EName::NAME_SHORT:
			fSuccess = SetDigitData<SHORT>(llData);
			break;
		case EName::NAME_USHORT:
			fSuccess = SetDigitData<USHORT>(llData);
			break;
		case EName::NAME_LONG:
			fSuccess = SetDigitData<LONG>(llData);
			break;
		case EName::NAME_ULONG:
			fSuccess = SetDigitData<ULONG>(llData);
			break;
		case EName::NAME_LONGLONG:
			if (WCharsToll(wstrValue, llData, false))
				fSuccess = SetDigitData<LONGLONG>(llData);
			break;
		case EName::NAME_ULONGLONG:
			if (WCharsToUll(wstrValue, ullData, false))
				fSuccess = SetDigitData<ULONGLONG>(static_cast<LONGLONG>(ullData));
			break;
		}
	}
	break;
	case EGroup::FLOAT:
	case EGroup::TIME:
		switch (refGridData->eName)
		{
		case EName::NAME_FLOAT:
		{
			wchar_t* pEndPtr;
			float fl = wcstof(wstrValue, &pEndPtr);
			if (fl == 0 && (pEndPtr == wstrValue.GetBuffer() || *pEndPtr != '\0'))
				break;
			//TODO:	DWORD dw=std::bit_cast<DWORD>(fl);
			DWORD dwData = *reinterpret_cast<DWORD*>(&fl);
			if (m_fBigEndian)
				dwData = _byteswap_ulong(dwData);
			m_pHexCtrl->SetData(m_ullOffset, dwData);
			fSuccess = true;
		}
		break;
		case EName::NAME_DOUBLE:
		{
			wchar_t* pEndPtr;
			double dd = wcstod(wstrValue, &pEndPtr);
			if (dd == 0 && (pEndPtr == wstrValue.GetBuffer() || *pEndPtr != '\0'))
				break;
			QWORD qwData = *reinterpret_cast<QWORD*>(&dd);
			if (m_fBigEndian)
				qwData = _byteswap_uint64(qwData);
			m_pHexCtrl->SetData(m_ullOffset, qwData);
			fSuccess = true;
		}
		break;
		case EName::NAME_TIME32T:
		{
			tm tm { };
			int iYear { };
			swscanf_s(wstrValue, L"%2i/%2i/%4i %2i:%2i:%2i", &tm.tm_mday, &tm.tm_mon, &iYear, &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
			tm.tm_year = iYear - 1900;
			--tm.tm_mon; //Months start from 0.
			__time32_t time32 = _mktime32(&tm);
			if (time32 == -1)
				break;
			if (m_fBigEndian)
				time32 = _byteswap_ulong(time32);
			m_pHexCtrl->SetData(m_ullOffset, static_cast<DWORD>(time32));
			fSuccess = true;
		}
		break;
		case EName::NAME_TIME64T:
		{
			tm tm { };
			int iYear { };
			swscanf_s(wstrValue, L"%2i/%2i/%4i %2i:%2i:%2i", &tm.tm_mday, &tm.tm_mon, &iYear, &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
			tm.tm_year = iYear - 1900;
			--tm.tm_mon; //Months start from 0.
			__time64_t time64 = _mktime64(&tm);
			if (time64 == -1)
				break;
			if (m_fBigEndian)
				time64 = _byteswap_uint64(time64);
			m_pHexCtrl->SetData(m_ullOffset, static_cast<QWORD>(time64));
			fSuccess = true;
		};
		break;
		}
	}
	if (!fSuccess)
		MessageBoxW(L"Wrong number format or out of range.", L"Data error...", MB_ICONERROR);

	UpdateHexCtrl();
	InspectOffset(m_ullOffset);
}

void CHexDlgDataInterpret::OnClose()
{
	m_ullSize = 0;
	CDialogEx::OnClose();
}

LRESULT CHexDlgDataInterpret::OnPropertyChanged(WPARAM wParam, LPARAM lParam)
{
	if (wParam == IDC_HEXCTRL_DATAINTERPRET_PROPERTY_DATA)
	{
		m_pPropChanged = reinterpret_cast<CMFCPropertyGridProperty*>(lParam);
		OnOK();
	}

	return 0;
}

BOOL CHexDlgDataInterpret::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT * pResult)
{
	if (wParam == HEXCTRL_PROPGRIDCTRL)
	{
		auto pHdr = reinterpret_cast<NMHDR*>(lParam);
		if (pHdr->code != HEXCTRL_PROPGRIDCTRL_SELCHANGED)
			return FALSE;

		auto pData = std::find_if(m_vecProp.begin(), m_vecProp.end(),
			[this](const GRIDDATA& refData) {return refData.pProp == m_stCtrlGrid.GetCurrentProp(); });

		if (pData != m_vecProp.end())
		{
			switch (pData->eSize)
			{
			case ESize::SIZE_BYTE:
				m_ullSize = static_cast<ULONGLONG>(ESize::SIZE_BYTE);
				break;
			case ESize::SIZE_WORD:
				m_ullSize = static_cast<ULONGLONG>(ESize::SIZE_WORD);
				break;
			case ESize::SIZE_DWORD:
				m_ullSize = static_cast<ULONGLONG>(ESize::SIZE_DWORD);
				break;
			case ESize::SIZE_QWORD:
				m_ullSize = static_cast<ULONGLONG>(ESize::SIZE_QWORD);
				break;
			}
			UpdateHexCtrl();
		}
	}

	return CDialogEx::OnNotify(wParam, lParam, pResult);
}

void CHexDlgDataInterpret::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	if (m_fVisible)
		m_stCtrlGrid.GetHeaderCtrl().SetItem(0, &m_hdItemPropGrid); //Property grid column size.
}

void CHexDlgDataInterpret::OnClickRadioLe()
{
	m_fBigEndian = false;
	InspectOffset(m_ullOffset);
}

void CHexDlgDataInterpret::OnClickRadioBe()
{
	m_fBigEndian = true;
	InspectOffset(m_ullOffset);
}

void CHexDlgDataInterpret::OnClickRadioDec()
{
	m_fShowAsHex = false;
	InspectOffset(m_ullOffset);
}

void CHexDlgDataInterpret::OnClickRadioHex()
{
	m_fShowAsHex = true;
	InspectOffset(m_ullOffset);
}

void CHexDlgDataInterpret::InspectOffset(ULONGLONG ullOffset)
{
	if (!m_fVisible)
		return;

	auto ullSize = m_pHexCtrl->GetDataSize();
	if (ullOffset >= ullSize) //Out of data bounds.
		return;

	m_ullOffset = ullOffset;
	WCHAR buff[32];
	std::wstring wstrFormat { };

	//Binary
	const WCHAR* nibbles[] = { L"0000", L"0001", L"0010", L"0011", L"0100", L"0101", L"0110", L"0111",	L"1000", L"1001", L"1010", L"1011", L"1100", L"1101", L"1110", L"1111" };
	wstrFormat = L"%s%s";
	
	const auto byte = m_pHexCtrl->GetData<BYTE>(ullOffset);
	swprintf_s(buff, _countof(buff), wstrFormat.data(), nibbles[byte >> 4], nibbles[byte & 0x0F]);
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const GRIDDATA& refData) {return refData.eName == EName::NAME_BINARY; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);

	//Char
	if (m_fShowAsHex)
		wstrFormat = L"0x%hhX";
	else
		wstrFormat = L"%hhi";
	
	swprintf_s(buff, _countof(buff), wstrFormat.data(), static_cast<char>(byte));
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const GRIDDATA& refData) {return refData.eName == EName::NAME_CHAR; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);

	//Uchar
	if (m_fShowAsHex)
		wstrFormat = L"0x%hhX";
	else
		wstrFormat = L"%hhu";

	swprintf_s(buff, _countof(buff), wstrFormat.data(), byte);
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const GRIDDATA& refData) {return refData.eName == EName::NAME_UCHAR; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);

	if (ullOffset + sizeof(WORD) > ullSize)
	{
		for (const auto& iter : m_vecProp)
		{
			if (iter.eSize > ESize::SIZE_BYTE)
			{
				iter.pProp->SetValue(L"0");
				iter.pProp->Enable(0);
			}
		}
		return;
	}

	for (const auto& iter : m_vecProp)
		if (iter.eSize == ESize::SIZE_WORD)
			iter.pProp->Enable(1);

	auto word = m_pHexCtrl->GetData<WORD>(ullOffset);
	if (m_fBigEndian)
		word = _byteswap_ushort(word);

	//Short
	if (m_fShowAsHex)
		wstrFormat = L"0x%hX";
	else
		wstrFormat = L"%hi";

	swprintf_s(buff, _countof(buff), wstrFormat.data(), static_cast<short>(word));
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const GRIDDATA& refData) {return refData.eName == EName::NAME_SHORT; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);

	//UShort
	if (m_fShowAsHex)
		wstrFormat = L"0x%hX";
	else
		wstrFormat = L"%hu";

	swprintf_s(buff, _countof(buff), wstrFormat.data(), word);
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const GRIDDATA& refData) {return refData.eName == EName::NAME_USHORT; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);

	if (ullOffset + sizeof(DWORD) > ullSize)
	{
		for (const auto& iter : m_vecProp)
		{
			if (iter.eSize > ESize::SIZE_WORD)
			{
				iter.pProp->SetValue(L"0");
				iter.pProp->Enable(0);
			}
		}
		return;
	}

	for (const auto& iter : m_vecProp)
		if (iter.eSize == ESize::SIZE_DWORD)
			iter.pProp->Enable(1);

	auto dword = m_pHexCtrl->GetData<DWORD>(ullOffset);
	if (m_fBigEndian)
		dword = _byteswap_ulong(dword);


	//Long
	if (m_fShowAsHex)
		wstrFormat = L"0x%X";
	else
		wstrFormat = L"%i";

	swprintf_s(buff, _countof(buff), wstrFormat.data(), static_cast<int>(dword));
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const GRIDDATA& refData) {return refData.eName == EName::NAME_LONG; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);

	//Ulong
	if (m_fShowAsHex)
		wstrFormat = L"0x%X";
	else
		wstrFormat = L"%u";

	swprintf_s(buff, _countof(buff), wstrFormat.data(), dword);
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const GRIDDATA& refData) {return refData.eName == EName::NAME_ULONG; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);

	swprintf_s(buff, _countof(buff), L"%.9e", *reinterpret_cast<const float*>(&dword));
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const GRIDDATA& refData) {return refData.eName == EName::NAME_FLOAT; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);

	//Time32.
	std::wstring wstrTime;
	tm tm;
	if (_localtime32_s(&tm, reinterpret_cast<const __time32_t*>(&dword)) == 0)
	{
		char str[32];
		strftime(str, 31, "%d/%m/%Y %H:%M:%S", &tm);
		wstrTime = StrToWstr(str);
	}
	else
		wstrTime = L"N/A";

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const GRIDDATA& refData) {return refData.eName == EName::NAME_TIME32T; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());

	if (ullOffset + sizeof(QWORD) > ullSize)
	{
		for (const auto& iter : m_vecProp)
		{
			if (iter.eSize > ESize::SIZE_QWORD)
			{
				iter.pProp->SetValue(L"0");
				iter.pProp->Enable(0);
			}
		}
		return;
	}

	for (const auto& iter : m_vecProp)
		if (iter.eSize == ESize::SIZE_QWORD)
			iter.pProp->Enable(1);

	auto qword = m_pHexCtrl->GetData<QWORD>(ullOffset);
	if (m_fBigEndian)
		qword = _byteswap_uint64(qword);

	//Longlong
	if (m_fShowAsHex)
		wstrFormat = L"0x%llX";
	else
		wstrFormat = L"%lli";

	swprintf_s(buff, _countof(buff), wstrFormat.data(), static_cast<long long>(qword));
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const GRIDDATA& refData) {return refData.eName == EName::NAME_LONGLONG; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);

	//Ulonglong
	if (m_fShowAsHex)
		wstrFormat = L"0x%llX";
	else
		wstrFormat = L"%llu";

	swprintf_s(buff, _countof(buff), wstrFormat.data(), qword);
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const GRIDDATA& refData) {return refData.eName == EName::NAME_ULONGLONG; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);

	swprintf_s(buff, _countof(buff), L"%.18e", *reinterpret_cast<const double*>(&qword));
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const GRIDDATA& refData) {return refData.eName == EName::NAME_DOUBLE; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);

	//Time64.
	if (_localtime64_s(&tm, reinterpret_cast<const __time64_t*>(&qword)) == 0)
	{
		char str[32];
		strftime(str, _countof(str), "%d/%m/%Y %H:%M:%S", &tm);
		wstrTime = StrToWstr(str);
	}
	else
		wstrTime = L"N/A";

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const GRIDDATA& refData) {return refData.eName == EName::NAME_TIME64T; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());

	//FILETIME	
	wstrTime = L"N/A";
	SYSTEMTIME SysTime = {0};
	if (FileTimeToSystemTime(reinterpret_cast<const FILETIME*>(&qword), &SysTime))
		wstrTime = SystemTimeToString(&SysTime, true, true).GetString();

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const GRIDDATA& refData) {return refData.eName == EName::NAME_FILETIME; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());

	//Javatime (signed)
	//Number of milliseconds after/before January 1, 1970, 00:00:00 UTC
	wstrTime = L"N/A";
	
	FILETIME ftJavaTime;
	if (qword >= 0)
	{
		LARGE_INTEGER Time;
		Time.HighPart = FILETIME1970_HIGH;
		Time.LowPart = FILETIME1970_LOW;
		Time.QuadPart += qword * FTTICKSPERMS;
		ftJavaTime.dwHighDateTime = Time.HighPart;
		ftJavaTime.dwLowDateTime = Time.LowPart;		
	}
	else
	{
		LARGE_INTEGER Time;
		Time.HighPart = FILETIME1970_HIGH;
		Time.LowPart = FILETIME1970_LOW;
		Time.QuadPart -= qword * FTTICKSPERMS;
		ftJavaTime.dwHighDateTime = Time.HighPart;
		ftJavaTime.dwLowDateTime = Time.LowPart;
	}

	if (FileTimeToSystemTime(&ftJavaTime, &SysTime))
		wstrTime = SystemTimeToString(&SysTime, true, true).GetString();

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const GRIDDATA& refData) {return refData.eName == EName::NAME_JAVATIME; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());

	//MS-DOS date/time
	typedef union _MSDOSDATETIME		//MS-DOS Date+Time structure (as used in FAT file system directory entry)
	{									//See: https://msdn.microsoft.com/en-us/library/ms724274(v=vs.85).aspx
		struct
		{
			WORD wTime;					//Time component
			WORD wDate;					//Date component
		} TimeDate;
		DWORD dwTimeDate;
	} MSDOSDATETIME, *PMSDOSDATETIME;
	   	 
	//Convert to FILETIME
	wstrTime = L"N/A";
	FILETIME ftMSDOS;
	MSDOSDATETIME msdosDateTime;
	msdosDateTime.dwTimeDate = dword;	
	if (DosDateTimeToFileTime(msdosDateTime.TimeDate.wDate, msdosDateTime.TimeDate.wTime, &ftMSDOS))
	{
		if (FileTimeToSystemTime(&ftMSDOS, &SysTime))
			wstrTime = SystemTimeToString(&SysTime, true, true).GetString();
	}

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const GRIDDATA& refData) {return refData.eName == EName::NAME_MSDOSTIME; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());
	
	typedef union _DQWORD128
	{									
		struct
		{
			QWORD qwLow;
			QWORD qwHigh;
		} Value;
		GUID gGUID;
	} DQWORD128, *PDQWORD128;

	auto dqword = m_pHexCtrl->GetData<DQWORD128>(ullOffset);	
	if (m_fBigEndian)
	{
		//TODO: Test this thoroughly
		QWORD tmp = dqword.Value.qwLow;
		dqword.Value.qwLow = _byteswap_uint64(dqword.Value.qwHigh);
		dqword.Value.qwHigh = _byteswap_uint64(tmp);
	}

	//Guid
	CString sGUID;
	sGUID.Format(_T("{%.8x-%.4x-%.4x-%.2x%.2x-%.2x%.2x%.2x%.2x%.2x%.2x}"), dqword.gGUID.Data1, dqword.gGUID.Data2, dqword.gGUID.Data3, dqword.gGUID.Data4[0], dqword.gGUID.Data4[1], dqword.gGUID.Data4[2], dqword.gGUID.Data4[3], dqword.gGUID.Data4[4], dqword.gGUID.Data4[5], dqword.gGUID.Data4[6], dqword.gGUID.Data4[7]);
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const GRIDDATA& refData) {return refData.eName == EName::NAME_GUID; }); iter != m_vecProp.end())
		iter->pProp->SetValue(sGUID);

	//Windows SYSTEMTIME
	memcpy(&SysTime, &dqword, sizeof(SysTime));
	wstrTime = SystemTimeToString(&SysTime, true, true).GetString();

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const GRIDDATA& refData) {return refData.eName == EName::NAME_SYSTEMTIME; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());
	
	//Guid v1 Datetime UTC
	//First, verify GUID is actually version 1 style
	wstrTime = L"N/A";
	unsigned short unGuidVersion = (dqword.gGUID.Data3 & 0xf000) >> 12;
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
		ullSubtractTicks.QuadPart = QWORD(FTTICKSPERSECOND)*QWORD(SECONDSPERHOUR)*QWORD(HOURSPERDAY)*QWORD(FILETIME1582OFFSETDAYS);
		qwGUIDTime.QuadPart -= ullSubtractTicks.QuadPart;

		//Convert to SYSTEMTIME
		FILETIME ftGUIDTime;
		ftGUIDTime.dwHighDateTime = qwGUIDTime.HighPart;
		ftGUIDTime.dwLowDateTime = qwGUIDTime.LowPart;
		if (FileTimeToSystemTime(&ftGUIDTime, &SysTime))
			wstrTime = SystemTimeToString(&SysTime, true, true).GetString();
	}

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const GRIDDATA& refData) {return refData.eName == EName::NAME_GUIDTIME; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());	
}

ULONGLONG CHexDlgDataInterpret::GetSize()
{
	return m_ullSize;
}

BOOL CHexDlgDataInterpret::ShowWindow(int nCmdShow)
{
	if (!m_pHexCtrl)
		return FALSE;

	m_fVisible = nCmdShow == SW_SHOW;
	if (m_fVisible)
		InspectOffset(m_pHexCtrl->GetCaretPos());

	return CWnd::ShowWindow(nCmdShow);
}

void CHexDlgDataInterpret::UpdateHexCtrl()
{
	if (m_pHexCtrl)
		m_pHexCtrl->RedrawWindow();
}

CString CHexDlgDataInterpret::SystemTimeToString(PSYSTEMTIME pSysTime, bool bIncludeDate, bool bIncludeTime)
{
	if (pSysTime->wDay > 0 && pSysTime->wDay < 32 && pSysTime->wMonth>0 && pSysTime->wMonth < 13 && pSysTime->wYear < 10000 && pSysTime->wHour < 25 && pSysTime->wMinute < 61 && pSysTime->wSecond < 61 && pSysTime->wMilliseconds < 1000)
	{
		//Generate human formatted date. Fall back to UK/European if unable to determine
		CString sResult;
		if (bIncludeDate)
		{
			switch (m_dwDateFormat)
			{
				//0=Month-Day-Year
			case 0:		sResult.Format(L"%.2d%s%.2d%s%.4d", pSysTime->wMonth, m_wszDateSeparator, pSysTime->wDay, m_wszDateSeparator, pSysTime->wYear);
				break;

				//2=Year-Month-Day 
			case 2:		sResult.Format(L"%.4d%s%.2d%s%.2d", pSysTime->wYear, m_wszDateSeparator, pSysTime->wMonth, m_wszDateSeparator, pSysTime->wDay);
				break;

				//1=Day-Month-Year (default)
			default:	sResult.Format(L"%.2d%s%.2d%s%.4d", pSysTime->wDay, m_wszDateSeparator, pSysTime->wMonth, m_wszDateSeparator, pSysTime->wYear);
				break;
			}

			if (bIncludeTime)
				sResult.Append(L" ");
		}

		//Append optional time elements
		if (bIncludeTime)
		{
			sResult.AppendFormat(L"%.2d:%.2d:%.2d", pSysTime->wHour, pSysTime->wMinute, pSysTime->wSecond);

			if (pSysTime->wMilliseconds > 0)
				sResult.AppendFormat(L".%.3d", pSysTime->wMilliseconds);
		}

		return sResult;
	}
	else
		return L"N/A";
}