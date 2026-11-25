#include "CHexCtrlInit.h"
#include "CppUnitTest.h"
#include <vector>

namespace TestHexCtrl {
	TEST_CLASS(CModifyREPEAT) {
private:
	static void RunRepeatForSpans(std::size_t uzPatternSize, const VecHexSpan& vecSpan) {
		CreateRandomTestData();

		std::vector<std::byte> vecPattern(uzPatternSize);
		for (std::size_t i { 0 }; i < uzPatternSize; ++i) {
			vecPattern[i] = static_cast<std::byte>((i * 13 + 0x37) & 0xFF);
		}

		GetHexCtrl()->ModifyData({ .eModifyMode { MODIFY_REPEAT }, .spnData { vecPattern }, .vecSpan { vecSpan } });

		for (const auto& hs : vecSpan) {
			const auto u64Repeats = hs.ullSize / uzPatternSize;
			for (std::uint64_t r { 0 }; r < u64Repeats; ++r) {
				std::memcpy(GetDataReference() + hs.ullOffset + (r * uzPatternSize), vecPattern.data(), uzPatternSize);
			}
		}

		CompareHexCtrlAndRefData<std::uint8_t>();
	}

	static void RunRepeatAllModes(std::initializer_list<std::size_t> listSizes) {
		const VecHexSpan vecSingleSpan { { .ullOffset { 0 }, .ullSize { GetTestDataSize() } } };
		const auto& vecMultiSpan = GetTestMultiSpans();

		SetVirtualMode(false);
		for (const auto uzSize : listSizes) {
			RunRepeatForSpans(uzSize, vecSingleSpan);
			RunRepeatForSpans(uzSize, vecMultiSpan);
		}

		SetVirtualMode(true);
		for (const auto uzSize : listSizes) {
			RunRepeatForSpans(uzSize, vecSingleSpan);
			RunRepeatForSpans(uzSize, vecMultiSpan);
		}
	}

public:
	//Fast 256-byte tiling buffer path (256 % pattern.size() == 0 and pattern.size() < 256).
	TEST_METHOD(FastTilePowerOfTwo) {
		RunRepeatAllModes({ 1ULL, 8ULL, 64ULL });
	}

	//Unaligned / non-divisor pattern sizes (bypasses 256-byte tiling buffer).
	TEST_METHOD(UnalignedPatternSizes) {
		RunRepeatAllModes({ 3ULL, 13ULL });
	}

	//Patterns >= 256 bytes (exact 256-byte boundary and larger than 256 bytes).
	TEST_METHOD(LargePatternSizes) {
		RunRepeatAllModes({ 256ULL, 300ULL });
	}
	};
}
