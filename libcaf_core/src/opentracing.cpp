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

#include <caf/opentracing.hpp>

#ifdef CAF_ENABLE_OPENTRACING
#include <iostream> // debug
#include <sstream>
#include <opentracing/span.h>
#include <opentracing/tracer.h>

namespace caf {
  namespace tracing {

    namespace {
      static thread_local std::string traceNamePrefix;
      static thread_local std::unique_ptr<opentracing::Span> currentSpan;
      static thread_local std::unique_ptr<opentracing::SpanContext> currentContext;
      static TraceAppend traceAppend = TraceAppend::FirstAtom;

      class TextMapStore: public opentracing::TextMapWriter
      {
        public:
            opentracing::expected<void> Set(opentracing::string_view k, opentracing::string_view v) const override
            {
                if ((std::string)k == "uber-trace-id")
                    _str = v;
                return {};
            }
            std::string str() { return _str; }
      private:
          mutable std::string _str;
      };
    }

    void setTraceAppend(TraceAppend append) {
      traceAppend = append;
    }

    TraceAppend getTraceAppend() {
      return traceAppend;
    }

    std::string getCurrentContext() {
      auto cs = currentSpan.get();
      if (cs) {
        std::stringstream ss;
        opentracing::Tracer::Global()->Inject(cs->context(), ss);
        //std::cerr << "got context from span " << ss.str().size() << std::endl;
        return ss.str();
      }
      else {
        auto sc = currentContext.get();
        if (!sc)
          return std::string();
        std::stringstream ss;
        opentracing::Tracer::Global()->Inject(*sc, ss);
        return ss.str();
      }
    }

    std::string getCurrentContextString()
    {
        TextMapStore tms;
        auto cs = currentSpan.get();
        if (cs) {
            opentracing::Tracer::Global()->Inject(cs->context(), tms);
        }
        else {
            auto sc = currentContext.get();
            if (!sc)
                return std::string();
            opentracing::Tracer::Global()->Inject(*sc, tms);
        }
        return tms.str();
    }

    void setContext(std::string const& context) {
      if (context.empty()) {
        //std::cerr << "set an empty context" << std::endl;
        resetContext();
      }
      else {
        //std::cerr << "set a context" << std::endl;
        currentSpan.reset();
        std::stringstream ss(context);
        currentContext.reset(
          opentracing::Tracer::Global()->Extract(ss)->release());
      }
    }

    void resetContext() {
      currentSpan.reset();
      currentContext.reset();
    }

    void openTrace(std::string const& name) {
      auto fullName = traceNamePrefix + name;
      auto ctx = currentContext.get();
      std::unique_ptr<opentracing::Span> span;
      if (ctx)
        span = opentracing::Tracer::Global()->StartSpan(
          fullName, { opentracing::ChildOf(ctx) });
      else
        span = opentracing::Tracer::Global()->StartSpan(fullName);
      std::swap(currentSpan, span);
      //std::cerr << "opentrace " << fullName << " " << !!ctx << std::endl;
    }
    void closeTrace() {
      currentSpan.reset();
    }
    void setTraceNamePrefix(std::string const& prefix) {
      traceNamePrefix = prefix;
    }
    void resetTraceNamePrefix() {
      traceNamePrefix.clear();
    }
  }
}



#else // CAF_ENABLE_OPENTRACING

namespace caf {
  namespace tracing {
    void setTraceAppend(TraceAppend append) {}
    TraceAppend getTraceAppend() {}
    std::string getCurrentContext() {}
    void setContext(std::string const& context) {}
    void resetContext() {}
    void openTrace(std::string const& name) {}
    void closeTrace() {}
    void setTraceNamePrefix(std::string const& prefix) {}
    void resetTraceNamePrefix() {}
  }
}

#endif // CAF_ENABLE_OPENTRACING