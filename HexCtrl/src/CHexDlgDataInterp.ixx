/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
module;
#include <SDKDDKVer.h>
#include "../res/HexCtrlRes.h"
#include "../HexCtrl.h"
#include <Windows.h>
#include <commctrl.h>
#include <bit>
#include <format>
#include <limits>
export module HEXCTRL:CHexDlgDataInterp;

import :HexUtility;

namespace HEXCTRL::INTERNAL {
	class CHexDlgDataInterp final {
	public:
		void ClearData();
		void CreateDlg()const;
		void DestroyDlg();
		[[nodiscard]] auto GetDlgItemHandle(EHexDlgItem eItem)const -> HWND;
		[[nodiscard]] auto GetHighlightSize()const -> DWORD;
		[[nodiscard]] auto GetHWND()const -> HWND;
		void Initialize(IHexCtrl* pHexCtrl, HINSTANCE hInstRes);
		[[nodiscard]] bool HasHighlight()const;
		[[nodiscard]] bool PreTranslateMsg(MSG* pMsg);
		[[nodiscard]] auto ProcessMsg(const MSG& msg) -> INT_PTR;
		void SetDlgProperties(std::uint64_t u64Flags);
		void ShowWindow(int iCmdShow);
		void UpdateData();
	private:
		struct LISTDATA; union UMSDOSDateTime; union UDTTM; enum class EName : std::uint8_t;
		[[nodiscard]] auto GetCurrFieldName()const -> EName;
		[[nodiscard]] auto GetCurrFieldSize()const -> std::uint8_t;
		[[nodiscard]] auto GetFieldSize(EName eName)const -> std::uint8_t;
		[[nodiscard]] auto GetListData(EName eName) -> LISTDATA*; //Non-const overload.
		[[nodiscard]] auto GetListData(EName eName)const -> const LISTDATA*; //Const overload.
		[[nodiscard]] auto GetListData(int iItem) -> LISTDATA*;
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
		[[nodiscard]] bool SetDataASCII(std::wstring_view wsv)const;
		[[nodiscard]] bool SetDataUTF8(std::wstring_view wsv)const;
		[[nodiscard]] bool SetDataUTF16(std::wstring_view wsv)const;
		template <ut::TSize1248 T> void SetTData(T tData)const;
		void SetHighlightSize(std::uint32_t u32Size);
		void ShowValueBinary();
		void ShowValueInt8(SpanCByte spn);
		void ShowValueUInt8(SpanCByte spn);
		void ShowValueInt16(SpanCByte spn);
		void ShowValueUInt16(SpanCByte spn);
		void ShowValueInt32(SpanCByte spn);
		void ShowValueUInt32(SpanCByte spn);
		void ShowValueInt64(SpanCByte spn);
		void ShowValueUInt64(SpanCByte spn);
		void ShowValueFloat(SpanCByte spn);
		void ShowValueDouble(SpanCByte spn);
		void ShowValueTime32(SpanCByte spn);
		void ShowValueTime64(SpanCByte spn);
		void ShowValueFILETIME(SpanCByte spn);
		void ShowValueOLEDATETIME(SpanCByte spn);
		void ShowValueJAVATIME(SpanCByte spn);
		void ShowValueMSDOSTIME(SpanCByte spn);
		void ShowValueMSDTTMTIME(SpanCByte spn);
		void ShowValueSYSTEMTIME(SpanCByte spn);
		void ShowValueGUID(SpanCByte spn);
		void ShowValueGUIDTIME(SpanCByte spn);
		void ShowValueASCII(SpanCByte spn);
		void ShowValueUTF8(SpanCByte spn);
		void ShowValueUTF16(SpanCByte spn);
	private:
		HINSTANCE m_hInstRes { };
		GDIUT::CWnd m_Wnd;
		GDIUT::CWndBtn m_WndBtnHex; //Check-box "Hex numbers".
		GDIUT::CWndBtn m_WndBtnBE;  //Check-box "Big endian".
		GDIUT::CDynLayout m_DynLayout;
		LISTEX::CListEx m_ListEx;
		std::vector<LISTDATA> m_vecData;
		IHexCtrl* m_pHexCtrl { };
		ULONGLONG m_ullOffset { };
		std::uint64_t m_u64Flags { }; //Data from SetDlgProperties.
		DWORD m_dwHighlightSize { };  //Size of the data to highlight in the HexCtrl.
		DWORD m_dwDateFormat { };     //Date format.
		wchar_t m_wchDateSepar { };   //Date separator.
		EName m_eCurrField { };       //Currently selected field in the list.
	};
}

using namespace HEXCTRL::INTERNAL;

//MS-DOS Date+Time structure (as used in FAT file system directory entry).
//https://msdn.microsoft.com/en-us/library/ms724274(v=vs.85).aspx
union CHexDlgDataInterp::UMSDOSDateTime {
	struct {
		WORD wTime;	//Time component.
		WORD wDate;	//Date component.
	} TimeDate;
	DWORD dwTimeDate;
};

//Microsoft UDTTM time (as used by Microsoft Compound Document format).
//https://docs.microsoft.com/en-us/openspecs/office_file_formats/ms-doc/164c0c2e-6031-439e-88ad-69d00b69f414
union CHexDlgDataInterp::UDTTM {
	struct {
		unsigned long minute : 6; //6+5+5+4+9+3=32.
		unsigned long hour : 5;
		unsigned long dayofmonth : 5;
		unsigned long month : 4;
		unsigned long year : 9;
		unsigned long weekday : 3;
	} components;
	unsigned long dwValue;
};

enum class CHexDlgDataInterp::EName : std::uint8_t { //Members order equals m_vecData initialization order.
	NAME_BINARY = 0, NAME_INT8, NAME_UINT8, NAME_INT16, NAME_UINT16,
	NAME_INT32, NAME_UINT32, NAME_INT64, NAME_UINT64,
	NAME_FLOAT, NAME_DOUBLE, NAME_TIME32T, NAME_TIME64T,
	NAME_FILETIME, NAME_OLEDATETIME, NAME_JAVATIME, NAME_MSDOSTIME,
	NAME_MSDTTMTIME, NAME_SYSTEMTIME, NAME_GUIDTIME, NAME_GUID, NAME_ASCII, NAME_UTF8, NAME_UTF16
};

struct CHexDlgDataInterp::LISTDATA {
	std::wstring wstrTypeName;
	std::wstring wstrValue;
	EName        eName { };
	std::uint8_t u8Size { };
	bool         fAllowEdit { true };
};


void CHexDlgDataInterp::ClearData()
{
	if (!m_Wnd.IsWindow()) {
		return;
	}

	for (auto& field : m_vecData) {
		field.wstrValue.clear();
		field.fAllowEdit = false;
	}

	SetHighlightSize(0);
	m_ListEx.RedrawWindow();
}

void CHexDlgDataInterp::CreateDlg()const
{
	//m_Wnd is set in the OnInitDialog().
	if (const auto hWnd = ::CreateDialogParamW(m_hInstRes, MAKEINTRESOURCEW(IDD_HEXCTRL_DATAINTERP),
		m_pHexCtrl->GetWndHandle(EHexWnd::WND_MAIN), GDIUT::DlgProc<CHexDlgDataInterp>, reinterpret_cast<LPARAM>(this));
		hWnd == nullptr) {
		ut::DBG_REPORT(L"CreateDialogParamW failed.");
	}
}

void CHexDlgDataInterp::DestroyDlg()
{
	if (m_Wnd.IsWindow()) {
		m_Wnd.DestroyWindow();
	}
}

auto CHexDlgDataInterp::GetDlgItemHandle(EHexDlgItem eItem)const->HWND
{
	if (!m_Wnd.IsWindow()) {
		return { };
	}

	using enum EHexDlgItem;
	switch (eItem) {
	case DATAINTERP_CHK_HEX:
		return m_WndBtnHex;
	case DATAINTERP_CHK_BE:
		return m_WndBtnBE;
	default:
		return { };
	}
}

auto CHexDlgDataInterp::GetHighlightSize()const->DWORD
{
	return m_dwHighlightSize;
}

auto CHexDlgDataInterp::GetHWND()const->HWND
{
	return m_Wnd;
}

void CHexDlgDataInterp::Initialize(IHexCtrl* pHexCtrl, HINSTANCE hInstRes)
{
	if (pHexCtrl == nullptr || hInstRes == nullptr) {
		ut::DBG_REPORT(L"Initialize == nullptr");
		return;
	}

	m_pHexCtrl = pHexCtrl;
	m_hInstRes = hInstRes;
}

bool CHexDlgDataInterp::HasHighlight()const
{
	return GetHighlightSize() > 0;
}

bool CHexDlgDataInterp::PreTranslateMsg(MSG* pMsg)
{
	return m_Wnd.IsDlgMessage(pMsg);
}

auto CHexDlgDataInterp::ProcessMsg(const MSG& msg)->INT_PTR
{
	switch (msg.message) {
	case WM_ACTIVATE: return OnActivate(msg);
	case WM_COMMAND: return OnCommand(msg);
	case WM_CLOSE: return OnClose();
	case WM_DESTROY: return OnDestroy();
	case WM_DRAWITEM: return OnDrawItem(msg);
	case WM_INITDIALOG: return OnInitDialog(msg);
	case WM_MEASUREITEM: return OnMeasureItem(msg);
	case WM_NOTIFY: return OnNotify(msg);
	case WM_SIZE: return OnSize(msg);
	default:
		return 0;
	}
}

void CHexDlgDataInterp::SetDlgProperties(std::uint64_t u64Flags)
{
	m_u64Flags = u64Flags;
}

void CHexDlgDataInterp::ShowWindow(int iCmdShow)
{
	if (!m_Wnd.IsWindow()) {
		CreateDlg();
	}

	m_Wnd.ShowWindow(iCmdShow);
	UpdateData();
}

