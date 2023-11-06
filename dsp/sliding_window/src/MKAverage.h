//
// Created by wayne on 2022/10/24.
//

#ifndef FFALCONXR_MKAVERAGE_H
#define FFALCONXR_MKAVERAGE_H


#include <vector>
#include <set>

class MKAverage {
public:
    MKAverage(int m, int k);
    void addElement(double num);
    double calculateMKAverage();
private:
    void remove(double n);
    void add(double n);

    int m = 0, k = 0, sz = 0, pos = 0;
    double sum = 0;
    std::vector<double> v;
    std::multiset<double> left, mid, right;
};


#endif //FFALCONXR_MKAVERAGE_H
