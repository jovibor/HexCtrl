#include "stdafx.h"
#include "HexSample.h"
#include "HexSampleDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CHexSampleDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_SETDATARO, &CHexSampleDlg::OnBnSetDataRO)
	ON_BN_CLICKED(IDC_SETDATARW, &CHexSampleDlg::OnBnSetDataRW)
	ON_BN_CLICKED(IDC_CLEARDATA, &CHexSampleDlg::OnBnClearData)
	ON_BN_CLICKED(IDC_FILEOPENRO, &CHexSampleDlg::OnBnFileOpenRO)
	ON_BN_CLICKED(IDC_FILEOPENRW, &CHexSampleDlg::OnBnFileOpenRW)
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_HEXPOPUP, &CHexSampleDlg::OnBnPopup)
END_MESSAGE_MAP()

CHexSampleDlg::CHexSampleDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_HEXSAMPLE_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIconW(IDR_MAINFRAME);
}

void CHexSampleDlg::CreateHexPopup()
{
	if (m_pHexPopup->IsCreated())
		return;

	const auto dwStyle = WS_POPUP | WS_OVERLAPPEDWINDOW;
	const auto dwExStyle = WS_EX_APPWINDOW; //To force to the taskbar.

	HEXCREATE hcs { .hWndParent { m_hWnd }, .dwStyle { dwStyle }, .dwExStyle { dwExStyle } };
	m_pHexPopup->Create(hcs);
	if (!m_hds.spnData.empty())
		m_pHexPopup->SetData(m_hds);

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

void CHexSampleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BOOL CHexSampleDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);	 //Set big icon
	SetIcon(m_hIcon, FALSE); //Set small icon

	m_pHexChild->CreateDialogCtrl(IDC_MY_HEX, m_hWnd);
	//m_pHexChild->SetWheelRatio(0.5);
	m_pHexChild->SetPageSize(64);

	//m_hds.pHexVirtColors = this;
	//m_hds.fHighLatency = true;

	return TRUE;
}