void CHexDlgDataInterp::UpdateData()
{
	if (!m_Wnd.IsWindow() || !m_Wnd.IsWindowVisible()) {
		return;
	}

	if (!m_pHexCtrl->IsDataSet()) {
		ClearData();
		return;
	}

	const auto ullOffset = m_ullOffset = m_pHexCtrl->GetCaretPos();
	const auto ullDataSize = m_pHexCtrl->GetDataSize();
	const auto dwlGetSize = ullOffset + 16 <= ullDataSize ? 16U : (ullOffset + 8 <= ullDataSize ?
		8U : (ullOffset + 4 <= ullDataSize ? 4U : (ullOffset + 3 <= ullDataSize ?
			3U : (ullOffset + 2 <= ullDataSize ? 2U : (ullOffset + 1 <= ullDataSize ? 1U : 0U)))));
	const auto spnData = m_pHexCtrl->GetData({ .ullOffset { ullOffset }, .ullSize { dwlGetSize } });
	ShowValueInt8(spnData);
	ShowValueUInt8(spnData);
	ShowValueInt16(spnData);
	ShowValueUInt16(spnData);
	ShowValueInt32(spnData);
	ShowValueUInt32(spnData);
	ShowValueFloat(spnData);
	ShowValueTime32(spnData);
	ShowValueMSDOSTIME(spnData);
	ShowValueMSDTTMTIME(spnData);
	ShowValueInt64(spnData);
	ShowValueUInt64(spnData);
	ShowValueDouble(spnData);
	ShowValueTime64(spnData);
	ShowValueFILETIME(spnData);
	ShowValueOLEDATETIME(spnData);
	ShowValueJAVATIME(spnData);
	ShowValueSYSTEMTIME(spnData);
	ShowValueGUID(spnData);
	ShowValueGUIDTIME(spnData);
	ShowValueASCII(spnData);
	ShowValueUTF8(spnData);
	ShowValueUTF16(spnData);
	ShowValueBinary();
	m_ListEx.RedrawWindow();
}


//Private methods.

auto CHexDlgDataInterp::GetCurrFieldName()const->EName
{
	return m_eCurrField;
}

auto CHexDlgDataInterp::GetCurrFieldSize()const->std::uint8_t
{
	return GetFieldSize(GetCurrFieldName());
}

auto CHexDlgDataInterp::GetFieldSize(EName eName)const->std::uint8_t
{
	return GetListData(eName)->u8Size;
}

auto CHexDlgDataInterp::GetListData(EName eName)->LISTDATA*
{
	return &m_vecData[static_cast<std::uint8_t>(eName)];
}

auto CHexDlgDataInterp::GetListData(EName eName)const->const LISTDATA*
{
	return &m_vecData[static_cast<std::uint8_t>(eName)];
}

auto CHexDlgDataInterp::GetListData(int iItem)->LISTDATA*
{
	return GetListData(static_cast<EName>(iItem));
}

bool CHexDlgDataInterp::IsBigEndian()const
{
	return m_WndBtnBE.IsChecked();
}

bool CHexDlgDataInterp::IsNoEsc()const
{
	return m_u64Flags & HEXCTRL_FLAG_DLG_NOESC;
}

bool CHexDlgDataInterp::IsShowAsHex()const
{
	return m_WndBtnHex.IsChecked();
}

auto CHexDlgDataInterp::OnActivate(const MSG& msg)->INT_PTR
{
	if (m_pHexCtrl == nullptr || !m_pHexCtrl->IsCreated())
		return FALSE;

	const auto nState = LOWORD(msg.wParam);
	if (nState == WA_ACTIVE || nState == WA_CLICKACTIVE) {
		const auto [dwFormat, wchSepar] = m_pHexCtrl->GetDateInfo();
		m_dwDateFormat = dwFormat;
		m_wchDateSepar = wchSepar;
		const auto wstrTitle = L"Date/Time format is: " + ut::GetDateFormatString(m_dwDateFormat, m_wchDateSepar);
		m_Wnd.SetWndText(wstrTitle); //Update dialog title to reflect current date format.
		UpdateData();
	}
	else {
		SetHighlightSize(0); //Remove data highlighting when dialog window is inactive.
		RedrawHexCtrl();
	}

	return FALSE; //Default handler.
}

void CHexDlgDataInterp::OnCancel()
{
	if (IsNoEsc()) //Not closing Dialog on Escape key.
		return;

	OnClose();
}

void CHexDlgDataInterp::OnCheckHex()
{
	UpdateData();
}

void CHexDlgDataInterp::OnCheckBigEndian()
{
	UpdateData();
	SetHighlightSize(GetCurrFieldSize());
	RedrawHexCtrl();
}

auto CHexDlgDataInterp::OnClose()->INT_PTR
{
	ShowWindow(SW_HIDE);
	return TRUE;
}

auto CHexDlgDataInterp::OnCommand(const MSG& msg)->INT_PTR
{
	const auto uCtrlID = LOWORD(msg.wParam);
	const auto uCode = HIWORD(msg.wParam);   //Control code, zero for menu.

	if (uCode != BN_CLICKED) { return FALSE; }
	switch (uCtrlID) {
	case IDCANCEL: OnCancel(); break;
	case IDC_HEXCTRL_DATAINTERP_CHK_HEX: OnCheckHex(); break;
	case IDC_HEXCTRL_DATAINTERP_CHK_BE: OnCheckBigEndian(); break;
	default:
		return FALSE;
	}
	return TRUE;
}

auto CHexDlgDataInterp::OnDestroy()->INT_PTR
{
	m_vecData.clear();
	m_u64Flags = { };
	m_pHexCtrl = nullptr;
	m_DynLayout.RemoveAll();

	return TRUE;
}

auto CHexDlgDataInterp::OnDrawItem(const MSG& msg)->INT_PTR
{
	const auto pDIS = reinterpret_cast<LPDRAWITEMSTRUCT>(msg.lParam);
	if (pDIS->CtlID == static_cast<UINT>(IDC_HEXCTRL_DATAINTERP_LIST)) {
		m_ListEx.DrawItem(pDIS);
	}

	return TRUE;
}

auto CHexDlgDataInterp::OnInitDialog(const MSG& msg)->INT_PTR
{
	m_Wnd.Attach(msg.hwnd);
	m_WndBtnHex.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_DATAINTERP_CHK_HEX));
	m_WndBtnBE.Attach(m_Wnd.GetDlgItem(IDC_HEXCTRL_DATAINTERP_CHK_BE));
	m_WndBtnHex.SetCheck(true);
	m_WndBtnBE.SetCheck(false);

	using enum EName;
	//The order of data initialization follows exactly EName members order,
	//to easily reference the vecor later as vector[eName], for the best performance.
	std::vector<LISTDATA> vecData {
		{ L"Binary:", L"", NAME_BINARY, 0U }, { L"int8_t:", L"", NAME_INT8, 1U },
		{ L"uint8_t:", L"", NAME_UINT8, 1U }, { L"int16_t:", L"", NAME_INT16, 2U },
		{ L"uint16_t:", L"", NAME_UINT16, 2U }, { L"int32_t:", L"", NAME_INT32, 4U },
		{ L"uint32_t:", L"", NAME_UINT32, 4U }, { L"int64_t:", L"", NAME_INT64, 8U },
		{ L"uint64_t:", L"", NAME_UINT64, 8U }, { L"float:", L"", NAME_FLOAT, 4U },
		{ L"double:", L"", NAME_DOUBLE, 8U }, { L"time32_t:", L"", NAME_TIME32T, 4U },
		{ L"time64_t:", L"", NAME_TIME64T, 8U }, { L"FILETIME:", L"", NAME_FILETIME, 8U },
		{ L"OLE time:", L"", NAME_OLEDATETIME, 8U }, { L"Java time:", L"", NAME_JAVATIME, 8U },
		{ L"MS-DOS time:", L"", NAME_MSDOSTIME, 8U }, { L"MS-UDTTM time:", L"", NAME_MSDTTMTIME, 4U },
		{ L"Windows SYSTEMTIME:", L"", NAME_SYSTEMTIME, 16U }, { L"GUID v1 UTC time:", L"", NAME_GUIDTIME, 16U },
		{ L"GUID:", L"", NAME_GUID, 16U }, { L"ASCII character:", L"", NAME_ASCII, 1U },
		{ L"UTF-8 code point:", L"", NAME_UTF8, 0U }, { L"UTF-16 code point:", L"", NAME_UTF16, 0U }
	};
	m_vecData = std::move(vecData);

	m_ListEx.Create({ .hWndParent { m_Wnd }, .uID { IDC_HEXCTRL_DATAINTERP_LIST }, .fDialogCtrl { true } });
	m_ListEx.SetExtendedStyle(LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT);
	m_ListEx.InsertColumn(0, L"Data type", LVCFMT_LEFT, 143);
	m_ListEx.InsertColumn(1, L"Value", LVCFMT_LEFT, 240, -1, LVCFMT_LEFT, true);
	m_ListEx.SetItemCountEx(static_cast<int>(m_vecData.size()), LVSICF_NOSCROLL);

	m_DynLayout.SetHost(m_Wnd);
	m_DynLayout.AddItem(IDC_HEXCTRL_DATAINTERP_LIST, GDIUT::CDynLayout::MoveNone(), GDIUT::CDynLayout::SizeHorzAndVert(100, 100));
	m_DynLayout.AddItem(IDC_HEXCTRL_DATAINTERP_CHK_HEX, GDIUT::CDynLayout::MoveVert(100), GDIUT::CDynLayout::SizeNone());
	m_DynLayout.AddItem(IDC_HEXCTRL_DATAINTERP_CHK_BE, GDIUT::CDynLayout::MoveVert(100), GDIUT::CDynLayout::SizeNone());
	m_DynLayout.Enable(true);

	return TRUE;
}

auto CHexDlgDataInterp::OnMeasureItem(const MSG& msg)->INT_PTR
{
	const auto pMIS = reinterpret_cast<LPMEASUREITEMSTRUCT>(msg.lParam);
	if (pMIS->CtlID == static_cast<UINT>(IDC_HEXCTRL_DATAINTERP_LIST)) {
		m_ListEx.MeasureItem(pMIS);
	}

	return TRUE;
}

