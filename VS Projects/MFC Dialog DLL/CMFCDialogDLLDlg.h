#pragma once
#include <afxcontrolbars.h>
#include "../../HexCtrl/HexCtrl.h"

using namespace HEXCTRL;

class CMFCDialogDLLDlg : public CDialogEx {
public:
	CMFCDialogDLLDlg(CWnd* pParent = nullptr);
private:
	void DoDataExchange(CDataExchange* pDX)override;
	[[nodiscard]] bool IsRW()const;
	void LoadTemplates(IHexCtrl* pHexCtrl);
	BOOL OnInitDialog()override;
	afx_msg void OnBnClearData();
	afx_msg void OnBnSetRndData();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	BOOL PreTranslateMessage(MSG* pMsg)override;
	DECLARE_MESSAGE_MAP()
private:
	HICON m_hIcon;
	IHexCtrlPtr m_pHexDlg { CreateHexCtrl() };
	HEXDATA m_hds;
	std::byte m_RandomData[16 * 1024];
};