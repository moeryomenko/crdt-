#include <cstdint>
#include <utility>

#include <boost/ut.hpp>
#include <rapidcheck.h>

#include <mvreg.hpp>
#include <version_vector.hpp>

using namespace crdt;

using map = std::unordered_map<int, std::uint64_t>;
using reg = std::pair<map, int>;
using robin_map = robin_hood::unordered_map<int, std::uint64_t>;

mvreg<int, std::uint64_t> build_from_reg(reg &&r) {
  auto &&[m, val] = r;
  using val_type = mvreg<int, std::uint64_t>::value;
  std::vector<val_type> result;
  result.push_back(
      val_type(version_vector<int>{robin_map(m.begin(), m.end())}, val));
  return mvreg<int, std::uint64_t>{result};
}

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
    expect(r1_snapshot.read().value ==
           std::vector<std::string>{"bob", "alice"});
  };

  "another case"_test = [] {
    mvreg<std::string, std::string> r1{};
    mvreg<std::string, std::string> r2{};

    auto r1_read_ctx = r1.read();
    auto r2_read_ctx = r2.read();

    r1.apply(r1.write(r1_read_ctx.derive_add_context("A"), "bob"));
    auto op = r2.write(r2_read_ctx.derive_add_context("B"), "alice");
    r2.apply(op);

    auto r1_snapshot = r1;
    r1.apply(op);

    expect(r1.read().value == std::vector<std::string>{"bob", "alice"});

    r1_snapshot.merge(r2);
    expect(r1_snapshot.read().value ==
           std::vector<std::string>{"bob", "alice"});
  };

  assert(rc::check("associative", [](reg r1, reg r2, reg r3) {
    auto reg1 = build_from_reg(std::move(r1));
    auto reg2 = build_from_reg(std::move(r2));
    auto reg3 = build_from_reg(std::move(r3));

    auto reg1_snapshot = reg1;

    reg1.merge(reg2);
    reg1.merge(reg3);

    reg2.merge(reg3);
    reg1_snapshot.merge(reg2);

    RC_ASSERT(reg1 == reg1_snapshot);
  }));

  assert(rc::check("commutative", [](reg r1, reg r2) {
    auto reg1 = build_from_reg(std::move(r1));
    auto reg2 = build_from_reg(std::move(r2));

    auto reg1_snapshot = reg1;

    reg1.merge(reg2);

    reg2.merge(reg1_snapshot);

    RC_ASSERT(reg1 == reg2);
  }));

  assert(rc::check("idempotent", [](reg r) {
    auto reg = build_from_reg(std::move(r));
    auto reg_snapshot = build_from_reg(std::move(r));

    reg.merge(reg_snapshot);

    RC_ASSERT(reg == reg_snapshot);
  }));

  assert(rc::check("change delta", [](reg r, int value) {
	auto [m, actor] = r;
    auto replica1 = build_from_reg(std::move(r));
	auto replica2 = replica1;

	auto delta = replica1.write(actor, value);

	replica2.merge(delta);

    RC_ASSERT(replica1 == replica2);
  }));
}