void CHexSampleDlg::OnPaint()
{
	if (IsIconic())
	{
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
	else
	{
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
	m_pHexChild->ClearData();
	if (m_pHexPopup->IsCreated())
		m_pHexPopup->ClearData();

	m_hds.spnData = { };
}

void CHexSampleDlg::OnBnFileOpenRO()
{
	if (auto optFiles = OpenFileDlg(); optFiles)
		FileOpen(optFiles->front(), false);
}

void CHexSampleDlg::OnBnFileOpenRW()
{
	if (auto optFiles = OpenFileDlg(); optFiles)
		FileOpen(optFiles->front(), true);
}

void CHexSampleDlg::OnBnPopup()
{
	if (!m_pHexPopup->IsCreated()) {
		CreateHexPopup();
		::ShowWindow(m_pHexPopup->GetWindowHandle(EHexWnd::WND_MAIN), SW_SHOWNORMAL);
	}
	else {
		const auto hWnd = m_pHexPopup->GetWindowHandle(EHexWnd::WND_MAIN);
		::ShowWindow(hWnd, ::IsWindowVisible(hWnd) ? SW_HIDE : SW_SHOWNORMAL);
	}
}

void CHexSampleDlg::OnBnSetDataRO()
{
	if (IsFileOpen())
		FileClose();
	else if (m_pHexChild->IsDataSet())
	{
		m_pHexChild->SetMutable(false);
		if (m_pHexPopup->IsCreated() && m_pHexPopup->IsDataSet())
			m_pHexPopup->SetMutable(false);
		return;
	}

	m_hds.spnData = { reinterpret_cast<std::byte*>(m_RandomData), sizeof(m_RandomData) };
	m_hds.fMutable = false;
	m_pHexChild->SetData(m_hds);
	if (m_pHexPopup->IsCreated())
		m_pHexPopup->SetData(m_hds);
}

void CHexSampleDlg::OnBnSetDataRW()
{
	if (IsFileOpen())
		FileClose();
	else if (m_pHexChild->IsDataSet())
	{
		m_pHexChild->SetMutable(true);
		if (m_pHexPopup->IsCreated() && m_pHexPopup->IsDataSet())
			m_pHexPopup->SetMutable(true);
		return;
	}

	m_hds.spnData = { reinterpret_cast<std::byte*>(m_RandomData), sizeof(m_RandomData) };
	m_hds.fMutable = true;
	m_pHexChild->SetData(m_hds);
	if (m_pHexPopup->IsCreated())
		m_pHexPopup->SetData(m_hds);
}

void CHexSampleDlg::OnHexGetColor(HEXCOLORINFO& hci)
{
	//Sample code for custom colors:
	if (hci.ullOffset < 18)
	{
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

		hci.pClr = &vec[static_cast<size_t>(hci.ullOffset)];
	}
}

void CHexSampleDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
}

void CHexSampleDlg::OnClose()
{
	FileClose();
	CDialogEx::OnClose();
}

auto CHexSampleDlg::OpenFileDlg()const->std::optional<std::vector<std::wstring>>
{
	CFileDialog fd(TRUE, nullptr, nullptr,
		OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_DONTADDTORECENT | OFN_ENABLESIZING
		| OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, L"All files (*.*)|*.*||");

	std::vector<std::wstring> vecFiles { };
	if (fd.DoModal() == IDOK)
	{
		CComPtr<IFileOpenDialog> pIFOD = fd.GetIFileOpenDialog();
		CComPtr<IShellItemArray> pResults;
		pIFOD->GetResults(&pResults);

		DWORD dwCount { };
		pResults->GetCount(&dwCount);
		for (unsigned i = 0; i < dwCount; i++)
		{
			CComPtr<IShellItem> pItem;
			pResults->GetItemAt(i, &pItem);
			CComHeapPtr<wchar_t> pwstrPath;
			pItem->GetDisplayName(SIGDN_FILESYSPATH, &pwstrPath);
			vecFiles.emplace_back(pwstrPath);
		}
	}

	std::optional<std::vector<std::wstring>> optRet { };
	if (!vecFiles.empty())
		optRet = std::move(vecFiles);

	return optRet;
}

bool CHexSampleDlg::IsFileOpen()const
{
	return m_fFileOpen;
}

void CHexSampleDlg::FileOpen(std::wstring_view wstrPath, bool fRW)
{
	FileClose();

	m_hFile = CreateFileW(wstrPath.data(), fRW ? GENERIC_READ | GENERIC_WRITE : GENERIC_READ,
		FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (m_hFile == INVALID_HANDLE_VALUE)
	{
		MessageBoxW(L"CreateFile call failed.\r\nFile might be already opened by another process.", L"Error", MB_ICONERROR);
		return;
	}

	m_hMapObject = CreateFileMappingW(m_hFile, nullptr, fRW ? PAGE_READWRITE : PAGE_READONLY, 0, 0, nullptr);
	if (!m_hMapObject)
	{
		CloseHandle(m_hFile);
		MessageBoxW(L"CreateFileMapping call failed.", L"Error", MB_ICONERROR);
		return;
	}

	m_lpBase = MapViewOfFile(m_hMapObject, fRW ? FILE_MAP_WRITE : FILE_MAP_READ, 0, 0, 0);
	if (!m_lpBase)
	{
		MessageBoxW(L"MapViewOfFile failed.\r\n File might be too big to fit in memory.", L"Error", MB_ICONERROR);
		CloseHandle(m_hMapObject);
		CloseHandle(m_hFile);
		return;
	}
	m_fFileOpen = true;

	LARGE_INTEGER stFileSize;
	::GetFileSizeEx(m_hFile, &stFileSize);

	m_hds.spnData = { static_cast<std::byte*>(m_lpBase), static_cast<std::size_t>(stFileSize.QuadPart) };
	m_hds.fMutable = fRW;
	m_pHexChild->SetData(m_hds);
	if (m_pHexPopup->IsCreated())
		m_pHexPopup->SetData(m_hds);
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