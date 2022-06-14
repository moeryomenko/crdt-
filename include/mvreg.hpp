#ifndef MV_REG_H
#define MV_REG_H

#include <algorithm>
#include <vector>

#include <context.hpp>
#include <crdt_traits.hpp>
#include <version_vector.hpp>

namespace crdt {

template <actor_type A, value_type T> struct mvreg {
  struct value {
    version_vector<A> vclock;
    T val;

    value() = default;
    value(const value &) = default;
    value(value &&) = default;
    value(const version_vector<A> &vc, T v) : vclock(vc), val(v) {}
    value &operator=(const value &) = default;
    value &operator=(value &&) = default;

    auto operator<=>(const value &) const noexcept = default;
  };
  std::vector<value> vals;

  auto operator==(const mvreg<A, T> &other) const noexcept -> bool {
    const auto cmp_values = [](const std::vector<value> &v1,
                               const std::vector<value> &v2) -> bool {
      for (const auto &d : v1) {
        if (std::none_of(v2.begin(), v2.end(),
                         [=](const value &v) { return v == d; }))
          return false;
      }
      return true;
    };
    return cmp_values(vals, other.vals) && cmp_values(other.vals, vals);
  }

  void reset_remove(const version_vector<A> &clock) noexcept {
    std::erase_if(vals, [clock = &clock](const auto &val) {
      val.vclock.reset_remove(clock);
      return val.vclock.empty();
    });
  }

  auto validate_merge(const mvreg<A, T> &other) const noexcept
      -> std::optional<std::error_condition> {
    return std::nullopt;
  }

  void merge(mvreg<A, T> other) noexcept {
    auto erase_filter = [](auto &origin, const auto &filter) {
      std::erase_if(origin, [vals = &filter](const auto &val) {
        return std::count_if(vals->begin(), vals->end(),
                             [clock = val.vclock](const auto &val) {
                               return clock < val.vclock;
                             }) != 0;
      });
    };
    erase_filter(vals, other.vals);
    erase_filter(other.vals, vals);
    vals.insert(vals.end(), other.vals.begin(), other.vals.end());
  }

  auto validate_op(const value &_) const noexcept
      -> std::optional<std::error_condition> {
    return std::nullopt;
  }

  void apply(const value &op) noexcept {
    if (std::none_of(vals.begin(), vals.end(),
                     [clock = op.vclock](const auto &val) {
                       return val.vclock > clock;
                     }))
      vals.push_back(op);
  }

  auto write(add_context<A> &&ctx, T val) const noexcept -> value {
    return value{ctx.vector, val};
  }

  auto write(const A &actor, const T &val) const noexcept -> mvreg<A, T> {
    mvreg<A, T> reg;
    reg.apply(reg.write(read().derive_add_context(actor), val));
    return mvreg<A, T>{};
  }

  auto read() const noexcept -> read_context<std::vector<T>, A> {
    auto read_ctx = this->read_ctx();
    std::transform(vals.begin(), vals.end(), std::back_inserter(read_ctx.value),
                   [](const auto &val) { return val.val; });
    return read_ctx;
  }

  auto read_ctx() const noexcept -> read_context<std::vector<T>, A> {
    auto clock = this->clock();
    return read_context<std::vector<T>, A>(clock, clock, std::vector<T>{});
  }

  auto clock() const noexcept -> version_vector<A> {
    version_vector<A> resutl{};
    for (const auto &val : vals)
      resutl.merge(val.vclock);
    return resutl;
  }
};

} // namespace crdt.

#endif // MV_REG_H
