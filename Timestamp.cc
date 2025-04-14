#include "Timestamp.h"

#include <time.h>

Timestamp Timestamp::now() {
    return Timestamp(time(NULL) * kMicroSecondsPerSecond);
}

std::string Timestamp::toString() const {
    time_t seconds = static_cast<time_t>(_microSecondsSinceEpoch / kMicroSecondsPerSecond);
    char buffer[32];
    struct tm tm_time;
    localtime_r(&seconds, &tm_time);
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
             tm_time.tm_year + 1900,
             tm_time.tm_mon + 1,
             tm_time.tm_mday,
             tm_time.tm_hour,
             tm_time.tm_min,
             tm_time.tm_sec);
    return buffer;
}

#include <iostream>

// int main() {
//     Timestamp ts = Timestamp::now();
//     std::cout << "Current timestamp: " << ts.toString() << std::endl;
//     return 0;
// }

