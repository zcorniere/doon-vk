#pragma once
#include <deque>
#include <functional>

struct DeleteionQueue {
    void push(std::function<void()> &&function) { deletor.push_back(function); }

    void flush()
    {
        for (auto it = deletor.rbegin(); it != deletor.rend(); it++) { (*it)(); }
        deletor.clear();
    }

    std::deque<std::function<void()>> deletor;
};
