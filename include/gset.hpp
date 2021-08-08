#ifndef GSET_H
#define GSET_H

#include <robin_hood.h>

#include <crdt_traits.hpp>

namespace crdt {

template <value_type T> struct gset {
    robin_hood::unordered_flat_set<T> value;

    gset() = default;
    gset(const gset<T>&) = default;
    gset(gset<T>&&) = default;

    auto validate_merge(const gset<T>&) noexcept -> std::optional<std::error_condition> { return std::nullopt; }

    void merge(const gset<T>& other) noexcept { value.merge(other.value); }

    auto validate_op(const T& val) noexcept -> std::optional<std::error_condition> { return std::nullopt; }

    void apply(const T& val) noexcept { insert(val); }

    void insert(const T& val) noexcept { value.insert(val); }

    bool contains(const T& val) noexcept { return value.contains(val); }

    auto read() -> robin_hood::unordered_flat_set<T> { return value; }
};

} // namespace crdt.

#endif // GSET_H
