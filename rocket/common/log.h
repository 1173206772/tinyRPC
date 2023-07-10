#ifndef ROCKET_COMMON_LOG_H
#define ROCKET_COMMON_LOG_H
#include <string>
#include <queue>
#include <memory>
#include "rocket/common/mutex.h"

namespace rocket {

template<typename... Args>
std::string formatString(const char* str, Args&&... args) {

  int size = snprintf(nullptr, 0, str, args...);

  std::string result;
  if (size > 0) {
    result.resize(size);
    snprintf(&result[0], size + 1, str, args...);
  }

  return result;
}

#define DEBUGLOG(str, ...)\        
  if(rocket::Logger::GetGlobalLogger()->getLogLevel() >= rocket::LogLevel::Debug) {                                         \                                                                                                     
    rocket::Logger::GetGlobalLogger()->pushLog((new rocket::LogEvent(rocket::LogLevel::Debug))->toString() + rocket::formatString(str, ##__VA_ARGS__)+"\n");                                                                         \
    rocket::Logger::GetGlobalLogger()->log();    }                                                                        \

#define INFOLOG(str, ...)\
  if(rocket::Logger::GetGlobalLogger()->getLogLevel() >= rocket::LogLevel::Info) {                                         \
    rocket::Logger::GetGlobalLogger()->pushLog((new rocket::LogEvent(rocket::LogLevel::Info))->toString() + rocket::formatString(str, ##__VA_ARGS__)+"\n");                                                                         \
    rocket::Logger::GetGlobalLogger()->log();    }                                                                            \

#define ERRORLOG(str, ...)\
  if(rocket::Logger::GetGlobalLogger()->getLogLevel() >= rocket::LogLevel::Error) {                                         \
    rocket::Logger::GetGlobalLogger()->pushLog((new rocket::LogEvent(rocket::LogLevel::Error))->toString() + rocket::formatString(str, ##__VA_ARGS__)+"\n");                                                                         \
    rocket::Logger::GetGlobalLogger()->log();    }                                                                         \

#define UNKNOWNLOG(str, ...)\
  if(rocket::Logger::GetGlobalLogger()->getLogLevel() >= rocket::LogLevel::Unknown) {                                         \
    rocket::Logger::GetGlobalLogger()->pushLog((new rocket::LogEvent(rocket::LogLevel::Unknown))->toString() + rocket::formatString(str, ##__VA_ARGS__)+"\n");                                                                         \
    rocket::Logger::GetGlobalLogger()->log();     }                                                                     \

enum LogLevel {
  Unknown = 0,
  Debug = 1,
  Info = 2,
  Error = 3
};


std::string LogLevelToString(LogLevel level);

LogLevel StringToLogLevel(const std::string& log_level);

class Logger {
public:

  typedef std::shared_ptr<Logger> s_ptr;

  Logger(LogLevel level): m_set_level(level) {
  
  }

  static Logger* GetGlobalLogger();
  static void InitGlobalLogger();
  void pushLog(const std::string& msg);
  void log();

  LogLevel getLogLevel() const {
    return m_set_level;
  }

private:
  LogLevel m_set_level;
  std::queue<std::string> m_buffer;
  // m_file_path/m_file_name_yyyymmdd.1
  
  Mutex m_mutex;

  std::string m_file_name;    // 日志输出文件名字
  std::string m_file_path;    // 日志输出路径
};


class LogEvent {
 public:

  LogEvent(LogLevel level);

  std::string getFileName() const {
    return m_file_name;  
  }

  LogLevel getLogLevel() const {
    return m_level;
  }

  std::string toString();


 private:
  std::string m_file_name;  // 文件名
  int32_t m_file_line;  // 行号
  int32_t m_pid;  // 进程号
  int32_t m_thread_id;  // 线程号

  LogLevel m_level;     //日志级别


};
}
#endif