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

BEGIN_MESSAGE_MAP(CHexDlgDataInterpret, CDialogEx)
	ON_WM_CLOSE()
	ON_WM_ACTIVATE()
	ON_WM_SIZE()
	ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, &CHexDlgDataInterpret::OnPropertyChanged)
	ON_COMMAND(IDC_HEXCTRL_DATAINTERPRET_RADIO_BE, &CHexDlgDataInterpret::OnHexctrlDatainterpretRadioBe)
	ON_COMMAND(IDC_HEXCTRL_DATAINTERPRET_RADIO_LE, &CHexDlgDataInterpret::OnHexctrlDatainterpretRadioLe)
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

	if (auto pRadio = (CButton*)GetDlgItem(IDC_HEXCTRL_DATAINTERPRET_RADIO_LE); pRadio)
		pRadio->SetCheck(1);

	m_hdItemPropGrid.mask = HDI_WIDTH;
	m_hdItemPropGrid.cxy = 120;
	m_stCtrlGrid.EnableHeaderCtrl(TRUE, L"Data type", L"Value");
	m_stCtrlGrid.GetHeaderCtrl().SetItem(0, &m_hdItemPropGrid); //Property grid column size.

	m_vecProp.emplace_back(GRIDDATA { EGroup::DIGITS, EName::NAME_CHAR, ESize::SIZE_BYTE, new CMFCPropertyGridProperty(L"char:", L"0") });
	m_vecProp.emplace_back(GRIDDATA { EGroup::DIGITS, EName::NAME_UCHAR, ESize::SIZE_BYTE, new CMFCPropertyGridProperty(L"unsigned char:", L"0") });
	m_vecProp.emplace_back(GRIDDATA { EGroup::DIGITS, EName::NAME_SHORT, ESize::SIZE_WORD, new CMFCPropertyGridProperty(L"short:", L"0") });
	m_vecProp.emplace_back(GRIDDATA { EGroup::DIGITS, EName::NAME_USHORT, ESize::SIZE_WORD, new CMFCPropertyGridProperty(L"unsigned short:", L"0") });
	m_vecProp.emplace_back(GRIDDATA { EGroup::DIGITS, EName::NAME_LONG, ESize::SIZE_DWORD, new CMFCPropertyGridProperty(L"long:", L"0") });
	m_vecProp.emplace_back(GRIDDATA { EGroup::DIGITS, EName::NAME_ULONG, ESize::SIZE_DWORD, new CMFCPropertyGridProperty(L"unsigned long:", L"0") });
	m_vecProp.emplace_back(GRIDDATA { EGroup::DIGITS, EName::NAME_LONGLONG, ESize::SIZE_QWORD, new CMFCPropertyGridProperty(L"long long:", L"0") });
	m_vecProp.emplace_back(GRIDDATA { EGroup::DIGITS, EName::NAME_ULONGLONG, ESize::SIZE_QWORD, new CMFCPropertyGridProperty(L"unsigned long long:", L"0") });
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

void CHexDlgDataInterpret::OnHexctrlDatainterpretRadioBe()
{
	m_fBigEndian = true;
	InspectOffset(m_ullOffset);
}

void CHexDlgDataInterpret::OnHexctrlDatainterpretRadioLe()
{
	m_fBigEndian = false;
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

	const BYTE byte = m_pHexCtrl->GetData<BYTE>(ullOffset);
	swprintf_s(buff, 31, L"%hhi", static_cast<char>(byte));
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const GRIDDATA& refData) {return refData.eName == EName::NAME_CHAR; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);

	swprintf_s(buff, 31, L"%hhu", byte);
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

	swprintf_s(buff, 31, L"%hi", static_cast<short>(word));
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const GRIDDATA& refData) {return refData.eName == EName::NAME_SHORT; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);

	swprintf_s(buff, 31, L"%hu", word);
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

	swprintf_s(buff, 31, L"%i", static_cast<int>(dword));
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const GRIDDATA& refData) {return refData.eName == EName::NAME_LONG; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);

	swprintf_s(buff, 31, L"%u", dword);
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const GRIDDATA& refData) {return refData.eName == EName::NAME_ULONG; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);

	swprintf_s(buff, 31, L"%.9e", *reinterpret_cast<const float*>(&dword));
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

	swprintf_s(buff, 31, L"%lli", static_cast<long long>(qword));
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const GRIDDATA& refData) {return refData.eName == EName::NAME_LONGLONG; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);

	swprintf_s(buff, 31, L"%llu", qword);
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const GRIDDATA& refData) {return refData.eName == EName::NAME_ULONGLONG; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);

	swprintf_s(buff, 31, L"%.18e", *reinterpret_cast<const double*>(&qword));
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const GRIDDATA& refData) {return refData.eName == EName::NAME_DOUBLE; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);

	//Time64.
	if (_localtime64_s(&tm, reinterpret_cast<const __time64_t*>(&qword)) == 0)
	{
		char str[32];
		strftime(str, 31, "%d/%m/%Y %H:%M:%S", &tm);
		wstrTime = StrToWstr(str);
	}
	else
		wstrTime = L"N/A";

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const GRIDDATA& refData) {return refData.eName == EName::NAME_TIME64T; }); iter != m_vecProp.end())
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