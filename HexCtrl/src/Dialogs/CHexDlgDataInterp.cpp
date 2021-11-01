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
#include "strsafe.h"
#include <algorithm>
#include <bit>
#include <cassert>

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

void CHexDlgDataInterp::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HEXCTRL_DATAINTERP_PROPDATA, m_stCtrlGrid);
}

BOOL CHexDlgDataInterp::Create(UINT nIDTemplate, CWnd* pParent, IHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	if (pHexCtrl == nullptr)
		return FALSE;

	m_pHexCtrl = pHexCtrl;

	return CDialogEx::Create(nIDTemplate, pParent);
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
	m_vecProp.emplace_back(new CMFCPropertyGridProperty(L"Float:", L"0"), EGroup::FLOAT, EName::NAME_FLOAT, ESize::SIZE_DWORD);
	m_vecProp.emplace_back(new CMFCPropertyGridProperty(L"Double:", L"0"), EGroup::FLOAT, EName::NAME_DOUBLE, ESize::SIZE_QWORD);
	m_vecProp.emplace_back(new CMFCPropertyGridProperty(L"GUID:", L"0"), EGroup::MISC, EName::NAME_GUID, ESize::SIZE_DQWORD);
	m_vecProp.emplace_back(new CMFCPropertyGridProperty(L"GUID v1 UTC time:", L"0"), EGroup::TIME, EName::NAME_GUIDTIME, ESize::SIZE_DQWORD, true);

	m_stCtrlGrid.EnableHeaderCtrl(TRUE, L"Data type", L"Value");
	HDITEMW hdItemPropGrid { };
	hdItemPropGrid.mask = HDI_WIDTH;
	hdItemPropGrid.cxy = 150;
	m_stCtrlGrid.GetHeaderCtrl().SetItem(0, &hdItemPropGrid); //Property grid column size.

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

void CHexDlgDataInterp::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if (nState == WA_INACTIVE)
	{
		m_ullSize = 0;
		UpdateHexCtrl();
	}

	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
}

void CHexDlgDataInterp::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	//Update dialog title to reflect current date format
	CString sTitle;
	GetWindowTextW(sTitle);
	sTitle.AppendFormat(L" [%s]", GetCurrentUserDateFormatString().data());
	SetWindowTextW(sTitle);

	if (bShow != FALSE)
		InspectOffset(m_pHexCtrl->GetCaretPos());
}

void CHexDlgDataInterp::OnOK()
{
	if (!m_pHexCtrl->IsMutable() || !m_pPropChanged)
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

	auto word = GetIHexTData<WORD>(*m_pHexCtrl, ullOffset);
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

	auto dword = GetIHexTData<DWORD>(*m_pHexCtrl, ullOffset);
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

	auto qword = GetIHexTData<QWORD>(*m_pHexCtrl, ullOffset);
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

	auto dqword = GetIHexTData<UDQWORD>(*m_pHexCtrl, ullOffset);
	if (m_fBigEndian)
	{
		//TODO: Test this thoroughly
		const auto tmp = dqword.Value.qwLow;
		dqword.Value.qwLow = _byteswap_uint64(dqword.Value.qwHigh);
		dqword.Value.qwHigh = _byteswap_uint64(tmp);
	}

	ShowNAME_GUID(dqword);
	ShowNAME_GUIDTIME(dqword);
	ShowNAME_SYSTEMTIME(dqword);
}

void CHexDlgDataInterp::OnClose()
{
	m_ullSize = 0;

	CDialogEx::OnClose();
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
			m_ullSize = static_cast<ULONGLONG>(pData->eSize);
			UpdateHexCtrl();
		}
	}

	return CDialogEx::OnNotify(wParam, lParam, pResult);
}

void CHexDlgDataInterp::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
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

void CHexDlgDataInterp::OnDestroy()
{
	CDialogEx::OnDestroy();

	m_stCtrlGrid.DestroyWindow();
	m_vecProp.clear();
}

