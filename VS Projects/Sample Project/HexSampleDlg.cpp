#include "stdafx.h"
#include "HexSampleDlg.h"
#include "Resource.h"
#include <filesystem>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CHexSampleDlg, CDialogEx)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_SETDATARND, &CHexSampleDlg::OnBnSetRndData)
	ON_BN_CLICKED(IDC_CLEARDATA, &CHexSampleDlg::OnBnClearData)
	ON_BN_CLICKED(IDC_FILEOPEN, &CHexSampleDlg::OnBnFileOpen)
	ON_BN_CLICKED(IDC_HEXPOPUP, &CHexSampleDlg::OnBnPopup)
	ON_BN_CLICKED(IDC_CHK_RW, &CHexSampleDlg::OnChkRW)
	ON_WM_DROPFILES()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
END_MESSAGE_MAP()

constexpr const auto WstrTextRO { L"Random data: RO" };
constexpr const auto WstrTextRW { L"Random data: RW" };

CHexSampleDlg::CHexSampleDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_HEXSAMPLE_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIconW(IDR_MAINFRAME);
}

void CHexSampleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHK_RW, m_chkRW);
	DDX_Control(pDX, IDC_CHK_LNK, m_chkLnk);
}

BOOL CHexSampleDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

	SetIcon(m_hIcon, TRUE);	 //Set big icon
	SetIcon(m_hIcon, FALSE); //Set small icon

	//For Drag'n Drop to work even in elevated mode.
	//https://helgeklein.com/blog/2010/03/how-to-enable-drag-and-drop-for-an-elevated-mfc-application-on-vistawindows-7/
	ChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);
	ChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);
	ChangeWindowMessageFilter(0x0049, MSGFLT_ADD);
	DragAcceptFiles(TRUE);

	m_pHexDlg->CreateDialogCtrl(IDC_MY_HEX, m_hWnd);
	m_pHexDlg->SetWheelRatio(2, true); //Two lines scroll with mouse-wheel.
	m_pHexDlg->SetPageSize(64);

	LoadTemplates(&*m_pHexDlg);
	//OnBnSetRndData();
	//m_pHexDlg->ExecuteCmd(EHexCmd::CMD_BKM_DLG_MGR);

	//m_hds.pHexVirtColors = this;
	//m_hds.fHighLatency = true;

	m_chkLnk.SetCheck(BST_CHECKED);

	if (!m_wstrStartupFile.empty()) {
		FileOpen(m_wstrStartupFile, IsLnk());
	}

	if (const auto pDL = GetDynamicLayout(); pDL != nullptr) {
		pDL->SetMinSize({ 0, 0 });
	}

	return TRUE;
}

void CHexSampleDlg::OnPaint()
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

HCURSOR CHexSampleDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CHexSampleDlg::OnBnClearData()
{
	FileClose();
	m_pHexDlg->ClearData();
	if (m_pHexPopup->IsCreated()) {
		m_pHexPopup->ClearData();
		::SetWindowTextW(m_pHexPopup->GetWindowHandle(EHexWnd::WND_MAIN), L"");
	}

	m_hds.spnData = { };
	SetWindowTextW(L"HexCtrl Sample Dialog");
}

void CHexSampleDlg::OnBnSetRndData()
{
	if (IsFileOpen()) {
		FileClose();
	}
	else if (m_pHexDlg->IsDataSet()) {
		m_pHexDlg->SetMutable(IsRW());
		SetWindowTextW(IsRW() ? WstrTextRW : WstrTextRO);

		if (m_pHexPopup->IsCreated() && m_pHexPopup->IsDataSet()) {
			m_pHexPopup->SetMutable(IsRW());
			::SetWindowTextW(m_pHexPopup->GetWindowHandle(EHexWnd::WND_MAIN), IsRW() ? WstrTextRW : WstrTextRO);
		}

		return;
	}

	m_hds.spnData = { reinterpret_cast<std::byte*>(m_RandomData), sizeof(m_RandomData) };
	m_hds.fMutable = IsRW();
	m_pHexDlg->SetData(m_hds);
	SetWindowTextW(IsRW() ? WstrTextRW : WstrTextRO);

	if (m_pHexPopup->IsCreated()) {
		m_pHexPopup->SetData(m_hds);
		::SetWindowTextW(m_pHexPopup->GetWindowHandle(EHexWnd::WND_MAIN), IsRW() ? WstrTextRW : WstrTextRO);
	}
}

void CHexSampleDlg::OnBnFileOpen()
{
	if (auto vecFiles = OpenFileDlg(); !vecFiles.empty()) {
		FileOpen(vecFiles.front(), IsLnk());
	}
}

void CHexSampleDlg::OnBnPopup()
{
	if (!m_pHexPopup->IsCreated()) {
		CreateHexPopup();
		LoadTemplates(&*m_pHexPopup);
		::ShowWindow(m_pHexPopup->GetWindowHandle(EHexWnd::WND_MAIN), SW_SHOWNORMAL);
	}
	else {
		const auto hWnd = m_pHexPopup->GetWindowHandle(EHexWnd::WND_MAIN);
		::ShowWindow(hWnd, ::IsWindowVisible(hWnd) ? SW_HIDE : SW_SHOWNORMAL);
	}
}

