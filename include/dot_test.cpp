#include "dot.hpp"

#include <rapidcheck.h>

namespace rc {
template <actor_type A>
struct Arbitrary<dot<A>> {
    static Gen<dot<A>> arbitrary()
    {
        return gen::build<dot<A>>(
            gen::set(&dot<A>::actor),
            gen::set(&dot<A>::counter, gen::inRange(0, 100)));
    }
};
}

auto main() -> int
{
    rc::check("inc increments only the counter",
        [](dot<int> d) {
            RC_ASSERT(dot(d.actor, d.counter + 1) == ++d);
        });

    rc::check("dots ordered by actor first",
        [](dot<int> a, dot<int> b) {
            auto ord_property = a.actor < b.actor ? a < b:
                                a.actor > b.actor ? a > b:
                                a.actor == b.actor ? (a.counter < b.counter ? a < b:
                                                      a.counter > b.counter ? a > b:
                                                      a.counter == b.counter ? a == b : false) :
                                false;
            RC_ASSERT(ord_property);
        });
    return 0;
}