ULONGLONG CHexDlgDataInterp::GetSize()const
{
	return m_ullSize;
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

void CHexDlgDataInterp::UpdateHexCtrl()const
{
	if (m_pHexCtrl && m_pHexCtrl->IsCreated())
		m_pHexCtrl->Redraw();
}

std::wstring CHexDlgDataInterp::GetCurrentUserDateFormatString()const
{
	std::wstring_view wstrFormat { };
	switch (m_pHexCtrl->GetDateInfo())
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
	swprintf_s(buff, std::size(buff), wstrFormat.data(), m_wDateSeparator, m_wDateSeparator);

	return buff;
}

std::wstring CHexDlgDataInterp::SystemTimeToString(const SYSTEMTIME& refSysTime)const
{
	if (refSysTime.wDay == 0 || refSysTime.wDay > 31 || refSysTime.wMonth == 0 || refSysTime.wMonth > 12
		|| refSysTime.wYear > 9999 || refSysTime.wHour > 23 || refSysTime.wMinute > 59 || refSysTime.wSecond > 59
		|| refSysTime.wMilliseconds > 999)
		return L"N/A";

	//Generate human formatted date. Fall back to UK/European if unable to determine
	WCHAR buff[32];
	switch (m_pHexCtrl->GetDateInfo())
	{
	case 0:	//0=Month-Day-Year
		swprintf_s(buff, std::size(buff), L"%.2d%s%.2d%s%.4d",
			refSysTime.wMonth, m_wDateSeparator, refSysTime.wDay, m_wDateSeparator, refSysTime.wYear);
		break;
	case 2:	//2=Year-Month-Day
		swprintf_s(buff, std::size(buff), L"%.4d%s%.2d%s%.2d",
			refSysTime.wYear, m_wDateSeparator, refSysTime.wMonth, m_wDateSeparator, refSysTime.wDay);
		break;
	default: //1=Day-Month-Year (default)
		swprintf_s(buff, std::size(buff), L"%.2d%s%.2d%s%.4d",
			refSysTime.wDay, m_wDateSeparator, refSysTime.wMonth, m_wDateSeparator, refSysTime.wYear);
	}

	std::wstring wstrRet = buff;
	wstrRet += L" ";

	//Append optional time elements
	swprintf_s(buff, std::size(buff), L"%.2d:%.2d:%.2d", refSysTime.wHour, refSysTime.wMinute, refSysTime.wSecond);
	wstrRet += buff;

	if (refSysTime.wMilliseconds > 0)
	{
		swprintf_s(buff, std::size(buff), L".%.3d", refSysTime.wMilliseconds);
		wstrRet += buff;
	}

	return wstrRet;
}

void CHexDlgDataInterp::ShowNAME_BINARY(BYTE byte)const
{
	WCHAR buff[32];
	swprintf_s(buff, std::size(buff), L"%s%s", arrNibbles[byte >> 4], arrNibbles[byte & 0x0F]);
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_BINARY; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);
}

void CHexDlgDataInterp::ShowNAME_CHAR(BYTE byte)const
{
	WCHAR buff[32];
	swprintf_s(buff, std::size(buff), m_fShowAsHex ? L"0x%hhX" : L"%hhi", static_cast<char>(byte));
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_CHAR; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);
}

void CHexDlgDataInterp::ShowNAME_UCHAR(BYTE byte)const
{
	WCHAR buff[32];
	swprintf_s(buff, std::size(buff), m_fShowAsHex ? L"0x%hhX" : L"%hhu", byte);
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_UCHAR; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);
}

void CHexDlgDataInterp::ShowNAME_SHORT(WORD word)const
{
	WCHAR buff[32];
	swprintf_s(buff, std::size(buff), m_fShowAsHex ? L"0x%hX" : L"%hi", static_cast<short>(word));
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_SHORT; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);
}

void CHexDlgDataInterp::ShowNAME_USHORT(WORD word)const
{
	WCHAR buff[32];
	swprintf_s(buff, std::size(buff), m_fShowAsHex ? L"0x%hX" : L"%hu", word);
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_USHORT; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);
}

void CHexDlgDataInterp::ShowNAME_LONG(DWORD dword)const
{
	WCHAR buff[32];
	swprintf_s(buff, std::size(buff), m_fShowAsHex ? L"0x%X" : L"%i", static_cast<int>(dword));
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_LONG; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);
}

void CHexDlgDataInterp::ShowNAME_ULONG(DWORD dword)const
{
	WCHAR buff[32];
	swprintf_s(buff, std::size(buff), m_fShowAsHex ? L"0x%X" : L"%u", dword);
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_ULONG; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);
}

void CHexDlgDataInterp::ShowNAME_FLOAT(DWORD dword)const
{
	WCHAR buff[32];
	swprintf_s(buff, std::size(buff), L"%.9e", std::bit_cast<float>(dword));
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_FLOAT; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);
}

