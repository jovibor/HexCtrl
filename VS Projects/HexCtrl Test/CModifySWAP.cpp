#include "CHexCtrlInit.h"
#include "CppUnitTest.h"

namespace TestHexCtrl {
	TEST_CLASS(CModifySWAP) {
public:
	TEST_METHOD(OperInt16) {
		using TestType = std::int16_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SWAP);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperUInt16) {
		using TestType = std::uint16_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SWAP);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperInt32) {
		using TestType = std::int32_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SWAP);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperUInt32) {
		using TestType = std::uint32_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SWAP);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperInt64) {
		using TestType = std::int64_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SWAP);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperUInt64) {
		using TestType = std::uint64_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SWAP);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperFloat) {
		using TestType = float;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SWAP);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperDouble) {
		using TestType = double;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SWAP);
		VerifyDataForType<TestType>();
	}

	//Big-endian.

	TEST_METHOD(OperInt16BE) {
		using TestType = std::int16_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SWAP, 0, true);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperUInt16BE) {
		using TestType = std::uint16_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SWAP, 0, true);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperInt32BE) {
		using TestType = std::int32_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SWAP, 0, true);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperUInt32BE) {
		using TestType = std::uint32_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SWAP, 0, true);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperInt64BE) {
		using TestType = std::int64_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SWAP, 0, true);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperUInt64BE) {
		using TestType = std::uint64_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SWAP, 0, true);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperFloatBE) {
		using TestType = float;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SWAP, 0, true);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperDoubleBE) {
		using TestType = double;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SWAP, 0, true);
		VerifyDataForType<TestType>();
	}
	};
}