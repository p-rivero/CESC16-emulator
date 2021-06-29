#pragma once

#include <queue>
#include <mutex>

// This class receives a series of datapoints and computes the arithmetic mean of the last n points
template <class T>
class ArithmeticMean {
    using Size = unsigned long long int;
private:
    std::queue<T> dataPoints;
    Size max_size;
    T current_sum;
    std::mutex mtx;

public:
    // Constructor
    ArithmeticMean(Size length) {
        max_size = length;
        current_sum = 0;
    }
    // Copy constructor
    ArithmeticMean(const ArithmeticMean<T>& a) {
        max_size = a.max_size;
        dataPoints = a.dataPoints;
        current_sum = a.current_sum;
    }
    // Add a new datapoint
    void addDataPoint(T x) {
        std::lock_guard<std::mutex> lock(mtx);
        if (dataPoints.size() == max_size) {
            current_sum -= dataPoints.front();
            dataPoints.pop();
        }
        dataPoints.push(x);
        current_sum += x;
    }
    // Compute the mean of the last n datapoints
    double getCurrentMean() {
        std::lock_guard<std::mutex> lock(mtx);
        if (dataPoints.empty()) return 0;   // Invalid
        return double(current_sum)/double(dataPoints.size());
    }
};
