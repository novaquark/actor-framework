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

#pragma once

#include <string>

#include "caf/config.hpp"

namespace caf {
  namespace tracing {
    std::string getCurrentContext();
    void setContext(std::string const& context);
    void resetContext();
    void openTrace(std::string const& name);
    void closeTrace();
    void setTraceNamePrefix(std::string const& prefix);
    void resetTraceNamePrefix();

    class ScopedContext {
    public:
      ScopedContext(std::string const& context) {
        setContext(context);
      }
      ~ScopedContext() {
        resetContext();
      }
    };
    class ScopedTrace {
    public:
      ScopedTrace(std::string const& name) {
        openTrace(name);
      }
      ~ScopedTrace() {
        closeTrace();
      }
    };
    class ScopedNamePrefix {
    public:
      ScopedNamePrefix(std::string const& name) {
        setTraceNamePrefix(name);
      }
      ~ScopedNamePrefix() {
        resetTraceNamePrefix();
      }
    };
  }
}