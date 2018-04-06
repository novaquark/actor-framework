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

#ifndef CAF_MESSAGE_METADATA_HPP
#define CAF_MESSAGE_METADATA_HPP

#include <memory>
#include <atomic>
#include <utility>
#include <iostream>

#include <opentracing/tracer.h>

namespace caf {

/// Stores additional data that is transmitted along messages.
struct message_metadata {
  uint64_t                           id;
  std::shared_ptr<opentracing::Span> span;

  void swap(message_metadata& other) {
    std::swap(id, other.id);
    span.swap(other.span);
  }
};

inline message_metadata metadata_new() {
  static uint64_t next_id = 1;
  return message_metadata{next_id++, nullptr};
}

inline std::ostream& operator<<(std::ostream& s, const message_metadata& p) {
  s << "[metadata #" << std::to_string(p.id);
  if (p.span) {
    s << " with span " << p.span;
  }
  s << "]";
  return s;
}

} // namespace caf

#endif // CAF_MESSAGE_METADATA_HPP
