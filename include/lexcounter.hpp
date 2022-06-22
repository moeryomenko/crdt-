#ifndef LEXCOUNTER_H
#define LEXCOUNTER_H

#include <numeric>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <utility>

#include <robin_hood.h>

#include <crdt_traits.hpp>

namespace crdt {

template <actor_type A, arithmetic_value T = std::uint64_t> struct lexcounter {
  std::unordered_map<A, std::pair<T, std::uint64_t>> dots;
  A _actor;

  static constexpr T zero = T{};
  static constexpr T one = zero + 1;

  using actor_t = A;
  using Op = std::tuple<A, T, std::uint64_t>;

  lexcounter() = default;
  lexcounter(A actor) : _actor(actor) { dots[actor] = {zero, 0}; }
  lexcounter(A actor, const lexcounter<A, T> &other)
      : _actor(actor), dots(other.dots) {
    dots[actor] = {zero, 0};
  };
  lexcounter(lexcounter<A, T> &&) = default;

  auto operator<=>(const lexcounter<A, T> &) const noexcept = default;

  auto validate_op(Op op) const noexcept
      -> std::optional<std::error_condition> {
    auto [actor, _, clock] = op;
    if (auto it = dots.find(actor);
        it != dots.end() && it->second.second >= clock)
      return std::make_error_condition(std::errc::state_not_recoverable);
    return std::nullopt;
  }

  void apply(const Op &op) noexcept {
    auto [actor, value, clock] = op;
    if (auto it = dots.find(actor); it == dots.end()) {
      dots[actor] = {value, clock};
    } else {
      if (it->second.second <= clock) {
        it->second = {value, clock};
      }
    }
  }

  auto validate_merge(const lexcounter<A> &other) const noexcept
      -> std::optional<std::error_condition> {
    return std::nullopt;
  }

  void merge(const lexcounter<A> &other) noexcept {
    for (const auto &[other_actor, dot] : other.dots) {
      if (other_actor == _actor)
        continue;

      apply({other_actor, dot.first, dot.second});
    }
  }

  void reset_remove(const version_vector<A> &v) {
    for (const auto &[actor, counter] : v.dots) {
      if (auto it = dots.find(actor);
          it != dots.end() && it->second.second <= counter) {
        dots.erase(it);
      }
    }
  }

  auto inc_op(T tosum = one) const noexcept -> Op {
	  auto it = dots.find(_actor);
	  return {_actor, it->second.first + tosum, it->second.second + 1};
  }

  auto dec_op(T tosum = one) const noexcept -> Op {
	  auto it = dots.find(_actor);
	  return {_actor, it->second.first - tosum, it->second.second + 1};
  }

  auto inc(T tosum = one) noexcept -> lexcounter<A> {
    lexcounter<A, T> delta;
    if (auto it = dots.find(_actor); it != dots.end()) {
      it->second.first += tosum;
      delta.apply({_actor, it->second.first, it->second.second});
    }
    return delta;
  }

  auto operator++() noexcept -> lexcounter<A> { return inc(); }

  auto dec(T tosum = one) noexcept -> lexcounter<A> {
    return inc(zero - tosum);
  }

  auto operator--() noexcept -> lexcounter<A> { return dec(); }

  auto read() const noexcept -> std::uint64_t {
    return std::accumulate(
        dots.begin(), dots.end(), 0,
        [](std::uint64_t sum, const auto &v) { return sum + v.second.first; });
  }
};

} // namespace crdt.

#endif // LEXCOUNTER_H
