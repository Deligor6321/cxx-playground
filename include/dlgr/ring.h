// Copyright 2023 Deligor <deligor6321@gmail.com>

#pragma once

#include <concepts>
#include <cstdlib>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace dlgr {

// == Utility concepts

template <class T, class... Ts>
concept is_any_of = (std::same_as<T, Ts> || ...);

namespace ranges {

// == Type aliases and concepts

using ring_view_bound_t = std::size_t;
using ring_view_unreachable_bound_t = std::unreachable_sentinel_t;

template <class T>
concept ring_view_bound = is_any_of<T, ring_view_bound_t, ring_view_unreachable_bound_t>;

constexpr inline ring_view_unreachable_bound_t ring_view_unreachable_bound = {};

// == ring_view implementation

template <std::ranges::forward_range RangeType,
          ring_view_bound BoundType = ring_view_unreachable_bound_t>
class ring_view : public std::ranges::view_interface<ring_view<RangeType, BoundType>> {
 public:
  // -- Nested types
  template <bool Const>
  class iterator;
  class sentinel;

  // -- Member types
  using base_type = RangeType;
  using bound_type = BoundType;
  using iterator_type = iterator<false>;
  using const_iterator_type = iterator<true>;

  // -- Sentinel

  class sentinel {
    friend class ring_view;

    template <bool Const>
    friend class iterator;

   public:
    [[nodiscard]] constexpr sentinel() noexcept = default;

   private:
    [[nodiscard]] constexpr explicit sentinel(bound_type bound) noexcept : bound_(bound) {}

    bound_type bound_ = {};
  };

  // -- Constructors

  [[nodiscard]] constexpr ring_view()
    requires std::default_initializable<base_type>
  = default;

  [[nodiscard]] constexpr explicit ring_view(base_type base, bound_type bound = {})
      : base_(std::move(base)), sentinel_{bound} {}

  // -- Range operation

  [[nodiscard]] constexpr auto begin() -> iterator_type {
    auto base_begin = std::ranges::begin(base_);
    auto base_end = std::ranges::end(base_);
    return {base_begin, base_begin, base_end};
  }

  [[nodiscard]] constexpr auto begin() const -> const_iterator_type {
    auto base_begin = std::ranges::begin(base_);
    auto base_end = std::ranges::end(base_);
    return {base_begin, base_begin, base_end};
  }

  [[nodiscard]] constexpr auto end() const noexcept -> sentinel { return sentinel_; }

  // -- Base access

  [[nodiscard]] constexpr auto base() const& -> base_type
    requires std::copy_constructible<base_type>
  {
    return base_;
  }
  [[nodiscard]] constexpr auto base() && -> base_type { return std::move(base_); }

 private:
  base_type base_;
  sentinel sentinel_;
};

// == ring_view::iterator implementation

template <std::ranges::forward_range RangeType, ring_view_bound BoundType>
template <bool Const>
class ring_view<RangeType, BoundType>::iterator {
  using parent_type = std::conditional_t<Const, std::add_const_t<ring_view<RangeType, BoundType>>,
                                         ring_view<RangeType, BoundType>>;
  friend class ring_view<RangeType, BoundType>;

  using parent_base_type =
      std::conditional_t<Const, std::add_const_t<typename parent_type::base_type>,
                         typename parent_type::base_type>;
  using sentinel = typename parent_type::sentinel;
  using bound_type = typename parent_type::bound_type;

  using position_type = std::make_signed_t<ring_view_bound_t>;

 public:
  // -- Member types

  using base_iterator_type = std::ranges::iterator_t<parent_base_type>;

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

  // -- Data access

  [[nodiscard]] constexpr auto operator*() const -> reference { return *curr_; }

  // -- Operations

  constexpr auto operator++() -> iterator& { return inc(); }

  constexpr auto operator++(int) -> iterator {
    auto iter = *this;
    inc();
    return iter;
  }

  constexpr auto operator--() -> iterator&
    requires std::ranges::bidirectional_range<parent_base_type>
  {
    return dec();
  }

  constexpr auto operator--(int) -> iterator
    requires std::ranges::bidirectional_range<parent_base_type>
  {
    auto iter = *this;
    dec();
    return iter;
  }

