//
// Created by wayne on 2022/10/24.
//

#include "MKAverage.h"

MKAverage::MKAverage(int m, int k) : m(m), k(k), sz(m - 2 * k) {
    v = std::vector<double>(m);
}

void MKAverage::addElement(double num) {
    if (pos >= m) {
        remove(v[pos % m]);
    }
    add(num);
    v[pos++ % m] = num;
}

double MKAverage::calculateMKAverage() {
    if (pos < m)
        return 0.0f;
    return sum / (float) sz;
}

void MKAverage::remove(double n) {
    if (n <= *left.rbegin())
        left.erase(left.find(n));
    else if (n <= *mid.rbegin()) {
        auto it = mid.find(n);
        sum -= *it;
        mid.erase(it);
    } else
        right.erase(right.find(n));
    if (left.size() < k) {
        left.insert(*begin(mid));
        sum -= *begin(mid);
        mid.erase(begin(mid));
    }
    if (mid.size() < sz) {
        mid.insert(*begin(right));
        sum += *begin(right);
        right.erase(begin(right));
    }
}

void MKAverage::add(double n) {
    left.insert(n);
    if (left.size() > k) {
        auto it = prev(end(left));
        mid.insert(*it);
        sum += *it;
        left.erase(it);
    }
    if (mid.size() > sz) {
        auto it = prev(end(mid));
        sum -= *it;
        right.insert(*it);
        mid.erase(it);
    }
}
