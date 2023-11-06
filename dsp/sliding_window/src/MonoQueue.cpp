//
// Created by wayne on 2022/10/24.
//

#include "MonoQueue.h"

MonoQueue::MonoQueue(int win) :
        win(win),
        cnt(0) {
}

void MonoQueue::push(double val) {
    int count = 0;
    while (!m_deque.empty() && m_deque.back().first < val) {
        count += m_deque.back().second + 1;
        m_deque.pop_back();
    }
    m_deque.emplace_back(val, count);
    cnt = std::min({cnt + 1, win + 1});
    if (cnt > win) {
        pop();
    }
}

double MonoQueue::max() {
    return m_deque.front().first;
}

void MonoQueue::pop() {
    if (m_deque.front().second > 0) {
        m_deque.front().second--;
        return;
    }
    m_deque.pop_front();
}
