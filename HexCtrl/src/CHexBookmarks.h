/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
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
	class CHexBookmarks : public IHexBookmarks
	{
	public:
		ULONGLONG AddBkm(const HEXBKM& hbs, bool fRedraw)override; //Returns new bookmark Id.
		void Attach(IHexCtrl* pHexCtrl);
		void ClearAll()override;
		[[nodiscard]] auto GetByID(ULONGLONG ullID)->PHEXBKM override;       //Bookmark by ID.
		[[nodiscard]] auto GetByIndex(ULONGLONG ullIndex)->PHEXBKM override; //Bookmark by index (in inner list).
		[[nodiscard]] ULONGLONG GetCount()override;
		[[nodiscard]] ULONGLONG GetCurrent()const;
		[[nodiscard]] auto GetTouchTime()const->__time64_t;
		void GoBookmark(ULONGLONG ullIndex);
		void GoNext();
		void GoPrev();
		[[nodiscard]] bool HasBookmarks()const;
		[[nodiscard]] auto HitTest(ULONGLONG ullOffset)->PHEXBKM override;
		[[nodiscard]] bool IsVirtual()const;
		void RemoveByOffset(ULONGLONG ullOffset);
		void RemoveByID(ULONGLONG ullID)override;
		void SetVirtual(IHexBookmarks* pVirtBkm);
		void SortData(int iColumn, bool fAscending);
		void Update(ULONGLONG ullID, const HEXBKM& stBookmark);
	private:
		std::deque<HEXBKM> m_deqBookmarks;
		IHexCtrl* m_pHexCtrl { };
		IHexBookmarks* m_pVirtual { };
		LONGLONG m_llIndexCurr { }; //Current bookmark position index, to move next/prev.
		__time64_t m_time { };      //Last modification time.
	};
}