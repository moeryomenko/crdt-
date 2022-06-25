#include <unordered_map>

#include <mvreg.hpp>
#include <version_vector.hpp>

using map = std::unordered_map<int, std::uint64_t>;
using reg = std::pair<map, int>;

crdt::mvreg<int, std::uint64_t> build_from_reg(reg &&r) {
  auto &&[m, val] = r;
  using val_type = crdt::mvreg<int, std::uint64_t>::value;

  crdt::version_vector<int, std::unordered_map<int, std::uint64_t>> clock;
  clock.dots = m;

  val_type v;
  v.val = val;
  v.vclock = clock;

  std::vector<val_type> result;
  result.push_back(v);
  return crdt::mvreg<int, std::uint64_t>{result};
}
