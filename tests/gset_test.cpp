#include <set>
#include <string>

#include <rapidcheck.h>

#include <gset.hpp>

using set = std::set<std::string>;
using crdt_set = crdt::gset<std::string>;

auto build_crdt_set(set &&s1) -> crdt_set {
  crdt_set s;
  for (auto &&e : s1)
    s.insert(e);
  return s;
}

auto main() -> int {
  assert(rc::check("associative", [](set se1, set se2, set se3) {
    auto set1 = build_crdt_set(std::move(se1));
    auto set2 = build_crdt_set(std::move(se2));
    auto set3 = build_crdt_set(std::move(se3));

    auto set1_snapshot = set1;

    set1.merge(set2);
    set1.merge(set3);

    set2.merge(set3);
    set1_snapshot.merge(set2);

    RC_ASSERT(set1 == set1_snapshot);
  }));

  assert(rc::check("commutative", [](set s1, set s2) {
    auto set1 = build_crdt_set(std::move(s1));
    auto set2 = build_crdt_set(std::move(s2));

    auto set1_snapshot = set1;

    set1.merge(set2);

    set2.merge(set1_snapshot);

    RC_ASSERT(set1 == set2);
  }));

  assert(rc::check("idempotent", [](set r) {
    auto s = build_crdt_set(std::move(r));
    auto s_snapshot = build_crdt_set(std::move(r));

    s.merge(s_snapshot);

    RC_ASSERT(s == s_snapshot);
  }));

  assert(rc::check("change delta", [](set s, std::string value) {
    auto replica1 = build_crdt_set(std::move(s));
    auto replica2 = replica1;

    auto delta = replica1.insert(value);
    replica2.merge(delta);

    RC_ASSERT(replica1 == replica2);
  }));

  assert(rc::check("changes delta",
                   [](set s, std::string value1, std::string value2) {
                     auto replica1 = build_crdt_set(std::move(s));
                     auto replica2 = replica1;

                     auto delta = replica1.insert({value1, value2});
                     replica2.merge(delta);

                     RC_ASSERT(replica1 == replica2);
                   }));
}
