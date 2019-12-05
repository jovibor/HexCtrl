/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/                         *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#pragma once
#include "CHexCtrl.h"
#include <vector>
#include <ctime>

namespace HEXCTRL::INTERNAL
{
	class CHexBookmarks
	{
	public:
		CHexBookmarks() {}
		void Attach(CHexCtrl* pHex);
		DWORD Add(const HEXBOOKMARKSTRUCT& stBookmark); //Returns new bookmark's Id.
		void Remove(ULONGLONG ullOffset);
		void RemoveId(DWORD dwId);
		void ClearAll();
		auto GetVector()->std::deque<HEXBOOKMARKSTRUCT>&;
		auto GetBookmark(DWORD dwId)->HEXBOOKMARKSTRUCT*;
		auto GetTouchTime()const->std::time_t;
		void GoBookmark(DWORD nId);
		void GoNext();
		void GoPrev();
		auto HitTest(ULONGLONG ullByte)->HEXBOOKMARKSTRUCT*;
		bool HasBookmarks()const;
		void Update(DWORD dwId, const HEXBOOKMARKSTRUCT& stBookmark);
	private:
		std::deque<HEXBOOKMARKSTRUCT> m_deqBookmarks;
		CHexCtrl* m_pHex { };
		int m_iCurrent { };
		std::time_t m_time { }; //Last modification time.
	};
}