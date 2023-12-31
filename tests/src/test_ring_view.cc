// Copyright 2023 Deligor <deligor6321@gmail.com>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>

#include <algorithm>
#include <cstddef>
#include <deque>
#include <forward_list>
#include <iterator>
#include <list>
#include <ostream>
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

  range_type type = range_type::none;
  bool is_viewable_range = false;
  bool is_output_range = false;
  bool is_common_range = false;
  bool is_sized_range = false;
};

template <check_range_concepts_config Config>
constexpr auto static_check_range_concepts(auto&& range) -> void {
  using RangeType = decltype(range);
  using ValueType = typename decltype(Config)::value_type;

  constexpr auto is_input_range = (Config.type >= range_type::input_range);
  constexpr auto is_forward_range = (Config.type >= range_type::forward_range);
  constexpr auto is_bidirectional_range = (Config.type >= range_type::bidirectional_range);
  constexpr auto is_random_access_range = (Config.type >= range_type::random_access_range);
  constexpr auto is_continuous_range = (Config.type >= range_type::contiguous_range);

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
constexpr auto static_check_iterator_category(auto&& range) -> void {
  using RangeType = decltype(range);
  using IteratorTraits = std::iterator_traits<std::ranges::iterator_t<RangeType>>;
  using IteartorCategory = typename IteratorTraits::iterator_category;
  STATIC_CHECK(std::is_same_v<IteartorCategory, ExpectedIteratorCategory>);
}

template <class RangeType>
concept empty_testable_range =
    std::ranges::range<RangeType> && requires(const RangeType& range) { range.empty(); };

template <class RangeType>
constexpr auto check_empty(RangeType&& range, bool expected_empty) -> void
  requires empty_testable_range<RangeType>
{
  CHECK(range.empty() == expected_empty);
}

template <class RangeType>
constexpr auto check_size(RangeType&& range, std::size_t expected_size) -> void
  requires std::ranges::sized_range<RangeType>
{
  CHECK(range.size() == expected_size);

  if constexpr (empty_testable_range<RangeType>) {
    const auto expected_empty = (expected_size == 0);
    check_empty(std::forward<RangeType>(range), expected_empty);
  }

  if constexpr (std::ranges::random_access_range<RangeType>) {
    const auto size = static_cast<std::size_t>(range.end() - range.begin());
    CHECK(size == expected_size);
  }
}

template <class IterType>
concept iter_with_shift_op = requires(IterType iter, std::iter_difference_t<IterType> diff) {
  { iter + diff } -> std::same_as<IterType>;
};
template <class IterType>
concept iter_with_diff_op = requires(IterType lhs, IterType rhs) {
  { lhs - rhs } -> std::same_as<std::iter_difference_t<IterType>>;
};

template <class IterType>
constexpr auto check_iter_value(IterType iter, std::iter_value_t<IterType> expected_value) -> void {
  CHECK((*iter) == expected_value);
  CHECK(*(iter.operator->()) == expected_value);
  CHECK(iter[0] == expected_value);
}

template <class IterType>
constexpr auto check_iter_value_with_shift(IterType iter, std::iter_difference_t<IterType> shift,
                                           std::iter_value_t<IterType> expected_value) -> void
  requires iter_with_shift_op<IterType>
{
  constexpr auto has_diff_op = iter_with_diff_op<IterType>;

  const auto origin_value = *iter;
  CHECK(iter[shift] == expected_value);

  auto iter_shifted = iter + shift;
  check_iter_value(iter_shifted, expected_value);
  if constexpr (has_diff_op) {
    CHECK(iter_shifted - iter == shift);
    CHECK(iter - iter_shifted == -shift);
  }

  iter += shift;
  check_iter_value(iter, expected_value);
  CHECK(iter[-shift] == origin_value);
  if constexpr (has_diff_op) {
    CHECK(iter_shifted - iter == shift);
    CHECK(iter - iter_shifted == -shift);
    CHECK(iter == iter_shifted);
  }
}

constexpr auto check_random_access_ops(auto&& range, auto&& init_range) -> void {
  using ::Catch::Generators::random;
  using ::Catch::Generators::take;

  SECTION("random access ops") {
    using difference_type = std::ranges::range_difference_t<decltype(init_range)>;
    using size_type = std::ranges::range_size_t<decltype(init_range)>;

    auto rng_begin = std::ranges::begin(range);

    check_iter_value(rng_begin, init_range[0]);

    auto init_size = std::ranges::size(init_range);

    constexpr size_type lap_max = 100;

    SECTION("shift inside initial range") {
      const auto index = GENERATE_REF(take(1, random<size_type>(0, init_size - 1)));
      const auto shift = static_cast<difference_type>(index);
      check_iter_value_with_shift(rng_begin, shift, init_range[index]);
    }

    SECTION("shift far forward") {
      const auto lap = GENERATE_REF(take(1, random<size_type>(1, lap_max)));
      const auto index = GENERATE_REF(take(1, random<size_type>(0, init_size - 1)));
      const auto shift = static_cast<difference_type>(lap * init_size + index);
      check_iter_value_with_shift(rng_begin, shift, init_range[index]);
    }

    SECTION("shift backward") {
      const auto scalar = GENERATE_REF(take(1, random<size_type>(0, lap_max)));
      const auto index = GENERATE_REF(take(1, random<size_type>(1, init_size)));
      const auto shift = static_cast<difference_type>(scalar * init_size + index);
      check_iter_value_with_shift(rng_begin, -shift, init_range[init_size - index]);
    }
  }
}

struct test_struct {
  int val = {};
};

constexpr auto operator==(const test_struct& lhs, const test_struct& rhs) -> bool {
  return lhs.val == rhs.val;
}

auto operator<<(std::ostream& stream, const test_struct& value) -> std::ostream& {
  stream << "{.val = " << value.val << '}';
  return stream;
}

}  // namespace

