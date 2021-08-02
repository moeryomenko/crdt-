#include "dot.hpp"
#include "version_vector.hpp"

#include <rapidcheck.h>
#include <unordered_map>

using map = std::unordered_map<int, std::uint64_t>;
using robin_map = robin_hood::unordered_flat_map<int, std::uint64_t>;

auto main() -> int
{
    assert(rc::check("increment method return the dot incremented by one", [](map dots) {
        version_vector<int> v(robin_map(dots.begin(), dots.end()));
        for (auto [actor, counter] : v.dots) {
            auto incremanted_dot = dot { actor, counter + 1 };
            RC_ASSERT(v.inc(actor) == incremanted_dot);
        }
    }));
    return 0;
}
