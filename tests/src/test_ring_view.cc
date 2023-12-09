
// Copyright 2023 Deligor <deligor6321@gmail.com>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <deque>
#include <forward_list>
#include <iterator>
#include <list>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <dlgr/ring_view.h>

namespace {

using dlgr::ranges::ring_view;
using dlgr::views::ring;

template <std::ranges::range RangeType>
auto to_vector(RangeType&& range) {
  using ValueType = std::ranges::range_value_t<RangeType>;
  auto out = std::vector<ValueType>();
  std::ranges::copy(std::forward<RangeType>(range), std::back_insert_iterator(out));
  return out;
}

enum class range_type {
  none,
  input_range,
  forward_range,
  bidirectional_range,
  random_access_range,
  contiguous_range,
};
constexpr auto operator<=>(range_type lhs, range_type rhs) {
  using value_type = std::underlying_type_t<range_type>;
  return static_cast<value_type>(lhs) <=> static_cast<value_type>(rhs);
}

template <class ValueType>
struct check_range_concepts_config {
  using value_type = ValueType;

  range_type range_type = range_type::none;
  bool is_viewable_range = false;
  bool is_output_range = false;
  bool is_common_range = false;
  bool is_sized_range = false;
};

template <check_range_concepts_config Config>
constexpr auto check_range_concepts(auto&& range) -> void {
  using RangeType = decltype(range);
  using ValueType = typename decltype(Config)::value_type;

  constexpr auto is_input_range = (Config.range_type >= range_type::input_range);
  constexpr auto is_forward_range = (Config.range_type >= range_type::forward_range);
  constexpr auto is_bidirectional_range = (Config.range_type >= range_type::bidirectional_range);
  constexpr auto is_random_access_range = (Config.range_type >= range_type::random_access_range);
  constexpr auto is_continuous_range = (Config.range_type >= range_type::contiguous_range);

  STATIC_CHECK(std::ranges::input_range<RangeType> == is_input_range);
  STATIC_CHECK(std::ranges::forward_range<RangeType> == is_forward_range);
  STATIC_CHECK(std::ranges::bidirectional_range<RangeType> == is_bidirectional_range);
  STATIC_CHECK(std::ranges::random_access_range<RangeType> == is_random_access_range);
  STATIC_CHECK(std::ranges::contiguous_range<RangeType> == is_continuous_range);

  STATIC_CHECK(std::ranges::viewable_range<RangeType> == Config.is_viewable_range);
  STATIC_CHECK(std::ranges::output_range<RangeType, ValueType> == Config.is_output_range);
  STATIC_CHECK(std::ranges::common_range<RangeType> == Config.is_common_range);
  STATIC_CHECK(std::ranges::sized_range<RangeType> == Config.is_sized_range);
}

template <class ExpectedIteratorCategory>
constexpr auto check_iterator_category(auto&& range) -> void {
  using RangeType = decltype(range);
  using IteratorTraits = std::iterator_traits<std::ranges::iterator_t<RangeType>>;
  using IteartorCategory = typename IteratorTraits::iterator_category;
  STATIC_CHECK(std::is_same_v<IteartorCategory, ExpectedIteratorCategory>);
}

}  // namespace

// NOLINTBEGIN
TEST_CASE("ring_view for vector", "[ring_view]") {  // cppcheck-suppress[naming-functionName]
  const auto vec = std::vector<int>{0, 11, 23, 24, 27};

  SECTION("unbounded") {
    auto nums = std::views::all(vec) | std::views::reverse | ring() | std::views::take(11);

    constexpr auto checks = check_range_concepts_config<int>{
        .range_type = range_type::input_range,
        .is_viewable_range = true,
    };
    check_range_concepts<checks>(nums);
    check_iterator_category<std::input_iterator_tag>(nums);

    CHECK(to_vector(nums) == std::vector<int>{27, 24, 23, 11, 0, 27, 24, 23, 11, 0, 27});
  }

  SECTION("bound = 2") {
    auto nums = ring_view(vec, 2) | std::views::reverse;

    constexpr auto checks = check_range_concepts_config<int>{
        .range_type = range_type::random_access_range,
        .is_viewable_range = true,
        .is_common_range = true,
        .is_sized_range = true,
    };
    check_range_concepts<checks>(nums);
    check_iterator_category<std::random_access_iterator_tag>(nums);

    CHECK(!nums.empty());
    CHECK(nums.size() == 10);
    CHECK((nums.end() - nums.begin()) == 10);
    CHECK(to_vector(nums) == std::vector<int>{27, 24, 23, 11, 0, 27, 24, 23, 11, 0});
  }
}

