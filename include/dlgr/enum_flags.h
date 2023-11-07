// Copyright 2023 Deligor <deligor6321@gmail.com>

#pragma once

#include <bit>
#include <type_traits>

namespace dlgr {

// == Mask specification

struct enum_flags_mask_unspecified_t {
  // NOLINTNEXTLINE(runtime/explicit): Explicit default ctor for tag type
  constexpr explicit enum_flags_mask_unspecified_t() noexcept = default;
};

inline constexpr enum_flags_mask_unspecified_t enum_flags_mask_unspecified{};

template <std::unsigned_integral MaskType, MaskType MaskValue>
struct enum_flags_mask_spec_t : std::integral_constant<MaskType, MaskValue> {
  // NOLINTNEXTLINE(runtime/explicit): Explicit default ctor for tag type
  constexpr explicit enum_flags_mask_spec_t() noexcept = default;
};

template <std::unsigned_integral MaskType, MaskType MaskValue>
inline constexpr enum_flags_mask_spec_t<MaskType, MaskValue> enum_flags_mask_spec{};

// == Implementation details declarations and utils

namespace detail {

template <class EnumType>
using enum_flags_data_t = std::make_unsigned_t<std::underlying_type_t<EnumType>>;

template <class EnumType, enum_flags_data_t<EnumType> EffectiveMask>
  requires std::is_enum_v<EnumType>
class enum_flags_impl;

template <class EnumType, EnumType... EnumValues>
  requires std::is_enum_v<EnumType>
struct make_enum_flags_mask_spec {
 private:
  using enum_flags_data_type = detail::enum_flags_data_t<EnumType>;

 public:
  using type = enum_flags_mask_spec_t<enum_flags_data_type,
                                      (enum_flags_data_type(0) | ... |
                                       std::bit_cast<enum_flags_data_type>(EnumValues))>;
};

}  // namespace detail

// == Make mask utils

template <class EnumType, EnumType... EnumValues>
  requires std::is_enum_v<EnumType>
using enum_flags_mask_t = typename detail::make_enum_flags_mask_spec<EnumType, EnumValues...>::type;

template <class EnumType, EnumType... EnumValues>
  requires std::is_enum_v<EnumType>
inline constexpr auto enum_flags_mask = enum_flags_mask_t<EnumType, EnumValues...>{};

// == Template declaration

template <class Enum, class Mask = enum_flags_mask_unspecified_t>
class enum_flags;

// == Template specialization for unspecified mask

template <class EnumType>
  requires std::is_enum_v<EnumType>
class enum_flags<EnumType, enum_flags_mask_unspecified_t> {
  using enum_flags_impl_type =
      detail::enum_flags_impl<EnumType, static_cast<detail::enum_flags_data_t<EnumType>>(~0ULL)>;

 public:
  // -- Member types

  using flag_type = typename enum_flags_impl_type::flag_type;
  using underlying_data_type = typename enum_flags_impl_type::underlying_data_type;
  using mask_spec_type = enum_flags_mask_unspecified_t;

  // -- Constructors

  [[nodiscard]] constexpr enum_flags() noexcept = default;

  [[nodiscard]] constexpr enum_flags(flag_type flag) noexcept  // NOLINT: Allow non-explicit ctor
      : flags_(flag) {}

  [[nodiscard]] constexpr enum_flags(flag_type flag,
                                     [[maybe_unused]] mask_spec_type mask_spec) noexcept
      : enum_flags(flag) {}

  // -- Comparison

  [[nodiscard]] constexpr auto operator==(const enum_flags&) const noexcept -> bool = default;

  // -- Conversion

  [[nodiscard]] constexpr explicit operator bool() const noexcept { return has_any(); }

  [[nodiscard]] constexpr explicit operator enum_flags::underlying_data_type() const noexcept {
    return static_cast<underlying_data_type>(flags_);
  }

  // -- Access

