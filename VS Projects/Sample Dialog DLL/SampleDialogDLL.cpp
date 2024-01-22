#include "stdafx.h"
#include "SampleDialogDLL.h"
#include "SampleDialogDLLDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CSampleDialogDLLApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

CSampleDialogDLLApp theApp;

BOOL CSampleDialogDLLApp::InitInstance()
{
	CWinApp::InitInstance();

	auto dlg = new CSampleDialogDLLDlg;
	m_pMainWnd = dlg;
	INT_PTR nResponse = dlg->DoModal();
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

BOOL CSampleDialogDLLApp::PreTranslateMessage(MSG* pMsg)
{
	if (HEXCTRL::HexCtrlPreTranslateMessage(pMsg))
		return TRUE;

	return CWinApp::PreTranslateMessage(pMsg);
}