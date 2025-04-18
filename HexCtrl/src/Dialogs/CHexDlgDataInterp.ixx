/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
module;
#include <SDKDDKVer.h>
#include "../../res/HexCtrlRes.h"
#include "../../HexCtrl.h"
#include <Windows.h>
#include <commctrl.h>
#include <algorithm>
#include <bit>
#include <format>
export module HEXCTRL:CHexDlgDataInterp;

import :HexUtility;

namespace HEXCTRL::INTERNAL {
	class CHexDlgDataInterp final {
	public:
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
		LISTEX::CListEx m_ListEx;
		std::vector<LISTDATA> m_vecData;
		IHexCtrl* m_pHexCtrl { };
		ULONGLONG m_ullOffset { };
		std::uint64_t m_u64Flags { }; //Data from SetDlgProperties.
		DWORD m_dwHglDataSize { };    //Size of the data to highlight in the HexCtrl.
		DWORD m_dwDateFormat { };     //Date format.
		wchar_t m_wchDateSepar { };   //Date separator.
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

enum class CHexDlgDataInterp::EGroup : std::uint8_t {
	GR_BINARY, GR_INTEGRAL, GR_FLOAT, GR_TIME, GR_MISC, GR_GUIDTIME
};

enum class CHexDlgDataInterp::EName : std::uint8_t {
	NAME_BINARY, NAME_INT8, NAME_UINT8, NAME_INT16, NAME_UINT16,
	NAME_INT32, NAME_UINT32, NAME_INT64, NAME_UINT64,
	NAME_FLOAT, NAME_DOUBLE, NAME_TIME32T, NAME_TIME64T,
	NAME_FILETIME, NAME_OLEDATETIME, NAME_JAVATIME, NAME_MSDOSTIME,
	NAME_MSDTTMTIME, NAME_SYSTEMTIME, NAME_GUIDTIME, NAME_GUID
};

enum class CHexDlgDataInterp::EDataSize : std::uint8_t {
	SIZE_BYTE = 1U, SIZE_WORD = 2U, SIZE_DWORD = 4U, SIZE_QWORD = 8U, SIZE_DQWORD = 16U
};

struct CHexDlgDataInterp::LISTDATA {
	std::wstring wstrDataType;
	std::wstring wstrValue;
	EGroup       eGroup { };
	EName        eName { };
	EDataSize    eSize { };
	bool         fAllowEdit { true };
};

void CHexDlgDataInterp::CreateDlg()
{
	//m_Wnd is set in the OnInitDialog().
	if (const auto hWnd = ::CreateDialogParamW(m_hInstRes, MAKEINTRESOURCEW(IDD_HEXCTRL_DATAINTERP),
		m_pHexCtrl->GetWndHandle(EHexWnd::WND_MAIN), wnd::DlgProc<CHexDlgDataInterp>, reinterpret_cast<LPARAM>(this));
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

auto CHexDlgDataInterp::GetHglDataSize()const->DWORD
{
	return m_dwHglDataSize;
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
	return m_dwHglDataSize > 0;
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

	using enum EDataSize;
	const auto ullOffset = m_ullOffset = m_pHexCtrl->GetCaretPos();
	const auto ullDataSize = m_pHexCtrl->GetDataSize();
	const auto fMutable = m_pHexCtrl->IsMutable();
	const auto pListBin = GetListData(EName::NAME_BINARY);
	const auto eSizeBin = pListBin->eSize;

	for (auto& ref : m_vecData) {
		ref.fAllowEdit = fMutable;
	}

	const auto ui8Data = ut::GetIHexTData<std::uint8_t>(*m_pHexCtrl, ullOffset);

	if (eSizeBin == SIZE_BYTE) {
		ShowValueBinary(ui8Data);
	}

	ShowValueInt8(ui8Data);
	ShowValueUInt8(ui8Data);

	//EDataSize::SIZE_WORD
	if (ullOffset + static_cast<std::uint8_t>(SIZE_WORD) > ullDataSize) {
		for (auto& ref : m_vecData) {
			if (ref.eSize >= SIZE_WORD) {
				ref.wstrValue.clear();
				ref.fAllowEdit = false;
			}
		}

		m_ListEx.RedrawWindow();
		return;
	}

	auto ui16Data = ut::GetIHexTData<std::uint16_t>(*m_pHexCtrl, ullOffset);
	if (IsBigEndian()) {
		ui16Data = ut::ByteSwap(ui16Data);
	}

	if (eSizeBin == SIZE_WORD) {
		ShowValueBinary(ui16Data);
	}

	ShowValueInt16(ui16Data);
	ShowValueUInt16(ui16Data);

	//EDataSize::SIZE_DWORD
	if (ullOffset + static_cast<std::uint8_t>(SIZE_DWORD) > ullDataSize) {
		for (auto& ref : m_vecData) {
			if (ref.eSize >= SIZE_DWORD) {
				ref.wstrValue.clear();
				ref.fAllowEdit = false;
			}
		}

		m_ListEx.RedrawWindow();
		return;
	}

	auto ui32Data = ut::GetIHexTData<std::uint32_t>(*m_pHexCtrl, ullOffset);
	if (IsBigEndian()) {
		ui32Data = ut::ByteSwap(ui32Data);
	}

	if (eSizeBin == SIZE_DWORD) {
		ShowValueBinary(ui32Data);
	}

	ShowValueInt32(ui32Data);
	ShowValueUInt32(ui32Data);
	ShowValueFloat(ui32Data);
	ShowValueTime32(ui32Data);
	ShowValueMSDOSTIME(ui32Data);
	ShowValueMSDTTMTIME(ui32Data);

	//EDataSize::SIZE_QWORD
	if (ullOffset + static_cast<std::uint8_t>(SIZE_QWORD) > ullDataSize) {
		for (auto& ref : m_vecData) {
			if (ref.eSize >= SIZE_QWORD) {
				ref.wstrValue.clear();
				ref.fAllowEdit = false;
			}
		}

		m_ListEx.RedrawWindow();
		return;
	}

	auto ui64Data = ut::GetIHexTData<std::uint64_t>(*m_pHexCtrl, ullOffset);
	if (IsBigEndian()) {
		ui64Data = ut::ByteSwap(ui64Data);
	}

	if (eSizeBin == SIZE_QWORD) {
		ShowValueBinary(ui64Data);
	}

	ShowValueInt64(ui64Data);
	ShowValueUInt64(ui64Data);
	ShowValueDouble(ui64Data);
	ShowValueTime64(ui64Data);
	ShowValueFILETIME(ui64Data);
	ShowValueOLEDATETIME(ui64Data);
	ShowValueJAVATIME(ui64Data);

	//EDataSize::SIZE_DQWORD
	if (ullOffset + static_cast<std::uint8_t>(SIZE_DQWORD) > ullDataSize) {
		for (auto& ref : m_vecData) {
			if (ref.eSize >= SIZE_DQWORD) {
				ref.wstrValue.clear();
				ref.fAllowEdit = false;
			}
		}

		m_ListEx.RedrawWindow();
		return;
	}

	//Getting DQWORD size of data in form of SYSTEMTIME.
	//GUID could be used instead, it doesn't matter, they are same size.
	const auto dblQWORD = ut::GetIHexTData<SYSTEMTIME>(*m_pHexCtrl, ullOffset);

	//Note: big-endian swapping is INDIVIDUAL for every struct.
	ShowValueSYSTEMTIME(dblQWORD);
	ShowValueGUID(std::bit_cast<GUID>(dblQWORD));
	ShowValueGUIDTIME(std::bit_cast<GUID>(dblQWORD));
	m_ListEx.RedrawWindow();
}


//Private methods.

auto CHexDlgDataInterp::GetListData(EName eName)const->const LISTDATA*
{
	const auto it = std::find_if(m_vecData.cbegin(), m_vecData.cend(), [=](const LISTDATA& ref) {
		return ref.eName == eName; });
	return it != m_vecData.end() ? &*it : nullptr;
}

auto CHexDlgDataInterp::GetListData(EName eName)->LISTDATA*
{
	const auto it = std::find_if(m_vecData.begin(), m_vecData.end(), [=](const LISTDATA& ref) {
		return ref.eName == eName; });
	return it != m_vecData.end() ? &*it : nullptr;
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

		if (m_pHexCtrl->IsDataSet()) {
			UpdateData();
		}
	}
	else {
		m_dwHglDataSize = 0; //Remove data highlighting when dialog window is inactive.
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

	using enum EGroup; using enum EName; using enum EDataSize;
	m_vecData.reserve(21);
	m_vecData.emplace_back(L"Binary view:", L"", GR_BINARY, NAME_BINARY, SIZE_BYTE);
	m_vecData.emplace_back(L"Int8:", L"", GR_INTEGRAL, NAME_INT8, SIZE_BYTE);
	m_vecData.emplace_back(L"Unsigned Int8:", L"", GR_INTEGRAL, NAME_UINT8, SIZE_BYTE);
	m_vecData.emplace_back(L"Int16:", L"", GR_INTEGRAL, NAME_INT16, SIZE_WORD);
	m_vecData.emplace_back(L"Unsigned Int16:", L"", GR_INTEGRAL, NAME_UINT16, SIZE_WORD);
	m_vecData.emplace_back(L"Int32:", L"", GR_INTEGRAL, NAME_INT32, SIZE_DWORD);
	m_vecData.emplace_back(L"Unsigned Int32:", L"", GR_INTEGRAL, NAME_UINT32, SIZE_DWORD);
	m_vecData.emplace_back(L"Int64:", L"", GR_INTEGRAL, NAME_INT64, SIZE_QWORD);
	m_vecData.emplace_back(L"Unsigned Int64:", L"", GR_INTEGRAL, NAME_UINT64, SIZE_QWORD);
	m_vecData.emplace_back(L"Float:", L"", GR_FLOAT, NAME_FLOAT, SIZE_DWORD);
	m_vecData.emplace_back(L"Double:", L"", GR_FLOAT, NAME_DOUBLE, SIZE_QWORD);
	m_vecData.emplace_back(L"time32_t:", L"", GR_TIME, NAME_TIME32T, SIZE_DWORD);
	m_vecData.emplace_back(L"time64_t:", L"", GR_TIME, NAME_TIME64T, SIZE_QWORD);
	m_vecData.emplace_back(L"FILETIME:", L"", GR_TIME, NAME_FILETIME, SIZE_QWORD);
	m_vecData.emplace_back(L"OLE time:", L"", GR_TIME, NAME_OLEDATETIME, SIZE_QWORD);
	m_vecData.emplace_back(L"Java time:", L"", GR_TIME, NAME_JAVATIME, SIZE_QWORD);
	m_vecData.emplace_back(L"MS-DOS time:", L"", GR_TIME, NAME_MSDOSTIME, SIZE_QWORD);
	m_vecData.emplace_back(L"MS-UDTTM time:", L"", GR_TIME, NAME_MSDTTMTIME, SIZE_DWORD);
	m_vecData.emplace_back(L"Windows SYSTEMTIME:", L"", GR_TIME, NAME_SYSTEMTIME, SIZE_DQWORD);
	m_vecData.emplace_back(L"GUID:", L"", GR_MISC, NAME_GUID, SIZE_DQWORD);
	m_vecData.emplace_back(L"GUID v1 UTC time:", L"", GR_GUIDTIME, NAME_GUIDTIME, SIZE_DQWORD);

	m_ListEx.Create({ .hWndParent { m_Wnd }, .uID { IDC_HEXCTRL_DATAINTERP_LIST }, .fDialogCtrl { true } });
	m_ListEx.InsertColumn(0, L"Data type", LVCFMT_LEFT, 143);
	m_ListEx.InsertColumn(1, L"Value", LVCFMT_LEFT, 240, -1, LVCFMT_LEFT, true);
	m_ListEx.SetItemCountEx(static_cast<int>(m_vecData.size()), LVSICF_NOSCROLL);

	m_DynLayout.SetHost(m_Wnd);
	m_DynLayout.AddItem(IDC_HEXCTRL_DATAINTERP_LIST, wnd::CDynLayout::MoveNone(), wnd::CDynLayout::SizeHorzAndVert(100, 100));
	m_DynLayout.AddItem(IDC_HEXCTRL_DATAINTERP_CHK_HEX, wnd::CDynLayout::MoveVert(100), wnd::CDynLayout::SizeNone());
	m_DynLayout.AddItem(IDC_HEXCTRL_DATAINTERP_CHK_BE, wnd::CDynLayout::MoveVert(100), wnd::CDynLayout::SizeNone());
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
		case LVN_ITEMCHANGED: OnNotifyListItemChanged(pNMHDR); break;
		case LISTEX::LISTEX_MSG_GETCOLOR: OnNotifyListGetColor(pNMHDR); break;
		case LISTEX::LISTEX_MSG_SETDATA: OnNotifyListSetData(pNMHDR); break;
		case LISTEX::LISTEX_MSG_EDITBEGIN: OnNotifyListEditBegin(pNMHDR); break;
		default: break;
		}
	default: break;
	}

	return TRUE;
}

void CHexDlgDataInterp::OnNotifyListEditBegin(NMHDR* pNMHDR)
{
	const auto pListDataInfo = reinterpret_cast<LISTEX::PLISTEXDATAINFO>(pNMHDR);
	const auto iItem = pListDataInfo->iItem;
	const auto iSubItem = pListDataInfo->iSubItem;
	if (iItem < 0 || iSubItem < 0 || iItem > static_cast<int>(m_vecData.size()))
		return;

	pListDataInfo->fAllowEdit = m_vecData[iItem].fAllowEdit;
}

void CHexDlgDataInterp::OnNotifyListGetColor(NMHDR* pNMHDR)
{
	const auto pLCI = reinterpret_cast<LISTEX::PLISTEXCOLORINFO>(pNMHDR);
	const auto iItem = pLCI->iItem;
	const auto iSubItem = pLCI->iSubItem;
	if (iItem < 0 || iSubItem < 0)
		return;

	const auto& ref = m_vecData[iItem];

	if (ref.eName == EName::NAME_BINARY) {
		pLCI->stClr.clrBk = RGB(240, 255, 255);
		return;
	}

	if (!ref.fAllowEdit) {
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
		pItem->pszText = m_vecData[iItem].wstrDataType.data();
		break;
	case 1: //Value.
		pItem->pszText = m_vecData[iItem].wstrValue.data();
		break;
	default: break;
	}
}

void CHexDlgDataInterp::OnNotifyListItemChanged(NMHDR* pNMHDR)
{
	const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	const auto iItem = pNMI->iItem;
	const auto iSubitem = pNMI->iSubItem;
	if (iItem < 0 || iSubitem < 0 || ((pNMI->uNewState & LVIS_SELECTED) == 0)) {
		return;
	}

	const auto& refData = m_vecData[iItem];
	const auto ui8DataSize = static_cast<std::uint8_t>(refData.eSize);
	using enum EName; using enum EDataSize;
	if (refData.eName != NAME_BINARY) {
		const auto pGridBin = GetListData(NAME_BINARY);
		pGridBin->eSize = refData.eSize;
		const auto ullDataSize = m_pHexCtrl->GetDataSize();

		if (m_ullOffset + ui8DataSize > ullDataSize || ui8DataSize == 16) {
			pGridBin->wstrValue.clear();
			pGridBin->fAllowEdit = false;
		}
		else {
			pGridBin->fAllowEdit = true;
			switch (ui8DataSize) {
			case 1:
				ShowValueBinary(ut::GetIHexTData<std::uint8_t>(*m_pHexCtrl, m_ullOffset));
				break;
			case 2:
				ShowValueBinary(ut::GetIHexTData<std::uint16_t>(*m_pHexCtrl, m_ullOffset));
				break;
			case 4:
				ShowValueBinary(ut::GetIHexTData<std::uint32_t>(*m_pHexCtrl, m_ullOffset));
				break;
			case 8:
				ShowValueBinary(ut::GetIHexTData<std::uint64_t>(*m_pHexCtrl, m_ullOffset));
				break;
			default:
				break;
			};
		}
	}

	m_dwHglDataSize = ui8DataSize;
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

	const auto eName = m_vecData[iItem].eName;
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
	default:
		break;
	};