  [[nodiscard]] constexpr auto operator&(flag_type flag) const noexcept -> bool {
    return test(flag);
  }

  [[nodiscard]] constexpr auto test(flag_type flag) const noexcept -> bool {
    return flags_.test(flag);
  }

  [[nodiscard]] constexpr auto test(enum_flags flags) const noexcept -> bool {
    return flags_.test(flags.flags_);
  }

  [[nodiscard]] constexpr auto has_any() const noexcept -> bool { return !has_none(); }

  [[nodiscard]] constexpr auto has_none() const noexcept -> bool { return flags_.has_none(); }

  // -- Modification

  constexpr auto operator|=(enum_flags other) noexcept -> enum_flags& {
    flags_ |= other.flags_;
    return *this;
  }

  constexpr auto operator&=(enum_flags other) noexcept -> enum_flags& {
    flags_ &= other.flags_;
    return *this;
  }

  constexpr auto operator^=(enum_flags other) noexcept -> enum_flags& {
    flags_ ^= other.flags_;
    return *this;
  }

  constexpr auto set(enum_flags flags) noexcept -> enum_flags& {
    flags_.set(flags.flags_);
    return *this;
  }

  constexpr auto reset(enum_flags flags) noexcept -> enum_flags& {
    flags_.reset(flags.flags_);
    return *this;
  }

  constexpr auto flip(enum_flags flags) noexcept -> enum_flags& {
    flags_.flip(flags.flags_);
    return *this;
  }

  constexpr auto reset_all() noexcept -> enum_flags& {
    flags_.reset_all();
    return *this;
  }

  // -- Non-member functions

  [[nodiscard]] friend constexpr auto operator|(enum_flags lhs, enum_flags rhs) noexcept
      -> enum_flags {
    return (lhs |= rhs);
  }

  [[nodiscard]] friend constexpr auto operator&(enum_flags lhs, enum_flags rhs) noexcept
      -> enum_flags {
    return (lhs &= rhs);
  }

  [[nodiscard]] friend constexpr auto operator^(enum_flags lhs, enum_flags rhs) noexcept
      -> enum_flags {
    return (lhs ^= rhs);
  }

  // -- Static functions

  [[nodiscard]] static constexpr auto none() noexcept -> enum_flags {
    auto flags = enum_flags();
    flags.flags_ = enum_flags_impl_type::none();
    return flags;
  }

 private:
  enum_flags_impl_type flags_ = {};
};

// == Template specialization for specified mask

template <class EnumType, detail::enum_flags_data_t<EnumType> Mask>
  requires std::is_enum_v<EnumType>
class enum_flags<EnumType, enum_flags_mask_spec_t<decltype(Mask), Mask>> {
  using enum_flags_impl_type = detail::enum_flags_impl<EnumType, Mask>;
  static_assert(enum_flags_mask_spec_t<decltype(Mask), Mask>::value ==
                enum_flags_impl_type::effective_mask);

 public:
  // -- Member types

  using flag_type = typename enum_flags_impl_type::flag_type;
  using underlying_data_type = typename enum_flags_impl_type::underlying_data_type;
  using mask_spec_type = enum_flags_mask_spec_t<decltype(Mask), Mask>;

  // -- Static members

  constexpr static auto mask = mask_spec_type::value;

  // -- Constructors

  [[nodiscard]] constexpr enum_flags() noexcept = default;

  [[nodiscard]] constexpr enum_flags(flag_type flag) noexcept  // NOLINT: Allow non-explicit ctor
      : flags_(flag) {}

  [[nodiscard]] constexpr enum_flags(flag_type flag,
                                     [[maybe_unused]] mask_spec_type mask_spec) noexcept
      : enum_flags(flag) {}

  // -- Comparison

  [[nodiscard]] constexpr auto operator==(const enum_flags&) const noexcept -> bool = default;

  // -- Conversion

  [[nodiscard]] constexpr explicit operator bool() const noexcept { return has_any(); }

