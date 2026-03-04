#pragma once
#include <afxcontrolbars.h>
#include "../../HexCtrl/HexCtrl.h"
#include <memory>

using namespace HEXCTRL;

class CMFCDialogDlg : public CDialogEx, public IHexVirtColors {
public:
	explicit CMFCDialogDlg(CWnd* pParent = nullptr);
	void SetStartupFile(LPCWSTR pwszFile);
	static std::wstring LnkToPath(LPCWSTR pwszLnk);
private:
	void CreateHexPopup();
	bool OnHexGetColor(HEXCOLORINFO& hci)override;
	BOOL OnInitDialog()override;
	void DoDataExchange(CDataExchange* pDX)override;
	afx_msg void OnBnSetRndData();
	afx_msg void OnBnFileOpen();
	afx_msg void OnBnClearData();
	afx_msg void OnClose();
	auto OnDPIChanged(WPARAM wParam, LPARAM lParam) -> LRESULT;
	afx_msg void OnDropFiles(HDROP hDropInfo);
	auto OnGetDPIScaledSize(WPARAM wParam, LPARAM lParam) -> LRESULT;
	BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)override;
	afx_msg void OnBnPopup();
	afx_msg void OnChkRW();
	[[nodiscard]] bool IsFileOpen()const;
	void FileOpen(std::wstring_view wsvPath, bool fResolveLnk = true);
	void FileClose();
	void LoadTemplates(IHexCtrl* pHexCtrl);
	bool IsRW()const;
	bool IsLnk()const;
	BOOL PreTranslateMessage(MSG* pMsg)override;
	[[nodiscard]] static auto OpenFileDlg() -> std::vector<std::wstring>;
	[[nodiscard]] static auto GetLastErrorWstr() -> std::wstring;
	DECLARE_MESSAGE_MAP();
private:
	IHexCtrlPtr m_pHexDlg { CreateHexCtrl() };
	IHexCtrlPtr m_pHexPopup { CreateHexCtrl() };
	HEXDATA m_hds;
	std::unique_ptr<std::byte[]> m_pData;
	bool m_fFileOpen { false };
	HANDLE m_hFile { };
	HANDLE m_hMapObject { };
	LPVOID m_lpBase { };
	std::wstring m_wstrStartupFile;
	CButton m_chkRW;
	CButton m_chkLnk;
	CEdit m_editDataSize;
};