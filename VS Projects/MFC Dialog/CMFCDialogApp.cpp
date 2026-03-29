#include "stdafx.h"
#include "CMFCDialogApp.h"
#include "CMFCDialogDlg.h"
#include "Resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CMFCDialogApp theApp;

BOOL CMFCDialogApp::InitInstance()
{
	CWinApp::InitInstance();

	// Activate "Windows Native" visual manager for enabling themes in MFC controls
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	const auto pDlg = new CMFCDialogDlg;
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	if (!cmdInfo.m_strFileName.IsEmpty()) {
		pDlg->SetStartupFile(cmdInfo.m_strFileName);
	}

	m_pMainWnd = pDlg;
	pDlg->DoModal();
	delete pDlg;

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}