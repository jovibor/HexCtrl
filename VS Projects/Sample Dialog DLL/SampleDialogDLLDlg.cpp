#include "stdafx.h"
#include "SampleDialogDLLDlg.h"
#include "Resource.h"
#include <filesystem>
#include <random>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

constexpr const auto WstrTextRO { L"Random data: RO" };
constexpr const auto WstrTextRW { L"Random data: RW" };

BEGIN_MESSAGE_MAP(CSampleDialogDLLDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_SETDATARND, &CSampleDialogDLLDlg::OnBnSetRndData)
	ON_BN_CLICKED(IDC_CLEARDATA, &CSampleDialogDLLDlg::OnBnClearData)
END_MESSAGE_MAP()

CSampleDialogDLLDlg::CSampleDialogDLLDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_HEXCTRL_SAMPLEDLL, pParent) {
	m_hIcon = AfxGetApp()->LoadIconW(IDR_MAINFRAME);
}

void CSampleDialogDLLDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BOOL CSampleDialogDLLDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_pHexDlg->CreateDialogCtrl(IDC_MY_HEX, m_hWnd);
	LoadTemplates(&*m_pHexDlg);

	if (const auto pDL = GetDynamicLayout(); pDL != nullptr) {
		pDL->SetMinSize({ 0, 0 });
	}

	return TRUE;
}

void CSampleDialogDLLDlg::OnPaint()
{
	if (IsIconic()) {
		CPaintDC dc(this); // device context for painting

		SendMessageW(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;
		dc.DrawIcon(x, y, m_hIcon);
	}
	else {
		CDialogEx::OnPaint();
	}
}

HCURSOR CSampleDialogDLLDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

bool CSampleDialogDLLDlg::IsRW()const
{
	return true;
}

void CSampleDialogDLLDlg::LoadTemplates(const IHexCtrl* pHexCtrl)
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
					//Using exported LoadTemplateFromFile function here.
					//To test its work within the DLL.
					const auto p = HEXCTRL::IHexTemplates::LoadFromFile(wsvFile.data());
					pTempl->AddTemplate(*p);
				}
			}
		}
	}
}

void CSampleDialogDLLDlg::OnBnSetRndData()
{
	if (m_pHexDlg->IsDataSet()) {
		m_pHexDlg->SetMutable(true);
		SetWindowTextW(IsRW() ? WstrTextRW : WstrTextRO);

		return;
	}

	m_hds.spnData = { m_RandomData, sizeof(m_RandomData) };
	m_hds.fMutable = IsRW();
	m_pHexDlg->SetData(m_hds);
	SetWindowTextW(IsRW() ? WstrTextRW : WstrTextRO);
}

void CSampleDialogDLLDlg::OnBnClearData()
{
	m_pHexDlg->ClearData();
	m_hds.spnData = { };
	SetWindowTextW(L"HexCtrl Sample Dialog DLL");
}