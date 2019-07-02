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
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	IHexCtrlPtr m_myHex { CreateHexCtrl() };
	HEXDATASTRUCT m_hds;
	UCHAR m_data[0xffff];
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnSetDataRO();
	afx_msg void OnBnSetDataRW();
	afx_msg void OnBnClearData();
	DECLARE_MESSAGE_MAP()
};