auto CHexDlgDataInterp::OnNotify(const MSG& msg)->INT_PTR
{
	const auto pNMHDR = reinterpret_cast<NMHDR*>(msg.lParam);
	switch (pNMHDR->idFrom) {
	case IDC_HEXCTRL_DATAINTERP_LIST:
		switch (pNMHDR->code) {
		case LVN_GETDISPINFOW: OnNotifyListGetDispInfo(pNMHDR); break;
		case LVN_ITEMCHANGED:
		case NM_CLICK: OnNotifyListItemChanged(pNMHDR); break;
		case LISTEX::LISTEX_MSG_EDITBEGIN: OnNotifyListEditBegin(pNMHDR); break;
		case LISTEX::LISTEX_MSG_GETCOLOR: OnNotifyListGetColor(pNMHDR); break;
		case LISTEX::LISTEX_MSG_SETDATA: OnNotifyListSetData(pNMHDR); break;
		default: break;
		}
	default: break;
	}

	return TRUE;
}

void CHexDlgDataInterp::OnNotifyListEditBegin(NMHDR* pNMHDR)
{
	const auto pLDI = reinterpret_cast<LISTEX::PLISTEXDATAINFO>(pNMHDR);
	const auto iItem = pLDI->iItem;
	const auto iSubItem = pLDI->iSubItem;
	if (iItem < 0 || iSubItem < 0 || iItem >= static_cast<int>(m_vecData.size()))
		return;

	const auto pField = GetListData(iItem);
	pLDI->fAllowEdit = m_pHexCtrl->IsDataSet() ? pField->fAllowEdit : false;
	if (pLDI->fAllowEdit && (pField->eName == EName::NAME_UTF8 || pField->eName == EName::NAME_UTF16)) {
		GDIUT::CWndEdit(pLDI->hWndEdit).SetCueBanner(L"Enter Unicode character:", true);
		if (const auto u = pField->wstrValue.find_first_of(L" U+"); u != std::wstring::npos) {
			pLDI->pwszData[u] = 0; //Null terminating the buffer to not show "U+" part in the edit box.
		}
	}
}

void CHexDlgDataInterp::OnNotifyListGetColor(NMHDR* pNMHDR)
{
	const auto pLCI = reinterpret_cast<LISTEX::PLISTEXCOLORINFO>(pNMHDR);
	const auto iItem = pLCI->iItem;
	const auto iSubItem = pLCI->iSubItem;
	if (iItem < 0 || iSubItem < 0)
		return;

	const auto pField = GetListData(iItem);

	if (pField->eName == EName::NAME_BINARY) {
		pLCI->stClr.clrBk = RGB(240, 255, 255);
		return;
	}

	if (!pField->fAllowEdit) {
		pLCI->stClr.clrBk = RGB(245, 245, 245);
		return;
	}
}

void CHexDlgDataInterp::OnNotifyListGetDispInfo(NMHDR* pNMHDR)
{
	const auto pDispInfo = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
	const auto pItem = &pDispInfo->item;
	if ((pItem->mask & LVIF_TEXT) == 0)
		return;

	const auto iItem = pItem->iItem;
	switch (pItem->iSubItem) {
	case 0: //Data type.
		pItem->pszText = GetListData(iItem)->wstrTypeName.data();
		break;
	case 1: //Value.
		pItem->pszText = GetListData(iItem)->wstrValue.data();
		break;
	default: break;
	}
}

void CHexDlgDataInterp::OnNotifyListItemChanged(NMHDR* pNMHDR)
{
	const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	const auto iItem = pNMI->iItem;
	const auto iSubitem = pNMI->iSubItem;
	if (iItem < 0 || iSubitem < 0 || !m_pHexCtrl->IsDataSet() || iItem >= static_cast<int>(m_vecData.size())) {
		return;
	}

	const auto pField = GetListData(iItem);
	if (pField->eName == EName::NAME_BINARY) //Do nothing if clicked at NAME_BINARY.
		return;

	m_eCurrField = pField->eName;
	SetHighlightSize(pField->u8Size);
	ShowValueBinary();
	m_ListEx.RedrawWindow();
	RedrawHexCtrl();
}

void CHexDlgDataInterp::OnNotifyListSetData(NMHDR* pNMHDR)
{
	const auto pListDataInfo = reinterpret_cast<LISTEX::PLISTEXDATAINFO>(pNMHDR);
	const auto iItem = pListDataInfo->iItem;
	if (!m_pHexCtrl->IsCreated() || !m_pHexCtrl->IsDataSet() || !m_pHexCtrl->IsMutable()
		|| iItem > static_cast<int>(m_vecData.size()))
		return;

	const auto eName = GetListData(iItem)->eName;
	const std::wstring_view wsv = pListDataInfo->pwszData;

	using enum EName;
	bool fSuccess { false };
	switch (eName) {
	case NAME_BINARY:
		fSuccess = SetDataBinary(wsv);
		break;
	case NAME_INT8:
		fSuccess = SetDataNUMBER<std::int8_t>(wsv);
		break;
	case NAME_UINT8:
		fSuccess = SetDataNUMBER<std::uint8_t>(wsv);
		break;
	case NAME_INT16:
		fSuccess = SetDataNUMBER<std::int16_t>(wsv);
		break;
	case NAME_UINT16:
		fSuccess = SetDataNUMBER<std::uint16_t>(wsv);
		break;
	case NAME_INT32:
		fSuccess = SetDataNUMBER<std::int32_t>(wsv);
		break;
	case NAME_UINT32:
		fSuccess = SetDataNUMBER<std::uint32_t>(wsv);
		break;
	case NAME_INT64:
		fSuccess = SetDataNUMBER<std::int64_t>(wsv);
		break;
	case NAME_UINT64:
		fSuccess = SetDataNUMBER<std::uint64_t>(wsv);
		break;
	case NAME_FLOAT:
		fSuccess = SetDataNUMBER<float>(wsv);
		break;
	case NAME_DOUBLE:
		fSuccess = SetDataNUMBER<double>(wsv);
		break;
	case NAME_TIME32T:
		fSuccess = SetDataTime32(wsv);
		break;
	case NAME_TIME64T:
		fSuccess = SetDataTime64(wsv);
		break;
	case NAME_FILETIME:
		fSuccess = SetDataFILETIME(wsv);
		break;
	case NAME_OLEDATETIME:
		fSuccess = SetDataOLEDATETIME(wsv);
		break;
	case NAME_JAVATIME:
		fSuccess = SetDataJAVATIME(wsv);
		break;
	case NAME_MSDOSTIME:
		fSuccess = SetDataMSDOSTIME(wsv);
		break;
	case NAME_MSDTTMTIME:
		fSuccess = SetDataMSDTTMTIME(wsv);
		break;
	case NAME_SYSTEMTIME:
		fSuccess = SetDataSYSTEMTIME(wsv);
		break;
	case NAME_GUIDTIME:
		fSuccess = SetDataGUIDTIME(wsv);
		break;
	case NAME_GUID:
		fSuccess = SetDataGUID(wsv);
		break;
	case NAME_ASCII:
		fSuccess = SetDataASCII(wsv);
		break;
	case NAME_UTF8:
		fSuccess = SetDataUTF8(wsv);
		break;
	case NAME_UTF16:
		fSuccess = SetDataUTF16(wsv);
		break;
	default:
		break;
	};

	if (!fSuccess) {
		::MessageBoxW(m_Wnd, L"Wrong data format or out of range.", L"Data error...", MB_ICONERROR);
	}

	UpdateData();
	SetHighlightSize(GetCurrFieldSize());
	RedrawHexCtrl();
}

auto CHexDlgDataInterp::OnSize(const MSG& msg)->INT_PTR
{
	const auto wWidth = LOWORD(msg.lParam);
	const auto wHeight = HIWORD(msg.lParam);
	m_DynLayout.OnSize(wWidth, wHeight);
	return TRUE;
}

void CHexDlgDataInterp::RedrawHexCtrl()const
{
	if (m_pHexCtrl != nullptr && m_pHexCtrl->IsCreated()) {
		m_pHexCtrl->Redraw();
	}
}

bool CHexDlgDataInterp::SetDataBinary(std::wstring_view wsv)const
{
	const auto pListCurr = GetListData(GetCurrFieldName());
	const auto u32SizeBits = static_cast<std::uint32_t>(pListCurr->u8Size) * 8;
	std::wstring wstr { wsv };
	std::erase(wstr, L' ');
	if (wstr.size() != u32SizeBits || wstr.find_first_not_of(L"01") != std::wstring::npos)
		return false;

	using enum EName;
	switch (GetCurrFieldName()) {
	case NAME_INT8: case NAME_UINT8:
		SetTData(*stn::StrToUInt8(wstr, 2));
		return true;
	case NAME_INT16: case NAME_UINT16:
		SetTData(*stn::StrToUInt16(wstr, 2));
		return true;
	case NAME_INT32: case NAME_UINT32:
	case NAME_FLOAT: case NAME_TIME32T:
	case NAME_MSDOSTIME: case NAME_MSDTTMTIME:
		SetTData(*stn::StrToUInt32(wstr, 2));
		return true;
	case NAME_INT64: case NAME_UINT64:
	case NAME_DOUBLE: case NAME_TIME64T:
	case NAME_FILETIME:	case NAME_OLEDATETIME: case NAME_JAVATIME:
		SetTData(*stn::StrToUInt64(wstr, 2));
		return true;
	case NAME_UTF8: //UTF-8 is endianness agnostic.
		switch (pListCurr->u8Size) {
		case 1:
			SetTData(*stn::StrToUInt8(wstr, 2));
			return true;
		case 2:
			ut::SetIHexTData(*m_pHexCtrl, m_ullOffset, ut::ByteSwap(*stn::StrToUInt16(wstr, 2)));
			return true;
		case 3:
		{
			const auto opt = stn::StrToUInt32(wstr, 2); //Convert to UInt32, then take first 3 bytes.
			const auto u80 = static_cast<std::uint8_t>(*opt >> 16);
			const auto u81 = static_cast<std::uint8_t>(*opt >> 8);
			const auto u82 = static_cast<std::uint8_t>(*opt);
			std::uint8_t arru8[3] { u80, u81, u82 };
			SpanCByte spn { reinterpret_cast<const std::byte*>(arru8), 3 };
			ut::SetIHexTData(*m_pHexCtrl, m_ullOffset, spn);
		}
		return true;
		case 4:
			SetTData(*stn::StrToUInt32(wstr, 2));
			return true;
		default:
			return false;
		}
	case NAME_UTF16:
		switch (pListCurr->u8Size) {
		case 2:
			SetTData(*stn::StrToUInt16(wstr, 2));
			return true;
		case 4:
		{
			const auto opt160 = stn::StrToUInt16(wstr.substr(0, 16), 2); //High surrogate.
			const auto opt161 = stn::StrToUInt16(wstr.substr(16), 2);    //Low surrogate.
			ut::SetIHexTData(*m_pHexCtrl, m_ullOffset, IsBigEndian() ? ut::ByteSwap(*opt160) : *opt160);
			ut::SetIHexTData(*m_pHexCtrl, m_ullOffset + sizeof(std::uint16_t), IsBigEndian() ? ut::ByteSwap(*opt161) : *opt161);
			return true;
		}
		default:
			return false;
		}
	default:
		return false;
	}
}

