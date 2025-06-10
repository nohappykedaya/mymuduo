#pragma once
#include "nocopyable.h"

#include <string>

// // LOG_INFO("%s %d",arg1,arg2);
// #define LOG_INFO(logmsgFormat, ...)                       \
//     do                                                    \
//     {                                                     \
//         Logger &logger = Logger::instance();              \
//         logger.setLogLevel(INFO);                         \
//         char buf[1024] = {0};                             \
//         snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
//     } while (0)

// #define LOG_ERROR(logmsgFormat, ...)                      \
//     do                                                    \
//     {                                                     \
//         Logger &logger = Logger::instance();              \
//         logger.setLogLevel(ERROR);                        \
//         char buf[1024] = {0};                             \
//         snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
//     } while (0)

// #define LOG_FATAL(LogmsgFormat, ...)                      \
//     do                                                    \
//     {                                                     \
//         Logger &logger = Logger::instance();              \
//         logger.setLogLevel(FATAL);                       \
//         char buf[1024] = {0};                             \
//         snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__); \
//         exit(-1); \
//     } while (0)
// #ifdef  MUDEBUG
// #define LOG_DEBUG(LogmsgFormat, ...)                      \
//     do                                                    \
//     {                                                     \
//         Logger &logger = Logger::instance();              \
//         logger.setLogLevel(DEBUG);                        \
//         char buf[1024] = {0};                             \
//         snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__); \
//     } while (0)
// #else
//     #define LOG_DEBUG(LogmsgFormat,...)
// #endif

// // 定义日志的级别 INFO  ERROR FATAL  DEBUG
// enum LogLevel
// {
//     INFO,  // 普通信息
//     ERROR, // 错误信息
//     FATAL, // core信息
//     DEBUG, // 调试信息
// };
// LOG_INFO("%s %d", arg1, arg2)
#define LOG_INFO(logmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(INFO); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0) 

#define LOG_ERROR(logmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(ERROR); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0) 

#define LOG_FATAL(logmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(FATAL); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
        exit(-1); \
    } while(0) 

#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(DEBUG); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0) 
#else
    #define LOG_DEBUG(logmsgFormat, ...)
#endif

// 定义日志的级别  INFO  ERROR  FATAL  DEBUG 
enum LogLevel
{
    INFO,  // 普通信息
    ERROR, // 错误信息
    FATAL, // core信息
    DEBUG, // 调试信息
};

// 输出一个日志类
class Logger : nocopyable
{
public:
    // 获取日志的唯一实例对象
    static Logger &instance();
    // 设置日志级别
    void setLogLevel(int LeveL);
    // 写日志
    void log(std::string msg);

private:
    int loglevel_;
    Logger() {}
};
