/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/                         *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#include "stdafx.h"
#include "CHexSelect.h"
#include <algorithm>

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

void CHexSelect::Attach(CHexCtrl* p)
{
	m_pHex = p;
}

CHexCtrl* CHexSelect::GetHex()
{
	return m_pHex;
}

bool CHexSelect::HasSelection()const
{
	return !m_vecSelect.empty();
}

bool CHexSelect::HitTest(ULONGLONG ullIndex)const
{
	return std::any_of(m_vecSelect.begin(), m_vecSelect.end(),
		[ullIndex](const HEXSPANSTRUCT& ref) { return ullIndex >= ref.ullOffset && ullIndex < (ref.ullOffset + ref.ullSize); });
}

void CHexSelect::SetSelection(const std::vector<HEXSPANSTRUCT>& vecSelect)
{
	m_vecSelect = vecSelect;
}

void CHexSelect::SetSelectionStart(ULONGLONG ullOffset)
{
	m_ullMarkSelStart = ullOffset;
	if (m_ullMarkSelEnd == 0xFFFFFFFFFFFFFFFFULL || m_ullMarkSelStart > m_ullMarkSelEnd)
		return;
	else
	{
		ULONGLONG ullSize = m_ullMarkSelEnd - m_ullMarkSelStart + 1;
		m_vecSelect.clear();
		m_vecSelect.emplace_back(HEXSPANSTRUCT { m_ullMarkSelStart, ullSize });

		CHexCtrl* pHex = GetHex();
		if (pHex)
			pHex->UpdateInfoText();
	}
}

void CHexSelect::SetSelectionEnd(ULONGLONG ullOffset)
{
	m_ullMarkSelEnd = ullOffset;
	if (m_ullMarkSelStart == 0xFFFFFFFFFFFFFFFFULL || m_ullMarkSelEnd < m_ullMarkSelStart)
		return;
	else
	{
		ULONGLONG ullSize = m_ullMarkSelEnd - m_ullMarkSelStart + 1;
		m_vecSelect.clear();
		m_vecSelect.emplace_back(HEXSPANSTRUCT { m_ullMarkSelStart, ullSize });

		CHexCtrl* pHex = GetHex();
		if (pHex)
			pHex->UpdateInfoText();
	}
}

ULONGLONG CHexSelect::GetSelectionSize()const
{
	if (!HasSelection())
		return 0;

	return 	m_vecSelect.size() * m_vecSelect.at(0).ullSize;
}

ULONGLONG CHexSelect::GetSelectionStart()const
{
	if (!HasSelection())
		return 0xFFFFFFFFFFFFFFFFul;

	return m_vecSelect.front().ullOffset;
}

ULONGLONG CHexSelect::GetSelectionEnd()const
{
	if (!HasSelection())
		return 0xFFFFFFFFFFFFFFFFul;

	return m_vecSelect.back().ullOffset + m_vecSelect.back().ullSize;
}

ULONGLONG CHexSelect::GetOffsetByIndex(ULONGLONG ullIndex)const
{
	ULONGLONG ullOffset { 0xFFFFFFFFFFFFFFFFull };

	if (ullIndex >= GetSelectionSize())
		return ullOffset;

	ULONGLONG ullTotal { };
	for (auto& i : m_vecSelect)
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

DWORD CHexSelect::GetLineLength()const
{
	if (!HasSelection())
		return 0;

	return (DWORD)m_vecSelect.front().ullSize;
}

std::vector<HEXSPANSTRUCT>& CHexSelect::GetVector()
{
	return m_vecSelect;
}

void CHexSelect::ClearAll()
{
	m_vecSelect.clear();
	m_ullMarkSelStart = 0xFFFFFFFFFFFFFFFFULL;
	m_ullMarkSelEnd = 0xFFFFFFFFFFFFFFFFULL;
}