template<typename T> requires ut::TSize1248<T>
bool CHexDlgDataInterp::SetDataNUMBER(std::wstring_view wsv)const
{
	if (IsShowAsHex()) {
		using UT = std::conditional_t<sizeof(T) == 1, std::uint8_t,
			std::conditional_t<sizeof(T) == 2, std::uint16_t,
			std::conditional_t<sizeof(T) == 4, std::uint32_t, std::uint64_t>>>;
		if (const auto opt = stn::StrToNum<UT>(wsv); opt) {
			SetTData(*opt);
			return true;
		}
	}
	else {
		if (const auto opt = stn::StrToNum<T>(wsv); opt) {
			SetTData(*opt);
			return true;
		}
	}

	return false;
}

bool CHexDlgDataInterp::SetDataTime32(std::wstring_view wsv)const
{
	//The number of seconds since midnight January 1st 1970 UTC (32-bit). This wraps on 19 January 2038.
	const auto optSysTime = ut::StringToSystemTime(wsv, m_dwDateFormat);
	if (!optSysTime)
		return false;

	//Unix times are signed but value before 1st January 1970 is not considered valid.
	//This is apparently because early compilers didn't support unsigned types. _mktime32() has the same limit.
	if (optSysTime->wYear < 1970)
		return false;

	FILETIME ftTime;
	if (::SystemTimeToFileTime(&*optSysTime, &ftTime) == FALSE)
		return false;

	//Convert ticks to seconds and adjust epoch.
	LARGE_INTEGER lTicks { .LowPart { ftTime.dwLowDateTime }, .HighPart { static_cast<LONG>(ftTime.dwHighDateTime) } };
	lTicks.QuadPart /= ut::g_uFTTicksPerSec;
	lTicks.QuadPart -= ut::g_ullUnixEpochDiff;
	if (lTicks.QuadPart >= (std::numeric_limits<long>::max)())
		return false;

	const auto lTime32 = static_cast<__time32_t>(lTicks.QuadPart);
	SetTData(lTime32);

	return true;
}

bool CHexDlgDataInterp::SetDataTime64(std::wstring_view wsv)const
{
	//The number of seconds since midnight January 1st 1970 UTC (32-bit). This wraps on 19 January 2038.
	const auto optSysTime = ut::StringToSystemTime(wsv, m_dwDateFormat);
	if (!optSysTime)
		return false;

	//Unix times are signed but value before 1st January 1970 is not considered valid.
	//This is apparently because early compilers didn't support unsigned types. _mktime64() has the same limit.
	if (optSysTime->wYear < 1970)
		return false;

	FILETIME ftTime;
	if (::SystemTimeToFileTime(&*optSysTime, &ftTime) == FALSE)
		return false;

	//Convert ticks to seconds and adjust epoch.
	LARGE_INTEGER lTicks { .LowPart { ftTime.dwLowDateTime }, .HighPart { static_cast<LONG>(ftTime.dwHighDateTime) } };
	lTicks.QuadPart /= ut::g_uFTTicksPerSec;
	lTicks.QuadPart -= ut::g_ullUnixEpochDiff;
	SetTData(lTicks.QuadPart);

	return true;
}

bool CHexDlgDataInterp::SetDataFILETIME(std::wstring_view wsv)const
{
	const auto optFileTime = ut::StringToFileTime(wsv, m_dwDateFormat);
	if (!optFileTime)
		return false;

	const ULARGE_INTEGER uliTime { .LowPart { optFileTime->dwLowDateTime }, .HighPart { optFileTime->dwHighDateTime } };
	SetTData(uliTime.QuadPart);

	return true;
}

bool CHexDlgDataInterp::SetDataOLEDATETIME(std::wstring_view wsv)const
{
	auto optSysTime = ut::StringToSystemTime(wsv, m_dwDateFormat);
	if (!optSysTime)
		return false;

	DOUBLE dbl;
	if (::SystemTimeToVariantTime(&*optSysTime, &dbl) == FALSE)
		return false;

	SetTData(dbl);

	return true;
}

bool CHexDlgDataInterp::SetDataJAVATIME(std::wstring_view wsv)const
{
	const auto optFileTime = ut::StringToFileTime(wsv, m_dwDateFormat);
	if (!optFileTime)
		return false;

	//Number of milliseconds after/before January 1, 1970, 00:00:00 UTC.
	const LARGE_INTEGER lJavaTicks { .LowPart { optFileTime->dwLowDateTime },
		.HighPart { static_cast<LONG>(optFileTime->dwHighDateTime) } };
	const LARGE_INTEGER lEpochTicks { .LowPart { ut::g_ulFileTime1970_LOW }, .HighPart { ut::g_ulFileTime1970_HIGH } };
	const LONGLONG llDiffTicks { lEpochTicks.QuadPart > lJavaTicks.QuadPart ?
		-(lEpochTicks.QuadPart - lJavaTicks.QuadPart) : lJavaTicks.QuadPart - lEpochTicks.QuadPart };
	const auto llDiffMillis = llDiffTicks / ut::g_uFTTicksPerMS;
	SetTData(llDiffMillis);

	return true;
}

bool CHexDlgDataInterp::SetDataMSDOSTIME(std::wstring_view wsv)const
{
	const auto optFileTime = ut::StringToFileTime(wsv, m_dwDateFormat);
	if (!optFileTime)
		return false;

	UMSDOSDateTime msdosDateTime;
	if (::FileTimeToDosDateTime(&*optFileTime, &msdosDateTime.TimeDate.wDate, &msdosDateTime.TimeDate.wTime) == FALSE)
		return false;

	//Note: Big-endian is not currently supported. This has never existed in the "wild".
	ut::SetIHexTData(*m_pHexCtrl, m_ullOffset, msdosDateTime.dwTimeDate);

	return true;
}

bool CHexDlgDataInterp::SetDataMSDTTMTIME(std::wstring_view wsv)const
{
	const auto optSysTime = ut::StringToSystemTime(wsv, m_dwDateFormat);
	if (!optSysTime)
		return false;

	//Microsoft UDTTM time (as used by Microsoft Compound Document format).
	const UDTTM dttm { .components { .minute { optSysTime->wMinute }, .hour { optSysTime->wHour },
		.dayofmonth { optSysTime->wDay }, .month { optSysTime->wMonth }, .year { optSysTime->wYear - 1900UL },
		.weekday { optSysTime->wDayOfWeek } } };

	//Note: Big-endian is not currently supported. This has never existed in the "wild".
	ut::SetIHexTData(*m_pHexCtrl, m_ullOffset, dttm.dwValue);

	return true;
}

bool CHexDlgDataInterp::SetDataSYSTEMTIME(std::wstring_view wsv)const
{
	const auto optSysTime = ut::StringToSystemTime(wsv, m_dwDateFormat);
	if (!optSysTime)
		return false;

	auto stSTime = *optSysTime;
	if (IsBigEndian()) {
		stSTime.wYear = ut::ByteSwap(stSTime.wYear);
		stSTime.wMonth = ut::ByteSwap(stSTime.wMonth);
		stSTime.wDayOfWeek = ut::ByteSwap(stSTime.wDayOfWeek);
		stSTime.wDay = ut::ByteSwap(stSTime.wDay);
		stSTime.wHour = ut::ByteSwap(stSTime.wHour);
		stSTime.wMinute = ut::ByteSwap(stSTime.wMinute);
		stSTime.wSecond = ut::ByteSwap(stSTime.wSecond);
		stSTime.wMilliseconds = ut::ByteSwap(stSTime.wMilliseconds);
	}
	ut::SetIHexTData(*m_pHexCtrl, m_ullOffset, stSTime);

	return true;
}

bool CHexDlgDataInterp::SetDataGUID(std::wstring_view wsv)const
{
	GUID stGUID;
	if (::IIDFromString(wsv.data(), &stGUID) != S_OK)
		return false;

	if (IsBigEndian()) {
		stGUID.Data1 = ut::ByteSwap(stGUID.Data1);
		stGUID.Data2 = ut::ByteSwap(stGUID.Data2);
		stGUID.Data3 = ut::ByteSwap(stGUID.Data3);
	}
	ut::SetIHexTData(*m_pHexCtrl, m_ullOffset, stGUID);

	return true;
}

