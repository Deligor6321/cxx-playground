// Copyright 2023 Deligor <deligor6321@gmail.com>

#pragma once

#include <iterator>
#include <ranges>
#include <utility>

namespace dlgr {

template <std::ranges::forward_range ViewType>
  requires std::ranges::view<ViewType>
class ring_view : public std::ranges::view_interface<ring_view<ViewType>> {
 public:
  class iterator;
  class sentinel {};

  using base_type = ViewType;

  explicit ring_view(const base_type& base) : base_(base) {}

  auto begin() const {
    return iterator{
        std::ranges::begin(base_),
        std::ranges::end(base_),
    };
  }

  auto end() const { return sentinel{}; }

  auto base() const noexcept -> const base_type& { return base_; }

 private:
  base_type base_;
};

template <std::ranges::forward_range ViewType>
  requires std::ranges::view<ViewType>
class ring_view<ViewType>::iterator {
 public:
  using base_iterator_type = std::ranges::iterator_t<ViewType>;

  using difference_type = typename base_iterator_type::difference_type;
  using value_type = typename base_iterator_type::value_type;
  using pointer = typename base_iterator_type::pointer;
  using reference = typename base_iterator_type::reference;
  using iterator_category = typename base_iterator_type::iterator_category;

  iterator() = default;

  iterator(base_iterator_type begin, base_iterator_type end) : iterator(begin, begin, end) {}

  friend auto operator==([[maybe_unused]] ring_view<ViewType>::sentinel sentinel, iterator iterator)
      -> bool {
    return iterator.begin_ == iterator.end_;
  }

  auto operator*() const -> reference { return *curr_; }

  auto operator++() -> iterator& {
    ++curr_;
    if (curr_ == end_) {
      curr_ = begin_;
    }
    return *this;
  }

  auto operator++(int) -> iterator {
    auto res = *this;
    this->operator++();
    return res;
  }

  auto operator--() -> iterator&
    requires std::ranges::bidirectional_range<ViewType>
  {
    if (curr_ == begin_) {
      curr_ = end_;
    }
    --curr_;
    return *this;
  }

  auto operator--(int) -> iterator
    requires std::ranges::bidirectional_range<ViewType>
  {
    auto res = *this;
    this->operator--();
    return res;
  }

 private:
  iterator(base_iterator_type curr, base_iterator_type begin, base_iterator_type end)
      : curr_{curr}, begin_{begin}, end_{end} {}

  base_iterator_type curr_ = {};
  base_iterator_type begin_ = {};
  base_iterator_type end_ = {};
};
template <class R>
ring_view(R&&) -> ring_view<std::views::all_t<R>>;

namespace views {

struct ring {
  constexpr auto operator()(std::ranges::forward_range auto range) const {
    return ring_view(range);
  }
};

auto operator|(std::ranges::forward_range auto&& range, const ring& ring) {
  return ring(std::forward<decltype(range)>(range));
}

}  // namespace views

}  // namespace dlgr
