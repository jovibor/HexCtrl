/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "CHexBookmarks.h"
#include <algorithm>
#include <cassert>
#include <numeric>

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

void CHexBookmarks::Attach(IHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	if (pHexCtrl == nullptr)
		return;

	m_pHexCtrl = pHexCtrl;
}

ULONGLONG CHexBookmarks::AddBkm(const HEXBKM& hbs, bool fRedraw)
{
	if (m_pHexCtrl == nullptr || !m_pHexCtrl->IsDataSet())
		return 0;

	ULONGLONG ullID { static_cast<ULONGLONG>(-1) };
	if (m_pVirtual) {
		ullID = m_pVirtual->AddBkm(hbs, fRedraw);
	}
	else
	{
		ullID = 1; //Bookmarks' ID starts from 1.

		if (auto iter = std::max_element(m_deqBookmarks.begin(), m_deqBookmarks.end(),
			[](const HEXBKM& ref1, const HEXBKM& ref2)
			{return ref1.ullID < ref2.ullID; }); iter != m_deqBookmarks.end())
			ullID = iter->ullID + 1; //Increasing next bookmark's ID by 1.

		m_deqBookmarks.emplace_back(hbs.vecSpan, hbs.wstrDesc, ullID, hbs.ullData, hbs.clrBk, hbs.clrText);

		if (fRedraw)
			m_pHexCtrl->Redraw();
	}

	m_time = _time64(nullptr);

	return ullID;
}

void CHexBookmarks::ClearAll()
{
	if (m_pVirtual) {
		m_pVirtual->ClearAll();
	}
	else
		m_deqBookmarks.clear();

	if (m_pHexCtrl && m_pHexCtrl->IsCreated())
		m_pHexCtrl->Redraw();

	m_time = _time64(nullptr);
}

auto CHexBookmarks::GetByID(ULONGLONG ullID)->PHEXBKM
{
	PHEXBKM pBkm { };
	if (m_pVirtual) {
		pBkm = m_pVirtual->GetByID(ullID);
	}
	else if (auto iter = std::find_if(m_deqBookmarks.begin(), m_deqBookmarks.end(),
		[ullID](const HEXBKM& ref) {return ullID == ref.ullID; }); iter != m_deqBookmarks.end())
		pBkm = &*iter;

	return pBkm;
}

auto CHexBookmarks::GetByIndex(ULONGLONG ullIndex)->PHEXBKM
{
	PHEXBKM pBkm { };
	if (m_pVirtual) {
		pBkm = m_pVirtual->GetByIndex(ullIndex);
	}
	else if (ullIndex < m_deqBookmarks.size())
		pBkm = &m_deqBookmarks[static_cast<size_t>(ullIndex)];

	return pBkm;
}

ULONGLONG CHexBookmarks::GetCount()
{
	return IsVirtual() ? m_pVirtual->GetCount() : m_deqBookmarks.size();
}

ULONGLONG CHexBookmarks::GetCurrent()const
{
	return static_cast<ULONGLONG>(m_llIndexCurr);
}

auto CHexBookmarks::GetTouchTime()const->__time64_t
{
	return m_time;
}

void CHexBookmarks::GoBookmark(ULONGLONG ullIndex)
{
	if (m_pHexCtrl == nullptr || !m_pHexCtrl->IsDataSet())
		return;

	if (const auto* const pBkm = GetByIndex(ullIndex); pBkm != nullptr)
	{
		m_llIndexCurr = static_cast<LONGLONG>(ullIndex);
		const auto ullOffset = pBkm->vecSpan.front().ullOffset;
		m_pHexCtrl->SetCaretPos(ullOffset);
		if (!m_pHexCtrl->IsOffsetVisible(ullOffset))
			m_pHexCtrl->GoToOffset(ullOffset);
	}
}

void CHexBookmarks::GoNext()
{
	if (m_pHexCtrl == nullptr || !m_pHexCtrl->IsDataSet())
		return;

	if (++m_llIndexCurr >= static_cast<LONGLONG>(GetCount()))
		m_llIndexCurr = 0;

	if (const auto* const pBkm = GetByIndex(m_llIndexCurr); pBkm != nullptr)
	{
		const auto ullOffset = pBkm->vecSpan.front().ullOffset;
		m_pHexCtrl->SetCaretPos(ullOffset);
		if (!m_pHexCtrl->IsOffsetVisible(ullOffset))
			m_pHexCtrl->GoToOffset(ullOffset);
	}
}

void CHexBookmarks::GoPrev()
{
	if (m_pHexCtrl == nullptr || !m_pHexCtrl->IsDataSet())
		return;

	if (--m_llIndexCurr; m_llIndexCurr < 0 || m_llIndexCurr >= static_cast<LONGLONG>(GetCount()))
		m_llIndexCurr = static_cast<LONGLONG>(GetCount()) - 1;

	if (const auto* const pBkm = GetByIndex(m_llIndexCurr); pBkm != nullptr)
	{
		const auto ullOffset = pBkm->vecSpan.front().ullOffset;
		m_pHexCtrl->SetCaretPos(ullOffset);
		if (!m_pHexCtrl->IsOffsetVisible(ullOffset))
			m_pHexCtrl->GoToOffset(ullOffset);
	}
}

bool CHexBookmarks::HasBookmarks()const
{
	return IsVirtual() ? m_pVirtual->GetCount() > 0 : !m_deqBookmarks.empty();
}

