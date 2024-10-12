#include "stdafx.h"
#include "SampleDialogDlg.h"
#include "../../HexCtrl/dep/StrToNum/StrToNum.h"
#include "Resource.h"
#include <filesystem>
#include <random>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

constexpr const auto WstrTextRO { L"Random data: RO" };
constexpr const auto WstrTextRW { L"Random data: RW" };

BEGIN_MESSAGE_MAP(CSampleDialogDlg, CDialogEx)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_SETDATARND, &CSampleDialogDlg::OnBnSetRndData)
	ON_BN_CLICKED(IDC_CLEARDATA, &CSampleDialogDlg::OnBnClearData)
	ON_BN_CLICKED(IDC_FILEOPEN, &CSampleDialogDlg::OnBnFileOpen)
	ON_BN_CLICKED(IDC_HEXPOPUP, &CSampleDialogDlg::OnBnPopup)
	ON_BN_CLICKED(IDC_CHK_RW, &CSampleDialogDlg::OnChkRW)
	ON_WM_DROPFILES()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
END_MESSAGE_MAP()

CSampleDialogDlg::CSampleDialogDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_HEXCTRL_SAMPLE, pParent) {
	m_hIcon = AfxGetApp()->LoadIconW(IDR_MAINFRAME);
}

void CSampleDialogDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHK_RW, m_chkRW);
	DDX_Control(pDX, IDC_CHK_LNK, m_chkLnk);
	DDX_Control(pDX, IDC_EDIT_DATASIZE, m_editDataSize);
}

BOOL CSampleDialogDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

	SetIcon(m_hIcon, TRUE);	 //Set big icon
	SetIcon(m_hIcon, FALSE); //Set small icon
	m_editDataSize.SetWindowTextW(L"0x4000");
	m_chkRW.SetCheck(BST_CHECKED);

	//For Drag'n Drop to work even in elevated mode.
	//https://helgeklein.com/blog/2010/03/how-to-enable-drag-and-drop-for-an-elevated-mfc-application-on-vistawindows-7/
	ChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);
	ChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);
	ChangeWindowMessageFilter(0x0049, MSGFLT_ADD);
	DragAcceptFiles(TRUE);

	m_pHexDlg->CreateDialogCtrl(IDC_MY_HEX, m_hWnd);
	m_pHexDlg->SetScrollRatio(2, true); //Two lines scroll with mouse-wheel.
	m_pHexDlg->SetPageSize(64);
//	m_pHexDlg->SetDlgProperties(EHexWnd::DLG_CODEPAGE, HEXCTRL_FLAG_DLG_NOESC);
//	m_pHexDlg->SetCharsExtraSpace(2);

	LoadTemplates(&*m_pHexDlg);
	//OnBnSetRndData();
	//m_pHexDlg->ExecuteCmd(EHexCmd::CMD_BKM_DLG_MGR);

//	m_hds.pHexVirtColors = this;
//	m_hds.fHighLatency = true;

	m_chkLnk.SetCheck(BST_CHECKED);

	if (!m_wstrStartupFile.empty()) {
		FileOpen(m_wstrStartupFile, IsLnk());
	}

	if (const auto pDL = GetDynamicLayout(); pDL != nullptr) {
		pDL->SetMinSize({ 0, 0 });
	}

	return TRUE;
}

