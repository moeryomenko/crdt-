#ifndef VERSION_VECTOR_H
#define VERSION_VECTOR_H

#include "crdt_traits.hpp"
#include "dot.hpp"

#include <compare>
#include <cstdint>
#include <map>
#include <utility>

template<actor_type A>
struct version_vector {
    std::map<A, std::uint32_t> dots;

    version_vector(const version_vector<A>&) = default;
    version_vector(version_vector<A>&&) = default;

    version_vector(std::map<A, uint32_t>&& v) noexcept : dots(v) {}

    auto operator<=>(const version_vector<A>& other) const = default;

    void reset_remove(const version_vector<A>& other) {
        for (auto [actor, counter] : other.dots) {
            if (auto dot = dots.find(actor);
                    dot != dots.end() && counter >= dot->second) {
                dots.erase(dot);
            }
        }
    }

    auto get(const A& a) -> dot<A>&& {
        auto [actor, counter] = *(dots.find(a));
        return std::move(dot(actor, counter));
    }

    auto increment(const A& a) -> dot<A>&& {
        return std::move(++get(a));
    }

    auto validate_op(const dot<A>& Op) -> std::optional<std::error_condition> {
        auto next_counter = dots.find(Op.actor)->second + 1;
        if (Op.counter > next_counter) {
            return std::make_error_condition(std::errc::invalid_argument);
        }

        return std::nullopt;
    }

    void apply(const dot<A>& Op) {
        if(auto counter = dots.find(&Op.actor)->second;
                counter < Op.counter) {
            dots.insert(Op.actor, Op.counter);
        }
    }

    auto validate_merge(const version_vector<A>& T)
        -> std::optional<std::error_condition> { return std::nullopt; }

    void merge(const version_vector<A>& other) {
        for(dot<A> d : other.dots) {
            dots.apply(d);
        }
    }
};

#endif  // VERSION_VECTOR_H
