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
	class CHexDlgDataInterpret final : public CDialogEx
	{
	private:
#pragma pack(push, 1)
		union MSDOSDATETIME //MS-DOS Date+Time structure (as used in FAT file system directory entry)
		{				 //See: https://msdn.microsoft.com/en-us/library/ms724274(v=vs.85).aspx
			struct
			{
				WORD wTime;	//Time component
				WORD wDate;	//Date component
			} TimeDate;
			DWORD dwTimeDate;
		};
		using PMSDOSDATETIME = MSDOSDATETIME*;
#pragma pack(pop)

		//Microsoft DTTM time (as used by Microsoft Compound Document format)
		//See: https://docs.microsoft.com/en-us/openspecs/office_file_formats/ms-doc/164c0c2e-6031-439e-88ad-69d00b69f414
#pragma pack(push, 1)
		union DTTM
		{
			struct
			{
				unsigned long minute : 6; //6+5+5+4+9+3=32
				unsigned long hour : 5;
				unsigned long dayofmonth : 5;
				unsigned long month : 4;
				unsigned long year : 9;
				unsigned long weekday : 3;
			} components;
			unsigned long dwValue;
		};
		using PDTTM = DTTM*;
#pragma pack(pop)

#pragma pack(push, 1)
		union DQWORD128
		{
			struct
			{
				QWORD qwLow;
				QWORD qwHigh;
			} Value;
			GUID gGUID;
		};
		using PDQWORD128 = DQWORD128*;
#pragma pack(pop)
		//Time calculation constants
		static inline const wchar_t* const arrNibbles [] {
			L"0000", L"0001", L"0010", L"0011", L"0100", L"0101", L"0110", L"0111",
			L"1000", L"1001", L"1010", L"1011", L"1100", L"1101", L"1110", L"1111" };
		static constexpr auto m_uFTTicksPerMS = 10000U;             //Number of 100ns intervals in a milli-second
		static constexpr auto m_uFTTicksPerSec = 10000000UL;        //Number of 100ns intervals in a second
		static constexpr auto m_uHoursPerDay = 24U;                 //24 hours per day
		static constexpr auto m_uSecondsPerHour = 3600U;            //3600 seconds per hour
		static constexpr auto m_uFileTime1582OffsetDays = 6653U;    //FILETIME is based upon 1 Jan 1601 whilst GUID time is from 15 Oct 1582. Add 6653 days to convert to GUID time
		static constexpr auto m_ulFileTime1970_LOW = 0xd53e8000UL;  //1st Jan 1970 as FILETIME
		static constexpr auto m_ulFileTime1970_HIGH = 0x019db1deUL; //Used for Unix and Java times
		static constexpr auto m_ullUnixEpochDiff = 11644473600ULL;  //Number of ticks from FILETIME epoch of 1st Jan 1601 to Unix epoch of 1st Jan 1970
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
		CString GetCurrentUserDateFormatString();
		CString SystemTimeToString(const SYSTEMTIME* pSysTime, bool bIncludeDate, bool bIncludeTime);
		bool StringToSystemTime(std::wstring_view wstrDateTime, PSYSTEMTIME pSysTime, bool bIncludeDate, bool bIncludeTime);
		template <typename T>bool SetDigitData(LONGLONG llData);
		bool StringToGuid(std::wstring_view wstrSource, GUID& GUIDResult);
		void ShowNAME_BINARY(BYTE byte);
		void ShowNAME_CHAR(BYTE byte);
		void ShowNAME_UCHAR(BYTE byte);
		void ShowNAME_SHORT(WORD word);
		void ShowNAME_USHORT(WORD word);
		void ShowNAME_LONG(DWORD dword);
		void ShowNAME_ULONG(DWORD dword);
		void ShowNAME_FLOAT(DWORD dword);
		void ShowNAME_TIME32(DWORD dword);
		void ShowNAME_MSDOSTIME(DWORD dword);
		void ShowNAME_MSDTTMTIME(DWORD word);
		void ShowNAME_LONGLONG(QWORD qword);
		void ShowNAME_ULONGLONG(QWORD qword);
		void ShowNAME_DOUBLE(QWORD qword);
		void ShowNAME_TIME64(QWORD qword);
		void ShowNAME_FILETIME(QWORD qword);
		void ShowNAME_OLEDATETIME(QWORD qword);
		void ShowNAME_JAVATIME(QWORD qword);
		void ShowNAME_GUID(const DQWORD128& dqword);
		void ShowNAME_GUIDTIME(const DQWORD128& dqword);
		void ShowNAME_SYSTEMTIME(const DQWORD128& dqword);
		bool SetDataNAME_BINARY(std::wstring_view wstr);
		bool SetDataNAME_CHAR(std::wstring_view wstr);
		bool SetDataNAME_UCHAR(std::wstring_view wstr);
		bool SetDataNAME_SHORT(std::wstring_view wstr);
		bool SetDataNAME_USHORT(std::wstring_view wstr);
		bool SetDataNAME_LONG(std::wstring_view wstr);
		bool SetDataNAME_ULONG(std::wstring_view wstr);
		bool SetDataNAME_LONGLONG(std::wstring_view wstr);
		bool SetDataNAME_ULONGLONG(std::wstring_view wstr);
		bool SetDataNAME_FLOAT(std::wstring_view wstr);
		bool SetDataNAME_DOUBLE(std::wstring_view wstr);
		bool SetDataNAME_TIME32T(std::wstring_view wstr);
		bool SetDataNAME_TIME64T(std::wstring_view wstr);
		bool SetDataNAME_FILETIME(std::wstring_view wstr);
		bool SetDataNAME_OLEDATETIME(std::wstring_view wstr);
		bool SetDataNAME_JAVATIME(std::wstring_view wstr);
		bool SetDataNAME_MSDOSTIME(std::wstring_view wstr);
		bool SetDataNAME_MSDTTMTIME(std::wstring_view wstr);
		bool SetDataNAME_SYSTEMTIME(std::wstring_view wstr);
		bool SetDataNAME_GUIDTIME(std::wstring_view wstr);
		bool SetDataNAME_GUID(std::wstring_view wstr);
		DECLARE_MESSAGE_MAP()
	private:
		enum class EGroup : WORD { DIGITS, FLOAT, TIME, MISC };
		enum class EName : WORD {
			NAME_BINARY, NAME_CHAR, NAME_UCHAR, NAME_SHORT, NAME_USHORT,
			NAME_LONG, NAME_ULONG, NAME_LONGLONG, NAME_ULONGLONG,
			NAME_FLOAT, NAME_DOUBLE, NAME_TIME32T, NAME_TIME64T,
			NAME_FILETIME, NAME_OLEDATETIME, NAME_JAVATIME, NAME_MSDOSTIME,
			NAME_MSDTTMTIME, NAME_SYSTEMTIME, NAME_GUIDTIME, NAME_GUID
		};
		enum class ESize : WORD { SIZE_BYTE = 0x1, SIZE_WORD = 0x2, SIZE_DWORD = 0x4, SIZE_QWORD = 0x8, SIZE_DQWORD = 0x10 };
		struct GRIDDATA
		{
			EGroup eGroup { };
			EName eName { };
			ESize eSize { };
			CMFCPropertyGridProperty* pProp { };
		};
	private:
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
		WCHAR m_wszDateSeparator[4] { };
	};

	template<typename T>
	inline bool CHexDlgDataInterpret::SetDigitData(LONGLONG llData)
	{
		//Do not check numeric_limits for sizeof(QWORD).
		//There is no sense for that, because input argument is LONGLONG,
		//and it can not be bigger than std::numeric_limits<(U)LONGLONG>::max()
		if constexpr (!std::is_same_v<T, ULONGLONG> && !std::is_same_v<T, LONGLONG>)
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