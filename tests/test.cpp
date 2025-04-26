#include <iostream>
#include "server/log.h"
#include "server/util.h"

int main(int argc, char** argv){

    server::Logger::ptr logger(new server::Logger);
    logger->addAppender(server::LogAppender::ptr(new server::StdoutLogAppender));

    server::FileLogAppender::ptr file_apperder(new server::FileLogAppender("./log.txt"));
    server::LogFormatter::ptr fmt(new server::LogFormatter("%d%T%p%T%m%n"));
    file_apperder->setFormatter(fmt);
    file_apperder->setLevel(server::LogLevel::ERROR);
    logger->addAppender(file_apperder);
    
    // server::LogEvent::ptr event(new server::LogEvent(__FILE__, __LINE__, 0, server::GetThreadId(), server::GetFiberId(), time(0)));
    // event->getSS() << "hello log";
    // logger->log(server::LogLevel::DEBUG, event);
 
    SERVER_LOG_INFO(logger) << "test info";
    SERVER_LOG_ERROR(logger) << "test error";

    SERVER_LOG_FMT_ERROR(logger, "test fmt error %s", "aaa");

    auto l = server::LoggerMgr::GetInstance()->getLogger("xx");
    SERVER_LOG_INFO(l) << "xxx";

    return 0;
}