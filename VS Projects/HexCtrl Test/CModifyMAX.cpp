#include "CHexCtrlInit.h"
#include "CppUnitTest.h"

namespace TestHexCtrl {
	TEST_CLASS(CModifyMAX) {
public:
	TEST_METHOD(OperInt8) {
		using TestType = std::int8_t;
		CreateDataForType<TestType>();
		OperDataForType<TestType>(OPER_MAX, 2);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperUInt8) {
		using TestType = std::uint8_t;
		CreateDataForType<TestType>();
		OperDataForType<TestType>(OPER_MAX, 2);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperInt16) {
		using TestType = std::int16_t;
		CreateDataForType<TestType>();
		OperDataForType<TestType>(OPER_MAX, 12);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperUInt16) {
		using TestType = std::uint16_t;
		CreateDataForType<TestType>();
		OperDataForType<TestType>(OPER_MAX, 12);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperInt32) {
		using TestType = std::int32_t;
		CreateDataForType<TestType>();
		OperDataForType<TestType>(OPER_MAX, 123);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperUInt32) {
		using TestType = std::uint32_t;
		CreateDataForType<TestType>();
		OperDataForType<TestType>(OPER_MAX, 123);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperInt64) {
		using TestType = std::int64_t;
		CreateDataForType<TestType>();
		OperDataForType<TestType>(OPER_MAX, 1234);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperUInt64) {
		using TestType = std::uint64_t;
		CreateDataForType<TestType>();
		OperDataForType<TestType>(OPER_MAX, 1234);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperFloat) {
		using TestType = float;
		CreateDataForType<TestType>();
		OperDataForType<TestType>(OPER_MAX, 123.8765432F);
		VerifyDataForType<TestType>();
	}
	TEST_METHOD(OperDouble) {
		using TestType = double;
		CreateDataForType<TestType>();
		OperDataForType<TestType>(OPER_MAX, 123.8765432);
		VerifyDataForType<TestType>();
	}
	};
}