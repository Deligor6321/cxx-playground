
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

template <class T>
using it_traits = std::iterator_traits<std::ranges::iterator_t<T>>;

template <class T>
using it_category = typename it_traits<T>::iterator_category;

}  // namespace

// NOLINTBEGIN
TEST_CASE("ring_view_unbounded") {  // cppcheck-suppress[naming-functionName]
  {
    const auto vec = std::vector<int>{0, 11, 23, 24, 27};
    auto nums = std::views::all(vec) | std::views::reverse | ring() | std::views::take(11);

    STATIC_CHECK(std::ranges::viewable_range<decltype(nums)>);
    STATIC_CHECK(std::ranges::input_range<decltype(nums)>);
    STATIC_CHECK(!std::ranges::output_range<decltype(nums), int>);
    STATIC_CHECK(std::is_same_v<it_category<decltype(nums)>, std::input_iterator_tag>);

    CHECK(to_vector(nums) == std::vector<int>{27, 24, 23, 11, 0, 27, 24, 23, 11, 0, 27});
  }

  {
    auto lst = std::list<int>{0, 11, 23, 24, 27};
    auto nums = std::ranges::take_view(ring_view(lst), 9);

    STATIC_CHECK(std::ranges::viewable_range<decltype(nums)>);
    STATIC_CHECK(std::ranges::input_range<decltype(nums)>);
    STATIC_CHECK(std::ranges::output_range<decltype(nums), int>);
    STATIC_CHECK(std::is_same_v<it_category<decltype(nums)>, std::input_iterator_tag>);

    CHECK(to_vector(nums) == std::vector<int>{0, 11, 23, 24, 27, 0, 11, 23, 24});
  }

  {
    auto flst = std::forward_list<int>{0, 11, 23, 24, 27};
    auto nums = std::views::all(flst) | ring() | std::views::take(17) | std::views::drop(4);

    STATIC_CHECK(std::ranges::viewable_range<decltype(nums)>);
    STATIC_CHECK(std::ranges::input_range<decltype(nums)>);
    STATIC_CHECK(std::ranges::output_range<decltype(nums), int>);
    STATIC_CHECK(std::is_same_v<it_category<decltype(nums)>, std::input_iterator_tag>);

    CHECK(to_vector(nums) == std::vector<int>{27, 0, 11, 23, 24, 27, 0, 11, 23, 24, 27, 0, 11});
  }

  {
    auto str = std::string("abcx");
    auto chars = std::views::all(str) | ring() | std::views::take(7);

    STATIC_CHECK(std::ranges::viewable_range<decltype(chars)>);
    STATIC_CHECK(std::ranges::input_range<decltype(chars)>);
    STATIC_CHECK(std::ranges::output_range<decltype(chars), char>);
    STATIC_CHECK(std::is_same_v<it_category<decltype(chars)>, std::input_iterator_tag>);

    CHECK(to_vector(chars) == std::vector<char>{'a', 'b', 'c', 'x', 'a', 'b', 'c'});
  }

  {
    auto empt = std::string_view("");
    auto chars = std::views::all(empt) | ring();

    STATIC_CHECK(std::ranges::viewable_range<decltype(chars)>);
    STATIC_CHECK(std::ranges::input_range<decltype(chars)>);
    STATIC_CHECK(!std::ranges::output_range<decltype(chars), char>);
    STATIC_CHECK(std::is_same_v<it_category<decltype(chars)>, std::input_iterator_tag>);

    CHECK(to_vector(chars) == std::vector<char>{});
  }

  {
    const auto chars = std::views::empty<const char> | ring();

    STATIC_CHECK(std::ranges::viewable_range<decltype(chars)>);
    STATIC_CHECK(std::ranges::input_range<decltype(chars)>);
    STATIC_CHECK(!std::ranges::output_range<decltype(chars), char>);
    STATIC_CHECK(std::is_same_v<it_category<decltype(chars)>, std::input_iterator_tag>);

    CHECK(to_vector(chars) == std::vector<char>{});
  }

  {
    auto bools = std::views::empty<bool> | ring();

    STATIC_CHECK(std::ranges::viewable_range<decltype(bools)>);
    STATIC_CHECK(std::ranges::input_range<decltype(bools)>);
    STATIC_CHECK(std::ranges::output_range<decltype(bools), bool>);
    STATIC_CHECK(std::is_same_v<it_category<decltype(bools)>, std::input_iterator_tag>);

    CHECK(to_vector(bools) == std::vector<bool>{});
  }

  {
    auto dbl = 12.0;
    auto dbls = std::views::single(dbl) | ring()
                | std::views::transform([n = 0](auto val) mutable { return ++n + val; })
                | std::views::take(5);

    STATIC_CHECK(std::ranges::viewable_range<decltype(dbls)>);
    STATIC_CHECK(std::ranges::input_range<decltype(dbls)>);
    STATIC_CHECK(!std::ranges::output_range<decltype(dbls), double>);
    // STATIC_CHECK(std::is_same_v<it_category<decltype(dbls)>, std::input_iterator_tag>);

    CHECK(to_vector(dbls) == std::vector<double>{13.0, 14.0, 15.0, 16.0, 17.0});
  }

  {
    auto deq = std::deque<int>{1, 3, 5, 7};
    auto nums = std::ranges::ref_view(deq) | ring();

    STATIC_CHECK(std::ranges::viewable_range<decltype(nums)>);
    STATIC_CHECK(std::ranges::input_range<decltype(nums)>);
    STATIC_CHECK(std::ranges::output_range<decltype(nums), int>);
    STATIC_CHECK(std::is_same_v<it_category<decltype(nums)>, std::input_iterator_tag>);

    auto it = std::ranges::begin(nums);

    it += 6;
    CHECK(*it == 5);
    CHECK(*(it + 17) == 7);

    it -= 3;
    CHECK(*it == 7);
    CHECK(*(it - 15) == 1);
  }
}

