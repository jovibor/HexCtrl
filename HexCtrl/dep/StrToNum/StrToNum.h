/**********************************************************
* SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception *
* Copyright © 2022 Jovibor https://github.com/jovibor/    *
* StrToNum library, https://github.com/jovibor/StrToNum   *
**********************************************************/
#pragma once
#include <cassert>
#include <string_view>
#include <type_traits>
#ifdef STN_USE_EXPECTED
#include <expected>
#define STN_RETURN_TYPE(NumT, CharT) std::expected<NumT, from_chars_result<CharT>>
#define STN_RETURN_VALUE(x) std::unexpected(x);
#else
#include <optional>
#define STN_RETURN_TYPE(NumT, CharT) std::optional<NumT>
#define STN_RETURN_VALUE(x) std::nullopt;
#endif


namespace stn //String to Num.
{
	enum class errc { // names for generic error codes
		invalid_argument = 22,   // EINVAL
		result_out_of_range = 34 // ERANGE
	};

	enum class chars_format {
		scientific = 0b001,
		fixed = 0b010,
		hex = 0b100,
		general = fixed | scientific,
	};

	template<typename CharT>
	struct from_chars_result {
		const CharT* ptr;
		errc ec;
		[[nodiscard]] friend bool operator==(const from_chars_result&, const from_chars_result&) = default;
	};

	namespace impl
	{
		inline constexpr unsigned char Digit_from_byte[] = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 255, 255,
			255, 255, 255, 255, 255, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
			32, 33, 34, 35, 255, 255, 255, 255, 255, 255, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
			26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255 };
		static_assert(std::size(Digit_from_byte) == 256);

		[[nodiscard]] inline constexpr unsigned char Digit_from_char(const wchar_t Ch) noexcept {
			// convert ['0', '9'] ['A', 'Z'] ['a', 'z'] to [0, 35], everything else to 255
			return Digit_from_byte[static_cast<unsigned char>(Ch)];
		}

		template <class RawTy, class CharT>
		[[nodiscard]] constexpr from_chars_result<CharT> Integer_from_chars(
			const CharT* const First, const CharT* const Last, RawTy& Raw_value, int Base) noexcept {
			assert(Base == 0 || (Base >= 2 && Base <= 36));

			bool Minus_sign = false;

			const auto* Next = First;

			if constexpr (std::is_signed_v<RawTy>) {
				if (Next != Last && *Next == '-') {
					Minus_sign = true;
					++Next;
				}
			}

			//Checking for '0x'/'0X' prefix if Base == 0 or 16.
			constexpr auto lmbCheck16 = [](const CharT* const pFirst, const CharT* const pLast) {
				return (pFirst != pLast && (pFirst + 1) != pLast && (*pFirst == '0' && (*(pFirst + 1) == 'x' || *(pFirst + 1) == 'X')));
			};

			if (Base == 0) {
				if (lmbCheck16(Next, Last)) {
					Next += 2;
					Base = 16;
				}
				else {
					Base = 10;
				}
			}
			else if (Base == 16) { //Base16 string may or may not contain the `0x` prefix.
				if (lmbCheck16(Next, Last)) {
					Next += 2;
				}
			}
			//End of '0x'/'0X' prefix checking.

			using Unsigned = std::make_unsigned_t<RawTy>;

			constexpr Unsigned Uint_max = static_cast<Unsigned>(-1);
			constexpr Unsigned Int_max = static_cast<Unsigned>(Uint_max >> 1);
			constexpr Unsigned Abs_int_min = static_cast<Unsigned>(Int_max + 1);

			Unsigned Risky_val;
			Unsigned Max_digit;

			if constexpr (std::is_signed_v<RawTy>) {
				if (Minus_sign) {
					Risky_val = static_cast<Unsigned>(Abs_int_min / Base);
					Max_digit = static_cast<Unsigned>(Abs_int_min % Base);
				}
				else {
					Risky_val = static_cast<Unsigned>(Int_max / Base);
					Max_digit = static_cast<Unsigned>(Int_max % Base);
				}
			}
			else {
				Risky_val = static_cast<Unsigned>(Uint_max / Base);
				Max_digit = static_cast<Unsigned>(Uint_max % Base);
			}

			Unsigned Value = 0;

			bool Overflowed = false;

			for (; Next != Last; ++Next) {
				const unsigned char Digit = Digit_from_char(*Next);

				if (Digit >= Base) {
					break;
				}

				if (Value < Risky_val // never overflows
					|| (Value == Risky_val && Digit <= Max_digit)) { // overflows for certain digits
					Value = static_cast<Unsigned>(Value * Base + Digit);
				}
				else { // Value > Risky_val always overflows
					Overflowed = true; // keep going, Next still needs to be updated, Value is now irrelevant
				}
			}

			if (Next - First == static_cast<ptrdiff_t>(Minus_sign)) {
				return { First, errc::invalid_argument };
			}

			if (Overflowed) {
				return { Next, errc::result_out_of_range };
			}

			if constexpr (std::is_signed_v<RawTy>) {
				if (Minus_sign) {
					Value = static_cast<Unsigned>(0 - Value);
				}
			}

			Raw_value = static_cast<RawTy>(Value); // implementation-defined for negative, N4713 7.8 [conv.integral]/3

			return { Next, errc { } };
		}

		template <class FloatingType>
		struct Floating_type_traits;

		template <>
		struct Floating_type_traits<float> {
			static constexpr int32_t Mantissa_bits = 24; // FLT_MANT_DIG
			static constexpr int32_t Exponent_bits = 8; // sizeof(float) * CHAR_BIT - FLT_MANT_DIG
			static constexpr int32_t Maximum_binary_exponent = 127; // FLT_MAX_EXP - 1
			static constexpr int32_t Minimum_binary_exponent = -126; // FLT_MIN_EXP - 1
			static constexpr int32_t Exponent_bias = 127;
			static constexpr int32_t Sign_shift = 31; // Exponent_bits + Mantissa_bits - 1
			static constexpr int32_t Exponent_shift = 23; // Mantissa_bits - 1

			using Uint_type = uint32_t;

			static constexpr uint32_t Exponent_mask = 0x000000FFu; // (1u << Exponent_bits) - 1
			static constexpr uint32_t Normal_mantissa_mask = 0x00FFFFFFu; // (1u << Mantissa_bits) - 1
			static constexpr uint32_t Denormal_mantissa_mask = 0x007FFFFFu; // (1u << (Mantissa_bits - 1)) - 1
			static constexpr uint32_t Special_nan_mantissa_mask = 0x00400000u; // 1u << (Mantissa_bits - 2)
			static constexpr uint32_t Shifted_sign_mask = 0x80000000u; // 1u << Sign_shift
			static constexpr uint32_t Shifted_exponent_mask = 0x7F800000u; // Exponent_mask << Exponent_shift
		};

		template <>
		struct Floating_type_traits<double> {
			static constexpr int32_t Mantissa_bits = 53; // DBL_MANT_DIG
			static constexpr int32_t Exponent_bits = 11; // sizeof(double) * CHAR_BIT - DBL_MANT_DIG
			static constexpr int32_t Maximum_binary_exponent = 1023; // DBL_MAX_EXP - 1
			static constexpr int32_t Minimum_binary_exponent = -1022; // DBL_MIN_EXP - 1
			static constexpr int32_t Exponent_bias = 1023;
			static constexpr int32_t Sign_shift = 63; // Exponent_bits + Mantissa_bits - 1
			static constexpr int32_t Exponent_shift = 52; // Mantissa_bits - 1

			using Uint_type = uint64_t;

			static constexpr uint64_t Exponent_mask = 0x00000000000007FFu; // (1ULL << Exponent_bits) - 1
			static constexpr uint64_t Normal_mantissa_mask = 0x001FFFFFFFFFFFFFu; // (1ULL << Mantissa_bits) - 1
			static constexpr uint64_t Denormal_mantissa_mask = 0x000FFFFFFFFFFFFFu; // (1ULL << (Mantissa_bits - 1)) - 1
			static constexpr uint64_t Special_nan_mantissa_mask = 0x0008000000000000u; // 1ULL << (Mantissa_bits - 2)
			static constexpr uint64_t Shifted_sign_mask = 0x8000000000000000u; // 1ULL << Sign_shift
			static constexpr uint64_t Shifted_exponent_mask = 0x7FF0000000000000u; // Exponent_mask << Exponent_shift
		};

		template <>
		struct Floating_type_traits<long double> : Floating_type_traits<double> {};

		struct Big_integer_flt {
			Big_integer_flt() noexcept : Myused(0) {}

			Big_integer_flt(const Big_integer_flt& Other) noexcept : Myused(Other.Myused) {
				std::memcpy(Mydata, Other.Mydata, Other.Myused * sizeof(uint32_t));
			}

			Big_integer_flt& operator=(const Big_integer_flt& Other) noexcept {
				Myused = Other.Myused;
				std::memmove(Mydata, Other.Mydata, Other.Myused * sizeof(uint32_t));
				return *this;
			}

			[[nodiscard]] bool operator<(const Big_integer_flt& Rhs) const noexcept {
				if (Myused != Rhs.Myused) {
					return Myused < Rhs.Myused;
				}

				for (uint32_t Ix = Myused - 1; Ix != static_cast<uint32_t>(-1); --Ix) {
					if (Mydata[Ix] != Rhs.Mydata[Ix]) {
						return Mydata[Ix] < Rhs.Mydata[Ix];
					}
				}

				return false;
			}

			static constexpr uint32_t Maximum_bits = 1074 // 1074 bits required to represent 2^1074
				+ 2552 // ceil(log2(10^768))
				+ 54; // shift space

			static constexpr uint32_t Element_bits = 32;

			static constexpr uint32_t Element_count = (Maximum_bits + Element_bits - 1) / Element_bits;

			uint32_t Myused; // The number of elements currently in use
			uint32_t Mydata[Element_count]; // The number, stored in little-endian form
		};

		[[nodiscard]] inline Big_integer_flt Make_big_integer_flt_one() noexcept {
			Big_integer_flt Xval { };
			Xval.Mydata[0] = 1;
			Xval.Myused = 1;
			return Xval;
		}

		[[nodiscard]] inline uint32_t Bit_scan_reverse(const uint32_t Value) noexcept {
			unsigned long Index; // Intentionally uninitialized for better codegen

			if (_BitScanReverse(&Index, Value)) {
				return Index + 1;
			}

			return 0;
		}

		[[nodiscard]] inline uint32_t Bit_scan_reverse(const uint64_t Value) noexcept {
			unsigned long Index; // Intentionally uninitialized for better codegen

		#ifdef WIN64
			if (BitScanReverse64(&Index, Value)) {
				return Index + 1;
			}
		#else // ^^^ 64-bit ^^^ / vvv 32-bit vvv
			uint32_t Ui32 = static_cast<uint32_t>(Value >> 32);

			if (_BitScanReverse(&Index, Ui32)) {
				return Index + 1 + 32;
			}

			Ui32 = static_cast<uint32_t>(Value);

			if (_BitScanReverse(&Index, Ui32)) {
				return Index + 1;
			}
		#endif // ^^^ 32-bit ^^^

			return 0;
		}

