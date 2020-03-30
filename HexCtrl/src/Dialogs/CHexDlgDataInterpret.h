/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#pragma once
#include "../../res/HexCtrlRes.h"
#include "../CHexCtrl.h"
#include "CHexPropGridCtrl.h"
#include <afxdialogex.h> //Standard MFC's controls header.

namespace HEXCTRL::INTERNAL
{
	#pragma pack(push, 1)
	typedef union _MSDOSDATETIME		//MS-DOS Date+Time structure (as used in FAT file system directory entry)
	{									//See: https://msdn.microsoft.com/en-us/library/ms724274(v=vs.85).aspx
		struct
		{
			WORD wTime;					//Time component
			WORD wDate;					//Date component
		} TimeDate;
		DWORD dwTimeDate;
	} MSDOSDATETIME, *PMSDOSDATETIME;
	#pragma pack(pop)

	//Microsoft DTTM time (as used by Microsoft Compound Document format)
	//See: https://docs.microsoft.com/en-us/openspecs/office_file_formats/ms-doc/164c0c2e-6031-439e-88ad-69d00b69f414
	#pragma pack(push, 1)
	typedef union _DTTM
	{
		struct
		{
			unsigned long minute : 6;		//6+5+5+4+9+3=32
			unsigned long hour : 5;
			unsigned long dayofmonth : 5;
			unsigned long month : 4;
			unsigned long year : 9;
			unsigned long weekday : 3;
		} components;
		unsigned long dwValue;
	} DTTM, *PDTTM;
	#pragma pack(pop)

	#pragma pack(push, 1)
	typedef union _DQWORD128
	{
		struct
		{
			QWORD qwLow;
			QWORD qwHigh;
		} Value;
		GUID gGUID;
	} DQWORD128, *PDQWORD128;
	#pragma pack(pop)

	class CHexDlgDataInterpret final : public CDialogEx
	{
	public:
		explicit CHexDlgDataInterpret(CWnd* pParent = nullptr) : CDialogEx(IDD_HEXCTRL_DATAINTERPRET, pParent) {}
		BOOL Create(UINT nIDTemplate, CHexCtrl* pHexCtrl);
		ULONGLONG GetSize();
		void InspectOffset(ULONGLONG ullOffset);
		BOOL ShowWindow(int nCmdShow);
	protected:
		void DoDataExchange(CDataExchange* pDX)override;
		BOOL OnInitDialog()override;
		afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		void OnOK()override;
		afx_msg void OnClose();
		BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)override;
		void UpdateHexCtrl();
		LRESULT OnPropertyChanged(WPARAM wparam, LPARAM lparam);
		afx_msg void OnSize(UINT nType, int cx, int cy);
		afx_msg void OnClickRadioLe();
		afx_msg void OnClickRadioBe();
		afx_msg void OnClickRadioDec();
		afx_msg void OnClickRadioHex();
		CString SystemTimeToString(PSYSTEMTIME pSysTime, bool bIncludeDate, bool bIncludeTime);
		bool StringToSystemTime(const CString sDateTime, PSYSTEMTIME pSysTime, bool bIncludeDate, bool bIncludeTime);
		template <typename T>bool SetDigitData(LONGLONG llData);
		bool StringToGuid(const wchar_t* pwszSource, LPGUID pGUIDResult);
		DECLARE_MESSAGE_MAP()
	private:
		enum class EGroup : WORD { DIGITS, FLOAT, TIME };
		enum class EName : WORD {
			NAME_BINARY,
			NAME_CHAR, NAME_UCHAR, NAME_SHORT, NAME_USHORT,
			NAME_LONG, NAME_ULONG, NAME_LONGLONG, NAME_ULONGLONG, NAME_GUID,
			NAME_FLOAT, NAME_DOUBLE, NAME_TIME32T, NAME_TIME64T,
			NAME_FILETIME, NAME_OLEDATETIME, NAME_JAVATIME, NAME_MSDOSTIME, NAME_MSDTTMTIME, NAME_SYSTEMTIME, NAME_GUIDTIME
		};
		enum class ESize : WORD { SIZE_BYTE = 0x1, SIZE_WORD = 0x2, SIZE_DWORD = 0x4, SIZE_QWORD = 0x8, SIZE_DQWORD = 0x10 };
		struct GRIDDATA
		{
			EGroup eGroup { };
			EName eName { };
			ESize eSize { };
			CMFCPropertyGridProperty* pProp { };
		};
		CHexCtrl* m_pHexCtrl { };
		bool m_fVisible { false };
		bool m_fBigEndian { false };
		bool m_fShowAsHex { false };
		CHexPropGridCtrl m_stCtrlGrid;
		std::vector<GRIDDATA> m_vecProp;
		CMFCPropertyGridProperty* m_pPropChanged { };
		ULONGLONG m_ullOffset { };
		ULONGLONG m_ullSize { };
		HDITEMW m_hdItemPropGrid { };
		DWORD m_dwDateFormat { };
		WCHAR m_wszDateSeparator[128] { };
	};

	template<typename T>
	inline bool CHexDlgDataInterpret::SetDigitData(LONGLONG llData)
	{
		if constexpr (!std::is_same_v<T, ULONGLONG> && !std::is_same_v<T, LONGLONG>) //Do not check numeric_limits for sizeof(QWORD)
		{
			if (llData > static_cast<LONGLONG>(std::numeric_limits<T>::max())
				|| llData < static_cast<LONGLONG>(std::numeric_limits<T>::min()))
				return false;
		}

		T tData = static_cast<T>(llData);
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
			}
		}
		m_pHexCtrl->SetData(m_ullOffset, tData);

		return true;
	}
}