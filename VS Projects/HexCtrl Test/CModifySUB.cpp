#include "CHexCtrlInit.h"
#include "CppUnitTest.h"

namespace TestHexCtrl {
	TEST_CLASS(CModifySUB) {
public:
	TEST_METHOD(OperInt8) {
		using TestType = std::int8_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SUB, 2);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperUInt8) {
		using TestType = std::uint8_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SUB, 2);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperInt16) {
		using TestType = std::int16_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SUB, 12);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperUInt16) {
		using TestType = std::uint16_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SUB, 12);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperInt32) {
		using TestType = std::int32_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SUB, 123);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperUInt32) {
		using TestType = std::uint32_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SUB, 123);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperInt64) {
		using TestType = std::int64_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SUB, 1234);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperUInt64) {
		using TestType = std::uint64_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SUB, 1234);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperFloat) {
		using TestType = float;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SUB, 123.8765432F);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperDouble) {
		using TestType = double;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SUB, 123.8765432);
		VerifyDataForType<TestType>();
	}

	//Big-endian.

	TEST_METHOD(OperInt16BE) {
		using TestType = std::int16_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SUB, 12, true);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperUInt16BE) {
		using TestType = std::uint16_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SUB, 12, true);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperInt32BE) {
		using TestType = std::int32_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SUB, 123, true);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperUInt32BE) {
		using TestType = std::uint32_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SUB, 123, true);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperInt64BE) {
		using TestType = std::int64_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SUB, 1234, true);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperUInt64BE) {
		using TestType = std::uint64_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SUB, 1234, true);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperFloatBE) {
		using TestType = float;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SUB, 123.8765432F, true);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperDoubleBE) {
		using TestType = double;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_SUB, 123.8765432, true);
		VerifyDataForType<TestType>();
	}
	};
}