  [[nodiscard]] constexpr explicit operator enum_flags::underlying_data_type() const noexcept {
    return static_cast<underlying_data_type>(flags_);
  }

  // -- Access

  [[nodiscard]] constexpr auto operator&(flag_type flag) const noexcept -> bool {
    return test(flag);
  }

  [[nodiscard]] constexpr auto test(flag_type flag) const noexcept -> bool {
    return flags_.test(flag);
  }

  [[nodiscard]] constexpr auto test(enum_flags flags) const noexcept -> bool {
    return flags_.test(flags.flags_);
  }

  [[nodiscard]] constexpr auto has_all() const noexcept -> bool { return flags_.has_all(); }

  [[nodiscard]] constexpr auto has_any() const noexcept -> bool { return !has_none(); }

  [[nodiscard]] constexpr auto has_none() const noexcept -> bool { return flags_.has_none(); }

  // -- Modification

  constexpr auto operator|=(enum_flags other) noexcept -> enum_flags& {
    flags_ |= other.flags_;
    return *this;
  }

  constexpr auto operator&=(enum_flags other) noexcept -> enum_flags& {
    flags_ &= other.flags_;
    return *this;
  }

  constexpr auto operator^=(enum_flags other) noexcept -> enum_flags& {
    flags_ ^= other.flags_;
    return *this;
  }

  constexpr auto set(enum_flags flags) noexcept -> enum_flags& {
    flags_.set(flags.flags_);
    return *this;
  }

  constexpr auto reset(enum_flags flags) noexcept -> enum_flags& {
    flags_.reset(flags.flags_);
    return *this;
  }

  constexpr auto flip(enum_flags flags) noexcept -> enum_flags& {
    flags_.flip(flags.flags_);
    return *this;
  }

  constexpr auto set_all() noexcept -> enum_flags& {
    flags_.set_all();
    return *this;
  }

  constexpr auto reset_all() noexcept -> enum_flags& {
    flags_.reset_all();
    return *this;
  }

  constexpr auto flip_all() noexcept -> enum_flags& {
    flags_.flip_all();
    return *this;
  }

  // -- Non-member functions

  [[nodiscard]] friend constexpr auto operator|(enum_flags lhs, enum_flags rhs) noexcept
      -> enum_flags {
    return (lhs |= rhs);
  }

  [[nodiscard]] friend constexpr auto operator&(enum_flags lhs, enum_flags rhs) noexcept
      -> enum_flags {
    return (lhs &= rhs);
  }

  [[nodiscard]] friend constexpr auto operator^(enum_flags lhs, enum_flags rhs) noexcept
      -> enum_flags {
    return (lhs ^= rhs);
  }

  [[nodiscard]] friend constexpr auto operator~(enum_flags flags) noexcept -> enum_flags {
    return flags.flip_all();
  }

  // -- Static functions

  [[nodiscard]] static constexpr auto all() noexcept -> enum_flags {
    auto flags = enum_flags();
    flags.flags_ = enum_flags_impl_type::all();
    return flags;
  }

  [[nodiscard]] static constexpr auto none() noexcept -> enum_flags {
    auto flags = enum_flags();
    flags.flags_ = enum_flags_impl_type::none();
    return flags;
  }

 private:
  enum_flags_impl_type flags_ = {};
};

// == Deduction guides

template <class EnumType>
enum_flags(EnumType) -> enum_flags<EnumType, enum_flags_mask_unspecified_t>;

template <class EnumType>
enum_flags(EnumType, enum_flags_mask_unspecified_t)
    -> enum_flags<EnumType, enum_flags_mask_unspecified_t>;

template <class EnumType, detail::enum_flags_data_t<EnumType> Mask>
enum_flags(EnumType, enum_flags_mask_spec_t<detail::enum_flags_data_t<EnumType>, Mask>)
    -> enum_flags<EnumType, enum_flags_mask_spec_t<detail::enum_flags_data_t<EnumType>, Mask>>;

// == Implementation details

namespace detail {
template <class EnumType, enum_flags_data_t<EnumType> EffectiveMask>
  requires std::is_enum_v<EnumType>
class enum_flags_impl {
 public:
  // -- Member types

