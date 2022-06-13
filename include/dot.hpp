#ifndef DOT_H
#define DOT_H

#include <compare>
#include <cstdint>

#include <crdt_traits.hpp>

namespace crdt {

template <actor_type A> struct dot {
  A actor;
  std::uint64_t counter;

  dot() = default;
  dot(A a, std::uint64_t counter) : actor(a), counter(counter) {}
  dot(dot &&) = default;
  dot(dot &dot) = delete;
  auto operator<=>(const dot<A> &b) const = default;

  auto clone() -> dot<A> { return ++(*this); };
};

template <actor_type A> auto operator++(const dot<A> &a) noexcept -> dot<A> {
  return dot<A>(a.actor, (a.counter + 1));
}

} // namespace crdt.

#endif // DOT_H
