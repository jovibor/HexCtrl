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

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

void CHexBookmarks::Attach(CHexCtrl * pHex)
{
	m_pHex = pHex;
}

void CHexBookmarks::Add(const std::vector<HEXSPANSTRUCT>& vecBookmarks)
{
	m_deqBookmarks.emplace_back(vecBookmarks);
}

void CHexBookmarks::Remove(ULONGLONG ullOffset)
{
	if (!m_pHex || m_deqBookmarks.empty() || !m_pHex->IsDataSet())
		return;

	auto iter = std::find_if(m_deqBookmarks.begin(), m_deqBookmarks.end(), [ullOffset](const std::vector<HEXSPANSTRUCT>& ref)
	{return std::any_of(ref.begin(), ref.end(), [ullOffset](const HEXSPANSTRUCT& refV)
	{return ullOffset >= refV.ullOffset && ullOffset < (refV.ullOffset + refV.ullSize); });
	});

	if (iter != m_deqBookmarks.end())
	{
		m_deqBookmarks.erase(iter);
		m_pHex->RedrawWindow();
	}
}

void CHexBookmarks::ClearAll()
{
	m_deqBookmarks.clear();

	if (m_pHex)
		m_pHex->RedrawWindow();
}

void CHexBookmarks::GoNext()
{
	if (!m_pHex || m_deqBookmarks.empty())
		return;

	if (++m_iCurrent > int(m_deqBookmarks.size() - 1))
		m_iCurrent = 0;

	m_pHex->GoToOffset(m_deqBookmarks.at((size_t)m_iCurrent).front().ullOffset);
}

void CHexBookmarks::GoPrev()
{
	if (!m_pHex || m_deqBookmarks.empty())
		return;

	if (--m_iCurrent < 0)
		m_iCurrent = (int)m_deqBookmarks.size() - 1;

	m_pHex->GoToOffset(m_deqBookmarks.at((size_t)m_iCurrent).front().ullOffset);
}

bool CHexBookmarks::HitTest(ULONGLONG ullByte)
{
	return std::any_of(m_deqBookmarks.begin(), m_deqBookmarks.end(), [ullByte](const std::vector<HEXSPANSTRUCT>& ref)
	{return std::any_of(ref.begin(), ref.end(), [ullByte](const HEXSPANSTRUCT& refV)
	{return ullByte >= refV.ullOffset && ullByte < (refV.ullOffset + refV.ullSize); });
	});
}

bool CHexBookmarks::HasBookmarks()const
{
	return !m_deqBookmarks.empty();
}