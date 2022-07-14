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


namespace HEXCTRL::stn //String to Num.
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
		[[nodiscard]] inline unsigned char _Digit_from_char(const wchar_t _Ch) noexcept {
			// convert ['0', '9'] ['A', 'Z'] ['a', 'z'] to [0, 35], everything else to 255
			static constexpr unsigned char _Digit_from_byte[] = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
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
			static_assert(std::size(_Digit_from_byte) == 256);

			return _Digit_from_byte[static_cast<unsigned char>(_Ch)];
		}

		template <class _RawTy, class CharT>
		[[nodiscard]] from_chars_result<CharT> _Integer_from_chars(
			const CharT* const _First, const CharT* const _Last, _RawTy& _Raw_value, int _Base) noexcept {
			assert(_Base == 0 || (_Base >= 2 && _Base <= 36));

			bool _Minus_sign = false;

			const auto* _Next = _First;

			if constexpr (std::is_signed_v<_RawTy>) {
				if (_Next != _Last && *_Next == '-') {
					_Minus_sign = true;
					++_Next;
				}
			}

			//Checking for '0x'/'0X' prefix if _Base == 0 or 16.
			constexpr auto lmbCheck16 = [](const CharT* const pFirst, const CharT* const pLast) {
				return (pFirst != pLast && (pFirst + 1) != pLast && (*pFirst == '0' && (*(pFirst + 1) == 'x' || *(pFirst + 1) == 'X')));
			};

			if (_Base == 0) {
				if (lmbCheck16(_Next, _Last)) {
					_Next += 2;
					_Base = 16;
				}
				else {
					_Base = 10;
				}
			}
			else if (_Base == 16) { //Base16 string may or may not contain the `0x` prefix.
				if (lmbCheck16(_Next, _Last)) {
					_Next += 2;
				}
			}
			//End of '0x'/'0X' prefix checking.

			using _Unsigned = std::make_unsigned_t<_RawTy>;

			constexpr _Unsigned _Uint_max = static_cast<_Unsigned>(-1);
			constexpr _Unsigned _Int_max = static_cast<_Unsigned>(_Uint_max >> 1);
			constexpr _Unsigned _Abs_int_min = static_cast<_Unsigned>(_Int_max + 1);

			_Unsigned _Risky_val;
			_Unsigned _Max_digit;

			if constexpr (std::is_signed_v<_RawTy>) {
				if (_Minus_sign) {
					_Risky_val = static_cast<_Unsigned>(_Abs_int_min / _Base);
					_Max_digit = static_cast<_Unsigned>(_Abs_int_min % _Base);
				}
				else {
					_Risky_val = static_cast<_Unsigned>(_Int_max / _Base);
					_Max_digit = static_cast<_Unsigned>(_Int_max % _Base);
				}
			}
			else {
				_Risky_val = static_cast<_Unsigned>(_Uint_max / _Base);
				_Max_digit = static_cast<_Unsigned>(_Uint_max % _Base);
			}

			_Unsigned _Value = 0;

			bool _Overflowed = false;

			for (; _Next != _Last; ++_Next) {
				const unsigned char _Digit = _Digit_from_char(*_Next);

				if (_Digit >= _Base) {
					break;
				}

				if (_Value < _Risky_val // never overflows
					|| (_Value == _Risky_val && _Digit <= _Max_digit)) { // overflows for certain digits
					_Value = static_cast<_Unsigned>(_Value * _Base + _Digit);
				}
				else { // _Value > _Risky_val always overflows
					_Overflowed = true; // keep going, _Next still needs to be updated, _Value is now irrelevant
				}
			}

			if (_Next - _First == static_cast<ptrdiff_t>(_Minus_sign)) {
				return { _First, errc::invalid_argument };
			}

			if (_Overflowed) {
				return { _Next, errc::result_out_of_range };
			}

			if constexpr (std::is_signed_v<_RawTy>) {
				if (_Minus_sign) {
					_Value = static_cast<_Unsigned>(0 - _Value);
				}
			}

			_Raw_value = static_cast<_RawTy>(_Value); // implementation-defined for negative, N4713 7.8 [conv.integral]/3

			return { _Next, errc { } };
		}

		template <class _FloatingType>
		struct _Floating_type_traits;

		template <>
		struct _Floating_type_traits<float> {
			static constexpr int32_t _Mantissa_bits = 24; // FLT_MANT_DIG
			static constexpr int32_t _Exponent_bits = 8; // sizeof(float) * CHAR_BIT - FLT_MANT_DIG
			static constexpr int32_t _Maximum_binary_exponent = 127; // FLT_MAX_EXP - 1
			static constexpr int32_t _Minimum_binary_exponent = -126; // FLT_MIN_EXP - 1
			static constexpr int32_t _Exponent_bias = 127;
			static constexpr int32_t _Sign_shift = 31; // _Exponent_bits + _Mantissa_bits - 1
			static constexpr int32_t _Exponent_shift = 23; // _Mantissa_bits - 1

			using _Uint_type = uint32_t;

			static constexpr uint32_t _Exponent_mask = 0x000000FFu; // (1u << _Exponent_bits) - 1
			static constexpr uint32_t _Normal_mantissa_mask = 0x00FFFFFFu; // (1u << _Mantissa_bits) - 1
			static constexpr uint32_t _Denormal_mantissa_mask = 0x007FFFFFu; // (1u << (_Mantissa_bits - 1)) - 1
			static constexpr uint32_t _Special_nan_mantissa_mask = 0x00400000u; // 1u << (_Mantissa_bits - 2)
			static constexpr uint32_t _Shifted_sign_mask = 0x80000000u; // 1u << _Sign_shift
			static constexpr uint32_t _Shifted_exponent_mask = 0x7F800000u; // _Exponent_mask << _Exponent_shift
		};

		template <>
		struct _Floating_type_traits<double> {
			static constexpr int32_t _Mantissa_bits = 53; // DBL_MANT_DIG
			static constexpr int32_t _Exponent_bits = 11; // sizeof(double) * CHAR_BIT - DBL_MANT_DIG
			static constexpr int32_t _Maximum_binary_exponent = 1023; // DBL_MAX_EXP - 1
			static constexpr int32_t _Minimum_binary_exponent = -1022; // DBL_MIN_EXP - 1
			static constexpr int32_t _Exponent_bias = 1023;
			static constexpr int32_t _Sign_shift = 63; // _Exponent_bits + _Mantissa_bits - 1
			static constexpr int32_t _Exponent_shift = 52; // _Mantissa_bits - 1

			using _Uint_type = uint64_t;

			static constexpr uint64_t _Exponent_mask = 0x00000000000007FFu; // (1ULL << _Exponent_bits) - 1
			static constexpr uint64_t _Normal_mantissa_mask = 0x001FFFFFFFFFFFFFu; // (1ULL << _Mantissa_bits) - 1
			static constexpr uint64_t _Denormal_mantissa_mask = 0x000FFFFFFFFFFFFFu; // (1ULL << (_Mantissa_bits - 1)) - 1
			static constexpr uint64_t _Special_nan_mantissa_mask = 0x0008000000000000u; // 1ULL << (_Mantissa_bits - 2)
			static constexpr uint64_t _Shifted_sign_mask = 0x8000000000000000u; // 1ULL << _Sign_shift
			static constexpr uint64_t _Shifted_exponent_mask = 0x7FF0000000000000u; // _Exponent_mask << _Exponent_shift
		};

		template <>
		struct _Floating_type_traits<long double> : _Floating_type_traits<double> {};

		struct _Big_integer_flt {
			_Big_integer_flt() noexcept : _Myused(0) {}

			_Big_integer_flt(const _Big_integer_flt& _Other) noexcept : _Myused(_Other._Myused) {
				std::memcpy(_Mydata, _Other._Mydata, _Other._Myused * sizeof(uint32_t));
			}

			_Big_integer_flt& operator=(const _Big_integer_flt& _Other) noexcept {
				_Myused = _Other._Myused;
				std::memmove(_Mydata, _Other._Mydata, _Other._Myused * sizeof(uint32_t));
				return *this;
			}

			[[nodiscard]] bool operator<(const _Big_integer_flt& _Rhs) const noexcept {
				if (_Myused != _Rhs._Myused) {
					return _Myused < _Rhs._Myused;
				}

				for (uint32_t _Ix = _Myused - 1; _Ix != static_cast<uint32_t>(-1); --_Ix) {
					if (_Mydata[_Ix] != _Rhs._Mydata[_Ix]) {
						return _Mydata[_Ix] < _Rhs._Mydata[_Ix];
					}
				}

				return false;
			}

			static constexpr uint32_t _Maximum_bits = 1074 // 1074 bits required to represent 2^1074
				+ 2552 // ceil(log2(10^768))
				+ 54; // shift space

			static constexpr uint32_t _Element_bits = 32;

			static constexpr uint32_t _Element_count = (_Maximum_bits + _Element_bits - 1) / _Element_bits;

			uint32_t _Myused; // The number of elements currently in use
			uint32_t _Mydata[_Element_count]; // The number, stored in little-endian form
		};

		[[nodiscard]] inline _Big_integer_flt _Make_big_integer_flt_one() noexcept {
			_Big_integer_flt _Xval { };
			_Xval._Mydata[0] = 1;
			_Xval._Myused = 1;
			return _Xval;
		}

		[[nodiscard]] inline uint32_t _Bit_scan_reverse(const uint32_t _Value) noexcept {
			unsigned long _Index; // Intentionally uninitialized for better codegen

			if (_BitScanReverse(&_Index, _Value)) {
				return _Index + 1;
			}

			return 0;
		}

		[[nodiscard]] inline uint32_t _Bit_scan_reverse(const uint64_t _Value) noexcept {
			unsigned long _Index; // Intentionally uninitialized for better codegen

#ifdef _WIN64
			if (_BitScanReverse64(&_Index, _Value)) {
				return _Index + 1;
			}
#else // ^^^ 64-bit ^^^ / vvv 32-bit vvv
			uint32_t _Ui32 = static_cast<uint32_t>(_Value >> 32);

			if (_BitScanReverse(&_Index, _Ui32)) {
				return _Index + 1 + 32;
			}

			_Ui32 = static_cast<uint32_t>(_Value);

			if (_BitScanReverse(&_Index, _Ui32)) {
				return _Index + 1;
			}
#endif // ^^^ 32-bit ^^^

			return 0;
		}

		[[nodiscard]] inline uint32_t _Bit_scan_reverse(const _Big_integer_flt& _Xval) noexcept {
			if (_Xval._Myused == 0) {
				return 0;
			}

			const uint32_t _Bx = _Xval._Myused - 1;

			assert(_Xval._Mydata[_Bx] != 0); // _Big_integer_flt should always be trimmed

			unsigned long _Index; // Intentionally uninitialized for better codegen

			_BitScanReverse(&_Index, _Xval._Mydata[_Bx]); // assumes _Xval._Mydata[_Bx] != 0

			return _Index + 1 + _Bx * _Big_integer_flt::_Element_bits;
		}

		[[nodiscard]] inline bool _Shift_left(_Big_integer_flt& _Xval, const uint32_t _Nx) noexcept {
			if (_Xval._Myused == 0) {
				return true;
			}

			const uint32_t _Unit_shift = _Nx / _Big_integer_flt::_Element_bits;
			const uint32_t _Bit_shift = _Nx % _Big_integer_flt::_Element_bits;

			if (_Xval._Myused + _Unit_shift > _Big_integer_flt::_Element_count) {
				// Unit shift will overflow.
				_Xval._Myused = 0;
				return false;
			}

			if (_Bit_shift == 0) {
				std::memmove(_Xval._Mydata + _Unit_shift, _Xval._Mydata, _Xval._Myused * sizeof(uint32_t));
				_Xval._Myused += _Unit_shift;
			}
			else {
				const bool _Bit_shifts_into_next_unit =
					_Bit_shift > (_Big_integer_flt::_Element_bits - _Bit_scan_reverse(_Xval._Mydata[_Xval._Myused - 1]));

				const uint32_t _New_used = _Xval._Myused + _Unit_shift + static_cast<uint32_t>(_Bit_shifts_into_next_unit);

				if (_New_used > _Big_integer_flt::_Element_count) {
					// Bit shift will overflow.
					_Xval._Myused = 0;
					return false;
				}

				const uint32_t _Msb_bits = _Bit_shift;
				const uint32_t _Lsb_bits = _Big_integer_flt::_Element_bits - _Msb_bits;

				const uint32_t _Lsb_mask = (1UL << _Lsb_bits) - 1UL;
				const uint32_t _Msb_mask = ~_Lsb_mask;

				// If _Unit_shift == 0, this will wraparound, which is okay.
				for (uint32_t _Dest_index = _New_used - 1; _Dest_index != _Unit_shift - 1; --_Dest_index) {
					// performance note: PSLLDQ and PALIGNR instructions could be more efficient here

					// If _Bit_shifts_into_next_unit, the first iteration will trigger the bounds check below, which is okay.
					const uint32_t _Upper_source_index = _Dest_index - _Unit_shift;

					// When _Dest_index == _Unit_shift, this will wraparound, which is okay (see bounds check below).
					const uint32_t _Lower_source_index = _Dest_index - _Unit_shift - 1;

					const uint32_t _Upper_source = _Upper_source_index < _Xval._Myused ? _Xval._Mydata[_Upper_source_index] : 0;
					const uint32_t _Lower_source = _Lower_source_index < _Xval._Myused ? _Xval._Mydata[_Lower_source_index] : 0;

					const uint32_t _Shifted_upper_source = (_Upper_source & _Lsb_mask) << _Msb_bits;
					const uint32_t _Shifted_lower_source = (_Lower_source & _Msb_mask) >> _Lsb_bits;

					const uint32_t _Combined_shifted_source = _Shifted_upper_source | _Shifted_lower_source;

					_Xval._Mydata[_Dest_index] = _Combined_shifted_source;
				}

				_Xval._Myused = _New_used;
			}

			std::memset(_Xval._Mydata, 0, _Unit_shift * sizeof(uint32_t));

			return true;
		}

		[[nodiscard]] inline bool _Add(_Big_integer_flt& _Xval, const uint32_t _Value) noexcept {
			if (_Value == 0) {
				return true;
			}

			uint32_t _Carry = _Value;
			for (uint32_t _Ix = 0; _Ix != _Xval._Myused; ++_Ix) {
				const uint64_t _Result = static_cast<uint64_t>(_Xval._Mydata[_Ix]) + _Carry;
				_Xval._Mydata[_Ix] = static_cast<uint32_t>(_Result);
				_Carry = static_cast<uint32_t>(_Result >> 32);
			}

			if (_Carry != 0) {
				if (_Xval._Myused < _Big_integer_flt::_Element_count) {
					_Xval._Mydata[_Xval._Myused] = _Carry;
					++_Xval._Myused;
				}
				else {
					_Xval._Myused = 0;
					return false;
				}
			}

			return true;
		}

		[[nodiscard]] inline uint32_t _Add_carry(uint32_t& _Ux1, const uint32_t _Ux2, const uint32_t _U_carry) noexcept {
			const uint64_t _Uu = static_cast<uint64_t>(_Ux1) + _Ux2 + _U_carry;
			_Ux1 = static_cast<uint32_t>(_Uu);
			return static_cast<uint32_t>(_Uu >> 32);
		}

		[[nodiscard]] inline uint32_t _Add_multiply_carry(
			uint32_t& _U_add, const uint32_t _U_mul_1, const uint32_t _U_mul_2, const uint32_t _U_carry) noexcept {
			const uint64_t _Uu_res = static_cast<uint64_t>(_U_mul_1) * _U_mul_2 + _U_add + _U_carry;
			_U_add = static_cast<uint32_t>(_Uu_res);
			return static_cast<uint32_t>(_Uu_res >> 32);
		}

		[[nodiscard]] inline uint32_t _Multiply_core(
			uint32_t* const _Multiplicand, const uint32_t _Multiplicand_count, const uint32_t _Multiplier) noexcept {
			uint32_t _Carry = 0;
			for (uint32_t _Ix = 0; _Ix != _Multiplicand_count; ++_Ix) {
				const uint64_t _Result = static_cast<uint64_t>(_Multiplicand[_Ix]) * _Multiplier + _Carry;
				_Multiplicand[_Ix] = static_cast<uint32_t>(_Result);
				_Carry = static_cast<uint32_t>(_Result >> 32);
			}

			return _Carry;
		}

		[[nodiscard]] inline bool _Multiply(_Big_integer_flt& _Multiplicand, const uint32_t _Multiplier) noexcept {
			if (_Multiplier == 0) {
				_Multiplicand._Myused = 0;
				return true;
			}

			if (_Multiplier == 1) {
				return true;
			}

			if (_Multiplicand._Myused == 0) {
				return true;
			}

			const uint32_t _Carry = _Multiply_core(_Multiplicand._Mydata, _Multiplicand._Myused, _Multiplier);
			if (_Carry != 0) {
				if (_Multiplicand._Myused < _Big_integer_flt::_Element_count) {
					_Multiplicand._Mydata[_Multiplicand._Myused] = _Carry;
					++_Multiplicand._Myused;
				}
				else {
					_Multiplicand._Myused = 0;
					return false;
				}
			}

			return true;
		}

		[[nodiscard]] inline bool _Multiply(_Big_integer_flt& _Multiplicand, const _Big_integer_flt& _Multiplier) noexcept {
			if (_Multiplicand._Myused == 0) {
				return true;
			}

			if (_Multiplier._Myused == 0) {
				_Multiplicand._Myused = 0;
				return true;
			}

			if (_Multiplier._Myused == 1) {
				return _Multiply(_Multiplicand, _Multiplier._Mydata[0]); // when overflow occurs, resets to zero
			}

			if (_Multiplicand._Myused == 1) {
				const uint32_t _Small_multiplier = _Multiplicand._Mydata[0];
				_Multiplicand = _Multiplier;
				return _Multiply(_Multiplicand, _Small_multiplier); // when overflow occurs, resets to zero
			}

			// We prefer more iterations on the inner loop and fewer on the outer:
			const bool _Multiplier_is_shorter = _Multiplier._Myused < _Multiplicand._Myused;
			const uint32_t* const _Rgu1 = _Multiplier_is_shorter ? _Multiplier._Mydata : _Multiplicand._Mydata;
			const uint32_t* const _Rgu2 = _Multiplier_is_shorter ? _Multiplicand._Mydata : _Multiplier._Mydata;

			const uint32_t _Cu1 = _Multiplier_is_shorter ? _Multiplier._Myused : _Multiplicand._Myused;
			const uint32_t _Cu2 = _Multiplier_is_shorter ? _Multiplicand._Myused : _Multiplier._Myused;

			_Big_integer_flt _Result { };
			for (uint32_t _Iu1 = 0; _Iu1 != _Cu1; ++_Iu1) {
				const uint32_t _U_cur = _Rgu1[_Iu1];
				if (_U_cur == 0) {
					if (_Iu1 == _Result._Myused) {
						_Result._Mydata[_Iu1] = 0;
						_Result._Myused = _Iu1 + 1;
					}

					continue;
				}

				uint32_t _U_carry = 0;
				uint32_t _Iu_res = _Iu1;
				for (uint32_t _Iu2 = 0; _Iu2 != _Cu2 && _Iu_res != _Big_integer_flt::_Element_count; ++_Iu2, ++_Iu_res) {
					if (_Iu_res == _Result._Myused) {
						_Result._Mydata[_Iu_res] = 0;
						_Result._Myused = _Iu_res + 1;
					}

					_U_carry = _Add_multiply_carry(_Result._Mydata[_Iu_res], _U_cur, _Rgu2[_Iu2], _U_carry);
				}

				while (_U_carry != 0 && _Iu_res != _Big_integer_flt::_Element_count) {
					if (_Iu_res == _Result._Myused) {
						_Result._Mydata[_Iu_res] = 0;
						_Result._Myused = _Iu_res + 1;
					}

					_U_carry = _Add_carry(_Result._Mydata[_Iu_res++], 0, _U_carry);
				}

				if (_Iu_res == _Big_integer_flt::_Element_count) {
					_Multiplicand._Myused = 0;
					return false;
				}
			}

			// Store the _Result in the _Multiplicand and compute the actual number of elements used:
			_Multiplicand = _Result;
			return true;
		}

		[[nodiscard]] inline bool _Multiply_by_power_of_ten(_Big_integer_flt& _Xval, const uint32_t _Power) noexcept {
			// To improve performance, we use a table of precomputed powers of ten, from 10^10 through 10^380, in increments
			// of ten. In its unpacked form, as an array of _Big_integer_flt objects, this table consists mostly of zero
			// elements. Thus, we store the table in a packed form, trimming leading and trailing zero elements. We provide an
			// index that is used to unpack powers from the table, using the function that appears after this function in this
			// file.

			// The minimum value representable with double-precision is 5E-324.
			// With this table we can thus compute most multiplications with a single multiply.

			static constexpr uint32_t _Large_power_data[] = { 0x540be400, 0x00000002, 0x63100000, 0x6bc75e2d, 0x00000005,
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

			struct _Unpack_index {
				uint16_t _Offset; // The offset of this power's initial element in the array
				uint8_t _Zeroes; // The number of omitted leading zero elements
				uint8_t _Size; // The number of elements present for this power
			};

			static constexpr _Unpack_index _Large_power_indices[] = { { 0, 0, 2 }, { 2, 0, 3 }, { 5, 0, 4 }, { 9, 1, 4 }, { 13, 1, 5 },
				{ 18, 1, 6 }, { 24, 2, 6 }, { 30, 2, 7 }, { 37, 2, 8 }, { 45, 3, 8 }, { 53, 3, 9 }, { 62, 3, 10 }, { 72, 4, 10 }, { 82, 4, 11 },
				{ 93, 4, 12 }, { 105, 5, 12 }, { 117, 5, 13 }, { 130, 5, 14 }, { 144, 5, 15 }, { 159, 6, 15 }, { 174, 6, 16 }, { 190, 6, 17 },
				{ 207, 7, 17 }, { 224, 7, 18 }, { 242, 7, 19 }, { 261, 8, 19 }, { 280, 8, 21 }, { 301, 8, 22 }, { 323, 9, 22 }, { 345, 9, 23 },
				{ 368, 9, 24 }, { 392, 10, 24 }, { 416, 10, 25 }, { 441, 10, 26 }, { 467, 10, 27 }, { 494, 11, 27 }, { 521, 11, 28 },
				{ 549, 11, 29 } };

			for (uint32_t _Large_power = _Power / 10; _Large_power != 0;) {
				const uint32_t _Current_power =
					(_STD min)(_Large_power, static_cast<uint32_t>(_STD size(_Large_power_indices)));

				const _Unpack_index& _Index = _Large_power_indices[_Current_power - 1];
				_Big_integer_flt _Multiplier { };
				_Multiplier._Myused = static_cast<uint32_t>(_Index._Size + _Index._Zeroes);

				const uint32_t* const _Source = _Large_power_data + _Index._Offset;

				std::memset(_Multiplier._Mydata, 0, _Index._Zeroes * sizeof(uint32_t));
				std::memcpy(_Multiplier._Mydata + _Index._Zeroes, _Source, _Index._Size * sizeof(uint32_t));

				if (!_Multiply(_Xval, _Multiplier)) { // when overflow occurs, resets to zero
					return false;
				}

				_Large_power -= _Current_power;
			}

			static constexpr uint32_t _Small_powers_of_ten[9] = {
				10, 100, 1'000, 10'000, 100'000, 1'000'000, 10'000'000, 100'000'000, 1'000'000'000 };

			const uint32_t _Small_power = _Power % 10;

			if (_Small_power == 0) {
				return true;
			}

			return _Multiply(_Xval, _Small_powers_of_ten[_Small_power - 1]); // when overflow occurs, resets to zero
		}

		[[nodiscard]] inline uint32_t _Count_sequential_high_zeroes(const uint32_t _Ux) noexcept {
			unsigned long _Index; // Intentionally uninitialized for better codegen
			return _BitScanReverse(&_Index, _Ux) ? 31 - _Index : 32;
		}

		[[nodiscard]] inline uint64_t _Divide(_Big_integer_flt& _Numerator, const _Big_integer_flt& _Denominator) noexcept {
			// If the _Numerator is zero, then both the quotient and remainder are zero:
			if (_Numerator._Myused == 0) {
				return 0;
			}

			// If the _Denominator is zero, then uh oh. We can't divide by zero:
			assert(_Denominator._Myused != 0); // Division by zero

			uint32_t _Max_numerator_element_index = _Numerator._Myused - 1;
			const uint32_t _Max_denominator_element_index = _Denominator._Myused - 1;

			// The _Numerator and _Denominator are both nonzero.
			// If the _Denominator is only one element wide, we can take the fast route:
			if (_Max_denominator_element_index == 0) {
				const uint32_t _Small_denominator = _Denominator._Mydata[0];

				if (_Max_numerator_element_index == 0) {
					const uint32_t _Small_numerator = _Numerator._Mydata[0];

					if (_Small_denominator == 1) {
						_Numerator._Myused = 0;
						return _Small_numerator;
					}

					_Numerator._Mydata[0] = _Small_numerator % _Small_denominator;
					_Numerator._Myused = _Numerator._Mydata[0] > 0 ? 1u : 0u;
					return _Small_numerator / _Small_denominator;
				}

				if (_Small_denominator == 1) {
					uint64_t _Quotient = _Numerator._Mydata[1];
					_Quotient <<= 32;
					_Quotient |= _Numerator._Mydata[0];
					_Numerator._Myused = 0;
					return _Quotient;
				}

				// We count down in the next loop, so the last assignment to _Quotient will be the correct one.
				uint64_t _Quotient = 0;

				uint64_t _Uu = 0;
				for (uint32_t _Iv = _Max_numerator_element_index; _Iv != static_cast<uint32_t>(-1); --_Iv) {
					_Uu = (_Uu << 32) | _Numerator._Mydata[_Iv];
					_Quotient = (_Quotient << 32) + static_cast<uint32_t>(_Uu / _Small_denominator);
					_Uu %= _Small_denominator;
				}

				_Numerator._Mydata[1] = static_cast<uint32_t>(_Uu >> 32);
				_Numerator._Mydata[0] = static_cast<uint32_t>(_Uu);

				if (_Numerator._Mydata[1] > 0) {
					_Numerator._Myused = 2u;
				}
				else if (_Numerator._Mydata[0] > 0) {
					_Numerator._Myused = 1u;
				}
				else {
					_Numerator._Myused = 0u;
				}

				return _Quotient;
			}

			if (_Max_denominator_element_index > _Max_numerator_element_index) {
				return 0;
			}

			const uint32_t _Cu_den = _Max_denominator_element_index + 1;
			const int32_t _Cu_diff = static_cast<int32_t>(_Max_numerator_element_index - _Max_denominator_element_index);

			// Determine whether the result will have _Cu_diff or _Cu_diff + 1 digits:
			int32_t _Cu_quo = _Cu_diff;
			for (int32_t _Iu = static_cast<int32_t>(_Max_numerator_element_index);; --_Iu) {
				if (_Iu < _Cu_diff) {
					++_Cu_quo;
					break;
				}

				if (_Denominator._Mydata[_Iu - _Cu_diff] != _Numerator._Mydata[_Iu]) {
					if (_Denominator._Mydata[_Iu - _Cu_diff] < _Numerator._Mydata[_Iu]) {
						++_Cu_quo;
					}

					break;
				}
			}

			if (_Cu_quo == 0) {
				return 0;
			}

			// Get the uint to use for the trial divisions. We normalize so the high bit is set:
			uint32_t _U_den = _Denominator._Mydata[_Cu_den - 1];
			uint32_t _U_den_next = _Denominator._Mydata[_Cu_den - 2];

			const uint32_t _Cbit_shift_left = _Count_sequential_high_zeroes(_U_den);
			const uint32_t _Cbit_shift_right = 32 - _Cbit_shift_left;
			if (_Cbit_shift_left > 0) {
				_U_den = (_U_den << _Cbit_shift_left) | (_U_den_next >> _Cbit_shift_right);
				_U_den_next <<= _Cbit_shift_left;

				if (_Cu_den > 2) {
					_U_den_next |= _Denominator._Mydata[_Cu_den - 3] >> _Cbit_shift_right;
				}
			}

			uint64_t _Quotient = 0;
			for (int32_t _Iu = _Cu_quo; --_Iu >= 0;) {
				// Get the high (normalized) bits of the _Numerator:
				const uint32_t _U_num_hi =
					(_Iu + _Cu_den <= _Max_numerator_element_index) ? _Numerator._Mydata[_Iu + _Cu_den] : 0;

				uint64_t _Uu_num =
					(static_cast<uint64_t>(_U_num_hi) << 32) | static_cast<uint64_t>(_Numerator._Mydata[_Iu + _Cu_den - 1]);

				uint32_t _U_num_next = _Numerator._Mydata[_Iu + _Cu_den - 2];
				if (_Cbit_shift_left > 0) {
					_Uu_num = (_Uu_num << _Cbit_shift_left) | (_U_num_next >> _Cbit_shift_right);
					_U_num_next <<= _Cbit_shift_left;

					if (_Iu + _Cu_den >= 3) {
						_U_num_next |= _Numerator._Mydata[_Iu + _Cu_den - 3] >> _Cbit_shift_right;
					}
				}

				// Divide to get the quotient digit:
				uint64_t _Uu_quo = _Uu_num / _U_den;
				uint64_t _Uu_rem = static_cast<uint32_t>(_Uu_num % _U_den);

				if (_Uu_quo > UINT32_MAX) {
					_Uu_rem += _U_den * (_Uu_quo - UINT32_MAX);
					_Uu_quo = UINT32_MAX;
				}

				while (_Uu_rem <= UINT32_MAX && _Uu_quo * _U_den_next > ((_Uu_rem << 32) | _U_num_next)) {
					--_Uu_quo;
					_Uu_rem += _U_den;
				}

				// Multiply and subtract. Note that _Uu_quo may be one too large.
				// If we have a borrow at the end, we'll add the _Denominator back on and decrement _Uu_quo.
				if (_Uu_quo > 0) {
					uint64_t _Uu_borrow = 0;

					for (uint32_t _Iu2 = 0; _Iu2 < _Cu_den; ++_Iu2) {
						_Uu_borrow += _Uu_quo * _Denominator._Mydata[_Iu2];

						const uint32_t _U_sub = static_cast<uint32_t>(_Uu_borrow);
						_Uu_borrow >>= 32;
						if (_Numerator._Mydata[_Iu + _Iu2] < _U_sub) {
							++_Uu_borrow;
						}

						_Numerator._Mydata[_Iu + _Iu2] -= _U_sub;
					}

					if (_U_num_hi < _Uu_borrow) {
						// Add, tracking carry:
						uint32_t _U_carry = 0;
						for (uint32_t _Iu2 = 0; _Iu2 < _Cu_den; ++_Iu2) {
							const uint64_t _Sum = static_cast<uint64_t>(_Numerator._Mydata[_Iu + _Iu2])
								+ static_cast<uint64_t>(_Denominator._Mydata[_Iu2]) + _U_carry;

							_Numerator._Mydata[_Iu + _Iu2] = static_cast<uint32_t>(_Sum);
							_U_carry = static_cast<uint32_t>(_Sum >> 32);
						}

						--_Uu_quo;
					}

					_Max_numerator_element_index = _Iu + _Cu_den - 1;
				}

				_Quotient = (_Quotient << 32) + static_cast<uint32_t>(_Uu_quo);
			}

			// Trim the remainder:
			for (uint32_t _Ix = _Max_numerator_element_index + 1; _Ix < _Numerator._Myused; ++_Ix) {
				_Numerator._Mydata[_Ix] = 0;
			}

			uint32_t _Used = _Max_numerator_element_index + 1;

			while (_Used != 0 && _Numerator._Mydata[_Used - 1] == 0) {
				--_Used;
			}

			_Numerator._Myused = _Used;

			return _Quotient;
		}

		struct _Floating_point_string {
			bool _Myis_negative;
			int32_t _Myexponent;
			uint32_t _Mymantissa_count;
			uint8_t _Mymantissa[768];
		};

		template <class _FloatingType>
		void _Assemble_floating_point_zero(const bool _Is_negative, _FloatingType& _Result) noexcept {
			using _Floating_traits = _Floating_type_traits<_FloatingType>;
			using _Uint_type = typename _Floating_traits::_Uint_type;

			_Uint_type _Sign_component = _Is_negative;
			_Sign_component <<= _Floating_traits::_Sign_shift;

			_Result = std::bit_cast<_FloatingType>(_Sign_component);
		}

		template <class _FloatingType>
		void _Assemble_floating_point_infinity(const bool _Is_negative, _FloatingType& _Result) noexcept {
			using _Floating_traits = _Floating_type_traits<_FloatingType>;
			using _Uint_type = typename _Floating_traits::_Uint_type;

			_Uint_type _Sign_component = _Is_negative;
			_Sign_component <<= _Floating_traits::_Sign_shift;

			const _Uint_type _Exponent_component = _Floating_traits::_Shifted_exponent_mask;

			_Result = std::bit_cast<_FloatingType>(_Sign_component | _Exponent_component);
		}

		[[nodiscard]] inline bool _Should_round_up(
			const bool _Lsb_bit, const bool _Round_bit, const bool _Has_tail_bits) noexcept {

			return _Round_bit && (_Has_tail_bits || _Lsb_bit);
		}

		[[nodiscard]] inline uint64_t _Right_shift_with_rounding(
			const uint64_t _Value, const uint32_t _Shift, const bool _Has_zero_tail) noexcept {
			constexpr uint32_t _Total_number_of_bits = 64;
			if (_Shift >= _Total_number_of_bits) {
				if (_Shift == _Total_number_of_bits) {
					constexpr uint64_t _Extra_bits_mask = (1ULL << (_Total_number_of_bits - 1)) - 1;
					constexpr uint64_t _Round_bit_mask = (1ULL << (_Total_number_of_bits - 1));

					const bool _Round_bit = (_Value & _Round_bit_mask) != 0;
					const bool _Tail_bits = !_Has_zero_tail || (_Value & _Extra_bits_mask) != 0;

					// We round up the answer to 1 if the answer is greater than 0.5. Otherwise, we round down the answer to 0
					// if either [1] the answer is less than 0.5 or [2] the answer is exactly 0.5.
					return static_cast<uint64_t>(_Round_bit && _Tail_bits);
				}
				else {
					// If we'd need to shift 65 or more bits, the answer is less than 0.5 and is always rounded to zero:
					return 0;
				}
			}

			const uint64_t _Lsb_bit = _Value;
			const uint64_t _Round_bit = _Value << 1;
			const uint64_t _Has_tail_bits = _Round_bit - static_cast<uint64_t>(_Has_zero_tail);
			const uint64_t _Should_round = ((_Round_bit & (_Has_tail_bits | _Lsb_bit)) >> _Shift) & uint64_t { 1 };

			return (_Value >> _Shift) + _Should_round;
		}

		template <class _FloatingType>
		void _Assemble_floating_point_value_no_shift(const bool _Is_negative, const int32_t _Exponent,
			const typename _Floating_type_traits<_FloatingType>::_Uint_type _Mantissa, _FloatingType& _Result) noexcept {
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
			using _Floating_traits = _Floating_type_traits<_FloatingType>;
			using _Uint_type = typename _Floating_traits::_Uint_type;

			_Uint_type _Sign_component = _Is_negative;
			_Sign_component <<= _Floating_traits::_Sign_shift;

			_Uint_type _Exponent_component = static_cast<uint32_t>(_Exponent + (_Floating_traits::_Exponent_bias - 1));
			_Exponent_component <<= _Floating_traits::_Exponent_shift;

			_Result = std::bit_cast<_FloatingType>(_Sign_component | (_Exponent_component + _Mantissa));
		}

		template <class _FloatingType>
		[[nodiscard]] errc _Assemble_floating_point_value(const uint64_t _Initial_mantissa, const int32_t _Initial_exponent,
			const bool _Is_negative, const bool _Has_zero_tail, _FloatingType& _Result) noexcept {
			using _Traits = _Floating_type_traits<_FloatingType>;

			// Assume that the number is representable as a normal value.
			// Compute the number of bits by which we must adjust the mantissa to shift it into the correct position,
			// and compute the resulting base two exponent for the normalized mantissa:
			const uint32_t _Initial_mantissa_bits = _Bit_scan_reverse(_Initial_mantissa);
			const int32_t _Normal_mantissa_shift = static_cast<int32_t>(_Traits::_Mantissa_bits - _Initial_mantissa_bits);
			const int32_t _Normal_exponent = _Initial_exponent - _Normal_mantissa_shift;

			if (_Normal_exponent > _Traits::_Maximum_binary_exponent) {
				// The exponent is too large to be represented by the floating-point type; report the overflow condition:
				_Assemble_floating_point_infinity(_Is_negative, _Result);
				return errc::result_out_of_range; // Overflow example: "1e+1000"
			}

			uint64_t _Mantissa = _Initial_mantissa;
			int32_t _Exponent = _Normal_exponent;
			errc _Error_code { };

			if (_Normal_exponent < _Traits::_Minimum_binary_exponent) {
				// The exponent is too small to be represented by the floating-point type as a normal value, but it may be
				// representable as a denormal value.

				// The exponent of subnormal values (as defined by the mathematical model of floating-point numbers, not the
				// exponent field in the bit representation) is equal to the minimum exponent of normal values.
				_Exponent = _Traits::_Minimum_binary_exponent;

				// Compute the number of bits by which we need to shift the mantissa in order to form a denormal number.
				const int32_t _Denormal_mantissa_shift = _Initial_exponent - _Exponent;

				if (_Denormal_mantissa_shift < 0) {
					_Mantissa =
						_Right_shift_with_rounding(_Mantissa, static_cast<uint32_t>(-_Denormal_mantissa_shift), _Has_zero_tail);

					// from_chars in MSVC STL and strto[f|d|ld] in UCRT reports underflow only when the result is zero after
					// rounding to the floating-point format. This behavior is different from IEEE 754 underflow exception.
					if (_Mantissa == 0) {
						_Error_code = errc::result_out_of_range; // Underflow example: "1e-1000"
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
					// This is handled by _Assemble_floating_point_value_no_shift.
				}
				else {
					_Mantissa <<= _Denormal_mantissa_shift;
				}
			}
			else {
				if (_Normal_mantissa_shift < 0) {
					_Mantissa =
						_Right_shift_with_rounding(_Mantissa, static_cast<uint32_t>(-_Normal_mantissa_shift), _Has_zero_tail);

					// When we round the mantissa, it may produce a result that is too large. In this case,
					// we divide the mantissa by two and increment the exponent (this does not change the value).
					// This is handled by _Assemble_floating_point_value_no_shift.

					// The increment of the exponent may have generated a value too large to be represented.
					// In this case, report the overflow:
					if (_Mantissa > _Traits::_Normal_mantissa_mask && _Exponent == _Traits::_Maximum_binary_exponent) {
						_Error_code = errc::result_out_of_range; // Overflow example: "1.ffffffp+127" for float
																 // Overflow example: "1.fffffffffffff8p+1023" for double
					}
				}
				else {
					_Mantissa <<= _Normal_mantissa_shift;
				}
			}

			// Assemble the floating-point value from the computed components:
			using _Uint_type = typename _Traits::_Uint_type;

			_Assemble_floating_point_value_no_shift(_Is_negative, _Exponent, static_cast<_Uint_type>(_Mantissa), _Result);

			return _Error_code;
		}

		template <class _FloatingType>
		[[nodiscard]] errc _Assemble_floating_point_value_from_big_integer_flt(const _Big_integer_flt& _Integer_value,
			const uint32_t _Integer_bits_of_precision, const bool _Is_negative, const bool _Has_nonzero_fractional_part,
			_FloatingType& _Result) noexcept {
			using _Traits = _Floating_type_traits<_FloatingType>;

			const int32_t _Base_exponent = _Traits::_Mantissa_bits - 1;

			// Very fast case: If we have 64 bits of precision or fewer,
			// we can just take the two low order elements from the _Big_integer_flt:
			if (_Integer_bits_of_precision <= 64) {
				const int32_t _Exponent = _Base_exponent;

				const uint32_t _Mantissa_low = _Integer_value._Myused > 0 ? _Integer_value._Mydata[0] : 0;
				const uint32_t _Mantissa_high = _Integer_value._Myused > 1 ? _Integer_value._Mydata[1] : 0;
				const uint64_t _Mantissa = _Mantissa_low + (static_cast<uint64_t>(_Mantissa_high) << 32);

				return _Assemble_floating_point_value(
					_Mantissa, _Exponent, _Is_negative, !_Has_nonzero_fractional_part, _Result);
			}

			const uint32_t _Top_element_bits = _Integer_bits_of_precision % 32;
			const uint32_t _Top_element_index = _Integer_bits_of_precision / 32;

			const uint32_t _Middle_element_index = _Top_element_index - 1;
			const uint32_t _Bottom_element_index = _Top_element_index - 2;

			// Pretty fast case: If the top 64 bits occupy only two elements, we can just combine those two elements:
			if (_Top_element_bits == 0) {
				const int32_t _Exponent = static_cast<int32_t>(_Base_exponent + _Bottom_element_index * 32);

				const uint64_t _Mantissa = _Integer_value._Mydata[_Bottom_element_index]
					+ (static_cast<uint64_t>(_Integer_value._Mydata[_Middle_element_index]) << 32);

				bool _Has_zero_tail = !_Has_nonzero_fractional_part;
				for (uint32_t _Ix = 0; _Has_zero_tail && _Ix != _Bottom_element_index; ++_Ix) {
					_Has_zero_tail = _Integer_value._Mydata[_Ix] == 0;
				}

				return _Assemble_floating_point_value(_Mantissa, _Exponent, _Is_negative, _Has_zero_tail, _Result);
			}

			// Not quite so fast case: The top 64 bits span three elements in the _Big_integer_flt. Assemble the three pieces:
			const uint32_t _Top_element_mask = (1u << _Top_element_bits) - 1;
			const uint32_t _Top_element_shift = 64 - _Top_element_bits; // Left

			const uint32_t _Middle_element_shift = _Top_element_shift - 32; // Left

			const uint32_t _Bottom_element_bits = 32 - _Top_element_bits;
			const uint32_t _Bottom_element_mask = ~_Top_element_mask;
			const uint32_t _Bottom_element_shift = 32 - _Bottom_element_bits; // Right

			const int32_t _Exponent = static_cast<int32_t>(_Base_exponent + _Bottom_element_index * 32 + _Top_element_bits);

			const uint64_t _Mantissa =
				(static_cast<uint64_t>(_Integer_value._Mydata[_Top_element_index] & _Top_element_mask) << _Top_element_shift)
				+ (static_cast<uint64_t>(_Integer_value._Mydata[_Middle_element_index]) << _Middle_element_shift)
				+ (static_cast<uint64_t>(_Integer_value._Mydata[_Bottom_element_index] & _Bottom_element_mask)
					>> _Bottom_element_shift);

			bool _Has_zero_tail =
				!_Has_nonzero_fractional_part && (_Integer_value._Mydata[_Bottom_element_index] & _Top_element_mask) == 0;

			for (uint32_t _Ix = 0; _Has_zero_tail && _Ix != _Bottom_element_index; ++_Ix) {
				_Has_zero_tail = _Integer_value._Mydata[_Ix] == 0;
			}

			return _Assemble_floating_point_value(_Mantissa, _Exponent, _Is_negative, _Has_zero_tail, _Result);
		}

		inline void _Accumulate_decimal_digits_into_big_integer_flt(
			const uint8_t* const _First_digit, const uint8_t* const _Last_digit, _Big_integer_flt& _Result) noexcept {
			// We accumulate nine digit chunks, transforming the base ten string into base one billion on the fly,
			// allowing us to reduce the number of high-precision multiplication and addition operations by 8/9.
			uint32_t _Accumulator = 0;
			uint32_t _Accumulator_count = 0;
			for (const uint8_t* _It = _First_digit; _It != _Last_digit; ++_It) {
				if (_Accumulator_count == 9) {
					[[maybe_unused]] const bool _Success1 = _Multiply(_Result, 1'000'000'000); // assumes no overflow
					assert(_Success1);
					[[maybe_unused]] const bool _Success2 = _Add(_Result, _Accumulator); // assumes no overflow
					assert(_Success2);

					_Accumulator = 0;
					_Accumulator_count = 0;
				}

				_Accumulator *= 10;
				_Accumulator += *_It;
				++_Accumulator_count;
			}

			if (_Accumulator_count != 0) {
				[[maybe_unused]] const bool _Success3 =
					_Multiply_by_power_of_ten(_Result, _Accumulator_count); // assumes no overflow
				assert(_Success3);
				[[maybe_unused]] const bool _Success4 = _Add(_Result, _Accumulator); // assumes no overflow
				assert(_Success4);
			}
		}

		template <class _FloatingType>
		[[nodiscard]] errc _Convert_decimal_string_to_floating_type(
			const _Floating_point_string& _Data, _FloatingType& _Result, bool _Has_zero_tail) noexcept {
			using _Traits = _Floating_type_traits<_FloatingType>;

			// To generate an N bit mantissa we require N + 1 bits of precision. The extra bit is used to correctly round
			// the mantissa (if there are fewer bits than this available, then that's totally okay;
			// in that case we use what we have and we don't need to round).
			const uint32_t _Required_bits_of_precision = static_cast<uint32_t>(_Traits::_Mantissa_bits + 1);

			// The input is of the form 0.mantissa * 10^exponent, where 'mantissa' are the decimal digits of the mantissa
			// and 'exponent' is the decimal exponent. We decompose the mantissa into two parts: an integer part and a
			// fractional part. If the exponent is positive, then the integer part consists of the first 'exponent' digits,
			// or all present digits if there are fewer digits. If the exponent is zero or negative, then the integer part
			// is empty. In either case, the remaining digits form the fractional part of the mantissa.
			const uint32_t _Positive_exponent = static_cast<uint32_t>((_STD max)(0, _Data._Myexponent));
			const uint32_t _Integer_digits_present = (_STD min)(_Positive_exponent, _Data._Mymantissa_count);
			const uint32_t _Integer_digits_missing = _Positive_exponent - _Integer_digits_present;
			const uint8_t* const _Integer_first = _Data._Mymantissa;
			const uint8_t* const _Integer_last = _Data._Mymantissa + _Integer_digits_present;

			const uint8_t* const _Fractional_first = _Integer_last;
			const uint8_t* const _Fractional_last = _Data._Mymantissa + _Data._Mymantissa_count;
			const uint32_t _Fractional_digits_present = static_cast<uint32_t>(_Fractional_last - _Fractional_first);

			// First, we accumulate the integer part of the mantissa into a _Big_integer_flt:
			_Big_integer_flt _Integer_value { };
			_Accumulate_decimal_digits_into_big_integer_flt(_Integer_first, _Integer_last, _Integer_value);

			if (_Integer_digits_missing > 0) {
				if (!_Multiply_by_power_of_ten(_Integer_value, _Integer_digits_missing)) {
					_Assemble_floating_point_infinity(_Data._Myis_negative, _Result);
					return errc::result_out_of_range; // Overflow example: "1e+2000"
				}
			}

			// At this point, the _Integer_value contains the value of the integer part of the mantissa. If either
			// [1] this number has more than the required number of bits of precision or
			// [2] the mantissa has no fractional part, then we can assemble the result immediately:
			const uint32_t _Integer_bits_of_precision = _Bit_scan_reverse(_Integer_value);
			{
				const bool _Has_zero_fractional_part = _Fractional_digits_present == 0 && _Has_zero_tail;

				if (_Integer_bits_of_precision >= _Required_bits_of_precision || _Has_zero_fractional_part) {
					return _Assemble_floating_point_value_from_big_integer_flt(
						_Integer_value, _Integer_bits_of_precision, _Data._Myis_negative, !_Has_zero_fractional_part, _Result);
				}
			}

			// Otherwise, we did not get enough bits of precision from the integer part, and the mantissa has a fractional
			// part. We parse the fractional part of the mantissa to obtain more bits of precision. To do this, we convert
			// the fractional part into an actual fraction N/M, where the numerator N is computed from the digits of the
			// fractional part, and the denominator M is computed as the power of 10 such that N/M is equal to the value
			// of the fractional part of the mantissa.
			_Big_integer_flt _Fractional_numerator { };
			_Accumulate_decimal_digits_into_big_integer_flt(_Fractional_first, _Fractional_last, _Fractional_numerator);

			const uint32_t _Fractional_denominator_exponent =
				_Data._Myexponent < 0 ? _Fractional_digits_present + static_cast<uint32_t>(-_Data._Myexponent)
				: _Fractional_digits_present;

			_Big_integer_flt _Fractional_denominator = _Make_big_integer_flt_one();
			if (!_Multiply_by_power_of_ten(_Fractional_denominator, _Fractional_denominator_exponent)) {
				// If there were any digits in the integer part, it is impossible to underflow (because the exponent
				// cannot possibly be small enough), so if we underflow here it is a true underflow and we return zero.
				_Assemble_floating_point_zero(_Data._Myis_negative, _Result);
				return errc::result_out_of_range; // Underflow example: "1e-2000"
			}

			// Because we are using only the fractional part of the mantissa here, the numerator is guaranteed to be smaller
			// than the denominator. We normalize the fraction such that the most significant bit of the numerator is in the
			// same position as the most significant bit in the denominator. This ensures that when we later shift the
			// numerator N bits to the left, we will produce N bits of precision.
			const uint32_t _Fractional_numerator_bits = _Bit_scan_reverse(_Fractional_numerator);
			const uint32_t _Fractional_denominator_bits = _Bit_scan_reverse(_Fractional_denominator);

			const uint32_t _Fractional_shift = _Fractional_denominator_bits > _Fractional_numerator_bits
				? _Fractional_denominator_bits - _Fractional_numerator_bits
				: 0;

			if (_Fractional_shift > 0) {
				[[maybe_unused]] const bool _Shift_success1 =
					_Shift_left(_Fractional_numerator, _Fractional_shift); // assumes no overflow
				assert(_Shift_success1);
			}

			const uint32_t _Required_fractional_bits_of_precision = _Required_bits_of_precision - _Integer_bits_of_precision;

			uint32_t _Remaining_bits_of_precision_required = _Required_fractional_bits_of_precision;
			if (_Integer_bits_of_precision > 0) {
				// If the fractional part of the mantissa provides no bits of precision and cannot affect rounding,
				// we can just take whatever bits we got from the integer part of the mantissa. This is the case for numbers
				// like 5.0000000000000000000001, where the significant digits of the fractional part start so far to the
				// right that they do not affect the floating-point representation.

				// If the fractional shift is exactly equal to the number of bits of precision that we require,
				// then no fractional bits will be part of the result, but the result may affect rounding.
				// This is e.g. the case for large, odd integers with a fractional part greater than or equal to .5.
				// Thus, we need to do the division to correctly round the result.
				if (_Fractional_shift > _Remaining_bits_of_precision_required) {
					return _Assemble_floating_point_value_from_big_integer_flt(_Integer_value, _Integer_bits_of_precision,
						_Data._Myis_negative, _Fractional_digits_present != 0 || !_Has_zero_tail, _Result);
				}

				_Remaining_bits_of_precision_required -= _Fractional_shift;
			}

			// If there was no integer part of the mantissa, we will need to compute the exponent from the fractional part.
			// The fractional exponent is the power of two by which we must multiply the fractional part to move it into the
			// range [1.0, 2.0). This will either be the same as the shift we computed earlier, or one greater than that shift:
			const uint32_t _Fractional_exponent =
				_Fractional_numerator < _Fractional_denominator ? _Fractional_shift + 1 : _Fractional_shift;

			[[maybe_unused]] const bool _Shift_success2 =
				_Shift_left(_Fractional_numerator, _Remaining_bits_of_precision_required); // assumes no overflow
			assert(_Shift_success2);

			uint64_t _Fractional_mantissa = _Divide(_Fractional_numerator, _Fractional_denominator);

			_Has_zero_tail = _Has_zero_tail && _Fractional_numerator._Myused == 0;

			// We may have produced more bits of precision than were required. Check, and remove any "extra" bits:
			const uint32_t _Fractional_mantissa_bits = _Bit_scan_reverse(_Fractional_mantissa);
			if (_Fractional_mantissa_bits > _Required_fractional_bits_of_precision) {
				const uint32_t _Shift = _Fractional_mantissa_bits - _Required_fractional_bits_of_precision;
				_Has_zero_tail = _Has_zero_tail && (_Fractional_mantissa & ((1ULL << _Shift) - 1)) == 0;
				_Fractional_mantissa >>= _Shift;
			}

			// Compose the mantissa from the integer and fractional parts:
			const uint32_t _Integer_mantissa_low = _Integer_value._Myused > 0 ? _Integer_value._Mydata[0] : 0;
			const uint32_t _Integer_mantissa_high = _Integer_value._Myused > 1 ? _Integer_value._Mydata[1] : 0;
			const uint64_t _Integer_mantissa = _Integer_mantissa_low + (static_cast<uint64_t>(_Integer_mantissa_high) << 32);

			const uint64_t _Complete_mantissa =
				(_Integer_mantissa << _Required_fractional_bits_of_precision) + _Fractional_mantissa;

			// Compute the final exponent:
			// * If the mantissa had an integer part, then the exponent is one less than the number of bits we obtained
			// from the integer part. (It's one less because we are converting to the form 1.11111,
			// with one 1 to the left of the decimal point.)
			// * If the mantissa had no integer part, then the exponent is the fractional exponent that we computed.
			// Then, in both cases, we subtract an additional one from the exponent,
			// to account for the fact that we've generated an extra bit of precision, for use in rounding.
			const int32_t _Final_exponent = _Integer_bits_of_precision > 0
				? static_cast<int32_t>(_Integer_bits_of_precision - 2)
				: -static_cast<int32_t>(_Fractional_exponent) - 1;

			return _Assemble_floating_point_value(
				_Complete_mantissa, _Final_exponent, _Data._Myis_negative, _Has_zero_tail, _Result);
		}

		template <class _FloatingType>
		[[nodiscard]] errc _Convert_hexadecimal_string_to_floating_type(
			const _Floating_point_string& _Data, _FloatingType& _Result, bool _Has_zero_tail) noexcept {
			using _Traits = _Floating_type_traits<_FloatingType>;

			uint64_t _Mantissa = 0;
			int32_t _Exponent = _Data._Myexponent + _Traits::_Mantissa_bits - 1;

			// Accumulate bits into the mantissa buffer
			const uint8_t* const _Mantissa_last = _Data._Mymantissa + _Data._Mymantissa_count;
			const uint8_t* _Mantissa_it = _Data._Mymantissa;
			while (_Mantissa_it != _Mantissa_last && _Mantissa <= _Traits::_Normal_mantissa_mask) {
				_Mantissa *= 16;
				_Mantissa += *_Mantissa_it++;
				_Exponent -= 4; // The exponent is in binary; log2(16) == 4
			}

			while (_Has_zero_tail && _Mantissa_it != _Mantissa_last) {
				_Has_zero_tail = *_Mantissa_it++ == 0;
			}

			return _Assemble_floating_point_value(_Mantissa, _Exponent, _Data._Myis_negative, _Has_zero_tail, _Result);
		}

		template <class _Floating, class CharT>
		[[nodiscard]] from_chars_result<CharT> _Ordinary_floating_from_chars(const CharT* const _First, const CharT* const _Last,
			_Floating& _Value, const chars_format _Fmt, const bool _Minus_sign, const CharT* _Next) noexcept {
			// vvvvvvvvvv DERIVED FROM corecrt_internal_strtox.h WITH SIGNIFICANT MODIFICATIONS vvvvvvvvvv

			const bool _Is_hexadecimal = _Fmt == chars_format::hex;
			const int _Base { _Is_hexadecimal ? 16 : 10 };

			// PERFORMANCE NOTE: _Fp_string is intentionally left uninitialized. Zero-initialization is quite expensive
			// and is unnecessary. The benefit of not zero-initializing is greatest for short inputs.
			_Floating_point_string _Fp_string;

			// Record the optional minus sign:
			_Fp_string._Myis_negative = _Minus_sign;

			uint8_t* const _Mantissa_first = _Fp_string._Mymantissa;
			uint8_t* const _Mantissa_last = _STD end(_Fp_string._Mymantissa);
			uint8_t* _Mantissa_it = _Mantissa_first;

			// [_Whole_begin, _Whole_end) will contain 0 or more digits/hexits
			const auto* const _Whole_begin = _Next;

			// Skip past any leading zeroes in the mantissa:
			for (; _Next != _Last && *_Next == '0'; ++_Next) {
			}
			const auto* const _Leading_zero_end = _Next;

			// Scan the integer part of the mantissa:
			for (; _Next != _Last; ++_Next) {
				const unsigned char _Digit_value = _Digit_from_char(*_Next);

				if (_Digit_value >= _Base) {
					break;
				}

				if (_Mantissa_it != _Mantissa_last) {
					*_Mantissa_it++ = _Digit_value;
				}
			}
			const auto* const _Whole_end = _Next;

			// Defend against _Exponent_adjustment integer overflow. (These values don't need to be strict.)
			constexpr ptrdiff_t _Maximum_adjustment = 1'000'000;
			constexpr ptrdiff_t _Minimum_adjustment = -1'000'000;

			// The exponent adjustment holds the number of digits in the mantissa buffer that appeared before the radix point.
			// It can be negative, and leading zeroes in the integer part are ignored. Examples:
			// For "03333.111", it is 4.
			// For "00000.111", it is 0.
			// For "00000.001", it is -2.
			int _Exponent_adjustment = static_cast<int>((_STD min)(_Whole_end - _Leading_zero_end, _Maximum_adjustment));

			// [_Whole_end, _Dot_end) will contain 0 or 1 '.' characters
			if (_Next != _Last && *_Next == '.') {
				++_Next;
			}
			const auto* const _Dot_end = _Next;

			// [_Dot_end, _Frac_end) will contain 0 or more digits/hexits

			// If we haven't yet scanned any nonzero digits, continue skipping over zeroes,
			// updating the exponent adjustment to account for the zeroes we are skipping:
			if (_Exponent_adjustment == 0) {
				for (; _Next != _Last && *_Next == '0'; ++_Next) {
				}

				_Exponent_adjustment = static_cast<int>((_STD max)(_Dot_end - _Next, _Minimum_adjustment));
			}

			// Scan the fractional part of the mantissa:
			bool _Has_zero_tail = true;

			for (; _Next != _Last; ++_Next) {
				const unsigned char _Digit_value = _Digit_from_char(*_Next);

				if (_Digit_value >= _Base) {
					break;
				}

				if (_Mantissa_it != _Mantissa_last) {
					*_Mantissa_it++ = _Digit_value;
				}
				else {
					_Has_zero_tail = _Has_zero_tail && _Digit_value == 0;
				}
			}
			const auto* const _Frac_end = _Next;

			// We must have at least 1 digit/hexit
			if (_Whole_begin == _Whole_end && _Dot_end == _Frac_end) {
				return { _First, errc::invalid_argument };
			}

			const char _Exponent_prefix { _Is_hexadecimal ? 'p' : 'e' };

			bool _Exponent_is_negative = false;
			int _Exponent = 0;

			constexpr int _Maximum_temporary_decimal_exponent = 5200;
			constexpr int _Minimum_temporary_decimal_exponent = -5200;

			if (_Fmt != chars_format::fixed // N4713 23.20.3 [charconv.from.chars]/7.3
											// "if fmt has chars_format::fixed set but not chars_format::scientific,
											// the optional exponent part shall not appear"
				&& _Next != _Last && (static_cast<unsigned char>(*_Next) | 0x20) == _Exponent_prefix) { // found exponent prefix
				const auto* _Unread = _Next + 1;

				if (_Unread != _Last && (*_Unread == '+' || *_Unread == '-')) { // found optional sign
					_Exponent_is_negative = *_Unread == '-';
					++_Unread;
				}

				while (_Unread != _Last) {
					const unsigned char _Digit_value = _Digit_from_char(*_Unread);

					if (_Digit_value >= 10) {
						break;
					}

					// found decimal digit

					if (_Exponent <= _Maximum_temporary_decimal_exponent) {
						_Exponent = _Exponent * 10 + _Digit_value;
					}

					++_Unread;
					_Next = _Unread; // consume exponent-part/binary-exponent-part
				}

				if (_Exponent_is_negative) {
					_Exponent = -_Exponent;
				}
			}

			// [_Frac_end, _Exponent_end) will either be empty or contain "[EPep] sign[opt] digit-sequence"
			const auto* const _Exponent_end = _Next;

			if (_Fmt == chars_format::scientific
				&& _Frac_end == _Exponent_end) { // N4713 23.20.3 [charconv.from.chars]/7.2
												 // "if fmt has chars_format::scientific set but not chars_format::fixed,
												 // the otherwise optional exponent part shall appear"
				return { _First, errc::invalid_argument };
			}

			// Remove trailing zeroes from mantissa:
			while (_Mantissa_it != _Mantissa_first && *(_Mantissa_it - 1) == 0) {
				--_Mantissa_it;
			}

			// If the mantissa buffer is empty, the mantissa was composed of all zeroes (so the mantissa is 0).
			// All such strings have the value zero, regardless of what the exponent is (because 0 * b^n == 0 for all b and n).
			// We can return now. Note that we defer this check until after we scan the exponent, so that we can correctly
			// update _Next to point past the end of the exponent.
			if (_Mantissa_it == _Mantissa_first) {
				assert(_Has_zero_tail);
				_Assemble_floating_point_zero(_Fp_string._Myis_negative, _Value);
				return { _Next, errc { } };
			}

			// Before we adjust the exponent, handle the case where we detected a wildly
			// out of range exponent during parsing and clamped the value:
			if (_Exponent > _Maximum_temporary_decimal_exponent) {
				_Assemble_floating_point_infinity(_Fp_string._Myis_negative, _Value);
				return { _Next, errc::result_out_of_range }; // Overflow example: "1e+9999"
			}

			if (_Exponent < _Minimum_temporary_decimal_exponent) {
				_Assemble_floating_point_zero(_Fp_string._Myis_negative, _Value);
				return { _Next, errc::result_out_of_range }; // Underflow example: "1e-9999"
			}

			// In hexadecimal floating constants, the exponent is a base 2 exponent. The exponent adjustment computed during
			// parsing has the same base as the mantissa (so, 16 for hexadecimal floating constants).
			// We therefore need to scale the base 16 multiplier to base 2 by multiplying by log2(16):
			const int _Exponent_adjustment_multiplier { _Is_hexadecimal ? 4 : 1 };

			_Exponent += _Exponent_adjustment * _Exponent_adjustment_multiplier;

			// Verify that after adjustment the exponent isn't wildly out of range (if it is, it isn't representable
			// in any supported floating-point format).
			if (_Exponent > _Maximum_temporary_decimal_exponent) {
				_Assemble_floating_point_infinity(_Fp_string._Myis_negative, _Value);
				return { _Next, errc::result_out_of_range }; // Overflow example: "10e+5199"
			}

			if (_Exponent < _Minimum_temporary_decimal_exponent) {
				_Assemble_floating_point_zero(_Fp_string._Myis_negative, _Value);
				return { _Next, errc::result_out_of_range }; // Underflow example: "0.001e-5199"
			}

			_Fp_string._Myexponent = _Exponent;
			_Fp_string._Mymantissa_count = static_cast<uint32_t>(_Mantissa_it - _Mantissa_first);

			if (_Is_hexadecimal) {
				const errc _Ec = _Convert_hexadecimal_string_to_floating_type(_Fp_string, _Value, _Has_zero_tail);
				return { _Next, _Ec };
			}
			else {
				const errc _Ec = _Convert_decimal_string_to_floating_type(_Fp_string, _Value, _Has_zero_tail);
				return { _Next, _Ec };
			}

			// ^^^^^^^^^^ DERIVED FROM corecrt_internal_strtox.h WITH SIGNIFICANT MODIFICATIONS ^^^^^^^^^^
		}

		template <class CharT>
		[[nodiscard]] inline bool _Starts_with_case_insensitive(
			const CharT* _First, const CharT* const _Last, const char* _Lowercase) noexcept {
			// pre: _Lowercase contains only ['a', 'z'] and is null-terminated
			for (; _First != _Last && *_Lowercase != '\0'; ++_First, ++_Lowercase) {
				if ((static_cast<unsigned char>(*_First) | 0x20) != *_Lowercase) {
					return false;
				}
			}

			return *_Lowercase == '\0';
		}

		template <class _Floating, class CharT>
		[[nodiscard]] from_chars_result<CharT> _Infinity_from_chars(const CharT* const _First, const CharT* const _Last, _Floating& _Value,
			const bool _Minus_sign, const CharT* _Next) noexcept {
			// pre: _Next points at 'i' (case-insensitively)
			if (!_Starts_with_case_insensitive(_Next + 1, _Last, "nf")) { // definitely invalid
				return { _First, errc::invalid_argument };
			}

			// definitely inf
			_Next += 3;

			if (_Starts_with_case_insensitive(_Next, _Last, "inity")) { // definitely infinity
				_Next += 5;
			}

			_Assemble_floating_point_infinity(_Minus_sign, _Value);

			return { _Next, errc { } };
		}

		template <class _Floating, class CharT>
		[[nodiscard]] from_chars_result<CharT> _Nan_from_chars(const CharT* const _First, const CharT* const _Last, _Floating& _Value,
			bool _Minus_sign, const CharT* _Next) noexcept {
			// pre: _Next points at 'n' (case-insensitively)
			if (!_Starts_with_case_insensitive(_Next + 1, _Last, "an")) { // definitely invalid
				return { _First, errc::invalid_argument };
			}

			// definitely nan
			_Next += 3;

			bool _Quiet = true;

			if (_Next != _Last && *_Next == '(') { // possibly nan(n-char-sequence[opt])
				const auto* const _Seq_begin = _Next + 1;

				for (const auto* _Temp = _Seq_begin; _Temp != _Last; ++_Temp) {
					if (*_Temp == ')') { // definitely nan(n-char-sequence[opt])
						_Next = _Temp + 1;

						if (_Temp - _Seq_begin == 3
							&& _Starts_with_case_insensitive(_Seq_begin, _Temp, "ind")) { // definitely nan(ind)
							// The UCRT considers indeterminate NaN to be negative quiet NaN with no payload bits set.
							// It parses "nan(ind)" and "-nan(ind)" identically.
							_Minus_sign = true;
						}
						else if (_Temp - _Seq_begin == 4
							&& _Starts_with_case_insensitive(_Seq_begin, _Temp, "snan")) { // definitely nan(snan)
							_Quiet = false;
						}

						break;
					}
					else if (*_Temp == '_' || ('0' <= *_Temp && *_Temp <= '9') || ('A' <= *_Temp && *_Temp <= 'Z')
						|| ('a' <= *_Temp && *_Temp <= 'z')) { // possibly nan(n-char-sequence[opt]), keep going
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

			using _Traits = _Floating_type_traits<_Floating>;
			using _Uint_type = typename _Traits::_Uint_type;

			_Uint_type _Uint_value = _Traits::_Shifted_exponent_mask;

			if (_Minus_sign) {
				_Uint_value |= _Traits::_Shifted_sign_mask;
			}

			if (_Quiet) {
				_Uint_value |= _Traits::_Special_nan_mantissa_mask;
			}
			else {
				_Uint_value |= 1;
			}

			_Value = std::bit_cast<_Floating>(_Uint_value);

			return { _Next, errc { } };
		}

		template <class _Floating, class CharT>
		[[nodiscard]] from_chars_result<CharT> _Floating_from_chars(
			const CharT* const _First, const CharT* const _Last, _Floating& _Value, const chars_format _Fmt) noexcept {

			assert(_Fmt == chars_format::general || _Fmt == chars_format::scientific || _Fmt == chars_format::fixed
				|| _Fmt == chars_format::hex);

			bool _Minus_sign = false;

			const auto* _Next = _First;

			if (_Next == _Last) {
				return { _First, errc::invalid_argument };
			}

			if (*_Next == '-') {
				_Minus_sign = true;
				++_Next;

				if (_Next == _Last) {
					return { _First, errc::invalid_argument };
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
			const unsigned char _Folded_start = static_cast<unsigned char>(static_cast<unsigned char>(*_Next) | 0x20);

			if (_Folded_start <= 'f') { // possibly an ordinary number
				return _Ordinary_floating_from_chars(_First, _Last, _Value, _Fmt, _Minus_sign, _Next);
			}
			else if (_Folded_start == 'i') { // possibly inf
				return _Infinity_from_chars(_First, _Last, _Value, _Minus_sign, _Next);
			}
			else if (_Folded_start == 'n') { // possibly nan
				return _Nan_from_chars(_First, _Last, _Value, _Minus_sign, _Next);
			}
			else { // definitely invalid
				return { _First, errc::invalid_argument };
			}
		}
	};

	//Main converting methods.

	template<typename IntegralT> requires std::is_integral_v<IntegralT>
	[[nodiscard]] auto StrToNum(std::string_view str, int iBase = 0)noexcept
		->STN_RETURN_TYPE(IntegralT, char)
	{
		assert(!str.empty());

		IntegralT TData;
		const auto result = impl::_Integer_from_chars(str.data(), str.data() + str.size(), TData, iBase);
		if (result.ec == errc()) {
			return TData;
		}
		return STN_RETURN_VALUE(result)
	}

	template<typename IntegralT> requires std::is_integral_v<IntegralT>
	[[nodiscard]] auto StrToNum(std::wstring_view wstr, int iBase = 0)noexcept
		->STN_RETURN_TYPE(IntegralT, wchar_t)
	{
		assert(!wstr.empty());

		IntegralT TData;
		const auto result = impl::_Integer_from_chars(wstr.data(), wstr.data() + wstr.size(), TData, iBase);
		if (result.ec == errc()) {
			return TData;
		}
		return STN_RETURN_VALUE(result)
	}

	template<typename FloatingT> requires std::is_floating_point_v<FloatingT>
	[[nodiscard]] auto StrToNum(std::string_view str, chars_format fmt = chars_format::general)noexcept
		->STN_RETURN_TYPE(FloatingT, char)
	{
		assert(!str.empty());

		FloatingT TData;
		const auto result = impl::_Floating_from_chars(str.data(), str.data() + str.size(), TData, fmt);
		if (result.ec == errc()) {
			return TData;
		}
		return STN_RETURN_VALUE(result)
	}

	template<typename FloatingT> requires std::is_floating_point_v<FloatingT>
	[[nodiscard]] auto StrToNum(std::wstring_view wstr, chars_format fmt = chars_format::general)noexcept
		->STN_RETURN_TYPE(FloatingT, wchar_t)
	{
		assert(!wstr.empty());

		FloatingT TData;
		const auto result = impl::_Floating_from_chars(wstr.data(), wstr.data() + wstr.size(), TData, fmt);
		if (result.ec == errc()) {
			return TData;
		}
		return STN_RETURN_VALUE(result)
	}


	//Aliases with predefined types, for convenience.

	[[nodiscard]] inline auto StrToChar(std::string_view str, int iBase = 0)noexcept
		->STN_RETURN_TYPE(char, char) {
		return StrToNum<char>(str, iBase);
	}

	[[nodiscard]] inline auto StrToChar(std::wstring_view wstr, int iBase = 0)noexcept
		->STN_RETURN_TYPE(char, wchar_t) {
		return StrToNum<char>(wstr, iBase);
	}

	[[nodiscard]] inline auto StrToUChar(std::string_view str, int iBase = 0)noexcept
		->STN_RETURN_TYPE(unsigned char, char) {
		return StrToNum<unsigned char>(str, iBase);
	}

	[[nodiscard]] inline auto StrToUChar(std::wstring_view wstr, int iBase = 0)noexcept
		->STN_RETURN_TYPE(unsigned char, wchar_t) {
		return StrToNum<unsigned char>(wstr, iBase);
	}

	[[nodiscard]] inline auto StrToShort(std::string_view str, int iBase = 0)noexcept
		->STN_RETURN_TYPE(short, char) {
		return StrToNum<short>(str, iBase);
	}

	[[nodiscard]] inline auto StrToShort(std::wstring_view wstr, int iBase = 0)noexcept
		->STN_RETURN_TYPE(short, wchar_t) {
		return StrToNum<short>(wstr, iBase);
	}

	[[nodiscard]] inline auto StrToUShort(std::string_view str, int iBase = 0)noexcept
		->STN_RETURN_TYPE(unsigned short, char) {
		return StrToNum<unsigned short>(str, iBase);
	}

	[[nodiscard]] inline auto StrToUShort(std::wstring_view wstr, int iBase = 0)noexcept
		->STN_RETURN_TYPE(unsigned short, wchar_t) {
		return StrToNum<unsigned short>(wstr, iBase);
	}

	[[nodiscard]] inline auto StrToInt(std::string_view str, int iBase = 0)noexcept
		->STN_RETURN_TYPE(int, char) {
		return StrToNum<int>(str, iBase);
	}

	[[nodiscard]] inline auto StrToInt(std::wstring_view wstr, int iBase = 0)noexcept
		->STN_RETURN_TYPE(int, wchar_t) {
		return StrToNum<int>(wstr, iBase);
	}

	[[nodiscard]] inline auto StrToUInt(std::string_view str, int iBase = 0)noexcept
		->STN_RETURN_TYPE(unsigned int, char) {
		return StrToNum<unsigned int>(str, iBase);
	}

	[[nodiscard]] inline auto StrToUInt(std::wstring_view wstr, int iBase = 0)noexcept
		->STN_RETURN_TYPE(unsigned int, wchar_t) {
		return StrToNum<unsigned int>(wstr, iBase);
	}

	[[nodiscard]] inline auto StrToLL(std::string_view str, int iBase = 0)noexcept
		->STN_RETURN_TYPE(long long, char) {
		return StrToNum<long long>(str, iBase);
	}

	[[nodiscard]] inline auto StrToLL(std::wstring_view wstr, int iBase = 0)noexcept
		->STN_RETURN_TYPE(long long, wchar_t) {
		return StrToNum<long long>(wstr, iBase);
	}

	[[nodiscard]] inline auto StrToULL(std::string_view str, int iBase = 0)noexcept
		->STN_RETURN_TYPE(unsigned long long, char) {
		return StrToNum<unsigned long long>(str, iBase);
	}

	[[nodiscard]] inline auto StrToULL(std::wstring_view wstr, int iBase = 0)noexcept
		->STN_RETURN_TYPE(unsigned long long, wchar_t) {
		return StrToNum<unsigned long long>(wstr, iBase);
	}

	[[nodiscard]] inline auto StrToFloat(std::string_view str, chars_format fmt = chars_format::general)noexcept
		->STN_RETURN_TYPE(float, char) {
		return StrToNum<float>(str, fmt);
	}

	[[nodiscard]] inline auto StrToFloat(std::wstring_view wstr, chars_format fmt = chars_format::general)noexcept
		->STN_RETURN_TYPE(float, wchar_t) {
		return StrToNum<float>(wstr, fmt);
	}

	[[nodiscard]] inline auto StrToDouble(std::string_view str, chars_format fmt = chars_format::general)noexcept
		->STN_RETURN_TYPE(double, char) {
		return StrToNum<double>(str, fmt);
	}

	[[nodiscard]] inline auto StrToDouble(std::wstring_view wstr, chars_format fmt = chars_format::general)noexcept
		->STN_RETURN_TYPE(double, wchar_t) {
		return StrToNum<double>(wstr, fmt);
	}
};