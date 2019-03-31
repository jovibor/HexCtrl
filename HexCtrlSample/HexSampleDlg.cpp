#include "stdafx.h"
#include "HexSample.h"
#include "HexSampleDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CHexSampleDlg::CHexSampleDlg(CWnd* pParent /*=nullptr*/) //-V730
	: CDialogEx(IDD_HEXSAMPLE_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIconW(IDR_MAINFRAME);
}

void CHexSampleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MY_HEX, *m_myHex);
}

BEGIN_MESSAGE_MAP(CHexSampleDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CHexSampleDlg::OnBnRW)
	ON_BN_CLICKED(IDOK2, &CHexSampleDlg::OnBnRO)
END_MESSAGE_MAP()

BOOL CHexSampleDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);	 //Set big icon
	SetIcon(m_hIcon, FALSE); //Set small icon

/*	HEXCREATESTRUCT hcs;
	hcs.dwExStyle = WS_EX_APPWINDOW;
	hcs.pwndParent = this;
	hcs.fFloat = true;
	m_myHex->Create(hcs);*/
	m_myHex->CreateDialogCtrl();

	m_hds.pData = m_data;
	m_hds.ullDataSize = sizeof(m_data);

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

void CHexSampleDlg::OnBnRO()
{
	if (!m_myHex->IsDataSet())
	{
		m_hds.fMutable = false;
		m_myHex->SetData(m_hds);
	}
	m_myHex->EditEnable(false);
}

void CHexSampleDlg::OnBnRW()
{
	if (!m_myHex->IsDataSet())
	{
		m_hds.fMutable = true;
		m_myHex->SetData(m_hds);
	}
	m_myHex->EditEnable(true);
}