		[[nodiscard]] inline uint32_t Bit_scan_reverse(const Big_integer_flt& Xval) noexcept {
			if (Xval.Myused == 0) {
				return 0;
			}

			const uint32_t Bx = Xval.Myused - 1;

			assert(Xval.Mydata[Bx] != 0); // Big_integer_flt should always be trimmed

			unsigned long Index; // Intentionally uninitialized for better codegen

			_BitScanReverse(&Index, Xval.Mydata[Bx]); // assumes Xval.Mydata[Bx] != 0

			return Index + 1 + Bx * Big_integer_flt::Element_bits;
		}

		[[nodiscard]] inline bool Shift_left(Big_integer_flt& Xval, const uint32_t Nx) noexcept {
			if (Xval.Myused == 0) {
				return true;
			}

			const uint32_t Unit_shift = Nx / Big_integer_flt::Element_bits;
			const uint32_t Bit_shift = Nx % Big_integer_flt::Element_bits;

			if (Xval.Myused + Unit_shift > Big_integer_flt::Element_count) {
				// Unit shift will overflow.
				Xval.Myused = 0;
				return false;
			}

			if (Bit_shift == 0) {
				std::memmove(Xval.Mydata + Unit_shift, Xval.Mydata, Xval.Myused * sizeof(uint32_t));
				Xval.Myused += Unit_shift;
			}
			else {
				const bool Bit_shifts_into_next_unit =
					Bit_shift > (Big_integer_flt::Element_bits - Bit_scan_reverse(Xval.Mydata[Xval.Myused - 1]));

				const uint32_t New_used = Xval.Myused + Unit_shift + static_cast<uint32_t>(Bit_shifts_into_next_unit);

				if (New_used > Big_integer_flt::Element_count) {
					// Bit shift will overflow.
					Xval.Myused = 0;
					return false;
				}

				const uint32_t Msb_bits = Bit_shift;
				const uint32_t Lsb_bits = Big_integer_flt::Element_bits - Msb_bits;

				const uint32_t Lsb_mask = (1UL << Lsb_bits) - 1UL;
				const uint32_t Msb_mask = ~Lsb_mask;

				// If Unit_shift == 0, this will wraparound, which is okay.
				for (uint32_t Dest_index = New_used - 1; Dest_index != Unit_shift - 1; --Dest_index) {
					// performance note: PSLLDQ and PALIGNR instructions could be more efficient here

					// If Bit_shifts_into_next_unit, the first iteration will trigger the bounds check below, which is okay.
					const uint32_t Upper_source_index = Dest_index - Unit_shift;

					// When Dest_index == Unit_shift, this will wraparound, which is okay (see bounds check below).
					const uint32_t Lower_source_index = Dest_index - Unit_shift - 1;

					const uint32_t Upper_source = Upper_source_index < Xval.Myused ? Xval.Mydata[Upper_source_index] : 0;
					const uint32_t Lower_source = Lower_source_index < Xval.Myused ? Xval.Mydata[Lower_source_index] : 0;

					const uint32_t Shifted_upper_source = (Upper_source & Lsb_mask) << Msb_bits;
					const uint32_t Shifted_lower_source = (Lower_source & Msb_mask) >> Lsb_bits;

					const uint32_t Combined_shifted_source = Shifted_upper_source | Shifted_lower_source;

					Xval.Mydata[Dest_index] = Combined_shifted_source;
				}

				Xval.Myused = New_used;
			}

			std::memset(Xval.Mydata, 0, Unit_shift * sizeof(uint32_t));

			return true;
		}

		[[nodiscard]] inline bool Add(Big_integer_flt& Xval, const uint32_t Value) noexcept {
			if (Value == 0) {
				return true;
			}

			uint32_t Carry = Value;
			for (uint32_t Ix = 0; Ix != Xval.Myused; ++Ix) {
				const uint64_t Result = static_cast<uint64_t>(Xval.Mydata[Ix]) + Carry;
				Xval.Mydata[Ix] = static_cast<uint32_t>(Result);
				Carry = static_cast<uint32_t>(Result >> 32);
			}

			if (Carry != 0) {
				if (Xval.Myused < Big_integer_flt::Element_count) {
					Xval.Mydata[Xval.Myused] = Carry;
					++Xval.Myused;
				}
				else {
					Xval.Myused = 0;
					return false;
				}
			}

			return true;
		}

		[[nodiscard]] inline uint32_t Add_carry(uint32_t& Ux1, const uint32_t Ux2, const uint32_t U_carry) noexcept {
			const uint64_t Uu = static_cast<uint64_t>(Ux1) + Ux2 + U_carry;
			Ux1 = static_cast<uint32_t>(Uu);
			return static_cast<uint32_t>(Uu >> 32);
		}

		[[nodiscard]] inline uint32_t Add_multiply_carry(
			uint32_t& U_add, const uint32_t U_mul_1, const uint32_t U_mul_2, const uint32_t U_carry) noexcept {
			const uint64_t Uu_res = static_cast<uint64_t>(U_mul_1) * U_mul_2 + U_add + U_carry;
			U_add = static_cast<uint32_t>(Uu_res);
			return static_cast<uint32_t>(Uu_res >> 32);
		}

		[[nodiscard]] inline uint32_t Multiply_core(
			uint32_t* const Multiplicand, const uint32_t Multiplicand_count, const uint32_t Multiplier) noexcept {
			uint32_t Carry = 0;
			for (uint32_t Ix = 0; Ix != Multiplicand_count; ++Ix) {
				const uint64_t Result = static_cast<uint64_t>(Multiplicand[Ix]) * Multiplier + Carry;
				Multiplicand[Ix] = static_cast<uint32_t>(Result);
				Carry = static_cast<uint32_t>(Result >> 32);
			}

			return Carry;
		}

		[[nodiscard]] inline bool Multiply(Big_integer_flt& Multiplicand, const uint32_t Multiplier) noexcept {
			if (Multiplier == 0) {
				Multiplicand.Myused = 0;
				return true;
			}

			if (Multiplier == 1) {
				return true;
			}

			if (Multiplicand.Myused == 0) {
				return true;
			}

			const uint32_t Carry = Multiply_core(Multiplicand.Mydata, Multiplicand.Myused, Multiplier);
			if (Carry != 0) {
				if (Multiplicand.Myused < Big_integer_flt::Element_count) {
					Multiplicand.Mydata[Multiplicand.Myused] = Carry;
					++Multiplicand.Myused;
				}
				else {
					Multiplicand.Myused = 0;
					return false;
				}
			}

			return true;
		}

		[[nodiscard]] inline bool Multiply(Big_integer_flt& Multiplicand, const Big_integer_flt& Multiplier) noexcept {
			if (Multiplicand.Myused == 0) {
				return true;
			}

			if (Multiplier.Myused == 0) {
				Multiplicand.Myused = 0;
				return true;
			}

			if (Multiplier.Myused == 1) {
				return Multiply(Multiplicand, Multiplier.Mydata[0]); // when overflow occurs, resets to zero
			}

			if (Multiplicand.Myused == 1) {
				const uint32_t Small_multiplier = Multiplicand.Mydata[0];
				Multiplicand = Multiplier;
				return Multiply(Multiplicand, Small_multiplier); // when overflow occurs, resets to zero
			}

			// We prefer more iterations on the inner loop and fewer on the outer:
			const bool Multiplier_is_shorter = Multiplier.Myused < Multiplicand.Myused;
			const uint32_t* const Rgu1 = Multiplier_is_shorter ? Multiplier.Mydata : Multiplicand.Mydata;
			const uint32_t* const Rgu2 = Multiplier_is_shorter ? Multiplicand.Mydata : Multiplier.Mydata;

			const uint32_t Cu1 = Multiplier_is_shorter ? Multiplier.Myused : Multiplicand.Myused;
			const uint32_t Cu2 = Multiplier_is_shorter ? Multiplicand.Myused : Multiplier.Myused;

			Big_integer_flt Result { };
			for (uint32_t Iu1 = 0; Iu1 != Cu1; ++Iu1) {
				const uint32_t U_cur = Rgu1[Iu1];
				if (U_cur == 0) {
					if (Iu1 == Result.Myused) {
						Result.Mydata[Iu1] = 0;
						Result.Myused = Iu1 + 1;
					}

					continue;
				}

				uint32_t U_carry = 0;
				uint32_t Iu_res = Iu1;
				for (uint32_t Iu2 = 0; Iu2 != Cu2 && Iu_res != Big_integer_flt::Element_count; ++Iu2, ++Iu_res) {
					if (Iu_res == Result.Myused) {
						Result.Mydata[Iu_res] = 0;
						Result.Myused = Iu_res + 1;
					}

					U_carry = Add_multiply_carry(Result.Mydata[Iu_res], U_cur, Rgu2[Iu2], U_carry);
				}

				while (U_carry != 0 && Iu_res != Big_integer_flt::Element_count) {
					if (Iu_res == Result.Myused) {
						Result.Mydata[Iu_res] = 0;
						Result.Myused = Iu_res + 1;
					}

					U_carry = Add_carry(Result.Mydata[Iu_res++], 0, U_carry);
				}

				if (Iu_res == Big_integer_flt::Element_count) {
					Multiplicand.Myused = 0;
					return false;
				}
			}

			// Store the Result in the Multiplicand and compute the actual number of elements used:
			Multiplicand = Result;
			return true;
		}

