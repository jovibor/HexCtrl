module;
/****************************************************************************************
* Copyright © 2018-present Jovibor https://github.com/jovibor/                          *
* Hex Control for Windows applications.                                                 *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include <SDKDDKVer.h>
#include "../HexCtrl.h"
#include <algorithm>
#include <cassert>
#include <vector>
export module HEXCTRL.CHexSelection;

namespace HEXCTRL::INTERNAL {
	export class CHexSelection final {
	public:
		void ClearAll();
		[[nodiscard]] auto GetSelEnd()const->ULONGLONG;
		[[nodiscard]] auto GetSelSize()const->ULONGLONG;
		[[nodiscard]] auto GetSelStart()const->ULONGLONG;
		[[nodiscard]] auto GetLineLength()const->DWORD; //Length of the selected line. Used in block selection (with Alt).
		[[nodiscard]] auto GetOffsetByIndex(ULONGLONG ullIndex)const->ULONGLONG; //Retrieves selection's offset by index [0...GetSelSize())
		[[nodiscard]] auto GetData()const->VecSpan;
		[[nodiscard]] bool HasSelection()const;
		[[nodiscard]] bool HasSelHighlight()const;
		[[nodiscard]] bool HitTest(ULONGLONG ullOffset)const;          //Is given offset within selection.
		[[nodiscard]] bool HitTestHighlight(ULONGLONG ullOffset)const; //Is given offset within highlighted selection.
		[[nodiscard]] bool HitTestRange(const HEXSPAN& hss)const;      //Is there any selection within given range.
		void SetSelection(const VecSpan& vecSel, bool fHighlight);     //Set a selection or selection highlight.
		void SetSelStartEnd(ULONGLONG ullOffset, bool fStart);         //fStart true: Start, false: End.
	private:
		VecSpan m_vecSelection;                                //Selection data vector.
		VecSpan m_vecSelHighlight;                             //Selection highlight data vector.
		ULONGLONG m_ullMarkSelStart { 0xFFFFFFFFFFFFFFFFULL }; //For SetSelStartEnd().
		ULONGLONG m_ullMarkSelEnd { 0xFFFFFFFFFFFFFFFFULL };   //For SetSelStartEnd().
	};

	void CHexSelection::ClearAll()
	{
		m_vecSelection.clear();
		m_vecSelHighlight.clear();
		m_ullMarkSelStart = 0xFFFFFFFFFFFFFFFFULL;
		m_ullMarkSelEnd = 0xFFFFFFFFFFFFFFFFULL;
	}

	auto CHexSelection::GetData()const->VecSpan
	{
		return m_vecSelection;
	}

	auto CHexSelection::GetLineLength()const->DWORD
	{
		if (!HasSelection()) {
			return 0UL;
		}

		return static_cast<DWORD>(m_vecSelection.front().ullSize);
	}

	auto CHexSelection::GetOffsetByIndex(ULONGLONG ullIndex)const->ULONGLONG
	{
		ULONGLONG ullOffset { 0xFFFFFFFFFFFFFFFFULL };
		if (ullIndex >= GetSelSize())
			return ullOffset;

		for (ULONGLONG ullTotal { }; const auto & iterData : m_vecSelection) {
			ullTotal += iterData.ullSize;
			if (ullIndex < ullTotal) {
				ullOffset = iterData.ullOffset + (ullIndex - (ullTotal - iterData.ullSize));
				break;
			}
		}
		return ullOffset;
	}

	auto CHexSelection::GetSelEnd()const->ULONGLONG
	{
		if (!HasSelection()) {
			return 0xFFFFFFFFFFFFFFFFULL;
		}

		return m_vecSelection.back().ullOffset + m_vecSelection.back().ullSize - 1;
	}

	auto CHexSelection::GetSelSize()const->ULONGLONG
	{
		if (!HasSelection()) {
			return 0ULL;
		}

		return m_vecSelection.size() * m_vecSelection.at(0).ullSize;
	}

	auto CHexSelection::GetSelStart()const->ULONGLONG
	{
		if (!HasSelection()) {
			return 0xFFFFFFFFFFFFFFFFULL;
		}

		return m_vecSelection.front().ullOffset;
	}

	bool CHexSelection::HasSelection()const
	{
		return !m_vecSelection.empty();
	}

	bool CHexSelection::HasSelHighlight() const
	{
		return !m_vecSelHighlight.empty();
	}

	bool CHexSelection::HitTest(ULONGLONG ullOffset)const
	{
		return std::any_of(m_vecSelection.begin(), m_vecSelection.end(),
			[ullOffset](const HEXSPAN& ref) { return ullOffset >= ref.ullOffset && ullOffset < (ref.ullOffset + ref.ullSize); });
	}

	bool CHexSelection::HitTestHighlight(ULONGLONG ullOffset) const
	{
		return std::any_of(m_vecSelHighlight.begin(), m_vecSelHighlight.end(),
			[ullOffset](const HEXSPAN& ref) { return ullOffset >= ref.ullOffset && ullOffset < (ref.ullOffset + ref.ullSize); });
	}

	bool CHexSelection::HitTestRange(const HEXSPAN& hss)const
	{
		return std::any_of(m_vecSelection.begin(), m_vecSelection.end(),
			[&](const HEXSPAN& ref) {
				return (hss.ullOffset >= ref.ullOffset && hss.ullOffset < (ref.ullOffset + ref.ullSize))
					|| (ref.ullOffset >= hss.ullOffset && ref.ullOffset < (hss.ullOffset + hss.ullSize))
					|| (hss.ullOffset + hss.ullSize > ref.ullOffset && hss.ullOffset + hss.ullSize <= (ref.ullOffset + ref.ullSize));
			});
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

	void CHexSelection::SetSelStartEnd(ULONGLONG ullOffset, bool fStart)
	{
		if (fStart) {
			m_ullMarkSelStart = ullOffset;
			if (m_ullMarkSelEnd == 0xFFFFFFFFFFFFFFFFULL || m_ullMarkSelStart > m_ullMarkSelEnd)
				return;
		}
		else {
			m_ullMarkSelEnd = ullOffset;
			if (m_ullMarkSelStart == 0xFFFFFFFFFFFFFFFFULL || m_ullMarkSelEnd < m_ullMarkSelStart)
				return;
		}

		m_vecSelection.clear();
		m_vecSelection.emplace_back(m_ullMarkSelStart, m_ullMarkSelEnd - m_ullMarkSelStart + 1);
	}
}