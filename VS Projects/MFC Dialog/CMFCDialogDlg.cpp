#include "stdafx.h"
#include "CMFCDialogDlg.h"
#include "Resource.h"
#include <filesystem>
#include <random>
import HexCtrl_StrToNum;
import GDIUtility;

namespace stn = HEXCTRL::stn;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

constexpr const auto WstrTextRO { L"Random data: RO" };
constexpr const auto WstrTextRW { L"Random data: RW" };

BEGIN_MESSAGE_MAP(CMFCDialogDlg, CDialogEx)
	ON_MESSAGE(WM_DPICHANGED, &CMFCDialogDlg::OnDPIChanged)
	ON_MESSAGE(WM_GETDPISCALEDSIZE, &CMFCDialogDlg::OnGetDPIScaledSize)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_SETDATARND, &CMFCDialogDlg::OnBnSetRndData)
	ON_BN_CLICKED(IDC_CLEARDATA, &CMFCDialogDlg::OnBnClearData)
	ON_BN_CLICKED(IDC_FILEOPEN, &CMFCDialogDlg::OnBnFileOpen)
	ON_BN_CLICKED(IDC_HEXPOPUP, &CMFCDialogDlg::OnBnPopup)
	ON_BN_CLICKED(IDC_CHK_RW, &CMFCDialogDlg::OnChkRW)
	ON_NOTIFY(HEXCTRL::HEXCTRL_MSG_CONTEXTMENU, IDC_MY_HEX, &CMFCDialogDlg::OnNotifyHexContextMenu)
	ON_NOTIFY(HEXCTRL::HEXCTRL_MSG_TEMPL_LIMIT, IDC_MY_HEX, &CMFCDialogDlg::OnNotifyHexTemplLimit)
	ON_WM_DROPFILES()
END_MESSAGE_MAP()

void CMFCDialogDlg::SetStartupFile(LPCWSTR pwszFile) {
	m_wstrStartupFile = pwszFile;
}


//Private methods.

CMFCDialogDlg::CMFCDialogDlg(CWnd* pParent) : CDialogEx(IDD_HEXCTRL_SAMPLE, pParent) { }

void CMFCDialogDlg::CreateHexPopup() {
	if (m_pHexPopup->IsCreated())
		return;

	constexpr auto dwStyle = WS_POPUP | WS_OVERLAPPEDWINDOW;
	constexpr auto dwExStyle = WS_EX_APPWINDOW; //To force entry to the taskbar.

	m_pHexPopup->Create({ .hWndParent { m_hWnd }, .dwStyle { dwStyle }, .dwExStyle { dwExStyle } });
	m_hds.fMutable = IsRW();
	if (!m_hds.spnData.empty()) {
		m_pHexPopup->SetData(m_hds);
	}

	SetIconsForHexCtrl(*m_pHexPopup);

	const auto hWndHex = m_pHexPopup->GetWndHandle(EHexWnd::WND_MAIN);
	const auto iWidthActual = m_pHexPopup->GetActualWidth() + ::GetSystemMetrics(SM_CXVSCROLL);
	CRect rcHex(0, 0, iWidthActual, iWidthActual); //Square window.
	::AdjustWindowRectEx(rcHex, dwStyle, FALSE, dwExStyle);
	const auto iWidth = rcHex.Width();
	const auto iHeight = rcHex.Height() - rcHex.Height() / 3;
	const auto iPosX = ::GetSystemMetrics(SM_CXSCREEN) / 2 - iWidth / 2;
	const auto iPosY = ::GetSystemMetrics(SM_CYSCREEN) / 2 - iHeight / 2;
	m_pHexPopup->SetWindowPos(m_hWnd, iPosX, iPosY, iWidth, iHeight, SWP_NOACTIVATE);

	const auto hIconSmall = static_cast<HICON>(::LoadImageW(AfxGetInstanceHandle(),
		MAKEINTRESOURCEW(IDR_MAINFRAME), IMAGE_ICON, 0, 0, 0));
	const auto hIconBig = static_cast<HICON>(::LoadImageW(AfxGetInstanceHandle(),
		MAKEINTRESOURCEW(IDR_MAINFRAME), IMAGE_ICON, 96, 96, 0));
	if (hIconSmall != nullptr) {
		::SendMessageW(hWndHex, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIconSmall));
		::SendMessageW(hWndHex, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIconBig));
	}
}

