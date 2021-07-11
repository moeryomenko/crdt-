#include "dot.hpp"
#include "version_vector.hpp"

#include <rapidcheck.h>

auto main() -> int
{
    rc::check("increment method return the dot incremented by one",
            [](std::map<int, uint32_t> dots) {
                version_vector<int> v{std::move(dots)};
                for(auto [actor, counter] : v.dots) {
                    auto incremanted_dot = dot{actor, counter+1};
                    RC_ASSERT(v.increment(actor) == incremanted_dot);
                }
            });
    return 0;
}
