#include "../../HexCtrl/HexCtrl.h"
#include "CppUnitTest.h"
#include <bit>
#include <cassert>
#include <cmath>
#include <limits>
#include <random>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace HEXCTRL;
using enum EHexModifyMode;
using enum EHexOperMode;
using enum EHexDataType;

namespace TestHexCtrl {
	[[nodiscard]] consteval auto GetTestDataSize() {
		return 477UL; //Size deliberately not equal to power of two.
	}

	[[nodiscard]] inline auto GetHexCtrl() -> IHexCtrl* {
		static IHexCtrl* pHexCtrl = []() {
			static auto pHex { CreateHexCtrl() };
			static std::byte byteData[GetTestDataSize()] { };
			pHex->Create({ .dwStyle { WS_POPUP | WS_OVERLAPPEDWINDOW }, .dwExStyle { WS_EX_APPWINDOW } });
			pHex->SetData({ .spnData { byteData, sizeof(byteData) }, .fMutable { true } });
			return pHex.get();
			}(); //Immediate lambda for one time HexCtrl creation.
		return pHexCtrl;
	}

	static std::byte byteReferenceData[GetTestDataSize()]; //Reference data array.
	[[nodiscard]] consteval auto GetReferenceData() {
		return &byteReferenceData;
	}

	[[nodiscard]] inline auto& GetMT19937() {
		static std::mt19937 gen(std::random_device { }());
		return gen;
	}

	template<typename T>
	void CreateDataForType() {
		constexpr auto iElemetsCount = GetTestDataSize() / sizeof(T);
		if constexpr (std::is_integral_v<T>) {
			constexpr bool fIsInt8 = std::is_same_v<T, std::int8_t> || std::is_same_v<T, std::uint8_t>;
			using IntType = std::conditional_t<fIsInt8, int, T>; //uniform_int_distribution doesn't allow (u)int8 types.
			std::uniform_int_distribution<IntType> distr((std::numeric_limits<T>::min)(), (std::numeric_limits<T>::max)());
			for (auto i { 0 }; i < iElemetsCount; ++i) {
				reinterpret_cast<T*>(GetReferenceData())[i] = static_cast<T>(distr(GetMT19937()));
			}
		}
		else if constexpr (std::is_floating_point_v<T>) {
			std::uniform_real_distribution<T> distr((std::numeric_limits<T>::min)(), (std::numeric_limits<T>::max)());
			for (auto i { 0 }; i < iElemetsCount; ++i) {
				reinterpret_cast<T*>(GetReferenceData())[i] = distr(GetMT19937());
			}
		}

		const HEXMODIFY hms { .eModifyMode { MODIFY_ONCE },
			.spnData { reinterpret_cast<const std::byte*>(GetReferenceData()), GetTestDataSize() }, .vecSpan { { 0, GetTestDataSize() } } };
		GetHexCtrl()->ModifyData(hms); //Now set the HexCtrl data equal to the Reference data.
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

	template<typename T> concept TSize1248 = (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);

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

	template<typename T>
	void OperDataForType(EHexOperMode eOperMode, T tOper = { }) {
		ModifyOperTData(eOperMode, tOper, { .ullOffset { 0 }, .ullSize { GetTestDataSize() } }); //Operate on whole HexCtrl's data.

		constexpr auto iElemetsCount = GetTestDataSize() / sizeof(T);
		for (auto i { 0 }; i < iElemetsCount; ++i) { //Operate on Reference data.
			auto tData = reinterpret_cast<T*>(GetReferenceData())[i];
			if constexpr (std::is_integral_v<T>) { //Operations only for integral types.
				switch (eOperMode) {
				case OPER_OR:
					tData |= tOper;
					break;
				case OPER_XOR:
					tData ^= tOper;
					break;
				case OPER_AND:
					tData &= tOper;
					break;
				case OPER_NOT:
					tData = ~tData;
					break;
				case OPER_SHL:
					tData <<= tOper;
					break;
				case OPER_SHR:
					tData >>= tOper;
					break;
				case OPER_ROTL:
					tData = std::rotl(static_cast<std::make_unsigned_t<T>>(tData), static_cast<const int>(tOper));
					break;
				case OPER_ROTR:
					tData = std::rotr(static_cast<std::make_unsigned_t<T>>(tData), static_cast<const int>(tOper));
					break;
				case OPER_BITREV:
					tData = BitReverse(tData);
					break;
				default:
					break;
				}
			}

			switch (eOperMode) { //Operations for integral and floating types.
			case OPER_ASSIGN: //Implemented as MODIFY_REPEAT.
				break;
			case OPER_ADD:
				tData += tOper;
				break;
			case OPER_SUB:
				tData -= tOper;
				break;
			case OPER_MUL:
				tData *= tOper;
				break;
			case OPER_DIV:
				assert(tOper > 0);
				tData /= tOper;
				break;
			case OPER_MIN:
				tData = (std::max)(tData, tOper);
				break;
			case OPER_MAX:
				tData = (std::min)(tData, tOper);
				break;
			case OPER_SWAP:
				tData = ByteSwap(tData);
				break;
			default:
				break;
			}

			reinterpret_cast<T*>(GetReferenceData())[i] = tData;
		}
	}

	template<typename T>
	void VerifyDataForType() {
		constexpr auto iElemetsCount = GetTestDataSize() / sizeof(T);
		const auto spnHexData = GetHexCtrl()->GetData({ .ullOffset { 0 }, .ullSize { GetTestDataSize() } });
		if constexpr (std::is_integral_v<T>) {
			for (auto i { 0 }; i < iElemetsCount; ++i) {
				Assert::AreEqual(reinterpret_cast<T*>(GetReferenceData())[i], reinterpret_cast<T*>(spnHexData.data())[i]);
			}
		}
		else if constexpr (std::is_floating_point_v<T>) {
			for (auto i { 0 }; i < iElemetsCount; ++i) {
				const auto tRefData = reinterpret_cast<T*>(GetReferenceData())[i];
				const auto tHexData = reinterpret_cast<T*>(spnHexData.data())[i];
				if (std::isfinite(tRefData) && std::isfinite(tHexData)) {
					Assert::AreEqual(tRefData, tHexData, std::numeric_limits<T>::epsilon());
				}
			}
		}
	}
}