//
// Created by wayne on 2022/10/24.
//

#include "WelfordStd.h"
#include <cmath>
#include <algorithm>


WelfordStd::WelfordStd(int win) : avg(0.0f), var(0.0f), std(0.0f), win(win), cnt(0) {
}

double WelfordStd::calcSlidingStd(double newData) {
    cnt = std::min({cnt + 1, 100 * win});
    deque.emplace_back(newData);
    if (cnt > win + 1) {
        // old_data, new_data_1, new_data_2, ..., new_data_N
        deque.pop_front();
    }
    double oldData = deque.at(0);
    double preAvg = avg;
    if (cnt <= win) {
        avg += (newData - preAvg) / (double) cnt;
        var += (newData - avg) * (newData - preAvg);
    } else {
        avg += (newData - oldData) / (double) win;
        var += (newData - oldData) * (newData - avg + oldData - preAvg);
    }
    if (cnt >= win) {
        std = sqrt(std::max({var, 0.0}) / (double) (win - 1));
    } else {
        std = cnt <= 1 ? 0.0 : sqrt(std::max({var, 0.0}) / (double) (cnt - 1));
    }
    return std;
}

double WelfordStd::getStd() {
    return std;
}

int WelfordStd::getCnt() {
    return cnt;
}
