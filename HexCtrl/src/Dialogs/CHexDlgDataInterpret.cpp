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
#include "strsafe.h"
#include "../Helper.h"
#include <ctime>

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

BEGIN_MESSAGE_MAP(CHexDlgDataInterpret, CDialogEx)
	ON_WM_CLOSE()
	ON_WM_ACTIVATE()
END_MESSAGE_MAP()

void CHexDlgDataInterpret::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_HEXCTRL_DATAINTERPRET_EDIT_8SIGN, m_edit8sign);
	DDX_Control(pDX, IDC_HEXCTRL_DATAINTERPRET_EDIT_8UNSIGN, m_edit8unsign);
	DDX_Control(pDX, IDC_HEXCTRL_DATAINTERPRET_EDIT_16SIGN, m_edit16sign);
	DDX_Control(pDX, IDC_HEXCTRL_DATAINTERPRET_EDIT_16UNSIGN, m_edit16unsign);
	DDX_Control(pDX, IDC_HEXCTRL_DATAINTERPRET_EDIT_32SIGN, m_edit32sign);
	DDX_Control(pDX, IDC_HEXCTRL_DATAINTERPRET_EDIT_32UNSIGN, m_edit32unsign);
	DDX_Control(pDX, IDC_HEXCTRL_DATAINTERPRET_EDIT_64SIGN, m_edit64sign);
	DDX_Control(pDX, IDC_HEXCTRL_DATAINTERPRET_EDIT_64UNSIGN, m_edit64unsign);
	DDX_Control(pDX, IDC_HEXCTRL_DATAINTERPRET_EDIT_FLOAT, m_editFloat);
	DDX_Control(pDX, IDC_HEXCTRL_DATAINTERPRET_EDIT_DOUBLE, m_editDouble);
	DDX_Control(pDX, IDC_HEXCTRL_DATAINTERPRET_EDIT_TIME32, m_editTime32);
	DDX_Control(pDX, IDC_HEXCTRL_DATAINTERPRET_EDIT_TIME64, m_editTime64);
}

BOOL CHexDlgDataInterpret::Create(UINT nIDTemplate, CHexCtrl* pHexCtrl)
{
	if (!pHexCtrl)
		return FALSE;

	m_pHexCtrl = pHexCtrl;

	return CDialogEx::Create(nIDTemplate, pHexCtrl);
}

ULONGLONG CHexDlgDataInterpret::GetSize()
{
	return m_ullSize;
}

BOOL CHexDlgDataInterpret::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_vec64.push_back(&m_edit64sign);
	m_vec64.push_back(&m_edit64unsign);
	m_vec64.push_back(&m_editDouble);
	m_vec64.push_back(&m_editTime64);

	m_vec32.push_back(&m_edit32sign);
	m_vec32.push_back(&m_edit32unsign);
	m_vec32.push_back(&m_editFloat);
	m_vec32.push_back(&m_editTime32);
	std::copy(m_vec64.begin(), m_vec64.end(), std::back_inserter(m_vec32));

	m_vec16.push_back(&m_edit16sign);
	m_vec16.push_back(&m_edit16unsign);
	std::copy(m_vec32.begin(), m_vec32.end(), std::back_inserter(m_vec16));

	return TRUE;
}

