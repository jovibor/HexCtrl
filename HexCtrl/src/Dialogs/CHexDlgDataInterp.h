/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../../HexCtrl.h"
#include <afxcontrolbars.h>
#include <afxdialogex.h>

namespace HEXCTRL::INTERNAL
{
	constexpr auto WM_PROPGRID_PROPERTY_SELECTED = 0x0401U; //Message to parent when new property selected.
	class CHexPropGridCtrl final : public CMFCPropertyGridCtrl {
	private:
		void OnChangeSelection(CMFCPropertyGridProperty* pNewProp, CMFCPropertyGridProperty* /*pOldProp*/)override {
			GetParent()->SendMessageW(WM_PROPGRID_PROPERTY_SELECTED, GetDlgCtrlID(), reinterpret_cast<LPARAM>(pNewProp));
		}
		void OnSize(UINT /*f*/, int /*cx*/, int /*cy*/) {
			EndEditItem();
			AdjustLayout();
		}
		DECLARE_MESSAGE_MAP();
	};

	class CHexDlgDataInterp final : public CDialogEx {
	public:
		[[nodiscard]] auto GetDataSize()const->ULONGLONG;
		[[nodiscard]] auto GetDlgData()const->std::uint64_t;
		void Initialize(IHexCtrl* pHexCtrl);
		auto SetDlgData(std::uint64_t ullData) -> HWND;
		BOOL ShowWindow(int nCmdShow);
		void UpdateData();
	private:
	#pragma pack(push, 1)
		//MS-DOS Date+Time structure (as used in FAT file system directory entry)
		//See: https://msdn.microsoft.com/en-us/library/ms724274(v=vs.85).aspx
		union UMSDOSDATETIME {
			struct {
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
		union UDTTM {
			struct {
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
	private:
		void DoDataExchange(CDataExchange* pDX)override;
		[[nodiscard]] bool IsShowAsHex()const;
		[[nodiscard]] bool IsBigEndian()const;
		BOOL OnInitDialog()override;
		afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		afx_msg void OnOK()override;
		afx_msg void OnCheckHex();
		afx_msg void OnCheckBigEndian();
		afx_msg void OnClose();
		afx_msg void OnDestroy();
		LRESULT OnPropertyDataChanged(WPARAM wParam, LPARAM lParam);
		LRESULT OnPropertySelected(WPARAM wParam, LPARAM lParam);
		void RedrawHexCtrl()const;
		void ShowValueBinary(BYTE byte)const;
		void ShowValueChar(BYTE byte)const;
		void ShowValueUChar(BYTE byte)const;
		void ShowValueShort(WORD word)const;
		void ShowValueUShort(WORD word)const;
		void ShowValueInt(DWORD dword)const;
		void ShowValueUInt(DWORD dword)const;
		void ShowValueLL(QWORD qword)const;
		void ShowValueULL(QWORD qword)const;
		void ShowValueFloat(DWORD dword)const;
		void ShowValueDouble(QWORD qword)const;
		void ShowValueTime32(DWORD dword)const;
		void ShowValueTime64(QWORD qword)const;
		void ShowValueFILETIME(QWORD qword)const;
		void ShowValueOLEDATETIME(QWORD qword)const;
		void ShowValueJAVATIME(QWORD qword)const;
		void ShowValueMSDOSTIME(DWORD dword)const;
		void ShowValueMSDTTMTIME(DWORD dword)const;
		void ShowValueSYSTEMTIME(SYSTEMTIME stSysTime)const;
		void ShowValueGUID(GUID stGUID)const;
		void ShowValueGUIDTIME(GUID stGUID)const;
		template <typename T>
		void SetTData(T tData)const;
		[[nodiscard]] bool SetDataBinary(std::wstring_view wsv)const;
		[[nodiscard]] bool SetDataChar(std::wstring_view wsv)const;
		[[nodiscard]] bool SetDataUChar(std::wstring_view wsv)const;
		[[nodiscard]] bool SetDataShort(std::wstring_view wsv)const;
		[[nodiscard]] bool SetDataUShort(std::wstring_view wsv)const;
		[[nodiscard]] bool SetDataInt(std::wstring_view wsv)const;
		[[nodiscard]] bool SetDataUInt(std::wstring_view wsv)const;
		[[nodiscard]] bool SetDataLL(std::wstring_view wsv)const;
		[[nodiscard]] bool SetDataULL(std::wstring_view wsv)const;
		[[nodiscard]] bool SetDataFloat(std::wstring_view wsv)const;
		[[nodiscard]] bool SetDataDouble(std::wstring_view wsv)const;
		[[nodiscard]] bool SetDataTime32(std::wstring_view wsv)const;
		[[nodiscard]] bool SetDataTime64(std::wstring_view wsv)const;
		[[nodiscard]] bool SetDataFILETIME(std::wstring_view wsv)const;
		[[nodiscard]] bool SetDataOLEDATETIME(std::wstring_view wsv)const;
		[[nodiscard]] bool SetDataJAVATIME(std::wstring_view wsv)const;
		[[nodiscard]] bool SetDataMSDOSTIME(std::wstring_view wsv)const;
		[[nodiscard]] bool SetDataMSDTTMTIME(std::wstring_view wsv)const;
		[[nodiscard]] bool SetDataSYSTEMTIME(std::wstring_view wsv)const;
		[[nodiscard]] bool SetDataGUID(std::wstring_view wsv)const;
		[[nodiscard]] bool SetDataGUIDTIME(std::wstring_view wsv)const;
		DECLARE_MESSAGE_MAP();
	private:
		enum class EGroup : std::uint8_t { GR_INTEGRAL, GR_FLOAT, GR_TIME, GR_MISC, GR_GUIDTIME };
		enum class EName : std::uint8_t {
			NAME_BINARY, NAME_CHAR, NAME_UCHAR, NAME_SHORT, NAME_USHORT,
			NAME_INT, NAME_UINT, NAME_LONGLONG, NAME_ULONGLONG,
			NAME_FLOAT, NAME_DOUBLE, NAME_TIME32T, NAME_TIME64T,
			NAME_FILETIME, NAME_OLEDATETIME, NAME_JAVATIME, NAME_MSDOSTIME,
			NAME_MSDTTMTIME, NAME_SYSTEMTIME, NAME_GUIDTIME, NAME_GUID
		};
		enum class ESize : std::uint8_t {
			SIZE_BYTE = 0x1, SIZE_WORD = 0x2, SIZE_DWORD = 0x4,
			SIZE_QWORD = 0x8, SIZE_DQWORD = 0x10
		};
		struct SGRIDDATA {
			CMFCPropertyGridProperty* pProp { };
			EGroup eGroup { };
			EName eName { };
			ESize eSize { };
		};
		std::vector<SGRIDDATA> m_vecProp;
		IHexCtrl* m_pHexCtrl { };
		CHexPropGridCtrl m_stCtrlGrid;
		CButton m_btnHex;            //Check-box "Hex numbers".
		CButton m_btnBE;             //Check-box "Big endian".
		ULONGLONG m_ullOffset { };
		ULONGLONG m_ullDataSize { }; //Size of the currently interpreted data.
		DWORD m_dwDateFormat { };    //Date format.
		wchar_t m_wchDateSepar { };  //Date separator.
		bool m_fBigEndian { false };
		bool m_fShowAsHex { true };
	};
}