TEST_CASE("ring_view for list", "[ring_view]") {  // cppcheck-suppress[naming-functionName]
  auto lst = std::list<int>{0, 11, 23, 24, 27};

  SECTION("unbounded") {
    auto nums = std::ranges::take_view(ring_view(lst), 9);

    constexpr auto checks = check_range_concepts_config<int>{
        .range_type = range_type::input_range,
        .is_viewable_range = true,
        .is_output_range = true,
    };
    check_range_concepts<checks>(nums);
    check_iterator_category<std::input_iterator_tag>(nums);

    CHECK(to_vector(nums) == std::vector<int>{0, 11, 23, 24, 27, 0, 11, 23, 24});
  }

  SECTION("bound = 2") {
    auto nums = ring_view(lst, 2);

    constexpr auto checks = check_range_concepts_config<int>{
        .range_type = range_type::bidirectional_range,
        .is_viewable_range = true,
        .is_common_range = true,
        .is_output_range = true,
        .is_sized_range = true,
    };
    check_range_concepts<checks>(nums);
    check_iterator_category<std::bidirectional_iterator_tag>(nums);

    CHECK(!nums.empty());
    CHECK(nums.size() == 10);
    CHECK(to_vector(nums) == std::vector<int>{0, 11, 23, 24, 27, 0, 11, 23, 24, 27});
  }
}

TEST_CASE("ring_view for forward_list", "[ring_view]") {  // cppcheck-suppress[naming-functionName]
  auto flst = std::forward_list<int>{0, 11, 23, 24, 27};

  SECTION("unbounded") {
    auto nums = std::views::all(flst) | ring() | std::views::take(17) | std::views::drop(4);

    constexpr auto checks = check_range_concepts_config<int>{
        .range_type = range_type::input_range,
        .is_viewable_range = true,
        .is_output_range = true,
    };
    check_range_concepts<checks>(nums);
    check_iterator_category<std::input_iterator_tag>(nums);

    CHECK(to_vector(nums) == std::vector<int>{27, 0, 11, 23, 24, 27, 0, 11, 23, 24, 27, 0, 11});
  }

  SECTION("bound = 0") {
    auto nums = std::views::all(flst) | ring(0);

    constexpr auto checks = check_range_concepts_config<int>{
        .range_type = range_type::forward_range,
        .is_viewable_range = true,
        .is_common_range = true,
        .is_output_range = true,
    };
    check_range_concepts<checks>(nums);
    check_iterator_category<std::forward_iterator_tag>(nums);

    CHECK(nums.empty());
    CHECK(to_vector(nums) == std::vector<int>{});
  }
}

TEST_CASE("ring_view for string", "[ring_view]") {  // cppcheck-suppress[naming-functionName]
  auto str = std::string("abcx");

  SECTION("unbounded") {
    auto chars = std::views::all(str) | ring() | std::views::take(7);

    constexpr auto checks = check_range_concepts_config<char>{
        .range_type = range_type::input_range,
        .is_viewable_range = true,
        .is_output_range = true,
    };
    check_range_concepts<checks>(chars);
    check_iterator_category<std::input_iterator_tag>(chars);

    CHECK(to_vector(chars) == std::vector<char>{'a', 'b', 'c', 'x', 'a', 'b', 'c'});
  }

  SECTION("bound = 1") {
    auto chars = std::views::all(str) | ring(1);

    constexpr auto checks = check_range_concepts_config<int>{
        .range_type = range_type::random_access_range,
        .is_viewable_range = true,
        .is_common_range = true,
        .is_output_range = true,
        .is_sized_range = true,
    };
    check_range_concepts<checks>(chars);
    check_iterator_category<std::random_access_iterator_tag>(chars);

    CHECK(!chars.empty());
    CHECK(chars.size() == 4);
    CHECK((chars.end() - chars.begin()) == 4);
    CHECK(to_vector(chars) == std::vector<char>{'a', 'b', 'c', 'x'});
  }
}

