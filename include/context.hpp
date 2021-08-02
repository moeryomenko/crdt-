#ifndef CONTEXT_H
#define CONTEXT_H

#include <unordered_map>
#include <utility>

#include <crdt_traits.hpp>
#include <dot.hpp>
#include <version_vector.hpp>

template <actor_type A> struct add_context {
    version_vector<A> vector;
    dot<A> dot;
};

template <actor_type A> struct remove_context {
    version_vector<A> vector;
};

template <typename T, actor_type A>
requires std::is_default_constructible_v<T>
struct read_context {
    version_vector<A> add_vector;
    version_vector<A> remove_vector;
    T value;

    read_context() = default;
    read_context(const read_context<T, A>&) = default;
    read_context(read_context<T, A>&&) = default;
    read_context(const version_vector<A>& add, const version_vector<A>& rm, const T& val)
        : add_vector(add)
        , remove_vector(rm)
        , value(val)
    {
    }

    auto derive_add_context(A a) -> add_context<A>
    {
        auto ret = add_vector;
        auto d = ret.inc(a);
        ret.apply(d);
        return add_context<A> { std::move(ret), std::move(d) };
    }

    auto derive_remove_context() -> remove_context<A> { return remove_context<A> { remove_vector }; }

    auto split() { return std::pair { value, read_context { add_vector, remove_vector } }; }
};

#endif // CONTEXT_H
