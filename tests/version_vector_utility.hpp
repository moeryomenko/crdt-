#include <iostream>
#include <unordered_map>

#include <crdt_traits.hpp>
#include <version_vector.hpp>

template <crdt::actor_type A> using map = std::unordered_map<A, std::uint64_t>;

template <crdt::actor_type A> auto build_vector(map<A> &&counter) {
  crdt::version_vector<A> vector;
  vector.dots = counter;
  return vector;
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
