#include "stdafx.h"
#define HEXCTRL_SHARED_DLL
#include "../../HexCtrl/HexCtrl.h"
#include "CppUnitTest.h"
#include <limits>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace HEXCTRL;
using enum EHexModifyMode;
using enum EHexOperMode;
using enum EHexDataType;

namespace TestHexCtrl {
	TEST_MODULE_INITIALIZE(InitModule) {}

	template<typename T>
	[[nodiscard]] T GetTData(std::byte* pData) {
		return *reinterpret_cast<T*>(pData);
	}

	template<typename T>
	[[nodiscard]] auto TypeToEHexDataType() -> EHexDataType {
		if constexpr (std::is_same_v<T, std::int8_t>) {
			return DATA_INT8;
		}
		else if constexpr (std::is_same_v<T, std::uint8_t>) {
			return DATA_UINT8;
		}
		if constexpr (std::is_same_v<T, std::int16_t>) {
			return DATA_INT16;
		}
		else if constexpr (std::is_same_v<T, std::uint16_t>) {
			return DATA_UINT16;
		}
		if constexpr (std::is_same_v<T, std::int32_t>) {
			return DATA_INT32;
		}
		else if constexpr (std::is_same_v<T, std::uint32_t>) {
			return DATA_UINT32;
		}
		if constexpr (std::is_same_v<T, std::int64_t>) {
			return DATA_INT64;
		}
		else if constexpr (std::is_same_v<T, std::uint64_t>) {
			return DATA_UINT64;
		}
		if constexpr (std::is_same_v<T, float>) {
			return DATA_FLOAT;
		}
		else if constexpr (std::is_same_v<T, double>) {
			return DATA_DOUBLE;
		}
	}