bool CHexDlgDataInterp::SetDataGUIDTIME(std::wstring_view wsv)const
{
	//This time is within NAME_GUID structure, and it depends on it.
	//We can not just set a NAME_GUIDTIME for data range if it's not 
	//a valid NAME_GUID range, so checking first.

	auto dqword = ut::GetIHexTData<GUID>(*m_pHexCtrl, m_ullOffset);

	if (IsBigEndian()) { //Swap before any processing.
		dqword.Data1 = ut::ByteSwap(dqword.Data1);
		dqword.Data2 = ut::ByteSwap(dqword.Data2);
		dqword.Data3 = ut::ByteSwap(dqword.Data3);
	}

	const unsigned short unGuidVersion = (dqword.Data3 & 0xF000UL) >> 12;
	if (unGuidVersion != 1)
		return false;

	//RFC4122: The timestamp is a 60-bit value. For UUID version 1, this is represented by Coordinated Universal Time (UTC)
	//as a count of 100-nanosecond intervals since 00:00:00.00, 15 October 1582 (the date of Gregorian reform to the Christian calendar).
	//Both FILETIME and GUID time are based upon 100ns intervals.
	//FILETIME is based upon 1 Jan 1601 whilst GUID time is from 1582. Add 6653 days to convert to GUID time.
	//NB: 6653 days from 15 Oct 1582 to 1 Jan 1601.
	const auto optFTime = ut::StringToFileTime(wsv, m_dwDateFormat);
	if (!optFTime)
		return false;

	const auto ftTime = *optFTime;
	LARGE_INTEGER qwGUIDTime { .LowPart { ftTime.dwLowDateTime }, .HighPart { static_cast<LONG>(ftTime.dwHighDateTime) } };
	const ULARGE_INTEGER ullAddTicks { .QuadPart = static_cast<std::uint64_t>(ut::g_uFTTicksPerSec) *
		static_cast<std::uint64_t>(ut::g_uSecondsPerHour) * static_cast<std::uint64_t>(ut::g_uHoursPerDay)
		* static_cast<std::uint64_t>(ut::g_uFileTime1582OffsetDays) };
	qwGUIDTime.QuadPart += ullAddTicks.QuadPart;

	//Encode version 1 GUID with time.
	dqword.Data1 = qwGUIDTime.LowPart;
	dqword.Data2 = qwGUIDTime.HighPart & 0xFFFFUL;
	dqword.Data3 = ((qwGUIDTime.HighPart >> 16) & 0x0FFFUL) | 0x1000UL; //Including Type 1 flag (0x1000).

	if (IsBigEndian()) { //After processing swap back.
		dqword.Data1 = ut::ByteSwap(dqword.Data1);
		dqword.Data2 = ut::ByteSwap(dqword.Data2);
		dqword.Data3 = ut::ByteSwap(dqword.Data3);
	}
	ut::SetIHexTData(*m_pHexCtrl, m_ullOffset, dqword);

	return true;
}

bool CHexDlgDataInterp::SetDataASCII(std::wstring_view wsv)const
{
	if (wsv.size() != 1) {
		return false;
	}

	const auto wch = wsv[0];
	if (wch < 0x20 || wch > 0x7E) { //Unprintable.
		return false;
	}

	SetTData(static_cast<unsigned char>(wch));

	return true;
}

bool CHexDlgDataInterp::SetDataUTF8(std::wstring_view wsv)const
{
	const auto u8Size = GetFieldSize(EName::NAME_UTF8);
	if (u8Size == 0) {
		return false;
	}

	//New UTF-8 code point can occupy less bytes than the current.
	char buff[8] { };
	if (const auto iBytesToSet = ::WideCharToMultiByte(CP_UTF8, 0, wsv.data(),
		static_cast<int>(wsv.size()), buff, 8, nullptr, nullptr); iBytesToSet <= u8Size) {
		SpanCByte spn { reinterpret_cast<const std::byte*>(buff), static_cast<std::size_t>(iBytesToSet) };
		ut::SetIHexTData(*m_pHexCtrl, m_ullOffset, spn);
		return true;
	}

	return false;
}

bool CHexDlgDataInterp::SetDataUTF16(std::wstring_view wsv)const
{
	const auto u8Size = GetFieldSize(EName::NAME_UTF16);
	if (u8Size == 0 || wsv.size() > u8Size) {
		return false;
	}

	wchar_t buff[2] { IsBigEndian() ? ut::ByteSwap(wsv[0]) : wsv[0], 0 };
	auto dwSizeToSet = 2U;

	//New UTF-16 code point can occupy less bytes than the current.
	//If the current UTF-16 codepoint is 4 bytes in size, and new one is also 4 bytes, we set 4 bytes.
	//Otherwise just one wchar is set.
	if (u8Size == 4 && wsv.size() == 2) {
		buff[1] = { IsBigEndian() ? ut::ByteSwap(wsv[1]) : wsv[1] };
		dwSizeToSet = 4;
	}

	SpanCByte spn { reinterpret_cast<const std::byte*>(buff), dwSizeToSet };
	ut::SetIHexTData(*m_pHexCtrl, m_ullOffset, spn);

	return true;
}

template<ut::TSize1248 T>
void CHexDlgDataInterp::SetTData(T tData)const {
	ut::SetIHexTData(*m_pHexCtrl, m_ullOffset, IsBigEndian() ? ut::ByteSwap(tData) : tData);
}

void CHexDlgDataInterp::SetHighlightSize(std::uint32_t u32Size)
{
	m_dwHighlightSize = u32Size;
}

void CHexDlgDataInterp::ShowValueBinary()
{
	const auto pListCurr = GetListData(GetCurrFieldName());
	if (pListCurr->eName == EName::NAME_BINARY) //Show binary form only for other fields, not for self.
		return;

	const auto ullOffset = m_pHexCtrl->GetCaretPos();
	const auto u8SizeCurr = pListCurr->u8Size;
	const auto& wstr = pListCurr->wstrTypeName;
	const auto pListBin = GetListData(EName::NAME_BINARY);
	std::wstring_view wsv(wstr.begin(), wstr.end() - 1); //Name without end colon.
	pListBin->wstrTypeName = std::format(L"Binary ({}):", wsv);
	using enum EName;

	//Show binary data only for 1-8 bytes sizes.
	if (u8SizeCurr == 0 || u8SizeCurr > 8 || ((ullOffset + u8SizeCurr) > m_pHexCtrl->GetDataSize())) {
		pListBin->wstrValue = L"N/A";
		pListBin->fAllowEdit = false;
		return;
	}

	const auto spnData = m_pHexCtrl->GetData({ .ullOffset { ullOffset }, .ullSize { u8SizeCurr } });
	pListBin->fAllowEdit = m_pHexCtrl->IsMutable();

	switch (pListCurr->eName) {
	case NAME_INT8:
	case NAME_UINT8:
		pListBin->wstrValue = std::format(L"{:08b}", static_cast<std::uint8_t>(spnData[0]));
		break;
	case NAME_INT16:
	case NAME_UINT16:
	{
		const auto u80 = static_cast<std::uint8_t>(spnData[0]); const auto u81 = static_cast<std::uint8_t>(spnData[1]);
		pListBin->wstrValue = IsBigEndian() ? std::format(L"{:08b} {:08b}", u80, u81)
			: std::format(L"{:08b} {:08b}", u81, u80);
	}
	break;
	case NAME_INT32:
	case NAME_UINT32:
	case NAME_FLOAT:
	case NAME_TIME32T:
	case NAME_MSDOSTIME:
	case NAME_MSDTTMTIME:
	{
		const auto u80 = static_cast<std::uint8_t>(spnData[0]); const auto u81 = static_cast<std::uint8_t>(spnData[1]);
		const auto u82 = static_cast<std::uint8_t>(spnData[2]); const auto u83 = static_cast<std::uint8_t>(spnData[3]);
		pListBin->wstrValue = IsBigEndian() ? std::format(L"{:08b} {:08b} {:08b} {:08b}", u80, u81, u82, u83)
			: std::format(L"{:08b} {:08b} {:08b} {:08b}", u83, u82, u81, u80);
	}
	break;
	case NAME_INT64:
	case NAME_UINT64:
	case NAME_DOUBLE:
	case NAME_TIME64T:
	case NAME_FILETIME:
	case NAME_OLEDATETIME:
	case NAME_JAVATIME:
	{
		const auto u80 = static_cast<std::uint8_t>(spnData[0]); const auto u81 = static_cast<std::uint8_t>(spnData[1]);
		const auto u82 = static_cast<std::uint8_t>(spnData[2]); const auto u83 = static_cast<std::uint8_t>(spnData[3]);
		const auto u84 = static_cast<std::uint8_t>(spnData[4]); const auto u85 = static_cast<std::uint8_t>(spnData[5]);
		const auto u86 = static_cast<std::uint8_t>(spnData[6]); const auto u87 = static_cast<std::uint8_t>(spnData[7]);
		pListBin->wstrValue = IsBigEndian() ? std::format(L"{:08b} {:08b} {:08b} {:08b} {:08b} {:08b} {:08b} {:08b}",
			u80, u81, u82, u83, u84, u85, u86, u87) : std::format(L"{:08b} {:08b} {:08b} {:08b} {:08b} {:08b} {:08b} {:08b}",
			u87, u86, u85, u84, u83, u82, u81, u80);
	}
	break;
	case NAME_ASCII:
		pListBin->wstrValue = std::format(L"{:08b}", static_cast<std::uint8_t>(spnData[0]));
		break;
	case NAME_UTF8:
		switch (pListCurr->u8Size) {
		case 1:
			pListBin->wstrValue = std::format(L"{:08b}", static_cast<std::uint8_t>(spnData[0]));
			break;
		case 2:
		{
			const auto u80 = static_cast<std::uint8_t>(spnData[0]);	const auto u81 = static_cast<std::uint8_t>(spnData[1]);
			pListBin->wstrValue = std::format(L"{:08b} {:08b}", u80, u81);
		}
		break;
		case 3:
		{
			const auto u80 = static_cast<std::uint8_t>(spnData[0]);	const auto u81 = static_cast<std::uint8_t>(spnData[1]);
			const auto u82 = static_cast<std::uint8_t>(spnData[2]);
			pListBin->wstrValue = std::format(L"{:08b} {:08b} {:08b}", u80, u81, u82);
		}
		break;
		case 4:
		{
			const auto u80 = static_cast<std::uint8_t>(spnData[0]); const auto u81 = static_cast<std::uint8_t>(spnData[1]);
			const auto u82 = static_cast<std::uint8_t>(spnData[2]); const auto u83 = static_cast<std::uint8_t>(spnData[3]);
			pListBin->wstrValue = std::format(L"{:08b} {:08b} {:08b} {:08b}", u80, u81, u82, u83);
		}
		break;
		default:
			break;
		}
		break;
	case NAME_UTF16:
		switch (pListCurr->u8Size) {
		case 2:
		{
			const auto u80 = static_cast<std::uint8_t>(spnData[0]); const auto u81 = static_cast<std::uint8_t>(spnData[1]);
			pListBin->wstrValue = IsBigEndian() ? std::format(L"{:08b} {:08b}", u80, u81)
				: std::format(L"{:08b} {:08b}", u81, u80);
		}
		break;
		case 4:
		{
			const auto u80 = static_cast<std::uint8_t>(spnData[0]); const auto u81 = static_cast<std::uint8_t>(spnData[1]);
			const auto u82 = static_cast<std::uint8_t>(spnData[2]); const auto u83 = static_cast<std::uint8_t>(spnData[3]);
			pListBin->wstrValue = IsBigEndian() ? std::format(L"{:08b} {:08b} {:08b} {:08b}", u80, u81, u82, u83)
				: std::format(L"{:08b} {:08b} {:08b} {:08b}", u81, u80, u83, u82);
		}
		break;
		default:
			break;
		}
		break;
	default:
		pListBin->wstrValue = L"N/A";
		pListBin->fAllowEdit = false;
		break;
	}
}

