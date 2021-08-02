#include <boost/ut.hpp>

#include <mvreg.hpp>

auto main() -> int {
    using namespace boost::ut;

    "simple case"_test = [] {
        mvreg<int, std::string> r1{};
        mvreg<int, std::string> r2{};

        auto r1_read_ctx = r1.read();
        auto r2_read_ctx = r2.read();

        r1.apply(r1.write(r1_read_ctx.derive_add_context(123), "bob"));
        auto op = r2.write(r2_read_ctx.derive_add_context(111), "alice");
        r2.apply(op);

        auto r1_snapshot = r1;
        r1.apply(op);

        expect(r1.read().value == std::vector<std::string>{"bob", "alice"});

        r1_snapshot.merge(r2);
        expect(r1_snapshot.read().value == std::vector<std::string>{"bob", "alice"});
    };
}
