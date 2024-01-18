/****************************************************************************************
* Copyright © 2018-2024 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../../HexCtrl.h"
#include <afxcontrolbars.h>
#include <afxdialogex.h>

namespace HEXCTRL::INTERNAL {
	constexpr auto WM_PROPGRID_PROPERTY_SELECTED = WM_USER + 0x1U; //Message to the parent, when new property is selected.
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

	//CHexDlgDataInterp.
	class CHexDlgDataInterp final : public CDialogEx {
	public:
		CHexDlgDataInterp();
		~CHexDlgDataInterp();
		[[nodiscard]] auto GetDlgData()const->std::uint64_t;
		[[nodiscard]] auto GetHglDataSize()const->DWORD;
		void Initialize(IHexCtrl* pHexCtrl);
		[[nodiscard]] bool HasHighlight()const;
		auto SetDlgData(std::uint64_t ullData, bool fCreate) -> HWND;
		BOOL ShowWindow(int nCmdShow);
		void UpdateData();
	private:
		union UMSDOSDateTime; //Forward declarations.
		union UDTTM;
		enum class EGroup : std::uint8_t;
		enum class EName : std::uint8_t;
		enum class ESize : std::uint8_t;
		struct GRIDDATA;
		void ApplyDlgData();
		void DoDataExchange(CDataExchange* pDX)override;
		[[nodiscard]] auto GetGridData(EName eName)const->const GRIDDATA*;
		[[nodiscard]] bool IsBigEndian()const;
		[[nodiscard]] bool IsNoEsc()const;
		[[nodiscard]] bool IsShowAsHex()const;
		afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		void OnCancel()override;
		afx_msg void OnCheckHex();
		afx_msg void OnCheckBigEndian();
		afx_msg void OnClose();
		afx_msg void OnDestroy();
		BOOL OnInitDialog()override;
		void OnOK()override;
		auto OnPropertyDataChanged(WPARAM wParam, LPARAM lParam) -> LRESULT;
		auto OnPropertySelected(WPARAM wParam, LPARAM lParam) -> LRESULT;
		void RedrawHexCtrl()const;
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
		DECLARE_MESSAGE_MAP();
	private:
		std::vector<GRIDDATA> m_vecGrid;
		IHexCtrl* m_pHexCtrl { };
		CHexPropGridCtrl m_gridCtrl;
		CButton m_btnHex;               //Check-box "Hex numbers".
		CButton m_btnBE;                //Check-box "Big endian".
		ULONGLONG m_ullOffset { };
		DWORD m_dwHglDataSize { };      //Size of the data to highlight in the HexCtrl.
		DWORD m_dwDateFormat { };       //Date format.
		std::uint64_t m_u64DlgData { }; //Data from SetDlgData.
		wchar_t m_wchDateSepar { };     //Date separator.
	};
}