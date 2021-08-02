#include "dot.hpp"
#include "version_vector.hpp"

#include <rapidcheck.h>
#include <unordered_map>

auto main() -> int
{
    assert(rc::check("increment method return the dot incremented by one",
            [](std::unordered_map<int, uint32_t> dots) {
                version_vector<int> v(robin_hood::unordered_flat_map<int, uint64_t>(dots.begin(), dots.end()));
                for(auto [actor, counter] : v.dots) {
                    auto incremanted_dot = dot{actor, counter+1};
                    RC_ASSERT(v.inc(actor) == incremanted_dot);
                }
            }));
    return 0;
}