void CHexDlgDataInterpret::InspectOffset(ULONGLONG ullOffset)
{
	if (!m_fVisible)
		return;

	ULONGLONG ullSize;
	m_pHexCtrl->GetData(&ullSize);
	if (ullOffset >= ullSize) //Out of data bounds.
		return;

	m_ullOffset = ullOffset;
	WCHAR buff[32];

	const BYTE byte = m_pHexCtrl->GetByte(ullOffset);
	swprintf_s(buff, 31, L"%hhi", (char)byte);
	m_edit8sign.SetWindowTextW(buff);
	swprintf_s(buff, 31, L"%hhu", byte);
	m_edit8unsign.SetWindowTextW(buff);

	if (ullOffset + sizeof(WORD) > ullSize)
	{
		for (auto i : m_vec16)
		{
			i->SetWindowTextW(L"");
			i->EnableWindow(FALSE);
		}
		return;
	}
	else
	{
		for (auto i : m_vec16)
			i->EnableWindow(TRUE);
	}

	const WORD word = m_pHexCtrl->GetWord(ullOffset);
	swprintf_s(buff, 31, L"%hi", (short)word);
	m_edit16sign.SetWindowTextW(buff);
	swprintf_s(buff, 31, L"%hu", word);
	m_edit16unsign.SetWindowTextW(buff);

	if (ullOffset + sizeof(DWORD) > ullSize)
	{
		for (auto i : m_vec32)
		{
			i->SetWindowTextW(L"");
			i->EnableWindow(FALSE);
		}
		return;
	}
	else
	{
		for (auto i : m_vec32)
			i->EnableWindow(TRUE);
	}

	const DWORD dword = m_pHexCtrl->GetDword(ullOffset);
	swprintf_s(buff, 31, L"%i", (int)dword);
	m_edit32sign.SetWindowTextW(buff);
	swprintf_s(buff, 31, L"%u", dword);
	m_edit32unsign.SetWindowTextW(buff);
	swprintf_s(buff, 31, L"%.9e", *((float*)&dword));
	m_editFloat.SetWindowTextW(buff);

	//Time32.
	std::wstring wstrTime;
	tm tm;
	if (_localtime32_s(&tm, (__time32_t*)&dword) == 0)
	{
		char str[32];
		strftime(str, 31, "%d/%m/%Y %H:%M:%S", &tm);
		wstrTime = StrToWstr(str).data();
	}
	else
		wstrTime = L"N/A";
	m_editTime32.SetWindowTextW(wstrTime.data());

	if (ullOffset + sizeof(QWORD) > ullSize)
	{
		for (auto i : m_vec64)
		{
			i->SetWindowTextW(L"");
			i->EnableWindow(FALSE);
		}
		return;
	}
	else
	{
		for (auto i : m_vec64)
			i->EnableWindow(TRUE);
	}

	const QWORD qword = m_pHexCtrl->GetQword(ullOffset);
	swprintf_s(buff, 31, L"%lli", (long long)qword);
	m_edit64sign.SetWindowTextW(buff);
	swprintf_s(buff, 31, L"%llu", qword);
	m_edit64unsign.SetWindowTextW(buff);
	swprintf_s(buff, 31, L"%.18e", *((double*)&qword));
	m_editDouble.SetWindowTextW(buff);

	//Time64.
	if (_localtime64_s(&tm, (__time64_t*)&qword) == 0)
	{
		char str[32];
		strftime(str, 31, "%d/%m/%Y %H:%M:%S", &tm);
		wstrTime = StrToWstr(str).data();
	}
	else
		wstrTime = L"N/A";
	m_editTime64.SetWindowTextW(wstrTime.data());
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

BOOL CHexDlgDataInterpret::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	if (wParam == HEXCTRL_EDITCTRL)
	{
		NMHDR* pnmh = (NMHDR*)lParam;
		switch (pnmh->code)
		{
		case VK_RETURN:
			m_dwCurrID = pnmh->idFrom;
			OnOK();
			break;
		case VK_ESCAPE:
			OnCancel();
			break;
		case VK_TAB:
			PostMessageW(WM_NEXTDLGCTL);
			break;
		case VK_UP:
			PostMessageW(WM_NEXTDLGCTL, TRUE);
			break;
		case VK_DOWN:
			PostMessageW(WM_NEXTDLGCTL);
			break;
		}
	}

	return CDialogEx::OnNotify(wParam, lParam, pResult);
}

void CHexDlgDataInterpret::UpdateHexCtrl()
{
	if (m_pHexCtrl)
		m_pHexCtrl->RedrawWindow();
}

BOOL CHexDlgDataInterpret::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (!m_fVisible)
		return CDialogEx::OnCommand(wParam, lParam);

	WORD wID = LOWORD(wParam);
	WORD wCode = HIWORD(wParam);

	if (wCode == EN_SETFOCUS)
	{
		switch (wID)
		{
		case IDC_HEXCTRL_DATAINTERPRET_EDIT_8SIGN:
		case IDC_HEXCTRL_DATAINTERPRET_EDIT_8UNSIGN:
			m_ullSize = 1;
			break;
		case IDC_HEXCTRL_DATAINTERPRET_EDIT_16SIGN:
		case IDC_HEXCTRL_DATAINTERPRET_EDIT_16UNSIGN:
			m_ullSize = 2;
			break;
		case IDC_HEXCTRL_DATAINTERPRET_EDIT_32SIGN:
		case IDC_HEXCTRL_DATAINTERPRET_EDIT_32UNSIGN:
		case IDC_HEXCTRL_DATAINTERPRET_EDIT_FLOAT:
		case IDC_HEXCTRL_DATAINTERPRET_EDIT_TIME32:
			m_ullSize = 4;
			break;
		case IDC_HEXCTRL_DATAINTERPRET_EDIT_64SIGN:
		case IDC_HEXCTRL_DATAINTERPRET_EDIT_64UNSIGN:
		case IDC_HEXCTRL_DATAINTERPRET_EDIT_DOUBLE:
		case IDC_HEXCTRL_DATAINTERPRET_EDIT_TIME64:
			m_ullSize = 8;
			break;
		}
		UpdateHexCtrl();
	}

	return CDialogEx::OnCommand(wParam, lParam);
}

