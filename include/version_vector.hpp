#ifndef VERSION_VECTOR_H
#define VERSION_VECTOR_H

#include <algorithm>
#include <compare>
#include <cstdint>
#include <iterator>
#include <numeric>
#include <unordered_map>
#include <utility>

#include <crdt_traits.hpp>
#include <dot.hpp>

namespace crdt {

template <actor_type _Actor,
          iterable_assiative_type<_Actor, std::uint64_t> _Map>
struct version_vector {
  using actor_t = _Actor;
  using dots_map = _Map;
  using Op = dot<_Actor>;
  dots_map dots;

  version_vector() = default;
  version_vector(const version_vector<_Actor, _Map> &) = default;
  version_vector(version_vector<_Actor, _Map> &&) = default;

  version_vector<_Actor, _Map> &
  operator=(const version_vector<_Actor, _Map> &) = default;
  version_vector<_Actor, _Map> &
  operator=(version_vector<_Actor, _Map> &&) = default;
  version_vector<_Actor, _Map> &
  operator=(version_vector<_Actor, _Map> &) = default;

  auto operator<=>(const version_vector<_Actor, _Map> &other) const noexcept {
    auto all_gt = [](const version_vector<_Actor, _Map> &left,
                     const version_vector<_Actor, _Map> &right) -> bool {
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

  bool
  operator==(const version_vector<_Actor, _Map> &) const noexcept = default;

  void reset_remove(const version_vector<_Actor, _Map> &other) noexcept {
    for (auto [actor, counter] : other.dots) {
      if (auto dot = dots.find(actor);
          dot != dots.end() && counter >= dot->second) {
        dots.erase(dot);
      }
    }
  }

  bool empty() const noexcept { return dots.empty(); }

  auto get(const _Actor &a) const noexcept -> std::uint64_t {
    auto it = dots.find(a);
    if (it == dots.end())
      return 0;
    return it->second;
  }

  auto get_dot(const _Actor &a) const noexcept -> dot<_Actor> {
    if (auto it = dots.find(a); it != dots.end())
      return dot(it->first, it->second);
    return dot(a, 0);
  }

  auto inc(const _Actor &a) const noexcept -> dot<_Actor> {
    return ++get_dot(a);
  }

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

  auto validate_merge(const version_vector<_Actor, _Map> &) const noexcept
      -> std::optional<std::error_condition> {
    return std::nullopt;
  }

  void merge(const version_vector<_Actor, _Map> &other) noexcept {
    for (const auto &[actor, counter] : other.dots)
      apply(dot{actor, counter});
  }

  auto clone_without(version_vector<_Actor, _Map> base_clock) const noexcept
      -> version_vector<_Actor, _Map> {
    version_vector<_Actor, _Map> cloned(*this);
    cloned.reset_remove(base_clock);
    return cloned;
  }
};

template <actor_type A, iterable_assiative_type<A, std::uint64_t> T>
auto intersection(const version_vector<A, T> &left,
                  const version_vector<A, T> &right) noexcept
    -> version_vector<A, T> {
  version_vector<A, T> res;
  std::set_intersection(left.dots.begin(), left.dots.end(), right.dots.begin(),
                        right.dots.end(), std::inserter(res.dots, res.dots.begin()));
  return res;
}

} // namespace crdt.

namespace std {

using namespace crdt;

template <actor_type A, iterable_assiative_type<A, std::uint64_t> T>
struct hash<version_vector<A, T>> {
  size_t operator()(const version_vector<A, T> &k) const {
    std::hash<A> elem_hash;
    return std::accumulate(k.dots.begin(), k.dots.end(), 0,
                           [&elem_hash](size_t acc, const auto &elem) {
                             return acc ^ elem_hash(elem.first);
                           });
  }
};

} // namespace std

#endif // VERSION_VECTOR_H
