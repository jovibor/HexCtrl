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
#include <ctime>
#include <optional>
#include <vector>

namespace HEXCTRL::INTERNAL
{
	class CHexBookmarks
	{
	public:
		explicit CHexBookmarks() = default;
		~CHexBookmarks() = default;
		DWORD Add(const HEXBOOKMARKSTRUCT& hbs, bool fRedraw = true); //Returns new bookmark Id.
		void Attach(CHexCtrl* pHex);
		void ClearAll();
		[[nodiscard]] auto GetBookmark(DWORD dwID)const->std::optional<HEXBOOKMARKSTRUCT>;
		[[nodiscard]] auto GetData()const->const std::deque<HEXBOOKMARKSTRUCT>*;
		[[nodiscard]] auto GetTouchTime()const->__time64_t;
		void GoBookmark(DWORD dwID);
		void GoNext();
		void GoPrev();
		[[nodiscard]] bool HasBookmarks()const;
		[[nodiscard]] auto HitTest(ULONGLONG ullOffset)->HEXBOOKMARKSTRUCT*;
		[[nodiscard]] bool IsVirtual()const;
		void Remove(ULONGLONG ullOffset);
		void RemoveId(DWORD dwID);
		void SetVirtual(bool fEnable, IHexBkmVirtual* pVirtual = nullptr);
		void Update(DWORD dwID, const HEXBOOKMARKSTRUCT& stBookmark);
	private:
		std::deque<HEXBOOKMARKSTRUCT> m_deqBookmarks;
		CHexCtrl* m_pHex { };
		IHexBkmVirtual* m_pVirtual { };
		int m_iCurrent { };        //Current bookmark position in deque, to move next/prev.
		__time64_t m_time { };     //Last modification time.
		bool m_fVirtual { false }; //Working in Virtual mode or not.
	};
}