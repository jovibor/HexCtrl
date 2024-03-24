#include "stdafx.h"
#include "CHexCtrlInit.h"
#include "CppUnitTest.h"

namespace TestHexCtrl {
	TEST_CLASS(CModifyOR) {
private:
	static constexpr auto m_iRangeToTest = 18; //How many elements of given type to test.
	IHexCtrl* m_pHex { GetHexCtrl() };
public:
	template<typename T>
	void OperTData(T tData, HEXSPAN hsp) {
		ModifyOperTData(OPER_OR, tData, hsp);
	}

	//Test methods.

	TEST_METHOD(OperInt8) {
		constexpr std::int8_t i8Set = 12;
		constexpr std::int8_t i8Oper = 2;
		constexpr std::int8_t i8Result = i8Set | i8Oper;
		const HEXSPAN hss { 0, m_pHex->GetDataSize() };
		AssignTData(i8Set, hss);
		OperTData(i8Oper, hss);
		const auto spnData = m_pHex->GetData(hss);
		for (auto i = 0; i < sizeof(std::int8_t) * m_iRangeToTest; i += sizeof(std::int8_t)) {
			Assert::AreEqual(i8Result, GetTData<std::int8_t>(&spnData[i]));
		}
	}
	TEST_METHOD(OperUInt8) {
		constexpr std::uint8_t ui8Set = 12;
		constexpr std::uint8_t ui8Oper = 2;
		constexpr std::uint8_t ui8Result = ui8Set | ui8Oper;
		const HEXSPAN hss { 0, m_pHex->GetDataSize() };
		AssignTData(ui8Set, hss);
		OperTData(ui8Oper, hss);
		const auto spnData = m_pHex->GetData(hss);
		for (auto i = 0; i < sizeof(std::uint8_t) * m_iRangeToTest; i += sizeof(std::uint8_t)) {
			Assert::AreEqual(ui8Result, GetTData<std::uint8_t>(&spnData[i]));
		}
	}
	TEST_METHOD(OperInt16) {
		constexpr std::int16_t i16Set = 1234;
		constexpr std::int16_t i16Oper = 12;
		constexpr std::int16_t i16Result = i16Set | i16Oper;
		const HEXSPAN hss { 0, m_pHex->GetDataSize() };
		AssignTData(i16Set, hss);
		OperTData(i16Oper, hss);
		const auto spnData = m_pHex->GetData(hss);
		for (auto i = 0; i < sizeof(std::int16_t) * m_iRangeToTest; i += sizeof(std::int16_t)) {
			Assert::AreEqual(i16Result, GetTData<std::int16_t>(&spnData[i]));
		}
	}
	TEST_METHOD(OperUInt16) {
		constexpr std::uint16_t ui16Set = 1234;
		constexpr std::uint16_t ui16Oper = 12;
		constexpr std::uint16_t ui16Result = ui16Set | ui16Oper;
		const HEXSPAN hss { 0, m_pHex->GetDataSize() };
		AssignTData(ui16Set, hss);
		OperTData(ui16Oper, hss);
		const auto spnData = m_pHex->GetData(hss);
		for (auto i = 0; i < sizeof(std::uint16_t) * m_iRangeToTest; i += sizeof(std::uint16_t)) {
			Assert::AreEqual(ui16Result, GetTData<std::uint16_t>(&spnData[i]));
		}
	}
	TEST_METHOD(OperInt32) {
		constexpr std::int32_t i32Set = 12345678;
		constexpr std::int32_t i32Oper = 123;
		constexpr std::int32_t i32Result = i32Set | i32Oper;
		const HEXSPAN hss { 0, m_pHex->GetDataSize() };
		AssignTData(i32Set, hss);
		OperTData(i32Oper, hss);
		const auto spnData = m_pHex->GetData(hss);
		for (auto i = 0; i < sizeof(std::int32_t) * m_iRangeToTest; i += sizeof(std::int32_t)) {
			Assert::AreEqual(i32Result, GetTData<std::int32_t>(&spnData[i]));
		}
	}
	TEST_METHOD(OperUInt32) {
		constexpr std::uint32_t ui32Set = 12345678;
		constexpr std::uint32_t ui32Oper = 123;
		constexpr std::uint32_t ui32Result = ui32Set | ui32Oper;
		const HEXSPAN hss { 0, m_pHex->GetDataSize() };
		AssignTData(ui32Set, hss);
		OperTData(ui32Oper, hss);
		const auto spnData = m_pHex->GetData(hss);
		for (auto i = 0; i < sizeof(std::uint32_t) * m_iRangeToTest; i += sizeof(std::uint32_t)) {
			Assert::AreEqual(ui32Result, GetTData<std::uint32_t>(&spnData[i]));
		}
	}
	TEST_METHOD(OperInt64) {
		constexpr std::int64_t i64Set = 12345678987;
		constexpr std::int64_t i64Oper = 1234;
		constexpr std::int64_t i64Result = i64Set | i64Oper;
		const HEXSPAN hss { 0, m_pHex->GetDataSize() };
		AssignTData(i64Set, hss);
		OperTData(i64Oper, hss);
		const auto spnData = m_pHex->GetData(hss);
		for (auto i = 0; i < sizeof(std::int64_t) * m_iRangeToTest; i += sizeof(std::int64_t)) {
			Assert::AreEqual(i64Result, GetTData<std::int64_t>(&spnData[i]));
		}
	}
	TEST_METHOD(OperUInt64) {
		constexpr std::uint64_t ui64Set = 12345678987;
		constexpr std::uint64_t ui64Oper = 1234;
		constexpr std::uint64_t ui64Result = ui64Set | ui64Oper;
		const HEXSPAN hss { 0, m_pHex->GetDataSize() };
		AssignTData(ui64Set, hss);
		OperTData(ui64Oper, hss);
		const auto spnData = m_pHex->GetData(hss);
		for (auto i = 0; i < sizeof(std::uint64_t) * m_iRangeToTest; i += sizeof(std::uint64_t)) {
			Assert::AreEqual(ui64Result, GetTData<std::uint64_t>(&spnData[i]));
		}
	}
	};
}