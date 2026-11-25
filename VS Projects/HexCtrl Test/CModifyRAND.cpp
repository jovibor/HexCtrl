#include "CHexCtrlInit.h"
#include "CppUnitTest.h"
#include <algorithm>

namespace TestHexCtrl {
	TEST_CLASS(CModifyRAND) {
private:
	[[nodiscard]] static bool IsOffsetInSpans(std::uint64_t u64Offset, const VecHexSpan& vecSpan) {
		return std::any_of(vecSpan.begin(), vecSpan.end(), [u64Offset](const HEXSPAN& hs) {
			return u64Offset >= hs.ullOffset && u64Offset < (hs.ullOffset + hs.ullSize);
		});
	}

	static void RunRandForSpans(EHexModifyMode eMode, const VecHexSpan& vecSpan) {
		CreateRandomTestData();
		GetHexCtrl()->ModifyData({ .eModifyMode { eMode }, .vecSpan { vecSpan } });

		//1. Verify every byte outside vecSpan remained completely untouched.
		for (std::uint64_t i { 0 }; i < GetTestDataSize(); ++i) {
			if (!IsOffsetInSpans(i, vecSpan)) {
				const auto u8Ref = std::to_integer<std::uint8_t>(GetDataReference()[i]);
				const auto u8Hex = std::to_integer<std::uint8_t>(GetDataHexCtrl()[i]);
				Assert::AreEqual(u8Ref, u8Hex);
			}
		}

		//2. Verify each span was actually mutated (for spans >= 4 bytes, collision probability is 2^-32).
		for (const auto& hs : vecSpan) {
			if (hs.ullSize >= 4) {
				const bool fChanged = std::memcmp(GetDataReference() + hs.ullOffset,
					GetDataHexCtrl() + hs.ullOffset, static_cast<std::size_t>(hs.ullSize)) != 0;
				Assert::IsTrue(fChanged);
			}
		}
	}

	static void RunRandAllModes(EHexModifyMode eMode) {
		const VecHexSpan vecSingleSpan { { .ullOffset { 0 }, .ullSize { GetTestDataSize() } } };
		const auto& vecMultiSpan = GetTestMultiSpans();

		SetVirtualMode(false);
		RunRandForSpans(eMode, vecSingleSpan);
		RunRandForSpans(eMode, vecMultiSpan);

		SetVirtualMode(true);
		RunRandForSpans(eMode, vecSingleSpan);
		RunRandForSpans(eMode, vecMultiSpan);
	}

public:
	TEST_METHOD(RandMT19937) {
		RunRandAllModes(MODIFY_RAND_MT19937);
	}

	TEST_METHOD(RandFast) {
		RunRandAllModes(MODIFY_RAND_FAST);
	}
	};
}