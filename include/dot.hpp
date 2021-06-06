#include <compare>
#include <concepts>
#include <cstdint>
#include <functional>
#include <type_traits>

template <typename F, typename T>
concept Hashable = std::regular_invocable<F, T> && std::convertible_to<std::invoke_result_t<F, T>, size_t>;

template <typename T, typename F = std::hash<T>>
concept actor_type = std::equality_comparable<T> && std::copyable<T> && Hashable<F, T>;

template <actor_type A>
struct dot {
    A actor;
    std::uint64_t counter;

    dot() = default;
    dot(A a, std::uint64_t counter)
        : actor(a)
        , counter(counter)
    {
    }
    dot(dot&&) = default;
    dot(dot& dot)
    {
        actor = dot.actor;
        counter = dot.counter + 1;
    }

    auto operator<=>(const dot<A>& b) const = default;

    dot& operator++()
    {
        counter++;
        return (*this);
    }
};
