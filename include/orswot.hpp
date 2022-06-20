#ifndef ORSWOT_H
#define ORSWOT_H

#include <algorithm>
#include <compare>
#include <iterator>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>

#include <robin_hood.h>

#include <context.hpp>
#include <crdt_traits.hpp>
#include <dot.hpp>
#include <version_vector.hpp>

namespace crdt {

template <actor_type M, actor_type A> struct orswot {
  using vector_clock = version_vector<A>;
  using deferred_set = robin_hood::unordered_flat_set<M>;

  vector_clock clock;
  robin_hood::unordered_flat_map<M, vector_clock> entries;
  robin_hood::unordered_flat_map<vector_clock, deferred_set> deferred;

  orswot() = default;
  orswot(const orswot<M, A> &) = default;
  orswot(orswot<M, A> &&) = default;

  auto operator==(const orswot<M, A> &other) const noexcept -> bool {
    for (const auto &e : other.entries)
      if (!entries.contains(e.first)) {
        return false;
      }
    for (const auto &e : entries)
      if (!other.entries.contains(e.first)) {
        return false;
      }
    return true;
  }
  struct Add {
    dot<A> d;
    std::vector<M> members;
  };

  struct Rm {
    version_vector<A> clock;
    std::vector<M> members;
  };

  using Op = std::variant<Add, Rm>;

  auto validate_op(const Op &op) const noexcept
      -> std::optional<std::error_condition> {
    return std::visit(
        overloaded{
            [this](Add add) { return this->clock.validate_op(add.d); },
            [](Rm) { return std::nullopt; },
        },
        op);
  }

  void apply(const Op &op) noexcept {
    std::visit(overloaded{
                   [this](const Add &add) {
                     if (clock.get(add.d.actor) >= add.d.counter) {
                       return;
                     }

                     for (const auto &member : add.members) {
                       auto member_vclock = this->entries[member];
                       member_vclock.apply(add.d);
                       this->entries[member] = member_vclock;
                     }

                     clock.apply(add.d);
                     this->apply_deferred();
                   },
                   [this](const Rm &rm) {
                     apply_rm(
                         deferred_set(rm.members.begin(), rm.members.end()),
                         rm.clock);
                   },
               },
               op);
  }

  void apply_deferred() {
    for (auto [vclock, entrs] : deferred) {
      apply_rm(entrs, vclock);
    }
  }

  void apply_rm(deferred_set members, vector_clock vclock) {
    for (const auto &member : members) {
      if (const auto &member_clock = entries.find(member);
          member_clock != entries.end()) {
        member_clock->second.reset_remove(vclock);

        if (member_clock->second.empty())
          entries.erase(member_clock);
      }
    }

    if (auto cmp = vclock <=> this->clock;
        cmp == std::partial_ordering::greater ||
        cmp == std::partial_ordering::unordered) {
      if (auto existing_deferred = deferred.find(vclock);
          existing_deferred != deferred.end()) {
        existing_deferred->second.insert(members.begin(), members.end());
      } else {
        deferred[vclock] = members;
      }
    }
  }

  void reset_remove(version_vector<A> vclock) {
	  clock.reset_remove(vclock);

	  for (auto it = entries.begin(); it != entries.end();) {
		  it->second.reset_remove(vclock);
		  if (it->second.empty()) {
			  it = entries.erase(it);
		  } else {
			  ++it;
		  }
	  }

	  for (auto it = deferred.begin(); it != deferred.end();) {
		  it->first.reset_remove(vclock);
		  if (it->first.empty()) {
			  it = deferred.erase(it);
		  } else {
			  ++it;
		  }
	  }
  }

  void merge(const orswot<M, A> &other) {
    for (auto it = entries.begin(); it != entries.end();) {
      if (!other.entries.contains(it->first)) {
        if (other.clock >= it->second) {
          it = entries.erase(it);
        } else {
          it->second.reset_remove(other.clock);
          ++it;
        }
      } else {
        ++it;
      }
    }

    for (auto [entry, vclock] : other.entries) {
      if (auto our_clock = entries.find(entry); our_clock != entries.end()) {
        auto common = intersection(vclock, our_clock->second);
        common.merge(vclock.clone_without(this->clock));
        common.merge(our_clock->second.clone_without(other.clock));
        if (common.empty()) {
          entries.erase(entry);
        } else {
          our_clock->second = common;
        }
      } else {
        if (this->clock >= vclock) {
        } else {
          vclock.reset_remove(this->clock);
          entries[entry] = vclock;
        }
      }
    }

    for (auto [rm_clock, members] : other.deferred)
      this->apply_rm(members, rm_clock);

    this->clock.merge(other.clock);
    apply_deferred();
  }

  auto contains(const M &member) noexcept -> read_context<bool, A> {
    return read_context(clock, entries[member], entries.contains(member));
  }

  auto read() const noexcept -> read_context<deferred_set, A> {
    deferred_set val;
    std::transform(entries.begin(), entries.end(),
                   std::inserter(val, val.begin()),
                   [](auto pair) { return pair.first; });
    return read_context(clock, clock, val);
  }

  auto read_ctx() const noexcept -> read_context<deferred_set, A> {
    return read_context(clock, clock, deferred_set());
  }

  auto add(add_context<A> ctx, M member) noexcept -> Op {
    return Add{std::move(ctx.dot), {member}};
  }

  auto add(const A &actor, const M &member) noexcept -> orswot<M, A> {
    orswot<M, A> delta;
    delta.apply(delta.add(read_ctx().derive_add_context(actor), member));
    merge(delta);
    return delta;
  }

  auto rm(remove_context<A> ctx, M member) noexcept -> Op {
    return Rm{ctx.vector, {member}};
  }

  auto rm(const A &_, const M &member) noexcept -> orswot<M, A> {
    orswot<M, A> delta;
    delta.apply(delta.rm(read_ctx().derive_remove_context(), member));
    merge(delta);
    return delta;
  }
};

} // namespace crdt

#endif // ORSWOT_H
