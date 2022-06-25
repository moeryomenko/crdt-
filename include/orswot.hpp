#ifndef ORSWOT_H
#define ORSWOT_H

#include <algorithm>
#include <compare>
#include <iterator>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include <context.hpp>
#include <crdt_traits.hpp>
#include <dot.hpp>
#include <version_vector.hpp>

namespace crdt {

template <actor_type _Key, actor_type _Actor,
          iterable_assiative_type<_Key, version_vector<_Actor>> _Entries_map =
              std::unordered_map<_Key, version_vector<_Actor>>,
          set_type<_Key> _Deferred_set_type = std::unordered_set<_Key>,
          iterable_assiative_type<version_vector<_Actor>, _Deferred_set_type>
              _Deferred_map =
                  std::unordered_map<version_vector<_Actor>, _Deferred_set_type>>
struct orswot {
  using vector_clock = version_vector<_Actor>;
  using deferred_set = _Deferred_set_type;
  using orswot_type = orswot<_Key, _Actor, _Entries_map, _Deferred_set_type, _Deferred_map>;

  vector_clock clock;
  _Entries_map entries;
  _Deferred_map deferred;

  orswot() = default;
  orswot(const orswot_type &) = default;
  orswot(orswot_type &&) = default;

  auto operator==(const orswot_type &other) const noexcept -> bool {
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
    dot<_Actor> d;
    std::vector<_Key> members;
  };

  struct Rm {
    version_vector<_Actor> clock;
    std::vector<_Key> members;
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

  void reset_remove(version_vector<_Actor> vclock) {
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

  void merge(const orswot_type &other) {
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

  auto contains(const _Key &member) noexcept -> read_context<bool, _Actor> {
    return read_context(clock, entries[member], entries.contains(member));
  }

  auto read() const noexcept -> read_context<deferred_set, _Actor> {
    deferred_set val;
    std::transform(entries.begin(), entries.end(),
                   std::inserter(val, val.begin()),
                   [](auto pair) { return pair.first; });
    return read_context(clock, clock, val);
  }

  auto read_ctx() const noexcept -> read_context<deferred_set, _Actor> {
    return read_context(clock, clock, deferred_set());
  }

  auto add(add_context<_Actor> ctx, _Key member) noexcept -> Op {
    return Add{std::move(ctx.dot), {member}};
  }

  auto add(const _Actor &actor, const _Key &member) noexcept
      -> orswot_type {
    orswot_type delta;
    delta.apply(delta.add(read_ctx().derive_add_context(actor), member));
    merge(delta);
    return delta;
  }

  auto rm(remove_context<_Actor> ctx, _Key member) noexcept -> Op {
    return Rm{ctx.vector, {member}};
  }

  auto rm(const _Actor &_, const _Key &member) noexcept
      -> orswot_type {
    orswot_type delta;
    delta.apply(delta.rm(read_ctx().derive_remove_context(), member));
    merge(delta);
    return delta;
  }
};

} // namespace crdt

#endif // ORSWOT_H
