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
    bool operator==(const dot<A>& d) const
    {
        return actor == d.actor && counter == d.counter;
    }

    bool operator<(const dot<A>& d) const
    {
        if (actor == d.actor) {
            return counter < d.counter;
        } else {
            return actor < d.actor;
        }
    }

    bool operator>(const dot<A>& d) const
    {
        return d < (*this);
    }

    dot& operator++()
    {
        counter++;
        return (*this);
    }
};

template<actor_type A>
bool operator==(const dot<A>& d1, const dot<A>& d2) {
    return d1.actor == d2.actor && d1.counter == d2.counter;
}

template<actor_type A>
bool operator<(const dot<A>& a, const dot<A>& b) {
    if (a.actor == b.actor) {
        return a.couter < b.counter;
    } else {
        return a.actor < b.actor;
    }
}
