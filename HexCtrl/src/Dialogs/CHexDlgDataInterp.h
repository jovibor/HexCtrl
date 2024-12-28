/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../../HexCtrl.h"
#include <afxcontrolbars.h>

import HEXCTRL.HexUtility;

namespace HEXCTRL::INTERNAL {
	constexpr auto PROPGRID_MSG_CHANGESELECTION = 0x01U; //Notify codes for the parent.
	constexpr auto PROPGRID_MSG_PROPDATACHANGED = 0x02U;
	class CHexPropGridCtrl final : public CMFCPropertyGridCtrl {
	private:
		void OnChangeSelection(CMFCPropertyGridProperty* pNewProp, CMFCPropertyGridProperty* /*pOldProp*/)override {
			const auto uID = static_cast<UINT_PTR>(GetDlgCtrlID());
			const NMHDR hdr { .hwndFrom { reinterpret_cast<HWND>(pNewProp) }, .idFrom { uID },
				.code { PROPGRID_MSG_CHANGESELECTION } };
			GetParent()->SendMessageW(WM_NOTIFY, uID, reinterpret_cast<LPARAM>(&hdr));
		}
		void OnPropertyChanged(CMFCPropertyGridProperty* pProp)const override {
			const auto uID = static_cast<UINT_PTR>(GetDlgCtrlID());
			const NMHDR hdr { .hwndFrom { reinterpret_cast<HWND>(pProp) }, .idFrom { uID },
				.code { PROPGRID_MSG_PROPDATACHANGED } };
			GetParent()->SendMessageW(WM_NOTIFY, uID, reinterpret_cast<LPARAM>(&hdr));
		}
		void OnSize(UINT /*f*/, int /*cx*/, int /*cy*/) {
			EndEditItem();
			AdjustLayout();
		}
		DECLARE_MESSAGE_MAP();
	};

	//CHexDlgDataInterp.
	class CHexDlgDataInterp final {
	public:
		CHexDlgDataInterp();
		~CHexDlgDataInterp();
		void CreateDlg();
		[[nodiscard]] auto GetDlgItemHandle(EHexDlgItem eItem)const->HWND;
		[[nodiscard]] auto GetHglDataSize()const->DWORD;
		[[nodiscard]] auto GetHWND()const->HWND;
		void Initialize(IHexCtrl* pHexCtrl);
		[[nodiscard]] bool HasHighlight()const;
		[[nodiscard]] bool PreTranslateMsg(MSG* pMsg);
		[[nodiscard]] auto ProcessMsg(const MSG& msg) -> INT_PTR;
		void SetDlgProperties(std::uint64_t u64Flags);
		void ShowWindow(int iCmdShow);
		void UpdateData();
	private:
		union UMSDOSDateTime; //Forward declarations.
		union UDTTM;
		enum class EGroup : std::uint8_t;
		enum class EName : std::uint8_t;
		enum class EDataSize : std::uint8_t;
		struct GRIDDATA;
		[[nodiscard]] auto GetGridData(EName eName)const->const GRIDDATA*;
		[[nodiscard]] auto GetGridData(EName eName) -> GRIDDATA*; //Non-const overload.
		[[nodiscard]] bool IsBigEndian()const;
		[[nodiscard]] bool IsNoEsc()const;
		[[nodiscard]] bool IsShowAsHex()const;
		auto OnActivate(const MSG& msg) -> INT_PTR;
		void OnCancel();
		void OnCheckHex();
		void OnCheckBigEndian();
		auto OnClose() -> INT_PTR;
		auto OnCommand(const MSG& msg) -> INT_PTR;
		auto OnDestroy() -> INT_PTR;
		auto OnInitDialog(const MSG& msg) -> INT_PTR;
		auto OnNotify(const MSG& msg) -> INT_PTR;
		void OnOK();
		auto OnNotifyPropDataChanged(NMHDR* pNMHDR) -> INT_PTR;
		auto OnNotifyPropSelected(NMHDR* pNMHDR) -> INT_PTR;
		auto OnSize(const MSG& msg) -> INT_PTR;
		void RedrawHexCtrl()const;
		[[nodiscard]] bool SetDataBinary(std::wstring_view wsv)const;
		template<typename T> requires TSize1248<T>
		[[nodiscard]] bool SetDataNUMBER(std::wstring_view wsv)const;
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
		void SetGridRedraw(bool fRedraw);
		template <TSize1248 T> void SetTData(T tData)const;
		template <TSize1248 T> void ShowValueBinary(T tData)const;
		void ShowValueInt8(BYTE byte)const;
		void ShowValueUInt8(BYTE byte)const;
		void ShowValueInt16(WORD word)const;
		void ShowValueUInt16(WORD word)const;
		void ShowValueInt32(DWORD dword)const;
		void ShowValueUInt32(DWORD dword)const;
		void ShowValueInt64(QWORD qword)const;
		void ShowValueUInt64(QWORD qword)const;
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
	private:
		wnd::CWnd m_Wnd;              //Main window.
		wnd::CWndBtn m_WndBtnHex;     //Check-box "Hex numbers".
		wnd::CWndBtn m_WndBtnBE;      //Check-box "Big endian".
		wnd::CDynLayout m_DynLayout;
		std::vector<GRIDDATA> m_vecGrid;
		IHexCtrl* m_pHexCtrl { };
		CHexPropGridCtrl m_gridCtrl;
		ULONGLONG m_ullOffset { };
		std::uint64_t m_u64Flags { }; //Data from SetDlgProperties.
		DWORD m_dwHglDataSize { };    //Size of the data to highlight in the HexCtrl.
		DWORD m_dwDateFormat { };     //Date format.
		wchar_t m_wchDateSepar { };   //Date separator.
	};
}