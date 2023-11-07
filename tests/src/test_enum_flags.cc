
// Copyright 2023 Deligor <deligor6321@gmail.com>

#include <catch2/catch_test_macros.hpp>

#include <bitset>
#include <climits>

#include <dlgr/enum_flags.h>

namespace {

using dlgr::enum_flags;
using dlgr::enum_flags_mask_unspecified_t;
using dlgr::make_enum_flags_mask_spec;

constexpr auto to_bitset(auto flags) {
  using underlying_data_t = typename decltype(flags)::underlying_data_type;
  return std::bitset<sizeof(underlying_data_t) * CHAR_BIT>{static_cast<underlying_data_t>(flags)};
}

enum class my_flag : std::uint8_t {
  first = (1U << 0U),
  second = (1U << 1U),
  third = (1U << 2U),

  first_and_second = first | second,
};

}  // namespace

// NOLINTBEGIN
TEST_CASE("enum_flags_wo_mask") {
  using test_flags_t = enum_flags<my_flag, enum_flags_mask_unspecified_t>;

  STATIC_CHECK(std::is_trivially_copyable_v<test_flags_t>);
  STATIC_CHECK(std::is_standard_layout_v<test_flags_t>);

  STATIC_CHECK(to_bitset(test_flags_t::none()) == 0b00000000);
  STATIC_CHECK(test_flags_t::none().has_none());
  STATIC_CHECK(!test_flags_t::none().has_any());

  test_flags_t flags1;
  CHECK(to_bitset(flags1) == 0b00000000);
  CHECK(!flags1.has_any());
  CHECK(!static_cast<bool>(flags1));
  CHECK(flags1.has_none());

  test_flags_t flags2 = my_flag::third;
  CHECK(to_bitset(flags2) == 0b00000100);
  CHECK(flags2.has_any());
  CHECK(static_cast<bool>(flags2));
  CHECK(!flags2.has_none());
  CHECK(flags2 & my_flag::third);

  auto flags3 = test_flags_t() | my_flag::first | my_flag::second;
  CHECK(to_bitset(flags3) == 0b00000011);
  CHECK(flags3 & my_flag::first);
  CHECK(flags3.test(my_flag::second));
  CHECK(flags3 & my_flag::first_and_second);
  CHECK(!flags3.test(my_flag::third));

  flags3 |= flags2;
  CHECK(to_bitset(flags3) == 0b00000111);
  CHECK(flags3 & my_flag::third);

  flags3 ^= my_flag::second;
  CHECK(to_bitset(flags3) == 0b00000101);
  CHECK(flags3.test(my_flag::first));
  CHECK(!flags3.test(my_flag::second));
  CHECK(!flags3.test(my_flag::first_and_second));

  flags1 = flags3 & flags2;
  CHECK(to_bitset(flags1) == 0b00000100);

  flags1 = flags3 ^ flags2;
  CHECK(to_bitset(flags1) == 0b00000001);
  CHECK(flags1 == my_flag::first);

  flags1 = flags2 | flags1;
  CHECK(to_bitset(flags1) == 0b00000101);
  CHECK(flags1 == flags3);

  flags1 &= my_flag::third;
  CHECK(to_bitset(flags1) == 0b00000100);

  flags1.set(my_flag::second).reset(my_flag::third);
  CHECK(to_bitset(flags1) == 0b00000010);

  flags3.flip(my_flag::first_and_second);
  CHECK(to_bitset(flags3) == 0b00000111);
  flags3.flip(my_flag::first_and_second);
  CHECK(to_bitset(flags3) == 0b00000100);

  flags2.reset_all().flip(my_flag::first);
  CHECK(to_bitset(flags2) == 0b00000001);
  CHECK(flags2.test(my_flag::first));

  flags2.set(my_flag::third).set(my_flag::second).flip(my_flag::first_and_second);
  CHECK(to_bitset(flags2) == 0b00000100);
  CHECK(flags2.test(my_flag::third));
}

