/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
module;
#include <SDKDDKVer.h>
#include "../HexCtrl.h"
#include <algorithm>
#include <vector>
export module HEXCTRL:CHexSelection;

namespace HEXCTRL::INTERNAL {
	class CHexSelection final {
	public:
		void ClearAll();
		[[nodiscard]] auto GetData()const->VecSpan;
		[[nodiscard]] auto GetLineLength()const->DWORD; //Length of the selected line. Used in block selection (with Alt).
		[[nodiscard]] auto GetOffsetByIndex(ULONGLONG ullIndex)const->ULONGLONG; //Retrieves selection's offset by index [0...GetSelSize())
		[[nodiscard]] auto GetSelEnd()const->ULONGLONG;
		[[nodiscard]] auto GetSelSize()const->ULONGLONG;
		[[nodiscard]] auto GetSelStart()const->ULONGLONG;
		[[nodiscard]] bool HasSelection()const;
		[[nodiscard]] bool HasSelHighlight()const;
		[[nodiscard]] bool HasContiguousSel()const; //Has contiguous selection, not multiline with Alt.
		[[nodiscard]] bool HitTest(ULONGLONG ullOffset)const;          //Is given offset within selection.
		[[nodiscard]] bool HitTestHighlight(ULONGLONG ullOffset)const; //Is given offset within highlighted selection.
		[[nodiscard]] bool HitTestRange(const HEXSPAN& hss)const;      //Is there any selection within given range.
		void SetMarkStartEnd(ULONGLONG ullOffset);
		void SetSelection(const VecSpan& vecSel, bool fHighlight);     //Set a selection or selection highlight.
	private:
		VecSpan m_vecSelection;    //Selection data.
		VecSpan m_vecSelHighlight; //Selection highlight data.
		ULONGLONG m_ullMarkStartEnd { (std::numeric_limits<std::uint64_t>::max)() };
	};
}

using namespace HEXCTRL::INTERNAL;

void CHexSelection::ClearAll()
{
	m_vecSelection.clear();
	m_vecSelHighlight.clear();
	m_ullMarkStartEnd = (std::numeric_limits<std::uint64_t>::max)();
}

auto CHexSelection::GetData()const->VecSpan
{
	return m_vecSelection;
}

auto CHexSelection::GetLineLength()const->DWORD
{
	if (!HasSelection()) {
		return { };
	}

	return static_cast<DWORD>(m_vecSelection.front().ullSize);
}

auto CHexSelection::GetOffsetByIndex(ULONGLONG ullIndex)const->ULONGLONG
{
	ULONGLONG ullOffset { };
	if (ullIndex >= GetSelSize())
		return ullOffset;

	for (ULONGLONG ullTotal { }; const auto & ref : m_vecSelection) {
		ullTotal += ref.ullSize;
		if (ullIndex < ullTotal) {
			ullOffset = ref.ullOffset + (ullIndex - (ullTotal - ref.ullSize));
			break;
		}
	}

	return ullOffset;
}

auto CHexSelection::GetSelEnd()const->ULONGLONG
{
	if (!HasSelection()) {
		return { };
	}

	const auto& span = m_vecSelection.back();
	return span.ullOffset + span.ullSize - 1;
}

auto CHexSelection::GetSelSize()const->ULONGLONG
{
	if (!HasSelection()) {
		return { };
	}

	return m_vecSelection.size() * m_vecSelection[0].ullSize;
}

auto CHexSelection::GetSelStart()const->ULONGLONG
{
	if (!HasSelection()) {
		return { };
	}

	return m_vecSelection.front().ullOffset;
}

bool CHexSelection::HasSelection()const
{
	return !m_vecSelection.empty();
}

bool CHexSelection::HasSelHighlight()const
{
	return !m_vecSelHighlight.empty();
}

bool CHexSelection::HasContiguousSel()const
{
	return m_vecSelection.size() == 1;
}

bool CHexSelection::HitTest(ULONGLONG ullOffset)const
{
	return std::any_of(m_vecSelection.begin(), m_vecSelection.end(),
		[ullOffset](const HEXSPAN& ref) {
			return ullOffset >= ref.ullOffset && ullOffset < (ref.ullOffset + ref.ullSize); });
}

bool CHexSelection::HitTestHighlight(ULONGLONG ullOffset)const
{
	return std::any_of(m_vecSelHighlight.begin(), m_vecSelHighlight.end(),
		[ullOffset](const HEXSPAN& ref) {
			return ullOffset >= ref.ullOffset && ullOffset < (ref.ullOffset + ref.ullSize); });
}

bool CHexSelection::HitTestRange(const HEXSPAN& hss)const
{
	return std::any_of(m_vecSelection.begin(), m_vecSelection.end(),
		[&](const HEXSPAN& ref) {
			return (hss.ullOffset >= ref.ullOffset && hss.ullOffset < (ref.ullOffset + ref.ullSize))
				|| (ref.ullOffset >= hss.ullOffset && ref.ullOffset < (hss.ullOffset + hss.ullSize))
				|| (hss.ullOffset + hss.ullSize > ref.ullOffset && hss.ullOffset
					+ hss.ullSize <= (ref.ullOffset + ref.ullSize));
		});
}

void CHexSelection::SetMarkStartEnd(ULONGLONG ullOffset)
{
	if (m_ullMarkStartEnd == (std::numeric_limits<std::uint64_t>::max)()) {
		m_ullMarkStartEnd = ullOffset; //Setting selection first mark.
		return;
	}

	const auto ullStart = (std::min)(m_ullMarkStartEnd, ullOffset);
	const auto ullSize = (std::max)(m_ullMarkStartEnd, ullOffset) - ullStart + 1;
	SetSelection({ { .ullOffset { ullStart }, .ullSize { ullSize } } }, false);
	m_ullMarkStartEnd = (std::numeric_limits<std::uint64_t>::max)(); //Reset back to default.
}

void CHexSelection::SetSelection(const VecSpan& vecSel, bool fHighlight)
{
	if (fHighlight) {
		m_vecSelHighlight = vecSel;
	}
	else {
		m_vecSelHighlight.clear(); //On new selection clear all highlights.
		m_vecSelection = vecSel;
	}
}