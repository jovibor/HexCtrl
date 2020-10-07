/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#include "stdafx.h"
#include "../../res/HexCtrlRes.h"
#include "../Helper.h"
#include "CHexDlgDataInterpret.h"
#include "strsafe.h"
#include <algorithm>
#include <cassert>

using namespace HEXCTRL::INTERNAL;

BEGIN_MESSAGE_MAP(CHexDlgDataInterpret, CDialogEx)
	ON_WM_CLOSE()
	ON_WM_ACTIVATE()
	ON_WM_SIZE()
	ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, &CHexDlgDataInterpret::OnPropertyChanged)
	ON_COMMAND(IDC_HEXCTRL_DATAINTERPRET_RADIO_LE, &CHexDlgDataInterpret::OnClickRadioLe)
	ON_COMMAND(IDC_HEXCTRL_DATAINTERPRET_RADIO_BE, &CHexDlgDataInterpret::OnClickRadioBe)
	ON_COMMAND(IDC_HEXCTRL_DATAINTERPRET_RADIO_DEC, &CHexDlgDataInterpret::OnClickRadioDec)
	ON_COMMAND(IDC_HEXCTRL_DATAINTERPRET_RADIO_HEX, &CHexDlgDataInterpret::OnClickRadioHex)
	ON_WM_DESTROY()
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()

void CHexDlgDataInterpret::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HEXCTRL_DATAINTERPRET_PROPERTY_DATA, m_stCtrlGrid);
}

BOOL CHexDlgDataInterpret::Create(UINT nIDTemplate, CHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	if (pHexCtrl == nullptr)
		return FALSE;

	m_pHexCtrl = pHexCtrl;

	return CDialogEx::Create(nIDTemplate, pHexCtrl);
}

BOOL CHexDlgDataInterpret::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	//Determine locale specific date format. Default to UK/European if unable to determine
	//See: https://docs.microsoft.com/en-gb/windows/win32/intl/locale-idate
	if (!GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IDATE | LOCALE_RETURN_NUMBER,
		reinterpret_cast<LPTSTR>(&m_dwDateFormat), sizeof(m_dwDateFormat)))
		m_dwDateFormat = 1;

	//Determine 'short' date seperator character. Default to UK/European if unable to determine	
	if (!GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDATE, m_warrDateSeparator, static_cast<int>(std::size(m_warrDateSeparator))))
		swprintf_s(m_warrDateSeparator, std::size(m_warrDateSeparator), L"/");

	//Update dialog title to include date format
	CString sTitle;
	GetWindowTextW(sTitle);
	sTitle.AppendFormat(L" [%s]", GetCurrentUserDateFormatString().data());
	SetWindowTextW(sTitle);

	if (auto pRadio = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_DATAINTERPRET_RADIO_LE)); pRadio)
		pRadio->SetCheck(BST_CHECKED);
	if (auto pRadio = static_cast<CButton*>(GetDlgItem(IDC_HEXCTRL_DATAINTERPRET_RADIO_HEX)); pRadio)
		pRadio->SetCheck(BST_CHECKED);

	m_hdItemPropGrid.mask = HDI_WIDTH;
	m_hdItemPropGrid.cxy = 150;

	m_vecProp.emplace_back(SGRIDDATA { new CMFCPropertyGridProperty(L"binary:", L"0"), EGroup::DIGITS, EName::NAME_BINARY, ESize::SIZE_BYTE });
	m_vecProp.emplace_back(SGRIDDATA { new CMFCPropertyGridProperty(L"char:", L"0"), EGroup::DIGITS, EName::NAME_CHAR, ESize::SIZE_BYTE });
	m_vecProp.emplace_back(SGRIDDATA { new CMFCPropertyGridProperty(L"unsigned char:", L"0"), EGroup::DIGITS, EName::NAME_UCHAR, ESize::SIZE_BYTE });
	m_vecProp.emplace_back(SGRIDDATA { new CMFCPropertyGridProperty(L"short:", L"0"), EGroup::DIGITS, EName::NAME_SHORT, ESize::SIZE_WORD });
	m_vecProp.emplace_back(SGRIDDATA { new CMFCPropertyGridProperty(L"unsigned short:", L"0"), EGroup::DIGITS, EName::NAME_USHORT, ESize::SIZE_WORD });
	m_vecProp.emplace_back(SGRIDDATA { new CMFCPropertyGridProperty(L"long:", L"0"), EGroup::DIGITS, EName::NAME_LONG, ESize::SIZE_DWORD });
	m_vecProp.emplace_back(SGRIDDATA { new CMFCPropertyGridProperty(L"unsigned long:", L"0"), EGroup::DIGITS, EName::NAME_ULONG, ESize::SIZE_DWORD });
	m_vecProp.emplace_back(SGRIDDATA { new CMFCPropertyGridProperty(L"long long:", L"0"), EGroup::DIGITS, EName::NAME_LONGLONG, ESize::SIZE_QWORD });
	m_vecProp.emplace_back(SGRIDDATA { new CMFCPropertyGridProperty(L"unsigned long long:", L"0"), EGroup::DIGITS, EName::NAME_ULONGLONG, ESize::SIZE_QWORD });
	m_vecProp.emplace_back(SGRIDDATA { new CMFCPropertyGridProperty(L"time32_t:", L"0"), EGroup::TIME, EName::NAME_TIME32T, ESize::SIZE_DWORD });
	m_vecProp.emplace_back(SGRIDDATA { new CMFCPropertyGridProperty(L"time64_t:", L"0"), EGroup::TIME, EName::NAME_TIME64T, ESize::SIZE_QWORD });
	m_vecProp.emplace_back(SGRIDDATA { new CMFCPropertyGridProperty(L"FILETIME:", L"0"), EGroup::TIME, EName::NAME_FILETIME, ESize::SIZE_QWORD });
	m_vecProp.emplace_back(SGRIDDATA { new CMFCPropertyGridProperty(L"OLE time:", L"0"), EGroup::TIME, EName::NAME_OLEDATETIME, ESize::SIZE_QWORD });
	m_vecProp.emplace_back(SGRIDDATA { new CMFCPropertyGridProperty(L"Java time:", L"0"), EGroup::TIME, EName::NAME_JAVATIME, ESize::SIZE_QWORD });
	m_vecProp.emplace_back(SGRIDDATA { new CMFCPropertyGridProperty(L"MS-DOS time:", L"0"), EGroup::TIME, EName::NAME_MSDOSTIME, ESize::SIZE_QWORD });
	m_vecProp.emplace_back(SGRIDDATA { new CMFCPropertyGridProperty(L"MS-UDTTM time:", L"0"), EGroup::TIME, EName::NAME_MSDTTMTIME, ESize::SIZE_DWORD });
	m_vecProp.emplace_back(SGRIDDATA { new CMFCPropertyGridProperty(L"Windows SYSTEMTIME:", L"0"), EGroup::TIME, EName::NAME_SYSTEMTIME, ESize::SIZE_DQWORD });
	m_vecProp.emplace_back(SGRIDDATA { new CMFCPropertyGridProperty(L"Float:", L"0"), EGroup::FLOAT, EName::NAME_FLOAT, ESize::SIZE_DWORD });
	m_vecProp.emplace_back(SGRIDDATA { new CMFCPropertyGridProperty(L"Double:", L"0"), EGroup::FLOAT, EName::NAME_DOUBLE, ESize::SIZE_QWORD });
	m_vecProp.emplace_back(SGRIDDATA { new CMFCPropertyGridProperty(L"GUID:", L"0"), EGroup::MISC, EName::NAME_GUID, ESize::SIZE_DQWORD });
	m_vecProp.emplace_back(SGRIDDATA { new CMFCPropertyGridProperty(L"GUID v1 UTC time:", L"0"), EGroup::TIME, EName::NAME_GUIDTIME, ESize::SIZE_DQWORD, true });

	m_stCtrlGrid.EnableHeaderCtrl(TRUE, L"Data type", L"Value");
	m_stCtrlGrid.GetHeaderCtrl().SetItem(0, &m_hdItemPropGrid); //Property grid column size.

	//Digits group
	auto pDigits = new CMFCPropertyGridProperty(L"Digits:");
	for (const auto& iter : m_vecProp)
		if (iter.eGroup == EGroup::DIGITS && !iter.fChild)
			pDigits->AddSubItem(iter.pProp);
	m_stCtrlGrid.AddProperty(pDigits);

	//Floats group
	auto pFloats = new CMFCPropertyGridProperty(L"Floats:");
	for (const auto& iter : m_vecProp)
		if (iter.eGroup == EGroup::FLOAT && !iter.fChild)
			pFloats->AddSubItem(iter.pProp);
	m_stCtrlGrid.AddProperty(pFloats);

	//Time group
	auto pTime = new CMFCPropertyGridProperty(L"Time:");
	for (const auto& iter : m_vecProp)
		if (iter.eGroup == EGroup::TIME && !iter.fChild)
			pTime->AddSubItem(iter.pProp);
	m_stCtrlGrid.AddProperty(pTime);

	//Miscellaneous group
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

