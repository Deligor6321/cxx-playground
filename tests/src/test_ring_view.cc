
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
constexpr auto static_check_range_concepts(auto&& range) -> void {
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
constexpr auto static_check_iterator_category(auto&& range) -> void {
  using RangeType = decltype(range);
  using IteratorTraits = std::iterator_traits<std::ranges::iterator_t<RangeType>>;
  using IteartorCategory = typename IteratorTraits::iterator_category;
  STATIC_CHECK(std::is_same_v<IteartorCategory, ExpectedIteratorCategory>);
}

}  // namespace

// NOLINTBEGIN
TEST_CASE("ring_view for vector", "[ring_view]") {  // cppcheck-suppress[naming-functionName]
  const auto vec = std::vector<int>{0, 11, 23, 24, 27};

  SECTION("all -> reverse -> ring(unbounded)") {
    auto nums = std::views::all(vec) | std::views::reverse | ring();

    constexpr auto checks = check_range_concepts_config<int>{
        .range_type = range_type::input_range,
        .is_viewable_range = true,
    };
    static_check_range_concepts<checks>(nums);
    static_check_iterator_category<std::input_iterator_tag>(nums);

    CHECK(!nums.empty());

    SECTION("-> take") {
      auto nums2 = nums | std::views::take(11);

      static_check_range_concepts<checks>(nums2);
      static_check_iterator_category<std::input_iterator_tag>(nums2);

      CHECK(to_vector(nums2) == std::vector<int>{27, 24, 23, 11, 0, 27, 24, 23, 11, 0, 27});
    }
  }

  SECTION("all -> ring(bound = 2)") {
    auto nums = ring_view(vec, 2);

    constexpr auto checks = check_range_concepts_config<int>{
        .range_type = range_type::random_access_range,
        .is_viewable_range = true,
        .is_common_range = true,
        .is_sized_range = true,
    };
    static_check_range_concepts<checks>(nums);
    static_check_iterator_category<std::random_access_iterator_tag>(nums);

    CHECK(!nums.empty());
    CHECK(nums.size() == 10);
    CHECK((nums.end() - nums.begin()) == 10);
    CHECK(to_vector(nums) == std::vector<int>{0, 11, 23, 24, 27, 0, 11, 23, 24, 27});

    SECTION(" -> reverse") {
      auto nums2 = nums | std::views::reverse;

      static_check_range_concepts<checks>(nums2);
      static_check_iterator_category<std::random_access_iterator_tag>(nums2);

      CHECK(!nums2.empty());
      CHECK(nums2.size() == 10);
      CHECK((nums2.end() - nums2.begin()) == 10);
      CHECK(to_vector(nums2) == std::vector<int>{27, 24, 23, 11, 0, 27, 24, 23, 11, 0});
    }
  }
}

TEST_CASE("ring_view for list", "[ring_view]") {  // cppcheck-suppress[naming-functionName]
  auto lst = std::list<int>{0, 11, 23, 24, 27};

  SECTION("all -> ring(unbounded)") {
    auto nums = ring_view(lst);

    constexpr auto checks = check_range_concepts_config<int>{
        .range_type = range_type::input_range,
        .is_viewable_range = true,
        .is_output_range = true,
    };
    static_check_range_concepts<checks>(nums);
    static_check_iterator_category<std::input_iterator_tag>(nums);

    CHECK(!nums.empty());

    SECTION("-> take") {
      auto nums2 = nums | std::views::take(9);

      static_check_range_concepts<checks>(nums2);
      static_check_iterator_category<std::input_iterator_tag>(nums2);

      CHECK(to_vector(nums2) == std::vector<int>{0, 11, 23, 24, 27, 0, 11, 23, 24});
    }
  }

  SECTION("all -> ring(bound = 2)") {
    auto nums = ring_view(lst, 2);

    constexpr auto checks = check_range_concepts_config<int>{
        .range_type = range_type::bidirectional_range,
        .is_viewable_range = true,
        .is_common_range = true,
        .is_output_range = true,
        .is_sized_range = true,
    };
    static_check_range_concepts<checks>(nums);
    static_check_iterator_category<std::bidirectional_iterator_tag>(nums);

    CHECK(!nums.empty());
    CHECK(nums.size() == 10);
    CHECK(to_vector(nums) == std::vector<int>{0, 11, 23, 24, 27, 0, 11, 23, 24, 27});
  }
}

