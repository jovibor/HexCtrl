#include "stdafx.h"
#define HEXCTRL_SHARED_DLL
#include "../../HexCtrl/HexCtrl.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace HEXCTRL;
using enum EHexModifyMode;
using enum EHexOperMode;
using enum EHexDataType;

namespace MAINNS {
	TEST_MODULE_INITIALIZE(InitModule) {}

	//Class TestHexCtrl
	TEST_CLASS(TestHexCtrl) {
private:
	IHexCtrlPtr m_hex { CreateHexCtrl() };
	std::byte m_byteData[1024] { };
public:
	TestHexCtrl() {
		m_hex->Create({ .dwStyle = WS_POPUP | WS_OVERLAPPEDWINDOW, .dwExStyle = WS_EX_APPWINDOW });
		m_hex->SetData({ .spnData { m_byteData, sizeof(m_byteData) }, .fMutable { true } });
	}

	void ZeroDataArray() {
		std::memset(m_byteData, 0, sizeof(m_byteData));
	}

	void Set1DataArray() {
		std::memset(m_byteData, 1, sizeof(m_byteData));
	}

	//Test methods.

	TEST_METHOD(Creation) {
		Assert::AreEqual(true, m_hex->IsCreated());
		Assert::AreEqual(true, m_hex->IsDataSet());
		Assert::AreEqual(true, m_hex->IsMutable());
	}

	TEST_METHOD(ModifyAssignInt8) {
		const std::int8_t iOper = 1;
		const HEXMODIFY hms { .eModifyMode { MODIFY_OPERATION }, .eOperMode { OPER_ASSIGN }, .eDataType { DATA_INT8 },
			.spnData { reinterpret_cast<const std::byte*>(&iOper), sizeof(iOper) },
			.vecSpan { { 0, m_hex->GetDataSize() } } };
		m_hex->ModifyData(hms);

		const auto spnData = m_hex->GetData({ 0, 4 });
		Assert::AreEqual(iOper, static_cast<std::int8_t>(spnData[0]));
		Assert::AreEqual(iOper, static_cast<std::int8_t>(spnData[1]));
		Assert::AreEqual(iOper, static_cast<std::int8_t>(spnData[2]));
		Assert::AreEqual(iOper, static_cast<std::int8_t>(spnData[3]));
	}

	TEST_METHOD(ModifyAddInt8) {
		Set1DataArray();
		const std::int8_t iOper = 1;
		const HEXMODIFY hms { .eModifyMode { MODIFY_OPERATION }, .eOperMode { OPER_ADD }, .eDataType { DATA_INT8 },
			.spnData { reinterpret_cast<const std::byte*>(&iOper), sizeof(iOper) },
			.vecSpan { { 0, m_hex->GetDataSize() } } };
		m_hex->ModifyData(hms);

		const auto spnData = m_hex->GetData({ 0, 4 });
		Assert::AreEqual(2, static_cast<int>(spnData[0]));
		Assert::AreEqual(2, static_cast<int>(spnData[1]));
		Assert::AreEqual(2, static_cast<int>(spnData[2]));
		Assert::AreEqual(2, static_cast<int>(spnData[3]));
	}
	};
}