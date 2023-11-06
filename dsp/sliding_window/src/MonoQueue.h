//
// Created by wayne on 2022/10/24.
//

#ifndef FFALCONXR_MONOQUEUE_H
#define FFALCONXR_MONOQUEUE_H

#include <deque>

class MonoQueue {
public:
    MonoQueue(int win);

    void push(double val);

    double max();

private:
    void pop();

    //pair.first: the actual value,
    //pair.second: how many elements were deleted between it and the one before it.
    std::deque<std::pair<double, int>> m_deque;
    int cnt;
    int win;
};


#endif //FFALCONXR_MONOQUEUE_H
