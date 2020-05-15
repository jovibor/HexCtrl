#pragma once
#include "../HexCtrl/HexCtrl.h"

using namespace HEXCTRL;

class CHexSampleDlg : public CDialogEx, public IHexVirtColors
{
public:
	explicit CHexSampleDlg(CWnd* pParent = nullptr);
private:
	PHEXCOLOR GetColor(ULONGLONG ullOffset)override;
	BOOL OnInitDialog()override;
	void DoDataExchange(CDataExchange* pDX) override;
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnSetDataRO();
	afx_msg void OnBnSetDataRW();
	afx_msg void OnBnFileOpenRO();
	afx_msg void OnBnFileOpenRW();
	afx_msg void OnBnClearData();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnClose();
	DECLARE_MESSAGE_MAP()
	[[nodiscard]] auto OpenFileDlg()const->std::optional<std::vector<std::wstring>>;
	[[nodiscard]] bool IsFileOpen()const;
	void FileOpen(std::wstring_view wstrPath, bool fRW);
	void FileClose();
private:
	IHexCtrlPtr m_myHex { CreateHexCtrl() };
	HEXDATASTRUCT m_hds;
	BYTE m_RandomData[1024 * 1024];
	HICON m_hIcon;
	bool m_fFileOpen { false };
	HANDLE m_hFile { };
	HANDLE m_hMapObject { };
	LPVOID m_lpBase { };
};