void CMFCDialogDlg::CreateIconsForHexCtrl() {
	using enum HEXCTRL::EHexMenuItem;
	m_vecHexIcons.clear();
	const auto flDPIScale = GDIUT::GetDPIScaleForHWND(m_hWnd);
	const auto iSizeIcon = static_cast<int>(16 * flDPIScale);

	auto hBitmap = GDIUT::SVGToHBITMAP(IDSVG_SEARCH, iSizeIcon, iSizeIcon);
	m_vecHexIcons.emplace_back(IDM_SEARCH_POPUP, hBitmap);
	m_vecHexIcons.emplace_back(IDM_SEARCH_SEARCH, hBitmap);
	hBitmap = GDIUT::SVGToHBITMAP(IDSVG_GROUP, iSizeIcon, iSizeIcon);
	m_vecHexIcons.emplace_back(IDM_GROUPDATA_POPUP, hBitmap);
	hBitmap = GDIUT::SVGToHBITMAP(IDSVG_BKM, iSizeIcon, iSizeIcon);
	m_vecHexIcons.emplace_back(IDM_BKM_POPUP, hBitmap);
	m_vecHexIcons.emplace_back(IDM_BKM_ADD, hBitmap);
	hBitmap = GDIUT::SVGToHBITMAP(IDSVG_CLPBRD_COPYHEX, iSizeIcon, iSizeIcon);
	m_vecHexIcons.emplace_back(IDM_CLPBRD_POPUP, hBitmap);
	m_vecHexIcons.emplace_back(IDM_CLPBRD_COPYHEX, hBitmap);
	hBitmap = GDIUT::SVGToHBITMAP(IDSVG_CLPBRD_PASTEHEX, iSizeIcon, iSizeIcon);
	m_vecHexIcons.emplace_back(IDM_CLPBRD_PASTEHEX, hBitmap);
	hBitmap = GDIUT::SVGToHBITMAP(IDSVG_MODIFY, iSizeIcon, iSizeIcon);
	m_vecHexIcons.emplace_back(IDM_MODIFY_POPUP, hBitmap);
	hBitmap = GDIUT::SVGToHBITMAP(IDSVG_MODIFY_FILLZEROS, iSizeIcon, iSizeIcon);
	m_vecHexIcons.emplace_back(IDM_MODIFY_FILLZEROS, hBitmap);
	hBitmap = GDIUT::SVGToHBITMAP(IDSVG_FONTCHOOSE, iSizeIcon, iSizeIcon);
	m_vecHexIcons.emplace_back(IDM_APPEAR_CHOOSEFONT, hBitmap);
}

void CMFCDialogDlg::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHK_RW, m_chkRW);
	DDX_Control(pDX, IDC_CHK_LNK, m_chkDRLNK);
	DDX_Control(pDX, IDC_EDIT_DATASIZE, m_editDataSize);
}

void CMFCDialogDlg::FileOpen(std::wstring_view wsvPath, bool fResolveLnk) {
	FileClose();

	std::wstring wstrPath(wsvPath);
	if (fResolveLnk && wstrPath.ends_with(L".lnk")) {
		wstrPath = ResolveLNK(wstrPath.data());
	}

	if (m_hFile = ::CreateFileW(wstrPath.data(), GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr); m_hFile == INVALID_HANDLE_VALUE) {
		const std::wstring wstr = L"CreateFileW failed.\r\n" + GetLastErrorWstr();
		MessageBoxW(wstr.data(), L"Error", MB_ICONERROR);
		return;
	}

	if (m_hMapObject = ::CreateFileMappingW(m_hFile, nullptr, PAGE_READWRITE, 0, 0, nullptr); !m_hMapObject) {
		::CloseHandle(m_hFile);
		const std::wstring wstr = L"CreateFileMappingW failed.\r\n" + GetLastErrorWstr();
		MessageBoxW(wstr.data(), L"Error", MB_ICONERROR);
		return;
	}

	if (m_lpBase = ::MapViewOfFile(m_hMapObject, FILE_MAP_WRITE, 0, 0, 0); !m_lpBase) {
		const std::wstring wstr = L"MapViewOfFileW failed.\r\n" + GetLastErrorWstr();
		MessageBoxW(wstr.data(), L"Error", MB_ICONERROR);
		::CloseHandle(m_hMapObject);
		::CloseHandle(m_hFile);
		return;
	}

	m_fFileOpen = true;
	LARGE_INTEGER stFileSize;
	::GetFileSizeEx(m_hFile, &stFileSize);
	m_hds.spnData = { static_cast<std::byte*>(m_lpBase), static_cast<std::size_t>(stFileSize.QuadPart) };
	m_hds.fMutable = IsRW();
	m_pHexDlg->SetData(m_hds);
	if (m_pHexPopup->IsCreated()) {
		m_pHexPopup->SetData(m_hds);
		::SetWindowTextW(m_pHexPopup->GetWndHandle(EHexWnd::WND_MAIN), wstrPath.data());
	}

	m_pData.reset();
	SetWindowTextW(wstrPath.data());
}

