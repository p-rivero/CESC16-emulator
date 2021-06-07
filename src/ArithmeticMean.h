#pragma once

#include <queue>

// This class receives a series of datapoints and computes the arithmetic mean of the last n points
template <class T>
class ArithmeticMean {
    using Size = unsigned long long int;
private:
    std::queue<T> dataPoints;
    Size max_size;
    T current_sum = 0;

public:
    ArithmeticMean(Size length) {
        max_size = length;
    }
    void addDataPoint(T x) {
        if (dataPoints.size() == max_size) {
            current_sum -= dataPoints.front();
            dataPoints.pop();
        }
        dataPoints.push(x);
        current_sum += x;
    }
    double getCurrentMean() {
        if (dataPoints.empty()) return 0;   // Invalid
        return double(current_sum)/double(dataPoints.size());
    }
};
