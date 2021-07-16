#ifndef VERSION_VECTOR_H
#define VERSION_VECTOR_H

#include <compare>
#include <cstdint>
#include <numeric>
#include <robin_hood.h>
#include <utility>

#include <crdt_traits.hpp>
#include <dot.hpp>

template<actor_type A>
struct version_vector {
    robin_hood::unordered_flat_map<A, std::uint32_t> dots;

    version_vector() = default;
    version_vector(const version_vector<A>&) = default;
    version_vector(version_vector<A>&&) = default;

    version_vector(robin_hood::unordered_map<A, uint32_t>&& v) noexcept : dots(v) {}

    auto operator<=>(const version_vector<A>&) const noexcept = default;

    void reset_remove(const version_vector<A>& other) {
        for (auto [actor, counter] : other.dots) {
            if (auto dot = dots.find(actor);
                    dot != dots.end() && counter >= dot->second) {
                dots.erase(dot);
            }
        }
    }

    auto get(const A& a) const noexcept -> std::uint32_t {
        auto it = dots.find(a);
        if (it == dots.end()) return 0;
        return it->second;
    }

    auto get_dot(const A& a) const noexcept -> dot<A> {
        auto it = dots.find(a);
        if (it == dots.end()) return dot(a, 0);
        return dot(it->first, it->second);
    }

    auto inc(const A& a) const noexcept -> dot<A> {
        return ++get_dot(a);
    }

    auto validate_op(const dot<A>& Op) noexcept -> std::optional<std::error_condition> {
        auto next_counter = dots.find(Op.actor)->second + 1;
        if (Op.counter > next_counter) {
            return std::make_error_condition(std::errc::invalid_argument);
        }

        return std::nullopt;
    }

    void apply(const dot<A>& Op) noexcept {
        if(auto counter = get(Op.actor);
                counter < Op.counter) {
            dots[Op.actor] = Op.counter;
        }
    }

    auto validate_merge(const version_vector<A>& T) noexcept
        -> std::optional<std::error_condition> { return std::nullopt; }

    void merge(const version_vector<A>& other) noexcept {
        for(dot<A> d : other.dots) {
            dots.apply(d);
        }
    }
};

#endif  // VERSION_VECTOR_H
