#ifndef LINKNET_LOGGER_H_
#define LINKNET_LOGGER_H_

#include <string>
#include <iostream>
#include <fstream>
#include <mutex>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace linknet {

enum class LogLevel {
  DEBUG,
  INFO,
  WARNING,
  ERROR,
  FATAL
};

class Logger {
 public:
  static Logger& GetInstance() {
    static Logger instance;
    return instance;
  }

  void SetLogLevel(LogLevel level) {
    _log_level = level;
  }

  void SetLogFile(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_log_file.is_open()) {
      _log_file.close();
    }
    _log_file.open(filepath, std::ios::app);
  }

  template<typename... Args>
  void Log(LogLevel level, const char* file, int line, Args&&... args) {
    if (level < _log_level) {
      return;
    }

    std::stringstream message;
    FormatLog(message, std::forward<Args>(args)...);
    
    // Get current time
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
        
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << now_ms.count();
    ss << " [" << LevelToString(level) << "] "
       << file << ":" << line << " - " 
       << message.str();

    std::string log_entry = ss.str();
    
    {
      std::lock_guard<std::mutex> lock(_mutex);
      std::cout << log_entry << std::endl;
      
      if (_log_file.is_open()) {
        _log_file << log_entry << std::endl;
      }
    }
  }

 private:
  Logger() : _log_level(LogLevel::INFO) {}
  ~Logger() {
    if (_log_file.is_open()) {
      _log_file.close();
    }
  }
  
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  template<typename T>
  void FormatLog(std::stringstream& ss, T&& arg) {
    ss << std::forward<T>(arg);
  }

  template<typename T, typename... Args>
  void FormatLog(std::stringstream& ss, T&& arg, Args&&... args) {
    ss << std::forward<T>(arg);
    FormatLog(ss, std::forward<Args>(args)...);
  }

  const char* LevelToString(LogLevel level) {
    switch (level) {
      case LogLevel::DEBUG: return "DEBUG";
      case LogLevel::INFO: return "INFO";
      case LogLevel::WARNING: return "WARNING";
      case LogLevel::ERROR: return "ERROR";
      case LogLevel::FATAL: return "FATAL";
      default: return "UNKNOWN";
    }
  }

  LogLevel _log_level;
  std::mutex _mutex;
  std::ofstream _log_file;
};

}  // namespace linknet

// Convenience macros for logging
#define LOG_DEBUG(...) \
    linknet::Logger::GetInstance().Log(linknet::LogLevel::DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...) \
    linknet::Logger::GetInstance().Log(linknet::LogLevel::INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARNING(...) \
    linknet::Logger::GetInstance().Log(linknet::LogLevel::WARNING, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) \
    linknet::Logger::GetInstance().Log(linknet::LogLevel::ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_FATAL(...) \
    linknet::Logger::GetInstance().Log(linknet::LogLevel::FATAL, __FILE__, __LINE__, __VA_ARGS__)

#endif  // LINKNET_LOGGER_H_
