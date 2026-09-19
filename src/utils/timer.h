#pragma once

#include <chrono>
#include <string>
#include <iostream>

namespace itch {

class ScopedTimer {
public:
    explicit ScopedTimer(const std::string& label) 
        : label_(label), start_(std::chrono::high_resolution_clock::now()) {}

    ~ScopedTimer() {
        std::cout << label_ << " took " << elapsed_ms() << " ms\n";
    }

    double elapsed_ms() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(now - start_).count();
    }

    double elapsed_us() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::micro>(now - start_).count();
    }

private:
    std::string label_;
    std::chrono::high_resolution_clock::time_point start_;
};

} // namespace itch