void CHexDlgDataInterp::ShowNAME_TIME32(DWORD dword)const
{
	std::wstring wstrTime = L"N/A";

	//The number of seconds since midnight January 1st 1970 UTC (32-bit). This is signed and wraps on 19 January 2038 
	const auto lDiffSeconds = static_cast<LONG>(dword);

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
			wstrTime = SystemTimeToString(SysTime);
	}

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_TIME32T; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());
}

void CHexDlgDataInterp::ShowNAME_MSDOSTIME(DWORD dword)const
{
	std::wstring wstrTime = L"N/A";
	FILETIME ftMSDOS;
	UMSDOSDATETIME msdosDateTime;
	msdosDateTime.dwTimeDate = dword;
	if (DosDateTimeToFileTime(msdosDateTime.TimeDate.wDate, msdosDateTime.TimeDate.wTime, &ftMSDOS))
	{
		SYSTEMTIME SysTime { };
		if (FileTimeToSystemTime(&ftMSDOS, &SysTime))
			wstrTime = SystemTimeToString(SysTime);
	}

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_MSDOSTIME; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());
}

void CHexDlgDataInterp::ShowNAME_MSDTTMTIME(DWORD dword)const
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
		wstrTime = SystemTimeToString(SysTime);
	}

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_MSDTTMTIME; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());
}

void CHexDlgDataInterp::ShowNAME_LONGLONG(QWORD qword)const
{
	WCHAR buff[32];
	swprintf_s(buff, std::size(buff), m_fShowAsHex ? L"0x%llX" : L"%lli", static_cast<long long>(qword));
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_LONGLONG; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);
}

void CHexDlgDataInterp::ShowNAME_ULONGLONG(QWORD qword)const
{
	WCHAR buff[32];
	swprintf_s(buff, std::size(buff), m_fShowAsHex ? L"0x%llX" : L"%llu", qword);
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_ULONGLONG; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);
}

void CHexDlgDataInterp::ShowNAME_DOUBLE(QWORD qword)const
{
	WCHAR buff[32];
	swprintf_s(buff, std::size(buff), L"%.18e", std::bit_cast<double>(qword));
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_DOUBLE; }); iter != m_vecProp.end())
		iter->pProp->SetValue(buff);
}

void CHexDlgDataInterp::ShowNAME_TIME64(QWORD qword)const
{
	std::wstring wstrTime = L"N/A";

	//The number of seconds since midnight January 1st 1970 UTC (64-bit). This is signed
	const auto llDiffSeconds = static_cast<LONGLONG>(qword);

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
			wstrTime = SystemTimeToString(SysTime);
	}

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_TIME64T; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());
}

void CHexDlgDataInterp::ShowNAME_FILETIME(QWORD qword)const
{
	std::wstring wstrTime = L"N/A";
	SYSTEMTIME SysTime { };

	if (FileTimeToSystemTime(reinterpret_cast<const FILETIME*>(&qword), &SysTime))
		wstrTime = SystemTimeToString(SysTime);

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_FILETIME; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());
}

void CHexDlgDataInterp::ShowNAME_OLEDATETIME(QWORD qword)const
{
	//OLE (including MS Office) date/time
	//Implemented using an 8-byte floating-point number. Days are represented as whole number increments starting with 30 December 1899, midnight as time zero.
	//See: https://docs.microsoft.com/en-us/cpp/atl-mfc-shared/date-type?view=vs-2019

	std::wstring wstrTime = L"N/A";

	DATE date;
	std::memcpy(&date, &qword, sizeof(date));
	const COleDateTime dt(date);

	SYSTEMTIME SysTime { };
	if (dt.GetAsSystemTime(SysTime))
		wstrTime = SystemTimeToString(SysTime);

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_OLEDATETIME; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());

}

void CHexDlgDataInterp::ShowNAME_JAVATIME(QWORD qword)const
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
		wstrTime = SystemTimeToString(SysTime);

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_JAVATIME; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());
}

void CHexDlgDataInterp::ShowNAME_GUID(const UDQWORD& dqword)const
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

void CHexDlgDataInterp::ShowNAME_GUIDTIME(const UDQWORD& dqword)const
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
		SYSTEMTIME SysTime { };
		if (FileTimeToSystemTime(&ftGUIDTime, &SysTime))
			wstrTime = SystemTimeToString(SysTime);
	}

	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_GUIDTIME; }); iter != m_vecProp.end())
		iter->pProp->SetValue(wstrTime.data());
}

