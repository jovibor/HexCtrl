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

#ifdef _M_IX86
#ifdef _DEBUG
#define HEXCTRL_LIBNAME "HexCtrlx86D"
#else //^^^ _DEBUG / vvv !_DEBUG
#define HEXCTRL_LIBNAME "HexCtrlx86"
#endif //^^^ !_DEBUG
#elif defined(_M_X64) //^^^ _M_IX86 / vvv _M_X64
#ifdef _DEBUG
#define HEXCTRL_LIBNAME "HexCtrlx64D"
#else //^^^ _DEBUG / vvv !_DEBUG
#define HEXCTRL_LIBNAME "HexCtrlx64"
#endif //^^^ !_DEBUG
#elif defined(_M_ARM64) //^^^ _M_X64 / vvv _M_ARM64
#ifdef _DEBUG
#define HEXCTRL_LIBNAME "HexCtrlARM64D"
#else //^^^ _DEBUG / vvv !_DEBUG
#define HEXCTRL_LIBNAME "HexCtrlARM64"
#endif //^^^ _DEBUG
#endif //^^^ _M_ARM64
#pragma comment(lib, HEXCTRL_LIBNAME)

#define WIDEN_STRING(x) (L#x)

namespace TestHexCtrl {
	[[nodiscard]] consteval auto GetTestDataSize() {
		return 477UL; //Size deliberately not equal to power of two.
	}

	[[nodiscard]] inline auto GetDataHexCtrl() -> std::byte* {
		static std::byte arrDataHexCtrl[GetTestDataSize()];
		return arrDataHexCtrl;
	}

	[[nodiscard]] inline auto GetDataReference() -> std::byte* {
		static std::byte arrDataReference[GetTestDataSize()];
		return arrDataReference;
	}

	[[nodiscard]] inline auto GetHexCtrl() -> IHexCtrl* {
		static IHexCtrl* pHexCtrl = []() {
			static auto pHex { CreateHexCtrl() };
			pHex->Create({ .hInstRes { ::GetModuleHandleW(WIDEN_STRING(HEXCTRL_LIBNAME)) },
				.dwStyle { WS_POPUP | WS_OVERLAPPEDWINDOW }, .dwExStyle { WS_EX_APPWINDOW } });
			pHex->SetData({ .spnData { GetDataHexCtrl(), GetTestDataSize() }, .fMutable { true } });
			return pHex.get();
			}(); //Immediate lambda for one time HexCtrl creation.
		return pHexCtrl;
	}

	[[nodiscard]] inline auto& GetMT19937() {
		static std::mt19937 gen(std::random_device { }());
		return gen;
	}