TEST_CASE("enum_flags_with_mask") {
  using test_flags_t =
      enum_flags<my_flag,
                 make_enum_flags_mask_spec<my_flag>::from_enums_t<
                     my_flag::first, my_flag::second, my_flag::third, my_flag::first_and_second>>;

  STATIC_CHECK(std::is_trivially_copyable_v<test_flags_t>);
  STATIC_CHECK(std::is_standard_layout_v<test_flags_t>);

  STATIC_CHECK(to_bitset(test_flags_t::all()) == 0b00000111);
  STATIC_CHECK(test_flags_t::all().has_any());
  STATIC_CHECK(!test_flags_t::all().has_none());

  STATIC_CHECK(to_bitset(test_flags_t::none()) == 0b00000000);
  STATIC_CHECK(test_flags_t::none().has_none());
  STATIC_CHECK(!test_flags_t::none().has_any());

  test_flags_t flags1;
  CHECK(to_bitset(flags1) == 0b00000000);
  CHECK(!flags1.has_any());
  CHECK(!static_cast<bool>(flags1));
  CHECK(flags1.has_none());

  test_flags_t flags2 = my_flag::third;
  CHECK(to_bitset(flags2) == 0b00000100);
  CHECK(flags2.has_any());
  CHECK(static_cast<bool>(flags2));
  CHECK(!flags2.has_none());
  CHECK(flags2 & my_flag::third);

  auto flags3 = test_flags_t() | my_flag::first | my_flag::second;
  CHECK(to_bitset(flags3) == 0b00000011);
  CHECK(flags3 & my_flag::first);
  CHECK(flags3.test(my_flag::second));
  CHECK(flags3 & my_flag::first_and_second);
  CHECK(!flags3.test(my_flag::third));

  flags3 |= flags2;
  CHECK(to_bitset(flags3) == 0b00000111);
  CHECK(flags3 & my_flag::third);

  flags3 ^= my_flag::second;
  CHECK(to_bitset(flags3) == 0b00000101);
  CHECK(flags3.test(my_flag::first));
  CHECK(!flags3.test(my_flag::second));
  CHECK(!flags3.test(my_flag::first_and_second));

  flags1 = flags3 & flags2;
  CHECK(to_bitset(flags1) == 0b00000100);

  flags1 = flags3 ^ flags2;
  CHECK(to_bitset(flags1) == 0b00000001);
  CHECK(flags1 == my_flag::first);

  flags1 = flags2 | flags1;
  CHECK(to_bitset(flags1) == 0b00000101);
  CHECK(flags1 == flags3);

  flags1 &= my_flag::third;
  CHECK(to_bitset(flags1) == 0b00000100);

  flags1.set(my_flag::second).reset(my_flag::third);
  CHECK(to_bitset(flags1) == 0b00000010);

  flags3.flip(my_flag::first_and_second);
  CHECK(to_bitset(flags3) == 0b00000111);
  flags3.flip(my_flag::first_and_second);
  CHECK(to_bitset(flags3) == 0b00000100);

  flags2.reset_all().flip(my_flag::first);
  CHECK(to_bitset(flags2) == 0b00000001);
  CHECK(flags2.test(my_flag::first));

  flags2.set_all().flip(my_flag::first_and_second);
  CHECK(to_bitset(flags2) == 0b00000100);
  CHECK(flags2.test(my_flag::third));

  flags2.flip_all();
  CHECK(to_bitset(flags2) == 0b00000011);
  CHECK(flags2.test(my_flag::first));
  CHECK(flags2.test(my_flag::second));
  CHECK(flags2.test(~test_flags_t(my_flag::third)));
}

