#ifndef PNCOUNTER_H
#define PNCOUNTER_H

#include <optional>
#include <utility>

#include <crdt_traits.hpp>
#include <dot.hpp>
#include <gcounter.hpp>

namespace crdt {

/// The Direction of an Op.
enum class Dir {
  pos, /// signals that the op increments the counter.
  neg, /// signals that the op decrements the counter.
};

template <actor_type A> struct pncounter {
  gcounter<A> p, n;

  pncounter() = default;
  pncounter(const pncounter<A> &) = default;
  pncounter(pncounter<A> &&) = default;

  auto operator<=>(const pncounter<A> &) const noexcept = default;

  struct op {
    dot<A> Op;
    Dir dir;

    op() = default;
    op(const op &) = default;
    op(op &&) = default;
    op(dot<A> &&d, Dir &&dir) : Op(std::move(d)), dir(std::move(dir)) {}
  };

  auto validate_op(op Op) noexcept -> std::optional<std::error_condition> {
    return get_direction(Op.dir).validate_op(Op.Op);
  }

  void apply(const op &Op) noexcept { get_direction(Op.dir).apply(Op.Op); }

  auto validate_merge(const pncounter<A> &other) noexcept
      -> std::optional<std::error_condition> {
    auto op = p.validate_merge(other.p);
    if (op == std::nullopt)
      return n.validate_merge(other.n);
    return op;
  }

  void merge(const pncounter<A> &other) noexcept {
    p.merge(other.p);
    n.merge(other.n);
  }

  void reset_remove(const version_vector<A> &v) {
    p.reset_remove(v);
    n.reset_remove(v);
  }

  auto inc(const A &a, std::uint32_t steps = 1) noexcept -> pncounter<A> {
	  pncounter<A> res;
	  res.p = p.inc(a, steps);
	  return res;
  }

  auto operator+(const A &a) const noexcept -> op {
    return ((*this) + std::pair{a, 1});
  }

  auto operator+(std::pair<A, std::uint32_t> &&a) const noexcept -> op {
    return op(p.operator+(std::move(a)), Dir::pos);
  }

  auto dec(const A &a, std::uint32_t steps = 1) noexcept -> pncounter<A> {
	  pncounter<A> res;
	  res.n = n.inc(a, steps);
	  return res;
  }

  auto operator-(const A &a) const noexcept -> op {
	  return ((*this) - std::pair{a, 1});
  }

  auto operator-(std::pair<A, std::uint32_t> &&a) const noexcept -> op {
    return op(n.operator+(std::move(a)), Dir::neg);
  }

  auto read() -> std::uint32_t { return p.read() - n.read(); }

private:
  gcounter<A> &get_direction(Dir dir) noexcept {
    return dir == Dir::pos ? p : n;
  }
};

} // namespace crdt.

#endif // PNCOUNTER_H
