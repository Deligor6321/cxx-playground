
// Copyright 2023 Deligor <deligor6321@gmail.com>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <deque>
#include <forward_list>
#include <initializer_list>
#include <iterator>
#include <list>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include <dlgr/ring.h>

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

}  // namespace

// NOLINTBEGIN
TEST_CASE("ring_view") {
  {
    auto vec = std::vector<int>{0, 11, 23, 24, 27};
    auto nums = std::views::all(vec) | std::views::reverse | ring() | std::views::take(11);

    STATIC_CHECK(std::ranges::viewable_range<decltype(nums)>);
    STATIC_CHECK(std::ranges::random_access_range<decltype(nums)>);

    CHECK(to_vector(nums) == std::vector<int>{27, 24, 23, 11, 0, 27, 24, 23, 11, 0, 27});
  }

  {
    auto lst = std::list<int>{0, 11, 23, 24, 27};
    auto nums = std::ranges::take_view(ring_view(lst), 9);

    STATIC_CHECK(std::ranges::viewable_range<decltype(nums)>);
    STATIC_CHECK(std::ranges::bidirectional_range<decltype(nums)>);

    CHECK(to_vector(nums) == std::vector<int>{0, 11, 23, 24, 27, 0, 11, 23, 24});
  }

  {
    auto flst = std::forward_list<int>{0, 11, 23, 24, 27};
    auto nums = std::views::all(flst) | ring() | std::views::take(17) | std::views::drop(4);

    STATIC_CHECK(std::ranges::viewable_range<decltype(nums)>);
    STATIC_CHECK(std::ranges::forward_range<decltype(nums)>);

    CHECK(to_vector(nums) == std::vector<int>{27, 0, 11, 23, 24, 27, 0, 11, 23, 24, 27, 0, 11});
  }

  {
    auto str = std::string("abcx");
    auto chars = std::views::all(str) | ring() | std::views::take(7);

    STATIC_CHECK(std::ranges::viewable_range<decltype(chars)>);
    STATIC_CHECK(std::ranges::random_access_range<decltype(chars)>);

    CHECK(to_vector(chars) == std::vector<char>{'a', 'b', 'c', 'x', 'a', 'b', 'c'});
  }

  {
    auto empt = std::string_view("");
    auto chars = std::views::all(empt) | ring();

    STATIC_CHECK(std::ranges::viewable_range<decltype(chars)>);
    STATIC_CHECK(std::ranges::random_access_range<decltype(chars)>);

    CHECK(to_vector(chars) == std::vector<char>{});
  }

  {
    auto chars = std::views::empty<char> | ring();

    STATIC_CHECK(std::ranges::viewable_range<decltype(chars)>);
    STATIC_CHECK(std::ranges::random_access_range<decltype(chars)>);

    CHECK(to_vector(chars) == std::vector<char>{});
  }

  {
    auto bools = std::views::empty<bool> | ring();

    STATIC_CHECK(std::ranges::viewable_range<decltype(bools)>);
    STATIC_CHECK(std::ranges::random_access_range<decltype(bools)>);

    CHECK(to_vector(bools) == std::vector<bool>{});
  }

  {
    auto dbl = 12.0;
    auto dbls = std::views::single(dbl) | ring()
                | std::views::transform([n = 0](auto val) mutable { return ++n + val; })
                | std::views::take(5);

    STATIC_CHECK(std::ranges::viewable_range<decltype(dbls)>);
    STATIC_CHECK(std::ranges::random_access_range<decltype(dbls)>);

    CHECK(to_vector(dbls) == std::vector<double>{13.0, 14.0, 15.0, 16.0, 17.0});
  }

  {
    auto deq = std::deque<int>{1, 3, 5, 7};
    auto nums = std::ranges::ref_view(deq) | ring();

    STATIC_CHECK(std::ranges::viewable_range<decltype(nums)>);
    STATIC_CHECK(std::ranges::random_access_range<decltype(nums)>);

    auto it = std::ranges::begin(nums);

    it += 6;
    CHECK(*it == 5);
    CHECK(*(it + 17) == 7);

    it -= 3;
    CHECK(*it == 7);
    CHECK(*(it - 15) == 1);
  }

  // TODO: reverse, output, borrow, bounds, non-common range?
}
// NOLINTEND