void CHexDlgDataInterp::ShowValueInt8(SpanCByte spn)
{
	if (const auto pList = GetListData(EName::NAME_INT8); spn.size() >= sizeof(std::int8_t)) {
		const auto i8 = static_cast<std::int8_t>(spn[0]);
		const auto u8 = static_cast<std::uint8_t>(spn[0]);
		pList->wstrValue = std::vformat(IsShowAsHex() ? L"0x{0:02X}" : L"{1}", std::make_wformat_args(u8, i8));
		pList->fAllowEdit = m_pHexCtrl->IsMutable();
	}
	else {
		pList->wstrValue.clear();
		pList->fAllowEdit = false;
	}
}

void CHexDlgDataInterp::ShowValueUInt8(SpanCByte spn)
{
	if (const auto pList = GetListData(EName::NAME_UINT8); spn.size() >= sizeof(std::uint8_t)) {
		const auto u8 = static_cast<std::uint8_t>(spn[0]);
		pList->wstrValue = std::vformat(IsShowAsHex() ? L"0x{:02X}" : L"{}", std::make_wformat_args(u8));
		pList->fAllowEdit = m_pHexCtrl->IsMutable();
	}
	else {
		pList->wstrValue.clear();
		pList->fAllowEdit = false;
	}
}

void CHexDlgDataInterp::ShowValueInt16(SpanCByte spn)
{
	if (const auto pList = GetListData(EName::NAME_INT16); spn.size() >= sizeof(std::int16_t)) {
		auto u16 = *reinterpret_cast<const std::uint16_t*>(spn.data());
		if (IsBigEndian()) { u16 = ut::ByteSwap(u16); }
		const auto i16 = static_cast<std::int16_t>(u16);
		pList->wstrValue = std::vformat(IsShowAsHex() ? L"0x{0:04X}" : L"{1}", std::make_wformat_args(u16, i16));
		pList->fAllowEdit = m_pHexCtrl->IsMutable();
	}
	else {
		pList->wstrValue.clear();
		pList->fAllowEdit = false;
	}
}

void CHexDlgDataInterp::ShowValueUInt16(SpanCByte spn)
{
	if (const auto pList = GetListData(EName::NAME_UINT16); spn.size() >= sizeof(std::uint16_t)) {
		auto u16 = *reinterpret_cast<const std::uint16_t*>(spn.data());
		if (IsBigEndian()) { u16 = ut::ByteSwap(u16); }
		pList->wstrValue = std::vformat(IsShowAsHex() ? L"0x{:04X}" : L"{}", std::make_wformat_args(u16));
		pList->fAllowEdit = m_pHexCtrl->IsMutable();
	}
	else {
		pList->wstrValue.clear();
		pList->fAllowEdit = false;
	}
}

void CHexDlgDataInterp::ShowValueInt32(SpanCByte spn)
{
	if (const auto pList = GetListData(EName::NAME_INT32); spn.size() >= sizeof(std::int32_t)) {
		auto u32 = *reinterpret_cast<const std::uint32_t*>(spn.data());
		if (IsBigEndian()) { u32 = ut::ByteSwap(u32); }
		const auto i32 = static_cast<std::int32_t>(u32);
		pList->wstrValue = std::vformat(IsShowAsHex() ? L"0x{0:08X}" : L"{1}", std::make_wformat_args(u32, i32));
		pList->fAllowEdit = m_pHexCtrl->IsMutable();
	}
	else {
		pList->wstrValue.clear();
		pList->fAllowEdit = false;
	}
}

void CHexDlgDataInterp::ShowValueUInt32(SpanCByte spn)
{
	if (const auto pList = GetListData(EName::NAME_UINT32); spn.size() >= sizeof(std::uint32_t)) {
		auto u32 = *reinterpret_cast<const std::uint32_t*>(spn.data());
		if (IsBigEndian()) { u32 = ut::ByteSwap(u32); }
		pList->wstrValue = std::vformat(IsShowAsHex() ? L"0x{:08X}" : L"{}", std::make_wformat_args(u32));
		pList->fAllowEdit = m_pHexCtrl->IsMutable();
	}
	else {
		pList->wstrValue.clear();
		pList->fAllowEdit = false;
	}
}

void CHexDlgDataInterp::ShowValueInt64(SpanCByte spn)
{
	if (const auto pList = GetListData(EName::NAME_INT64); spn.size() >= sizeof(std::int64_t)) {
		auto u64 = *reinterpret_cast<const std::uint64_t*>(spn.data());
		if (IsBigEndian()) { u64 = ut::ByteSwap(u64); }
		const auto i64 = static_cast<std::int64_t>(u64);
		pList->wstrValue = std::vformat(IsShowAsHex() ? L"0x{0:016X}" : L"{1}", std::make_wformat_args(u64, i64));
		pList->fAllowEdit = m_pHexCtrl->IsMutable();
	}
	else {
		pList->wstrValue.clear();
		pList->fAllowEdit = false;
	}
}

void CHexDlgDataInterp::ShowValueUInt64(SpanCByte spn)
{
	if (const auto pList = GetListData(EName::NAME_UINT64); spn.size() >= sizeof(std::uint64_t)) {
		auto u64 = *reinterpret_cast<const std::uint64_t*>(spn.data());
		if (IsBigEndian()) { u64 = ut::ByteSwap(u64); }
		pList->wstrValue = std::vformat(IsShowAsHex() ? L"0x{:016X}" : L"{}", std::make_wformat_args(u64));
		pList->fAllowEdit = m_pHexCtrl->IsMutable();
	}
	else {
		pList->wstrValue.clear();
		pList->fAllowEdit = false;
	}
}

void CHexDlgDataInterp::ShowValueFloat(SpanCByte spn)
{
	if (const auto pList = GetListData(EName::NAME_FLOAT); spn.size() >= sizeof(float)) {
		auto u32 = *reinterpret_cast<const std::uint32_t*>(spn.data());
		if (IsBigEndian()) { u32 = ut::ByteSwap(u32); }
		const auto fl = std::bit_cast<float>(u32);
		pList->wstrValue = std::vformat(IsShowAsHex() ? L"0x{0:08X}" : L"{1:.9e}", std::make_wformat_args(u32, fl));
		pList->fAllowEdit = m_pHexCtrl->IsMutable();
	}
	else {
		pList->wstrValue.clear();
		pList->fAllowEdit = false;
	}
}

void CHexDlgDataInterp::ShowValueDouble(SpanCByte spn)
{
	if (const auto pList = GetListData(EName::NAME_DOUBLE); spn.size() >= sizeof(double)) {
		auto u64 = *reinterpret_cast<const std::uint64_t*>(spn.data());
		if (IsBigEndian()) { u64 = ut::ByteSwap(u64); }
		const auto dbl = std::bit_cast<double>(u64);
		pList->wstrValue = std::vformat(IsShowAsHex() ? L"0x{0:016X}" : L"{1:.18e}", std::make_wformat_args(u64, dbl));
		pList->fAllowEdit = m_pHexCtrl->IsMutable();
	}
	else {
		pList->wstrValue.clear();
		pList->fAllowEdit = false;
	}
}

void CHexDlgDataInterp::ShowValueTime32(SpanCByte spn)
{
	if (const auto pList = GetListData(EName::NAME_TIME32T); spn.size() >= sizeof(std::uint32_t)) {
		auto u32 = *reinterpret_cast<const std::uint32_t*>(spn.data());
		if (IsBigEndian()) { u32 = ut::ByteSwap(u32); }

		std::wstring wstrTime = L"N/A";
		//The number of seconds since midnight January 1st 1970 UTC (32-bit). This is signed and wraps on 19 January 2038.
		const auto lTime32 = static_cast<__time32_t>(u32);

		//Unix times are signed and value before 1st January 1970 is not considered valid.
		//This is apparently because early compilers didn't support unsigned types. _mktime32() has the same limit.
		if (lTime32 >= 0) {
			//Add seconds from epoch time.
			LARGE_INTEGER Time { .LowPart { ut::g_ulFileTime1970_LOW }, .HighPart { ut::g_ulFileTime1970_HIGH } };
			Time.QuadPart += static_cast<LONGLONG>(lTime32) * ut::g_uFTTicksPerSec;

			//Convert to FILETIME.
			const FILETIME ftTime { .dwLowDateTime { Time.LowPart }, .dwHighDateTime { static_cast<DWORD>(Time.HighPart) } };
			wstrTime = ut::FileTimeToString(ftTime, m_dwDateFormat, m_wchDateSepar);
		}
		pList->wstrValue = std::move(wstrTime);
		pList->fAllowEdit = m_pHexCtrl->IsMutable();
	}
	else {
		pList->wstrValue.clear();
		pList->fAllowEdit = false;
	}
}

