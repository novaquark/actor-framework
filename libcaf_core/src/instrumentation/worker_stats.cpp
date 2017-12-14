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

#include "caf/instrumentation/worker_stats.hpp"

namespace caf {
namespace instrumentation {

void callsite_stats::record_pre_behavior(int64_t mb_wait_time, size_t mb_size)
{
  mailbox_wait_times_.record(mb_wait_time);
  mailbox_sizes_.record(mb_size);
}

std::string callsite_stats::to_string() const
{
  return std::string("WAIT ") + mailbox_wait_times_.to_string()
         + " | " + " SIZE " + mailbox_sizes_.to_string();
}

void worker_stats::record_pre_behavior(actortype_id at, callsite_id cs, int64_t mb_wait_time, size_t mb_size)
{
  callsite_stats_[at][cs].record_pre_behavior(mb_wait_time, mb_size);
}

std::string worker_stats::to_string() const
{
//  return "TEST";
    std::string res;
    for (const auto& by_actor : callsite_stats_) {
      for (const auto& by_callsite : by_actor.second) {
        res += "ACTOR " + registry_.identify_actortype(by_actor.first);
        res += " CALLSITE " + registry_.identify_signature(by_callsite.first);
        res += " => " + by_callsite.second.to_string();
        res += "\n";
      }
    }
    return res;
}

} // namespace instrumentation
} // namespace caf
