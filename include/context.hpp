#ifndef CONTEXT_H
#define CONTEXT_H

#include "crdt_traits.hpp"
#include "dot.hpp"
#include "version_vector.hpp"

#include <unordered_map>
#include <utility>

template <actor_type A>
struct add_context {
    version_vector<A> vector;
    dot<A> dot;
};

template <actor_type A>
struct remove_context {
    version_vector<A> vector;
};

template <typename T, actor_type A> requires std::is_default_constructible_v<T>
struct read_context {
    version_vector<A> add_vector;
    version_vector<A> remove_vector;
    T value;

    auto derive_add_context(A a) -> add_context<A> {
        auto ret = add_vector;
        auto d = ret.incremant(a);
        ret.apply(d);
        return add_context{ std::move(ret), std::move(d) };
    }

    auto derive_remove_context() -> remove_context<A> {
        return remove_context{ remove_vector };
    }

    auto split() {
        return std::pair{
            value,
            read_context{ add_vector, remove_vector }};
    }
};

#endif // CONTEXT_H
