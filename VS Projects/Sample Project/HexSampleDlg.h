#pragma once
#include "../../HexCtrl/HexCtrl.h"
#include <afxcontrolbars.h>

using namespace HEXCTRL;

class CHexSampleDlg : public CDialogEx, public IHexVirtColors {
public:
	explicit CHexSampleDlg(CWnd* pParent = nullptr);
	void SetStartupFile(LPCWSTR pwszFile);
	static std::wstring LnkToPath(LPCWSTR pwszLnk);
private:
	void CreateHexPopup();
	bool OnHexGetColor(HEXCOLORINFO& hci)override;
	BOOL OnInitDialog()override;
	void DoDataExchange(CDataExchange* pDX)override;
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnSetRndData();
	afx_msg void OnBnFileOpen();
	afx_msg void OnBnClearData();
	afx_msg void OnClose();
	afx_msg void OnDropFiles(HDROP hDropInfo);
	BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)override;
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnBnPopup();
	afx_msg void OnChkRW();
	[[nodiscard]] bool IsFileOpen()const;
	void FileOpen(std::wstring_view wsvPath, bool fResolveLnk = true);
	void FileClose();
	void LoadTemplates(const IHexCtrl* pHexCtrl);
	bool IsRW()const;
	bool IsLnk()const;
	[[nodiscard]] static auto OpenFileDlg() -> std::vector<std::wstring>;
	DECLARE_MESSAGE_MAP();
private:
	IHexCtrlPtr m_pHexDlg { CreateHexCtrl() };
	IHexCtrlPtr m_pHexPopup { CreateHexCtrl() };
	HEXDATA m_hds;
	BYTE m_RandomData[16 * 1024];
	HICON m_hIcon;
	bool m_fFileOpen { false };
	HANDLE m_hFile { };
	HANDLE m_hMapObject { };
	LPVOID m_lpBase { };
	std::wstring m_wstrStartupFile;
	CButton m_chkRW;
	CButton m_chkLnk;
};