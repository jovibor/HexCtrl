#pragma once
#include <afxcontrolbars.h>

class CSampleDialogDLLApp : public CWinApp {
public:
	BOOL InitInstance()override;
	BOOL PreTranslateMessage(MSG* pMsg)override;
	DECLARE_MESSAGE_MAP();
};