	if (!fSuccess) {
		::MessageBoxW(m_Wnd, L"Wrong number format or out of range.", L"Data error...", MB_ICONERROR);
	}
	else {
		RedrawHexCtrl();
	}

	UpdateData();
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
	const auto pGrid = GetListData(EName::NAME_BINARY);
	const auto eSize = pGrid->eSize;
	auto wstr = std::wstring { wsv };
	std::erase(wstr, L' ');
	const auto uiSizeBits = static_cast<std::uint32_t>(static_cast<std::uint8_t>(eSize) * 8);

	if (wstr.size() != uiSizeBits || wstr.find_first_not_of(L"01") != std::wstring::npos)
		return false;

	using enum EDataSize;
	switch (eSize) {
	case SIZE_BYTE:
		if (const auto opt = stn::StrToUInt8(wstr, 2); opt) {
			SetTData(*opt);
			return true;
		}
		return false;
	case SIZE_WORD:
		if (const auto opt = stn::StrToUInt16(wstr, 2); opt) {
			SetTData(*opt);
			return true;
		}
		return false;
	case SIZE_DWORD:
		if (const auto opt = stn::StrToUInt32(wstr, 2); opt) {
			SetTData(*opt);
			return true;
		}
		return false;
	case SIZE_QWORD:
		if (const auto opt = stn::StrToUInt64(wstr, 2); opt) {
			SetTData(*opt);
			return true;
		}
		return false;
	default:
		return false;
	};
}

template<typename T> requires ut::TSize1248<T>
bool CHexDlgDataInterp::SetDataNUMBER(std::wstring_view wsv)const
{
	if (IsShowAsHex()) {
		//Unsigned type in case of float or double.
		using UTF = std::conditional_t<std::is_same_v<T, float>, std::uint32_t, std::uint64_t>;
		//Unsigned type in case of Integral or float/double.
		using UT = typename std::conditional_t<std::is_integral_v<T>, std::make_unsigned<T>, std::type_identity<UTF>>::type;
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
	if (lTicks.QuadPart >= LONG_MAX)
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

template<ut::TSize1248 T>
void CHexDlgDataInterp::SetTData(T tData)const
{
	if (IsBigEndian()) {
		tData = ut::ByteSwap(tData);
	}

	ut::SetIHexTData(*m_pHexCtrl, m_ullOffset, tData);
}

template<ut::TSize1248 T>
void CHexDlgDataInterp::ShowValueBinary(T tData)
{
	const auto pGrid = GetListData(EName::NAME_BINARY);
	constexpr auto pSepar { L" " };
	if constexpr (sizeof(T) == sizeof(std::uint8_t)) {
		pGrid->wstrValue = std::format(L"{:08b}", tData);
	}
	if constexpr (sizeof(T) == sizeof(std::uint16_t)) {
		auto wstr = std::format(L"{:016b}", tData);
		wstr.insert(8, pSepar);
		pGrid->wstrValue = std::move(wstr);
	}
	if constexpr (sizeof(T) == sizeof(std::uint32_t)) {
		auto wstr = std::format(L"{:032b}", tData);
		int iIndex = 0;
		for (auto i = 1; i < sizeof(std::uint32_t); ++i) {
			wstr.insert(i * 8 + iIndex++ * 2, pSepar);
		}
		pGrid->wstrValue = std::move(wstr);
	}
	if constexpr (sizeof(T) == sizeof(std::uint64_t)) {
		auto wstr = std::format(L"{:064b}", tData);
		int iIndex = 0;
		for (auto i = 1; i < sizeof(std::uint64_t); ++i) {
			wstr.insert(i * 8 + iIndex++ * 2, pSepar);
		}
		pGrid->wstrValue = std::move(wstr);
	}
}

void CHexDlgDataInterp::ShowValueInt8(BYTE byte)
{
	const auto i8 = static_cast<std::int8_t>(byte);
	GetListData(EName::NAME_INT8)->wstrValue = std::vformat(IsShowAsHex() ? L"0x{0:02X}" : L"{1}",
		std::make_wformat_args(byte, i8));
}

void CHexDlgDataInterp::ShowValueUInt8(BYTE byte)
{
	GetListData(EName::NAME_UINT8)->wstrValue = std::vformat(IsShowAsHex() ? L"0x{:02X}" : L"{}",
		std::make_wformat_args(byte));
}

void CHexDlgDataInterp::ShowValueInt16(WORD word)
{
	const auto i16 = static_cast<std::int16_t>(word);
	GetListData(EName::NAME_INT16)->wstrValue = std::vformat(IsShowAsHex() ? L"0x{0:04X}" : L"{1}",
		std::make_wformat_args(word, i16));
}

void CHexDlgDataInterp::ShowValueUInt16(WORD word)
{
	GetListData(EName::NAME_UINT16)->wstrValue = std::vformat(IsShowAsHex() ? L"0x{:04X}" : L"{}",
		std::make_wformat_args(word));
}

void CHexDlgDataInterp::ShowValueInt32(DWORD dword)
{
	const auto i32 = static_cast<std::int32_t>(dword);
	GetListData(EName::NAME_INT32)->wstrValue = std::vformat(IsShowAsHex() ? L"0x{0:08X}" : L"{1}",
		std::make_wformat_args(dword, i32));
}

void CHexDlgDataInterp::ShowValueUInt32(DWORD dword)
{
	GetListData(EName::NAME_UINT32)->wstrValue = std::vformat(IsShowAsHex() ? L"0x{:08X}" : L"{}",
		std::make_wformat_args(dword));
}

void CHexDlgDataInterp::ShowValueInt64(std::uint64_t qword)
{
	const auto i64 = static_cast<std::int64_t>(qword);
	GetListData(EName::NAME_INT64)->wstrValue = std::vformat(IsShowAsHex() ? L"0x{0:016X}" : L"{1}",
		std::make_wformat_args(qword, i64));
}

void CHexDlgDataInterp::ShowValueUInt64(std::uint64_t qword)
{
	GetListData(EName::NAME_UINT64)->wstrValue = std::vformat(IsShowAsHex() ? L"0x{:016X}" : L"{}",
		std::make_wformat_args(qword));
}

void CHexDlgDataInterp::ShowValueFloat(DWORD dword)
{
	const auto fl = std::bit_cast<float>(dword);
	GetListData(EName::NAME_FLOAT)->wstrValue = std::vformat(IsShowAsHex() ? L"0x{0:08X}" : L"{1:.9e}",
		std::make_wformat_args(dword, fl));
}

void CHexDlgDataInterp::ShowValueDouble(std::uint64_t qword)
{
	const auto dbl = std::bit_cast<double>(qword);
	GetListData(EName::NAME_DOUBLE)->wstrValue = std::vformat(IsShowAsHex() ? L"0x{0:016X}" : L"{1:.18e}",
		std::make_wformat_args(qword, dbl));
}

void CHexDlgDataInterp::ShowValueTime32(DWORD dword)
{
	std::wstring wstrTime = L"N/A";
	//The number of seconds since midnight January 1st 1970 UTC (32-bit). This is signed and wraps on 19 January 2038.
	const auto lTime32 = static_cast<__time32_t>(dword);

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

	GetListData(EName::NAME_TIME32T)->wstrValue = std::move(wstrTime);
}

void CHexDlgDataInterp::ShowValueTime64(std::uint64_t qword)
{
	std::wstring wstrTime = L"N/A";
	const auto llTime64 = static_cast<__time64_t>(qword); //The number of seconds since midnight January 1st 1970 UTC (64-bit).

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

	GetListData(EName::NAME_TIME64T)->wstrValue = std::move(wstrTime);
}

void CHexDlgDataInterp::ShowValueFILETIME(std::uint64_t qword)
{
	GetListData(EName::NAME_FILETIME)->wstrValue = ut::FileTimeToString(std::bit_cast<FILETIME>(qword),
		m_dwDateFormat, m_wchDateSepar);
}

void CHexDlgDataInterp::ShowValueOLEDATETIME(std::uint64_t qword)
{
	//OLE (including MS Office) date/time.
	//Implemented using an 8-byte floating-point number. 
	//Days are represented as whole number increments starting with 30 December 1899, midnight as time zero.
	//See: https://docs.microsoft.com/en-us/cpp/atl-mfc-shared/date-type?view=vs-2019

	std::wstring wstrTime = L"N/A";
	if (SYSTEMTIME stSysTime; ::VariantTimeToSystemTime(std::bit_cast<DATE>(qword), &stSysTime) == TRUE) {
		wstrTime = ut::SystemTimeToString(stSysTime, m_dwDateFormat, m_wchDateSepar);
	}

	GetListData(EName::NAME_OLEDATETIME)->wstrValue = std::move(wstrTime);
}

void CHexDlgDataInterp::ShowValueJAVATIME(std::uint64_t qword)
{
	//Javatime (signed).
	//Number of milliseconds after/before January 1, 1970, 00:00:00 UTC.

	//Add/subtract milliseconds from epoch time.
	LARGE_INTEGER Time { { .LowPart { ut::g_ulFileTime1970_LOW }, .HighPart { ut::g_ulFileTime1970_HIGH } } };
	Time.QuadPart += qword * ut::g_uFTTicksPerMS * (static_cast<LONGLONG>(qword) >= 0 ? 1 : -1);

	//Convert to FILETIME.
	const FILETIME ftJavaTime { .dwLowDateTime { Time.LowPart }, .dwHighDateTime { static_cast<DWORD>(Time.HighPart) } };
	GetListData(EName::NAME_JAVATIME)->wstrValue = ut::FileTimeToString(ftJavaTime, m_dwDateFormat, m_wchDateSepar);
}

void CHexDlgDataInterp::ShowValueMSDOSTIME(DWORD dword)
{
	std::wstring wstrTime = L"N/A";
	const UMSDOSDateTime msdosDateTime { .dwTimeDate { dword } };
	if (FILETIME ftMSDOS;
		::DosDateTimeToFileTime(msdosDateTime.TimeDate.wDate, msdosDateTime.TimeDate.wTime, &ftMSDOS) != FALSE) {
		wstrTime = ut::FileTimeToString(ftMSDOS, m_dwDateFormat, m_wchDateSepar);
	}

	GetListData(EName::NAME_MSDOSTIME)->wstrValue = std::move(wstrTime);
}

void CHexDlgDataInterp::ShowValueMSDTTMTIME(DWORD dword)
{
	//Microsoft UDTTM time (as used by Microsoft Compound Document format).
	std::wstring wstrTime = L"N/A";
	const UDTTM dttm { .dwValue { dword } };
	if (dttm.components.dayofmonth > 0 && dttm.components.dayofmonth < 32
		&& dttm.components.hour < 24 && dttm.components.minute < 60
		&& dttm.components.month>0 && dttm.components.month < 13 && dttm.components.weekday < 7) {
		const SYSTEMTIME stSysTime { .wYear { static_cast<WORD>(1900 + dttm.components.year) },
			.wMonth { static_cast<WORD>(dttm.components.month) }, .wDayOfWeek { static_cast<WORD>(dttm.components.weekday) },
			.wDay { static_cast<WORD>(dttm.components.dayofmonth) }, .wHour { static_cast<WORD>(dttm.components.hour) },
			.wMinute { static_cast<WORD>(dttm.components.minute) }, .wSecond { 0 }, .wMilliseconds { 0 } };

		wstrTime = ut::SystemTimeToString(stSysTime, m_dwDateFormat, m_wchDateSepar);
	}

	GetListData(EName::NAME_MSDTTMTIME)->wstrValue = std::move(wstrTime);
}

void CHexDlgDataInterp::ShowValueSYSTEMTIME(SYSTEMTIME stSysTime)
{
	if (IsBigEndian()) {
		stSysTime.wYear = ut::ByteSwap(stSysTime.wYear);
		stSysTime.wMonth = ut::ByteSwap(stSysTime.wMonth);
		stSysTime.wDayOfWeek = ut::ByteSwap(stSysTime.wDayOfWeek);
		stSysTime.wDay = ut::ByteSwap(stSysTime.wDay);
		stSysTime.wHour = ut::ByteSwap(stSysTime.wHour);
		stSysTime.wMinute = ut::ByteSwap(stSysTime.wMinute);
		stSysTime.wSecond = ut::ByteSwap(stSysTime.wSecond);
		stSysTime.wMilliseconds = ut::ByteSwap(stSysTime.wMilliseconds);
	}

	GetListData(EName::NAME_SYSTEMTIME)->wstrValue = ut::SystemTimeToString(stSysTime, m_dwDateFormat, m_wchDateSepar);
}

void CHexDlgDataInterp::ShowValueGUID(GUID stGUID)
{
	if (IsBigEndian()) {
		stGUID.Data1 = ut::ByteSwap(stGUID.Data1);
		stGUID.Data2 = ut::ByteSwap(stGUID.Data2);
		stGUID.Data3 = ut::ByteSwap(stGUID.Data3);
	}

	GetListData(EName::NAME_GUID)->wstrValue =
		std::format(L"{{{:0>8x}-{:0>4x}-{:0>4x}-{:0>2x}{:0>2x}-{:0>2x}{:0>2x}{:0>2x}{:0>2x}{:0>2x}{:0>2x}}}",
		stGUID.Data1, stGUID.Data2, stGUID.Data3, stGUID.Data4[0],
		stGUID.Data4[1], stGUID.Data4[2], stGUID.Data4[3], stGUID.Data4[4],
		stGUID.Data4[5], stGUID.Data4[6], stGUID.Data4[7]);
}

void CHexDlgDataInterp::ShowValueGUIDTIME(GUID stGUID)
{
	if (IsBigEndian()) {
		stGUID.Data1 = ut::ByteSwap(stGUID.Data1);
		stGUID.Data2 = ut::ByteSwap(stGUID.Data2);
		stGUID.Data3 = ut::ByteSwap(stGUID.Data3);
	}

	//Guid v1 Datetime UTC. The time structure within the NAME_GUID.
	//First, verify GUID is actually version 1 style.
	std::wstring wstrTime = L"N/A";
	const unsigned short unGuidVersion = (stGUID.Data3 & 0xF000UL) >> 12;
	if (unGuidVersion == 1) {
		LARGE_INTEGER qwGUIDTime { .LowPart { stGUID.Data1 }, .HighPart { stGUID.Data3 & 0x0FFFUL } };
		qwGUIDTime.HighPart = (qwGUIDTime.HighPart << 16) | stGUID.Data2;

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
		const FILETIME ftGUIDTime { .dwLowDateTime { qwGUIDTime.LowPart }, .dwHighDateTime { static_cast<DWORD>(qwGUIDTime.HighPart) } };
		wstrTime = ut::FileTimeToString(ftGUIDTime, m_dwDateFormat, m_wchDateSepar);
	}

	GetListData(EName::NAME_GUIDTIME)->wstrValue = std::move(wstrTime);
}