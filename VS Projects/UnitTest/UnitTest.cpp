#include "stdafx.h"
#define HEXCTRL_SHARED_DLL
#include "../../HexCtrl/HexCtrl.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace HEXCTRL;

namespace TESTNS {
	IHexCtrlPtr m_hex;
	TEST_MODULE_INITIALIZE(methodName) {
		m_hex = CreateHexCtrl();
	}

	TEST_CLASS(HexCtrl) {
public:
	HexCtrl() {	}

	TEST_METHOD(Creation) {
		m_hex->Create({ .dwStyle = WS_POPUP | WS_OVERLAPPEDWINDOW, .dwExStyle = WS_EX_APPWINDOW });
		Assert::AreEqual(true, m_hex->IsCreated());
	}
	};
}