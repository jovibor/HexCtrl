#include "../../HexCtrl/HexCtrl.h"
#include "CppUnitTest.h"
#include <bit>
#include <cassert>
#include <cmath>
#include <limits>
#include <vector>
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
		return 70'003UL; //Size is deliberately not equal to power of two.
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
		constexpr auto u32DataChunks = GetTestDataSize() / sizeof(std::uint64_t);
		constexpr auto u32TailBytes = GetTestDataSize() % sizeof(std::uint64_t);
		std::uniform_int_distribution<std::uint64_t> distInt(0, (std::numeric_limits<std::uint64_t>::max)());

		for (auto i = 0U; i < u32DataChunks; ++i) {
			reinterpret_cast<std::uint64_t*>(GetDataReference())[i] = distInt(GetMT19937());
		}

		//Populate remaining tail bytes, so unaligned trailing preservation is tested with non-zero values.
		if constexpr (u32TailBytes > 0) {
			const auto u64Tail = distInt(GetMT19937());
			std::memcpy(GetDataReference() + (u32DataChunks * sizeof(std::uint64_t)), &u64Tail, u32TailBytes);
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
	[[nodiscard]] constexpr T ByteSwap(T tData)noexcept {
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

	template<TSize1248 T>
	[[nodiscard]] constexpr T BitReverse(T tData)noexcept {
		using U = std::make_unsigned_t<T>;
		auto uData = static_cast<U>(tData);
		U uReversed { };
		constexpr auto iBitsCount = std::numeric_limits<U>::digits;
		for (auto i = 0; i < iBitsCount; ++i, uData >>= 1) {
			uReversed = static_cast<U>((uReversed << 1) | (uData & 1));
		}

		return static_cast<T>(uReversed);
	}


	//Mock IHexVirtData implementation backed by GetDataHexCtrl() to test Virtual Data Mode.
	class CVirtDataTest final : public IHexVirtData {
	public:
		void OnHexGetData(HEXDATAINFO& hdi)override {
			const auto ullOffset = hdi.stHexSpan.ullOffset;
			const auto ullSize = hdi.stHexSpan.ullSize;
			if (ullOffset < GetTestDataSize() && ullSize <= (GetTestDataSize() - ullOffset)) {
				m_vecCache.resize(static_cast<std::size_t>(ullSize));
				std::memcpy(m_vecCache.data(), GetDataHexCtrl() + ullOffset, m_vecCache.size());
				hdi.spnData = m_vecCache;
			}
		}

		void OnHexGetOffset(HEXDATAINFO&, bool)override { }

		void OnHexSetData(const HEXDATAINFO& hdi)override {
			const auto ullOffset = hdi.stHexSpan.ullOffset;
			const auto ullSize = hdi.stHexSpan.ullSize;
			if (ullOffset < GetTestDataSize() && ullSize <= (GetTestDataSize() - ullOffset)
				&& hdi.spnData.size() >= ullSize) {
				std::memcpy(GetDataHexCtrl() + ullOffset, hdi.spnData.data(), static_cast<std::size_t>(ullSize));
			}
		}

	private:
		std::vector<std::byte> m_vecCache; //Simulates separate virtual memory cache buffer.
	};

	[[nodiscard]] inline auto GetVirtData() {
		static CVirtDataTest virtData;
		return &virtData;
	}

	//Switches the shared test HexCtrl instance between standard memory mode and Virtual Data Mode.
	inline void SetVirtualMode(bool fVirtual) {
		const auto pHex = GetHexCtrl(); //Ensure initial SetData() has run first.
		static bool fVirtCurr = { false };
		if (fVirtCurr == fVirtual) {
			return;
		}

		fVirtCurr = fVirtual;

		pHex->SetData({ .spnData { GetDataHexCtrl(), GetTestDataSize() },
			.pHexVirtData { fVirtual ? GetVirtData() : nullptr },
			.dwCacheSize { 1024UL * 64UL }, //64 KB minimum cache size to force multi-chunk splits if data > 64 KB.
			.fMutable { true }
		});
	}

	//Generates a mix of unaligned, tiny (< sizeof(T)), small (< SIMD width), large (>= SIMD width), and >64KB cache-crossing spans.
	[[nodiscard]] inline auto GetTestMultiSpans() -> const VecHexSpan& {
		static const VecHexSpan vecSpans { { .ullOffset { 3 }, .ullSize { 2 } }, //Tiny unaligned span (smaller than 4/8-byte types, skipped for larger T).
			{ .ullOffset { 11 }, .ullSize { 19 } },       //Unaligned span smaller than AVX2 32-byte vector (scalar path).
			{ .ullOffset { 45 }, .ullSize { 137 } },      //Unaligned span larger than AVX2 vector + unaligned remainder.
			{ .ullOffset { 210 }, .ullSize { 253 } },     //Unaligned medium span.
			{ .ullOffset { 65'000 }, .ullSize { 4'991 } } //Large span crossing the 64 KB virtual cache boundary (leaving 12 untouched tail bytes).
		};
		return vecSpans;
	}

	template<typename T>
	void ModifyHexCtrlAndRefData(EHexOperMode eOperMode, const VecHexSpan& vecSpan, T tOperData = { }, bool fBigEndian = false) {
		//Modify HexCtrl's data across all spans.
		const HEXMODIFY hms { .eModifyMode { MODIFY_OPERATION }, .eOperMode { eOperMode },
			.eDataType { TypeToEHexDataType<T>() }, .spnData { reinterpret_cast<const std::byte*>(&tOperData),
			sizeof(tOperData) }, .vecSpan { vecSpan }, .fBigEndian { fBigEndian } };
		GetHexCtrl()->ModifyData(hms);

		//Modify reference data span by span using the exact same rules.
		for (const auto& hs : vecSpan) {
			const auto u64Chunks = hs.ullSize / sizeof(T);
			auto* const pBaseRef = GetDataReference() + hs.ullOffset;

			for (std::uint64_t i { 0 }; i < u64Chunks; ++i) {
				T tData { };
				std::memcpy(&tData, pBaseRef + (i * sizeof(T)), sizeof(T)); //Safe unaligned load.
				if (fBigEndian) { tData = ByteSwap(tData); }

				if constexpr (std::is_integral_v<T>) {
					switch (eOperMode) {
					case OPER_OR:     tData |= tOperData; break;
					case OPER_XOR:    tData ^= tOperData; break;
					case OPER_AND:    tData &= tOperData; break;
					case OPER_NOT:    tData = ~tData; break;
					case OPER_SHL:    if (tOperData < 1) { return; } tData <<= tOperData; break;
					case OPER_SHR:    if (tOperData < 1) { return; } tData >>= tOperData; break;
					case OPER_ROTL:   tData = std::rotl(static_cast<std::make_unsigned_t<T>>(tData),
						static_cast<const int>(tOperData)); break;
					case OPER_ROTR:   tData = std::rotr(static_cast<std::make_unsigned_t<T>>(tData),
						static_cast<const int>(tOperData)); break;
					case OPER_BITREV: tData = BitReverse(tData); break;
					default: break;
					}
				}

				switch (eOperMode) {
				case OPER_ASSIGN: tData = tOperData; break;
				case OPER_ADD:    tData += tOperData; break;
				case OPER_SUB:    tData -= tOperData; break;
				case OPER_MUL:    tData *= tOperData; break;
				case OPER_DIV:    if (tOperData == 0) { return; } tData /= tOperData; break;
				case OPER_MIN:
					if constexpr (std::is_floating_point_v<T>) { tData = std::fmax(tData, tOperData); }
					else { tData = (std::max)(tData, tOperData); }
					break;
				case OPER_MAX:
					if constexpr (std::is_floating_point_v<T>) { tData = std::fmin(tData, tOperData); }
					else { tData = (std::min)(tData, tOperData); }
					break;
				case OPER_SWAP:   tData = ByteSwap(tData); break;
				default: break;
				}

				if (fBigEndian) { tData = ByteSwap(tData); }
				std::memcpy(pBaseRef + (i * sizeof(T)), &tData, sizeof(T)); //Safe unaligned store.
			}
		}
	}

	//Existing single-span overload forwards directly to the multi-span implementation.
	template<typename T>
	void ModifyHexCtrlAndRefData(EHexOperMode eOperMode, T tOperData = { }, bool fBigEndian = false) {
		ModifyHexCtrlAndRefData<T>(eOperMode, { { .ullOffset { 0 }, .ullSize { GetTestDataSize() } } }, tOperData, fBigEndian);
	}

	template<typename T>
	void CompareHexCtrlAndRefData() {
		if constexpr (std::is_integral_v<T>) {
			//For integral types we first compare with memcmp, which is fastest way.
			//Only if it fails we fallback to element by element comparing path,
			//to know exactly where a mismatch happens.
			if (std::memcmp(GetDataReference(), GetDataHexCtrl(), GetTestDataSize()) == 0) {
				return;
			}
		}

		constexpr auto u32DataChunks = GetTestDataSize() / sizeof(T);
		for (auto i { 0U }; i < u32DataChunks; ++i) {
			const auto tDataRef = reinterpret_cast<const T*>(GetDataReference())[i];
			const auto tDataHex = reinterpret_cast<const T*>(GetDataHexCtrl())[i];
			if constexpr (std::is_integral_v<T>) {
				Assert::AreEqual(tDataRef, tDataHex);
			}
			else if constexpr (std::is_floating_point_v<T>) {
				if (std::isnan(tDataRef) || std::isnan(tDataHex)) {
					Assert::AreEqual(std::isnan(tDataRef), std::isnan(tDataHex));
				}
				else if (std::isinf(tDataRef) || std::isinf(tDataHex)) {
					Assert::AreEqual(tDataRef, tDataHex);
				}
				else {
					const auto tMaxAbs = (std::max)(std::abs(tDataRef), std::abs(tDataHex));
					const auto tTolerance = (std::max)(std::numeric_limits<T>::denorm_min(),
						std::numeric_limits<T>::epsilon() * tMaxAbs * static_cast<T>(4));
					Assert::AreEqual(tDataRef, tDataHex, tTolerance);
				}
			}
		}

		//Verify unaligned trailing bytes one by one so Assert::AreEqual reports the exact mismatched byte.
		constexpr auto u32ProcessedBytes = u32DataChunks * sizeof(T);
		for (auto i { u32ProcessedBytes }; i < GetTestDataSize(); ++i) {
			const auto u8Ref = std::to_integer<std::uint8_t>(GetDataReference()[i]);
			const auto u8Hex = std::to_integer<std::uint8_t>(GetDataHexCtrl()[i]);
			Assert::AreEqual(u8Ref, u8Hex);
		}
	}
}