void CHexDlgDataInterpret::OnClose()
{
	m_ullSize = 0;
	CDialogEx::OnClose();
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
	WCHAR buff[32];
	GetDlgItem((int)m_dwCurrID)->GetWindowTextW(buff, 31);
	LONGLONG llData;

	switch (m_dwCurrID)
	{
	case IDC_HEXCTRL_DATAINTERPRET_EDIT_8SIGN:
	case IDC_HEXCTRL_DATAINTERPRET_EDIT_8UNSIGN:
		if (!StrToInt64ExW(buff, STIF_SUPPORT_HEX, &llData))
		{
			MessageBoxW(L"Wrong number format!", L"Format Error", MB_ICONERROR);
			break;
		}
		m_pHexCtrl->SetByte(m_ullOffset, (BYTE)llData);
		break;
	case IDC_HEXCTRL_DATAINTERPRET_EDIT_16SIGN:
	case IDC_HEXCTRL_DATAINTERPRET_EDIT_16UNSIGN:
		if (!StrToInt64ExW(buff, STIF_SUPPORT_HEX, &llData))
		{
			MessageBoxW(L"Wrong number format!", L"Format Error", MB_ICONERROR);
			break;
		}
		m_pHexCtrl->SetWord(m_ullOffset, (WORD)llData);
		break;
	case IDC_HEXCTRL_DATAINTERPRET_EDIT_32SIGN:
	case IDC_HEXCTRL_DATAINTERPRET_EDIT_32UNSIGN:
		if (!StrToInt64ExW(buff, STIF_SUPPORT_HEX, &llData))
		{
			MessageBoxW(L"Wrong number format!", L"Format Error", MB_ICONERROR);
			break;
		}
		m_pHexCtrl->SetDword(m_ullOffset, (DWORD)llData);
		break;
	case IDC_HEXCTRL_DATAINTERPRET_EDIT_64SIGN:
	case IDC_HEXCTRL_DATAINTERPRET_EDIT_64UNSIGN:
		if (!StrToInt64ExW(buff, STIF_SUPPORT_HEX, &llData))
		{
			MessageBoxW(L"Wrong number format!", L"Format Error", MB_ICONERROR);
			break;
		}
		m_pHexCtrl->SetQword(m_ullOffset, (QWORD)llData);
		break;
	case IDC_HEXCTRL_DATAINTERPRET_EDIT_FLOAT:
	{
		wchar_t* pEndPtr;
		float fl = wcstof(buff, &pEndPtr);
		if (fl == 0 && (pEndPtr == buff || *pEndPtr != '\0'))
		{
			MessageBoxW(L"Wrong number format!", L"Format Error", MB_ICONERROR);
			break;
		}
		m_pHexCtrl->SetDword(m_ullOffset, *((DWORD*)&fl));
	}
	break;
	case IDC_HEXCTRL_DATAINTERPRET_EDIT_DOUBLE:
	{
		wchar_t* pEndPtr;
		double dd = wcstod(buff, &pEndPtr);
		if (dd == 0 && (pEndPtr == buff || *pEndPtr != '\0'))
		{
			MessageBoxW(L"Wrong number format!", L"Format Error", MB_ICONERROR);
			break;
		}
		m_pHexCtrl->SetQword(m_ullOffset, *((QWORD*)&dd));
	}
	break;
	case IDC_HEXCTRL_DATAINTERPRET_EDIT_TIME32:
	{
		tm tm { };
		int iYear { };
		swscanf_s(buff, L"%2i/%2i/%4i %2i:%2i:%2i", &tm.tm_mday, &tm.tm_mon, &iYear, &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
		tm.tm_year = iYear - 1900;
		--tm.tm_mon; //Months start from 0.
		__time32_t time32 = _mktime32(&tm);
		if (time32 == -1)
		{
			MessageBoxW(L"Wrong date/time format!", L"Format Error", MB_ICONERROR);
			break;
		}
		m_pHexCtrl->SetDword(m_ullOffset, (DWORD)time32);
	}
	break;
	case IDC_HEXCTRL_DATAINTERPRET_EDIT_TIME64:
	{
		tm tm { };
		int iYear { };
		swscanf_s(buff, L"%2i/%2i/%4i %2i:%2i:%2i", &tm.tm_mday, &tm.tm_mon, &iYear, &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
		tm.tm_year = iYear - 1900;
		--tm.tm_mon; //Months start from 0.
		__time64_t time64 = _mktime64(&tm);
		if (time64 == -1)
		{
			MessageBoxW(L"Wrong date/time format!", L"Format Error", MB_ICONERROR);
			break;
		}
		m_pHexCtrl->SetQword(m_ullOffset, (QWORD)time64);
	}
	break;
	}

	UpdateHexCtrl();
	InspectOffset(m_ullOffset);
}