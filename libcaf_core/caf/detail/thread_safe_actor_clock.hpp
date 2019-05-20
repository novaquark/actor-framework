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

#include <array>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <vector>

#include "caf/detail/double_ended_queue.hpp"
#include "caf/detail/simple_actor_clock.hpp"
#include "caf/variant.hpp"

namespace caf {
namespace detail {

class thread_safe_actor_clock : public actor_clock {
public:
  using time_point = typename simple_actor_clock::clock_type::time_point;

  thread_safe_actor_clock();

  void set_ordinary_timeout(time_point t, abstract_actor* self, atom_value type,
                            uint64_t id) override;

  void set_request_timeout(time_point t, abstract_actor* self,
                           message_id id) override __attribute__ ((noiinline)) ;

  void cancel_ordinary_timeout(abstract_actor* self, atom_value type) override;

  void cancel_request_timeout(abstract_actor* self, message_id id) override;

  void cancel_timeouts(abstract_actor* self) override;

  void schedule_message(time_point t, strong_actor_ptr receiver,
                        mailbox_element_ptr content) override;

  void schedule_message(time_point t, group target, strong_actor_ptr sender,
                        message content) override;

  virtual void set_multi_timeout(time_point t, abstract_actor* self,
                                 atom_value type, uint64_t id) override;

  void cancel_all() override;

  void run_dispatch_loop();

  void cancel_dispatch_loop();

private:
  struct Messages {
    struct visitor;
    struct set_ordinary_timeout {
      time_point t;
      weak_actor_ptr self;
      atom_value type;
      uint64_t id;
    };
    struct set_request_timeout {
      time_point t;
      weak_actor_ptr self;
      message_id id;
    };
    struct cancel_ordinary_timeout {
      abstract_actor* self;
      atom_value type;
    };
    struct cancel_request_timeout {
      // danger here, at the time we process the message, we might have another
      // existing actor at the exact same address.
      abstract_actor* self;
      message_id id;
    };
    struct cancel_timeouts {
      // that one is called at the time of destruction when an actor dies.
      // we have a potentially nasty race condition here if the message is
      // processed async and another actor got the same pointer value. we can't
      // take a strong reference to the actor because it is being destroyed
      // (more or less) at the time we get here. the good news, is that we
      // probably don't have any issue in practice. cancel_timeouts is scheduled
      // *before* the pointer is freed. any subsequent timers processed by
      // another occurrence of an actor at the same address would be enqued
      // *after* the processing of the cancel_timeouts. So, if we preserve the
      // ordering (which is the case), we are safe. this forbids smarter
      // implementations that would keep a list of messages for the actor clock
      // using the thread local storage.
      abstract_actor* self;
    };

    struct schedule_message {
      time_point t;
      strong_actor_ptr receiver;
      mailbox_element_ptr content;
    };

    struct schedule_message_group {
      time_point t;
      group target;
      strong_actor_ptr sender;
      message content;
    };

    struct set_multi_timeout {
      time_point t;
      weak_actor_ptr self;
      atom_value type;
      uint64_t id;
    };

    struct cancel_all {};

    using value_type =
      variant<set_ordinary_timeout, set_request_timeout,
              cancel_ordinary_timeout, cancel_request_timeout, cancel_timeouts,
              schedule_message, schedule_message_group, set_multi_timeout,
              cancel_all>;
  };

  void enqueueInvocation(Messages::value_type&&);
  void pumpMessages();

  std::condition_variable cv_;
  std::mutex mutex_;

  // concurrent access, must be protected by the mutex
  std::vector<Messages::value_type> messages_;

  // buffer only accessed from the caf actor thread. for hotswap with [messages]
  std::vector<Messages::value_type> tempBuffer_; 

  std::atomic<bool> done_;
  simple_actor_clock realClock_;
};

} // namespace detail
} // namespace caf