void CHexDlgDataInterp::ShowNAME_SYSTEMTIME(const UDQWORD& dqword)const
{
	if (auto iter = std::find_if(m_vecProp.begin(), m_vecProp.end(),
		[](const SGRIDDATA& refData) {return refData.eName == EName::NAME_SYSTEMTIME; }); iter != m_vecProp.end())
		iter->pProp->SetValue(SystemTimeToString(std::bit_cast<SYSTEMTIME>(dqword)).data());
}

bool CHexDlgDataInterp::SetDataNAME_BINARY(const std::wstring& wstr)const
{
	if (wstr.size() != 8 || wstr.find_first_not_of(L"01") != std::wstring_view::npos)
		return false;

	bool fSuccess;
	UCHAR uchData;
	if (fSuccess = wstr2num(wstr, uchData, 2); fSuccess)
		SetTData(uchData);

	return fSuccess;
}

bool CHexDlgDataInterp::SetDataNAME_CHAR(const std::wstring& wstr)const
{
	bool fSuccess;
	CHAR chData;
	if (fSuccess = wstr2num(wstr, chData); fSuccess)
		SetTData(chData);

	return fSuccess;
}

bool CHexDlgDataInterp::SetDataNAME_UCHAR(const std::wstring& wstr)const
{
	bool fSuccess;
	UCHAR uchData;
	if (fSuccess = wstr2num(wstr, uchData); fSuccess)
		SetTData(uchData);

	return fSuccess;
}

bool CHexDlgDataInterp::SetDataNAME_SHORT(const std::wstring& wstr)const
{
	bool fSuccess;
	SHORT shData;
	if (fSuccess = wstr2num(wstr, shData); fSuccess)
		SetTData(shData);

	return fSuccess;
}

bool CHexDlgDataInterp::SetDataNAME_USHORT(const std::wstring& wstr)const
{
	bool fSuccess;
	USHORT ushData;
	if (fSuccess = wstr2num(wstr, ushData); fSuccess)
		SetTData(ushData);

	return fSuccess;
}

bool CHexDlgDataInterp::SetDataNAME_LONG(const std::wstring& wstr)const
{
	bool fSuccess;
	LONG lData;
	if (fSuccess = wstr2num(wstr, lData); fSuccess)
		SetTData(lData);

	return fSuccess;
}

bool CHexDlgDataInterp::SetDataNAME_ULONG(const std::wstring& wstr)const
{
	bool fSuccess;
	ULONG ulData;
	if (fSuccess = wstr2num(wstr, ulData); fSuccess)
		SetTData(ulData);

	return fSuccess;
}

bool CHexDlgDataInterp::SetDataNAME_LONGLONG(const std::wstring& wstr)const
{
	bool fSuccess;
	LONGLONG llData;
	if (fSuccess = wstr2num(wstr, llData); fSuccess)
		SetTData(llData);

	return fSuccess;
}

bool CHexDlgDataInterp::SetDataNAME_ULONGLONG(const std::wstring& wstr)const
{
	bool fSuccess;
	ULONGLONG ullData;
	if (fSuccess = wstr2num(wstr, ullData); fSuccess)
		SetTData(ullData);

	return fSuccess;
}

bool CHexDlgDataInterp::SetDataNAME_FLOAT(const std::wstring& wstr)const
{
	bool fSuccess;
	float fl;
	if (fSuccess = wstr2num(wstr, fl); fSuccess)
		SetTData(fl);

	return fSuccess;
}

bool CHexDlgDataInterp::SetDataNAME_DOUBLE(const std::wstring& wstr)const
{
	bool fSuccess;
	double dd;
	if (fSuccess = wstr2num(wstr, dd); fSuccess)
		SetTData(dd);

	return fSuccess;
}

bool CHexDlgDataInterp::SetDataNAME_TIME32T(std::wstring_view wstr)const
{
	//The number of seconds since midnight January 1st 1970 UTC (32-bit). This wraps on 19 January 2038 
	const auto optSysTime = StringToSystemTime(wstr, m_pHexCtrl->GetDateInfo());
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

	if (lTicks.QuadPart < LONG_MAX)
	{
		LONG lTime32 = static_cast<LONG>(lTicks.QuadPart);

		if (m_fBigEndian)
			lTime32 = _byteswap_ulong(lTime32);

		SetIHexTData(*m_pHexCtrl, m_ullOffset, lTime32);
	}

	return true;
}

