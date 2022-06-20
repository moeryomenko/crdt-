#include <unordered_map>

#include <mvreg.hpp>
#include <robin_hood.h>
#include <version_vector.hpp>

using map = std::unordered_map<int, std::uint64_t>;
using reg = std::pair<map, int>;
using robin_map = robin_hood::unordered_map<int, std::uint64_t>;

crdt::mvreg<int, std::uint64_t> build_from_reg(reg &&r) {
  auto &&[m, val] = r;
  using val_type = crdt::mvreg<int, std::uint64_t>::value;
  std::vector<val_type> result;
  result.push_back(
      val_type(crdt::version_vector<int>{robin_map(m.begin(), m.end())}, val));
  return crdt::mvreg<int, std::uint64_t>{result};
}
