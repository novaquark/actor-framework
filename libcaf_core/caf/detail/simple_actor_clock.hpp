/******************************************************************************
 *                       ____    _    _____                                   *
 *                      / ___|  / \  |  ___|    C++                           *
 *                     | |     / _ \ | |_       Actor                         *
 *                     | |___ / ___ \|  _|      Framework                     *
 *                      \____/_/   \_|_|                                      *
 *                                                                            *
 * Copyright 2011-2018 Dominik Charousset                                     *
 *                                                                            *
 * Distributed under the terms and conditions of the BSD 3-Clause License or  *
 * (at your option) under the terms and conditions of the Boost Software      *
 * License 1.0. See accompanying files LICENSE and LICENSE_ALTERNATIVE.       *
 *                                                                            *
 * If you did not receive a copy of the license files, see                    *
 * http://opensource.org/licenses/BSD-3-Clause and                            *
 * http://www.boost.org/LICENSE_1_0.txt.                                      *
 ******************************************************************************/

#pragma once

#include <map>
#include <unordered_map>

#include "caf/actor_clock.hpp"
#include "caf/actor_control_block.hpp"
#include "caf/group.hpp"
#include "caf/mailbox_element.hpp"
#include "caf/message.hpp"
#include "caf/message_id.hpp"
#include "caf/variant.hpp"

namespace caf {
namespace detail {

class simple_actor_clock : public actor_clock {
public:
  // -- member types -----------------------------------------------------------

  /// Request for a `timeout_msg`.
  struct ordinary_timeout {
    strong_actor_ptr self;
    atom_value type;
    uint64_t id;
  };

  struct multi_timeout {
    strong_actor_ptr self;
    atom_value type;
    uint64_t id;
  };

  /// Request for a `sec::request_timeout` error.
  struct request_timeout {
    strong_actor_ptr self;
    message_id id;
  };

  /// Request for sending a message to an actor at a later time.
  struct actor_msg {
    strong_actor_ptr receiver;
    mailbox_element_ptr content;
  };

  /// Request for sending a message to a group at a later time.
  struct group_msg {
    group target;
    strong_actor_ptr sender;
    message content;
  };

  using value_type = variant<ordinary_timeout, multi_timeout, request_timeout,
                             actor_msg, group_msg>;

  using map_type = std::multimap<time_point, value_type>;

  // this struct will hold all the timers related to an actor
  struct actor_timers {
    abstract_actor* actor_ptr = nullptr;

    using map = std::unordered_multimap<uint64_t, map_type::iterator>;
    // group timers by a token (either a "type" or an "id") for efficient timer manipulations.
    map timersByToken;

    // the only legal way to construct a value is when adding a timer.
    template <typename Predicate>
    explicit actor_timers(abstract_actor* v, Predicate p, map_type::iterator it)
        : actor_ptr(v) {
      addTimer(p, it);
    }

    template <typename Predicate>
    void addTimer(Predicate p, map_type::iterator it) {
      timersByToken.emplace(p.fast_key(), it);
    }

    bool empty() const {
      return timersByToken.empty();
    }
  };

  using secondary_map = std::unordered_map<abstract_actor*, actor_timers>;

  struct ordinary_predicate {
    atom_value type;
    bool operator()(const actor_timers::map::value_type& x) const noexcept;

    uint64_t fast_key() const {
      return static_cast<uint64_t>(type);
    }
  };

  struct multi_predicate {
    atom_value type;
    bool operator()(const actor_timers::map::value_type& x) const noexcept;
    uint64_t fast_key() const {
      return static_cast<uint64_t>(type);
    }
  };

  struct request_predicate {
    message_id id;
    bool operator()(const actor_timers::map::value_type& x) const noexcept;

    uint64_t fast_key() const {
      return id.integer_value();
    }
  };

  struct visitor {
    simple_actor_clock* thisptr;

    void operator()(ordinary_timeout& x);

    void operator()(multi_timeout& x);

    void operator()(request_timeout& x);

    void operator()(actor_msg& x);

    void operator()(group_msg& x);
  };

protected:
  void set_ordinary_timeout(time_point t, abstract_actor* self, atom_value type,
                            uint64_t id) override;
  void set_multi_timeout(time_point t, abstract_actor* self, atom_value type,
                         uint64_t id) override;
  void set_request_timeout(time_point t, abstract_actor* self,
                           message_id id) override;

public:
  void set_ordinary_timeout(time_point t, abstract_actor* self,
                            strong_actor_ptr sptr, atom_value type,
                            uint64_t id);