		[[nodiscard]] inline bool Multiply_by_power_of_ten(Big_integer_flt& Xval, const uint32_t Power) noexcept {
			// To improve performance, we use a table of precomputed powers of ten, from 10^10 through 10^380, in increments
			// of ten. In its unpacked form, as an array of Big_integer_flt objects, this table consists mostly of zero
			// elements. Thus, we store the table in a packed form, trimming leading and trailing zero elements. We provide an
			// index that is used to unpack powers from the table, using the function that appears after this function in this
			// file.

			// The minimum value representable with double-precision is 5E-324.
			// With this table we can thus compute most multiplications with a single multiply.

			static constexpr uint32_t Large_power_data[] = { 0x540be400, 0x00000002, 0x63100000, 0x6bc75e2d, 0x00000005,
				0x40000000, 0x4674edea, 0x9f2c9cd0, 0x0000000c, 0xb9f56100, 0x5ca4bfab, 0x6329f1c3, 0x0000001d, 0xb5640000,
				0xc40534fd, 0x926687d2, 0x6c3b15f9, 0x00000044, 0x10000000, 0x946590d9, 0xd762422c, 0x9a224501, 0x4f272617,
				0x0000009f, 0x07950240, 0x245689c1, 0xc5faa71c, 0x73c86d67, 0xebad6ddc, 0x00000172, 0xcec10000, 0x63a22764,
				0xefa418ca, 0xcdd17b25, 0x6bdfef70, 0x9dea3e1f, 0x0000035f, 0xe4000000, 0xcdc3fe6e, 0x66bc0c6a, 0x2e391f32,
				0x5a450203, 0x71d2f825, 0xc3c24a56, 0x000007da, 0xa82e8f10, 0xaab24308, 0x8e211a7c, 0xf38ace40, 0x84c4ce0b,
				0x7ceb0b27, 0xad2594c3, 0x00001249, 0xdd1a4000, 0xcc9f54da, 0xdc5961bf, 0xc75cabab, 0xf505440c, 0xd1bc1667,
				0xfbb7af52, 0x608f8d29, 0x00002a94, 0x21000000, 0x17bb8a0c, 0x56af8ea4, 0x06479fa9, 0x5d4bb236, 0x80dc5fe0,
				0xf0feaa0a, 0xa88ed940, 0x6b1a80d0, 0x00006323, 0x324c3864, 0x8357c796, 0xe44a42d5, 0xd9a92261, 0xbd3c103d,
				0x91e5f372, 0xc0591574, 0xec1da60d, 0x102ad96c, 0x0000e6d3, 0x1e851000, 0x6e4f615b, 0x187b2a69, 0x0450e21c,
				0x2fdd342b, 0x635027ee, 0xa6c97199, 0x8e4ae916, 0x17082e28, 0x1a496e6f, 0x0002196e, 0x32400000, 0x04ad4026,
				0xf91e7250, 0x2994d1d5, 0x665bcdbb, 0xa23b2e96, 0x65fa7ddb, 0x77de53ac, 0xb020a29b, 0xc6bff953, 0x4b9425ab,
				0x0004e34d, 0xfbc32d81, 0x5222d0f4, 0xb70f2850, 0x5713f2f3, 0xdc421413, 0xd6395d7d, 0xf8591999, 0x0092381c,
				0x86b314d6, 0x7aa577b9, 0x12b7fe61, 0x000b616a, 0x1d11e400, 0x56c3678d, 0x3a941f20, 0x9b09368b, 0xbd706908,
				0x207665be, 0x9b26c4eb, 0x1567e89d, 0x9d15096e, 0x7132f22b, 0xbe485113, 0x45e5a2ce, 0x001a7f52, 0xbb100000,
				0x02f79478, 0x8c1b74c0, 0xb0f05d00, 0xa9dbc675, 0xe2d9b914, 0x650f72df, 0x77284b4c, 0x6df6e016, 0x514391c2,
				0x2795c9cf, 0xd6e2ab55, 0x9ca8e627, 0x003db1a6, 0x40000000, 0xf4ecd04a, 0x7f2388f0, 0x580a6dc5, 0x43bf046f,
				0xf82d5dc3, 0xee110848, 0xfaa0591c, 0xcdf4f028, 0x192ea53f, 0xbcd671a0, 0x7d694487, 0x10f96e01, 0x791a569d,
				0x008fa475, 0xb9b2e100, 0x8288753c, 0xcd3f1693, 0x89b43a6b, 0x089e87de, 0x684d4546, 0xfddba60c, 0xdf249391,
				0x3068ec13, 0x99b44427, 0xb68141ee, 0x5802cac3, 0xd96851f1, 0x7d7625a2, 0x014e718d, 0xfb640000, 0xf25a83e6,
				0x9457ad0f, 0x0080b511, 0x2029b566, 0xd7c5d2cf, 0xa53f6d7d, 0xcdb74d1c, 0xda9d70de, 0xb716413d, 0x71d0ca4e,
				0xd7e41398, 0x4f403a90, 0xf9ab3fe2, 0x264d776f, 0x030aafe6, 0x10000000, 0x09ab5531, 0xa60c58d2, 0x566126cb,
				0x6a1c8387, 0x7587f4c1, 0x2c44e876, 0x41a047cf, 0xc908059e, 0xa0ba063e, 0xe7cfc8e8, 0xe1fac055, 0xef0144b2,
				0x24207eb0, 0xd1722573, 0xe4b8f981, 0x071505ae, 0x7a3b6240, 0xcea45d4f, 0x4fe24133, 0x210f6d6d, 0xe55633f2,
				0x25c11356, 0x28ebd797, 0xd396eb84, 0x1e493b77, 0x471f2dae, 0x96ad3820, 0x8afaced1, 0x4edecddb, 0x5568c086,
				0xb2695da1, 0x24123c89, 0x107d4571, 0x1c410000, 0x6e174a27, 0xec62ae57, 0xef2289aa, 0xb6a2fbdd, 0x17e1efe4,
				0x3366bdf2, 0x37b48880, 0xbfb82c3e, 0x19acde91, 0xd4f46408, 0x35ff6a4e, 0x67566a0e, 0x40dbb914, 0x782a3bca,
				0x6b329b68, 0xf5afc5d9, 0x266469bc, 0xe4000000, 0xfb805ff4, 0xed55d1af, 0x9b4a20a8, 0xab9757f8, 0x01aefe0a,
				0x4a2ca67b, 0x1ebf9569, 0xc7c41c29, 0xd8d5d2aa, 0xd136c776, 0x93da550c, 0x9ac79d90, 0x254bcba8, 0x0df07618,
				0xf7a88809, 0x3a1f1074, 0xe54811fc, 0x59638ead, 0x97cbe710, 0x26d769e8, 0xb4e4723e, 0x5b90aa86, 0x9c333922,
				0x4b7a0775, 0x2d47e991, 0x9a6ef977, 0x160b40e7, 0x0c92f8c4, 0xf25ff010, 0x25c36c11, 0xc9f98b42, 0x730b919d,
				0x05ff7caf, 0xb0432d85, 0x2d2b7569, 0xa657842c, 0xd01fef10, 0xc77a4000, 0xe8b862e5, 0x10d8886a, 0xc8cd98e5,
				0x108955c5, 0xd059b655, 0x58fbbed4, 0x03b88231, 0x034c4519, 0x194dc939, 0x1fc500ac, 0x794cc0e2, 0x3bc980a1,
				0xe9b12dd1, 0x5e6d22f8, 0x7b38899a, 0xce7919d8, 0x78c67672, 0x79e5b99f, 0xe494034e, 0x00000001, 0xa1000000,
				0x6c5cd4e9, 0x9be47d6f, 0xf93bd9e7, 0x77626fa1, 0xc68b3451, 0xde2b59e8, 0xcf3cde58, 0x2246ff58, 0xa8577c15,
				0x26e77559, 0x17776753, 0xebe6b763, 0xe3fd0a5f, 0x33e83969, 0xa805a035, 0xf631b987, 0x211f0f43, 0xd85a43db,
				0xab1bf596, 0x683f19a2, 0x00000004, 0xbe7dfe64, 0x4bc9042f, 0xe1f5edb0, 0x8fa14eda, 0xe409db73, 0x674fee9c,
				0xa9159f0d, 0xf6b5b5d6, 0x7338960e, 0xeb49c291, 0x5f2b97cc, 0x0f383f95, 0x2091b3f6, 0xd1783714, 0xc1d142df,
				0x153e22de, 0x8aafdf57, 0x77f5e55f, 0xa3e7ca8b, 0x032f525b, 0x42e74f3d, 0x0000000a, 0xf4dd1000, 0x5d450952,
				0xaeb442e1, 0xa3b3342e, 0x3fcda36f, 0xb4287a6e, 0x4bc177f7, 0x67d2c8d0, 0xaea8f8e0, 0xadc93b67, 0x6cc856b3,
				0x959d9d0b, 0x5b48c100, 0x4abe8a3d, 0x52d936f4, 0x71dbe84d, 0xf91c21c5, 0x4a458109, 0xd7aad86a, 0x08e14c7c,
				0x759ba59c, 0xe43c8800, 0x00000017, 0x92400000, 0x04f110d4, 0x186472be, 0x8736c10c, 0x1478abfb, 0xfc51af29,
				0x25eb9739, 0x4c2b3015, 0xa1030e0b, 0x28fe3c3b, 0x7788fcba, 0xb89e4358, 0x733de4a4, 0x7c46f2c2, 0x8f746298,
				0xdb19210f, 0x2ea3b6ae, 0xaa5014b2, 0xea39ab8d, 0x97963442, 0x01dfdfa9, 0xd2f3d3fe, 0xa0790280, 0x00000037,
				0x509c9b01, 0xc7dcadf1, 0x383dad2c, 0x73c64d37, 0xea6d67d0, 0x519ba806, 0xc403f2f8, 0xa052e1a2, 0xd710233a,
				0x448573a9, 0xcf12d9ba, 0x70871803, 0x52dc3a9b, 0xe5b252e8, 0x0717fb4e, 0xbe4da62f, 0x0aabd7e1, 0x8c62ed4f,
				0xceb9ec7b, 0xd4664021, 0xa1158300, 0xcce375e6, 0x842f29f2, 0x00000081, 0x7717e400, 0xd3f5fb64, 0xa0763d71,
				0x7d142fe9, 0x33f44c66, 0xf3b8f12e, 0x130f0d8e, 0x734c9469, 0x60260fa8, 0x3c011340, 0xcc71880a, 0x37a52d21,
				0x8adac9ef, 0x42bb31b4, 0xd6f94c41, 0xc88b056c, 0xe20501b8, 0x5297ed7c, 0x62c361c4, 0x87dad8aa, 0xb833eade,
				0x94f06861, 0x13cc9abd, 0x8dc1d56a, 0x0000012d, 0x13100000, 0xc67a36e8, 0xf416299e, 0xf3493f0a, 0x77a5a6cf,
				0xa4be23a3, 0xcca25b82, 0x3510722f, 0xbe9d447f, 0xa8c213b8, 0xc94c324e, 0xbc9e33ad, 0x76acfeba, 0x2e4c2132,
				0x3e13cd32, 0x70fe91b4, 0xbb5cd936, 0x42149785, 0x46cc1afd, 0xe638ddf8, 0x690787d2, 0x1a02d117, 0x3eb5f1fe,
				0xc3b9abae, 0x1c08ee6f, 0x000002be, 0x40000000, 0x8140c2aa, 0x2cf877d9, 0x71e1d73d, 0xd5e72f98, 0x72516309,
				0xafa819dd, 0xd62a5a46, 0x2a02dcce, 0xce46ddfe, 0x2713248d, 0xb723d2ad, 0xc404bb19, 0xb706cc2b, 0x47b1ebca,
				0x9d094bdc, 0xc5dc02ca, 0x31e6518e, 0x8ec35680, 0x342f58a8, 0x8b041e42, 0xfebfe514, 0x05fffc13, 0x6763790f,
				0x66d536fd, 0xb9e15076, 0x00000662, 0x67b06100, 0xd2010a1a, 0xd005e1c0, 0xdb12733b, 0xa39f2e3f, 0x61b29de2,
				0x2a63dce2, 0x942604bc, 0x6170d59b, 0xc2e32596, 0x140b75b9, 0x1f1d2c21, 0xb8136a60, 0x89d23ba2, 0x60f17d73,
				0xc6cad7df, 0x0669df2b, 0x24b88737, 0x669306ed, 0x19496eeb, 0x938ddb6f, 0x5e748275, 0xc56e9a36, 0x3690b731,
				0xc82842c5, 0x24ae798e, 0x00000ede, 0x41640000, 0xd5889ac1, 0xd9432c99, 0xa280e71a, 0x6bf63d2e, 0x8249793d,
				0x79e7a943, 0x22fde64a, 0xe0d6709a, 0x05cacfef, 0xbd8da4d7, 0xe364006c, 0xa54edcb3, 0xa1a8086e, 0x748f459e,
				0xfc8e54c8, 0xcc74c657, 0x42b8c3d4, 0x57d9636e, 0x35b55bcc, 0x6c13fee9, 0x1ac45161, 0xb595badb, 0xa1f14e9d,
				0xdcf9e750, 0x07637f71, 0xde2f9f2b, 0x0000229d, 0x10000000, 0x3c5ebd89, 0xe3773756, 0x3dcba338, 0x81d29e4f,
				0xa4f79e2c, 0xc3f9c774, 0x6a1ce797, 0xac5fe438, 0x07f38b9c, 0xd588ecfa, 0x3e5ac1ac, 0x85afccce, 0x9d1f3f70,
				0xe82d6dd3, 0x177d180c, 0x5e69946f, 0x648e2ce1, 0x95a13948, 0x340fe011, 0xb4173c58, 0x2748f694, 0x7c2657bd,
				0x758bda2e, 0x3b8090a0, 0x2ddbb613, 0x6dcf4890, 0x24e4047e, 0x00005099 };

			struct Unpack_index {
				uint16_t Offset; // The offset of this power's initial element in the array
				uint8_t Zeroes; // The number of omitted leading zero elements
				uint8_t Size; // The number of elements present for this power
			};

			static constexpr Unpack_index Large_power_indices[] = { { 0, 0, 2 }, { 2, 0, 3 }, { 5, 0, 4 }, { 9, 1, 4 }, { 13, 1, 5 },
				{ 18, 1, 6 }, { 24, 2, 6 }, { 30, 2, 7 }, { 37, 2, 8 }, { 45, 3, 8 }, { 53, 3, 9 }, { 62, 3, 10 }, { 72, 4, 10 }, { 82, 4, 11 },
				{ 93, 4, 12 }, { 105, 5, 12 }, { 117, 5, 13 }, { 130, 5, 14 }, { 144, 5, 15 }, { 159, 6, 15 }, { 174, 6, 16 }, { 190, 6, 17 },
				{ 207, 7, 17 }, { 224, 7, 18 }, { 242, 7, 19 }, { 261, 8, 19 }, { 280, 8, 21 }, { 301, 8, 22 }, { 323, 9, 22 }, { 345, 9, 23 },
				{ 368, 9, 24 }, { 392, 10, 24 }, { 416, 10, 25 }, { 441, 10, 26 }, { 467, 10, 27 }, { 494, 11, 27 }, { 521, 11, 28 },
				{ 549, 11, 29 } };

			for (uint32_t Large_power = Power / 10; Large_power != 0;) {
				const uint32_t Current_power =
					(std::min)(Large_power, static_cast<uint32_t>(std::size(Large_power_indices)));

				const Unpack_index& Index = Large_power_indices[Current_power - 1];
				Big_integer_flt Multiplier { };
				Multiplier.Myused = static_cast<uint32_t>(Index.Size + Index.Zeroes);

				const uint32_t* const Source = Large_power_data + Index.Offset;

				std::memset(Multiplier.Mydata, 0, Index.Zeroes * sizeof(uint32_t));
				std::memcpy(Multiplier.Mydata + Index.Zeroes, Source, Index.Size * sizeof(uint32_t));

				if (!Multiply(Xval, Multiplier)) { // when overflow occurs, resets to zero
					return false;
				}

				Large_power -= Current_power;
			}

			static constexpr uint32_t Small_powers_of_ten[9] = {
				10, 100, 1'000, 10'000, 100'000, 1'000'000, 10'000'000, 100'000'000, 1'000'000'000 };

			const uint32_t Small_power = Power % 10;

			if (Small_power == 0) {
				return true;
			}

			return Multiply(Xval, Small_powers_of_ten[Small_power - 1]); // when overflow occurs, resets to zero
		}

