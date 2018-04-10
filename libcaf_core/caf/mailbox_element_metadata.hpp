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

#ifndef CAF_MAILBOX_ELEMENT_METADATA_HPP
#define CAF_MAILBOX_ELEMENT_METADATA_HPP

#include <utility>

#include <opentracing/tracer.h>

#include "caf/message.hpp"

#include "caf/instrumentation/instrumentation_ids.hpp"

namespace caf {

struct mailbox_element_metadata {
public:
  std::unique_ptr<opentracing::Span> span;
};

inline mailbox_element_metadata start_span_for(const message& msg) {
  const auto tracer = opentracing::Tracer::Global();
  const auto span_name = instrumentation::to_string(instrumentation::get_msgtype(msg));
  auto span = tracer->StartSpan(span_name);
  return mailbox_element_metadata{std::move(span)};
}

} // namespace caf

#endif // CAF_MAILBOX_ELEMENT_METADATA_HPP