void CHexSampleDlg::OnChkRW()
{
	if (m_pHexDlg->IsDataSet()) {
		m_pHexDlg->SetMutable(IsRW());
		if (!IsFileOpen()) {
			SetWindowTextW(IsRW() ? WstrTextRW : WstrTextRO);
		}

		if (m_pHexPopup->IsCreated() && m_pHexPopup->IsDataSet()) {
			m_pHexPopup->SetMutable(IsRW());
			if (!IsFileOpen()) {
				::SetWindowTextW(m_pHexPopup->GetWindowHandle(EHexWnd::WND_MAIN), IsRW() ? WstrTextRW : WstrTextRO);
			}
		}
	}
}

void CHexSampleDlg::OnClose()
{
	FileClose();
	CDialogEx::OnClose();
}

void CHexSampleDlg::OnDropFiles(HDROP hDropInfo)
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

BOOL CHexSampleDlg::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	const auto pNMHDR = reinterpret_cast<PHEXMENUINFO>(lParam);
	if (pNMHDR->hdr.idFrom == IDC_MY_HEX && pNMHDR->hdr.code == HEXCTRL_MSG_CONTEXTMENU) {
		// pNMHDR->fShow = false; //Ability to disable HexCtrl context menu.
	}

	return CDialogEx::OnNotify(wParam, lParam, pResult);
}

bool CHexSampleDlg::OnHexGetColor(HEXCOLORINFO& hci)
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

void CHexSampleDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
}

void CHexSampleDlg::SetStartupFile(LPCWSTR pwszFile)
{
	m_wstrStartupFile = pwszFile;
}

void CHexSampleDlg::CreateHexPopup()
{
	if (m_pHexPopup->IsCreated())
		return;

	const auto dwStyle = WS_POPUP | WS_OVERLAPPEDWINDOW;
	const auto dwExStyle = WS_EX_APPWINDOW; //To force to the taskbar.

	const HEXCREATE hcs { .hWndParent { m_hWnd }, .dwStyle { dwStyle }, .dwExStyle { dwExStyle } };
	m_pHexPopup->Create(hcs);
	if (!m_hds.spnData.empty()) {
		m_pHexPopup->SetData(m_hds);
	}

	const auto hWndHex = m_pHexPopup->GetWindowHandle(EHexWnd::WND_MAIN);
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

bool CHexSampleDlg::IsFileOpen()const
{
	return m_fFileOpen;
}

void CHexSampleDlg::FileOpen(std::wstring_view wsvPath, bool fResolveLnk)
{
	FileClose();

	std::wstring wstrPath(wsvPath);
	if (fResolveLnk && wstrPath.ends_with(L".lnk")) {
		wstrPath = LnkToPath(wstrPath.data());
	}

	m_hFile = CreateFileW(wstrPath.data(), GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (m_hFile == INVALID_HANDLE_VALUE) {
		MessageBoxW(L"CreateFileW failed.\r\nFile might be already opened by another process.", L"Error", MB_ICONERROR);
		return;
	}

	m_hMapObject = CreateFileMappingW(m_hFile, nullptr, PAGE_READWRITE, 0, 0, nullptr);
	if (!m_hMapObject) {
		CloseHandle(m_hFile);
		MessageBoxW(L"CreateFileMappingW failed.", L"Error", MB_ICONERROR);
		return;
	}

	m_lpBase = MapViewOfFile(m_hMapObject, FILE_MAP_WRITE, 0, 0, 0);
	if (!m_lpBase) {
		MessageBoxW(L"MapViewOfFileW failed.\r\n File might be too big to fit in memory.", L"Error", MB_ICONERROR);
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
		::SetWindowTextW(m_pHexPopup->GetWindowHandle(EHexWnd::WND_MAIN), wstrPath.data());
	}

	SetWindowTextW(wstrPath.data());
}

void CHexSampleDlg::FileClose()
{
	if (!IsFileOpen())
		return;

	if (m_lpBase)
		UnmapViewOfFile(m_lpBase);
	if (m_hMapObject)
		CloseHandle(m_hMapObject);
	if (m_hFile)
		CloseHandle(m_hFile);

	m_fFileOpen = false;
}

void CHexSampleDlg::LoadTemplates(const IHexCtrl* pHexCtrl)
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
					pTempl->LoadTemplate(wsvFile.data());
				}
			}
		}
	}
}

bool CHexSampleDlg::IsRW()const
{
	return m_chkRW.GetCheck() == BST_CHECKED;
}

bool CHexSampleDlg::IsLnk() const
{
	return m_chkLnk.GetCheck() == BST_CHECKED;
}

std::wstring CHexSampleDlg::LnkToPath(LPCWSTR pwszLnk)
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

auto CHexSampleDlg::OpenFileDlg()->std::vector<std::wstring>
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