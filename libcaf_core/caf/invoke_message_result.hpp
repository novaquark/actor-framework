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

#ifndef CAF_INVOKE_MESSAGE_RESULT_HPP
#define CAF_INVOKE_MESSAGE_RESULT_HPP

#include <string>

namespace caf {

enum invoke_message_result {
  im_success,
  im_skipped,
  im_dropped
};

inline std::string to_string(invoke_message_result x) {
  return x == im_success ? "im_success"
                         : (x == im_skipped ? "im_skipped" : "im_dropped" );
}

} // namespace caf

#endif // CAF_INVOKE_MESSAGE_RESULT_HPP

