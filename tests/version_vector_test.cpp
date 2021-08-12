#include <ostream>
#include <unordered_map>

#include <rapidcheck.h>

#include <dot.hpp>
#include <version_vector.hpp>

using namespace crdt;

using map = std::unordered_map<int, std::uint64_t>;
using robin_map = robin_hood::unordered_flat_map<int, std::uint64_t>;

auto build_vector(map&& counter) {
    return version_vector<int>(robin_map(counter.begin(), counter.end()));
}

namespace crdt {
void showValue(const version_vector<int>& v, std::ostream &os) {
    os << "[ ";
    for (const auto& [key, value]: v.dots) {
        os << "{ " << key << ": " << value << "}, ";
    }
    os << "]";
}
}

auto main() -> int
{
    assert(rc::check("increment method return the dot incremented by one", [](map dots) {
        auto v = build_vector(std::move(dots));
        for (auto [actor, counter] : v.dots) {
            auto incremanted_dot = dot { actor, counter + 1 };
            RC_ASSERT(v.inc(actor) == incremanted_dot);
        }
    }));

    assert(rc::check("associative", [] (map dots1, map dots2, map dots3) {
        auto v1 = build_vector(std::move(dots1));
        auto v2 = build_vector(std::move(dots2));
        auto v3 = build_vector(std::move(dots3));

        auto v1_snapshot = v1;

        v1.merge(v2);
        v1.merge(v3);

        v2.merge(v3);
        v1_snapshot.merge(v2);

        RC_ASSERT(v1 == v1_snapshot);
    }));

    assert(rc::check("commutative", [](map dots1, map dots2) {
        auto v1 = build_vector(std::move(dots1));
        auto v2 = build_vector(std::move(dots2));

        auto v1_snapshot(v1);

        v1.merge(v2);

        v2.merge(v1_snapshot);

        RC_ASSERT(v1 == v2);
    }));

    assert(rc::check("idempotent", [](map dots) {
        auto v = build_vector(std::move(dots));
        auto v_snapshot = build_vector(std::move(dots));

        v.merge(v_snapshot);

        RC_ASSERT(v == v_snapshot);
    }));
}
