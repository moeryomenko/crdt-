#ifndef VERSION_VECTOR_H
#define VERSION_VECTOR_H

#include <algorithm>
#include <compare>
#include <cstdint>
#include <numeric>
#include <robin_hood.h>
#include <utility>

#include <crdt_traits.hpp>
#include <dot.hpp>

namespace crdt {

template <actor_type A> struct version_vector {
    using dots_map = robin_hood::unordered_flat_map<A, std::uint64_t>;
    dots_map dots;

    version_vector() = default;
    version_vector(const version_vector<A>&) = default;
    version_vector(version_vector<A>&&) = default;
    version_vector(robin_hood::unordered_map<A, uint64_t>&& v) noexcept
        : dots(v)
    {
    }

    version_vector<A>& operator=(const version_vector<A>&) = default;
    version_vector<A>& operator=(version_vector<A>&&) = default;

    auto operator<=>(const version_vector<A>& other) const noexcept
    {
        auto cmp_dots = [](const dots_map& a, const dots_map& b) -> bool {
            return std::ranges::all_of(b, [&a](const auto& d) {
                const auto [actor, counter] = d;
                if (const auto& r = a.find(actor); r != a.end() && r->second >= counter)
                    return true;
                return false;
            });
        };
        if (cmp_dots(dots, other.dots))
            return std::partial_ordering::greater;
        if (cmp_dots(other.dots, dots))
            return std::partial_ordering::less;
        return std::partial_ordering::unordered;
    }

    bool operator==(const version_vector<A>&) const noexcept = default;

    void reset_remove(const version_vector<A>& other)
    {
        for (auto [actor, counter] : other.dots) {
            if (auto dot = dots.find(actor); dot != dots.end() && counter >= dot->second) {
                dots.erase(dot);
            }
        }
    }

    bool empty() const { return dots.empty(); }

    auto get(const A& a) const noexcept -> std::uint64_t
    {
        auto it = dots.find(a);
        if (it == dots.end())
            return 0;
        return it->second;
    }

    auto get_dot(const A& a) const noexcept -> dot<A>
    {
        if (auto it = dots.find(a); it != dots.end())
            return dot(it->first, it->second);
        return dot(a, 0);
    }

    auto inc(const A& a) const noexcept -> dot<A> { return ++get_dot(a); }

    auto validate_op(const dot<A>& Op) noexcept -> std::optional<std::error_condition>
    {
        auto next_counter = dots.find(Op.actor)->second + 1;
        if (Op.counter > next_counter) {
            return std::make_error_condition(std::errc::invalid_argument);
        }

        return std::nullopt;
    }

    void apply(const dot<A>& Op) noexcept
    {
        if (auto counter = get(Op.actor); counter <= Op.counter) {
            dots[Op.actor] = Op.counter;
        }
    }

    auto validate_merge(const version_vector<A>& T) noexcept -> std::optional<std::error_condition>
    {
        return std::nullopt;
    }

    void merge(const version_vector<A>& other) noexcept
    {
        for (const auto& [actor, counter] : other.dots)
            apply(dot { actor, counter });
    }
};

} // namespace crdt.

#endif // VERSION_VECTOR_H
