#ifndef ORSMOT_H
#define ORSMOT_H

#include <algorithm>
#include <compare>
#include <cstddef>
#include <numeric>
#include <optional>
#include <set>
#include <system_error>
#include <tuple>
#include <vector>
#include <variant>

#include <robin_hood.h>

#include <context.hpp>
#include <crdt_traits.hpp>
#include <dot.hpp>
#include <version_vector.hpp>

namespace crdt {

template <value_type K, crdt V> struct ormwot {
  using actor_t = typename V::actor_t;
  using vector_clock = version_vector<actor_t>;
  using deferred_set = robin_hood::unordered_flat_set<K>;

  struct Entry {
    vector_clock clock;
    V val;
  };

  vector_clock clock;
  robin_hood::unordered_flat_map<K, Entry> entries;
  robin_hood::unordered_flat_map<vector_clock, deferred_set> deferred;

  ormwot() = default;
  ormwot(const ormwot<K, V> &) = default;
  ormwot(ormwot<K, V> &&) = default;

  struct Add {
    dot<actor_t> d;
    K key;
    typename V::Op op;
  };

  struct Rm {
    vector_clock clock;
    deferred_set keyset;
  };

  auto operator==(const ormwot<K, V> &other) const noexcept -> bool {
    for (const auto &[key, entry] : entries) {
      if (auto it = other.entries.find(key); it == other.entries.end()) {
        return false;
      } else {
        if (entry.val != it->second.val) {
          return false;
        }
      }
    }
    for (const auto &[key, entry] : other.entries) {
      if (auto it = entries.find(key); it == entries.end()) {
        return false;
      } else {
        if (entry.val != it->second.val) {
          return false;
        }
      }
    }
    return true;
  }

  using Op = std::variant<Add, Rm>;

  void apply_keyset_rm(deferred_set keyset, vector_clock vclock) noexcept {
    for (const auto &key : keyset) {
      if (auto entry = entries.find(key); entry != entries.end()) {
        entry->second.clock.reset_remove(vclock);

        if (entry->second.clock.empty()) {
          entries.erase(entry);
        } else {
          entry->second.val.reset_remove(vclock);
        }
      }
    }

    if (auto cmp = this->clock <=> vclock;
        cmp == std::partial_ordering::less ||
        cmp == std::partial_ordering::unordered) {
      if (auto existing_deferred = deferred.find(vclock);
          existing_deferred != deferred.end()) {
        existing_deferred->second.insert(keyset.begin(), keyset.end());
      } else {
        deferred[vclock] = keyset;
      }
    }
  }

  void apply_deferrd() noexcept {
    for (auto [vclock, entrs] : deferred) {
      apply_keyset_rm(entrs, vclock);
    }
  }

  void reset_remove(const version_vector<actor_t> &vclock) noexcept {
    for (auto it = entries.begin(); it != entries.end();) {
      it->second.val.reset_remove(vclock);
      it->second.clock.reset_remove(vclock);
      if (it->second.clock.empty()) {
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

    clock.reset_remove(vclock);
  }

  auto validate_op(const Op &op) const noexcept
      -> std::optional<std::error_condition> {
    return std::visit(overloaded{
                          [this](Add add) {
                            auto res(this->clock.validate_op(add.d));
                            return res.value_or(validate_entry_op(add));
                          },
                          [](Rm) { return std::nullopt; },
                      },
                      op);
  }

  auto validate_entry_op(const Add add) const noexcept
      -> std::optional<std::error_condition> {
    Entry entry;
    if (auto it = entries.find(add.key); it != entries.end()) {
      std::tie(entry.clock, entry.val) = it;
    }

    return entry.clock.validate_op(add.d).value_or(
        entry.val.validate_op(add.op));
  }

  void apply(const Op &op) noexcept {
    std::visit(
        overloaded{
            [this](const Add &add) {
              if (clock.get(add.d.actor) >= add.d.counter) {
                return;
              }

              auto it = entries.find(add.key);
              if (it == entries.end()) {
                std::tie(it, std::ignore) = entries.insert({add.key, Entry{}});
              }

              it->second.clock.apply(dot(add.d.actor, add.d.counter));
              it->second.val.apply(add.op);

              clock.apply(dot(add.d.actor, add.d.counter));
              apply_deferrd();
            },
            [this](const Rm &rm) { apply_keyset_rm(rm.keyset, rm.clock); },
        },
        op);
  }

  void merge(const ormwot<K, V> &other) noexcept {
    for (auto it = entries.begin(); it != entries.end();) {
      if (!other.entries.contains(it->first)) {
        if (other.clock >= it->second.clock) {
          it = entries.erase(it);
        } else {
          it->second.clock.reset_remove(other.clock);
          auto removed_info(other.clock);
          removed_info.reset_remove(it->second.clock);
          it->second.val.reset_remove(removed_info);
          ++it;
        }
      } else {
        ++it;
      }
    }

    for (auto [key, entry] : other.entries) {
      if (auto our_entry = entries.find(key); our_entry != entries.end()) {
        auto common = intersection(entry.clock, our_entry->second.clock);
        common.merge(entry.clock.clone_without(this->clock));
        common.merge(our_entry->second.clock.clone_without(other.clock));
        if (common.empty()) {
          entries.erase(our_entry);
        } else {
          our_entry->second.val.merge(entry.val);

          auto removed_info(entry.clock);
          removed_info.merge(our_entry->second.clock);
          removed_info.reset_remove(common);
          our_entry->second.val.reset_remove(removed_info);
          our_entry->second.clock = common;
        }
      } else {
        if (this->clock >= entry.clock) {
        } else {
          entry.clock.reset_remove(clock);
          auto removed_info(clock);
          removed_info.reset_remove(entry.clock);
          entry.val.reset_remove(removed_info);
          entries[key] = entry;
        }
      }
    }

    for (auto [rm_clock, keys] : other.deferred) {
      apply_keyset_rm(keys, rm_clock);
    }

    clock.merge(other.clock);
    apply_deferrd();
  }

  auto empty() const noexcept -> read_context<bool, actor_t> {
    return read_context(clock, clock, entries.empty());
  }

  auto size() const noexcept -> read_context<std::size_t, actor_t> {
    return read_context(clock, clock, entries.size());
  }

  auto get(K key) const noexcept -> read_context<std::optional<V>, actor_t> {
    std::optional<V> entry;
    version_vector<actor_t> rm_clock;

    if (auto it = entries.find(key); it == entries.end()) {
      entry = std::nullopt;
    } else {
      entry = it->second.val;
      rm_clock = it->second.clock;
    }

    return read_context(clock, clock, entry);
  }

  auto update(const add_context<actor_t> &ctx, K key, auto f) const noexcept
      -> Op {
    V val;
    if (auto it = entries.find(key); it != entries.end())
      val = it->second.val;
    return Add{dot(ctx.dot.actor, ctx.dot.counter), key, f(ctx, val)};
  }

  auto update(actor_t actor, K key, auto f) noexcept -> ormwot<K, V> {
    ormwot<K, V> delta;
    delta.apply(delta.update(read_ctx().derive_add_context(actor), key, f));
    merge(delta);
    return delta;
  }

  auto rm(const remove_context<actor_t> &ctx, K key) const noexcept -> Op {
    deferred_set keyset;
    keyset.insert(key);
    return Rm{clock, keyset};
  }

  auto rm(actor_t _, K key) noexcept -> ormwot<K, V> {
    ormwot<K, V> delta;
    delta.apply(delta.rm(read_ctx().derive_remove_context(), key));
    merge(delta);
    return delta;
  }

  auto read_ctx() const noexcept -> read_context<void *, actor_t> {
    return read_context<void *, actor_t>(clock, clock, nullptr);
  }

  auto keys() const noexcept -> std::set<read_context<K, actor_t>> {
    std::set<read_context<K, actor_t>> keys;
    for (auto [key, val] : entries)
      keys.emplace(read_context(clock, val.clock, key));
    return keys;
  }

  auto values() const noexcept -> std::vector<read_context<K, actor_t>> {
    std::vector<read_context<K, actor_t>> values;
    for (auto [key, val] : entries)
      values.emplace_back(read_context(clock, val.clock, key));
    return values;
  }
};

} // namespace crdt

#endif // ORMOT_H
