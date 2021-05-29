#pragma once

#include "ThreadedQ.hpp"
#include <atomic>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <unordered_map>

#define S1(x) #x
#define S2(x) S1(x)
#define LOCATION __FILE__ ":" S2(__LINE__)
#define BRACKETS(c, s) "[\e[" << c << "m" << s << "\e[0m] "

class Logger
{
public:
    Logger();
    ~Logger();
    void start();
    void stop();
    void flush();

    std::stringstream &warn(const std::string &msg = "WARNING");
    std::stringstream &err(const std::string &msg = "ERROR");
    std::stringstream &info(const std::string &msg = "INFO");
    std::stringstream &debug(const std::string &msg = "DEBUG");
    std::stringstream &msg(const std::string &msg = "MESSAGE");
    std::stringstream &raw();
    void endl();

private:
    void thread_loop();
    std::mutex mutBuffer;
    std::atomic_bool bExit = false;
    std::thread msgT;
    ThreadedQ<std::string> qMsg;
    std::unordered_map<std::thread::id, std::stringstream> mBuffers;
};

extern Logger *logger;

#define LOGGER_WARN logger->warn(LOCATION)
#define LOGGER_ERR logger->err(LOCATION)
#define LOGGER_INFO logger->info(LOCATION)
#define LOGGER_DEBUG logger->debug(LOCATION)
#define LOGGER_ENDL logger->endl();