void CHexDlgDataInterpret::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if (nState == WA_INACTIVE)
	{
		m_ullSize = 0;
		UpdateHexCtrl();
	}

	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
}

void CHexDlgDataInterpret::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	if ((m_fVisible = bShow != FALSE) == true)
		InspectOffset(m_pHexCtrl->GetCaretPos());
}

void CHexDlgDataInterpret::OnOK()
{
	if (!m_pHexCtrl->IsMutable() || !m_pPropChanged)
		return;

	const auto& refGridData = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[this](const SGRIDDATA& refData) {return refData.pProp == m_pPropChanged; });
	if (refGridData == m_vecProp.end())
		return;

	std::wstring wstr = static_cast<CStringW>(m_pPropChanged->GetValue()).GetString();

	bool fSuccess { false };
	switch (refGridData->eName)
	{
	case EName::NAME_BINARY:
		fSuccess = SetDataNAME_BINARY(wstr);
		break;
	case EName::NAME_CHAR:
		fSuccess = SetDataNAME_CHAR(wstr);
		break;
	case EName::NAME_UCHAR:
		fSuccess = SetDataNAME_UCHAR(wstr);
		break;
	case EName::NAME_SHORT:
		fSuccess = SetDataNAME_SHORT(wstr);
		break;
	case EName::NAME_USHORT:
		fSuccess = SetDataNAME_USHORT(wstr);
		break;
	case EName::NAME_LONG:
		fSuccess = SetDataNAME_LONG(wstr);
		break;
	case EName::NAME_ULONG:
		fSuccess = SetDataNAME_ULONG(wstr);
		break;
	case EName::NAME_LONGLONG:
		fSuccess = SetDataNAME_LONGLONG(wstr);
		break;
	case EName::NAME_ULONGLONG:
		fSuccess = SetDataNAME_ULONGLONG(wstr);
		break;
	case EName::NAME_FLOAT:
		fSuccess = SetDataNAME_FLOAT(wstr);
		break;
	case EName::NAME_DOUBLE:
		fSuccess = SetDataNAME_DOUBLE(wstr);
		break;
	case EName::NAME_TIME32T:
		fSuccess = SetDataNAME_TIME32T(wstr);
		break;
	case EName::NAME_TIME64T:
		fSuccess = SetDataNAME_TIME64T(wstr);
		break;
	case EName::NAME_FILETIME:
		fSuccess = SetDataNAME_FILETIME(wstr);
		break;
	case EName::NAME_OLEDATETIME:
		fSuccess = SetDataNAME_OLEDATETIME(wstr);
		break;
	case EName::NAME_JAVATIME:
		fSuccess = SetDataNAME_JAVATIME(wstr);
		break;
	case EName::NAME_MSDOSTIME:
		fSuccess = SetDataNAME_MSDOSTIME(wstr);
		break;
	case EName::NAME_MSDTTMTIME:
		fSuccess = SetDataNAME_MSDTTMTIME(wstr);
		break;
	case EName::NAME_SYSTEMTIME:
		fSuccess = SetDataNAME_SYSTEMTIME(wstr);
		break;
	case EName::NAME_GUIDTIME:
		fSuccess = SetDataNAME_GUIDTIME(wstr);
		break;
	case EName::NAME_GUID:
		fSuccess = SetDataNAME_GUID(wstr);
		break;
	};

	if (!fSuccess)
		MessageBoxW(L"Wrong number format or out of range.", L"Data error...", MB_ICONERROR);
	else
		UpdateHexCtrl();

	InspectOffset(m_ullOffset);
}

