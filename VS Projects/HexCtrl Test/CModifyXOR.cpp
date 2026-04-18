#include "CHexCtrlInit.h"
#include "CppUnitTest.h"

namespace TestHexCtrl {
	TEST_CLASS(CModifyXOR) {
public:
	TEST_METHOD(OperInt8) {
		using TestType = std::int8_t;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_XOR, 0x7F);
		CompareHexCtrlAndRefData<TestType>();
	}
	TEST_METHOD(OperUInt8) {
		using TestType = std::uint8_t;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_XOR, 0x7F);
		CompareHexCtrlAndRefData<TestType>();
	}
	TEST_METHOD(OperInt16) {
		using TestType = std::int16_t;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_XOR, 0x7FFF);
		CompareHexCtrlAndRefData<TestType>();
	}
	TEST_METHOD(OperUInt16) {
		using TestType = std::uint16_t;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_XOR, 0x7FFF);
		CompareHexCtrlAndRefData<TestType>();
	}
	TEST_METHOD(OperInt32) {
		using TestType = std::int32_t;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_XOR, 0x7FFFFFFF);
		CompareHexCtrlAndRefData<TestType>();
	}
	TEST_METHOD(OperUInt32) {
		using TestType = std::uint32_t;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_XOR, 0x7FFFFFFF);
		CompareHexCtrlAndRefData<TestType>();
	}
	TEST_METHOD(OperInt64) {
		using TestType = std::int64_t;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_XOR, 0x7FFFFFFFFFFFFFFF);
		CompareHexCtrlAndRefData<TestType>();
	}
	TEST_METHOD(OperUInt64) {
		using TestType = std::uint64_t;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_XOR, 0x7FFFFFFFFFFFFFFF);
		CompareHexCtrlAndRefData<TestType>();
	}

	//Big-endian.

	TEST_METHOD(OperInt16BE) {
		using TestType = std::int16_t;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_XOR, 0x7FFF, true);
		CompareHexCtrlAndRefData<TestType>();
	}
	TEST_METHOD(OperUInt16BE) {
		using TestType = std::uint16_t;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_XOR, 0x7FFF, true);
		CompareHexCtrlAndRefData<TestType>();
	}
	TEST_METHOD(OperInt32BE) {
		using TestType = std::int32_t;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_XOR, 0x7FFFFFFF, true);
		CompareHexCtrlAndRefData<TestType>();
	}
	TEST_METHOD(OperUInt32BE) {
		using TestType = std::uint32_t;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_XOR, 0x7FFFFFFF, true);
		CompareHexCtrlAndRefData<TestType>();
	}
	TEST_METHOD(OperInt64BE) {
		using TestType = std::int64_t;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_XOR, 0x7FFFFFFFFFFFFFFF, true);
		CompareHexCtrlAndRefData<TestType>();
	}
	TEST_METHOD(OperUInt64BE) {
		using TestType = std::uint64_t;
		CreateRandomTestData();
		ModifyHexCtrlAndRefData<TestType>(OPER_XOR, 0x7FFFFFFFFFFFFFFF, true);
		CompareHexCtrlAndRefData<TestType>();
	}
	};
}