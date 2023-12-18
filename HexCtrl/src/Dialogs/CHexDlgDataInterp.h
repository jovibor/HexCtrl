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

namespace HEXCTRL::INTERNAL {
	constexpr auto WM_PROPGRID_PROPERTY_SELECTED = 0x0401U; //Message to a parent when new property is selected.
	class CHexPropGridCtrl final : public CMFCPropertyGridCtrl {
	public:
		void SetRedraw(bool fRedraw) {
			m_fRedraw = fRedraw;
		}
	private:
		void OnChangeSelection(CMFCPropertyGridProperty* pNewProp, CMFCPropertyGridProperty* /*pOldProp*/)override {
			GetParent()->SendMessageW(WM_PROPGRID_PROPERTY_SELECTED, GetDlgCtrlID(), reinterpret_cast<LPARAM>(pNewProp));
		}
		int OnDrawProperty(CDC* pDC, CMFCPropertyGridProperty* pProp)const override {
			return m_fRedraw ? CMFCPropertyGridCtrl::OnDrawProperty(pDC, pProp) : TRUE;
		}
		void OnSize(UINT /*f*/, int /*cx*/, int /*cy*/) {
			EndEditItem();
			AdjustLayout();
		}
		DECLARE_MESSAGE_MAP();
	private:
		bool m_fRedraw { true };
	};

	//CHexDlgDataInterp.
	class CHexDlgDataInterp final : public CDialogEx {
	public:
		CHexDlgDataInterp();
		~CHexDlgDataInterp();
		[[nodiscard]] auto GetDataSize()const->ULONGLONG;
		[[nodiscard]] auto GetDlgData()const->std::uint64_t;
		void Initialize(IHexCtrl* pHexCtrl);
		auto SetDlgData(std::uint64_t ullData) -> HWND;
		BOOL ShowWindow(int nCmdShow);
		void UpdateData();
	private:
		union UMSDOSDateTime; //Forward declarations.
		union UDTTM;
		enum class EGroup : std::uint8_t;
		enum class EName : std::uint8_t;
		enum class ESize : std::uint8_t;
		struct GRIDDATA;
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
		void SetRedraw(bool fRedraw);
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
		std::vector<GRIDDATA> m_vecProp;
		IHexCtrl* m_pHexCtrl { };
		CHexPropGridCtrl m_gridCtrl;
		CButton m_btnHex;            //Check-box "Hex numbers".
		CButton m_btnBE;             //Check-box "Big endian".
		ULONGLONG m_ullOffset { };
		ULONGLONG m_ullDataSize { }; //Size of the currently interpreted data.
		DWORD m_dwDateFormat { };    //Date format.
		wchar_t m_wchDateSepar { };  //Date separator.
	};
}