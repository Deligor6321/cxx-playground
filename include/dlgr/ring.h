// Copyright 2023 Deligor <deligor6321@gmail.com>

#pragma once

#include <concepts>
#include <iterator>
#include <ranges>
#include <utility>

namespace dlgr {

template <std::ranges::forward_range RangeType>
  requires std::ranges::view<RangeType> && std::ranges::common_range<RangeType>
class ring_view : public std::ranges::view_interface<ring_view<RangeType>> {
 public:
  class iterator;
  class sentinel {};

  using base_type = RangeType;

  constexpr ring_view()
    requires std::default_initializable<base_type>
  = default;

  constexpr explicit ring_view(base_type base) : base_(std::move(base)) {}

  constexpr auto begin() -> iterator {
    return iterator{
        std::ranges::begin(base_),
        std::ranges::end(base_),
    };
  }

  constexpr auto end() const -> sentinel { return sentinel{}; }

  constexpr auto base() const& -> base_type
    requires std::copy_constructible<base_type>
  {
    return base_;
  }
  constexpr auto base() && -> base_type { return std::move(base_); }

 private:
  base_type base_;
};

template <std::ranges::forward_range RangeType>
  requires std::ranges::view<RangeType> && std::ranges::common_range<RangeType>
class ring_view<RangeType>::iterator {
  using parent_type = ring_view<RangeType>;
  using sentinel = parent_type::sentinel;

 public:
  using base_iterator_type = std::ranges::iterator_t<RangeType>;

  using difference_type = std::iter_difference_t<base_iterator_type>;
  using value_type = std::iter_value_t<base_iterator_type>;
  using reference = std::iter_reference_t<base_iterator_type>;
  using pointer = typename std::iterator_traits<base_iterator_type>::pointer;
  using iterator_category = std::conditional_t<
      std::is_same_v<typename std::iterator_traits<base_iterator_type>::iterator_category,
                     std::contiguous_iterator_tag>,
      std::random_access_iterator_tag,
      typename std::iterator_traits<base_iterator_type>::iterator_category>;

  constexpr iterator() noexcept = default;

  constexpr iterator(base_iterator_type begin, base_iterator_type end)
      : iterator(begin, begin, end) {}

  constexpr friend auto operator==([[maybe_unused]] sentinel sentinel, iterator iterator) -> bool {
    return iterator.begin_ == iterator.end_;
  }

  constexpr auto operator*() const -> reference { return *curr_; }

  constexpr auto operator++() -> iterator& {
    ++curr_;
    if (curr_ == end_) {
      curr_ = begin_;
    }
    return *this;
  }

  constexpr auto operator++(int) -> iterator {
    auto res = *this;
    this->operator++();
    return res;
  }

  constexpr auto operator--() -> iterator&
    requires std::ranges::bidirectional_range<RangeType>
  {
    if (curr_ == begin_) {
      curr_ = end_;
    }
    --curr_;
    return *this;
  }

  constexpr auto operator--(int) -> iterator
    requires std::ranges::bidirectional_range<RangeType>
  {
    auto res = *this;
    this->operator--();
    return res;
  }

 private:
  constexpr iterator(base_iterator_type curr, base_iterator_type begin, base_iterator_type end)
      : curr_{std::move(curr)}, begin_{std::move(begin)}, end_{std::move(end)} {}

  base_iterator_type curr_ = {};
  base_iterator_type begin_ = {};
  base_iterator_type end_ = {};
};

template <class RangeType>
ring_view(RangeType&&) -> ring_view<std::views::all_t<RangeType>>;

namespace views {

struct ring {
  constexpr ring() noexcept = default;

  template <std::ranges::forward_range RangeType>
  constexpr auto operator()(RangeType&& range) const {
    return ring_view(std::forward<RangeType>(range));
  }
};

template <std::ranges::forward_range RangeType>
constexpr auto operator|(RangeType&& range, const ring& ring) {
  return ring(std::forward<RangeType>(range));
}

}  // namespace views

}  // namespace dlgr
