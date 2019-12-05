/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/                         *
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

void CHexBookmarks::Attach(CHexCtrl * pHex)
{
	m_pHex = pHex;
}

DWORD CHexBookmarks::Add(const HEXBOOKMARKSTRUCT& stBookmark)
{
	auto iter = std::max_element(m_deqBookmarks.begin(), m_deqBookmarks.end(), []
	(const HEXBOOKMARKSTRUCT& ref1, const HEXBOOKMARKSTRUCT& ref2)
	{return ref1.dwId < ref2.dwId; });

	DWORD dwId { 1 }; //Bookmarks Id starts from 1.
	if (iter != m_deqBookmarks.end())
		dwId = iter->dwId + 1; //Increasing nex bookmark's Id by 1.

	m_deqBookmarks.emplace_back(HEXBOOKMARKSTRUCT
		{ stBookmark.vecSpan, stBookmark.wstrDesc, stBookmark.clrBk, stBookmark.clrText, dwId });
	if (m_pHex)
		m_pHex->RedrawWindow();

	m_time = std::time(0);

	return dwId;
}

void CHexBookmarks::Remove(ULONGLONG ullOffset)
{
	if (!m_pHex || m_deqBookmarks.empty() || !m_pHex->IsDataSet())
		return;

	//Searching from the end, to remove last added bookmark, if many at the given offset.
	auto iter = std::find_if(std::reverse_iterator(m_deqBookmarks.end()), std::reverse_iterator(m_deqBookmarks.begin()),
		[ullOffset](const HEXBOOKMARKSTRUCT& ref)
	{return std::any_of(ref.vecSpan.begin(), ref.vecSpan.end(), [ullOffset](const HEXSPANSTRUCT& refV)
	{return ullOffset >= refV.ullOffset && ullOffset < (refV.ullOffset + refV.ullSize); });
	});

	if (iter != std::reverse_iterator(m_deqBookmarks.begin()))
	{
		m_deqBookmarks.erase((iter + 1).base()); //Weird notation for reverse_iterator to work in erase() (acc to standard).
		m_pHex->RedrawWindow();
	}

	m_time = std::time(0);
}

void CHexBookmarks::RemoveId(DWORD dwId)
{
	if (m_deqBookmarks.empty())
		return;

	auto iter = std::find_if(m_deqBookmarks.begin(), m_deqBookmarks.end(),
		[dwId](const HEXBOOKMARKSTRUCT& ref) {return dwId == ref.dwId; });

	if (iter != m_deqBookmarks.end())
	{
		m_deqBookmarks.erase(iter);
		if (m_pHex)
			m_pHex->RedrawWindow();
	}

	m_time = std::time(0);
}

void CHexBookmarks::ClearAll()
{
	m_deqBookmarks.clear();
	if (m_pHex)
		m_pHex->RedrawWindow();

	m_time = std::time(0);
}

auto CHexBookmarks::GetVector()->std::deque<HEXBOOKMARKSTRUCT>&
{
	return m_deqBookmarks;
}

auto CHexBookmarks::GetBookmark(DWORD dwId)->HEXBOOKMARKSTRUCT*
{
	if (m_deqBookmarks.empty())
		return nullptr;

	auto iter = std::find_if(m_deqBookmarks.begin(), m_deqBookmarks.end(),
		[dwId](const HEXBOOKMARKSTRUCT& ref) {return dwId == ref.dwId; });

	if (iter != m_deqBookmarks.end())
	{
		return &*iter;
	}
	else
		return nullptr;
}

auto CHexBookmarks::GetTouchTime() const -> std::time_t
{
	return m_time;
}

void CHexBookmarks::GoBookmark(DWORD dwId)
{
	if (!m_pHex)
		return;

	auto iter = std::find_if(m_deqBookmarks.begin(), m_deqBookmarks.end(),
		[dwId](const HEXBOOKMARKSTRUCT& ref) {return dwId == ref.dwId; });

	if (iter != m_deqBookmarks.end())
		m_pHex->GoToOffset(iter->vecSpan.front().ullOffset);
}

void CHexBookmarks::GoNext()
{
	if (!m_pHex || m_deqBookmarks.empty())
		return;

	if (++m_iCurrent > int(m_deqBookmarks.size() - 1))
		m_iCurrent = 0;

	m_pHex->GoToOffset(m_deqBookmarks.at((size_t)m_iCurrent).vecSpan.front().ullOffset);
}

void CHexBookmarks::GoPrev()
{
	if (!m_pHex || m_deqBookmarks.empty())
		return;

	if (--m_iCurrent < 0)
		m_iCurrent = (int)m_deqBookmarks.size() - 1;

	m_pHex->GoToOffset(m_deqBookmarks.at((size_t)m_iCurrent).vecSpan.front().ullOffset);
}

auto CHexBookmarks::HitTest(ULONGLONG ullByte)->HEXBOOKMARKSTRUCT*
{
	auto iter = std::find_if(std::reverse_iterator(m_deqBookmarks.end()), std::reverse_iterator(m_deqBookmarks.begin()),
		[ullByte](const HEXBOOKMARKSTRUCT& ref)
	{return std::any_of(ref.vecSpan.begin(), ref.vecSpan.end(), [ullByte](const HEXSPANSTRUCT& refV)
	{return ullByte >= refV.ullOffset && ullByte < (refV.ullOffset + refV.ullSize); });
	});

	if (iter != std::reverse_iterator(m_deqBookmarks.begin()))
		return &*iter;
	else
		return nullptr;
}

bool CHexBookmarks::HasBookmarks()const
{
	return !m_deqBookmarks.empty();
}

void CHexBookmarks::Update(DWORD dwId, const HEXBOOKMARKSTRUCT& stBookmark)
{
	if (m_deqBookmarks.empty())
		return;

	auto iter = std::find_if(m_deqBookmarks.begin(), m_deqBookmarks.end(),
		[dwId](const HEXBOOKMARKSTRUCT& ref) {return dwId == ref.dwId; });

	if (iter != m_deqBookmarks.end())
	{
		*iter = stBookmark;
		if (m_pHex)
			m_pHex->RedrawWindow();
	}

	m_time = std::time(0);
}