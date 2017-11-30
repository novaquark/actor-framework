/******************************************************************************
 *                       ____    _    _____                                   *
 *                      / ___|  / \  |  ___|    C++                           *
 *                     | |     / _ \ | |_       Actor                         *
 *                     | |___ / ___ \|  _|      Framework                     *
 *                      \____/_/   \_|_|                                      *
 *                                                                            *
 * Copyright (C) 2011 - 2017                                                  *
 * Dominik Charousset <dominik.charousset (at) haw-hamburg.de>                *
 *                                                                            *
 * Distributed under the terms and conditions of the BSD 3-Clause License or  *
 * (at your option) under the terms and conditions of the Boost Software      *
 * License 1.0. See accompanying files LICENSE and LICENSE_ALTERNATIVE.       *
 *                                                                            *
 * If you did not receive a copy of the license files, see                    *
 * http://opensource.org/licenses/BSD-3-Clause and                            *
 * http://www.boost.org/LICENSE_1_0.txt.                                      *
 ******************************************************************************/

#include "caf/stream_aborter.hpp"

#include "caf/actor.hpp"
#include "caf/actor_cast.hpp"
#include "caf/downstream_msg.hpp"
#include "caf/logger.hpp"
#include "caf/message.hpp"
#include "caf/system_messages.hpp"
#include "caf/upstream_msg.hpp"

namespace caf {

stream_aborter::~stream_aborter() {
  // nop
}

void stream_aborter::actor_exited(const error& rsn, execution_unit* host) {
  CAF_ASSERT(observed_ != observer_);
  auto observer = actor_cast<strong_actor_ptr>(observer_);
  if (observer != nullptr)
  {
    if (mode_ == source_aborter)
      observer->enqueue(nullptr, make_message_id(),
                        make_message(caf::make<downstream_msg::forced_close>(
                          slot_, observed_, rsn)),
                        host);
    else
      observer->enqueue(nullptr, make_message_id(),
                        make_message(caf::make<upstream_msg::forced_drop>(
                          slot_, observed_, rsn)),
                        host);
  }
}

bool stream_aborter::matches(const attachable::token& what) {
  if (what.subtype != attachable::token::stream_aborter)
    return false;
  auto& ot = *reinterpret_cast<const token*>(what.ptr);
  return ot.observer == observer_ && ot.slot == slot_;
}

stream_aborter::stream_aborter(actor_addr&& observed, actor_addr&& observer,
                               stream_slot slot, mode m)
    : observed_(std::move(observed)),
      observer_(std::move(observer)),
      slot_(slot),
      mode_(m) {
  // nop
}

void stream_aborter::add(strong_actor_ptr observed, actor_addr observer,
                         stream_slot slot, mode m) {
  CAF_LOG_TRACE(CAF_ARG(observed) << CAF_ARG(observer) << CAF_ARG(slot));
  auto ptr = make_stream_aborter(observed->address(), std::move(observer),
                                 slot, m);
  observed->get()->attach(std::move(ptr));
}

void stream_aborter::del(strong_actor_ptr observed, const actor_addr& observer,
                         stream_slot slot, mode m) {
  CAF_LOG_TRACE(CAF_ARG(observed) << CAF_ARG(observer) << CAF_ARG(slot));
  token tk{observer, slot, m};
  observed->get()->detach(tk);
}

attachable_ptr make_stream_aborter(actor_addr observed, actor_addr observer,
                                   stream_slot slot, stream_aborter::mode m) {
  return attachable_ptr{
    new stream_aborter(std::move(observed), std::move(observer), slot, m)};
}

} // namespace caf
