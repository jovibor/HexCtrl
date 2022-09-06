#pragma once
#include "../../HexCtrl/HexCtrl.h"

using namespace HEXCTRL;

class CHexSampleDlg : public CDialogEx, public IHexVirtColors
{
public:
	explicit CHexSampleDlg(CWnd* pParent = nullptr);
private:
	void CreateHexPopup();
	void OnHexGetColor(HEXCOLORINFO& hci)override;
	BOOL OnInitDialog()override;
	void DoDataExchange(CDataExchange* pDX)override;
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnSetRndDataRO();
	afx_msg void OnBnSetRndDataRW();
	afx_msg void OnBnFileOpenRO();
	afx_msg void OnBnFileOpenRW();
	afx_msg void OnBnClearData();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnClose();
	afx_msg void OnBnPopup();
	[[nodiscard]] bool IsFileOpen()const;
	void FileOpen(std::wstring_view wstrPath, bool fRW);
	void FileClose();
	[[nodiscard]] static auto OpenFileDlg()->std::optional<std::vector<std::wstring>>;
	DECLARE_MESSAGE_MAP()
private:
	IHexCtrlPtr m_pHexChild { CreateHexCtrl() };
	IHexCtrlPtr m_pHexPopup { CreateHexCtrl() };
	HEXDATA m_hds;
	BYTE m_RandomData[16 * 1024];
	HICON m_hIcon;
	bool m_fFileOpen { false };
	HANDLE m_hFile { };
	HANDLE m_hMapObject { };
	LPVOID m_lpBase { };
};