void CHexDlgDataInterp::ShowValueTime64(SpanCByte spn)
{
	if (const auto pList = GetListData(EName::NAME_TIME64T); spn.size() >= sizeof(std::uint64_t)) {
		auto u64 = *reinterpret_cast<const std::uint64_t*>(spn.data());
		if (IsBigEndian()) { u64 = ut::ByteSwap(u64); }

		std::wstring wstrTime = L"N/A";
		const auto llTime64 = static_cast<__time64_t>(u64); //The number of seconds since midnight January 1st 1970 UTC (64-bit).

		//Unix times are signed and value before 1st January 1970 is not considered valid.
		//This is apparently because early compilers didn't support unsigned types. _mktime64() has the same limit.
		if (llTime64 >= 0) {
			//Add seconds from epoch time.
			LARGE_INTEGER Time { { .LowPart { ut::g_ulFileTime1970_LOW }, .HighPart { ut::g_ulFileTime1970_HIGH } } };
			Time.QuadPart += llTime64 * ut::g_uFTTicksPerSec;

			//Convert to FILETIME.
			const FILETIME ftTime { .dwLowDateTime { Time.LowPart }, .dwHighDateTime { static_cast<DWORD>(Time.HighPart) } };
			wstrTime = ut::FileTimeToString(ftTime, m_dwDateFormat, m_wchDateSepar);
		}
		pList->wstrValue = std::move(wstrTime);
		pList->fAllowEdit = m_pHexCtrl->IsMutable();
	}
	else {
		pList->wstrValue.clear();
		pList->fAllowEdit = false;
	}
}

void CHexDlgDataInterp::ShowValueFILETIME(SpanCByte spn)
{
	if (const auto pList = GetListData(EName::NAME_FILETIME); spn.size() >= sizeof(std::uint64_t)) {
		auto u64 = *reinterpret_cast<const std::uint64_t*>(spn.data());
		if (IsBigEndian()) { u64 = ut::ByteSwap(u64); }
		pList->wstrValue = ut::FileTimeToString(std::bit_cast<FILETIME>(u64), m_dwDateFormat, m_wchDateSepar);
		pList->fAllowEdit = m_pHexCtrl->IsMutable();
	}
	else {
		pList->wstrValue.clear();
		pList->fAllowEdit = false;
	}
}

void CHexDlgDataInterp::ShowValueOLEDATETIME(SpanCByte spn)
{
	if (const auto pList = GetListData(EName::NAME_OLEDATETIME); spn.size() >= sizeof(std::uint64_t)) {
		auto u64 = *reinterpret_cast<const std::uint64_t*>(spn.data());
		if (IsBigEndian()) { u64 = ut::ByteSwap(u64); }

		//OLE (including MS Office) date/time.
		//Implemented using an 8-byte floating-point number. 
		//Days are represented as whole number increments starting with 30 December 1899, midnight as time zero.
		//See: https://docs.microsoft.com/en-us/cpp/atl-mfc-shared/date-type?view=vs-2019
		std::wstring wstrTime = L"N/A";
		if (SYSTEMTIME stSysTime; ::VariantTimeToSystemTime(std::bit_cast<DATE>(u64), &stSysTime) == TRUE) {
			wstrTime = ut::SystemTimeToString(stSysTime, m_dwDateFormat, m_wchDateSepar);
		}
		pList->wstrValue = std::move(wstrTime);
		pList->fAllowEdit = m_pHexCtrl->IsMutable();
	}
	else {
		pList->wstrValue.clear();
		pList->fAllowEdit = false;
	}
}

void CHexDlgDataInterp::ShowValueJAVATIME(SpanCByte spn)
{
	if (const auto pList = GetListData(EName::NAME_JAVATIME); spn.size() >= sizeof(std::uint64_t)) {
		auto u64 = *reinterpret_cast<const std::uint64_t*>(spn.data());
		if (IsBigEndian()) { u64 = ut::ByteSwap(u64); }

		//Javatime (signed). Number of milliseconds after/before January 1, 1970, 00:00:00 UTC.
		//Add/subtract milliseconds from epoch time.
		LARGE_INTEGER Time { { .LowPart { ut::g_ulFileTime1970_LOW }, .HighPart { ut::g_ulFileTime1970_HIGH } } };
		Time.QuadPart += u64 * ut::g_uFTTicksPerMS * (static_cast<LONGLONG>(u64) >= 0 ? 1 : -1);

		//Convert to FILETIME.
		const FILETIME ftJavaTime { .dwLowDateTime { Time.LowPart }, .dwHighDateTime { static_cast<DWORD>(Time.HighPart) } };
		pList->wstrValue = ut::FileTimeToString(ftJavaTime, m_dwDateFormat, m_wchDateSepar);
		pList->fAllowEdit = m_pHexCtrl->IsMutable();
	}
	else {
		pList->wstrValue.clear();
		pList->fAllowEdit = false;
	}
}

void CHexDlgDataInterp::ShowValueMSDOSTIME(SpanCByte spn)
{
	if (const auto pList = GetListData(EName::NAME_MSDOSTIME); spn.size() >= sizeof(std::uint32_t)) {
		auto u32 = *reinterpret_cast<const std::uint32_t*>(spn.data());
		if (IsBigEndian()) { u32 = ut::ByteSwap(u32); }
		std::wstring wstrTime = L"N/A";
		const UMSDOSDateTime msdosDateTime { .dwTimeDate { u32 } };
		if (FILETIME ftMSDOS;
			::DosDateTimeToFileTime(msdosDateTime.TimeDate.wDate, msdosDateTime.TimeDate.wTime, &ftMSDOS) != FALSE) {
			wstrTime = ut::FileTimeToString(ftMSDOS, m_dwDateFormat, m_wchDateSepar);
		}
		pList->wstrValue = std::move(wstrTime);
		pList->fAllowEdit = m_pHexCtrl->IsMutable();
	}
	else {
		pList->wstrValue.clear();
		pList->fAllowEdit = false;
	}
}

void CHexDlgDataInterp::ShowValueMSDTTMTIME(SpanCByte spn)
{
	if (const auto pList = GetListData(EName::NAME_MSDTTMTIME); spn.size() >= sizeof(std::uint32_t)) {
		auto u32 = *reinterpret_cast<const std::uint32_t*>(spn.data());
		if (IsBigEndian()) { u32 = ut::ByteSwap(u32); }

		//Microsoft UDTTM time (as used by Microsoft Compound Document format).
		std::wstring wstrTime = L"N/A";
		const UDTTM dttm { .dwValue { u32 } };
		if (dttm.components.dayofmonth > 0 && dttm.components.dayofmonth < 32
			&& dttm.components.hour < 24 && dttm.components.minute < 60
			&& dttm.components.month>0 && dttm.components.month < 13 && dttm.components.weekday < 7) {
			const SYSTEMTIME stSysTime { .wYear { static_cast<WORD>(1900 + dttm.components.year) },
				.wMonth { static_cast<WORD>(dttm.components.month) }, .wDayOfWeek { static_cast<WORD>(dttm.components.weekday) },
				.wDay { static_cast<WORD>(dttm.components.dayofmonth) }, .wHour { static_cast<WORD>(dttm.components.hour) },
				.wMinute { static_cast<WORD>(dttm.components.minute) }, .wSecond { 0 }, .wMilliseconds { 0 } };

			wstrTime = ut::SystemTimeToString(stSysTime, m_dwDateFormat, m_wchDateSepar);
		}
		pList->wstrValue = std::move(wstrTime);
		pList->fAllowEdit = m_pHexCtrl->IsMutable();
	}
	else {
		pList->wstrValue.clear();
		pList->fAllowEdit = false;
	}
}

void CHexDlgDataInterp::ShowValueSYSTEMTIME(SpanCByte spn)
{
	if (const auto pList = GetListData(EName::NAME_SYSTEMTIME); spn.size() >= sizeof(SYSTEMTIME)) {
		auto syst = *reinterpret_cast<const SYSTEMTIME*>(spn.data());
		if (IsBigEndian()) {
			syst.wYear = ut::ByteSwap(syst.wYear);
			syst.wMonth = ut::ByteSwap(syst.wMonth);
			syst.wDayOfWeek = ut::ByteSwap(syst.wDayOfWeek);
			syst.wDay = ut::ByteSwap(syst.wDay);
			syst.wHour = ut::ByteSwap(syst.wHour);
			syst.wMinute = ut::ByteSwap(syst.wMinute);
			syst.wSecond = ut::ByteSwap(syst.wSecond);
			syst.wMilliseconds = ut::ByteSwap(syst.wMilliseconds);
		}
		pList->wstrValue = ut::SystemTimeToString(syst, m_dwDateFormat, m_wchDateSepar);
		pList->fAllowEdit = m_pHexCtrl->IsMutable();
	}
	else {
		pList->wstrValue.clear();
		pList->fAllowEdit = false;
	}
}

void CHexDlgDataInterp::ShowValueGUID(SpanCByte spn)
{
	if (const auto pList = GetListData(EName::NAME_GUID); spn.size() >= sizeof(GUID)) {
		auto guid = *reinterpret_cast<const GUID*>(spn.data());
		if (IsBigEndian()) {
			guid.Data1 = ut::ByteSwap(guid.Data1);
			guid.Data2 = ut::ByteSwap(guid.Data2);
			guid.Data3 = ut::ByteSwap(guid.Data3);
		}
		pList->wstrValue = std::format(L"{{{:08X}-{:04X}-{:04X}-{:02X}{:02X}-{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}}}",
			guid.Data1, guid.Data2, guid.Data3, guid.Data4[0],
			guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4],
			guid.Data4[5], guid.Data4[6], guid.Data4[7]);
		pList->fAllowEdit = m_pHexCtrl->IsMutable();
	}
	else {
		pList->wstrValue.clear();
		pList->fAllowEdit = false;
	}
}