void CHexDlgDataInterpret::InspectOffset(ULONGLONG ullOffset)
{
	if (!m_fVisible)
		return;

	auto ullDataSize = m_pHexCtrl->GetDataSize();
	if (ullOffset >= ullDataSize) //Out of data bounds.
		return;

	for (const auto& iter : m_vecProp)
		iter.pProp->AllowEdit(m_pHexCtrl->IsMutable());

	m_ullOffset = ullOffset;
	const auto byte = m_pHexCtrl->GetData<BYTE>(ullOffset);

	ShowNAME_BINARY(byte);
	ShowNAME_CHAR(byte);
	ShowNAME_UCHAR(byte);

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

	auto word = m_pHexCtrl->GetData<WORD>(ullOffset);
	if (m_fBigEndian)
		word = _byteswap_ushort(word);

	ShowNAME_SHORT(word);
	ShowNAME_USHORT(word);

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

	auto dword = m_pHexCtrl->GetData<DWORD>(ullOffset);
	if (m_fBigEndian)
		dword = _byteswap_ulong(dword);

	ShowNAME_LONG(dword);
	ShowNAME_ULONG(dword);
	ShowNAME_FLOAT(dword);
	ShowNAME_TIME32(dword);
	ShowNAME_MSDOSTIME(dword);
	ShowNAME_MSDTTMTIME(dword);

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

	auto qword = m_pHexCtrl->GetData<QWORD>(ullOffset);
	if (m_fBigEndian)
		qword = _byteswap_uint64(qword);

	ShowNAME_LONGLONG(qword);
	ShowNAME_ULONGLONG(qword);
	ShowNAME_DOUBLE(qword);
	ShowNAME_TIME64(qword);
	ShowNAME_FILETIME(qword);
	ShowNAME_OLEDATETIME(qword);
	ShowNAME_JAVATIME(qword);

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

	auto dqword = m_pHexCtrl->GetData<UDQWORD>(ullOffset);
	if (m_fBigEndian)
	{
		//TODO: Test this thoroughly
		QWORD tmp = dqword.Value.qwLow;
		dqword.Value.qwLow = _byteswap_uint64(dqword.Value.qwHigh);
		dqword.Value.qwHigh = _byteswap_uint64(tmp);
	}

	ShowNAME_GUID(dqword);
	ShowNAME_GUIDTIME(dqword);
	ShowNAME_SYSTEMTIME(dqword);
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

BOOL CHexDlgDataInterpret::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	if (wParam == HEXCTRL_PROPGRIDCTRL)
	{
		auto pHdr = reinterpret_cast<NMHDR*>(lParam);
		if (pHdr->code != HEXCTRL_PROPGRIDCTRL_SELCHANGED)
			return FALSE;

		if (auto pData = std::find_if(m_vecProp.begin(), m_vecProp.end(),
			[this](const SGRIDDATA& refData)
			{return refData.pProp == m_stCtrlGrid.GetCurrentProp(); }); pData != m_vecProp.end())
		{
			m_ullSize = static_cast<ULONGLONG>(pData->eSize);
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

void CHexDlgDataInterpret::OnDestroy()
{
	CDialogEx::OnDestroy();

	m_stCtrlGrid.DestroyWindow();
	m_vecProp.clear();
}

ULONGLONG CHexDlgDataInterpret::GetSize()const
{
	return m_ullSize;
}

template<typename T>void CHexDlgDataInterpret::SetDigitData(T tData)const
{
	if (m_fBigEndian)
	{
		switch (sizeof(T))
		{
		case (sizeof(WORD)):
			tData = static_cast<T>(_byteswap_ushort(static_cast<WORD>(tData)));
			break;
		case (sizeof(DWORD)):
			tData = static_cast<T>(_byteswap_ulong(static_cast<DWORD>(tData)));
			break;
		case (sizeof(QWORD)):
			tData = static_cast<T>(_byteswap_uint64(static_cast<QWORD>(tData)));
			break;
		default:
			break;
		}
	}
	m_pHexCtrl->SetData(m_ullOffset, tData);
}

void CHexDlgDataInterpret::UpdateHexCtrl()const
{
	if (m_pHexCtrl)
		m_pHexCtrl->RedrawWindow();
}

std::wstring CHexDlgDataInterpret::GetCurrentUserDateFormatString()const
{
	std::wstring_view wstrFormat { };
	switch (m_dwDateFormat)
	{
	case 0:	//0=Month-Day-Year
		wstrFormat = L"mm%sdd%syyyy";
		break;
	case 2:	//2=Year-Month-Day
		wstrFormat = L"yyyy%smm%sdd";
		break;
	default: //1=Day-Month-Year (default)
		wstrFormat = L"dd%smm%syyyy";
	}

	WCHAR buff[32];
	swprintf_s(buff, std::size(buff), wstrFormat.data(), m_warrDateSeparator, m_warrDateSeparator);

	return buff;
}

std::wstring CHexDlgDataInterpret::SystemTimeToString(const SYSTEMTIME* pSysTime, bool bIncludeDate, bool bIncludeTime)const
{
	if (!pSysTime)
		return L"Invalid";

	std::wstring wstrRet = L"N/A";

	if (pSysTime->wDay > 0 && pSysTime->wDay < 32 && pSysTime->wMonth > 0 && pSysTime->wMonth < 13
		&& pSysTime->wYear < 10000 && pSysTime->wHour < 24 && pSysTime->wMinute < 60 && pSysTime->wSecond < 60
		&& pSysTime->wMilliseconds < 1000)
	{
		//Generate human formatted date. Fall back to UK/European if unable to determine
		WCHAR buff[32];
		if (bIncludeDate)
		{
			switch (m_dwDateFormat)
			{
			case 0:	//0=Month-Day-Year
				swprintf_s(buff, std::size(buff), L"%.2d%s%.2d%s%.4d",
					pSysTime->wMonth, m_warrDateSeparator, pSysTime->wDay, m_warrDateSeparator, pSysTime->wYear);
				break;
			case 2:	//2=Year-Month-Day
				swprintf_s(buff, std::size(buff), L"%.4d%s%.2d%s%.2d",
					pSysTime->wYear, m_warrDateSeparator, pSysTime->wMonth, m_warrDateSeparator, pSysTime->wDay);
				break;
			default: //1=Day-Month-Year (default)
				swprintf_s(buff, std::size(buff), L"%.2d%s%.2d%s%.4d",
					pSysTime->wDay, m_warrDateSeparator, pSysTime->wMonth, m_warrDateSeparator, pSysTime->wYear);
			}
			wstrRet = buff;
			if (bIncludeTime)
				wstrRet += L" ";
		}

		//Append optional time elements
		if (bIncludeTime)
		{
			swprintf_s(buff, std::size(buff), L"%.2d:%.2d:%.2d", pSysTime->wHour, pSysTime->wMinute, pSysTime->wSecond);
			wstrRet += buff;

			if (pSysTime->wMilliseconds > 0)
			{
				swprintf_s(buff, std::size(buff), L".%.3d", pSysTime->wMilliseconds);
				wstrRet += buff;
			}
		}
	}

	return wstrRet;
}

bool CHexDlgDataInterpret::StringToSystemTime(std::wstring_view wstr, PSYSTEMTIME pSysTime, bool bIncludeDate, bool bIncludeTime)const
{
	if (wstr.empty() || pSysTime == nullptr)
		return false;

	std::memset(pSysTime, 0, sizeof(SYSTEMTIME));

	//Normalise the input string by replacing non-numeric characters except space with /
	//This should regardless of the current date/time separator character
	std::wstring wstrDateTimeCooked { };
	for (const auto iter : wstr)
	{
		if (iswdigit(iter) || iter == L' ')
			wstrDateTimeCooked += iter;
		else
			wstrDateTimeCooked += L'/';
	}

	//Parse date component
	if (bIncludeDate)
	{
		switch (m_dwDateFormat)
		{
		case 0:	//0=Month-Day-Year
			swscanf_s(wstrDateTimeCooked.data(), L"%2hu/%2hu/%4hu", &pSysTime->wMonth, &pSysTime->wDay, &pSysTime->wYear);
			break;
		case 2:	//2=Year-Month-Day 
			swscanf_s(wstrDateTimeCooked.data(), L"%4hu/%2hu/%2hu", &pSysTime->wYear, &pSysTime->wMonth, &pSysTime->wDay);
			break;
		default: //1=Day-Month-Year (default)
			swscanf_s(wstrDateTimeCooked.data(), L"%2hu/%2hu/%4hu", &pSysTime->wDay, &pSysTime->wMonth, &pSysTime->wYear);
			break;
		}

		//Find time seperator (if present)
		if (auto nPos = wstrDateTimeCooked.find(L' '); nPos != std::wstring::npos)
			wstrDateTimeCooked = wstrDateTimeCooked.substr(nPos + 1);
	}

	//Parse time component
	if (bIncludeTime)
	{
		swscanf_s(wstrDateTimeCooked.data(), L"%2hu/%2hu/%2hu/%3hu", &pSysTime->wHour, &pSysTime->wMinute,
			&pSysTime->wSecond, &pSysTime->wMilliseconds);

		//Ensure valid SYSTEMTIME is calculated but only if time was specified but date was not
		if (!bIncludeDate)
		{
			pSysTime->wYear = 1900;
			pSysTime->wMonth = 1;
			pSysTime->wDay = 1;
		}
	}

	FILETIME ftValidCheck;

	return SystemTimeToFileTime(pSysTime, &ftValidCheck);
}

void CHexDlgDataInterpret::ShowNAME_BINARY(BYTE byte)const
{
	WCHAR buff[32];
	swprintf_s(buff, std::size(buff), L"%s%s", arrNibbles[byte >> 4], arrNibbles[byte & 0x0F]);
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_BINARY; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);
}

void CHexDlgDataInterpret::ShowNAME_CHAR(BYTE byte)const
{
	std::wstring_view wstrFormat { };
	if (m_fShowAsHex)
		wstrFormat = L"0x%hhX";
	else
		wstrFormat = L"%hhi";

	WCHAR buff[32];
	swprintf_s(buff, std::size(buff), wstrFormat.data(), static_cast<char>(byte));
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_CHAR; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);
}

void CHexDlgDataInterpret::ShowNAME_UCHAR(BYTE byte)const
{
	std::wstring_view wstrFormat { };
	if (m_fShowAsHex)
		wstrFormat = L"0x%hhX";
	else
		wstrFormat = L"%hhu";

	WCHAR buff[32];
	swprintf_s(buff, std::size(buff), wstrFormat.data(), byte);
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_UCHAR; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);
}

