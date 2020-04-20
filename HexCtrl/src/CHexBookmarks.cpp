/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#include "stdafx.h"
#include "CHexBookmarks.h"
#include <algorithm>

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

void CHexBookmarks::Attach(CHexCtrl* pHex)
{
	m_pHex = pHex;
}

ULONGLONG CHexBookmarks::Add(const HEXBOOKMARKSTRUCT& hbs, bool fRedraw)
{
	if (!m_pHex || !m_pHex->IsDataSet())
		return 0;

	ULONGLONG ullID { static_cast<ULONGLONG>(-1) };
	if (m_fVirtual)
	{
		if (m_pVirtual)
			ullID = m_pVirtual->Add(hbs);
	}
	else
	{
		ullID = 1; //Bookmarks' ID starts from 1.

		if (auto iter = std::max_element(m_deqBookmarks.begin(), m_deqBookmarks.end(),
			[](const HEXBOOKMARKSTRUCT& ref1, const HEXBOOKMARKSTRUCT& ref2)
			{return ref1.ullID < ref2.ullID; }); iter != m_deqBookmarks.end())
			ullID = iter->ullID + 1; //Increasing next bookmark's ID by 1.

		m_deqBookmarks.emplace_back(
			HEXBOOKMARKSTRUCT { hbs.vecSpan, hbs.wstrDesc, ullID, hbs.ullData, hbs.clrBk, hbs.clrText });
	}

	if (fRedraw && m_pHex)
		m_pHex->RedrawWindow();
	m_time = _time64(nullptr);

	return ullID;
}

void CHexBookmarks::ClearAll()
{
	if (m_fVirtual)
	{
		if (m_pVirtual)
			m_pVirtual->ClearAll();
	}
	else
		m_deqBookmarks.clear();

	if (m_pHex)
		m_pHex->RedrawWindow();
	m_time = _time64(nullptr);
}

auto CHexBookmarks::GetByID(ULONGLONG ullID)->HEXBOOKMARKSTRUCT*
{
	HEXBOOKMARKSTRUCT* pBkm { };
	if (m_fVirtual)
	{
		if (m_pVirtual)
			pBkm = m_pVirtual->GetByID(ullID);
	}
	else
	{
		if (auto iter = std::find_if(m_deqBookmarks.begin(), m_deqBookmarks.end(),
			[ullID](const HEXBOOKMARKSTRUCT& ref) {return ullID == ref.ullID; }); iter != m_deqBookmarks.end())
			pBkm = &*iter;
	}

	return pBkm;
}

auto CHexBookmarks::GetData()->std::deque<HEXBOOKMARKSTRUCT>*
{
	return &m_deqBookmarks;
}

auto CHexBookmarks::GetByIndex(ULONGLONG ullIndex)->HEXBOOKMARKSTRUCT*
{
	HEXBOOKMARKSTRUCT* pBkm { };
	if (m_fVirtual)
	{
		if (m_pVirtual)
			pBkm = m_pVirtual->GetByIndex(ullIndex);
	}
	else
	{
		if (ullIndex < m_deqBookmarks.size())
			pBkm = &m_deqBookmarks.at(static_cast<size_t>(ullIndex));
	}

	return pBkm;
}

ULONGLONG CHexBookmarks::GetCount()const
{
	ULONGLONG ullCount { };
	if (m_fVirtual)
	{
		if (m_pVirtual)
			ullCount = m_pVirtual->GetCount();
	}
	else
		ullCount = m_deqBookmarks.size();

	return ullCount;
}

ULONGLONG CHexBookmarks::GetCurrent()const
{
	return static_cast<ULONGLONG>(m_llCurrent);
}

auto CHexBookmarks::GetTouchTime()const->__time64_t
{
	return m_time;
}

void CHexBookmarks::GoBookmark(ULONGLONG ullIndex)
{
	if (!m_pHex)
		return;

	if (auto pBkm = GetByIndex(ullIndex); pBkm != nullptr)
	{
		m_llCurrent = static_cast<LONGLONG>(ullIndex);
		m_pHex->GoToOffset(pBkm->vecSpan.front().ullOffset, true, 1);
	}
}

void CHexBookmarks::GoNext()
{
	if (!m_pHex)
		return;

	if (++m_llCurrent >= static_cast<LONGLONG>(GetCount()))
		m_llCurrent = 0;

	if (auto pBkm = GetByIndex(m_llCurrent); pBkm != nullptr)
		m_pHex->GoToOffset(pBkm->vecSpan.front().ullOffset, true, 1);
}

