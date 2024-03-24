#define HEXCTRL_SHARED_DLL
#include "../../HexCtrl/HexCtrl.h"
#include "CppUnitTest.h"
#include <bit>
#include <limits>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace HEXCTRL;
using enum EHexModifyMode;
using enum EHexOperMode;
using enum EHexDataType;

namespace TestHexCtrl {
	inline auto GetHexCtrl() -> IHexCtrl* {
		static IHexCtrl* pHexCtrl = []() {
			static auto pHex { CreateHexCtrl() };
			static std::byte byteData[256] { };
			pHex->Create({ .dwStyle = WS_POPUP | WS_OVERLAPPEDWINDOW, .dwExStyle = WS_EX_APPWINDOW });
			pHex->SetData({ .spnData { byteData, sizeof(byteData) }, .fMutable { true } });
			return pHex.get();
			}(); //Immediate lambda for one time HexCtrl creation.
		return pHexCtrl;
	}

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

	template<typename T>
	void ModifyOperTData(EHexOperMode eOperMode, T tData, HEXSPAN hsp) {
		const HEXMODIFY hms { .eModifyMode { MODIFY_OPERATION }, .eOperMode { eOperMode },
			.eDataType { TypeToEHexDataType<T>() }, .spnData { reinterpret_cast<const std::byte*>(&tData),
			sizeof(tData) }, .vecSpan { hsp } };
		GetHexCtrl()->ModifyData(hms);
	}

	template<typename T>
	void AssignTData(T tData, HEXSPAN hsp) {
		ModifyOperTData(OPER_ASSIGN, tData, hsp);
	}

	template<typename T> concept TSize1248 = (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);

	//Bytes swap for types of 2, 4, or 8 byte size.
	template<TSize1248 T> [[nodiscard]] constexpr T ByteSwap(T tData)noexcept
	{
		//Since a swapping-data type can be any type of 2, 4, or 8 bytes size,
		//we first bit_cast swapping-data to an integral type of the same size,
		//then byte-swapping and then bit_cast to the original type back.
		if constexpr (sizeof(T) == sizeof(std::uint8_t)) { //1 byte.
			return tData;
		}
		else if constexpr (sizeof(T) == sizeof(std::uint16_t)) { //2 bytes.
			auto wData = std::bit_cast<std::uint16_t>(tData);
			if (std::is_constant_evaluated()) {
				wData = static_cast<std::uint16_t>((wData << 8) | (wData >> 8));
				return std::bit_cast<T>(wData);
			}
			return std::bit_cast<T>(_byteswap_ushort(wData));
		}
		else if constexpr (sizeof(T) == sizeof(std::uint32_t)) { //4 bytes.
			auto ulData = std::bit_cast<std::uint32_t>(tData);
			if (std::is_constant_evaluated()) {
				ulData = (ulData << 24) | ((ulData << 8) & 0x00FF'0000U)
					| ((ulData >> 8) & 0x0000'FF00U) | (ulData >> 24);
				return std::bit_cast<T>(ulData);
			}
			return std::bit_cast<T>(_byteswap_ulong(ulData));
		}
		else if constexpr (sizeof(T) == sizeof(std::uint64_t)) { //8 bytes.
			auto ullData = std::bit_cast<std::uint64_t>(tData);
			if (std::is_constant_evaluated()) {
				ullData = (ullData << 56) | ((ullData << 40) & 0x00FF'0000'0000'0000ULL)
					| ((ullData << 24) & 0x0000'FF00'0000'0000ULL) | ((ullData << 8) & 0x0000'00FF'0000'0000ULL)
					| ((ullData >> 8) & 0x0000'0000'FF00'0000ULL) | ((ullData >> 24) & 0x0000'0000'00FF'0000ULL)
					| ((ullData >> 40) & 0x0000'0000'0000'FF00ULL) | (ullData >> 56);
				return std::bit_cast<T>(ullData);
			}
			return std::bit_cast<T>(_byteswap_uint64(ullData));
		}
	}

	template<TSize1248 T> [[nodiscard]] constexpr T BitReverse(T tData) {
		T tReversed { };
		constexpr auto iBitsCount = sizeof(T) * 8;
		for (auto i = 0; i < iBitsCount; ++i, tData >>= 1) {
			tReversed = (tReversed << 1) | (tData & 1);
		}
		return tReversed;
	}
}