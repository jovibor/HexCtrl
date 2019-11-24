/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/                         *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#pragma once
#include "CHexCtrl.h"
#include <vector>

namespace HEXCTRL::INTERNAL
{
	class CHexSelect
	{
	public:
		bool HasSelection()const;
		bool HitTest(ULONGLONG ullIndex)const;
		void SetSelection(const std::vector<HEXSPANSTRUCT>& vecSelect);
		ULONGLONG GetSelectionSize()const;
		ULONGLONG GetSelectionStart()const;
		ULONGLONG GetSelectionEnd()const;
		ULONGLONG GetOffsetByIndex(ULONGLONG ullIndex)const;  //Retrieves selection offset by index [0...selectionSize)
		DWORD GetLineLength()const;  //Length of the selected line. Used in block selection (with Alt).
		std::vector<HEXSPANSTRUCT>& GetVector();
		void ClearAll();
	private:
		std::vector<HEXSPANSTRUCT> m_vecSelect { };
	};
}