TEST_CASE("ring_view for forward_list", "[ring_view]") {  // cppcheck-suppress[naming-functionName]
  auto flst = std::forward_list<int>{0, 11, 23, 24, 27};

  SECTION("all -> ring(unbounded)") {
    auto nums = std::views::all(flst) | ring();

    constexpr auto checks = check_range_concepts_config<int>{
        .range_type = range_type::input_range,
        .is_viewable_range = true,
        .is_output_range = true,
    };
    static_check_range_concepts<checks>(nums);
    static_check_iterator_category<std::input_iterator_tag>(nums);

    CHECK(!nums.empty());

    SECTION("-> take -> drop") {
      auto nums2 = nums | std::views::take(17) | std::views::drop(4);

      static_check_range_concepts<checks>(nums2);
      static_check_iterator_category<std::input_iterator_tag>(nums2);

      CHECK(to_vector(nums2) == std::vector<int>{27, 0, 11, 23, 24, 27, 0, 11, 23, 24, 27, 0, 11});
    }
  }

  SECTION("all -> ring(bound = 0)") {
    auto nums = std::views::all(flst) | ring(0);

    constexpr auto checks = check_range_concepts_config<int>{
        .range_type = range_type::forward_range,
        .is_viewable_range = true,
        .is_common_range = true,
        .is_output_range = true,
    };
    static_check_range_concepts<checks>(nums);
    static_check_iterator_category<std::forward_iterator_tag>(nums);

    CHECK(nums.empty());
    CHECK(to_vector(nums) == std::vector<int>{});
  }
}

TEST_CASE("ring_view for string", "[ring_view]") {  // cppcheck-suppress[naming-functionName]
  auto str = std::string("abcx");

  SECTION("all -> drop -> ring(unbounded)") {
    auto chars = std::views::all(str) | std::views::drop(2) | ring();

    constexpr auto checks = check_range_concepts_config<char>{
        .range_type = range_type::input_range,
        .is_viewable_range = true,
        .is_output_range = true,
    };
    static_check_range_concepts<checks>(chars);
    static_check_iterator_category<std::input_iterator_tag>(chars);

    CHECK(!chars.empty());

    SECTION("-> take") {
      auto chars2 = chars | std::views::take(7);

      static_check_range_concepts<checks>(chars2);
      static_check_iterator_category<std::input_iterator_tag>(chars2);

      CHECK(to_vector(chars2) == std::vector<char>{'c', 'x', 'c', 'x', 'c', 'x', 'c'});
    }
  }

  SECTION("all -> take -> ring(bound = 1)") {
    auto chars = std::views::all(str) | std::views::take(3) | ring(1);

    constexpr auto checks = check_range_concepts_config<int>{
        .range_type = range_type::random_access_range,
        .is_viewable_range = true,
        .is_common_range = true,
        .is_output_range = true,
        .is_sized_range = true,
    };
    static_check_range_concepts<checks>(chars);
    static_check_iterator_category<std::random_access_iterator_tag>(chars);

    CHECK(!chars.empty());
    CHECK(chars.size() == 3);
    CHECK((chars.end() - chars.begin()) == 3);
    CHECK(to_vector(chars) == std::vector<char>{'a', 'b', 'c'});
  }
}

