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

#include "caf/serializer.hpp"
#include "caf/deserializer.hpp"

namespace caf {

enum class metadata_state {
  Unknown,
  Deserialized,
  Initialized,
  Finished
};

    /// Stores additional data that is transmitted along messages.
class message_metadata {
public:

  static message_metadata root(const std::string& span_name);

  static message_metadata subspan(const message_metadata& parent, const std::string& span_name);

  uint64_t                           id = 0;                          // for debug purposes
  std::shared_ptr<opentracing::Span> span;
  metadata_state                     state = metadata_state::Unknown; // not serialized
  std::shared_ptr<std::string>       name;                            // not serialized, for debug purposes

  inline operator bool() const {
      return static_cast<bool>(span);
  }

  // TODO move to cpp
  void finish() {
    if (span && (state == metadata_state::Deserialized || state == metadata_state::Initialized)) {
      span->Finish();
      state = metadata_state::Finished;
    }
  }

  // TODO move to cpp
  template <typename T>
  void log(std::string&& key, T&& value) {
    if (span) {
      span->Log({{std::move(key), opentracing::Value(std::forward<T>(value))}});
    }
  }

  void swap(message_metadata& other) {
    std::swap(id, other.id);
    span.swap(other.span);
    std::swap(state, other.state);
  }
};

std::ostream& operator<<(std::ostream& s, const message_metadata& p);

error inspect(serializer& sink, message_metadata& meta);

error inspect(deserializer& source, message_metadata& meta);

} // namespace caf

#endif // CAF_MESSAGE_METADATA_HPP
