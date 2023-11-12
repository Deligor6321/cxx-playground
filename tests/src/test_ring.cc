
// Copyright 2023 Deligor <deligor6321@gmail.com>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <forward_list>
#include <iterator>
#include <list>
#include <vector>

#include <dlgr/ring.h>

namespace {
using dlgr::ring_view;
using dlgr::views::ring;

}  // namespace

// NOLINTBEGIN
TEST_CASE("ring_view") {
  {
    auto v = std::vector<int>{0, 11, 23, 24, 27};

    auto nums = std::views::all(v) | std::views::reverse | ring() | std::views::take(17);

    auto out = std::vector<int>();
    std::ranges::copy(nums, std::back_insert_iterator(out));

    CHECK(out == std::vector<int>{27, 24, 23, 11, 0, 27, 24, 23, 11, 0, 27, 24, 23, 11, 0, 27, 24});
  }

  {
    auto l = std::list<int>{0, 11, 23, 24, 27};

    auto nums = ring_view(std::views::all(l)) | std::views::take(10);

    auto out = std::vector<int>();
    std::ranges::copy(nums, std::back_insert_iterator(out));

    CHECK(out == std::vector<int>{0, 11, 23, 24, 27, 0, 11, 23, 24, 27});
  }

  {
    auto fl = std::forward_list<int>{0, 11, 23, 24, 27};

    auto nums = std::views::all(fl) | ring() | std::views::take(17) | std::views::drop(4);

    auto out = std::vector<int>();
    std::ranges::copy(nums, std::back_insert_iterator(out));

    CHECK(out == std::vector<int>{27, 0, 11, 23, 24, 27, 0, 11, 23, 24, 27, 0, 11});
  }
}
// NOLINTEND
