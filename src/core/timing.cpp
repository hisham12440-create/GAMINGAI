#include "superior/core/timing.h"
#include "superior/core/error.h"
#include <windows.h>
namespace superior {
double elapsed_ms(std::int64_t begin, std::int64_t end, std::int64_t frequency) noexcept {
    if (frequency <= 0 || end < begin) return 0.0;
    const auto delta = static_cast<std::uint64_t>(end) - static_cast<std::uint64_t>(begin);
    return static_cast<double>(delta / frequency) * 1000.0 +
        static_cast<double>(delta % frequency) * 1000.0 / static_cast<double>(frequency);
}
Clock::Clock() {
    LARGE_INTEGER f; check_win(QueryPerformanceFrequency(&f), "QueryPerformanceFrequency");
    frequency_ = f.QuadPart;
    if (frequency_ <= 0) throw Error("Invalid performance-counter frequency");
}
std::int64_t Clock::now() const {
    LARGE_INTEGER t; check_win(QueryPerformanceCounter(&t), "QueryPerformanceCounter"); return t.QuadPart;
}
double Clock::milliseconds(std::int64_t begin, std::int64_t end) const noexcept { return elapsed_ms(begin,end,frequency_); }
IntervalMeter::IntervalMeter(std::int64_t frequency) : frequency_(frequency) {}
void IntervalMeter::sample(std::int64_t tick) noexcept {
    if (has_last_ && tick >= last_) {
        const auto delta = tick - last_;
        if (count_ == 64) sum_ -= samples_[next_]; else ++count_;
        samples_[next_] = delta; sum_ += delta; next_ = (next_ + 1) % 64;
    }
    last_ = tick; has_last_ = true;
}
void IntervalMeter::reset() noexcept { last_ = sum_ = 0; count_ = next_ = 0; has_last_ = false; }
double IntervalMeter::average_ms() const noexcept { return count_ ? elapsed_ms(0,sum_,frequency_) / count_ : 0; }
double IntervalMeter::fps() const noexcept { const auto ms = average_ms(); return ms > 0 ? 1000.0 / ms : 0; }
} // namespace superior
