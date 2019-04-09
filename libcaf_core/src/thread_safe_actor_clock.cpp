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

namespace caf {
namespace detail {

namespace {

using guard_type = std::unique_lock<std::mutex>;

} // namespace

thread_safe_actor_clock::thread_safe_actor_clock() : done_(false) {
  // nop
}

// only accessed under a lock, but in theory several instance might be used
// TODO: clean that
static std::atomic<int> maxThreadIndex;

static thread_local int currentThreadIndex = -1;

void thread_safe_actor_clock::enqueueInvocation(Pouet::value_type&& i) {
  if (currentThreadIndex <= 0) {
    currentThreadIndex = maxThreadIndex++;
    CAF_ASSERT(currentThreadIndex < MaxThread);
  }
  {
    TLSQueue& queue = kaouest_[currentThreadIndex];
    guard_type guard{queue.mutex};
    queue.messages.push_back(std::move(i));
  }
  {
    // lock or not lock
    // there is a race condition if we don't take the lock.
    cv_.notify_all();
  }
}

void thread_safe_actor_clock::set_ordinary_timeout(time_point t,
                                                   abstract_actor* self,
                                                   atom_value type,
                                                   uint64_t id) {

  enqueueInvocation(Pouet::set_ordinary_timeout{t, self, type, id});
}

void thread_safe_actor_clock::set_request_timeout(time_point t,
                                                  abstract_actor* self,
                                                  message_id id) {
  enqueueInvocation(Pouet::set_request_timeout{t, self, id});
}

void thread_safe_actor_clock::cancel_ordinary_timeout(abstract_actor* self,
                                                      atom_value type) {
  enqueueInvocation(Pouet::cancel_ordinary_timeout{self, type});
}

void thread_safe_actor_clock::cancel_request_timeout(abstract_actor* self,
                                                     message_id id) {
  enqueueInvocation(Pouet::cancel_request_timeout{self, id});
}

void thread_safe_actor_clock::cancel_timeouts(abstract_actor* self) {
  enqueueInvocation(Pouet::cancel_timeouts{self});
}

void thread_safe_actor_clock::schedule_message(time_point t,
                                               strong_actor_ptr receiver,
                                               mailbox_element_ptr content) {
  enqueueInvocation(
    Pouet::schedule_message{t, std::move(receiver), std::move(content)});
}

void thread_safe_actor_clock::schedule_message(time_point t, group target,
                                               strong_actor_ptr sender,
                                               message content) {
  enqueueInvocation(Pouet::schedule_message_group{
    t, std::move(target), std::move(sender), std::move(content)});
}

void thread_safe_actor_clock::set_multi_timeout(time_point t,
                                                abstract_actor* self,
                                                atom_value type, uint64_t id) {
  enqueueInvocation(Pouet::set_multi_timeout{t, self, type, id});
}

void thread_safe_actor_clock::cancel_all() {
  enqueueInvocation(Pouet::cancel_all{});
}

struct thread_safe_actor_clock::Pouet::visitor {
  simple_actor_clock& clock;

  void operator()(set_ordinary_timeout& v) {
    clock.set_ordinary_timeout(v.t, v.self, v.type, v.id);
  }

  void operator()(set_request_timeout& v) {
    clock.set_request_timeout(v.t, v.self, v.id);
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
    clock.set_multi_timeout(v.t, v.self, v.type, v.id);
  }

  void operator()(cancel_all&) {
    clock.cancel_all();
  }
};

void thread_safe_actor_clock::pumpMessages() {
  // called under the lock
  int currentMax = maxThreadIndex.load();
  Pouet::visitor aVisitor{realClock_};
  for (int i = 0; i < currentMax; ++i) {
    // this loop might be a bit expansive, find a way to cut it.
    TLSQueue& queue = kaouest_.at(i);
    {
      // only lock while swapping the buffers.
      guard_type guard{queue.mutex};
      std::swap(queue.messages, queue.tempBuffer);
      queue.messages.clear();
    }

    for (auto& val : queue.tempBuffer) {
      visit(aVisitor, val);
    }
    queue.tempBuffer.clear();
  }
}

void thread_safe_actor_clock::run_dispatch_loop() {
  simple_actor_clock::visitor f{&realClock_};
  guard_type guard{mx_};
  while (done_ == false) {
    // Wait for non-empty schedule.
    // Note: The thread calling run_dispatch_loop() is guaranteed not to lock
    //       the mutex recursively. Otherwise, cv_.wait() or cv_.wait_until()
    //       would be unsafe, because wait operations call unlock() only once.
    pumpMessages();
    if (realClock_.schedule().empty()) {
      cv_.wait(guard);
    } else {
      auto tout = realClock_.schedule().begin()->first;
      cv_.wait_until(guard, tout);
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
  guard_type guard{mx_};
  done_ = true;
  cv_.notify_all();
}

} // namespace detail
} // namespace caf
