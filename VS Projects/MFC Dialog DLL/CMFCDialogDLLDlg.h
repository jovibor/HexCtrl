#pragma once
#include "../../HexCtrl/HexCtrl.h"
#include <afxcontrolbars.h>

using namespace HEXCTRL;

class CMFCDialogDLLDlg : public CDialogEx {
public:
	CMFCDialogDLLDlg(CWnd* pParent = nullptr);
private:
	void DoDataExchange(CDataExchange* pDX)override;
	BOOL OnInitDialog()override;
private:
	[[nodiscard]] bool IsRW()const;
	void LoadTemplates(const IHexCtrl* pHexCtrl);
	afx_msg void OnBnClearData();
	afx_msg void OnBnSetRndData();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
private:
	HICON m_hIcon;
	IHexCtrlPtr m_pHexDlg { CreateHexCtrl(AfxGetInstanceHandle()) };
	HEXDATA m_hds;
	std::byte m_RandomData[16 * 1024];
};