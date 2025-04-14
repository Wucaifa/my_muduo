#pragma once

#include <iostream>
#include <string>

class Timestamp {
public:
    Timestamp() : _microSecondsSinceEpoch(0) {}
    explicit Timestamp(int64_t microSecondsSinceEpoch) : _microSecondsSinceEpoch(microSecondsSinceEpoch) {}

    static Timestamp now();
    std::string toString() const;
private:
    static const int64_t kMicroSecondsPerSecond = 1000 * 1000;
    int64_t _microSecondsSinceEpoch;
};