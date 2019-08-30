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

void CHexBookmarks::Add()
{
	if (!m_pHex)
		return;

	ULONGLONG ullOffset, ullSize;
	m_pHex->GetSelection(ullOffset, ullSize);
	m_vecBookmarks.emplace_back(BOOKMARKSTRUCT { ullOffset, ullSize });
}

void CHexBookmarks::Remove()
{
	if (!m_pHex || m_vecBookmarks.empty() || !m_pHex->IsDataSet())
		return;

	ULONGLONG ullByte = m_pHex->m_ullRMouseHex;
	if (ullByte == 0xFFFFFFFFFFFFFFFFull)
		if (m_pHex->m_ullSelectionSize > 0)
			ullByte = m_pHex->m_ullSelectionClick;
		else
			return;

	auto iter = std::find_if(m_vecBookmarks.begin(), m_vecBookmarks.end(),
		[ullByte](const BOOKMARKSTRUCT& r)
	{return ullByte >= r.ullOffset && ullByte < (r.ullOffset + r.ullSize); });
	if (iter != m_vecBookmarks.end()) {
		m_vecBookmarks.erase(iter);
		m_pHex->RedrawWindow();
	}

	m_pHex->m_ullRMouseHex = 0xFFFFFFFFFFFFFFFFull;
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
		[ullByte](const BOOKMARKSTRUCT& r)
	{return ullByte >= r.ullOffset && ullByte < (r.ullOffset + r.ullSize); })
		!= m_vecBookmarks.end();
}

bool CHexBookmarks::HasBookmarks()
{
	return !m_vecBookmarks.empty();
}