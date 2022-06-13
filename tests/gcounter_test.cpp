#include <string>

#include <boost/ut.hpp>
#include <rapidcheck.h>

#include <dot.hpp>
#include <gcounter.hpp>

#include "version_vector_utility.hpp"

auto main() -> int {
  using namespace boost::ut;
  using namespace crdt;

  "basic by one"_test = [] {
    gcounter<std::string> A;
    gcounter<std::string> B;

    A.apply(A + "A");
    B.apply(B + "B");

    expect(A.read() == B.read());
    expect(A != B);

    A.apply(A + "A");

    expect(A.read() == (B.read() + 1));
  };

  "basic by many"_test = [] {
    gcounter<std::string> A;
    gcounter<std::string> B;

    std::uint32_t steps = 3;

    A.apply(A + std::pair{"A", steps});
    B.apply(B + std::pair{"B", steps});

    expect(A.read() == B.read());
    expect(A != B);

    A.apply(A + std::pair{"A", steps});

    expect(A.read() == (B.read() + steps));
  };

  assert(rc::check("associative",
                   [](map<int> dots1, map<int> dots2, map<int> dots3) {
                     auto counter1 = gcounter(build_vector(std::move(dots1)));
                     auto counter2 = gcounter(build_vector(std::move(dots2)));
                     auto counter3 = gcounter(build_vector(std::move(dots3)));

                     auto v1_snapshot = counter1;

                     counter1.merge(counter2);
                     counter1.merge(counter3);

                     counter2.merge(counter3);
                     v1_snapshot.merge(counter2);

                     RC_ASSERT(counter1 == v1_snapshot);
                   }));

  assert(rc::check("commutative", [](map<int> dots1, map<int> dots2) {
    auto counter1 = gcounter(build_vector(std::move(dots1)));
    auto counter2 = gcounter(build_vector(std::move(dots2)));

    auto v1_snapshot(counter1);

    counter1.merge(counter2);

    counter2.merge(v1_snapshot);

    RC_ASSERT(counter1 == counter2);
  }));

  assert(rc::check("idempotent", [](map<int> dots) {
    auto counter = gcounter(build_vector(std::move(dots)));
    auto counter_snapshot = gcounter(build_vector(std::move(dots)));

    counter.merge(counter_snapshot);

    RC_ASSERT(counter == counter_snapshot);
  }));

  assert(rc::check("change delta", [](map<int> dots, int value) {
    auto replica1 = gcounter(build_vector(std::move(dots)));
	auto replica2 = replica1;

	auto delta = replica1.inc(value);

	replica2.merge(delta);

    RC_ASSERT(replica1 == replica2);
  }));
}
