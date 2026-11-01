#pragma once
#include <cstdint>
namespace superior {
// Pure conversion: callers supply ticks; no clock or GPU dependency.
double elapsed_ms(std::int64_t begin, std::int64_t end, std::int64_t frequency) noexcept;
class Clock {
public:
    Clock();
    std::int64_t now() const;
    std::int64_t frequency() const noexcept { return frequency_; }
    double milliseconds(std::int64_t begin, std::int64_t end) const noexcept;
private:
    std::int64_t frequency_;
};
class IntervalMeter {
public:
    explicit IntervalMeter(std::int64_t frequency);
    void sample(std::int64_t tick) noexcept;
    void reset() noexcept;
    double average_ms() const noexcept;
    double fps() const noexcept;
private:
    std::int64_t frequency_, last_ = 0, sum_ = 0;
    std::int64_t samples_[64]{};
    unsigned count_ = 0, next_ = 0;
    bool has_last_ = false;
};
} // namespace superior
