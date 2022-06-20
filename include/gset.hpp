#ifndef GSET_H
#define GSET_H

#include <initializer_list>
#include <robin_hood.h>

#include <crdt_traits.hpp>

namespace crdt {

template <value_type T> struct gset {
  using Op = T;
  robin_hood::unordered_flat_set<T> value;

  gset() = default;
  gset(const gset<T> &) = default;
  gset(gset<T> &&) = default;

  auto operator<=>(const gset<T> &) const noexcept = default;

  auto validate_merge(const gset<T> &) const noexcept
      -> std::optional<std::error_condition> {
    return std::nullopt;
  }

  void merge(const gset<T> &other) noexcept {
    value.insert(other.value.begin(), other.value.end());
  }

  auto validate_op(const Op &val) const noexcept
      -> std::optional<std::error_condition> {
    return std::nullopt;
  }

  void apply(const Op &val) noexcept { insert(val); }

  auto insert(const T &val) noexcept -> gset<T> {
    gset<T> res;
    value.insert(val);
    res.value.insert(val);
    return res;
  }

  auto insert(std::initializer_list<T> list) noexcept -> gset<T> {
    gset<T> res;
    value.insert(list);
    res.value.insert(list);
    return res;
  }

  bool contains(const T &val) const noexcept { return value.contains(val); }

  auto read() const noexcept -> robin_hood::unordered_flat_set<T> { return value; }
};

} // namespace crdt.

#endif // GSET_H
