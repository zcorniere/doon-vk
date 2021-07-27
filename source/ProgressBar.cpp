#include "ProgressBar.hpp"
#include <chrono>

ProgressBar::ProgressBar(std::ostream &out_, std::string _message, int width_, char fill_, char space_, bool show_time_)
    : message(_message), out(out_), uWidth(width_), fill(fill_), space(space_), bShowTime(show_time_)
{
}

ProgressBar::~ProgressBar() {}

void ProgressBar::update(uint64_t cur, uint64_t total)
{
    out << "\033[A\033[2K\r" << message << '[';
    uint64_t fills = (int64_t)((float)cur / total * uWidth);
    for (uint64_t i = 0; i < uWidth; i++) {
        if (i < fills) {
            out << fill;
        } else if (i == fills) {
            out << equal;
        } else if (i > fills) {
            out << space;
        }
    }
    out << "] " << cur << '/' << total;

    if (bShowTime) {
        if (cur > 0 && cur < total) {
            // if not first call, output time estimate for remaining time
            out << ' ';
            auto elapsed = (std::chrono::steady_clock::now() - start_time);
            auto estimate = elapsed / cur * (total - cur);
            writeTime(out, estimate);
            out << " remaining; ";
            writeTime(out, elapsed);
            out << " elapsed.";
        } else {
            out << " done";
        }
    }
    out << std::endl;
}

void ProgressBar::writeTime(std::ostream &out, std::chrono::duration<float> dur)
{
    using namespace std::chrono_literals;

    auto old_prec = out.precision();
    out.precision(3);
    if (dur > 1h) {
        out << std::chrono::duration<float, std::ratio<3600>>(dur).count() << "h";
    } else if (dur > 1min) {
        out << std::chrono::duration<float, std::ratio<60>>(dur).count() << "m";
    } else if (dur > 1s) {
        out << std::chrono::duration<float>(dur).count() << "s";
    } else {
        out << std::chrono::duration<float, std::milli>(dur).count() << "ms";
    }
    out.precision(old_prec);
}
