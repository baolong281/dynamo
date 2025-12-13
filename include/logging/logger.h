#pragma once

#pragma once

#include <atomic>
#include <mutex>
#include <string>

enum class LogLevel { DEBUG, INFO, WARN, ERROR };

class Logger {
public:
  static Logger &instance() {
    static Logger inst{};
    return inst;
  }

  void log(LogLevel lvl, const std::string &msg);

  void debug(const std::string &msg);
  void info(const std::string &msg);
  void warn(const std::string &msg);
  void error(const std::string &msg);

private:
  Logger() = default;
  std::mutex mu_;
  std::atomic<LogLevel> level_{LogLevel::INFO};

  static std::string timestamp();

  static std::string threadId();

  static const char *levelToStr(LogLevel lvl);
};
