//
// Created by wayne on 2022/10/24.
//

#ifndef FFALCONXR_WELFORDSTD_H
#define FFALCONXR_WELFORDSTD_H


#include <deque>

class WelfordStd {
public:
    WelfordStd(int win);
    double calcSlidingStd(double newData);
    double getStd();
    int getCnt();

    double avg;
    double var;
private:
    int win;
    int cnt;
    double std;
    std::deque<double> deque;
};


#endif //FFALCONXR_WELFORDSTD_H