TEST_CASE("ring_view for empty string_view",
          "[ring_view]") {  // cppcheck-suppress[naming-functionName]
  auto empt = std::string_view("");

  SECTION("unbounded") {
    auto chars = std::views::all(empt) | ring();

    constexpr auto checks = check_range_concepts_config<char>{
        .range_type = range_type::input_range,
        .is_viewable_range = true,
    };
    check_range_concepts<checks>(chars);
    check_iterator_category<std::input_iterator_tag>(chars);

    CHECK(to_vector(chars) == std::vector<char>{});
  }

  SECTION("bound = 100'000") {
    auto chars = std::views::all(empt) | ring(100'000);

    constexpr auto checks = check_range_concepts_config<int>{
        .range_type = range_type::random_access_range,
        .is_viewable_range = true,
        .is_common_range = true,
        .is_sized_range = true,
    };
    check_range_concepts<checks>(chars);
    check_iterator_category<std::random_access_iterator_tag>(chars);

    CHECK(chars.empty());
    CHECK(chars.size() == 0);
    CHECK((chars.end() - chars.begin()) == 0);
    CHECK(to_vector(chars) == std::vector<char>{});
  }
}

TEST_CASE("ring_view for empty_view", "[ring_view]") {  // cppcheck-suppress[naming-functionName]
  auto empt = std::views::empty<char>;

  SECTION("unbounded") {
    auto chars = empt | ring();

    constexpr auto checks = check_range_concepts_config<char>{
        .range_type = range_type::input_range,
        .is_viewable_range = true,
        .is_output_range = true,
    };
    check_range_concepts<checks>(chars);

    check_iterator_category<std::input_iterator_tag>(chars);

    CHECK(to_vector(chars) == std::vector<char>{});
  }
}

TEST_CASE("ring_view for single value", "[ring_view]") {  // cppcheck-suppress[naming-functionName]
  auto dbl = 12.0;

  SECTION("unbounded") {
    auto dbls = std::views::single(dbl) | ring()
                | std::views::transform([n = 0](auto val) mutable { return ++n + val; })
                | std::views::take(5);

    constexpr auto checks = check_range_concepts_config<double>{
        .range_type = range_type::input_range,
        .is_viewable_range = true,
    };
    check_range_concepts<checks>(dbls);
    // check_iterator_category<std::input_iterator_tag>(dbls);

    CHECK(to_vector(dbls) == std::vector<double>{13.0, 14.0, 15.0, 16.0, 17.0});
  }

  SECTION("bound = 7") {
    auto dbls = std::ranges::single_view(dbl) | ring(7) | std::views::reverse;

    constexpr auto checks = check_range_concepts_config<double>{
        .range_type = range_type::random_access_range,
        .is_viewable_range = true,
        .is_output_range = true,
        .is_common_range = true,
        .is_sized_range = true,
    };
    check_range_concepts<checks>(dbls);
    check_iterator_category<std::random_access_iterator_tag>(dbls);

    CHECK(!dbls.empty());
    CHECK(dbls.size() == 7);
    CHECK(to_vector(dbls) == std::vector<double>{12.0, 12.0, 12.0, 12.0, 12.0, 12.0, 12.0});
  }
}

TEST_CASE("ring_view for deque", "[ring_view]") {  // cppcheck-suppress[naming-functionName]
  auto deq = std::deque<int>{1, 3, 5, 7};

  SECTION("unbounded") {
    auto nums = std::ranges::ref_view(deq) | ring();

    constexpr auto checks = check_range_concepts_config<int>{
        .range_type = range_type::input_range,
        .is_viewable_range = true,
        .is_output_range = true,
    };
    check_range_concepts<checks>(nums);
    check_iterator_category<std::input_iterator_tag>(nums);

    auto it = std::ranges::begin(nums);

    it += 6;
    CHECK(*it == 5);
    CHECK(*(it + 17) == 7);

    it -= 3;
    CHECK(*it == 7);
    CHECK(*(it - 15) == 1);
  }
}

TEST_CASE("ring_view output range", "[ring_view]") {  // cppcheck-suppress[naming-functionName]
  auto vec = std::vector<std::size_t>{0, 1, 2, 3};
  auto nums = vec | ring(2);
  std::ranges::for_each(nums, [](auto& val) { ++val; });

  constexpr auto checks = check_range_concepts_config<int>{
      .range_type = range_type::random_access_range,
      .is_viewable_range = true,
      .is_output_range = true,
      .is_common_range = true,
      .is_sized_range = true,
  };
  check_range_concepts<checks>(nums);
  check_iterator_category<std::random_access_iterator_tag>(nums);

  CHECK(vec == std::vector<std::size_t>{2, 3, 4, 5});
  CHECK(to_vector(nums) == std::vector<std::size_t>{2, 3, 4, 5, 2, 3, 4, 5});
}

// TODO(tests): deduction guides, more bounded tests, other std views and algorithms,
// kv-containers, iterator/sentinel concepts, big bounds, out of range

// NOLINTEND
