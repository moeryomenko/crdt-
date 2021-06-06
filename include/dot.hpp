#include <compare>
#include <cstdint>

#include "crdt_traits.hpp"

template <actor_type A>
struct dot {
    A actor;
    std::uint64_t counter;

    dot() = default;
    dot(A a, std::uint64_t counter): actor(a), counter(counter) {}
    dot(dot&&) = default;
    dot(dot& dot): actor(dot.actor), counter(dot.counter + 1) {}
    auto operator<=>(const dot<A>& b) const = default;

    dot& operator++()
    {
        counter++;
        return (*this);
    }
};