		[[nodiscard]] inline uint32_t Count_sequential_high_zeroes(const uint32_t Ux) noexcept {
			unsigned long Index; // Intentionally uninitialized for better codegen
			return _BitScanReverse(&Index, Ux) ? 31 - Index : 32;
		}

		[[nodiscard]] inline uint64_t Divide(Big_integer_flt& Numerator, const Big_integer_flt& Denominator) noexcept {
			// If the Numerator is zero, then both the quotient and remainder are zero:
			if (Numerator.Myused == 0) {
				return 0;
			}

			// If the Denominator is zero, then uh oh. We can't divide by zero:
			assert(Denominator.Myused != 0); // Division by zero

			uint32_t Max_numerator_element_index = Numerator.Myused - 1;
			const uint32_t Max_denominator_element_index = Denominator.Myused - 1;

			// The Numerator and Denominator are both nonzero.
			// If the Denominator is only one element wide, we can take the fast route:
			if (Max_denominator_element_index == 0) {
				const uint32_t Small_denominator = Denominator.Mydata[0];

				if (Max_numerator_element_index == 0) {
					const uint32_t Small_numerator = Numerator.Mydata[0];

					if (Small_denominator == 1) {
						Numerator.Myused = 0;
						return Small_numerator;
					}

					Numerator.Mydata[0] = Small_numerator % Small_denominator;
					Numerator.Myused = Numerator.Mydata[0] > 0 ? 1u : 0u;
					return Small_numerator / Small_denominator;
				}

				if (Small_denominator == 1) {
					uint64_t Quotient = Numerator.Mydata[1];
					Quotient <<= 32;
					Quotient |= Numerator.Mydata[0];
					Numerator.Myused = 0;
					return Quotient;
				}

				// We count down in the next loop, so the last assignment to Quotient will be the correct one.
				uint64_t Quotient = 0;

				uint64_t Uu = 0;
				for (uint32_t Iv = Max_numerator_element_index; Iv != static_cast<uint32_t>(-1); --Iv) {
					Uu = (Uu << 32) | Numerator.Mydata[Iv];
					Quotient = (Quotient << 32) + static_cast<uint32_t>(Uu / Small_denominator);
					Uu %= Small_denominator;
				}

				Numerator.Mydata[1] = static_cast<uint32_t>(Uu >> 32);
				Numerator.Mydata[0] = static_cast<uint32_t>(Uu);

				if (Numerator.Mydata[1] > 0) {
					Numerator.Myused = 2u;
				}
				else if (Numerator.Mydata[0] > 0) {
					Numerator.Myused = 1u;
				}
				else {
					Numerator.Myused = 0u;
				}

				return Quotient;
			}

			if (Max_denominator_element_index > Max_numerator_element_index) {
				return 0;
			}

			const uint32_t Cu_den = Max_denominator_element_index + 1;
			const int32_t Cu_diff = static_cast<int32_t>(Max_numerator_element_index - Max_denominator_element_index);

			// Determine whether the result will have Cu_diff or Cu_diff + 1 digits:
			int32_t Cu_quo = Cu_diff;
			for (int32_t Iu = static_cast<int32_t>(Max_numerator_element_index);; --Iu) {
				if (Iu < Cu_diff) {
					++Cu_quo;
					break;
				}

				if (Denominator.Mydata[Iu - Cu_diff] != Numerator.Mydata[Iu]) {
					if (Denominator.Mydata[Iu - Cu_diff] < Numerator.Mydata[Iu]) {
						++Cu_quo;
					}

					break;
				}
			}

			if (Cu_quo == 0) {
				return 0;
			}

			// Get the uint to use for the trial divisions. We normalize so the high bit is set:
			uint32_t U_den = Denominator.Mydata[Cu_den - 1];
			uint32_t U_den_next = Denominator.Mydata[Cu_den - 2];

			const uint32_t Cbit_shift_left = Count_sequential_high_zeroes(U_den);
			const uint32_t Cbit_shift_right = 32 - Cbit_shift_left;
			if (Cbit_shift_left > 0) {
				U_den = (U_den << Cbit_shift_left) | (U_den_next >> Cbit_shift_right);
				U_den_next <<= Cbit_shift_left;

				if (Cu_den > 2) {
					U_den_next |= Denominator.Mydata[Cu_den - 3] >> Cbit_shift_right;
				}
			}

			uint64_t Quotient = 0;
			for (int32_t Iu = Cu_quo; --Iu >= 0;) {
				// Get the high (normalized) bits of the Numerator:
				const uint32_t U_num_hi =
					(Iu + Cu_den <= Max_numerator_element_index) ? Numerator.Mydata[Iu + Cu_den] : 0;

				uint64_t Uu_num =
					(static_cast<uint64_t>(U_num_hi) << 32) | static_cast<uint64_t>(Numerator.Mydata[Iu + Cu_den - 1]);

				uint32_t U_num_next = Numerator.Mydata[Iu + Cu_den - 2];
				if (Cbit_shift_left > 0) {
					Uu_num = (Uu_num << Cbit_shift_left) | (U_num_next >> Cbit_shift_right);
					U_num_next <<= Cbit_shift_left;

					if (Iu + Cu_den >= 3) {
						U_num_next |= Numerator.Mydata[Iu + Cu_den - 3] >> Cbit_shift_right;
					}
				}

				// Divide to get the quotient digit:
				uint64_t Uu_quo = Uu_num / U_den;
				uint64_t Uu_rem = static_cast<uint32_t>(Uu_num % U_den);

				if (Uu_quo > UINT32_MAX) {
					Uu_rem += U_den * (Uu_quo - UINT32_MAX);
					Uu_quo = UINT32_MAX;
				}

				while (Uu_rem <= UINT32_MAX && Uu_quo * U_den_next > ((Uu_rem << 32) | U_num_next)) {
					--Uu_quo;
					Uu_rem += U_den;
				}

				// Multiply and subtract. Note that Uu_quo may be one too large.
				// If we have a borrow at the end, we'll add the Denominator back on and decrement Uu_quo.
				if (Uu_quo > 0) {
					uint64_t Uu_borrow = 0;

					for (uint32_t Iu2 = 0; Iu2 < Cu_den; ++Iu2) {
						Uu_borrow += Uu_quo * Denominator.Mydata[Iu2];

						const uint32_t U_sub = static_cast<uint32_t>(Uu_borrow);
						Uu_borrow >>= 32;
						if (Numerator.Mydata[Iu + Iu2] < U_sub) {
							++Uu_borrow;
						}

						Numerator.Mydata[Iu + Iu2] -= U_sub;
					}

					if (U_num_hi < Uu_borrow) {
						// Add, tracking carry:
						uint32_t U_carry = 0;
						for (uint32_t Iu2 = 0; Iu2 < Cu_den; ++Iu2) {
							const uint64_t Sum = static_cast<uint64_t>(Numerator.Mydata[Iu + Iu2])
								+ static_cast<uint64_t>(Denominator.Mydata[Iu2]) + U_carry;

							Numerator.Mydata[Iu + Iu2] = static_cast<uint32_t>(Sum);
							U_carry = static_cast<uint32_t>(Sum >> 32);
						}

						--Uu_quo;
					}

					Max_numerator_element_index = Iu + Cu_den - 1;
				}

				Quotient = (Quotient << 32) + static_cast<uint32_t>(Uu_quo);
			}

			// Trim the remainder:
			for (uint32_t Ix = Max_numerator_element_index + 1; Ix < Numerator.Myused; ++Ix) {
				Numerator.Mydata[Ix] = 0;
			}

			uint32_t Used = Max_numerator_element_index + 1;

			while (Used != 0 && Numerator.Mydata[Used - 1] == 0) {
				--Used;
			}

			Numerator.Myused = Used;

			return Quotient;
		}

		struct Floating_point_string {
			bool Myis_negative;
			int32_t Myexponent;
			uint32_t Mymantissa_count;
			uint8_t Mymantissa[768];
		};

		template <class FloatingType>
		void Assemble_floating_point_zero(const bool Is_negative, FloatingType& Result) noexcept {
			using Floating_traits = Floating_type_traits<FloatingType>;
			using Uint_type = typename Floating_traits::Uint_type;

			Uint_type Sign_component = Is_negative;
			Sign_component <<= Floating_traits::Sign_shift;

			Result = std::bit_cast<FloatingType>(Sign_component);
		}

