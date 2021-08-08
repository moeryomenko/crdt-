#ifndef CRDT_TRAITS_H
#define CRDT_TRAITS_H

#include <concepts>
#include <functional>
#include <type_traits>
#include <optional>
#include <system_error>

namespace crdt {

template <typename T>
concept value_type = std::default_initializable<T> && std::equality_comparable<T>;

template <typename F, typename T>
concept hashable = std::regular_invocable<F, T> && std::convertible_to<std::invoke_result_t<F, T>, size_t>;

template <typename T, typename F = std::hash<T>>
concept actor_type = value_type<T> && std::copyable<T> && hashable<F, T>;

template <typename T>
concept cvrdt = requires(T a, T b) {
    { a.validate_merge(b) } -> std::convertible_to<std::optional<std::error_condition>>;
    { a.merge(b) } -> std::convertible_to<void>;
};

template <typename T, typename Op>
concept cmrdt = requires(T a, Op op) {
    { a.validate_op(op) } -> std::convertible_to<std::optional<std::error_condition>>;
    { a.apply(op) } -> std::convertible_to<void>;
};

template<actor_type A> struct version_vector;

template <typename T, typename V, typename A>
concept reset_removable = std::is_same_v<V, version_vector<A>> && requires(T t, V v) {
    { t.reset_remove(v) } -> std::convertible_to<void>;
};

} // namespace crdt.

#endif // CRDT_TRAITS_H