void CHexDlgDataInterpret::ShowNAME_SHORT(WORD word)const
{
	std::wstring_view wstrFormat { };
	if (m_fShowAsHex)
		wstrFormat = L"0x%hX";
	else
		wstrFormat = L"%hi";

	WCHAR buff[32];
	swprintf_s(buff, std::size(buff), wstrFormat.data(), static_cast<short>(word));
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_SHORT; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);
}

void CHexDlgDataInterpret::ShowNAME_USHORT(WORD word)const
{
	std::wstring_view wstrFormat { };
	if (m_fShowAsHex)
		wstrFormat = L"0x%hX";
	else
		wstrFormat = L"%hu";

	WCHAR buff[32];
	swprintf_s(buff, std::size(buff), wstrFormat.data(), word);
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_USHORT; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);
}

void CHexDlgDataInterpret::ShowNAME_LONG(DWORD dword)const
{
	std::wstring_view wstrFormat { };
	if (m_fShowAsHex)
		wstrFormat = L"0x%X";
	else
		wstrFormat = L"%i";

	WCHAR buff[32];
	swprintf_s(buff, std::size(buff), wstrFormat.data(), static_cast<int>(dword));
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_LONG; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);
}

void CHexDlgDataInterpret::ShowNAME_ULONG(DWORD dword)const
{
	std::wstring_view wstrFormat { };
	if (m_fShowAsHex)
		wstrFormat = L"0x%X";
	else
		wstrFormat = L"%u";

	WCHAR buff[32];
	swprintf_s(buff, std::size(buff), wstrFormat.data(), dword);
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_ULONG; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);
}

