#include "CHexCtrlInit.h"
#include "CppUnitTest.h"

namespace TestHexCtrl {
	TEST_CLASS(CModifyROTL) {
public:
	TEST_METHOD(OperInt8) {
		using TestType = std::int8_t;
		CreateDataForType<TestType>();
		OperDataForType<TestType>(OPER_ROTL, 1);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperUInt8) {
		using TestType = std::uint8_t;
		CreateDataForType<TestType>();
		OperDataForType<TestType>(OPER_ROTL, 1);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperInt16) {
		using TestType = std::int16_t;
		CreateDataForType<TestType>();
		OperDataForType<TestType>(OPER_ROTL, 1);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperUInt16) {
		using TestType = std::uint16_t;
		CreateDataForType<TestType>();
		OperDataForType<TestType>(OPER_ROTL, 1);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperInt32) {
		using TestType = std::int32_t;
		CreateDataForType<TestType>();
		OperDataForType<TestType>(OPER_ROTL, 1);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperUInt32) {
		using TestType = std::uint32_t;
		CreateDataForType<TestType>();
		OperDataForType<TestType>(OPER_ROTL, 1);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperInt64) {
		using TestType = std::int64_t;
		CreateDataForType<TestType>();
		OperDataForType<TestType>(OPER_ROTL, 1);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperUInt64) {
		using TestType = std::uint64_t;
		CreateDataForType<TestType>();
		OperDataForType<TestType>(OPER_ROTL, 1);
		VerifyDataForType<TestType>();
	}
	};
}