TEST_CASE("ring_view for empty string_view",
          "[ring_view]") {  // cppcheck-suppress[naming-functionName]
  auto empt = std::string_view("");

  SECTION("all -> ring(unbounded)") {
    auto chars = std::views::all(empt) | ring();

    constexpr auto checks = check_range_concepts_config<char>{
        .range_type = range_type::input_range,
        .is_viewable_range = true,
    };
    static_check_range_concepts<checks>(chars);
    static_check_iterator_category<std::input_iterator_tag>(chars);

    CHECK(chars.empty());
    CHECK(to_vector(chars) == std::vector<char>{});
  }

  SECTION("all -> ring(100'000)") {
    auto chars = std::views::all(empt) | ring(100'000);

    constexpr auto checks = check_range_concepts_config<int>{
        .range_type = range_type::random_access_range,
        .is_viewable_range = true,
        .is_common_range = true,
        .is_sized_range = true,
    };
    static_check_range_concepts<checks>(chars);
    static_check_iterator_category<std::random_access_iterator_tag>(chars);

    CHECK(chars.empty());
    CHECK(chars.size() == 0);
    CHECK((chars.end() - chars.begin()) == 0);
    CHECK(to_vector(chars) == std::vector<char>{});

    SECTION("-> drop(1'000)") {
      auto chars2 = chars | std::views::drop(1'000);

      static_check_range_concepts<checks>(chars2);
      static_check_iterator_category<std::random_access_iterator_tag>(chars2);

      CHECK(chars2.empty());
      CHECK(chars2.size() == 0);
      CHECK((chars2.end() - chars2.begin()) == 0);
      CHECK(to_vector(chars2) == std::vector<char>{});
    }
  }
}

TEST_CASE("ring_view for empty_view", "[ring_view]") {  // cppcheck-suppress[naming-functionName]
  auto empt = std::views::empty<char>;

  SECTION("empty -> ring(unbounded)") {
    auto chars = empt | ring();

    constexpr auto checks = check_range_concepts_config<char>{
        .range_type = range_type::input_range,
        .is_viewable_range = true,
        .is_output_range = true,
    };
    static_check_range_concepts<checks>(chars);
    static_check_iterator_category<std::input_iterator_tag>(chars);

    CHECK(chars.empty());
    CHECK(to_vector(chars) == std::vector<char>{});
  }

  SECTION("empty -> ring(bound = 3)") {
    auto chars = empt | ring(3);

    constexpr auto checks = check_range_concepts_config<int>{
        .range_type = range_type::random_access_range,
        .is_viewable_range = true,
        .is_common_range = true,
        .is_output_range = true,
        .is_sized_range = true,
    };
    static_check_range_concepts<checks>(chars);
    static_check_iterator_category<std::random_access_iterator_tag>(chars);

    CHECK(chars.empty());
    CHECK(chars.size() == 0);
    CHECK((chars.end() - chars.begin()) == 0);
    CHECK(to_vector(chars) == std::vector<char>{});

    SECTION("-> take") {
      auto chars2 = chars | std::views::take(5);

      static_check_range_concepts<checks>(chars2);
      static_check_iterator_category<std::random_access_iterator_tag>(chars2);

      CHECK(chars2.empty());
      CHECK(chars2.size() == 0);
      CHECK((chars2.end() - chars2.begin()) == 0);
      CHECK(to_vector(chars2) == std::vector<char>{});
    }
  }
}

TEST_CASE("ring_view for single value", "[ring_view]") {  // cppcheck-suppress[naming-functionName]
  auto dbl = 12.0;

  SECTION("single -> ring(unbounded)") {
    auto dbls = std::views::single(dbl) | ring();

    constexpr auto checks = check_range_concepts_config<double>{
        .range_type = range_type::input_range,
        .is_viewable_range = true,
        .is_output_range = true,
    };
    static_check_range_concepts<checks>(dbls);
    static_check_iterator_category<std::input_iterator_tag>(dbls);

    CHECK(!dbls.empty());

    SECTION("-> transform -> take") {
      auto dbls2 = dbls | std::views::transform([n = 0](auto val) mutable { return ++n + val; })
                   | std::views::take(5);

      constexpr auto checks2 = check_range_concepts_config<double>{
          .range_type = range_type::input_range,
          .is_viewable_range = true,
      };
      static_check_range_concepts<checks2>(dbls2);
      // static_check_iterator_category<std::input_iterator_tag>(dbls2);

      CHECK(to_vector(dbls2) == std::vector<double>{13.0, 14.0, 15.0, 16.0, 17.0});
    }
  }

  SECTION("single -> ring(bound = 7)") {
    auto dbls = std::ranges::single_view(dbl) | ring(7);

    constexpr auto checks = check_range_concepts_config<double>{
        .range_type = range_type::random_access_range,
        .is_viewable_range = true,
        .is_output_range = true,
        .is_common_range = true,
        .is_sized_range = true,
    };
    static_check_range_concepts<checks>(dbls);
    static_check_iterator_category<std::random_access_iterator_tag>(dbls);

    CHECK(!dbls.empty());
    CHECK(dbls.size() == 7);
    CHECK((dbls.end() - dbls.begin()) == 7);
    CHECK(to_vector(dbls) == std::vector<double>{12.0, 12.0, 12.0, 12.0, 12.0, 12.0, 12.0});

    SECTION("-> transform") {
      auto dbls2 = dbls | std::views::transform([n = 0](auto val) mutable { return ++n + val; });

      constexpr auto checks2 = check_range_concepts_config<double>{
          .range_type = range_type::random_access_range,
          .is_viewable_range = true,
          .is_common_range = true,
          .is_sized_range = true,
      };
      static_check_range_concepts<checks2>(dbls2);
      // static_check_iterator_category<std::random_access_iterator_tag>(dbls2);

      CHECK(!dbls2.empty());
      CHECK(dbls2.size() == 7);
      CHECK((dbls2.end() - dbls2.begin()) == 7);
      CHECK(to_vector(dbls2) == std::vector<double>{13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0});
    }
  }
}

TEST_CASE("ring_view for deque", "[ring_view]") {  // cppcheck-suppress[naming-functionName]
  auto deq = std::deque<int>{1, 3, 5, 7};

  SECTION("ref -> ring(unbounded)") {
    auto nums = std::ranges::ref_view(deq) | ring();

    constexpr auto checks = check_range_concepts_config<int>{
        .range_type = range_type::input_range,
        .is_viewable_range = true,
        .is_output_range = true,
    };
    static_check_range_concepts<checks>(nums);
    static_check_iterator_category<std::input_iterator_tag>(nums);

    auto it = std::ranges::begin(nums);

    it += 6;
    CHECK(*it == 5);
    CHECK(*(it + 17) == 7);

    it -= 3;
    CHECK(*it == 7);
    CHECK(*(it - 15) == 1);

    CHECK(!nums.empty());

    SECTION("-> take") {
      auto nums2 = nums | std::views::take(5);

      static_check_range_concepts<checks>(nums2);
      static_check_iterator_category<std::input_iterator_tag>(nums2);

      CHECK(to_vector(nums2) == std::vector<int>{1, 3, 5, 7, 1});
    }
  }

  SECTION("all -> drop -> ring(bound = 3)") {
    auto nums = deq | std::views::drop(1) | ring(3);

    constexpr auto checks = check_range_concepts_config<int>{
        .range_type = range_type::random_access_range,
        .is_viewable_range = true,
        .is_common_range = true,
        .is_output_range = true,
        .is_sized_range = true,
    };
    static_check_range_concepts<checks>(nums);
    static_check_iterator_category<std::random_access_iterator_tag>(nums);

    CHECK(!nums.empty());
    CHECK(nums.size() == 9);
    CHECK((nums.end() - nums.begin()) == 9);
    CHECK(to_vector(nums) == std::vector<int>{3, 5, 7, 3, 5, 7, 3, 5, 7});
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
  static_check_range_concepts<checks>(nums);
  static_check_iterator_category<std::random_access_iterator_tag>(nums);

  CHECK(vec == std::vector<std::size_t>{2, 3, 4, 5});
  CHECK(to_vector(nums) == std::vector<std::size_t>{2, 3, 4, 5, 2, 3, 4, 5});
}

// TODO(tests): deduction guides, more bounded tests, other std views and algorithms,
// kv-containers, iterator/sentinel concepts, big bounds, out of range, random access ops,
// constexpr, noexcept

// NOLINTEND
