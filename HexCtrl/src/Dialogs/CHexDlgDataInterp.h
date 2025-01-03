/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../../HexCtrl.h"
#include <commctrl.h>

import HEXCTRL.HexUtility;

namespace HEXCTRL::INTERNAL {
	class CHexDlgDataInterp final {
	public:
		CHexDlgDataInterp();
		~CHexDlgDataInterp();
		void CreateDlg();
		void DestroyDlg();
		[[nodiscard]] auto GetDlgItemHandle(EHexDlgItem eItem)const->HWND;
		[[nodiscard]] auto GetHglDataSize()const->DWORD;
		[[nodiscard]] auto GetHWND()const->HWND;
		void Initialize(IHexCtrl* pHexCtrl, HINSTANCE hInstRes);
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
		struct LISTDATA;
		[[nodiscard]] auto GetListData(EName eName)const->const LISTDATA*;
		[[nodiscard]] auto GetListData(EName eName) -> LISTDATA*; //Non-const overload.
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
		auto OnDrawItem(const MSG& msg) -> INT_PTR;
		auto OnInitDialog(const MSG& msg) -> INT_PTR;
		auto OnMeasureItem(const MSG& msg) -> INT_PTR;
		auto OnNotify(const MSG& msg) -> INT_PTR;
		void OnNotifyListEditBegin(NMHDR* pNMHDR);
		void OnNotifyListGetColor(NMHDR* pNMHDR);
		void OnNotifyListGetDispInfo(NMHDR* pNMHDR);
		void OnNotifyListItemChanged(NMHDR* pNMHDR);
		void OnNotifyListSetData(NMHDR* pNMHDR);
		void OnOK();
		auto OnSize(const MSG& msg) -> INT_PTR;
		void RedrawHexCtrl()const;
		[[nodiscard]] bool SetDataBinary(std::wstring_view wsv)const;
		template<typename T> requires ut::TSize1248<T>
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
		template <ut::TSize1248 T> void SetTData(T tData)const;
		template <ut::TSize1248 T> void ShowValueBinary(T tData);
		void ShowValueInt8(BYTE byte);
		void ShowValueUInt8(BYTE byte);
		void ShowValueInt16(WORD word);
		void ShowValueUInt16(WORD word);
		void ShowValueInt32(DWORD dword);
		void ShowValueUInt32(DWORD dword);
		void ShowValueInt64(std::uint64_t qword);
		void ShowValueUInt64(std::uint64_t qword);
		void ShowValueFloat(DWORD dword);
		void ShowValueDouble(std::uint64_t qword);
		void ShowValueTime32(DWORD dword);
		void ShowValueTime64(std::uint64_t qword);
		void ShowValueFILETIME(std::uint64_t qword);
		void ShowValueOLEDATETIME(std::uint64_t qword);
		void ShowValueJAVATIME(std::uint64_t qword);
		void ShowValueMSDOSTIME(DWORD dword);
		void ShowValueMSDTTMTIME(DWORD dword);
		void ShowValueSYSTEMTIME(SYSTEMTIME stSysTime);
		void ShowValueGUID(GUID stGUID);
		void ShowValueGUIDTIME(GUID stGUID);
	private:
		HINSTANCE m_hInstRes { };
		wnd::CWnd m_Wnd;              //Main window.
		wnd::CWndBtn m_WndBtnHex;     //Check-box "Hex numbers".
		wnd::CWndBtn m_WndBtnBE;      //Check-box "Big endian".
		wnd::CDynLayout m_DynLayout;
		std::vector<LISTDATA> m_vecData;
		IHexCtrl* m_pHexCtrl { };
		LISTEX::CListEx m_ListEx;
		ULONGLONG m_ullOffset { };
		std::uint64_t m_u64Flags { }; //Data from SetDlgProperties.
		DWORD m_dwHglDataSize { };    //Size of the data to highlight in the HexCtrl.
		DWORD m_dwDateFormat { };     //Date format.
		wchar_t m_wchDateSepar { };   //Date separator.
	};
}