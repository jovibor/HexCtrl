#pragma once
#include <afxcontrolbars.h>

class CMFCDialogDLLApp : public CWinApp {
public:
	BOOL InitInstance()override;
	BOOL PreTranslateMessage(MSG* pMsg)override;
	DECLARE_MESSAGE_MAP();
};