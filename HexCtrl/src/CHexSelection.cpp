/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#include "stdafx.h"
#include "CHexSelection.h"
#include <algorithm>

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

void CHexSelection::Attach(CHexCtrl* p)
{
	m_pHex = p;
}

void CHexSelection::ClearAll()
{
	m_vecSelect.clear();
	m_ullMarkSelStart = 0xFFFFFFFFFFFFFFFFULL;
	m_ullMarkSelEnd = 0xFFFFFFFFFFFFFFFFULL;
}

CHexCtrl* CHexSelection::GetHexCtrl()const
{
	return m_pHex;
}

bool CHexSelection::HasSelection()const
{
	return !m_vecSelect.empty();
}

bool CHexSelection::HitTest(ULONGLONG ullOffset)const
{
	return std::any_of(m_vecSelect.begin(), m_vecSelect.end(),
		[ullOffset](const HEXSPANSTRUCT& ref)
		{ return ullOffset >= ref.ullOffset && ullOffset < (ref.ullOffset + ref.ullSize); });
}

bool CHexSelection::HitTestRange(const HEXSPANSTRUCT& hss)const
{
	if (!HasSelection())
		return false;

	for (auto i = 0; i < hss.ullSize; ++i)
		if (HitTest(hss.ullOffset + i))
			return true;

	return false;
}

ULONGLONG CHexSelection::GetSelectionEnd()const
{
	if (!HasSelection())
		return 0xFFFFFFFFFFFFFFFFULL;

	return m_vecSelect.back().ullOffset + m_vecSelect.back().ullSize;
}

ULONGLONG CHexSelection::GetSelectionSize()const
{
	if (!HasSelection())
		return 0;

	return 	m_vecSelect.size() * m_vecSelect.at(0).ullSize;
}

ULONGLONG CHexSelection::GetSelectionStart()const
{
	if (!HasSelection())
		return 0xFFFFFFFFFFFFFFFFULL;

	return m_vecSelect.front().ullOffset;
}

DWORD CHexSelection::GetLineLength()const
{
	if (!HasSelection())
		return 0;

	return static_cast<DWORD>(m_vecSelect.front().ullSize);
}

ULONGLONG CHexSelection::GetOffsetByIndex(ULONGLONG ullIndex)const
{
	ULONGLONG ullOffset { 0xFFFFFFFFFFFFFFFFULL };

	if (ullIndex >= GetSelectionSize())
		return ullOffset;

	ULONGLONG ullTotal { };
	for (const auto& i : m_vecSelect)
	{
		ullTotal += i.ullSize;
		if (ullIndex < ullTotal)
		{
			ullOffset = i.ullOffset + (ullIndex - (ullTotal - i.ullSize));
			break;
		}
	}
	return ullOffset;
}

auto CHexSelection::GetData()const ->std::vector<HEXSPANSTRUCT>
{
	return m_vecSelect;
}

void CHexSelection::SetSelection(const std::vector<HEXSPANSTRUCT>& vecSelect)
{
	m_vecSelect = vecSelect;
}

void CHexSelection::SetSelectionEnd(ULONGLONG ullOffset)
{
	m_ullMarkSelEnd = ullOffset;
	if (m_ullMarkSelStart == 0xFFFFFFFFFFFFFFFFULL || m_ullMarkSelEnd < m_ullMarkSelStart)
		return;

	ULONGLONG ullSize = m_ullMarkSelEnd - m_ullMarkSelStart + 1;
	m_vecSelect.clear();
	m_vecSelect.emplace_back(HEXSPANSTRUCT { m_ullMarkSelStart, ullSize });

	CHexCtrl* pHex = GetHexCtrl();
	if (pHex)
		pHex->UpdateInfoText();
}

void CHexSelection::SetSelectionStart(ULONGLONG ullOffset)
{
	m_ullMarkSelStart = ullOffset;
	if (m_ullMarkSelEnd == 0xFFFFFFFFFFFFFFFFULL || m_ullMarkSelStart > m_ullMarkSelEnd)
		return;

	ULONGLONG ullSize = m_ullMarkSelEnd - m_ullMarkSelStart + 1;
	m_vecSelect.clear();
	m_vecSelect.emplace_back(HEXSPANSTRUCT { m_ullMarkSelStart, ullSize });

	CHexCtrl* pHex = GetHexCtrl();
	if (pHex)
		pHex->UpdateInfoText();
}