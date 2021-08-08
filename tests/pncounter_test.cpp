#include <boost/ut.hpp>

#include <dot.hpp>
#include <pncounter.hpp>

auto main() -> int
{
    using namespace boost::ut;
    using namespace crdt;

    "basic by one"_test = [] {
        pncounter<std::string> a;
        expect(a.read() == 0);

        a.apply(a.inc("A"));
        expect(a.read() == 1);

        a.apply(a.inc("A"));
        expect(a.read() == 2);

        a.apply(a.dec("A"));
        expect(a.read() == 1);

        a.apply(a.inc("A"));
        expect(a.read() == 2);
    };

    "basic by many"_test = [] {
        pncounter<std::string> a;
        expect(a.read() == 0);

        std::uint32_t steps = 3;

        a.apply(a.inc("A", steps));
        expect(a.read() == steps);

        a.apply(a.inc("A", steps));
        expect(a.read() == (2 * steps));

        a.apply(a.dec("A", steps));
        expect(a.read() == steps);

        a.apply(a.inc("A", 1));
        expect(a.read() == (steps + 1));
    };
}