void CMFCDialogDlg::FileClose() {
	if (!IsFileOpen())
		return;

	m_pHexDlg->ClearData();
	if (m_pHexPopup->IsCreated()) {
		m_pHexPopup->ClearData();
	}

	if (m_lpBase)
		::UnmapViewOfFile(m_lpBase);
	if (m_hMapObject)
		::CloseHandle(m_hMapObject);
	if (m_hFile)
		::CloseHandle(m_hFile);

	m_fFileOpen = false;
}

bool CMFCDialogDlg::IsFileOpen()const {
	return m_fFileOpen;
}

bool CMFCDialogDlg::IsResolveLNK()const {
	return m_chkDRLNK.GetCheck() == BST_UNCHECKED;
}

bool CMFCDialogDlg::IsRW()const {
	return m_chkRW.GetCheck() == BST_CHECKED;
}

void CMFCDialogDlg::OnBnClearData() {
	FileClose();
	m_pHexDlg->ClearData();
	if (m_pHexPopup->IsCreated()) {
		m_pHexPopup->ClearData();
		::SetWindowTextW(m_pHexPopup->GetWndHandle(EHexWnd::WND_MAIN), L"");
	}

	m_hds.spnData = { };
	SetWindowTextW(L"HexCtrl Sample Dialog");
}

void CMFCDialogDlg::OnBnSetRndData() {
	if (IsFileOpen()) {
		FileClose();
	}

	CStringW wstr;
	m_editDataSize.GetWindowTextW(wstr);
	if (wstr.IsEmpty()) {
		m_editDataSize.SetFocus();
		MessageBoxW(L"Set the data size", 0, MB_ICONERROR);
		return;
	}

	const auto optSize = stn::StrToUInt32(wstr.GetString());
	if (!optSize) {
		m_editDataSize.SetFocus();
		MessageBoxW(L"Data size is incorrect.", 0, MB_ICONERROR);
		return;
	}

	m_pData.reset(new std::byte[*optSize]);
	std::mt19937 gen(std::random_device { }());
	std::uniform_int_distribution<std::uint64_t> distInt(0, (std::numeric_limits<std::uint64_t>::max)());
	for (auto i = 0U; i < *optSize / sizeof(std::uint64_t); ++i) {
		reinterpret_cast<std::uint64_t*>(m_pData.get())[i] = distInt(gen);
	}

	m_hds.spnData = { m_pData.get(), *optSize };
	m_hds.fMutable = IsRW();
	m_pHexDlg->SetData(m_hds);
	SetWindowTextW(IsRW() ? WstrTextRW : WstrTextRO);

	if (m_pHexPopup->IsCreated()) {
		m_pHexPopup->SetData(m_hds);
		::SetWindowTextW(m_pHexPopup->GetWndHandle(EHexWnd::WND_MAIN), IsRW() ? WstrTextRW : WstrTextRO);
	}
}

void CMFCDialogDlg::OnBnFileOpen() {
	struct TEMPOFD {
		IFileDialogCustomize *pFDC { };
		DWORD dwChkLnkID { 100UL };
		bool fDontResolveLNK { };
	} tofd;

	const auto lmbBefore = [](IFileOpenDialog* pFOD, uintptr_t uzData) {
		const auto pTOFD = reinterpret_cast<TEMPOFD*>(uzData);
		if (pFOD->QueryInterface(IID_PPV_ARGS(&pTOFD->pFDC)) != S_OK) {
			return;
		}

		//Adding a check-box.
		pTOFD->pFDC->AddCheckButton(pTOFD->dwChkLnkID, L"Don't resolve .lnk", FALSE);
		};
	const auto lmbAfter = [](IFileOpenDialog*, uintptr_t uzData) {
		const auto pTOFD = reinterpret_cast<TEMPOFD*>(uzData);
		BOOL iChecked;
		pTOFD->pFDC->GetCheckButtonState(pTOFD->dwChkLnkID, &iChecked);
		pTOFD->fDontResolveLNK = iChecked == TRUE;  //"Don't Resolve .lnk" is checked.
		};

	const COMDLG_FILTERSPEC arrFilter[] { { .pszName { L"All files (*.*)" }, .pszSpec { L"*.*" } } };
	const GDIUT::OPENFILEDLG ofd { .spnFilter { arrFilter, 1 },
		.dwFlags { FOS_FORCEFILESYSTEM | FOS_NODEREFERENCELINKS | FOS_DONTADDTORECENT | FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST },
		.hWndParent { m_hWnd }, .pfnOnBeforeDlg { lmbBefore }, .uzDataBeforeDlg { reinterpret_cast<std::size_t>(&tofd) },
		.pfnOnAfterDlg { lmbAfter }, .uzDataAfterDlg { reinterpret_cast<std::size_t>(&tofd) } };
	auto optFile = GDIUT::OpenFileDlg(ofd);
	if (tofd.pFDC != nullptr) {
		tofd.pFDC->Release();
	}

	if (optFile) {
		if (!tofd.fDontResolveLNK) {
			(*optFile).front() = ResolveLNK((*optFile).front().data());
		}

		FileOpen((*optFile).front(), false);
	}
}

