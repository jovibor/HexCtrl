#pragma once
#include "CHexCtrl.h"
#include <vector>

namespace HEXCTRL {
	namespace INTERNAL
	{
		struct BOOKMARKSTRUCT
		{
			ULONGLONG ullOffset;
			ULONGLONG ullSize;
		};

		class CHexBookmarks
		{
		public:
			CHexBookmarks() {}
			void Attach(CHexCtrl* pHex);
			void Add();
			void Remove();
			void ClearAll();
			void GoNext();
			void GoPrev();
			bool HitTest(ULONGLONG ullByte);
			bool HasBookmarks();
		private:
			std::vector<BOOKMARKSTRUCT> m_vecBookmarks;
			CHexCtrl* m_pHex { };
			int m_iCurrent { };
		};
	}
}