#include "Logger.hpp"

#include <exception>
#include <iostream>
#include <optional>
#include <utility>

Logger::Logger() {}

Logger::~Logger() { this->stop(); }

void Logger::thread_loop()
{
    while (!bExit) {
        try {
            qMsg.waitTimeout<100>();
            for (unsigned i = 0; i < qBars.size() - newBars; i++) std::cout << "\r\033[2K\033[A";
            newBars = 0;
            while (!qMsg.empty()) {
                auto i = qMsg.pop_front();
                if (i) std::cout << "\33[2K" << *i << "\e[0m" << std::endl;
            }
            for (size_t i = 0; i < qBars.size(); i++) { qBars.at(i)->update(std::cout); }
        } catch (const std::exception &e) {
            std::cout << "LOGGER ERROR:" << e.what() << std::endl;
        }
    }
}

void Logger::start() { msgT = std::thread(&Logger::thread_loop, this); }

void Logger::stop()
{
    this->flush();
    bExit = true;
    qMsg.setWaitMode(false);
    if (msgT.joinable()) { msgT.join(); }
}

void Logger::flush()
{
    std::unique_lock<std::mutex> lBuffers(mutBuffer);

    for (auto &[_, i]: mBuffers) {
        std::string msg(i.str());
        if (!msg.empty()) std::cout << msg << std::endl;
        i = std::stringstream();
    }
}

std::shared_ptr<ProgressBar> Logger::newProgressBar(const std::string &message, uint64_t uMax)
{
    auto p = std::make_shared<ProgressBar>(message, uMax);

    qBars.push_back(p);
    newBars += 1;
    return p;
}

void Logger::deleteProgressBar(const std::shared_ptr<ProgressBar> &bar) { std::erase(qBars, bar); }

void Logger::endl()
{
    std::string msg(this->raw().str());
    qMsg.push_back(msg);
    mBuffers.at(std::this_thread::get_id()) = std::stringstream();
}

std::stringstream &Logger::warn(const std::string &msg)
{
    auto &buf = this->raw();
    buf << BRACKETS(33, "WARN") << BRACKETS(33, msg);
    return buf;
}

std::stringstream &Logger::err(const std::string &msg)
{
    auto &buf = this->raw();
    buf << BRACKETS(31, "ERROR") << BRACKETS(31, msg);
    return buf;
}

std::stringstream &Logger::info(const std::string &msg)
{
    auto &buf = this->raw();
    buf << BRACKETS(0, "INFO") << BRACKETS(0, msg);
    return buf;
}

std::stringstream &Logger::debug(const std::string &msg)
{
    auto &buf = this->raw();
    buf << BRACKETS(36, "DEBUG") << BRACKETS(36, msg);
    return buf;
}

std::stringstream &Logger::msg(const std::string &msg)
{
    auto &buf = this->raw();
    buf << BRACKETS(0, msg);
    return buf;
}

std::stringstream &Logger::raw()
{
    if (!mBuffers.contains(std::this_thread::get_id())) {
        std::unique_lock<std::mutex> lBuffers(mutBuffer);
        mBuffers.insert({std::this_thread::get_id(), std::stringstream()});
    }
    return mBuffers.at(std::this_thread::get_id());
}
