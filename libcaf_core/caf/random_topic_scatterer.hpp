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

#ifndef CAF_RANDOM_TOPIC_SCATTERER_HPP
#define CAF_RANDOM_TOPIC_SCATTERER_HPP

#include <map>
#include <tuple>
#include <deque>
#include <vector>
#include <functional>

#include "caf/topic_scatterer.hpp"

namespace caf {

/// A topic scatterer that delivers data to sinks in random order.
template <class T, class Filter, class Select>
class random_topic_scatterer
    : public topic_scatterer<T, Filter, Select> {
public:
  using super = topic_scatterer<T, Filter, Select>;

  random_topic_scatterer(local_actor* selfptr) : super(selfptr) {
    // nop
  }

  long credit() const override {
    // We receive messages until we have exhausted all downstream credit and
    // have filled our buffer to its minimum size.
    return this->total_credit();
  }

  void emit_batches() override {
    CAF_LOG_TRACE("");
    this->fan_out();
    for (auto& kvp : this->lanes_) {
      auto& l = kvp.second;
      super::sort_by_credit(l.paths);
      for (auto& x : l.paths) {
        auto hint = x->desired_batch_size;
        auto next_chunk = [&] {
          return super::get_chunk(l.buf, std::min(x->open_credit, hint));
        };
        for (auto chunk = next_chunk(); !chunk.empty(); chunk = next_chunk()) {
          auto csize = static_cast<long>(chunk.size());
          x->emit_batch(csize, make_message(std::move(chunk)));
        }
      }
    }
  }

  long desired_batch_size() const override {
    // TODO: this is an O(n) computation, consider storing the result in a
    //       member variable for
    return super::total_desired_batch_size();
  }
};

} // namespace caf

#endif // CAF_RANDOM_TOPIC_SCATTERER_HPP