// NOLINTBEGIN
TEST_CASE("ring_view for vector", "[ring_view]") {  // cppcheck-suppress[naming-functionName]
  const auto init = std::vector{0, 11, 23, 24, 27};
  using value_type = decltype(init)::value_type;

  SECTION("all -> reverse -> ring(unbounded)") {
    auto rng = std::views::all(init) | std::views::reverse | ring();

    constexpr auto checks = check_range_concepts_config<value_type>{
        .type = range_type::input_range,
        .is_viewable_range = true,
    };
    static_check_range_concepts<checks>(rng);
    static_check_iterator_category<std::input_iterator_tag>(rng);

    check_empty(rng, false);

    SECTION("-> take") {
      auto rng2 = rng | std::views::take(11);

      static_check_range_concepts<checks>(rng2);
      static_check_iterator_category<std::input_iterator_tag>(rng2);

      CHECK(to_vector(rng2) == std::vector<value_type>{27, 24, 23, 11, 0, 27, 24, 23, 11, 0, 27});
    }
  }

  SECTION("all -> ring(bound = 2)") {
    auto rng = ring_view(init, 2);

    constexpr auto checks = check_range_concepts_config<value_type>{
        .type = range_type::random_access_range,
        .is_viewable_range = true,
        .is_common_range = true,
        .is_sized_range = true,
    };
    static_check_range_concepts<checks>(rng);
    static_check_iterator_category<std::random_access_iterator_tag>(rng);

    check_size(rng, 10);
    CHECK(to_vector(rng) == std::vector<value_type>{0, 11, 23, 24, 27, 0, 11, 23, 24, 27});

    SECTION(" -> reverse") {
      auto rng2 = rng | std::views::reverse;

      static_check_range_concepts<checks>(rng2);
      static_check_iterator_category<std::random_access_iterator_tag>(rng2);

      check_size(rng2, 10);
      CHECK(to_vector(rng2) == std::vector<value_type>{27, 24, 23, 11, 0, 27, 24, 23, 11, 0});
    }
  }
}

