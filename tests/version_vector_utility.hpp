#include <unordered_map>

#include <robin_hood.h>

#include <crdt_traits.hpp>
#include <version_vector.hpp>

template <crdt::actor_type A> using map = std::unordered_map<A, std::uint64_t>;

template <crdt::actor_type A>
using robin_map = robin_hood::unordered_flat_map<A, std::uint64_t>;

template <crdt::actor_type A> auto build_vector(map<A> &&counter) {
  return crdt::version_vector<A>(robin_map<A>(counter.begin(), counter.end()));
}

namespace crdt {
template <actor_type A>
void showValue(const version_vector<A> &v, std::ostream &os) {
  os << "[ ";
  for (const auto &[key, value] : v.dots) {
    os << "{ " << key << ": " << value << "}, ";
  }
  os << "]";
}
} // namespace crdt