void CHexBookmarks::GoPrev()
{
	if (!m_pHex)
		return;

	if (--m_llCurrent; m_llCurrent < 0 || m_llCurrent >= static_cast<LONGLONG>(GetCount()))
		m_llCurrent = static_cast<LONGLONG>(GetCount()) - 1;

	if (auto pBkm = GetByIndex(m_llCurrent); pBkm != nullptr)
		m_pHex->GoToOffset(pBkm->vecSpan.front().ullOffset, true, 1);
}

bool CHexBookmarks::HasBookmarks()const
{
	bool fHasBookmarks { false };
	if (m_fVirtual)
	{
		if (m_pVirtual)
			fHasBookmarks = m_pVirtual->GetCount() > 0;
	}
	else
		fHasBookmarks = !m_deqBookmarks.empty();

	return fHasBookmarks;
}

auto CHexBookmarks::HitTest(ULONGLONG ullOffset)->HEXBOOKMARKSTRUCT*
{
	HEXBOOKMARKSTRUCT* pBkm { };
	if (m_fVirtual)
	{
		if (m_pVirtual)
			pBkm = m_pVirtual->HitTest(ullOffset);
	}
	else
	{
		if (auto rIter = std::find_if(m_deqBookmarks.rbegin(), m_deqBookmarks.rend(),
			[ullOffset](const HEXBOOKMARKSTRUCT& ref)
			{return std::any_of(ref.vecSpan.begin(), ref.vecSpan.end(),
				[ullOffset](const HEXSPANSTRUCT& refV)
				{return ullOffset >= refV.ullOffset && ullOffset < (refV.ullOffset + refV.ullSize); });
			}); rIter != m_deqBookmarks.rend())
			pBkm = &*rIter;
	}

	return pBkm;
}

bool CHexBookmarks::IsVirtual()const
{
	return m_fVirtual;
}

void CHexBookmarks::Remove(ULONGLONG ullOffset)
{
	if (!m_pHex || !m_pHex->IsDataSet())
		return;

	if (m_fVirtual)
	{
		if (m_pVirtual)
		{
			if (auto pBkm = m_pVirtual->HitTest(ullOffset); pBkm != nullptr)
				m_pVirtual->RemoveByID(pBkm->ullID);
		}
	}
	else
	{
		if (m_deqBookmarks.empty())
			return;

		//Searching from the end, to remove last added bookmark if few at the given offset.
		if (auto rIter = std::find_if(m_deqBookmarks.rbegin(), m_deqBookmarks.rend(),
			[ullOffset](const HEXBOOKMARKSTRUCT& ref)
			{return std::any_of(ref.vecSpan.begin(), ref.vecSpan.end(),
				[ullOffset](const HEXSPANSTRUCT& refV)
				{return ullOffset >= refV.ullOffset && ullOffset < (refV.ullOffset + refV.ullSize); });
			}); rIter != m_deqBookmarks.rend())
			m_deqBookmarks.erase((rIter + 1).base()); //Weird notation for reverse_iterator to work in erase() (acc to standard).
	}

	if (m_pHex)
		m_pHex->RedrawWindow();
	m_time = _time64(nullptr);
}

void CHexBookmarks::RemoveByID(ULONGLONG ullID)
{
	if (m_fVirtual)
	{
		if (m_pVirtual)
			m_pVirtual->RemoveByID(ullID);
	}
	else
	{
		if (m_deqBookmarks.empty())
			return;

		if (auto iter = std::find_if(m_deqBookmarks.begin(), m_deqBookmarks.end(),
			[ullID](const HEXBOOKMARKSTRUCT& ref) {return ullID == ref.ullID; }); iter != m_deqBookmarks.end())
			m_deqBookmarks.erase(iter);
	}

	if (m_pHex)
		m_pHex->RedrawWindow();
	m_time = _time64(nullptr);
}

void CHexBookmarks::SetVirtual(bool fEnable, IHexBkmVirtual* pVirtual)
{
	m_fVirtual = fEnable;
	if (fEnable && pVirtual != nullptr)
		m_pVirtual = pVirtual;

	m_time = _time64(nullptr);
}

void CHexBookmarks::Update(ULONGLONG ullID, const HEXBOOKMARKSTRUCT& stBookmark)
{
	if (m_fVirtual || m_deqBookmarks.empty())
		return;

	auto iter = std::find_if(m_deqBookmarks.begin(), m_deqBookmarks.end(),
		[ullID](const HEXBOOKMARKSTRUCT& ref) {return ullID == ref.ullID; });

	if (iter != m_deqBookmarks.end())
		*iter = stBookmark;

	if (m_pHex)
		m_pHex->RedrawWindow();
	m_time = _time64(nullptr);
}