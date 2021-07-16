#include <boost/ut.hpp>

#include <dot.hpp>
#include <gcounter.hpp>

auto main() -> int {
    using namespace boost::ut;

    "basic by one"_test = [] {
        gcounter<std::string> A;
        gcounter<std::string> B;

        A.apply(A.inc("A"));
        B.apply(B.inc("B"));

        expect(A.read() == B.read());
        expect(A != B);

        A.apply(A.inc("A"));

        expect(A.read() == (B.read() + 1));
    };

    "basic by many"_test = [] {
        gcounter<std::string> A;
        gcounter<std::string> B;

        std::uint32_t steps = 3;

        A.apply(A.inc("A", steps));
        B.apply(B.inc("B", steps));

        expect(A.read() == B.read());
        expect(A != B);

        A.apply(A.inc("A", steps));

        expect(A.read() == (B.read() + steps));
    };
}
