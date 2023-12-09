// Copyright 2023 Deligor <deligor6321@gmail.com>

#pragma once

#include <concepts>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <gsl/assert>

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
  constexpr static bool is_unbounded_ = std::is_same_v<BoundType, ring_view_unreachable_bound_t>;

 public:
  // -- Nested types
  template <bool Const>
  class iterator;

  class unreachable_sentinel {};

  // -- Member types
  using base_type = RangeType;
  using bound_type = BoundType;
  using iterator_type = iterator<false>;
  using const_iterator_type = iterator<true>;
  using sentinel_type = std::conditional_t<is_unbounded_, unreachable_sentinel, iterator_type>;
  using const_sentinel_type =
      std::conditional_t<is_unbounded_, unreachable_sentinel, const_iterator_type>;

  // -- Constructors

  [[nodiscard]] constexpr ring_view()
    requires std::default_initializable<base_type>
  = default;

  [[nodiscard]] constexpr explicit ring_view(base_type base, bound_type bound = {})
      : base_(std::move(base)), bound_{bound} {
    validate();
  }

  // -- Range operation

  [[nodiscard]] constexpr auto begin() -> iterator_type {
    auto base_begin = std::ranges::begin(base_);
    auto base_end = std::ranges::end(base_);
    return {
        /* begin */ base_begin,
        /* end   */ base_end,
    };
  }

  [[nodiscard]] constexpr auto begin() const -> const_iterator_type {
    auto base_begin = std::ranges::begin(base_);
    auto base_end = std::ranges::end(base_);
    return {
        /* begin */ base_begin,
        /* end   */ base_end,
    };
  }

  [[nodiscard]] constexpr auto end() -> iterator_type
    requires(!is_unbounded_)
  {
    auto base_begin = std::ranges::begin(base_);
    auto base_end = std::ranges::end(base_);
    return {
        /* begin */ base_begin,
        /* end   */ base_end,
        /* pos   */ base_begin != base_end ? bound_ : bound_type{},
    };
  }

  [[nodiscard]] constexpr auto end() const -> const_iterator_type
    requires(!is_unbounded_)
  {
    auto base_begin = std::ranges::begin(base_);
    auto base_end = std::ranges::end(base_);
    return {
        /* begin */ base_begin,
        /* end   */ base_end,
        /* pos   */ base_begin != base_end ? bound_ : bound_type{},
    };
  }

  [[nodiscard]] constexpr auto end() const -> unreachable_sentinel
    requires(is_unbounded_)
  {
    return {};
  }

  [[nodiscard]] constexpr auto size() const
    requires(!is_unbounded_ && std::ranges::sized_range<const base_type>)
  {
    return bound_ * std::ranges::size(base_);
  }

  [[nodiscard]] constexpr auto size()
    requires(!is_unbounded_ && std::ranges::sized_range<base_type>)
  {
    return bound_ * std::ranges::size(base_);
  }

  [[nodiscard]] constexpr auto empty() const -> bool
    requires requires(const base_type& base) { std::ranges::empty(base); }
  {
    if constexpr (!is_unbounded_) {
      if (bound_ == bound_type{}) {
        return true;
      }
    }
    return std::ranges::empty(base_);
  }

  [[nodiscard]] constexpr auto empty() -> bool
    requires requires(base_type& base) { std::ranges::empty(base); }
  {
    if constexpr (!is_unbounded_) {
      if (bound_ == bound_type{}) {
        return true;
      }
    }
    return std::ranges::empty(base_);
  }

  // -- Base access

  [[nodiscard]] constexpr auto base() const& -> base_type
    requires std::copy_constructible<base_type>
  {
    return base_;
  }

  [[nodiscard]] constexpr auto base() && -> base_type { return std::move(base_); }

 private:
  // -- Helper functions

  constexpr auto validate() const
      noexcept(is_unbounded_ || !std::ranges::random_access_range<base_type>) -> void {
    if constexpr (!is_unbounded_ && std::ranges::random_access_range<base_type>) {
      const auto base_size = std::ranges::size(base_);
      constexpr auto max_size = std::numeric_limits<decltype(base_size)>::max();
      if (base_size != 0 && bound_ > static_cast<bound_type>(max_size / base_size)) {
        // cppcheck-suppress[throwInNoexceptFunction]: Conditional noexcept is ok
        throw std::overflow_error("bound overflow");
      }
    }
  }

  // -- Data members

  base_type base_;
  bound_type bound_ = {};
};

