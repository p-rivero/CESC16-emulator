#pragma once

#include <queue>
#include <mutex>

// This class receives a series of datapoints and computes the arithmetic mean of the last n points
template <class T>
class ArithmeticMean {
    using Size = unsigned long long int;
    std::queue<T> dataPoints;
    Size max_size;
    T current_sum = 0;
    std::mutex mtx;

public:
    // Constructor
    explicit ArithmeticMean(Size length) : max_size(length) {}
    
    // Add a new datapoint
    void addDataPoint(T x) {
        std::scoped_lock lock(mtx);
        if (dataPoints.size() == max_size) {
            current_sum -= dataPoints.front();
            dataPoints.pop();
        }
        dataPoints.push(x);
        current_sum += x;
    }
    // Compute the mean of the last n datapoints
    double getCurrentMean() {
        std::scoped_lock lock(mtx);
        if (dataPoints.empty()) return 0;   // Invalid
        return double(current_sum)/double(dataPoints.size());
    }
};
