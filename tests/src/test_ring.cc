
// Copyright 2023 Deligor <deligor6321@gmail.com>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <forward_list>
#include <iterator>
#include <list>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include <dlgr/ring.h>

namespace {

using dlgr::ring_view;
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
    CHECK(to_vector(nums) == std::vector<int>{27, 24, 23, 11, 0, 27, 24, 23, 11, 0, 27});
  }

  {
    auto lst = std::list<int>{0, 11, 23, 24, 27};
    auto nums = std::ranges::take_view(ring_view(lst), 9);
    CHECK(to_vector(nums) == std::vector<int>{0, 11, 23, 24, 27, 0, 11, 23, 24});
  }

  {
    auto flst = std::forward_list<int>{0, 11, 23, 24, 27};
    auto nums = std::views::all(flst) | ring() | std::views::take(17) | std::views::drop(4);
    CHECK(to_vector(nums) == std::vector<int>{27, 0, 11, 23, 24, 27, 0, 11, 23, 24, 27, 0, 11});
  }

  {
    auto str = std::string("abcx");
    auto chars = std::views::all(str) | ring() | std::views::take(7);
    CHECK(to_vector(chars) == std::vector<char>{'a', 'b', 'c', 'x', 'a', 'b', 'c'});
  }

  {
    auto empt = std::string_view("");
    auto chars = std::views::all(empt) | ring();
    CHECK(to_vector(chars) == std::vector<char>{});
  }

  {
    auto empt = std::string_view("");
    auto chars = std::views::all(empt) | ring();
    CHECK(to_vector(chars) == std::vector<char>{});
  }

  {
    auto bools = std::views::empty<bool> | ring();
    CHECK(to_vector(bools) == std::vector<bool>{});
  }

  {
    auto dbl = 12.0;
    auto dbls = std::views::single(dbl) | ring()
                | std::views::transform([n = 0](auto val) mutable { return ++n + val; })
                | std::views::take(5);
    CHECK(to_vector(dbls) == std::vector<double>{13.0, 14.0, 15.0, 16.0, 17.0});
  }
}
// NOLINTEND
