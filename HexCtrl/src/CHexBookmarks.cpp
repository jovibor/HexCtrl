/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/                         *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#include "stdafx.h"
#include "CHexBookmarks.h"
#include <algorithm>

using namespace HEXCTRL::INTERNAL;

void CHexBookmarks::Attach(CHexCtrl * pHex)
{
	m_pHex = pHex;
}

void CHexBookmarks::Add(ULONGLONG ullOffset, ULONGLONG ullSize)
{
	if (ullSize == 0)
		return;

	m_vecBookmarks.emplace_back(BOOKMARKSTRUCT { ullOffset, ullSize });
}

void CHexBookmarks::Remove(ULONGLONG ullOffset)
{
	if (!m_pHex || m_vecBookmarks.empty() || !m_pHex->IsDataSet())
		return;

	auto iter = std::find_if(m_vecBookmarks.begin(), m_vecBookmarks.end(),
		[ullOffset](const BOOKMARKSTRUCT& r)
	{return ullOffset >= r.ullOffset && ullOffset < (r.ullOffset + r.ullSize); });
	if (iter != m_vecBookmarks.end()) {
		m_vecBookmarks.erase(iter);
		m_pHex->RedrawWindow();
	}
}

void CHexBookmarks::ClearAll()
{
	m_vecBookmarks.clear();

	if (m_pHex)
		m_pHex->RedrawWindow();
}

void CHexBookmarks::GoNext()
{
	if (!m_pHex || m_vecBookmarks.empty())
		return;

	if (++m_iCurrent > int(m_vecBookmarks.size() - 1))
		m_iCurrent = 0;

	m_pHex->GoToOffset(m_vecBookmarks.at((size_t)m_iCurrent).ullOffset);
}

void CHexBookmarks::GoPrev()
{
	if (!m_pHex || m_vecBookmarks.empty())
		return;

	if (--m_iCurrent < 0)
		m_iCurrent = (int)m_vecBookmarks.size() - 1;

	m_pHex->GoToOffset(m_vecBookmarks.at((size_t)m_iCurrent).ullOffset);
}

bool CHexBookmarks::HitTest(ULONGLONG ullByte)
{
	if (m_vecBookmarks.empty())
		return false;

	return std::find_if(m_vecBookmarks.begin(), m_vecBookmarks.end(),
		[ullByte](const BOOKMARKSTRUCT& r) {return ullByte >= r.ullOffset && ullByte < (r.ullOffset + r.ullSize); })
		!= m_vecBookmarks.end();
}

bool CHexBookmarks::HasBookmarks()const
{
	return !m_vecBookmarks.empty();
}