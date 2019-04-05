#pragma once
#include "../HexCtrl/HexCtrl.h"

using namespace HEXCTRL;

class CHexSampleDlg : public CDialogEx
{
public:
	explicit CHexSampleDlg(CWnd* pParent = nullptr);

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_HEXSAMPLE_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV support

protected:
	IHexCtrlPtr m_myHex { CreateHexCtrl() };
	HEXDATASTRUCT m_hds;
	UCHAR m_data[0xfff];
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnRW();
	afx_msg void OnBnRO();
};