		template <class FloatingType>
		void Assemble_floating_point_infinity(const bool Is_negative, FloatingType& Result) noexcept {
			using Floating_traits = Floating_type_traits<FloatingType>;
			using Uint_type = typename Floating_traits::Uint_type;

			Uint_type Sign_component = Is_negative;
			Sign_component <<= Floating_traits::Sign_shift;

			const Uint_type Exponent_component = Floating_traits::Shifted_exponent_mask;

			Result = std::bit_cast<FloatingType>(Sign_component | Exponent_component);
		}

		[[nodiscard]] inline bool Should_round_up(
			const bool Lsb_bit, const bool Round_bit, const bool Has_tail_bits) noexcept {

			return Round_bit && (Has_tail_bits || Lsb_bit);
		}

		[[nodiscard]] inline uint64_t Right_shift_with_rounding(
			const uint64_t Value, const uint32_t Shift, const bool Has_zero_tail) noexcept {
			constexpr uint32_t Total_number_of_bits = 64;
			if (Shift >= Total_number_of_bits) {
				if (Shift == Total_number_of_bits) {
					constexpr uint64_t Extra_bits_mask = (1ULL << (Total_number_of_bits - 1)) - 1;
					constexpr uint64_t Round_bit_mask = (1ULL << (Total_number_of_bits - 1));

					const bool Round_bit = (Value & Round_bit_mask) != 0;
					const bool Tail_bits = !Has_zero_tail || (Value & Extra_bits_mask) != 0;

					// We round up the answer to 1 if the answer is greater than 0.5. Otherwise, we round down the answer to 0
					// if either [1] the answer is less than 0.5 or [2] the answer is exactly 0.5.
					return static_cast<uint64_t>(Round_bit && Tail_bits);
				}
				else {
					// If we'd need to shift 65 or more bits, the answer is less than 0.5 and is always rounded to zero:
					return 0;
				}
			}

			const uint64_t Lsb_bit = Value;
			const uint64_t Round_bit = Value << 1;
			const uint64_t Has_tail_bits = Round_bit - static_cast<uint64_t>(Has_zero_tail);
			const uint64_t Should_round = ((Round_bit & (Has_tail_bits | Lsb_bit)) >> Shift) & uint64_t { 1 };

			return (Value >> Shift) + Should_round;
		}

		template <class FloatingType>
		void Assemble_floating_point_value_no_shift(const bool Is_negative, const int32_t Exponent,
			const typename Floating_type_traits<FloatingType>::Uint_type Mantissa, FloatingType& Result) noexcept {
			// The following code assembles floating-point values based on an alternative interpretation of the IEEE 754 binary
			// floating-point format. It is valid for all of the following cases:
			// [1] normal value,
			// [2] normal value, needs renormalization and exponent increment after rounding up the mantissa,
			// [3] normal value, overflows after rounding up the mantissa,
			// [4] subnormal value,
			// [5] subnormal value, becomes a normal value after rounding up the mantissa.

			// Examples for float:
			// | Case |     Input     | Exponent |  Exponent  |  Exponent  |  Rounded  | Result Bits |     Result      |
			// |      |               |          | + Bias - 1 |  Component |  Mantissa |             |                 |
			// | ---- | ------------- | -------- | ---------- | ---------- | --------- | ----------- | --------------- |
			// | [1]  | 1.000000p+0   |     +0   |    126     | 0x3f000000 |  0x800000 | 0x3f800000  | 0x1.000000p+0   |
			// | [2]  | 1.ffffffp+0   |     +0   |    126     | 0x3f000000 | 0x1000000 | 0x40000000  | 0x1.000000p+1   |
			// | [3]  | 1.ffffffp+127 |   +127   |    253     | 0x7e800000 | 0x1000000 | 0x7f800000  |     inf         |
			// | [4]  | 0.fffffep-126 |   -126   |      0     | 0x00000000 |  0x7fffff | 0x007fffff  | 0x0.fffffep-126 |
			// | [5]  | 0.ffffffp-126 |   -126   |      0     | 0x00000000 |  0x800000 | 0x00800000  | 0x1.000000p-126 |
			using Floating_traits = Floating_type_traits<FloatingType>;
			using Uint_type = typename Floating_traits::Uint_type;

			Uint_type Sign_component = Is_negative;
			Sign_component <<= Floating_traits::Sign_shift;

			Uint_type Exponent_component = static_cast<uint32_t>(Exponent + (Floating_traits::Exponent_bias - 1));
			Exponent_component <<= Floating_traits::Exponent_shift;

			Result = std::bit_cast<FloatingType>(Sign_component | (Exponent_component + Mantissa));
		}

		template <class FloatingType>
		[[nodiscard]] errc Assemble_floating_point_value(const uint64_t Initial_mantissa, const int32_t Initial_exponent,
			const bool Is_negative, const bool Has_zero_tail, FloatingType& Result) noexcept {
			using Traits = Floating_type_traits<FloatingType>;

			// Assume that the number is representable as a normal value.
			// Compute the number of bits by which we must adjust the mantissa to shift it into the correct position,
			// and compute the resulting base two exponent for the normalized mantissa:
			const uint32_t Initial_mantissa_bits = Bit_scan_reverse(Initial_mantissa);
			const int32_t Normal_mantissa_shift = static_cast<int32_t>(Traits::Mantissa_bits - Initial_mantissa_bits);
			const int32_t Normal_exponent = Initial_exponent - Normal_mantissa_shift;

			if (Normal_exponent > Traits::Maximum_binary_exponent) {
				// The exponent is too large to be represented by the floating-point type; report the overflow condition:
				Assemble_floating_point_infinity(Is_negative, Result);
				return errc::result_out_of_range; // Overflow example: "1e+1000"
			}

			uint64_t Mantissa = Initial_mantissa;
			int32_t Exponent = Normal_exponent;
			errc Error_code { };

			if (Normal_exponent < Traits::Minimum_binary_exponent) {
				// The exponent is too small to be represented by the floating-point type as a normal value, but it may be
				// representable as a denormal value.

				// The exponent of subnormal values (as defined by the mathematical model of floating-point numbers, not the
				// exponent field in the bit representation) is equal to the minimum exponent of normal values.
				Exponent = Traits::Minimum_binary_exponent;

				// Compute the number of bits by which we need to shift the mantissa in order to form a denormal number.
				const int32_t Denormal_mantissa_shift = Initial_exponent - Exponent;

				if (Denormal_mantissa_shift < 0) {
					Mantissa =
						Right_shift_with_rounding(Mantissa, static_cast<uint32_t>(-Denormal_mantissa_shift), Has_zero_tail);

					// from_chars in MSVC STL and strto[f|d|ld] in UCRT reports underflow only when the result is zero after
					// rounding to the floating-point format. This behavior is different from IEEE 754 underflow exception.
					if (Mantissa == 0) {
						Error_code = errc::result_out_of_range; // Underflow example: "1e-1000"
					}

					// When we round the mantissa, the result may be so large that the number becomes a normal value.
					// For example, consider the single-precision case where the mantissa is 0x01ffffff and a right shift
					// of 2 is required to shift the value into position. We perform the shift in two steps: we shift by
					// one bit, then we shift again and round using the dropped bit. The initial shift yields 0x00ffffff.
					// The rounding shift then yields 0x007fffff and because the least significant bit was 1, we add 1
					// to this number to round it. The final result is 0x00800000.

					// 0x00800000 is 24 bits, which is more than the 23 bits available in the mantissa.
					// Thus, we have rounded our denormal number into a normal number.

					// We detect this case here and re-adjust the mantissa and exponent appropriately, to form a normal number.
					// This is handled by Assemble_floating_point_value_no_shift.
				}
				else {
					Mantissa <<= Denormal_mantissa_shift;
				}
			}
			else {
				if (Normal_mantissa_shift < 0) {
					Mantissa =
						Right_shift_with_rounding(Mantissa, static_cast<uint32_t>(-Normal_mantissa_shift), Has_zero_tail);

					// When we round the mantissa, it may produce a result that is too large. In this case,
					// we divide the mantissa by two and increment the exponent (this does not change the value).
					// This is handled by Assemble_floating_point_value_no_shift.

					// The increment of the exponent may have generated a value too large to be represented.
					// In this case, report the overflow:
					if (Mantissa > Traits::Normal_mantissa_mask && Exponent == Traits::Maximum_binary_exponent) {
						Error_code = errc::result_out_of_range; // Overflow example: "1.ffffffp+127" for float
																 // Overflow example: "1.fffffffffffff8p+1023" for double
					}
				}
				else {
					Mantissa <<= Normal_mantissa_shift;
				}
			}

			// Assemble the floating-point value from the computed components:
			using Uint_type = typename Traits::Uint_type;

			Assemble_floating_point_value_no_shift(Is_negative, Exponent, static_cast<Uint_type>(Mantissa), Result);

			return Error_code;
		}

		template <class FloatingType>
		[[nodiscard]] errc Assemble_floating_point_value_from_big_integer_flt(const Big_integer_flt& Integer_value,
			const uint32_t Integer_bits_of_precision, const bool Is_negative, const bool Has_nonzero_fractional_part,
			FloatingType& Result) noexcept {
			using Traits = Floating_type_traits<FloatingType>;

			const int32_t Base_exponent = Traits::Mantissa_bits - 1;

			// Very fast case: If we have 64 bits of precision or fewer,
			// we can just take the two low order elements from the Big_integer_flt:
			if (Integer_bits_of_precision <= 64) {
				const int32_t Exponent = Base_exponent;

				const uint32_t Mantissa_low = Integer_value.Myused > 0 ? Integer_value.Mydata[0] : 0;
				const uint32_t Mantissa_high = Integer_value.Myused > 1 ? Integer_value.Mydata[1] : 0;
				const uint64_t Mantissa = Mantissa_low + (static_cast<uint64_t>(Mantissa_high) << 32);

				return Assemble_floating_point_value(
					Mantissa, Exponent, Is_negative, !Has_nonzero_fractional_part, Result);
			}

			const uint32_t Top_element_bits = Integer_bits_of_precision % 32;
			const uint32_t Top_element_index = Integer_bits_of_precision / 32;

			const uint32_t Middle_element_index = Top_element_index - 1;
			const uint32_t Bottom_element_index = Top_element_index - 2;

			// Pretty fast case: If the top 64 bits occupy only two elements, we can just combine those two elements:
			if (Top_element_bits == 0) {
				const int32_t Exponent = static_cast<int32_t>(Base_exponent + Bottom_element_index * 32);

				const uint64_t Mantissa = Integer_value.Mydata[Bottom_element_index]
					+ (static_cast<uint64_t>(Integer_value.Mydata[Middle_element_index]) << 32);

				bool Has_zero_tail = !Has_nonzero_fractional_part;
				for (uint32_t Ix = 0; Has_zero_tail && Ix != Bottom_element_index; ++Ix) {
					Has_zero_tail = Integer_value.Mydata[Ix] == 0;
				}

				return Assemble_floating_point_value(Mantissa, Exponent, Is_negative, Has_zero_tail, Result);
			}

			// Not quite so fast case: The top 64 bits span three elements in the Big_integer_flt. Assemble the three pieces:
			const uint32_t Top_element_mask = (1u << Top_element_bits) - 1;
			const uint32_t Top_element_shift = 64 - Top_element_bits; // Left

			const uint32_t Middle_element_shift = Top_element_shift - 32; // Left

			const uint32_t Bottom_element_bits = 32 - Top_element_bits;
			const uint32_t Bottom_element_mask = ~Top_element_mask;
			const uint32_t Bottom_element_shift = 32 - Bottom_element_bits; // Right

			const int32_t Exponent = static_cast<int32_t>(Base_exponent + Bottom_element_index * 32 + Top_element_bits);

			const uint64_t Mantissa =
				(static_cast<uint64_t>(Integer_value.Mydata[Top_element_index] & Top_element_mask) << Top_element_shift)
				+ (static_cast<uint64_t>(Integer_value.Mydata[Middle_element_index]) << Middle_element_shift)
				+ (static_cast<uint64_t>(Integer_value.Mydata[Bottom_element_index] & Bottom_element_mask)
					>> Bottom_element_shift);

			bool Has_zero_tail =
				!Has_nonzero_fractional_part && (Integer_value.Mydata[Bottom_element_index] & Top_element_mask) == 0;

			for (uint32_t Ix = 0; Has_zero_tail && Ix != Bottom_element_index; ++Ix) {
				Has_zero_tail = Integer_value.Mydata[Ix] == 0;
			}

			return Assemble_floating_point_value(Mantissa, Exponent, Is_negative, Has_zero_tail, Result);
		}

