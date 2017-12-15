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

#ifndef CAF_WORKER_STATS_HPP
#define CAF_WORKER_STATS_HPP

#include "caf/instrumentation/instrumentation_ids.hpp"
#include "caf/instrumentation/callsite_stats.hpp"
#include "caf/instrumentation/name_registry.hpp"
#include "caf/instrumentation/metric.hpp"

#include <unordered_map>
#include <cstdint>
#include <string>
#include <mutex>

namespace caf {
namespace instrumentation {

/// Instrumentation stats aggregated per-worker for all callsites.
class worker_stats {
public:
  void record_pre_behavior(actortype_id at, callsite_id cs, int64_t mb_wait_time, size_t mb_size);
  std::vector<metric> collect_metrics();
  std::string to_string() const;

  name_registry& registry() {
    return registry_;
  }

private:
  std::mutex access_mutex_;
  std::unordered_map<actortype_id, std::unordered_map<callsite_id, callsite_stats>> callsite_stats_; // indexed by actortype_id and then callsite_id
  name_registry registry_;
};

} // namespace instrumentation
} // namespace caf

#endif // CAF_WORKER_STATS_HPP
