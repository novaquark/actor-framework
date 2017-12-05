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

#ifndef CAF_INVALID_STREAM_SCATTERER_HPP
#define CAF_INVALID_STREAM_SCATTERER_HPP

#include "caf/stream_scatterer.hpp"

namespace caf {

/// Type-erased policy for dispatching data to sinks.
class invalid_stream_scatterer : public stream_scatterer {
public:
  invalid_stream_scatterer(local_actor* self);

  ~invalid_stream_scatterer() override;

  void emit_batches() override;

  size_t capacity() const noexcept override;

  size_t buffered() const noexcept override;
};

} // namespace caf

#endif // CAF_INVALID_STREAM_SCATTERER_HPP
