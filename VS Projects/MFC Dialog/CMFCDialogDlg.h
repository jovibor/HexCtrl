#pragma once
#include <afxcontrolbars.h>
#include "../../HexCtrl/HexCtrl.h"
#include <memory>

using namespace HEXCTRL;

class CMFCDialogDlg : public CDialogEx, public IHexVirtColors {
public:
	explicit CMFCDialogDlg(CWnd* pParent = nullptr);
	void SetStartupFile(LPCWSTR pwszFile);
	[[nodiscard]] static auto LnkToPath(LPCWSTR pwszLnk) -> std::wstring;
private:
	void CreateHexPopup();
	void CreateIconsForHexCtrl();
	void DoDataExchange(CDataExchange* pDX)override;
	void FileOpen(std::wstring_view wsvPath, bool fResolveLnk = true);
	void FileClose();
	bool IsRW()const;
	bool IsLnk()const;
	afx_msg void OnBnSetRndData();
	afx_msg void OnBnFileOpen();
	afx_msg void OnBnClearData();
	afx_msg void OnBnPopup();
	afx_msg void OnChkRW();
	afx_msg void OnClose();
	auto OnDPIChanged(WPARAM wParam, LPARAM lParam) -> LRESULT;
	afx_msg void OnDropFiles(HDROP hDropInfo);
	auto OnGetDPIScaledSize(WPARAM wParam, LPARAM lParam) -> LRESULT;
	bool OnHexGetColor(HEXCOLORINFO& hci)override;
	BOOL OnInitDialog()override;
	BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)override;
	[[nodiscard]] bool IsFileOpen()const;
	BOOL PreTranslateMessage(MSG* pMsg)override;
	void SetIconsForHexCtrl(IHexCtrl& HexCtrl); //Creates and sets icons for the HexCtrl's menu.
	[[nodiscard]] static auto GetLastErrorWstr() -> std::wstring;
	static void LoadTemplates(IHexCtrl* pHexCtrl);
	[[nodiscard]] static auto OpenFileDlg() -> std::vector<std::wstring>;
	DECLARE_MESSAGE_MAP();
private:
	struct HEXMENUICON {
		EHexMenuItem eItem;
		HBITMAP hBitmap { };
		HEXMENUICON(EHexMenuItem eItem, HBITMAP hBitmap) : eItem(eItem), hBitmap(hBitmap) { }
		HEXMENUICON(HEXMENUICON&& other) {
			eItem = other.eItem;
			hBitmap = other.hBitmap;
			other.hBitmap = nullptr;
		}
		~HEXMENUICON() { if (hBitmap) { ::DeleteObject(hBitmap); } }
	};
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
	std::vector<HEXMENUICON> m_vecHexIcons; //Icons for the HexCtrl's menu.
};