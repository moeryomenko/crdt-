#include <numeric>
#include <optional>
#include <system_error>

#include "crdt_traits.hpp"
#include "dot.hpp"
#include "version_vector.hpp"

template <actor_type A> struct gcounter {
    version_vector<A> inner;

    gcounter() = default;
    gcounter(version_vector<A>&& v) noexcept : inner(std::move(v)) {}
    gcounter(const gcounter&) = default;
    gcounter(gcounter&&) = default;

    auto operator<=>(const gcounter<A>&) const noexcept = default;

    auto validate_op(const dot<A>& Op) noexcept -> std::optional<std::error_condition> { return std::nullopt; }

    void apply(const dot<A>& Op) noexcept { inner.apply(Op); }

    auto validate_merge(const version_vector<A>&) noexcept
        -> std::optional<std::error_condition> { return std::nullopt; }

    void merge(const version_vector<A>& v) noexcept { inner.merge(v); }

    void reset_remove(const version_vector<A>& v) { inner.reset_remove(v); }

    auto inc(const A& a) const noexcept -> dot<A> { return inner.inc(a); }

    auto inc(const A& a, std::uint32_t steps) const noexcept -> dot<A> {
        steps += inner.get(a);
        return dot(a, steps);
    }

    auto read() -> std::uint32_t {
        return std::accumulate(inner.dots.begin(), inner.dots.end(), 0,
                [] (std::uint32_t sum, auto n) { return sum + n.second; });
    }
};