void CMFCDialogDlg::OnBnPopup() {
	if (!m_pHexPopup->IsCreated()) {
		CreateHexPopup();
		LoadTemplates(&*m_pHexPopup);
		::ShowWindow(m_pHexPopup->GetWndHandle(EHexWnd::WND_MAIN), SW_SHOWNORMAL);
	}
	else {
		const auto hWnd = m_pHexPopup->GetWndHandle(EHexWnd::WND_MAIN);
		::ShowWindow(hWnd, ::IsWindowVisible(hWnd) ? SW_HIDE : SW_SHOWNORMAL);
	}
}

void CMFCDialogDlg::OnChkRW() {
	if (m_pHexDlg->IsDataSet()) {
		m_pHexDlg->SetMutable(IsRW());
		if (!IsFileOpen()) {
			SetWindowTextW(IsRW() ? WstrTextRW : WstrTextRO);
		}

		if (m_pHexPopup->IsCreated() && m_pHexPopup->IsDataSet()) {
			m_pHexPopup->SetMutable(IsRW());
			if (!IsFileOpen()) {
				::SetWindowTextW(m_pHexPopup->GetWndHandle(EHexWnd::WND_MAIN), IsRW() ? WstrTextRW : WstrTextRO);
			}
		}
	}
}

void CMFCDialogDlg::OnClose() {
	FileClose();
	CDialogEx::OnClose();
}

auto CMFCDialogDlg::OnDPIChanged(WPARAM /*wParam*/, LPARAM /*lParam*/)->LRESULT {
	const auto ret = CDialogEx::Default();
	EnableDynamicLayout(TRUE);
	LoadDynamicLayoutResource(MAKEINTRESOURCEW(IDD_HEXCTRL_SAMPLE));
	GetDynamicLayout()->SetMinSize({ 0, 0 });

	CreateIconsForHexCtrl();
	SetIconsForHexCtrl(*m_pHexDlg);
	SetIconsForHexCtrl(*m_pHexPopup);

	return ret;
}

void CMFCDialogDlg::OnDropFiles(HDROP hDropInfo) {
	PVOID pOldValue;
	::Wow64DisableWow64FsRedirection(&pOldValue);

	const auto dwFilesDropped = ::DragQueryFileW(hDropInfo, 0xFFFFFFFFUL, nullptr, 0);
	if (dwFilesDropped > 0) { //If more than one file, we only use the first.
		const auto dwBuffer = ::DragQueryFileW(hDropInfo, 0, nullptr, 0);
		std::wstring wstrFile(dwBuffer, '\0');
		::DragQueryFileW(hDropInfo, 0, wstrFile.data(), dwBuffer + 1);
		FileOpen(wstrFile, IsResolveLNK());
	}
	::DragFinish(hDropInfo);

	CDialogEx::OnDropFiles(hDropInfo);
	::Wow64RevertWow64FsRedirection(pOldValue);
}

auto CMFCDialogDlg::OnGetDPIScaledSize(WPARAM /*wParam*/, LPARAM /*lParam*/)->LRESULT {
	EnableDynamicLayout(FALSE);
	return CDialogEx::Default();
}

