#include <boost/ut.hpp>
#include <cstdint>
#include <rapidcheck.h>

#include <lexcounter.hpp>

auto main() -> int {
  using namespace boost::ut;
  using namespace crdt;

  "basic case"_test = [] {
	  lexcounter<char> o1('a');
	  lexcounter<char> o2('b');

	  o1.inc(3);
	  o1.inc(2);
	  --o1;
	  ++o2;

	  expect(o1.read() == 4_i);
	  expect(o2.read() == 1_i);

	  o2.merge(o1);

	  expect(o2.read() == 5_i);
  };

  "op case"_test = [] {
	  lexcounter<char> o1('a');

	  ++o1;
	  o1.apply(o1.dec_op());

	  expect(o1.read() == 0_i);
  };

  "property based tests"_test = [] {
    using rc::check;

    expect(check("associative", [](std::uint64_t a, std::uint64_t b, std::uint64_t c) {
	  lexcounter<char> counter1('a');
	  counter1.inc(a);
	  lexcounter<char> counter2('b');
	  counter2.inc(b);
	  lexcounter<char> counter3('c');
	  counter3.dec(c);

      lexcounter<char> counter1_snapshot('d', counter1);

      counter1.merge(counter2);
      counter1.merge(counter3);

      counter2.merge(counter3);
      counter1_snapshot.merge(counter2);

      RC_ASSERT(counter1.read() == counter1_snapshot.read());
    }));

    expect(check("commutative", [](std::uint64_t a, std::uint64_t b) {
	  lexcounter<char> counter1('a');
	  counter1.inc(a);
	  lexcounter<char> counter2('b');
	  counter2.dec(b);

      lexcounter<char> counter1_snapshot('c', counter1);

      counter1.merge(counter2);

      counter2.merge(counter1_snapshot);

      RC_ASSERT(counter1.read() == counter2.read());
    }));

    expect(check("idempotent", [](std::uint64_t a) {
	  lexcounter<char> counter('a');
	  counter.inc(a);
      lexcounter<char> counter_snapshot('b', counter);

      counter.merge(counter_snapshot);

      RC_ASSERT(counter.read() == counter_snapshot.read());
    }));

    expect(rc::check("increament change delta", [](std::uint64_t val, std::uint64_t increment) {
	  lexcounter<char> replica1('a');
	  replica1.inc(1);
      lexcounter<char> replica2('b', replica1);

      auto delta = replica1.inc(1);
      RC_ASSERT(replica1.read() == 2);
      RC_ASSERT(delta.read() == 2);

      replica2.merge(delta);

      RC_ASSERT(replica1.read() == replica2.read());
    }));

    expect(rc::check("decreament change delta", [](std::uint64_t val, std::uint64_t decreament) {
	  lexcounter<char> replica1('a');
	  replica1.inc(val);
      lexcounter<char> replica2('b', replica1);

      auto delta = replica1.dec(decreament);

      replica2.merge(delta);

      RC_ASSERT(replica1.read() == replica2.read());
    }));
  };
}
