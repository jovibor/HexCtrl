/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
****************************************************************************************/
#pragma once
#include "../HexCtrlDefs.h"
#include <vector>

namespace HEXCTRL::INTERNAL
{
	class CHexSelection final
	{
	public:
		void ClearAll();
		[[nodiscard]] ULONGLONG GetSelEnd()const;
		[[nodiscard]] ULONGLONG GetSelSize()const;
		[[nodiscard]] ULONGLONG GetSelStart()const;
		[[nodiscard]] DWORD GetLineLength()const;  //Length of the selected line. Used in block selection (with Alt).
		[[nodiscard]] ULONGLONG GetOffsetByIndex(ULONGLONG ullIndex)const;  //Retrieves selection's offset by index [0...GetSelSize())
		[[nodiscard]] auto GetData()const->std::vector<HEXOFFSET>;
		[[nodiscard]] bool HasSelection()const;
		[[nodiscard]] bool HasSelHighlight()const;
		[[nodiscard]] bool HitTest(ULONGLONG ullOffset)const;           //Is given offset within selection.
		[[nodiscard]] bool HitTestHighlight(ULONGLONG ullOffset)const;  //Is given offset within highlighted selection.
		[[nodiscard]] bool HitTestRange(const HEXOFFSET& hss)const; //Is there any selection within given range.
		void SetSelection(const std::vector<HEXOFFSET>& vecSel, bool fHighlight); //Set a selection or selection highlight.
		void SetSelStartEnd(ULONGLONG ullOffset, bool fStart); //fStart true - Start, false - End.
	private:
		std::vector<HEXOFFSET> m_vecSelection { };         //Selection data vector.
		std::vector<HEXOFFSET> m_vecSelHighlight { };      //Selection highlight data vector.
		ULONGLONG m_ullMarkSelStart { 0xFFFFFFFFFFFFFFFFULL }; //For SetSelStartEnd().
		ULONGLONG m_ullMarkSelEnd { 0xFFFFFFFFFFFFFFFFULL };   //For SetSelStartEnd().
	};
}