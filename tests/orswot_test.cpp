#include <algorithm>
#include <boost/ut.hpp>
#include <rapidcheck.h>

#include <orswot.hpp>
#include <utility>

using set = std::unordered_set<std::string>;
using replicated_set = crdt::orswot<std::string, std::string>;
void setup_set(std::string &&ctx, replicated_set &crdt_set, set s) {
  for (auto &&v : s) {
    crdt_set.apply(crdt_set.add(crdt_set.read().derive_add_context(ctx), v));
  }
};

namespace crdt {
template <actor_type M, actor_type A>
void showValue(const orswot<M, A> &v, std::ostream &os) {
  os << "{ ";
  os << "clock: [ ";
  for (const auto &[key, value] : v.clock.dots) {
    os << "{ " << key << ": " << value << " }, ";
  }
  os << " ], entries: { ";
  for (const auto &[key, value] : v.entries) {
    os << "{ " << key << ": [ ";
    for (const auto &[k, v] : value.dots) {
      os << "{ " << k << ": " << v << " }, ";
    }
    os << "] } ";
  }

  os << " }, deferred: { ";
  for (const auto &[clock, dset] : v.deferred) {
    os << "[ [ ";
    for (const auto &[actor, value] : clock.dots) {
      os << "{ " << actor << ": " << value << " }, ";
    }
    os << " ]: [ ";
    for (const auto &k : dset) {
      os << k << ", ";
    }
    os << "], ";
  }

  os << "} }";
}
} // namespace crdt

auto main() -> int {
  using namespace boost::ut;
  using namespace crdt;

  "ensure deferred merges"_test = [] {
    replicated_set a;
    replicated_set b;

    b.apply(b.add(b.read().derive_add_context("A"), "element 1"));

    // remove with a future context.
    version_vector<std::string> rm;
    rm.dots["A"] = 4;
    b.apply(b.rm(remove_context<std::string>{rm}, "element 1"));
    expect(b.deferred.size() == 1_i);

    a.apply(a.add(a.read().derive_add_context("B"), "element 4"));

    // remove with a future context.
    version_vector<std::string> rm2;
    rm2.dots["C"] = 4;
    b.apply(b.rm(remove_context<std::string>{rm2}, "element 9"));
    expect(b.deferred.size() == 2_i);

    orswot<std::string, std::string> merged;
    merged.merge(a);
    merged.merge(b);
    merged.merge(orswot<std::string, std::string>());
    expect(merged.deferred.size() == 2_i);
  };

  "preserve deferred across merges"_test = [] {
    orswot<int, std::string> a;
    orswot<int, std::string> b(a);
    orswot<int, std::string> c(a);

    a.apply(a.add(a.read().derive_add_context("A"), 5));

    version_vector<std::string> vc;
    vc.apply(dot<std::string>("A", 3));
    vc.apply(dot<std::string>("B", 8));

    b.apply(b.rm(remove_context<std::string>{vc}, 5));
    expect(b.deferred.size() == 1_i);

    c.merge(b);
    expect(c.deferred.size() == 1_i);

    a.merge(c);
    expect(a.read().value.empty());
  };

  "test present but removed"_test = [] {
    orswot<int, std::string> a;
    orswot<int, std::string> b;

    a.apply(a.add(a.read().derive_add_context("A"), 1));

    // Replicate it to 'c' so 'a' has 1->{a, 1};
    auto c(a);

    a.apply(a.rm(a.contains(1).derive_remove_context(), 1));

    b.apply(b.add(b.read().derive_add_context("B"), 1));

    // Replicate 'b' to 'a', so now 'a' has a 1
    // the one with a Dot of {b, 1} and clock
    // of [{a, 1}, {b, 1}]
    a.merge(b);

    b.apply(b.rm(b.contains(1).derive_remove_context(), 1));

    // Both 'c' and 'a' have a '1', but when they merge, there should be
    // no '1' as 'c's has been removed by 'a' and 'a's has been removed by 'c'.
    a.merge(b);
    a.merge(c);
    expect(a.read().value.empty());
  };

  "weird highlight 1"_test = [] {
    orswot<int, std::string> a;
    orswot<int, std::string> b;

    a.apply(a.add(a.read().derive_add_context("A"), 1));
    b.apply(b.add(b.read().derive_add_context("A"), 2));

    a.merge(b);
    expect(a.read().value.empty());
  };

  "adds dont destroy causality"_test = [] {
    orswot<int, std::string> a;
    orswot<int, std::string> b = a;
    orswot<int, std::string> c = a;

    c.apply(c.add(c.read().derive_add_context("A"), 1));
    c.apply(c.add(c.read().derive_add_context("B"), 1));

    auto c_elem_ctx = c.contains(1);

    // If we want to remove this entry, the remove context
    // should descend from vclock { 1->1, 2->1 }
    expect(c_elem_ctx.remove_vector ==
           version_vector<std::string>(
               robin_hood::unordered_map<std::string, std::uint64_t>(
                   {{"A", 1}, {"B", 1}})));

    a.apply(a.add(a.read().derive_add_context("C"), 1));
    b.apply(c.rm(c_elem_ctx.derive_remove_context(), 1));
    a.apply(a.add(a.read().derive_add_context("A"), 1));

    a.merge(b);
    expect(a.read().value.contains(1));
  };

  "property based tests"_test = [] {
    using rc::check;

    expect(check("associative", [](set s1, set s2, set s3) {
      replicated_set set1;
      replicated_set set2;
      replicated_set set3;
      setup_set("A", set1, s1);
      setup_set("B", set2, s2);
      setup_set("C", set3, s3);

      auto set1_snapshot(set1);

      set1.merge(set2);
      set1.merge(set3);

      set2.merge(set3);
      set1_snapshot.merge(set2);

      RC_ASSERT(set1 == set1_snapshot);
    }));

    expect(check("commutative", [](set s1, set s2) {
      replicated_set set1;
      replicated_set set2;
      setup_set("A", set1, s1);
      setup_set("B", set2, s2);

      auto set1_snapshot(set1);

      set1.merge(set2);

      set2.merge(set1_snapshot);

      RC_ASSERT(set1 == set2);
    }));

    expect(check("idempotent", [](set s) {
      replicated_set set;
      replicated_set set_snapshot;
      setup_set("A", set, s);
      setup_set("A", set_snapshot, s);

      set.merge(set_snapshot);

      RC_ASSERT(set == set_snapshot);
    }));

    expect(rc::check("add change delta", [](set r, std::string value) {
      replicated_set replica1;
      setup_set("A", replica1, r);
      auto replica2(replica1);

      auto delta = replica1.add("A", value);

      replica2.merge(delta);

      RC_ASSERT(replica1 == replica2);
    }));

    expect(rc::check("remove change delta", [](set r, std::string value) {
      replicated_set replica1;
      setup_set("A", replica1, r);
      auto replica2(replica1);

      auto delta = replica1.rm("A", value);

      replica2.merge(delta);

      RC_ASSERT(replica1 == replica2);
      RC_ASSERT(replica1.deferred.size() == replica2.deferred.size());
    }));
  };
}
