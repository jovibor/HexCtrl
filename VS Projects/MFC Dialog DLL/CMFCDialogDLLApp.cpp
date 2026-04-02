#include "stdafx.h"
#include "CMFCDialogDLLApp.h"
#include "CMFCDialogDLLDlg.h"

#ifdef _M_IX86
#ifdef _DEBUG
#define HEXCTRL_LIBNAME "HexCtrlx86D"
#else //^^^ _DEBUG / vvv !_DEBUG
#define HEXCTRL_LIBNAME "HexCtrlx86"
#endif //^^^ !_DEBUG
#elif defined(_M_X64) //^^^ _M_IX86 / vvv _M_X64
#ifdef _DEBUG
#define HEXCTRL_LIBNAME "HexCtrlx64D"
#else //^^^ _DEBUG / vvv !_DEBUG
#define HEXCTRL_LIBNAME "HexCtrlx64"
#endif //^^^ !_DEBUG
#elif defined(_M_ARM64) //^^^ _M_X64 / vvv _M_ARM64
#ifdef _DEBUG
#define HEXCTRL_LIBNAME "HexCtrlARM64D"
#else //^^^ _DEBUG / vvv !_DEBUG
#define HEXCTRL_LIBNAME "HexCtrlARM64"
#endif //^^^ _DEBUG
#endif //^^^ _M_ARM64
#pragma comment(lib, HEXCTRL_LIBNAME)

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CMFCDialogDLLApp theApp;

BOOL CMFCDialogDLLApp::InitInstance()
{
	CWinApp::InitInstance();

	auto dlg = new CMFCDialogDLLDlg;
	m_pMainWnd = dlg;
	dlg->DoModal();
	delete dlg;

	return FALSE;
}