bool CMFCDialogDlg::OnHexGetColor(HEXCOLORINFO& hci) {
	//Sample code for custom colors:
	if (hci.ullOffset < 18) {
		static std::vector<HEXCOLOR> vec {
			{ RGB(50, 0, 0), RGB(255, 255, 255) },
			{ RGB(0, 150, 0), RGB(255, 255, 255) },
			{ RGB(255, 255, 0), RGB(0, 0, 0) },
			{ RGB(0, 0, 50), RGB(255, 255, 255) },
			{ RGB(50, 50, 110), RGB(255, 255, 255) },
			{ RGB(50, 250, 50), RGB(255, 255, 255) },
			{ RGB(110, 0, 0), RGB(255, 255, 255) },
			{ RGB(0, 110, 0), RGB(255, 255, 255) },
			{ RGB(0, 0, 110), RGB(255, 255, 255) },
			{ RGB(110, 110, 0), RGB(255, 255, 255) },
			{ RGB(0, 110, 110), RGB(255, 255, 255) },
			{ RGB(110, 110, 110), RGB(255, 255, 255) },
			{ RGB(220, 0, 0), RGB(255, 255, 255) },
			{ RGB(0, 220, 0), RGB(255, 255, 255) },
			{ RGB(0, 0, 220), RGB(255, 255, 255) },
			{ RGB(220, 220, 0), RGB(255, 255, 255) },
			{ RGB(0, 220, 220), RGB(255, 255, 255) },
			{ RGB(0, 250, 0), RGB(255, 255, 255) }
		};

		hci.stClr = vec[static_cast<std::size_t>(hci.ullOffset)];
		return true;
	}

	return false;
}

BOOL CMFCDialogDlg::OnInitDialog() {
	CDialogEx::OnInitDialog();

	::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

	const auto hIcon = AfxGetApp()->LoadIconW(IDR_MAINFRAME);
	SetIcon(hIcon, TRUE);  //Set big icon
	SetIcon(hIcon, FALSE); //Set small icon
	m_editDataSize.SetWindowTextW(L"0x4000");
	m_chkRW.SetCheck(BST_CHECKED);

	//For Drag'n Drop to work in elevated mode.
	//https://helgeklein.com/blog/how-to-enable-drag-and-drop-for-an-elevated-mfc-application-on-vistawindows-7/
	::ChangeWindowMessageFilterEx(m_hWnd, WM_DROPFILES, MSGFLT_ALLOW, nullptr);
	::ChangeWindowMessageFilterEx(m_hWnd, WM_COPYDATA, MSGFLT_ALLOW, nullptr);
	::ChangeWindowMessageFilterEx(m_hWnd, 0x0049, MSGFLT_ALLOW, nullptr);
	DragAcceptFiles(TRUE);

	m_pHexDlg->CreateDialogCtrl(IDC_MY_HEX, m_hWnd);
	m_pHexDlg->SetScrollRatio(2, true); //Two lines scroll with mouse-wheel.
	m_pHexDlg->SetPageSize(64);
	//m_pHexDlg->SetDlgProperties(EHexWnd::DLG_CODEPAGE, HEXCTRL_FLAG_DLG_NOESC);
	//m_pHexDlg->SetCharsExtraSpace(2);

	LoadTemplates(&*m_pHexDlg);
	//OnBnSetRndData();
	//m_pHexDlg->ExecuteCmd(EHexCmd::CMD_BKM_DLG_MGR);

	//m_hds.pHexVirtColors = this;
	//m_hds.fHighLatency = true;

	m_chkDRLNK.SetCheck(BST_UNCHECKED);

	if (!m_wstrStartupFile.empty()) {
		FileOpen(m_wstrStartupFile, IsResolveLNK());
	}

	GetDynamicLayout()->SetMinSize({ 0, 0 });
	CreateIconsForHexCtrl();
	SetIconsForHexCtrl(*m_pHexDlg);

	return TRUE;
}

BOOL CMFCDialogDlg::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) {
	return CDialogEx::OnNotify(wParam, lParam, pResult);
}

void CMFCDialogDlg::OnNotifyHexContextMenu([[maybe_unused]] NMHDR* pNMHDR, LRESULT* /*pResult*/) {
	//const auto pHMI = reinterpret_cast<PHEXMENUINFO>(pNMHDR);
	//pHMI->fShow = false; //Ability to disable HexCtrl's context menu.
}

