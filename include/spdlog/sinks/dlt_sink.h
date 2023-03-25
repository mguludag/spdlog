// Copyright(c) 2015-present, Gabi Melman, mguludag and spdlog
// contributors. Distributed under the MIT License
// (http://opensource.org/licenses/MIT)

#pragma once

#include "spdlog/common.h"
#include "spdlog/details/log_msg.h"
#include "spdlog/details/synchronous_factory.h"
#include "spdlog/sinks/base_sink.h"

#include <dlt/dlt.h>

namespace spdlog {
namespace sinks {
template <typename Mutex> class dlt_sink : public base_sink<Mutex> {
  class dlt_wrapper {
  public:
    dlt_wrapper(const std::string &ctxid, const std::string description)
        : loglevel_{DLT_LOG_VERBOSE, DLT_LOG_DEBUG, DLT_LOG_INFO,
                    DLT_LOG_WARN,    DLT_LOG_ERROR, DLT_LOG_FATAL,
                    DLT_LOG_DEFAULT},
          errcodes_{"DLT_RETURN_FILESZERR",
                    "DLT_RETURN_LOGGING_DISABLED",
                    "DLT_RETURN_USER_BUFFER_FULL",
                    "DLT_RETURN_WRONG_PARAMETER",
                    "DLT_RETURN_BUFFER_FULL",
                    "DLT_RETURN_PIPE_FULL",
                    "DLT_RETURN_PIPE_ERROR",
                    "DLT_RETURN_ERROR",
                    "DLT_RETURN_OK",
                    "DLT_RETURN_TRUE"} {
      int ret = dlt_register_context(
          &context_, (ctxid.size() > 4 ?.substr(0, 4).c_str() : ctxid.c_str()),
          description.c_str());
      if (ret != DLT_RETURN_OK || ret != DLT_RETURN_TRUE) {
        throw spdlog_ex(errcodes_[8 + ret].data());
      }
    }

    ~dlt_wrapper() { dlt_unregister_context(&context_); }

    void log_dlt(spdlog::level::level_enum logLevel, const char *message) {
      DLT_LOG_STRING(&context_, loglevel_[static_cast<int>(logLevel)], message);
    }

  private:
    static DltContext context_;
    std::array<DltLogLevelType, 7> loglevel_;
    std::array<spdlog::string_view_t, 9> errcodes_;
  };

  DltContext dlt_wrapper::context_{};

public:
  dlt_sink(const std::string &contextId, const std::string &description)
      : wrapper_{contextId, description} {}

  ~dlt_sink() { DLT_UNREGISTER_APP_FLUSH_BUFFERED_LOGS(); }

protected:
  void sink_it_(const details::log_msg &msg) override {
    memory_buf_t formatted;
    base_sink<Mutex>::formatter_->format(msg, formatted);
    string_view_t str = string_view_t(formatted.data(), formatted.size());
    wrapper_.log_dlt(msg.level, str.data());
  }

  void flush_() override {}

private:
  dlt_wrapper wrapper_;
};

#include "spdlog/details/null_mutex.h"
#include <mutex>
using dlt_sink_mt = dlt_sink<std::mutex>;
using dlt_sink_st = dlt_sink<spdlog::details::null_mutex>;
} // namespace sinks

//
// Factory functions
//
template <typename Factory = spdlog::synchronous_factory>
inline std::shared_ptr<logger> dlt_logger_mt(const std::string &logger_name,
                                             const std::string &contextId,
                                             const std::string &description) {
  return Factory::template create<sinks::dlt_sink_mt>(logger_name, contextId,
                                                      description);
}

template <typename Factory = spdlog::synchronous_factory>
inline std::shared_ptr<logger> dlt_logger_st(const std::string &logger_name,
                                             const std::string &contextId,
                                             const std::string &description) {
  return Factory::template create<sinks::dlt_sink_st>(logger_name, contextId,
                                                      description);
}
} // namespace spdlog
