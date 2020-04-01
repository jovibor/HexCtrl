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

	ULONGLONG ullID { };
	if (m_fVirtual)
	{
		if (!m_pVirtual)
			return static_cast<ULONGLONG>(-1);
		ullID = m_pVirtual->Add(hbs);
	}
	else
	{
		auto iter = std::max_element(m_deqBookmarks.begin(), m_deqBookmarks.end(),
			[](const HEXBOOKMARKSTRUCT& ref1, const HEXBOOKMARKSTRUCT& ref2)
		{return ref1.ullID < ref2.ullID; });

		ullID = 1; //Bookmarks' ID starts from 1.
		if (iter != m_deqBookmarks.end())
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
		if (!m_pVirtual)
			return;
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
	if (m_fVirtual)
	{
		if (!m_pVirtual)
			return nullptr;
		return m_pVirtual->GetByID(ullID);
	}
	else
	{
		auto iter = std::find_if(m_deqBookmarks.begin(), m_deqBookmarks.end(),
			[ullID](const HEXBOOKMARKSTRUCT& ref) {return ullID == ref.ullID; });

		if (iter != m_deqBookmarks.end())
			return &*iter;
	}

	return nullptr;
}

auto CHexBookmarks::GetData()->std::deque<HEXBOOKMARKSTRUCT>*
{
	return &m_deqBookmarks;
}

auto CHexBookmarks::GetByIndex(ULONGLONG ullIndex)->HEXBOOKMARKSTRUCT*
{
	if (m_fVirtual)
	{
		if (m_pVirtual)
			return m_pVirtual->GetByIndex(ullIndex);
		return nullptr;
	}

	if (ullIndex >= m_deqBookmarks.size())
		return nullptr;

	return &m_deqBookmarks.at(static_cast<size_t>(ullIndex));
}

ULONGLONG CHexBookmarks::GetCount()
{
	if (m_fVirtual)
	{
		if (m_pVirtual)
			return m_pVirtual->GetCount();
		return 0;
	}

	return m_deqBookmarks.size();
}

auto CHexBookmarks::GetTouchTime()const->__time64_t
{
	return m_time;
}

void CHexBookmarks::GoBookmark(ULONGLONG ullID)
{
	if (!m_pHex || m_fVirtual)
		return;

	auto iter = std::find_if(m_deqBookmarks.begin(), m_deqBookmarks.end(),
		[ullID](const HEXBOOKMARKSTRUCT& ref) {return ullID == ref.ullID; });

	if (iter != m_deqBookmarks.end())
		m_pHex->GoToOffset(iter->vecSpan.front().ullOffset, true, 1);
}

void CHexBookmarks::GoNext()
{
	if (!m_pHex || m_deqBookmarks.empty())
		return;

	if (m_fVirtual)
	{
		if (m_pVirtual)
		{
			auto ptr = m_pVirtual->GetNext();
			if (ptr)
				m_pHex->GoToOffset(ptr->vecSpan.front().ullOffset);
		}
		return;
	}

	if (++m_iCurrent > static_cast<int>(m_deqBookmarks.size() - 1))
		m_iCurrent = 0;

	m_pHex->GoToOffset(m_deqBookmarks.at(static_cast<size_t>(m_iCurrent)).vecSpan.front().ullOffset, true, 1);
}

void CHexBookmarks::GoPrev()
{
	if (!m_pHex || m_deqBookmarks.empty())
		return;

	if (m_fVirtual)
	{
		if (m_pVirtual)
		{
			auto ptr = m_pVirtual->GetPrev();
			if (ptr)
				m_pHex->GoToOffset(ptr->vecSpan.front().ullOffset);
		}
		return;
	}

	if (--m_iCurrent < 0)
		m_iCurrent = static_cast<int>(m_deqBookmarks.size()) - 1;

	m_pHex->GoToOffset(m_deqBookmarks.at(static_cast<size_t>(m_iCurrent)).vecSpan.front().ullOffset, true, 1);
}

bool CHexBookmarks::HasBookmarks()const
{
	if (m_fVirtual)
	{
		if (m_pVirtual)
			return m_pVirtual->GetCount() > 0;
		return false;
	}

	return !m_deqBookmarks.empty();
}

auto CHexBookmarks::HitTest(ULONGLONG ullOffset)->HEXBOOKMARKSTRUCT*
{
	if (m_fVirtual)
	{
		if (m_pVirtual)
			return m_pVirtual->HitTest(ullOffset);
		return nullptr;
	}

	auto riter = std::find_if(m_deqBookmarks.rbegin(), m_deqBookmarks.rend(),
		[ullOffset](const HEXBOOKMARKSTRUCT& ref)
	{return std::any_of(ref.vecSpan.begin(), ref.vecSpan.end(),
		[ullOffset](const HEXSPANSTRUCT& refV)
	{return ullOffset >= refV.ullOffset && ullOffset < (refV.ullOffset + refV.ullSize); });
	});

	if (riter != m_deqBookmarks.rend())
		return &*riter;

	return nullptr;
}

bool CHexBookmarks::IsVirtual()const
{
	return m_fVirtual;
}

void CHexBookmarks::Remove(ULONGLONG ullOffset)
{
	if (!m_pHex || m_deqBookmarks.empty() || !m_pHex->IsDataSet())
		return;

	if (m_fVirtual)
	{
		if (!m_pVirtual)
			return;
		m_pVirtual->Remove(ullOffset);
	}
	else
	{
		//Searching from the end, to remove last added bookmark if few at the given offset.
		auto riter = std::find_if(m_deqBookmarks.rbegin(), m_deqBookmarks.rend(),
			[ullOffset](const HEXBOOKMARKSTRUCT& ref)
		{return std::any_of(ref.vecSpan.begin(), ref.vecSpan.end(),
			[ullOffset](const HEXSPANSTRUCT& refV)
		{return ullOffset >= refV.ullOffset && ullOffset < (refV.ullOffset + refV.ullSize); });
		});

		if (riter != m_deqBookmarks.rend())
			m_deqBookmarks.erase((riter + 1).base()); //Weird notation for reverse_iterator to work in erase() (acc to standard).
	}

	if (m_pHex)
		m_pHex->RedrawWindow();
	m_time = _time64(nullptr);
}

void CHexBookmarks::RemoveByID(ULONGLONG ullID)
{
	if (m_fVirtual)
	{
		if (!m_pVirtual)
			return;
		m_pVirtual->RemoveByID(ullID);
	}
	else
	{
		if (m_deqBookmarks.empty())
			return;

		auto iter = std::find_if(m_deqBookmarks.begin(), m_deqBookmarks.end(),
			[ullID](const HEXBOOKMARKSTRUCT& ref) {return ullID == ref.ullID; });

		if (iter != m_deqBookmarks.end())
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