  constexpr auto operator+=(difference_type diff) -> iterator&
    requires std::ranges::random_access_range<parent_base_type>
  {
    if (empty()) {
      return *this;
    }

    if constexpr (std::is_signed_v<difference_type>) {
      if (diff < 0) {
        return sub(-diff);
      }
    }

    return add(diff);
  }

  constexpr auto operator-=(difference_type diff) -> iterator&
    requires std::ranges::random_access_range<parent_base_type>
  {
    if (empty()) {
      return *this;
    }

    if constexpr (std::is_signed_v<difference_type>) {
      if (diff < 0) {
        return add(-diff);
      }
    }

    return sub(diff);
  }

  [[nodiscard]] constexpr auto operator[](difference_type diff) const -> reference
    requires std::ranges::random_access_range<parent_base_type>
  {
    return *(*this + diff);
  }

  // -- Sentinel equality comparison

  [[nodiscard]] constexpr auto operator==(const sentinel& sen) const -> bool {
    if (curr_ == end_) {
      return true;
    }
    if constexpr (std::integral<bound_type>) {
      if (pos_ >= 0) {
        return static_cast<bound_type>(pos_) >= sen.bound_;
      }
      return true;
    } else {
      return false;
    }
  }

  [[nodiscard]] constexpr friend auto operator==(const sentinel& sen, const iterator& iter)
      -> bool {
    return iter == sen;
  }

  // -- Non-member operations

  [[nodiscard]] constexpr friend auto operator+(iterator iter, difference_type diff) -> iterator
    requires std::ranges::random_access_range<parent_base_type>
  {
    iter += diff;
    return iter;
  }

  [[nodiscard]] constexpr friend auto operator+(difference_type diff, iterator iter) -> iterator
    requires std::ranges::random_access_range<parent_base_type>
  {
    iter += diff;
    return iter;
  }

  [[nodiscard]] constexpr friend auto operator-(iterator iter, difference_type diff) -> iterator
    requires std::ranges::random_access_range<parent_base_type>
  {
    iter -= diff;
    return iter;
  }

  [[nodiscard]] constexpr friend auto operator-(difference_type diff, iterator iter) -> iterator
    requires std::ranges::random_access_range<parent_base_type>
  {
    iter -= diff;
    return iter;
  }

  [[nodiscard]] constexpr friend auto operator-(const iterator& lhs, const iterator& rhs)
      -> difference_type
    requires std::ranges::random_access_range<parent_base_type>
  {
    return lhs.curr_ - rhs.curr_;
  }

  // -- Comparison

  [[nodiscard]] constexpr friend auto operator==(const iterator& lhs, const iterator& rhs) -> bool
    requires std::equality_comparable<base_iterator_type>
  {
    return lhs.pos_ == rhs.pos_ && lhs.curr_ == rhs.curr_;
  }

  [[nodiscard]] constexpr friend auto operator<(const iterator& lhs, const iterator& rhs) -> bool
    requires std::totally_ordered<base_iterator_type>
  {
    return lhs.pos_ < rhs.pos_ || (lhs.pos_ == rhs.pos_ && lhs.curr_ < rhs.curr_);
  }

  [[nodiscard]] constexpr friend auto operator>(const iterator& lhs, const iterator& rhs) -> bool
    requires std::totally_ordered<base_iterator_type>
  {
    return lhs.pos_ > rhs.pos_ || (lhs.pos_ == rhs.pos_ && lhs.curr_ > rhs.curr_);
  }

  [[nodiscard]] constexpr friend auto operator<=(const iterator& lhs, const iterator& rhs) -> bool
    requires std::totally_ordered<base_iterator_type>
  {
    return lhs.pos_ < rhs.pos_ || (lhs.pos_ == rhs.pos_ && lhs.curr_ <= rhs.curr_);
  }

  [[nodiscard]] constexpr friend auto operator>=(const iterator& lhs, const iterator& rhs) -> bool
    requires std::totally_ordered<base_iterator_type>
  {
    return lhs.pos_ > rhs.pos_ || (lhs.pos_ == rhs.pos_ && lhs.curr_ >= rhs.curr_);
  }

