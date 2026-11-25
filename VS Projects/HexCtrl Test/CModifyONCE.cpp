#include "CHexCtrlInit.h"
#include "CppUnitTest.h"
#include <array>

namespace TestHexCtrl {
	TEST_CLASS(CModifyONCE) {
public:
	TEST_METHOD(SingleSpan) {
		SetVirtualMode(false);
		CreateRandomTestData();

		constexpr std::array<std::byte, 17> arrPattern {
			std::byte { 0x11 }, std::byte { 0x22 }, std::byte { 0x33 }, std::byte { 0x44 },
			std::byte { 0x55 }, std::byte { 0x66 }, std::byte { 0x77 }, std::byte { 0x88 },
			std::byte { 0x99 }, std::byte { 0xAA }, std::byte { 0xBB }, std::byte { 0xCC },
			std::byte { 0xDD }, std::byte { 0xEE }, std::byte { 0xFF }, std::byte { 0x12 },
			std::byte { 0x34 }
		};

		constexpr HEXSPAN hs { .ullOffset { 123 }, .ullSize { arrPattern.size() } };
		GetHexCtrl()->ModifyData({ .eModifyMode { MODIFY_ONCE }, .spnData { arrPattern }, .vecSpan { { hs } } });
		std::memcpy(GetDataReference() + hs.ullOffset, arrPattern.data(), arrPattern.size());
		CompareHexCtrlAndRefData<std::uint8_t>();
	}

	TEST_METHOD(MultiSpanAndClamping) {
		SetVirtualMode(false);
		CreateRandomTestData();

		std::array<std::byte, 64> arrPattern { };
		for (std::size_t i { 0 }; i < arrPattern.size(); ++i) {
			arrPattern[i] = static_cast<std::byte>(0xA0 + (i & 0x1F));
		}

		const auto& vecSpan = GetTestMultiSpans();
		GetHexCtrl()->ModifyData({ .eModifyMode { MODIFY_ONCE }, .spnData { arrPattern }, .vecSpan { vecSpan } });

		for (const auto& hs : vecSpan) {
			const auto u64ToCopy = (std::min)(hs.ullSize, static_cast<std::uint64_t>(arrPattern.size()));
			std::memcpy(GetDataReference() + hs.ullOffset, arrPattern.data(), static_cast<std::size_t>(u64ToCopy));
		}

		CompareHexCtrlAndRefData<std::uint8_t>();
	}

	TEST_METHOD(VirtualCrossCacheBoundary) {
		SetVirtualMode(true);
		CreateRandomTestData();

		//Pattern larger than 64 KB to cross virtual cache boundary in a single MODIFY_ONCE call.
		std::vector<std::byte> vecPattern(67'000);
		for (std::size_t i { 0 }; i < vecPattern.size(); ++i) {
			vecPattern[i] = static_cast<std::byte>((i * 7 + 3) & 0xFF);
		}

		constexpr HEXSPAN hs { .ullOffset { 1'500 }, .ullSize { 67'000 } };
		GetHexCtrl()->ModifyData({ .eModifyMode { MODIFY_ONCE }, .spnData { vecPattern }, .vecSpan { { hs } } });
		std::memcpy(GetDataReference() + hs.ullOffset, vecPattern.data(), vecPattern.size());
		CompareHexCtrlAndRefData<std::uint8_t>();
		SetVirtualMode(false);
	}
	};
}