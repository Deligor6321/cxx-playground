// Copyright 2023 Deligor <deligor6321@gmail.com>

#pragma once

#include <bit>
#include <limits>
#include <type_traits>

namespace dlgr {

template <class E, std::make_unsigned_t<std::underlying_type_t<E>> M =
                       std::numeric_limits<std::make_unsigned_t<std::underlying_type_t<E>>>::max()>
  requires std::is_enum_v<E>
class enum_flags {
 public:
  // -- Member types

  using flag_type = E;
  using underlying_data_type = std::make_unsigned_t<std::underlying_type_t<flag_type>>;

  // -- Constructors

  [[nodiscard]] constexpr enum_flags() noexcept = default;

  [[nodiscard]] constexpr enum_flags(flag_type flag) noexcept  // NOLINT: Allow non-explicit ctor
      : enum_flags(std::bit_cast<underlying_data_type>(flag)) {}

  // -- Comparison

  [[nodiscard]] constexpr auto operator==(const enum_flags&) const noexcept -> bool = default;

  // -- Conversion

  [[nodiscard]] constexpr explicit operator bool() const noexcept { return has_any(); }

  [[nodiscard]] constexpr explicit operator enum_flags::underlying_data_type() const noexcept {
    return flags_data_;
  }

  // -- Access

  [[nodiscard]] constexpr auto operator&(flag_type flag) const noexcept -> bool {
    return test(flag);
  }

  [[nodiscard]] constexpr auto test(enum_flags flags) const noexcept -> bool {
    return (*this & flags) == flags;
  }

  [[nodiscard]] constexpr auto has_all() const noexcept -> bool {
    return (*this == enum_flags::all());
  }

  [[nodiscard]] constexpr auto has_any() const noexcept -> bool { return !has_none(); }

  [[nodiscard]] constexpr auto has_none() const noexcept -> bool {
    return (*this == enum_flags::none());
  }

  // -- Modification

  constexpr auto operator|=(enum_flags other) noexcept -> enum_flags& {
    flags_data_ |= other.flags_data_;
    return *this;
  }

  constexpr auto operator&=(enum_flags other) noexcept -> enum_flags& {
    flags_data_ &= other.flags_data_;
    return *this;
  }

  constexpr auto operator^=(enum_flags other) noexcept -> enum_flags& {
    flags_data_ ^= other.flags_data_;
    return *this;
  }

  constexpr auto set(flag_type flag) noexcept -> enum_flags& { return (*this |= enum_flags(flag)); }

  constexpr auto reset(flag_type flag) noexcept -> enum_flags& {
    return (*this &= enum_flags(flag).flip_all());
  }

  constexpr auto flip(flag_type flag) noexcept -> enum_flags& {
    return test(flag) ? reset(flag) : set(flag);
  }

  constexpr auto set_all() noexcept -> enum_flags& { return (*this = enum_flags::all()); }

  constexpr auto reset_all() noexcept -> enum_flags& { return (*this = enum_flags::none()); }

  constexpr auto flip_all() noexcept -> enum_flags& {
    flags_data_ = (~flags_data_ & M);
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

  [[nodiscard]] static constexpr auto all() noexcept -> enum_flags { return enum_flags(M); }

  [[nodiscard]] static constexpr auto none() noexcept -> enum_flags { return enum_flags(0); }

 private:
  [[nodiscard]] constexpr explicit enum_flags(underlying_data_type flags_data) noexcept
      : flags_data_(flags_data) {}

  underlying_data_type flags_data_ = 0;
};

}  // namespace dlgr