 private:
  [[nodiscard]] constexpr iterator(base_iterator_type curr, base_iterator_type begin,
                                   base_iterator_type end, position_type pos = {})
      : curr_{std::move(curr)}, begin_{std::move(begin)}, end_{std::move(end)}, pos_{pos} {}

  [[nodiscard]] constexpr auto empty() const -> bool {
    if constexpr (std::totally_ordered<base_iterator_type>) {
      return begin_ >= end_;
    } else {
      return begin_ == end_;
    }
  }

  constexpr auto inc() -> iterator& {
    ++curr_;
    if (curr_ == end_) {
      ++pos_;
      curr_ = begin_;
    }

    return *this;
  }

  constexpr auto dec() -> iterator&
    requires std::ranges::bidirectional_range<parent_base_type>
  {
    if (curr_ == begin_) {
      curr_ = end_;
      --pos_;
    }
    --curr_;

    return *this;
  }

  // Precondition: diff >= 0
  constexpr auto add(difference_type diff) -> iterator&
    requires std::ranges::random_access_range<parent_base_type>
  {
    const auto len = std::ranges::distance(begin_, end_);
    const auto to_end = curr_ == begin_ ? len : std::ranges::distance(curr_, end_);
    if constexpr (std::is_integral_v<difference_type>) {
      using std::div;  // enable ADL
      const auto div_mod = div(diff - to_end, len);
      pos_ += div_mod.quot;
      diff = div_mod.rem;
    } else {
      if (diff > to_end) {
        diff -= to_end;
        ++pos_;
        while (diff >= len) {
          diff -= len;
          ++pos_;
        }
      }
    }

    curr_ = begin_ + diff;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return *this;
  }

  // Precondition: diff >= 0
  constexpr auto sub(difference_type diff) -> iterator&
    requires std::ranges::random_access_range<parent_base_type>
  {
    // rend = reverse end, rbegin = reverse begin
    const auto len = std::ranges::distance(begin_, end_);
    const auto rbegin = end_ - 1;
    const auto to_rend = curr_ == rbegin ? len : std::ranges::distance(begin_, curr_) + 1;
    if constexpr (std::is_integral_v<difference_type>) {
      using std::div;  // enable ADL
      const auto div_mod = div(diff - to_rend, len);
      pos_ -= div_mod.quot;
      diff = div_mod.rem;
    } else {
      if (diff > to_rend) {
        diff -= to_rend;
        --pos_;
        while (diff >= len) {
          --pos_;
          diff -= len;
        }
      }
    }

    curr_ = rbegin - diff;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return *this;
  }

  base_iterator_type curr_ = {};
  base_iterator_type begin_ = {};
  base_iterator_type end_ = {};
  position_type pos_ = {};
};

// == ring_view deduction guides

template <std::ranges::forward_range RangeType, std::integral BoundType>
ring_view(RangeType&&, BoundType) -> ring_view<std::views::all_t<RangeType>, ring_view_bound_t>;

template <std::ranges::forward_range RangeType>
ring_view(RangeType&&, ring_view_unreachable_bound_t)
    -> ring_view<std::views::all_t<RangeType>, ring_view_unreachable_bound_t>;

template <std::ranges::forward_range RangeType>
ring_view(RangeType&&) -> ring_view<std::views::all_t<RangeType>>;

}  // namespace ranges

namespace views {

// == ring implementation

template <ranges::ring_view_bound BoundType = ranges::ring_view_unreachable_bound_t>
class ring {
 public:
  using bound_type = BoundType;

  [[nodiscard]] constexpr explicit ring(bound_type bound = {}) noexcept : bound_(bound) {}

  template <std::ranges::forward_range RangeType>
  [[nodiscard]] constexpr auto operator()(RangeType&& range) const {
    return ranges::ring_view(std::forward<RangeType>(range), bound_);
  }

 private:
  bound_type bound_ = {};
};

template <std::ranges::forward_range RangeType, ranges::ring_view_bound BoundType>
constexpr auto operator|(RangeType&& range, const ring<BoundType>& ring) {
  return ring(std::forward<RangeType>(range));
}

// == ring deduction guides

template <std::integral BoundType>
ring(BoundType) -> ring<ranges::ring_view_bound_t>;

}  // namespace views

// TODO(improve): reverse, borrow, non-common range?, noexcept

}  // namespace dlgr
