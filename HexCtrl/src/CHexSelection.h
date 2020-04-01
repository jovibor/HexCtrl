/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#pragma once
#include "CHexCtrl.h"
#include <vector>

namespace HEXCTRL::INTERNAL
{
	class CHexSelection final
	{
	public:
		explicit CHexSelection() = default;
		~CHexSelection() = default;
		void Attach(CHexCtrl* p);
		void ClearAll();
		[[nodiscard]] CHexCtrl* GetHexCtrl();
		[[nodiscard]] ULONGLONG GetSelectionEnd()const;
		[[nodiscard]] ULONGLONG GetSelectionSize()const;
		[[nodiscard]] ULONGLONG GetSelectionStart()const;
		[[nodiscard]] DWORD GetLineLength()const;  //Length of the selected line. Used in block selection (with Alt).
		[[nodiscard]] ULONGLONG GetOffsetByIndex(ULONGLONG ullIndex)const;  //Retrieves selection's offset by index [0...selectionSize)
		[[nodiscard]] auto GetData()const->std::vector<HEXSPANSTRUCT>;
		[[nodiscard]] bool HasSelection()const;
		[[nodiscard]] bool HitTest(ULONGLONG ullIndex)const;
		void SetSelection(const std::vector<HEXSPANSTRUCT>& vecSelect);
		void SetSelectionEnd(ULONGLONG ullOffset);
		void SetSelectionStart(ULONGLONG ullOffset);
	private:
		CHexCtrl* m_pHex { };
		std::vector<HEXSPANSTRUCT> m_vecSelect { };
		ULONGLONG m_ullMarkSelStart { 0xFFFFFFFFFFFFFFFFULL };  //For SetSelectionStart().
		ULONGLONG m_ullMarkSelEnd { 0xFFFFFFFFFFFFFFFFULL };    //For SetSelectionEnd().
	};
}