		inline void Accumulate_decimal_digits_into_big_integer_flt(
			const uint8_t* const First_digit, const uint8_t* const Last_digit, Big_integer_flt& Result) noexcept {
			// We accumulate nine digit chunks, transforming the base ten string into base one billion on the fly,
			// allowing us to reduce the number of high-precision multiplication and addition operations by 8/9.
			uint32_t Accumulator = 0;
			uint32_t Accumulator_count = 0;
			for (const uint8_t* It = First_digit; It != Last_digit; ++It) {
				if (Accumulator_count == 9) {
					[[maybe_unused]] const bool Success1 = Multiply(Result, 1'000'000'000); // assumes no overflow
					assert(Success1);
					[[maybe_unused]] const bool Success2 = Add(Result, Accumulator); // assumes no overflow
					assert(Success2);

					Accumulator = 0;
					Accumulator_count = 0;
				}

				Accumulator *= 10;
				Accumulator += *It;
				++Accumulator_count;
			}

			if (Accumulator_count != 0) {
				[[maybe_unused]] const bool Success3 =
					Multiply_by_power_of_ten(Result, Accumulator_count); // assumes no overflow
				assert(Success3);
				[[maybe_unused]] const bool Success4 = Add(Result, Accumulator); // assumes no overflow
				assert(Success4);
			}
		}

		template <class FloatingType>
		[[nodiscard]] errc Convert_decimal_string_to_floating_type(
			const Floating_point_string& Data, FloatingType& Result, bool Has_zero_tail) noexcept {
			using Traits = Floating_type_traits<FloatingType>;

			// To generate an N bit mantissa we require N + 1 bits of precision. The extra bit is used to correctly round
			// the mantissa (if there are fewer bits than this available, then that's totally okay;
			// in that case we use what we have and we don't need to round).
			const uint32_t Required_bits_of_precision = static_cast<uint32_t>(Traits::Mantissa_bits + 1);

			// The input is of the form 0.mantissa * 10^exponent, where 'mantissa' are the decimal digits of the mantissa
			// and 'exponent' is the decimal exponent. We decompose the mantissa into two parts: an integer part and a
			// fractional part. If the exponent is positive, then the integer part consists of the first 'exponent' digits,
			// or all present digits if there are fewer digits. If the exponent is zero or negative, then the integer part
			// is empty. In either case, the remaining digits form the fractional part of the mantissa.
			const uint32_t Positive_exponent = static_cast<uint32_t>((std::max)(0, Data.Myexponent));
			const uint32_t Integer_digits_present = (std::min)(Positive_exponent, Data.Mymantissa_count);
			const uint32_t Integer_digits_missing = Positive_exponent - Integer_digits_present;
			const uint8_t* const Integer_first = Data.Mymantissa;
			const uint8_t* const Integer_last = Data.Mymantissa + Integer_digits_present;

			const uint8_t* const Fractional_first = Integer_last;
			const uint8_t* const Fractional_last = Data.Mymantissa + Data.Mymantissa_count;
			const uint32_t Fractional_digits_present = static_cast<uint32_t>(Fractional_last - Fractional_first);

			// First, we accumulate the integer part of the mantissa into a Big_integer_flt:
			Big_integer_flt Integer_value { };
			Accumulate_decimal_digits_into_big_integer_flt(Integer_first, Integer_last, Integer_value);

			if (Integer_digits_missing > 0) {
				if (!Multiply_by_power_of_ten(Integer_value, Integer_digits_missing)) {
					Assemble_floating_point_infinity(Data.Myis_negative, Result);
					return errc::result_out_of_range; // Overflow example: "1e+2000"
				}
			}

			// At this point, the Integer_value contains the value of the integer part of the mantissa. If either
			// [1] this number has more than the required number of bits of precision or
			// [2] the mantissa has no fractional part, then we can assemble the result immediately:
			const uint32_t Integer_bits_of_precision = Bit_scan_reverse(Integer_value);
			{
				const bool Has_zero_fractional_part = Fractional_digits_present == 0 && Has_zero_tail;

				if (Integer_bits_of_precision >= Required_bits_of_precision || Has_zero_fractional_part) {
					return Assemble_floating_point_value_from_big_integer_flt(
						Integer_value, Integer_bits_of_precision, Data.Myis_negative, !Has_zero_fractional_part, Result);
				}
			}

			// Otherwise, we did not get enough bits of precision from the integer part, and the mantissa has a fractional
			// part. We parse the fractional part of the mantissa to obtain more bits of precision. To do this, we convert
			// the fractional part into an actual fraction N/M, where the numerator N is computed from the digits of the
			// fractional part, and the denominator M is computed as the power of 10 such that N/M is equal to the value
			// of the fractional part of the mantissa.
			Big_integer_flt Fractional_numerator { };
			Accumulate_decimal_digits_into_big_integer_flt(Fractional_first, Fractional_last, Fractional_numerator);

			const uint32_t Fractional_denominator_exponent =
				Data.Myexponent < 0 ? Fractional_digits_present + static_cast<uint32_t>(-Data.Myexponent)
				: Fractional_digits_present;

			Big_integer_flt Fractional_denominator = Make_big_integer_flt_one();
			if (!Multiply_by_power_of_ten(Fractional_denominator, Fractional_denominator_exponent)) {
				// If there were any digits in the integer part, it is impossible to underflow (because the exponent
				// cannot possibly be small enough), so if we underflow here it is a true underflow and we return zero.
				Assemble_floating_point_zero(Data.Myis_negative, Result);
				return errc::result_out_of_range; // Underflow example: "1e-2000"
			}

			// Because we are using only the fractional part of the mantissa here, the numerator is guaranteed to be smaller
			// than the denominator. We normalize the fraction such that the most significant bit of the numerator is in the
			// same position as the most significant bit in the denominator. This ensures that when we later shift the
			// numerator N bits to the left, we will produce N bits of precision.
			const uint32_t Fractional_numerator_bits = Bit_scan_reverse(Fractional_numerator);
			const uint32_t Fractional_denominator_bits = Bit_scan_reverse(Fractional_denominator);

			const uint32_t Fractional_shift = Fractional_denominator_bits > Fractional_numerator_bits
				? Fractional_denominator_bits - Fractional_numerator_bits
				: 0;

			if (Fractional_shift > 0) {
				[[maybe_unused]] const bool Shift_success1 =
					Shift_left(Fractional_numerator, Fractional_shift); // assumes no overflow
				assert(Shift_success1);
			}

			const uint32_t Required_fractional_bits_of_precision = Required_bits_of_precision - Integer_bits_of_precision;

			uint32_t Remaining_bits_of_precision_required = Required_fractional_bits_of_precision;
			if (Integer_bits_of_precision > 0) {
				// If the fractional part of the mantissa provides no bits of precision and cannot affect rounding,
				// we can just take whatever bits we got from the integer part of the mantissa. This is the case for numbers
				// like 5.0000000000000000000001, where the significant digits of the fractional part start so far to the
				// right that they do not affect the floating-point representation.

				// If the fractional shift is exactly equal to the number of bits of precision that we require,
				// then no fractional bits will be part of the result, but the result may affect rounding.
				// This is e.g. the case for large, odd integers with a fractional part greater than or equal to .5.
				// Thus, we need to do the division to correctly round the result.
				if (Fractional_shift > Remaining_bits_of_precision_required) {
					return Assemble_floating_point_value_from_big_integer_flt(Integer_value, Integer_bits_of_precision,
						Data.Myis_negative, Fractional_digits_present != 0 || !Has_zero_tail, Result);
				}

				Remaining_bits_of_precision_required -= Fractional_shift;
			}

			// If there was no integer part of the mantissa, we will need to compute the exponent from the fractional part.
			// The fractional exponent is the power of two by which we must multiply the fractional part to move it into the
			// range [1.0, 2.0). This will either be the same as the shift we computed earlier, or one greater than that shift:
			const uint32_t Fractional_exponent =
				Fractional_numerator < Fractional_denominator ? Fractional_shift + 1 : Fractional_shift;

			[[maybe_unused]] const bool Shift_success2 =
				Shift_left(Fractional_numerator, Remaining_bits_of_precision_required); // assumes no overflow
			assert(Shift_success2);

			uint64_t Fractional_mantissa = Divide(Fractional_numerator, Fractional_denominator);

			Has_zero_tail = Has_zero_tail && Fractional_numerator.Myused == 0;

			// We may have produced more bits of precision than were required. Check, and remove any "extra" bits:
			const uint32_t Fractional_mantissa_bits = Bit_scan_reverse(Fractional_mantissa);
			if (Fractional_mantissa_bits > Required_fractional_bits_of_precision) {
				const uint32_t Shift = Fractional_mantissa_bits - Required_fractional_bits_of_precision;
				Has_zero_tail = Has_zero_tail && (Fractional_mantissa & ((1ULL << Shift) - 1)) == 0;
				Fractional_mantissa >>= Shift;
			}

			// Compose the mantissa from the integer and fractional parts:
			const uint32_t Integer_mantissa_low = Integer_value.Myused > 0 ? Integer_value.Mydata[0] : 0;
			const uint32_t Integer_mantissa_high = Integer_value.Myused > 1 ? Integer_value.Mydata[1] : 0;
			const uint64_t Integer_mantissa = Integer_mantissa_low + (static_cast<uint64_t>(Integer_mantissa_high) << 32);

			const uint64_t Complete_mantissa =
				(Integer_mantissa << Required_fractional_bits_of_precision) + Fractional_mantissa;

			// Compute the final exponent:
			// * If the mantissa had an integer part, then the exponent is one less than the number of bits we obtained
			// from the integer part. (It's one less because we are converting to the form 1.11111,
			// with one 1 to the left of the decimal point.)
			// * If the mantissa had no integer part, then the exponent is the fractional exponent that we computed.
			// Then, in both cases, we subtract an additional one from the exponent,
			// to account for the fact that we've generated an extra bit of precision, for use in rounding.
			const int32_t Final_exponent = Integer_bits_of_precision > 0
				? static_cast<int32_t>(Integer_bits_of_precision - 2)
				: -static_cast<int32_t>(Fractional_exponent) - 1;

			return Assemble_floating_point_value(
				Complete_mantissa, Final_exponent, Data.Myis_negative, Has_zero_tail, Result);
		}

		template <class FloatingType>
		[[nodiscard]] errc Convert_hexadecimal_string_to_floating_type(
			const Floating_point_string& Data, FloatingType& Result, bool Has_zero_tail) noexcept {
			using Traits = Floating_type_traits<FloatingType>;

			uint64_t Mantissa = 0;
			int32_t Exponent = Data.Myexponent + Traits::Mantissa_bits - 1;

			// Accumulate bits into the mantissa buffer
			const uint8_t* const Mantissa_last = Data.Mymantissa + Data.Mymantissa_count;
			const uint8_t* Mantissa_it = Data.Mymantissa;
			while (Mantissa_it != Mantissa_last && Mantissa <= Traits::Normal_mantissa_mask) {
				Mantissa *= 16;
				Mantissa += *Mantissa_it++;
				Exponent -= 4; // The exponent is in binary; log2(16) == 4
			}

			while (Has_zero_tail && Mantissa_it != Mantissa_last) {
				Has_zero_tail = *Mantissa_it++ == 0;
			}

			return Assemble_floating_point_value(Mantissa, Exponent, Data.Myis_negative, Has_zero_tail, Result);
		}

		template <class Floating, class CharT>
		[[nodiscard]] from_chars_result<CharT> Ordinary_floating_from_chars(const CharT* const First, const CharT* const Last,
			Floating& Value, const chars_format Fmt, const bool Minus_sign, const CharT* Next) noexcept {
			// vvvvvvvvvv DERIVED FROM corecrt_internal_strtox.h WITH SIGNIFICANT MODIFICATIONS vvvvvvvvvv

			const bool Is_hexadecimal = Fmt == chars_format::hex;
			const int Base { Is_hexadecimal ? 16 : 10 };

			// PERFORMANCE NOTE: Fp_string is intentionally left uninitialized. Zero-initialization is quite expensive
			// and is unnecessary. The benefit of not zero-initializing is greatest for short inputs.
			Floating_point_string Fp_string;

			// Record the optional minus sign:
			Fp_string.Myis_negative = Minus_sign;

			uint8_t* const Mantissa_first = Fp_string.Mymantissa;
			uint8_t* const Mantissa_last = std::end(Fp_string.Mymantissa);
			uint8_t* Mantissa_it = Mantissa_first;

			// [Whole_begin, Whole_end) will contain 0 or more digits/hexits
			const auto* const Whole_begin = Next;

			// Skip past any leading zeroes in the mantissa:
			for (; Next != Last && *Next == '0'; ++Next) {
			}
			const auto* const Leading_zero_end = Next;

			// Scan the integer part of the mantissa:
			for (; Next != Last; ++Next) {
				const unsigned char Digit_value = Digit_from_char(*Next);

				if (Digit_value >= Base) {
					break;
				}

				if (Mantissa_it != Mantissa_last) {
					*Mantissa_it++ = Digit_value;
				}
			}
			const auto* const Whole_end = Next;

			// Defend against Exponent_adjustment integer overflow. (These values don't need to be strict.)
			constexpr ptrdiff_t Maximum_adjustment = 1'000'000;
			constexpr ptrdiff_t Minimum_adjustment = -1'000'000;

			// The exponent adjustment holds the number of digits in the mantissa buffer that appeared before the radix point.
			// It can be negative, and leading zeroes in the integer part are ignored. Examples:
			// For "03333.111", it is 4.
			// For "00000.111", it is 0.
			// For "00000.001", it is -2.
			int Exponent_adjustment = static_cast<int>((std::min)(Whole_end - Leading_zero_end, Maximum_adjustment));

			// [Whole_end, Dot_end) will contain 0 or 1 '.' characters
			if (Next != Last && *Next == '.') {
				++Next;
			}
			const auto* const Dot_end = Next;

			// [Dot_end, Frac_end) will contain 0 or more digits/hexits

			// If we haven't yet scanned any nonzero digits, continue skipping over zeroes,
			// updating the exponent adjustment to account for the zeroes we are skipping:
			if (Exponent_adjustment == 0) {
				for (; Next != Last && *Next == '0'; ++Next) {
				}

				Exponent_adjustment = static_cast<int>((std::max)(Dot_end - Next, Minimum_adjustment));
			}

			// Scan the fractional part of the mantissa:
			bool Has_zero_tail = true;

			for (; Next != Last; ++Next) {
				const unsigned char Digit_value = Digit_from_char(*Next);

				if (Digit_value >= Base) {
					break;
				}

				if (Mantissa_it != Mantissa_last) {
					*Mantissa_it++ = Digit_value;
				}
				else {
					Has_zero_tail = Has_zero_tail && Digit_value == 0;
				}
			}
			const auto* const Frac_end = Next;

			// We must have at least 1 digit/hexit
			if (Whole_begin == Whole_end && Dot_end == Frac_end) {
				return { First, errc::invalid_argument };
			}

			const char Exponent_prefix { Is_hexadecimal ? 'p' : 'e' };

			bool Exponent_is_negative = false;
			int Exponent = 0;

			constexpr int Maximum_temporary_decimal_exponent = 5200;
			constexpr int Minimum_temporary_decimal_exponent = -5200;

			if (Fmt != chars_format::fixed // N4713 23.20.3 [charconv.from.chars]/7.3
											// "if fmt has chars_format::fixed set but not chars_format::scientific,
											// the optional exponent part shall not appear"
				&& Next != Last && (static_cast<unsigned char>(*Next) | 0x20) == Exponent_prefix) { // found exponent prefix
				const auto* Unread = Next + 1;

				if (Unread != Last && (*Unread == '+' || *Unread == '-')) { // found optional sign
					Exponent_is_negative = *Unread == '-';
					++Unread;
				}

				while (Unread != Last) {
					const unsigned char Digit_value = Digit_from_char(*Unread);

					if (Digit_value >= 10) {
						break;
					}

					// found decimal digit

					if (Exponent <= Maximum_temporary_decimal_exponent) {
						Exponent = Exponent * 10 + Digit_value;
					}

					++Unread;
					Next = Unread; // consume exponent-part/binary-exponent-part
				}

				if (Exponent_is_negative) {
					Exponent = -Exponent;
				}
			}

			// [Frac_end, Exponent_end) will either be empty or contain "[EPep] sign[opt] digit-sequence"
			const auto* const Exponent_end = Next;

			if (Fmt == chars_format::scientific
				&& Frac_end == Exponent_end) { // N4713 23.20.3 [charconv.from.chars]/7.2
												 // "if fmt has chars_format::scientific set but not chars_format::fixed,
												 // the otherwise optional exponent part shall appear"
				return { First, errc::invalid_argument };
			}

			// Remove trailing zeroes from mantissa:
			while (Mantissa_it != Mantissa_first && *(Mantissa_it - 1) == 0) {
				--Mantissa_it;
			}

			// If the mantissa buffer is empty, the mantissa was composed of all zeroes (so the mantissa is 0).
			// All such strings have the value zero, regardless of what the exponent is (because 0 * b^n == 0 for all b and n).
			// We can return now. Note that we defer this check until after we scan the exponent, so that we can correctly
			// update Next to point past the end of the exponent.
			if (Mantissa_it == Mantissa_first) {
				assert(Has_zero_tail);
				Assemble_floating_point_zero(Fp_string.Myis_negative, Value);
				return { Next, errc { } };
			}

			// Before we adjust the exponent, handle the case where we detected a wildly
			// out of range exponent during parsing and clamped the value:
			if (Exponent > Maximum_temporary_decimal_exponent) {
				Assemble_floating_point_infinity(Fp_string.Myis_negative, Value);
				return { Next, errc::result_out_of_range }; // Overflow example: "1e+9999"
			}

			if (Exponent < Minimum_temporary_decimal_exponent) {
				Assemble_floating_point_zero(Fp_string.Myis_negative, Value);
				return { Next, errc::result_out_of_range }; // Underflow example: "1e-9999"
			}

			// In hexadecimal floating constants, the exponent is a base 2 exponent. The exponent adjustment computed during
			// parsing has the same base as the mantissa (so, 16 for hexadecimal floating constants).
			// We therefore need to scale the base 16 multiplier to base 2 by multiplying by log2(16):
			const int Exponent_adjustment_multiplier { Is_hexadecimal ? 4 : 1 };

			Exponent += Exponent_adjustment * Exponent_adjustment_multiplier;

			// Verify that after adjustment the exponent isn't wildly out of range (if it is, it isn't representable
			// in any supported floating-point format).
			if (Exponent > Maximum_temporary_decimal_exponent) {
				Assemble_floating_point_infinity(Fp_string.Myis_negative, Value);
				return { Next, errc::result_out_of_range }; // Overflow example: "10e+5199"
			}

			if (Exponent < Minimum_temporary_decimal_exponent) {
				Assemble_floating_point_zero(Fp_string.Myis_negative, Value);
				return { Next, errc::result_out_of_range }; // Underflow example: "0.001e-5199"
			}

			Fp_string.Myexponent = Exponent;
			Fp_string.Mymantissa_count = static_cast<uint32_t>(Mantissa_it - Mantissa_first);

			if (Is_hexadecimal) {
				const errc Ec = Convert_hexadecimal_string_to_floating_type(Fp_string, Value, Has_zero_tail);
				return { Next, Ec };
			}
			else {
				const errc Ec = Convert_decimal_string_to_floating_type(Fp_string, Value, Has_zero_tail);
				return { Next, Ec };
			}

			// ^^^^^^^^^^ DERIVED FROM corecrt_internal_strtox.h WITH SIGNIFICANT MODIFICATIONS ^^^^^^^^^^
		}

		template <class CharT>
		[[nodiscard]] inline bool Starts_with_case_insensitive(
			const CharT* First, const CharT* const Last, const char* Lowercase) noexcept {
			// pre: Lowercase contains only ['a', 'z'] and is null-terminated
			for (; First != Last && *Lowercase != '\0'; ++First, ++Lowercase) {
				if ((static_cast<unsigned char>(*First) | 0x20) != *Lowercase) {
					return false;
				}
			}

			return *Lowercase == '\0';
		}

		template <class Floating, class CharT>
		[[nodiscard]] from_chars_result<CharT> Infinity_from_chars(const CharT* const First, const CharT* const Last, Floating& Value,
			const bool Minus_sign, const CharT* Next) noexcept {
			// pre: Next points at 'i' (case-insensitively)
			if (!Starts_with_case_insensitive(Next + 1, Last, "nf")) { // definitely invalid
				return { First, errc::invalid_argument };
			}

			// definitely inf
			Next += 3;

			if (Starts_with_case_insensitive(Next, Last, "inity")) { // definitely infinity
				Next += 5;
			}

			Assemble_floating_point_infinity(Minus_sign, Value);

			return { Next, errc { } };
		}

		template <class Floating, class CharT>
		[[nodiscard]] from_chars_result<CharT> Nan_from_chars(const CharT* const First, const CharT* const Last, Floating& Value,
			bool Minus_sign, const CharT* Next) noexcept {
			// pre: Next points at 'n' (case-insensitively)
			if (!Starts_with_case_insensitive(Next + 1, Last, "an")) { // definitely invalid
				return { First, errc::invalid_argument };
			}

			// definitely nan
			Next += 3;

			bool Quiet = true;

			if (Next != Last && *Next == '(') { // possibly nan(n-char-sequence[opt])
				const auto* const Seq_begin = Next + 1;

				for (const auto* Temp = Seq_begin; Temp != Last; ++Temp) {
					if (*Temp == ')') { // definitely nan(n-char-sequence[opt])
						Next = Temp + 1;

						if (Temp - Seq_begin == 3
							&& Starts_with_case_insensitive(Seq_begin, Temp, "ind")) { // definitely nan(ind)
							// The UCRT considers indeterminate NaN to be negative quiet NaN with no payload bits set.
							// It parses "nan(ind)" and "-nan(ind)" identically.
							Minus_sign = true;
						}
						else if (Temp - Seq_begin == 4
							&& Starts_with_case_insensitive(Seq_begin, Temp, "snan")) { // definitely nan(snan)
							Quiet = false;
						}

						break;
					}
					else if (*Temp == '_' || ('0' <= *Temp && *Temp <= '9') || ('A' <= *Temp && *Temp <= 'Z')
						|| ('a' <= *Temp && *Temp <= 'z')) { // possibly nan(n-char-sequence[opt]), keep going
					}
					else { // definitely nan, not nan(n-char-sequence[opt])
						break;
					}
				}
			}

			// Intentional behavior difference between the UCRT and the STL:
			// strtod()/strtof() parse plain "nan" as being a quiet NaN with all payload bits set.
			// numeric_limits::quiet_NaN() returns a quiet NaN with no payload bits set.
			// This implementation of from_chars() has chosen to be consistent with numeric_limits.

			using Traits = Floating_type_traits<Floating>;
			using Uint_type = typename Traits::Uint_type;

			Uint_type Uint_value = Traits::Shifted_exponent_mask;

			if (Minus_sign) {
				Uint_value |= Traits::Shifted_sign_mask;
			}

			if (Quiet) {
				Uint_value |= Traits::Special_nan_mantissa_mask;
			}
			else {
				Uint_value |= 1;
			}

			Value = std::bit_cast<Floating>(Uint_value);

			return { Next, errc { } };
		}

		template <class Floating, class CharT>
		[[nodiscard]] from_chars_result<CharT> Floating_from_chars(
			const CharT* const First, const CharT* const Last, Floating& Value, const chars_format Fmt) noexcept {

			assert(Fmt == chars_format::general || Fmt == chars_format::scientific || Fmt == chars_format::fixed
				|| Fmt == chars_format::hex);

			bool Minus_sign = false;

			const auto* Next = First;

			if (Next == Last) {
				return { First, errc::invalid_argument };
			}

			if (*Next == '-') {
				Minus_sign = true;
				++Next;

				if (Next == Last) {
					return { First, errc::invalid_argument };
				}
			}

			// Distinguish ordinary numbers versus inf/nan with a single test.
			// ordinary numbers start with ['.'] ['0', '9'] ['A', 'F'] ['a', 'f']
			// inf/nan start with ['I'] ['N'] ['i'] ['n']
			// All other starting characters are invalid.
			// Setting the 0x20 bit folds these ranges in a useful manner.
			// ordinary (and some invalid) starting characters are folded to ['.'] ['0', '9'] ['a', 'f']
			// inf/nan starting characters are folded to ['i'] ['n']
			// These are ordered: ['.'] ['0', '9'] ['a', 'f'] < ['i'] ['n']
			// Note that invalid starting characters end up on both sides of this test.
			const unsigned char Folded_start = static_cast<unsigned char>(static_cast<unsigned char>(*Next) | 0x20);

			if (Folded_start <= 'f') { // possibly an ordinary number
				return Ordinary_floating_from_chars(First, Last, Value, Fmt, Minus_sign, Next);
			}
			else if (Folded_start == 'i') { // possibly inf
				return Infinity_from_chars(First, Last, Value, Minus_sign, Next);
			}
			else if (Folded_start == 'n') { // possibly nan
				return Nan_from_chars(First, Last, Value, Minus_sign, Next);
			}
			else { // definitely invalid
				return { First, errc::invalid_argument };
			}
		}
	};