void CHexDlgDataInterpret::ShowNAME_FLOAT(DWORD dword)const
{
	WCHAR buff[32];
	swprintf_s(buff, std::size(buff), L"%.9e", *reinterpret_cast<const float*>(&dword));
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_FLOAT; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);
}

void CHexDlgDataInterpret::ShowNAME_TIME32(DWORD dword)const
{
	std::wstring wstrTime = L"N/A";

	//The number of seconds since midnight January 1st 1970 UTC (32-bit). This is signed and wraps on 19 January 2038 
	auto lDiffSeconds = static_cast<LONG>(dword);

	//Unix times are signed and value before 1st January 1970 is not considered valid
	//This is apparently because early complilers didn't support unsigned types. _mktime32() has the same limit
	if (lDiffSeconds >= 0)
	{
		//Add seconds from epoch time
		LARGE_INTEGER Time;
		Time.HighPart = m_ulFileTime1970_HIGH;
		Time.LowPart = m_ulFileTime1970_LOW;
		Time.QuadPart += static_cast<LONGLONG>(lDiffSeconds) * m_uFTTicksPerSec;

		//Convert to FILETIME
		FILETIME ftTime;
		ftTime.dwHighDateTime = Time.HighPart;
		ftTime.dwLowDateTime = Time.LowPart;

		//Convert to SYSTEMTIME for display
		SYSTEMTIME SysTime { };
		if (FileTimeToSystemTime(&ftTime, &SysTime))
			wstrTime = SystemTimeToString(&SysTime, true, true);
	}

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_TIME32T; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());
}

void CHexDlgDataInterpret::ShowNAME_MSDOSTIME(DWORD dword)const
{
	std::wstring wstrTime = L"N/A";
	FILETIME ftMSDOS;
	UMSDOSDATETIME msdosDateTime;
	msdosDateTime.dwTimeDate = dword;
	if (DosDateTimeToFileTime(msdosDateTime.TimeDate.wDate, msdosDateTime.TimeDate.wTime, &ftMSDOS))
	{
		SYSTEMTIME SysTime { };
		if (FileTimeToSystemTime(&ftMSDOS, &SysTime))
			wstrTime = SystemTimeToString(&SysTime, true, true);
	}

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_MSDOSTIME; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());
}

void CHexDlgDataInterpret::ShowNAME_MSDTTMTIME(DWORD dword)const
{
	//Microsoft UDTTM time (as used by Microsoft Compound Document format)
	std::wstring wstrTime = L"N/A";
	UDTTM dttm;
	dttm.dwValue = dword;
	if (dttm.components.dayofmonth > 0 && dttm.components.dayofmonth < 32
		&& dttm.components.hour < 24 && dttm.components.minute < 60
		&& dttm.components.month>0 && dttm.components.month < 13 && dttm.components.weekday < 7)
	{
		SYSTEMTIME SysTime { };
		SysTime.wYear = 1900 + static_cast<WORD>(dttm.components.year);
		SysTime.wMonth = dttm.components.month;
		SysTime.wDayOfWeek = dttm.components.weekday;
		SysTime.wDay = dttm.components.dayofmonth;
		SysTime.wHour = dttm.components.hour;
		SysTime.wMinute = dttm.components.minute;
		SysTime.wSecond = 0;
		SysTime.wMilliseconds = 0;
		wstrTime = SystemTimeToString(&SysTime, true, true);
	}

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_MSDTTMTIME; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());
}

void CHexDlgDataInterpret::ShowNAME_LONGLONG(QWORD qword)const
{
	std::wstring_view wstrFormat { };
	if (m_fShowAsHex)
		wstrFormat = L"0x%llX";
	else
		wstrFormat = L"%lli";

	WCHAR buff[32];
	swprintf_s(buff, std::size(buff), wstrFormat.data(), static_cast<long long>(qword));
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_LONGLONG; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);
}

void CHexDlgDataInterpret::ShowNAME_ULONGLONG(QWORD qword)const
{
	std::wstring_view wstrFormat { };
	if (m_fShowAsHex)
		wstrFormat = L"0x%llX";
	else
		wstrFormat = L"%llu";

	WCHAR buff[32];
	swprintf_s(buff, std::size(buff), wstrFormat.data(), qword);
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_ULONGLONG; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);
}

