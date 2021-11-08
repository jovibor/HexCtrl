/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../../HexCtrl.h"
#include "CHexPropGridCtrl.h"
#include <afxcontrolbars.h>
#include <afxdialogex.h>

namespace HEXCTRL::INTERNAL
{
	class CHexDlgDataInterp final : public CDialogEx
	{
	public:
		BOOL Create(UINT nIDTemplate, CWnd* pParent, IHexCtrl* pHexCtrl);
		[[nodiscard]] ULONGLONG GetDataSize()const;
		void InspectOffset(ULONGLONG ullOffset);
	private:
#pragma pack(push, 1)
		union UMSDOSDATETIME //MS-DOS Date+Time structure (as used in FAT file system directory entry)
		{				    //See: https://msdn.microsoft.com/en-us/library/ms724274(v=vs.85).aspx
			struct
			{
				WORD wTime;	//Time component
				WORD wDate;	//Date component
			} TimeDate;
			DWORD dwTimeDate;
		};
		using PMSDOSDATETIME = UMSDOSDATETIME*;
#pragma pack(pop)

		//Microsoft UDTTM time (as used by Microsoft Compound Document format)
		//See: https://docs.microsoft.com/en-us/openspecs/office_file_formats/ms-doc/164c0c2e-6031-439e-88ad-69d00b69f414
#pragma pack(push, 1)
		union UDTTM
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
		using PDTTM = UDTTM*;
#pragma pack(pop)

#pragma pack(push, 1)
		union UDQWORD
		{
			struct
			{
				QWORD qwLow;
				QWORD qwHigh;
			} Value;
			GUID gGUID;
		};
		using PDQWORD = UDQWORD*;
#pragma pack(pop)
		//Time calculation constants
		static constexpr auto m_uFTTicksPerMS = 10000U;             //Number of 100ns intervals in a milli-second
		static constexpr auto m_uFTTicksPerSec = 10000000UL;        //Number of 100ns intervals in a second
		static constexpr auto m_uHoursPerDay = 24U;                 //24 hours per day
		static constexpr auto m_uSecondsPerHour = 3600U;            //3600 seconds per hour
		static constexpr auto m_uFileTime1582OffsetDays = 6653U;    //FILETIME is based upon 1 Jan 1601 whilst GUID time is from 15 Oct 1582. Add 6653 days to convert to GUID time
		static constexpr auto m_ulFileTime1970_LOW = 0xd53e8000UL;  //1st Jan 1970 as FILETIME
		static constexpr auto m_ulFileTime1970_HIGH = 0x019db1deUL; //Used for Unix and Java times
		static constexpr auto m_ullUnixEpochDiff = 11644473600ULL;  //Number of ticks from FILETIME epoch of 1st Jan 1601 to Unix epoch of 1st Jan 1970
	private:
		void DoDataExchange(CDataExchange* pDX)override;
		BOOL OnInitDialog()override;
		afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		afx_msg void OnOK()override;
		afx_msg void OnClickRadioBeLe();
		afx_msg void OnClickRadioHexDec();
		afx_msg void OnClose();
		afx_msg void OnDestroy();
		BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)override;
		LRESULT OnPropertyChanged(WPARAM wParam, LPARAM lParam);
		afx_msg void OnSize(UINT nType, int cx, int cy);
		template <typename T>
		void SetTData(T tData)const;
		[[nodiscard]] bool SetDataBINARY(const std::wstring& wstr)const;
		[[nodiscard]] bool SetDataCHAR(const std::wstring& wstr)const;
		[[nodiscard]] bool SetDataUCHAR(const std::wstring& wstr)const;
		[[nodiscard]] bool SetDataSHORT(const std::wstring& wstr)const;
		[[nodiscard]] bool SetDataUSHORT(const std::wstring& wstr)const;
		[[nodiscard]] bool SetDataLONG(const std::wstring& wstr)const;
		[[nodiscard]] bool SetDataULONG(const std::wstring& wstr)const;
		[[nodiscard]] bool SetDataLONGLONG(const std::wstring& wstr)const;
		[[nodiscard]] bool SetDataULONGLONG(const std::wstring& wstr)const;
		[[nodiscard]] bool SetDataFLOAT(const std::wstring& wstr)const;
		[[nodiscard]] bool SetDataDOUBLE(const std::wstring& wstr)const;
		[[nodiscard]] bool SetDataTIME32T(std::wstring_view wstr)const;
		[[nodiscard]] bool SetDataTIME64T(std::wstring_view wstr)const;
		[[nodiscard]] bool SetDataFILETIME(std::wstring_view wstr)const;
		[[nodiscard]] bool SetDataOLEDATETIME(std::wstring_view wstr)const;
		[[nodiscard]] bool SetDataJAVATIME(std::wstring_view wstr)const;
		[[nodiscard]] bool SetDataMSDOSTIME(std::wstring_view wstr)const;
		[[nodiscard]] bool SetDataMSDTTMTIME(std::wstring_view wstr)const;
		[[nodiscard]] bool SetDataSYSTEMTIME(std::wstring_view wstr)const;
		[[nodiscard]] bool SetDataGUIDTIME(std::wstring_view wstr)const;
		[[nodiscard]] bool SetDataGUID(const std::wstring& wstr)const;
		void ShowValueBINARY(BYTE byte)const;
		void ShowValueCHAR(BYTE byte)const;
		void ShowValueUCHAR(BYTE byte)const;
		void ShowValueSHORT(WORD word)const;
		void ShowValueUSHORT(WORD word)const;
		void ShowValueLONG(DWORD dword)const;
		void ShowValueULONG(DWORD dword)const;
		void ShowValueFLOAT(DWORD dword)const;
		void ShowValueTIME32(DWORD dword)const;
		void ShowValueMSDOSTIME(DWORD dword)const;
		void ShowValueMSDTTMTIME(DWORD dword)const;
		void ShowValueLONGLONG(QWORD qword)const;
		void ShowValueULONGLONG(QWORD qword)const;
		void ShowValueDOUBLE(QWORD qword)const;
		void ShowValueTIME64(QWORD qword)const;
		void ShowValueFILETIME(QWORD qword)const;
		void ShowValueOLEDATETIME(QWORD qword)const;
		void ShowValueJAVATIME(QWORD qword)const;
		void ShowValueGUID(const UDQWORD& dqword)const;
		void ShowValueGUIDTIME(const UDQWORD& dqword)const;
		void ShowValueSYSTEMTIME(const UDQWORD& dqword)const;
		void UpdateHexCtrl()const;
		DECLARE_MESSAGE_MAP()
	private:
		enum class EGroup : std::uint8_t { DIGITS, FLOAT, TIME, MISC };
		enum class EName : std::uint8_t
		{
			NAME_BINARY, NAME_CHAR, NAME_UCHAR, NAME_SHORT, NAME_USHORT,
			NAME_LONG, NAME_ULONG, NAME_LONGLONG, NAME_ULONGLONG,
			NAME_FLOAT, NAME_DOUBLE, NAME_TIME32T, NAME_TIME64T,
			NAME_FILETIME, NAME_OLEDATETIME, NAME_JAVATIME, NAME_MSDOSTIME,
			NAME_MSDTTMTIME, NAME_SYSTEMTIME, NAME_GUIDTIME, NAME_GUID
		};
		enum class ESize : std::uint8_t
		{
			SIZE_BYTE = 0x1, SIZE_WORD = 0x2, SIZE_DWORD = 0x4,
			SIZE_QWORD = 0x8, SIZE_DQWORD = 0x10
		};
		struct SGRIDDATA
		{
			CMFCPropertyGridProperty* pProp { };
			EGroup eGroup { };
			EName eName { };
			ESize eSize { };
			bool fChild { false };
		};
		std::vector<SGRIDDATA> m_vecProp;
		IHexCtrl* m_pHexCtrl { };
		CHexPropGridCtrl m_stCtrlGrid;
		CMFCPropertyGridProperty* m_pPropChanged { };
		ULONGLONG m_ullOffset { };
		ULONGLONG m_ullDataSize { }; //Size of the currently interpreted data.
		DWORD m_dwDateFormat { };    //Date format.
		wchar_t m_wchDateSepar { };  //Date separator.
		bool m_fBigEndian { false };
		bool m_fShowAsHex { true };
	};
}