TEST_CASE("enum_flags_with_custom_mask") {
  using test_flags_t =
      enum_flags<my_flag,
                 make_enum_flags_mask_spec<my_flag>::from_enums_t<my_flag::first, my_flag::third>>;

  STATIC_CHECK(std::is_trivially_copyable_v<test_flags_t>);
  STATIC_CHECK(std::is_standard_layout_v<test_flags_t>);

  STATIC_CHECK(to_bitset(test_flags_t::all()) == 0b00000101);
  STATIC_CHECK(test_flags_t::all().has_any());
  STATIC_CHECK(!test_flags_t::all().has_none());

  STATIC_CHECK(to_bitset(test_flags_t::none()) == 0b00000000);
  STATIC_CHECK(test_flags_t::none().has_none());
  STATIC_CHECK(!test_flags_t::none().has_any());

  test_flags_t flags1;
  CHECK(to_bitset(flags1) == 0b00000000);
  CHECK(!flags1.has_any());
  CHECK(!static_cast<bool>(flags1));
  CHECK(flags1.has_none());

  test_flags_t flags2 = my_flag::first_and_second;
  CHECK(to_bitset(flags2) == 0b00000001);
  CHECK(flags2.has_any());
  CHECK(static_cast<bool>(flags2));
  CHECK(!flags2.has_none());
  CHECK(flags2 & my_flag::first);
  CHECK(!flags2.test(my_flag::second));
  CHECK(!flags2.test(my_flag::third));
  CHECK(!flags2.test(my_flag::first_and_second));

  auto flags3 = test_flags_t() | my_flag::first | my_flag::third;
  CHECK(to_bitset(flags3) == 0b00000101);
  CHECK(flags3 & my_flag::first);
  CHECK(!flags3.test(my_flag::second));
  CHECK(flags3 & my_flag::third);
  CHECK(!flags3.test(my_flag::first_and_second));

  flags2 |= my_flag::third;
  CHECK(to_bitset(flags2) == 0b00000101);
  flags2 |= my_flag::second;
  CHECK(to_bitset(flags2) == 0b00000101);
  flags2 |= my_flag::first_and_second;
  CHECK(to_bitset(flags2) == 0b00000101);

  flags2 ^= my_flag::third;
  CHECK(to_bitset(flags2) == 0b00000001);
  flags2 ^= my_flag::second;
  CHECK(to_bitset(flags2) == 0b00000001);
  flags2 ^= my_flag::first_and_second;
  CHECK(to_bitset(flags2) == 0b00000000);

  flags3 &= my_flag::first_and_second;
  CHECK(to_bitset(flags3) == 0b00000001);
  flags3 &= my_flag::second;
  CHECK(to_bitset(flags3) == 0b00000000);

  flags1 = test_flags_t::all() & test_flags_t(my_flag::first_and_second);
  CHECK(to_bitset(flags1) == 0b00000001);

  flags1 = flags1 | my_flag::third | my_flag::second;
  CHECK(to_bitset(flags1) == 0b00000101);

  flags1 = flags1 ^ my_flag::first_and_second;
  CHECK(to_bitset(flags1) == 0b00000100);

  flags1.set(my_flag::second).reset(my_flag::third);
  CHECK(to_bitset(flags1) == 0b00000000);

  flags3.flip(my_flag::first_and_second);
  CHECK(to_bitset(flags3) == 0b00000001);
  flags3.flip(my_flag::first_and_second);
  CHECK(to_bitset(flags3) == 0b00000000);

  flags2.reset_all().flip(my_flag::first_and_second);
  CHECK(to_bitset(flags2) == 0b00000001);
  flags2.set_all().flip(my_flag::first_and_second);
  CHECK(to_bitset(flags2) == 0b00000100);
  flags2.flip_all();
  CHECK(to_bitset(flags2) == 0b00000001);
}

TEST_CASE("enum_flags_deductions") {
  STATIC_CHECK(std::is_same_v<decltype(enum_flags(my_flag::first)),
                              enum_flags<my_flag, enum_flags_mask_unspecified_t>>);

  STATIC_CHECK(std::is_same_v<decltype(enum_flags<my_flag>()),
                              enum_flags<my_flag, enum_flags_mask_unspecified_t>>);
}

TEST_CASE("enum_flags_underlying_type") {
  // TBD
}
// NOLINTEND