void CHexDlgDataInterp::ShowValueGUIDTIME(SpanCByte spn)
{
	if (const auto pList = GetListData(EName::NAME_GUIDTIME); spn.size() >= sizeof(GUID)) {
		auto guid = *reinterpret_cast<const GUID*>(spn.data());
		if (IsBigEndian()) {
			guid.Data1 = ut::ByteSwap(guid.Data1);
			guid.Data2 = ut::ByteSwap(guid.Data2);
			guid.Data3 = ut::ByteSwap(guid.Data3);
		}

		//Guid v1 Datetime UTC. The time structure within the NAME_GUID.
		//First, verify GUID is actually version 1 style.
		std::wstring wstrTime = L"N/A";
		const unsigned short unGuidVersion = (guid.Data3 & 0xF000UL) >> 12;
		if (unGuidVersion == 1) {
			LARGE_INTEGER qwGUIDTime { .LowPart { guid.Data1 }, .HighPart { guid.Data3 & 0x0FFFUL } };
			qwGUIDTime.HighPart = (qwGUIDTime.HighPart << 16) | guid.Data2;

			//RFC4122: The timestamp is a 60-bit value. For UUID version 1, this is represented by Coordinated Universal Time (UTC)
			//as a count of 100-nanosecond intervals since 00:00:00.00, 15 October 1582 (the date of Gregorian reform to the Christian calendar).
			//Both FILETIME and GUID time are based upon 100ns intervals.
			//FILETIME is based upon 1 Jan 1601 whilst GUID time is from 1582. Subtract 6653 days to convert from GUID time.
			//NB: 6653 days from 15 Oct 1582 to 1 Jan 1601.
			const ULARGE_INTEGER ullSubtractTicks { .QuadPart = static_cast<std::uint64_t>(ut::g_uFTTicksPerSec)
				* static_cast<std::uint64_t>(ut::g_uSecondsPerHour) * static_cast<std::uint64_t>(ut::g_uHoursPerDay)
				* static_cast<std::uint64_t>(ut::g_uFileTime1582OffsetDays) };
			qwGUIDTime.QuadPart -= ullSubtractTicks.QuadPart;

			//Convert to SYSTEMTIME.
			const FILETIME ftGUIDTime { .dwLowDateTime { qwGUIDTime.LowPart },
				.dwHighDateTime { static_cast<DWORD>(qwGUIDTime.HighPart) } };
			wstrTime = ut::FileTimeToString(ftGUIDTime, m_dwDateFormat, m_wchDateSepar);
		}
		pList->wstrValue = std::move(wstrTime);
		pList->fAllowEdit = m_pHexCtrl->IsMutable();
	}
	else {
		pList->wstrValue.clear();
		pList->fAllowEdit = false;
	}
}

void CHexDlgDataInterp::ShowValueASCII(SpanCByte spn)
{
	if (const auto pList = GetListData(EName::NAME_ASCII); spn.size() >= sizeof(char)) {
		if (const auto ch = *reinterpret_cast<const unsigned char*>(spn.data()); ch < 0x20 || ch > 0x7E) {
			pList->wstrValue = L"N/A";
		}
		else {
			pList->wstrValue = ch;
		}
		pList->fAllowEdit = m_pHexCtrl->IsMutable();
	}
	else {
		pList->wstrValue.clear();
		pList->fAllowEdit = false;
	}
}

void CHexDlgDataInterp::ShowValueUTF8(SpanCByte spn)
{
	const auto pListUTF8 = GetListData(EName::NAME_UTF8);
	const auto lmbClear = [=] {
		pListUTF8->wstrValue = L"N/A";
		pListUTF8->fAllowEdit = false;
		pListUTF8->u8Size = 0; };

	if (spn.empty()) { lmbClear(); return; }

	//Code point bits distribution in code units (bytes):
	//[U+0000  -  U+007F]: 0yyyzzzz
	//[U+0080  -  U+07FF]: 110xxxyy 10yyzzzz
	//[U+0800  -  U+FFFF]: 1110wwww 10xxxxyy 10yyzzzz
	//[U+010000-U+10FFFF]: 11110uvv 10vvwwww 10xxxxyy 10yyzzzz

	//First code unit in the UTF-8 code point (bytes sequence).
	const auto u80 = static_cast<std::uint8_t>(spn[0]);

	//How many code units (bytes) this UTF-8 code point occupies.
	const auto u8UTF8Bytes = static_cast<std::uint8_t>((u80 & 0b10000000) == 0b00000000 ? 1U : ((u80 & 0b11100000) == 0b11000000 ? 2U
		: ((u80 & 0b11110000) == 0b11100000 ? 3U : ((u80 & 0b11111000) == 0b11110000 ? 4U : 0U))));
	if (u8UTF8Bytes == 0 || u8UTF8Bytes > spn.size()) { lmbClear(); return; }

	//Unicode code point number (U+XXXXXX), deliberately initialized with an invalid number.
	std::uint64_t u64CP { 0xFFFFFFFFU };
	switch (u8UTF8Bytes) {
	case 1:
		u64CP = u80 & 0b01111111;
		break;
	case 2:
		if (const auto u81 = static_cast<std::uint8_t>(spn[1]);	(u81 & 0b11000000) == 0b10000000) {
			u64CP = u80 & 0b00011111;
			u64CP = (u64CP << 6) | (u81 & 0b00111111);
		}
		break;
	case 3:
		if (const auto u81 = static_cast<std::uint8_t>(spn[1]), u82 = static_cast<std::uint8_t>(spn[2]);
			(u81 & 0b11000000) == 0b10000000 && (u82 & 0b11000000) == 0b10000000) {
			u64CP = u80 & 0b00001111;
			u64CP = (u64CP << 6) | (u81 & 0b00111111);
			u64CP = (u64CP << 6) | (u82 & 0b00111111);
		}
		break;
	case 4:
		if (const auto u81 = static_cast<std::uint8_t>(spn[1]), u82 = static_cast<std::uint8_t>(spn[2]),
			u83 = static_cast<std::uint8_t>(spn[3]);
			(u81 & 0b11000000) == 0b10000000 && (u82 & 0b11000000) == 0b10000000 && (u83 & 0b11000000) == 0b10000000) {
			u64CP = u80 & 0b00000111;
			u64CP = (u64CP << 6) | (u81 & 0b00111111);
			u64CP = (u64CP << 6) | (u82 & 0b00111111);
			u64CP = (u64CP << 6) | (u83 & 0b00111111);
		}
		break;
	default:
		break;
	}

	//The [U+D800-U+DFFF] code points range is for UTF-16 surrogate pairs encoding.
	//The official Unicode standard says that no UTF forms can encode these surrogate code points.
	//However, Windows allows unpaired surrogates in filenames and other places.
	//0x10FFFFU is the maximum valid Unicode code point number.
	if (u64CP > 0x10FFFFU) { lmbClear(); return; }

	wchar_t buff[4] { };
	if (::MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<LPCCH>(spn.data()), u8UTF8Bytes, buff, 4) == 0) {
		lmbClear();
		return;
	}

	pListUTF8->wstrValue = std::format(L"{} (U+{:04X})", buff, u64CP);
	pListUTF8->fAllowEdit = m_pHexCtrl->IsMutable();
	pListUTF8->u8Size = u8UTF8Bytes;
}

void CHexDlgDataInterp::ShowValueUTF16(SpanCByte spn)
{
	const auto pListUTF16 = GetListData(EName::NAME_UTF16);
	const auto lmbClear = [=] {
		pListUTF16->wstrValue = L"N/A";
		pListUTF16->fAllowEdit = false;
		pListUTF16->u8Size = 0; };

	if (spn.size() < sizeof(wchar_t)) { lmbClear(); return; }

	//First code unit in the UTF-16 code point. UTF-16 can occupy up to two wchar_t.
	auto u160 = *reinterpret_cast<const std::uint16_t*>(spn.data());
	if (IsBigEndian()) { u160 = ut::ByteSwap(u160); }

	//The [U+D800-U+DFFF] code points range is for UTF-16 surrogate pairs encoding.
	const auto u8UTF16Bytes = static_cast<std::uint8_t>(((u160 & 0xF800U) == 0xD800U) ? 4U : 2U);
	if (u8UTF16Bytes > spn.size()) { lmbClear(); return; }

	//Unicode code point number (U+XXXXXX), deliberately initialized with an invalid number.
	std::uint64_t u64CP { 0xFFFFFFFFU };
	std::uint16_t u161 { };
	switch (u8UTF16Bytes) {
	case 2:
		u64CP = u160;
		break;
	case 4:
		u161 = *reinterpret_cast<const std::uint16_t*>(spn.data() + sizeof(wchar_t));
		if (IsBigEndian()) { u161 = ut::ByteSwap(u161); }

		if ((u161 & 0xFC00U) == 0xDC00U) { //Is it valid UTF-16 low surrogate [0xDC00U-0xDFFFU]?
			u64CP = (static_cast<std::uint64_t>(u160 - 0xD800U) << 10) + (u161 - 0xDC00U) + 0x10000U;
		}
		break;
	default:
		break;
	}

	//0x10FFFFU is the maximum valid Unicode code point number.
	if (u64CP > 0x10FFFFU) { lmbClear(); return; }

	wchar_t buff[4] { u160, u161, 0, 0 };
	pListUTF16->wstrValue = std::format(L"{} (U+{:04X})", buff, u64CP);
	pListUTF16->fAllowEdit = m_pHexCtrl->IsMutable();
	pListUTF16->u8Size = u8UTF16Bytes;
}