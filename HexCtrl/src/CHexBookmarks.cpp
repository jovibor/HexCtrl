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

DWORD CHexBookmarks::Add(const HEXBOOKMARKSTRUCT& hbs)
{
	if (!m_pHex || !m_pHex->IsDataSet())
		return 0;

	if (m_pVirtual)
		return m_pVirtual->Add(hbs);

	auto iter = std::max_element(m_deqBookmarks.begin(), m_deqBookmarks.end(), []
	(const HEXBOOKMARKSTRUCT& ref1, const HEXBOOKMARKSTRUCT& ref2)
	{return ref1.dwID < ref2.dwID; });

	DWORD dwId { 1 }; //Bookmarks Id starts from 1.
	if (iter != m_deqBookmarks.end())
		dwId = iter->dwID + 1; //Increasing next bookmark's Id by 1.

	m_deqBookmarks.emplace_back(HEXBOOKMARKSTRUCT
		{ hbs.vecSpan, hbs.wstrDesc, hbs.clrBk, hbs.clrText, dwId });
	if (m_pHex)
		m_pHex->RedrawWindow();

	m_time = std::time(0);

	return dwId;
}

void CHexBookmarks::ClearAll()
{
	if (m_pVirtual)
		return m_pVirtual->ClearAll();

	m_deqBookmarks.clear();
	if (m_pHex)
		m_pHex->RedrawWindow();

	m_time = std::time(0);
}

auto CHexBookmarks::GetBookmark(DWORD dwID)->HEXBOOKMARKSTRUCT*
{
	if (m_deqBookmarks.empty())
		return nullptr;

	auto iter = std::find_if(m_deqBookmarks.begin(), m_deqBookmarks.end(),
		[dwID](const HEXBOOKMARKSTRUCT& ref) {return dwID == ref.dwID; });

	if (iter != m_deqBookmarks.end())
	{
		return &*iter;
	}
	else
		return nullptr;
}

auto CHexBookmarks::GetData()->std::deque<HEXBOOKMARKSTRUCT>&
{
	return m_deqBookmarks;
}

auto CHexBookmarks::GetTouchTime()const ->__time64_t
{
	return m_time;
}

void CHexBookmarks::GoBookmark(DWORD dwID)
{
	if (!m_pHex || m_pVirtual)
		return;

	auto iter = std::find_if(m_deqBookmarks.begin(), m_deqBookmarks.end(),
		[dwID](const HEXBOOKMARKSTRUCT& ref) {return dwID == ref.dwID; });

	if (iter != m_deqBookmarks.end())
		m_pHex->GoToOffset(iter->vecSpan.front().ullOffset);
}

void CHexBookmarks::GoNext()
{
	if (!m_pHex || m_deqBookmarks.empty())
		return;

	if (m_pVirtual)
	{
		auto ptr = m_pVirtual->GetNext();
		if (ptr)
			m_pHex->GoToOffset(ptr->vecSpan.front().ullOffset);
		return;
	}

	if (++m_iCurrent > int(m_deqBookmarks.size() - 1))
		m_iCurrent = 0;

	m_pHex->GoToOffset(m_deqBookmarks.at((size_t)m_iCurrent).vecSpan.front().ullOffset);
}

void CHexBookmarks::GoPrev()
{
	if (!m_pHex || m_deqBookmarks.empty())
		return;

	if (m_pVirtual)
	{
		auto ptr = m_pVirtual->GetPrev();
		if (ptr)
			m_pHex->GoToOffset(ptr->vecSpan.front().ullOffset);
		return;
	}

	if (--m_iCurrent < 0)
		m_iCurrent = (int)m_deqBookmarks.size() - 1;

	m_pHex->GoToOffset(m_deqBookmarks.at((size_t)m_iCurrent).vecSpan.front().ullOffset);
}

bool CHexBookmarks::HasBookmarks()const
{
	if (m_pVirtual)
		return m_pVirtual->HasBookmarks();

	return !m_deqBookmarks.empty();
}

auto CHexBookmarks::HitTest(ULONGLONG ullOffset)->HEXBOOKMARKSTRUCT*
{
	if (m_pVirtual)
		return m_pVirtual->HitTest(ullOffset);

	auto riter = std::find_if(m_deqBookmarks.rbegin(), m_deqBookmarks.rend(),
		[ullOffset](const HEXBOOKMARKSTRUCT& ref)
	{return std::any_of(ref.vecSpan.begin(), ref.vecSpan.end(),
		[ullOffset](const HEXSPANSTRUCT& refV)
	{return ullOffset >= refV.ullOffset && ullOffset < (refV.ullOffset + refV.ullSize); });
	});

	if (riter != m_deqBookmarks.rend())
		return &*riter;
	else
		return nullptr;
}

bool CHexBookmarks::IsVirtual()const
{
	return m_pVirtual;
}

void CHexBookmarks::Remove(ULONGLONG ullOffset)
{
	if (!m_pHex || m_deqBookmarks.empty() || !m_pHex->IsDataSet())
		return;

	if (m_pVirtual)
		return m_pVirtual->Remove(ullOffset);

	//Searching from the end, to remove last added bookmark if few at the given offset.
	auto riter = std::find_if(m_deqBookmarks.rbegin(), m_deqBookmarks.rend(),
		[ullOffset](const HEXBOOKMARKSTRUCT& ref)
	{return std::any_of(ref.vecSpan.begin(), ref.vecSpan.end(),
		[ullOffset](const HEXSPANSTRUCT& refV)
	{return ullOffset >= refV.ullOffset && ullOffset < (refV.ullOffset + refV.ullSize); });
	});

	if (riter != m_deqBookmarks.rend())
	{
		m_deqBookmarks.erase((riter + 1).base()); //Weird notation for reverse_iterator to work in erase() (acc to standard).
		m_pHex->RedrawWindow();
	}

	m_time = _time64(nullptr);
}

void CHexBookmarks::RemoveId(DWORD dwID)
{
	if (m_pVirtual)
		return m_pVirtual->RemoveId(dwID);

	if (m_deqBookmarks.empty())
		return;

	auto iter = std::find_if(m_deqBookmarks.begin(), m_deqBookmarks.end(),
		[dwID](const HEXBOOKMARKSTRUCT& ref) {return dwID == ref.dwID; });

	if (iter != m_deqBookmarks.end())
	{
		m_deqBookmarks.erase(iter);
		if (m_pHex)
			m_pHex->RedrawWindow();
	}

	m_time = std::time(0);
}

void CHexBookmarks::SetVirtual(IHexBkmVirtual* pVirtual)
{
	m_pVirtual = pVirtual;
}

void CHexBookmarks::Update(DWORD dwId, const HEXBOOKMARKSTRUCT& stBookmark)
{
	if (m_pVirtual || m_deqBookmarks.empty())
		return;

	auto iter = std::find_if(m_deqBookmarks.begin(), m_deqBookmarks.end(),
		[dwId](const HEXBOOKMARKSTRUCT& ref) {return dwId == ref.dwID; });

	if (iter != m_deqBookmarks.end())
	{
		*iter = stBookmark;
		if (m_pHex)
			m_pHex->RedrawWindow();
	}

	m_time = std::time(0);
}