  void set_multi_timeout(time_point t, abstract_actor* self,
                         strong_actor_ptr sptr, atom_value type, uint64_t id);

  void set_request_timeout(time_point t, abstract_actor* self,
                           strong_actor_ptr sptr, message_id id);

  void cancel_ordinary_timeout(abstract_actor* self, atom_value type) override;

  void cancel_request_timeout(abstract_actor* self, message_id id) override;

  void cancel_timeouts(abstract_actor* self) override;

  void schedule_message(time_point t, strong_actor_ptr receiver,
                        mailbox_element_ptr content) override;

  void schedule_message(time_point t, group target, strong_actor_ptr sender,
                        message content) override;

  void cancel_all() override;

  inline const map_type& schedule() const {
    return schedule_;
  }

  inline map_type& schedule() {
    return schedule_;
  }

  inline const secondary_map& actor_lookup() const {
    return actor_lookup_;
  }

protected:
  // this is the result of a lookup with an actor and a predicate.
  struct lookup_result {
    // iterator to the actor bucket of all its timers.
    // always an initialized value.  It is actor_lookup_.end() when there is no
    // actor found.
    secondary_map::iterator actor_timers_it = {};

    // iterator to one of the timer in the actor bucket.
    // only initialized when [valid] is true.  It might be completely garbage
    // when [valid] is false.
    actor_timers::map::iterator timer_it = {};

    // means both iterators are valid.
    // when invalid, actor_timers_it could be valid.
    bool valid = false;

    lookup_result(secondary_map::iterator actor_timers_it,
                  actor_timers::map::iterator timer_it, bool valid)
        : actor_timers_it(actor_timers_it), timer_it(timer_it), valid(valid) {
    }

    explicit operator bool() const {
      return valid;
    }
  };

  template <class Predicate>
  lookup_result lookup(abstract_actor* self, Predicate pred) {
    auto e = actor_lookup_.end();
    actor_timers::map::iterator uninitTimerVal;
    auto actor_timers_it = actor_lookup_.find(self);
    if (actor_timers_it == e)
      return lookup_result{e, uninitTimerVal, false};
    actor_timers& actor_bucket = actor_timers_it->second;
    auto range = actor_bucket.timersByToken.equal_range(pred.fast_key());
    auto i = std::find_if(range.first, range.second, pred);
    if (i != range.second) {
      return lookup_result{actor_timers_it, i, true};
    } else {
      // we return an invalid lookup, but with a valid iterator to the actor
      // bucket this might save a lookup sometimes.
      return lookup_result{actor_timers_it, uninitTimerVal, false};
    }
  }

  void erase(lookup_result res) {
    if (res) {
      actor_timers& actor_bucket = res.actor_timers_it->second;
      actor_bucket.timersByToken.erase(res.timer_it);
      if (actor_bucket.empty()) {
        actor_lookup_.erase(res.actor_timers_it);
      }
    }
  }

  template <class Predicate>
  void add_actor_timer(lookup_result i, abstract_actor* self, Predicate p,
               map_type::iterator it) {
    if (i.actor_timers_it != actor_lookup_.end()) {
      i.actor_timers_it->second.addTimer(p, it);
    } else {
      actor_lookup_.emplace(self, actor_timers(self, p, it));
    }
  }

  template <class Predicate>
  void add_actor_timer(abstract_actor* self, Predicate p, map_type::iterator it) {
    auto actorIt = actor_lookup_.find(self);
    if (actorIt != actor_lookup_.end()) {
      actorIt->second.addTimer(p, it);
    } else {
      actor_lookup_.emplace(self, actor_timers(self, p, it));
    }
  }

  template <class Predicate>
  void cancel(abstract_actor* self, Predicate pred) {
    auto i = lookup(self, pred);
    if (i) {
      schedule_.erase(i.timer_it->second);
      erase(i);
    }
  }

  template <class Predicate>
  void drop_lookup(abstract_actor* self, Predicate pred) {
    auto i = lookup(self, pred);
    if (i)
      erase(i);
  }

  /// Timeout schedule.
  map_type schedule_;

  /// Secondary index for accessing timeouts by actor.
  secondary_map actor_lookup_;
};

} // namespace detail
} // namespace caf
