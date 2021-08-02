#include <boost/ut.hpp>
#include <rapidcheck.h>

#include <cstdint>
#include <string>
#include <system_error>
#include <utility>

#include <lwwreg.hpp>

auto build_from_pair(std::pair<std::uint8_t, std::uint16_t>&& prim)
    -> lwwreg<std::uint8_t, std::pair<uint16_t, uint8_t>>
{
    return lwwreg { prim.first, std::pair { prim.second, prim.first } };
}

template <value_type V, marker_type M> auto is_conflicted_marker(lwwreg<V, M>& reg1, lwwreg<V, M>& reg2) -> bool
{
    return (reg1.marker == reg2.marker && reg1.value != reg2.value);
}

auto main() -> int
{
    using namespace boost::ut;

    "test default"_test = [] {
        lwwreg<std::string, int> reg {};
        expect(reg == lwwreg { std::string {}, 0 });
    };

    "test update"_test = [] {
        lwwreg reg { 123, 0 };

        reg.update(32, 2);
        expect(reg == lwwreg { 32, 2 });

        reg.update(57, 1);
        expect(reg == lwwreg { 32, 2 });

        reg.update(32, 2);
        expect(reg == lwwreg { 32, 2 });

        expect(reg.validate_update(4000, 2) == std::errc::operation_would_block);
        reg.update(4000, 2);
        expect(reg == lwwreg { 32, 2 });
    };

    using pair = std::pair<std::uint8_t, std::uint16_t>;
    assert(rc::check("associative", [](pair r1, pair r2, pair r3) {
        auto reg1 = build_from_pair(std::move(r1));
        auto reg2 = build_from_pair(std::move(r2));
        auto reg3 = build_from_pair(std::move(r3));

        auto has_confliciting_marker
            = is_conflicted_marker(reg1, reg2) || is_conflicted_marker(reg1, reg3) || is_conflicted_marker(reg2, reg3);

        if (has_confliciting_marker) {
            RC_DISCARD();
        }

        auto reg1_snapshot = reg1;

        reg1.merge(reg2);
        reg1.merge(reg3);

        reg2.merge(reg3);
        reg1_snapshot.merge(reg2);

        RC_ASSERT(reg1 == reg1_snapshot);
    }));

    assert(rc::check("commutative", [](pair r1, pair r2) {
        auto reg1 = build_from_pair(std::move(r1));
        auto reg2 = build_from_pair(std::move(r2));

        if (is_conflicted_marker(reg1, reg2)) {
            RC_DISCARD();
        }

        auto reg1_snapshot = reg1;

        reg1.merge(reg2);

        reg2.merge(reg1_snapshot);

        RC_ASSERT(reg1 == reg2);
    }));

    assert(rc::check("idempotent", [](pair r) {
        auto reg = build_from_pair(std::move(r));
        auto reg_snapshot = build_from_pair(std::move(r));

        reg.merge(reg_snapshot);

        RC_ASSERT(reg == reg_snapshot);
    }));
}
