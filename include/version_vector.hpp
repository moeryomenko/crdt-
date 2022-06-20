#ifndef VERSION_VECTOR_H
#define VERSION_VECTOR_H

#include <algorithm>
#include <compare>
#include <cstdint>
#include <iterator>
#include <numeric>
#include <robin_hood.h>
#include <utility>

#include <crdt_traits.hpp>
#include <dot.hpp>

namespace crdt {

template <actor_type A> struct version_vector {
  using dots_map = robin_hood::unordered_flat_map<A, std::uint64_t>;
  using Op = dot<A>;
  dots_map dots;

  version_vector() = default;
  version_vector(const version_vector<A> &) = default;
  version_vector(version_vector<A> &&) = default;
  version_vector(robin_hood::unordered_map<A, uint64_t> &&v) noexcept
      : dots(v) {}

  version_vector<A> &operator=(const version_vector<A> &) = default;
  version_vector<A> &operator=(version_vector<A> &&) = default;
  version_vector<A> &operator=(version_vector<A> &) = default;

  auto operator<=>(const version_vector<A> &other) const noexcept {
    auto all_gt = [](const version_vector<A> &left,
                     const version_vector<A> &right) -> bool {
      return std::all_of(
          right.dots.begin(), right.dots.end(),
          [&left](const auto &d) { return left.get(d.first) >= d.second; });
    };

    if (this->dots.size() == other.dots.size() &&
        std::equal(this->dots.begin(), this->dots.end(), other.dots.begin())) {
      return std::partial_ordering::equivalent;
    } else if (all_gt(*this, other)) {
      return std::partial_ordering::greater;
    } else if (all_gt(other, *this)) {
      return std::partial_ordering::less;
    }

    return std::partial_ordering::unordered;
  }

  bool operator==(const version_vector<A> &) const noexcept = default;

  void reset_remove(const version_vector<A> &other) noexcept {
    for (auto [actor, counter] : other.dots) {
      if (auto dot = dots.find(actor);
          dot != dots.end() && counter >= dot->second) {
        dots.erase(dot);
      }
    }
  }

  bool empty() const noexcept { return dots.empty(); }

  auto get(const A &a) const noexcept -> std::uint64_t {
    auto it = dots.find(a);
    if (it == dots.end())
      return 0;
    return it->second;
  }

  auto get_dot(const A &a) const noexcept -> dot<A> {
    if (auto it = dots.find(a); it != dots.end())
      return dot(it->first, it->second);
    return dot(a, 0);
  }

  auto inc(const A &a) const noexcept -> dot<A> { return ++get_dot(a); }

  auto validate_op(const Op &op) const noexcept
      -> std::optional<std::error_condition> {
    auto next_counter = dots.find(op.actor)->second + 1;
    if (op.counter > next_counter) {
      return std::make_error_condition(std::errc::invalid_argument);
    }

    return std::nullopt;
  }

  void apply(const Op &op) noexcept {
    if (auto counter = get(op.actor); counter <= op.counter) {
      dots[op.actor] = op.counter;
    }
  }

  auto validate_merge(const version_vector<A> &T) const noexcept
      -> std::optional<std::error_condition> {
    return std::nullopt;
  }

  void merge(const version_vector<A> &other) noexcept {
    for (const auto &[actor, counter] : other.dots)
      apply(dot{actor, counter});
  }

  auto clone_without(version_vector<A> base_clock) const noexcept
      -> version_vector<A> {
    version_vector<A> cloned(*this);
    cloned.reset_remove(base_clock);
    return cloned;
  }
};

template <actor_type A>
auto intersection(const version_vector<A> &left,
                  const version_vector<A> &right) noexcept
    -> version_vector<A> {
  robin_hood::unordered_map<A, std::uint64_t> dots;
  std::set_intersection(left.dots.begin(), left.dots.end(), right.dots.begin(),
                        right.dots.end(), std::inserter(dots, dots.begin()));
  return version_vector(std::move(dots));
}

} // namespace crdt.

namespace robin_hood {

using namespace crdt;

template <actor_type A> struct hash<version_vector<A>> {
  size_t operator()(const version_vector<A> &k) const {
    robin_hood::hash<A> elem_hash;
    return std::accumulate(k.dots.begin(), k.dots.end(), 0,
                           [&elem_hash](size_t acc, const auto &elem) {
                             return acc ^ elem_hash(elem.first);
                           });
  }
};

} // namespace robin_hood

#endif // VERSION_VECTOR_H