	//Main converting methods.

	template<typename IntegralT> requires std::is_integral_v<IntegralT>
	[[nodiscard]] constexpr auto StrToNum(std::string_view sv, int iBase = 0)noexcept
		->STN_RETURN_TYPE(IntegralT, char)
	{
		assert(!sv.empty());

		IntegralT TData;
		const auto result = impl::Integer_from_chars(sv.data(), sv.data() + sv.size(), TData, iBase);
		if (result.ec == errc()) {
			return TData;
		}
		return STN_RETURN_VALUE(result)
	}

	template<typename IntegralT> requires std::is_integral_v<IntegralT>
	[[nodiscard]] constexpr auto StrToNum(std::wstring_view wsv, int iBase = 0)noexcept
		->STN_RETURN_TYPE(IntegralT, wchar_t)
	{
		assert(!wsv.empty());

		IntegralT TData;
		const auto result = impl::Integer_from_chars(wsv.data(), wsv.data() + wsv.size(), TData, iBase);
		if (result.ec == errc()) {
			return TData;
		}
		return STN_RETURN_VALUE(result)
	}

	template<typename FloatingT> requires std::is_floating_point_v<FloatingT>
	[[nodiscard]] auto StrToNum(std::string_view sv, chars_format fmt = chars_format::general)noexcept
		->STN_RETURN_TYPE(FloatingT, char)
	{
		assert(!sv.empty());

		FloatingT TData;
		const auto result = impl::Floating_from_chars(sv.data(), sv.data() + sv.size(), TData, fmt);
		if (result.ec == errc()) {
			return TData;
		}
		return STN_RETURN_VALUE(result)
	}