	//Class CModifyTest
	TEST_CLASS(CModifyTest) {
private:
	static constexpr auto m_iRangeToTest = 4; //How many elements of given type to test.
	IHexCtrlPtr m_hex { CreateHexCtrl() };
	std::byte m_byteData[128] { };
public:
	CModifyTest() {
		m_hex->Create({ .dwStyle = WS_POPUP | WS_OVERLAPPEDWINDOW, .dwExStyle = WS_EX_APPWINDOW });
		m_hex->SetData({ .spnData { m_byteData, sizeof(m_byteData) }, .fMutable { true } });
	}

	template<typename T>
	void SetTData(T tData, HEXSPAN hsp) {
		const HEXMODIFY hms { .eModifyMode { MODIFY_OPERATION }, .eOperMode { OPER_ASSIGN },
			.eDataType { TypeToEHexDataType<T>() }, .spnData { reinterpret_cast<const std::byte*>(&tData),
			sizeof(tData) }, .vecSpan { hsp } };
		m_hex->ModifyData(hms);
	}

	template<typename T>
	void AddTData(T tOper, HEXSPAN hsp) {
		const HEXMODIFY hms { .eModifyMode { MODIFY_OPERATION }, .eOperMode { OPER_ADD },
			.eDataType { TypeToEHexDataType<T>() }, .spnData { reinterpret_cast<const std::byte*>(&tOper),
			sizeof(tOper) }, .vecSpan { hsp } };
		m_hex->ModifyData(hms);
	}


	//Test methods.

	TEST_METHOD(Creation) {
		Assert::AreEqual(true, m_hex->IsCreated());
		Assert::AreEqual(true, m_hex->IsDataSet());
		Assert::AreEqual(true, m_hex->IsMutable());
	}

	TEST_METHOD(AssignInt8) {
		constexpr std::int8_t i8Set = 1;
		const HEXSPAN hss { 0, m_hex->GetDataSize() };
		SetTData(i8Set, hss);
		const auto spnData = m_hex->GetData(hss);
		for (auto i = 0; i < sizeof(std::int8_t) * m_iRangeToTest; i += sizeof(std::int8_t)) {
			Assert::AreEqual(i8Set, GetTData<std::int8_t>(&spnData[i]));
		}
	}

	TEST_METHOD(AddInt8) {
		constexpr std::int8_t i8Set = 12;
		constexpr std::int8_t i8Oper = 12;
		constexpr std::int8_t i8Result = i8Set + i8Oper;
		const HEXSPAN hss { 0, m_hex->GetDataSize() };
		SetTData(i8Set, hss);
		AddTData(i8Oper, hss);
		const auto spnData = m_hex->GetData(hss);
		for (auto i = 0; i < sizeof(std::int8_t) * m_iRangeToTest; i += sizeof(std::int8_t)) {
			Assert::AreEqual(i8Result, GetTData<std::int8_t>(&spnData[i]));
		}
	}
	TEST_METHOD(AddUInt8) {
		constexpr std::uint8_t ui8Set = 12;
		constexpr std::uint8_t ui8Oper = 12;
		constexpr std::uint8_t ui8Result = ui8Set + ui8Oper;
		const HEXSPAN hss { 0, m_hex->GetDataSize() };
		SetTData(ui8Set, hss);
		AddTData(ui8Oper, hss);
		const auto spnData = m_hex->GetData(hss);
		for (auto i = 0; i < sizeof(std::uint8_t) * m_iRangeToTest; i += sizeof(std::uint8_t)) {
			Assert::AreEqual(ui8Result, GetTData<std::uint8_t>(&spnData[i]));
		}
	}
	TEST_METHOD(AddInt16) {
		constexpr std::int16_t i16Set = 12345;
		constexpr std::int16_t i16Oper = 12345;
		constexpr std::int16_t i16Result = i16Set + i16Oper;
		const HEXSPAN hss { 0, m_hex->GetDataSize() };
		SetTData(i16Set, hss);
		AddTData(i16Oper, hss);
		const auto spnData = m_hex->GetData(hss);
		for (auto i = 0; i < sizeof(std::int16_t) * m_iRangeToTest; i += sizeof(std::int16_t)) {
			Assert::AreEqual(i16Result, GetTData<std::int16_t>(&spnData[i]));
		}
	}
	TEST_METHOD(AddUInt16) {
		constexpr std::uint16_t ui16Set = 12345;
		constexpr std::uint16_t ui16Oper = 12345;
		constexpr std::uint16_t ui16Result = ui16Set + ui16Oper;
		const HEXSPAN hss { 0, m_hex->GetDataSize() };
		SetTData(ui16Set, hss);
		AddTData(ui16Oper, hss);
		const auto spnData = m_hex->GetData(hss);
		for (auto i = 0; i < sizeof(std::uint16_t) * m_iRangeToTest; i += sizeof(std::uint16_t)) {
			Assert::AreEqual(ui16Result, GetTData<std::uint16_t>(&spnData[i]));
		}
	}
	TEST_METHOD(AddInt32) {
		constexpr std::int32_t i32Set = 12345678;
		constexpr std::int32_t i32Oper = 12345678;
		constexpr std::int32_t i32Result = i32Set + i32Oper;
		const HEXSPAN hss { 0, m_hex->GetDataSize() };
		SetTData(i32Set, hss);
		AddTData(i32Oper, hss);
		const auto spnData = m_hex->GetData(hss);
		for (auto i = 0; i < sizeof(std::int32_t) * m_iRangeToTest; i += sizeof(std::int32_t)) {
			Assert::AreEqual(i32Result, GetTData<std::int32_t>(&spnData[i]));
		}
	}
	TEST_METHOD(AddUInt32) {
		constexpr std::uint32_t ui32Set = 12345678;
		constexpr std::uint32_t ui32Oper = 12345678;
		constexpr std::uint32_t ui64Result = ui32Set + ui32Oper;
		const HEXSPAN hss { 0, m_hex->GetDataSize() };
		SetTData(ui32Set, hss);
		AddTData(ui32Oper, hss);
		const auto spnData = m_hex->GetData(hss);
		for (auto i = 0; i < sizeof(std::uint32_t) * m_iRangeToTest; i += sizeof(std::uint32_t)) {
			Assert::AreEqual(ui64Result, GetTData<std::uint32_t>(&spnData[i]));
		}
	}
	TEST_METHOD(AddInt64) {
		constexpr std::int64_t i64Set = 12345678987654321;
		constexpr std::int64_t i64Oper = 12345678987654321;
		constexpr std::int64_t i64Result = i64Set + i64Oper;
		const HEXSPAN hss { 0, m_hex->GetDataSize() };
		SetTData(i64Set, hss);
		AddTData(i64Oper, hss);
		const auto spnData = m_hex->GetData(hss);
		for (auto i = 0; i < sizeof(std::int64_t) * m_iRangeToTest; i += sizeof(std::int64_t)) {
			Assert::AreEqual(i64Result, GetTData<std::int64_t>(&spnData[i]));
		}
	}
	TEST_METHOD(AddUInt64) {
		constexpr std::uint64_t ui64Set = 12345678987654321;
		constexpr std::uint64_t ui64Oper = 12345678987654321;
		constexpr std::uint64_t ui64Result = ui64Set + ui64Oper;
		const HEXSPAN hss { 0, m_hex->GetDataSize() };
		SetTData(ui64Set, hss);
		AddTData(ui64Oper, hss);
		const auto spnData = m_hex->GetData(hss);
		for (auto i = 0; i < sizeof(std::uint64_t) * m_iRangeToTest; i += sizeof(std::uint64_t)) {
			Assert::AreEqual(ui64Result, GetTData<std::uint64_t>(&spnData[i]));
		}
	}
	TEST_METHOD(AddFloat) {
		constexpr float flSet = 123456789.87654321F;
		constexpr float flOper = 123456789.87654321F;
		constexpr float flResult = flSet + flOper;
		const HEXSPAN hss { 0, m_hex->GetDataSize() };
		SetTData(flSet, hss);
		AddTData(flOper, hss);
		const auto spnData = m_hex->GetData(hss);
		for (auto i = 0; i < sizeof(float) * m_iRangeToTest; i += sizeof(float)) {
			Assert::AreEqual(flResult, GetTData<float>(&spnData[i]), std::numeric_limits<float>::epsilon());
		}
	}
	TEST_METHOD(AddDouble) {
		constexpr double dblSet = 1234567898765432.87654321;
		constexpr double dblOper = 1234567898765432.87654321;
		constexpr double dblResult = dblSet + dblOper;
		const HEXSPAN hss { 0, m_hex->GetDataSize() };
		SetTData(dblSet, hss);
		AddTData(dblOper, hss);
		const auto spnData = m_hex->GetData(hss);
		for (auto i = 0; i < sizeof(double) * m_iRangeToTest; i += sizeof(double)) {
			Assert::AreEqual(dblResult, GetTData<double>(&spnData[i]), std::numeric_limits<double>::epsilon());
		}
	}
	};
}