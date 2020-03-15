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
	void DoDataExchange(CDataExchange* pDX) override;

protected:
	IHexCtrlPtr m_myHex { CreateHexCtrl() };
	HEXDATASTRUCT m_hds;
//	BYTE m_data[16];
	BYTE m_data[1024 * 1024 * 8];
	HICON m_hIcon;

	BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnSetDataRO();
	afx_msg void OnBnSetDataRW();
	afx_msg void OnBnClearData();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	DECLARE_MESSAGE_MAP()
};