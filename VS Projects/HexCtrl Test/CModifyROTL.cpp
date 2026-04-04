#include "CHexCtrlInit.h"
#include "CppUnitTest.h"

namespace TestHexCtrl {
	TEST_CLASS(CModifyROTL) {
public:
	TEST_METHOD(OperInt8) {
		using TestType = std::int8_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_ROTL, 1);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperUInt8) {
		using TestType = std::uint8_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_ROTL, 1);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperInt16) {
		using TestType = std::int16_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_ROTL, 1);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperUInt16) {
		using TestType = std::uint16_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_ROTL, 1);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperInt32) {
		using TestType = std::int32_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_ROTL, 1);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperUInt32) {
		using TestType = std::uint32_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_ROTL, 1);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperInt64) {
		using TestType = std::int64_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_ROTL, 1);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperUInt64) {
		using TestType = std::uint64_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_ROTL, 1);
		VerifyDataForType<TestType>();
	}

	//Big-endian.

	TEST_METHOD(OperInt16BE) {
		using TestType = std::int16_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_ROTL, 1, true);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperUInt16BE) {
		using TestType = std::uint16_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_ROTL, 1, true);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperInt32BE) {
		using TestType = std::int32_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_ROTL, 1, true);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperUInt32BE) {
		using TestType = std::uint32_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_ROTL, 1, true);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperInt64BE) {
		using TestType = std::int64_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_ROTL, 1, true);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperUInt64BE) {
		using TestType = std::uint64_t;
		CreateRandomTestData();
		OperDataForType<TestType>(OPER_ROTL, 1, true);
		VerifyDataForType<TestType>();
	}
	};
}