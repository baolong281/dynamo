#pragma once

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include <iostream>

enum class LogLevel { DEBUG, INFO, WARN, ERROR };

class Logger {
public:
  static Logger &instance() {
    static Logger inst{};
    return inst;
  }

  void log(LogLevel lvl, const std::string &msg) {
    std::ostringstream oss;
    oss << timestamp() << " [thread-" << threadId() << "] [" << levelToStr(lvl)
        << "] " << msg << '\n';

    {
      std::lock_guard<std::mutex> lock(mu_);
      std::cerr << oss.str();
    }
  }

  void debug(const std::string &msg) { log(LogLevel::DEBUG, msg); }
  void info(const std::string &msg) { log(LogLevel::INFO, msg); }
  void warn(const std::string &msg) { log(LogLevel::WARN, msg); }
  void error(const std::string &msg) { log(LogLevel::ERROR, msg); }

private:
  Logger() = default;
  std::mutex mu_;
  std::atomic<LogLevel> level_{LogLevel::INFO};

  static std::string timestamp() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    std::time_t t = system_clock::to_time_t(now);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&t));
    char out[64];
    std::snprintf(out, sizeof(out), "%s.%03lld", buf, (long long)ms.count());
    return out;
  }

  static std::string threadId() {
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    return oss.str();
  }

  static const char *levelToStr(LogLevel lvl) {
    switch (lvl) {
    case LogLevel::DEBUG:
      return "DEBUG";
    case LogLevel::INFO:
      return "INFO";
    case LogLevel::WARN:
      return "WARN";
    case LogLevel::ERROR:
      return "ERROR";
    default:
      return "?";
    }
  }
};