TEST_CASE("ring_view_bounded") {  // cppcheck-suppress[naming-functionName]
  {
    auto nums = std::ranges::single_view<int>(134) | ring(10);

    STATIC_CHECK(std::ranges::viewable_range<decltype(nums)>);
    STATIC_CHECK(std::ranges::random_access_range<decltype(nums)>);
    STATIC_CHECK(std::is_same_v<it_category<decltype(nums)>, std::random_access_iterator_tag>);
    STATIC_CHECK(std::ranges::output_range<decltype(nums), int>);
    STATIC_CHECK(std::ranges::common_range<decltype(nums)>);
    STATIC_CHECK(std::ranges::sized_range<decltype(nums)>);

    CHECK(!nums.empty());
    CHECK(nums.size() == 10);
    CHECK(to_vector(nums) == std::vector<int>{134, 134, 134, 134, 134, 134, 134, 134, 134, 134});
  }

  {
    auto vec = std::vector<std::size_t>{0, 1, 2, 3};
    auto nums = vec | ring(2);
    std::ranges::for_each(nums, [](auto& val) { ++val; });

    STATIC_CHECK(std::ranges::viewable_range<decltype(nums)>);
    STATIC_CHECK(std::ranges::random_access_range<decltype(nums)>);
    STATIC_CHECK(std::is_same_v<it_category<decltype(nums)>, std::random_access_iterator_tag>);
    STATIC_CHECK(std::ranges::output_range<decltype(nums), std::size_t>);

    CHECK(vec == std::vector<std::size_t>{2, 3, 4, 5});
    CHECK(to_vector(nums) == std::vector<std::size_t>{2, 3, 4, 5, 2, 3, 4, 5});
  }
}

// TODO(tests): deduction guides, more bounded tests, other std views and algorithms,
// kv-containers

// NOLINTEND
