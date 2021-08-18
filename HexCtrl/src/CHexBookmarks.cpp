/****************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
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

ULONGLONG CHexBookmarks::Add(const HEXBKM& hbs, bool fRedraw)
{
	if (!m_pHexCtrl || !m_pHexCtrl->IsDataSet())
		return 0;

	ULONGLONG ullID { static_cast<ULONGLONG>(-1) };
	if (m_fVirtual)
	{
		if (m_pVirtual)
			ullID = m_pVirtual->OnHexBkmAdd(hbs);
	}
	else
	{
		ullID = 1; //Bookmarks' ID starts from 1.

		if (auto iter = std::max_element(m_deqBookmarks.begin(), m_deqBookmarks.end(),
			[](const HEXBKM& ref1, const HEXBKM& ref2)
			{return ref1.ullID < ref2.ullID; }); iter != m_deqBookmarks.end())
			ullID = iter->ullID + 1; //Increasing next bookmark's ID by 1.

		m_deqBookmarks.emplace_back(
			HEXBKM { hbs.vecOffset, hbs.wstrDesc, ullID, hbs.ullData, hbs.clrBk, hbs.clrText });
	}

	if (fRedraw && m_pHexCtrl)
		m_pHexCtrl->Redraw();
	m_time = _time64(nullptr);

	return ullID;
}

void CHexBookmarks::ClearAll()
{
	if (m_fVirtual)
	{
		if (m_pVirtual)
			m_pVirtual->OnHexBkmClearAll();
	}
	else
		m_deqBookmarks.clear();

	if (m_pHexCtrl)
		m_pHexCtrl->Redraw();
	m_time = _time64(nullptr);
}

auto CHexBookmarks::GetByID(ULONGLONG ullID)->HEXBKM*
{
	HEXBKM* pBkm { };
	if (m_fVirtual)
	{
		if (m_pVirtual)
			pBkm = m_pVirtual->OnHexBkmGetByID(ullID);
	}
	else if (auto iter = std::find_if(m_deqBookmarks.begin(), m_deqBookmarks.end(),
		[ullID](const HEXBKM& ref) {return ullID == ref.ullID; }); iter != m_deqBookmarks.end())
		pBkm = &*iter;

	return pBkm;
}

auto CHexBookmarks::GetByIndex(ULONGLONG ullIndex)->HEXBKM*
{
	HEXBKM* pBkm { };
	if (m_fVirtual)
	{
		if (m_pVirtual)
			pBkm = m_pVirtual->OnHexBkmGetByIndex(ullIndex);
	}
	else if (ullIndex < m_deqBookmarks.size())
		pBkm = &m_deqBookmarks[static_cast<size_t>(ullIndex)];

	return pBkm;
}

ULONGLONG CHexBookmarks::GetCount()const
{
	ULONGLONG ullCount { };
	if (m_fVirtual)
	{
		if (m_pVirtual)
			ullCount = m_pVirtual->OnHexBkmGetCount();
	}
	else
		ullCount = m_deqBookmarks.size();

	return ullCount;
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
	if (!m_pHexCtrl)
		return;

	if (const auto* const pBkm = GetByIndex(ullIndex); pBkm != nullptr)
	{
		m_llIndexCurr = static_cast<LONGLONG>(ullIndex);
		const auto ullOffset = pBkm->vecOffset.front().ullOffset;
		m_pHexCtrl->SetCaretPos(ullOffset);
		if (!m_pHexCtrl->IsOffsetVisible(ullOffset))
			m_pHexCtrl->GoToOffset(ullOffset);
	}
}

void CHexBookmarks::GoNext()
{
	if (!m_pHexCtrl)
		return;

	if (++m_llIndexCurr >= static_cast<LONGLONG>(GetCount()))
		m_llIndexCurr = 0;

	if (const auto* const pBkm = GetByIndex(m_llIndexCurr); pBkm != nullptr)
	{
		const auto ullOffset = pBkm->vecOffset.front().ullOffset;
		m_pHexCtrl->SetCaretPos(ullOffset);
		if (!m_pHexCtrl->IsOffsetVisible(ullOffset))
			m_pHexCtrl->GoToOffset(ullOffset);
	}
}

void CHexBookmarks::GoPrev()
{
	if (!m_pHexCtrl)
		return;

	if (--m_llIndexCurr; m_llIndexCurr < 0 || m_llIndexCurr >= static_cast<LONGLONG>(GetCount()))
		m_llIndexCurr = static_cast<LONGLONG>(GetCount()) - 1;

	if (const auto* const pBkm = GetByIndex(m_llIndexCurr); pBkm != nullptr)
	{
		const auto ullOffset = pBkm->vecOffset.front().ullOffset;
		m_pHexCtrl->SetCaretPos(ullOffset);
		if (!m_pHexCtrl->IsOffsetVisible(ullOffset))
			m_pHexCtrl->GoToOffset(ullOffset);
	}
}

bool CHexBookmarks::HasBookmarks()const
{
	bool fHasBookmarks { false };
	if (m_fVirtual)
	{
		if (m_pVirtual)
			fHasBookmarks = m_pVirtual->OnHexBkmGetCount() > 0;
	}
	else
		fHasBookmarks = !m_deqBookmarks.empty();

	return fHasBookmarks;
}

auto CHexBookmarks::HitTest(ULONGLONG ullOffset)->HEXBKM*
{
	HEXBKM* pBkm { };
	if (m_fVirtual)
	{
		if (m_pVirtual)
			pBkm = m_pVirtual->OnHexBkmHitTest(ullOffset);
	}
	else
	{
		if (auto rIter = std::find_if(m_deqBookmarks.rbegin(), m_deqBookmarks.rend(),
			[ullOffset](const HEXBKM& ref)
			{ return std::any_of(ref.vecOffset.begin(), ref.vecOffset.end(),
				[ullOffset](const HEXOFFSET& refV)
				{ return ullOffset >= refV.ullOffset && ullOffset < (refV.ullOffset + refV.ullSize); });
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
	if (!m_pHexCtrl || !m_pHexCtrl->IsDataSet())
		return;

	if (m_fVirtual)
	{
		if (m_pVirtual)
		{
			if (const auto* const pBkm = m_pVirtual->OnHexBkmHitTest(ullOffset); pBkm != nullptr)
				m_pVirtual->OnHexBkmRemoveByID(pBkm->ullID);
		}
	}
	else
	{
		if (m_deqBookmarks.empty())
			return;

		//Searching from the end, to remove last added bookmark if few at the given offset.
		if (auto rIter = std::find_if(m_deqBookmarks.rbegin(), m_deqBookmarks.rend(),
			[ullOffset](const HEXBKM& ref)
			{return std::any_of(ref.vecOffset.begin(), ref.vecOffset.end(),
				[ullOffset](const HEXOFFSET& refV)
				{return ullOffset >= refV.ullOffset && ullOffset < (refV.ullOffset + refV.ullSize); });
			}); rIter != m_deqBookmarks.rend())
			m_deqBookmarks.erase((rIter + 1).base()); //Weird notation for reverse_iterator to work in erase() (acc to standard).
	}

	if (m_pHexCtrl)
		m_pHexCtrl->Redraw();
	m_time = _time64(nullptr);
}

void CHexBookmarks::RemoveByID(ULONGLONG ullID)
{
	if (m_fVirtual)
	{
		if (m_pVirtual)
			m_pVirtual->OnHexBkmRemoveByID(ullID);
	}
	else
	{
		if (m_deqBookmarks.empty())
			return;

		if (auto iter = std::find_if(m_deqBookmarks.begin(), m_deqBookmarks.end(),
			[ullID](const HEXBKM& ref) {return ullID == ref.ullID; }); iter != m_deqBookmarks.end())
			m_deqBookmarks.erase(iter);
	}

	if (m_pHexCtrl)
		m_pHexCtrl->Redraw();
	m_time = _time64(nullptr);
}

void CHexBookmarks::SetVirtual(bool fEnable, IHexVirtBkm* pVirtual)
{
	m_fVirtual = fEnable;
	if (fEnable && pVirtual != nullptr)
		m_pVirtual = pVirtual;

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
				if (!st1.vecOffset.empty() && !st2.vecOffset.empty())
				{
					const auto ullOffset1 = st1.vecOffset.front().ullOffset;
					const auto ullOffset2 = st2.vecOffset.front().ullOffset;
					iCompare = ullOffset1 != ullOffset2 ? (ullOffset1 < ullOffset2 ? -1 : 1) : 0;
				}
				break;
			case 2: //Size.
				if (!st1.vecOffset.empty() && !st2.vecOffset.empty())
				{
					auto ullSize1 = std::accumulate(st1.vecOffset.begin(), st1.vecOffset.end(), 0ULL,
						[](auto ullTotal, const HEXOFFSET& ref) {return ullTotal + ref.ullSize; });
					auto ullSize2 = std::accumulate(st2.vecOffset.begin(), st2.vecOffset.end(), 0ULL,
						[](auto ullTotal, const HEXOFFSET& ref) {return ullTotal + ref.ullSize; });
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
	if (m_fVirtual || m_deqBookmarks.empty())
		return;

	auto iter = std::find_if(m_deqBookmarks.begin(), m_deqBookmarks.end(),
		[ullID](const HEXBKM& ref) {return ullID == ref.ullID; });

	if (iter != m_deqBookmarks.end())
		*iter = stBookmark;

	if (m_pHexCtrl)
		m_pHexCtrl->Redraw();
	m_time = _time64(nullptr);
}