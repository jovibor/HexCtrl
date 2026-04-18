#include "CHexCtrlInit.h"
#include "CppUnitTest.h"

namespace TestHexCtrl {
	TEST_CLASS(CModifyASSIGN) {
public:
	TEST_METHOD(OperInt8) {
		using TestType = std::int8_t;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_ASSIGN, 127);
		CompareHexCtrlAndRefData<TestType>();
	}
	TEST_METHOD(OperUInt8) {
		using TestType = std::uint8_t;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_ASSIGN, 127);
		CompareHexCtrlAndRefData<TestType>();
	}
	TEST_METHOD(OperInt16) {
		using TestType = std::int16_t;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_ASSIGN, 27568);
		CompareHexCtrlAndRefData<TestType>();
	}
	TEST_METHOD(OperUInt16) {
		using TestType = std::uint16_t;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_ASSIGN, 27568);
		CompareHexCtrlAndRefData<TestType>();
	}
	TEST_METHOD(OperInt32) {
		using TestType = std::int32_t;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_ASSIGN, 319876543);
		CompareHexCtrlAndRefData<TestType>();
	}
	TEST_METHOD(OperUInt32) {
		using TestType = std::uint32_t;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_ASSIGN, 319876543);
		CompareHexCtrlAndRefData<TestType>();
	}
	TEST_METHOD(OperInt64) {
		using TestType = std::int64_t;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_ASSIGN, 987654321234567);
		CompareHexCtrlAndRefData<TestType>();
	}
	TEST_METHOD(OperUInt64) {
		using TestType = std::uint64_t;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_ASSIGN, 987654321234567);
		CompareHexCtrlAndRefData<TestType>();
	}
	TEST_METHOD(OperFloat) {
		using TestType = float;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_ASSIGN, 12.8765432F);
		CompareHexCtrlAndRefData<TestType>();
	}
	TEST_METHOD(OperDouble) {
		using TestType = double;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_ASSIGN, 1235.8765432);
		CompareHexCtrlAndRefData<TestType>();
	}

	//Big-endian.

	TEST_METHOD(OperInt16BE) {
		using TestType = std::int16_t;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_ASSIGN, 12, true);
		CompareHexCtrlAndRefData<TestType>();
	}
	TEST_METHOD(OperUInt16BE) {
		using TestType = std::uint16_t;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_ASSIGN, 12, true);
		CompareHexCtrlAndRefData<TestType>();
	}
	TEST_METHOD(OperInt32BE) {
		using TestType = std::int32_t;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_ASSIGN, 123, true);
		CompareHexCtrlAndRefData<TestType>();
	}
	TEST_METHOD(OperUInt32BE) {
		using TestType = std::uint32_t;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_ASSIGN, 123, true);
		CompareHexCtrlAndRefData<TestType>();
	}
	TEST_METHOD(OperInt64BE) {
		using TestType = std::int64_t;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_ASSIGN, 1234, true);
		CompareHexCtrlAndRefData<TestType>();
	}
	TEST_METHOD(OperUInt64BE) {
		using TestType = std::uint64_t;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_ASSIGN, 1234, true);
		CompareHexCtrlAndRefData<TestType>();
	}
	TEST_METHOD(OperFloatBE) {
		using TestType = float;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_ASSIGN, 12.8765432F, true);
		CompareHexCtrlAndRefData<TestType>();
	}
	TEST_METHOD(OperDoubleBE) {
		using TestType = double;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_ASSIGN, 12.8765432, true);
		CompareHexCtrlAndRefData<TestType>();
	}
	};
}