void CHexDlgDataInterpret::ShowNAME_DOUBLE(QWORD qword)const
{
	WCHAR buff[32];
	swprintf_s(buff, std::size(buff), L"%.18e", *reinterpret_cast<const double*>(&qword));
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_DOUBLE; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);
}

void CHexDlgDataInterpret::ShowNAME_TIME64(QWORD qword)const
{
	std::wstring wstrTime = L"N/A";

	//The number of seconds since midnight January 1st 1970 UTC (64-bit). This is signed
	auto llDiffSeconds = static_cast<LONGLONG>(qword);

	//Unix times are signed and value before 1st January 1970 is not considered valid
	//This is apparently because early complilers didn't support unsigned types. _mktime64() has the same limit
	if (llDiffSeconds >= 0)
	{
		//Add seconds from epoch time
		LARGE_INTEGER Time;
		Time.HighPart = m_ulFileTime1970_HIGH;
		Time.LowPart = m_ulFileTime1970_LOW;
		Time.QuadPart += llDiffSeconds * m_uFTTicksPerSec;

		//Convert to FILETIME
		FILETIME ftTime;
		ftTime.dwHighDateTime = Time.HighPart;
		ftTime.dwLowDateTime = Time.LowPart;

		//Convert to SYSTEMTIME for display
		SYSTEMTIME SysTime { };
		if (FileTimeToSystemTime(&ftTime, &SysTime))
			wstrTime = SystemTimeToString(&SysTime, true, true);
	}

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_TIME64T; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());
}

void CHexDlgDataInterpret::ShowNAME_FILETIME(QWORD qword)const
{
	std::wstring wstrTime = L"N/A";
	SYSTEMTIME SysTime { };
	if (FileTimeToSystemTime(reinterpret_cast<const FILETIME*>(&qword), &SysTime))
		wstrTime = SystemTimeToString(&SysTime, true, true);

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_FILETIME; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());
}

void CHexDlgDataInterpret::ShowNAME_OLEDATETIME(QWORD qword)const
{
	//OLE (including MS Office) date/time
	//Implemented using an 8-byte floating-point number. Days are represented as whole number increments starting with 30 December 1899, midnight as time zero.
	//See: https://docs.microsoft.com/en-us/cpp/atl-mfc-shared/date-type?view=vs-2019

	std::wstring wstrTime = L"N/A";

	DATE date;
	std::memcpy(&date, &qword, sizeof(date));
	COleDateTime dt(date);

	SYSTEMTIME SysTime { };
	if (dt.GetAsSystemTime(SysTime))
		wstrTime = SystemTimeToString(&SysTime, true, true);

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_OLEDATETIME; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());

}

void CHexDlgDataInterpret::ShowNAME_JAVATIME(QWORD qword)const
{
	//Javatime (signed)
	//Number of milliseconds after/before January 1, 1970, 00:00:00 UTC
	std::wstring wstrTime = L"N/A";

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

	SYSTEMTIME SysTime { };
	if (FileTimeToSystemTime(&ftJavaTime, &SysTime))
		wstrTime = SystemTimeToString(&SysTime, true, true);

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_JAVATIME; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());
}

void CHexDlgDataInterpret::ShowNAME_GUID(const UDQWORD& dqword)const
{
	wchar_t buff[64];
	swprintf_s(buff, std::size(buff), L"{%.8x-%.4x-%.4x-%.2x%.2x-%.2x%.2x%.2x%.2x%.2x%.2x}",
		dqword.gGUID.Data1, dqword.gGUID.Data2, dqword.gGUID.Data3, dqword.gGUID.Data4[0],
		dqword.gGUID.Data4[1], dqword.gGUID.Data4[2], dqword.gGUID.Data4[3], dqword.gGUID.Data4[4],
		dqword.gGUID.Data4[5], dqword.gGUID.Data4[6], dqword.gGUID.Data4[7]);

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_GUID; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);
}

void CHexDlgDataInterpret::ShowNAME_GUIDTIME(const UDQWORD& dqword)const
{
	//Guid v1 Datetime UTC
	//The time structure within the NAME_GUID.
	//First, verify GUID is actually version 1 style
	std::wstring wstrTime = L"N/A";
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
		ullSubtractTicks.QuadPart = static_cast<QWORD>(m_uFTTicksPerSec) * static_cast<QWORD>(m_uSecondsPerHour)
			* static_cast<QWORD>(m_uHoursPerDay) * static_cast<QWORD>(m_uFileTime1582OffsetDays);
		qwGUIDTime.QuadPart -= ullSubtractTicks.QuadPart;

		//Convert to SYSTEMTIME
		FILETIME ftGUIDTime;
		ftGUIDTime.dwHighDateTime = qwGUIDTime.HighPart;
		ftGUIDTime.dwLowDateTime = qwGUIDTime.LowPart;
		SYSTEMTIME SysTime { };
		if (FileTimeToSystemTime(&ftGUIDTime, &SysTime))
			wstrTime = SystemTimeToString(&SysTime, true, true);
	}

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_GUIDTIME; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());
}

void CHexDlgDataInterpret::ShowNAME_SYSTEMTIME(const UDQWORD& dqword)const
{
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_SYSTEMTIME; }); iter != m_vecProp.end())
		iter->pProp->SetValue(SystemTimeToString(reinterpret_cast<const SYSTEMTIME*>(&dqword), true, true).data());
}

bool CHexDlgDataInterpret::SetDataNAME_BINARY(const std::wstring& wstr)const
{
	if (wstr.size() != 8 || wstr.find_first_not_of(L"01") != std::wstring_view::npos)
		return false;

	bool fSuccess;
	UCHAR uchData;
	if (fSuccess = wstr2num(wstr, uchData, 2); fSuccess)
		SetDigitData(uchData);

	return fSuccess;
}

bool CHexDlgDataInterpret::SetDataNAME_CHAR(const std::wstring& wstr)const
{
	bool fSuccess;
	CHAR chData;
	if (fSuccess = wstr2num(wstr, chData); fSuccess)
		SetDigitData(chData);

	return fSuccess;
}

