#include "stdafx.h"
#include "CMFCDialogApp.h"
#include "CMFCDialogDlg.h"
#include "Resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CMFCDialogApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

CMFCDialogApp theApp;

BOOL CMFCDialogApp::InitInstance()
{
	CWinApp::InitInstance();

	// Activate "Windows Native" visual manager for enabling themes in MFC controls
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	auto dlg = new CMFCDialogDlg;
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	if (!cmdInfo.m_strFileName.IsEmpty()) {
		dlg->SetStartupFile(cmdInfo.m_strFileName);
	}

	m_pMainWnd = dlg;
	const auto nResponse = dlg->DoModal();
	if (nResponse == -1) {
		TRACE(traceAppMsg, 0, "Warning: dialog creation failed, so application is terminating unexpectedly.\n");
		TRACE(traceAppMsg, 0, "Warning: if you are using MFC controls on the dialog, you cannot #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS.\n");
	}

#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
	ControlBarCleanUp();
#endif

	delete dlg;

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}