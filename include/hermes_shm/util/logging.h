//
// Created by lukemartinlogan on 3/31/23.
//

#ifndef HERMES_SHM_INCLUDE_HERMES_SHM_UTIL_LOGGING_H_
#define HERMES_SHM_INCLUDE_HERMES_SHM_UTIL_LOGGING_H_

#include <unistd.h>
#include <climits>

#include <vector>
#include <iostream>
#include <iomanip>
#include <filesystem>
#include "formatter.h"
#include "singleton.h"

namespace hshm {

/**
 * A macro to indicate the verbosity of the logger.
 * Verbosity indicates how much data will be printed.
 * The higher the verbosity, the more data will be printed.
 * */
#ifndef HERMES_LOG_VERBOSITY
#define HERMES_LOG_VERBOSITY 10
#endif
#if HERMES_LOG_VERBOSITY < 0
#error "HERMES_LOG_VERBOSITY cannot be less than 0"
#endif

/** Prints log verbosity at compile time */
#define XSTR(s) STR(s)
#define STR(s) #s
// #pragma message XSTR(HERMES_LOG_VERBOSITY)

/** Simplify access to Logger singleton */
#define HERMES_LOG hshm::EasySingleton<hshm::Logger>::GetInstance()

/** Information Logging levels */
#define kDebug 10  /**< Low-priority debugging information*/
// ... may want to add more levels here
#define kInfo 1    /**< Useful information the user should know */

/** Error Logging Levels */
#define kFatal 0   /**< A fatal error has occurred */
#define kError 1   /**< A non-fatal error has occurred */
#define kWarning 2   /**< Something might be wrong */

/**
 * Hermes Info (HI) Log
 * LOG_LEVEL indicates the priority of the log.
 * LOG_LEVEL 0 is considered required
 * LOG_LEVEL 10 is considered debugging priority.
 * */
#define HILOG(LOG_LEVEL, ...) \
  if constexpr(LOG_LEVEL <= HERMES_LOG_VERBOSITY) { \
    HERMES_LOG->InfoLog(LOG_LEVEL, __func__, __LINE__, __VA_ARGS__); \
  }

/**
 * Hermes Error (HE) Log
 * LOG_LEVEL indicates the priority of the log.
 * LOG_LEVEL 0 is considered required
 * LOG_LEVEL 10 is considered debugging priority.
 * */
#define HELOG(LOG_LEVEL, ...) \
  if constexpr(LOG_LEVEL <= HERMES_LOG_VERBOSITY) { \
    HERMES_LOG->ErrorLog(LOG_LEVEL, __func__, __LINE__, __VA_ARGS__); \
  }

class Logger {
 public:
  std::string exe_path_;
  // std::string exe_name_;
  int verbosity_;

 public:
  Logger() {
    GetExePath();
    // exe_name_ = std::filesystem::path(exe_path_).filename().string();
    int verbosity = kDebug;
    auto verbosity_env = getenv("HERMES_LOG_VERBOSITY");
    if (verbosity_env && strlen(verbosity_env)) {
      try {
        verbosity = std::stoi(verbosity_env);
      } catch (...) {
        verbosity = kDebug;
      }
    }
    SetVerbosity(verbosity);
  }

  void SetVerbosity(int LOG_LEVEL) {
    verbosity_ = LOG_LEVEL;
    if (verbosity_ < 0) {
      verbosity_ = 0;
    }
  }

  template<typename ...Args>
  void InfoLog(int LOG_LEVEL,
           const char *func,
           int line,
           const char *fmt,
           Args&& ...args) {
    if (LOG_LEVEL > verbosity_) { return; }
    std::string msg =
      hshm::Formatter::format(fmt, std::forward<Args>(args)...);
    int tid = gettid();
    std::cerr << hshm::Formatter::format(
      "{}:{} {} {} {}\n",
      exe_path_, line, tid, func, msg);
  }

  template<typename ...Args>
  void ErrorLog(int LOG_LEVEL,
               const char *func,
               int line,
               const char *fmt,
               Args&& ...args) {
    InfoLog(LOG_LEVEL, func, line, fmt, std::forward<Args>(args)...);
    if (LOG_LEVEL == kFatal) {
      exit(1);
    }
  }

 private:
  void GetExePath() {
    std::vector<char> exe_path(PATH_MAX, 0);
    if (readlink("/proc/self/exe", exe_path.data(), PATH_MAX - 1) == -1) {
      std::cerr
        << "Could not introspect the name of this program for logging"
        << std::endl;
    }
    exe_path_ = std::string(exe_path.data());
  }
};

}  // namespace hshm

#endif //HERMES_SHM_INCLUDE_HERMES_SHM_UTIL_LOGGING_H_