#ifndef PNCOUNTER_H
#define PNCOUNTER_H

#include <optional>

#include <crdt_traits.hpp>
#include <dot.hpp>
#include <gcounter.hpp>

/// The Direction of an Op.
enum class Dir {
    pos, /// signals that the op increments the counter.
    neg, /// signals that the op decrements the counter.
};

template <actor_type A> struct op {
    dot<A> Op;
    Dir dir;

    op() = default;
    op(const op<A>&) = default;
    op(op<A>&&) = default;

    op(dot<A>&& d, Dir&& dir) : Op(std::move(d)), dir(std::move(dir)) {}
};

template <actor_type A> struct pncounter {
    gcounter<A> p, n;

    pncounter() = default;
    pncounter(const pncounter<A>&) = default;
    pncounter(pncounter<A>&&) = default;

    auto operator<=>(const pncounter<A>&) const noexcept = default;

    auto validate_op(op<A> Op) noexcept -> std::optional<std::error_condition> {
        return get_direction(Op.dir).validate_op(Op.Op);
    }

    void apply(const op<A>& Op) noexcept {
        get_direction(Op.dir).apply(Op.Op);
    }

    auto validate_merge(const pncounter<A>& other) noexcept -> std::optional<std::error_condition> {
        auto op = p.validate_merge(other.p);
        if (op == std::nullopt) return n.validate_merge(other.n);
        return op;
    }

    void merge(const pncounter<A>& other) noexcept { p.merge(other.p); n.merge(other.n); }

    void reset_remove(const version_vector<A>& v) { p.reset_remove(v); n.reset_remove(v); }

    auto inc(const A& a) const noexcept -> op<A> { return op<A>(p.inc(a), Dir::pos); }

    auto dec(const A& a) const noexcept -> op<A> { return op<A>(n.inc(a), Dir::neg); }

    auto inc(const A& a, std::uint32_t steps) const noexcept -> op<A> { return op<A>(p.inc(a, steps), Dir::pos); }

    auto dec(const A& a, std::uint32_t steps) const noexcept -> op<A> { return op<A>(n.inc(a, steps), Dir::neg); }

    auto read() -> std::uint32_t { return p.read() - n.read(); }

private:
    gcounter<A>& get_direction(Dir dir) noexcept {
        return dir == Dir::pos ? p : n;
    }
};

#endif // PNCOUNTER_H
