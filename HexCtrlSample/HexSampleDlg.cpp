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
END_MESSAGE_MAP()

CHexSampleDlg::CHexSampleDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_HEXSAMPLE_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIconW(IDR_MAINFRAME);
}

PHEXCOLOR CHexSampleDlg::GetColor(ULONGLONG ullOffset)
{
	//Sample code for custom colors:
	if (ullOffset < 18)
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
		return &vec[ullOffset];
	}

	return nullptr;
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

	m_myHex->CreateDialogCtrl(IDC_MY_HEX, m_hWnd);
	m_myHex->SetWheelRatio(0.5);
	//m_myHex->SetSectorSize(64);

	//Classical approach:
	//HEXCREATESTRUCT hcs;
	//hcs.hwndParent = m_hWnd;
	//hcs.uID = IDC_MY_HEX;
	//hcs.enCreateMode = EHexCreateMode::CREATE_CUSTOMCTRL;
	//hcs.enShowMode = EHexShowMode::ASDWORD;
	//m_myHex->Create(hcs);

	//m_hds.pHexVirtColors = this;
	//m_hds.fHighLatency = true;

	return TRUE; //return TRUE  unless you set the focus to a control
}

void CHexSampleDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);

		SendMessageW(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

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

void CHexSampleDlg::OnBnSetDataRO()
{
	if (IsFileOpen())
		FileClose();
	else if (m_myHex->IsDataSet())
	{
		m_myHex->SetMutable(false);
		return;
	}

	m_hds.pData = reinterpret_cast<std::byte*>(m_RandomData);
	m_hds.ullDataSize = sizeof(m_RandomData);
	m_hds.fMutable = false;
	m_myHex->SetData(m_hds);
}

void CHexSampleDlg::OnBnSetDataRW()
{
	if (IsFileOpen())
		FileClose();
	else if (m_myHex->IsDataSet())
	{
		m_myHex->SetMutable(true);
		return;
	}

	m_hds.pData = reinterpret_cast<std::byte*>(m_RandomData);
	m_hds.ullDataSize = sizeof(m_RandomData);
	m_hds.fMutable = true;
	m_myHex->SetData(m_hds);
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

void CHexSampleDlg::OnBnClearData()
{
	FileClose();
	m_myHex->ClearData();
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

	m_hds.pData = static_cast<std::byte*>(m_lpBase);
	m_hds.ullDataSize = static_cast<ULONGLONG>(stFileSize.QuadPart);
	m_hds.fMutable = fRW;
	m_myHex->SetData(m_hds);
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