TEST_CASE("ring_view for list", "[ring_view]") {  // cppcheck-suppress[naming-functionName]
  auto init = std::list{0, 11, 23, 24, 27};
  using value_type = decltype(init)::value_type;

  SECTION("all -> ring(unbounded)") {
    auto rng = ring_view(init);

    constexpr auto checks = check_range_concepts_config<value_type>{
        .type = range_type::input_range,
        .is_viewable_range = true,
        .is_output_range = true,
    };
    static_check_range_concepts<checks>(rng);
    static_check_iterator_category<std::input_iterator_tag>(rng);

    check_empty(rng, false);

    SECTION("-> take") {
      auto rng2 = rng | std::views::take(9);

      static_check_range_concepts<checks>(rng2);
      static_check_iterator_category<std::input_iterator_tag>(rng2);

      CHECK(to_vector(rng2) == std::vector<value_type>{0, 11, 23, 24, 27, 0, 11, 23, 24});
    }
  }

  SECTION("all -> ring(bound = 2)") {
    auto rng = ring_view(init, 2);

    constexpr auto checks = check_range_concepts_config<value_type>{
        .type = range_type::bidirectional_range,
        .is_viewable_range = true,
        .is_output_range = true,
        .is_common_range = true,
        .is_sized_range = true,
    };
    static_check_range_concepts<checks>(rng);
    static_check_iterator_category<std::bidirectional_iterator_tag>(rng);

    check_size(rng, 10);
    CHECK(to_vector(rng) == std::vector<value_type>{0, 11, 23, 24, 27, 0, 11, 23, 24, 27});
  }
}

TEST_CASE("ring_view for forward_list", "[ring_view]") {  // cppcheck-suppress[naming-functionName]
  auto init = std::forward_list{0, 11, 23, 24, 27};
  using value_type = decltype(init)::value_type;

  SECTION("all -> ring(unbounded)") {
    auto rng = std::views::all(init) | ring();

    constexpr auto checks = check_range_concepts_config<value_type>{
        .type = range_type::input_range,
        .is_viewable_range = true,
        .is_output_range = true,
    };
    static_check_range_concepts<checks>(rng);
    static_check_iterator_category<std::input_iterator_tag>(rng);

    check_empty(rng, false);

    SECTION("-> take -> drop") {
      auto rng2 = rng | std::views::take(17) | std::views::drop(4);

      static_check_range_concepts<checks>(rng2);
      static_check_iterator_category<std::input_iterator_tag>(rng2);

      CHECK(to_vector(rng2)
            == std::vector<value_type>{27, 0, 11, 23, 24, 27, 0, 11, 23, 24, 27, 0, 11});
    }
  }

  SECTION("all -> ring(bound = 0)") {
    auto rng = std::views::all(init) | ring(0);

    constexpr auto checks = check_range_concepts_config<value_type>{
        .type = range_type::forward_range,
        .is_viewable_range = true,
        .is_output_range = true,
        .is_common_range = true,
    };
    static_check_range_concepts<checks>(rng);
    static_check_iterator_category<std::forward_iterator_tag>(rng);

    check_empty(rng, true);
    CHECK(to_vector(rng) == std::vector<value_type>{});
  }
}

TEST_CASE("ring_view for string", "[ring_view]") {  // cppcheck-suppress[naming-functionName]
  auto str = std::string("abcx");
  using value_type = decltype(str)::value_type;

  SECTION("all -> drop -> ring(unbounded)") {
    auto rng = std::views::all(str) | std::views::drop(2) | ring();

    constexpr auto checks = check_range_concepts_config<value_type>{
        .type = range_type::input_range,
        .is_viewable_range = true,
        .is_output_range = true,
    };
    static_check_range_concepts<checks>(rng);
    static_check_iterator_category<std::input_iterator_tag>(rng);

    check_empty(rng, false);

    SECTION("-> take") {
      auto rng2 = rng | std::views::take(7);

      static_check_range_concepts<checks>(rng2);
      static_check_iterator_category<std::input_iterator_tag>(rng2);

      CHECK(to_vector(rng2) == std::vector<value_type>{'c', 'x', 'c', 'x', 'c', 'x', 'c'});
    }
  }

  SECTION("all -> take -> ring(bound = 1)") {
    auto rng = std::views::all(str) | std::views::take(3) | ring(1);

    constexpr auto checks = check_range_concepts_config<value_type>{
        .type = range_type::random_access_range,
        .is_viewable_range = true,
        .is_output_range = true,
        .is_common_range = true,
        .is_sized_range = true,
    };
    static_check_range_concepts<checks>(rng);
    static_check_iterator_category<std::random_access_iterator_tag>(rng);

    check_size(rng, 3);
    CHECK(to_vector(rng) == std::vector<value_type>{'a', 'b', 'c'});
  }
}

