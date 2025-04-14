#include "Logger.h"
#include "Timestamp.h"

#include <iostream>

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

void Logger::setLogLevel(LogLevel level) {
    logLevel_ = level;
}

// 写日志 [级别信息] time : msg
void Logger::log(std::string message) {
    // 这里可以实现日志的输出逻辑，比如输出到文件或控制台
    switch (logLevel_)
    {
    case LOG_INFO:
        std::cout << "[INFO] ";
        break;
    case LOG_DEBUG:
        std::cout << "[DEBUG] ";
        break;
    case LOG_WARN:
        std::cout << "[WARN] ";
        break;
    case LOG_ERROR:
        std::cout << "[ERROR] ";
        break;
    case LOG_FATAL:
        std::cout << "[FATAL] ";
        break;
    default:
        break;
    }

    // 打印时间和消息
    std::cout << "time: " << Timestamp::now().toString() << " : " << message << std::endl;
}