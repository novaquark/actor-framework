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

#include "caf/message_metadata.hpp"

#include <string>
#include <iomanip>

#include "caf/sec.hpp"

namespace caf {

static message_metadata metadata_gen() {
  static std::atomic<uint64_t> next_id{1};
  message_metadata result;
  result.id = next_id++;
  result.state = metadata_state::Unknown;
  return result;
}

message_metadata message_metadata::root(const std::string& span_name) {
  message_metadata result = metadata_gen();
  result.name = std::make_shared<std::string>(span_name);
  {
    auto tracer = opentracing::Tracer::Global();
    result.span = tracer->StartSpan(span_name);
  }
  result.state = metadata_state::Initialized;
  return result;
}

message_metadata message_metadata::subspan(const message_metadata& parent, const std::string& span_name) {
  message_metadata result = metadata_gen();
  result.name = std::make_shared<std::string>(span_name);
  if (parent) {
    const auto& tracer = parent.span->tracer();
    result.span = tracer.StartSpan(span_name, {opentracing::ChildOf(&parent.span->context())});
  } else {
    auto tracer = opentracing::Tracer::Global();
    result.span = tracer->StartSpan(span_name);
  }
  result.state = metadata_state::Initialized;
  return result;
}

// TODO TEMP
static inline std::string ToHex(const std::string& s)
{
  std::ostringstream ret;

  for (std::string::size_type i = 0; i < s.length(); ++i)
    ret << std::hex << std::setfill('0') << std::setw(2) << std::nouppercase << (int)s[i];

  return ret.str();
}

std::ostream& operator<<(std::ostream& s, const message_metadata& p) {
  s << "[metadata #" << std::to_string(p.id) << " state=";
  switch (p.state) {
    case metadata_state::Deserialized: s << "Deserialized"; break;
    case metadata_state::Initialized:  s << "Initialized";  break;
    case metadata_state::Finished:     s << "Finished";     break;
    default:                           s << "Unknown";      break;
  }
  return s << "]";
}

error inspect(serializer& sink, message_metadata& meta) {
  std::cout << "Serializing " << meta << std::endl;
  if (meta.span && meta.id > 0) {
    sink(meta.id);
    const auto& tracer = meta.span->tracer();
    auto& context = meta.span->context();
    std::ostringstream os;
    tracer.Inject(context, os);
    std::string encoded_span = os.str();
    std::cout << "   Encoded span is: " << ToHex(encoded_span) << std::endl;
    sink(encoded_span);
  } else {
    sink((uint64_t) 0);
  }
  return none;
}

error inspect(deserializer& source, message_metadata& meta) {
  uint64_t metadata_id;
  error err = source(metadata_id);
  if (err)
    return err;
  std::cout << "Deserializing metadata #" << metadata_id << std::endl;
  if (metadata_id > 0) {
    std::string encoded_span;
    source(encoded_span);
    std::cout << "   Encoded span is: " << ToHex(encoded_span) << std::endl;
    std::istringstream encoded_span_stream{encoded_span};
    const auto& tracer = opentracing::Tracer::Global();
    auto context = tracer->Extract(encoded_span_stream);
    if (context.has_value()) {
      auto span = tracer->StartSpan("", {opentracing::ChildOf(context.value().get())});
      meta.id = metadata_id;
      meta.span = std::move(span);
      meta.state = metadata_state::Deserialized;
    } else {
      std::cout << "   ERROR! could not extract context from the network data" << std::endl;
      return sec::unknown_type; // TODO better error?
    }
  }
  return none;
}

} // namespace caf
