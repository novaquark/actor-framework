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

#include "caf/detail/thread_safe_actor_clock.hpp"
#include "caf/actor.hpp"
#include "caf/actor_cast.hpp"

namespace caf {
namespace detail {

namespace {

using guard_type = std::unique_lock<std::mutex>;

} // namespace

thread_safe_actor_clock::thread_safe_actor_clock() : done_(false) {
  // nop
}

void __attribute__((noinline))
thread_safe_actor_clock::enqueueInvocation(Messages::value_type&& i) {
  if (done_)
    return;
  {
    guard_type guard{mutex_};
    messages_.push_back(std::move(i));
  }
  cv_.notify_all();
}

void thread_safe_actor_clock::set_ordinary_timeout(time_point t,
                                                   abstract_actor* self,
                                                   atom_value type,
                                                   uint64_t id) {

  enqueueInvocation(Messages::set_ordinary_timeout{
    t, actor_cast<weak_actor_ptr>(self), type, id});
}

void thread_safe_actor_clock::set_request_timeout(time_point t,
                                                  abstract_actor* self,
                                                  message_id id) {
  enqueueInvocation(
    Messages::set_request_timeout{t, actor_cast<weak_actor_ptr>(self), id});
}

void thread_safe_actor_clock::cancel_ordinary_timeout(abstract_actor* self,
                                                      atom_value type) {
  enqueueInvocation(Messages::cancel_ordinary_timeout{self, type});
}

void thread_safe_actor_clock::cancel_request_timeout(abstract_actor* self,
                                                     message_id id) {
  enqueueInvocation(Messages::cancel_request_timeout{self, id});
}

void thread_safe_actor_clock::cancel_timeouts(abstract_actor* self) {
  enqueueInvocation(Messages::cancel_timeouts{self});
}

void thread_safe_actor_clock::schedule_message(time_point t,
                                               strong_actor_ptr receiver,
                                               mailbox_element_ptr content) {
  enqueueInvocation(
    Messages::schedule_message{t, std::move(receiver), std::move(content)});
}

void thread_safe_actor_clock::schedule_message(time_point t, group target,
                                               strong_actor_ptr sender,
                                               message content) {
  enqueueInvocation(Messages::schedule_message_group{
    t, std::move(target), std::move(sender), std::move(content)});
}

void thread_safe_actor_clock::set_multi_timeout(time_point t,
                                                abstract_actor* self,
                                                atom_value type, uint64_t id) {
  enqueueInvocation(
    Messages::set_multi_timeout{t, actor_cast<weak_actor_ptr>(self), type, id});
}

void thread_safe_actor_clock::cancel_all() {
  enqueueInvocation(Messages::cancel_all{});
}

struct thread_safe_actor_clock::Messages::visitor {
  simple_actor_clock& clock;

  void operator()(set_ordinary_timeout& v) {
    if (auto strong = actor_cast<strong_actor_ptr>(v.self)) {
      clock.set_ordinary_timeout(v.t, actor_cast<abstract_actor*>(strong),
                                 strong, v.type, v.id);
    }
  }

  void operator()(set_request_timeout& v) {
    if (auto strong = actor_cast<strong_actor_ptr>(v.self)) {
      clock.set_request_timeout(v.t, actor_cast<abstract_actor*>(strong),
                                strong, v.id);
    }
  }

  void operator()(cancel_ordinary_timeout& v) {
    clock.cancel_ordinary_timeout(v.self, v.type);
  }

  void operator()(cancel_request_timeout& v) {
    clock.cancel_request_timeout(v.self, v.id);
  }

  void operator()(cancel_timeouts& v) {
    clock.cancel_timeouts(v.self);
  }

  void operator()(schedule_message& v) {
    clock.schedule_message(v.t, std::move(v.receiver), std::move(v.content));
  }

  void operator()(schedule_message_group& v) {
    clock.schedule_message(v.t, std::move(v.target), std::move(v.sender),
                           std::move(v.content));
  }

  void operator()(set_multi_timeout& v) {
    if (auto strong = actor_cast<strong_actor_ptr>(v.self)) {

      clock.set_multi_timeout(v.t, actor_cast<abstract_actor*>(strong), strong,
                              v.type, v.id);
    }
  }

  void operator()(cancel_all&) {
    clock.cancel_all();
  }
};

void __attribute__((noinline)) thread_safe_actor_clock::pumpMessages() {
  Messages::visitor aVisitor{realClock_};
  tempBuffer_.clear();

  {
    // only lock while swapping the buffers.
    guard_type guard{mutex_};
    std::swap(messages_, tempBuffer_);
  }

  for (auto& val : tempBuffer_) {
    visit(aVisitor, val);
  }
  tempBuffer_.clear();
}

void thread_safe_actor_clock::run_dispatch_loop() {
  simple_actor_clock::visitor f{&realClock_};
  while (done_ == false) {

    // will take the lock for a very short time.
    pumpMessages();

    if (realClock_.schedule().empty()) {
      std::unique_lock<std::mutex> guard{mutex_};
      cv_.wait(guard);
    } else {
      auto untilDate = realClock_.schedule().begin()->first;
      std::unique_lock<std::mutex> guard{mutex_};
      cv_.wait_until(guard, untilDate);
    }
    // Double-check whether schedule is non-empty and execute it.
    if (!realClock_.schedule().empty()) {
      auto t = now();
      auto i = realClock_.schedule().begin();
      while (i != realClock_.schedule().end() && i->first <= t) {
        visit(f, i->second);
        i = realClock_.schedule().erase(i);
      }
    }
  }
  realClock_.schedule().clear();
}

void thread_safe_actor_clock::cancel_dispatch_loop() {
  guard_type guard{mutex_};
  done_ = true;
  cv_.notify_all();
}

} // namespace detail
} // namespace caf