// == Utility funtions implementation details of

namespace detail {

template <class Tag1, class Tag2>
using min_iterator_category_t = std::conditional_t<std::is_base_of_v<Tag1, Tag2>, Tag1, Tag2>;

}  // namespace detail

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
  using bound_type = typename parent_type::bound_type;
  using sentinel = typename parent_type::unreachable_sentinel;

  constexpr static bool is_unbounded_ = std::is_same_v<bound_type, ring_view_unreachable_bound_t>;

 public:
  // -- Member types

  using base_iterator_type = std::ranges::iterator_t<parent_base_type>;

  using difference_type = std::iter_difference_t<base_iterator_type>;
  using value_type = std::iter_value_t<base_iterator_type>;
  using reference = std::iter_reference_t<base_iterator_type>;
  using pointer = typename std::iterator_traits<base_iterator_type>::pointer;
  using iterator_category =
      std::conditional_t<is_unbounded_, std::input_iterator_tag,
                         detail::min_iterator_category_t<
                             typename std::iterator_traits<base_iterator_type>::iterator_category,
                             std::random_access_iterator_tag>>;

  // -- Constructors

  [[nodiscard]] constexpr iterator() noexcept = default;

  // -- Data access

  [[nodiscard]] constexpr auto operator*() const -> reference {
    expects_not_empty();
    return *curr_;
  }

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

  constexpr auto operator+=(difference_type diff) -> iterator& requires(
      std::ranges::random_access_range<parent_base_type>&& std::signed_integral<difference_type>) {
    return diff > 0 ? add(diff) : diff < 0 ? sub(-diff) : (*this);
  }

  constexpr auto
  operator-=(difference_type diff) -> iterator& requires(
      std::ranges::random_access_range<parent_base_type>&& std::signed_integral<difference_type>) {
    return diff > 0 ? sub(diff) : diff < 0 ? add(-diff) : (*this);
  }

  [[nodiscard]] constexpr auto
  operator[](difference_type diff) const -> reference
    requires(std::ranges::random_access_range<parent_base_type>
             && std::signed_integral<difference_type>)
  {
    return *(*this + diff);
  }

  // -- Conversions

  // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-*)
  [[nodiscard]] constexpr operator iterator<true>() const
    requires(!Const)
  {
    return {
        /* curr  */ curr_,
        /* begin */ begin_,
        /* end   */ end_,
        /* pos   */ pos_,
    };
  }

  // -- Sentinel equality comparison

  [[nodiscard]] constexpr auto operator==([[maybe_unused]] const sentinel& sen) const -> bool
    requires(is_unbounded_)
  {
    return curr_ == end_;
  }

  [[nodiscard]] constexpr friend auto operator==(const sentinel& sen, const iterator& iter) -> bool
    requires(is_unbounded_)
  {
    return iter == sen;
  }

  // -- Non-member operations

  [[nodiscard]] constexpr friend auto operator+(iterator iter, difference_type diff) -> iterator
    requires(std::ranges::random_access_range<parent_base_type>
             && std::signed_integral<difference_type>)
  {
    iter += diff;
    return iter;
  }

  [[nodiscard]] constexpr friend auto operator+(difference_type diff, iterator iter) -> iterator
    requires(std::ranges::random_access_range<parent_base_type>
             && std::signed_integral<difference_type>)
  {
    iter += diff;
    return iter;
  }

  [[nodiscard]] constexpr friend auto operator-(iterator iter, difference_type diff) -> iterator
    requires(std::ranges::random_access_range<parent_base_type>
             && std::signed_integral<difference_type>)
  {
    iter -= diff;
    return iter;
  }

  [[nodiscard]] constexpr friend auto operator-(difference_type diff, iterator iter) -> iterator
    requires(std::ranges::random_access_range<parent_base_type>
             && std::signed_integral<difference_type>)
  {
    iter -= diff;
    return iter;
  }

  [[nodiscard]] constexpr friend auto operator-(const iterator& lhs, const iterator& rhs)
      -> difference_type
    requires(!is_unbounded_ && std::ranges::random_access_range<parent_base_type>
             && std::signed_integral<difference_type>)
  {
    expects_same_range(lhs, rhs);

    return lhs.pos_ >= rhs.pos_ ? dist_from_to(lhs, rhs) : -dist_from_to(rhs, lhs);
  }

  // -- Comparison

  [[nodiscard]] constexpr friend auto operator==(const iterator& lhs, const iterator& rhs) -> bool
    requires(!is_unbounded_ && std::equality_comparable<base_iterator_type>)
  {
    expects_same_range(lhs, rhs);

    return lhs.pos_ == rhs.pos_ && lhs.curr_ == rhs.curr_;
  }

  [[nodiscard]] constexpr friend auto operator<(const iterator& lhs, const iterator& rhs) -> bool
    requires(!is_unbounded_ && std::totally_ordered<base_iterator_type>)
  {
    expects_same_range(lhs, rhs);

    return lhs.pos_ < rhs.pos_ || (lhs.pos_ == rhs.pos_ && lhs.curr_ < rhs.curr_);
  }

  [[nodiscard]] constexpr friend auto operator>(const iterator& lhs, const iterator& rhs) -> bool
    requires(!is_unbounded_ && std::totally_ordered<base_iterator_type>)
  {
    expects_same_range(lhs, rhs);

    return lhs.pos_ > rhs.pos_ || (lhs.pos_ == rhs.pos_ && lhs.curr_ > rhs.curr_);
  }

  [[nodiscard]] constexpr friend auto operator<=(const iterator& lhs, const iterator& rhs) -> bool
    requires(!is_unbounded_ && std::totally_ordered<base_iterator_type>)
  {
    expects_same_range(lhs, rhs);

    return lhs.pos_ < rhs.pos_ || (lhs.pos_ == rhs.pos_ && lhs.curr_ <= rhs.curr_);
  }

  [[nodiscard]] constexpr friend auto operator>=(const iterator& lhs, const iterator& rhs) -> bool
    requires(!is_unbounded_ && std::totally_ordered<base_iterator_type>)
  {
    expects_same_range(lhs, rhs);

    return lhs.pos_ > rhs.pos_ || (lhs.pos_ == rhs.pos_ && lhs.curr_ >= rhs.curr_);
  }

 private:
  // -- Constructor

  [[nodiscard]] constexpr iterator(base_iterator_type begin, base_iterator_type end,
                                   bound_type pos = {})
      : curr_{begin}, begin_{std::move(begin)}, end_{std::move(end)}, pos_{pos} {
    // Constraints
    if constexpr (!is_unbounded_) {
      static_assert(std::numeric_limits<difference_type>::max()
                    <= std::numeric_limits<bound_type>::max());
    }
  }

  // -- Helper functions

  constexpr auto inc() -> iterator& {
    expects_not_empty();

    ++curr_;
    if (curr_ == end_) {
      if constexpr (!is_unbounded_) {
        Expects(pos_ <= std::numeric_limits<bound_type>::max() - 1);
        ++pos_;
      }
      curr_ = begin_;
    }

    return *this;
  }

  constexpr auto dec() -> iterator&
    requires std::ranges::bidirectional_range<parent_base_type>
  {
    expects_not_empty();

    if (curr_ == begin_) {
      curr_ = end_;
      if constexpr (!is_unbounded_) {
        Expects(pos_ >= std::numeric_limits<bound_type>::min() + 1);
        --pos_;
      }
    }
    --curr_;

    return *this;
  }

  constexpr auto add(difference_type diff) -> iterator& requires(
      std::ranges::random_access_range<parent_base_type>&& std::signed_integral<difference_type>) {
    expects_not_empty();

    GSL_ASSUME(diff > 0);
    GSL_ASSUME(begin_ < end_ && begin_ <= curr_ && curr_ < end_);

    const auto to_end = std::ranges::distance(curr_, end_);
    GSL_ASSUME(to_end > 0);

    if (diff >= to_end) {
      const auto len = std::ranges::distance(begin_, end_);
      GSL_ASSUME(len > 0);

      const auto div_mod = std::div(diff - to_end, len);
      GSL_ASSUME(div_mod.rem >= 0 && div_mod.rem < len);

      if constexpr (!is_unbounded_) {
        GSL_ASSUME(div_mod.quot >= 0 && div_mod.quot < std::numeric_limits<difference_type>::max());

        constexpr auto bound_max = std::numeric_limits<bound_type>::max();
        const auto pos_diff = static_cast<bound_type>(div_mod.quot) + 1;
        Expects(pos_ <= bound_max - pos_diff);
        pos_ += pos_diff;
      }

      diff = div_mod.rem;
    }

    curr_ = begin_ + diff;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return *this;
  }

  constexpr auto sub(difference_type diff) -> iterator& requires(
      std::ranges::random_access_range<parent_base_type>&& std::signed_integral<difference_type>) {
    expects_not_empty();

    GSL_ASSUME(diff > 0);
    GSL_ASSUME(begin_ < end_ && begin_ <= curr_ && curr_ < end_);

    // rend = reverse end, rbegin = reverse begin
    const auto rbegin = end_ - 1;
    const auto to_rend = std::ranges::distance(begin_, curr_) + 1;
    GSL_ASSUME(to_rend > 0);

    if (diff >= to_rend) {
      const auto len = std::ranges::distance(begin_, end_);
      GSL_ASSUME(len > 0);

      const auto div_mod = std::div(diff - to_rend, len);
      GSL_ASSUME(div_mod.rem >= 0 && div_mod.rem < len);

      if constexpr (!is_unbounded_) {
        GSL_ASSUME(div_mod.quot >= 0 && div_mod.quot < std::numeric_limits<difference_type>::max());

        const auto pos_diff = static_cast<bound_type>(div_mod.quot) + 1;
        Expects(pos_ >= pos_diff);
        pos_ -= pos_diff;
      }

      diff = div_mod.rem;
    }

    curr_ = rbegin - diff;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return *this;
  }

  [[nodiscard]] constexpr friend auto dist_from_to(const iterator& from_it, const iterator& to_it)
      -> difference_type
    requires(!is_unbounded_ && std::ranges::random_access_range<parent_base_type>
             && std::signed_integral<difference_type>)
  {
    GSL_ASSUME(from_it.pos_ >= to_it.pos_);
    GSL_ASSUME(from_it.begin_ == to_it.begin_);
    GSL_ASSUME(from_it.end_ == to_it.end_);

    const auto len = std::ranges::distance(from_it.begin_, to_it.end_);
    if (len == 0) {
      return 0;
    }

    const auto pos_diff = from_it.pos_ - to_it.pos_;
    expects_mult_no_overflow(std::abs(len), pos_diff);

    return len * static_cast<difference_type>(pos_diff) + from_it.curr_ - to_it.curr_;
  }

  // -- Contracts

  // TODO(compiler): Replace with standard contracts
  constexpr static auto expects_same_range(const iterator& lhs, const iterator& rhs) -> void {
    Expects(lhs.begin_ == rhs.begin_);
    Expects(lhs.end_ == rhs.end_);
  }

  constexpr static auto expects_mult_no_overflow(const difference_type& len,
                                                 const bound_type& bound) -> void
    requires(!is_unbounded_)
  {
    GSL_ASSUME(len > 0);
    Expects(bound <= static_cast<bound_type>(std::numeric_limits<difference_type>::max() / len));
  }

  constexpr auto expects_not_empty() const -> void { Expects(curr_ != end_ && begin_ != end_); }

  base_iterator_type curr_ = {};
  base_iterator_type begin_ = {};
  base_iterator_type end_ = {};
  bound_type pos_ = {};
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

// TODO(compiler): Use range_adaptor_closure
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

// TODO(compiler): Replace GSL_ASSUME with assume attribute

// TODO(improve): borrow, non-common range?, noexcept, reverse unbounded, guarantees

}  // namespace dlgr
