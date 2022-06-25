#ifndef GSET_H
#define GSET_H

#include <initializer_list>

#include <crdt_traits.hpp>
#include <unordered_set>

namespace crdt {

template <value_type _Key, set_type<_Key> _Set = std::unordered_set<_Key>> struct gset {
  using Op = _Key;
  _Set value;

  gset() = default;
  gset(const gset<_Key, _Set> &) = default;
  gset(gset<_Key, _Set> &&) = default;

  auto operator<=>(const gset<_Key, _Set> &) const noexcept = default;

  auto validate_merge(const gset<_Key, _Set> &) const noexcept
      -> std::optional<std::error_condition> {
    return std::nullopt;
  }

  void merge(const gset<_Key, _Set> &other) noexcept {
    value.insert(other.value.begin(), other.value.end());
  }

  auto validate_op(const Op &val) const noexcept
      -> std::optional<std::error_condition> {
    return std::nullopt;
  }

  void apply(const Op &val) noexcept { insert(val); }

  auto insert(const _Key &val) noexcept -> gset<_Key, _Set> {
    gset<_Key, _Set> res;
    value.insert(val);
    res.value.insert(val);
    return res;
  }

  auto insert(std::initializer_list<_Key> list) noexcept -> gset<_Key, _Set> {
    gset<_Key, _Set> res;
    value.insert(list);
    res.value.insert(list);
    return res;
  }

  bool contains(const _Key &val) const noexcept { return value.contains(val); }

  auto read() const noexcept -> _Set { return value; }
};

} // namespace crdt.

#endif // GSET_H
