#pragma once

#include <string>

#include "noncopyable.h"

// 日志宏定义LOG_INFO("%s %d", arg1, arg2);
// 每行结尾加 \ 是为了让 整个宏在预处理阶段被识别为一条连续的语句。
#define LOG_INFO(LogmsgFormat, ...) \
    do { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(LOG_INFO); \
        char buffer[1024]; \
        snprintf(buffer, sizeof(buffer), LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buffer); \
    } while (0)

#define LOG_DEBUG(LogmsgFormat, ...) \
    do { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(LOG_DEBUG); \
        char buffer[1024]; \
        snprintf(buffer, sizeof(buffer), LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buffer); \
    } while (0)

#define LOG_WARN(LogmsgFormat, ...) \
    do { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(LOG_WARN); \
        char buffer[1024]; \
        snprintf(buffer, sizeof(buffer), LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buffer); \
    } while (0)

#define LOG_ERROR(LogmsgFormat, ...) \
    do { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(LOG_ERROR); \
        char buffer[1024]; \
        snprintf(buffer, sizeof(buffer), LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buffer); \
    } while (0)
// 致命错误，退出程序
#define LOG_FATAL(LogmsgFormat, ...) \
    do { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(LOG_FATAL); \
        char buffer[1024]; \
        snprintf(buffer, sizeof(buffer), LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buffer); \
        exit(1); \
    } while (0)

// 定义日志级别
enum LogLevel {
    LOG_DEBUG,  // 调试信息
    LOG_INFO,   // 一般信息
    LOG_WARN,   // 警告信息
    LOG_ERROR,  // 错误信息
    LOG_FATAL   // 致命错误
};

// 日志类
class Logger : public Noncopyable {
public:
    // 获取日志唯一的实例对象
    static Logger& instance();
    // 设置日志级别
    void setLogLevel(LogLevel level);
    // 写日志
    void log(std::string message);
private:
    int logLevel_;  // 日志级别
private:
    Logger(){}
};
        