void CSampleDialogDlg::OnPaint()
{
	if (IsIconic()) {
		CPaintDC dc(this);

		SendMessageW(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		const auto cxIcon = GetSystemMetrics(SM_CXICON);
		const auto cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		const auto x = (rect.Width() - cxIcon + 1) / 2;
		const auto y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else {
		CDialogEx::OnPaint();
	}
}

HCURSOR CSampleDialogDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CSampleDialogDlg::OnBnClearData()
{
	FileClose();
	m_pHexDlg->ClearData();
	if (m_pHexPopup->IsCreated()) {
		m_pHexPopup->ClearData();
		::SetWindowTextW(m_pHexPopup->GetWndHandle(EHexWnd::WND_MAIN), L"");
	}

	m_hds.spnData = { };
	SetWindowTextW(L"HexCtrl Sample Dialog");
}

void CSampleDialogDlg::OnBnSetRndData()
{
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
	std::random_device rd;
	std::mt19937 gen(rd());
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

void CSampleDialogDlg::OnBnFileOpen()
{
	if (auto vecFiles = OpenFileDlg(); !vecFiles.empty()) {
		FileOpen(vecFiles.front(), IsLnk());
	}
}

void CSampleDialogDlg::OnBnPopup()
{
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

void CSampleDialogDlg::OnChkRW()
{
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

void CSampleDialogDlg::OnClose()
{
	FileClose();
	CDialogEx::OnClose();
}

void CSampleDialogDlg::OnDropFiles(HDROP hDropInfo)
{
	PVOID pOldValue;
	Wow64DisableWow64FsRedirection(&pOldValue);

	const auto nFilesDropped = DragQueryFileW(hDropInfo, 0xFFFFFFFF, nullptr, 0);
	if (nFilesDropped > 0) { //If more than one file, we only use the first.
		const auto nBuffer = DragQueryFileW(hDropInfo, 0, nullptr, 0);
		std::wstring wstrFile(nBuffer, '\0');
		DragQueryFileW(hDropInfo, 0, wstrFile.data(), nBuffer + 1);
		FileOpen(wstrFile, IsLnk());
	}
	DragFinish(hDropInfo);

	CDialogEx::OnDropFiles(hDropInfo);
	Wow64RevertWow64FsRedirection(pOldValue);
}

BOOL CSampleDialogDlg::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	const auto pNMHDR = reinterpret_cast<PHEXMENUINFO>(lParam);
	if (pNMHDR->hdr.idFrom == IDC_MY_HEX && pNMHDR->hdr.code == HEXCTRL_MSG_CONTEXTMENU) {
		// pNMHDR->fShow = false; //Ability to disable HexCtrl context menu.
	}

	return CDialogEx::OnNotify(wParam, lParam, pResult);
}

bool CSampleDialogDlg::OnHexGetColor(HEXCOLORINFO& hci)
{
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

void CSampleDialogDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
}

void CSampleDialogDlg::SetStartupFile(LPCWSTR pwszFile)
{
	m_wstrStartupFile = pwszFile;
}

void CSampleDialogDlg::CreateHexPopup()
{
	if (m_pHexPopup->IsCreated())
		return;

	constexpr auto dwStyle = WS_POPUP | WS_OVERLAPPEDWINDOW;
	constexpr auto dwExStyle = WS_EX_APPWINDOW; //To force entry to the taskbar.

	m_pHexPopup->Create({ .hWndParent { m_hWnd }, .dwStyle { dwStyle }, .dwExStyle { dwExStyle } });
	m_hds.fMutable = IsRW();
	if (!m_hds.spnData.empty()) {
		m_pHexPopup->SetData(m_hds);
	}

	const auto hWndHex = m_pHexPopup->GetWndHandle(EHexWnd::WND_MAIN);
	const auto iWidthActual = m_pHexPopup->GetActualWidth() + GetSystemMetrics(SM_CXVSCROLL);
	CRect rcHex(0, 0, iWidthActual, iWidthActual); //Square window.
	AdjustWindowRectEx(rcHex, dwStyle, FALSE, dwExStyle);
	const auto iWidth = rcHex.Width();
	const auto iHeight = rcHex.Height() - rcHex.Height() / 3;
	const auto iPosX = GetSystemMetrics(SM_CXSCREEN) / 2 - iWidth / 2;
	const auto iPosY = GetSystemMetrics(SM_CYSCREEN) / 2 - iHeight / 2;
	::SetWindowPos(hWndHex, m_hWnd, iPosX, iPosY, iWidth, iHeight, SWP_NOACTIVATE);

	const auto hIconSmall = static_cast<HICON>(LoadImageW(AfxGetInstanceHandle(), MAKEINTRESOURCEW(IDR_MAINFRAME), IMAGE_ICON, 0, 0, 0));
	const auto hIconBig = static_cast<HICON>(LoadImageW(AfxGetInstanceHandle(), MAKEINTRESOURCEW(IDR_MAINFRAME), IMAGE_ICON, 96, 96, 0));
	if (hIconSmall != nullptr) {
		::SendMessageW(hWndHex, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIconSmall));
		::SendMessageW(hWndHex, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIconBig));
	}
}

bool CSampleDialogDlg::IsFileOpen()const
{
	return m_fFileOpen;
}

void CSampleDialogDlg::FileOpen(std::wstring_view wsvPath, bool fResolveLnk)
{
	FileClose();

	std::wstring wstrPath(wsvPath);
	if (fResolveLnk && wstrPath.ends_with(L".lnk")) {
		wstrPath = LnkToPath(wstrPath.data());
	}

	m_hFile = CreateFileW(wstrPath.data(), GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (m_hFile == INVALID_HANDLE_VALUE) {
		const std::wstring wstr = L"CreateFileW failed.\r\n" + GetLastErrorWstr();
		MessageBoxW(wstr.data(), L"Error", MB_ICONERROR);
		return;
	}

	m_hMapObject = CreateFileMappingW(m_hFile, nullptr, PAGE_READWRITE, 0, 0, nullptr);
	if (!m_hMapObject) {
		CloseHandle(m_hFile);
		const std::wstring wstr = L"CreateFileMappingW failed.\r\n" + GetLastErrorWstr();
		MessageBoxW(wstr.data(), L"Error", MB_ICONERROR);
		return;
	}

	m_lpBase = MapViewOfFile(m_hMapObject, FILE_MAP_WRITE, 0, 0, 0);
	if (!m_lpBase) {
		const std::wstring wstr = L"MapViewOfFileW failed.\r\n" + GetLastErrorWstr();
		MessageBoxW(wstr.data(), L"Error", MB_ICONERROR);
		CloseHandle(m_hMapObject);
		CloseHandle(m_hFile);
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

void CSampleDialogDlg::FileClose()
{
	if (!IsFileOpen())
		return;

	m_pHexDlg->ClearData();
	if (m_pHexPopup->IsCreated()) {
		m_pHexPopup->ClearData();
	}

	if (m_lpBase)
		UnmapViewOfFile(m_lpBase);
	if (m_hMapObject)
		CloseHandle(m_hMapObject);
	if (m_hFile)
		CloseHandle(m_hFile);

	m_fFileOpen = false;
}

void CSampleDialogDlg::LoadTemplates(const IHexCtrl* pHexCtrl)
{
	wchar_t buff[MAX_PATH];
	GetModuleFileNameW(nullptr, buff, MAX_PATH);
	std::wstring wstrPath = buff;
	wstrPath = wstrPath.substr(0, wstrPath.find_last_of(L'\\'));
	wstrPath += L"\\Templates\\";
	if (const std::filesystem::path pathTemplates { wstrPath }; std::filesystem::exists(pathTemplates)) {
		const auto pTempl = pHexCtrl->GetTemplates();
		for (const auto& entry : std::filesystem::directory_iterator { pathTemplates }) {
			const std::wstring_view wsvFile = entry.path().c_str();
			if (const auto npos = wsvFile.find_last_of(L'.'); npos != std::wstring_view::npos) {
				if (wsvFile.substr(npos + 1) == L"json") { //Check json extension of templates.
					const auto p = HEXCTRL::IHexTemplates::LoadFromFile(wsvFile.data());
					pTempl->AddTemplate(*p);
					//pTempl->LoadTemplate(wsvFile.data());
				}
			}
		}
	}
}

bool CSampleDialogDlg::IsRW()const
{
	return m_chkRW.GetCheck() == BST_CHECKED;
}

bool CSampleDialogDlg::IsLnk() const
{
	return m_chkLnk.GetCheck() == BST_CHECKED;
}

std::wstring CSampleDialogDlg::LnkToPath(LPCWSTR pwszLnk)
{
	CComPtr<IShellLinkW> psl;
	psl.CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER);
	CComPtr<IPersistFile> ppf;
	psl->QueryInterface(IID_PPV_ARGS(&ppf));
	ppf->Load(pwszLnk, STGM_READ);

	std::wstring wstrPath(MAX_PATH, L'\0');
	psl->GetPath(wstrPath.data(), MAX_PATH, nullptr, 0);

	return wstrPath;
}

auto CSampleDialogDlg::OpenFileDlg()->std::vector<std::wstring>
{
	CFileDialog fd(TRUE, nullptr, nullptr,
		OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_DONTADDTORECENT | OFN_ENABLESIZING
		| OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NODEREFERENCELINKS, L"All files (*.*)|*.*||");

	std::vector<std::wstring> vecFiles;
	if (fd.DoModal() == IDOK) {
		const CComPtr<IFileOpenDialog> pIFOD = fd.GetIFileOpenDialog();
		CComPtr<IShellItemArray> pResults;
		pIFOD->GetResults(&pResults);

		DWORD dwCount { };
		pResults->GetCount(&dwCount);
		vecFiles.reserve(dwCount);
		for (auto i { 0U }; i < dwCount; ++i) {
			CComPtr<IShellItem> pItem;
			pResults->GetItemAt(i, &pItem);
			CComHeapPtr<wchar_t> pwstrPath;
			pItem->GetDisplayName(SIGDN_FILESYSPATH, &pwstrPath);
			vecFiles.emplace_back(pwstrPath);
		}
	}

	return vecFiles;
}

auto CSampleDialogDlg::GetLastErrorWstr()->std::wstring {
	wchar_t wbuff[MAX_PATH];
	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
			::GetLastError(), 0, wbuff, MAX_PATH, nullptr);
	return wbuff;
}