	inline void CreateRandomTestData() {
		constexpr auto u32DataChunks = GetTestDataSize() / sizeof(std::uint64_t); //Tail can be omitted.
		std::uniform_int_distribution<std::uint64_t> distInt(0, (std::numeric_limits<std::uint64_t>::max)());
		for (auto i = 0U; i < u32DataChunks; ++i) {
			reinterpret_cast<std::uint64_t*>(GetDataReference())[i] = distInt(GetMT19937());
		}

		//Set HexCtrl's data equal to the reference data.
		std::memcpy(GetDataHexCtrl(), GetDataReference(), GetTestDataSize());
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

	template<typename T> concept TSize1248 = (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);

	template<TSize1248 T>
	[[nodiscard]] constexpr T ByteSwap(T tData)noexcept
	{
		//Since a swapping-data type can be any type of 2, 4, or 8 bytes size,
		//we first bit_cast swapping-data to an integral type of the same size,
		//then byte-swapping and then bit_cast to the original type back.
		if constexpr (sizeof(T) == sizeof(std::uint8_t)) { //1 byte.
			return tData;
		}
		else if constexpr (sizeof(T) == sizeof(std::uint16_t)) { //2 bytes.
			auto u16Data = std::bit_cast<std::uint16_t>(tData);
			if (std::is_constant_evaluated()) {
				u16Data = static_cast<std::uint16_t>((u16Data << 8) | (u16Data >> 8));
				return std::bit_cast<T>(u16Data);
			}
			return std::bit_cast<T>(_byteswap_ushort(u16Data));
		}
		else if constexpr (sizeof(T) == sizeof(std::uint32_t)) { //4 bytes.
			auto u32Data = std::bit_cast<std::uint32_t>(tData);
			if (std::is_constant_evaluated()) {
				u32Data = (u32Data << 24) | ((u32Data << 8) & 0x00FF'0000U)
					| ((u32Data >> 8) & 0x0000'FF00U) | (u32Data >> 24);
				return std::bit_cast<T>(u32Data);
			}
			return std::bit_cast<T>(_byteswap_ulong(u32Data));
		}
		else if constexpr (sizeof(T) == sizeof(std::uint64_t)) { //8 bytes.
			auto u64Data = std::bit_cast<std::uint64_t>(tData);
			if (std::is_constant_evaluated()) {
				u64Data = (u64Data << 56) | ((u64Data << 40) & 0x00FF'0000'0000'0000ULL)
					| ((u64Data << 24) & 0x0000'FF00'0000'0000ULL) | ((u64Data << 8) & 0x0000'00FF'0000'0000ULL)
					| ((u64Data >> 8) & 0x0000'0000'FF00'0000ULL) | ((u64Data >> 24) & 0x0000'0000'00FF'0000ULL)
					| ((u64Data >> 40) & 0x0000'0000'0000'FF00ULL) | (u64Data >> 56);
				return std::bit_cast<T>(u64Data);
			}
			return std::bit_cast<T>(_byteswap_uint64(u64Data));
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
	void ModifyHexCtrlAndRefData(EHexOperMode eOperMode, T tOperData = { }, bool fBigEndian = false) {
		//Modify HexCtrl's data.
		const HEXMODIFY hms { .eModifyMode { MODIFY_OPERATION }, .eOperMode { eOperMode },
			.eDataType { TypeToEHexDataType<T>() }, .spnData { reinterpret_cast<const std::byte*>(&tOperData),
			sizeof(tOperData) }, .vecSpan { { .ullOffset { 0 }, .ullSize { GetTestDataSize() } } }, .fBigEndian { fBigEndian } };
		GetHexCtrl()->ModifyData(hms);

		//Modify reference data, to compare then with the HexCtrl's modified data.
		constexpr auto u32DataChunks = GetTestDataSize() / sizeof(T);
		for (auto i { 0U }; i < u32DataChunks; ++i) {
			auto tData = reinterpret_cast<const T*>(GetDataReference())[i];
			if (fBigEndian) { tData = ByteSwap(tData); }

			if constexpr (std::is_integral_v<T>) { //Operations only for integral types.
				switch (eOperMode) {
				case OPER_OR:
					tData |= tOperData;
					break;
				case OPER_XOR:
					tData ^= tOperData;
					break;
				case OPER_AND:
					tData &= tOperData;
					break;
				case OPER_NOT:
					tData = ~tData;
					break;
				case OPER_SHL:
					tData <<= tOperData;
					break;
				case OPER_SHR:
					tData >>= tOperData;
					break;
				case OPER_ROTL:
					tData = std::rotl(static_cast<std::make_unsigned_t<T>>(tData), static_cast<const int>(tOperData));
					break;
				case OPER_ROTR:
					tData = std::rotr(static_cast<std::make_unsigned_t<T>>(tData), static_cast<const int>(tOperData));
					break;
				case OPER_BITREV:
					tData = BitReverse(tData);
					break;
				default:
					break;
				}
			}

			switch (eOperMode) { //Operations for integral and floating types.
			case OPER_ASSIGN:
				tData = tOperData;
				break;
			case OPER_ADD:
				tData += tOperData;
				break;
			case OPER_SUB:
				tData -= tOperData;
				break;
			case OPER_MUL:
				tData *= tOperData;
				break;
			case OPER_DIV:
				assert(tOperData > 0);
				tData /= tOperData;
				break;
			case OPER_MIN:
				if constexpr (std::is_floating_point_v<T>) {
					tData = std::fmax(tData, tOperData);
				}
				else {
					tData = (std::max)(tData, tOperData);
				}
				break;
			case OPER_MAX:
				if constexpr (std::is_floating_point_v<T>) {
					tData = std::fmin(tData, tOperData);
				}
				else {
					tData = (std::min)(tData, tOperData);
				}
				break;
			case OPER_SWAP:
				tData = ByteSwap(tData);
				break;
			default:
				break;
			}

			if (fBigEndian) {
				tData = ByteSwap(tData);
			}

			reinterpret_cast<T*>(GetDataReference())[i] = tData;
		}
	}

	template<typename T>
	void CompareHexCtrlAndRefData() {
		constexpr auto u32DataChunks = GetTestDataSize() / sizeof(T);
		for (auto i { 0U }; i < u32DataChunks; ++i) {
			const auto tDataRef = reinterpret_cast<const T*>(GetDataReference())[i];
			const auto tDataHex = reinterpret_cast<const T*>(GetDataHexCtrl())[i];
			if constexpr (std::is_integral_v<T>) {
				Assert::AreEqual(tDataRef, tDataHex);
			}
			else if constexpr (std::is_floating_point_v<T>) {
				if (std::isfinite(tDataRef) && std::isfinite(tDataHex)) {
					Assert::AreEqual(tDataRef, tDataHex, std::numeric_limits<T>::epsilon());
				}
			}
		}
	}
}