auto CHexBookmarks::HitTest(ULONGLONG ullOffset)->PHEXBKM
{
	PHEXBKM pBkm { };
	if (m_pVirtual) {
		pBkm = m_pVirtual->HitTest(ullOffset);
	}
	else
	{
		if (auto rIter = std::find_if(m_deqBookmarks.rbegin(), m_deqBookmarks.rend(),
			[ullOffset](const HEXBKM& ref)
			{ return std::any_of(ref.vecSpan.begin(), ref.vecSpan.end(),
				[ullOffset](const HEXSPAN& refV)
				{ return ullOffset >= refV.ullOffset && ullOffset < (refV.ullOffset + refV.ullSize); });
			}); rIter != m_deqBookmarks.rend())
			pBkm = &*rIter;
	}

	return pBkm;
}

bool CHexBookmarks::IsVirtual()const
{
	return m_pVirtual != nullptr;
}

void CHexBookmarks::RemoveByOffset(ULONGLONG ullOffset)
{
	if (m_pHexCtrl == nullptr || !m_pHexCtrl->IsDataSet())
		return;

	if (m_pVirtual) {
		if (const auto* const pBkm = m_pVirtual->HitTest(ullOffset); pBkm != nullptr)
			m_pVirtual->RemoveByID(pBkm->ullID);
	}
	else
	{
		if (m_deqBookmarks.empty())
			return;

		//Searching from the end, to remove last added bookmark if few at the given offset.
		if (auto rIter = std::find_if(m_deqBookmarks.rbegin(), m_deqBookmarks.rend(),
			[ullOffset](const HEXBKM& ref)
			{return std::any_of(ref.vecSpan.begin(), ref.vecSpan.end(),
				[ullOffset](const HEXSPAN& refV)
				{return ullOffset >= refV.ullOffset && ullOffset < (refV.ullOffset + refV.ullSize); });
			}); rIter != m_deqBookmarks.rend())
			m_deqBookmarks.erase((rIter + 1).base()); //Weird notation for reverse_iterator to work in erase() (acc to standard).
	}

	m_pHexCtrl->Redraw();
	m_time = _time64(nullptr);
}

void CHexBookmarks::RemoveByID(ULONGLONG ullID)
{
	if (m_pHexCtrl == nullptr || !m_pHexCtrl->IsDataSet())
		return;

	if (m_pVirtual) {
		m_pVirtual->RemoveByID(ullID);
	}
	else
	{
		if (m_deqBookmarks.empty())
			return;

		if (auto iter = std::find_if(m_deqBookmarks.begin(), m_deqBookmarks.end(),
			[ullID](const HEXBKM& ref) {return ullID == ref.ullID; }); iter != m_deqBookmarks.end())
			m_deqBookmarks.erase(iter);
	}

	m_pHexCtrl->Redraw();
	m_time = _time64(nullptr);
}

void CHexBookmarks::SetVirtual(IHexBookmarks* pVirtBkm)
{
	assert(pVirtBkm != this);
	if (pVirtBkm == this) //To avoid setting self as a Virtual Bookmarks handler.
		return;

	m_pVirtual = pVirtBkm;
	m_time = _time64(nullptr);
}

void CHexBookmarks::SortData(int iColumn, bool fAscending)
{
	//iColumn is column number in CHexDlgBkmMgr::m_pListMain.
	std::sort(m_deqBookmarks.begin(), m_deqBookmarks.end(),
		[iColumn, fAscending](const HEXBKM& st1, const HEXBKM& st2)
		{
			int iCompare { };
			switch (iColumn)
			{
			case 0:
				break;
			case 1: //Offset.
				if (!st1.vecSpan.empty() && !st2.vecSpan.empty())
				{
					const auto ullOffset1 = st1.vecSpan.front().ullOffset;
					const auto ullOffset2 = st2.vecSpan.front().ullOffset;
					iCompare = ullOffset1 != ullOffset2 ? (ullOffset1 < ullOffset2 ? -1 : 1) : 0;
				}
				break;
			case 2: //Size.
				if (!st1.vecSpan.empty() && !st2.vecSpan.empty())
				{
					auto ullSize1 = std::accumulate(st1.vecSpan.begin(), st1.vecSpan.end(), 0ULL,
						[](auto ullTotal, const HEXSPAN& ref) {return ullTotal + ref.ullSize; });
					auto ullSize2 = std::accumulate(st2.vecSpan.begin(), st2.vecSpan.end(), 0ULL,
						[](auto ullTotal, const HEXSPAN& ref) {return ullTotal + ref.ullSize; });
					iCompare = ullSize1 != ullSize2 ? (ullSize1 < ullSize2 ? -1 : 1) : 0;
				}
				break;
			case 3: //Description.
				iCompare = st1.wstrDesc.compare(st2.wstrDesc);
				break;
			default:
				break;
			}

			return fAscending ? iCompare < 0 : iCompare > 0;
		});
}

void CHexBookmarks::Update(ULONGLONG ullID, const HEXBKM& stBookmark)
{
	if (m_pHexCtrl == nullptr || !m_pHexCtrl->IsDataSet())
		return;

	if (IsVirtual() || m_deqBookmarks.empty())
		return;

	auto iter = std::find_if(m_deqBookmarks.begin(), m_deqBookmarks.end(),
		[ullID](const HEXBKM& ref) {return ullID == ref.ullID; });

	if (iter != m_deqBookmarks.end())
		*iter = stBookmark;

	m_pHexCtrl->Redraw();
	m_time = _time64(nullptr);
}