/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../HexCtrl.h"
#include <vector>

namespace HEXCTRL::INTERNAL
{
	class CHexSelection final {
	public:
		void ClearAll();
		[[nodiscard]] ULONGLONG GetSelEnd()const;
		[[nodiscard]] ULONGLONG GetSelSize()const;
		[[nodiscard]] ULONGLONG GetSelStart()const;
		[[nodiscard]] DWORD GetLineLength()const;  //Length of the selected line. Used in block selection (with Alt).
		[[nodiscard]] ULONGLONG GetOffsetByIndex(ULONGLONG ullIndex)const;  //Retrieves selection's offset by index [0...GetSelSize())
		[[nodiscard]] auto GetData()const->VecSpan;
		[[nodiscard]] bool HasSelection()const;
		[[nodiscard]] bool HasSelHighlight()const;
		[[nodiscard]] bool HitTest(ULONGLONG ullOffset)const;           //Is given offset within selection.
		[[nodiscard]] bool HitTestHighlight(ULONGLONG ullOffset)const;  //Is given offset within highlighted selection.
		[[nodiscard]] bool HitTestRange(const HEXSPAN& hss)const;       //Is there any selection within given range.
		void SetSelection(const VecSpan& vecSel, bool fHighlight);      //Set a selection or selection highlight.
		void SetSelStartEnd(ULONGLONG ullOffset, bool fStart);          //fStart true: Start, false: End.
	private:
		VecSpan m_vecSelection { };         //Selection data vector.
		VecSpan m_vecSelHighlight { };      //Selection highlight data vector.
		ULONGLONG m_ullMarkSelStart { 0xFFFFFFFFFFFFFFFFULL }; //For SetSelStartEnd().
		ULONGLONG m_ullMarkSelEnd { 0xFFFFFFFFFFFFFFFFULL };   //For SetSelStartEnd().
	};
}