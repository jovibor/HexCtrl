#pragma once
#include "Resource.h"
#include <afxcontrolbars.h>

class CHexSampleApp : public CWinApp
{
public:
	CHexSampleApp() = default;
public:
	BOOL InitInstance() override;
	DECLARE_MESSAGE_MAP()
};