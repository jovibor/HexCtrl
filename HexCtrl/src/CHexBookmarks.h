/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../HexCtrl.h"
#include <ctime>
#include <deque>
#include <optional>
#include <vector>

namespace HEXCTRL::INTERNAL
{
	class CHexBookmarks
	{
	public:
		ULONGLONG Add(const HEXBKM& hbs, bool fRedraw = true); //Returns new bookmark Id.
		void Attach(IHexCtrl* pHexCtrl);
		void ClearAll();
		[[nodiscard]] auto GetByID(ULONGLONG ullID)->HEXBKM*;       //Bookmark by ID.
		[[nodiscard]] auto GetByIndex(ULONGLONG ullIndex)->HEXBKM*; //Bookmark by index (in inner list).
		[[nodiscard]] ULONGLONG GetCount()const;
		[[nodiscard]] ULONGLONG GetCurrent()const;
		[[nodiscard]] auto GetTouchTime()const->__time64_t;
		void GoBookmark(ULONGLONG ullIndex);
		void GoNext();
		void GoPrev();
		[[nodiscard]] bool HasBookmarks()const;
		[[nodiscard]] auto HitTest(ULONGLONG ullOffset)->HEXBKM*;
		[[nodiscard]] bool IsVirtual()const;
		void Remove(ULONGLONG ullOffset);
		void RemoveByID(ULONGLONG ullID);
		void SetVirtual(bool fEnable, IHexVirtBkm* pVirtual = nullptr);
		void SortData(int iColumn, bool fAscending);
		void Update(ULONGLONG ullID, const HEXBKM& stBookmark);
	private:
		std::deque<HEXBKM> m_deqBookmarks;
		IHexCtrl* m_pHexCtrl { };
		IHexVirtBkm* m_pVirtual { };
		LONGLONG m_llIndexCurr { }; //Current bookmark position index, to move next/prev.
		__time64_t m_time { };      //Last modification time.
		bool m_fVirtual { false };  //Working in Virtual mode or not.
	};
}