bool CHexDlgDataInterpret::SetDataNAME_UCHAR(const std::wstring& wstr)const
{
	bool fSuccess;
	UCHAR uchData;
	if (fSuccess = wstr2num(wstr, uchData); fSuccess)
		SetDigitData(uchData);

	return fSuccess;
}

bool CHexDlgDataInterpret::SetDataNAME_SHORT(const std::wstring& wstr)const
{
	bool fSuccess;
	SHORT shData;
	if (fSuccess = wstr2num(wstr, shData); fSuccess)
		SetDigitData(shData);

	return fSuccess;
}

bool CHexDlgDataInterpret::SetDataNAME_USHORT(const std::wstring& wstr)const
{
	bool fSuccess;
	USHORT ushData;
	if (fSuccess = wstr2num(wstr, ushData); fSuccess)
		SetDigitData(ushData);

	return fSuccess;
}

bool CHexDlgDataInterpret::SetDataNAME_LONG(const std::wstring& wstr)const
{
	bool fSuccess;
	LONG lData;
	if (fSuccess = wstr2num(wstr, lData); fSuccess)
		SetDigitData(lData);

	return fSuccess;
}

bool CHexDlgDataInterpret::SetDataNAME_ULONG(const std::wstring& wstr)const
{
	bool fSuccess;
	ULONG ulData;
	if (fSuccess = wstr2num(wstr, ulData); fSuccess)
		SetDigitData(ulData);

	return fSuccess;
}

bool CHexDlgDataInterpret::SetDataNAME_LONGLONG(const std::wstring& wstr)const
{
	bool fSuccess;
	LONGLONG llData;
	if (fSuccess = wstr2num(wstr, llData); fSuccess)
		SetDigitData(llData);

	return fSuccess;
}

bool CHexDlgDataInterpret::SetDataNAME_ULONGLONG(const std::wstring& wstr)const
{
	bool fSuccess;
	ULONGLONG ullData;
	if (fSuccess = wstr2num(wstr, ullData); fSuccess)
		SetDigitData(ullData);

	return fSuccess;
}

bool CHexDlgDataInterpret::SetDataNAME_FLOAT(const std::wstring& wstr)const
{
	bool fSuccess;
	float fl;
	if (fSuccess = wstr2num(wstr, fl); fSuccess)
		SetDigitData(fl);

	return fSuccess;
}

bool CHexDlgDataInterpret::SetDataNAME_DOUBLE(const std::wstring& wstr)const
{
	bool fSuccess;
	double dd;
	if (fSuccess = wstr2num(wstr, dd); fSuccess)
		SetDigitData(dd);

	return fSuccess;
}

bool CHexDlgDataInterpret::SetDataNAME_TIME32T(std::wstring_view wstr)const
{
	//The number of seconds since midnight January 1st 1970 UTC (32-bit). This wraps on 19 January 2038 
	SYSTEMTIME stTime;
	if (!StringToSystemTime(wstr, &stTime, true, true))
		return false;

	//Unix times are signed but value before 1st January 1970 is not considered valid
	//This is apparently because early complilers didn't support unsigned types. _mktime32() has the same limit
	if (stTime.wYear < 1970)
		return false;

	FILETIME ftTime;
	if (!SystemTimeToFileTime(&stTime, &ftTime))
		return false;

	//Convert ticks to seconds and adjust epoch
	LARGE_INTEGER lTicks;
	lTicks.HighPart = ftTime.dwHighDateTime;
	lTicks.LowPart = ftTime.dwLowDateTime;
	lTicks.QuadPart /= m_uFTTicksPerSec;
	lTicks.QuadPart -= m_ullUnixEpochDiff;

	if (lTicks.QuadPart < LONG_MAX)
	{
		LONG lTime32 = static_cast<LONG>(lTicks.QuadPart);

		if (m_fBigEndian)
			lTime32 = _byteswap_ulong(lTime32);

		m_pHexCtrl->SetData(m_ullOffset, lTime32);
	}

	return true;
}

bool CHexDlgDataInterpret::SetDataNAME_TIME64T(std::wstring_view wstr)const
{
	//The number of seconds since midnight January 1st 1970 UTC (32-bit). This wraps on 19 January 2038 
	SYSTEMTIME stTime;
	if (!StringToSystemTime(wstr, &stTime, true, true))
		return false;

	//Unix times are signed but value before 1st January 1970 is not considered valid
	//This is apparently because early complilers didn't support unsigned types. _mktime64() has the same limit
	if (stTime.wYear < 1970)
		return false;

	FILETIME ftTime;
	if (!SystemTimeToFileTime(&stTime, &ftTime))
		return false;

	//Convert ticks to seconds and adjust epoch
	LARGE_INTEGER lTicks;
	lTicks.HighPart = ftTime.dwHighDateTime;
	lTicks.LowPart = ftTime.dwLowDateTime;
	lTicks.QuadPart /= m_uFTTicksPerSec;
	lTicks.QuadPart -= m_ullUnixEpochDiff;
	auto llTime64 = static_cast<LONGLONG>(lTicks.QuadPart);

	if (m_fBigEndian)
		llTime64 = _byteswap_uint64(llTime64);

	m_pHexCtrl->SetData(m_ullOffset, llTime64);

	return true;
}

bool CHexDlgDataInterpret::SetDataNAME_FILETIME(std::wstring_view wstr)const
{
	SYSTEMTIME stTime;
	if (!StringToSystemTime(wstr, &stTime, true, true))
		return false;

	FILETIME ftTime;
	if (!SystemTimeToFileTime(&stTime, &ftTime))
		return false;

	ULARGE_INTEGER stLITime;
	stLITime.LowPart = ftTime.dwLowDateTime;
	stLITime.HighPart = ftTime.dwHighDateTime;

	if (m_fBigEndian)
		stLITime.QuadPart = _byteswap_uint64(stLITime.QuadPart);

	m_pHexCtrl->SetData(m_ullOffset, stLITime.QuadPart);

	return true;
}

