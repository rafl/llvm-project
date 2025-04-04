//===-- Unittests for the UInt integer class ------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/__support/CPP/optional.h"
#include "src/__support/big_int.h"
#include "src/__support/integer_literals.h" // parse_unsigned_bigint
#include "src/__support/macros/config.h"
#include "src/__support/macros/properties/types.h" // LIBC_TYPES_HAS_INT128

#include "hdr/math_macros.h" // HUGE_VALF, HUGE_VALF
#include "test/UnitTest/Test.h"

namespace LIBC_NAMESPACE_DECL {

enum Value { ZERO, ONE, TWO, MIN, MAX };

template <typename T> auto create(Value value) {
  switch (value) {
  case ZERO:
    return T(0);
  case ONE:
    return T(1);
  case TWO:
    return T(2);
  case MIN:
    return T::min();
  case MAX:
    return T::max();
  }
  __builtin_unreachable();
}

using Types = testing::TypeList< //
#ifdef LIBC_TYPES_HAS_INT64
    BigInt<64, false, uint64_t>, // 64-bits unsigned (1 x uint64_t)
    BigInt<64, true, uint64_t>,  // 64-bits   signed (1 x uint64_t)
#endif
#ifdef LIBC_TYPES_HAS_INT128
    BigInt<128, false, __uint128_t>, // 128-bits unsigned (1 x __uint128_t)
    BigInt<128, true, __uint128_t>,  // 128-bits   signed (1 x __uint128_t)
#endif
    BigInt<16, false, uint16_t>, // 16-bits unsigned (1 x uint16_t)
    BigInt<16, true, uint16_t>,  // 16-bits   signed (1 x uint16_t)
    BigInt<64, false, uint16_t>, // 64-bits unsigned (4 x uint16_t)
    BigInt<64, true, uint16_t>   // 64-bits   signed (4 x uint16_t)
    >;

#define ASSERT_SAME(A, B) ASSERT_TRUE((A) == (B))

TYPED_TEST(LlvmLibcUIntClassTest, Additions, Types) {
  ASSERT_SAME(create<T>(ZERO) + create<T>(ZERO), create<T>(ZERO));
  ASSERT_SAME(create<T>(ONE) + create<T>(ZERO), create<T>(ONE));
  ASSERT_SAME(create<T>(ZERO) + create<T>(ONE), create<T>(ONE));
  ASSERT_SAME(create<T>(ONE) + create<T>(ONE), create<T>(TWO));
  // 2's complement addition works for signed and unsigned types.
  // - unsigned : 0xff + 0x01 = 0x00 (255 + 1 = 0)
  // -   signed : 0xef + 0x01 = 0xf0 (127 + 1 = -128)
  ASSERT_SAME(create<T>(MAX) + create<T>(ONE), create<T>(MIN));
}

TYPED_TEST(LlvmLibcUIntClassTest, Subtraction, Types) {
  ASSERT_SAME(create<T>(ZERO) - create<T>(ZERO), create<T>(ZERO));
  ASSERT_SAME(create<T>(ONE) - create<T>(ONE), create<T>(ZERO));
  ASSERT_SAME(create<T>(ONE) - create<T>(ZERO), create<T>(ONE));
  // 2's complement subtraction works for signed and unsigned types.
  // - unsigned : 0x00 - 0x01 = 0xff (   0 - 1 = 255)
  // -   signed : 0xf0 - 0x01 = 0xef (-128 - 1 = 127)
  ASSERT_SAME(create<T>(MIN) - create<T>(ONE), create<T>(MAX));
}

TYPED_TEST(LlvmLibcUIntClassTest, Multiplication, Types) {
  ASSERT_SAME(create<T>(ZERO) * create<T>(ZERO), create<T>(ZERO));
  ASSERT_SAME(create<T>(ZERO) * create<T>(ONE), create<T>(ZERO));
  ASSERT_SAME(create<T>(ONE) * create<T>(ZERO), create<T>(ZERO));
  ASSERT_SAME(create<T>(ONE) * create<T>(ONE), create<T>(ONE));
  ASSERT_SAME(create<T>(ONE) * create<T>(TWO), create<T>(TWO));
  ASSERT_SAME(create<T>(TWO) * create<T>(ONE), create<T>(TWO));
  // - unsigned : 0xff x 0xff = 0x01 (mod 0xff)
  // -   signed : 0xef x 0xef = 0x01 (mod 0xff)
  ASSERT_SAME(create<T>(MAX) * create<T>(MAX), create<T>(ONE));
}

template <typename T> void print(const char *msg, T value) {
  testing::tlog << msg;
  IntegerToString<T, radix::Hex> buffer(value);
  testing::tlog << buffer.view() << "\n";
}

TEST(LlvmLibcUIntClassTest, SignedAddSub) {
  // Computations performed by https://www.wolframalpha.com/
  using T = BigInt<128, true, uint32_t>;
  const T a = parse_bigint<T>("1927508279017230597");
  const T b = parse_bigint<T>("278789278723478925");
  const T s = parse_bigint<T>("2206297557740709522");
  // Addition
  ASSERT_SAME(a + b, s);
  ASSERT_SAME(b + a, s); // commutative
  // Subtraction
  ASSERT_SAME(a - s, -b);
  ASSERT_SAME(s - a, b);
}

TEST(LlvmLibcUIntClassTest, SignedMulDiv) {
  // Computations performed by https://www.wolframalpha.com/
  using T = BigInt<128, true, uint16_t>;
  struct {
    const char *a;
    const char *b;
    const char *mul;
  } const test_cases[] = {{"-4", "3", "-12"},
                          {"-3", "-3", "9"},
                          {"1927508279017230597", "278789278723478925",
                           "537368642840747885329125014794668225"}};
  for (auto tc : test_cases) {
    const T a = parse_bigint<T>(tc.a);
    const T b = parse_bigint<T>(tc.b);
    const T mul = parse_bigint<T>(tc.mul);
    // Multiplication
    ASSERT_SAME(a * b, mul);
    ASSERT_SAME(b * a, mul);   // commutative
    ASSERT_SAME(a * -b, -mul); // sign
    ASSERT_SAME(-a * b, -mul); // sign
    ASSERT_SAME(-a * -b, mul); // sign
    // Division
    ASSERT_SAME(mul / a, b);
    ASSERT_SAME(mul / b, a);
    ASSERT_SAME(-mul / a, -b); // sign
    ASSERT_SAME(mul / -a, -b); // sign
    ASSERT_SAME(-mul / -a, b); // sign
  }
}

TYPED_TEST(LlvmLibcUIntClassTest, Division, Types) {
  ASSERT_SAME(create<T>(ZERO) / create<T>(ONE), create<T>(ZERO));
  ASSERT_SAME(create<T>(MAX) / create<T>(ONE), create<T>(MAX));
  ASSERT_SAME(create<T>(MAX) / create<T>(MAX), create<T>(ONE));
  ASSERT_SAME(create<T>(ONE) / create<T>(ONE), create<T>(ONE));
  if constexpr (T::SIGNED) {
    // Special case found by fuzzing.
    ASSERT_SAME(create<T>(MIN) / create<T>(MIN), create<T>(ONE));
  }
  // - unsigned : 0xff / 0x02 = 0x7f
  // -   signed : 0xef / 0x02 = 0x77
  ASSERT_SAME(create<T>(MAX) / create<T>(TWO), (create<T>(MAX) >> 1));

  using word_type = typename T::word_type;
  const T zero_one_repeated = T::all_ones() / T(0xff);
  const word_type pattern = word_type(~0) / word_type(0xff);
  for (const word_type part : zero_one_repeated.val) {
    if constexpr (T::SIGNED == false) {
      EXPECT_EQ(part, pattern);
    }
  }
}

TYPED_TEST(LlvmLibcUIntClassTest, is_neg, Types) {
  EXPECT_FALSE(create<T>(ZERO).is_neg());
  EXPECT_FALSE(create<T>(ONE).is_neg());
  EXPECT_FALSE(create<T>(TWO).is_neg());
  EXPECT_EQ(create<T>(MIN).is_neg(), T::SIGNED);
  EXPECT_FALSE(create<T>(MAX).is_neg());
}

TYPED_TEST(LlvmLibcUIntClassTest, Masks, Types) {
  if constexpr (!T::SIGNED) {
    constexpr size_t BITS = T::BITS;
    // mask_trailing_ones
    ASSERT_SAME((mask_trailing_ones<T, 0>()), T::zero());
    ASSERT_SAME((mask_trailing_ones<T, 1>()), T::one());
    ASSERT_SAME((mask_trailing_ones<T, BITS - 1>()), T::all_ones() >> 1);
    ASSERT_SAME((mask_trailing_ones<T, BITS>()), T::all_ones());
    // mask_leading_ones
    ASSERT_SAME((mask_leading_ones<T, 0>()), T::zero());
    ASSERT_SAME((mask_leading_ones<T, 1>()), T::one() << (BITS - 1));
    ASSERT_SAME((mask_leading_ones<T, BITS - 1>()), T::all_ones() - T::one());
    ASSERT_SAME((mask_leading_ones<T, BITS>()), T::all_ones());
    // mask_trailing_zeros
    ASSERT_SAME((mask_trailing_zeros<T, 0>()), T::all_ones());
    ASSERT_SAME((mask_trailing_zeros<T, 1>()), T::all_ones() - T::one());
    ASSERT_SAME((mask_trailing_zeros<T, BITS - 1>()), T::one() << (BITS - 1));
    ASSERT_SAME((mask_trailing_zeros<T, BITS>()), T::zero());
    // mask_trailing_zeros
    ASSERT_SAME((mask_leading_zeros<T, 0>()), T::all_ones());
    ASSERT_SAME((mask_leading_zeros<T, 1>()), T::all_ones() >> 1);
    ASSERT_SAME((mask_leading_zeros<T, BITS - 1>()), T::one());
    ASSERT_SAME((mask_leading_zeros<T, BITS>()), T::zero());
  }
}

TYPED_TEST(LlvmLibcUIntClassTest, CountBits, Types) {
  if constexpr (!T::SIGNED) {
    for (size_t i = 0; i < T::BITS; ++i) {
      const auto l_one = T::all_ones() << i; // 0b111...000
      const auto r_one = T::all_ones() >> i; // 0b000...111
      const int zeros = static_cast<int>(i);
      const int ones = static_cast<int>(T::BITS - static_cast<size_t>(zeros));
      ASSERT_EQ(cpp::countr_one(r_one), ones);
      ASSERT_EQ(cpp::countl_one(l_one), ones);
      ASSERT_EQ(cpp::countr_zero(l_one), zeros);
      ASSERT_EQ(cpp::countl_zero(r_one), zeros);
    }
  }
}

using LL_UInt16 = UInt<16>;
using LL_UInt32 = UInt<32>;
using LL_UInt64 = UInt<64>;
// We want to test UInt<128> explicitly. So, for
// convenience, we use a sugar which does not conflict with the UInt128 type
// which can resolve to __uint128_t if the platform has it.
using LL_UInt128 = UInt<128>;
using LL_UInt192 = UInt<192>;
using LL_UInt256 = UInt<256>;
using LL_UInt320 = UInt<320>;
using LL_UInt512 = UInt<512>;
using LL_UInt1024 = UInt<1024>;

using LL_Int128 = Int<128>;
using LL_Int192 = Int<192>;

TEST(LlvmLibcUIntClassTest, BitCastToFromDouble) {
  static_assert(cpp::is_trivially_copyable<LL_UInt64>::value);
  static_assert(sizeof(LL_UInt64) == sizeof(double));
  const double inf = HUGE_VAL;
  const double max = DBL_MAX;
  const double array[] = {0.0, 0.1, 1.0, max, inf};
  for (double value : array) {
    LL_UInt64 back = cpp::bit_cast<LL_UInt64>(value);
    double forth = cpp::bit_cast<double>(back);
    EXPECT_TRUE(value == forth);
  }
}

#ifdef LIBC_TYPES_HAS_INT128
TEST(LlvmLibcUIntClassTest, BitCastToFromNativeUint128) {
  static_assert(cpp::is_trivially_copyable<LL_UInt128>::value);
  static_assert(sizeof(LL_UInt128) == sizeof(__uint128_t));
  const __uint128_t array[] = {0, 1, ~__uint128_t(0)};
  for (__uint128_t value : array) {
    LL_UInt128 back = cpp::bit_cast<LL_UInt128>(value);
    __uint128_t forth = cpp::bit_cast<__uint128_t>(back);
    EXPECT_TRUE(value == forth);
  }
}
#endif // LIBC_TYPES_HAS_INT128

#ifdef LIBC_TYPES_HAS_FLOAT128
TEST(LlvmLibcUIntClassTest, BitCastToFromNativeFloat128) {
  static_assert(cpp::is_trivially_copyable<LL_UInt128>::value);
  static_assert(sizeof(LL_UInt128) == sizeof(float128));
  const float128 array[] = {0, 0.1, 1};
  for (float128 value : array) {
    LL_UInt128 back = cpp::bit_cast<LL_UInt128>(value);
    float128 forth = cpp::bit_cast<float128>(back);
    EXPECT_TRUE(value == forth);
  }
}
#endif // LIBC_TYPES_HAS_FLOAT128

#ifdef LIBC_TYPES_HAS_FLOAT16
TEST(LlvmLibcUIntClassTest, BitCastToFromNativeFloat16) {
  static_assert(cpp::is_trivially_copyable<LL_UInt16>::value);
  static_assert(sizeof(LL_UInt16) == sizeof(float16));
  const float16 array[] = {
      static_cast<float16>(0.0),
      static_cast<float16>(0.1),
      static_cast<float16>(1.0),
  };
  for (float16 value : array) {
    LL_UInt16 back = cpp::bit_cast<LL_UInt16>(value);
    float16 forth = cpp::bit_cast<float16>(back);
    EXPECT_TRUE(value == forth);
  }
}
#endif // LIBC_TYPES_HAS_FLOAT16

TEST(LlvmLibcUIntClassTest, BasicInit) {
  LL_UInt128 half_val(12345);
  LL_UInt128 full_val({12345, 67890});
  ASSERT_TRUE(half_val != full_val);
}

TEST(LlvmLibcUIntClassTest, AdditionTests) {
  LL_UInt128 val1(12345);
  LL_UInt128 val2(54321);
  LL_UInt128 result1(66666);
  EXPECT_EQ(val1 + val2, result1);
  EXPECT_EQ((val1 + val2), (val2 + val1)); // addition is commutative

  // Test overflow
  LL_UInt128 val3({0xf000000000000001, 0});
  LL_UInt128 val4({0x100000000000000f, 0});
  LL_UInt128 result2({0x10, 0x1});
  EXPECT_EQ(val3 + val4, result2);
  EXPECT_EQ(val3 + val4, val4 + val3);

  // Test overflow
  LL_UInt128 val5({0x0123456789abcdef, 0xfedcba9876543210});
  LL_UInt128 val6({0x1111222233334444, 0xaaaabbbbccccdddd});
  LL_UInt128 result3({0x12346789bcdf1233, 0xa987765443210fed});
  EXPECT_EQ(val5 + val6, result3);
  EXPECT_EQ(val5 + val6, val6 + val5);

  // Test 192-bit addition
  LL_UInt192 val7({0x0123456789abcdef, 0xfedcba9876543210, 0xfedcba9889abcdef});
  LL_UInt192 val8({0x1111222233334444, 0xaaaabbbbccccdddd, 0xeeeeffffeeeeffff});
  LL_UInt192 result4(
      {0x12346789bcdf1233, 0xa987765443210fed, 0xedcbba98789acdef});
  EXPECT_EQ(val7 + val8, result4);
  EXPECT_EQ(val7 + val8, val8 + val7);

  // Test 256-bit addition
  LL_UInt256 val9({0x1f1e1d1c1b1a1918, 0xf1f2f3f4f5f6f7f8, 0x0123456789abcdef,
                   0xfedcba9876543210});
  LL_UInt256 val10({0x1111222233334444, 0xaaaabbbbccccdddd, 0x1111222233334444,
                    0xaaaabbbbccccdddd});
  LL_UInt256 result5({0x302f3f3e4e4d5d5c, 0x9c9dafb0c2c3d5d5,
                      0x12346789bcdf1234, 0xa987765443210fed});
  EXPECT_EQ(val9 + val10, result5);
  EXPECT_EQ(val9 + val10, val10 + val9);
}

TEST(LlvmLibcUIntClassTest, SubtractionTests) {
  LL_UInt128 val1(12345);
  LL_UInt128 val2(54321);
  LL_UInt128 result1({0xffffffffffff5c08, 0xffffffffffffffff});
  LL_UInt128 result2(0xa3f8);
  EXPECT_EQ(val1 - val2, result1);
  EXPECT_EQ(val1, val2 + result1);
  EXPECT_EQ(val2 - val1, result2);
  EXPECT_EQ(val2, val1 + result2);

  LL_UInt128 val3({0xf000000000000001, 0});
  LL_UInt128 val4({0x100000000000000f, 0});
  LL_UInt128 result3(0xdffffffffffffff2);
  LL_UInt128 result4({0x200000000000000e, 0xffffffffffffffff});
  EXPECT_EQ(val3 - val4, result3);
  EXPECT_EQ(val3, val4 + result3);
  EXPECT_EQ(val4 - val3, result4);
  EXPECT_EQ(val4, val3 + result4);

  LL_UInt128 val5({0x0123456789abcdef, 0xfedcba9876543210});
  LL_UInt128 val6({0x1111222233334444, 0xaaaabbbbccccdddd});
  LL_UInt128 result5({0xf0122345567889ab, 0x5431fedca9875432});
  LL_UInt128 result6({0x0feddcbaa9877655, 0xabce01235678abcd});
  EXPECT_EQ(val5 - val6, result5);
  EXPECT_EQ(val5, val6 + result5);
  EXPECT_EQ(val6 - val5, result6);
  EXPECT_EQ(val6, val5 + result6);
}

TEST(LlvmLibcUIntClassTest, MultiplicationTests) {
  LL_UInt128 val1({5, 0});
  LL_UInt128 val2({10, 0});
  LL_UInt128 result1({50, 0});
  EXPECT_EQ((val1 * val2), result1);
  EXPECT_EQ((val1 * val2), (val2 * val1)); // multiplication is commutative

  // Check that the multiplication works accross the whole number
  LL_UInt128 val3({0xf, 0});
  LL_UInt128 val4({0x1111111111111111, 0x1111111111111111});
  LL_UInt128 result2({0xffffffffffffffff, 0xffffffffffffffff});
  EXPECT_EQ((val3 * val4), result2);
  EXPECT_EQ((val3 * val4), (val4 * val3));

  // Check that multiplication doesn't reorder the bits.
  LL_UInt128 val5({2, 0});
  LL_UInt128 val6({0x1357024675316420, 0x0123456776543210});
  LL_UInt128 result3({0x26ae048cea62c840, 0x02468aceeca86420});

  EXPECT_EQ((val5 * val6), result3);
  EXPECT_EQ((val5 * val6), (val6 * val5));

  // Make sure that multiplication handles overflow correctly.
  LL_UInt128 val7(2);
  LL_UInt128 val8({0x8000800080008000, 0x8000800080008000});
  LL_UInt128 result4({0x0001000100010000, 0x0001000100010001});
  EXPECT_EQ((val7 * val8), result4);
  EXPECT_EQ((val7 * val8), (val8 * val7));

  // val9 is the 128 bit mantissa of 1e60 as a float, val10 is the mantissa for
  // 1e-60. They almost cancel on the high bits, but the result we're looking
  // for is just the low bits. The full result would be
  // 0x7fffffffffffffffffffffffffffffff3a4f32d17f40d08f917cf11d1e039c50
  LL_UInt128 val9({0x01D762422C946590, 0x9F4F2726179A2245});
  LL_UInt128 val10({0x3792F412CB06794D, 0xCDB02555653131B6});
  LL_UInt128 result5({0x917cf11d1e039c50, 0x3a4f32d17f40d08f});
  EXPECT_EQ((val9 * val10), result5);
  EXPECT_EQ((val9 * val10), (val10 * val9));

  // Test 192-bit multiplication
  LL_UInt192 val11(
      {0xffffffffffffffff, 0x01D762422C946590, 0x9F4F2726179A2245});
  LL_UInt192 val12(
      {0xffffffffffffffff, 0x3792F412CB06794D, 0xCDB02555653131B6});

  LL_UInt192 result6(
      {0x0000000000000001, 0xc695a9ab08652121, 0x5de7faf698d32732});
  EXPECT_EQ((val11 * val12), result6);
  EXPECT_EQ((val11 * val12), (val12 * val11));

  LL_UInt256 val13({0xffffffffffffffff, 0x01D762422C946590, 0x9F4F2726179A2245,
                    0xffffffffffffffff});
  LL_UInt256 val14({0xffffffffffffffff, 0xffffffffffffffff, 0x3792F412CB06794D,
                    0xCDB02555653131B6});
  LL_UInt256 result7({0x0000000000000001, 0xfe289dbdd36b9a6f,
                      0x291de4c71d5f646c, 0xfd37221cb06d4978});
  EXPECT_EQ((val13 * val14), result7);
  EXPECT_EQ((val13 * val14), (val14 * val13));
}

TEST(LlvmLibcUIntClassTest, DivisionTests) {
  LL_UInt128 val1({10, 0});
  LL_UInt128 val2({5, 0});
  LL_UInt128 result1({2, 0});
  EXPECT_EQ((val1 / val2), result1);
  EXPECT_EQ((val1 / result1), val2);

  // Check that the division works accross the whole number
  LL_UInt128 val3({0xffffffffffffffff, 0xffffffffffffffff});
  LL_UInt128 val4({0xf, 0});
  LL_UInt128 result2({0x1111111111111111, 0x1111111111111111});
  EXPECT_EQ((val3 / val4), result2);
  EXPECT_EQ((val3 / result2), val4);

  // Check that division doesn't reorder the bits.
  LL_UInt128 val5({0x26ae048cea62c840, 0x02468aceeca86420});
  LL_UInt128 val6({2, 0});
  LL_UInt128 result3({0x1357024675316420, 0x0123456776543210});
  EXPECT_EQ((val5 / val6), result3);
  EXPECT_EQ((val5 / result3), val6);

  // Make sure that division handles inexact results correctly.
  LL_UInt128 val7({1001, 0});
  LL_UInt128 val8({10, 0});
  LL_UInt128 result4({100, 0});
  EXPECT_EQ((val7 / val8), result4);
  EXPECT_EQ((val7 / result4), val8);

  // Make sure that division handles divisors of one correctly.
  LL_UInt128 val9({0x1234567812345678, 0x9abcdef09abcdef0});
  LL_UInt128 val10({1, 0});
  LL_UInt128 result5({0x1234567812345678, 0x9abcdef09abcdef0});
  EXPECT_EQ((val9 / val10), result5);
  EXPECT_EQ((val9 / result5), val10);

  // Make sure that division handles results of slightly more than 1 correctly.
  LL_UInt128 val11({1050, 0});
  LL_UInt128 val12({1030, 0});
  LL_UInt128 result6({1, 0});
  EXPECT_EQ((val11 / val12), result6);

  // Make sure that division handles dividing by zero correctly.
  LL_UInt128 val13({1234, 0});
  LL_UInt128 val14({0, 0});
  EXPECT_FALSE(val13.div(val14).has_value());
}

TEST(LlvmLibcUIntClassTest, ModuloTests) {
  LL_UInt128 val1({10, 0});
  LL_UInt128 val2({5, 0});
  LL_UInt128 result1({0, 0});
  EXPECT_EQ((val1 % val2), result1);

  LL_UInt128 val3({101, 0});
  LL_UInt128 val4({10, 0});
  LL_UInt128 result2({1, 0});
  EXPECT_EQ((val3 % val4), result2);

  LL_UInt128 val5({10000001, 0});
  LL_UInt128 val6({10, 0});
  LL_UInt128 result3({1, 0});
  EXPECT_EQ((val5 % val6), result3);

  LL_UInt128 val7({12345, 10});
  LL_UInt128 val8({0, 1});
  LL_UInt128 result4({12345, 0});
  EXPECT_EQ((val7 % val8), result4);

  LL_UInt128 val9({12345, 10});
  LL_UInt128 val10({0, 11});
  LL_UInt128 result5({12345, 10});
  EXPECT_EQ((val9 % val10), result5);

  LL_UInt128 val11({10, 10});
  LL_UInt128 val12({10, 10});
  LL_UInt128 result6({0, 0});
  EXPECT_EQ((val11 % val12), result6);

  LL_UInt128 val13({12345, 0});
  LL_UInt128 val14({1, 0});
  LL_UInt128 result7({0, 0});
  EXPECT_EQ((val13 % val14), result7);

  LL_UInt128 val15({0xffffffffffffffff, 0xffffffffffffffff});
  LL_UInt128 val16({0x1111111111111111, 0x111111111111111});
  LL_UInt128 result8({0xf, 0});
  EXPECT_EQ((val15 % val16), result8);

  LL_UInt128 val17({5076944270305263619, 54210108624}); // (10 ^ 30) + 3
  LL_UInt128 val18({10, 0});
  LL_UInt128 result9({3, 0});
  EXPECT_EQ((val17 % val18), result9);
}

TEST(LlvmLibcUIntClassTest, PowerTests) {
  LL_UInt128 val1({10, 0});
  val1.pow_n(30);
  LL_UInt128 result1({5076944270305263616, 54210108624}); // (10 ^ 30)
  EXPECT_EQ(val1, result1);

  LL_UInt128 val2({1, 0});
  val2.pow_n(10);
  LL_UInt128 result2({1, 0});
  EXPECT_EQ(val2, result2);

  LL_UInt128 val3({0, 0});
  val3.pow_n(10);
  LL_UInt128 result3({0, 0});
  EXPECT_EQ(val3, result3);

  LL_UInt128 val4({10, 0});
  val4.pow_n(0);
  LL_UInt128 result4({1, 0});
  EXPECT_EQ(val4, result4);

  // Test zero to the zero. Currently it returns 1, since that's the easiest
  // result.
  LL_UInt128 val5({0, 0});
  val5.pow_n(0);
  LL_UInt128 result5({1, 0});
  EXPECT_EQ(val5, result5);

  // Test a number that overflows. 100 ^ 20 is larger than 2 ^ 128.
  LL_UInt128 val6({100, 0});
  val6.pow_n(20);
  LL_UInt128 result6({0xb9f5610000000000, 0x6329f1c35ca4bfab});
  EXPECT_EQ(val6, result6);

  // Test that both halves of the number are being used.
  LL_UInt128 val7({1, 1});
  val7.pow_n(2);
  LL_UInt128 result7({1, 2});
  EXPECT_EQ(val7, result7);

  LL_UInt128 val_pow_two;
  LL_UInt128 result_pow_two;
  for (size_t i = 0; i < 128; ++i) {
    val_pow_two = 2;
    val_pow_two.pow_n(i);
    result_pow_two = 1;
    result_pow_two = result_pow_two << i;
    EXPECT_EQ(val_pow_two, result_pow_two);
  }
}

TEST(LlvmLibcUIntClassTest, ShiftLeftTests) {
  LL_UInt128 val1(0x0123456789abcdef);
  LL_UInt128 result1(0x123456789abcdef0);
  EXPECT_EQ((val1 << 4), result1);

  LL_UInt128 val2({0x13579bdf02468ace, 0x123456789abcdef0});
  LL_UInt128 result2({0x02468ace00000000, 0x9abcdef013579bdf});
  EXPECT_EQ((val2 << 32), result2);
  LL_UInt128 val22 = val2;
  val22 <<= 32;
  EXPECT_EQ(val22, result2);

  LL_UInt128 result3({0, 0x13579bdf02468ace});
  EXPECT_EQ((val2 << 64), result3);

  LL_UInt128 result4({0, 0x02468ace00000000});
  EXPECT_EQ((val2 << 96), result4);

  LL_UInt128 result5({0, 0x2468ace000000000});
  EXPECT_EQ((val2 << 100), result5);

  LL_UInt192 val3({1, 0, 0});
  LL_UInt192 result7({0, 1, 0});
  EXPECT_EQ((val3 << 64), result7);
}

TEST(LlvmLibcUIntClassTest, ShiftRightTests) {
  LL_UInt128 val1(0x0123456789abcdef);
  LL_UInt128 result1(0x00123456789abcde);
  EXPECT_EQ((val1 >> 4), result1);

  LL_UInt128 val2({0x13579bdf02468ace, 0x123456789abcdef0});
  LL_UInt128 result2({0x9abcdef013579bdf, 0x0000000012345678});
  EXPECT_EQ((val2 >> 32), result2);
  LL_UInt128 val22 = val2;
  val22 >>= 32;
  EXPECT_EQ(val22, result2);

  LL_UInt128 result3({0x123456789abcdef0, 0});
  EXPECT_EQ((val2 >> 64), result3);

  LL_UInt128 result4({0x0000000012345678, 0});
  EXPECT_EQ((val2 >> 96), result4);

  LL_UInt128 result5({0x0000000001234567, 0});
  EXPECT_EQ((val2 >> 100), result5);

  LL_UInt128 v1({0x1111222233334444, 0xaaaabbbbccccdddd});
  LL_UInt128 r1({0xaaaabbbbccccdddd, 0});
  EXPECT_EQ((v1 >> 64), r1);

  LL_UInt192 v2({0x1111222233334444, 0x5555666677778888, 0xaaaabbbbccccdddd});
  LL_UInt192 r2({0x5555666677778888, 0xaaaabbbbccccdddd, 0});
  LL_UInt192 r3({0xaaaabbbbccccdddd, 0, 0});
  EXPECT_EQ((v2 >> 64), r2);
  EXPECT_EQ((v2 >> 128), r3);
  EXPECT_EQ((r2 >> 64), r3);

  LL_UInt192 val3({0, 0, 1});
  LL_UInt192 result7({0, 1, 0});
  EXPECT_EQ((val3 >> 64), result7);
}

TEST(LlvmLibcUIntClassTest, AndTests) {
  LL_UInt128 base({0xffff00000000ffff, 0xffffffff00000000});
  LL_UInt128 val128({0xf0f0f0f00f0f0f0f, 0xff00ff0000ff00ff});
  uint64_t val64 = 0xf0f0f0f00f0f0f0f;
  int val32 = 0x0f0f0f0f;
  LL_UInt128 result128({0xf0f0000000000f0f, 0xff00ff0000000000});
  LL_UInt128 result64(0xf0f0000000000f0f);
  LL_UInt128 result32(0x00000f0f);
  EXPECT_EQ((base & val128), result128);
  EXPECT_EQ((base & val64), result64);
  EXPECT_EQ((base & val32), result32);
}

TEST(LlvmLibcUIntClassTest, OrTests) {
  LL_UInt128 base({0xffff00000000ffff, 0xffffffff00000000});
  LL_UInt128 val128({0xf0f0f0f00f0f0f0f, 0xff00ff0000ff00ff});
  uint64_t val64 = 0xf0f0f0f00f0f0f0f;
  int val32 = 0x0f0f0f0f;
  LL_UInt128 result128({0xfffff0f00f0fffff, 0xffffffff00ff00ff});
  LL_UInt128 result64({0xfffff0f00f0fffff, 0xffffffff00000000});
  LL_UInt128 result32({0xffff00000f0fffff, 0xffffffff00000000});
  EXPECT_EQ((base | val128), result128);
  EXPECT_EQ((base | val64), result64);
  EXPECT_EQ((base | val32), result32);
}

TEST(LlvmLibcUIntClassTest, CompoundAssignments) {
  LL_UInt128 x({0xffff00000000ffff, 0xffffffff00000000});
  LL_UInt128 b({0xf0f0f0f00f0f0f0f, 0xff00ff0000ff00ff});

  LL_UInt128 a = x;
  a |= b;
  LL_UInt128 or_result({0xfffff0f00f0fffff, 0xffffffff00ff00ff});
  EXPECT_EQ(a, or_result);

  a = x;
  a &= b;
  LL_UInt128 and_result({0xf0f0000000000f0f, 0xff00ff0000000000});
  EXPECT_EQ(a, and_result);

  a = x;
  a ^= b;
  LL_UInt128 xor_result({0x0f0ff0f00f0ff0f0, 0x00ff00ff00ff00ff});
  EXPECT_EQ(a, xor_result);

  a = LL_UInt128(uint64_t(0x0123456789abcdef));
  LL_UInt128 shift_left_result(uint64_t(0x123456789abcdef0));
  a <<= 4;
  EXPECT_EQ(a, shift_left_result);

  a = LL_UInt128(uint64_t(0x123456789abcdef1));
  LL_UInt128 shift_right_result(uint64_t(0x0123456789abcdef));
  a >>= 4;
  EXPECT_EQ(a, shift_right_result);

  a = LL_UInt128({0xf000000000000001, 0});
  b = LL_UInt128({0x100000000000000f, 0});
  LL_UInt128 add_result({0x10, 0x1});
  a += b;
  EXPECT_EQ(a, add_result);

  a = LL_UInt128({0xf, 0});
  b = LL_UInt128({0x1111111111111111, 0x1111111111111111});
  LL_UInt128 mul_result({0xffffffffffffffff, 0xffffffffffffffff});
  a *= b;
  EXPECT_EQ(a, mul_result);
}

TEST(LlvmLibcUIntClassTest, UnaryPredecrement) {
  LL_UInt128 a = LL_UInt128({0x1111111111111111, 0x1111111111111111});
  ++a;
  EXPECT_EQ(a, LL_UInt128({0x1111111111111112, 0x1111111111111111}));

  a = LL_UInt128({0xffffffffffffffff, 0x0});
  ++a;
  EXPECT_EQ(a, LL_UInt128({0x0, 0x1}));

  a = LL_UInt128({0xffffffffffffffff, 0xffffffffffffffff});
  ++a;
  EXPECT_EQ(a, LL_UInt128({0x0, 0x0}));
}

TEST(LlvmLibcUIntClassTest, EqualsTests) {
  LL_UInt128 a1({0xffffffff00000000, 0xffff00000000ffff});
  LL_UInt128 a2({0xffffffff00000000, 0xffff00000000ffff});
  LL_UInt128 b({0xff00ff0000ff00ff, 0xf0f0f0f00f0f0f0f});
  LL_UInt128 a_reversed({0xffff00000000ffff, 0xffffffff00000000});
  LL_UInt128 a_upper(0xffff00000000ffff);
  LL_UInt128 a_lower(0xffffffff00000000);
  ASSERT_TRUE(a1 == a1);
  ASSERT_TRUE(a1 == a2);
  ASSERT_FALSE(a1 == b);
  ASSERT_FALSE(a1 == a_reversed);
  ASSERT_FALSE(a1 == a_lower);
  ASSERT_FALSE(a1 == a_upper);
  ASSERT_TRUE(a_lower != a_upper);
}

TEST(LlvmLibcUIntClassTest, ComparisonTests) {
  LL_UInt128 a({0xffffffff00000000, 0xffff00000000ffff});
  LL_UInt128 b({0xff00ff0000ff00ff, 0xf0f0f0f00f0f0f0f});
  EXPECT_GT(a, b);
  EXPECT_GE(a, b);
  EXPECT_LT(b, a);
  EXPECT_LE(b, a);

  LL_UInt128 x(0xffffffff00000000);
  LL_UInt128 y(0x00000000ffffffff);
  EXPECT_GT(x, y);
  EXPECT_GE(x, y);
  EXPECT_LT(y, x);
  EXPECT_LE(y, x);

  EXPECT_LE(a, a);
  EXPECT_GE(a, a);
}

TEST(LlvmLibcUIntClassTest, FullMulTests) {
  LL_UInt128 a({0xffffffffffffffffULL, 0xffffffffffffffffULL});
  LL_UInt128 b({0xfedcba9876543210ULL, 0xfefdfcfbfaf9f8f7ULL});
  LL_UInt256 r({0x0123456789abcdf0ULL, 0x0102030405060708ULL,
                0xfedcba987654320fULL, 0xfefdfcfbfaf9f8f7ULL});
  LL_UInt128 r_hi({0xfedcba987654320eULL, 0xfefdfcfbfaf9f8f7ULL});

  EXPECT_EQ(a.ful_mul(b), r);
  EXPECT_EQ(a.quick_mul_hi(b), r_hi);

  LL_UInt192 c(
      {0x7766554433221101ULL, 0xffeeddccbbaa9988ULL, 0x1f2f3f4f5f6f7f8fULL});
  LL_UInt320 rr({0x8899aabbccddeeffULL, 0x0011223344556677ULL,
                 0x583715f4d3b29171ULL, 0xffeeddccbbaa9988ULL,
                 0x1f2f3f4f5f6f7f8fULL});

  EXPECT_EQ(a.ful_mul(c), rr);
  EXPECT_EQ(a.ful_mul(c), c.ful_mul(a));
}

#define TEST_QUICK_MUL_HI(Bits, Error)                                         \
  do {                                                                         \
    LL_UInt##Bits a = ~LL_UInt##Bits(0);                                       \
    LL_UInt##Bits hi = a.quick_mul_hi(a);                                      \
    LL_UInt##Bits trunc = static_cast<LL_UInt##Bits>(a.ful_mul(a) >> Bits);    \
    uint64_t overflow = trunc.sub_overflow(hi);                                \
    EXPECT_EQ(overflow, uint64_t(0));                                          \
    EXPECT_LE(uint64_t(trunc), uint64_t(Error));                               \
  } while (0)

TEST(LlvmLibcUIntClassTest, QuickMulHiTests) {
  TEST_QUICK_MUL_HI(128, 1);
  TEST_QUICK_MUL_HI(192, 2);
  TEST_QUICK_MUL_HI(256, 3);
  TEST_QUICK_MUL_HI(512, 7);
}

TEST(LlvmLibcUIntClassTest, ConstexprInitTests) {
  constexpr LL_UInt128 add = LL_UInt128(1) + LL_UInt128(2);
  ASSERT_EQ(add, LL_UInt128(3));
  constexpr LL_UInt128 sub = LL_UInt128(5) - LL_UInt128(4);
  ASSERT_EQ(sub, LL_UInt128(1));
}

#define TEST_QUICK_DIV_UINT32_POW2(x, e)                                       \
  do {                                                                         \
    LL_UInt320 y({0x8899aabbccddeeffULL, 0x0011223344556677ULL,                \
                  0x583715f4d3b29171ULL, 0xffeeddccbbaa9988ULL,                \
                  0x1f2f3f4f5f6f7f8fULL});                                     \
    LL_UInt320 d = LL_UInt320(x);                                              \
    d <<= e;                                                                   \
    LL_UInt320 q1 = y / d;                                                     \
    LL_UInt320 r1 = y % d;                                                     \
    LL_UInt320 r2 = *y.div_uint_half_times_pow_2(x, e);                        \
    EXPECT_EQ(q1, y);                                                          \
    EXPECT_EQ(r1, r2);                                                         \
  } while (0)

TEST(LlvmLibcUIntClassTest, DivUInt32TimesPow2Tests) {
  for (size_t i = 0; i < 320; i += 32) {
    TEST_QUICK_DIV_UINT32_POW2(1, i);
    TEST_QUICK_DIV_UINT32_POW2(13151719, i);
  }

  TEST_QUICK_DIV_UINT32_POW2(1, 75);
  TEST_QUICK_DIV_UINT32_POW2(1, 101);

  TEST_QUICK_DIV_UINT32_POW2(1000000000, 75);
  TEST_QUICK_DIV_UINT32_POW2(1000000000, 101);
}

TEST(LlvmLibcUIntClassTest, ComparisonInt128Tests) {
  LL_Int128 a(123);
  LL_Int128 b(0);
  LL_Int128 c(-1);

  ASSERT_TRUE(a == a);
  ASSERT_TRUE(b == b);
  ASSERT_TRUE(c == c);

  ASSERT_TRUE(a != b);
  ASSERT_TRUE(a != c);
  ASSERT_TRUE(b != a);
  ASSERT_TRUE(b != c);
  ASSERT_TRUE(c != a);
  ASSERT_TRUE(c != b);

  ASSERT_TRUE(a > b);
  ASSERT_TRUE(a >= b);
  ASSERT_TRUE(a > c);
  ASSERT_TRUE(a >= c);
  ASSERT_TRUE(b > c);
  ASSERT_TRUE(b >= c);

  ASSERT_TRUE(b < a);
  ASSERT_TRUE(b <= a);
  ASSERT_TRUE(c < a);
  ASSERT_TRUE(c <= a);
  ASSERT_TRUE(c < b);
  ASSERT_TRUE(c <= b);
}

TEST(LlvmLibcUIntClassTest, BasicArithmeticInt128Tests) {
  LL_Int128 a(123);
  LL_Int128 b(0);
  LL_Int128 c(-3);

  ASSERT_EQ(a * a, LL_Int128(123 * 123));
  ASSERT_EQ(a * c, LL_Int128(-369));
  ASSERT_EQ(c * a, LL_Int128(-369));
  ASSERT_EQ(c * c, LL_Int128(9));
  ASSERT_EQ(a * b, b);
  ASSERT_EQ(b * a, b);
  ASSERT_EQ(b * c, b);
  ASSERT_EQ(c * b, b);
}

#ifdef LIBC_TYPES_HAS_INT128

TEST(LlvmLibcUIntClassTest, ConstructorFromUInt128Tests) {
  __uint128_t a = (__uint128_t(123) << 64) + 1;
  __int128_t b = -static_cast<__int128_t>(a);
  LL_Int128 c(a);
  LL_Int128 d(b);

  LL_Int192 e(a);
  LL_Int192 f(b);

  ASSERT_EQ(static_cast<int>(c), 1);
  ASSERT_EQ(static_cast<int>(c >> 64), 123);
  ASSERT_EQ(static_cast<uint64_t>(d), static_cast<uint64_t>(b));
  ASSERT_EQ(static_cast<uint64_t>(d >> 64), static_cast<uint64_t>(b >> 64));
  ASSERT_EQ(c + d, LL_Int128(a + static_cast<__uint128_t>(b)));

  ASSERT_EQ(static_cast<int>(e), 1);
  ASSERT_EQ(static_cast<int>(e >> 64), 123);
  ASSERT_EQ(static_cast<uint64_t>(f), static_cast<uint64_t>(b));
  ASSERT_EQ(static_cast<uint64_t>(f >> 64), static_cast<uint64_t>(b >> 64));
  ASSERT_EQ(LL_UInt192(e + f), LL_UInt192(a + static_cast<__uint128_t>(b)));
}

TEST(LlvmLibcUIntClassTest, WordTypeUInt128Tests) {
  using LL_UInt256_128 = BigInt<256, false, __uint128_t>;
  using LL_UInt128_128 = BigInt<128, false, __uint128_t>;

  LL_UInt256_128 a(1);

  ASSERT_EQ(static_cast<int>(a), 1);
  a = (a << 128) + 2;
  ASSERT_EQ(static_cast<int>(a), 2);
  ASSERT_EQ(static_cast<uint64_t>(a), uint64_t(2));
  a = (a << 32) + 3;
  ASSERT_EQ(static_cast<int>(a), 3);
  ASSERT_EQ(static_cast<uint64_t>(a), uint64_t(0x2'0000'0003));
  ASSERT_EQ(static_cast<int>(a >> 32), 2);
  ASSERT_EQ(static_cast<int>(a >> (128 + 32)), 1);

  LL_UInt128_128 b(__uint128_t(1) << 127);
  LL_UInt128_128 c(b);
  a = b.ful_mul(c);

  ASSERT_EQ(static_cast<int>(a >> 254), 1);

  LL_UInt256_128 d = LL_UInt256_128(123) << 4;
  ASSERT_EQ(static_cast<int>(d), 123 << 4);
  LL_UInt256_128 e = a / d;
  LL_UInt256_128 f = a % d;
  LL_UInt256_128 r = *a.div_uint_half_times_pow_2(123, 4);
  EXPECT_TRUE(e == a);
  EXPECT_TRUE(f == r);
}

#endif // LIBC_TYPES_HAS_INT128

TEST(LlvmLibcUIntClassTest, OtherWordTypeTests) {
  using LL_UInt96 = BigInt<96, false, uint32_t>;

  LL_UInt96 a(1);

  ASSERT_EQ(static_cast<int>(a), 1);
  a = (a << 32) + 2;
  ASSERT_EQ(static_cast<int>(a), 2);
  ASSERT_EQ(static_cast<uint64_t>(a), uint64_t(0x1'0000'0002));
  a = (a << 32) + 3;
  ASSERT_EQ(static_cast<int>(a), 3);
  ASSERT_EQ(static_cast<int>(a >> 32), 2);
  ASSERT_EQ(static_cast<int>(a >> 64), 1);
}

TEST(LlvmLibcUIntClassTest, OtherWordTypeCastTests) {
  using LL_UInt96 = BigInt<96, false, uint32_t>;

  LL_UInt96 a({123, 456, 789});

  ASSERT_EQ(static_cast<int>(a), 123);
  ASSERT_EQ(static_cast<int>(a >> 32), 456);
  ASSERT_EQ(static_cast<int>(a >> 64), 789);

  // Bigger word with more bits to smaller word with less bits.
  LL_UInt128 b(a);

  ASSERT_EQ(static_cast<int>(b), 123);
  ASSERT_EQ(static_cast<int>(b >> 32), 456);
  ASSERT_EQ(static_cast<int>(b >> 64), 789);
  ASSERT_EQ(static_cast<int>(b >> 96), 0);

  b = (b << 32) + 987;

  ASSERT_EQ(static_cast<int>(b), 987);
  ASSERT_EQ(static_cast<int>(b >> 32), 123);
  ASSERT_EQ(static_cast<int>(b >> 64), 456);
  ASSERT_EQ(static_cast<int>(b >> 96), 789);

  // Smaller word with less bits to bigger word with more bits.
  LL_UInt96 c(b);

  ASSERT_EQ(static_cast<int>(c), 987);
  ASSERT_EQ(static_cast<int>(c >> 32), 123);
  ASSERT_EQ(static_cast<int>(c >> 64), 456);

  // Smaller word with more bits to bigger word with less bits
  LL_UInt64 d(c);

  ASSERT_EQ(static_cast<int>(d), 987);
  ASSERT_EQ(static_cast<int>(d >> 32), 123);

  // Bigger word with less bits to smaller word with more bits

  LL_UInt96 e(d);

  ASSERT_EQ(static_cast<int>(e), 987);
  ASSERT_EQ(static_cast<int>(e >> 32), 123);

  e = (e << 32) + 654;

  ASSERT_EQ(static_cast<int>(e), 654);
  ASSERT_EQ(static_cast<int>(e >> 32), 987);
  ASSERT_EQ(static_cast<int>(e >> 64), 123);
}

TEST(LlvmLibcUIntClassTest, SignedOtherWordTypeCastTests) {
  using LL_Int64 = BigInt<64, true, uint64_t>;
  using LL_Int96 = BigInt<96, true, uint32_t>;

  LL_Int64 zero_64(0);
  LL_Int96 zero_96(0);
  LL_Int192 zero_192(0);

  LL_Int96 plus_a({0x1234, 0x5678, 0x9ABC});

  ASSERT_EQ(static_cast<int>(plus_a), 0x1234);
  ASSERT_EQ(static_cast<int>(plus_a >> 32), 0x5678);
  ASSERT_EQ(static_cast<int>(plus_a >> 64), 0x9ABC);

  LL_Int96 minus_a(-plus_a);

  // The reason that the numbers are inverted and not negated is that we're
  // using two's complement. To negate a two's complement number you flip the
  // bits and add 1, so minus_a is {~0x1234, ~0x5678, ~0x9ABC} + {1,0,0}.
  ASSERT_EQ(static_cast<int>(minus_a), (~0x1234) + 1);
  ASSERT_EQ(static_cast<int>(minus_a >> 32), ~0x5678);
  ASSERT_EQ(static_cast<int>(minus_a >> 64), ~0x9ABC);

  ASSERT_TRUE(plus_a + minus_a == zero_96);

  // 192 so there's an extra block to get sign extended to
  LL_Int192 bigger_plus_a(plus_a);

  ASSERT_EQ(static_cast<int>(bigger_plus_a), 0x1234);
  ASSERT_EQ(static_cast<int>(bigger_plus_a >> 32), 0x5678);
  ASSERT_EQ(static_cast<int>(bigger_plus_a >> 64), 0x9ABC);
  ASSERT_EQ(static_cast<int>(bigger_plus_a >> 96), 0);
  ASSERT_EQ(static_cast<int>(bigger_plus_a >> 128), 0);
  ASSERT_EQ(static_cast<int>(bigger_plus_a >> 160), 0);

  LL_Int192 bigger_minus_a(minus_a);

  ASSERT_EQ(static_cast<int>(bigger_minus_a), (~0x1234) + 1);
  ASSERT_EQ(static_cast<int>(bigger_minus_a >> 32), ~0x5678);
  ASSERT_EQ(static_cast<int>(bigger_minus_a >> 64), ~0x9ABC);
  ASSERT_EQ(static_cast<int>(bigger_minus_a >> 96), ~0);
  ASSERT_EQ(static_cast<int>(bigger_minus_a >> 128), ~0);
  ASSERT_EQ(static_cast<int>(bigger_minus_a >> 160), ~0);

  ASSERT_TRUE(bigger_plus_a + bigger_minus_a == zero_192);

  LL_Int64 smaller_plus_a(plus_a);

  ASSERT_EQ(static_cast<int>(smaller_plus_a), 0x1234);
  ASSERT_EQ(static_cast<int>(smaller_plus_a >> 32), 0x5678);

  LL_Int64 smaller_minus_a(minus_a);

  ASSERT_EQ(static_cast<int>(smaller_minus_a), (~0x1234) + 1);
  ASSERT_EQ(static_cast<int>(smaller_minus_a >> 32), ~0x5678);

  ASSERT_TRUE(smaller_plus_a + smaller_minus_a == zero_64);

  // Also try going from bigger word size to smaller word size
  LL_Int96 smaller_back_plus_a(smaller_plus_a);

  ASSERT_EQ(static_cast<int>(smaller_back_plus_a), 0x1234);
  ASSERT_EQ(static_cast<int>(smaller_back_plus_a >> 32), 0x5678);
  ASSERT_EQ(static_cast<int>(smaller_back_plus_a >> 64), 0);

  LL_Int96 smaller_back_minus_a(smaller_minus_a);

  ASSERT_EQ(static_cast<int>(smaller_back_minus_a), (~0x1234) + 1);
  ASSERT_EQ(static_cast<int>(smaller_back_minus_a >> 32), ~0x5678);
  ASSERT_EQ(static_cast<int>(smaller_back_minus_a >> 64), ~0);

  ASSERT_TRUE(smaller_back_plus_a + smaller_back_minus_a == zero_96);

  LL_Int96 bigger_back_plus_a(bigger_plus_a);

  ASSERT_EQ(static_cast<int>(bigger_back_plus_a), 0x1234);
  ASSERT_EQ(static_cast<int>(bigger_back_plus_a >> 32), 0x5678);
  ASSERT_EQ(static_cast<int>(bigger_back_plus_a >> 64), 0x9ABC);

  LL_Int96 bigger_back_minus_a(bigger_minus_a);

  ASSERT_EQ(static_cast<int>(bigger_back_minus_a), (~0x1234) + 1);
  ASSERT_EQ(static_cast<int>(bigger_back_minus_a >> 32), ~0x5678);
  ASSERT_EQ(static_cast<int>(bigger_back_minus_a >> 64), ~0x9ABC);

  ASSERT_TRUE(bigger_back_plus_a + bigger_back_minus_a == zero_96);
}

TEST(LlvmLibcUIntClassTest, MixedSignednessOtherWordTypeCastTests) {
  using LL_UInt96 = BigInt<96, false, uint32_t>;
  LL_UInt96 x = -123;
  // ensure that -123 gets extended, even though the input type is signed while
  // the BigInt is unsigned.
  ASSERT_EQ(int64_t(x), int64_t(-123));
}

} // namespace LIBC_NAMESPACE_DECL
