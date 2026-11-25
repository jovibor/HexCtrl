#include "CHexCtrlInit.h"
#include "CppUnitTest.h"

namespace TestHexCtrl {
	TEST_CLASS(CModifyOPERATION) {
private:
	template<typename T>
	static void RunOperBothSpans(EHexOperMode eOperMode, T tOperData = { }, bool fBigEndian = false) {
		//1. Single contiguous span across the entire 70,003-byte buffer.
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<T>(eOperMode, tOperData, fBigEndian);
		CompareHexCtrlAndRefData<T>();

		//2. Multi-span (unaligned, tiny, scalar, SIMD, and cross-64KB-cache-boundary spans).
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<T>(eOperMode, GetTestMultiSpans(), tOperData, fBigEndian);
		CompareHexCtrlAndRefData<T>();
	}

	template<EHexOperMode eOperMode>
	static void RunIntTypesForCurrentMode(std::uint64_t u64OperVal) {
		//Little-endian (all 8 integer types).
		RunOperBothSpans<std::int8_t>(eOperMode, static_cast<std::int8_t>(u64OperVal));
		RunOperBothSpans<std::uint8_t>(eOperMode, static_cast<std::uint8_t>(u64OperVal));
		RunOperBothSpans<std::int16_t>(eOperMode, static_cast<std::int16_t>(u64OperVal));
		RunOperBothSpans<std::uint16_t>(eOperMode, static_cast<std::uint16_t>(u64OperVal));
		RunOperBothSpans<std::int32_t>(eOperMode, static_cast<std::int32_t>(u64OperVal));
		RunOperBothSpans<std::uint32_t>(eOperMode, static_cast<std::uint32_t>(u64OperVal));
		RunOperBothSpans<std::int64_t>(eOperMode, static_cast<std::int64_t>(u64OperVal));
		RunOperBothSpans<std::uint64_t>(eOperMode, u64OperVal);

		//Big-endian (16, 32, and 64-bit integer types).
		RunOperBothSpans<std::int16_t>(eOperMode, static_cast<std::int16_t>(u64OperVal), true);
		RunOperBothSpans<std::uint16_t>(eOperMode, static_cast<std::uint16_t>(u64OperVal), true);
		RunOperBothSpans<std::int32_t>(eOperMode, static_cast<std::int32_t>(u64OperVal), true);
		RunOperBothSpans<std::uint32_t>(eOperMode, static_cast<std::uint32_t>(u64OperVal), true);
		RunOperBothSpans<std::int64_t>(eOperMode, static_cast<std::int64_t>(u64OperVal), true);
		RunOperBothSpans<std::uint64_t>(eOperMode, u64OperVal, true);
	}

	template<EHexOperMode eOperMode>
	static void RunAllTypesForCurrentMode(std::uint64_t u64OperVal, double dOperVal) {
		RunIntTypesForCurrentMode<eOperMode>(u64OperVal);

		//Floating-point types (Little-endian and Big-endian).
		RunOperBothSpans<float>(eOperMode, static_cast<float>(dOperVal));
		RunOperBothSpans<double>(eOperMode, dOperVal);
		RunOperBothSpans<float>(eOperMode, static_cast<float>(dOperVal), true);
		RunOperBothSpans<double>(eOperMode, dOperVal, true);
	}

	//Runs an integral-only operation in both Standard and Virtual Data modes.
	template<EHexOperMode eOperMode>
	static void RunIntOperAllModes(std::uint64_t u64OperVal = 0) {
		SetVirtualMode(false);
		RunIntTypesForCurrentMode<eOperMode>(u64OperVal);

		SetVirtualMode(true);
		RunIntTypesForCurrentMode<eOperMode>(u64OperVal);
	}

	//Runs an arithmetic/assign/clamp operation across all 10 types in both Standard and Virtual Data modes.
	template<EHexOperMode eOperMode>
	static void RunAllTypesOperAllModes(std::uint64_t u64OperVal, double dOperVal) {
		SetVirtualMode(false);
		RunAllTypesForCurrentMode<eOperMode>(u64OperVal, dOperVal);

		SetVirtualMode(true);
		RunAllTypesForCurrentMode<eOperMode>(u64OperVal, dOperVal);
	}

public:
	TEST_METHOD(OperAssign) { RunAllTypesOperAllModes<OPER_ASSIGN>(0x123456789ABCDEF0ULL, 123.8765432); }
	TEST_METHOD(OperAdd) { RunAllTypesOperAllModes<OPER_ADD>(12, 12.8765432); }
	TEST_METHOD(OperSub) { RunAllTypesOperAllModes<OPER_SUB>(12, 12.8765432); }
	TEST_METHOD(OperMul) { RunAllTypesOperAllModes<OPER_MUL>(3, 1.5); }
	TEST_METHOD(OperDiv) { RunAllTypesOperAllModes<OPER_DIV>(3, 3.25); }
	TEST_METHOD(OperMin) { RunAllTypesOperAllModes<OPER_MIN>(12, 123.8765432); }
	TEST_METHOD(OperMax) { RunAllTypesOperAllModes<OPER_MAX>(12, 123.8765432); }

	TEST_METHOD(OperOr) { RunIntOperAllModes<OPER_OR>(0x5A); }
	TEST_METHOD(OperXor) { RunIntOperAllModes<OPER_XOR>(0x5A); }
	TEST_METHOD(OperAnd) { RunIntOperAllModes<OPER_AND>(0x5A); }
	TEST_METHOD(OperNot) { RunIntOperAllModes<OPER_NOT>(); }
	TEST_METHOD(OperShl) { RunIntOperAllModes<OPER_SHL>(3); }
	TEST_METHOD(OperShr) { RunIntOperAllModes<OPER_SHR>(3); }
	TEST_METHOD(OperRotl) { RunIntOperAllModes<OPER_ROTL>(3); }
	TEST_METHOD(OperRotr) { RunIntOperAllModes<OPER_ROTR>(3); }
	TEST_METHOD(OperBitRev) { RunIntOperAllModes<OPER_BITREV>(); }

	TEST_METHOD(OperSwap) {
		const auto lmbSwapTypes = []() {
			RunOperBothSpans<std::int16_t>(OPER_SWAP);
			RunOperBothSpans<std::uint16_t>(OPER_SWAP);
			RunOperBothSpans<std::int32_t>(OPER_SWAP);
			RunOperBothSpans<std::uint32_t>(OPER_SWAP);
			RunOperBothSpans<std::int64_t>(OPER_SWAP);
			RunOperBothSpans<std::uint64_t>(OPER_SWAP);
			};

		SetVirtualMode(false);
		lmbSwapTypes();

		SetVirtualMode(true);
		lmbSwapTypes();
	}
	};
}