TEST_CASE("ring_view for empty string_view",
          "[ring_view]") {  // cppcheck-suppress[naming-functionName]
  auto init = std::string_view("");
  using value_type = decltype(init)::value_type;

  SECTION("all -> ring(unbounded)") {
    auto rng = std::views::all(init) | ring();

    constexpr auto checks = check_range_concepts_config<value_type>{
        .type = range_type::input_range,
        .is_viewable_range = true,
    };
    static_check_range_concepts<checks>(rng);
    static_check_iterator_category<std::input_iterator_tag>(rng);

    check_empty(rng, true);
    CHECK(to_vector(rng) == std::vector<value_type>{});
  }

  SECTION("all -> ring(100'000)") {
    auto rng = std::views::all(init) | ring(100'000);

    constexpr auto checks = check_range_concepts_config<value_type>{
        .type = range_type::random_access_range,
        .is_viewable_range = true,
        .is_common_range = true,
        .is_sized_range = true,
    };
    static_check_range_concepts<checks>(rng);
    static_check_iterator_category<std::random_access_iterator_tag>(rng);

    check_size(rng, 0);
    CHECK(to_vector(rng) == std::vector<value_type>{});

    SECTION("-> drop(1'000)") {
      auto rng2 = rng | std::views::drop(1'000);

      static_check_range_concepts<checks>(rng2);
      static_check_iterator_category<std::random_access_iterator_tag>(rng2);

      check_size(rng2, 0);
      CHECK(to_vector(rng2) == std::vector<value_type>{});
    }
  }
}

TEST_CASE("ring_view for empty_view", "[ring_view]") {  // cppcheck-suppress[naming-functionName]
  using value_type = bool;
  auto init = std::views::empty<value_type>;

  SECTION("empty -> ring(unbounded)") {
    auto rng = init | ring();

    constexpr auto checks = check_range_concepts_config<value_type>{
        .type = range_type::input_range,
        .is_viewable_range = true,
        .is_output_range = true,
    };
    static_check_range_concepts<checks>(rng);
    static_check_iterator_category<std::input_iterator_tag>(rng);

    check_empty(rng, true);
    CHECK(to_vector(rng) == std::vector<value_type>{});
  }

  SECTION("empty -> ring(bound = 3)") {
    auto rng = init | ring(3);

    constexpr auto checks = check_range_concepts_config<value_type>{
        .type = range_type::random_access_range,
        .is_viewable_range = true,
        .is_output_range = true,
        .is_common_range = true,
        .is_sized_range = true,
    };
    static_check_range_concepts<checks>(rng);
    static_check_iterator_category<std::random_access_iterator_tag>(rng);

    check_size(rng, 0);
    CHECK(to_vector(rng) == std::vector<value_type>{});

    SECTION("-> take") {
      auto rng2 = rng | std::views::take(5);

      static_check_range_concepts<checks>(rng2);
      static_check_iterator_category<std::random_access_iterator_tag>(rng2);

      check_size(rng2, 0);
      CHECK(to_vector(rng2) == std::vector<value_type>{});
    }
  }
}

TEST_CASE("ring_view for single value", "[ring_view]") {  // cppcheck-suppress[naming-functionName]
  auto init = 12.0;
  using value_type = decltype(init);

  SECTION("single -> ring(unbounded)") {
    auto rng = std::views::single(init) | ring();

    constexpr auto checks = check_range_concepts_config<value_type>{
        .type = range_type::input_range,
        .is_viewable_range = true,
        .is_output_range = true,
    };
    static_check_range_concepts<checks>(rng);
    static_check_iterator_category<std::input_iterator_tag>(rng);

    check_empty(rng, false);

    SECTION("-> transform -> take") {
      auto rng2 = rng | std::views::transform([n = 0](auto val) mutable { return ++n + val; })
                  | std::views::take(5);

      constexpr auto checks2 = check_range_concepts_config<value_type>{
          .type = range_type::input_range,
          .is_viewable_range = true,
      };
      static_check_range_concepts<checks2>(rng2);
      // static_check_iterator_category<std::input_iterator_tag>(rng2);

      CHECK(to_vector(rng2) == std::vector<value_type>{13.0, 14.0, 15.0, 16.0, 17.0});
    }
  }

  SECTION("single -> ring(bound = 7)") {
    auto rng = std::ranges::single_view(init) | ring(7);

    constexpr auto checks = check_range_concepts_config<value_type>{
        .type = range_type::random_access_range,
        .is_viewable_range = true,
        .is_output_range = true,
        .is_common_range = true,
        .is_sized_range = true,
    };
    static_check_range_concepts<checks>(rng);
    static_check_iterator_category<std::random_access_iterator_tag>(rng);

    check_size(rng, 7);
    CHECK(to_vector(rng) == std::vector<value_type>{12.0, 12.0, 12.0, 12.0, 12.0, 12.0, 12.0});

    SECTION("-> transform") {
      auto rng2 = rng | std::views::transform([n = 0](auto val) mutable { return ++n + val; });

      constexpr auto checks2 = check_range_concepts_config<value_type>{
          .type = range_type::random_access_range,
          .is_viewable_range = true,
          .is_common_range = true,
          .is_sized_range = true,
      };
      static_check_range_concepts<checks2>(rng2);
      // static_check_iterator_category<std::random_access_iterator_tag>(rng2);

      check_size(rng2, 7);
      CHECK(to_vector(rng2) == std::vector<value_type>{13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0});
    }
  }
}

TEST_CASE("ring_view for deque", "[ring_view]") {  // cppcheck-suppress[naming-functionName]
  auto init = std::deque<test_struct>{{1}, {3}, {5}, {7}};
  using value_type = decltype(init)::value_type;

  SECTION("ref -> ring(unbounded)") {
    auto rng = std::ranges::ref_view(init) | ring();

    constexpr auto checks = check_range_concepts_config<value_type>{
        .type = range_type::input_range,
        .is_viewable_range = true,
        .is_output_range = true,
    };
    static_check_range_concepts<checks>(rng);
    static_check_iterator_category<std::input_iterator_tag>(rng);

    check_empty(rng, false);
    check_random_access_ops(rng, init);

    SECTION("-> take") {
      auto rng2 = rng | std::views::take(5);

      static_check_range_concepts<checks>(rng2);
      static_check_iterator_category<std::input_iterator_tag>(rng2);

      CHECK(to_vector(rng2) == std::vector<value_type>{{1}, {3}, {5}, {7}, {1}});
    }
  }

  SECTION("all -> drop -> ring(bound = 3)") {
    auto rng = init | std::views::drop(1) | ring(3);

    constexpr auto checks = check_range_concepts_config<value_type>{
        .type = range_type::random_access_range,
        .is_viewable_range = true,
        .is_output_range = true,
        .is_common_range = true,
        .is_sized_range = true,
    };
    static_check_range_concepts<checks>(rng);
    static_check_iterator_category<std::random_access_iterator_tag>(rng);

    check_size(rng, 9);
    CHECK(to_vector(rng) == std::vector<value_type>{{3}, {5}, {7}, {3}, {5}, {7}, {3}, {5}, {7}});

    SECTION("-> take -> drop") {
      auto rng2 = rng | std::views::take(8) | std::views::drop(1);

      static_check_range_concepts<checks>(rng2);
      static_check_iterator_category<std::random_access_iterator_tag>(rng2);

      check_size(rng2, 7);
      CHECK(to_vector(rng2) == std::vector<value_type>{{5}, {7}, {3}, {5}, {7}, {3}, {5}});
    }
  }
}

TEST_CASE("ring_view output range", "[ring_view]") {  // cppcheck-suppress[naming-functionName]
  auto init = std::vector<std::size_t>{0, 1, 2, 3};
  using value_type = decltype(init)::value_type;

  auto rng = init | ring(2);
  std::ranges::for_each(rng, [](auto& val) { ++val; });

  constexpr auto checks = check_range_concepts_config<value_type>{
      .type = range_type::random_access_range,
      .is_viewable_range = true,
      .is_output_range = true,
      .is_common_range = true,
      .is_sized_range = true,
  };
  static_check_range_concepts<checks>(rng);
  static_check_iterator_category<std::random_access_iterator_tag>(rng);

  CHECK(init == std::vector<value_type>{2, 3, 4, 5});
  CHECK(to_vector(rng) == std::vector<value_type>{2, 3, 4, 5, 2, 3, 4, 5});
}

// TODO(tests): deduction guides, more bounded tests, other std views and algorithms,
// kv-containers, iterator/sentinel concepts, big bounds, out of range, random access ops,
// constexpr, noexcept, const iter

// NOLINTEND
