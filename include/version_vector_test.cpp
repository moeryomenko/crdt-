#include "dot.hpp"
#include "version_vector.hpp"

#include <rapidcheck.h>
#include <unordered_map>

template<typename K, typename V>
auto stdMapToRobinHood(std::unordered_map<K, V> stdmap) -> robin_hood::unordered_flat_map<K, V> {
    robin_hood::unordered_flat_map<K, V> res;
    for (auto [k, v]: stdmap) {
        res[k] = v;
    }
    return res;
}

auto main() -> int
{
    rc::check("increment method return the dot incremented by one",
            [](std::unordered_map<int, uint32_t> dots) {
                version_vector<int> v{std::move(stdMapToRobinHood(dots))};
                for(auto [actor, counter] : v.dots) {
                    auto incremanted_dot = dot{actor, counter+1};
                    RC_ASSERT(v.increment(actor) == incremanted_dot);
                }
            });
    return 0;
}
