#pragma once

#include <atomic>

struct Parameters {
    // Thread-safe parameters
    std::atomic<float> masterGain{0.5f};
    std::atomic<bool> audioRunning{false};
};