#include <ostream>
#include <string>
#include <unordered_map>

#include <rapidcheck.h>

#include <dot.hpp>
#include <version_vector.hpp>

#include "version_vector_utility.hpp"

using namespace crdt;

auto main() -> int {
  assert(rc::check("increment method return the dot incremented by one",
                   [](map<int> dots) {
                     auto v = build_vector(std::move(dots));
                     for (auto [actor, counter] : v.dots) {
                       auto incremanted_dot = dot{actor, counter + 1};
                       RC_ASSERT(v.inc(actor) == incremanted_dot);
                     }
                   }));

  assert(rc::check("associative",
                   [](map<std::string> dots1, map<std::string> dots2,
                      map<std::string> dots3) {
                     auto v1 = build_vector(std::move(dots1));
                     auto v2 = build_vector(std::move(dots2));
                     auto v3 = build_vector(std::move(dots3));

                     auto v1_snapshot = v1;

                     v1.merge(v2);
                     v1.merge(v3);

                     v2.merge(v3);
                     v1_snapshot.merge(v2);

                     RC_ASSERT(v1 == v1_snapshot);
                   }));

  assert(rc::check("commutative",
                   [](map<std::string> dots1, map<std::string> dots2) {
                     auto v1 = build_vector(std::move(dots1));
                     auto v2 = build_vector(std::move(dots2));

                     auto v1_snapshot(v1);

                     v1.merge(v2);

                     v2.merge(v1_snapshot);

                     RC_ASSERT(v1 == v2);
                   }));

  assert(rc::check("idempotent", [](map<std::string> dots) {
    auto v = build_vector(std::move(dots));
    auto v_snapshot = build_vector(std::move(dots));

    v.merge(v_snapshot);

    RC_ASSERT(v == v_snapshot);
  }));
}
