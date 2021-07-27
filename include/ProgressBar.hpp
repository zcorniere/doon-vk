#pragma once

#include <chrono>
#include <ostream>

class ProgressBar
{
public:
    ProgressBar(std::ostream &out_, std::string _message = "", int width_ = 40, char fill_ = '#', char space_ = '.',
                bool show_time_ = true);
    ~ProgressBar();

    void update(uint64_t cur, uint64_t total);

private:
    void writeTime(std::ostream &out, std::chrono::duration<float> dur);

private:
    std::string message;
    std::ostream &out;
    uint64_t uWidth;
    char fill;
    char equal = '>';
    char space;
    bool bShowTime = false;
    std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
};