  using flag_type = EnumType;
  using underlying_data_type = enum_flags_data_t<flag_type>;

  // -- Static members

  constexpr static underlying_data_type effective_mask = EffectiveMask;

  // -- Constructors

  [[nodiscard]] constexpr enum_flags_impl() noexcept = default;

  [[nodiscard]] constexpr explicit enum_flags_impl(flag_type flag) noexcept
      : enum_flags_impl(std::bit_cast<underlying_data_type>(flag)) {}

  // -- Comparison

  [[nodiscard]] constexpr auto operator==(const enum_flags_impl&) const noexcept -> bool = default;

  // -- Conversion

  [[nodiscard]] constexpr explicit operator enum_flags_impl::underlying_data_type() const noexcept {
    return flags_data_;
  }

  // -- Access
  [[nodiscard]] constexpr auto test(flag_type flag) const noexcept -> bool {
    if (!enum_flags_impl::can_represent(flag)) {
      return false;
    }
    return test(enum_flags_impl(flag));
  }

  [[nodiscard]] constexpr auto test(enum_flags_impl flags) const noexcept -> bool {
    return (flags_data_ & flags.flags_data_) == flags.flags_data_;
  }

  [[nodiscard]] constexpr auto has_all() const noexcept -> bool {
    return (*this == enum_flags_impl::all());
  }

  [[nodiscard]] constexpr auto has_none() const noexcept -> bool {
    return (*this == enum_flags_impl::none());
  }

  // -- Modification

  constexpr auto operator|=(enum_flags_impl other) noexcept -> enum_flags_impl& {
    flags_data_ |= other.flags_data_;
    return *this;
  }

  constexpr auto operator&=(enum_flags_impl other) noexcept -> enum_flags_impl& {
    flags_data_ &= other.flags_data_;
    return *this;
  }

  constexpr auto operator^=(enum_flags_impl other) noexcept -> enum_flags_impl& {
    flags_data_ ^= other.flags_data_;
    return *this;
  }

  constexpr auto set(enum_flags_impl flags) noexcept -> enum_flags_impl& {
    return (*this |= flags);
  }

  constexpr auto reset(enum_flags_impl flags) noexcept -> enum_flags_impl& {
    return (*this &= flags.flip_all());
  }

  constexpr auto flip(enum_flags_impl flags) noexcept -> enum_flags_impl& {
    return test(flags) ? reset(flags) : set(flags);
  }

  constexpr auto set_all() noexcept -> enum_flags_impl& { return (*this = enum_flags_impl::all()); }

  constexpr auto reset_all() noexcept -> enum_flags_impl& {
    return (*this = enum_flags_impl::none());
  }

  constexpr auto flip_all() noexcept -> enum_flags_impl& {
    flags_data_ = (~flags_data_ & enum_flags_impl::effective_mask);
    return *this;
  }

  // -- Static functions

  [[nodiscard]] static constexpr auto all() noexcept -> enum_flags_impl {
    return enum_flags_impl(effective_mask);
  }

  [[nodiscard]] static constexpr auto none() noexcept -> enum_flags_impl {
    return enum_flags_impl(0);
  }

  static constexpr auto can_represent(flag_type flag) noexcept -> bool {
    return enum_flags_impl(flag).flags_data_ == std::bit_cast<underlying_data_type>(flag);
  }

 private:
  [[nodiscard]] constexpr explicit enum_flags_impl(underlying_data_type flags_data) noexcept
      : flags_data_(flags_data & enum_flags_impl::effective_mask) {}

  underlying_data_type flags_data_ = 0;
};

}  // namespace detail

}  // namespace dlgr
