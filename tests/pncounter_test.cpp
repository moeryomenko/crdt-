#include <array>
#include <boost/ut.hpp>
#include <rapidcheck.h>

#include <dot.hpp>
#include <pncounter.hpp>

#include "version_vector_utility.hpp"

auto main() -> int {
  using namespace boost::ut;
  using namespace crdt;

  "basic by one"_test = [] {
    pncounter<std::string> a;
    expect(a.read() == 0);

    a.apply(a + "A");
    expect(a.read() == 1);

    a.apply(a + "A");
    expect(a.read() == 2);

    a.apply(a - "A");
    expect(a.read() == 1);

    a.apply(a + "A");
    expect(a.read() == 2);
  };

  "basic by many"_test = [] {
    pncounter<std::string> a;
    expect(a.read() == 0);

    std::uint32_t steps = 3;

    a.apply(a + std::pair{"A", steps});
    expect(a.read() == steps);

    a.apply(a + std::pair{"A", steps});
    expect(a.read() == (2 * steps));

    a.apply(a - std::pair{"A", steps});
    expect(a.read() == steps);

    a.apply(a + "A");
    expect(a.read() == (steps + 1));
  };

  assert(rc::check("associative", [](std::array<map<int>, 6> dots) {
    pncounter<int> pnc1;
    pnc1.p = gcounter(build_vector(std::move(dots[0])));
    pnc1.n = gcounter(build_vector(std::move(dots[1])));
    pncounter<int> pnc2;
    pnc2.p = gcounter(build_vector(std::move(dots[2])));
    pnc2.n = gcounter(build_vector(std::move(dots[3])));
    pncounter<int> pnc3;
    pnc3.p = gcounter(build_vector(std::move(dots[4])));
    pnc3.n = gcounter(build_vector(std::move(dots[5])));

    auto pnc1_snapshot = pnc1;

    pnc1.merge(pnc2);
    pnc1.merge(pnc3);

    pnc2.merge(pnc3);
    pnc1_snapshot.merge(pnc2);

    RC_ASSERT(pnc1 == pnc1_snapshot);
  }));

  assert(rc::check("commutative", [](std::array<map<int>, 4> dots) {
    pncounter<int> pnc1;
    pnc1.p = gcounter(build_vector(std::move(dots[0])));
    pnc1.n = gcounter(build_vector(std::move(dots[1])));
    pncounter<int> pnc2;
    pnc2.p = gcounter(build_vector(std::move(dots[2])));
    pnc2.n = gcounter(build_vector(std::move(dots[3])));

    auto v1_snapshot(pnc1);

    pnc1.merge(pnc2);

    pnc2.merge(v1_snapshot);

    RC_ASSERT(pnc1 == pnc2);
  }));

  assert(rc::check("idempotent", [](std::array<map<int>, 2> dots) {
    pncounter<int> counter;
    counter.p = gcounter(build_vector(std::move(dots[0])));
    counter.n = gcounter(build_vector(std::move(dots[1])));
    pncounter<int> counter_snapshot;
	counter_snapshot.p = gcounter(build_vector(std::move(dots[0])));
    counter_snapshot.n = gcounter(build_vector(std::move(dots[1])));

    counter.merge(counter_snapshot);

    RC_ASSERT(counter == counter_snapshot);
  }));

  assert(rc::check("increament change delta", [](std::array<map<int>, 2> dots, int value) {
    pncounter<int> replica1;
    replica1.p = gcounter(build_vector(std::move(dots[0])));
    replica1.n = gcounter(build_vector(std::move(dots[1])));
	auto replica2 = replica1;

	auto delta = replica1.inc(value);

	replica2.merge(delta);

    RC_ASSERT(replica1 == replica2);
  }));

  assert(rc::check("decreament change delta", [](std::array<map<int>, 2> dots, int value) {
    pncounter<int> replica1;
    replica1.p = gcounter(build_vector(std::move(dots[0])));
    replica1.n = gcounter(build_vector(std::move(dots[1])));
	auto replica2 = replica1;

	auto delta = replica1.dec(value);

	replica2.merge(delta);

    RC_ASSERT(replica1 == replica2);
  }));
}
