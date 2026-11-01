#include "superior/core/timing.h"
#include <gtest/gtest.h>
#include <limits>
using namespace superior;
TEST(Timing, ExactConversions) {
    EXPECT_DOUBLE_EQ(elapsed_ms(0,1000,10000000),.1);
    EXPECT_DOUBLE_EQ(elapsed_ms(100,100100,10000000),10);
    EXPECT_DOUBLE_EQ(elapsed_ms(0,0,10000000),0);
}
TEST(Timing, InvalidFrequencyAndBackwardClockDoNotProduceNegativeTime) {
    EXPECT_DOUBLE_EQ(elapsed_ms(0,10,0),0);EXPECT_DOUBLE_EQ(elapsed_ms(0,10,-1),0);EXPECT_DOUBLE_EQ(elapsed_ms(20,10,1000),0);
}
TEST(Timing, LargeCounterDoesNotOverflowMilliseconds) {
    const auto max=(std::numeric_limits<std::int64_t>::max)();
    EXPECT_GT(elapsed_ms(0,max,10000000),0);EXPECT_TRUE(std::isfinite(elapsed_ms(0,max,10000000)));
}
TEST(Timing, MeterMeasuresRealIntervalsAndIgnoresBackwardSamples) {
    IntervalMeter m(1000);EXPECT_DOUBLE_EQ(m.fps(),0);m.sample(0);m.sample(10);m.sample(20);
    EXPECT_DOUBLE_EQ(m.average_ms(),10);EXPECT_DOUBLE_EQ(m.fps(),100);m.sample(5);EXPECT_DOUBLE_EQ(m.average_ms(),10);
    m.reset();EXPECT_DOUBLE_EQ(m.average_ms(),0);
}
TEST(Timing, MeterHasBoundedWindow) {
    IntervalMeter m(1000);for(int i=0;i<=128;++i)m.sample(i*10);
    EXPECT_DOUBLE_EQ(m.average_ms(),10);EXPECT_DOUBLE_EQ(m.fps(),100);
}
TEST(Timing, WindowsClockIsMonotonicAndPositiveFrequency) {
    Clock c;const auto a=c.now();const auto b=c.now();EXPECT_GE(b,a);EXPECT_GE(c.milliseconds(a,b),0);
}

TEST(Timing, RollingWindowUsesExactOldestSample) {
    IntervalMeter m(1000); m.sample(0); m.sample(100);
    for(int i=2;i<=65;++i)m.sample(100+(i-1)*10);
    EXPECT_DOUBLE_EQ(m.average_ms(),10); EXPECT_DOUBLE_EQ(m.fps(),100);
}