bool CHexDlgDataInterp::SetDataNAME_TIME64T(std::wstring_view wstr)const
{
	//The number of seconds since midnight January 1st 1970 UTC (32-bit). This wraps on 19 January 2038 
	const auto optSysTime = StringToSystemTime(wstr, m_pHexCtrl->GetDateInfo());
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
	auto llTime64 = static_cast<LONGLONG>(lTicks.QuadPart);

	if (m_fBigEndian)
		llTime64 = _byteswap_uint64(llTime64);

	SetIHexTData(*m_pHexCtrl, m_ullOffset, llTime64);

	return true;
}

bool CHexDlgDataInterp::SetDataNAME_FILETIME(std::wstring_view wstr)const
{
	const auto optFileTime = StringToFileTime(wstr, m_pHexCtrl->GetDateInfo());
	if (!optFileTime)
		return false;

	ULARGE_INTEGER stLITime;
	stLITime.LowPart = optFileTime.value().dwLowDateTime;
	stLITime.HighPart = optFileTime.value().dwHighDateTime;

	if (m_fBigEndian)
		stLITime.QuadPart = _byteswap_uint64(stLITime.QuadPart);

	SetIHexTData(*m_pHexCtrl, m_ullOffset, stLITime.QuadPart);

	return true;
}

bool CHexDlgDataInterp::SetDataNAME_OLEDATETIME(std::wstring_view wstr)const
{
	const auto optSysTime = StringToSystemTime(wstr, m_pHexCtrl->GetDateInfo());
	if (!optSysTime)
		return false;

	const COleDateTime dt(*optSysTime);
	if (dt.GetStatus() != COleDateTime::valid)
		return false;

	ULONGLONG ullValue;
	std::memcpy(&ullValue, &dt.m_dt, sizeof(dt.m_dt));

	if (m_fBigEndian)
		ullValue = _byteswap_uint64(ullValue);

	SetIHexTData(*m_pHexCtrl, m_ullOffset, ullValue);

	return true;
}

bool CHexDlgDataInterp::SetDataNAME_JAVATIME(std::wstring_view wstr)const
{
	const auto optFileTime = StringToFileTime(wstr, m_pHexCtrl->GetDateInfo());
	if (!optFileTime)
		return false;

	//Number of milliseconds after/before January 1, 1970, 00:00:00 UTC
	LARGE_INTEGER lJavaTicks;
	lJavaTicks.HighPart = optFileTime.value().dwHighDateTime;
	lJavaTicks.LowPart = optFileTime.value().dwLowDateTime;

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

	SetIHexTData(*m_pHexCtrl, m_ullOffset, llDiffMillis);

	return true;
}

bool CHexDlgDataInterp::SetDataNAME_MSDOSTIME(std::wstring_view wstr)const
{
	const auto optFileTime = StringToFileTime(wstr, m_pHexCtrl->GetDateInfo());
	if (!optFileTime)
		return false;

	UMSDOSDATETIME msdosDateTime;
	if (!FileTimeToDosDateTime(&*optFileTime, &msdosDateTime.TimeDate.wDate, &msdosDateTime.TimeDate.wTime))
		return false;

	//Note: Big-endian is not currently supported. This has never existed in the "wild".

	SetIHexTData(*m_pHexCtrl, m_ullOffset, msdosDateTime.dwTimeDate);

	return true;
}

bool CHexDlgDataInterp::SetDataNAME_MSDTTMTIME(std::wstring_view wstr)const
{
	const auto optSysTime = StringToSystemTime(wstr, m_pHexCtrl->GetDateInfo());
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

bool CHexDlgDataInterp::SetDataNAME_SYSTEMTIME(std::wstring_view wstr)const
{
	const auto optSysTime = StringToSystemTime(wstr, m_pHexCtrl->GetDateInfo());
	if (!optSysTime)
		return false;

	//Note: Big-endian is not currently supported. This has never existed in the "wild".

	SetIHexTData(*m_pHexCtrl, m_ullOffset, *optSysTime);

	return true;
}

bool CHexDlgDataInterp::SetDataNAME_GUIDTIME(std::wstring_view wstr)const
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
	const auto optSysTime = StringToSystemTime(wstr, m_pHexCtrl->GetDateInfo());
	if (!optSysTime)
		return false;

	FILETIME ftTime;
	if (!SystemTimeToFileTime(&*optSysTime, &ftTime))
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

	SetIHexTData(*m_pHexCtrl, m_ullOffset, dqword);

	return true;
}

bool CHexDlgDataInterp::SetDataNAME_GUID(const std::wstring& wstr)const
{
	GUID guid;
	if (IIDFromString(wstr.data(), &guid) != S_OK)
		return false;

	SetIHexTData(*m_pHexCtrl, m_ullOffset, guid);

	return true;
}