void CMFCDialogDlg::OnNotifyHexTemplLimit(NMHDR* pNMHDR, LRESULT* /*pResult*/) {
	const auto pHTL = reinterpret_cast<PHEXTEMPLATELIMIT>(pNMHDR);
	if (pHTL->u64ActualData < pHTL->u64LimitMin) {
		const auto wstr = std::format(L"The actual value of the `{}` field: {} is smaller\r\n"
			L"than the `limit: min' value: {}.\r\n"
			L"Use the actual data anyway (Yes), or use limit data (No)?\r\n"
			L"Pressing 'Cancel' will stop further template processing.",
			pHTL->pwszSizeFieldName, pHTL->u64ActualData, pHTL->u64LimitMin);
		switch (::MessageBoxW(m_hWnd, wstr.data(), pHTL->pwszTemplateName, MB_YESNOCANCEL | MB_ICONEXCLAMATION)) {
		case IDYES:
			//Do nothing. Using the pHTL->u64ActualData as is.
			break;
		case IDNO:
			pHTL->u64ActualData = pHTL->u64LimitMin;
			break;
		case IDCANCEL:
			pHTL->fCancel = true;
			break;
		default:
			break;
		}
	}
	else if (pHTL->u64ActualData > pHTL->u64LimitMax) {
		const auto wstr = std::format(L"The actual value of the `{}` field: {} is bigger\r\n"
			L"than the `limit: max' value: {}.\r\n"
			L"Use the actual data anyway (Yes), or use limit data (No)?\r\n"
			L"Pressing 'Cancel' will stop further template processing.",
			pHTL->pwszSizeFieldName, pHTL->u64ActualData, pHTL->u64LimitMax);
		switch (::MessageBoxW(m_hWnd, wstr.data(), pHTL->pwszTemplateName, MB_YESNOCANCEL | MB_ICONEXCLAMATION)) {
		case IDYES:
			//Do nothing. Using the pHTL->u64ActualData as is.
			break;
		case IDNO:
			pHTL->u64ActualData = pHTL->u64LimitMax;
			break;
		case IDCANCEL:
			pHTL->fCancel = true;
			break;
		default:
			break;
		}
	}
}

BOOL CMFCDialogDlg::PreTranslateMessage(MSG* pMsg) {
	if ((m_pHexDlg->IsCreated() && m_pHexDlg->PreTranslateMsg(pMsg))
		|| (m_pHexPopup->IsCreated() && m_pHexPopup->PreTranslateMsg(pMsg))) {
		return TRUE;
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CMFCDialogDlg::SetIconsForHexCtrl(IHexCtrl& HexCtrl) {
	if (!HexCtrl.IsCreated()) {
		return;
	}

	MENUITEMINFOW mii { .cbSize { sizeof(MENUITEMINFOW) }, .fMask { MIIM_BITMAP } };
	for (const auto& vec : m_vecHexIcons) {
		mii.hbmpItem = vec.hBitmap;
		HexCtrl.SetMenuItem(vec.eItem, mii);
	}
}


//Static functions.

auto CMFCDialogDlg::GetLastErrorWstr()->std::wstring {
	wchar_t wbuff[MAX_PATH];
	::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
		::GetLastError(), 0, wbuff, MAX_PATH, nullptr);
	return wbuff;
}

void CMFCDialogDlg::LoadTemplates(IHexCtrl* pHexCtrl) {
	wchar_t buff[MAX_PATH];
	::GetModuleFileNameW(nullptr, buff, MAX_PATH);
	std::wstring wstrPath = buff;
	wstrPath = wstrPath.substr(0, wstrPath.find_last_of(L'\\'));
	wstrPath += L"\\Templates\\";
	const std::filesystem::path pathTemplates { wstrPath };
	if (!std::filesystem::exists(pathTemplates))
		return;

	for (const auto& entry : std::filesystem::directory_iterator { pathTemplates }) {
		const auto& path = entry.path();
		if (path.filename() == L"HexCtrl.Templates.Schema.json") { //Do not load json schema.
			continue;
		}

		if (path.extension() == L".json") { //Add all *.json files.
			pHexCtrl->GetTemplates()->AddTemplateFile(path.c_str());
		}
	}
}

auto CMFCDialogDlg::ResolveLNK(const wchar_t* pwszPath) -> std::wstring {
	IShellLinkW* pShellLinkW;
	::CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pShellLinkW));
	IPersistFile* pPersistFile;
	pShellLinkW->QueryInterface(IID_PPV_ARGS(&pPersistFile));
	pPersistFile->Load(pwszPath, STGM_READ);
	wchar_t buff[MAX_PATH];
	const auto hr = pShellLinkW->GetPath(buff, MAX_PATH, nullptr, 0);
	pPersistFile->Release();
	pShellLinkW->Release();

	return hr == S_OK ? buff : pwszPath;
};