bool CHexDlgDataInterpret::SetDataNAME_OLEDATETIME(std::wstring_view wstr)const
{
	SYSTEMTIME stTime;
	if (!StringToSystemTime(wstr, &stTime, true, true))
		return false;

	COleDateTime dt(stTime);
	if (dt.GetStatus() != COleDateTime::valid)
		return false;

	ULONGLONG ullValue;
	std::memcpy(&ullValue, &dt.m_dt, sizeof(dt.m_dt));

	if (m_fBigEndian)
		ullValue = _byteswap_uint64(ullValue);

	m_pHexCtrl->SetData(m_ullOffset, ullValue);

	return true;
}

bool CHexDlgDataInterpret::SetDataNAME_JAVATIME(std::wstring_view wstr)const
{
	SYSTEMTIME stTime;
	if (!StringToSystemTime(wstr, &stTime, true, true))
		return false;

	FILETIME ftTime;
	if (!SystemTimeToFileTime(&stTime, &ftTime))
		return false;

	//Number of milliseconds after/before January 1, 1970, 00:00:00 UTC
	LARGE_INTEGER lJavaTicks;
	lJavaTicks.HighPart = ftTime.dwHighDateTime;
	lJavaTicks.LowPart = ftTime.dwLowDateTime;

	LARGE_INTEGER lEpochTicks;
	lEpochTicks.HighPart = m_ulFileTime1970_HIGH;
	lEpochTicks.LowPart = m_ulFileTime1970_LOW;

	LONGLONG llDiffTicks;
	if (lEpochTicks.QuadPart > lJavaTicks.QuadPart)
		llDiffTicks = -(lEpochTicks.QuadPart - lJavaTicks.QuadPart);
	else
		llDiffTicks = lJavaTicks.QuadPart - lEpochTicks.QuadPart;

	LONGLONG llDiffMillis = llDiffTicks / m_uFTTicksPerMS;

	if (m_fBigEndian)
		llDiffMillis = _byteswap_uint64(llDiffMillis);

	m_pHexCtrl->SetData(m_ullOffset, llDiffMillis);

	return true;
}

bool CHexDlgDataInterpret::SetDataNAME_MSDOSTIME(std::wstring_view wstr)const
{
	SYSTEMTIME stTime;
	if (!StringToSystemTime(wstr, &stTime, true, true))
		return false;

	FILETIME ftTime;
	if (!SystemTimeToFileTime(&stTime, &ftTime))
		return false;

	UMSDOSDATETIME msdosDateTime;
	if (!FileTimeToDosDateTime(&ftTime, &msdosDateTime.TimeDate.wDate, &msdosDateTime.TimeDate.wTime))
		return false;

	//Note: Big-endian is not currently supported. This has never existed in the "wild".

	m_pHexCtrl->SetData(m_ullOffset, msdosDateTime.dwTimeDate);

	return true;
}

bool CHexDlgDataInterpret::SetDataNAME_MSDTTMTIME(std::wstring_view wstr)const
{
	SYSTEMTIME stTime;
	if (!StringToSystemTime(wstr, &stTime, true, true))
		return false;

	//Microsoft UDTTM time (as used by Microsoft Compound Document format)
	UDTTM dttm;
	dttm.components.year = stTime.wYear - 1900;
	dttm.components.month = stTime.wMonth;
	dttm.components.weekday = stTime.wDayOfWeek;
	dttm.components.dayofmonth = stTime.wDay;
	dttm.components.hour = stTime.wHour;
	dttm.components.minute = stTime.wMinute;

	//Note: Big-endian is not currently supported. This has never existed in the "wild".

	m_pHexCtrl->SetData(m_ullOffset, dttm.dwValue);

	return true;
}

bool CHexDlgDataInterpret::SetDataNAME_SYSTEMTIME(std::wstring_view wstr)const
{
	SYSTEMTIME stTime;
	if (!StringToSystemTime(wstr, &stTime, true, true))
		return false;

	//Note: Big-endian is not currently supported. This has never existed in the "wild".

	m_pHexCtrl->SetData(m_ullOffset, stTime);

	return true;
}

bool CHexDlgDataInterpret::SetDataNAME_GUIDTIME(std::wstring_view wstr)const
{
	//This time is within NAME_GUID structure, and it depends on it.
	//We can not just set a NAME_GUIDTIME for data range if it's not 
	//a valid NAME_GUID range, so checking first.

	auto dqword = m_pHexCtrl->GetData<UDQWORD>(m_ullOffset);
	unsigned short unGuidVersion = (dqword.gGUID.Data3 & 0xf000) >> 12;
	if (unGuidVersion != 1)
		return false;

	//RFC4122: The timestamp is a 60-bit value.  For UUID version 1, this is represented by Coordinated Universal Time (UTC) as a count of 100-
	//nanosecond intervals since 00:00:00.00, 15 October 1582 (the date of Gregorian reform to the Christian calendar).
	//
	//Both FILETIME and GUID time are based upon 100ns intervals
	//FILETIME is based upon 1 Jan 1601 whilst GUID time is from 1582. Add 6653 days to convert to GUID time
	SYSTEMTIME stTime;
	if (!StringToSystemTime(wstr, &stTime, true, true))
		return false;

	FILETIME ftTime;
	if (!SystemTimeToFileTime(&stTime, &ftTime))
		return false;

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

	m_pHexCtrl->SetData(m_ullOffset, dqword);

	return true;
}

bool CHexDlgDataInterpret::SetDataNAME_GUID(const std::wstring& wstr)const
{
	GUID guid;
	if (IIDFromString(wstr.data(), &guid) != S_OK)
		return false;

	m_pHexCtrl->SetData(m_ullOffset, guid);

	return true;
}