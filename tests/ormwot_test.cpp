#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <vector>

#include <boost/ut.hpp>
#include <rapidcheck.h>

#include <crdt_traits.hpp>
#include <mvreg.hpp>
#include <ormwot.hpp>
#include <orswot.hpp>

#include "utility.hpp"

using val_type = crdt::mvreg<int, std::uint64_t>;
using entry_type = std::pair<int, std::uint64_t>;
using entries_type = std::vector<entry_type>;
using replicated_map = crdt::ormwot<int, val_type>;

void setup_map(int actor, replicated_map &m, entries_type entries) {
  for (auto entry : entries) {
    m.apply(m.update(m.read_ctx().derive_add_context(actor), entry.first,
                     [val = entry.second](const auto &ctx, auto &v) {
                       return v.write(ctx, val);
                     }));
  }
}

namespace crdt {

template <actor_type K, crdt V>
void showValue(const ormwot<K, V> &v, std::ostream &os) {
  os << "{ ";
  os << "clock: { ";
  for (const auto &[key, value] : v.clock.dots) {
    os << "{ " << key << ": " << value << " }, ";
  }
  os << " }, entries: { ";
  for (const auto &[key, value] : v.entries) {
    os << "{ " << key << ": { ";
    for (const auto &v : value.val.vals) {
      os << v.val << ": { ";
	  for (const auto &[actor, counter]: v.vclock.dots) {
		  os << " { " << actor << ": " << counter << " }, ";
	  }
	  os << " }, ";
    }
    os << "} } ";
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

  "property based tests"_test = [] {
    using rc::check;

    expect(check("associative",
                 [](entries_type e1, entries_type e2, entries_type e3) {
                   replicated_map m1;
                   replicated_map m2;
                   replicated_map m3;
                   setup_map(1, m1, e1);
                   setup_map(2, m2, e2);
                   setup_map(3, m3, e3);

                   auto m1_snapshot(m1);

                   m1.merge(m2);
                   m1.merge(m3);

                   m2.merge(m3);
                   m1_snapshot.merge(m1);

                   RC_ASSERT(m1 == m1_snapshot);
                 }));

    expect(check("commutative", [](entries_type e1, entries_type e2) {
      replicated_map m1;
      replicated_map m2;
      setup_map(1, m1, e1);
      setup_map(2, m2, e2);

      auto m1_snapshot(m1);

      m1.merge(m2);

      m2.merge(m1_snapshot);

      RC_ASSERT(m1 == m2);
    }));

    expect(check("idempotent", [](entries_type e) {
      replicated_map m;
      setup_map(1, m, e);
      replicated_map m_snapshot(m);

      m.merge(m_snapshot);

      RC_ASSERT(m == m_snapshot);
    }));
  };
}
