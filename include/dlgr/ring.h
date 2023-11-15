// Copyright 2023 Deligor <deligor6321@gmail.com>

#pragma once

#include <concepts>
#include <iterator>
#include <ranges>
#include <utility>

namespace dlgr {
namespace ranges {

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
  // -- Member types

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

  // -- Constructors

  [[nodiscard]] constexpr iterator() noexcept = default;

  [[nodiscard]] constexpr iterator(base_iterator_type begin, base_iterator_type end)
      : iterator(begin, begin, end) {}

  // -- Data access

  [[nodiscard]] constexpr auto operator*() const -> reference { return *curr_; }

  // -- Operations

  constexpr auto operator++() -> iterator& {
    if (empty()) {
      return *this;
    }

    ++curr_;
    if (curr_ == end_) {
      curr_ = begin_;
    }

    return *this;
  }

  constexpr auto operator++(int) -> iterator {
    auto iter = *this;
    this->operator++();
    return iter;
  }

  constexpr auto operator--() -> iterator&
    requires std::ranges::bidirectional_range<RangeType>
  {
    if (empty()) {
      return *this;
    }

    if (curr_ == begin_) {
      curr_ = end_;
    }
    --curr_;

    return *this;
  }

  constexpr auto operator--(int) -> iterator
    requires std::ranges::bidirectional_range<RangeType>
  {
    auto iter = *this;
    this->operator--();
    return iter;
  }

  constexpr auto operator+=(difference_type diff) -> iterator&
    requires std::ranges::random_access_range<RangeType>
  {
    if (empty()) {
      return *this;
    }

    if constexpr (std::is_signed_v<difference_type>) {
      if (diff < 0) {
        return *this -= (-diff);
      }
    }

    const auto len = std::ranges::distance(begin_, end_);
    const auto to_end = curr_ == begin_ ? len : std::ranges::distance(curr_, end_);
    if constexpr (std::is_integral_v<difference_type>) {
      diff = (diff - to_end) % len;
    } else {
      if (diff > to_end) {
        diff -= to_end;
        while (diff >= len) {
          diff -= len;
        }
      }
    }

    curr_ = begin_ + diff;
    return *this;
  }

  constexpr auto operator-=(difference_type diff) -> iterator&
    requires std::ranges::random_access_range<RangeType>
  {
    if (empty()) {
      return *this;
    }

    if constexpr (std::is_signed_v<difference_type>) {
      if (diff < 0) {
        return *this += (-diff);
      }
    }

    // rend = reverse end, rbegin = reverse begin
    const auto len = std::ranges::distance(begin_, end_);
    const auto rbegin = end_ - 1;
    const auto to_rend = curr_ == rbegin ? len : std::ranges::distance(begin_, curr_) + 1;
    if constexpr (std::is_integral_v<difference_type>) {
      diff = (diff - to_rend) % len;
    } else {
      if (diff > to_rend) {
        diff -= to_rend;
        while (diff >= len) {
          diff -= len;
        }
      }
    }

    curr_ = rbegin - diff;
    return *this;
  }

  [[nodiscard]] constexpr auto operator[](difference_type diff) const -> reference
    requires std::ranges::random_access_range<RangeType>
  {
    return *(*this + diff);
  }

  // -- Non-member operations

  [[nodiscard]] constexpr friend auto operator==([[maybe_unused]] sentinel sentinel,
                                                 const iterator& iterator) -> bool {
    return iterator.empty();
  }

  [[nodiscard]] constexpr friend auto operator+(iterator iter, difference_type diff) -> iterator
    requires std::ranges::random_access_range<RangeType>
  {
    iter += diff;
    return iter;
  }

  [[nodiscard]] constexpr friend auto operator+(difference_type diff, iterator iter) -> iterator
    requires std::ranges::random_access_range<RangeType>
  {
    iter += diff;
    return iter;
  }

  [[nodiscard]] constexpr friend auto operator-(iterator iter, difference_type diff) -> iterator
    requires std::ranges::random_access_range<RangeType>
  {
    iter -= diff;
    return iter;
  }

  [[nodiscard]] constexpr friend auto operator-(difference_type diff, iterator iter) -> iterator
    requires std::ranges::random_access_range<RangeType>
  {
    iter -= diff;
    return iter;
  }

  [[nodiscard]] constexpr friend auto operator-(const iterator& lhs, const iterator& rhs)
      -> difference_type
    requires std::ranges::random_access_range<RangeType>
  {
    return lhs.curr_ - rhs.curr_;
  }

  // -- Comparison

  [[nodiscard]] constexpr friend auto operator==(const iterator& lhs, const iterator& rhs) -> bool
    requires std::equality_comparable<base_iterator_type>
  {
    return lhs.curr_ == rhs.curr_;
  }

  [[nodiscard]] constexpr friend auto operator<(const iterator& lhs, const iterator& rhs) -> bool
    requires std::totally_ordered<base_iterator_type>
  {
    return lhs.curr_ < rhs.curr_;
  }

  [[nodiscard]] constexpr friend auto operator>(const iterator& lhs, const iterator& rhs) -> bool
    requires std::totally_ordered<base_iterator_type>
  {
    return lhs.curr_ < rhs.curr_;
  }

  [[nodiscard]] constexpr friend auto operator<=(const iterator& lhs, const iterator& rhs) -> bool
    requires std::totally_ordered<base_iterator_type>
  {
    return lhs.curr_ <= rhs.curr_;
  }

  [[nodiscard]] constexpr friend auto operator>=(const iterator& lhs, const iterator& rhs) -> bool
    requires std::totally_ordered<base_iterator_type>
  {
    return lhs.curr_ <= rhs.curr_;
  }

 private:
  constexpr iterator(base_iterator_type curr, base_iterator_type begin, base_iterator_type end)
      : curr_{std::move(curr)}, begin_{std::move(begin)}, end_{std::move(end)} {}

  [[nodiscard]] constexpr auto empty() const -> bool {
    if constexpr (std::totally_ordered<base_iterator_type>) {
      return begin_ >= end_;
    } else {
      return begin_ == end_;
    }
  }

  base_iterator_type curr_ = {};
  base_iterator_type begin_ = {};
  base_iterator_type end_ = {};
};

template <class RangeType>
ring_view(RangeType&&) -> ring_view<std::views::all_t<RangeType>>;

}  // namespace ranges

namespace views {

struct ring {
  constexpr ring() noexcept = default;

  template <std::ranges::forward_range RangeType>
  constexpr auto operator()(RangeType&& range) const {
    return ranges::ring_view(std::forward<RangeType>(range));
  }
};

template <std::ranges::forward_range RangeType>
constexpr auto operator|(RangeType&& range, const ring& ring) {
  return ring(std::forward<RangeType>(range));
}

}  // namespace views

}  // namespace dlgr