	template<typename FloatingT> requires std::is_floating_point_v<FloatingT>
	[[nodiscard]] auto StrToNum(std::wstring_view wsv, chars_format fmt = chars_format::general)noexcept
		->STN_RETURN_TYPE(FloatingT, wchar_t)
	{
		assert(!wsv.empty());

		FloatingT TData;
		const auto result = impl::Floating_from_chars(wsv.data(), wsv.data() + wsv.size(), TData, fmt);
		if (result.ec == errc()) {
			return TData;
		}
		return STN_RETURN_VALUE(result)
	}


	//Aliases with predefined types, for convenience.

	[[nodiscard]] inline constexpr auto StrToChar(std::string_view sv, int iBase = 0)noexcept
		->STN_RETURN_TYPE(char, char) {
		return StrToNum<char>(sv, iBase);
	}

	[[nodiscard]] inline constexpr auto StrToChar(std::wstring_view wsv, int iBase = 0)noexcept
		->STN_RETURN_TYPE(char, wchar_t) {
		return StrToNum<char>(wsv, iBase);
	}

	[[nodiscard]] inline constexpr auto StrToUChar(std::string_view sv, int iBase = 0)noexcept
		->STN_RETURN_TYPE(unsigned char, char) {
		return StrToNum<unsigned char>(sv, iBase);
	}

	[[nodiscard]] inline constexpr auto StrToUChar(std::wstring_view wsv, int iBase = 0)noexcept
		->STN_RETURN_TYPE(unsigned char, wchar_t) {
		return StrToNum<unsigned char>(wsv, iBase);
	}

	[[nodiscard]] inline constexpr auto StrToShort(std::string_view sv, int iBase = 0)noexcept
		->STN_RETURN_TYPE(short, char) {
		return StrToNum<short>(sv, iBase);
	}

	[[nodiscard]] inline constexpr auto StrToShort(std::wstring_view wsv, int iBase = 0)noexcept
		->STN_RETURN_TYPE(short, wchar_t) {
		return StrToNum<short>(wsv, iBase);
	}

	[[nodiscard]] inline constexpr auto StrToUShort(std::string_view sv, int iBase = 0)noexcept
		->STN_RETURN_TYPE(unsigned short, char) {
		return StrToNum<unsigned short>(sv, iBase);
	}

	[[nodiscard]] inline constexpr auto StrToUShort(std::wstring_view wsv, int iBase = 0)noexcept
		->STN_RETURN_TYPE(unsigned short, wchar_t) {
		return StrToNum<unsigned short>(wsv, iBase);
	}

	[[nodiscard]] inline constexpr auto StrToInt(std::string_view sv, int iBase = 0)noexcept
		->STN_RETURN_TYPE(int, char) {
		return StrToNum<int>(sv, iBase);
	}

	[[nodiscard]] inline constexpr auto StrToInt(std::wstring_view wsv, int iBase = 0)noexcept
		->STN_RETURN_TYPE(int, wchar_t) {
		return StrToNum<int>(wsv, iBase);
	}

	[[nodiscard]] inline constexpr auto StrToUInt(std::string_view sv, int iBase = 0)noexcept
		->STN_RETURN_TYPE(unsigned int, char) {
		return StrToNum<unsigned int>(sv, iBase);
	}

	[[nodiscard]] inline constexpr auto StrToUInt(std::wstring_view wsv, int iBase = 0)noexcept
		->STN_RETURN_TYPE(unsigned int, wchar_t) {
		return StrToNum<unsigned int>(wsv, iBase);
	}

	[[nodiscard]] inline constexpr auto StrToLL(std::string_view sv, int iBase = 0)noexcept
		->STN_RETURN_TYPE(long long, char) {
		return StrToNum<long long>(sv, iBase);
	}

	[[nodiscard]] inline constexpr auto StrToLL(std::wstring_view wsv, int iBase = 0)noexcept
		->STN_RETURN_TYPE(long long, wchar_t) {
		return StrToNum<long long>(wsv, iBase);
	}

	[[nodiscard]] inline constexpr auto StrToULL(std::string_view sv, int iBase = 0)noexcept
		->STN_RETURN_TYPE(unsigned long long, char) {
		return StrToNum<unsigned long long>(sv, iBase);
	}

	[[nodiscard]] inline constexpr auto StrToULL(std::wstring_view wsv, int iBase = 0)noexcept
		->STN_RETURN_TYPE(unsigned long long, wchar_t) {
		return StrToNum<unsigned long long>(wsv, iBase);
	}

	[[nodiscard]] inline auto StrToFloat(std::string_view sv, chars_format fmt = chars_format::general)noexcept
		->STN_RETURN_TYPE(float, char) {
		return StrToNum<float>(sv, fmt);
	}

	[[nodiscard]] inline auto StrToFloat(std::wstring_view wsv, chars_format fmt = chars_format::general)noexcept
		->STN_RETURN_TYPE(float, wchar_t) {
		return StrToNum<float>(wsv, fmt);
	}

	[[nodiscard]] inline auto StrToDouble(std::string_view sv, chars_format fmt = chars_format::general)noexcept
		->STN_RETURN_TYPE(double, char) {
		return StrToNum<double>(sv, fmt);
	}

	[[nodiscard]] inline auto StrToDouble(std::wstring_view wsv, chars_format fmt = chars_format::general)noexcept
		->STN_RETURN_TYPE(double, wchar_t) {
		return StrToNum<double>(wsv, fmt);
	}
};