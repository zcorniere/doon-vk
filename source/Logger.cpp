#include "Logger.hpp"

Logger::Logger() {}

Logger::~Logger() { this->stop(); }

void Logger::thread_loop()
{
    while (!bExit) {
        try {
            qMsg.wait();
            while (!qMsg.empty()) {
                auto i = qMsg.pop_front();
                if (i) { std::cerr << *i << "\e[0m" << std::endl; }
            }
        } catch (const std::exception &e) {
            std::cerr << "LOGGER ERROR:" << e.what() << std::endl;
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
        if (!msg.empty()) std::cerr << msg << std::endl;
        i = std::stringstream();
    }
}

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
    buf << BRACKETS(36, "INFO") << BRACKETS(36, msg);
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
